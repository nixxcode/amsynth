/*
 *  main.cpp
 *
 *  Copyright (c) 2001 Nick Dowell
 *
 *  This file is part of amsynth.
 *
 *  amsynth is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  amsynth is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with amsynth.  If not, see <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H

#include "config.h"

#endif

#include "main.h"

#include "AudioOutput.h"
#include "JackOutput.h"
#include "core/Configuration.h"
#include "core/filesystem.h"
#include "core/gettext.h"
#include "core/midi.h"
#include "core/synth/LowPassFilter.h"
#include "core/synth/MidiController.h"
#include "core/synth/Synthesizer.h"
#include "core/synth/VoiceAllocationUnit.h"
#include "drivers/ALSAMidiDriver.h"
#include "drivers/OSSMidiDriver.h"
#include "lash.h"

#ifdef WITH_GUI

#include "core/gui/ControlPanel.h"
#include "core/gui/MainComponent.h"

#endif

#ifdef WITH_NSM

#include "nsm/NsmClient.h"
#include "nsm/NsmHandler.h"

#endif

#include <iostream>
#include <fcntl.h>
#include <fstream>
#include <getopt.h>
#include <unistd.h>
#include <string>
#include <climits>
#include <csignal>
#include <cstring>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <algorithm>

#define _(string) gettext (string)


Configuration & config = Configuration::get();

#ifdef ENABLE_REALTIME
static void sched_realtime()
{
#ifdef linux
	struct sched_param sched = {0};
	sched.sched_priority = 50;
	if (sched_setscheduler(0, SCHED_FIFO, &sched) == -1) {
		perror("sched_setscheduler");
		config.realtime = 0;
	} else {
		config.realtime = 1;
	}
#else
#warning "sched_realtime not implemented for this OS"
#endif
}
#endif

////////////////////////////////////////////////////////////////////////////////

void ptest ();

static MidiDriver *midiDriver;
Synthesizer *s_synthesizer;
static unsigned char *midiBuffer;
static const size_t midiBufferSize = 4096;
static int gui_midi_pipe[2];

////////////////////////////////////////////////////////////////////////////////

static GenericOutput * open_audio()
{	
	if (config.audio_driver == "jack" ||
		config.audio_driver == "auto")
	{
		JackOutput *jack = new JackOutput();
		if (jack->init() == 0)
		{
			return jack;
		}
		else
		{
			std::string jack_error = jack->get_error_msg();
			delete jack;
			
			// we were asked specifically for jack, so don't use anything else
			if (config.audio_driver == "jack") {
				std::cerr << _("JACK init failed: ") << jack_error << "\n";
				return new NullAudioOutput;
			}
		}
	}
	
	return new AudioOutput();
}

static MidiDriver *opened_midi_driver(MidiDriver *driver)
{
	if (driver && driver->open() != 0) {
		delete driver;
		return nullptr;
	}
	return driver;
}

static void open_midi()
{
	const char *alsa_client_name = config.jack_client_name.empty() ? PACKAGE_NAME : config.jack_client_name.c_str();
	
	if (config.midi_driver == "alsa" || config.midi_driver == "ALSA") {
		if (!(midiDriver = opened_midi_driver(CreateAlsaMidiDriver(alsa_client_name)))) {
			std::cerr << _("error: could not open ALSA MIDI interface") << std::endl;
		}
		return;
	}

	if (config.midi_driver == "oss" || config.midi_driver == "OSS") {
		if (!(midiDriver = opened_midi_driver(CreateOSSMidiDriver()))) {
			std::cerr << _("error: could not open OSS MIDI interface") << std::endl;
		}
		return;
	}

	if (config.midi_driver == "auto") {
		midiDriver = opened_midi_driver(CreateAlsaMidiDriver(alsa_client_name)) ?:
					 opened_midi_driver(CreateOSSMidiDriver());
		if (config.current_midi_driver.empty()) {
			std::cerr << _("error: could not open any MIDI interface") << std::endl;
		}
		return;
	}
}

static void fatal_error(const std::string & msg) __attribute__ ((noreturn));

static void fatal_error(const std::string & msg)
{
	std::cerr << msg << "\n";
	exit(1);
}

////////////////////////////////////////////////////////////////////////////////

#ifdef WITH_GUI

class MainWindow : public juce::DocumentWindow
{
public:
	MainWindow()
	: DocumentWindow(PACKAGE_NAME, juce::Colours::lightgrey, juce::DocumentWindow::closeButton | juce::DocumentWindow::minimiseButton)
	{
		auto mainComponent = new MainComponent(s_synthesizer->getPresetController(), s_synthesizer->getMidiController());
		for (const auto &it : s_synthesizer->getProperties()) {
			mainComponent->propertyChanged(it.first.c_str(), it.second.c_str());
		}
		mainComponent->sendProperty = [] (const char *name, const char *value) {
			s_synthesizer->setProperty(name, value);
			if (name == std::string(PROP_NAME(max_polyphony)))
				Configuration::get().polyphony = std::stoi(value);
			if (name == std::string(PROP_NAME(midi_channel)))
				Configuration::get().midi_channel = std::stoi(value);
			if (name == std::string(PROP_NAME(pitch_bend_range)))
				Configuration::get().pitch_bend_range = std::stoi(value);
			Configuration::get().save();
		};
		setContentOwned(mainComponent, true);
		centreWithSize(getWidth(), getHeight());
		setResizable(false, false);
		// Would like to use native title bar but JUCE's Linux implementation is buggy;
		// - window is always resizable
		// - window has incorrect initial size
		// setUsingNativeTitleBar(true);
	}

	void closeButtonPressed() override
	{
		juce::JUCEApplication::getInstance()->systemRequestedQuit();
	}

private:
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainWindow)
};

class Application : public juce::JUCEApplication
{
public:
	static juce::JUCEApplicationBase * create() { return new Application; }

	void initialise(const juce::String &commandLine) override
	{
		(new MainWindow())->setVisible(true);

		juce::String errorMessage;
		if (config.current_audio_driver.empty()) {
			errorMessage = GETTEXT("Could not initialise the configured audio device.\n\n"
								   "Please edit ~/.config/amsynth/config and restart");
		} else if (config.current_midi_driver.empty()) {
			errorMessage = GETTEXT("Could not initialise the configured MIDI device.\n\n"
								   "Please edit ~/.config/amsynth/config and restart");
		}

		if (errorMessage.isNotEmpty()) {
			juce::AlertWindow::showAsync(
					juce::MessageBoxOptions()
							.withTitle(GETTEXT("Configuration error"))
							.withMessage(errorMessage)
							.withButton(GETTEXT("OK")),
					[this](int) {
						(void)!system(("xdg-open " + filesystem::get().config).c_str());
						quit();
					});
		}

		struct LashTimer : juce::Timer
		{
			void timerCallback() override { amsynth_lash_poll_events(); }
		};

		(new LashTimer())->startTimer(250);
	}

	void shutdown() override {}
	const juce::String getApplicationName() override { return PACKAGE_NAME; }
	const juce::String getApplicationVersion() override { return PACKAGE_VERSION; }
};

#endif // WITH_GUI

////////////////////////////////////////////////////////////////////////////////

static int signal_received = 0;

static void signal_handler(int signal)
{
	signal_received = signal;
}

int main( int argc, char *argv[] )
{
	srand((unsigned) time(nullptr));

#ifdef ENABLE_REALTIME
	sched_realtime();

	// need to drop our suid-root permissions :-
	// GTK will not work SUID for security reasons..
	setreuid(getuid(), getuid());
	setregid(getgid(), getgid());
#endif

#if ENABLE_NLS
	setlocale(LC_ALL, "");
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALEDIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
#else
#warning text will not be localized because ENABLE_NLS not set
#endif

	int initial_preset_no = 0;

	// needs to be called before our own command line parsing code
	amsynth_lash_process_args(&argc, &argv);
	
	bool no_gui = (getenv("AMSYNTH_NO_GUI") != nullptr);
	int gui_scale_factor = 0;

	static struct option longopts[] = {
		{ "jack_autoconnect", optional_argument, nullptr, 0 },
		{ "force-device-scale-factor", required_argument, nullptr, 0 },
		{ nullptr }
	};
	
	int opt, longindex = -1;
	while ((opt = getopt_long(argc, argv, "vhsdzxm:c:a:r:p:b:U:P:n:t:", longopts, &longindex)) != -1) {
		switch (opt) {
			case 'v':
				std::cout << PACKAGE_STRING << std::endl;
				return 0;
			case 'h':
				std::cout << _("usage: ") << PACKAGE << _(" [options]") << "\n"
					<< "\n"
					<< _("Any options given here override those in the config file ($HOME/.amSynthrc)") << "\n"
					<< "\n"
					<< _("OPTIONS:") << "\n"
					<< "\n"
					<< _("	-h          show this usage message") << "\n"
					<< _("	-v          show version information") << "\n"
					<< _("	-x          run in headless mode (without GUI)") << "\n"
					<< "\n"
					<< _("	-b <file>   use <file> as the bank to store presets") << "\n"
					<< _("	-P <int>    set preset number to use") << "\n"
					<< _("	-t <file>   use <file> as a tuning file") << "\n"
					<< "\n"
					<< _("	-a <string> set the sound output driver to use [alsa/oss/auto(default)]") << "\n"
					<< _("	-r <int>    set the sampling rate to use") << "\n"
					<< _("	-m <string> set the MIDI driver to use [alsa/oss/auto(default)]") << "\n"
					<< _("	-c <int>    set the MIDI channel to respond to (default=all)") << "\n"
					<< _("	-p <int>    set the polyphony (maximum active voices)") << "\n"
					<< "\n"
					<< _("	-n <name>   specify the JACK client name to use") << "\n"
					<< _("	--jack_autoconnect[=<true|false>]") << "\n"
					<< _("	            automatically connect jack audio ports to hardware I/O ports. (Default: true)") << "\n"
					<< "\n"
					<< _("	--force-device-scale-factor <scale>") << "\n"
					<< _("	            override the default scaling factor for the control panel") << "\n"
					<< std::endl;
				return 0;
			case 'z':
				ptest();
				return 0;
			case 'P':
				initial_preset_no = atoi(optarg);
				break;
			case 'x':
				no_gui = true;
				break;
			case 'm':
				config.midi_driver = optarg;
				break;
			case 'b':
				config.current_bank_file = optarg;
				break;
			case 'c':
				config.midi_channel = atoi( optarg );
				break;
			case 'a':
				config.audio_driver = optarg;
				break;
			case 'r':
				config.sample_rate = atoi( optarg );
				break;
			case 'p':
				config.polyphony = atoi( optarg );
				break;
			case 't':
				config.current_tuning_file = optarg;
				break;
			case 'n':
				config.jack_client_name_preference = optarg;
				break;
			case 0:
				if (strcmp(longopts[longindex].name, "jack_autoconnect") == 0) {
					config.jack_autoconnect = !optarg || (strcmp(optarg, "true") == 0);
				}
				if (strcmp(longopts[longindex].name, "force-device-scale-factor") == 0) {
					gui_scale_factor = atoi(optarg);
				}
				break;
			default:
				break;
		}
	}

	std::string amsynth_bank_file = config.current_bank_file;
	// string amsynth_tuning_file = config.current_tuning_file;

	GenericOutput *out = open_audio();
	if (!out)
		fatal_error(std::string(_("Fatal Error: open_audio() returned NULL.\n")) +
					"config.audio_driver = " + config.audio_driver);

	// errors now detected & reported in the GUI
	out->init();

	Preset::setLockedParameterNames(config.locked_parameters);

	s_synthesizer = new Synthesizer();
	s_synthesizer->setSampleRate(config.sample_rate);
	s_synthesizer->setMaxNumVoices(config.polyphony);
	s_synthesizer->setMidiChannel(config.midi_channel);
	s_synthesizer->setPitchBendRangeSemitones(config.pitch_bend_range);
	if (config.current_tuning_file != "default") {
		s_synthesizer->loadTuningScale(config.current_tuning_file.c_str());
	}
	s_synthesizer->loadBank(config.current_bank_file.c_str());
	
	amsynth_load_bank(config.current_bank_file.c_str());
	amsynth_set_preset_number(initial_preset_no);

#ifdef WITH_NSM
	NsmClient nsmClient(argv[0]);
	NsmHandler nsmHandler(&nsmClient);
	nsmClient.Init(PACKAGE_NAME);
#endif

	if (config.current_tuning_file != "default")
		amsynth_load_tuning_file(config.current_tuning_file.c_str());
	
	// errors now detected & reported in the GUI
	out->Start();
	
	open_midi();
	midiBuffer = (unsigned char *)malloc(midiBufferSize);

	// prevent lash from spawning a new jack server
	setenv("JACK_NO_START_SERVER", "1", 0);
	
	if (config.alsa_seq_client_id != 0 || !config.jack_client_name.empty())
		// LASH only works with ALSA MIDI and JACK
		amsynth_lash_init();

	if (config.alsa_seq_client_id != 0) // alsa midi is active
		amsynth_lash_set_alsa_client_id((unsigned char) config.alsa_seq_client_id);

	if (!config.jack_client_name.empty())
		amsynth_lash_set_jack_client_name(config.jack_client_name.c_str());

	// give audio/midi threads time to start up first..
	// if (jack) sleep (1);

	if (pipe(gui_midi_pipe) != -1) {
		fcntl(gui_midi_pipe[0], F_SETFL, O_NONBLOCK);
	}

#ifdef WITH_GUI
	if (!no_gui) {
		if (gui_scale_factor)
			juce::Desktop::getInstance().setGlobalScaleFactor((float)gui_scale_factor);
		juce::JUCEApplicationBase::createInstance = &Application::create;
		juce::JUCEApplicationBase::main(JUCE_MAIN_FUNCTION_ARGS);
	} else {
#endif
		printf(_("amsynth running in headless mode, press ctrl-c to exit\n"));
		signal(SIGINT, &signal_handler);
		while (!signal_received)
			sleep(2); // delivery of a signal will wake us early
		printf("\n");
		printf(_("shutting down...\n"));
#ifdef WITH_GUI
	}
#endif

	out->Stop ();

	if (config.xruns) std::cerr << config.xruns << _(" audio buffer underruns occurred\n");

	delete out;
	return 0;
}

void amsynth_midi_input(unsigned char status, unsigned char data1, unsigned char data2)
{
	unsigned char buffer[3] = { status, data1, data2 };
	if (config.midi_channel > 1) {
		buffer[0] |= ((config.midi_channel - 1) & 0x0f);
	}
	if (write(gui_midi_pipe[1], buffer, sizeof(buffer)) == -1) {
		perror("write(gui_midi_pipe)");
	}
}

static bool compare(const amsynth_midi_event_t &first, const amsynth_midi_event_t &second) {
	return (first.offset_frames < second.offset_frames);
}

void amsynth_audio_callback(
		float *buffer_l, float *buffer_r, unsigned num_frames, int stride,
		const std::vector<amsynth_midi_event_t> &midi_in,
		std::vector<amsynth_midi_cc_t> &midi_out)
{
	std::vector<amsynth_midi_event_t> midi_in_merged = midi_in;

	if (midiBuffer) {
		unsigned char *buffer = midiBuffer;
		ssize_t bufferSize = midiBufferSize;
		memset(buffer, 0, bufferSize);

		if (gui_midi_pipe[0]) {
			ssize_t bytes_read = read(gui_midi_pipe[0], buffer, bufferSize);
			if (bytes_read > 0) {
				amsynth_midi_event_t event = {0};
				event.offset_frames = num_frames - 1;
				event.length = (unsigned int) bytes_read;
				event.buffer = buffer;
				midi_in_merged.push_back(event);
				buffer += bytes_read;
				bufferSize -= bytes_read;
			}
		}

		if (midiDriver) {
			int bytes_read = midiDriver->read(buffer, (unsigned) bufferSize);
			if (bytes_read > 0) {
				amsynth_midi_event_t event = {0};
				event.offset_frames = num_frames - 1;
				event.length = bytes_read;
				event.buffer = buffer;
				midi_in_merged.push_back(event);
			}
		}
	}

	std::sort(midi_in_merged.begin(), midi_in_merged.end(), compare);

	if (s_synthesizer) {
		s_synthesizer->process(num_frames, midi_in_merged, midi_out, buffer_l, buffer_r, stride);
	}

	if (midiDriver && !midi_out.empty()) {
		std::vector<amsynth_midi_cc_t>::const_iterator out_it;
		for (out_it = midi_out.begin(); out_it != midi_out.end(); ++out_it) {
			midiDriver->write_cc(out_it->channel, out_it->cc, out_it->value);
		}
	}
}

void
amsynth_save_bank(const char *filename)
{
	s_synthesizer->saveBank(filename);
}

void
amsynth_load_bank(const char *filename)
{
	s_synthesizer->loadBank(filename);
}

int
amsynth_load_tuning_file(const char *filename)
{
	int result = s_synthesizer->loadTuningScale(filename);
	if (result != 0) {
		std::cerr << _("error: could not load tuning file ") << filename << std::endl;
	}
	return result;
}

int
amsynth_get_preset_number()
{
	return s_synthesizer->getPresetNumber();
}

void
amsynth_set_preset_number(int preset_no)
{
	s_synthesizer->setPresetNumber(preset_no);
}

///////////////////////////////////////////////////////////////////////////////

void ptest ()
{
	//
	// test parameters
	// 
	const int kTestBufSize = 64;
	const int kTestSampleRate = 44100;
	const int kTimeSeconds = 60;
	const int kNumVoices = 10;

	float *buffer = new float [kTestBufSize];

	VoiceAllocationUnit *voiceAllocationUnit = new VoiceAllocationUnit;
	voiceAllocationUnit->SetSampleRate (kTestSampleRate);
	
	// trigger off some notes for amsynth to render.
	for (int v=0; v<kNumVoices; v++) {
		voiceAllocationUnit->HandleMidiNoteOn(60 + v, 1.0f);
	}
	
	struct rusage usage_before; 
	getrusage (RUSAGE_SELF, &usage_before);
	
	long total_samples = kTestSampleRate * kTimeSeconds;
	long total_calls = total_samples / kTestBufSize;
	unsigned remain_samples = total_samples % kTestBufSize;
	for (int i=0; i<total_calls; i++) {
		voiceAllocationUnit->Process (buffer, buffer, kTestBufSize);
	}
	voiceAllocationUnit->Process (buffer, buffer, remain_samples);

	struct rusage usage_after; 
	getrusage (RUSAGE_SELF, &usage_after);
	
	unsigned long user_usec = (usage_after.ru_utime.tv_sec*1000000 + usage_after.ru_utime.tv_usec)
							- (usage_before.ru_utime.tv_sec*1000000 + usage_before.ru_utime.tv_usec);
	
	unsigned long syst_usec = (usage_after.ru_stime.tv_sec*1000000 + usage_after.ru_stime.tv_usec)
							- (usage_before.ru_stime.tv_sec*1000000 + usage_before.ru_stime.tv_usec);

	unsigned long usec_audio = kTimeSeconds * kNumVoices * 1000000;
	unsigned long usec_cpu = user_usec + syst_usec;
	
	fprintf (stderr, _("user time: %f		system time: %f\n"), user_usec/1000000.f, syst_usec/1000000.f);
	fprintf (stderr, _("performance index: %f\n"), (float) usec_audio / (float) usec_cpu);
	
	delete [] buffer;
	delete voiceAllocationUnit;
}


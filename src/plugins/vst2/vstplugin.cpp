/*
 *  vstplugin.cpp
 *
 *  Copyright (c) 2014 Nick Dowell
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

#include "ardour/vestige.h"
#include "core/midi.h"
#include "core/gui/MainComponent.h"
#include "core/gui/JuceIntegration.h"
#include "core/synth/PresetController.h"
#include "core/synth/Synthesizer.h"

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <memory>

// from http://www.asseca.org/vst-24-specs/index.html
#define effGetParamLabel        6
#define effGetParamDisplay      7
#define effGetChunk             23
#define effSetChunk             24
#define effCanBeAutomated       26
#define effGetOutputProperties  34
#define effGetTailSize          52
#define effGetMidiKeyName       66
#define effBeginLoadBank        75
#define effFlagsProgramChunks   (1 << 5)

#define MIDI_BUFFER_SIZE 4096

static char hostProductString[64] = "";

#if defined(DEBUG) && DEBUG
static FILE *logFile;
#endif

struct Plugin final : public Parameter::Observer
{
	Plugin(AEffect *effect, audioMasterCallback master)
	: effect(effect)
	, audioMasterValues(kAmsynthParameterCount)
	{
		audioMaster = master;
		synthesizer = new Synthesizer;
		midiBuffer = (unsigned char *)malloc(MIDI_BUFFER_SIZE);
		for (int i = 0; i < kAmsynthParameterCount; i++)
			audioMasterValues[i] = synthesizer->_presetController->getCurrentPreset().getParameter(i).getNormalisedValue();
		synthesizer->_presetController->getCurrentPreset().addObserver(this);
	}

	~Plugin()
	{ 
		delete synthesizer;
		free(midiBuffer);
	}

	void parameterDidChange(const Parameter &parameter) override
	{
		if (audioMaster &&
			audioMasterValues.at(parameter.getId()) != parameter.getNormalisedValue())
			audioMaster(effect, audioMasterAutomate, parameter.getId(), 0, nullptr,
						audioMasterValues[parameter.getId()] = parameter.getNormalisedValue());
	}

	void parameterBeginEdit(const Parameter &parameter) override
	{
		if (audioMaster)
			audioMaster(effect, audioMasterBeginEdit, parameter.getId(), 0, nullptr, 0);
	}

	void parameterEndEdit(const Parameter &parameter) override
	{
		if (audioMaster)
			audioMaster(effect, audioMasterEndEdit, parameter.getId(), 0, nullptr, 0);
	}

	AEffect *effect;
	audioMasterCallback audioMaster;
	std::vector<float> audioMasterValues;
	Synthesizer *synthesizer;
	unsigned char *midiBuffer;
	std::vector<amsynth_midi_event_t> midiEvents;
	std::string chunk;
	JuceIntegration juceIntegration;
	std::unique_ptr<MainComponent> gui;
};

static intptr_t dispatcher(AEffect *effect, int opcode, int index, intptr_t val, void *ptr, float f)
{
	Plugin *plugin = (Plugin *)effect->ptr3;

	switch (opcode) {
		case effOpen:
			return 0;

		case effClose:
			delete plugin;
			memset(effect, 0, sizeof(AEffect));
			free(effect);
			return 0;

		case effSetProgram:
		case effGetProgram:
		case effGetProgramName:
			return 0;

		case effGetParamLabel:
			return 0;

		case effGetParamDisplay:
			parameter_get_display(index, plugin->synthesizer->_presetController->getCurrentPreset().getParameter(index).getValue(), (char *)ptr, 32);
			return 0;

		case effGetParamName:
			plugin->synthesizer->getParameterName((Param)index, (char *)ptr, 32);
			return 0;

		case effSetSampleRate:
			plugin->synthesizer->setSampleRate(f);
			return 0;

		case effSetBlockSize:
		case effMainsChanged:
			return 0;

		case effEditGetRect: {
			if (!plugin->gui) {
				plugin->gui = std::make_unique<MainComponent>(plugin->synthesizer->_presetController);
			}
			static short rect[4] = {};
			// There doesn't seem to be a way to determine which screen the host wants to open a plugin on and
			// apply the correct scale factor in a multiscreen setup.
			auto scaleFactor = juce::Desktop::getInstance().getGlobalScaleFactor();
			auto bounds = plugin->gui->getScreenBounds();
			rect[2] = bounds.getHeight() * scaleFactor;
			rect[3] = bounds.getWidth() * scaleFactor;
			*(short**)ptr = rect;
			return 1;
		}
		case effEditOpen: {
			if (!plugin->gui) {
				plugin->gui = std::make_unique<MainComponent>(plugin->synthesizer->_presetController);
			}
			for (const auto &it : plugin->synthesizer->getProperties()) {
				plugin->gui->propertyChanged(it.first.c_str(), it.second.c_str());
			}
			plugin->gui->sendProperty = [plugin] (const char *name, const char *value) {
				plugin->synthesizer->setProperty(name, value);
			};
			assert(plugin->gui->isOpaque()); // CreateWindowEx will fail if not opaque
			plugin->gui->addToDesktop(juce::ComponentPeer::windowIgnoresKeyPresses, ptr);
			plugin->gui->setVisible(true);
			return 1;
		}
		case effEditClose: {
			plugin->gui->removeFromDesktop();
			plugin->gui.reset();
			return 0;
		}
		case effEditIdle: {
			plugin->juceIntegration.idle();
			return 0;
		}

		case effGetChunk:
			plugin->chunk = plugin->synthesizer->getState();
			*(const char **)ptr = plugin->chunk.data();
			return plugin->chunk.size();

		case effSetChunk:
			plugin->synthesizer->setState(std::string((const char *)ptr, val));
			return 0;

		case effProcessEvents: {
			VstEvents *events = (VstEvents *)ptr;

			plugin->midiEvents.clear();
			memset(plugin->midiBuffer, 0, MIDI_BUFFER_SIZE);
			size_t bytesCopied = 0;
			
			for (int32_t i=0; i<events->numEvents; i++) {
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-align"
#endif
				auto event = (const VstMidiEvent *)events->events[i];
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
				if (event->type != kVstMidiType) {
					continue;
				}

				unsigned char *msgData = (unsigned char *)event->midiData;

				int msgLength = 0;
				switch (msgData[0] & 0xF0) {
				case MIDI_STATUS_NOTE_OFF:
				case MIDI_STATUS_NOTE_ON:
				case MIDI_STATUS_NOTE_PRESSURE:
				case MIDI_STATUS_CONTROLLER:
				case MIDI_STATUS_PITCH_WHEEL:
					msgLength = 3;
					break;
				case MIDI_STATUS_PROGRAM_CHANGE:
				case MIDI_STATUS_CHANNEL_PRESSURE:
					msgLength = 2;
					break;
				case 0xF0: // System message
					continue; // Ignore
				default:
					fprintf(stderr, "amsynth: bad status byte: %02x\n", msgData[0]);
					continue; // Ignore
				}

				if (bytesCopied + msgLength > MIDI_BUFFER_SIZE) {
					fprintf(stderr, "amsynth: midi buffer overflow\n");
					break;
				}

				amsynth_midi_event_t midi_event;
				midi_event.offset_frames = event->deltaSamples;
				midi_event.length = msgLength;
				midi_event.buffer = (unsigned char *)memcpy(plugin->midiBuffer + bytesCopied, msgData, msgLength);
				plugin->midiEvents.push_back(midi_event);
				bytesCopied += msgLength;
			}
			
			return 1;
		}

		case effCanBeAutomated:
		case effGetOutputProperties:
			return 0;

		case effGetPlugCategory:
			return kPlugCategSynth;
		case effGetEffectName:
			strcpy((char *)ptr, "amsynth");
			return 1;
		case effGetVendorString:
			strcpy((char *)ptr, "Nick Dowell");
			return 1;
		case effGetProductString:
			strcpy((char *)ptr, "amsynth");
			return 1;
		case effGetVendorVersion:
			return 0;

		case effCanDo:
			if (strcmp("receiveVstMidiEvent", (char *)ptr) == 0 ||
				false) return 1;
			if (strcmp("midiKeyBasedInstrumentControl", (char *)ptr) == 0 ||
				strcmp("midiSingleNoteTuningChange", (char *)ptr) == 0 ||
				strcmp("receiveVstSysexEvent", (char *)ptr) == 0 ||
				strcmp("sendVstMidiEvent", (char *)ptr) == 0 ||
				false) return 0;
#if defined(DEBUG) && DEBUG
			fprintf(logFile, "[amsynth_vst] unhandled canDo: %s\n", (char *)ptr);
			fflush(logFile);
#endif
			return 0;

		case effGetTailSize:
		case effIdle:
		case effGetParameterProperties:
			return 0;

		case effGetVstVersion:
			return 2400;

		case effGetMidiKeyName:
		case effStartProcess:
		case effStopProcess:
		case effBeginSetProgram:
		case effEndSetProgram:
		case effBeginLoadBank:
			return 0;

		default:
#if defined(DEBUG) && DEBUG
			fprintf(logFile, "[amsynth_vst] unhandled VST opcode: %d\n", opcode);
			fflush(logFile);
#endif
			return 0;
	}
}

static void process(AEffect *effect, float **inputs, float **outputs, int numSampleFrames)
{
	(void)inputs;
	Plugin *plugin = (Plugin *)effect->ptr3;
	std::vector<amsynth_midi_cc_t> midi_out;
	plugin->synthesizer->process(numSampleFrames, plugin->midiEvents, midi_out, outputs[0], outputs[1]);
	plugin->midiEvents.clear();
}

static void processReplacing(AEffect *effect, float **inputs, float **outputs, int numSampleFrames)
{
	(void)inputs;
	Plugin *plugin = (Plugin *)effect->ptr3;
	std::vector<amsynth_midi_cc_t> midi_out;
	plugin->synthesizer->process(numSampleFrames, plugin->midiEvents, midi_out, outputs[0], outputs[1]);
	plugin->midiEvents.clear();
}

static void setParameter(AEffect *effect, int i, float f)
{
	Plugin *plugin = (Plugin *)effect->ptr3;
	plugin->synthesizer->setNormalizedParameterValue((Param) i, plugin->audioMasterValues.at(i) = f);
}

static float getParameter(AEffect *effect, int i)
{
	Plugin *plugin = (Plugin *)effect->ptr3;
	return plugin->audioMasterValues.at(i) = plugin->synthesizer->getNormalizedParameterValue((Param) i);
}

extern "C" {
#ifdef _WIN32
__declspec(dllexport) AEffect * MAIN(audioMasterCallback);
__declspec(dllexport) AEffect * VSTPluginMain(audioMasterCallback);
#else // https://gcc.gnu.org/onlinedocs/gcc/Asm-Labels.html
__attribute__ ((visibility("default"))) AEffect * MAIN(audioMasterCallback) asm ("main");
__attribute__ ((visibility("default"))) AEffect * VSTPluginMain(audioMasterCallback);
#endif
}

AEffect * VSTPluginMain(audioMasterCallback audioMaster)
{
#if defined(DEBUG) && DEBUG
	if (!logFile) {
		logFile = fopen("/tmp/amsynth.log", "a");
	}
#endif
	if (audioMaster) {
		audioMaster(nullptr, audioMasterGetProductString, 0, 0, hostProductString, 0.0f);
	}
	AEffect *effect = (AEffect *)calloc(1, sizeof(AEffect));
	effect->magic = kEffectMagic;
	effect->dispatcher = dispatcher;
	effect->process = process;
	effect->setParameter = setParameter;
	effect->getParameter = getParameter;
	effect->numPrograms = 0;
	effect->numParams = kAmsynthParameterCount;
	effect->numInputs = 0;
	effect->numOutputs = 2;
	effect->flags = effFlagsCanReplacing | effFlagsIsSynth | effFlagsProgramChunks | effFlagsHasEditor;
	effect->ptr3 = new Plugin(effect, audioMaster);
	effect->uniqueID = CCONST('a', 'm', 's', 'y');
	effect->processReplacing = processReplacing;
	return effect;
}

AEffect * MAIN(audioMasterCallback audioMaster)
{
	return VSTPluginMain (audioMaster);
}

/*
 *  Configuration.h
 *
 *  Copyright (c) 2001-2020 Nick Dowell
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
 
#ifndef CONFIG_H
#define CONFIG_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string>

/**
 * @brief Encapsulates any configuration parameters which must be shared between
 * components in the system.
 */
class Configuration
{
private:
	Configuration();
	void Defaults();

public:

	static Configuration & get() {
		static Configuration instance;
		return instance;
	}

	int	load();
	int	save();
	
	/**
	 * The sampling rate at which the output is to be produced
	 */
	int sample_rate;
	/**
	 * The number of the MIDI channel [1-16] to listen for messages on.
	 * If set to 0, all MIDI channels are listened to.
	 */
	int midi_channel;
#ifdef ENABLE_REALTIME
	/**
	 * Set to 1 if the audio & midi threads are running with realtime scheduling
	 * priorities, 0 otherwise
	 */
	int realtime;
	int current_audio_driver_wants_realtime;
#endif
    /**
     * The number of audio channels to be opened by the audio driver.
     */
	int channels;
	/**
	 * erm..
	 */
	int buffer_size;
	/**
	 * Used to specify the maximum number of voices allowed to be active 
	 * simultaneously. Attempting to play too many voices simultaneously will
	 * overload the CPU and disrupt the audio signal. Setting to 0 results in
	 * unlimited polyphony.
	 */
	int polyphony;
	/*
	 */
	int pitch_bend_range;
	/**
	 * Specify the audio output driver to use. currently "oss", "alsa", or 
	 * "auto" (which picks the best one)
	 */
	std::string audio_driver;
	std::string current_audio_driver;
	/**
	 * Specify the midi input driver to use. currently "oss", "alsa", or 
	 * "auto" (which picks the best one)
	 */
	std::string midi_driver;
	std::string current_midi_driver;
	/**
	 * The name if the device file for the OSS midi device.
	 */
	std::string oss_midi_device;
	/**
	 * The name if the device file for the OSS audio device.
	 */
	std::string oss_audio_device;
	/**
	 * The name of the ALSA PCM device to use
	 */
	std::string alsa_audio_device;
	
	std::string	current_bank_file;

	std::string	current_tuning_file;

	std::string	amsynthrc_fname;

	/**
	 * A list of parameter names (separated by spaces) that will be ignored when loading presets.
	 */
	std::string locked_parameters;
	
	bool jack_autoconnect;

	/* internal */
	std::string	jack_client_name;
	std::string	jack_client_name_preference;
	int 	alsa_seq_client_id;
	// used to count buffer underruns
	int	xruns;
};

#endif

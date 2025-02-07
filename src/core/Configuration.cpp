/*
 *  Configuration.cpp
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
 
#include "Configuration.h"

#include "filesystem.h"

#include <fstream>
#include <iostream>
#include <cstdlib>
#include <sstream>

Configuration::Configuration()
{
	amsynthrc_fname = filesystem::get().config;
	sample_rate = midi_channel = polyphony = xruns = 0;
#ifdef ENABLE_REALTIME
	realtime = 0;
#endif
	Defaults();
	load();
}

void
Configuration::Defaults	()
{
	audio_driver = "auto";
	midi_driver = "auto";
	oss_midi_device = "/dev/midi";
	midi_channel = 0;
	oss_audio_device = "/dev/dsp";
	alsa_audio_device = "default";
	sample_rate = 44100;
	channels = 2;
	buffer_size = 128;
	polyphony = 10;
	pitch_bend_range = 2;
	jack_autoconnect = true;
	jack_client_name_preference = "amsynth";
	current_bank_file = filesystem::get().default_bank;
	current_tuning_file = "default";
}

int
Configuration::load	()
{
	std::string buffer;

	std::fstream file( amsynthrc_fname.c_str(), std::ios::in );
	while( file.good() ) {
		file >> buffer;
		if( buffer[0]=='#' ){
			// ignore lines beginning with '#' (comments)
			// this next line is needed to deal with a line with 
			// just a '#'
			file.unget();
			// this moves file on by a whole line, so we ignore it
			getline(file, buffer);
		} else if (buffer=="audio_driver"){
			file >> buffer;
			audio_driver = buffer;
		} else if (buffer=="midi_driver"){
			file >> buffer;
			midi_driver = buffer;
		} else if (buffer=="oss_midi_device"){
			file >> buffer;
			oss_midi_device = buffer;
		} else if (buffer=="midi_channel"){
			file >> buffer;
			std::istringstream(buffer) >> midi_channel;
		} else if (buffer=="oss_audio_device"){
			file >> buffer;
			oss_audio_device = buffer;
		} else if (buffer=="alsa_audio_device"){
			file >> buffer;
			alsa_audio_device = buffer;
		} else if (buffer=="sample_rate"){
			file >> buffer;
			std::istringstream(buffer) >> sample_rate;
		} else if (buffer=="polyphony"){
			file >> buffer;
			std::istringstream(buffer) >> polyphony;
		} else if (buffer=="pitch_bend_range"){
			file >> buffer;
			std::istringstream(buffer) >> pitch_bend_range;
		} else if (buffer=="tuning_file") {
			file >> buffer;
			current_tuning_file = buffer;
		} else if (buffer == "ignored_parameters") {
			file >> buffer;
			locked_parameters = buffer;
		} else if (buffer == "jack_autoconnect") {
			file >> buffer;
			jack_autoconnect = (buffer == "true");
		} else {
			file >> buffer;
		}
	}
	file.close();
	
	return 0;
}

int
Configuration::save	()
{
	FILE *fout = fopen (amsynthrc_fname.c_str(), "w"); if (nullptr == fout) return -1;
	fprintf (fout, "midi_driver\t%s\n", midi_driver.c_str());
	fprintf (fout, "oss_midi_device\t%s\n", oss_midi_device.c_str());
	fprintf (fout, "midi_channel\t%d\n", midi_channel);
	fprintf (fout, "audio_driver\t%s\n", audio_driver.c_str());
	fprintf (fout, "oss_audio_device\t%s\n", oss_audio_device.c_str());
	fprintf (fout, "alsa_audio_device\t%s\n", alsa_audio_device.c_str());
	fprintf (fout, "sample_rate\t%d\n", sample_rate);
	fprintf (fout, "polyphony\t%d\n", polyphony);
	fprintf (fout, "pitch_bend_range\t%d\n", pitch_bend_range);
	fprintf (fout, "tuning_file\t%s\n", current_tuning_file.c_str());
	fprintf (fout, "ignored_parameters\t%s\n", locked_parameters.c_str());
	fprintf (fout, "jack_autoconnect\t%s\n", jack_autoconnect ? "true" : "false");
	fclose (fout);
	return 0;
}

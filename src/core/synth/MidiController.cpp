/*
 *  MidiController.cpp
 *
 *  Copyright (c) 2001-2023 Nick Dowell
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

#include "MidiController.h"

#include "core/filesystem.h"
#include "core/midi.h"
#include "core/synth/Synth--.h"

#include <assert.h>
#include <cstdlib>
#include <fstream>
#include <iostream>


MidiController::MidiController()
{
	loadControllerMap();
}

void
MidiController::HandleMidiData(const unsigned char* bytes, unsigned numBytes)
{
    for (unsigned i=0; i<numBytes; i++)
	{
		const unsigned char byte = bytes[i];
		
		if (byte & 0x80) {	// then byte is a status byte
			if (byte < 0xf0) {	// dont deal with system messages
			status = byte;
			channel = (byte & 0x0f);
			data = 0xff;
			}
			continue;
		}
		// now we have at least one data byte

		bool ignore = (assignedChannel > 0) && ((int) channel != assignedChannel - 1);

		switch (status & 0xf0)
		{
		case MIDI_STATUS_NOTE_OFF:
			// N.B. many devices send a 'note on' event with 0 velocity
			// rather than a distinct 'note off' event.
			if (data == 0xff) {
				data = byte;
				break;
			}
			if (!ignore) dispatch_note( channel, data, 0 );
			data = 0xff;
			break;
	
		case MIDI_STATUS_NOTE_ON:
			if (data == 0xff) {
				data = byte;
				break;
			}
				if (!ignore) dispatch_note(channel, data, byte);
			data = 0xff;
			break;
	
		case MIDI_STATUS_NOTE_PRESSURE:
			if (data == 0xff) {
				data = byte;
				break;
			}
			if (_handler) _handler->HandleMidiPressure(data, byte / 127.f);
			data = 0xff;
			break;

		case MIDI_STATUS_CONTROLLER:
			if (data == 0xFF) {
				data = byte;
				break;
			}
				if (!ignore) controller_change(data, byte);
			data = 0xFF;
			break;

		case MIDI_STATUS_PROGRAM_CHANGE:
			if (!ignore && presetController->getCurrPresetNumber() != byte) {
				if (_handler) _handler->HandleMidiAllSoundOff();
				presetController->selectPreset((int) byte);
			}
			data = 0xff;
			break;
	
		case MIDI_STATUS_CHANNEL_PRESSURE:
			if (_handler) _handler->HandleMidiPressure(-1, byte / 127.f);
			data = 0xff;
			break;
	
		case MIDI_STATUS_PITCH_WHEEL:
			// 2 data bytes give a 14 bit value, least significant 7 bits
			// first
			if (data == 0xFF) {
				data = byte;
				break;
			}
			int bend; bend = (int) ((data & 0x7F) | ((byte & 0x7F) << 7));
			float fbend; fbend = (float) (bend - 0x2000) / (float) (0x2000);
			if (!ignore) pitch_wheel_change(fbend);
			data = 0xFF;
			break;
	
		default:
			fprintf(stderr, "amsynth: invalid status byte: %02x data byte: %02x\n", status, byte);
			break;
		}
    }
}

void
MidiController::pitch_wheel_change(float val)
{
	if (_handler) _handler->HandleMidiPitchWheel(val);
}

void
MidiController::dispatch_note(unsigned char, unsigned char note, unsigned char vel)
{
	static const float scale = 1.f/127.f;
    if (!_handler) return;
	if (vel) _handler->HandleMidiNoteOn((int) note, (float)vel * scale);
	else     _handler->HandleMidiNoteOff((int) note, (float)vel * scale);
}

void
MidiController::controller_change(unsigned char cc, unsigned char value)
{
	_midi_cc_vals[cc] = value;

	_lastActiveController = cc;

	if (!_handler || !presetController)
		return;

	int paramId = _cc_to_param_map[cc];
	if (paramId >= 0) {
		presetController->getCurrentPreset().getParameter(paramId).setMidiValue(value);
		// Store the canonical MIDI CC representation to avoid unnecessary output #202
		_midi_cc_vals[cc] = presetController->getCurrentPreset().getParameter(paramId).getMidiValue();
		return; // MIDI CCs mapped by the user take precedence over default behaviour
	}

	switch (cc) {
		case MIDI_CC_BANK_SELECT_MSB: {
			presetController->selectBank(value);
			presetController->selectPreset(presetController->getCurrPresetNumber());
			break;
		}
		case MIDI_CC_PAN_MSB: {
			// https://web.archive.org/web/20160115024014/http://www.midi.org/techspecs/rp36.php
			// the effective range for CC#10 is modified to be 1 to 127, and values 0 and 1 both pan hard left
			float scaled = (value < 1 ? 0 : value - 1) / 126.f;
			_handler->HandleMidiPan(cosf(m::halfPi * scaled), sinf(m::halfPi * scaled));
			break;
		}
		case MIDI_CC_SUSTAIN_PEDAL:
			_handler->HandleMidiSustainPedal(value);
			break;
		case MIDI_CC_DATA_ENTRY_MSB:
			if (_rpn_msb == 0x00 && _rpn_lsb == 0x00)
				_handler->HandleMidiPitchWheelSensitivity(value);
			break;
		case MIDI_CC_RPN_LSB:
			_rpn_lsb = value;
			break;
		case MIDI_CC_RPN_MSB:
			_rpn_msb = value;
			break;
		case MIDI_CC_ALL_SOUND_OFF:
			if (value == 0)
				_handler->HandleMidiAllSoundOff();
			break;
		case MIDI_CC_RESET_ALL_CONTROLLERS:
			// https://web.archive.org/web/20160105110518/http://www.midi.org/techspecs/rp15.php
			_handler->HandleMidiPitchWheel(0);
			break;
		case MIDI_CC_ALL_NOTES_OFF:
			if (value == 0)
				_handler->HandleMidiAllNotesOff();
			break;
		case MIDI_CC_OMNI_MODE_OFF:
		case MIDI_CC_OMNI_MODE_ON:
		case MIDI_CC_MONO_MODE_ON:
		case MIDI_CC_POLY_MODE_ON:
			_handler->HandleMidiAllNotesOff();
			break;
		default:
			break;
	}
}

void
MidiController::clearControllerMap()
{
	for (size_t i = 0; i < MAX_CC; i++) {
		_cc_to_param_map[i] = -1;
		_midi_cc_vals[i] = 0;
	}
	for (size_t i = 0; i < kAmsynthParameterCount; i++)
		_param_to_cc_map[i] = -1;

	_param_to_cc_map[kAmsynthParameter_LFOToOscillators] = MIDI_CC_MODULATION_WHEEL_MSB;
	_param_to_cc_map[kAmsynthParameter_PortamentoTime]   = MIDI_CC_PORTAMENTO_TIME;
	_param_to_cc_map[kAmsynthParameter_MasterVolume]     = MIDI_CC_VOLUME;
	_param_to_cc_map[kAmsynthParameter_FilterResonance]  = MIDI_CC_SOUND_CONTROLLER_2;
	_param_to_cc_map[kAmsynthParameter_AmpEnvRelease]    = MIDI_CC_SOUND_CONTROLLER_3;
	_param_to_cc_map[kAmsynthParameter_AmpEnvAttack]     = MIDI_CC_SOUND_CONTROLLER_4;
	_param_to_cc_map[kAmsynthParameter_FilterCutoff]     = MIDI_CC_SOUND_CONTROLLER_5;
	_param_to_cc_map[kAmsynthParameter_AmpEnvDecay]      = MIDI_CC_SOUND_CONTROLLER_6;
	_param_to_cc_map[kAmsynthParameter_ReverbWet]        = MIDI_CC_EFFECTS_1_DEPTH;

	for (size_t i = 0; i < kAmsynthParameterCount; i++)
		if (_param_to_cc_map[i] != -1)
			_cc_to_param_map[_param_to_cc_map[i]] = i;
}

void
MidiController::loadControllerMap()
{
	clearControllerMap();

#ifdef _WIN32
	return;
#endif

	std::ifstream file(filesystem::get().controllers.c_str(), std::ios::out);
	std::string name;
	file >> name;
	for (int cc = 0; cc < MAX_CC && file.good(); cc++, file >> name) {
		int paramId = parameter_index_from_name(name.c_str());
		_cc_to_param_map[cc] = paramId;
		if (paramId >= 0 && paramId < kAmsynthParameterCount)
			_param_to_cc_map[paramId] = cc;
	}
	file.close();
}

void
MidiController::saveControllerMap()
{
#ifdef _WIN32
	return;
#endif
	std::ofstream file(filesystem::get().controllers.c_str(), std::ios::out);
	if (file.bad())
		return;
  	for (unsigned char cc = 0; cc < MAX_CC; cc++) {
		int paramId = _cc_to_param_map[cc];
		const char *name = parameter_name_from_index(paramId);
		file << (name ? name : "null") << std::endl;
	}
	file.close();
}

int
MidiController::getControllerForParameter(Param paramId)
{
	assert(0 <= paramId && paramId < kAmsynthParameterCount);
	return _param_to_cc_map[paramId];
}

void
MidiController::setControllerForParameter(Param paramId, int cc)
{
	assert(paramId < kAmsynthParameterCount && cc < MAX_CC);

	if (0 <= paramId) {
		int old_cc = _param_to_cc_map[paramId];
		if (0 <= old_cc)
			_cc_to_param_map[old_cc] = -1;
		_param_to_cc_map[paramId] = cc;
	}

	if (0 <= cc) {
		int old_param = _cc_to_param_map[cc];
		if (0 <= old_param)
			_param_to_cc_map[old_param] = -1;
		_cc_to_param_map[cc] = paramId;
	}

	saveControllerMap();
}

void
MidiController::generateMidiOutput(std::vector<amsynth_midi_cc_t> &output)
{
	unsigned char outputChannel = std::max(0, assignedChannel - 1);
	
	for (int paramId = 0; paramId < kAmsynthParameterCount; paramId++) {
		int cc = _param_to_cc_map[paramId];
		if (0 <= cc && cc < MAX_CC) {
			Parameter &parameter = presetController->getCurrentPreset().getParameter(paramId);
			unsigned char value = parameter.getMidiValue();
			if (_midi_cc_vals[cc] != value) {
				_midi_cc_vals[cc] = value;
				amsynth_midi_cc_t out = { outputChannel, (unsigned char)cc, value };
				output.push_back(out);
			}
		}
	}
}

int
MidiController::getLastActiveController()
{
	int value = _lastActiveController;
	_lastActiveController = -1;
	return value;
}

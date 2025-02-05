/*
 *  MidiController.h
 *
 *  Copyright (c) 2001-2022 Nick Dowell
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

#ifndef _MIDICONTROLLER_H
#define _MIDICONTROLLER_H

#include "PresetController.h"
#include "Parameter.h"
#include "../types.h"


#define MAX_CC 128

typedef unsigned char uchar;

class MidiEventHandler
{
public:
	virtual void HandleMidiNoteOn(int /*note*/, float /*velocity*/) = 0;
	virtual void HandleMidiNoteOff(int /*note*/, float /*velocity*/) = 0;
	virtual void HandleMidiPitchWheel(float /*value*/) = 0;
	virtual void HandleMidiPitchWheelSensitivity(uchar semitones) = 0;
	virtual void HandleMidiPressure(int /*note*/, float /*pressure*/) = 0;
	virtual void HandleMidiAllSoundOff() = 0;
	virtual void HandleMidiAllNotesOff() = 0;
	virtual void HandleMidiSustainPedal(uchar /*value*/) = 0;
	virtual void HandleMidiPan(float left, float right) = 0;

protected:
	~MidiEventHandler() = default;
};

class MidiController
{
public:
	MidiController();

	void	setPresetController	(PresetController & pc) { presetController = &pc; }
	void	SetMidiEventHandler(MidiEventHandler* h) { _handler = h; }
	
	void	HandleMidiData(const unsigned char *bytes, unsigned numBytes);

	void	clearControllerMap();
	void	loadControllerMap();

	int		getControllerForParameter(Param paramId);
	void	setControllerForParameter(Param paramId, int cc);

	void 	generateMidiOutput	(std::vector<amsynth_midi_cc_t> &);

	int		getLastActiveController();

	unsigned char assignedChannel = 0; // 0 denotes any channel

private:
	void dispatch_note(unsigned char ch,
		       unsigned char note, unsigned char vel);
    void controller_change(unsigned char controller, unsigned char value);
    void pitch_wheel_change(float val);

    void saveControllerMap();

    PresetController *presetController = nullptr;
    unsigned char status, data, channel;
	int _lastActiveController = -1;
	unsigned char _midi_cc_vals[MAX_CC];
	MidiEventHandler* _handler = nullptr;
	unsigned char _rpn_msb = 0xff;
	unsigned char _rpn_lsb = 0xff;

	int _cc_to_param_map[MAX_CC];
	int _param_to_cc_map[kAmsynthParameterCount];
};

#endif

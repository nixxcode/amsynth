/*
 *  VoiceAllocationUnit.h
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

#ifndef _VOICEALLOCATIONUNIT_H
#define _VOICEALLOCATIONUNIT_H

#include "MidiController.h"
#include "TuningMap.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include <vector>


class VoiceBoard;
class SoftLimiter;
class revmodel;
class Distortion;


class VoiceAllocationUnit : public Parameter::Observer, public MidiEventHandler
{
public:
			VoiceAllocationUnit		();
	virtual	~VoiceAllocationUnit	();

	void	parameterDidChange		(const Parameter &) override;

	void	SetSampleRate		(int);
	
	void	HandleMidiNoteOn(int note, float velocity) override;
	void	HandleMidiNoteOff(int note, float velocity) override;
	void	HandleMidiPitchWheel(float value) override;
	void	HandleMidiPitchWheelSensitivity(uchar semitones) override;
	void	HandleMidiPressure(int note, float pressure) override;
	void	HandleMidiAllSoundOff() override;
	void	HandleMidiAllNotesOff() override;
	void	HandleMidiSustainPedal(uchar value) override;
	void	HandleMidiPan(float left, float right) override { mPanGainLeft = left; mPanGainRight = right; }

	void	SetMaxVoices	(int voices) { mMaxVoices = voices; }
	int		GetMaxVoices	() { return mMaxVoices; }

	float	getPitchBendRangeSemitones() {return mPitchBendRangeSemitones;}
	void	setPitchBendRangeSemitones(float range) { mPitchBendRangeSemitones = range; }
	void	setKeyboardMode(KeyboardMode);

	void	Process			(float *l, float *r, unsigned nframes, int stride=1);

	bool	shouldPlayNote	(int note) const;
	double	noteToPitch		(int note) const;
	int		loadScale		(const std::string & sclFileName);
	int		loadKeyMap		(const std::string & kbmFileName);

// private:

	void	resetAllVoices();

	int		mMaxVoices;

	float	mPortamentoTime;
	int		mPortamentoMode;
	bool	keyPressed[128], sustain;
	bool	active[128];
	
	unsigned	_keyboardMode;
	unsigned	_keyPresses[128];
	unsigned	_keyPressCounter;
	
	std::vector<VoiceBoard*>	_voices;
	
	SoftLimiter	*limiter;
	revmodel	*reverb;
	Distortion	*distortion;
	
	float	*mBuffer;

	float	mMasterVol;
	float	mPanGainLeft;
	float	mPanGainRight;
	float	mPitchBendRangeSemitones;
	float	mPitchBendValue;
	float	mLastNoteFrequency;

	TuningMap	tuningMap;
#ifdef WITH_MTS_ESP
	struct MTSClient *mtsClient;
	bool mtsEspDisabled {false};
#endif
};

#endif

/*
 *  tests.cpp
 *
 *  Copyright (c) 2016-2023 Nick Dowell
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

#include "core/controls.h"
#include "core/midi.h"
#include "core/synth/LowPassFilter.h"
#include "core/synth/MidiController.h"
#include "core/synth/Oscillator.h"
#include "core/synth/Synthesizer.h"
#include "core/synth/VoiceAllocationUnit.h"
#include "core/synth/VoiceBoard.h"

#include <cassert>
#include <cstdio>
#include <iostream>

#define TEST(name) static void name()

TEST(testMidiOutput) {
    static float audioBuffer[64];

    Synthesizer *synth = new Synthesizer();
    synth->setSampleRate(44100);
    
    std::vector<amsynth_midi_event_t> midiIn;
    std::vector<amsynth_midi_cc_t> midiOut;
    
    synth->process(32, midiIn, midiOut, &audioBuffer[0], &audioBuffer[32]);
    
    Param param = kAmsynthParameter_ReverbWet;

    unsigned char cc = 2;
    synth->getMidiController()->setControllerForParameter(param, cc);
    
    for (unsigned char value = 0; value <= 127; value++) {
        unsigned char midi[4] = { MIDI_STATUS_CONTROLLER, cc, value };
        amsynth_midi_event_t event;
        event.length = 3;
        event.offset_frames = 0;
        event.buffer = midi;
        midiIn.clear();
        midiIn.push_back(event);
        
        midiOut.clear();
        synth->process(32, midiIn, midiOut, &audioBuffer[0], &audioBuffer[32]);

        int outputValue = (int)roundf(synth->getNormalizedParameterValue(param) * 127.0f);
        assert(outputValue == value || 0 == "parameter value should be changed when a cc is processed");

        assert(midiOut.empty() || 0 == "no midi output should be generated when a cc is processed");
    }
    
    midiIn.clear();
    midiOut.clear();
    synth->setNormalizedParameterValue(param, 0);
    synth->process(32, midiIn, midiOut, &audioBuffer[0], &audioBuffer[32]);
    assert(midiOut.size() == 1 || 0 == "midi output should be generated when a parameter is changed");
    assert(midiOut[0].value == 0 || 0 == "midi output value is incorrect");

    delete synth;
}

TEST(testMidiOutput_OnOff) {
    Synthesizer *synth = new Synthesizer();
    MidiController *midiController = synth->getMidiController();
    midiController->setControllerForParameter(kAmsynthParameter_Oscillator2Sync, 2);
    synth->setNormalizedParameterValue(kAmsynthParameter_Oscillator2Sync, 0);
    std::vector<amsynth_midi_cc_t> midiOut;
    midiController->generateMidiOutput(midiOut);
    midiOut.clear();

    unsigned char data[3] = { MIDI_STATUS_CONTROLLER, 2, 0 };

    data[2] = 10; // off ~> 0 (not emitted)
    midiController->HandleMidiData(data, 3);
    midiController->generateMidiOutput(midiOut);
    assert(midiOut.empty());

    data[2] = 100; // on ~> 127 (not emitted)
    midiController->HandleMidiData(data, 3);
    midiController->generateMidiOutput(midiOut);
    assert(midiOut.empty());

    data[2] = 42; // off ~> 0 (not emitted)
    midiController->HandleMidiData(data, 3);
    midiController->generateMidiOutput(midiOut);
    assert(midiOut.empty());

    synth->setNormalizedParameterValue(kAmsynthParameter_Oscillator2Sync, 1);
    midiController->generateMidiOutput(midiOut);
    assert(midiOut[0].value == 127);

    delete synth;
}

static int countActiveVoices(Synthesizer *synth) {
    int count = 0;
    for (int i = 0; i < 128; i++) {
        count = count + (synth->_voiceAllocationUnit->active[i] ? 1 : 0);
    }
    return count;
}

TEST(testMidiAllNotesOff) {
    static float audioBuffer[64];

    Synthesizer *synth = new Synthesizer();
    synth->setSampleRate(44100);
    synth->setParameterValue(kAmsynthParameter_KeyboardMode, KeyboardModePoly);

    std::vector<amsynth_midi_event_t> midiIn;
    std::vector<amsynth_midi_cc_t> midiOut;

    synth->process(32, midiIn, midiOut, &audioBuffer[0], &audioBuffer[32]);
    assert(countActiveVoices(synth) == 0);

    /* */ {
        unsigned char midi[4] = { MIDI_STATUS_NOTE_ON, 64, 100 };
        amsynth_midi_event_t e = { 0, 3, midi };
        midiIn.clear();
        midiIn.push_back(e);
        synth->process(32, midiIn, midiOut, &audioBuffer[0], &audioBuffer[32]);
        assert(countActiveVoices(synth) == 1);
    }

    /* */ {
        unsigned char midi[4] = { MIDI_STATUS_NOTE_ON, 80, 100 };
        amsynth_midi_event_t e = { 0, 3, midi };
        midiIn.clear();
        midiIn.push_back(e);
        synth->process(32, midiIn, midiOut, &audioBuffer[0], &audioBuffer[32]);
        assert(countActiveVoices(synth) == 2);
    }

    /* */ {
        unsigned char midi[4] = { MIDI_STATUS_CONTROLLER, MIDI_CC_ALL_NOTES_OFF, 0 };
        amsynth_midi_event_t e = { 0, 3, midi };
        midiIn.clear();
        midiIn.push_back(e);
        synth->process(32, midiIn, midiOut, &audioBuffer[0], &audioBuffer[32]);
        assert(countActiveVoices(synth) == 0);
    }

    delete synth;
}

TEST(testPresetIgnoredParameters) {
    Preset basePreset;
    basePreset.getParameter(0).setValue(1);
    Preset newPreset = basePreset;
    newPreset.getParameter(0).setValue(0);
    assert(!basePreset.isEqual(newPreset));
    Preset::setLockedParameterNames("amp_attack amp_decay");
    assert(basePreset.isEqual(newPreset));
    Preset::setLockedParameterNames("");
    assert(!basePreset.isEqual(newPreset));
}

static size_t count(const char **strings) {
    size_t count;
    for (count = 0; strings[count]; count ++);
    return count;
}

TEST(testPresetValueStrings) {
    assert(count(parameter_get_value_strings(kAmsynthParameter_Oscillator1Waveform)) == (int)Oscillator::Waveform::kRandom + 1);
    assert(count(parameter_get_value_strings(kAmsynthParameter_Oscillator2Waveform)) == (int)Oscillator::Waveform::kRandom + 1);
    assert(count(parameter_get_value_strings(kAmsynthParameter_KeyboardMode)) == KeyboardModeLegato + 1);
    assert(count(parameter_get_value_strings(kAmsynthParameter_FilterType)) == (int)SynthFilter::Type::kBypass + 1);
    assert(count(parameter_get_value_strings(kAmsynthParameter_FilterSlope)) == (int)SynthFilter::Slope::k12 + 2);
    assert(count(parameter_get_value_strings(kAmsynthParameter_LFOOscillatorSelect)) == 3);
    assert(count(parameter_get_value_strings(kAmsynthParameter_PortamentoMode)) == PortamentoModeLegato + 1);
}

TEST(testOscillatorHighFrequency) {
    static float buffer[VoiceBoard::kMaxProcessBufferSize];
    
    Oscillator osc;
    osc.SetSampleRate(44100);
    for (int waveform = (int)Oscillator::Waveform::kSine; waveform <= (int)Oscillator::Waveform::kRandom; waveform++) {
        osc.SetWaveform((Oscillator::Waveform)waveform);
        osc.ProcessSamples(buffer, VoiceBoard::kMaxProcessBufferSize, 99999, 0.5f);
    }
}

#define RUN_TEST(testFunction) do { printf("%s()... ", #testFunction); testFunction(); printf("OK\n"); } while (0)

int main(int argc, const char * argv[])  {
    RUN_TEST(testMidiOutput);
    RUN_TEST(testMidiOutput_OnOff);
    RUN_TEST(testPresetIgnoredParameters);
    RUN_TEST(testPresetValueStrings);
    RUN_TEST(testMidiAllNotesOff);
    RUN_TEST(testOscillatorHighFrequency);
    return 0;
}

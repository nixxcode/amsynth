/*
 *  Parameter.h
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

#include "Parameter.h"

#include "core/gettext.h"
#include "core/synth/Synth--.h"

#define _(string) gettext (string)

#include <cassert>
#include <sstream>
#include <cstring>
#include <vector>

#if defined(__GNUC__) && !defined(__clang__) // GNU supports Designated Initializers in C++
#define SPEC(id, name, def, min, max, step, law, base, offset, label) [id] = { name, def, min, max,  step, law, base, offset, label }
#else
#define SPEC(id, name, def, min, max, step, law, base, offset, label)        { name, def, min, max,  step, law, base, offset, label }
#endif

static const ParameterSpec ParameterSpecs[] = { //                            def,    min,    max,  step,       law,                         base,  offset,     label
	SPEC(kAmsynthParameter_AmpEnvAttack,            "amp_attack",            0.0f,   0.0f,   2.5f,  0.0f,       kParameterLaw_Power,         3.0f,  0.0005f,    "s"  ),
	SPEC(kAmsynthParameter_AmpEnvDecay,             "amp_decay",             0.0f,   0.0f,   2.5f,  0.0f,       kParameterLaw_Power,         3.0f,  0.0005f,    "s"  ),
	SPEC(kAmsynthParameter_AmpEnvSustain,           "amp_sustain",           1.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_AmpEnvRelease,           "amp_release",           0.0f,   0.0f,   2.5f,  0.0f,       kParameterLaw_Power,         3.0f,  0.0005f,    "s"  ),
	SPEC(kAmsynthParameter_Oscillator1Waveform,     "osc1_waveform",         2.0f,   0.0f,   4.0f,  1.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_FilterEnvAttack,         "filter_attack",         0.0f,   0.0f,   2.5f,  0.0f,       kParameterLaw_Power,         3.0f,  0.0005f,    "s"  ),
	SPEC(kAmsynthParameter_FilterEnvDecay,          "filter_decay",          0.0f,   0.0f,   2.5f,  0.0f,       kParameterLaw_Power,         3.0f,  0.0005f,    "s"  ),
	SPEC(kAmsynthParameter_FilterEnvSustain,        "filter_sustain",        1.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_FilterEnvRelease,        "filter_release",        0.0f,   0.0f,   2.5f,  0.0f,       kParameterLaw_Power,         3.0f,  0.0005f,    "s"  ),
	SPEC(kAmsynthParameter_FilterResonance,         "filter_resonance",      0.0f,   0.0f,   0.97f, 0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_FilterEnvAmount,         "filter_env_amount",     0.0f, -16.0f,  16.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_FilterCutoff,            "filter_cutoff",         1.5f,  -0.5f,   1.5f,  0.0f,       kParameterLaw_Exponential,  16.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_Oscillator2Detune,       "osc2_detune",           0.0f,  -1.0f,   1.0f,  0.0f,       kParameterLaw_Exponential,   1.25f, 0.0f,       ""   ),
	SPEC(kAmsynthParameter_Oscillator2Waveform,     "osc2_waveform",         2.0f,   0.0f,   4.0f,  1.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_MasterVolume,            "master_vol",            0.67f,  0.0f,   1.0f,  0.0f,       kParameterLaw_Power,         2.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_LFOFreq,                 "lfo_freq",              0.0f,   0.0f,   7.5f,  0.0f,       kParameterLaw_Power,         2.0f,  0.0f,       "Hz" ),
	SPEC(kAmsynthParameter_LFOWaveform,             "lfo_waveform",          0.0f,   0.0f,   6.0f,  1.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_Oscillator2Octave,       "osc2_range",            0.0f,  -3.0f,   4.0f,  1.0f,       kParameterLaw_Exponential,   2.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_OscillatorMix,           "osc_mix",               0.0f,  -1.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_LFOToOscillators,        "freq_mod_amount",       0.0f,   0.0f, 1.25992105f, 0.0f,   kParameterLaw_Power,         3.0f, -1.0f,       ""   ),
	SPEC(kAmsynthParameter_LFOToFilterCutoff,       "filter_mod_amount",    -1.0f,  -1.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_LFOToAmp,                "amp_mod_amount",       -1.0f,  -1.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_OscillatorMixRingMod,    "osc_mix_mode",          0.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_Oscillator1Pulsewidth,   "osc1_pulsewidth",       1.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_Oscillator2Pulsewidth,   "osc2_pulsewidth",       1.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_ReverbRoomsize,          "reverb_roomsize",       0.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_ReverbDamp,              "reverb_damp",           0.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_ReverbWet,               "reverb_wet",            0.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_ReverbWidth,             "reverb_width",          1.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_AmpDistortion,           "distortion_crunch",     0.0f,   0.0f,   0.9f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_Oscillator2Sync,         "osc2_sync",             0.0f,   0.0f,   1.0f,  1.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_PortamentoTime,          "portamento_time",       0.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_KeyboardMode,            "keyboard_mode",         0.0f,   0.0f,   2.0f,  1.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_Oscillator2Pitch,        "osc2_pitch",            0.0f, -12.0f,  12.0f,  1.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_FilterType,              "filter_type",           0.0f,   0.0f,   4.0f,  1.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_FilterSlope,             "filter_slope",          1.0f,   0.0f,   1.0f,  1.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_LFOOscillatorSelect,     "freq_mod_osc",          0.0f,   0.0f,   2.0f,  1.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_FilterKeyTrackAmount,    "filter_kbd_track",      1.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_FilterKeyVelocityAmount, "filter_vel_sens",       1.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_AmpVelocityAmount,       "amp_vel_sens",          1.0f,   0.0f,   1.0f,  0.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
	SPEC(kAmsynthParameter_PortamentoMode,          "portamento_mode",       0.0f,   0.0f,   1.0f,  1.0f,       kParameterLaw_Linear,        1.0f,  0.0f,       ""   ),
};

static float getControlValue(const ParameterSpec &spec, float value)
{
	switch (spec.law) {
		case kParameterLaw_Linear:
			return spec.offset + spec.base * value;
		case kParameterLaw_Exponential:
			return spec.offset + ::pow((float)spec.base, value);
		case kParameterLaw_Power:
			return spec.offset + ::pow(value, (float)spec.base);
		default:
			assert(!"Invalid ParameterLaw");
	}
}

Parameter::Parameter(Param paramId)
:	_paramId	(paramId)
,	_spec		(ParameterSpecs[paramId])
,	_value		(_spec.def)
{}

void
Parameter::addObserver(Observer *observer)
{
	_observers.insert(observer);
	observer->parameterDidChange(*this);
}

void
Parameter::setValue(float value)
{
	if (value == _value) return;
	
	float newValue = std::min(std::max(value, _spec.min), _spec.max);

	if (_spec.step > 0.f) {
		newValue = _spec.min + roundf((newValue - _spec.min) / _spec.step) * _spec.step;
		assert(::fmodf(newValue - _spec.min, _spec.step) == 0);
	}

	if (_value == newValue) // warning: -ffast-math causes this comparison to fail
		return;

	_value = newValue;

	for (auto it : _observers) it->parameterDidChange(*this);
}

float
Parameter::getControlValue() const
{
	return ::getControlValue(_spec, _value);
}

float
Parameter::valueFromString(const std::string &str)
{
	// atof() and friends are affected by currently configured locale,
	// which can change the decimal point character.
	std::istringstream istr(str);
	static std::locale locale = std::locale("C");
	istr.imbue(locale);
	float value = m::nan;
	istr >> value;
	return value;
}

const std::string
Parameter::getStringValue() const
{
	std::ostringstream stream;
	stream << getControlValue();
	return stream.str();
}

void
Parameter::randomise()
{
	setValue( ((rand()/(float)RAND_MAX) * (getMax()-getMin()) + getMin()) );
}

///////////////////////////////////////////////////////////////////////////////

void get_parameter_properties(int parameter_index, double *minimum, double *maximum, double *default_value, double *step_size)
{
	const ParameterSpec &spec = ParameterSpecs[parameter_index];
	if (minimum) {
		*minimum = spec.min;
	}
	if (maximum) {
		*maximum = spec.max;
	}
	if (default_value) {
		*default_value = spec.def;
	}
	if (step_size) {
		*step_size = spec.step;
	}
}

const char *parameter_name_from_index(int param_index)
{
	if (param_index < 0 || param_index >= (int)kAmsynthParameterCount)
		return nullptr;
	return ParameterSpecs[param_index].name;
}

int parameter_index_from_name(const char *name)
{
	for (int i = 0; i < kAmsynthParameterCount; i++) {
		if (!strcmp(ParameterSpecs[i].name, name)) {
			return i;
		}
	}
	return -1;
}

int parameter_get_display(int param_index, float value, char *buffer, size_t maxlen)
{
	const ParameterSpec &spec = ParameterSpecs[param_index];
	const float cv = getControlValue(spec, value);
	const float normalised = (value - spec.min) / (spec.max - spec.min);
	
	switch ((Param)param_index) {
		case kAmsynthParameter_AmpEnvAttack:
		case kAmsynthParameter_AmpEnvDecay:
		case kAmsynthParameter_AmpEnvRelease:
		case kAmsynthParameter_FilterEnvAttack:
		case kAmsynthParameter_FilterEnvDecay:
		case kAmsynthParameter_FilterEnvRelease:
		case kAmsynthParameter_PortamentoTime:
			if (cv < 1.0) {
				return snprintf(buffer, maxlen, "%.0f ms", cv * 1000);
			} else {
				return snprintf(buffer, maxlen, "%.1f s", cv);
			}
		case kAmsynthParameter_LFOFreq:
			return snprintf(buffer, maxlen, "%.1f Hz", cv);
		case kAmsynthParameter_Oscillator2Detune:
			return snprintf(buffer, maxlen, "%+.1f cents", 1200.0 * log2(cv));
		case kAmsynthParameter_Oscillator2Pitch:
			return snprintf(buffer, maxlen, "%+.0f semitone%s", cv, fabsf(cv) < 2 ? "" : "s");
		case kAmsynthParameter_Oscillator2Octave:
			return snprintf(buffer, maxlen, "%+.0f octave%s", value, fabsf(value) < 2 ? "" : "s");
		case kAmsynthParameter_MasterVolume:
			return snprintf(buffer, maxlen, "%+.1f dB", 20.0 * log10(cv));
		case kAmsynthParameter_OscillatorMixRingMod:
			return snprintf(buffer, maxlen, "%d %%", (int)roundf(cv * 100.f));
		case kAmsynthParameter_FilterEnvAmount:
			return snprintf(buffer, maxlen, "%+d %%", (int)roundf(cv / 16.f * 100.f));
		case kAmsynthParameter_AmpEnvSustain:
		case kAmsynthParameter_FilterResonance:
		case kAmsynthParameter_FilterCutoff:
		case kAmsynthParameter_FilterEnvSustain:
		case kAmsynthParameter_LFOToOscillators:
		case kAmsynthParameter_LFOToFilterCutoff:
		case kAmsynthParameter_LFOToAmp:
		case kAmsynthParameter_ReverbRoomsize:
		case kAmsynthParameter_ReverbDamp:
		case kAmsynthParameter_ReverbWet:
		case kAmsynthParameter_ReverbWidth:
		case kAmsynthParameter_AmpDistortion:
		case kAmsynthParameter_FilterKeyTrackAmount:
		case kAmsynthParameter_FilterKeyVelocityAmount:
		case kAmsynthParameter_AmpVelocityAmount:
			return snprintf(buffer, maxlen, "%d %%", (int)roundf(normalised * 100.f));
		case kAmsynthParameter_FilterType: {
			const char **filter_type_names = parameter_get_value_strings(param_index);
			return filter_type_names ? snprintf(buffer, maxlen, "%s", filter_type_names[(int)cv]) : 0;
		}
		case kAmsynthParameter_Oscillator1Waveform:
		case kAmsynthParameter_Oscillator2Waveform:
		case kAmsynthParameter_LFOWaveform:
		case kAmsynthParameter_OscillatorMix:
		case kAmsynthParameter_Oscillator1Pulsewidth:
		case kAmsynthParameter_Oscillator2Pulsewidth:
		case kAmsynthParameter_Oscillator2Sync:
		case kAmsynthParameter_KeyboardMode:
		case kAmsynthParameter_FilterSlope:
		case kAmsynthParameter_LFOOscillatorSelect:
		case kAmsynthParameter_PortamentoMode:
			if (spec.min == 0.f) {
				const char **strings = parameter_get_value_strings(param_index);
				if (strings && value <= spec.max) {
					return snprintf(buffer, maxlen, "%s", strings[(int)roundf(value)]);
				} else if (spec.max == 1.f && spec.step == 1.f) {
					return snprintf(buffer, maxlen, (int)roundf(normalised) ? "on" : "off");
				}
			}
			return 0;
		case kAmsynthParameterCount:
		default:
			fprintf(stderr, "amsynth: parameter_get_display: out of bounds parameter index %d\n", param_index);
			return 0;
	}
}

const char **parameter_get_value_strings(int param_index)
{
	static std::vector<std::vector<const char *> > parameterStrings(kAmsynthParameterCount);
	if (param_index < 0 || param_index >= (int)parameterStrings.size())
		return nullptr;

	std::vector<const char *> & strings = parameterStrings[param_index];
	if (strings.empty()) {
		size_t i = 0, size = 0;
		switch (param_index) {
			case kAmsynthParameter_Oscillator1Waveform:
			case kAmsynthParameter_Oscillator2Waveform:
				strings.resize(size = 6);
				strings[i++] = _("sine");
				strings[i++] = _("square / pulse");
				strings[i++] = _("triangle / saw");
				strings[i++] = _("white noise");
				strings[i++] = _("noise + sample & hold");
				assert(i < size);
				break;

			case kAmsynthParameter_LFOWaveform:
				strings.resize(size = 8);
				strings[i++] = _("sine");
				strings[i++] = _("square");
				strings[i++] = _("triangle");
				strings[i++] = _("noise");
				strings[i++] = _("noise + sample & hold");
				strings[i++] = _("sawtooth (up)");
				strings[i++] = _("sawtooth (down)");
				assert(i < size);
				break;

			case kAmsynthParameter_KeyboardMode:
				strings.resize(size = 4);
				strings[i++] = _("poly");
				strings[i++] = _("mono");
				strings[i++] = _("legato");
				assert(i < size);
				break;

			case kAmsynthParameter_FilterType:
				strings.resize(size = 6);
				strings[i++] = _("low pass");
				strings[i++] = _("high pass");
				strings[i++] = _("band pass");
				strings[i++] = _("notch");
				strings[i++] = _("bypass");
				assert(i < size);
				break;

			case kAmsynthParameter_FilterSlope:
				strings.resize(size = 3);
				strings[i++] = _("12 dB / octave");
				strings[i++] = _("24 dB / octave");
				assert(i < size);
				break;

			case kAmsynthParameter_LFOOscillatorSelect:
				strings.resize(size = 4);
				strings[i++] = _("osc 1+2");
				strings[i++] = _("osc 1");
				strings[i++] = _("osc 2");
				assert(i < size);
				break;

			case kAmsynthParameter_PortamentoMode:
				strings.resize(size = 3);
				strings[i++] = _("always");
				strings[i++] = _("legato");
				assert(i < size);
				break;

			default:
				break;
		}
	}

	return strings.empty() ? nullptr : &strings[0];
}

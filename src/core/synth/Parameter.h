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

#ifndef _PARAMETER_H
#define _PARAMETER_H

#include "core/controls.h"

#include <cmath>
#include <set>
#include <string>

enum ParameterLaw {
	kParameterLaw_Linear,		// offset + base * value
	kParameterLaw_Exponential,	// offset + base ^ value
	kParameterLaw_Power			// offset + value ^ base
};

struct ParameterSpec {
	const char *name;
	float def;
	float min;
	float max;
	float step;
	ParameterLaw law;
	float base;
	float offset;
	const char label[4];
};

/**
 * @brief a Parameter holds a particular value for a slider, selector switch 
 * etc..
 *
 * This object is also responsible for keeping all audio/signal generation 
 * objects up to date with any controls which affect them.
 *
 * This object also easily enables non-linear relationships between the controls
 * (eg interface ParameterViews) and their effect on synthesis parameters. See
 * Parameter::Law for details.
 */

class Parameter {
public:
	class Observer {
	public:
		virtual void parameterDidChange(const Parameter &) {}
		virtual void parameterBeginEdit(const Parameter &) {}
		virtual void parameterEndEdit(const Parameter &) {}
	protected:
		~Observer() = default;
	};

	Parameter(Param paramId);

	float			getValue		() const { return _value; }
	void			setValue		(float value);

	static float	valueFromString	(const std::string &str);

	float			getNormalisedValue	() const { return (getValue()-getMin())/(getMax()-getMin()); }
	void			setNormalisedValue	(float val) { setValue (val*(getMax()-getMin())+getMin()); }

	unsigned char	getMidiValue		() const { return (unsigned char) roundf(getNormalisedValue() * 127.f); }
	void			setMidiValue		(unsigned char value) { setNormalisedValue(value / 127.f); }

	// The control value for this parameter.
	// The control value is what the synthesis will use to get its values.
	float			getControlValue		() const;

	const std::string getStringValue	() const;

	const char * 	getName				() const { return _spec.name; }

	Param			getId				() const { return _paramId; }

	void			addObserver			(Observer *observer);
	void			removeObserver		(Observer *observer) { _observers.erase(observer); }

	void			beginEdit		() const { for (auto it : _observers) it->parameterBeginEdit(*this); }
	void			endEdit			() const { for (auto it : _observers) it->parameterEndEdit(*this); }

	float			getDefault		() const { return _spec.def; }

	// min/max values apply for calls to setValue() not ControlValue
	float			getMin			() const { return _spec.min; }
	float			getMax			() const { return _spec.max; }

	// @return the increment value
	float			getStep			() const { return _spec.step; }
	// @returns The number of discrete steps allowable in this Parameter.
	int				getSteps		() const { return _spec.step > 0.f ? (int) ((_spec.max - _spec.min) / _spec.step) : 0; }

	// Set this parameter to a random value (in it's allowable range)
	void			randomise		();

	// The label assocaited with this Parameter. (e.g. "seconds")
	const std::string getLabel		() const { return _spec.label; }

// private:
	Param							_paramId;
	const ParameterSpec &			_spec;
	float							_value;
	std::set<Observer *>			_observers;
};

#endif

/*
 *  Preset.cpp
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

#include "Preset.h"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iterator>
#include <map>
#include <vector>


Preset::Preset(const std::string name) : mName (name)
{
	mParameters.reserve(kAmsynthParameterCount);
	for (int i = 0; i < kAmsynthParameterCount; i++) {
		mParameters.emplace_back((Param) i);
	}
}

Preset::~Preset() = default;

Preset&
Preset::operator =		(const Preset &rhs)
{
    for (int i = 0; i < kAmsynthParameterCount; i++) {
		if (isParameterLocked(i))
			continue;
		getParameter(i).setValue(rhs.getParameter(i).getValue());
    }
    setName(rhs.getName());
    return *this;
}

bool
Preset::isEqual(const Preset &rhs)
{
	for (int i = 0; i < kAmsynthParameterCount; i++) {
		if (isParameterLocked(i))
			continue;
		if (getParameter(i).getNormalisedValue() != rhs.getParameter(i).getNormalisedValue())
			return false;
	}
	return getName() == rhs.getName();
}

Parameter & 
Preset::getParameter(const std::string name)
{
	typedef std::map<std::string, int> name_map_t;
	static name_map_t name_map;
	if (name_map.empty()) {
		for (int i = 0; i < kAmsynthParameterCount; i++) {
			name_map[getParameter(i).getName()] = i;
		}
	}
	name_map_t::iterator it = name_map.find(name);
	assert(it != name_map.end());
	return getParameter((int) it->second);
}

void
Preset::randomise()
{
	for (int i = 0; i < kAmsynthParameterCount; i++) {
		if (i != kAmsynthParameter_MasterVolume) {
			getParameter(i).randomise();
		}
	}
}

void
Preset::addObserver(Parameter::Observer *observer)
{
	for (auto &it : mParameters) it.addObserver(observer);
}

void
Preset::toString(std::stringstream &stream)
{
	stream << "amSynth1.0preset" << std::endl;
	stream << "<preset> " << "<name> " << getName() << std::endl;
	for (int n = 0; n < kAmsynthParameterCount; n++) {
		stream << "<parameter> " << getParameter(n).getName() << " " << getParameter(n).getValue() << std::endl;
	}
}

bool
Preset::fromString(const std::string &str)
{
	std::stringstream stream (str);

	std::string buffer;
  
	stream >> buffer;
  
	if (buffer != "amSynth1.0preset") return false;
  
	stream >> buffer;
	if (buffer == "<preset>") {
		stream >> buffer;
		
		//get the preset's name
		stream >> buffer;
		std::string presetName;
		presetName += buffer;
		stream >> buffer;
		while (buffer != "<parameter>") {
			presetName += " ";
			presetName += buffer;
			stream >> buffer;
		}
		setName(presetName); 
		
		//get the parameters
		while (buffer == "<parameter>") {
			std::string name;
			stream >> buffer;
			name = buffer;
			stream >> buffer;
			if (name!="unused")
				getParameter(name).setValue(Parameter::valueFromString(buffer));
			stream >> buffer;
		}
	};
	return true;
}

static std::array<bool, kAmsynthParameterCount> lockedParameters {};

bool Preset::isParameterLocked(int parameter)
{
	return lockedParameters.at(parameter);
}

void Preset::setParameterLocked(int parameter, bool locked)
{
	lockedParameters.at(parameter) = locked;
}

std::string Preset::getLockedParameterNames()
{
	std::string names;
	for (int i = 0; i < kAmsynthParameterCount; i++) {
		if (isParameterLocked(i)) {
			if (!names.empty())
				names += " ";
			names += std::string(parameter_name_from_index(i));
		}
	}
	return names;
}

void Preset::setLockedParameterNames(std::string names)
{
	for (int i = 0; i < kAmsynthParameterCount; i++) {
		setParameterLocked(i, false);
	}

	std::stringstream ss(names);
	std::istream_iterator<std::string> begin(ss);
	std::istream_iterator<std::string> end;
	std::vector<std::string> vstrings(begin, end);

	std::vector<std::string>::const_iterator name_it;
	for (name_it = vstrings.begin(); name_it != vstrings.end(); ++name_it) {
		int index = parameter_index_from_name(name_it->c_str());
		if (index != -1) {
			setParameterLocked(index, true);
		}
	}
}

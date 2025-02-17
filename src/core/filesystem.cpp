/*
 *  filesystem.cpp
 *
 *  Copyright (c) 2019 Nick Dowell
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

#include "filesystem.h"

#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef _WIN32
#include <direct.h>
#endif

filesystem& filesystem::get()
{
    static filesystem singleton;
    return singleton;
}

filesystem::filesystem()
{
#ifdef PKGDATADIR
    factory_banks = std::string(PKGDATADIR "/banks");
    skins = std::string(PKGDATADIR "/skins");

    const char *env_home = getenv("HOME");
    if (!env_home) {
        return;
    }

    std::string home(env_home);

    // https://specifications.freedesktop.org/basedir-spec/basedir-spec-latest.html

    const char *env_xdg_config_home = getenv("XDG_CONFIG_HOME");
    std::string xdg_config_home = env_xdg_config_home ? std::string(env_xdg_config_home) : home + "/.config";
    std::string amsynth_config_dir = xdg_config_home + "/amsynth";
    config      = amsynth_config_dir + "/config";
    controllers = amsynth_config_dir + "/controllers";

    const char *env_xdg_data_home = getenv("XDG_DATA_HOME");
    std::string xdg_data_home = env_xdg_data_home ? std::string(env_xdg_data_home) : home + "/.local/share";
    std::string amsynth_data_dir = xdg_data_home + "/amsynth";
    user_banks = amsynth_data_dir + "/banks";
    default_bank = user_banks + "/default";

    create_dir(amsynth_config_dir);

    if (!exists(controllers)) {
        move(home + "/.amSynthControllersrc", controllers);
    }

    if (!exists(config) &&
        !move(home + "/.amSynthrc", config) &&
        !copy(PKGDATADIR "/rc", config)) {
        std::cerr << "Error: could not create " << controllers << std::endl;
    }

    if (!exists(amsynth_data_dir) &&
        !move(home + "/.amsynth", amsynth_data_dir) &&
        !(create_dir(amsynth_data_dir) && create_dir(user_banks))) {
        std::cerr << "Error: could not create " << amsynth_data_dir << std::endl;
    }

    if (!exists(default_bank) &&
        !move(home + "/.amSynth.presets", default_bank) &&
        !copy(PKGDATADIR "/banks/amsynth_factory.bank", default_bank)) {
        std::cerr << "Error: could not create " << default_bank << std::endl;
    }
#elif defined(__APPLE__)
	factory_banks = "/Library/Application Support/amsynth/banks";
	skins = "/Library/Application Support/amsynth/skins";
	auto prefs = std::string(getenv("HOME")) + "/Library/Preferences/amsynth";
	config = prefs + "/config";
	controllers = prefs + "/controllers";
	user_banks = prefs + "/banks";
	default_bank = user_banks + "/default";
	create_dir(prefs);
	create_dir(user_banks);
	if (!exists(default_bank)) {
		// Create an empty bank file
		std::ofstream(default_bank, std::ios::out) << "amSynth\nEOF\n";
	}
#elif defined(_WIN32)
	auto data = std::string(getenv("ProgramData")) + "\\amsynth";
	factory_banks = data + "\\banks";
	skins = data + "\\skins";
	auto prefs = std::string(getenv("APPDATA")) + "\\amsynth";
	config = prefs + "\\config";
	controllers = prefs + "\\controllers";
	user_banks = prefs + "\\banks";
	default_bank = user_banks + "\\default";
	create_dir(prefs);
	create_dir(user_banks);
	if (!exists(default_bank)) {
		// Create an empty bank file
		std::ofstream(default_bank, std::ios::out) << "amSynth\nEOF\n";
	}
#endif
}

bool filesystem::copy(const std::string &from, const std::string &to)
{
    std::ifstream input(from.c_str(), std::ios::in | std::ios::binary);
    if (!input.is_open()) {
        return false;
    }
    std::ofstream output(to.c_str(), std::ios::out | std::ios::binary);
    output << input.rdbuf();
    return true;
}

bool filesystem::create_dir(const std::string &path)
{
#if defined(_WIN32)
    return _mkdir(path.c_str()) == 0;
#else
    return mkdir(path.c_str(), 0755) == 0;
#endif
}

bool filesystem::exists(const std::string &path)
{
    struct stat sb;
    return stat(path.c_str(), &sb) == 0;
}

bool filesystem::move(const std::string &from, const std::string &to)
{
    return rename(from.c_str(), to.c_str()) == 0;
}

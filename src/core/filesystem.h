/*
 *  filesystem.h
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

#ifndef AMSYNTH_FILESYSTEM_H
#define AMSYNTH_FILESYSTEM_H

#include <string>

class filesystem
{
public:

    static filesystem& get();

    // data
    std::string factory_banks;
    std::string skins;

    // prefs
    std::string config;
    std::string controllers;
    std::string default_bank;
    std::string user_banks;

private:

    filesystem();

    bool copy(const std::string &from, const std::string &to);

    bool create_dir(const std::string &path);

    bool exists(const std::string &path);

    bool move(const std::string &from, const std::string &to);
};

#endif //AMSYNTH_FILESYSTEM_H

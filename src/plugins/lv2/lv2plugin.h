/*
 *  lv2plugin.h
 *
 *  Copyright (c) 2012 Nick Dowell
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

#ifndef AMSYNTH_LV2_H
#define AMSYNTH_LV2_H

#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/midi/midi.h"
#include "lv2/patch/patch.h"
#include "lv2/state/state.h"
#include "lv2/ui/ui.h"
#include "lv2/worker/worker.h"

#define AMSYNTH_LV2_URI             "http://code.google.com/p/amsynth/amsynth"
#define AMSYNTH_LV2UI_URI           "http://code.google.com/p/amsynth/amsynth/ui"

#define FOR_EACH_PROPERTY(X) \
	X(max_polyphony) \
	X(midi_channel) \
	X(pitch_bend_range) \
	X(preset_bank_name) \
	X(preset_name) \
	X(preset_number) \
	X(tuning_kbm_file) \
	X(tuning_scl_file) \
	X(tuning_mts_esp_disabled)

enum {
    PORT_CONTROL            = 0,
    PORT_NOTIFY             = 1,
    PORT_AUDIO_L            = 2,
    PORT_AUDIO_R            = 3,
    PORT_FIRST_PARAMETER    = 4,
};

#endif //AMSYNTH_LV2_H

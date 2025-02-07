/*
 *  PresetController.h
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

#ifndef _PRESETCONTROLLER_H
#define _PRESETCONTROLLER_H

#include <set>
#include <stack>
#include <string>
#include <vector>

#include "Preset.h"

struct BankInfo {
	~BankInfo();

	std::string name;
	std::string file_path;
	bool read_only;
	Preset presets[128];
};

class PresetController final : private Parameter::Observer {
public:

	static constexpr int kNumPresets = 128;

	PresetController();

	class Observer {
	public:
		virtual void currentPresetDidChange() = 0;
	protected:
		~Observer() = default;
	};
	
	/* Selects a Preset and makes it current, updating everything as necessary.
	 * If the requested preset does not exist, then the request is ignored, and
	 * an error value is returned. */
	int		selectPreset		(const int preset);

	// returns the preset currently being edited
	Preset&	getCurrentPreset	() { return currentPreset; }
	
	// Assign a new preset without changing `currentPresetNo` - useful if not
	// using the standard bank management mechanism.
	void	setCurrentPreset	(const Preset &preset) { currentPreset = preset; }
	
	// access presets in the memory bank
	Preset&	getPreset			(int preset) { return presets[preset]; }

	bool	containsPresetWithName(const std::string name);
	bool	isCurrentPresetModified() { return currentPresetNo != -1 && !currentPreset.isEqual(presets[currentPresetNo]); }
	
	// Commit the current preset to memory
	void	commitPreset		() { presets[currentPresetNo] = currentPreset; notify(); }

	void	saveCurrentPreset	();

	// Resets all parameters to default value and clears the name.
	void	clearPreset			();

	// Manages undo/redo for changes to current preset.
	void	undoChange			();
	void	redoChange			();

	// Randomises the current preset.
	void	randomiseCurrentPreset	();
	
	// Saves the active preset to the filename filename
	int		exportPreset		(const std::string filename);
	int		importPreset		(const std::string filename);
	
	// Loading & Saving of bank files - NOT REALTIME SAFE
	int		loadPresets			(const char *filename = NULL);
	int		savePresets			(const char *filename = NULL);

	// Switch bank at runtime - safe to call on audio thread
	void	selectBank			(int bankNumber);

	void	addObserver			(Observer *observer) { observers.insert(observer); }
	void	removeObserver		(Observer *observer) { observers.erase(observer); }

    int		getCurrPresetNumber	() { return currentPresetNo; }
	void	setCurrPresetNumber (int num) { currentPresetNo = num; }

	const std::string & getFilePath() { return bank_file; }

	static const std::vector<BankInfo> & getPresetBanks();
	static void rescanPresetBanks();

    static std::string getUserBanksDirectory();

	static bool createUserBank(const std::string &name);

	void	notify				() {
		for (auto observer : observers)
			observer->currentPresetDidChange();
	}

private:
	std::string		bank_file;
	std::set<Observer *> observers;
	Preset			presets[kNumPresets];
	Preset 			currentPreset;
	Preset			blankPreset;
	Preset 			nullpreset{"null preset"};
	int				currentBankNo = -1;
	int 			currentPresetNo = -1;
	long int 		lastPresetsFileModifiedTime = 0;

	// Parameter::Observer
	void parameterBeginEdit(const Parameter &) final;

	class ChangeData {
		public:
            virtual ~ChangeData() {};
			virtual void initiateUndo( PresetController * ) = 0;
			virtual void initiateRedo( PresetController * ) = 0;
	};

	class ParamChange: public ChangeData {
		public:
			Param param;
			float value;

			ParamChange(Param nParam, float nValue)
					:param(nParam),
					value(nValue) {}

			void initiateUndo(PresetController *presetController) override {
				presetController->undoChange(this);
			}

			void initiateRedo(PresetController *presetController) override {
				presetController->redoChange(this);
			}
	};

	class RandomiseChange: public ChangeData {
		public:
			Preset preset;

			RandomiseChange(Preset &nPreset) {
				preset = nPreset; // Uses operator override.
			}

			void initiateUndo(PresetController *presetController) override {
				presetController->undoChange(this);
			}

			void initiateRedo(PresetController *presetController) override {
				presetController->redoChange(this);
			}
	};

	void undoChange	( ParamChange * );
	void undoChange	( RandomiseChange * );
	void redoChange	( ParamChange * );
	void redoChange	( RandomiseChange * );

	std::stack<ChangeData *>	undoBuffer;
	std::stack<ChangeData *>	redoBuffer;
	void clearUndoBuffer	() { while( !undoBuffer.empty() ) { delete undoBuffer.top(); undoBuffer.pop(); } }
	void clearRedoBuffer	() { while( !redoBuffer.empty() ) { delete redoBuffer.top(); redoBuffer.pop(); } }
	void clearChangeBuffers	() { clearUndoBuffer(); clearRedoBuffer(); }

};

#endif

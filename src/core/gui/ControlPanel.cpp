/*
 *  ControlPanel.cpp
 *
 *  Copyright (c) 2022 Nick Dowell
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

#include "ControlPanel.h"

#include "Controls.h"
#include "LayoutDescription.h"
#include "core/Configuration.h"
#include "core/filesystem.h"
#include "core/synth/MidiController.h"
#include "core/synth/PresetController.h"
#include "seq24/controllers.h"

#include <cstdlib>
#include <utility>
#include <vector>

class Skin
{
public:
	explicit Skin(const std::string &path)
	: directory(path)
	, layout(directory.getChildFile("layout.ini").getFullPathName().toStdString())
	{}

	juce::File directory;
	LayoutDescription layout;

	juce::File getBackground() const
	{
		return directory.getChildFile(layout.background);
	}

	juce::Image getImage(const LayoutDescription::Resource& resource)
	{
		if (images_.find(resource.file) != images_.end()) {
			return images_.at(resource.file);
		} else {
			auto path = directory.getChildFile(resource.file).getFullPathName();
			return images_[resource.file] = juce::ImageFileFormat::loadFrom(path);
		}
	}

private:
	std::unordered_map<std::string, juce::Image> images_;
};

struct ControlPanel::Impl final : juce::MouseListener, juce::Timer
{
	Impl(ControlPanel *controlPanel, MidiController *midiController, PresetController *presetController)
	: midiController_(midiController)
	, presetController_(presetController)
	, label_(controlPanel)
	{
		skinDir_ = filesystem::get().skins + "/default";
		if (const char *path = getenv("AMSYNTH_SKIN"))
			skinDir_ = path;
		else
			label_.yInset = 6;

		auto skin = Skin(skinDir_);
		if (skin.layout.background.empty() || skin.layout.controls.empty()) {
			controlPanel->setSize(600, 400);
			return;
		}

		auto background = juce::Drawable::createFromImageFile(skin.getBackground());
		background->setOpaque(true);
		controlPanel->setOpaque(true);
		controlPanel->addAndMakeVisible(background.get());
		controlPanel->setSize(background->getWidth(), background->getHeight());
		components_.push_back(std::move(background));

		Control::isMainThread = true;

		for (int i = 0; i < kAmsynthParameterCount; i++) {
			auto &parameter = presetController_->getCurrentPreset().getParameter(i);

			const auto &control = skin.layout.controls.at(parameter.getName());
			const auto &resource = control.resource;
			auto image = skin.getImage(resource);

			std::unique_ptr<Control> component;
			if (control.type == "button") {
				component = std::make_unique<Button>(parameter, image, resource);
			}
			if (control.type == "knob") {
				component = std::make_unique<Knob>(parameter, image, resource, &label_);
			}
			if (control.type == "popup") {
				component = std::make_unique<Popup>(parameter, image, resource);
			}
			if (component) {
				component->setTopLeftPosition(control.x, control.y);
				component->addMouseListener(this, false);
				controlPanel->addAndMakeVisible(component.get());
				components_.push_back(std::move(component));
			}
		}

		startTimer(1000 / 30);
	}

	void mouseDown(const juce::MouseEvent& event) final {
		if (event.mods.isPopupMenu()) {
			showPopupMenu(dynamic_cast<Control *>(event.originalComponent));
		}
	}

	void showPopupMenu(Control *control) {
		auto paramId = control->parameter.getId();
		juce::PopupMenu menu;

		if (midiController_) {
			juce::PopupMenu ccSubmenu;
			int cc = midiController_->getControllerForParameter(paramId);
			for (int i = 0; i < 128; i++) {
				ccSubmenu.addItem(c_controller_names[i], true, i == cc, [this, paramId, i] {
					midiController_->setControllerForParameter(paramId, i);
				});
			}
			ccSubmenu.addItem(GETTEXT("None"), [this, paramId] {
				midiController_->setControllerForParameter(paramId, -1);
			});
			menu.addSubMenu(GETTEXT("Assign MIDI CC"), ccSubmenu, true, nullptr, cc != -1);
		}

		bool isLocked = Preset::isParameterLocked(paramId);
		menu.addItem(GETTEXT("Lock Parameter"), true, isLocked, [isLocked, paramId] {
			Preset::setParameterLocked(paramId, !isLocked);
			Configuration &config = Configuration::get();
			config.locked_parameters = Preset::getLockedParameterNames();
			config.save();
		});
		menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(control));
	}

	void timerCallback() final {
		for (auto &it : components_) {
			auto control = dynamic_cast<Control *>(it.get());
			if (control) {
				control->repaintIfNeeded();
			}
		}
	}

	std::vector<std::unique_ptr<juce::Component>> components_;
	MidiController *midiController_;
	PresetController *presetController_;
	Knob::Label label_;
	std::string skinDir_;
};

ControlPanel::ControlPanel(MidiController *midiController, PresetController *presetController)
: impl_(std::make_unique<Impl>(this, midiController, presetController)) {}

ControlPanel::~ControlPanel() noexcept = default;

void ControlPanel::paint(juce::Graphics &g)
{
	if (impl_->components_.empty()) {
		g.setFont(15.f);
		g.setColour(findColour(juce::Label::textColourId));
		g.drawFittedText("Error: could not load "  + impl_->skinDir_ + "/default/layout.ini",
						 20, 20, getWidth() - 40, getHeight() - 40,
						 juce::Justification::horizontallyCentred | juce::Justification::verticallyCentred, 2);
	} else {
		g.fillAll(juce::Colours::black);
	}
}

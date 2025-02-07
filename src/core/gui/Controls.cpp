/*
 *  Controls.cpp
 *
 *  Copyright (c) 2023 Nick Dowell
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

#include "Controls.h"

#include "core/Configuration.h"
#include "core/synth/Preset.h"

Control::Control(Parameter &p, juce::Image image, const LayoutDescription::Resource &r)
: parameter(p)
, frame_(0)
, image_(std::move(image))
, width_(r.width)
, height_(r.height)
, frames_(r.frames) {
	setSize(width_, height_);
	parameter.addObserver(this);
}

Control::~Control() { parameter.removeObserver(this); }

void Control::mouseDown(const juce::MouseEvent &event) {
	if (event.mods.isLeftButtonDown()) {
		leftMouseDown(event);
	}
}

void Control::mouseDoubleClick(const juce::MouseEvent &event) {
	if (event.mods.isLeftButtonDown()) {
		parameter.setValue(parameter.getDefault());
	}
}

void Control::paint(juce::Graphics &g) {
	int x = 0, y = 0;
	if (image_.getHeight() == height_) {
		x = frame_ * width_;
	} else {
		y = frame_ * height_;
	}
	g.drawImage(image_, 0, 0, width_, height_, x, y, width_, height_, false);
}

void Control::parameterDidChange(const Parameter &) {
	int frame = int(float(frames_ - 1) * parameter.getNormalisedValue());
	if (frame_ == frame) {
		return;
	}
	frame_ = frame;
	repaintFromAnyThread();
}

void Control::repaintFromAnyThread() {
	auto mm = juce::MessageManager::getInstanceWithoutCreating();
	if (mm && mm->isThisTheMessageThread()) {
		repaint();
	} else {
		juce::MessageManager::callAsync([this] { repaint(); });
	}
}

////////////////////////////////////////////////////////////////////////////////

Knob::Knob(Parameter &parameter, juce::Image image, const LayoutDescription::Resource &r, Label *label)
: Control(parameter, std::move(image), r)
, label_(label) {}

juce::String Knob::getLabelText() {
	char text[32] = "";
	return parameter_get_display(parameter.getId(), parameter.getValue(), text, sizeof text) ? text : "";
}

void Knob::mouseEnter(const juce::MouseEvent &) { label_->show(this, getLabelText()); }

void Knob::mouseExit(const juce::MouseEvent &) { label_->hide(); }

void Knob::mouseUp(const juce::MouseEvent &event) {
	if (event.mods.isLeftButtonDown())
		parameter.endEdit();
}

void Knob::leftMouseDown(const juce::MouseEvent &event) {
	referenceVal_ = parameter.getNormalisedValue();
	referenceY_ = event.y;
	parameter.beginEdit();
}

void Knob::mouseDrag(const juce::MouseEvent &event) {
	Control::mouseDrag(event);
	if (event.mods.isPopupMenu()) {
		return;
	}
	float sensitivity;
	if (parameter.getStep() == 0.f) {
		sensitivity = 300 * (event.mods.isCtrlDown() ? 4.f : 1.f) * (event.mods.isShiftDown() ? 4.f : 1.f);
	} else {
		sensitivity = std::min(40 * float(parameter.getSteps()), 480.f);
	}

	auto offset = float(referenceY_ - event.y) / sensitivity;
	auto newVal = referenceVal_ + offset;
	if (newVal != referenceVal_) {
		parameter.setNormalisedValue(referenceVal_ + offset);
		label_->show(this, getLabelText());
		referenceVal_ = newVal;
		referenceY_ = event.y;
	}
}

void Knob::mouseWheelMove(const juce::MouseEvent &event, const juce::MouseWheelDetails &wheel) {
	if (event.mouseWasDraggedSinceMouseDown()) {
		return;
	}
	auto delta = (wheel.deltaY / 2.f / (event.mods.isCtrlDown() ? 4.f : 1.f) / (event.mods.isShiftDown() ? 4.f : 1.f));
	parameter.setNormalisedValue(parameter.getNormalisedValue() + delta);
	label_->show(this, getLabelText());
}

Knob::Label::Label(juce::Component *parent)
: parent_(parent) {
	setAlwaysOnTop(true);
	setOpaque(true);
	setVisible(false);
	parent->addChildComponent(this);
}

void Knob::Label::show(juce::Component *control, juce::String text) {
	if (control_ && control_ != control)
		return;
	control_ = control;

	text_ = text;
	if (text.isEmpty()) {
		setVisible(false);
		return;
	}

	auto ttb = getLookAndFeel().getTooltipBounds(text, control->getPosition(), parent_->getLocalBounds());
	auto width = std::max(control->getWidth(), ttb.getWidth());
	auto inset = (control->getWidth() - width) / 2;
	setBounds(control->getX() + inset, control->getBottom() - yInset, width, ttb.getHeight());
	setVisible(true);
	repaint();
}

void Knob::Label::hide() {
	setVisible(false);
	control_ = nullptr;
	text_ = "";
}

void Knob::Label::paint(juce::Graphics &graphics) { getLookAndFeel().drawTooltip(graphics, text_, getWidth(), getHeight()); }

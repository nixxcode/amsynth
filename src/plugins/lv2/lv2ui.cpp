/*
 *  lv2ui.c
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

////////////////////////////////////////////////////////////////////////////////

#include "lv2plugin.h"

#include "core/controls.h"
#include "core/gui/MainComponent.h"
#include "core/gui/JuceIntegration.h"
#include "core/synth/PresetController.h"
#include "core/synth/Synthesizer.h"

#include <cstring>
#include <memory>
#include <utility>

////////////////////////////////////////////////////////////////////////////////

struct ParameterListener final : public Parameter::Observer {
	ParameterListener(PresetController *presetController, std::function<void(int, float)> &&writeFn, std::function<void(int, bool)> &&touchFn)
	: presetController(presetController), writeFunc(std::move(writeFn)), touchFunc(std::move(touchFn)) {
		presetController->getCurrentPreset().addObserver(this);
		active = true;
	}

	~ParameterListener() {
		for (int i = 0; i < kAmsynthParameterCount; i++) {
			presetController->getCurrentPreset().getParameter(i).removeObserver(this);
		}
	}

	void parameterDidChange(const Parameter &parameter) final {
		if (active) writeFunc(parameter.getId(), parameter.getValue());
	}

	void parameterBeginEdit(const Parameter &parameter) final {
		touchFunc(parameter.getId(), true);
	}

	void parameterEndEdit(const Parameter &parameter) final {
		touchFunc(parameter.getId(), false);
	}

	PresetController *presetController;
	std::function<void(int, float)> writeFunc;
	std::function<void(int, bool)> touchFunc;
	bool active{false};
};

////////////////////////////////////////////////////////////////////////////////

struct lv2_ui {
	PresetController presetController;
	JuceIntegration juceIntegration;
	std::unique_ptr<MainComponent> mainComponent;
	std::unique_ptr<ParameterListener> parameterListener;
	LV2UI_Widget parent {nullptr};

	LV2_Atom_Forge forge;
	LV2_URID_Map *map;
	LV2UI_Write_Function _write_function;
	LV2UI_Controller _controller;

	struct {
		LV2_URID atom_Float;
		LV2_URID atom_Path;
		LV2_URID atom_Resource;
		LV2_URID atom_Sequence;
		LV2_URID atom_String;
		LV2_URID atom_URID;
		LV2_URID atom_eventTransfer;
		LV2_URID midi_Event;
		LV2_URID patch_Get;
		LV2_URID patch_Set;
		LV2_URID patch_property;
		LV2_URID patch_value;
#define DECLARE_LV2_URID(name) LV2_URID amsynth_##name;
		FOR_EACH_PROPERTY(DECLARE_LV2_URID)
	} uris;
};

////////////////////////////////////////////////////////////////////////////////

struct lv2helper
{
	lv2helper(lv2_ui *ui_): ui(ui_) {}

	void send(const char *name, const char *value)
	{
#define SET_PROPERTY(Name) if (!strcmp(name, #Name)) send(ui->uris.amsynth_##Name, value);
		FOR_EACH_PROPERTY(SET_PROPERTY)
	}

	void send(LV2_URID key, const char *value)
	{
		uint8_t buffer[1024];

		LV2_Atom_Forge_Frame frame;
		LV2_Atom_Forge *forge = &ui->forge;
		lv2_atom_forge_set_buffer(forge, buffer, sizeof(buffer));
		LV2_Atom *msg = (LV2_Atom *) lv2_atom_forge_object(forge, &frame, 0, ui->uris.patch_Set);
		lv2_atom_forge_key(forge, ui->uris.patch_property);
		lv2_atom_forge_urid(forge, key);
		lv2_atom_forge_key(forge, ui->uris.patch_value);
		lv2_atom_forge_string(forge, value, (uint32_t)strlen(value));
		lv2_atom_forge_pop(forge, &frame);

		ui->_write_function(
				ui->_controller,
				PORT_CONTROL,
				lv2_atom_total_size(msg),
				ui->uris.atom_eventTransfer,
				msg);
	}

	void getProperties()
	{
		auto getProp = [&] (LV2_URID key) {
			uint8_t buffer[1024];

			LV2_Atom_Forge_Frame frame;
			LV2_Atom_Forge *forge = &ui->forge;
			lv2_atom_forge_set_buffer(forge, buffer, sizeof(buffer));
			LV2_Atom *msg = (LV2_Atom *) lv2_atom_forge_object(forge, &frame, 0, ui->uris.patch_Get);
			lv2_atom_forge_key(forge, ui->uris.patch_property);
			lv2_atom_forge_urid(forge, key);
			lv2_atom_forge_pop(forge, &frame);

			ui->_write_function(
					ui->_controller,
					PORT_CONTROL,
					lv2_atom_total_size(msg),
					ui->uris.atom_eventTransfer,
					msg);
		};
#define GET_PROP(name) getProp(ui->uris.amsynth_##name);
		FOR_EACH_PROPERTY(GET_PROP)
	}

	lv2_ui *ui;
};

////////////////////////////////////////////////////////////////////////////////

static LV2UI_Handle
lv2_ui_instantiate(const LV2UI_Descriptor*         /*descriptor*/,
				   const char*                     /*plugin_uri*/,
				   const char*                     /*bundle_path*/,
				   LV2UI_Write_Function            write_function,
				   LV2UI_Controller                controller,
				   LV2UI_Widget*                   widget,
				   const LV2_Feature* const*       features)
{
	lv2_ui *ui = new lv2_ui();

	LV2UI_Resize *resize {nullptr};
	LV2UI_Touch *touch {nullptr};

	for (auto f = features; *f; f++) {
		if (!strcmp((*f)->URI, LV2_UI__parent))
			ui->parent = reinterpret_cast<LV2UI_Widget>((*f)->data);
		if (!strcmp((*f)->URI, LV2_URID__map))
			ui->map = reinterpret_cast<LV2_URID_Map *>((*f)->data);
		if (!strcmp((*f)->URI, LV2_UI__touch))
			touch = reinterpret_cast<LV2UI_Touch *>((*f)->data);
		if (!strcmp((*f)->URI, LV2_UI__resize))
			resize = reinterpret_cast<LV2UI_Resize *>((*f)->data);
	}

	if (!ui->map) {
		delete ui;
		return nullptr;
	}

	ui->uris.atom_Float         = ui->map->map(ui->map->handle, LV2_ATOM__Float);
	ui->uris.atom_Path          = ui->map->map(ui->map->handle, LV2_ATOM__Path);
	ui->uris.atom_Resource      = ui->map->map(ui->map->handle, LV2_ATOM__Resource);
	ui->uris.atom_Sequence      = ui->map->map(ui->map->handle, LV2_ATOM__Sequence);
	ui->uris.atom_String        = ui->map->map(ui->map->handle, LV2_ATOM__String);
	ui->uris.atom_URID          = ui->map->map(ui->map->handle, LV2_ATOM__URID);
	ui->uris.atom_eventTransfer = ui->map->map(ui->map->handle, LV2_ATOM__eventTransfer);
	ui->uris.midi_Event         = ui->map->map(ui->map->handle, LV2_MIDI__MidiEvent);
	ui->uris.patch_Get          = ui->map->map(ui->map->handle, LV2_PATCH__Get);
	ui->uris.patch_Set          = ui->map->map(ui->map->handle, LV2_PATCH__Set);
	ui->uris.patch_property     = ui->map->map(ui->map->handle, LV2_PATCH__property);
	ui->uris.patch_value        = ui->map->map(ui->map->handle, LV2_PATCH__value);
#define MAP_URID(Name) ui->uris.amsynth_##Name = ui->map->map(ui->map->handle, AMSYNTH_LV2_URI "#" #Name);
	FOR_EACH_PROPERTY(MAP_URID)

	ui->_write_function = write_function;
	ui->_controller = controller;

	lv2_atom_forge_init(&ui->forge, ui->map);

	ui->parameterListener = std::make_unique<ParameterListener>(&ui->presetController, [=] (int idx, float value) {
		write_function(controller, PORT_FIRST_PARAMETER + idx, sizeof(float), 0, &value);
	}, [touch] (int idx, bool grabbed) {
		if (touch) touch->touch(touch->handle, PORT_FIRST_PARAMETER + idx, grabbed);
	});

	ui->mainComponent = std::make_unique<MainComponent>(&ui->presetController);
	ui->mainComponent->sendProperty = [ui] (const char *name, const char *value) {lv2helper(ui).send(name, value);};
	ui->mainComponent->addToDesktop(juce::ComponentPeer::windowIgnoresKeyPresses, ui->parent);
	ui->mainComponent->setVisible(true);
	if (resize) {
		auto bounds = ui->mainComponent->getScreenBounds();
		auto scaleFactor = (int)juce::Desktop::getInstance().getGlobalScaleFactor();
		resize->ui_resize(resize->handle, bounds.getWidth() * (int)scaleFactor, bounds.getHeight() * (int)scaleFactor);
	}
	*widget = ui->mainComponent->getWindowHandle();

	lv2helper(ui).getProperties();

	return ui;
}

static void
lv2_ui_cleanup(LV2UI_Handle ui)
{
	((lv2_ui *)ui)->mainComponent->removeFromDesktop();
	delete ((lv2_ui *)ui);
}

static void
lv2_ui_port_event(LV2UI_Handle handle,
				  uint32_t     port_index,
				  uint32_t     /*buffer_size*/,
				  uint32_t     format,
				  const void*  buffer)
{
	lv2_ui *ui = (lv2_ui *)handle;
	if (format == ui->uris.atom_eventTransfer) {
		auto atom = (const LV2_Atom *)buffer;
		if (lv2_atom_forge_is_object_type(&ui->forge, atom->type)) {
			auto obj = (const LV2_Atom_Object *)atom;
			if (obj->body.otype == ui->uris.patch_Set) {
				const LV2_Atom *property = NULL;
				const LV2_Atom *value = NULL;
				lv2_atom_object_get(obj,
									ui->uris.patch_property, &property,
									ui->uris.patch_value, &value,
									0);
				if (property && value && value->type == ui->uris.atom_String) {
					LV2_URID urid = ((LV2_Atom_URID *)(void *)property)->body;
#define PATCH_SET_PROP(Name) \
					if (ui->uris.amsynth_##Name == urid) \
						ui->mainComponent->propertyChanged(#Name, (const char *)LV2_ATOM_BODY_CONST(value));
					FOR_EACH_PROPERTY(PATCH_SET_PROP)
				}
			}
		}
	} else if (port_index >= PORT_FIRST_PARAMETER) {
		ui->parameterListener->active = false;
		ui->presetController.getCurrentPreset().getParameter(port_index - PORT_FIRST_PARAMETER).setValue(*(float *)buffer);
		ui->parameterListener->active = true;
	}
}

////////////////////////////////////////////////////////////////////////////////

static int
lv2_ui_idle(LV2UI_Handle handle)
{
	reinterpret_cast<lv2_ui *>(handle)->juceIntegration.idle();
	return 0;
}

static const void *
lv2_ui_extension_data(const char *uri)
{
	if (!strcmp(uri, LV2_UI__idleInterface)) {
		static LV2UI_Idle_Interface idleInterface {
			.idle = &lv2_ui_idle
		};
		return &idleInterface;
	}

	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

static const
LV2UI_Descriptor descriptor = {
	AMSYNTH_LV2UI_URI,
	&lv2_ui_instantiate,
	&lv2_ui_cleanup,
	&lv2_ui_port_event,
	&lv2_ui_extension_data,
};

LV2_SYMBOL_EXPORT
const LV2UI_Descriptor *
lv2ui_descriptor(uint32_t index)
{
	if (index == 0) {
		return &descriptor;
	}
	return nullptr;
}

////////////////////////////////////////////////////////////////////////////////

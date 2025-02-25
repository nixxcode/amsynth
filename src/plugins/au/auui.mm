/*
 *  auui.mm
 *
 *  Copyright (c) 2025 Nick Dowell
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

#import "auplugin.h"

#import "core/gui/ControlPanel.h"
#import "core/gui/MainComponent.h"
#import "core/synth/PresetController.h"

#import <AppKit/AppKit.h>
#import <AudioUnit/AUCocoaUIView.h>

@interface AmsynthAUCocoaUI : NSObject <AUCocoaUIBase>
@end

struct ParameterListener final : Parameter::Observer {
	ParameterListener(AudioUnit audioUnit, PresetController *presetController)
	: audioUnit_(audioUnit), presetController_(presetController)
	{
		AUListenerCreate(auParameterListenerProc, this, nullptr, nullptr, 0.f, &audioUnitListener_);

		for (int i = 0; i < kAmsynthParameterCount; i++) {
			AudioUnitParameter auParameter = {
				.mAudioUnit = audioUnit_,
				.mParameterID = (AudioUnitParameterID)i,
				.mScope = kAudioUnitScope_Global,
				.mElement = 0};
			auto &parameter = presetController_->getCurrentPreset().getParameter(i);
			AudioUnitParameterValue value = 0;
			AudioUnitGetParameter(audioUnit_, i, kAudioUnitScope_Global, 0, &value);
			parameter.setValue(value);
			AUListenerAddParameter(audioUnitListener_, &parameter, &auParameter);
		}

		presetController_->getCurrentPreset().addObserver(this, false);
	}

	~ParameterListener()
	{
		AUListenerDispose(audioUnitListener_);
	}

	void parameterDidChange(const Parameter &param) final
	{
		const AudioUnitParameter auParameter = {audioUnit_, param.getId(), kAudioUnitScope_Global, 0};
		AUParameterSet(audioUnitListener_, NULL, &auParameter, param.getValue(), 0); // notifies parameter & event listeners
	}

	void parameterBeginEdit(const Parameter &param) final
	{
		notify(param, kAudioUnitEvent_BeginParameterChangeGesture);
	}

	void parameterEndEdit(const Parameter &param) final
	{
		notify(param, kAudioUnitEvent_EndParameterChangeGesture);
	}

	void notify(const Parameter &param, AudioUnitEventType eventType)
	{
		AudioUnitEvent auEvent;
		auEvent.mArgument.mParameter.mAudioUnit = audioUnit_;
		auEvent.mArgument.mParameter.mParameterID = param.getId();
		auEvent.mArgument.mParameter.mScope = kAudioUnitScope_Global;
		auEvent.mArgument.mParameter.mElement = 0;
		auEvent.mEventType = eventType;
		AUEventListenerNotify(audioUnitListener_, NULL, &auEvent);
	}

	static void auParameterListenerProc(void *inUserData, void *inObject, const AudioUnitParameter *, AudioUnitParameterValue inValue)
	{
		reinterpret_cast<Parameter *>(inObject)->setValue(inValue, reinterpret_cast<ParameterListener *>(inUserData));
	}

	AudioUnit audioUnit_;
	PresetController *presetController_;
	AUEventListenerRef audioUnitListener_ {nullptr};
};

__attribute__((objc_direct_members))
@interface AmsynthAUView : NSView
@end

@implementation AmsynthAUView {
	AudioUnit _audioUnit;
	juce::ScopedJuceInitialiser_GUI libraryInitialiser;
	MainComponent *_component;
	std::unique_ptr<ParameterListener> _parameterListener;
	PresetController _presetController;
}

- (id)initWithAudioUnit:(AudioUnit)audioUnit
{
	if ((self = [super init])) {
		_audioUnit = audioUnit;

		_component = new MainComponent(&_presetController);
		_component->sendProperty = [audioUnit](const char *name, const char *value) {
			auto props = @{@(name): @(value)};
			AudioUnitSetProperty(audioUnit, kAudioUnitProperty_AmsynthProperties, kAudioUnitScope_Global, 0, &props, sizeof(props));
		};

		CFDictionaryRef properties {nullptr};
		UInt32 propertiesSize {sizeof(properties)};
		AudioUnitGetProperty(audioUnit, kAudioUnitProperty_AmsynthProperties, 0, kAudioUnitScope_Global, (void *)&properties, &propertiesSize);
		NSCParameterAssert(properties);
		for (NSString *key in (__bridge NSDictionary *)properties)
			_component->propertyChanged([key UTF8String], [((__bridge NSDictionary *)properties)[key] UTF8String]);
		CFRelease(properties);

		_parameterListener = std::make_unique<ParameterListener>(audioUnit, &_presetController);

		_component->addToDesktop(juce::ComponentPeer::windowIgnoresKeyPresses, self);
		_component->setVisible(true);

		[self setFrame:CGRectMake(0, 0, _component->getWidth(), _component->getHeight())];
	}
	return self;
}

- (void)dealloc
{
	_component->removeFromDesktop();
	delete _component; // Ensure this is deleted before juce::ScopedJuceInitialiser_GUI
	[super dealloc];
}

@end

@implementation AmsynthAUCocoaUI

- (unsigned)interfaceVersion
{
	return 0;
}

- (NSView *)uiViewForAudioUnit:(AudioUnit)inAudioUnit withSize:(NSSize)inPreferredSize
{
	return [[[AmsynthAUView alloc] initWithAudioUnit:inAudioUnit] autorelease];
}

- (NSString *)description
{
	return @"amsynth";
}

@end

OSStatus GetCococaUI(AudioUnitCocoaViewInfo *info)
{
	info->mCocoaAUViewBundleLocation = (CFURLRef)CFBridgingRetain([[NSBundle bundleForClass:[AmsynthAUCocoaUI class]] bundleURL]);
	info->mCocoaAUViewClass[0] = CFSTR("AmsynthAUCocoaUI");
	return noErr;
}

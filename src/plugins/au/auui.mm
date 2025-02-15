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
	ParameterListener(AudioUnit audioUnit, AUEventListenerRef audioUnitListener, PresetController *presetController)
	: audioUnit_(audioUnit), audioUnitListener_(audioUnitListener), presetController_(presetController) {}

	void parameterDidChange(const Parameter &param) final
	{
		const AudioUnitParameter auParameter = {
			.mAudioUnit = audioUnit_,
			.mParameterID = param.getId(),
			.mScope = kAudioUnitScope_Global,
			.mElement = 0};
		AUParameterSet(audioUnitListener_, NULL, &auParameter, param.getValue(), 0);
	}

	static void AUParameterListener(void *inUserData __unused, void *inObject, const AudioUnitParameter *inParameter __unused, AudioUnitParameterValue inValue)
	{
		assert(pthread_main_np());
		reinterpret_cast<Parameter *>(inObject)->setValue(inValue);
	}

	AudioUnit audioUnit_;
	AUEventListenerRef audioUnitListener_;
	PresetController *presetController_;
};

__attribute__((objc_direct_members))
@interface AmsynthAUView : NSView
@end

@implementation AmsynthAUView {
	AudioUnit _audioUnit;
	AUEventListenerRef _listenerRef;
	juce::ScopedJuceInitialiser_GUI libraryInitialiser;
	MainComponent *_component;
	std::unique_ptr<ParameterListener> _parameterListener;
	PresetController _presetController;
}

- (id)initWithAudioUnit:(AudioUnit)audioUnit
{
	if ((self = [super init])) {
		_audioUnit = audioUnit;

		if (ControlPanel::skinsDirectory.empty()) {
			ControlPanel::skinsDirectory = [[[[NSBundle bundleForClass:[self class]] URLForResource:@"skins" withExtension:nil] path] UTF8String];
		}

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

		_component->addToDesktop(juce::ComponentPeer::windowIgnoresKeyPresses, self);
		_component->setVisible(true);

		[self setFrame:CGRectMake(0, 0, _component->getWidth(), _component->getHeight())];

		AUListenerCreate(ParameterListener::AUParameterListener, self, CFRunLoopGetMain(), kCFRunLoopCommonModes, 1.f / 60.f, &_listenerRef);

		for (int i = 0; i < kAmsynthParameterCount; i++) {
			AudioUnitParameter auParameter = {
				.mAudioUnit = _audioUnit,
				.mParameterID = (AudioUnitParameterID)i,
				.mScope = kAudioUnitScope_Global,
				.mElement = 0};
			auto &parameter = _presetController.getCurrentPreset().getParameter(i);
			AudioUnitParameterValue value = 0;
			AudioUnitGetParameter(_audioUnit, i, kAudioUnitScope_Global, 0, &value);
			parameter.setValue(value);
			AUListenerAddParameter(_listenerRef, &parameter, &auParameter);
		}

		_parameterListener = std::make_unique<ParameterListener>(audioUnit, _listenerRef, &_presetController);
		_presetController.getCurrentPreset().addObserver(_parameterListener.get());
	}
	return self;
}

- (void)dealloc
{
	AUListenerDispose(_listenerRef);
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
	info->mCocoaAUViewBundleLocation = (CFURLRef)[[NSBundle bundleForClass:[AmsynthAUCocoaUI class]] bundleURL];
	info->mCocoaAUViewClass[0] = CFSTR("AmsynthAUCocoaUI");
	return noErr;
}

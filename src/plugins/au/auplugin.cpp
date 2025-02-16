/*
 *  auplugin.cpp
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

#include "auplugin.h"

#include "core/midi.h"
#include "core/synth/Preset.h"
#include "core/synth/PresetController.h"
#include "core/synth/Synthesizer.h"
#include "core/synth/VoiceAllocationUnit.h"
#include "MusicDeviceBase.h"

#include <vector>

#define kPropertiesKey CFSTR("_properties")
#define MIDI_BUFFER_SIZE 4096

class AmsynthAU : public MusicDeviceBase
{
public:
	AmsynthAU(AudioComponentInstance inInstance)
	: MusicDeviceBase(inInstance, 0, 1)
	{
		Globals()->UseIndexedParameters(kAmsynthParameterCount);
		AUElement *element = GetElement(kAudioUnitScope_Global, 0);
		for (int i = 0; i < kAmsynthParameterCount; i++) {
			float value = _synth.getParameterValue((Param)i);
			element->SetParameter(i, value);
		}
	}

	OSStatus Version() override { return 0; }

	bool CanScheduleParameters() const override { return false; }

	bool StreamFormatWritable(AudioUnitScope scope, AudioUnitElement element) override { return true; }

	OSStatus Initialize() override
	{
		if (GetOutput(0)->GetStreamFormat().mChannelsPerFrame != 2) {
			return kAudioUnitErr_FormatNotSupported;
		}
		_synth.setSampleRate(GetOutput(0)->GetStreamFormat().mSampleRate);
		_midiBuffer.resize(MIDI_BUFFER_SIZE);
		_midiBufferPtr = _midiBuffer.data();
		_midiEvents.reserve(MIDI_BUFFER_SIZE / 3);
		return noErr;
	}

	UInt32 SupportedNumChannels(const AUChannelInfo **outInfo) override
	{
		static const AUChannelInfo channelInfo[] = {{0, 2}};
		if (outInfo) {
			*outInfo = channelInfo;
		}
		return sizeof(channelInfo) / sizeof(channelInfo[0]);
	}

	OSStatus ChangeStreamFormat(AudioUnitScope inScope, AudioUnitElement inElement,
								const CAStreamBasicDescription &inPrevFormat,
								const CAStreamBasicDescription &inNewFormat) override
	{
		OSStatus status = MusicDeviceBase::ChangeStreamFormat(inScope, inElement, inPrevFormat, inNewFormat);
		if (status == noErr && inScope == kAudioUnitScope_Global) {
			_synth.setSampleRate(inNewFormat.mSampleRate);
		}
		return status;
	}

	OSStatus GetPropertyInfo(AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement,
							 UInt32 &outDataSize, Boolean &outWritable) override
	{
		switch (inID) {
			case kAudioUnitProperty_CocoaUI:
				outDataSize = sizeof(AudioUnitCocoaViewInfo);
				outWritable = false;
				return noErr;

			case kAudioUnitProperty_ParameterStringFromValue:
				outDataSize = sizeof(AudioUnitParameterStringFromValue);
				outWritable = false;
				return noErr;

			case kAudioUnitProperty_AmsynthProperties:
				outDataSize = sizeof(CFDictionaryRef);
				outWritable = true;
				return noErr;

			default:
				return MusicDeviceBase::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
		}
	}

	OSStatus GetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement,
						 void *outData) override
	{
		switch (inID) {
			case kAudioUnitProperty_CocoaUI:
				return GetCococaUI(reinterpret_cast<AudioUnitCocoaViewInfo *>(outData));

			case kAudioUnitProperty_ParameterStringFromValue:
				return GetParameterValueString(reinterpret_cast<AudioUnitParameterStringFromValue *>(outData));

			case kAudioUnitProperty_AmsynthProperties:
				*reinterpret_cast<CFDictionaryRef *>(outData) = CreatePropertiesDictionary();
				return noErr;

			default:
				return MusicDeviceBase::GetProperty(inID, inScope, inElement, outData);
		}
	}

	OSStatus SetProperty(AudioUnitPropertyID inID, AudioUnitScope inScope, AudioUnitElement inElement,
						 const void *inData, UInt32 inDataSize) override
	{
		switch (inID) {
			case kAudioUnitProperty_AmsynthProperties:
				ApplyPropertiesDictionary(*reinterpret_cast<const CFDictionaryRef *>(inData));
				return noErr;

			default:
				return MusicDeviceBase::SetProperty(inID, inScope, inElement, inData, inDataSize);
		}
	}

	CFDictionaryRef CreatePropertiesDictionary()
	{
		auto props = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
		for (auto &it : _synth.getProperties()) {
			auto key = CFStringCreateWithCString(NULL, it.first.c_str(), kCFStringEncodingUTF8);
			auto value = CFStringCreateWithCString(NULL, it.second.c_str(), kCFStringEncodingUTF8);
			CFDictionarySetValue(props, key, value);
			CFRelease(value);
			CFRelease(key);
		}
		return props;
	}

	void ApplyPropertiesDictionary(CFDictionaryRef props)
	{
		CFIndex count = CFDictionaryGetCount(props);
		std::vector<CFStringRef> keys(count);
		std::vector<CFStringRef> values(count);
		CFDictionaryGetKeysAndValues(props, (const void **)keys.data(), (const void **)values.data());
		for (CFIndex i = 0; i < count; i++) {
			char key[64];
			char value[PATH_MAX];
			CFStringGetCString(keys[i], key, sizeof(key), kCFStringEncodingUTF8);
			CFStringGetCString(values[i], value, sizeof(value), kCFStringEncodingUTF8);
			_synth.setProperty(key, value);
			if (!strcmp(key, PROP_NAME(preset_name))) {
				CFStringRef name = CFStringCreateWithCString(kCFAllocatorDefault, value, kCFStringEncodingUTF8);
				NewCustomPresetSet({-1, name});
				PropertyChanged(kAudioUnitProperty_PresentPreset, kAudioUnitScope_Global, 0);
				CFRelease(name);
			}
		}
	}

	OSStatus GetParameterInfo(AudioUnitScope inScope, AudioUnitParameterID inParameterID,
							  AudioUnitParameterInfo &outParameterInfo) override
	{
		if (inScope != kAudioUnitScope_Global || inParameterID >= kAmsynthParameterCount)
			return kAudioUnitErr_InvalidParameter;

		Preset defaultPreset;
		Parameter &parameter = defaultPreset.getParameter(inParameterID);

		memset(&outParameterInfo, 0, sizeof(outParameterInfo));
		outParameterInfo.cfNameString = CFStringCreateWithCString(0, parameter.getName(), kCFStringEncodingASCII);
		outParameterInfo.unit = kAudioUnitParameterUnit_Generic;
		outParameterInfo.minValue = parameter.getMin();
		outParameterInfo.maxValue = parameter.getMax();
		outParameterInfo.defaultValue = parameter.getValue();
		outParameterInfo.flags = kAudioUnitParameterFlag_HasCFNameString | kAudioUnitParameterFlag_CFNameRelease | kAudioUnitParameterFlag_IsReadable | kAudioUnitParameterFlag_IsWritable;

		char text[32] = "";
		if (parameter_get_display(inParameterID, outParameterInfo.defaultValue, text, sizeof(text)))
			outParameterInfo.flags |= kAudioUnitParameterFlag_HasName; // aka kAudioUnitParameterFlag_ValuesHaveStrings

		if (parameter_get_value_strings(inParameterID))
			outParameterInfo.unit = kAudioUnitParameterUnit_Indexed;
		else if (parameter.getSteps() == 1)
			outParameterInfo.unit = kAudioUnitParameterUnit_Boolean;

		return noErr;
	}

	OSStatus GetParameterValueString(AudioUnitParameterStringFromValue *info)
	{
		char text[32] = "";
		if (!parameter_get_display(info->inParamID, *info->inValue, text, sizeof(text)))
			return kAudioUnitErr_InvalidProperty;
		info->outString = CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8);
		return noErr;
	}

	OSStatus GetParameterValueStrings(AudioUnitScope inScope, AudioUnitParameterID inParameterID,
									  CFArrayRef *outStrings) override
	{
		if (outStrings) {
			const char **strings = parameter_get_value_strings(inParameterID);
			if (strings) {
				CFMutableArrayRef array = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
				for (const char **ptr = strings; ptr && *ptr; ptr++) {
					CFStringRef string = CFStringCreateWithCString(kCFAllocatorDefault, *ptr, kCFStringEncodingASCII);
					CFArrayAppendValue(array, (CFTypeRef)string);
					CFRelease(string);
				}
				*outStrings = array;
			}
		}
		return noErr;
	}

	OSStatus SetParameter(AudioUnitParameterID inID, AudioUnitScope inScope, AudioUnitElement inElement,
						  AudioUnitParameterValue inValue, UInt32 inBufferOffsetInFrames) override
	{
		if (inScope == kAudioUnitScope_Global && inID < kAmsynthParameterCount) {
			_synth.setParameterValue((Param)inID, inValue);
		}
		return MusicDeviceBase::SetParameter(inID, inScope, inElement, inValue, inBufferOffsetInFrames);
	}

	OSStatus SaveState(CFPropertyListRef *outData) override
	{
		OSStatus status = MusicDeviceBase::SaveState(outData);
		if (status == noErr && outData) {
			auto props = CreatePropertiesDictionary();
			CFDictionarySetValue(*(CFMutableDictionaryRef *)outData, kPropertiesKey, props);
			CFRelease(props);
		}
		return status;
	}

	OSStatus RestoreState(CFPropertyListRef inData) override
	{
		Preset defaultPreset;
		for (int i = 0; i < kAmsynthParameterCount; i++) {
			MusicDeviceBase::SetParameter(i, kAudioUnitScope_Global, 0, defaultPreset.getParameter(i).getValue(), 0);
		}
		OSStatus status = MusicDeviceBase::RestoreState(inData);
		if (status != noErr) {
			return status;
		}
		for (int i = 0; i < kAmsynthParameterCount; i++) {
			AudioUnitParameterValue value = 0;
			MusicDeviceBase::GetParameter(i, kAudioUnitScope_Global, 0, value);
			_synth.setParameterValue((Param)i, value);
		}
		if (auto props = (CFDictionaryRef)CFDictionaryGetValue((CFDictionaryRef)inData, kPropertiesKey)) {
			ApplyPropertiesDictionary(props);
		}
		return noErr;
	}

	OSStatus Render(AudioUnitRenderActionFlags &ioActionFlags, const AudioTimeStamp &inTimeStamp, UInt32 inNumberFrames) override
	{
		AUOutputElement *outputElement = GetOutput(0);
		AudioBufferList &outputBufferList = outputElement->GetBufferList();

		if (outputBufferList.mNumberBuffers != 2 ||
			outputBufferList.mBuffers[0].mNumberChannels != 1 ||
			outputBufferList.mBuffers[1].mNumberChannels != 1)
			return kAudioUnitErr_FormatNotSupported;

		std::vector<amsynth_midi_cc_t> midiOut;
		_synth.process(inNumberFrames, _midiEvents, midiOut, (float *)outputBufferList.mBuffers[0].mData, (float *)outputBufferList.mBuffers[1].mData);
		_midiBufferPtr = _midiBuffer.data();
		_midiEvents.clear();

		return noErr;
	}

	OSStatus MIDIEvent(UInt32 inStatus, UInt32 inData1, UInt32 inData2, UInt32 inOffsetSampleFrame) override
	{
		if (_midiBufferPtr + 3 > _midiBuffer.data() + _midiBuffer.size())
			return kAudioUnitErr_TooManyFramesToProcess;
		_midiEvents.push_back((amsynth_midi_event_t) {inOffsetSampleFrame, 3, _midiBufferPtr});
		_midiBufferPtr[0] = inStatus;
		_midiBufferPtr[1] = inData1;
		_midiBufferPtr[2] = inData2;
		_midiBufferPtr += 3;
		return noErr;
	}

private:
	Synthesizer _synth;
	unsigned char *_midiBufferPtr;
	std::vector<uint8_t> _midiBuffer;
	std::vector<amsynth_midi_event_t> _midiEvents;
};

AUDIOCOMPONENT_ENTRY(AUMusicDeviceFactory, AmsynthAU)

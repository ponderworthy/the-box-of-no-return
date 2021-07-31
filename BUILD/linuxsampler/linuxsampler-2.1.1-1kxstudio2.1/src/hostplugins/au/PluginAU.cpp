/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2009 - 2011 Grigor Iliev                                *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include "PluginAU.h"

#include "../../drivers/audio/AudioChannel.h"

#if AU_DEBUG_DISPATCHER
#include <AUDebugDispatcher.h>
#endif

COMPONENT_ENTRY(PluginAU)

//namespace LinuxSampler {

    const midi_chan_t PluginAU::midiChns[16] = {
        midi_chan_1,  midi_chan_2,  midi_chan_3,  midi_chan_4,  midi_chan_5,
        midi_chan_6,  midi_chan_7,  midi_chan_8,  midi_chan_9,  midi_chan_10,
        midi_chan_11, midi_chan_12, midi_chan_13, midi_chan_14, midi_chan_15,
        midi_chan_16
    };

    AUPreset  PluginAU::factoryPresets[kFactoryPresetCount] = {
        { kOneSamplerChannel, CFSTR("LS: 1 Channel") },
        { k16SamplerChannels, CFSTR("LS: 16 Channels") }
    };

    PluginAU::PluginAU(ComponentInstance inComponentInstance)
        : MusicDeviceBase(inComponentInstance, 0, 16), Plugin(false) {
    #if AU_DEBUG_DISPATCHER
        mDebugDispatcher = new AUDebugDispatcher(this);
    #endif
        charBufSize = 512;
        charBuf = new char[charBufSize];
        InstallListeners();

        SetAFactoryPresetAsCurrent(factoryPresets[kDefaultPreset]);
    }

    PluginAU::~PluginAU() {
    #if AU_DEBUG_DISPATCHER
        delete mDebugDispatcher;
    #endif
        delete [] charBuf;
        UninstallListeners();
    }

    void PluginAU::InstallListeners() {
        AddPropertyListener(kAudioUnitProperty_ContextName, PropertyListenerCallback, this);

    }

    void PluginAU::UninstallListeners() {
        RemovePropertyListener(kAudioUnitProperty_ContextName, PropertyListenerCallback, this, true);
    }

    void PluginAU::PropertyListenerCallback (
        void*                inRefCon,
        AudioUnit            ci,
        AudioUnitPropertyID  inID,
        AudioUnitScope       inScope,
        AudioUnitElement     inElement
    ) {
        ((PluginAU*)inRefCon)->PropertyChanged(inID, inScope, inElement);
    }

    void PluginAU::PropertyChanged (
        AudioUnitPropertyID  inID,
        AudioUnitScope       inScope,
        AudioUnitElement     inElement
    ) {
        switch (inID) {
            case kAudioUnitProperty_ContextName:
                // TODO: 
                //GetStr(mContextName);
                break;
        }
    }

    ComponentResult PluginAU::GetPropertyInfo (
        AudioUnitPropertyID  inID,
        AudioUnitScope       inScope,
        AudioUnitElement     inElement,
        UInt32&              outDataSize,
        Boolean&             outWritable
    ) {
        // TODO: 
        return MusicDeviceBase::GetPropertyInfo(inID, inScope, inElement, outDataSize, outWritable);
    }

    ComponentResult PluginAU::GetProperty (
        AudioUnitPropertyID  inID,
        AudioUnitScope       inScope,
        AudioUnitElement     inElement,
        void*                outData
    ) {
        // TODO: 
        return MusicDeviceBase::GetProperty(inID, inScope, inElement, outData);
    }

    ComponentResult PluginAU::SetProperty (
        AudioUnitPropertyID  inID,
        AudioUnitScope       inScope,
        AudioUnitElement     inElement,
        const void*          inData,
        UInt32               inDataSize
    ) {
        switch (inID) {
            case kAudioUnitProperty_AUHostIdentifier:
                if(inData != NULL) {
                    AUHostIdentifier* host = (AUHostIdentifier*)inData;
                    hostName = GetHostNameByID(GetStr(host->hostName));
                }
                break;
        }
        return MusicDeviceBase::SetProperty(inID, inScope, inElement, inData, inDataSize);
    }

    ComponentResult PluginAU::GetPresets(CFArrayRef* outData) const {
        if (outData == NULL) return noErr;

        CFMutableArrayRef presetsArray = CFArrayCreateMutable (
            NULL, kFactoryPresetCount, NULL
        );

        for (int i = 0; i < kFactoryPresetCount; i++) {
            CFArrayAppendValue(presetsArray, &factoryPresets[i] );
        }

        *outData = (CFArrayRef)presetsArray;
        return noErr;
    }

    OSStatus PluginAU::NewFactoryPresetSet(const AUPreset& inNewFactoryPreset) {
        if(global == NULL) return noErr;

        switch(inNewFactoryPreset.presetNumber) {
            case kOneSamplerChannel:
                AddSamplerChannel("GIG", midi_chan_1);
                break;
            case k16SamplerChannels:
                Set16SamplerChannelsPreset();
                break;
            default:
                return kAudioUnitErr_InvalidProperty;
        }

        SetAFactoryPresetAsCurrent(inNewFactoryPreset);
        return noErr;
    }

    UInt32 PluginAU::SupportedNumChannels(const AUChannelInfo** outInfo) {
        static AUChannelInfo plugChannelInfo = { 0, 2 };
        if (outInfo != NULL) *outInfo = &plugChannelInfo;
        return 1;
    }

    ComponentResult PluginAU::Initialize() {
        // format validation: current LS engines only support stereo
        // buses
        int busCount = Outputs().GetNumberOfElements();
        for (int i = 0 ; i < busCount ; i++) {
            if (GetOutput(i)->GetStreamFormat().mChannelsPerFrame != 2) {
                return kAudioUnitErr_FormatNotSupported;
            }
        }

        MusicDeviceBase::Initialize();

        // The timeconsuming tasks delayed until the plugin is to be used
        PreInit();

        if(pAudioDevice) {
            RemoveChannels();
            DestroyDevice(pAudioDevice);
            pAudioDevice = NULL;
        }

        if(pMidiDevice) {
            DestroyDevice(pMidiDevice);
            pMidiDevice = NULL;
        }

        int srate = (int)GetStreamFormat(kAudioUnitScope_Output, 0).mSampleRate;
        int chnNum = 0;
        for (int i = 0 ; i < busCount ; i++) {
            chnNum += GetOutput(i)->GetStreamFormat().mChannelsPerFrame;
        }

        Init(srate, GetMaxFramesPerSlice(), chnNum);

        if(hostName.empty()) {
            hostName = GetHostNameByID(GetHostBundleID());
        }

        if(!hostName.empty()) {
            for(int i = 0; i < pMidiDevice->PortCount(); i++) {
                DeviceRuntimeParameter* param;
                param = pMidiDevice->GetPort(i)->PortParameters()["NAME"];
                if(param != NULL) param->SetValue(hostName + " " + ToString(i + 1));

            }
        }

        if(!hostName.empty()) {
            for(int i = 0; i < pAudioDevice->ChannelCount(); i++) {
                DeviceRuntimeParameter* param;
                param = pAudioDevice->Channel(i)->ChannelParameters()["NAME"];
                if(param != NULL) param->SetValue(hostName + " " + ToString(i + 1));

            }
        }

        NewFactoryPresetSet(factoryPresets[kDefaultPreset]);

        /*AudioChannelLayout* pLayout = CAAudioChannelLayout::Create(chnNum);
        if(pLayout != NULL) {
            for(int i = 0; i < chnNum; i++) {
                pLayout->mChannelDescriptions[i].mChannelLabel =
            }
            SetAudioChannelLayout(kAudioUnitScope_Output, 0, pLayout);
        }*/

	return noErr;
    }

    bool PluginAU::StreamFormatWritable(AudioUnitScope scope, AudioUnitElement element) {
        return IsInitialized() ? false : true;
    }

    OSStatus PluginAU::HandleMidiEvent (
        UInt8 inStatus, UInt8 inChannel,
        UInt8 inData1, UInt8 inData2,
        UInt32 inStartFrame
    ) {
        if(!IsInitialized()) return kAudioUnitErr_Uninitialized;
        MidiInputPort* port = pMidiDevice->Port();

        switch (inStatus) {
            case 0x90:
                if(inData1 >= 0x80) break;

                if(inData2) {
                    port->DispatchNoteOn (
                        inData1, inData2, inChannel, inStartFrame
                    );
                } else {
                    port->DispatchNoteOff (
                        inData1, inData2, inChannel, inStartFrame
                    );
                }
                break;
            case 0x80:
                if(inData1 >= 0x80) break;
                port->DispatchNoteOff (
                    inData1, inData2, inChannel, inStartFrame
                );
                break;
            case 0xB0:
                if(inData1 == 0) {
                    port->DispatchBankSelectMsb(inData2, inChannel);
                } else if(inData1 == 32) {
                    port->DispatchBankSelectLsb(inData2, inChannel);
                }

                port->DispatchControlChange (
                    inData1, inData2, inChannel, inStartFrame
                );
                break;
            case 0xC0:
                if(inData1 < 0x80) port->DispatchProgramChange(inData1, inChannel);
                break;
            case 0xE0:
                port->DispatchPitchbend((inData1 | inData2 << 7) - 8192, inChannel, inStartFrame);
                break;
            case 0xD0:
                port->DispatchControlChange(128, inData1, inChannel);
                break;
        }

        return noErr;
    }

    ComponentResult PluginAU::Render (
        AudioUnitRenderActionFlags&  ioActionFlags,
        const AudioTimeStamp&        inTimeStamp,
        UInt32                       inNumberFrames
    ) {
        int i = 0;
        int busCount = Outputs().GetNumberOfElements();
        for (int bus = 0 ; bus < busCount ; bus++) {
            AudioBufferList& list = GetOutput(bus)->PrepareBuffer(inNumberFrames);
            int chnCount = list.mNumberBuffers;
            for (int chn = 0; chn < chnCount; chn++) {
                float* buf = static_cast<float*>(list.mBuffers[chn].mData);
                pAudioDevice->Channel(i)->SetBuffer(buf);
                i++;
            }
        }

        pAudioDevice->Render(inNumberFrames);
        return noErr;
    }

    void PluginAU::Set16SamplerChannelsPreset() {
        RemoveChannels();
        for(int i = 0; i < 16; i++) {
            AddSamplerChannel("GIG", midiChns[i]);
        }
    }

    void PluginAU::AddSamplerChannel(const char* engine, midi_chan_t midiChn) {
        SamplerChannel* channel = global->pSampler->AddSamplerChannel();
        channel->SetEngineType(engine);
        channel->SetAudioOutputDevice(pAudioDevice);
        channel->SetMidiInputDevice(pMidiDevice);
        channel->SetMidiInputChannel(midiChn);
    }

    OSStatus PluginAU::GetInstrumentCount(UInt32 &outInstCount) const {
        // TODO:
        return MusicDeviceBase::GetInstrumentCount(outInstCount);
    }

    std::string PluginAU::GetHostNameByID(std::string Identifier) {
        std::string name;
        if(Identifier == "com.apple.logic.pro") {
            name = "Logic Pro";
        } else if(Identifier == "com.apple.garageband") {
            name = "GarageBand";
        } else if(Identifier == "com.ableton.live") {
            name = "Ableton Live";
        } else if(Identifier == "com.apple.audio.aulab") {
            name = "AULab";
        }

        return name;
    }

    std::string PluginAU::GetHostBundleID() {
        std::string id;
        CFStringRef identifier;
        CFBundleRef mainBundle = CFBundleGetMainBundle();
        if (mainBundle) {
            identifier = CFBundleGetIdentifier(mainBundle);
            id = GetStr(identifier);
        }

        return id;
    }

    std::string PluginAU::GetStr(CFStringRef s) {
        if(s == NULL) return std::string("");

        CFRetain(s);
        CFIndex len = CFStringGetLength(s) + 1;
        if(charBufSize < len) {
            delete [] charBuf;
            charBufSize *= 2;
            if(charBufSize < len) charBufSize = len;
            charBuf = new char[charBufSize];
        }

        std::string s2;
        if(CFStringGetCString(s, charBuf, charBufSize, kCFStringEncodingASCII)) {
            s2 = charBuf;
        }
        CFRelease(s);

        return s2;
    }

    ComponentResult PluginAU::SaveState(CFPropertyListRef* outData) {
        ComponentResult result = AUBase::SaveState(outData);
        if(global == NULL || result != noErr) return result;

        void** d = const_cast<void**> (outData);
        CFMutableDictionaryRef dict = static_cast<CFMutableDictionaryRef> (*d);

        std::string s = GetState();
        CFStringRef cfsState = CFStringCreateWithCString(NULL, s.c_str(), kCFStringEncodingASCII);

        CFDictionarySetValue(dict, CFSTR("org.linuxsampler.LinuxSamplerPluginAU"), cfsState);
        CFRelease(cfsState);

        return noErr;
    }

    ComponentResult PluginAU::RestoreState(CFPropertyListRef inData) {
        ComponentResult result = AUBase::RestoreState(inData);
        if(global == NULL || result != noErr) return result;

        CFDictionaryRef dict = static_cast<CFDictionaryRef>(inData);

        const void* d = CFDictionaryGetValue (
            dict, CFSTR("org.linuxsampler.LinuxSamplerPluginAU")
        );

        CFStringRef str = static_cast<CFStringRef>(d);
        if(str != NULL) SetState(GetStr(str));

        return noErr;
    }

//} // namespace LinuxSampler

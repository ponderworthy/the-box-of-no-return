/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2009 Grigor Iliev                                       *
 *   Copyright (C) 2013 - 2016 Andreas Persson and Christian Schoenebeck   *
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

#include "CAAudioDeviceModel.h"

#include <iostream>
#include "../../common/Exception.h"
#include "../../common/global_private.h"

namespace LinuxSampler {

    CAAudioDeviceModel::CAAudioDeviceModel(AudioDeviceID Id) {
        this->Id = Id;
        ChannelNumber = 2;
        DefaultSamplerate = 44100;

        charBufSize = 512;
        charBuf = new char[charBufSize];

        Init();
    }

    CAAudioDeviceModel::CAAudioDeviceModel(const CAAudioDeviceModel& m) : Uid(m.Uid),
            Name(m.Name), NominalSamplerates(m.NominalSamplerates), AudioStreams(m.AudioStreams) {

        Id = m.Id;
        ChannelNumber = m.ChannelNumber;
        DefaultSamplerate = m.DefaultSamplerate;

        charBufSize = 512;
        charBuf = new char[charBufSize];

        Init();
    }

    CAAudioDeviceModel::~CAAudioDeviceModel() {
        CAAudioDeviceListModel::GetModel()->RemoveListener(this);
        delete [] charBuf;
    }

    CAAudioDeviceModel& CAAudioDeviceModel::operator=(const CAAudioDeviceModel& m) {
        Id = m.Id;
        Uid = m.Uid;
        Name = m.Name;
        ChannelNumber = m.ChannelNumber;
        DefaultSamplerate = m.DefaultSamplerate;
        NominalSamplerates = m.NominalSamplerates;
        AudioStreams = m.AudioStreams;

        return *this;
    }

    void CAAudioDeviceModel::Init() {
        CAAudioDeviceListModel::GetModel()->AddListener(this);
    }

    void CAAudioDeviceModel::DeviceChanged(AudioDeviceID devID) {
        if(GetID() == devID) FireDeviceChangedEvent();
    }

    void CAAudioDeviceModel::FireDeviceChangedEvent() {
        for(int i = 0; i < GetListenerCount(); i++) {
            GetListener(i)->DeviceChanged();
        }
    }

    AudioDeviceID CAAudioDeviceModel::GetID() { return Id; }

    std::string CAAudioDeviceModel::GetUID() { return Uid; }

    void CAAudioDeviceModel::SetUID(CFStringRef s) {
        Uid = GetStr(s);
    }

    std::string CAAudioDeviceModel::GetName() { return Name; }

    void CAAudioDeviceModel::SetName(CFStringRef s) {
        Name = GetStr(s);
    }

    int CAAudioDeviceModel::GetChannelNumber() { return ChannelNumber; }

    int CAAudioDeviceModel::GetDefaultSamplerate() { return DefaultSamplerate; }

    const std::vector<int> CAAudioDeviceModel::GetNominalSamplerates() {
        return NominalSamplerates;
    }

    std::vector<CAAudioStream> CAAudioDeviceModel::GetAudioStreams() {
        return AudioStreams;
    }

    std::string CAAudioDeviceModel::GetStr(CFStringRef s) {
        if(s == NULL) return std::string("Undefined");

        CFRetain(s);
        CFIndex len = CFStringGetLength(s) + 1;
        if(charBufSize < len) {
            delete [] charBuf;
            charBufSize *= 2;
            if(charBufSize < len) charBufSize = (UInt32)len;
            charBuf = new char[charBufSize];
        }
        
        std::string s2;
        if(CFStringGetCString(s, charBuf, charBufSize, kCFStringEncodingASCII)) {
            s2 = charBuf;
        }
        CFRelease(s);

        return s2;
    }

//////////////////////////////////////////////////////////////////
////

    CAAudioDeviceListModel* CAAudioDeviceListModel::pModel = NULL;

    CAAudioDeviceListModel* CAAudioDeviceListModel::GetModel() {
        if(pModel == NULL) {
            pModel = new CAAudioDeviceListModel();
            pModel->RescanDevices();
            pModel->InstallListeners();
        }
        return pModel;
    }
    
    OSStatus CAAudioDeviceListModel::AudioHardwarePropertyListenerCallback (
        AudioHardwarePropertyID inPropertyID, void* inClientData
    ) {
        CAAudioDeviceListModel* model = CAAudioDeviceListModel::GetModel();
        switch(inPropertyID) {
            case kAudioHardwarePropertyDevices:
                model->RescanDevices();
                break;
            case kAudioHardwarePropertyDefaultOutputDevice:
                model->UpdateDefaultOutputDevice();
                model->FireDefaultOutputDeviceChangedEvent(model->GetDefaultOutputDeviceID());
                break;
        }
        return noErr;
    }

    OSStatus CAAudioDeviceListModel::AudioDevicePropertyListenerCallback (
        AudioDeviceID inDevice,
        UInt32 inChannel,
        Boolean isInput,
        AudioDevicePropertyID inPropertyID,
        void* inClientData
    ) {
        CAAudioDeviceListModel* model = CAAudioDeviceListModel::GetModel();

        switch(inPropertyID) {
            case kAudioDevicePropertyDeviceHasChanged:
                model->FireDeviceChangedEvent(inDevice);
                break;
        }

        return noErr;
    }

    CAAudioDeviceListModel::CAAudioDeviceListModel() {
        DefaultOutputDeviceID = 0;
    }

    CAAudioDeviceListModel::~CAAudioDeviceListModel() {
        UninstallListeners();
    }

    void CAAudioDeviceListModel::InstallListeners() {
        InstallHardwareListeners();
        InstallDeviceListeners();
    }

    void CAAudioDeviceListModel::UninstallListeners() {
        UninstallHardwareListeners();
        UninstallDeviceListeners();
    }

    void CAAudioDeviceListModel::InstallHardwareListeners() {
        // Notify when device is added or removed
        InstallHardwareListener(kAudioHardwarePropertyDevices);
        //Notify when the default audio output device is changed
        InstallHardwareListener(kAudioHardwarePropertyDefaultOutputDevice);
    }

    void CAAudioDeviceListModel::UninstallHardwareListeners() {
        UninstallHardwareListener(kAudioHardwarePropertyDevices);
        UninstallHardwareListener(kAudioHardwarePropertyDefaultOutputDevice);
    }

    void CAAudioDeviceListModel::InstallHardwareListener(AudioHardwarePropertyID id) {
        AudioHardwareAddPropertyListener (
            id, AudioHardwarePropertyListenerCallback, NULL
        );
    }

    void CAAudioDeviceListModel::UninstallHardwareListener(AudioHardwarePropertyID id) {
        AudioHardwareRemovePropertyListener (
            id, AudioHardwarePropertyListenerCallback
        );
    }

    void CAAudioDeviceListModel::InstallDeviceListeners() {
        InstallDeviceListener(kAudioDevicePropertyDeviceHasChanged);
    }

    void CAAudioDeviceListModel::UninstallDeviceListeners() {
        UninstallDeviceListener(kAudioDevicePropertyDeviceHasChanged);
    }

    void CAAudioDeviceListModel::InstallDeviceListener(AudioDevicePropertyID id) {
        for(int i = 0; i < GetModel()->GetOutputDeviceCount(); i++) {
            AudioDeviceAddPropertyListener (
                GetModel()->GetOutputDevice(i).GetID(), 0, false,
                id, AudioDevicePropertyListenerCallback, NULL
            );
        }
    }

    void CAAudioDeviceListModel::UninstallDeviceListener(AudioDevicePropertyID id) {
        for(int i = 0; i < GetModel()->GetOutputDeviceCount(); i++) {
            AudioDeviceRemovePropertyListener (
                GetModel()->GetOutputDevice(i).GetID(), 0, false,
                id, AudioDevicePropertyListenerCallback
            );
        }
    }

    void CAAudioDeviceListModel::FireDeviceChangedEvent(AudioDeviceID devID) {
        LockGuard lock(DeviceMutex);
        for(int i = 0; i < GetListenerCount(); i++) {
            GetListener(i)->DeviceChanged(devID);
        }
    }

    void CAAudioDeviceListModel::FireDeviceListChangedEvent() {
        LockGuard lock(DeviceMutex);
        for(int i = 0; i < GetListenerCount(); i++) {
            GetListener(i)->DeviceListChanged();
        }
    }

    void CAAudioDeviceListModel::FireDefaultOutputDeviceChangedEvent(AudioDeviceID newID) {
        LockGuard lock(DeviceMutex);
        for(int i = 0; i < GetListenerCount(); i++) {
            GetListener(i)->DefaultOutputDeviceChanged(newID);
        }
    }

    AudioDeviceID CAAudioDeviceListModel::GetDefaultOutputDeviceID() {
        return DefaultOutputDeviceID;
    }

    UInt32 CAAudioDeviceListModel::GetOutputDeviceCount() {
        LockGuard lock(DeviceMutex);
        return (UInt32) outDevices.size();
    }

    CAAudioDeviceModel CAAudioDeviceListModel::GetOutputDevice(UInt32 Index) {
        LockGuard lock(DeviceMutex);
        if(/*Index < 0 ||*/ Index >= GetOutputDeviceCount()) {
            throw Exception("Device index out of bounds");
        }

        return outDevices[Index];
    }

    CAAudioDeviceModel CAAudioDeviceListModel::GetOutputDeviceByID(AudioDeviceID devID) {
        LockGuard lock(DeviceMutex);
        for(int i = 0; i < outDevices.size(); i++) {
            if(outDevices[i].GetID() == devID) {
                CAAudioDeviceModel dev = outDevices[i];
                return dev;
            }
        }
        throw Exception("Unknown audio device ID: " + ToString(devID));
    }

    UInt32 CAAudioDeviceListModel::GetOutputDeviceIndex(AudioDeviceID devID) {
        LockGuard lock(DeviceMutex);
        for(UInt32 i = 0; i < outDevices.size(); i++) {
            if(outDevices[i].GetID() == devID) {
                return i;
            }
        }
        throw Exception("Unknown audio device ID: " + ToString(devID));
    }

    void CAAudioDeviceListModel::RescanDevices() {
        LockGuard lock(DeviceMutex);
        inDevices.clear();
        outDevices.clear();

        UInt32   outSize;
        Boolean  outWritable;

        UpdateDefaultOutputDevice();

        OSStatus res = AudioHardwareGetPropertyInfo (
            kAudioHardwarePropertyDevices, &outSize, &outWritable
        );

        if(res) {
            std::cerr << "Failed to get device list: " << res << std::endl;
            return;
        }

        UInt32 deviceNumber = outSize / sizeof(AudioDeviceID);
        if(deviceNumber < 1) {
            std::cerr << "No audio devices found" << std::endl;
            return;
        }

        if(deviceNumber * sizeof(AudioDeviceID) != outSize) {
            std::cerr << "Invalid device size. This is a bug!" << std::endl;
            return;
        }

        AudioDeviceID* devs = new AudioDeviceID[deviceNumber];

        res = AudioHardwareGetProperty (
            kAudioHardwarePropertyDevices, &outSize, devs
        );

        if(res) {
            std::cerr << "Failed to get device IDs: " << res << std::endl;
            delete [] devs;
            return;
        }

        for(int i = 0; i < deviceNumber; i++) {
            // TODO: for now skip devices which are not alive
            outSize = sizeof(UInt32);
            UInt32 isAlive = 0;

            res = AudioDeviceGetProperty (
                devs[i], 0, false,
                kAudioDevicePropertyDeviceIsAlive,
                &outSize, &isAlive
            );

            if(res || !isAlive) continue;
            ///////

            CAAudioDeviceModel dev(devs[i]);

            CFStringRef tempStringRef = NULL;

            // Get the name of the device
            outSize = sizeof(CFStringRef);

            res = AudioDeviceGetProperty (
                devs[i], 0, false,
                kAudioObjectPropertyName,
                &outSize, &tempStringRef
            );

            dev.SetName(tempStringRef);
            if(tempStringRef != NULL) CFRelease(tempStringRef);
            ///////

            // Get the name of the device
            outSize = sizeof(CFStringRef);

            res = AudioDeviceGetProperty (
                devs[i], 0, false,
                kAudioDevicePropertyDeviceUID,
                &outSize, &tempStringRef
            );

            dev.SetUID(tempStringRef);
            if(tempStringRef != NULL) CFRelease(tempStringRef);
            ///////

            GetChannels(&dev);
            GetSamplerates(&dev);
            if(dev.GetChannelNumber() > 0) outDevices.push_back(dev);
        }

        delete [] devs;
        FireDeviceListChangedEvent();

        /*for(int i = 0; i < GetOutputDeviceCount(); i++) {
            std::cout << "Device ID: " << GetOutputDevice(i).GetID() << std::endl;
            std::cout << "Device name: " << GetOutputDevice(i).GetName() << std::endl;
            std::cout << "Device UID: " << GetOutputDevice(i).GetUID() << std::endl;
            std::cout << "Channels: " << GetOutputDevice(i).GetChannelNumber() << std::endl;
            std::cout << "Default samplerate: " << GetOutputDevice(i).GetDefaultSamplerate() << std::endl;

            const std::vector<int> rates = GetOutputDevice(i).GetNominalSamplerates();
            std::cout << "Nominal samplerates: " << std::endl;
            for(int j = 0; j < rates.size(); j++) {
                std::cout << "\t" << rates[j] << std::endl;
            }
            std::cout << std::endl;
        }*/
    }

    void CAAudioDeviceListModel::UpdateDefaultOutputDevice() {
        AudioDeviceID id;
        UInt32 size = sizeof(AudioDeviceID);
        OSStatus res = AudioHardwareGetProperty (
            kAudioHardwarePropertyDefaultOutputDevice, &size, &id
        );
        if(res) std::cerr << "Failed to get default output device: " << res << std::endl;
        else DefaultOutputDeviceID = id;
    }

    void CAAudioDeviceListModel::GetChannels(CAAudioDeviceModel* pDev) {
        UInt32 outSize;
        bool isInput = false;

        OSStatus res = AudioDeviceGetPropertyInfo (
            pDev->GetID(), 0, isInput,
            kAudioDevicePropertyStreamConfiguration,
            &outSize, NULL
        );
        if(res || outSize == 0) {
            if(res) std::cerr << "Failed to get stream config: " << res << std::endl;
            return;
        }

        AudioBufferList* pBufferList = NULL;
        pBufferList = (AudioBufferList*)malloc(outSize);
        if(pBufferList == NULL) {
            perror(NULL);
            return;
        }

        res = AudioDeviceGetProperty (
            pDev->GetID(), 0, isInput,
            kAudioDevicePropertyStreamConfiguration,
            &outSize, pBufferList
        );

        if(res) {
            std::cerr << "Failed to get channel number: " << res << std::endl;
            free(pBufferList);
            return;
        }

        UInt32 chns = 0;
        for(int i = 0; i < pBufferList->mNumberBuffers; i++) {
            chns += pBufferList->mBuffers[i].mNumberChannels;
        }

        pDev->ChannelNumber = chns;
        
        free(pBufferList);
    }

    /*void CAAudioDeviceListModel::GetStreams(CAAudioDeviceModel* pDev) {
        UInt32 outSize = sizeof(AudioStreamID);

        OSStatus res = AudioDeviceGetPropertyInfo (
            pDev->GetID(), 0, false,
            kAudioDevicePropertyStreams,
            &outSize, NULL
        );

        if(res) {
            std::cerr << "Failed to get device streams: " << res << std::endl;
            return;
        }

        UInt32 streamNumber = outSize / sizeof(AudioStreamID);
        if(streamNumber < 1) {
            std::cerr << "No streams found" << std::endl;
            return;
        }

        if(streamNumber * sizeof(AudioStreamID) != outSize) {
            std::cerr << "Invalid AudioStreamID size. This is a bug!" << std::endl;
            return;
        }

        AudioStreamID* streams = new AudioStreamID[streamNumber];

        res = AudioDeviceGetProperty (
            pDev->GetID(), 0, false,
            kAudioDevicePropertyStreams,
            &outSize, streams
        );

        if(res) {
            std::cerr << "Failed to get stream IDs: " << res << std::endl;
            delete [] streams;
            return;
        }

        for(int i = 0; i < streamNumber; i++) {
            UInt32 inStream;

            res = AudioStreamGetProperty (
                streams[i], 0, kAudioStreamPropertyDirection, &outSize, &inStream
            );

            if(res) {
                std::cerr << "Failed to get stream direction: " << res << std::endl;
                continue;
            }

            //pDev->AudioStreams.push_back(CAAudioStream(inStream, chns));
        }

        delete [] streams;
    }*/

    void CAAudioDeviceListModel::GetSamplerates(CAAudioDeviceModel* pDev) {
        UInt32 outSize;

        // Get current nominal sample rate
        outSize = sizeof(Float64);
        Float64 srate;

        OSStatus res = AudioDeviceGetProperty (
            pDev->GetID(), 0, false,
            kAudioDevicePropertyNominalSampleRate,
            &outSize, &srate
        );
        
        if(res) std::cerr << "Failed to get default samplerate: " << res << std::endl;
        else pDev->DefaultSamplerate = srate;
        ///////

        res = AudioDeviceGetPropertyInfo (
            pDev->GetID(), 0, false,
            kAudioDevicePropertyAvailableNominalSampleRates,
            &outSize, NULL
        );

        if(res) {
            std::cerr << "Failed to get nominal samplerates: " << res << std::endl;
            return;
        }

        UInt32 rangeNumber = outSize / sizeof(AudioValueRange);
        if(rangeNumber < 1) {
            std::cerr << "No nominal samplerates found" << std::endl;
            return;
        }

        if(rangeNumber * sizeof(AudioValueRange) != outSize) {
            std::cerr << "Invalid AudioValueRange size. This is a bug!" << std::endl;
            return;
        }

        AudioValueRange* ranges = new AudioValueRange[rangeNumber];

        res = AudioDeviceGetProperty (
            pDev->GetID(), 0, false,
            kAudioDevicePropertyAvailableNominalSampleRates,
            &outSize, ranges
        );

        if(res) {
            std::cerr << "Failed to get samplerate ranges: " << res << std::endl;
            delete [] ranges;
            return;
        }

        for(int i = 0; i < rangeNumber; i++) {
            // FIXME: might be ranges of valid samplerates
            UInt32 min = (UInt32)ranges[i].mMinimum;
            UInt32 max = (UInt32)ranges[i].mMaximum;
            pDev->NominalSamplerates.push_back(min);
            if(min != max) pDev->NominalSamplerates.push_back(max);
        }

        delete [] ranges;
    }
} // namespace LinuxSampler

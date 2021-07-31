/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2009 Grigor Iliev                                       *
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

#ifndef __CAAUDIODEVICEMODEL_H__
#define	__CAAUDIODEVICEMODEL_H__

#include <string>
#include <vector>
#include <CoreAudio/CoreAudio.h>

#include "../../EventListeners.h"
#include "../../common/Mutex.h"

namespace LinuxSampler {

    class CAAudioStream {
        private:
            UInt32  InputStream;
            UInt32  ChannelNumber;

        public:
            CAAudioStream(UInt32 input, UInt32 chns) {
                InputStream = input, ChannelNumber = chns;
            }

            CAAudioStream& operator=(const CAAudioStream& s) {
                InputStream = s.InputStream;
                ChannelNumber = s.ChannelNumber;
                return *this;
            }

            CAAudioStream(const CAAudioStream& s) { *this = s; }

            UInt32  IsInputStream() { return InputStream; }
            UInt32  GetChannelNumber() { return ChannelNumber; }
    };

    class CAAudioDeviceModelListener {
        public:
            virtual void DeviceChanged() { }
    };

    class CAAudioDeviceListModelListener {
        public:
            virtual void DeviceChanged(AudioDeviceID devID) { }
            virtual void DeviceListChanged() { }
            virtual void DefaultOutputDeviceChanged(AudioDeviceID newID) { }
    };

    class CAAudioDeviceModel : public ListenerList<CAAudioDeviceModelListener*>,
            public CAAudioDeviceListModelListener {

        friend class CAAudioDeviceListModel;

        public:
            CAAudioDeviceModel(AudioDeviceID Id);
            CAAudioDeviceModel(const CAAudioDeviceModel& m);
            ~CAAudioDeviceModel();
            CAAudioDeviceModel& operator=(const CAAudioDeviceModel& m);

            AudioDeviceID  GetID();
            std::string    GetUID();
            std::string    GetName();
            int            GetChannelNumber();
            int            GetDefaultSamplerate();

            const std::vector<int>      GetNominalSamplerates();
            std::vector<CAAudioStream>  GetAudioStreams();

            //from CAAudioDeviceListModelListener
            virtual void                DeviceChanged(AudioDeviceID devID);

        private:
            AudioDeviceID  Id;
            std::string    Uid;
            std::string    Name;
            int            ChannelNumber;
            int            DefaultSamplerate;

            std::vector<int>            NominalSamplerates;
            std::vector<CAAudioStream>  AudioStreams;

            char*          charBuf;
            UInt32         charBufSize;

            void           Init();
            std::string    GetStr(CFStringRef s);
            void           SetUID(CFStringRef s);
            void           SetName(CFStringRef s);
            void           FireDeviceChangedEvent();
    };

/////////////////////////////////////////////////////
////

    class CAAudioDeviceListModel : public ListenerList<CAAudioDeviceListModelListener*> {
        public:
            ~CAAudioDeviceListModel();
            static CAAudioDeviceListModel* GetModel();

            AudioDeviceID       GetDefaultOutputDeviceID();
            UInt32              GetOutputDeviceCount();
            CAAudioDeviceModel  GetOutputDevice(UInt32 Index);
            CAAudioDeviceModel  GetOutputDeviceByID(AudioDeviceID devID);
            UInt32              GetOutputDeviceIndex(AudioDeviceID devID);

        private:
            CAAudioDeviceListModel(); // Forbid instantiation of this class
            static CAAudioDeviceListModel*   pModel;
            std::vector<CAAudioDeviceModel>  inDevices;
            std::vector<CAAudioDeviceModel>  outDevices;
            AudioDeviceID                    DefaultOutputDeviceID;
            Mutex  DeviceMutex;

            void InstallListeners();
            void UninstallListeners();
            void InstallDeviceListeners();
            void InstallDeviceListener(AudioDevicePropertyID id);
            void UninstallDeviceListeners();
            void UninstallDeviceListener(AudioDevicePropertyID id);
            void InstallHardwareListeners();
            void InstallHardwareListener(AudioHardwarePropertyID id);
            void UninstallHardwareListeners();
            void UninstallHardwareListener(AudioHardwarePropertyID id);
            void RescanDevices();
            void GetSamplerates(CAAudioDeviceModel* pDev);
            void GetChannels(CAAudioDeviceModel* pDev);
            void GetStreams(CAAudioDeviceModel* pDev);
            void UpdateDefaultOutputDevice();
            void FireDeviceChangedEvent(AudioDeviceID devID);
            void FireDeviceListChangedEvent();
            void FireDefaultOutputDeviceChangedEvent(AudioDeviceID newID);

            static OSStatus AudioHardwarePropertyListenerCallback (
                AudioHardwarePropertyID inPropertyID, void* inClientData
            );

            static OSStatus AudioDevicePropertyListenerCallback (
                AudioDeviceID inDevice,
                UInt32 inChannel,
                Boolean isInput,
                AudioDevicePropertyID inPropertyID,
                void* inClientData
            );
    };
} // namespace LinuxSampler

#endif	/* __CAAUDIODEVICEMODEL_H__ */

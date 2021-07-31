/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2013 Christian Schoenebeck                       *
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

#ifndef _LS_AUDIOOUTPUTDEVICEASIO_H_
#define _LS_AUDIOOUTPUTDEVICEASIO_H_

#include <string.h>

#include <asiosys.h>
#include <asio.h>
#include <asiodrivers.h>
#include <math.h>
#include "iasiothiscallresolver.h"

#include "../../common/global_private.h"
#include "AudioOutputDevice.h"
#include "AudioChannel.h"
#include "../DeviceParameter.h"

namespace LinuxSampler { //

    /** Asio audio output driver
      *
      * Implements audio output to the Advanced Linux Sound Architecture (Asio).
      */
    class AudioOutputDeviceAsio : public AudioOutputDevice {
        public:
            AudioOutputDeviceAsio(std::map<String,DeviceCreationParameter*> Parameters);
            ~AudioOutputDeviceAsio();

            // derived abstract methods from class 'AudioOutputDevice'
            virtual void Play() OVERRIDE;
            virtual bool IsPlaying() OVERRIDE;
            virtual void Stop() OVERRIDE;
            virtual uint MaxSamplesPerCycle() OVERRIDE;
            virtual uint SampleRate() OVERRIDE;
            virtual AudioChannel* CreateChannel(uint ChannelNr) OVERRIDE;
            virtual String Driver() OVERRIDE;
            
            static String Name();
            static String Description();
            static String Version();

            /** Device Parameter 'CARD'
                *
                * Used to select the desired Asio sound card.
                */
            class ParameterCard : public DeviceCreationParameterString {
                public:
                    ParameterCard();
                    ParameterCard(String s) throw (Exception);
                    virtual String Description() OVERRIDE;
                    virtual bool   Fix() OVERRIDE;
                    virtual bool   Mandatory() OVERRIDE;
                    virtual std::map<String,DeviceCreationParameter*> DependsAsParameters() OVERRIDE;
                    virtual optional<String>    DefaultAsString(std::map<String,String> Parameters) OVERRIDE;
                    virtual std::vector<String> PossibilitiesAsString(std::map<String,String> Parameters) OVERRIDE;
                    virtual void                OnSetValue(String s) throw (Exception) OVERRIDE;
                    static String Name();
            };

            /** Device Parameter 'SAMPLERATE'
                *
                * Used to set the sample rate of the audio output device.
                */
            class ParameterSampleRate : public AudioOutputDevice::ParameterSampleRate {
                public:
                    ParameterSampleRate();
                    ParameterSampleRate(String s);
                    virtual std::map<String,DeviceCreationParameter*> DependsAsParameters() OVERRIDE;
                    virtual optional<int> DefaultAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual std::vector<int> PossibilitiesAsInt(std::map<String,String> Parameters) OVERRIDE;
            };

            /** Device Parameters 'CHANNELS'
                *
                * Used to increase / decrease the number of audio channels of
                * audio output device.
                */
            class ParameterChannels : public AudioOutputDevice::ParameterChannels {
                public:
                    ParameterChannels();
                    ParameterChannels(String s);
                    virtual std::map<String,DeviceCreationParameter*> DependsAsParameters() OVERRIDE;
                    virtual optional<int> DefaultAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual optional<int> RangeMinAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual optional<int> RangeMaxAsInt(std::map<String,String> Parameters) OVERRIDE;
            };


            /** Device Parameter 'FRAGMENTSIZE'
                *
                * Used to set the audio fragment size / period size.
                */
            class ParameterFragmentSize : public DeviceCreationParameterInt {
                public:
                    ParameterFragmentSize();
                    ParameterFragmentSize(String s) throw (Exception);
                    virtual String Description() OVERRIDE;
                    virtual bool   Fix() OVERRIDE;
                    virtual bool   Mandatory() OVERRIDE;
                    virtual std::map<String,DeviceCreationParameter*> DependsAsParameters() OVERRIDE;
                    virtual optional<int>    DefaultAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual optional<int>    RangeMinAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual optional<int>    RangeMaxAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual std::vector<int> PossibilitiesAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual void             OnSetValue(int i) throw (Exception) OVERRIDE;
                    static String Name();

            };

            // make protected methods public
            using AudioOutputDevice::RenderAudio;
            using AudioOutputDevice::Channels;

        private:
            uint                 uiAsioChannels;
            uint                 uiSamplerate;
            uint                 FragmentSize;
            bool asioIsPlaying;

            static void bufferSwitch(long index, ASIOBool processNow);
            static ASIOTime* bufferSwitchTimeInfo(ASIOTime *timeInfo, long index, ASIOBool processNow);

    };
}

#endif // _LS_AUDIOOUTPUTDEVICEASIO_H_

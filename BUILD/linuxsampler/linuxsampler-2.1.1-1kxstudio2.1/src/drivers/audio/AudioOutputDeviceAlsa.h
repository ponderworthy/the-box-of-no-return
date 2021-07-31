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

#ifndef __LS_AUDIOOUTPUTDEVICEALSA_H__
#define __LS_AUDIOOUTPUTDEVICEALSA_H__

#include <string.h>
#include <alsa/asoundlib.h>

#include "../../common/global_private.h"
#include "../../common/Thread.h"
#include "AudioOutputDevice.h"
#include "AudioChannel.h"
#include "../DeviceParameter.h"

namespace LinuxSampler {

    /** ALSA audio output driver
     *
     * Implements audio output to the Advanced Linux Sound Architecture (ALSA).
     */
    class AudioOutputDeviceAlsa : public AudioOutputDevice, protected Thread {
        public:
            AudioOutputDeviceAlsa(std::map<String,DeviceCreationParameter*> Parameters);
            ~AudioOutputDeviceAlsa();

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
             * Used to select the desired ALSA sound card.
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
                    virtual optional<int> RangeMinAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual optional<int> RangeMaxAsInt(std::map<String,String> Parameters) OVERRIDE;
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

            /** Device Parameter 'FRAGMENTS'
             *
             * Used to select the number of audio fragments / periods.
             */
            class ParameterFragments : public DeviceCreationParameterInt {
                public:
                    ParameterFragments();
                    ParameterFragments(String s) throw (Exception);
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

        protected:
            int Main();  ///< Implementation of virtual method from class Thread

        private:
            uint                 uiAlsaChannels;
            uint                 uiSamplerate;
            uint                 FragmentSize;
            int16_t*             pAlsaOutputBuffer; ///< This is the buffer where the final mix will be copied to and send to the sound card
            String               pcm_name;          ///< Name of the PCM device, like plughw:0,0 the first number is the number of the soundcard, the second number is the number of the device.
            snd_pcm_t*           pcm_handle;        ///< Handle for the PCM device
            snd_pcm_stream_t     stream;
            snd_pcm_hw_params_t* hwparams;          ///< This structure contains information about the hardware and can be used to specify the configuration to be used for the PCM stream.
            snd_pcm_sw_params_t* swparams;

            int  Output();
            bool HardwareParametersSupported(String card, uint channels, int samplerate, uint numfragments, uint fragmentsize) throw (AudioOutputException);
    };
}

#endif // __LS_AUDIOOUTPUTDEVICEALSA_H__

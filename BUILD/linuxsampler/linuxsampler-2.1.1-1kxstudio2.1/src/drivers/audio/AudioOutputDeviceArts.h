/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2006, 2007, 2013 Christian Schoenebeck                  *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this library; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#ifndef __LS_AUDIOOUTPUTDEVICEARTS_H__
#define __LS_AUDIOOUTPUTDEVICEARTS_H__

#include <artsc.h>

#include "../../common/global_private.h"
#include "../../common/Thread.h"
#include "AudioOutputDevice.h"
#include "AudioChannel.h"
#include "../DeviceParameter.h"

namespace LinuxSampler {

    /** aRts audio output driver
     *
     * Implements audio output to the Analog Realtime Synthesizer (aRts).
     */
    class AudioOutputDeviceArts : public AudioOutputDevice, protected Thread {
        public:
            AudioOutputDeviceArts(std::map<String,DeviceCreationParameter*> Parameters);
           ~AudioOutputDeviceArts();

            /** Audio Device Parameter 'NAME'
             *
             * Used to assign an arbitrary name to the aRts client of this
             * audio device.
             */
            class ParameterName : public DeviceCreationParameterString {
                public:
                    ParameterName();
                    ParameterName(String s) throw (Exception);
                    virtual String              Description() OVERRIDE;
                    virtual bool                Fix() OVERRIDE;
                    virtual bool                Mandatory() OVERRIDE;
                    virtual std::map<String,DeviceCreationParameter*> DependsAsParameters() OVERRIDE;
                    virtual std::vector<String> PossibilitiesAsString(std::map<String,String> Parameters) OVERRIDE;
                    virtual optional<String>    DefaultAsString(std::map<String,String> Parameters) OVERRIDE;
                    virtual void                OnSetValue(String s) throw (Exception) OVERRIDE;
                    static String Name();
            };

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

        protected:
            virtual int Main() OVERRIDE;  ///< Implementation of virtual method from class Thread

        private:
            unsigned int   uiArtsChannels;
            unsigned int   uiSampleRate;
            unsigned int   FragmentSize;
            unsigned int   uiPacketSize;
            arts_stream_t  hStream;
            int16_t*       pArtsOutputBuffer;
    };
};

#endif // __LS_AUDIOOUTPUTDEVICEARTS_H__

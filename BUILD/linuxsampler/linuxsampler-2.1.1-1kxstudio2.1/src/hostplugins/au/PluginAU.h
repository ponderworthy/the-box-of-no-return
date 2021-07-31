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

#ifndef __PLUGIN_AU_H__
#define	__PLUGIN_AU_H__

#include "PluginAUVersion.h"

#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>
#include <MusicDeviceBase.h>

#include "../../drivers/Plugin.h"

using namespace LinuxSampler;

//namespace LinuxSampler {

    class PluginAU : public MusicDeviceBase, public LinuxSampler::Plugin {
        public:
            PluginAU(ComponentInstance inComponentInstance);
            virtual ~PluginAU();
            virtual OSStatus  Version() { return kPluginAUVersion; }
            virtual UInt32    SupportedNumChannels(const AUChannelInfo** outInfo);
            virtual bool      StreamFormatWritable(AudioUnitScope scope, AudioUnitElement element);
            virtual ComponentResult Initialize();

            virtual ComponentResult GetPropertyInfo (
                AudioUnitPropertyID  inID,
                AudioUnitScope       inScope,
                AudioUnitElement     inElement,
                UInt32&              outDataSize,
                Boolean&             outWritable
            );
            
            virtual ComponentResult GetProperty (
                AudioUnitPropertyID  inID,
                AudioUnitScope       inScope,
                AudioUnitElement     inElement,
                void*                outData
            );

            virtual ComponentResult SetProperty (
                AudioUnitPropertyID  inID,
                AudioUnitScope       inScope,
                AudioUnitElement     inElement,
                const void*          inData,
                UInt32               inDataSize
            );

            virtual ComponentResult Render (
                AudioUnitRenderActionFlags&  ioActionFlags,
                const AudioTimeStamp&        inTimeStamp,
                UInt32                       inNumberFrames
            );

            virtual ComponentResult StartNote (
                MusicDeviceInstrumentID      inInstrument,
                MusicDeviceGroupID           inGroupID,
                NoteInstanceID*              outNoteInstanceID,
                UInt32                       inOffsetSampleFrame,
                const MusicDeviceNoteParams  &inParams
            ) { return noErr; }

            virtual ComponentResult StopNote (
                MusicDeviceGroupID  inGroupID,
                NoteInstanceID      inNoteInstanceID,
                UInt32              inOffsetSampleFrame
            ) { return noErr; }

            virtual OSStatus HandleMidiEvent (
                UInt8 inStatus, UInt8 inChannel,
                UInt8 inData1, UInt8 inData2,
                UInt32 inStartFrame
            );

            virtual void PropertyChanged (
                AudioUnitPropertyID  inID,
                AudioUnitScope       inScope,
                AudioUnitElement     inElement
            );

            virtual OSStatus         NewFactoryPresetSet (const AUPreset& inNewFactoryPreset);
            virtual OSStatus         GetInstrumentCount(UInt32 &outInstCount) const;
            virtual ComponentResult  GetPresets(CFArrayRef* outData) const;
            virtual ComponentResult  SaveState(CFPropertyListRef* outData);
            virtual ComponentResult  RestoreState(CFPropertyListRef inData);

        private:
            enum {
                kOneSamplerChannel = 0,
                k16SamplerChannels = 1
            };

            static const midi_chan_t  midiChns[16];
            static const UInt32       kDefaultPreset = kOneSamplerChannel;
            static const UInt32       kFactoryPresetCount = 2;
            static AUPreset           factoryPresets[kFactoryPresetCount];

            static void PropertyListenerCallback (
                void*                inRefCon,
                AudioUnit            ci,
                AudioUnitPropertyID  inID,
                AudioUnitScope       inScope,
                AudioUnitElement     inElement
            );

            std::string    hostName;
            char*          charBuf;
            UInt32         charBufSize;

            std::string    GetStr(CFStringRef s);
            void           InstallListeners();
            void           UninstallListeners();
            std::string    GetHostNameByID(std::string Identifier);
            std::string    GetHostBundleID();
            void           Set16SamplerChannelsPreset();
            void           AddSamplerChannel(const char* engine, midi_chan_t midiChn);
    };

//} // namespace LinuxSampler

#endif	/* __AU_PLUGIN_H__ */


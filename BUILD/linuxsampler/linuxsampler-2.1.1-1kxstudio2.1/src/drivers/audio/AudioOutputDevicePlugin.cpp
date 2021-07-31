/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008 - 2016 Andreas Persson and Christian Schoenebeck   *
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
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,                *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#include "AudioOutputDevicePlugin.h"
#include "../../common/global_private.h"
#include <algorithm>

namespace LinuxSampler {

// *************** AudioChannelPlugin ***************
// *

    AudioOutputDevicePlugin::AudioChannelPlugin::AudioChannelPlugin(uint ChannelNr) :
        AudioChannel(ChannelNr, 0, 0) {
    }


// *************** ParameterChannelsPlugin  ***************
// *

    void AudioOutputDevicePlugin::ParameterChannelsPlugin::ForceSetValue(int channels) {
        OnSetValue(channels);
        iVal = channels;
    }


// *************** ParameterFragmentSize ***************
// *

    AudioOutputDevicePlugin::ParameterFragmentSize::ParameterFragmentSize() : DeviceCreationParameterInt() {
        InitWithDefault();
    }

    AudioOutputDevicePlugin::ParameterFragmentSize::ParameterFragmentSize(String s) throw (Exception) : DeviceCreationParameterInt(s) {
    }

    String AudioOutputDevicePlugin::ParameterFragmentSize::Description() {
        return "Size of each buffer fragment";
    }

    bool AudioOutputDevicePlugin::ParameterFragmentSize::Fix() {
        return true;
    }

    bool AudioOutputDevicePlugin::ParameterFragmentSize::Mandatory() {
        return true;
    }

    std::map<String,DeviceCreationParameter*> AudioOutputDevicePlugin::ParameterFragmentSize::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>(); // no dependencies
    }

    optional<int> AudioOutputDevicePlugin::ParameterFragmentSize::DefaultAsInt(std::map<String,String> Parameters) {
        return optional<int>::nothing;
    }

    optional<int> AudioOutputDevicePlugin::ParameterFragmentSize::RangeMinAsInt(std::map<String,String> Parameters) {
        return optional<int>::nothing;
    }

    optional<int> AudioOutputDevicePlugin::ParameterFragmentSize::RangeMaxAsInt(std::map<String,String> Parameters) {
        return optional<int>::nothing;
    }

    std::vector<int> AudioOutputDevicePlugin::ParameterFragmentSize::PossibilitiesAsInt(std::map<String,String> Parameters) {
        return std::vector<int>();
    }

    void AudioOutputDevicePlugin::ParameterFragmentSize::OnSetValue(int i) throw (Exception) {
        // not posssible, as parameter is fix
    }

    String AudioOutputDevicePlugin::ParameterFragmentSize::Name() {
        return "FRAGMENTSIZE";
    }



// *************** AudioOutputDevicePlugin ***************
// *

    AudioOutputDevicePlugin::AudioOutputDevicePlugin(std::map<String, DeviceCreationParameter*> Parameters) : AudioOutputDevice(Parameters) {
        uiSampleRate = dynamic_cast<DeviceCreationParameterInt*>(
            Parameters["SAMPLERATE"])->ValueAsInt();
        uiMaxSamplesPerCycle = dynamic_cast<DeviceCreationParameterInt*>(
            Parameters["FRAGMENTSIZE"])->ValueAsInt();

        // create audio channels
        AcquireChannels(dynamic_cast<DeviceCreationParameterInt*>(
                            Parameters["CHANNELS"])->ValueAsInt());
    }

    void AudioOutputDevicePlugin::Play() {
    }

    bool AudioOutputDevicePlugin::IsPlaying() {
        return true;
    }

    void AudioOutputDevicePlugin::Stop() {
    }

    uint AudioOutputDevicePlugin::MaxSamplesPerCycle() {
        return uiMaxSamplesPerCycle;
    }

    uint AudioOutputDevicePlugin::SampleRate() {
        return uiSampleRate;
    }

    String AudioOutputDevicePlugin::Driver() {
        return Name();
    }

    String AudioOutputDevicePlugin::Name() {
        return "Plugin";
    }

    String AudioOutputDevicePlugin::Version() {
        String s = "$Revision: 3054 $";
        return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
    }

    String AudioOutputDevicePlugin::Description() {
        return Name();
    }

    AudioChannel* AudioOutputDevicePlugin::CreateChannel(uint ChannelNr) {
        return new AudioChannelPlugin(ChannelNr);
    }

    bool AudioOutputDevicePlugin::isAutonomousDevice() {
        return false;
    }

    bool AudioOutputDevicePlugin::isAutonomousDriver() {
        return false;
    }


    void AudioOutputDevicePlugin::AddChannels(int newChannels) {
        static_cast<ParameterChannelsPlugin*>(
            Parameters["CHANNELS"])->ForceSetValue(int(Channels.size() + newChannels));
    }

    void AudioOutputDevicePlugin::RemoveChannel(AudioChannel* pChannel) {
        std::vector<AudioChannel*>::iterator i = find(Channels.begin(), Channels.end(), pChannel);
        int channelNumber = int(i - Channels.begin());
        delete *i;
        Channels.erase(i);

        for ( ; channelNumber < Channels.size() ; channelNumber++) {
            static_cast<AudioChannelPlugin*>(Channels[channelNumber])->ChannelNr = channelNumber;
            Channels[channelNumber]->ChannelParameters()["NAME"]->SetValue("Channel " + ToString(channelNumber));
        }
        static_cast<ParameterChannelsPlugin*>(
            Parameters["CHANNELS"])->ForceSetValue(int(Channels.size()));
    }
}

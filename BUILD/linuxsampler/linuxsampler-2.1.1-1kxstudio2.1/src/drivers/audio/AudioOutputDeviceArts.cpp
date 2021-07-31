/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2006 Christian Schoenebeck                              *
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

#include "AudioOutputDeviceArts.h"
#include "AudioOutputDeviceFactory.h"

//TODO: this driver currently allows and expects only a bit depth of 16 !
#define LS_ARTS_BITDEPTH	16

namespace LinuxSampler {

    /// number of currently existing aRts audio output devices in LinuxSampler
    static int existingArtsDevices = 0;



// *************** ParameterName ***************
// *

    AudioOutputDeviceArts::ParameterName::ParameterName() : DeviceCreationParameterString() {
        InitWithDefault(); // use default name
    }

    AudioOutputDeviceArts::ParameterName::ParameterName(String s) throw (Exception) : DeviceCreationParameterString(s) {
    }

    String AudioOutputDeviceArts::ParameterName::Description() {
        return "Arbitrary aRts client name";
    }

    bool AudioOutputDeviceArts::ParameterName::Fix() {
        return true;
    }

    bool AudioOutputDeviceArts::ParameterName::Mandatory() {
        return false;
    }

    std::map<String,DeviceCreationParameter*> AudioOutputDeviceArts::ParameterName::DependsAsParameters() {
        return std::map<String,DeviceCreationParameter*>(); // no dependencies
    }

    std::vector<String> AudioOutputDeviceArts::ParameterName::PossibilitiesAsString(std::map<String,String> Parameters) {
        return std::vector<String>();
    }

    optional<String> AudioOutputDeviceArts::ParameterName::DefaultAsString(std::map<String,String> Parameters) {
        return (existingArtsDevices) ? "LinuxSampler" + ToString(existingArtsDevices) : "LinuxSampler";
    }

    void AudioOutputDeviceArts::ParameterName::OnSetValue(String s) throw (Exception) {
        // not possible, as parameter is fix
    }

    String AudioOutputDeviceArts::ParameterName::Name() {
        return "NAME";
    }



// *************** AudioOutputDeviceArts ***************
// *

    /**
     * Create and initialize aRts audio output device with given parameters.
     *
     * @param Parameters - optional parameters
     * @throws AudioOutputException  if output device cannot be opened
     */
    AudioOutputDeviceArts::AudioOutputDeviceArts(std::map<String,DeviceCreationParameter*> Parameters) : AudioOutputDevice(Parameters), Thread(true, true, 1, 0) {
        uiArtsChannels = ((DeviceCreationParameterInt*)Parameters["CHANNELS"])->ValueAsInt();
        uiSampleRate   = ((DeviceCreationParameterInt*)Parameters["SAMPLERATE"])->ValueAsInt();
        String name    = ((DeviceCreationParameterString*)Parameters["NAME"])->ValueAsString();

        int res;

        // initialize the aRts API in case there is no aRts audio output device yet
        if (!existingArtsDevices) {
            res = arts_init();
            if (res) throw AudioOutputException(String("arts_init() failed (err ") + ToString(res) + ")");
        }
        existingArtsDevices++;

        hStream      = arts_play_stream(uiSampleRate, LS_ARTS_BITDEPTH, uiArtsChannels, name.c_str());
        uiPacketSize = arts_stream_get(hStream, ARTS_P_PACKET_SIZE);
        FragmentSize = uiPacketSize / (LS_ARTS_BITDEPTH / 8 * uiArtsChannels);

        // create aRts audio output buffer
        pArtsOutputBuffer = new int16_t[uiPacketSize / (LS_ARTS_BITDEPTH / 8)];

        // create audio channels for this audio device to which the sampler engines can write to
        for (int i = 0; i < uiArtsChannels; i++)
            this->Channels.push_back(new AudioChannel(i, FragmentSize));

        // finally activate device if desired
        if (((DeviceCreationParameterBool*)Parameters["ACTIVE"])->ValueAsBool()) Play();
    }

    AudioOutputDeviceArts::~AudioOutputDeviceArts() {
        //dmsg(0,("Stopping aRts Thread..."));
        //StopThread();  //FIXME: commented out due to a bug in thread.cpp (StopThread() doesn't return at all)
        //dmsg(0,("OK\n"));
        arts_close_stream(hStream);
        existingArtsDevices--;
        if (!existingArtsDevices) arts_free();
        if (pArtsOutputBuffer) delete[] pArtsOutputBuffer;
    }

    void AudioOutputDeviceArts::Play() {
        StartThread();
    }

    bool AudioOutputDeviceArts::IsPlaying() {
        return IsRunning(); // if Thread is running
    }

    void AudioOutputDeviceArts::Stop() {
        StopThread();
    }

    AudioChannel* AudioOutputDeviceArts::CreateChannel(uint ChannelNr) {
        // just create a mix channel
        return new AudioChannel(ChannelNr, Channel(ChannelNr % uiArtsChannels));
    }

    uint AudioOutputDeviceArts::MaxSamplesPerCycle() {
        return FragmentSize;
    }

    uint AudioOutputDeviceArts::SampleRate() {
        return uiSampleRate;
    }

    String AudioOutputDeviceArts::Name() {
        return "ARTS";
    }

    String AudioOutputDeviceArts::Driver() {
        return Name();
    }

    String AudioOutputDeviceArts::Description() {
        return "Analog Realtime Synthesizer";
    }

    String AudioOutputDeviceArts::Version() {
       String s = "$Revision: 2494 $";
       return s.substr(11, s.size() - 13); // cut dollar signs, spaces and CVS macro keyword
    }

    /**
     * Entry point for the thread.
     */
    int AudioOutputDeviceArts::Main() {
        while (true) {

            // let all connected engines render 'FragmentSize' sample points
            RenderAudio(FragmentSize);

            // convert from DSP value range (-1.0..+1.0) to 16 bit integer value
            // range (-32768..+32767), check clipping and copy to aRts output buffer
            for (int c = 0; c < uiArtsChannels; c++) {
                float* in  = Channels[c]->Buffer();
                for (int i = 0, o = c; i < FragmentSize; i++ , o += uiArtsChannels) {
                    float sample_point = in[i] * 32768.0f;
                    if (sample_point < -32768.0) sample_point = -32768.0;
                    if (sample_point >  32767.0) sample_point =  32767.0;
                    pArtsOutputBuffer[o] = (int16_t) sample_point;
                }
            }

            // output sound
            int res = arts_write(hStream, pArtsOutputBuffer, uiPacketSize);
            if (res <= 0) {
                fprintf(stderr, "AudioOutputDeviceArts: Audio output error, exiting.\n");
                exit(EXIT_FAILURE);
            }
        }
    }

} // namespace LinuxSampler

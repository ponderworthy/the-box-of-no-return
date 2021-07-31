/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2014 Christian Schoenebeck                       *
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

#ifndef __LS_AUDIOOUTPUTDEVICEJACK_H__
#define __LS_AUDIOOUTPUTDEVICEJACK_H__

#include "../../common/global_private.h"

#if HAVE_JACK

#include <vector>
#include <sstream>
#include <jack/jack.h>

#include "AudioOutputDevice.h"
#include "../../common/ConditionServer.h"

#if HAVE_JACK_MIDI
#include "../midi/MidiInputDeviceJack.h"
#endif

namespace LinuxSampler {
    
    class JackClient;
    class JackListener;

    /** JACK audio output driver
     *
     * Implements audio output to the JACK Audio Connection Kit (JACK).
     */
    class AudioOutputDeviceJack : public AudioOutputDevice {
        public:
            AudioOutputDeviceJack(std::map<String,DeviceCreationParameter*> Parameters);
            virtual ~AudioOutputDeviceJack();

            /**
             * Audio channel implementation for the JACK audio driver.
             */
            class AudioChannelJack : public AudioChannel {
                public:
                    /** Audio Channel Parameter 'NAME'
                     *
                     * Used to assign an arbitrary name to an audio channel.
                     */
                    class ParameterName : public AudioChannel::ParameterName {
                        public:
                            ParameterName(AudioChannelJack* pChannel);
                            virtual void OnSetValue(String s) OVERRIDE;
                        protected:
                            AudioChannelJack* pChannel;
                    };

                    /** Audio Channel Parameter 'JACK_BINDINGS'
                     *
                     * Used to connect to other JACK clients.
                     */
                    class ParameterJackBindings : public DeviceRuntimeParameterStrings {
                        public:
                            ParameterJackBindings(AudioChannelJack* pChannel);
                            virtual String              Description() OVERRIDE;
                            virtual bool                Fix() OVERRIDE;
                            virtual std::vector<String> PossibilitiesAsString() OVERRIDE;
                            virtual void                OnSetValue(std::vector<String> vS) OVERRIDE;
                            static String Name();
                        protected:
                            AudioChannelJack*   pChannel;
                            std::vector<String> Bindings;
                    };
                protected:
                    AudioChannelJack(uint ChannelNr, AudioOutputDeviceJack* pDevice) throw (AudioOutputException);
                    ~AudioChannelJack();
                    void UpdateJackBuffer(uint size);
                    friend class AudioOutputDeviceJack;
                private:
                    AudioOutputDeviceJack* pDevice;
                    jack_port_t*           hJackPort;
                    uint                   ChannelNr;

                    float* CreateJackPort(uint ChannelNr, AudioOutputDeviceJack* pDevice) throw (AudioOutputException);
            };

            /** Audio Device Parameter 'NAME'
             *
             * Used to assign an arbitrary name to the JACK client of this
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

            /** Audio Device Parameter 'SAMPLERATE'
             *
             * Used to retrieve the sample rate of the JACK audio output
             * device. Even though the sample rate of the JACK server might
             * change during runtime, the JACK API currently however does not
             * allow clients to change the sample rate. So this parameter is
             * read only.
             *
             * This base parameter class has just been overridden for this JACK
             * driver to implement a valid default value for sample rate. The
             * default value will simply return the sample rate of the currently
             * running JACK server. It will return "nothing" if the JACK server
             * is not running at that point.
             */
            class ParameterSampleRate : public AudioOutputDevice::ParameterSampleRate {
                public:
                    ParameterSampleRate();
                    ParameterSampleRate(String s);
                    virtual optional<int> DefaultAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual void OnSetValue(int i) throw (Exception) OVERRIDE;
            };

            // derived abstract methods from class 'AudioOutputDevice'
            virtual void Play() OVERRIDE;
            virtual bool IsPlaying() OVERRIDE;
            virtual void Stop() OVERRIDE;
            virtual uint MaxSamplesPerCycle() OVERRIDE;
            virtual uint SampleRate() OVERRIDE;
            virtual AudioChannel* CreateChannel(uint ChannelNr) OVERRIDE;
            virtual String Driver() OVERRIDE;
            virtual float latency() OVERRIDE;
            
            static String Name();

            static String Description();
            static String Version();

            int Process(uint Samples);  // FIXME: should be private
            void UpdateJackBuffers(uint size);
            void addListener(JackListener* listener);
            jack_client_t* jackClientHandle();
        protected:
            AudioOutputDeviceJack(String* AutoConnectPortIDs = NULL, uint AutoConnectPorts = 0);
        private:
            ConditionServer csIsPlaying;
            uint            uiMaxSamplesPerCycle;
            jack_client_t*  hJackClient;
            JackClient* pJackClient;
    };

    // Callback functions for the libjack API
    int  linuxsampler_libjack_process_callback(jack_nframes_t nframes, void* arg);

    /** JACK client
     *
     * Represents a jack client. This class is shared by
     * AudioOutputDeviceJack and MidiInputDeviceJack. The jack server
     * calls JackClient::Process, which in turn calls
     * AudioOutputDeviceJack::Process and/or
     * MidiInputDeviceJack::Process.
     */
    class JackClient {
        public:
            static JackClient* CreateAudio(String Name);
            static JackClient* CreateMidi(String Name);
            static void ReleaseAudio(String Name);
            static void ReleaseMidi(String Name);
            int Process(uint Samples);
            void Stop();
            void SetAudioOutputDevice(AudioOutputDeviceJack* device);
            #if HAVE_JACK_MIDI
            void SetMidiInputDevice(MidiInputDeviceJack* device);
            #endif
            void addListener(JackListener* listener);

            jack_client_t* hJackClient;

        private:
            std::vector<JackListener*> jackListeners;
            static std::map<String, JackClient*> Clients;
            struct config_t {
                AudioOutputDeviceJack* AudioDevice;
                #if HAVE_JACK_MIDI
                MidiInputDeviceJack* MidiDevice;
                #endif
            };
            SynchronizedConfig<config_t> Config;
            SynchronizedConfig<config_t>::Reader ConfigReader;
            bool audio;
            bool midi;

            JackClient(String Name);
            ~JackClient();
        
            // Callback functions for the libjack API
#if HAVE_JACK_ON_INFO_SHUTDOWN
            static void libjackShutdownCallback(jack_status_t code, const char* reason, void *arg);
#else
            static void libjackShutdownCallback(void *arg);
#endif
            static int libjackSampleRateCallback(jack_nframes_t nframes, void *arg);
            static int libjackBufferSizeCallback(jack_nframes_t nframes, void *arg);
    };
    
    /**
     * Currently not derived / instantiated by the sampler itself, however this
     * class can be subclassed and used i.e. by a GUI build on top of the sampler,
     * to react on JACK events. Because registering JACK callback functions through
     * the general JACK API is not possible after the JACK client has been activated,
     * and the latter is already the case as soon as an AudioOutputDeviceJack object
     * has been instantiated.
     */
    class JackListener {
    public:
        virtual void onJackShutdown(jack_status_t code, const char* reason) = 0;
    };
}

#endif // HAVE_JACK
#endif // __LS_AUDIOOUTPUTDEVICEJACK_H__

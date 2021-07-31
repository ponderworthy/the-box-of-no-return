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

#ifndef __LS_AUDIOOUTPUTDEVICE_H__
#define __LS_AUDIOOUTPUTDEVICE_H__

#include <set>
#include <map>
#include <vector>
#include <stdexcept>

#include "../../common/global.h"
#include "../../common/Exception.h"
#include "../Device.h"
#include "../DeviceParameter.h"
#include "../../engines/Engine.h"
#include "AudioChannel.h"
#include "../../common/SynchronizedConfig.h"
#include "../../effects/EffectChain.h"

namespace LinuxSampler {

    // just symbol prototyping
    class Engine;
    class AudioOutputDeviceFactory;
    class IDGenerator;

    /** Abstract base class for audio output drivers in LinuxSampler
     *
     * This class will be derived by specialized classes which implement the
     * connection to a specific audio output system (e.g. Alsa, Jack,
     * CoreAudio).
     */
    class AudioOutputDevice : public Device {
        public:

            /////////////////////////////////////////////////////////////////
            // Device parameters

            /** Device Parameter 'ACTIVE'
             *
             * Used to activate / deactivate the audio output device.
             */
            class ParameterActive : public DeviceCreationParameterBool {
                public:
                    ParameterActive();
                    ParameterActive(String s);
                    virtual String Description() OVERRIDE;
                    virtual bool   Fix() OVERRIDE;
                    virtual bool   Mandatory() OVERRIDE;
                    virtual std::map<String,DeviceCreationParameter*> DependsAsParameters() OVERRIDE;
                    virtual optional<bool> DefaultAsBool(std::map<String,String> Parameters) OVERRIDE;
                    virtual void OnSetValue(bool b) throw (Exception) OVERRIDE;
                    static String Name();
            };

            /** Device Parameter 'SAMPLERATE'
             *
             * Used to set the sample rate of the audio output device.
             */
            class ParameterSampleRate : public DeviceCreationParameterInt {
                public:
                    ParameterSampleRate();
                    ParameterSampleRate(String s);
                    virtual String Description() OVERRIDE;
                    virtual bool   Fix() OVERRIDE;
                    virtual bool   Mandatory() OVERRIDE;
                    virtual std::map<String,DeviceCreationParameter*> DependsAsParameters() OVERRIDE;
                    virtual optional<int>    DefaultAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual optional<int>    RangeMinAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual optional<int>    RangeMaxAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual std::vector<int> PossibilitiesAsInt(std::map<String,String> Parameters) OVERRIDE;
                    virtual int              ValueAsInt() OVERRIDE;
                    virtual void             OnSetValue(int i) throw (Exception) OVERRIDE;
                    static String Name();
            };

            /** Device Parameters 'CHANNELS'
             *
             * Used to increase / decrease the number of audio channels of
             * audio output device.
             */
            class ParameterChannels : public DeviceCreationParameterInt {
                public:
                    ParameterChannels();
                    ParameterChannels(String s);
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



            /////////////////////////////////////////////////////////////////
            // abstract methods
            //     (these have to be implemented by the descendant)

            /**
             * Start playback of audio signal on the audio device. It's the
             * responsibility of the implementing audio device to call the
             * RenderAudio(uint Samples) method of all connected engines.
             * This will cause the engines to continue to render 'Samples'
             * number of audio sample points and the engines will
             * automatically add their audio signals to the audio buffers of
             * the audio channels of this audio device. So the implementing
             * audio device just has to access the buffers of it's audio
             * channels.
             *
             * @throws AudioOutputException  if playback can not be started
             * @see AudioChannel
             */
            virtual void Play() = 0;

            /**
             * Returns true if the audio device is currently playing back.
             */
            virtual bool IsPlaying() = 0;

            /**
             * Stop playback of audio signal on the audio device. The
             * implementing audio device will stop calling the RenderAudio()
             * method of all connected engines and close it's connection to
             * audio output system.
             */
            virtual void Stop() = 0;

            /**
             * Maximum amount of sample points the implementing audio
             * device will ever demand the sampler engines to write in one
             * fragment cycle / period. Simple audio device drivers usually
             * have a fixed fragment size, so those devices just would return
             * the fragment size in this method.
             *
             * @returns  max. amount of sample points ever
             */
            virtual uint MaxSamplesPerCycle() = 0;

            /**
             * Playback samplerate the audio device uses. The sampler engines
             * currently assume this to be a constant value for the whole
             * life time of an instance of the implementing audio device.
             *
             * @returns  sample rate in Hz
             */
            virtual uint SampleRate() = 0;

            /**
             * Return the audio output device driver name.
             */
            virtual String Driver() = 0;

            /**
             * Create new audio channel. This will be called by
             * AcquireChannels(). Each driver must implement it.
             */
            virtual AudioChannel* CreateChannel(uint ChannelNr) = 0;
        
            /**
             * Might be optionally implemented by the deriving driver. If
             * implemented, this method will return the current audio
             * latency in seconds. The driver might measure the actual
             * latency with timing functions to return the real latency.
             *
             * If this method is not implemented / overridden by the
             * descending driver, it will simply return the theoretical
             * latency, that is MaxSamplesPerCycle() / SampleRate().
             *
             * Currently this method is not used by the sampler itself.
             * It can be used however for GUIs built on top of the sampler
             * for example, to i.e. display the user the currently
             * effective audio latency in real-time.
             */
            virtual float latency();



            /////////////////////////////////////////////////////////////////
            // normal methods
            //     (usually not to be overriden by descendant)

            /**
             * Connect given sampler engine to this audio output device. The
             * engine will be added to the Engines container of this audio
             * device and the engine will also automatically be informed
             * about the connection.
             *
             * @param pEngine - sampler engine
             */
            void Connect(Engine* pEngine);

            /**
             * Disconnect given sampler engine from this audio output device.
             * Removes given sampler engine reference from the Engines
             * container of this audio device.
             *
             * @param pEngine - sampler engine
             */
            void Disconnect(Engine* pEngine);
        
            /**
             * This method can or should be called by the deriving driver in
             * case some fundamental parameter changed, especially if the
             * values returned by MaxSamplesPerCycle() and SamplerRate() have
             * changed, those are values which the sampler engines assume to be
             * constant for the life time of an audio device !
             *
             * By forcing a re-connection with this method, the sampler engines
             * are forced to update those informations metntioned above as well,
             * avoiding crashes or other undesired misbehaviors in such
             * circumstances.
             */
            void ReconnectAll();

            /**
             * Returns audio channel with index \a ChannelIndex or NULL if
             * index out of bounds.
             */
            AudioChannel* Channel(uint ChannelIndex);

            /**
             * This method will usually be called by the sampler engines that
             * are connected to this audio device to inform the audio device
             * how much audio channels the engine(s) need. It's the
             * responsibility of the audio device to offer that amount of
             * audio channels - again: this is not an option this is a must!
             * The engines will assume to be able to access those audio
             * channels right after. If the audio driver is not able to offer
             * that much channels, it can simply create mix channels which
             * are then just mixed to the 'real' audio channels. See
             * AudioChannel.h for details about channels and mix channels.
             *
             * @param Channels - amount of output channels required by
             *                   a sampler engine
             * @throws AudioOutputException  if desired amount of channels
             *                               cannot be offered
             * @see AudioChannel
             */
            void AcquireChannels(uint Channels);

            /**
             * Returns the amount of audio channels (including the so called
             * "mix channels") the device is currently providing.
             */
            uint ChannelCount();

            /**
             * Returns all device parameter settings.
             */
            std::map<String,DeviceCreationParameter*> DeviceParameters();

            /**
             * Add a chain of send effects to this AudioOutputDevice.
             * You actually have to add effects to that chain afterwards.
             */
            EffectChain* AddSendEffectChain();

            /**
             * Remove the send effect chain given by @a iChain .
             *
             * @throws Exception - if given send effect chain doesn't exist
             */
            void RemoveSendEffectChain(uint iChain) throw (Exception);

            /**
             * Returns send effect chain given by @a iChain or @c NULL if
             * there's no such effect chain.
             */
            EffectChain* SendEffectChain(uint iChain) const;

            /**
             * Returns send effect chain with ID @a iChainID or @c NULL if
             * there's no such effect chain.
             */
            EffectChain* SendEffectChainByID(uint iChainID) const;

            /**
             * Returns amount of send effect chains this AudioOutputDevice
             * currently provides.
             */
            uint SendEffectChainCount() const;

            /**
             * @deprecated This method will be removed, use AddSendEffectChain() instead!
             */
            EffectChain* AddMasterEffectChain() DEPRECATED_API;

            /**
             * @deprecated This method will be removed, use RemoveSendEffectChain() instead!
             */
            void RemoveMasterEffectChain(uint iChain) throw (Exception) DEPRECATED_API;

            /**
             * @deprecated This method will be removed, use SendEffectChain() instead!
             */
            EffectChain* MasterEffectChain(uint iChain) const DEPRECATED_API;

            /**
             * @deprecated This method will be removed, use SendEffectChainCount() instead!
             */
            uint MasterEffectChainCount() const DEPRECATED_API;

        protected:
            SynchronizedConfig<std::set<Engine*> >    Engines;     ///< All sampler engines that are connected to the audio output device.
            SynchronizedConfig<std::set<Engine*> >::Reader EnginesReader; ///< Audio thread access to Engines.
            std::vector<AudioChannel*>                Channels;    ///< All audio channels of the audio output device. This is just a container; the descendant has to create channels by himself.
            std::map<String,DeviceCreationParameter*> Parameters;  ///< All device parameters.
            std::vector<EffectChain*>                 vEffectChains;
            IDGenerator*                              EffectChainIDs;

            AudioOutputDevice(std::map<String,DeviceCreationParameter*> DriverParameters);

            virtual ~AudioOutputDevice();

            /**
             * This method should be called by the AudioOutputDevice
             * descendant to let all connected engines proceed to render the
             * given amount of sample points. The engines will place their
             * calculated audio data by themselfes into the buffers of the
             * respective AudioChannel objects, so the implementing audio
             * output device just has to copy the AudioChannel buffers to
             * the output buffer(s) of its audio system.
             *
             * @returns  0 on success or the last error return code of one
             *           engine
             */
            int RenderAudio(uint Samples);

            /**
             * This can be called as an alternative to RenderAudio() for
             * just writing silence to the audio output buffers and not
             * calling the connected sampler engines for rendering audio, so
             * to provide a method to stop playback if the used audio output
             * system doesn't provide a better way.
             *
             * @returns  0 on success
             */
            int RenderSilence(uint Samples);

            friend class AudioOutputDeviceFactory; // allow AudioOutputDeviceFactory class to destroy audio devices

    };

    /**
     * Audio output exception that should be thrown by the AudioOutputDevice
     * descendants in case initialization of the audio output system failed
     * (which should be done in the constructor of the AudioOutputDevice
     * descendant).
     */
    class AudioOutputException : public Exception {
        public:
            AudioOutputException(const std::string& msg) : Exception(msg) {}
    };
}

#endif // __LS_AUDIOOUTPUTDEVICE_H__

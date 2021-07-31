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

#ifndef __LS_SAMPLER_H__
#define __LS_SAMPLER_H__

#include <vector>
#include <map>
#include "EventListeners.h"
#include "common/global.h"
#include "common/Exception.h"
#include "engines/EngineChannel.h"
#include "drivers/midi/MidiInputDevice.h"
#include "drivers/audio/AudioOutputDevice.h"

namespace LinuxSampler {

    // just symbol prototyping
    class Sampler;

    /** @brief LinuxSampler sampler channel (a.k.a. "sampler part")
     *
     * Encapsulates a channel of a specific sampler engine type, one
     * connection to a MIDI input device and one connection to an audio
     * output device. You cannot create an instance of this class on your
     * own, you have to use the AddSamplerChannel() method of the Sampler
     * object to create a new sampler channel.
     */
    class SamplerChannel {
        public:
            /**
             * Assign a sampler engine type to this sampler channel.
             *
             * @param EngineType - type of the engine to use
             * @throws Exception - if \a EngineType is invalid
             * @see Sampler::AvailableEngineTypes()
             */
            void SetEngineType(String EngineType) throw (Exception);

            /**
             * Connect this sampler channel to an audio output device, that
             * is an instance of an audio output driver. If this sampler
             * channel was already connected to an audio output device, then
             * the old connection will automatically be removed before.
             *
             * @param pDevice - audio output device to connect to
             * @throws Exception in case the audio device is tried to be
             *                   changed while the sampler channel is used by a
             *                   host plugin (e.g. VST, AU, DSSI, LV2) which
             *                   don't allow to change the audio output device
             */
            void SetAudioOutputDevice(AudioOutputDevice* pDevice) throw (Exception);

            /**
             * Connect the given MIDIInputPort to this SamplerChannel. The
             * connection is added to the sampler channel. So other MIDI input
             * connections remain unaffected by this call. If the given port is
             * already connected to this sampler channel, then this call is
             * ignored.
             *
             * @param pPort - MIDI input port to connect to
             * @throws Exception in case the MIDI device is tried to be changed
             *                   while the sampler channel is being used by a
             *                   host plugin (e.g. VST, AU, DSSI, LV2) which
             *                   don't allow to change the MIDI port or even
             *                   device
             */
            void Connect(MidiInputPort* pPort) throw (Exception);

            /**
             * Disconnects the given MidiInputPort from this SamplerChannel.
             * All other MIDI input ports connected to this sampler channel
             * remain unaffected. If the given port is not currently connected
             * to this sampler channel, then this call is ignored.
             *
             * @param pPort - MIDI input port to disconnect
             * @throws Exception in case the MIDI device is tried to be changed
             *                   while the sampler channel is being used by a
             *                   host plugin (e.g. VST, AU, DSSI, LV2) which
             *                   don't allow to change the MIDI port or even
             *                   device
             */
            void Disconnect(MidiInputPort* pPort) throw (Exception);

            /**
             * Disconnects all MIDI input ports currently connected with this
             * SamplerChannel.
             *
             * @throws Exception in case the MIDI device is tried to be changed
             *                   while the sampler channel is being used by a
             *                   host plugin (e.g. VST, AU, DSSI, LV2) which
             *                   don't allow to change the MIDI port or even
             *                   device
             */
            void DisconnectAllMidiInputPorts() throw (Exception);

            /**
             * Returns all MIDI input ports currently connected to this sampler
             * channel.
             */
            std::vector<MidiInputPort*> GetMidiInputPorts();

            /**
             * Connect this sampler channel to a MIDI input device.
             *
             * This call will also disconnect <b>all</b> existing MIDI input
             * connections from this sampler channel before establishing the
             * new connection! Disconnection of all previous connections is
             * done to preserve full behavior backward compatibility to times
             * when this API only allowed one MIDI input port per sampler
             * channel.
             *
             * @param pDevice - MIDI input device to connect to
             * @throws Exception in case the MIDI device is tried to be changed
             *                   while the sampler channel is being used by a
             *                   host plugin (e.g. VST, AU, DSSI, LV2) which
             *                   don't allow to change the MIDI port or even
             *                   device
             * @deprecated This method is only provided for backward
             *             compatibility. It is a relict from days where there
             *             was only 1 MIDI input allowed per SamplerChannel.
             */
            void SetMidiInputDevice(MidiInputDevice *pDevice) throw (Exception) DEPRECATED_API;

            /**
             * Change the MIDI input port connected to this sampler channel.
             *
             * Calling this method will switch the connection of the first
             * (and only the first) MIDIInputPort currently being connected to
             * this sampler channel, to another port of the same
             * MidiInputDevice. Or in other words: the first MIDIInputPort
             * currently connected to this sampler channel will be disconnected,
             * and the requested other port of its MIDIInputDevice will be
             * connected to this sampler channel instead.
             *
             * This behavior is implemented to preserve full behavior backward
             * compatibility to times when this API only allowed one MIDI input
             * port per SamplerChannel.
             *
             * @param MidiPort - MIDI port to connect to
             * @throws Exception in case the MIDI port is tried to be changed
             *                   while the sampler channel is being used by a
             *                   host plugin (e.g. VST, AU, DSSI, LV2) which
             *                   don't allow to change the MIDI port
             * @deprecated This method is only provided for backward
             *             compatibility. It is a relict from days where there
             *             was only 1 MIDI input allowed per SamplerChannel.
             */
            void SetMidiInputPort(int MidiPort) throw (Exception) DEPRECATED_API;

            /**
             * Define on which MIDI channel(s) this sampler channel should
             * listen to (on all MIDI ports and all virtual MIDI devices
             * connected to this sampler channel). By default, that is after
             * creation of a new sampler channel, the sampler channel will
             * listen to all MIDI channels (a.k.a. "MIDI Omni mode").
             *
             * @param MidiChannel - MIDI channel to listen
             * @throws Exception if provided MidiChannel is not a valid constant
             *                   as defined by midi_chan_t
             */
            void SetMidiInputChannel(midi_chan_t MidiChannel);

            /**
             * Connect this sampler channel to a MIDI input triplet.
             *
             * This call will also disconnect <b>all</b> existing MIDI input
             * connections from this sampler channel before establishing the
             * new connection! Disconnection of all previous connections is
             * done to preserve full behavior backward compatibility to times
             * when this API only allowed one MIDI input port per sampler
             * channel.
             *
             * @param pDevice - MIDI input device to connect to
             * @param iMidiPort - MIDI port to connect to
             * @param MidiChannel - optional: MIDI channel on which the
             *                      sampler channel should listen to
             *                      (default: listen on all MIDI channels)
             * @throws Exception in case the MIDI port is tried to be changed
             *                   while the sampler channel is being used by a
             *                   host plugin (e.g. VST, AU, DSSI, LV2) which
             *                   don't allow to change the MIDI port
             * @deprecated This method is only provided for backward
             *             compatibility. It is a relict from days where there
             *             was only 1 MIDI input allowed per sampler channel.
             */
            void SetMidiInput(MidiInputDevice* pDevice, int iMidiPort, midi_chan_t MidiChannel = midi_chan_all) throw (Exception) DEPRECATED_API;

            /**
             * Returns the EngineChannel object that was deployed on this
             * sampler channel appropriate to the given sampler engine type.
             *
             * @returns  pointer to engine or NULL if no engine deployed
             */
            EngineChannel* GetEngineChannel();

            /**
             * Returns the MIDI input channel to which this sampler
             * channel is currently connected to.
             *
             * @returns  The MIDI input channel on which the sampler
             *           channel is listening to.
             */
            midi_chan_t GetMidiInputChannel();

            /**
             * Returns the MIDI input port number to which this sampler
             * channel is currently connected to.
             *
             * This method should not be used in new applications anymore!
             *
             * @returns  MIDI input port number or -1 if not connected
             * @deprecated This method is only provided for backward
             *             compatibility. It is a relict from days where there
             *             was only 1 MIDI input allowed per sampler channel.
             */
            int GetMidiInputPort() DEPRECATED_API;

            /**
             * Returns the audio output device to which this sampler channel
             * is currently connected to.
             *
             * @returns  pointer to audio output device or NULL if not
             *           connected
             */
            AudioOutputDevice* GetAudioOutputDevice();

            /**
             * Returns the MIDI input device to which this sampler channel
             * is currently connected to.
             *
             * @returns  pointer to MIDI input device or NULL if not
             *           connected
             * @deprecated This method is only provided for backward
             *             compatibility. It is a relict from days where there
             *             was only 1 MIDI input allowed per sampler channel.
             */
            MidiInputDevice* GetMidiInputDevice() DEPRECATED_API;

            /**
             * Returns the index number of this sampler channel within the
             * Sampler instance.
             */
            uint Index();

            /** Returns the sampler to which this channel belongs */
            Sampler* GetSampler();

            ///////////////////////////////////////////////////////////////
            // Event Listener methods

            /**
             * Registers the specified listener to be notified
             * when the engine type of this sampler channel is changed.
             */
            void AddEngineChangeListener(EngineChangeListener* l);

            /**
             * Removes the specified listener.
             */
            void RemoveEngineChangeListener(EngineChangeListener* l);

            /**
             * Removes the specified listener.
             */
            void RemoveAllEngineChangeListeners();

            /**
             * Notifies listeners that the engine type of this sampler
             * channel is going to be changed soon.
             */
            void fireEngineToBeChanged();

            /**
             * Notifies listeners that the engine
             * type of this sampler channel is changed.
             */
            void fireEngineChanged();


        protected:
            SamplerChannel(Sampler* pS);
            virtual ~SamplerChannel();

            Sampler*           pSampler;
            EngineChannel*     pEngineChannel;
            AudioOutputDevice* pAudioOutputDevice; //FIXME: should be stored as numeric device ID instead of raw pointer to avoid pointer invalidation problems
            int                iIndex;

            friend class Sampler;

        private:
            struct midi_conn_t {
                uint deviceID;
                uint portNr;

                bool operator== (const midi_conn_t& other) const {
                    return other.deviceID == this->deviceID &&
                           other.portNr   == this->portNr;
                }

                bool operator< (const midi_conn_t& other) const {
                    return memcmp(this, &other, sizeof(midi_conn_t)) < 0;
                }
            };

            int                iMidiPort;   ///< Don't access directly, read GetMidiInputPort() instead ! @deprecated This variable is just for backward compatibility from days when there was only one MIDI connection per SamplerChannel.
            midi_chan_t        midiChannel; ///< Don't access directly, read GetMidiInputChannel() instead !
            std::vector<midi_conn_t> vMidiInputs; ///< MIDI input ports connected to this sampler channel. Only used as "cache" (device id, port nr pair) in initial situation where no engine type is selected yet, and accordingly no EngineChannel instance exists which actually manages the device connections. This way users can "connect" MIDI input ports to this SamplerChannel before an engine type is chosen.
            ListenerList<EngineChangeListener*> llEngineChangeListeners;

            static MidiInputPort* _getPortForID(const midi_conn_t& c);
    };

    /** @brief LinuxSampler main class
     *
     * This is the toplevel class for a LinuxSampler instance.
     *
     * LinuxSampler can have arbitrary numbers of sampler channels. Each
     * sampler channel (a.k.a. "sampler part") can individually be deployed
     * with it's own sampler engine, connected to an arbitrary MIDI input
     * device and connected to an arbitrary audio output device. Here an
     * example setup:
     * @code
     * S.Channel    MIDI in    S.Engine         Audio out
     * -------------------------------------------------------------------
     *   0          Alsa   ->  gig::Engine  ->  Jack
     *   1          VSTi   ->  gig::Engine  ->  VSTi
     *   2          Jack   ->  sfz::Engine  ->  Jack
     *   3          Jack   ->  SF2::Engine  ->  Alsa
     *   4          LV2    ->  sfz::Engine  ->  LV2
     *
     * ... (and so on) ...
     * @endcode
     *
     * As you can see in the example setup, LinuxSampler is capable to use
     * several, different audio output and MIDI input systems
     * simultaniously at the same time. Here the example setup shown in the
     * aspect of MIDI input and audio output devices / drivers:
     * @code
     *                      ######################### #########################
     *                      # AudioOutputDeviceJack # # AudioOutputDeviceVSTi #
     *                      ######################### #########################
     *                                        ^   ^           ^
     *  /------------>|Sampler Channel 0|-----/   |           |
     *  |  /--------->|Sampler Channel 1|---------------------/
     *  |  |  /------>|Sampler Channel 2|---------/
     *  |  |  |  /--->|Sampler Channel 3|------------>#########################
     *  |  |  |  |    ... (and so on) ...             # AudioOutputDeviceAlsa #
     *  |  |  |  |                                    #########################
     *  |  |  |  \-----------------------------------------------------\
     *  |  |  \--------------------------------------------\           |
     *  |  \--------------------\                          |           |
     *  |                       |                          |           |
     * ####################### ####################### #######################
     * # MidiInputDeviceAlsa # # MidiInputDeviceVSTi # # MidiInputDeviceJack #
     * ####################### ####################### #######################
     * @endcode
     *
     * As you can see in this example setup, one device (that is midi input
     * driver / audio output driver) can be connected multiple times to
     * different sampler channels.
     *
     * It's even possible to create multiple instances of the same driver, for
     * example multiple instances of the Alsa output driver to use multiple
     * sound cards at the same time, or multiple instances of the JACK audio
     * output driver to leverage SMP systems or boxes with several hard discs.
     */
    class Sampler {
        public:
            /**
             * Constructor. Create a LinuxSampler instance.
             */
            Sampler();

            /**
             * Destructor.
             */
            virtual ~Sampler();

            /**
             * Returns the number of sampler channels currently allocated.
             */
            uint SamplerChannels();

            /**
             * Create and add a new sampler channel to this Sampler
             * instance. For race condition reasons the new channel will use
             * an index past the last already existing sampler channel
             * index (in case the index limit was not reached yet, otherwise
             * a free index starting from 0 is searched).
             *
             * @returns  pointer to new sampler channel
             */
            SamplerChannel* AddSamplerChannel();

            /**
             * Returns the sampler channel of the given sampler channel
             * index.
             *
             * @returns  pointer to sought sampler channel
             */
            SamplerChannel* GetSamplerChannel(uint uiSamplerChannel);

            /**
             * Returns all created sampler channels.
             */
            std::map<uint, SamplerChannel*> GetSamplerChannels();

            /**
             * Destroy and remove the given sampler channel from this
             * Sampler instance.
             *
             * @param pSamplerChannel - pointer to sampler channel to remove
             */
            void RemoveSamplerChannel(SamplerChannel* pSamplerChannel);

            /**
             * Destroy and remove the given sampler channel from this
             * Sampler instance.
             *
             * @param uiSamplerChannel - index of the sampler channel to
             *                           remove
             */
            void RemoveSamplerChannel(uint uiSamplerChannel);

            /**
             * Destroy and remove all sampler channels from this
             * Sampler instance.
             */
            void RemoveAllSamplerChannels();

            /**
             * Returns the names of all available audio output drivers.
             */
            std::vector<String> AvailableAudioOutputDrivers();

            /**
             * Returns the names of all available MIDI input drivers.
             */
            std::vector<String> AvailableMidiInputDrivers();

            /**
             * Returns the names of all available sampler engine types.
             * @see SamplerChannel::SetEngineType()
             */
            std::vector<String> AvailableEngineTypes();

            /**
             * Create an audio output device.
             *
             * @param AudioDriver - name of the audio driver
             * @param Parameters - eventually needed driver parameters to
             *                     create the device
             * @returns  pointer to created audio output device
             * @throws Exception  if device could not be created
             */
            AudioOutputDevice* CreateAudioOutputDevice(String AudioDriver, std::map<String,String> Parameters) throw (Exception);

            /**
             * Create a midi input device.
             *
             * @param MidiDriver - name of the midi driver
             * @param Parameters - eventually needed driver parameters to
             *                     create the device
             * @returns  pointer to created midi input device
             * @throws Exception  if device could not be created
             */
            MidiInputDevice* CreateMidiInputDevice(String MidiDriver, std::map<String,String> Parameters) throw (Exception);

            /**
             * Returns the number of all created audio output devices.
             */
            uint AudioOutputDevices();

            /**
             * Returns the number of all created MIDI input devices.
             */
            uint MidiInputDevices();

            /**
             * Returns all created audio output devices.
             */
            std::map<uint, AudioOutputDevice*> GetAudioOutputDevices();

            /**
             * Returns all created MIDI input devices.
             */
            std::map<uint, MidiInputDevice*> GetMidiInputDevices();

            /**
             * Destroy the given audio output device and takes care if there
             * are still sampler engines connected to this device, etc.
             *
             * @throws Exception  if sampler channels are still
             *                    connected to the device
             */
            void DestroyAudioOutputDevice(AudioOutputDevice* pDevice) throw (Exception);

            /**
             * Destroy all audio output devices and takes care if there
             * are still sampler engines connected to devices, etc.
             *
             * Note: non-autonomous devices, that is devices associated with
             * host plugin instances like VST, AU, DSSI, LV2 are not
             * destroyed by this method.
             *
             * @throws Exception  if sampler channels are still
             *                    connected to device
             */
            void DestroyAllAudioOutputDevices() throw (Exception);

            /**
             * Destroy the given MIDI input device and takes care if there
             * are still sampler engines connected to this device, etc.
             *
             * @throws Exception  if sampler channels are still
             *                    connected to the device
             */
            void DestroyMidiInputDevice(MidiInputDevice* pDevice) throw (Exception);

            /**
             * Destroy all MIDI input devices and take care if there
             * are still sampler engines connected to device, etc.
             *
             * Note: non-autonomous devices, that is devices associated with
             * host plugin instances like VST, AU, DSSI, LV2 are not
             * destroyed by this method.
             *
             * @throws Exception  if sampler channels are still
             *                    connected to device
             */
            void DestroyAllMidiInputDevices() throw (Exception);

             /**
             * Gets the current number of all active streams.
             * @returns The current number of all active streams.
             */
            int GetDiskStreamCount();

            /**
             * Gets the current number of all active voices.
             * @returns The current number of all active voices.
             */
            int GetVoiceCount();

            /**
             * @see SetGlobalMaxVoices()
             */
            int GetGlobalMaxVoices();

            /**
             * @see SetGlobalMaxStreams()
             */
            int GetGlobalMaxStreams();

            /**
             * Sets the global maximum amount limit of voices.
             *
             * Note that this voice limit can also be altered for
             * each sampler engine instance individually instead.
             *
             * @throws Exception  if \a n is invalid
             */
            void SetGlobalMaxVoices(int n) throw (Exception);

            /**
             * Sets the global maximum amount limit of disk streams.
             *
             * Note that this stream limit can also be altered for
             * each sampler engine instance individually instead.
			 *
             * @throws Exception  if \a n is invalid
             */
            void SetGlobalMaxStreams(int n) throw (Exception);

            /**
             * Reset the whole sampler. Destroy all engines, sampler
             * channels, MIDI input devices and audio output devices.
             */
            void Reset();

            ///////////////////////////////////////////////////////////////
            // Event Listener methods

            /**
             * Registers the specified listener to be notified
             * when the number of sampler chanels is changed.
             */
            void AddChannelCountListener(ChannelCountListener* l);

            /**
             * Removes the specified listener.
             */
            void RemoveChannelCountListener(ChannelCountListener* l);

            /**
             * Registers the specified listener to be notified
             * when the number of audio output devices is changed.
             */
            void AddAudioDeviceCountListener(AudioDeviceCountListener* l);

            /**
             * Removes the specified listener.
             */
            void RemoveAudioDeviceCountListener(AudioDeviceCountListener* l);

            /**
             * Registers the specified listener to be notified
             * when the number of MIDI input devices is changed.
             */
            void AddMidiDeviceCountListener(MidiDeviceCountListener* l);

            /**
             * Removes the specified listener.
             */
            void RemoveMidiDeviceCountListener(MidiDeviceCountListener* l);

            /**
             * Registers the specified listener to be notified when the number
             * of active voices in a particular sampler channel is changed.
             */
            void AddVoiceCountListener(VoiceCountListener* l);

            /**
             * Removes the specified listener.
             */
            void RemoveVoiceCountListener(VoiceCountListener* l);

            /**
             * Notifies listeners that the number of active voices
             * on the specified sampler channel is changed.
             * @param ChannelId The numerical ID of the sampler channel.
             * @param NewCount The new number of active voices.
             */
            void fireVoiceCountChanged(int ChannelId, int NewCount);

            /**
             * Registers the specified listener to be notified when the number
             * of active disk streams in a particular sampler channel is changed.
             */
            void AddStreamCountListener(StreamCountListener* l);

            /**
             * Removes the specified listener.
             */
            void RemoveStreamCountListener(StreamCountListener* l);

            /**
             * Notifies listeners that the number of active disk streams
             * on the specified sampler channel is changed.
             * @param ChannelId The numerical ID of the sampler channel.
             * @param NewCount The new number of active disk streams.
             */
            void fireStreamCountChanged(int ChannelId, int NewCount);

            /**
             * Registers the specified listener to be
             * notified when the fill state of the disk stream
             * buffers on a specific sampler channel is changed.
             */
            void AddBufferFillListener(BufferFillListener* l);

            /**
             * Removes the specified listener.
             */
            void RemoveBufferFillListener(BufferFillListener* l);

            /**
             * Notifies listeners that the fill state of the disk stream
             * buffers on the specified sampler channel is changed.
             * @param ChannelId The numerical ID of the sampler channel.
             * @param FillData The buffer fill data for the specified sampler channel.
             */
            void fireBufferFillChanged(int ChannelId, String FillData);

            /**
             * Registers the specified listener to be notified
             * when total number of active voices is changed.
             */
            void AddTotalVoiceCountListener(TotalVoiceCountListener* l);

            /**
             * Removes the specified listener.
             */
            void RemoveTotalVoiceCountListener(TotalVoiceCountListener* l);

            /**
             * Notifies listeners that the total number of active voices is changed.
             * @param NewCount The new number of active voices.
             */
            void fireTotalVoiceCountChanged(int NewCount);

            /**
             * Registers the specified listener to be notified when the number
             * of total streams is changed.
             */
            void AddTotalStreamCountListener(TotalStreamCountListener* l);

            /**
             * Removes the specified listener.
             */
            void RemoveTotalStreamCountListener(TotalStreamCountListener* l);

            /**
             * Notifies listeners that the total number of total streams changed.
             * @param NewCount The new number of total streams.
             */
            void fireTotalStreamCountChanged(int NewCount);

            /**
             * Registers the specified listener to be notified when the number
             * of effect sends on a particular sampler channel is changed.
             */
            void AddFxSendCountListener(FxSendCountListener* l);

            /**
             * Removes the specified listener.
             */
            void RemoveFxSendCountListener(FxSendCountListener* l);

            /**
             * Notifies listeners about the current number of voices,
             * streams and total voices, and the current fill state of
             * the disk stream buffers.
             */
            void fireStatistics();

            ///////////////////////////////////////////////////////////////
            // system specific methods

            /**
             * Advise the FPU to treat denormal floating point numbers as
             * zero, to avoid severe performance penalty when dealing with
             * such extreme floating point values.
             *
             * @returns @c true if FPU supports it, @c false otherwise
             */
            static bool EnableDenormalsAreZeroMode();

#if defined(WIN32)
            /**
             * Gets the directory where the liblinuxsampler dll is located.
             * Note: this method is currently only available for Windows.
             * @returns installation directory
             */
            static String GetInstallDir();
#endif
        protected:
            /**
             * Notifies listeners that the number of sampler channels has been changed.
             * @param NewCount The new number of sampler channels.
             */
            void fireChannelCountChanged(int NewCount);

            /**
             * Notifies listeners that the specified sampler channel has just
             * been added.
             * @param pChannel The new sampler channel.
             */
            void fireChannelAdded(SamplerChannel* pChannel);

            /**
             * Notifies listeners that the specified sampler channel is
             * going to be removed soon.
             * @param pChannel sampler channel to be removed.
             */
            void fireChannelToBeRemoved(SamplerChannel* pChannel);

            /**
             * Notifies listeners that the number of audio output devices has been changed.
             * @param NewCount The new number of audio output devices.
             */
            void fireAudioDeviceCountChanged(int NewCount);

            /**
             * Notifies listeners that the number of MIDI input devices has been changed.
             * @param NewCount The new number of MIDI input devices.
             */
            void fireMidiDeviceCountChanged(int NewCount);

            /**
             * Notifies listeners that the supplied MIDI input device is
             * going to be destroyed soon.
             * @param pDevice MIDI input device to be destroyed
             */
            void fireMidiDeviceToBeDestroyed(MidiInputDevice* pDevice);

            /**
             * Notifies listeners that the supplied MIDI input device was
             * just created.
             * @param pDevice new MIDI input device 
             */
            void fireMidiDeviceCreated(MidiInputDevice* pDevice);

            /**
             * Notifies listeners that the number of effect sends
             * on a particular sampler channel is changed.
             * @param ChannelId The numerical ID of the sampler channel.
             * @param NewCount The new number of sampler channels.
             */
            void fireFxSendCountChanged(int ChannelId, int NewCount);

            typedef std::map<uint, SamplerChannel*> SamplerChannelMap;

            SamplerChannelMap mSamplerChannels; ///< contains all created sampler channels

            // statistics cache
            uint uiOldTotalVoiceCount;
            uint uiOldTotalStreamCount;
            std::map<uint, uint> mOldVoiceCounts;
            std::map<uint, uint> mOldStreamCounts;

            friend class SamplerChannel;

        private:
            ListenerList<ChannelCountListener*> llChannelCountListeners;
            ListenerList<AudioDeviceCountListener*> llAudioDeviceCountListeners;
            ListenerList<MidiDeviceCountListener*> llMidiDeviceCountListeners;
            ListenerList<VoiceCountListener*> llVoiceCountListeners;
            ListenerList<StreamCountListener*> llStreamCountListeners;
            ListenerList<BufferFillListener*> llBufferFillListeners;
            ListenerList<TotalStreamCountListener*> llTotalStreamCountListeners;
            ListenerList<TotalVoiceCountListener*> llTotalVoiceCountListeners;
            ListenerList<FxSendCountListener*> llFxSendCountListeners;

            class EventHandler : public EngineChangeListener, public FxSendCountListener {
                public:
                    void SetSampler(Sampler* pSampler) { this->pSampler = pSampler; }

                    /**
                     * Invoked when the engine type of the specified sampler
                     * channel is going to be changed soon.
                     * @param ChannelId The numerical ID of the sampler channel
                     */
                    virtual void EngineToBeChanged(int ChannelId);

                    /**
                     * Invoked when the engine type of the
                     * specified sampler channel is changed.
                     * @param ChannelId The numerical ID of the sampler
                     * channel, which engine type has been changed.
                     */
                    virtual void EngineChanged(int ChannelId);

                    /**
                     * Invoked when the number of effect sends
                     * on the specified sampler channel has changed.
                     * @param ChannelId The numerical ID of the sampler channel.
                     * @param NewCount The new number of effect sends.
                     */
                    virtual void FxSendCountChanged(int ChannelId, int NewCount);

                private:
                    Sampler* pSampler;
            } eventHandler;
    };
}

#endif // __LS_SAMPLER_H__

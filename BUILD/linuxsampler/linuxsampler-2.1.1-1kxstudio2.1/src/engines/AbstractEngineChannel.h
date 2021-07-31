/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2008 Christian Schoenebeck                         *
 *   Copyright (C) 2009-2012 Christian Schoenebeck and Grigor Iliev        *
 *   Copyright (C) 2012-2017 Christian Schoenebeck and Andreas Persson     *
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

#ifndef __LS_ABSTRACTENGINECHANNEL_H__
#define __LS_ABSTRACTENGINECHANNEL_H__

#include "EngineChannel.h"
#include "AbstractEngine.h"

#include "../common/Pool.h"
#include "../common/RingBuffer.h"
#include "../common/ResourceManager.h"
#include "common/AbstractInstrumentManager.h"
#include "common/InstrumentScriptVM.h"

#define CTRL_TABLE_IDX_AFTERTOUCH   128
#define CTRL_TABLE_IDX_PITCHBEND    129
#define CTRL_TABLE_SIZE             130

namespace LinuxSampler {

    class MidiKeyboardManagerBase;

    class AbstractEngineChannel: public EngineChannel, public InstrumentScriptConsumer {
        public:
            // implementation of abstract methods derived from interface class 'LinuxSampler::EngineChannel'
            virtual void    PrepareLoadInstrument(const char* FileName, uint Instrument) OVERRIDE;
            virtual void    Reset() OVERRIDE;
            virtual void    SendNoteOn(uint8_t Key, uint8_t Velocity, uint8_t MidiChannel) OVERRIDE;
            virtual void    SendNoteOn(uint8_t Key, uint8_t Velocity, uint8_t MidiChannel, int32_t FragmentPos) OVERRIDE;
            virtual void    SendNoteOff(uint8_t Key, uint8_t Velocity, uint8_t MidiChannel) OVERRIDE;
            virtual void    SendNoteOff(uint8_t Key, uint8_t Velocity, uint8_t MidiChannel, int32_t FragmentPos) OVERRIDE;
            virtual void    SendPitchbend(int Pitch, uint8_t MidiChannel) OVERRIDE;
            virtual void    SendPitchbend(int Pitch, uint8_t MidiChannel, int32_t FragmentPos) OVERRIDE;
            virtual void    SendControlChange(uint8_t Controller, uint8_t Value, uint8_t MidiChannel) OVERRIDE;
            virtual void    SendControlChange(uint8_t Controller, uint8_t Value, uint8_t MidiChannel, int32_t FragmentPos) OVERRIDE;            
            virtual void    SendChannelPressure(uint8_t Value, uint8_t MidiChannel) OVERRIDE;
            virtual void    SendChannelPressure(uint8_t Value, uint8_t MidiChannel, int32_t FragmentPos) OVERRIDE;
            virtual void    SendPolyphonicKeyPressure(uint8_t Key, uint8_t Value, uint8_t MidiChannel) OVERRIDE;
            virtual void    SendPolyphonicKeyPressure(uint8_t Key, uint8_t Value, uint8_t MidiChannel, int32_t FragmentPos) OVERRIDE;
            virtual bool    StatusChanged(bool bNewStatus = false) OVERRIDE;
            virtual float   Volume() OVERRIDE;
            virtual void    Volume(float f) OVERRIDE;
            virtual float   Pan() OVERRIDE;
            virtual void    Pan(float f) OVERRIDE;
            virtual uint    Channels() OVERRIDE;
            virtual AudioOutputDevice* GetAudioOutputDevice() OVERRIDE;
            virtual void    SetOutputChannel(uint EngineAudioChannel, uint AudioDeviceChannel) OVERRIDE;
            virtual int     OutputChannel(uint EngineAudioChannel) OVERRIDE;
            virtual void    Connect(MidiInputPort* pMidiPort) OVERRIDE;
            virtual void    Disconnect(MidiInputPort* pMidiPort) OVERRIDE;
            virtual void    DisconnectAllMidiInputPorts() OVERRIDE;
            virtual uint    GetMidiInputPortCount() OVERRIDE;
            virtual MidiInputPort* GetMidiInputPort(uint index) OVERRIDE;
            virtual midi_chan_t MidiChannel() OVERRIDE;
            virtual void    SetMidiChannel(midi_chan_t MidiChannel) OVERRIDE;
            virtual void    Connect(MidiInputPort* pMidiPort, midi_chan_t MidiChannel) OVERRIDE; // deprecated, may be removed
            virtual void    DisconnectMidiInputPort() OVERRIDE; // deprecated, may be removed
            virtual MidiInputPort* GetMidiInputPort() OVERRIDE; // deprecated, may be removed
            virtual String  InstrumentFileName() OVERRIDE;
            virtual String  InstrumentName() OVERRIDE;
            virtual int     InstrumentIndex() OVERRIDE;
            virtual int     InstrumentStatus() OVERRIDE;
            virtual Engine* GetEngine() OVERRIDE;
            virtual String  EngineName() OVERRIDE;
            virtual FxSend* AddFxSend(uint8_t MidiCtrl, String Name = "") throw (Exception) OVERRIDE;
            virtual FxSend* GetFxSend(uint FxSendIndex) OVERRIDE;
            virtual uint    GetFxSendCount() OVERRIDE;
            virtual void    RemoveFxSend(FxSend* pFxSend) OVERRIDE;
            virtual void    Connect(VirtualMidiDevice* pDevice) OVERRIDE;
            virtual void    Disconnect(VirtualMidiDevice* pDevice) OVERRIDE;

            // implementation of abstract methods derived from AbstractEngine::ScriptConsumer
            virtual void ResourceToBeUpdated(VMParserContext* pResource, void*& pUpdateArg) OVERRIDE {}
            virtual void ResourceUpdated(VMParserContext* pOldResource, VMParserContext* pNewResource, void* pUpdateArg) OVERRIDE {}
            virtual void OnResourceProgress(float fProgress) OVERRIDE {}

            virtual AbstractEngine::Format GetEngineFormat() = 0;
            virtual MidiKeyboardManagerBase* GetMidiKeyboardManager() = 0;

            AudioOutputDevice* GetAudioOutputDeviceSafe();

            script_callback_id_t GetScriptCallbackID(const ScriptEvent* e) const {
                return pScript->pEvents->getID(e);
            }

            RTList<ScriptEvent>::Iterator ScriptCallbackByID(script_callback_id_t id) const {
                return pScript->pEvents->fromID(id);
            }

            void ScheduleResumeOfScriptCallback(RTList<ScriptEvent>::Iterator& itCallback, sched_time_t now, bool forever);

            bool hasFreeScriptCallbacks(int n) const {
                return pScript->pEvents->poolHasFreeElements(n);
            }

            RTList<ScriptEvent>::Iterator forkScriptCallback(ScriptEvent* parent, bool bAutoAbort);

            friend class AbstractVoice;
            friend class AbstractEngine;
            template<class TV, class TRR, class TR, class TD, class TIM, class TI> friend class EngineBase;
            template<class EC, class R, class S, class D> friend class VoiceBase;

        //protected:
            AbstractEngineChannel();
            virtual ~AbstractEngineChannel();

            AbstractEngine*           pEngine;
            Mutex                     EngineMutex; ///< protects the Engine from access by the instrument loader thread when lscp is disconnecting
            Mutex                     MidiInputMutex; ///< Introduced when support for multiple MIDI inputs per engine channel was added: protects the MIDI event input ringbuffer on this engine channel to be accessed concurrently by multiple midi input threads. As alternative one might also move the ringbuffer from this engine channel to the individual MIDI ports/devices and let the sampler engine read the events from there instead of receiving them here.

        //protected:
            AudioChannel*             pChannelLeft;             ///< encapsulates the audio rendering buffer (left)
            AudioChannel*             pChannelRight;            ///< encapsulates the audio rendering buffer (right)
            int                       AudioDeviceChannelLeft;   ///< audio device channel number to which the left channel is connected to
            int                       AudioDeviceChannelRight;  ///< audio device channel number to which the right channel is connected to
            DoubleBuffer< ArrayList<MidiInputPort*> > midiInputs; ///< MIDI input ports on which this sampler engine channel shall listen to.
            midi_chan_t               midiChannel;              ///< MIDI channel(s) on which this engine channel listens to (on all MIDI input ports).
            RingBuffer<Event,false>*  pEventQueue;              ///< Input event queue.
            RTList<Event>*            pEvents;                  ///< All engine channel specific events for the current audio fragment.
            struct _DelayedEvents {
                RTList<Event>*            pList; ///< Unsorted list where all delayed events are moved to and remain here until they're finally processed.
                Pool<ScheduledEvent>      schedulerNodes; ///< Nodes used to sort the delayed events (stored on pList) with time sorted queue.
                RTAVLTree<ScheduledEvent> queue; ///< Used to access the delayed events (from pList) in time sorted manner.

                _DelayedEvents() : pList(NULL), schedulerNodes(CONFIG_MAX_EVENTS_PER_FRAGMENT) {}

                inline void clear() {
                    if (pList) pList->clear();
                    schedulerNodes.clear();
                    queue.clear();
                }
            } delayedEvents;
            uint8_t                   ControllerTable[CTRL_TABLE_SIZE];     ///< Reflects the current values (0-127) of all MIDI controllers for this engine / sampler channel. Number 128 is for channel pressure (mono aftertouch), 129 for pitch bend.
            String                    InstrumentFile;
            int                       InstrumentIdx;
            String                    InstrumentIdxName;
            int                       InstrumentStat;
            double                    GlobalVolume;             ///< Master volume factor set through the C++ API / LSCP (a value < 1.0 means attenuation, a value > 1.0 means amplification)
            double                    MidiVolume;               ///< Volume factor altered by MIDI CC#7 (a value < 1.0 means attenuation, a value > 1.0 means amplification)
            int                       Pitch;                    ///< Current (absolute) MIDI pitch value.
            float                     CurrentKeyDimension;      ///< Current value (0-1.0) for the keyboard dimension, altered by pressing a keyswitching key.
            bool                      PortamentoMode;           ///< in Portamento Mode we slide the pitch from the last note to the current note.
            float                     PortamentoTime;           ///< How long it will take to glide from the previous note to the current (in seconds)
            float                     PortamentoPos;            ///< Current position on the keyboard, that is integer and fractional part (only used if PortamentoMode is on)
            std::vector<FxSend*>      fxSends;
            int                       GlobalTranspose;          ///< amount of semi tones all notes should be transposed
            int                       iLastPanRequest;          ///< just for the return value of Pan(), so we don't have to make an injective function
            int                       iEngineIndexSelf;         ///< Reflects the index of this EngineChannel in the Engine's ArrayList.
            bool                      bStatusChanged;           ///< true in case an engine parameter has changed (e.g. new instrument, another volumet)
            uint32_t                  RoundRobinIndex;          ///< counter for round robin sample selection, incremented for each note on
            InstrumentScript*         pScript;                  ///< Points to the real-time instrument script(s) to be executed, NULL if current instrument does not have an instrument script. Even though the underlying VM representation of the script is shared among multiple sampler channels, the InstrumentScript object here is not shared though, it exists for each sampler channel separately.

            SynchronizedConfig< ArrayList<VirtualMidiDevice*> > virtualMidiDevices;
            SynchronizedConfig< ArrayList<VirtualMidiDevice*> >::Reader virtualMidiDevicesReader_AudioThread;
            SynchronizedConfig< ArrayList<VirtualMidiDevice*> >::Reader virtualMidiDevicesReader_MidiThread;

            // specialization of RTList that doesn't require the pool
            // to be provided at construction time
            template<typename T>
            class LazyList : public RTList<T> {
            public:
                using RTList<T>::allocAppend;
                using RTList<T>::pPool;

                LazyList() : RTList<T>(0) { }
                typename RTList<T>::Iterator allocAppend(Pool<T>* pool) {
                    pPool = pool;
                    return allocAppend();
                }
            };
            
            typedef std::map<uint, LazyList<Event>*> ActiveKeyGroupMap;
            ActiveKeyGroupMap ActiveKeyGroups;      ///< Contains event queues for key groups, ordered by key group ID.

            virtual void ResetControllers();
            virtual void ResetInternal(bool bResetEngine);
            virtual void RemoveAllFxSends();

            void ImportEvents(uint Samples);
            virtual note_id_t ScheduleNoteMicroSec(const Event* pEvent, int delay) = 0;
            event_id_t ScheduleEventMicroSec(const Event* pEvent, int delay);
            void IgnoreEvent(event_id_t id);
            virtual void IgnoreNote(note_id_t id) = 0;
            void IgnoreEventByScriptID(const ScriptID& id);
            virtual uint AllNoteIDs(note_id_t* dstBuf, uint bufSize) = 0;

            void AddGroup(uint group);
            void HandleKeyGroupConflicts(uint KeyGroup, Pool<Event>::Iterator& itNoteOnEvent);
            void ClearGroupEventLists();
            void DeleteGroupEventLists();

        private:
            /**
             * Returns @c true if there are 2 ore more MidiInputPorts connected
             * to this engine channel.
             *
             * This method is currently only used to prevent unnecessary
             * MidiInputMutex.Lock() if there is not more than 1 MIDI input on
             * this engine channel.
             */
            inline bool hasMultipleMIDIInputs() const {
                //FIXME: leaves tiny time frames open (shortly after 1->2 devices connected or 2->1 disconnected) which could lead to concurrency issue for the purpose described above, however in practice it "should" be acceptable
                return midiInputs.unsafeBack().size() > 1;
            }

            inline bool applyTranspose(Event* event);
    };

} // namespace LinuxSampler

#endif  /* __LS_ABSTRACTENGINECHANNEL_H__ */


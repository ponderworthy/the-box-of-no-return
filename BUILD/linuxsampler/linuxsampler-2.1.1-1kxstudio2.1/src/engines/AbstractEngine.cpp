/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2008 Christian Schoenebeck                         *
 *   Copyright (C) 2009-2012 Christian Schoenebeck and Grigor Iliev        *
 *   Copyright (C) 2013-2016 Christian Schoenebeck and Andreas Persson     *
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

#include "AbstractEngine.h"
#include "AbstractEngineChannel.h"
#include "EngineFactory.h"
#include "../common/global_private.h"
#include "../effects/EffectFactory.h"

namespace LinuxSampler {

    //InstrumentResourceManager Engine::instruments;

    std::map<AbstractEngine::Format, std::map<AudioOutputDevice*,AbstractEngine*> > AbstractEngine::engines;

    /**
     * Get an AbstractEngine object for the given AbstractEngineChannel and the
     * given AudioOutputDevice. All engine channels which are connected to
     * the same audio output device will use the same engine instance. This
     * method will be called by an EngineChannel whenever it's
     * connecting to an audio output device.
     *
     * @param pChannel - engine channel which acquires an engine object
     * @param pDevice  - the audio output device \a pChannel is connected to
     */
    AbstractEngine* AbstractEngine::AcquireEngine(AbstractEngineChannel* pChannel, AudioOutputDevice* pDevice) {
        AbstractEngine* pEngine = NULL;
        // check if there's already an engine for the given audio output device
        std::map<AbstractEngine::Format, std::map<AudioOutputDevice*,AbstractEngine*> >::iterator it;
        it = engines.find(pChannel->GetEngineFormat());
        if (it != engines.end() && (*it).second.count(pDevice)) {
            dmsg(4,("Using existing Engine.\n"));
            pEngine = (*it).second[pDevice];

            // Disable the engine while the new engine channel is
            // added and initialized. The engine will be enabled again
            // in EngineChannel::Connect.
            pEngine->DisableAndLock();
        } else { // create a new engine (and disk thread) instance for the given audio output device
            dmsg(4,("Creating new Engine.\n"));
            pEngine = (AbstractEngine*) EngineFactory::Create(pChannel->EngineName());
            pEngine->CreateInstrumentScriptVM();
            pEngine->Connect(pDevice);
            engines[pChannel->GetEngineFormat()][pDevice] = pEngine;
        }
        // register engine channel to the engine instance
        pEngine->engineChannels.add(pChannel);
        // remember index in the ArrayList
        pChannel->iEngineIndexSelf = pEngine->engineChannels.size() - 1;
        dmsg(4,("This Engine has now %d EngineChannels.\n",pEngine->engineChannels.size()));
        return pEngine;
    }

    AbstractEngine::AbstractEngine() {
        pAudioOutputDevice = NULL;
        pEventGenerator    = new EventGenerator(44100);
        pSysexBuffer       = new RingBuffer<uint8_t,false>(CONFIG_SYSEX_BUFFER_SIZE, 0);
        pEventQueue        = new RingBuffer<Event,false>(CONFIG_MAX_EVENTS_PER_FRAGMENT, 0);
        pEventPool         = new Pool<Event>(CONFIG_MAX_EVENTS_PER_FRAGMENT);
        pEventPool->setPoolElementIDsReservedBits(INSTR_SCRIPT_EVENT_ID_RESERVED_BITS);
        pGlobalEvents      = new RTList<Event>(pEventPool);
        FrameTime          = 0;
        RandomSeed         = 0;
        pDedicatedVoiceChannelLeft = pDedicatedVoiceChannelRight = NULL;
        pScriptVM          = NULL;
    }

    AbstractEngine::~AbstractEngine() {
        if (pEventQueue) delete pEventQueue;
        if (pEventPool)  delete pEventPool;
        if (pEventGenerator) delete pEventGenerator;
        if (pGlobalEvents) delete pGlobalEvents;
        if (pSysexBuffer) delete pSysexBuffer;
        if (pDedicatedVoiceChannelLeft) delete pDedicatedVoiceChannelLeft;
        if (pDedicatedVoiceChannelRight) delete pDedicatedVoiceChannelRight;
        if (pScriptVM) delete pScriptVM;
        Unregister();
    }

    void AbstractEngine::CreateInstrumentScriptVM() {
        dmsg(2,("Created sampler format independent instrument script VM.\n"));
        if (pScriptVM) return;
        pScriptVM = new InstrumentScriptVM; // format independent script runner
    }

    /**
     * Once an engine channel is disconnected from an audio output device,
     * it will immediately call this method to unregister itself from the
     * engine instance and if that engine instance is not used by any other
     * engine channel anymore, then that engine instance will be destroyed.
     *
     * @param pChannel - engine channel which wants to disconnect from it's
     *                   engine instance
     * @param pDevice  - audio output device \a pChannel was connected to
     */
    void AbstractEngine::FreeEngine(AbstractEngineChannel* pChannel, AudioOutputDevice* pDevice) {
        dmsg(4,("Disconnecting EngineChannel from Engine.\n"));
        AbstractEngine* pEngine = engines[pChannel->GetEngineFormat()][pDevice];
        // unregister EngineChannel from the Engine instance
        pEngine->engineChannels.remove(pChannel);
        // if the used Engine instance is not used anymore, then destroy it
        if (pEngine->engineChannels.empty()) {
            pDevice->Disconnect(pEngine);
            engines[pChannel->GetEngineFormat()].erase(pDevice);
            delete pEngine;
            dmsg(4,("Destroying Engine.\n"));
        }
        else dmsg(4,("This Engine has now %d EngineChannels.\n",pEngine->engineChannels.size()));
    }

    void AbstractEngine::Enable() {
        dmsg(3,("AbstractEngine: enabling\n"));
        EngineDisabled.PushAndUnlock(false, 2, 0, true); // set condition object 'EngineDisabled' to false (wait max. 2s)
        dmsg(3,("AbstractEngine: enabled (val=%d)\n", EngineDisabled.GetUnsafe()));
    }

    /**
     * Temporarily stop the engine to not do anything. The engine will just be
     * frozen during that time, that means after enabling it again it will
     * continue where it was, with all its voices and playback state it had at
     * the point of disabling. Notice that the engine's (audio) thread will
     * continue to run, it just remains in an inactive loop during that time.
     *
     * If you need to be sure that all voices and disk streams are killed as
     * well, use @c SuspendAll() instead.
     *
     * @see Enable(), SuspendAll()
     */
    void AbstractEngine::Disable() {
        dmsg(3,("AbstractEngine: disabling\n"));
        bool* pWasDisabled = EngineDisabled.PushAndUnlock(true, 2); // wait max. 2s
        if (!pWasDisabled) dmsg(3,("AbstractEngine warning: Timeout waiting to disable engine.\n"));
    }

    void AbstractEngine::DisableAndLock() {
        dmsg(3,("AbstractEngine: disabling\n"));
        bool* pWasDisabled = EngineDisabled.Push(true, 2); // wait max. 2s
        if (!pWasDisabled) dmsg(3,("AbstractEngine warning: Timeout waiting to disable engine.\n"));
    }

    /**
     *  Reset all voices and disk thread and clear input event queue and all
     *  control and status variables.
     */
    void AbstractEngine::Reset() {
        DisableAndLock();
        ResetInternal();
        ResetScaleTuning();
        Enable();
    }

    /**
     * Reset to normal, chromatic scale (means equal tempered).
     */
    void AbstractEngine::ResetScaleTuning() {
        memset(&ScaleTuning[0], 0x00, 12);
        ScaleTuningChanged.raise();
    }

    /**
     * Copy all events from the engine's global input queue buffer to the
     * engine's internal event list. This will be done at the beginning of
     * each audio cycle (that is each RenderAudio() call) to distinguish
     * all global events which have to be processed in the current audio
     * cycle. These events are usually just SysEx messages. Every
     * EngineChannel has it's own input event queue buffer and event list
     * to handle common events like NoteOn, NoteOff and ControlChange
     * events.
     *
     * @param Samples - number of sample points to be processed in the
     *                  current audio cycle
     */
    void AbstractEngine::ImportEvents(uint Samples) {
        RingBuffer<Event,false>::NonVolatileReader eventQueueReader = pEventQueue->get_non_volatile_reader();
        Event* pEvent;
        while (true) {
            // get next event from input event queue
            if (!(pEvent = eventQueueReader.pop())) break;
            // if younger event reached, ignore that and all subsequent ones for now
            if (pEvent->FragmentPos() >= Samples) {
                eventQueueReader--;
                dmsg(2,("Younger Event, pos=%d ,Samples=%d!\n",pEvent->FragmentPos(),Samples));
                pEvent->ResetFragmentPos();
                break;
            }
            // copy event to internal event list
            if (pGlobalEvents->poolIsEmpty()) {
                dmsg(1,("Event pool emtpy!\n"));
                break;
            }
            *pGlobalEvents->allocAppend() = *pEvent;
        }
        eventQueueReader.free(); // free all copied events from input queue
    }

    /**
     * Clear all engine global event lists.
     */
    void AbstractEngine::ClearEventLists() {
        pGlobalEvents->clear();
    }

    /**
     * Will be called in case the respective engine channel sports FX send
     * channels. In this particular case, engine channel local buffers are
     * used to render and mix all voices to. This method is responsible for
     * copying the audio data from those local buffers to the master audio
     * output channels as well as to the FX send audio output channels with
     * their respective FX send levels.
     *
     * @param pEngineChannel - engine channel from which audio should be
     *                         routed
     * @param Samples        - amount of sample points to be routed in
     *                         this audio fragment cycle
     */
    void AbstractEngine::RouteAudio(EngineChannel* pEngineChannel, uint Samples) {
        AbstractEngineChannel* pChannel = static_cast<AbstractEngineChannel*>(pEngineChannel);
        AudioChannel* ppSource[2] = {
            pChannel->pChannelLeft,
            pChannel->pChannelRight
        };
        // route dry signal
        {
            AudioChannel* pDstL = pAudioOutputDevice->Channel(pChannel->AudioDeviceChannelLeft);
            AudioChannel* pDstR = pAudioOutputDevice->Channel(pChannel->AudioDeviceChannelRight);
            ppSource[0]->MixTo(pDstL, Samples);
            ppSource[1]->MixTo(pDstR, Samples);
        }
        // route FX send signal (wet)
        {
            for (int iFxSend = 0; iFxSend < pChannel->GetFxSendCount(); iFxSend++) {
                FxSend* pFxSend = pChannel->GetFxSend(iFxSend);
                const bool success = RouteFxSend(pFxSend, ppSource, pFxSend->Level(), Samples);
                if (!success) goto channel_cleanup;
            }
        }
        channel_cleanup:
        // reset buffers with silence (zero out) for the next audio cycle
        ppSource[0]->Clear();
        ppSource[1]->Clear();
    }
    
    /**
     * Similar to RouteAudio(), but this method is even more special. It is
     * only called by voices which have dedicated effect send(s) level(s). So
     * such voices have to be routed separately apart from the other voices
     * which can just be mixed together and routed afterwards in one turn.
     */
    void AbstractEngine::RouteDedicatedVoiceChannels(EngineChannel* pEngineChannel, optional<float> FxSendLevels[2], uint Samples) {
        AbstractEngineChannel* pChannel = static_cast<AbstractEngineChannel*>(pEngineChannel);
        AudioChannel* ppSource[2] = {
            pDedicatedVoiceChannelLeft,
            pDedicatedVoiceChannelRight
        };
        // route dry signal
        {
            AudioChannel* pDstL = pAudioOutputDevice->Channel(pChannel->AudioDeviceChannelLeft);
            AudioChannel* pDstR = pAudioOutputDevice->Channel(pChannel->AudioDeviceChannelRight);
            ppSource[0]->MixTo(pDstL, Samples);
            ppSource[1]->MixTo(pDstR, Samples);
        }
        // route FX send signals (wet)
        // (we simply hard code the voices 'reverb send' to the 1st effect
        // send bus, and the voioces 'chorus send' to the 2nd effect send bus)
        {
            for (int iFxSend = 0; iFxSend < 2 && iFxSend < pChannel->GetFxSendCount(); iFxSend++) {
                // no voice specific FX send level defined for this effect? 
                if (!FxSendLevels[iFxSend]) continue; // ignore this effect then
                
                FxSend* pFxSend = pChannel->GetFxSend(iFxSend);
                const bool success = RouteFxSend(pFxSend, ppSource, *FxSendLevels[iFxSend], Samples);
                if (!success) goto channel_cleanup;
            }
        }
        channel_cleanup:
        // reset buffers with silence (zero out) for the next dedicated voice rendering/routing process
        ppSource[0]->Clear();
        ppSource[1]->Clear();
    }
    
    /**
     * Route the audio signal given by @a ppSource to the effect send bus
     * defined by @a pFxSend (wet signal only).
     *
     * @param pFxSend - definition of effect send bus
     * @param ppSource - the 2 channels of the audio signal to be routed
     * @param FxSendLevel - the effect send level to by applied
     * @param Samples - amount of sample points to be processed
     * @returns true if signal was routed successfully, false on error
     */
    bool AbstractEngine::RouteFxSend(FxSend* pFxSend, AudioChannel* ppSource[2], float FxSendLevel, uint Samples) {
        for (int iChan = 0; iChan < 2; ++iChan) {
            const int iDstChan = pFxSend->DestinationChannel(iChan);
            if (iDstChan < 0) {
                dmsg(1,("Engine::RouteAudio() Error: invalid FX send (%s) destination channel (%d->%d)\n",
                        ((iChan) ? "R" : "L"), iChan, iDstChan));
                return false; // error
            }
            AudioChannel* pDstChan = NULL;
            Effect* pEffect = NULL;
            if (pFxSend->DestinationEffectChain() >= 0) { // fx send routed to an internal send effect
                EffectChain* pEffectChain =
                    pAudioOutputDevice->SendEffectChainByID(
                        pFxSend->DestinationEffectChain()
                    );
                if (!pEffectChain) {
                    dmsg(1,("Engine::RouteAudio() Error: invalid FX send (%s) destination effect chain %d\n",
                            ((iChan) ? "R" : "L"), pFxSend->DestinationEffectChain()));
                    return false; // error
                }
                pEffect =
                    pEffectChain->GetEffect(
                        pFxSend->DestinationEffectChainPosition()
                    );
                if (!pEffect) {
                    dmsg(1,("Engine::RouteAudio() Error: invalid FX send (%s) destination effect %d of effect chain %d\n",
                            ((iChan) ? "R" : "L"), pFxSend->DestinationEffectChainPosition(), pFxSend->DestinationEffectChain()));
                    return false; // error
                }
                pDstChan = pEffect->InputChannel(iDstChan);
            } else { // FX send routed directly to audio output device channel(s)
                pDstChan = pAudioOutputDevice->Channel(iDstChan);
            }
            if (!pDstChan) {
                if (pFxSend->DestinationEffectChain() >= 0) { // fx send routed to an internal send effect
                    const int iEffectChannels = pEffect ? pEffect->InputChannelCount() : 0;
                    dmsg(1,("Engine::RouteAudio() Error: invalid FX send (%s) destination channel (%d->%d): "
                            "FX send is routed to effect %d of effect chain %d and that effect has %d input channels\n",
                            ((iChan) ? "R" : "L"), iChan, iDstChan,
                            pFxSend->DestinationEffectChainPosition(),
                            pFxSend->DestinationEffectChain(), iEffectChannels));
                } else { // FX send routed directly to audio output device channel(s)
                    dmsg(1,("Engine::RouteAudio() Error: invalid FX send (%s) destination channel (%d->%d): "
                            "FX send is directly routed to audio output device which has %d output channels\n",
                            ((iChan) ? "R" : "L"), iChan, iDstChan,
                            (pAudioOutputDevice ? pAudioOutputDevice->ChannelCount() : 0)));
                }
                return false; // error
            }
            ppSource[iChan]->MixTo(pDstChan, Samples, FxSendLevel);
        }
        return true; // success
    }

    /**
     * Calculates the Roland GS sysex check sum.
     *
     * @param AddrReader - reader which currently points to the first GS
     *                     command address byte of the GS sysex message in
     *                     question
     * @param DataSize   - size of the GS message data (in bytes)
     */
    uint8_t AbstractEngine::GSCheckSum(const RingBuffer<uint8_t,false>::NonVolatileReader AddrReader, uint DataSize) {
        RingBuffer<uint8_t,false>::NonVolatileReader reader = AddrReader;
        uint bytes = 3 /*addr*/ + DataSize;        
        uint8_t sum = 0;
        uint8_t c;
        for (uint i = 0; i < bytes; ++i) {
            if (!reader.pop(&c)) break;
            sum += c;
        }
        return 128 - sum % 128;
    }

    /**
     * Allows to tune each of the twelve semitones of an octave.
     *
     * @param ScaleTunes - detuning of all twelve semitones (in cents)
     */
    void AbstractEngine::AdjustScaleTuning(const int8_t ScaleTunes[12]) {
        memcpy(&this->ScaleTuning[0], &ScaleTunes[0], 12);
        ScaleTuningChanged.raise();
    }
    
    void AbstractEngine::GetScaleTuning(int8_t* pScaleTunes) {
        memcpy(pScaleTunes, &this->ScaleTuning[0], 12);
    }

    uint AbstractEngine::VoiceCount() {
        return atomic_read(&ActiveVoiceCount);
    }

    void AbstractEngine::SetVoiceCount(uint Count) {
        atomic_set(&ActiveVoiceCount, Count);
    }

    uint AbstractEngine::VoiceCountMax() {
        return ActiveVoiceCountMax;
    }

    /**
     *  Stores the latest pitchbend event as current pitchbend scalar value.
     *
     *  @param pEngineChannel - engine channel on which this event occured on
     *  @param itPitchbendEvent - absolute pitch value and time stamp of the event
     */
    void AbstractEngine::ProcessPitchbend(AbstractEngineChannel* pEngineChannel, Pool<Event>::Iterator& itPitchbendEvent) {
        pEngineChannel->Pitch = itPitchbendEvent->Param.Pitch.Pitch; // store current pitch value
    }

    void AbstractEngine::ProcessFxSendControllers (
        AbstractEngineChannel*  pEngineChannel,
        Pool<Event>::Iterator&  itControlChangeEvent
    ) {
        if (!pEngineChannel->fxSends.empty()) {
            for (int iFxSend = 0; iFxSend < pEngineChannel->GetFxSendCount(); iFxSend++) {
                FxSend* pFxSend = pEngineChannel->GetFxSend(iFxSend);
                if (pFxSend->MidiController() == itControlChangeEvent->Param.CC.Controller) {
                    pFxSend->SetLevel(itControlChangeEvent->Param.CC.Value);
                    pFxSend->SetInfoChanged(true);
                }
            }
        }
    }

    /**
     *  Will be called by the MIDI input device whenever a MIDI system
     *  exclusive message has arrived.
     *
     *  @param pData - pointer to sysex data
     *  @param Size  - lenght of sysex data (in bytes)
     *  @param pSender - the MIDI input port on which the SysEx message was
     *                   received
     */
    void AbstractEngine::SendSysex(void* pData, uint Size, MidiInputPort* pSender) {
        Event event             = pEventGenerator->CreateEvent();
        event.Type              = Event::type_sysex;
        event.Param.Sysex.Size  = Size;
        event.pEngineChannel    = NULL; // as Engine global event
        event.pMidiInputPort    = pSender;
        if (pEventQueue->write_space() > 0) {
            if (pSysexBuffer->write_space() >= Size) {
                // copy sysex data to input buffer
                uint toWrite = Size;
                uint8_t* pPos = (uint8_t*) pData;
                while (toWrite) {
                    const uint writeNow = RTMath::Min(toWrite, pSysexBuffer->write_space_to_end());
                    pSysexBuffer->write(pPos, writeNow);
                    toWrite -= writeNow;
                    pPos    += writeNow;

                }
                // finally place sysex event into input event queue
                pEventQueue->push(&event);
            }
            else dmsg(1,("Engine: Sysex message too large (%d byte) for input buffer (%d byte)!",Size,CONFIG_SYSEX_BUFFER_SIZE));
        }
        else dmsg(1,("Engine: Input event queue full!"));
    }

    /**
     *  Reacts on MIDI system exclusive messages.
     *
     *  @param itSysexEvent - sysex data size and time stamp of the sysex event
     */
    void AbstractEngine::ProcessSysex(Pool<Event>::Iterator& itSysexEvent) {
        RingBuffer<uint8_t,false>::NonVolatileReader reader = pSysexBuffer->get_non_volatile_reader();

        uint8_t exclusive_status, id;
        if (!reader.pop(&exclusive_status)) goto free_sysex_data;
        if (!reader.pop(&id))               goto free_sysex_data;
        if (exclusive_status != 0xF0)       goto free_sysex_data;

        switch (id) {
            case 0x7f: { // (Realtime) Universal Sysex (GM Standard)
                uint8_t sysex_channel, sub_id1, sub_id2, val_msb, val_lsb;;
                if (!reader.pop(&sysex_channel)) goto free_sysex_data;
                if (!reader.pop(&sub_id1)) goto free_sysex_data;
                if (!reader.pop(&sub_id2)) goto free_sysex_data;
                if (!reader.pop(&val_lsb)) goto free_sysex_data;
                if (!reader.pop(&val_msb)) goto free_sysex_data;
                //TODO: for now we simply ignore the sysex channel, seldom used anyway
                switch (sub_id1) {
                    case 0x04: // Device Control
                        switch (sub_id2) {
                            case 0x01: { // Master Volume
                                const double volume =
                                    double((uint(val_msb)<<7) | uint(val_lsb)) / 16383.0;
                                #if CONFIG_MASTER_VOLUME_SYSEX_BY_PORT
                                // apply volume to all sampler channels that
                                // are connected to the same MIDI input port
                                // this sysex message arrived on
                                for (int i = 0; i < engineChannels.size(); ++i) {
                                    EngineChannel* pEngineChannel = engineChannels[i];
                                    if (pEngineChannel->GetMidiInputPort() ==
                                        itSysexEvent->pMidiInputPort)
                                    {
                                        pEngineChannel->Volume(volume);
                                    }
                                }
                                #else
                                // apply volume globally to the whole sampler
                                GLOBAL_VOLUME = volume;
                                #endif // CONFIG_MASTER_VOLUME_SYSEX_BY_PORT
                                break;
                            }
                        }
                        break;
                }
                break;
            }
            case 0x41: { // Roland
                dmsg(3,("Roland Sysex\n"));
                uint8_t device_id, model_id, cmd_id;
                if (!reader.pop(&device_id)) goto free_sysex_data;
                if (!reader.pop(&model_id))  goto free_sysex_data;
                if (!reader.pop(&cmd_id))    goto free_sysex_data;
                if (model_id != 0x42 /*GS*/) goto free_sysex_data;
                if (cmd_id != 0x12 /*DT1*/)  goto free_sysex_data;

                // command address
                uint8_t addr[3]; // 2 byte addr MSB, followed by 1 byte addr LSB)
                //const RingBuffer<uint8_t,false>::NonVolatileReader checksum_reader = reader; // so we can calculate the check sum later
                if (reader.read(&addr[0], 3) != 3) goto free_sysex_data;
                if (addr[0] == 0x40 && addr[1] == 0x00) { // System Parameters
                    dmsg(3,("\tSystem Parameter\n"));
                    if (addr[2] == 0x7f) { // GS reset
                        for (int i = 0; i < engineChannels.size(); ++i) {
                            AbstractEngineChannel* pEngineChannel
                                = static_cast<AbstractEngineChannel*>(engineChannels[i]);
                            Sync< ArrayList<MidiInputPort*> > midiInputs = pEngineChannel->midiInputs.front();
                            for (int k = 0; k < midiInputs->size(); ++k) {
                                if ((*midiInputs)[k] == itSysexEvent->pMidiInputPort) { 
                                    KillAllVoices(pEngineChannel, itSysexEvent);
                                    pEngineChannel->ResetControllers();
                                    break;
                                }
                            }
                        }
                    }
                }
                else if (addr[0] == 0x40 && addr[1] == 0x01) { // Common Parameters
                    dmsg(3,("\tCommon Parameter\n"));
                }
                else if (addr[0] == 0x40 && (addr[1] & 0xf0) == 0x10) { // Part Parameters (1)
                    dmsg(3,("\tPart Parameter\n"));
                    switch (addr[2]) {
                        case 0x40: { // scale tuning
                            dmsg(3,("\t\tScale Tuning\n"));
                            uint8_t scale_tunes[12]; // detuning of all 12 semitones of an octave
                            if (reader.read(&scale_tunes[0], 12) != 12) goto free_sysex_data;
                            uint8_t checksum;
                            if (!reader.pop(&checksum)) goto free_sysex_data;
                            #if CONFIG_ASSERT_GS_SYSEX_CHECKSUM
                            if (GSCheckSum(checksum_reader, 12)) goto free_sysex_data;
                            #endif // CONFIG_ASSERT_GS_SYSEX_CHECKSUM
                            for (int i = 0; i < 12; i++) scale_tunes[i] -= 64;
                            AdjustScaleTuning((int8_t*) scale_tunes);
                            dmsg(3,("\t\t\tNew scale applied.\n"));
                            break;
                        }
                        case 0x15: { // chromatic / drumkit mode
                            dmsg(3,("\t\tMIDI Instrument Map Switch\n"));
                            uint8_t part = addr[1] & 0x0f;
                            uint8_t map;
                            if (!reader.pop(&map)) goto free_sysex_data;
                            for (int i = 0; i < engineChannels.size(); ++i) {
                                AbstractEngineChannel* pEngineChannel
                                    = static_cast<AbstractEngineChannel*>(engineChannels[i]);
                                if (pEngineChannel->midiChannel == part ||
                                    pEngineChannel->midiChannel == midi_chan_all)
                                {   
                                    Sync< ArrayList<MidiInputPort*> > midiInputs = pEngineChannel->midiInputs.front();
                                    for (int k = 0; k < midiInputs->size(); ++k) {
                                        if ((*midiInputs)[k] == itSysexEvent->pMidiInputPort) {
                                            try {
                                                pEngineChannel->SetMidiInstrumentMap(map);
                                            } catch (Exception e) {
                                                dmsg(2,("\t\t\tCould not apply MIDI instrument map %d to part %d: %s\n", map, part, e.Message().c_str()));
                                                goto free_sysex_data;
                                            } catch (...) {
                                                dmsg(2,("\t\t\tCould not apply MIDI instrument map %d to part %d (unknown exception)\n", map, part));
                                                goto free_sysex_data;
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                            dmsg(3,("\t\t\tApplied MIDI instrument map %d to part %d.\n", map, part));
                            break;
                        }
                    }
                }
                else if (addr[0] == 0x40 && (addr[1] & 0xf0) == 0x20) { // Part Parameters (2)
                }
                else if (addr[0] == 0x41) { // Drum Setup Parameters
                }
                break;
            }
        }

        free_sysex_data: // finally free sysex data
        pSysexBuffer->increment_read_ptr(itSysexEvent->Param.Sysex.Size);
    }

    String AbstractEngine::GetFormatString(Format f) {
        switch(f) {
            case GIG: return "GIG";
            case SF2: return "SF2";
            case SFZ: return "SFZ";
            default:  return "UNKNOWN";
        }
    }

    String AbstractEngine::EngineName() {
        return GetFormatString(GetEngineFormat());
    }

    // static constant initializers
    const AbstractEngine::FloatTable AbstractEngine::VolumeCurve(InitVolumeCurve());
    const AbstractEngine::FloatTable AbstractEngine::PanCurve(InitPanCurve());
    const AbstractEngine::FloatTable AbstractEngine::CrossfadeCurve(InitCrossfadeCurve());

    float* AbstractEngine::InitVolumeCurve() {
        // line-segment approximation
        const float segments[] = {
            0, 0, 2, 0.0046, 16, 0.016, 31, 0.051, 45, 0.115, 54.5, 0.2,
            64.5, 0.39, 74, 0.74, 92, 1.03, 114, 1.94, 119.2, 2.2, 127, 2.2
        };
        return InitCurve(segments);
    }

    float* AbstractEngine::InitPanCurve() {
        // line-segment approximation
        const float segments[] = {
            0, 0, 1, 0,
            2, 0.05, 31.5, 0.7, 51, 0.851, 74.5, 1.12,
            127, 1.41, 128, 1.41
        };
        return InitCurve(segments, 129);
    }

    float* AbstractEngine::InitCrossfadeCurve() {
        // line-segment approximation
        const float segments[] = {
            0, 0, 1, 0.03, 10, 0.1, 51, 0.58, 127, 1
        };
        return InitCurve(segments);
    }

    float* AbstractEngine::InitCurve(const float* segments, int size) {
        float* y = new float[size];
        for (int x = 0 ; x < size ; x++) {
            if (x > segments[2]) segments += 2;
            y[x] = segments[1] + (x - segments[0]) *
                (segments[3] - segments[1]) / (segments[2] - segments[0]);
        }
        return y;
    }

    /**
     * Returns the volume factor to be used for the requested signed,
     * normalized @a pan value and audio @a channel index.
     *
     * @param pan - between -1.0 (most left) and +1.0 (most right)
     * @param channel - 0: return value for left channel, 1: return value for right channel
     * @returns final volume factor for the requested audio channel to be applied
     */
    float AbstractEngine::PanCurveValueNorm(float pan, int channel) {
        float pan128 = (pan + 1.f) / 2.f * 128.f;
        if (channel == 0) pan128 = 128 - pan128;
        return PanCurve[pan128]; //TODO: interpolation should be done here (float type "pan" argument here allows a much more fine graded result value than those 129 steps of the PanCurve)
    }

} // namespace LinuxSampler

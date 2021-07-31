/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2008 Christian Schoenebeck                         *
 *   Copyright (C) 2009-2012 Christian Schoenebeck and Grigor Iliev        *
 *   Copyright (C) 2013-2017 Christian Schoenebeck and Andreas Persson     *
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

#include "AbstractVoice.h"

namespace LinuxSampler {

    AbstractVoice::AbstractVoice(SignalUnitRack* pRack): pSignalUnitRack(pRack) {
        pEngineChannel = NULL;
        pLFO1 = new LFOUnsigned(1.0f);  // amplitude LFO (0..1 range)
        pLFO2 = new LFOUnsigned(1.0f);  // filter LFO (0..1 range)
        pLFO3 = new LFOSigned(1200.0f); // pitch LFO (-1200..+1200 range)
        PlaybackState = playback_state_end;
        SynthesisMode = 0; // set all mode bits to 0 first
        // select synthesis implementation (asm core is not supported ATM)
        #if 0 // CONFIG_ASM && ARCH_X86
        SYNTHESIS_MODE_SET_IMPLEMENTATION(SynthesisMode, Features::supportsMMX() && Features::supportsSSE());
        #else
        SYNTHESIS_MODE_SET_IMPLEMENTATION(SynthesisMode, false);
        #endif
        SYNTHESIS_MODE_SET_PROFILING(SynthesisMode, gig::Profiler::isEnabled());

        finalSynthesisParameters.filterLeft.Reset();
        finalSynthesisParameters.filterRight.Reset();
        
        pEq          = NULL;
        bEqSupport   = false;
    }

    AbstractVoice::~AbstractVoice() {
        if (pLFO1) delete pLFO1;
        if (pLFO2) delete pLFO2;
        if (pLFO3) delete pLFO3;
        
        if(pEq != NULL) delete pEq;
    }
            
    void AbstractVoice::CreateEq() {
        if(!bEqSupport) return;
        if(pEq != NULL) delete pEq;
        pEq = new EqSupport;
        pEq->InitEffect(GetEngine()->pAudioOutputDevice);
    }

    /**
     *  Resets voice variables. Should only be called if rendering process is
     *  suspended / not running.
     */
    void AbstractVoice::Reset() {
        finalSynthesisParameters.filterLeft.Reset();
        finalSynthesisParameters.filterRight.Reset();
        DiskStreamRef.pStream = NULL;
        DiskStreamRef.hStream = 0;
        DiskStreamRef.State   = Stream::state_unused;
        DiskStreamRef.OrderID = 0;
        PlaybackState = playback_state_end;
        itTriggerEvent = Pool<Event>::Iterator();
        itKillEvent    = Pool<Event>::Iterator();
    }

    /**
     *  Initializes and triggers the voice, a disk stream will be launched if
     *  needed.
     *
     *  @param pEngineChannel - engine channel on which this voice was ordered
     *  @param itNoteOnEvent  - event that caused triggering of this voice
     *  @param PitchBend      - MIDI detune factor (-8192 ... +8191)
     *  @param pRegion- points to the region which provides sample wave(s) and articulation data
     *  @param VoiceType      - type of this voice
     *  @param iKeyGroup      - a value > 0 defines a key group in which this voice is member of
     *  @returns 0 on success, a value < 0 if the voice wasn't triggered
     *           (either due to an error or e.g. because no region is
     *           defined for the given key)
     */
    int AbstractVoice::Trigger (
        AbstractEngineChannel*  pEngineChannel,
        Pool<Event>::Iterator&  itNoteOnEvent,
        int                     PitchBend,
        type_t                  VoiceType,
        int                     iKeyGroup
    ) {
        this->pEngineChannel = pEngineChannel;
        Orphan = false;

        #if CONFIG_DEVMODE
        if (itNoteOnEvent->FragmentPos() > GetEngine()->MaxSamplesPerCycle) { // just a sanity check for debugging
            dmsg(1,("Voice::Trigger(): ERROR, TriggerDelay > Totalsamples\n"));
        }
        #endif // CONFIG_DEVMODE

        Type            = VoiceType;
        pNote           = pEngineChannel->pEngine->NoteByID( itNoteOnEvent->Param.Note.ID );
        PlaybackState   = playback_state_init; // mark voice as triggered, but no audio rendered yet
        Delay           = itNoteOnEvent->FragmentPos();
        itTriggerEvent  = itNoteOnEvent;
        itKillEvent     = Pool<Event>::Iterator();
        MidiKeyBase* pKeyInfo = GetMidiKeyInfo(MIDIKey());

        pGroupEvents = iKeyGroup ? pEngineChannel->ActiveKeyGroups[iKeyGroup] : 0;

        SmplInfo   = GetSampleInfo();
        RgnInfo    = GetRegionInfo();
        InstrInfo  = GetInstrumentInfo();
        
        MIDIPan    = CalculatePan(pEngineChannel->iLastPanRequest);

        AboutToTrigger();

        // calculate volume
        const double velocityAttenuation = GetVelocityAttenuation(MIDIVelocity());
        float volume = CalculateVolume(velocityAttenuation) * pKeyInfo->Volume;
        if (volume <= 0) return -1;

        // select channel mode (mono or stereo)
        SYNTHESIS_MODE_SET_CHANNELS(SynthesisMode, SmplInfo.ChannelCount == 2);
        // select bit depth (16 or 24)
        SYNTHESIS_MODE_SET_BITDEPTH24(SynthesisMode, SmplInfo.BitDepth == 24);

        // get starting crossfade volume level
        float crossfadeVolume = CalculateCrossfadeVolume(MIDIVelocity());

        VolumeLeft  = volume * pKeyInfo->PanLeft;
        VolumeRight = volume * pKeyInfo->PanRight;

        // this rate is used for rather mellow volume fades
        const float subfragmentRate = GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE;
        // this rate is used for very fast volume fades
        const float quickRampRate = RTMath::Min(subfragmentRate, GetEngine()->SampleRate * 0.001f /* approx. 13ms */);
        CrossfadeSmoother.trigger(crossfadeVolume, subfragmentRate);

        VolumeSmoother.trigger(pEngineChannel->MidiVolume, subfragmentRate);
        NoteVolume.setCurveOnly(pNote ? pNote->Override.VolumeCurve : DEFAULT_FADE_CURVE);
        NoteVolume.setCurrentValue(pNote ? pNote->Override.Volume : 1.f);
        NoteVolume.setDefaultDuration(pNote ? pNote->Override.VolumeTime : DEFAULT_NOTE_VOLUME_TIME_S);

        // Check if the sample needs disk streaming or is too short for that
        long cachedsamples = GetSampleCacheSize() / SmplInfo.FrameSize;
        DiskVoice          = cachedsamples < SmplInfo.TotalFrameCount;

        SetSampleStartOffset();

        if (DiskVoice) { // voice to be streamed from disk
            if (cachedsamples > (GetEngine()->MaxSamplesPerCycle << CONFIG_MAX_PITCH)) {
                MaxRAMPos = cachedsamples - (GetEngine()->MaxSamplesPerCycle << CONFIG_MAX_PITCH) / SmplInfo.ChannelCount; //TODO: this calculation is too pessimistic and may better be moved to Render() method, so it calculates MaxRAMPos dependent to the current demand of sample points to be rendered (e.g. in case of JACK)
            } else {
                // The cache is too small to fit a max sample buffer.
                // Setting MaxRAMPos to 0 will probably cause a click
                // in the audio, but it's better than not handling
                // this case at all, which would have caused the
                // unsigned MaxRAMPos to be set to a negative number.
                MaxRAMPos = 0;
            }

            // check if there's a loop defined which completely fits into the cached (RAM) part of the sample
            RAMLoop = (SmplInfo.HasLoops && (SmplInfo.LoopStart + SmplInfo.LoopLength) <= MaxRAMPos);

            if (OrderNewStream()) return -1;
            dmsg(4,("Disk voice launched (cached samples: %ld, total Samples: %d, MaxRAMPos: %lu, RAMLooping: %s)\n", cachedsamples, SmplInfo.TotalFrameCount, MaxRAMPos, (RAMLoop) ? "yes" : "no"));
        }
        else { // RAM only voice
            MaxRAMPos = cachedsamples;
            RAMLoop = (SmplInfo.HasLoops);
            dmsg(4,("RAM only voice launched (Looping: %s)\n", (RAMLoop) ? "yes" : "no"));
        }
        if (RAMLoop) {
            loop.uiTotalCycles = SmplInfo.LoopPlayCount;
            loop.uiCyclesLeft  = SmplInfo.LoopPlayCount;
            loop.uiStart       = SmplInfo.LoopStart;
            loop.uiEnd         = SmplInfo.LoopStart + SmplInfo.LoopLength;
            loop.uiSize        = SmplInfo.LoopLength;
        }

        Pitch = CalculatePitchInfo(PitchBend);
        NotePitch.setCurveOnly(pNote ? pNote->Override.PitchCurve : DEFAULT_FADE_CURVE);
        NotePitch.setCurrentValue(pNote ? pNote->Override.Pitch : 1.0f);
        NotePitch.setDefaultDuration(pNote ? pNote->Override.PitchTime : DEFAULT_NOTE_PITCH_TIME_S);
        NoteCutoff = (pNote) ? pNote->Override.Cutoff : 1.0f;
        NoteResonance = (pNote) ? pNote->Override.Resonance : 1.0f;

        // the length of the decay and release curves are dependent on the velocity
        const double velrelease = 1 / GetVelocityRelease(MIDIVelocity());

        if (pSignalUnitRack == NULL) { // setup EG 1 (VCA EG)
            // get current value of EG1 controller
            double eg1controllervalue = GetEG1ControllerValue(MIDIVelocity());

            // calculate influence of EG1 controller on EG1's parameters
            EGInfo egInfo = CalculateEG1ControllerInfluence(eg1controllervalue);

            if (pNote) {
                egInfo.Attack  *= pNote->Override.Attack;
                egInfo.Decay   *= pNote->Override.Decay;
                egInfo.Release *= pNote->Override.Release;
            }

            TriggerEG1(egInfo, velrelease, velocityAttenuation, GetEngine()->SampleRate, MIDIVelocity());
        } else {
            pSignalUnitRack->Trigger();
        }

        const uint8_t pan = (pSignalUnitRack) ? pSignalUnitRack->GetEndpointUnit()->CalculatePan(MIDIPan) : MIDIPan;
        for (int c = 0; c < 2; ++c) {
            float value = (pNote) ? AbstractEngine::PanCurveValueNorm(pNote->Override.Pan, c) : 1.f;
            NotePan[c].setCurveOnly(pNote ? pNote->Override.PanCurve : DEFAULT_FADE_CURVE);
            NotePan[c].setCurrentValue(value);
            NotePan[c].setDefaultDuration(pNote ? pNote->Override.PanTime : DEFAULT_NOTE_PAN_TIME_S);
        }

        PanLeftSmoother.trigger(
            AbstractEngine::PanCurve[128 - pan],
            quickRampRate //NOTE: maybe we should have 2 separate pan smoothers, one for MIDI CC10 (with slow rate) and one for instrument script change_pan() calls (with fast rate)
        );
        PanRightSmoother.trigger(
            AbstractEngine::PanCurve[pan],
            quickRampRate //NOTE: maybe we should have 2 separate pan smoothers, one for MIDI CC10 (with slow rate) and one for instrument script change_pan() calls (with fast rate)
        );

#ifdef CONFIG_INTERPOLATE_VOLUME
        // setup initial volume in synthesis parameters
    #ifdef CONFIG_PROCESS_MUTED_CHANNELS
        if (pEngineChannel->GetMute()) {
            finalSynthesisParameters.fFinalVolumeLeft  = 0;
            finalSynthesisParameters.fFinalVolumeRight = 0;
        }
        else
    #else
        {
            float finalVolume;
            if (pSignalUnitRack == NULL) {
                finalVolume = pEngineChannel->MidiVolume * crossfadeVolume * pEG1->getLevel();
            } else {
                finalVolume = pEngineChannel->MidiVolume * crossfadeVolume * pSignalUnitRack->GetEndpointUnit()->GetVolume();
            }
            finalVolume *= NoteVolume.currentValue();

            finalSynthesisParameters.fFinalVolumeLeft  = finalVolume * VolumeLeft  * PanLeftSmoother.render()  * NotePan[0].currentValue();
            finalSynthesisParameters.fFinalVolumeRight = finalVolume * VolumeRight * PanRightSmoother.render() * NotePan[1].currentValue();
        }
    #endif
#endif

        if (pSignalUnitRack == NULL) {
            // setup EG 2 (VCF Cutoff EG)
            {
                // get current value of EG2 controller
                double eg2controllervalue = GetEG2ControllerValue(MIDIVelocity());

                // calculate influence of EG2 controller on EG2's parameters
                EGInfo egInfo = CalculateEG2ControllerInfluence(eg2controllervalue);

                if (pNote) {
                    egInfo.Attack  *= pNote->Override.CutoffAttack;
                    egInfo.Decay   *= pNote->Override.CutoffDecay;
                    egInfo.Release *= pNote->Override.CutoffRelease;
                }

                TriggerEG2(egInfo, velrelease, velocityAttenuation, GetEngine()->SampleRate, MIDIVelocity());
            }


            // setup EG 3 (VCO EG)
            {
                // if portamento mode is on, we dedicate EG3 purely for portamento, otherwise if portamento is off we do as told by the patch
                bool  bPortamento = pEngineChannel->PortamentoMode && pEngineChannel->PortamentoPos >= 0.0f;
                float eg3depth = (bPortamento)
                             ? RTMath::CentsToFreqRatio((pEngineChannel->PortamentoPos - (float) MIDIKey()) * 100)
                             : RTMath::CentsToFreqRatio(RgnInfo.EG3Depth);
                float eg3time = (bPortamento)
                            ? pEngineChannel->PortamentoTime
                            : RgnInfo.EG3Attack;
                EG3.trigger(eg3depth, eg3time, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                dmsg(5,("PortamentoPos=%f, depth=%f, time=%f\n", pEngineChannel->PortamentoPos, eg3depth, eg3time));
            }


            // setup LFO 1 (VCA LFO)
            InitLFO1();
            // setup LFO 2 (VCF Cutoff LFO)
            InitLFO2();
            // setup LFO 3 (VCO LFO)
            InitLFO3();
        }


        #if CONFIG_FORCE_FILTER
        const bool bUseFilter = true;
        #else // use filter only if instrument file told so
        const bool bUseFilter = RgnInfo.VCFEnabled;
        #endif // CONFIG_FORCE_FILTER
        SYNTHESIS_MODE_SET_FILTER(SynthesisMode, bUseFilter);
        if (bUseFilter) {
            #ifdef CONFIG_OVERRIDE_CUTOFF_CTRL
            VCFCutoffCtrl.controller = CONFIG_OVERRIDE_CUTOFF_CTRL;
            #else // use the one defined in the instrument file
            VCFCutoffCtrl.controller = GetVCFCutoffCtrl();
            #endif // CONFIG_OVERRIDE_CUTOFF_CTRL

            #ifdef CONFIG_OVERRIDE_RESONANCE_CTRL
            VCFResonanceCtrl.controller = CONFIG_OVERRIDE_RESONANCE_CTRL;
            #else // use the one defined in the instrument file
            VCFResonanceCtrl.controller = GetVCFResonanceCtrl();
            #endif // CONFIG_OVERRIDE_RESONANCE_CTRL

            #ifndef CONFIG_OVERRIDE_FILTER_TYPE
            finalSynthesisParameters.filterLeft.SetType(RgnInfo.VCFType);
            finalSynthesisParameters.filterRight.SetType(RgnInfo.VCFType);
            #else // override filter type
            finalSynthesisParameters.filterLeft.SetType(CONFIG_OVERRIDE_FILTER_TYPE);
            finalSynthesisParameters.filterRight.SetType(CONFIG_OVERRIDE_FILTER_TYPE);
            #endif // CONFIG_OVERRIDE_FILTER_TYPE

            VCFCutoffCtrl.value    = pEngineChannel->ControllerTable[VCFCutoffCtrl.controller];
            VCFResonanceCtrl.value = pEngineChannel->ControllerTable[VCFResonanceCtrl.controller];

            // calculate cutoff frequency
            CutoffBase = CalculateCutoffBase(MIDIVelocity());

            VCFCutoffCtrl.fvalue = CalculateFinalCutoff(CutoffBase);

            // calculate resonance
            float resonance = (float) (VCFResonanceCtrl.controller ? VCFResonanceCtrl.value : RgnInfo.VCFResonance);
            VCFResonanceCtrl.fvalue = resonance;
        } else {
            VCFCutoffCtrl.controller    = 0;
            VCFResonanceCtrl.controller = 0;
        }
        
        const bool bEq =
            pSignalUnitRack != NULL && pSignalUnitRack->HasEq() && pEq->HasSupport();

        if (bEq) {
            pEq->GetInChannelLeft()->Clear();
            pEq->GetInChannelRight()->Clear();
            pEq->RenderAudio(GetEngine()->pAudioOutputDevice->MaxSamplesPerCycle());
        }

        return 0; // success
    }
    
    void AbstractVoice::SetSampleStartOffset() {
        double pos = RgnInfo.SampleStartOffset; // offset where we should start playback of sample

        // if another sample playback start position was requested by instrument
        // script (built-in script function play_note())
        if (pNote && pNote->Override.SampleOffset >= 0) {
            double overridePos =
                double(SmplInfo.SampleRate) * double(pNote->Override.SampleOffset) / 1000000.0;
            if (overridePos < SmplInfo.TotalFrameCount)
                pos = overridePos;
        }

        finalSynthesisParameters.dPos = pos;
        Pos = pos;
    }

    /**
     *  Synthesizes the current audio fragment for this voice.
     *
     *  @param Samples - number of sample points to be rendered in this audio
     *                   fragment cycle
     *  @param pSrc    - pointer to input sample data
     *  @param Skip    - number of sample points to skip in output buffer
     */
    void AbstractVoice::Synthesize(uint Samples, sample_t* pSrc, uint Skip) {
        bool delay = false; // Whether the voice playback should be delayed for this call
        
        if (pSignalUnitRack != NULL) {
            uint delaySteps = pSignalUnitRack->GetEndpointUnit()->DelayTrigger();
            if (delaySteps > 0) { // delay on the endpoint unit means delay of the voice playback
                if (delaySteps >= Samples) {
                    pSignalUnitRack->GetEndpointUnit()->DecreaseDelay(Samples);
                    delay = true;
                } else {
                    pSignalUnitRack->GetEndpointUnit()->DecreaseDelay(delaySteps);
                    Samples -= delaySteps;
                    Skip += delaySteps;
                }
            }
        }
        
        AbstractEngineChannel* pChannel = pEngineChannel;
        MidiKeyBase* pMidiKeyInfo = GetMidiKeyInfo(MIDIKey());

        const bool bVoiceRequiresDedicatedRouting =
            pEngineChannel->GetFxSendCount() > 0 &&
            (pMidiKeyInfo->ReverbSend || pMidiKeyInfo->ChorusSend);
        
        const bool bEq =
            pSignalUnitRack != NULL && pSignalUnitRack->HasEq() && pEq->HasSupport();

        if (bEq) {
            pEq->GetInChannelLeft()->Clear();
            pEq->GetInChannelRight()->Clear();
            finalSynthesisParameters.pOutLeft  = &pEq->GetInChannelLeft()->Buffer()[Skip];
            finalSynthesisParameters.pOutRight = &pEq->GetInChannelRight()->Buffer()[Skip];
            pSignalUnitRack->UpdateEqSettings(pEq);
        } else if (bVoiceRequiresDedicatedRouting) {
            finalSynthesisParameters.pOutLeft  = &GetEngine()->pDedicatedVoiceChannelLeft->Buffer()[Skip];
            finalSynthesisParameters.pOutRight = &GetEngine()->pDedicatedVoiceChannelRight->Buffer()[Skip];
        } else {
            finalSynthesisParameters.pOutLeft  = &pChannel->pChannelLeft->Buffer()[Skip];
            finalSynthesisParameters.pOutRight = &pChannel->pChannelRight->Buffer()[Skip];
        }
        finalSynthesisParameters.pSrc = pSrc;

        RTList<Event>::Iterator itCCEvent = pChannel->pEvents->first();
        RTList<Event>::Iterator itNoteEvent;
        GetFirstEventOnKey(HostKey(), itNoteEvent);

        RTList<Event>::Iterator itGroupEvent;
        if (pGroupEvents && !Orphan) itGroupEvent = pGroupEvents->first();

        if (itTriggerEvent) { // skip events that happened before this voice was triggered
            while (itCCEvent && itCCEvent->FragmentPos() <= Skip) ++itCCEvent;
            while (itGroupEvent && itGroupEvent->FragmentPos() <= Skip) ++itGroupEvent;

            // we can't simply compare the timestamp here, because note events
            // might happen on the same time stamp, so we have to deal on the
            // actual sequence the note events arrived instead (see bug #112)
            for (; itNoteEvent; ++itNoteEvent) {
                if (itTriggerEvent == itNoteEvent) {
                    ++itNoteEvent;
                    break;
                }
            }
        }

        uint killPos = 0;
        if (itKillEvent) {
            int maxFadeOutPos = Samples - GetEngine()->GetMinFadeOutSamples();
            if (maxFadeOutPos < 0) {
                // There's not enough space in buffer to do a fade out
                // from max volume (this can only happen for audio
                // drivers that use Samples < MaxSamplesPerCycle).
                // End the EG1 here, at pos 0, with a shorter max fade
                // out time.
                if (pSignalUnitRack == NULL) {
                    pEG1->enterFadeOutStage(Samples / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                } else {
                    pSignalUnitRack->EnterFadeOutStage(Samples / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                }
                itKillEvent = Pool<Event>::Iterator();
            } else {
                killPos = RTMath::Min(itKillEvent->FragmentPos(), maxFadeOutPos);
            }
        }

        uint i = Skip;
        while (i < Samples) {
            int iSubFragmentEnd = RTMath::Min(i + CONFIG_DEFAULT_SUBFRAGMENT_SIZE, Samples);

            // initialize all final synthesis parameters
            fFinalCutoff    = VCFCutoffCtrl.fvalue;
            fFinalResonance = VCFResonanceCtrl.fvalue;

            // process MIDI control change, aftertouch and pitchbend events for this subfragment
            processCCEvents(itCCEvent, iSubFragmentEnd);
            uint8_t pan = MIDIPan;
            if (pSignalUnitRack != NULL) pan = pSignalUnitRack->GetEndpointUnit()->CalculatePan(MIDIPan);

            PanLeftSmoother.update(AbstractEngine::PanCurve[128 - pan]);
            PanRightSmoother.update(AbstractEngine::PanCurve[pan]);

            finalSynthesisParameters.fFinalPitch = Pitch.PitchBase * Pitch.PitchBend * NotePitch.render();

            float fFinalVolume = VolumeSmoother.render() * CrossfadeSmoother.render() * NoteVolume.render();
#ifdef CONFIG_PROCESS_MUTED_CHANNELS
            if (pChannel->GetMute()) fFinalVolume = 0;
#endif

            // process transition events (note on, note off & sustain pedal)
            processTransitionEvents(itNoteEvent, iSubFragmentEnd);
            processGroupEvents(itGroupEvent, iSubFragmentEnd);
            
            if (pSignalUnitRack == NULL) {
                // if the voice was killed in this subfragment, or if the
                // filter EG is finished, switch EG1 to fade out stage
                if ((itKillEvent && killPos <= iSubFragmentEnd) ||
                    (SYNTHESIS_MODE_GET_FILTER(SynthesisMode) &&
                    pEG2->getSegmentType() == EG::segment_end)) {
                    pEG1->enterFadeOutStage();
                    itKillEvent = Pool<Event>::Iterator();
                }

                // process envelope generators
                switch (pEG1->getSegmentType()) {
                    case EG::segment_lin:
                        fFinalVolume *= pEG1->processLin();
                        break;
                    case EG::segment_exp:
                        fFinalVolume *= pEG1->processExp();
                        break;
                    case EG::segment_end:
                        fFinalVolume *= pEG1->getLevel();
                        break; // noop
                    case EG::segment_pow:
                        fFinalVolume *= pEG1->processPow();
                        break;
                }
                switch (pEG2->getSegmentType()) {
                    case EG::segment_lin:
                        fFinalCutoff *= pEG2->processLin();
                        break;
                    case EG::segment_exp:
                        fFinalCutoff *= pEG2->processExp();
                        break;
                    case EG::segment_end:
                        fFinalCutoff *= pEG2->getLevel();
                        break; // noop
                    case EG::segment_pow:
                        fFinalCutoff *= pEG2->processPow();
                        break;
                }
                if (EG3.active()) finalSynthesisParameters.fFinalPitch *= EG3.render();

                // process low frequency oscillators
                if (bLFO1Enabled) fFinalVolume *= (1.0f - pLFO1->render());
                if (bLFO2Enabled) fFinalCutoff *= (1.0f - pLFO2->render());
                if (bLFO3Enabled) finalSynthesisParameters.fFinalPitch *= RTMath::CentsToFreqRatio(pLFO3->render());
            } else {
                // if the voice was killed in this subfragment, enter fade out stage
                if (itKillEvent && killPos <= iSubFragmentEnd) {
                    pSignalUnitRack->EnterFadeOutStage();
                    itKillEvent = Pool<Event>::Iterator();
                }
                
                // if the filter EG is finished, switch EG1 to fade out stage
                /*if (SYNTHESIS_MODE_GET_FILTER(SynthesisMode) &&
                    pEG2->getSegmentType() == EG::segment_end) {
                    pEG1->enterFadeOutStage();
                    itKillEvent = Pool<Event>::Iterator();
                }*/
                // TODO: ^^^

                fFinalVolume   *= pSignalUnitRack->GetEndpointUnit()->GetVolume();
                fFinalCutoff    = pSignalUnitRack->GetEndpointUnit()->CalculateFilterCutoff(fFinalCutoff);
                fFinalResonance = pSignalUnitRack->GetEndpointUnit()->CalculateResonance(fFinalResonance);
                
                finalSynthesisParameters.fFinalPitch =
                    pSignalUnitRack->GetEndpointUnit()->CalculatePitch(finalSynthesisParameters.fFinalPitch);
                    
            }

            fFinalCutoff    *= NoteCutoff;
            fFinalResonance *= NoteResonance;

            // limit the pitch so we don't read outside the buffer
            finalSynthesisParameters.fFinalPitch = RTMath::Min(finalSynthesisParameters.fFinalPitch, float(1 << CONFIG_MAX_PITCH));

            // if filter enabled then update filter coefficients
            if (SYNTHESIS_MODE_GET_FILTER(SynthesisMode)) {
                finalSynthesisParameters.filterLeft.SetParameters(fFinalCutoff, fFinalResonance, GetEngine()->SampleRate);
                finalSynthesisParameters.filterRight.SetParameters(fFinalCutoff, fFinalResonance, GetEngine()->SampleRate);
            }

            // do we need resampling?
            const float __PLUS_ONE_CENT  = 1.000577789506554859250142541782224725466f;
            const float __MINUS_ONE_CENT = 0.9994225441413807496009516495583113737666f;
            const bool bResamplingRequired = !(finalSynthesisParameters.fFinalPitch <= __PLUS_ONE_CENT &&
                                               finalSynthesisParameters.fFinalPitch >= __MINUS_ONE_CENT);
            SYNTHESIS_MODE_SET_INTERPOLATE(SynthesisMode, bResamplingRequired);

            // prepare final synthesis parameters structure
            finalSynthesisParameters.uiToGo            = iSubFragmentEnd - i;
#ifdef CONFIG_INTERPOLATE_VOLUME
            finalSynthesisParameters.fFinalVolumeDeltaLeft  =
                (fFinalVolume * VolumeLeft  * PanLeftSmoother.render() * NotePan[0].render() -
                 finalSynthesisParameters.fFinalVolumeLeft) / finalSynthesisParameters.uiToGo;
            finalSynthesisParameters.fFinalVolumeDeltaRight =
                (fFinalVolume * VolumeRight * PanRightSmoother.render() * NotePan[1].render() -
                 finalSynthesisParameters.fFinalVolumeRight) / finalSynthesisParameters.uiToGo;
#else
            finalSynthesisParameters.fFinalVolumeLeft  =
                fFinalVolume * VolumeLeft  * PanLeftSmoother.render()  * NotePan[0].render();
            finalSynthesisParameters.fFinalVolumeRight =
                fFinalVolume * VolumeRight * PanRightSmoother.render() * NotePan[1].render();
#endif
            // render audio for one subfragment
            if (!delay) RunSynthesisFunction(SynthesisMode, &finalSynthesisParameters, &loop);

            if (pSignalUnitRack == NULL) {
                // stop the rendering if volume EG is finished
                if (pEG1->getSegmentType() == EG::segment_end) break;
            } else {
                // stop the rendering if the endpoint unit is not active
                if (!pSignalUnitRack->GetEndpointUnit()->Active()) break;
            }

            const double newPos = Pos + (iSubFragmentEnd - i) * finalSynthesisParameters.fFinalPitch;

            if (pSignalUnitRack == NULL) {
                // increment envelopes' positions
                if (pEG1->active()) {

                    // if sample has a loop and loop start has been reached in this subfragment, send a special event to EG1 to let it finish the attack hold stage
                    if (SmplInfo.HasLoops && Pos <= SmplInfo.LoopStart && SmplInfo.LoopStart < newPos) {
                        pEG1->update(EG::event_hold_end, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                    }

                    pEG1->increment(1);
                    if (!pEG1->toStageEndLeft()) pEG1->update(EG::event_stage_end, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                }
                if (pEG2->active()) {
                    pEG2->increment(1);
                    if (!pEG2->toStageEndLeft()) pEG2->update(EG::event_stage_end, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                }
                EG3.increment(1);
                if (!EG3.toEndLeft()) EG3.update(); // neutralize envelope coefficient if end reached
            } else {
                    // if sample has a loop and loop start has been reached in this subfragment, send a special event to EG1 to let it finish the attack hold stage
                    /*if (SmplInfo.HasLoops && Pos <= SmplInfo.LoopStart && SmplInfo.LoopStart < newPos) {
                        pEG1->update(EG::event_hold_end, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                    }*/
                // TODO: ^^^
                
                if (!delay) pSignalUnitRack->Increment();
            }

            Pos = newPos;
            i = iSubFragmentEnd;
        }
        
        if (delay) return;

        if (bVoiceRequiresDedicatedRouting) {
            if (bEq) {
                pEq->RenderAudio(Samples);
                pEq->GetOutChannelLeft()->CopyTo(GetEngine()->pDedicatedVoiceChannelLeft, Samples);
                pEq->GetOutChannelRight()->CopyTo(GetEngine()->pDedicatedVoiceChannelRight, Samples);
            }
            optional<float> effectSendLevels[2] = {
                pMidiKeyInfo->ReverbSend,
                pMidiKeyInfo->ChorusSend
            };
            GetEngine()->RouteDedicatedVoiceChannels(pEngineChannel, effectSendLevels, Samples);
        } else if (bEq) {
            pEq->RenderAudio(Samples);
            pEq->GetOutChannelLeft()->MixTo(pChannel->pChannelLeft, Samples);
            pEq->GetOutChannelRight()->MixTo(pChannel->pChannelRight, Samples);
        }
    }

    /**
     * Process given list of MIDI control change, aftertouch and pitch bend
     * events for the given time.
     *
     * @param itEvent - iterator pointing to the next event to be processed
     * @param End     - youngest time stamp where processing should be stopped
     */
    void AbstractVoice::processCCEvents(RTList<Event>::Iterator& itEvent, uint End) {
        for (; itEvent && itEvent->FragmentPos() <= End; ++itEvent) {
            if ((itEvent->Type == Event::type_control_change || itEvent->Type == Event::type_channel_pressure)
                && itEvent->Param.CC.Controller) // if (valid) MIDI control change event
            {
                if (itEvent->Param.CC.Controller == VCFCutoffCtrl.controller) {
                    ProcessCutoffEvent(itEvent);
                }
                if (itEvent->Param.CC.Controller == VCFResonanceCtrl.controller) {
                    processResonanceEvent(itEvent);
                }
                if (itEvent->Param.CC.Controller == CTRL_TABLE_IDX_AFTERTOUCH ||
                    itEvent->Type == Event::type_channel_pressure)
                {
                    ProcessChannelPressureEvent(itEvent);
                }
                if (pSignalUnitRack == NULL) {
                    if (itEvent->Param.CC.Controller == pLFO1->ExtController) {
                        pLFO1->updateByMIDICtrlValue(itEvent->Param.CC.Value);
                    }
                    if (itEvent->Param.CC.Controller == pLFO2->ExtController) {
                        pLFO2->updateByMIDICtrlValue(itEvent->Param.CC.Value);
                    }
                    if (itEvent->Param.CC.Controller == pLFO3->ExtController) {
                        pLFO3->updateByMIDICtrlValue(itEvent->Param.CC.Value);
                    }
                }
                if (itEvent->Param.CC.Controller == 7) { // volume
                    VolumeSmoother.update(AbstractEngine::VolumeCurve[itEvent->Param.CC.Value]);
                } else if (itEvent->Param.CC.Controller == 10) { // panpot
                    MIDIPan = CalculatePan(itEvent->Param.CC.Value);
                }
            } else if (itEvent->Type == Event::type_pitchbend) { // if pitch bend event
                processPitchEvent(itEvent);
            } else if (itEvent->Type == Event::type_note_pressure) {
                ProcessPolyphonicKeyPressureEvent(itEvent);
            }

            ProcessCCEvent(itEvent);
            if (pSignalUnitRack != NULL) {
                pSignalUnitRack->ProcessCCEvent(itEvent);
            }
        }
    }

    void AbstractVoice::processPitchEvent(RTList<Event>::Iterator& itEvent) {
        Pitch.PitchBend = RTMath::CentsToFreqRatio(itEvent->Param.Pitch.Pitch * Pitch.PitchBendRange);
    }

    void AbstractVoice::processResonanceEvent(RTList<Event>::Iterator& itEvent) {
        // convert absolute controller value to differential
        const int ctrldelta = itEvent->Param.CC.Value - VCFResonanceCtrl.value;
        VCFResonanceCtrl.value = itEvent->Param.CC.Value;
        const float resonancedelta = (float) ctrldelta;
        fFinalResonance += resonancedelta;
        // needed for initialization of parameter
        VCFResonanceCtrl.fvalue = itEvent->Param.CC.Value;
    }

    /**
     * Process given list of MIDI note on, note off, sustain pedal events and
     * note synthesis parameter events for the given time.
     *
     * @param itEvent - iterator pointing to the next event to be processed
     * @param End     - youngest time stamp where processing should be stopped
     */
    void AbstractVoice::processTransitionEvents(RTList<Event>::Iterator& itEvent, uint End) {
        for (; itEvent && itEvent->FragmentPos() <= End; ++itEvent) {
            // some voice types ignore note off 
            if (!(Type & (Voice::type_one_shot | Voice::type_release_trigger | Voice::type_controller_triggered))) {
                if (itEvent->Type == Event::type_release_key) {
                    EnterReleaseStage();
                } else if (itEvent->Type == Event::type_cancel_release_key) {
                    if (pSignalUnitRack == NULL) {
                        pEG1->update(EG::event_cancel_release, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                        pEG2->update(EG::event_cancel_release, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                    } else {
                        pSignalUnitRack->CancelRelease();
                    }
                }
            }
            // process stop-note events (caused by built-in instrument script function note_off())
            if (itEvent->Type == Event::type_release_note && pNote &&
                pEngineChannel->pEngine->NoteByID( itEvent->Param.Note.ID ) == pNote)
            {
                EnterReleaseStage();
            }
            // process kill-note events (caused by built-in instrument script function fade_out())
            if (itEvent->Type == Event::type_kill_note && pNote &&
                pEngineChannel->pEngine->NoteByID( itEvent->Param.Note.ID ) == pNote)
            {
                Kill(itEvent);
            }
            // process synthesis parameter events (caused by built-in realt-time instrument script functions)
            if (itEvent->Type == Event::type_note_synth_param && pNote &&
                pEngineChannel->pEngine->NoteByID( itEvent->Param.NoteSynthParam.NoteID ) == pNote)
            {
                switch (itEvent->Param.NoteSynthParam.Type) {
                    case Event::synth_param_volume:
                        NoteVolume.fadeTo(itEvent->Param.NoteSynthParam.AbsValue, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                        break;
                    case Event::synth_param_volume_time:
                        NoteVolume.setDefaultDuration(itEvent->Param.NoteSynthParam.AbsValue);
                        break;
                    case Event::synth_param_volume_curve:
                        NoteVolume.setCurve((fade_curve_t)itEvent->Param.NoteSynthParam.AbsValue, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                        break;
                    case Event::synth_param_pitch:
                        NotePitch.fadeTo(itEvent->Param.NoteSynthParam.AbsValue, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                        break;
                    case Event::synth_param_pitch_time:
                        NotePitch.setDefaultDuration(itEvent->Param.NoteSynthParam.AbsValue);
                        break;
                    case Event::synth_param_pitch_curve:
                        NotePitch.setCurve((fade_curve_t)itEvent->Param.NoteSynthParam.AbsValue, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                        break;
                    case Event::synth_param_pan:
                        NotePan[0].fadeTo(
                            AbstractEngine::PanCurveValueNorm(itEvent->Param.NoteSynthParam.AbsValue, 0 /*left*/),
                            GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE
                        );
                        NotePan[1].fadeTo(
                            AbstractEngine::PanCurveValueNorm(itEvent->Param.NoteSynthParam.AbsValue, 1 /*right*/),
                            GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE
                        );
                        break;
                    case Event::synth_param_pan_time:
                        NotePan[0].setDefaultDuration(itEvent->Param.NoteSynthParam.AbsValue);
                        NotePan[1].setDefaultDuration(itEvent->Param.NoteSynthParam.AbsValue);
                        break;
                    case Event::synth_param_pan_curve:
                        NotePan[0].setCurve((fade_curve_t)itEvent->Param.NoteSynthParam.AbsValue, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                        NotePan[1].setCurve((fade_curve_t)itEvent->Param.NoteSynthParam.AbsValue, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                        break;
                    case Event::synth_param_cutoff:
                        NoteCutoff = itEvent->Param.NoteSynthParam.AbsValue;
                        break;
                    case Event::synth_param_resonance:
                        NoteResonance = itEvent->Param.NoteSynthParam.AbsValue;
                        break;
                    case Event::synth_param_amp_lfo_depth:
                        pLFO1->setScriptDepthFactor(itEvent->Param.NoteSynthParam.AbsValue);
                        break;
                    case Event::synth_param_amp_lfo_freq:
                        pLFO1->setScriptFrequencyFactor(itEvent->Param.NoteSynthParam.AbsValue, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                        break;
                    case Event::synth_param_cutoff_lfo_depth:
                        pLFO2->setScriptDepthFactor(itEvent->Param.NoteSynthParam.AbsValue);
                        break;
                    case Event::synth_param_cutoff_lfo_freq:
                        pLFO2->setScriptFrequencyFactor(itEvent->Param.NoteSynthParam.AbsValue, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                        break;
                    case Event::synth_param_pitch_lfo_depth:
                        pLFO3->setScriptDepthFactor(itEvent->Param.NoteSynthParam.AbsValue);
                        break;
                    case Event::synth_param_pitch_lfo_freq:
                        pLFO3->setScriptFrequencyFactor(itEvent->Param.NoteSynthParam.AbsValue, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
                        break;

                    case Event::synth_param_attack:
                    case Event::synth_param_decay:
                    case Event::synth_param_sustain:
                    case Event::synth_param_release:
                        break; // noop
                }
            }
        }
    }

    /**
     * Process given list of events aimed at all voices in a key group.
     *
     * @param itEvent - iterator pointing to the next event to be processed
     * @param End     - youngest time stamp where processing should be stopped
     */
    void AbstractVoice::processGroupEvents(RTList<Event>::Iterator& itEvent, uint End) {
        for (; itEvent && itEvent->FragmentPos() <= End; ++itEvent) {
            ProcessGroupEvent(itEvent);
        }
    }

    /** @brief Update current portamento position.
     *
     * Will be called when portamento mode is enabled to get the final
     * portamento position of this active voice from where the next voice(s)
     * might continue to slide on.
     *
     * @param itNoteOffEvent - event which causes this voice to die soon
     */
    void AbstractVoice::UpdatePortamentoPos(Pool<Event>::Iterator& itNoteOffEvent) {
        if (pSignalUnitRack == NULL) {
            const float fFinalEG3Level = EG3.level(itNoteOffEvent->FragmentPos());
            pEngineChannel->PortamentoPos = (float) MIDIKey() + RTMath::FreqRatioToCents(fFinalEG3Level) * 0.01f;
        } else {
            // TODO: 
        }
    }

    /**
     *  Kill the voice in regular sense. Let the voice render audio until
     *  the kill event actually occured and then fade down the volume level
     *  very quickly and let the voice die finally. Unlike a normal release
     *  of a voice, a kill process cannot be cancalled and is therefore
     *  usually used for voice stealing and key group conflicts.
     *
     *  @param itKillEvent - event which caused the voice to be killed
     */
    void AbstractVoice::Kill(Pool<Event>::Iterator& itKillEvent) {
        #if CONFIG_DEVMODE
        if (!itKillEvent) dmsg(1,("AbstractVoice::Kill(): ERROR, !itKillEvent !!!\n"));
        if (itKillEvent && !itKillEvent.isValid()) dmsg(1,("AbstractVoice::Kill(): ERROR, itKillEvent invalid !!!\n"));
        #endif // CONFIG_DEVMODE

        if (itTriggerEvent && itKillEvent->FragmentPos() <= itTriggerEvent->FragmentPos()) return;
        this->itKillEvent = itKillEvent;
    }

    Voice::PitchInfo AbstractVoice::CalculatePitchInfo(int PitchBend) {
        PitchInfo pitch;
        double pitchbasecents = InstrInfo.FineTune + RgnInfo.FineTune + GetEngine()->ScaleTuning[MIDIKey() % 12];

        // GSt behaviour: maximum transpose up is 40 semitones. If
        // MIDI key is more than 40 semitones above unity note,
        // the transpose is not done.
        //
        // Update: Removed this GSt misbehavior. I don't think that any stock
        // gig sound requires it to resemble its original sound.
        // -- Christian, 2017-07-09
        if (!SmplInfo.Unpitched /* && (MIDIKey() - (int) RgnInfo.UnityNote) < 40*/)
            pitchbasecents += (MIDIKey() - (int) RgnInfo.UnityNote) * 100;

        pitch.PitchBase = RTMath::CentsToFreqRatioUnlimited(pitchbasecents) * (double(SmplInfo.SampleRate) / double(GetEngine()->SampleRate));
        pitch.PitchBendRange = 1.0 / 8192.0 * 100.0 * InstrInfo.PitchbendRange;
        pitch.PitchBend = RTMath::CentsToFreqRatio(PitchBend * pitch.PitchBendRange);

        return pitch;
    }
    
    void AbstractVoice::onScaleTuningChanged() {
        PitchInfo pitch = this->Pitch;
        double pitchbasecents = InstrInfo.FineTune + RgnInfo.FineTune + GetEngine()->ScaleTuning[MIDIKey() % 12];
        
        // GSt behaviour: maximum transpose up is 40 semitones. If
        // MIDI key is more than 40 semitones above unity note,
        // the transpose is not done.
        //
        // Update: Removed this GSt misbehavior. I don't think that any stock
        // gig sound requires it to resemble its original sound.
        // -- Christian, 2017-07-09
        if (!SmplInfo.Unpitched /* && (MIDIKey() - (int) RgnInfo.UnityNote) < 40*/)
            pitchbasecents += (MIDIKey() - (int) RgnInfo.UnityNote) * 100;
        
        pitch.PitchBase = RTMath::CentsToFreqRatioUnlimited(pitchbasecents) * (double(SmplInfo.SampleRate) / double(GetEngine()->SampleRate));
        this->Pitch = pitch;
    }

    double AbstractVoice::CalculateVolume(double velocityAttenuation) {
        // For 16 bit samples, we downscale by 32768 to convert from
        // int16 value range to DSP value range (which is
        // -1.0..1.0). For 24 bit, we downscale from int32.
        float volume = velocityAttenuation / (SmplInfo.BitDepth == 16 ? 32768.0f : 32768.0f * 65536.0f);

        volume *= GetSampleAttenuation() * pEngineChannel->GlobalVolume * GLOBAL_VOLUME;

        // the volume of release triggered samples depends on note length
        if (Type & Voice::type_release_trigger) {
            float noteLength = float(GetEngine()->FrameTime + Delay -
                GetNoteOnTime(MIDIKey()) ) / GetEngine()->SampleRate;

            volume *= GetReleaseTriggerAttenuation(noteLength);
        }

        return volume;
    }

    float AbstractVoice::GetReleaseTriggerAttenuation(float noteLength) {
        return 1 - RgnInfo.ReleaseTriggerDecay * noteLength;
    }

    void AbstractVoice::EnterReleaseStage() {
        if (pSignalUnitRack == NULL) {
            pEG1->update(EG::event_release, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
            pEG2->update(EG::event_release, GetEngine()->SampleRate / CONFIG_DEFAULT_SUBFRAGMENT_SIZE);
        } else {
            pSignalUnitRack->EnterReleaseStage();
        }
    }

    bool AbstractVoice::EG1Finished() {
        if (pSignalUnitRack == NULL) {
            return pEG1->getSegmentType() == EG::segment_end;
        } else {
            return !pSignalUnitRack->GetEndpointUnit()->Active();
        }
    }

} // namespace LinuxSampler

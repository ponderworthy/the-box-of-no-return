/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2008 Christian Schoenebeck                         *
 *   Copyright (C) 2009-2012 Christian Schoenebeck and Grigor Iliev        *
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

#ifndef __LS_ABSTRACTVOICE_H__
#define __LS_ABSTRACTVOICE_H__

#include "Voice.h"

#include "../../common/global_private.h"
#include "../AbstractEngineChannel.h"
#include "LFOBase.h"
#include "Fade.h"
#include "../EngineBase.h"
#include "EG.h"
#include "../gig/EGADSR.h"
#include "../gig/EGDecay.h"
#include "../gig/SmoothVolume.h"
#include "../gig/Synthesizer.h"
#include "../gig/Profiler.h"
#include "SignalUnitRack.h"

// include the appropriate (unsigned) triangle LFO implementation
#if CONFIG_UNSIGNED_TRIANG_ALGO == INT_MATH_SOLUTION
# include "LFOTriangleIntMath.h"
#elif CONFIG_UNSIGNED_TRIANG_ALGO == INT_ABS_MATH_SOLUTION
# include "LFOTriangleIntAbsMath.h"
#elif CONFIG_UNSIGNED_TRIANG_ALGO == DI_HARMONIC_SOLUTION
# include "LFOTriangleDiHarmonic.h"
#else
# error "Unknown or no (unsigned) triangle LFO implementation selected!"
#endif

// include the appropriate (signed) triangle LFO implementation
#if CONFIG_SIGNED_TRIANG_ALGO == INT_MATH_SOLUTION
# include "LFOTriangleIntMath.h"
#elif CONFIG_SIGNED_TRIANG_ALGO == INT_ABS_MATH_SOLUTION
# include "LFOTriangleIntAbsMath.h"
#elif CONFIG_SIGNED_TRIANG_ALGO == DI_HARMONIC_SOLUTION
# include "LFOTriangleDiHarmonic.h"
#else
# error "Unknown or no (signed) triangle LFO implementation selected!"
#endif

namespace LinuxSampler {

    #if CONFIG_UNSIGNED_TRIANG_ALGO == INT_MATH_SOLUTION
    typedef LFOTriangleIntMath<range_unsigned> LFOUnsigned;
    #elif CONFIG_UNSIGNED_TRIANG_ALGO == INT_ABS_MATH_SOLUTION
    typedef LFOTriangleIntAbsMath<range_unsigned> LFOUnsigned;
    #elif CONFIG_UNSIGNED_TRIANG_ALGO == DI_HARMONIC_SOLUTION
    typedef LFOTriangleDiHarmonic<range_unsigned> LFOUnsigned;
    #endif

    #if CONFIG_SIGNED_TRIANG_ALGO == INT_MATH_SOLUTION
    typedef LFOTriangleIntMath<range_signed> LFOSigned;
    #elif CONFIG_SIGNED_TRIANG_ALGO == INT_ABS_MATH_SOLUTION
    typedef LFOTriangleIntAbsMath<range_signed> LFOSigned;
    #elif CONFIG_SIGNED_TRIANG_ALGO == DI_HARMONIC_SOLUTION
    typedef LFOTriangleDiHarmonic<range_signed> LFOSigned;
    #endif

    class AbstractVoice : public Voice {
        public:
            type_t       Type;         ///< Voice Type (bit field, a voice may have several types)
            NoteBase*    pNote;        ///< Note this voice belongs to and was caused by.
            int          MIDIPan;      ///< the current MIDI pan value plus the value from RegionInfo

            SignalUnitRack* const pSignalUnitRack;

            AbstractVoice(SignalUnitRack* pRack);
            virtual ~AbstractVoice();

            inline bool IsActive() { return PlaybackState; }
            inline bool IsStealable() { return !itKillEvent && PlaybackState >= playback_state_ram; }

            virtual void Reset();

            virtual int Trigger (
                AbstractEngineChannel*  pEngineChannel,
                Pool<Event>::Iterator&  itNoteOnEvent,
                int                     PitchBend,
                type_t                  VoiceType,
                int                     iKeyGroup
            );

            /** Invoked when the voice is freed - gone from active to inactive. */
            virtual void VoiceFreed() { }

            virtual void Synthesize(uint Samples, sample_t* pSrc, uint Skip);

            virtual release_trigger_t GetReleaseTriggerFlags() = 0;

            uint GetSampleRate() { return GetEngine()->SampleRate; }
            
            uint8_t GetControllerValue(uint8_t Controller) {
                return (Controller > 128) ? 0 : pEngineChannel->ControllerTable[Controller];
            }

            /// Keyboard key on which this voice should listen to transitional events (i.e. note-off events to release the voice).
            inline uint8_t HostKey() const { return pNote->hostKey; }
            /// Keyboard key which the voice should use for calculating any synthesis relevant parameters (i.e. pitch).
            inline uint8_t MIDIKey() const { return pNote->cause.Param.Note.Key; }
            /// MIDI note-on velocity value which the voice should use for calculating any synthesis relevant parameters (i.e. amplitude).
            inline uint8_t MIDIVelocity() const { return pNote->cause.Param.Note.Velocity; }

            void processCCEvents(RTList<Event>::Iterator& itEvent, uint End);
            void processPitchEvent(RTList<Event>::Iterator& itEvent);
            void processResonanceEvent(RTList<Event>::Iterator& itEvent);
            void processTransitionEvents(RTList<Event>::Iterator& itEvent, uint End);
            void processGroupEvents(RTList<Event>::Iterator& itEvent, uint End);
            void UpdatePortamentoPos(Pool<Event>::Iterator& itNoteOffEvent);
            void Kill(Pool<Event>::Iterator& itKillEvent);
            void CreateEq();
            void onScaleTuningChanged();

            bool                Orphan;             ///< true if this voice is playing a sample from an instrument that is unloaded. When the voice dies, the sample (and dimension region) will be handed back to the instrument resource manager.
            playback_state_t    PlaybackState;      ///< When a sample will be triggered, it will be first played from RAM cache and after a couple of sample points it will switch to disk streaming and at the end of a disk stream we have to add null samples, so the interpolator can do it's work correctly
            Stream::reference_t DiskStreamRef;      ///< Reference / link to the disk stream

            template<class TV, class TRR, class TR, class TD, class TIM, class TI> friend class EngineBase;

        #if CONFIG_DEVMODE
        public:
        #else
        protected:
        #endif
            SampleInfo      SmplInfo;
            RegionInfo      RgnInfo;
            InstrumentInfo  InstrInfo;
            AbstractEngineChannel* pEngineChannel;

            double                      Pos;                ///< Current playback position in sample
            PitchInfo                   Pitch;
            Fade                        NotePitch;          ///< Updated by calls to built-in instrument script function change_tune() (defaults to 1.0, that is neutral).
            float                       CutoffBase;         ///< Cutoff frequency before control change, EG and LFO are applied
            float                       VolumeLeft;         ///< Left channel volume. This factor is calculated when the voice is triggered and doesn't change after that.
            float                       VolumeRight;        ///< Right channel volume. This factor is calculated when the voice is triggered and doesn't change after that.
            Fade                        NotePan[2];         ///< Updated by calls to built-in instrument script function change_pan() (defaults to 1.0, that is neutral, index 0 for left pan, index 1 for right).
            float                       NoteCutoff;         ///< Updated by calls to built-in instrument script function change_cutoff() (defaults to 1.0, that is neutral).
            float                       NoteResonance;      ///< Updated by calls to built-in instrument script function change_reso() (defaults to 1.0, that is neutral).
            gig::SmoothVolume           CrossfadeSmoother;  ///< Crossfade volume, updated by crossfade CC events
            gig::SmoothVolume           VolumeSmoother;     ///< Volume, updated by CC 7 (volume) events
            gig::SmoothVolume           PanLeftSmoother;    ///< Left channel volume, updated by CC 10 (pan) events and change_pan() real-time instrument script calls.
            gig::SmoothVolume           PanRightSmoother;   ///< Right channel volume, updated by CC 10 (pan) events and change_pan() real-time instrument script calls.
            Fade                        NoteVolume;         ///< Note's global volume, updated by change_vol() real-time instrument script calls (defaults to 1.0, that is neutral).
            bool                        DiskVoice;          ///< If the sample is very short it completely fits into the RAM cache and doesn't need to be streamed from disk, in that case this flag is set to false
            bool                        RAMLoop;            ///< If this voice has a loop defined which completely fits into the cached RAM part of the sample, in this case we handle the looping within the voice class, else if the loop is located in the disk stream part, we let the disk stream handle the looping
            unsigned long               MaxRAMPos;          ///< The upper allowed limit (not actually the end) in the RAM sample cache, after that point it's not safe to chase the interpolator another time over over the current cache position, instead we switch to disk then.
            uint                        Delay;              ///< Number of sample points the rendering process of this voice should be delayed (jitter correction), will be set to 0 after the first audio fragment cycle
            EG*                         pEG1;               ///< Envelope Generator 1 (Amplification)
            EG*                         pEG2;               ///< Envelope Generator 2 (Filter cutoff frequency)
            gig::EGDecay                EG3;                ///< Envelope Generator 3 (Pitch) TODO: use common EG instead?
            midi_ctrl                   VCFCutoffCtrl;
            midi_ctrl                   VCFResonanceCtrl;
            LFOUnsigned*                pLFO1;               ///< Low Frequency Oscillator 1 (Amplification)
            LFOUnsigned*                pLFO2;               ///< Low Frequency Oscillator 2 (Filter cutoff frequency)
            LFOSigned*                  pLFO3;               ///< Low Frequency Oscillator 3 (Pitch)
            bool                        bLFO1Enabled;        ///< Should we use the Amplitude LFO for this voice?
            bool                        bLFO2Enabled;        ///< Should we use the Filter Cutoff LFO for this voice?
            bool                        bLFO3Enabled;        ///< Should we use the Pitch LFO for this voice?
            Pool<Event>::Iterator       itTriggerEvent;      ///< First event on the key's list the voice should process (only needed for the first audio fragment in which voice was triggered, after that it will be set to NULL).
            Pool<Event>::Iterator       itKillEvent;         ///< Event which caused this voice to be killed
            int                         SynthesisMode;
            float                       fFinalCutoff;
            float                       fFinalResonance;
            gig::SynthesisParam         finalSynthesisParameters;
            gig::Loop                   loop;
            RTList<Event>*              pGroupEvents;        ///< Events directed to an exclusive group
            
            EqSupport* pEq;         ///< Used for per voice equalization
            bool       bEqSupport;
            
            void PrintEqInfo() {
                if (!bEqSupport || pEq == NULL) {
                    dmsg(1,("EQ support: no\n"));
                } else {
                    pEq->PrintInfo();
                }
            }

            virtual AbstractEngine* GetEngine() = 0;
            virtual SampleInfo      GetSampleInfo() = 0;
            virtual RegionInfo      GetRegionInfo() = 0;
            virtual InstrumentInfo  GetInstrumentInfo() = 0;

            /**
             * Most of the important members of the voice are set when the voice
             * is triggered (like pEngineChannel, pRegion, pSample, etc).
             * This method is called after these members are set and before
             * the voice is actually triggered.
             * Override this method if you need to do some additional
             * initialization which depends on these members before the voice
             * is triggered.
             */
            virtual void AboutToTrigger() { }

            virtual bool EG1Finished(); 

            /**
             * Gets the sample cache size in bytes.
             */
            virtual unsigned long GetSampleCacheSize() = 0;
            
            /**
             * Because in most cases we cache part of the sample in RAM, if the
             * offset is too big (will extend beyond the RAM cache if the cache contains
             * the beginning of the sample) we should cache in the RAM buffer not the
             * beginning of the sample but a part that starts from the sample offset point.
             * In that case the current sample position should start from zero (Pos).
             * When the offset fits into RAM buffer or the whole sample is cached
             * in RAM, Pos should contain the actual offset.
             * We don't trim the sample because it might have a defined
             * loop start point before the start point of the playback.
             */
            virtual void SetSampleStartOffset();

            /**
             * Returns the correct amplitude factor for the given \a MIDIKeyVelocity.
             * All involved parameters (VelocityResponseCurve, VelocityResponseDepth
             * and VelocityResponseCurveScaling) involved are taken into account to
             * calculate the amplitude factor. Use this method when a key was
             * triggered to get the volume with which the sample should be played
             * back.
             *
             * @param MIDIKeyVelocity  MIDI velocity value of the triggered key (between 0 and 127)
             * @returns                amplitude factor (between 0.0 and 1.0)
             */
            virtual double GetVelocityAttenuation(uint8_t MIDIKeyVelocity) = 0;

            virtual double GetSampleAttenuation() = 0;

            virtual double CalculateVolume(double velocityAttenuation);

            virtual float GetReleaseTriggerAttenuation(float noteLength);

            /**
             * Get starting crossfade volume level
             */
            virtual double CalculateCrossfadeVolume(uint8_t MIDIKeyVelocity) = 0;

            virtual MidiKeyBase* GetMidiKeyInfo(int MIDIKey) = 0;

            virtual int   OrderNewStream() = 0;

            virtual PitchInfo CalculatePitchInfo(int PitchBend);

            // TODO: cleanup the interface. The following two methods
            // are maybe not neccessary after the TriggerEG1 method
            // was added.

            /**
             * Get current value of EG1 controller.
             */
            virtual double GetEG1ControllerValue(uint8_t MIDIKeyVelocity) = 0;

            /**
             * Calculate influence of EG1 controller on EG1's parameters.
             */
            virtual EGInfo CalculateEG1ControllerInfluence(double eg1ControllerValue) = 0;

            // TODO: cleanup the interface. The velrelase and
            // velocityAttenuation parameters are perhaps too gig
            // specific.
            /**
             * Trigger the amplitude envelope generator.
             */
            virtual void TriggerEG1(const EGInfo& egInfo, double velrelease, double velocityAttenuation, uint sampleRate, uint8_t velocity) = 0;

            /**
             * Get current value of EG2 controller.
             */
            virtual double GetEG2ControllerValue(uint8_t MIDIKeyVelocity) = 0;

            /**
             * Calculate influence of EG2 controller on EG2's parameters.
             */
            virtual EGInfo CalculateEG2ControllerInfluence(double eg2ControllerValue) = 0;

            virtual void TriggerEG2(const EGInfo& egInfo, double velrelease, double velocityAttenuation, uint sampleRate, uint8_t velocity) = 0;

            virtual float CalculateCutoffBase(uint8_t MIDIKeyVelocity) = 0;
            virtual float CalculateFinalCutoff(float cutoffBase) = 0;

            virtual void InitLFO1() = 0;
            virtual void InitLFO2() = 0;
            virtual void InitLFO3() = 0;

            virtual uint8_t GetVCFCutoffCtrl() = 0;
            virtual uint8_t GetVCFResonanceCtrl() = 0;
            virtual uint8_t CrossfadeAttenuation(uint8_t& CrossfadeControllerValue) = 0;

            virtual void    GetFirstEventOnKey(uint8_t MIDIKey, RTList<Event>::Iterator& itEvent) = 0;
            virtual void    ProcessCCEvent(RTList<Event>::Iterator& itEvent) = 0;
            virtual void    ProcessChannelPressureEvent(RTList<Event>::Iterator& itEvent) = 0;
            virtual void    ProcessPolyphonicKeyPressureEvent(RTList<Event>::Iterator& itEvent) = 0;
            virtual void    ProcessCutoffEvent(RTList<Event>::Iterator& itEvent) = 0;
            virtual double  GetVelocityRelease(uint8_t MIDIKeyVelocity) = 0;

            virtual unsigned long GetNoteOnTime(int MIDIKey) = 0;

            virtual void    ProcessGroupEvent(RTList<Event>::Iterator& itEvent) = 0;
            void            EnterReleaseStage();

            virtual int     CalculatePan(uint8_t pan) = 0;
    };
} // namespace LinuxSampler

#endif  /* __LS_ABSTRACTVOICE_H__ */

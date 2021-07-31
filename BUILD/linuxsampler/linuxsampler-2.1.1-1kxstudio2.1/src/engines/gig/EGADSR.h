/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2017 Christian Schoenebeck                       *
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

#ifndef __LS_GIG_EGADSR_H__
#define __LS_GIG_EGADSR_H__

#include "../common/EG.h"

namespace LinuxSampler { namespace gig {

/**
 * ADSR Envelope Generator
 *
 * Envelope Generator with stage 'Attack', 'Attack_Hold', 'Decay_1',
 * 'Decay_2', 'Sustain' and 'Release' for modulating arbitrary synthesis
 * parameters.
 */
class EGADSR : public EG {
    public:
        EGADSR();

        /**
         * Will be called by the voice when the key / voice was triggered.
         *
         * @param PreAttack       - Preattack value for the envelope
         *                          (0 - 1000 permille)
         * @param AttackTime      - Attack time for the envelope
         *                          (0.000 - 60.000s)
         * @param HoldAttack      - if true, Decay1 will be postponed until the
         *                          sample reached the sample loop start.
         * @param Decay1Time      - Decay1 time of the sample amplitude EG
         *                          (0.000 - 60.000s)
         * @param Decay2Time      - only if !InfiniteSustain: 2nd decay stage
         *                          time of the sample amplitude EG
         *                          (0.000 - 60.000s)
         * @param InfiniteSustain - if true, instead of going into Decay2
         *                          stage, Decay1 level will be hold until note
         *                          will be released
         * @param SustainLevel    - Sustain level of the sample amplitude EG
         *                          (0 - 1000 permille)
         * @param ReleaseTIme     - Release time for the envelope
         *                          (0.000 - 60.000s)
         * @param Volume          - volume the sample will be played at
         *                          (0.0 - 1.0) - used when calculating the
         *                          exponential curve parameters.
         * @param SampleRate      - sample rate of used audio output driver
         */
        void trigger(uint PreAttack, float AttackTime, bool HoldAttack, float Decay1Time, double Decay2Time, bool InfiniteSustain, uint SustainLevel, float ReleaseTime, float Volume, uint SampleRate); //FIXME: we should better use 'float' for SampleRate

        void setStateOptions(bool AttackCancel, bool AttackHoldCancel, bool Decay1Cancel, bool Decay2Cancel, bool ReleaseCancel);

        /**
         * Should be called to inform the EG about an external event and
         * also whenever an envelope stage is completed. This will handle
         * the envelope's transition to the respective next stage.
         *
         * @param Event        - what happened
         */
        void update(event_t Event, uint SampleRate);

    private:

        enum stage_t {
            stage_attack,
            stage_attack_hold,
            stage_decay1_part1,
            stage_decay1_part2,
            stage_decay2,
            stage_sustain,
            stage_release_part1,
            stage_release_part2,
            stage_fadeout,
            stage_end
        };

        stage_t   Stage;
        event_t   PostponedEvent;   ///< Only if *Cancel variable below is set to to false in the respective EG stage: holds the transition event type for processing after that current stage completed its full duration.
        bool      HoldAttack;
        bool      InfiniteSustain;
        bool      AttackCancel;     ///< Whether the "attack" stage is cancelled when receiving a note-off.
        bool      AttackHoldCancel; ///< Whether the "attack hold" stage is cancelled when receiving a note-off.
        bool      Decay1Cancel;     ///< Whether the "decay 1" stage is cancelled when receiving a note-off.
        bool      Decay2Cancel;     ///< Whether the "decay 2" stage is cancelled when receiving a note-off.
        bool      ReleaseCancel;    ///< Whether the "release" stage is cancelled when receiving a note-on.
        float     Decay1Time;
        float     Decay1Level2;
        float     Decay1Slope;
        float     Decay2Time;
        float     SustainLevel;
        float     ReleaseCoeff;
        float     ReleaseCoeff2;
        float     ReleaseCoeff3;
        float     ReleaseLevel2;
        float     ReleaseSlope;
        float     invVolume;
        float     ExpOffset;

        void enterNextStageForReleaseEvent(uint SampleRate);
        void enterAttackStage(const uint PreAttack, const float AttackTime, const uint SampleRate);
        void enterAttackHoldStage();
        void enterDecay1Part1Stage(const uint SampleRate);
        void enterDecay1Part2Stage(const uint SampleRate);
        void enterDecay2Stage(const uint SampleRate);
        void enterSustainStage();
        void enterReleasePart1Stage();
        void enterReleasePart2Stage();
};

}} // namespace LinuxSampler::gig

#endif // __LS_GIG_EGADSR_H__

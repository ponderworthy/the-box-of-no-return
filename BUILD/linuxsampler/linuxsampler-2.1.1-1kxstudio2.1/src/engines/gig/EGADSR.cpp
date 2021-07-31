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

#include "EGADSR.h"

#include "../../common/global_private.h"

namespace LinuxSampler { namespace gig {

    EGADSR::EGADSR() : EG() {
        AttackCancel     = true;
        AttackHoldCancel = true;
        Decay1Cancel     = true;
        Decay2Cancel     = true;
        ReleaseCancel    = true;
    }

    #define isTransitionEvent(type) \
        ( type == event_release || type == event_cancel_release )

    void EGADSR::update(event_t Event, uint SampleRate) {
        if (atEnd(Event)) return;

        if (Event == event_hold_end) HoldAttack = false;

        switch (Stage) {
            case stage_attack:
                switch (Event) {
                    case event_release:
                        if (AttackCancel)
                            enterNextStageForReleaseEvent(SampleRate);
                        else
                            PostponedEvent = Event;
                        break;
                    case event_stage_end:
                        if (PostponedEvent == event_release)
                            enterNextStageForReleaseEvent(SampleRate);
                        else if (HoldAttack)
                            enterAttackHoldStage();
                        else
                            enterDecay1Part1Stage(SampleRate);
                        break;
                    default: ; // noop
                }
                break;
            case stage_attack_hold:
                switch (Event) {
                    case event_stage_end: {// just refresh time
                        const int intMax = (unsigned int) -1 >> 1;
                        StepsLeft = intMax; // we use the highest possible value
                        break;
                    }
                    case event_hold_end:
                        if (PostponedEvent == event_release)
                            enterNextStageForReleaseEvent(SampleRate);
                        else
                            enterDecay1Part1Stage(SampleRate);
                        break;
                    case event_release:
                        if (AttackHoldCancel)
                            enterNextStageForReleaseEvent(SampleRate);
                        else
                            PostponedEvent = Event;
                        break;
                    default: ; // noop
                }
                break;
            case stage_decay1_part1:
                switch (Event) {
                    case event_stage_end:
                        enterDecay1Part2Stage(SampleRate);
                        break;
                    case event_release:
                        if (Decay1Cancel)
                            enterNextStageForReleaseEvent(SampleRate);
                        else
                            PostponedEvent = Event;
                        break;
                    default: ; // noop
                }
                break;
            case stage_decay1_part2:
                switch (Event) {
                    case event_release:
                        if (Decay1Cancel)
                            enterNextStageForReleaseEvent(SampleRate);
                        else
                            PostponedEvent = Event;
                        break;
                    case event_stage_end:
                        if (Level < CONFIG_EG_BOTTOM)
                            enterEndStage();
                        else if (PostponedEvent == event_release)
                            enterNextStageForReleaseEvent(SampleRate);
                        else if (InfiniteSustain)
                            enterSustainStage();
                        else
                            enterDecay2Stage(SampleRate);
                        break;
                    default: ; // noop
                }
                break;
            case stage_decay2:
                switch (Event) {
                    case event_stage_end:
                        enterFadeOutStage();
                        break;
                    case event_release:
                        if (Decay2Cancel)
                            enterReleasePart1Stage();
                        else
                            PostponedEvent = Event;
                        break;
                    case event_hold_end:
                        if (PostponedEvent == event_release && Decay1Cancel)
                            enterReleasePart1Stage();
                        else
                            enterDecay1Part1Stage(SampleRate);
                        break;
                    default: ; // noop
                }
                break;
            case stage_sustain:
                switch (Event) {
                    case event_stage_end: {// just refresh time
                        const int intMax = (unsigned int) -1 >> 1;
                        StepsLeft = intMax; // we use the highest possible value
                        break;
                    }
                    case event_release:
                        enterReleasePart1Stage();
                        break;
                    case event_hold_end:
                        enterDecay1Part1Stage(SampleRate);
                        break;
                    default: ; // noop
                }
                break;
            case stage_release_part1:
                switch (Event) {
                    case event_stage_end:
                        enterReleasePart2Stage();
                        break;
                    case event_cancel_release:
                        if (!ReleaseCancel)
                            break;
                        if (InfiniteSustain)
                            enterSustainStage();
                        else
                            enterDecay2Stage(SampleRate);
                        break;
                    default: ; // noop
                }
                break;
            case stage_release_part2:
                switch (Event) {
                    case event_stage_end:
                        enterFadeOutStage();
                        break;
                    case event_cancel_release:
                        if (!ReleaseCancel)
                            break;
                        if (InfiniteSustain)
                            enterSustainStage();
                        else
                            enterDecay2Stage(SampleRate);
                        break;
                    default: ; // noop
                }
                break;
            case stage_fadeout:
            case stage_end:
                ; // noop
        }
    }

    void EGADSR::trigger(uint PreAttack, float AttackTime, bool HoldAttack, float Decay1Time, double Decay2Time, bool InfiniteSustain, uint SustainLevel, float ReleaseTime, float Volume, uint SampleRate) {
        this->SustainLevel     = SustainLevel / 1000.0;
        this->InfiniteSustain  = InfiniteSustain;
        this->HoldAttack       = HoldAttack;

        this->Decay1Time = Decay1Time;
        this->Decay2Time = Decay2Time;

        invVolume = 1 / Volume;
        ExpOffset = (0.25 - 1 / 3.55) * invVolume;

        // calculate release stage parameters (lin+exp curve)
        if (ReleaseTime < CONFIG_EG_MIN_RELEASE_TIME) ReleaseTime = CONFIG_EG_MIN_RELEASE_TIME;  // to avoid click sounds at the end of the sample playback
        const float ReleaseStepsLeft = (long) (ReleaseTime * SampleRate);
        ReleaseSlope  = 1.365 * (0 - 1) / ReleaseStepsLeft;
        ReleaseCoeff  = ReleaseSlope * invVolume;
        ReleaseSlope  *= 3.55;
        ReleaseCoeff2 = exp(ReleaseSlope);
        ReleaseCoeff3 = ExpOffset * (1 - ReleaseCoeff2);
        ReleaseLevel2 = 0.25 * invVolume;

        PostponedEvent = (event_t) -1; // init with anything except release or cancel_release

        enterFirstStage();
        enterAttackStage(PreAttack, AttackTime, SampleRate);
    }

    void EGADSR::setStateOptions(bool AttackCancel, bool AttackHoldCancel, bool Decay1Cancel, bool Decay2Cancel, bool ReleaseCancel) {
        this->AttackCancel     = AttackCancel;
        this->AttackHoldCancel = AttackHoldCancel;
        this->Decay1Cancel     = Decay1Cancel;
        this->Decay2Cancel     = Decay2Cancel;
        this->ReleaseCancel    = ReleaseCancel;
    }

    void EGADSR::enterNextStageForReleaseEvent(uint SampleRate) {
        switch (Stage) {
            case stage_attack:
                if (HoldAttack && !AttackHoldCancel) {
                    enterAttackHoldStage();
                    return;
                }
            case stage_attack_hold:
                if (!Decay1Cancel) {
                    enterDecay1Part1Stage(SampleRate);
                    return;
                }
            case stage_decay1_part1:
            case stage_decay1_part2:
                if (InfiniteSustain) {
                    enterReleasePart1Stage();
                    return;
                } else if (!Decay2Cancel) {
                    enterDecay2Stage(SampleRate);
                    return;
                }
            default:
                enterReleasePart1Stage();
        }
    }

    void EGADSR::enterAttackStage(const uint PreAttack, const float AttackTime, const uint SampleRate) {
        Stage   = stage_attack;
        Segment = segment_lin;

        if (AttackTime >= 1e-8) {
            // Measurements of GSt output shows that the real attack time
            // is about 65.5% of the value specified in the gig file.
            // The minimum attack value used is 0.0316.
            StepsLeft = int(0.655f * RTMath::Max(AttackTime, 0.0316f) * SampleRate);
            Level = (float) PreAttack / 1000.0;
            Coeff = 0.896f * (1.0f - Level) / StepsLeft; // max level is a bit lower if attack != 0
        } else { // attack is zero - immediately jump to the next stage
            Level = 1.029f; // a bit higher than max sustain
            if (HoldAttack) enterAttackHoldStage();
            else            enterDecay1Part1Stage(SampleRate);
        }
    }

    void EGADSR::enterAttackHoldStage() {
        Stage     = stage_attack_hold;
        Segment   = segment_lin;
        Coeff     = 0.0f; // don't rise anymore
        const int intMax = (unsigned int) -1 >> 1;
        StepsLeft = intMax; // we use the highest value possible (we refresh StepsLeft in update() in case)
    }

    void EGADSR::enterDecay1Part1Stage(const uint SampleRate) {
        // The decay1 and release stage both consist of two parts,
        // first a linear curve, f, followed by an exponential curve,
        // g:
        //
        // f(x + d) = f(x) + Coeff
        // g(x + d) = Coeff2 * g(x) + Coeff3
        //
        // (where d is 1/SampleRate). The transition from f to g is
        // done when f(x) has reached Level2 = 25% of full volume.
        StepsLeft = (int) (Decay1Time * SampleRate);
        if (StepsLeft && Level > SustainLevel) {
            Stage        = stage_decay1_part1;
            Segment      = segment_lin;
            Decay1Slope = (1.347f * SustainLevel - 1.361f) / StepsLeft;
            Coeff        = Decay1Slope * invVolume;
            Decay1Level2 = 0.25 * invVolume;
            StepsLeft = int((RTMath::Max(Decay1Level2, SustainLevel) - Level) / Coeff);
            if (StepsLeft <= 0) enterDecay1Part2Stage(SampleRate);
        } else {
            if (PostponedEvent == event_release) {
                Stage = stage_decay1_part2; // pretend decay 1 part 2 was completed
                enterNextStageForReleaseEvent(SampleRate);
            } else if (InfiniteSustain) {
                enterSustainStage();
            } else {
                enterDecay2Stage(SampleRate);
            }
        }
    }

    void EGADSR::enterDecay1Part2Stage(const uint SampleRate) {
        if (SustainLevel < Decay1Level2) {
            Stage   = stage_decay1_part2;
            Segment = segment_exp;
            Decay1Slope *= 3.55;
            Coeff  = exp(Decay1Slope);
            Offset = ExpOffset * (1 - Coeff);
            StepsLeft = int(log((SustainLevel - ExpOffset) / (Level - ExpOffset)) / Decay1Slope);
            if (StepsLeft > 0) return;
        }
        if (PostponedEvent == event_release) {
            Stage = stage_decay1_part2;
            enterNextStageForReleaseEvent(SampleRate);
        } else if (InfiniteSustain) {
            enterSustainStage();
        } else {
            enterDecay2Stage(SampleRate);
        }
    }

    void EGADSR::enterDecay2Stage(const uint SampleRate) {
        Stage      = stage_decay2;
        Segment    = segment_lin;
        Decay2Time = RTMath::Max(Decay2Time, 0.05f);
        StepsLeft  = (int) (Decay2Time * SampleRate);
        Coeff      = (-1.03 / StepsLeft) * invVolume;
        //FIXME: do we really have to calculate 'StepsLeft' two times?
        StepsLeft  = int((CONFIG_EG_BOTTOM - Level) / Coeff);
        if (StepsLeft <= 0) enterEndStage();
    }

    void EGADSR::enterSustainStage() {
        Stage   = stage_sustain;
        Segment = segment_lin;
        Coeff   = 0.0f; // don't change the envelope level in this stage
        const int intMax = (unsigned int) -1 >> 1;
        StepsLeft = intMax; // we use the highest value possible (we refresh StepsLeft in update() in case)
        PostponedEvent = (event_t) -1; // reset with anything except release or cancel_release
    }

    void EGADSR::enterReleasePart1Stage() {
        Stage     = stage_release_part1;
        Segment   = segment_lin;
        StepsLeft = int((ReleaseLevel2 - Level) / ReleaseCoeff);
        Coeff     = ReleaseCoeff;
        PostponedEvent = (event_t) -1; // reset with anything except release or cancel_release
        if (StepsLeft <= 0) enterReleasePart2Stage();
    }

    void EGADSR::enterReleasePart2Stage() {
        Stage     = stage_release_part2;
        Segment   = segment_exp;
        StepsLeft = int(log((CONFIG_EG_BOTTOM - ExpOffset) / (Level - ExpOffset)) / ReleaseSlope);
        Coeff     = ReleaseCoeff2;
        Offset    = ReleaseCoeff3;
        if (StepsLeft <= 0) enterFadeOutStage();
    }

}} // namespace LinuxSampler::gig

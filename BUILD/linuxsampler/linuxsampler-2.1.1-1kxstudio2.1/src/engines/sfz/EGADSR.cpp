/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2012 Christian Schoenebeck                       *
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
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,                *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#include "EGADSR.h"

#include "../../common/global_private.h"

namespace LinuxSampler { namespace sfz {

    // Modeled after output from SFZ Player 1.97 / Rapture Demo 1.0.

    void EGADSR::update(event_t Event, uint SampleRate) {
        if (atEnd(Event)) return;

        switch (Stage) {
            case stage_attack:
                switch (Event) {
                    case event_release:
                        enterReleaseStage();
                        break;
                    case event_stage_end:
                        if (HoldSteps)
                            enterAttackHoldStage();
                        else
                            enterDecayStage(SampleRate);
                        break;
                    default: ; // noop
                }
                break;
            case stage_attack_hold:
                switch (Event) {
                    case event_stage_end:
                        enterDecayStage(SampleRate);
                        break;
                    case event_release:
                        enterReleaseStage();
                        break;
                    default: ; // noop
                }
                break;
            case stage_decay:
                switch (Event) {
                    case event_release:
                        enterReleaseStage();
                        break;
                    case event_stage_end:
                        if (Level < CONFIG_EG_BOTTOM)
                            enterEndStage();
                        else
                            enterSustainStage();
                        break;
                    default: ; // noop
                }
                break;
            case stage_sustain:
                switch (Event) {
                    case event_stage_end: { // just refresh time
                        const int intMax = (unsigned int) -1 >> 1;
                        StepsLeft = intMax; // we use the highest possible value
                        break;
                    }
                    case event_release:
                        enterReleaseStage();
                        break;
                    default: ; // noop
                }
                break;
            case stage_release:
                switch (Event) {
                    case event_stage_end:
                        enterFadeOutStage();
                        break;
                    case event_cancel_release:
                        enterSustainStage();
                        break;
                    default: ; // noop
                }
                break;

            case stage_fadeout:
            case stage_end:
                ; // noop
        }
    }

    void EGADSR::trigger(uint PreAttack, float AttackTime, float HoldTime, float DecayTime, uint SustainLevel, float ReleaseTime, uint SampleRate, bool LinearRelease) {
        this->SustainLevel = SustainLevel / 1000.0;
        this->HoldSteps    = int(HoldTime * SampleRate);
        this->DecayTime    = DecayTime;
        this->LinearRelease = LinearRelease;

        if (ReleaseTime < CONFIG_EG_MIN_RELEASE_TIME) ReleaseTime = CONFIG_EG_MIN_RELEASE_TIME;  // to avoid click sounds at the end of the sample playback
        this->ReleaseTime = ReleaseTime * SampleRate;
        Offset = 0;
        enterFirstStage();
        enterAttackStage(PreAttack, AttackTime, SampleRate);
    }

    void EGADSR::enterAttackStage(const uint PreAttack, const float AttackTime, const uint SampleRate) {
        Stage   = stage_attack;
        Segment = segment_lin;

        if (AttackTime >= 0.0005f) {
            StepsLeft = int(AttackTime * SampleRate);
            Level = (float) PreAttack / 1000.0;
            Coeff = (1.0f - Level) / StepsLeft;
        } else { // attack is zero - immediately jump to the next stage
            Level = 1.0f;
            if (HoldSteps) enterAttackHoldStage();
            else           enterDecayStage(SampleRate);
        }
    }

    void EGADSR::enterAttackHoldStage() {
        Stage     = stage_attack_hold;
        Segment   = segment_lin;
        Coeff     = 0.0f; // don't rise anymore
        StepsLeft = HoldSteps;
    }

    void EGADSR::enterDecayStage(const uint SampleRate) {
        if (LinearRelease) {
            StepsLeft = int(DecayTime * SampleRate * (Level - SustainLevel));
            if (StepsLeft > 0) {
                Stage = stage_decay;
                Segment = segment_lin;
                Coeff = -1 / (DecayTime * SampleRate);
                return;
            }
        } else {
            StepsLeft = int(DecayTime * SampleRate);
            if (StepsLeft && Level > SustainLevel) {
                Stage   = stage_decay;
                Segment = segment_exp;
                const float slope = -9.226 / StepsLeft;
                Coeff  = exp(slope);
                StepsLeft = int(log(std::max(SustainLevel, float(CONFIG_EG_BOTTOM)) / Level) / slope);
                if (StepsLeft > 0) return;
            }
        }
        Level = SustainLevel;
        enterSustainStage();
    }

    void EGADSR::enterSustainStage() {
        Stage   = stage_sustain;
        Segment = segment_lin;
        Coeff   = 0.0f; // don't change the envelope level in this stage
        const int intMax = (unsigned int) -1 >> 1;
        StepsLeft = intMax; // we use the highest value possible (we refresh StepsLeft in update() in case)
    }

    void EGADSR::enterReleaseStage() {
        Stage     = stage_release;
        if (LinearRelease) {
            Segment   = segment_lin;
            StepsLeft = int(Level * ReleaseTime);
            Coeff = -1 / ReleaseTime;
        } else {
            Segment   = segment_exp;
            const float slope = -9.226 / ReleaseTime;
            StepsLeft = int(log(CONFIG_EG_BOTTOM / Level) / slope);
            Coeff     = exp(slope);
        }
        if (StepsLeft <= 0) enterFadeOutStage();
    }

}} // namespace LinuxSampler::sfz

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

#ifndef LS_SFZ_EGADSR_H
#define LS_SFZ_EGADSR_H

#include "../common/EG.h"

namespace LinuxSampler { namespace sfz {

/**
 * ADSR Envelope Generator
 *
 * SFZ v1 envelope generator with 'Attack', 'Hold', 'Decay', 'Sustain'
 * and 'Release' stages for modulating arbitrary synthesis parameters.
 */
class EGADSR : public ::LinuxSampler::EG {
    public:

        /**
         * Will be called by the voice when the key / voice was triggered.
         *
         * @param PreAttack       - Preattack value for the envelope
         *                          (0 - 1000 permille)
         * @param AttackTime      - Attack time for the envelope
         * @param HoldTime        - Hold time for the envelope
         * @param DecayTime       - Decay1 time of the sample amplitude EG
         * @param SustainLevel    - Sustain level of the sample amplitude EG
         *                          (0 - 1000 permille)
         * @param ReleaseTIme     - Release time for the envelope
         * @param SampleRate      - sample rate of used audio output driver
         * @param LinearRelease   - true if decay and release is linear
         */
    void trigger(uint PreAttack, float AttackTime, float HoldTime, float DecayTime, uint SustainLevel, float ReleaseTime, uint SampleRate, bool LinearRelease); //FIXME: we should better use 'float' for SampleRate

        /**
         * Should be called to inform the EG about an external event and
         * also whenever an envelope stage is completed. This will handle
         * the envelope's transition to the respective next stage.
         *
         * @param Event        - what happened
         * @param SampleRate   - sample rate of used audio output driver
         */
        void update(event_t Event, uint SampleRate);

    private:

        enum stage_t {
            stage_attack,
            stage_attack_hold,
            stage_decay,
            stage_sustain,
            stage_release,
            stage_fadeout,
            stage_end
        };

        stage_t   Stage;
        int       HoldSteps;
        float     DecayTime;
        float     SustainLevel;
        float     ReleaseTime;
        bool      LinearRelease;

        void enterAttackStage(const uint PreAttack, const float AttackTime, const uint SampleRate);
        void enterAttackHoldStage();
        void enterDecayStage(const uint SampleRate);
        void enterSustainStage();
        void enterReleaseStage();
};

}} // namespace LinuxSampler::sfz

#endif // LS_SFZ_EGADSR_H

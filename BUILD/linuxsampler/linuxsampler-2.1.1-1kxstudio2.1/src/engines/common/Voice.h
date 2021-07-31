/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003,2004 by Benno Senoner and Christian Schoenebeck    *
 *   Copyright (C) 2005-2011 Christian Schoenebeck                         *
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

#ifndef __LS_VOICE_H__
#define __LS_VOICE_H__

#include "Event.h"
#include "../gig/Filter.h"
#include "../../common/Pool.h"

// TODO: remove gig dependency
#if AC_APPLE_UNIVERSAL_BUILD
# include <libgig/gig.h>
#else
# include <gig.h>
#endif

namespace LinuxSampler {

    class Voice {
        public:
            enum playback_state_t {
                playback_state_end  = 0,
                playback_state_init = 1,
                playback_state_ram  = 2,
                playback_state_disk = 3
            };

            // Type bits. Mostly mutual exclusive, but a voice may both be of one shot type and require a release trigger at the same time.
            enum type_t {
                type_normal = 0,
                type_release_trigger_required = 1 << 1,  ///< If the key of this voice will be released, it causes a release triggered voice to be spawned
                type_release_trigger = 1 << 2,           ///< Release triggered voice which cannot be killed by releasing its key
                type_one_shot = 1 << 3,                  ///< Voice should not be released by note off
                type_controller_triggered = 1 << 4       ///< Voice is triggered by MIDI CC instead of a note on
            };

            struct PitchInfo {
                float PitchBase;      ///< Basic pitch depth, stays the same for the whole life time of the voice
                float PitchBend;      ///< Current pitch value of the pitchbend wheel
                float PitchBendRange; ///< The pitch range of the pitchbend wheel, value is in cents / 8192
            };

            struct EGInfo {
                double Attack;
                double Decay;
                double Release;
            };

            struct SampleInfo {
                uint SampleRate;
                uint ChannelCount;
                uint FrameSize;
                uint TotalFrameCount;
                uint BitDepth;
                bool HasLoops;
                uint LoopStart;
                uint LoopLength;
                uint LoopPlayCount; ///< Number of times the loop should be played (a value of 0 = infinite).
                bool Unpitched; ///< sound which is not characterized by a perceived frequency
            };

            struct RegionInfo {
                uint8_t UnityNote; ///< The MIDI key number of the recorded pitch of the sample
                int16_t FineTune;
                int     Pan; ///< Panorama / Balance (-64..0..63 <-> left..middle..right)
                uint    SampleStartOffset; ///< Number of samples the sample start should be moved

                double  EG2PreAttack; ///< Preattack value of the filter cutoff EG (in permilles)
                double  EG2Attack; ///< Attack time of the filter cutoff EG (in seconds)
                double  EG2Decay1;  ///< Decay time of the filter cutoff EG (in seconds)
                double  EG2Decay2;  ///< Only if (EG2InfiniteSustain == false): 2nd decay stage time of the filter cutoff EG (in seconds)
                double  EG2Sustain; ///< Sustain value of the filter cutoff EG (in permilles)
                bool    EG2InfiniteSustain; ///< If true, instead of going into Decay2 phase, Decay1 level will be hold until note will be released.
                double  EG2Release; ///< Release time of the filter cutoff EG (in seconds)

                double  EG3Attack;  ///< Attack time of the sample pitch EG (in seconds)
                int     EG3Depth;   ///< Depth of the sample pitch EG (-1200 - +1200)
                double  ReleaseTriggerDecay; ///< How much release sample volume depends on note length. Release sample amplitude is multiplied with (1 - ReleaseTriggerDecay * note length).

                bool               VCFEnabled;    ///< If filter should be used.
                Filter::vcf_type_t VCFType;       ///< Defines the general filter characteristic (lowpass, highpass, bandpass, etc.).
                uint8_t            VCFResonance;  ///< Firm internal filter resonance weight.
            };

            struct InstrumentInfo {
                int   FineTune; // in cents
                uint  PitchbendRange;    ///< Number of semitones pitchbend controller can pitch
            };

            /// Reflects a MIDI controller
            struct midi_ctrl {
                uint8_t controller; ///< MIDI control change controller number
                uint8_t value;      ///< Current MIDI controller value
                float   fvalue;     ///< Transformed / effective value (e.g. volume level or filter cutoff frequency)
            };

            Voice() { }
            virtual ~Voice() { }
    };

    // |= operator for the type_t enum
    inline Voice::type_t operator|=(Voice::type_t& lhs, Voice::type_t rhs) {
        return lhs = static_cast<Voice::type_t>(static_cast<unsigned>(lhs) | static_cast<unsigned>(rhs));
    }

} // namespace LinuxSampler

#endif  /* __LS_VOICE_H__ */

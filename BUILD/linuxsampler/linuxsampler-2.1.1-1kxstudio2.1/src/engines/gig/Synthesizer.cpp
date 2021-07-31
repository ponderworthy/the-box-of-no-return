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
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "Synthesizer.h"

#define SYNTHESIZE(CHAN,LOOP,FILTER,INTERPOLATE,BITDEPTH24)                           \
        Synthesizer<CHAN,LOOP,FILTER,INTERPOLATE,BITDEPTH24>::SynthesizeSubFragment(  \
        pFinalParam, pLoop)

namespace LinuxSampler { namespace gig {

    void SynthesizeFragment_mode00(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,0,0,0,0);
    }

    void SynthesizeFragment_mode01(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,0,0,1,0);
    }

    void SynthesizeFragment_mode02(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,0,1,0,0);
    }

    void SynthesizeFragment_mode03(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,0,1,1,0);
    }

    void SynthesizeFragment_mode04(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,1,0,0,0);
    }

    void SynthesizeFragment_mode05(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,1,0,1,0);
    }

    void SynthesizeFragment_mode06(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,1,1,0,0);
    }

    void SynthesizeFragment_mode07(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,1,1,1,0);
    }

    void SynthesizeFragment_mode08(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,0,0,0,0);
    }

    void SynthesizeFragment_mode09(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,0,0,1,0);
    }

    void SynthesizeFragment_mode0a(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,0,1,0,0);
    }

    void SynthesizeFragment_mode0b(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,0,1,1,0);
    }

    void SynthesizeFragment_mode0c(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,1,0,0,0);
    }

    void SynthesizeFragment_mode0d(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,1,0,1,0);
    }

    void SynthesizeFragment_mode0e(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,1,1,0,0);
    }

    void SynthesizeFragment_mode0f(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,1,1,1,0);
    }

    void SynthesizeFragment_mode10(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,0,0,0,1);
    }

    void SynthesizeFragment_mode11(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,0,0,1,1);
    }

    void SynthesizeFragment_mode12(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,0,1,0,1);
    }

    void SynthesizeFragment_mode13(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,0,1,1,1);
    }

    void SynthesizeFragment_mode14(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,1,0,0,1);
    }

    void SynthesizeFragment_mode15(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,1,0,1,1);
    }

    void SynthesizeFragment_mode16(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,1,1,0,1);
    }

    void SynthesizeFragment_mode17(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(MONO,1,1,1,1);
    }

    void SynthesizeFragment_mode18(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,0,0,0,1);
    }

    void SynthesizeFragment_mode19(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,0,0,1,1);
    }

    void SynthesizeFragment_mode1a(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,0,1,0,1);
    }

    void SynthesizeFragment_mode1b(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,0,1,1,1);
    }

    void SynthesizeFragment_mode1c(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,1,0,0,1);
    }

    void SynthesizeFragment_mode1d(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,1,0,1,1);
    }

    void SynthesizeFragment_mode1e(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,1,1,0,1);
    }

    void SynthesizeFragment_mode1f(SynthesisParam* pFinalParam, Loop* pLoop) {
        SYNTHESIZE(STEREO,1,1,1,1);
    }

    void* GetSynthesisFunction(int SynthesisMode) {
        // Mode Bits: (PROF),(IMPL),24BIT,CHAN,LOOP,FILT,INTERP
        switch (SynthesisMode) {
            case 0x00: return (void*) SynthesizeFragment_mode00;
            case 0x01: return (void*) SynthesizeFragment_mode01;
            case 0x02: return (void*) SynthesizeFragment_mode02;
            case 0x03: return (void*) SynthesizeFragment_mode03;
            case 0x04: return (void*) SynthesizeFragment_mode04;
            case 0x05: return (void*) SynthesizeFragment_mode05;
            case 0x06: return (void*) SynthesizeFragment_mode06;
            case 0x07: return (void*) SynthesizeFragment_mode07;
            case 0x08: return (void*) SynthesizeFragment_mode08;
            case 0x09: return (void*) SynthesizeFragment_mode09;
            case 0x0a: return (void*) SynthesizeFragment_mode0a;
            case 0x0b: return (void*) SynthesizeFragment_mode0b;
            case 0x0c: return (void*) SynthesizeFragment_mode0c;
            case 0x0d: return (void*) SynthesizeFragment_mode0d;
            case 0x0e: return (void*) SynthesizeFragment_mode0e;
            case 0x0f: return (void*) SynthesizeFragment_mode0f;
            case 0x10: return (void*) SynthesizeFragment_mode10;
            case 0x11: return (void*) SynthesizeFragment_mode11;
            case 0x12: return (void*) SynthesizeFragment_mode12;
            case 0x13: return (void*) SynthesizeFragment_mode13;
            case 0x14: return (void*) SynthesizeFragment_mode14;
            case 0x15: return (void*) SynthesizeFragment_mode15;
            case 0x16: return (void*) SynthesizeFragment_mode16;
            case 0x17: return (void*) SynthesizeFragment_mode17;
            case 0x18: return (void*) SynthesizeFragment_mode18;
            case 0x19: return (void*) SynthesizeFragment_mode19;
            case 0x1a: return (void*) SynthesizeFragment_mode1a;
            case 0x1b: return (void*) SynthesizeFragment_mode1b;
            case 0x1c: return (void*) SynthesizeFragment_mode1c;
            case 0x1d: return (void*) SynthesizeFragment_mode1d;
            case 0x1e: return (void*) SynthesizeFragment_mode1e;
            case 0x1f: return (void*) SynthesizeFragment_mode1f;
            default: {
                std::cerr << "gig::Synthesizer: Invalid Synthesis Mode: " << SynthesisMode << std::endl << std::flush;
                exit(-1);
            }
        }
    }

    void RunSynthesisFunction(const int SynthesisMode, SynthesisParam* pFinalParam, Loop* pLoop) {
        SynthesizeFragment_Fn* f = (SynthesizeFragment_Fn*) GetSynthesisFunction(SynthesisMode);
        f(pFinalParam, pLoop);
    }

}} // namespace LinuxSampler::gig

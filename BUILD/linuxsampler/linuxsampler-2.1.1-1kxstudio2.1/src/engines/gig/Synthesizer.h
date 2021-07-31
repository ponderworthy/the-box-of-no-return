/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2016 Christian Schoenebeck                       *
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

#ifndef __LS_GIG_SYNTHESIZER_H__
#define __LS_GIG_SYNTHESIZER_H__

#include "../../common/global_private.h"
#include "../../common/RTMath.h"
#include "../common/Resampler.h"
#include "Filter.h"
#include "SynthesisParam.h"

#define SYNTHESIS_MODE_SET_INTERPOLATE(iMode,bVal)      { if (bVal) iMode |= 0x01; else iMode &= ~0x01; }  /* (un)set mode bit 0 */
#define SYNTHESIS_MODE_SET_FILTER(iMode,bVal)           { if (bVal) iMode |= 0x02; else iMode &= ~0x02; }   /* (un)set mode bit 1 */
#define SYNTHESIS_MODE_SET_LOOP(iMode,bVal)             { if (bVal) iMode |= 0x04; else iMode &= ~0x04; }   /* (un)set mode bit 2 */
#define SYNTHESIS_MODE_SET_CHANNELS(iMode,bVal)         { if (bVal) iMode |= 0x08; else iMode &= ~0x08; }   /* (un)set mode bit 3 */
#define SYNTHESIS_MODE_SET_BITDEPTH24(iMode,bVal)       { if (bVal) iMode |= 0x10; else iMode &= ~0x10; }   /* (un)set mode bit 4 */
//TODO: the Asm implementation mode is currently not implemented anymore, since Asm synthesis code is currently broken!
#define SYNTHESIS_MODE_SET_IMPLEMENTATION(iMode,bVal)   { if (bVal) iMode |= 0x20; else iMode &= ~0x20; }   /* (un)set mode bit 5 */
//TODO: the profiling mode is currently not implemented anymore!
#define SYNTHESIS_MODE_SET_PROFILING(iMode,bVal)        { if (bVal) iMode |= 0x40; else iMode &= ~0x40; }   /* (un)set mode bit 6 */

#define SYNTHESIS_MODE_GET_INTERPOLATE(iMode)           (iMode & 0x01)
#define SYNTHESIS_MODE_GET_FILTER(iMode)                (iMode & 0x02)
#define SYNTHESIS_MODE_GET_LOOP(iMode)                  (iMode & 0x04)
#define SYNTHESIS_MODE_GET_CHANNELS(iMode)              (iMode & 0x08)
#define SYNTHESIS_MODE_GET_BITDEPTH24(iMode)            (iMode & 0x10)
#define SYNTHESIS_MODE_GET_IMPLEMENTATION(iMode)        (iMode & 0x20)


namespace LinuxSampler { namespace gig {

    typedef void SynthesizeFragment_Fn(SynthesisParam* pFinalParam, Loop* pLoop);

    void* GetSynthesisFunction(const int SynthesisMode);
    void RunSynthesisFunction(const int SynthesisMode, SynthesisParam* pFinalParam, Loop* pLoop);

    enum channels_t {
        MONO,
        STEREO
    };

    /** @brief Main Synthesis algorithms for the gig::Engine
     *
     * Implementation of the main synthesis algorithms of the Gigasampler
     * format capable sampler engine. This means resampling / interpolation
     * for pitching the audio signal, looping, filter and amplification.
     */
    template<channels_t CHANNELS, bool DOLOOP, bool USEFILTER, bool INTERPOLATE, bool BITDEPTH24>
    class Synthesizer : public __RTMath<CPP>, public LinuxSampler::Resampler<INTERPOLATE,BITDEPTH24> {

            // declarations of derived functions (see "Name lookup,
            // templates, and accessing members of base classes" in
            // the gcc manual for an explanation of why this is
            // needed).
            //using LinuxSampler::Resampler<INTERPOLATE>::GetNextSampleMonoCPP;
            //using LinuxSampler::Resampler<INTERPOLATE>::GetNextSampleStereoCPP;
            using LinuxSampler::Resampler<INTERPOLATE,BITDEPTH24>::Interpolate1StepMonoCPP;
            using LinuxSampler::Resampler<INTERPOLATE,BITDEPTH24>::Interpolate1StepStereoCPP;

        public:
        //protected:

            static void SynthesizeSubFragment(SynthesisParam* pFinalParam, Loop* pLoop) {
                if (DOLOOP) {
                    const float fLoopEnd   = Float(pLoop->uiEnd);
                    const float fLoopStart = Float(pLoop->uiStart);
                    const float fLoopSize  = Float(pLoop->uiSize);
                    if (pLoop->uiTotalCycles) {
                        // render loop (loop count limited)
                        for (; pFinalParam->uiToGo > 0 && pLoop->uiCyclesLeft; pLoop->uiCyclesLeft -= WrapLoop(fLoopStart, fLoopSize, fLoopEnd, &pFinalParam->dPos)) {
                            const uint uiToGo = Min(pFinalParam->uiToGo, DiffToLoopEnd(fLoopEnd, &pFinalParam->dPos, pFinalParam->fFinalPitch) + 1); //TODO: instead of +1 we could also round up
                            SynthesizeSubSubFragment(pFinalParam, uiToGo);
                        }
                        // render on without loop
                        SynthesizeSubSubFragment(pFinalParam, pFinalParam->uiToGo);
                    } else { // render loop (endless loop)
                        for (; pFinalParam->uiToGo > 0; WrapLoop(fLoopStart, fLoopSize, fLoopEnd, &pFinalParam->dPos)) {
                            const uint uiToGo = Min(pFinalParam->uiToGo, DiffToLoopEnd(fLoopEnd, &pFinalParam->dPos, pFinalParam->fFinalPitch) + 1); //TODO: instead of +1 we could also round up
                            SynthesizeSubSubFragment(pFinalParam, uiToGo);
                        }
                    }
                } else { // no looping
                    SynthesizeSubSubFragment(pFinalParam, pFinalParam->uiToGo);
                }
            }

            /**
             * Returns the difference to the sample's loop end.
             */
            inline static int DiffToLoopEnd(const float& LoopEnd, const void* Pos, const float& Pitch) {
                return uint((LoopEnd - *((double *)Pos)) / Pitch);
            }

#if 0
            //TODO: this method is not in use yet, it's intended to be used for pitch=x.0f where we could use integer instead of float as playback position variable
            inline static int WrapLoop(const int& LoopStart, const int& LoopSize, const int& LoopEnd, int& Pos) {
                //TODO: we can easily eliminate the branch here
                if (Pos < LoopEnd) return 0;
                Pos = (Pos - LoopEnd) % LoopSize + LoopStart;
                return 1;
            }
#endif

            /**
             * This method handles looping of the RAM playback part of the
             * sample, thus repositioning the playback position once the
             * loop limit was reached. Note: looping of the disk streaming
             * part is handled by libgig (ReadAndLoop() method which will
             * be called by the DiskThread).
             */
            inline static int WrapLoop(const float& LoopStart, const float& LoopSize, const float& LoopEnd, void* vPos) {
                double * Pos = (double *)vPos;
                if (*Pos < LoopEnd) return 0;
                *Pos = fmod(*Pos - LoopEnd, LoopSize) + LoopStart;
                return 1;
            }

            inline static int32_t getSample(sample_t* src, int pos) {
                if (BITDEPTH24) {
                    pos *= 3;
                    #if WORDS_BIGENDIAN
                    unsigned char* p = (unsigned char*)src;
                    return p[pos] << 8 | p[pos + 1] << 16 | p[pos + 2] << 24;
                    #else
                    // 24bit read optimization: 
                    // a misaligned 32bit read and subquent 8 bit shift is faster (on x86)  than reading 3 single bytes and shifting them
                    return (*((int32_t *)(&((char *)(src))[pos])))<<8;
                    #endif
                } else {
                    return src[pos];
                }
            }

            static void SynthesizeSubSubFragment(SynthesisParam* pFinalParam, uint uiToGo) {
                float fVolumeL = pFinalParam->fFinalVolumeLeft;
                float fVolumeR = pFinalParam->fFinalVolumeRight;
                sample_t* pSrc = pFinalParam->pSrc;
                float* pOutL   = pFinalParam->pOutLeft;
                float* pOutR   = pFinalParam->pOutRight;
#ifdef CONFIG_INTERPOLATE_VOLUME
                float fDeltaL  = pFinalParam->fFinalVolumeDeltaLeft;
                float fDeltaR  = pFinalParam->fFinalVolumeDeltaRight;
#endif
                switch (CHANNELS) {
                    case MONO: {
                        float samplePoint;
                        if (INTERPOLATE) {
                            double dPos    = pFinalParam->dPos;
                            float fPitch   = pFinalParam->fFinalPitch;
                            if (USEFILTER) {
                                Filter& filterL = pFinalParam->filterLeft;
                                for (int i = 0; i < uiToGo; ++i) {
                                    samplePoint = Interpolate1StepMonoCPP(pSrc, &dPos, fPitch);
                                    samplePoint = filterL.Apply(samplePoint);
#ifdef CONFIG_INTERPOLATE_VOLUME
                                    fVolumeL += fDeltaL;
                                    fVolumeR += fDeltaR;
#endif
                                    pOutL[i] += samplePoint * fVolumeL;
                                    pOutR[i] += samplePoint * fVolumeR;
                                }
                            } else { // no filter needed
                                for (int i = 0; i < uiToGo; ++i) {
                                    samplePoint = Interpolate1StepMonoCPP(pSrc, &dPos, fPitch);
#ifdef CONFIG_INTERPOLATE_VOLUME
                                    fVolumeL += fDeltaL;
                                    fVolumeR += fDeltaR;
#endif
                                    pOutL[i] += samplePoint * fVolumeL;
                                    pOutR[i] += samplePoint * fVolumeR;
                                }
                            }
                            pFinalParam->dPos = dPos;
                        } else { // no interpolation
                            int pos_offset = (int) pFinalParam->dPos;
                            if (USEFILTER) {
                                Filter& filterL = pFinalParam->filterLeft;
                                for (int i = 0; i < uiToGo; ++i) {
                                    samplePoint = getSample(pSrc, i + pos_offset);
                                    samplePoint = filterL.Apply(samplePoint);
#ifdef CONFIG_INTERPOLATE_VOLUME
                                    fVolumeL += fDeltaL;
                                    fVolumeR += fDeltaR;
#endif
                                    pOutL[i] += samplePoint * fVolumeL;
                                    pOutR[i] += samplePoint * fVolumeR;
                                }
                            } else { // no filter needed
                                for (int i = 0; i < uiToGo; ++i) {
                                    samplePoint = getSample(pSrc, i + pos_offset);
#ifdef CONFIG_INTERPOLATE_VOLUME
                                    fVolumeL += fDeltaL;
                                    fVolumeR += fDeltaR;
#endif
                                    pOutL[i] += samplePoint * fVolumeL;
                                    pOutR[i] += samplePoint * fVolumeR;
                                }
                            }
                            pFinalParam->dPos += uiToGo;
                        }
                        break;
                    }
                    case STEREO: {
                        stereo_sample_t samplePoint;
                        if (INTERPOLATE) {
                            double dPos    = pFinalParam->dPos;
                            float fPitch   = pFinalParam->fFinalPitch;
                            if (USEFILTER) {
                                Filter& filterL = pFinalParam->filterLeft;
                                Filter& filterR = pFinalParam->filterRight;
                                for (int i = 0; i < uiToGo; ++i) {
                                    samplePoint = Interpolate1StepStereoCPP(pSrc, &dPos, fPitch);
                                    samplePoint.left  = filterL.Apply(samplePoint.left);
                                    samplePoint.right = filterR.Apply(samplePoint.right);
#ifdef CONFIG_INTERPOLATE_VOLUME
                                    fVolumeL += fDeltaL;
                                    fVolumeR += fDeltaR;
#endif
                                    pOutL[i] += samplePoint.left  * fVolumeL;
                                    pOutR[i] += samplePoint.right * fVolumeR;
                                }
                            } else { // no filter needed
                                for (int i = 0; i < uiToGo; ++i) {
                                    samplePoint = Interpolate1StepStereoCPP(pSrc, &dPos, fPitch);
#ifdef CONFIG_INTERPOLATE_VOLUME
                                    fVolumeL += fDeltaL;
                                    fVolumeR += fDeltaR;
#endif
                                    pOutL[i] += samplePoint.left  * fVolumeL;
                                    pOutR[i] += samplePoint.right * fVolumeR;
                                }
                            }
                            pFinalParam->dPos = dPos;
                        } else { // no interpolation
                            int pos_offset = ((int) pFinalParam->dPos) << 1;
                            if (USEFILTER) {
                                Filter& filterL = pFinalParam->filterLeft;
                                Filter& filterR = pFinalParam->filterRight;
                                for (int i = 0, ii = 0; i < uiToGo; ++i, ii+=2) {
                                    samplePoint.left = getSample(pSrc, ii + pos_offset);
                                    samplePoint.right = getSample(pSrc, ii + pos_offset + 1);
                                    samplePoint.left  = filterL.Apply(samplePoint.left);
                                    samplePoint.right = filterR.Apply(samplePoint.right);
#ifdef CONFIG_INTERPOLATE_VOLUME
                                    fVolumeL += fDeltaL;
                                    fVolumeR += fDeltaR;
#endif
                                    pOutL[i] += samplePoint.left  * fVolumeL;
                                    pOutR[i] += samplePoint.right * fVolumeR;
                                }
                            } else { // no filter needed
                                for (int i = 0, ii = 0; i < uiToGo; ++i, ii+=2) {
                                    samplePoint.left = getSample(pSrc, ii + pos_offset);
                                    samplePoint.right = getSample(pSrc, ii + pos_offset + 1);
#ifdef CONFIG_INTERPOLATE_VOLUME
                                    fVolumeL += fDeltaL;
                                    fVolumeR += fDeltaR;
#endif
                                    pOutL[i] += samplePoint.left  * fVolumeL;
                                    pOutR[i] += samplePoint.right * fVolumeR;
                                }
                            }
                            pFinalParam->dPos += uiToGo;
                        }
                        break;
                    }
                }
                pFinalParam->fFinalVolumeLeft = fVolumeL;
                pFinalParam->fFinalVolumeRight = fVolumeR;
                pFinalParam->pOutRight += uiToGo;
                pFinalParam->pOutLeft  += uiToGo;
                pFinalParam->uiToGo    -= uiToGo;
            }
    };

}} // namespace LinuxSampler::gig

#endif // __LS_GIG_SYNTHESIZER_H__

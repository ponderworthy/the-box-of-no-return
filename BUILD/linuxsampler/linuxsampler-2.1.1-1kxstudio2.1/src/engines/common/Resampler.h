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

// Note: the assembly code is currently disabled, as it doesn't fit into
// the new synthesis core introduced by LS 0.4.0

#ifndef __LS_RESAMPLER_H__
#define __LS_RESAMPLER_H__

#include "../../common/global_private.h"

// TODO: cubic interpolation is not yet supported by the MMX/SSE(1) version though
#ifndef USE_LINEAR_INTERPOLATION
# define USE_LINEAR_INTERPOLATION   1  ///< set to 0 if you prefer cubic interpolation (slower, better quality)
#endif

namespace LinuxSampler {

    /** @brief Stereo sample point
     *
     * Encapsulates one stereo sample point, thus signal value for one
     * sample point for left and right channel.
     */
    struct stereo_sample_t {
        float left;
        float right;
    };

    /** @brief Resampler Template
     *
     * This template provides pure C++ and MMX/SSE assembly implementations
     * for linear and cubic interpolation for pitching a mono or stereo
     * input signal.
     */
    template<bool INTERPOLATE,bool BITDEPTH24>
    class Resampler {
        public:
            inline static float GetNextSampleMonoCPP(sample_t* __restrict pSrc, double* __restrict Pos, float& Pitch) {
                if (INTERPOLATE) return Interpolate1StepMonoCPP(pSrc, Pos, Pitch);
                else { // no pitch, so no interpolation necessary
                    int pos_int = (int) *Pos;
                    *Pos += 1.0;
                    return pSrc [pos_int];
                }
            }

            inline static stereo_sample_t GetNextSampleStereoCPP(sample_t* __restrict pSrc, double* __restrict Pos, float& Pitch) {
                if (INTERPOLATE) return Interpolate1StepStereoCPP(pSrc, Pos, Pitch);
                else { // no pitch, so no interpolation necessary
                    int pos_int = (int) *Pos;
                    pos_int <<= 1;
                    *Pos += 1.0;
                    stereo_sample_t samplePoint;
                    samplePoint.left  = pSrc[pos_int];
                    samplePoint.right = pSrc[pos_int+1];
                    return samplePoint;
                }
            }

#if 0 // CONFIG_ASM && ARCH_X86
            inline static void GetNext4SamplesMonoMMXSSE(sample_t* pSrc, void* Pos, float& Pitch) {
                if (INTERPOLATE) Interpolate4StepsMonoMMXSSE(pSrc, Pos, Pitch);
                else { // no pitch, so no interpolation necessary
                    const float __4f = 4.0f;
                    __asm__ __volatile__ (
                        "movss    (%1), %%xmm5           # load Pos\n\t"
                        "cvtss2si %%xmm5, %%edi          # int(Pos)\n\t"
                        "addss    %2, %%xmm5             # Pos += 4.0f\n\t"
                        "movswl   (%0,%%edi,2), %%eax    # load sample 0\n\t"
                        "cvtsi2ss  %%eax, %%xmm2         # convert to float\n\t"
                        "shufps    $0x93, %%xmm2, %%xmm2 # shift up\n\t"
                        "movswl   2(%0,%%edi,2), %%edx   # load sample 1\n\t"
                        "cvtsi2ss  %%edx, %%xmm2         # convert to float\n\t"
                        "shufps    $0x93, %%xmm2, %%xmm2 # shift up\n\t"
                        "movss     %%xmm5, (%1)          # update Pos\n\t"
                        "movswl   4(%0,%%edi,2), %%eax   # load sample 2\n\t"
                        "cvtsi2ss  %%eax, %%xmm2         # convert to float\n\t"
                        "shufps    $0x93, %%xmm2, %%xmm2 # shift up\n\t"
                        "movswl   6(%0,%%edi,2), %%edx   # load sample 3\n\t"
                        "cvtsi2ss  %%edx, %%xmm2         # convert to float\n\t"
                        "shufps    $0x1b, %%xmm2, %%xmm2 # swap to correct order\n\t"
                        :: "r" (pSrc), "r" (Pos), "m" (__4f)
                        :  "%eax", "%edx", "%edi"
                    );
                }
            }

            inline static void GetNext4SamplesStereoMMXSSE(sample_t* pSrc, void* Pos, float& Pitch) {
                if (INTERPOLATE) {
                    Interpolate4StepsStereoMMXSSE(pSrc, Pos, Pitch);
                    //EMMS;
                } else { // no pitch, so no interpolation necessary
                    const float __4f = 4.0f;
                    __asm__ __volatile__ (
                        "movss    (%1), %%xmm5           # load Pos\n\t"
                        "cvtss2si %%xmm5, %%edi          # int(Pos)\n\t"
                        "addss    %2, %%xmm5             # Pos += 4.0f\n\t"
                        "movswl    (%0, %%edi,4), %%eax  # load sample 0 (left)\n\t"
                        "cvtsi2ss  %%eax, %%xmm2         # convert to float\n\t"
                        "shufps    $0x93, %%xmm2, %%xmm2 # shift up\n\t"
                        "movss     %%xmm5, (%1)          # update Pos\n\t"
                        "movswl   2(%0, %%edi,4), %%edx  # load sample 0 (left)\n\t"
                        "cvtsi2ss  %%edx, %%xmm3         # convert to float\n\t"
                        "shufps    $0x93, %%xmm3, %%xmm3 # shift up\n\t"
                        "movswl   4(%0, %%edi,4), %%eax  # load sample 1 (left)\n\t"
                        "cvtsi2ss  %%eax, %%xmm2         # convert to float\n\t"
                        "shufps    $0x93, %%xmm2, %%xmm2 # shift up\n\t"
                        "movswl   6(%0, %%edi,4), %%edx  # load sample 1 (right)\n\t"
                        "cvtsi2ss  %%edx, %%xmm3         # convert to float\n\t"
                        "shufps    $0x93, %%xmm3, %%xmm3 # shift up\n\t"
                        "movswl   8(%0, %%edi,4), %%eax  # load sample 2 (left)\n\t"
                        "cvtsi2ss  %%eax, %%xmm2         # convert to float\n\t"
                        "shufps    $0x93, %%xmm2, %%xmm2 # shift up\n\t"
                        "movswl  10(%0, %%edi,4), %%edx  # load sample 2 (right)\n\t"
                        "cvtsi2ss  %%edx, %%xmm3         # convert to float\n\t"
                        "shufps    $0x93, %%xmm3, %%xmm3 # shift up\n\t"
                        "movswl  12(%0, %%edi,4), %%eax  # load sample 3 (left)\n\t"
                        "cvtsi2ss  %%eax, %%xmm2         # convert to float\n\t"
                        "shufps    $0x1b, %%xmm2, %%xmm2 # swap to correct order\n\t"
                        "movswl  14(%0, %%edi,4), %%edx  # load sample 3 (right)\n\t"
                        "cvtsi2ss  %%edx, %%xmm3         # convert to float\n\t"
                        "shufps    $0x1b, %%xmm3, %%xmm3 # swap to correct order\n\t"
                        :: "r" (pSrc), "r" (Pos), "m" (__4f)
                        :  "%eax", "%edx", "%edi"
                    );
                }
            }
#endif // CONFIG_ASM && ARCH_X86

        protected:

            inline static int32_t getSample(sample_t* __restrict src, int pos) {
                if (BITDEPTH24) {
                    pos *= 3;
                    #if WORDS_BIGENDIAN
                    unsigned char* p = (unsigned char*)src;
                    return p[pos] << 8 | p[pos + 1] << 16 | p[pos + 2] << 24;
                    #else
                    // 24bit read optimization: 
                    // a misaligned 32bit read and subquent 8 bit shift is faster (on x86) than reading 3 single bytes and shifting them
                    return (*((int32_t *)(&((char *)(src))[pos])))<<8;
                    #endif
                } else {
                    return src[pos];
                }
            }

            inline static float Interpolate1StepMonoCPP(sample_t* __restrict pSrc, double* __restrict Pos, float& Pitch) {
                int   pos_int   = (int) *Pos;     // integer position
                float pos_fract = *Pos - pos_int; // fractional part of position

                #if USE_LINEAR_INTERPOLATION
                    int x1 = getSample(pSrc, pos_int);
                    int x2 = getSample(pSrc, pos_int + 1);
                    float samplePoint  = (x1 + pos_fract * (x2 - x1));
                #else // polynomial interpolation
                    float xm1 = getSample(pSrc, pos_int);
                    float x0  = getSample(pSrc, pos_int + 1);
                    float x1  = getSample(pSrc, pos_int + 2);
                    float x2  = getSample(pSrc, pos_int + 3);
                    float a   = (3.0f * (x0 - x1) - xm1 + x2) * 0.5f;
                    float b   = 2.0f * x1 + xm1 - (5.0f * x0 + x2) * 0.5f;
                    float c   = (x1 - xm1) * 0.5f;
                    float samplePoint =  (((a * pos_fract) + b) * pos_fract + c) * pos_fract + x0;
                #endif // USE_LINEAR_INTERPOLATION

                *Pos += Pitch;
                return samplePoint;
            }

            inline static stereo_sample_t Interpolate1StepStereoCPP(sample_t* __restrict pSrc, double* __restrict Pos, float& Pitch) {
                int   pos_int   = (int) *Pos;  // integer position
                float pos_fract = *Pos - pos_int;     // fractional part of position
                pos_int <<= 1;

                stereo_sample_t samplePoint;

                #if USE_LINEAR_INTERPOLATION
                    // left channel
                    int x1 = getSample(pSrc, pos_int);
                    int x2 = getSample(pSrc, pos_int + 2);
                    samplePoint.left  = (x1 + pos_fract * (x2 - x1));
                    // right channel
                    x1 = getSample(pSrc, pos_int + 1);
                    x2 = getSample(pSrc, pos_int + 3);
                    samplePoint.right = (x1 + pos_fract * (x2 - x1));
                #else // polynomial interpolation
                    // calculate left channel
                    float xm1 = getSample(pSrc, pos_int);
                    float x0  = getSample(pSrc, pos_int + 2);
                    float x1  = getSample(pSrc, pos_int + 4);
                    float x2  = getSample(pSrc, pos_int + 6);
                    float a   = (3.0f * (x0 - x1) - xm1 + x2) * 0.5f;
                    float b   = 2.0f * x1 + xm1 - (5.0f * x0 + x2) * 0.5f;
                    float c   = (x1 - xm1) * 0.5f;
                    samplePoint.left = (((a * pos_fract) + b) * pos_fract + c) * pos_fract + x0;

                    //calculate right channel
                    xm1 = getSample(pSrc, pos_int + 1);
                    x0  = getSample(pSrc, pos_int + 3);
                    x1  = getSample(pSrc, pos_int + 5);
                    x2  = getSample(pSrc, pos_int + 7);
                    a   = (3.0f * (x0 - x1) - xm1 + x2) * 0.5f;
                    b   = 2.0f * x1 + xm1 - (5.0f * x0 + x2) * 0.5f;
                    c   = (x1 - xm1) * 0.5f;
                    samplePoint.right =  (((a * pos_fract) + b) * pos_fract + c) * pos_fract + x0;
                #endif // USE_LINEAR_INTERPOLATION

                *Pos += Pitch;
                return samplePoint;
            }

#if 0 // CONFIG_ASM && ARCH_X86
            // TODO: no support for cubic interpolation yet
            inline static void Interpolate4StepsMonoMMXSSE(sample_t* pSrc, void* Pos, float& Pitch) {
                /* calculate playback position of each of the 4 samples by adding the associated pitch */
                __asm__ __volatile__ (
                    "movss    (%0),%%xmm0             # sample position of sample[0] -> xmm0[0]\n\t"
                    "movss    %1,%%xmm1               # copy pitch -> xmm1[0]\n\t"
                    "shufps   $0x90,%%xmm0,%%xmm0     # shift up, but keep xmm0[0]\n\t"
                    "addss    %%xmm1,%%xmm0           # calculate sample position of sample[1]\n\t"
                    "shufps   $0x90,%%xmm0,%%xmm0     # shift up, but keep xmm0[0]\n\t"
                    "addss    %%xmm1,%%xmm0           # calculate sample position of sample[2]\n\t"
                    "shufps   $0x90,%%xmm0,%%xmm0     # shift up, but keep xmm0[0]\n\t"
                    "addss    %%xmm1,%%xmm0           # calculate sample position of sample[3]\n\t"
                    "movss    %%xmm0,%%xmm2           # xmm0[0] -> xmm2[0]\n\t"
                    "addss    %%xmm1,%%xmm2           # calculate initial sample position for the next 4-sample cycle\n\t"
                    "movss    %%xmm2,(%0)             # update 'Pos'\n\t"
                    "shufps   $0x1b,%%xmm0,%%xmm0     # swap, so that xmm0[0]=sample pos 0, xmm0[1]=sample pos 1,...\n\t"
                    "cvttps2pi %%xmm0,%%mm4           # int(xmm0[0-1]) -> mm4\n\t"
                    "shufps   $0xe4,%%xmm0,%%xmm1     # xmm0[2-3] -> xmm1[2-3]\n\t"
                    "shufps   $0x0e,%%xmm1,%%xmm1     # xmm1[2-3] -> xmm1[0-1]\n\t"
                    "cvttps2pi %%xmm1,%%mm5           # int(xmm1[0-1]) -> mm5\n\t"
                    "cvtpi2ps %%mm5,%%xmm1            # double(mm5) -> xmm1[0-1]\n\t"
                    "shufps   $0x44,%%xmm1,%%xmm1     # shift lower 2 FPs up to the upper 2 cells\n\t"
                    "cvtpi2ps %%mm4,%%xmm1            # double(mm4) -> xmm1[0-1]\n\t"
                    "subps    %%xmm1,%%xmm0           # xmm0[1-3] = xmm0[1-3] - xmm1[1-3]\n\t"
                    :
                    : "r" (Pos),  /* %0 */
                      "m" (Pitch) /* %1 */
                    : "%xmm0", /* holds fractional position (0.0 <= x < 1.0) of sample 0-3 at the end */
                      "%xmm1", /* holds integer position (back converted to SPFP) of sample 0-3 at the end */
                      "mm4",  /* holds integer position of sample 0-1 at the end */
                      "mm5",  /* holds integer position of sample 2-3 at the end */
                      "st", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)"
                );
                /* get sample values of pSrc[pos_int] and pSrc[pos_int+1] of the 4 samples */
                __asm__ __volatile__ (
                    "movd   %%mm4,%%edi               # sample position of sample 0\n\t"
                    "psrlq  $32,%%mm4                 # mm4 >> 32\n\t"
                    "movswl (%0,%%edi,2),%%eax        # pSrc[pos_int] (sample 0)\n\t"
                    "movswl 2(%0,%%edi,2),%%ecx       # pSrc[pos_int] (sample 0+1)\n\t"
                    "cvtsi2ss %%eax, %%xmm2           # pSrc[pos_int] -> xmm2[0]\n\t"
                    "cvtsi2ss %%ecx, %%xmm3           # pSrc[pos_int] -> xmm3[0]\n\t"
                    "shufps    $0x93, %%xmm2, %%xmm2  # shift up\n\t"
                    "shufps    $0x93, %%xmm3, %%xmm3  # shift up\n\t"
                    "movd   %%mm4,%%edi               # sample position of sample 1\n\t"
                    "movswl (%0,%%edi,2),%%eax        # pSrc[pos_int] (sample 1)\n\t"
                    "movswl 2(%0,%%edi,2),%%ecx       # pSrc[pos_int] (sample 1+1)\n\t"
                    "cvtsi2ss %%eax, %%xmm2           # pSrc[pos_int] -> xmm2[0]\n\t"
                    "cvtsi2ss %%ecx, %%xmm3           # pSrc[pos_int] -> xmm3[0]\n\t"
                    "shufps    $0x93, %%xmm2, %%xmm2  # shift up\n\t"
                    "shufps    $0x93, %%xmm3, %%xmm3  # shift up\n\t"
                    "movd   %%mm5,%%edi               # sample position of sample 2\n\t"
                    "psrlq  $32,%%mm5                 # mm5 >> 32\n\t"
                    "movswl (%0,%%edi,2),%%eax        # pSrc[pos_int] (sample 2)\n\t"
                    "movswl 2(%0,%%edi,2),%%ecx       # pSrc[pos_int] (sample 2+1)\n\t"
                    "cvtsi2ss %%eax, %%xmm2           # pSrc[pos_int] -> xmm2[0]\n\t"
                    "cvtsi2ss %%ecx, %%xmm3           # pSrc[pos_int] -> xmm3[0]\n\t"
                    "shufps    $0x93, %%xmm2, %%xmm2  # shift up\n\t"
                    "shufps    $0x93, %%xmm3, %%xmm3  # shift up\n\t"
                    "movd   %%mm5,%%edi               # sample position of sample 2\n\t"
                    "movswl (%0,%%edi,2),%%eax        # pSrc[pos_int] (sample 3)\n\t"
                    "movswl 2(%0,%%edi,2),%%ecx       # pSrc[pos_int] (sample 3+1)\n\t"
                    "cvtsi2ss %%eax, %%xmm2           # pSrc[pos_int] -> xmm2[0]\n\t"
                    "cvtsi2ss %%ecx, %%xmm3           # pSrc[pos_int] -> xmm3[0]\n\t"
                    "shufps    $0x1b, %%xmm2, %%xmm2  # swap to correct order\n\t"
                    "shufps    $0x1b, %%xmm3, %%xmm3  # swap to correct order\n\t"
                    : /* no output */
                    : "S" (pSrc) /* %0 - sample read position  */
                    : "%eax", "%ecx", /*"%edx",*/ "%edi",
                      "%xmm2", /* holds pSrc[int_pos]   of the 4 samples at the end */
                      "%xmm3", /* holds pSrc[int_pos+1] of the 4 samples at the end */
                      "mm4",  /* holds integer position of sample 0-1 at the end */
                      "mm5",  /* holds integer position of sample 2-3 at the end */
                      "st", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)"
                );
                /* linear interpolation of the 4 samples simultaniously */
                __asm__ __volatile__ (
                    "subps %%xmm2,%%xmm3   # xmm3 = pSrc[pos_int+1] - pSrc[pos_int]\n\t"
                    "mulps %%xmm0,%%xmm3   # xmm3 = pos_fract * (pSrc[pos_int+1] - pSrc[pos_int])\n\t"
                    "addps %%xmm3,%%xmm2   # xmm2 = pSrc[pos_int] + (pos_fract * (pSrc[pos_int+1] - pSrc[pos_int]))\n\t"
                    : /* no output */
                    : /* no input */
                    : "%xmm2" /* holds linear interpolated sample point (of all 4 samples) at the end */
                );
            }

            // TODO: no support for cubic interpolation yet
            inline static void Interpolate4StepsStereoMMXSSE(sample_t* pSrc, void* Pos, float& Pitch) {
                /* calculate playback position of each of the 4 samples by adding the associated pitch */
                __asm__ __volatile__ (
                    "movss    (%0),%%xmm0             # sample position of sample[0] -> xmm0[0]\n\t"
                    "movss    %1,%%xmm1               # copy pitch -> xmm1[0]\n\t"
                    "shufps   $0x90,%%xmm0,%%xmm0     # shift up, but keep xmm0[0]\n\t"
                    "addss    %%xmm1,%%xmm0           # calculate sample position of sample[1]\n\t"
                    "shufps   $0x90,%%xmm0,%%xmm0     # shift up, but keep xmm0[0]\n\t"
                    "addss    %%xmm1,%%xmm0           # calculate sample position of sample[2]\n\t"
                    "shufps   $0x90,%%xmm0,%%xmm0     # shift up, but keep xmm0[0]\n\t"
                    "addss    %%xmm1,%%xmm0           # calculate sample position of sample[3]\n\t"
                    "movss    %%xmm0,%%xmm2           # xmm0[0] -> xmm2[0]\n\t"
                    "addss    %%xmm1,%%xmm2           # calculate initial sample position for the next 4-sample cycle\n\t"
                    "movss    %%xmm2,(%0)             # update 'Pos'\n\t"
                    "shufps   $0x1b,%%xmm0,%%xmm0     # swap, so that xmm0[0]=sample pos 0, xmm0[1]=sample pos 1,...\n\t"
                    "cvttps2pi %%xmm0,%%mm4           # int(xmm0[0-1]) -> mm4\n\t"
                    "shufps   $0xe4,%%xmm0,%%xmm1     # xmm0[2-3] -> xmm1[2-3]\n\t"
                    "shufps   $0x0e,%%xmm1,%%xmm1     # xmm1[2-3] -> xmm1[0-1]\n\t"
                    "cvttps2pi %%xmm1,%%mm5           # int(xmm1[0-1]) -> mm5\n\t"
                    "cvtpi2ps %%mm5,%%xmm1            # double(mm5) -> xmm1[0-1]\n\t"
                    "shufps   $0x44,%%xmm1,%%xmm1     # shift lower 2 FPs up to the upper 2 cells\n\t"
                    "cvtpi2ps %%mm4,%%xmm1            # double(mm4) -> xmm1[0-1]\n\t"
                    "subps    %%xmm1,%%xmm0           # xmm0[1-3] = xmm0[1-3] - xmm1[1-3]\n\t"
                    :
                    : "r" (Pos),  /* %0 */
                      "m" (Pitch) /* %1 */
                    : "%xmm0", /* holds fractional position (0.0 <= x < 1.0) of sample 0-3 at the end */
                      "%xmm1", /* holds integer position (back converted to SPFP) of sample 0-3 at the end */
                      "mm4",  /* holds integer position of sample 0-1 at the end */
                      "mm5",  /* holds integer position of sample 2-3 at the end */
                      "st", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)"
                );

                /* get sample values of pSrc[pos_int], pSrc[pos_int+1], pSrc[pos_int+2] and pSrc[pos_int+3] of the 4 samples */
                __asm__ __volatile__ (
                    "xorl   %%eax,%%eax               # clear eax\n\t"
                    "xorl   %%edx,%%edx               # clear edx\n\t"
                    "movd   %%mm4,%%edi               # sample position of sample 0\n\t"
                    "psrlq  $32,%%mm4                 # mm4 >> 32\n\t"
                    "movswl (%0,%%edi,4),%%eax        # pSrc[pos_int] (sample 0)\n\t"
                    "cvtsi2ss %%eax, %%xmm2           # pSrc[pos_int] -> xmm2[0]\n\t"
                    "shufps    $0x93, %%xmm2, %%xmm2  # shift up\n\t"
                    "movswl 2(%0,%%edi,4),%%edx       # pSrc[pos_int] (sample 0+1)\n\t"
                    "cvtsi2ss %%edx, %%xmm3           # pSrc[pos_int] -> xmm3[0]\n\t"
                    "shufps    $0x93, %%xmm3, %%xmm3  # shift up\n\t"
                    "movswl 4(%0,%%edi,4),%%eax       # pSrc[pos_int] (sample 0+2)\n\t"
                    "cvtsi2ss %%eax, %%xmm4           # pSrc[pos_int] -> xmm4[0]\n\t"
                    "shufps    $0x93, %%xmm4, %%xmm4  # shift up\n\t"
                    "movswl 6(%0,%%edi,4),%%edx       # pSrc[pos_int] (sample 0+3)\n\t"
                    "cvtsi2ss %%edx, %%xmm5           # pSrc[pos_int] -> xmm5[0]\n\t"
                    "movd   %%mm4,%%edi               # sample position of sample 1\n\t"
                    "shufps    $0x93, %%xmm5, %%xmm5  # shift up\n\t"
                    "movswl (%0,%%edi,4),%%eax        # pSrc[pos_int] (sample 1)\n\t"
                    "cvtsi2ss %%eax, %%xmm2           # pSrc[pos_int] -> xmm2[0]\n\t"
                    "shufps    $0x93, %%xmm2, %%xmm2  # shift up\n\t"
                    "movswl 2(%0,%%edi,4),%%edx       # pSrc[pos_int] (sample 1+1)\n\t"
                    "cvtsi2ss %%edx, %%xmm3           # pSrc[pos_int] -> xmm3[0]\n\t"
                    "shufps    $0x93, %%xmm3, %%xmm3  # shift up\n\t"
                    "movswl 4(%0,%%edi,4),%%eax       # pSrc[pos_int] (sample 1+2)\n\t"
                    "cvtsi2ss %%eax, %%xmm4           # pSrc[pos_int] -> xmm4[0]\n\t"
                    "shufps    $0x93, %%xmm4, %%xmm4  # shift up\n\t"
                    "movswl 6(%0,%%edi,4),%%edx       # pSrc[pos_int] (sample 1+3)\n\t"
                    "cvtsi2ss %%edx, %%xmm5           # pSrc[pos_int] -> xmm5[0]\n\t"
                    "movd   %%mm5,%%edi               # sample position of sample 2\n\t"
                    "shufps    $0x93, %%xmm5, %%xmm5  # shift up\n\t"
                    "psrlq  $32,%%mm5                 # mm5 >> 32\n\t"
                    "movswl (%0,%%edi,4),%%eax        # pSrc[pos_int] (sample 2)\n\t"
                    "cvtsi2ss %%eax, %%xmm2           # pSrc[pos_int] -> xmm2[0]\n\t"
                    "shufps    $0x93, %%xmm2, %%xmm2  # shift up\n\t"
                    "movswl 2(%0,%%edi,4),%%edx       # pSrc[pos_int] (sample 2+1)\n\t"
                    "cvtsi2ss %%edx, %%xmm3           # pSrc[pos_int] -> xmm3[0]\n\t"
                    "shufps    $0x93, %%xmm3, %%xmm3  # shift up\n\t"
                    "movswl 4(%0,%%edi,4),%%eax       # pSrc[pos_int] (sample 2+2)\n\t"
                    "cvtsi2ss %%eax, %%xmm4           # pSrc[pos_int] -> xmm4[0]\n\t"
                    "shufps    $0x93, %%xmm4, %%xmm4  # shift up\n\t"
                    "movswl 6(%0,%%edi,4),%%edx       # pSrc[pos_int] (sample 2+3)\n\t"
                    "cvtsi2ss %%edx, %%xmm5           # pSrc[pos_int] -> xmm5[0]\n\t"
                    "movd   %%mm5,%%edi               # sample position of sample 3\n\t"
                    "shufps    $0x93, %%xmm5, %%xmm5  # shift up\n\t"
                    "movswl (%0,%%edi,4),%%eax        # pSrc[pos_int] (sample 3)\n\t"
                    "cvtsi2ss %%eax, %%xmm2           # pSrc[pos_int] -> xmm2[0]\n\t"
                    "shufps    $0x1b, %%xmm2, %%xmm2  # shift up\n\t"
                    "movswl 2(%0,%%edi,4),%%edx       # pSrc[pos_int] (sample 3+1)\n\t"
                    "cvtsi2ss %%edx, %%xmm3           # pSrc[pos_int] -> xmm3[0]\n\t"
                    "shufps    $0x1b, %%xmm3, %%xmm3  # shift up\n\t"
                    "movswl 4(%0,%%edi,4),%%eax       # pSrc[pos_int] (sample 3+2)\n\t"
                    "cvtsi2ss %%eax, %%xmm4           # pSrc[pos_int] -> xmm4[0]\n\t"
                    "shufps    $0x1b, %%xmm4, %%xmm4  # swap to correct order\n\t"
                    "movswl 6(%0,%%edi,4),%%edx       # pSrc[pos_int] (sample 3+3)\n\t"
                    "cvtsi2ss %%edx, %%xmm5           # pSrc[pos_int] -> xmm5[0]\n\t"
                    "shufps    $0x1b, %%xmm5, %%xmm5  # swap to correct order\n\t"
                    : /* no output */
                    : "S" (pSrc) /* %0 - sample read position  */
                    : "%eax", "%edx", "%edi",
                      "xmm2", /* holds pSrc[int_pos]   of the 4 samples at the end */
                      "xmm3", /* holds pSrc[int_pos+1] of the 4 samples at the end */
                      "xmm4", /* holds pSrc[int_pos+2] of the 4 samples at the end */
                      "xmm5", /* holds pSrc[int_pos+3] of the 4 samples at the end */
                      "mm4",  /* holds integer position of sample 0-1 at the end */
                      "mm5",  /* holds integer position of sample 2-3 at the end */
                      "st", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)"
                );
                /* linear interpolation of the 4 samples (left & right channel) simultaniously */
                __asm__ __volatile__ (
                    "subps %%xmm2,%%xmm4   # xmm4 = pSrc[pos_int+2] - pSrc[pos_int] (left channel)\n\t"
                    "mulps %%xmm0,%%xmm4   # xmm4 = pos_fract * (pSrc[pos_int+2] - pSrc[pos_int]) (left channel)\n\t"
                    "addps %%xmm4,%%xmm2   # xmm2 = pSrc[pos_int] + (pos_fract * (pSrc[pos_int+2] - pSrc[pos_int])) (left channel)\n\t"
                    "subps %%xmm3,%%xmm5   # xmm5 = pSrc[pos_int+3] - pSrc[pos_int+1] (right channel)\n\t"
                    "mulps %%xmm0,%%xmm5   # xmm5 = pos_fract * (pSrc[pos_int+3] - pSrc[pos_int+1]) (right channel)\n\t"
                    "addps %%xmm5,%%xmm3   # xmm3 = pSrc[pos_int+1] + (pos_fract * (pSrc[pos_int+3] - pSrc[pos_int+1])) (right channel)\n\t"
                    : /* no output */
                    : /* no input */
                    : "%xmm2", /* holds linear interpolated sample of left  channel (of all 4 samples) at the end */
                      "%xmm3"  /* holds linear interpolated sample of right channel (of all 4 samples) at the end */
                );
            }
#endif // CONFIG_ASM && ARCH_X86
    };

} // namespace LinuxSampler

#endif // __LS_RESAMPLER_H__

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

#ifndef __LS_BIQUADFILTER_H__
#define __LS_BIQUADFILTER_H__

#include <math.h>

#include "../../common/global_private.h"

/// ln(2) / 2
#define LN_2_2			0.34657359f

#ifndef LIMIT
# define LIMIT(v,l,u)		(v < l ? l : (v > u ? u : v))
#endif

namespace LinuxSampler {

    typedef float bq_t;

    /**
     * Internal parameters of the biquad filter, which are actually the
     * final parameters of the filter's transfer function. This strucure is
     * only needed when these parameters should stored outside the
     * BiquadFilter class, e.g. to save calculation time by sharing them
     * between multiple filters.
     */
    struct biquad_param_t {
        bq_t b0;
        bq_t b1;
        bq_t b2;
        bq_t a1;
        bq_t a2;
    };

    /**
     * Bi-quadratic filter
     * (adapted from lisp code by Eli Brandt, http://www.cs.cmu.edu/~eli/)
     */
    class BiquadFilter {
        protected:
            // following five variables are only used if no external biquad_param_t reference is used
            bq_t b0;
            bq_t b1;
            bq_t b2;
            bq_t a1;
            bq_t a2;
            // following four variables are used to buffer the feedback
            bq_t x1;
            bq_t x2;
            bq_t y1;
            bq_t y2;

#if __GNUC__ >= 4
            float fbc;
#else
            const static float fbc = 0.98;
#endif

            /**
             * Prevent \a f from going into denormal mode which would slow down
             * subsequent floating point calculations, we achieve that by setting
             * \a f to zero when it falls under the denormal threshold value.
             */
            inline void KillDenormal(bq_t& f) {
                // TODO: this is a generic solution for 32bit floats, should be replaced by CPU specific asm code
                f += 1e-18f;
                f -= 1e-18f;
            }
        public:
            BiquadFilter() {
                Reset();
#if __GNUC__ >= 4
                fbc = 0.98f;
#endif
            }

            void Reset() {
                x1 = 0.0f;
                x2 = 0.0f;
                y1 = 0.0f;
                y2 = 0.0f;
            }

            inline bq_t Apply(const bq_t x) {
                bq_t y;

                y = this->b0 * x + this->b1 * this->x1 + this->b2 * this->x2 +
                    this->a1 * this->y1 + this->a2 * this->y2;
                KillDenormal(y);
                this->x2 = this->x1;
                this->x1 = x;
                this->y2 = this->y1;
                this->y1 = y;

                return y;
            }

            inline bq_t Apply(const biquad_param_t* __restrict param, const bq_t x) {
                bq_t y;

                y = param->b0 * x + param->b1 * this->x1 + param->b2 * this->x2 +
                    param->a1 * this->y1 + param->a2 * this->y2;
                KillDenormal(y);
                this->x2 = this->x1;
                this->x1 = x;
                this->y2 = this->y1;
                this->y1 = y;

                return y;
            }

#if 0 // CONFIG_ASM && ARCH_X86
            // expects to find input in xmm0 (xmm0 stays unmodified) and finally leaves output in xmm6
            inline void Apply4StepsSSE(biquad_param_t* param) {
                __asm__ __volatile__ (
                    "movss (%2),%%xmm4                # b0\n\t"
                    "shufps   $0x00,%%xmm4,%%xmm4     # copy b0 to other cells\n\t"
                    "mulps  %%xmm0,%%xmm4             # xmm4 = x*b0\n\t"
                    "movups (%0),%%xmm2               # load b1,b2,a1,a2\n\t"
                    "movups (%1),%%xmm5               # load x1,x2,y1,y2\n\t"
                    /* sample 0 */
                    "movaps %%xmm5,%%xmm3\n\t"
                    "mulps  %%xmm2,%%xmm5             # xmm5 = [b1,b2,a1,a2] * [x1,x2,y1,y2]\n\t"
                    "shufps $0x0a,%%xmm3,%%xmm3       # x2 = x1, y2 = y1\n\t"
                    "movss  %%xmm4,%%xmm6\n\t"
                    "addss  %%xmm5,%%xmm6\n\t"
                    "shufps $0x39,%%xmm5,%%xmm5\n\t"
                    "addss  %%xmm5,%%xmm6\n\t"
                    "shufps $0x39,%%xmm5,%%xmm5\n\t"
                    "addss  %%xmm5,%%xmm6\n\t"
                    "shufps $0x39,%%xmm5,%%xmm5\n\t"
                    "addss  %%xmm5,%%xmm6             # xmm6 = b0*x + b1*x1 + b2*x2 + a1*y1 + a2*y2\n\t"
                    /* sample 1 */
                    "shufps $0x39,%%xmm4,%%xmm4       # rotate xmm4 down 1 cell\n\t"
                    "movss  %%xmm6,%%xmm3             # y1 = y\n\t"
                    "shufps $0x4e,%%xmm3,%%xmm3       # rotate 2 cells\n\t"
                    "movss  %%xmm0,%%xmm3             # x1 = x\n\t"
                    "shufps $0x93,%%xmm6,%%xmm6       # rotate output up 1 cell\n\t"
                    "movaps %%xmm3,%%xmm5\n\t"
                    "shufps $0x39,%%xmm0,%%xmm0       # rotate input down 1 cell\n\t"
                    "mulps  %%xmm2,%%xmm5             # xmm5 = [b1,b2,a1,a2] * [x1,x2,y1,y2]\n\t"
                    "movss  %%xmm5,%%xmm6\n\t"
                    "addss  %%xmm4,%%xmm6\n\t"
                    "shufps $0x39,%%xmm5,%%xmm5\n\t"
                    "addss  %%xmm5,%%xmm6\n\t"
                    "shufps $0x39,%%xmm5,%%xmm5\n\t"
                    "addss  %%xmm5,%%xmm6\n\t"
                    "shufps $0x39,%%xmm5,%%xmm5\n\t"
                    "addss  %%xmm5,%%xmm6             # xmm6 = b0*x + b1*x1 + b2*x2 + a1*y1 + a2*y2\n\t"
                    /* sample 2 */
                    "shufps $0x0a,%%xmm3,%%xmm3       # x2 = x1, y2 = y1\n\t"
                    "shufps $0x39,%%xmm4,%%xmm4       # rotate xmm4 down 1 cell\n\t"
                    "movss  %%xmm6,%%xmm3             # y1 = y\n\t"
                    "shufps $0x4e,%%xmm3,%%xmm3       # rotate 2 cells\n\t"
                    "movss  %%xmm0,%%xmm3             # x1 = x\n\t"
                    "shufps $0x93,%%xmm6,%%xmm6       # rotate output up 1 cell\n\t"
                    "movaps %%xmm3,%%xmm5\n\t"
                    "shufps $0x39,%%xmm0,%%xmm0       # rotate input down 1 cell\n\t"
                    "mulps  %%xmm2,%%xmm5             # xmm5 = [b1,b2,a1,a2] * [x1,x2,y1,y2]\n\t"
                    "movss  %%xmm5,%%xmm6\n\t"
                    "addss  %%xmm4,%%xmm6\n\t"
                    "shufps $0x39,%%xmm5,%%xmm5\n\t"
                    "addss  %%xmm5,%%xmm6\n\t"
                    "shufps $0x39,%%xmm5,%%xmm5\n\t"
                    "addss  %%xmm5,%%xmm6\n\t"
                    "shufps $0x39,%%xmm5,%%xmm5\n\t"
                    "addss  %%xmm5,%%xmm6             # xmm6 = b0*x + b1*x1 + b2*x2 + a1*y1 + a2*y2\n\t"
                    /* sample 3 */
                    "shufps $0x0a,%%xmm3,%%xmm3       # x2 = x1, y2 = y1\n\t"
                    "shufps $0x39,%%xmm4,%%xmm4       # rotate xmm4 down 1 cell\n\t"
                    "movss  %%xmm6,%%xmm3             # y1 = y\n\t"
                    "shufps $0x4e,%%xmm3,%%xmm3       # rotate 2 cells\n\t"
                    "movss  %%xmm0,%%xmm3             # x1 = x\n\t"
                    "shufps $0x93,%%xmm6,%%xmm6       # rotate output up 1 cell\n\t"
                    "mulps  %%xmm3,%%xmm2             # xmm5 = [b1,b2,a1,a2] * [x1,x2,y1,y2]\n\t"
                    "shufps $0x39,%%xmm0,%%xmm0       # rotate input down 1 cell\n\t"
                    "movss  %%xmm2,%%xmm6\n\t"
                    "shufps $0x39,%%xmm2,%%xmm2\n\t"
                    "addss  %%xmm2,%%xmm6\n\t"
                    "shufps $0x39,%%xmm2,%%xmm2\n\t"
                    "addss  %%xmm2,%%xmm6\n\t"
                    "shufps $0x39,%%xmm2,%%xmm2\n\t"
                    "addss  %%xmm2,%%xmm6\n\t"
                    "addss  %%xmm4,%%xmm6             # xmm6 = b0*x + b1*x1 + b2*x2 + a1*y1 + a2*y2\n\t"
                    /* done */
                    "shufps $0x0a,%%xmm3,%%xmm3       # x2 = x1, y2 = y1\n\t"
                    "movss  %%xmm6,%%xmm3             # y1 = y\n\t"
                    "shufps $0x4e,%%xmm3,%%xmm3       # rotate 2 cells\n\t"
                    "movss  %%xmm0,%%xmm3             # x1 = x\n\t"
                    "shufps $0x1b,%%xmm6,%%xmm6       # swap output to correct order\n\t"
                    "shufps $0x39,%%xmm0,%%xmm0       # rotate input down 1 cell, to restore original input\n\t"
                    "movups %%xmm3,(%1)               # store x1,x2,y1,y2\n\t"
                    : /* no output */
                    : "r" (&param->b1), /* %0 - [b1,b2,a1,a2] */
                      "r" (&x1),        /* %1 - [x1,x2,y1,y2] */
                      "r" (&param->b0)  /* %2 */
                );
            }
#endif // CONFIG_ASM && ARCH_X86

            inline bq_t ApplyFB(bq_t x, const bq_t fb) {
                bq_t y;

                x += this->y1 * fb * 0.98;
                y = this->b0 * x + this->b1 * this->x1 + this->b2 * this->x2 +
                    this->a1 * this->y1 + this->a2 * this->y2;
                KillDenormal(y);
                this->x2 = this->x1;
                this->x1 = x;
                this->y2 = this->y1;
                this->y1 = y;

                return y;
            }

            inline bq_t ApplyFB(const biquad_param_t* __restrict param, bq_t x, const bq_t fb) {
                bq_t y;

                x += this->y1 * fb * 0.98;
                y = param->b0 * x + param->b1 * this->x1 + param->b2 * this->x2 +
                    param->a1 * this->y1 + param->a2 * this->y2;
                KillDenormal(y);
                this->x2 = this->x1;
                this->x1 = x;
                this->y2 = this->y1;
                this->y1 = y;

                return y;
            }

#if 0 // CONFIG_ASM && ARCH_X86
            // expects to find input in xmm0 (xmm0 stays unmodified) and finally leaves output in xmm7
            inline void ApplyFB4StepsSSE(biquad_param_t* param, const bq_t &fb) {
                float xs, ys;
                float t0, t1, t2, t3, t4, t5, t6, t7, t8; // temporary stack space
                __asm__ __volatile__ (
                    /* prepare input */
                    "movss  %15,%%xmm5\n\t"
                    "movss  %%xmm0,(%14)\n\t"
                    /* sample 0 */
                    "movss   %0, %%xmm3\n\t"
                    "movss   %1, %%xmm4\n\t"
                    "mulss   %%xmm4, %%xmm5\n\t"
                    "movss   %%xmm3, %2\n\t"
                    "movss   %%xmm5, %16\n\t"
                    "mulss   %%xmm3, %%xmm5\n\t"
                    "movss   %19, %%xmm2\n\t"
                    "movss   %3, %%xmm6\n\t"
                    "movss   %21, %%xmm3\n\t"
                    "addss   %%xmm5, %%xmm6\n\t"
                    "movss  %%xmm2, %%xmm5\n\t"
                    "movss   %20, %%xmm4\n\t"
                    "movss   %%xmm6, %4\n\t"
                    "mulss   %%xmm6, %%xmm5\n\t"
                    "movss   %5, %%xmm6\n\t"
                    "movss   %%xmm2, %6\n\t"
                    "movss   %%xmm4, %7\n\t"
                    "movss   %%xmm3, %%xmm2\n\t"
                    "mulss   %%xmm6, %%xmm4\n\t"
                    "mulss   %8, %%xmm2\n\t"
                    "movss   %%xmm3, %9\n\t"
                    "addss   %%xmm4, %%xmm5\n\t"
                    "movss   %18, %%xmm3\n\t"
                    "movss   %17, %%xmm4\n\t"
                    "addss   %%xmm2, %%xmm5\n\t"
                    "movss   %%xmm4, %10\n\t"
                    "movss   %%xmm3, %%xmm2\n\t"
                    "mulss   %11, %%xmm4\n\t"
                    "mulss   %12, %%xmm2\n\t"
                    "movss   %%xmm3, %13\n\t"
                    "addss   %%xmm4, %%xmm5\n\t"
                    "movss   %11, %%xmm3\n\t"
                    "movss   %4, %%xmm4\n\t"
                    "addss   %%xmm2, %%xmm5\n\t"
                    :: "m" (y1),  /* %0 */
                       "m" (fbc), /* %1 */
                       "m" (t0),  /* %2 */
                       "m" (xs),  /* %3 */
                       "m" (t7),  /* %4 */
                       "m" (x1),  /* %5 */
                       "m" (t1),  /* %6 */
                       "m" (t2),  /* %7 */
                       "m" (x2),  /* %8 */
                       "m" (t3),  /* %9 */
                       "m" (t4),  /* %10 */
                       "m" (t0),  /* %11 */
                       "m" (y2),  /* %12 */
                       "m" (t5),  /* %13 */
                       "r" (&xs),  /* %14 */
                       "m" (fb),   /* %15 */
                       "m" (ys),   /* %16 */
                       "m" (param->a1), /* %17 */
                       "m" (param->a2), /* %18 */
                       "m" (param->b0), /* %19 */
                       "m" (param->b1), /* %20 */
                       "m" (param->b2) /* %21 */
                );
                __asm__ __volatile__ (
                    "shufps $0x39,%%xmm0,%%xmm0  # rotate down one cell\n\t"
                    "movss  %%xmm5,%%xmm7\n\t"
                    ::
                );
                /* sample 1 */
                __asm__ __volatile__ (
                    "movss   %0, %%xmm4\n\t"
                    "movss   %%xmm0, %%xmm3\n\t"
                    "mulss   %%xmm5, %%xmm4\n\t"
                    "mulss   %3, %%xmm6\n\t"
                    "movss   %5, %%xmm2\n\t"
                    "addss   %%xmm4, %%xmm3\n\t"
                    "mulss   %7, %%xmm2\n\t"
                    "movss   %6, %%xmm4\n\t"
                    "movss   %%xmm3, %8\n\t"
                    "mulss   %%xmm3, %%xmm4\n\t"
                    "addss   %%xmm2, %%xmm4\n\t"
                    "movss   %9, %%xmm3\n\t"
                    "mulss   %%xmm5, %%xmm3\n\t"
                    "movss   %10, %%xmm2\n\t"
                    "addss   %%xmm6, %%xmm4\n\t"
                    "mulss   %11, %%xmm2\n\t"
                    "addss   %%xmm3, %%xmm4\n\t"
                    "addss   %%xmm2, %%xmm4\n\t"
                    :: "m" (ys),  /* %0 */
                       "m" (fbc), /* %1 */
                       "m" (xs),  /* %2 */
                       "m" (t3),  /* %3 */
                       "m" (y2),  /* %4 */
                       "m" (t2),  /* %5 */
                       "m" (t1),  /* %6 */
                       "m" (t7),  /* %7 */
                       "m" (t8),  /* %8 */
                       "m" (t4),  /* %9 */
                       "m" (t5),  /* %10 */
                       "m" (t0),  /* %11 */
                       "m" (x2),  /* %12 */
                       "m" (x1),  /* %13 */
                       "m" (y1)   /* %14 */
                );
                __asm__ __volatile__ (
                    "shufps $0x93,%%xmm7,%%xmm7  # rotate up one cell\n\t"
                    "shufps $0x39,%%xmm0,%%xmm0  # rotate down one cell\n\t"
                    "movss  %%xmm4,%%xmm7\n\t"
                    ::
                );
                /* sample 2 */
                __asm__ __volatile__ (
                    "movss   %2, %%xmm6\n\t"
                    "movss   %3, %%xmm3\n\t"
                    "mulss   %%xmm4, %%xmm6\n\t"
                    "movss   %4, %%xmm2\n\t"
                    "mulss   %9, %%xmm2\n\t"
                    "addss   %%xmm0, %%xmm6\n\t"
                    "mulss   %7, %%xmm5\n\t"
                    "mulss   %%xmm6, %%xmm3\n\t"
                    "addss   %%xmm2, %%xmm3\n\t"
                    "movss   %5, %%xmm2\n\t"
                    "mulss   %8, %%xmm2\n\t"
                    "addss   %%xmm2, %%xmm3\n\t"
                    "movss   %6, %%xmm2\n\t"
                    "mulss   %%xmm4, %%xmm2\n\t"
                    "addss   %%xmm5, %%xmm2\n\t"
                    "addss   %%xmm2, %%xmm3\n\t"
                    :: "m" (xs),  /* %0 */
                       "m" (fb),  /* %1 */
                       "m" (ys), /* %2 */
                       "m" (t1),  /* %3 */
                       "m" (t2),  /* %4 */
                       "m" (t3),  /* %5 */
                       "m" (t4),  /* %6 */
                       "m" (t5),  /* %7 */
                       "m" (t7),  /* %8 */
                       "m" (t8),  /* %9 */
                       "m" (x1),  /* %10 */
                       "m" (x2),  /* %11 */
                       "m" (y1),  /* %12 */
                       "m" (y2)   /* %13 */
                );
                __asm__ __volatile__ (
                    "shufps $0x39,%%xmm0,%%xmm0  # rotate down one cell\n\t"
                    "shufps $0x93,%%xmm7,%%xmm7  # rotate up one cell\n\t"
                    "movss  %%xmm3,%%xmm7\n\t"
                    ::
                );
                /* sample 3 */
                __asm__ __volatile__ (
                    "movss   %1, %%xmm2\n\t"
                    "mulss   %7, %%xmm4\n\t"
                    "mulss   %%xmm3, %%xmm2\n\t"
                    "movss   %3, %%xmm5\n\t"
                    "movss   %%xmm6, %11\n\t"
                    "addss   %%xmm0, %%xmm2\n\t"
                    "movss   %%xmm3, %13\n\t"
                    "mulss   %%xmm2, %%xmm5\n\t"
                    "mulss   %4, %%xmm6\n\t"
                    "movss   %%xmm2, %10\n\t"
                    "addss   %%xmm6, %%xmm5\n\t"
                    "movss   %5, %%xmm2\n\t"
                    "mulss   %9, %%xmm2\n\t"
                    "mulss   %6, %%xmm3\n\t"
                    "addss   %%xmm2, %%xmm5\n\t"
                    "addss   %%xmm3, %%xmm4\n\t"
                    "addss   %%xmm4, %%xmm5\n\t"
                    "movss   %%xmm5, %12\n\t"
                    :: "m" (xs),  /* %0 */
                       "m" (ys),  /* %1 */
                       "m" (fbc), /* %2 */
                       "m" (t1),  /* %3 */
                       "m" (t2),  /* %4 */
                       "m" (t3),  /* %5 */
                       "m" (t4),  /* %6 */
                       "m" (t5),  /* %7 */
                       "m" (t6),  /* %8 */
                       "m" (t8),  /* %9 */
                       "m" (x1),  /* %10 */
                       "m" (x2),  /* %11 */
                       "m" (y1),  /* %12 */
                       "m" (y2)   /* %13 */
                );
                __asm__ __volatile__ (
                    "shufps $0x93,%%xmm7,%%xmm7  # rotate up one cell\n\t"
                    "shufps $0x39,%%xmm0,%%xmm0  # rotate down one cell to restore original input\n\t"
                    "movss  %%xmm5,%%xmm7\n\t"
                    "shufps $0x1b,%%xmm7,%%xmm7  # swap output to correct order\n\t"
                    ::
                );
            }
#endif // CONFIG_ASM && ARCH_X86
    };

    /** @brief Lowpass Filter
     *
     * Lowpass filter based on biquad filter implementation.
     */
    class LowpassFilter : public BiquadFilter {
        public:
            inline LowpassFilter() : BiquadFilter() {}

            inline void SetParameters(bq_t fc, bq_t bw, bq_t fs) {
                bq_t omega = 2.0 * M_PI * fc / fs;
                bq_t sn    = sin(omega);
                bq_t cs    = cos(omega);
                bq_t alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);

                const float a0r = 1.0 / (1.0 + alpha);
                this->b0 = a0r * (1.0 - cs) * 0.5;
                this->b1 = a0r * (1.0 - cs);
                this->b2 = a0r * (1.0 - cs) * 0.5;
                this->a1 = a0r * (2.0 * cs);
                this->a2 = a0r * (alpha - 1.0);
            }

            inline void SetParameters(biquad_param_t* __restrict param, bq_t fc, bq_t bw, bq_t fs) {
                bq_t omega = 2.0 * M_PI * fc / fs;
                bq_t sn    = sin(omega);
                bq_t cs    = cos(omega);
                bq_t alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);

                const float a0r = 1.0 / (1.0 + alpha);
                param->b0 = a0r * (1.0 - cs) * 0.5;
                param->b1 = a0r * (1.0 - cs);
                param->b2 = a0r * (1.0 - cs) * 0.5;
                param->a1 = a0r * (2.0 * cs);
                param->a2 = a0r * (alpha - 1.0);
            }
    };

    /** @brief Bandpass Filter
     *
     * Bandpass filter based on biquad filter implementation.
     */
    class BandpassFilter : public BiquadFilter {
        public:
            inline BandpassFilter() : BiquadFilter() {}

            inline void SetParameters(bq_t fc, bq_t bw, bq_t fs) {
                bq_t omega = 2.0 * M_PI * fc / fs;
                bq_t sn    = sin(omega);
                bq_t cs    = cos(omega);
                bq_t alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);

                const float a0r = 1.0 / (1.0 + alpha);
                this->b0 = a0r * sn * 0.71;
                this->b1 = 0.0;
                this->b2 = a0r * -sn * 0.71;
                this->a1 = a0r * (2.0 * cs);
                this->a2 = a0r * (alpha - 1.0);
            }

            inline void SetParameters(biquad_param_t* __restrict param, bq_t fc, bq_t bw, bq_t fs) {
                bq_t omega = 2.0 * M_PI * fc / fs;
                bq_t sn    = sin(omega);
                bq_t cs    = cos(omega);
                bq_t alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);

                const float a0r = 1.0 / (1.0 + alpha);
                param->b0 = a0r * sn * 0.71;
                param->b1 = 0.0;
                param->b2 = a0r * -sn * 0.71;
                param->a1 = a0r * (2.0 * cs);
                param->a2 = a0r * (alpha - 1.0);
            }
    };

    /** @brief Highpass Filter
     *
     * Highpass filter based on biquad filter implementation.
     */
    class HighpassFilter : public BiquadFilter {
        public:
            inline HighpassFilter() : BiquadFilter() {}

            inline void SetParameters(bq_t fc, bq_t bw, bq_t fs) {
                bq_t omega = 2.0 * M_PI * fc / fs;
                bq_t sn    = sin(omega);
                bq_t cs    = cos(omega);
                bq_t alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);

                const float a0r = 1.0 / (1.0 + alpha);
                this->b0 = a0r * (1.0 + cs) * 0.5;
                this->b1 = a0r * -(1.0 + cs);
                this->b2 = a0r * (1.0 + cs) * 0.5;
                this->a1 = a0r * (2.0 * cs);
                this->a2 = a0r * (alpha - 1.0);
            }

            inline void SetParameters(biquad_param_t* __restrict param, bq_t fc, bq_t bw, bq_t fs) {
                bq_t omega = 2.0 * M_PI * fc / fs;
                bq_t sn    = sin(omega);
                bq_t cs    = cos(omega);
                bq_t alpha = sn * sinh(M_LN2 / 2.0 * bw * omega / sn);

                const float a0r = 1.0 / (1.0 + alpha);
                param->b0 = a0r * (1.0 + cs) * 0.5;
                param->b1 = a0r * -(1.0 + cs);
                param->b2 = a0r * (1.0 + cs) * 0.5;
                param->a1 = a0r * (2.0 * cs);
                param->a2 = a0r * (alpha - 1.0);
            }
    };

} // namespace LinuxSampler

#endif // __LS_BIQUADFILTER_H__

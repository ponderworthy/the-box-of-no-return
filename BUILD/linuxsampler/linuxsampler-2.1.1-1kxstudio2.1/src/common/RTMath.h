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

#ifndef __RT_MATH_H__
#define __RT_MATH_H__

#include <math.h>
#include <stdint.h>
#include "global_private.h"

/// Needed for calculating frequency ratio used to pitch a sample
#define TWELVEHUNDREDTH_ROOT_OF_TWO	1.000577789506555

enum implementation_t {
    CPP,
    ASM_X86_MMX_SSE
};

/** @brief Real Time Math Base Class
 *
 * Math functions for real time operation. This base class contains all
 * non-template methods.
 */
class RTMathBase {
    public:
        /**
         * High resolution time stamp.
         */
        typedef uint32_t time_stamp_t;

        typedef uint64_t usecs_t;

        /**
         * We read the processor's cycle count register as a reference
         * for the real time. These are of course only abstract values
         * with arbitrary time entity, but that's not a problem as long
         * as we calculate relatively.
         *
         * @see unsafeMicroSeconds()
         */
        static time_stamp_t CreateTimeStamp();

        /**
         * Calculates the frequency ratio for a pitch value given in cents
         * (assuming equal tempered scale of course, divided into 12
         * semitones per octave and 100 cents per semitone).
         *
         * Note: CONFIG_MAX_PITCH (defined in config.h) has to be defined to an
         * appropriate value, otherwise the behavior of this function is
         * undefined, but most probably if CONFIG_MAX_PITCH is too small, the
         * application will crash due to segmentation fault here.
         *
         * @param cents - pitch value in cents (+1200 cents means +1 octave)
         * @returns  frequency ratio (e.g. +2.0 for +1 octave)
         */
        inline static double CentsToFreqRatio(double Cents) {
            int   index_int   = (int) (Cents);      // integer index
            float index_fract = Cents - index_int;  // fractional part of index
            return pCentsToFreqTable[index_int] + index_fract * (pCentsToFreqTable[index_int+1] - pCentsToFreqTable[index_int]);
        }

        /**
         * Slower version of CentsToFreqRatio, for big values.
         *
         * @param cents - pitch value in cents (+1200 cents means +1 octave)
         * @returns  frequency ratio (e.g. +2.0 for +1 octave)
         */
        static double CentsToFreqRatioUnlimited(double Cents) {
            int octaves = int(Cents / 1200);
            double x = CentsToFreqRatio(Cents - octaves * 1200);
            return  octaves < 0 ? x / (1 << -octaves) : x * (1 << octaves);
        }

        /**
         * Inverse function to CentsToFreqRatio(). This function is a bit
         * slow, so it should not be called too frequently.
         */
        static double FreqRatioToCents(double FreqRatio) {
            return log(FreqRatio) / log(TWELVEHUNDREDTH_ROOT_OF_TWO);
        }

        /**
         * Calculates the linear ratio value representation (linear scale)
         * of the @a decibel value provided (exponential scale).
         *
         * The context of audio acoustic sound pressure levels is assumed, and
         * hence the field version of the dB unit is used here (which uses a
         * linear factor of 20). This function is a bit slow, so it should
         * not be called too frequently.
         *
         * @param decibel - sound pressure level in dB
         * @returns linear ratio of the supplied dB value
         * @see LinRatioToDecibel() as inverse function
         */
        static float DecibelToLinRatio(float decibel) {
            return powf(10.f, decibel / 20.f);
        }

        /**
         * Calculates the decibel value (exponential scale) of the @a linear
         * ratio value representation (linear scale) provided.
         *
         * The context of audio acoustic sound pressure levels is assumed, and
         * hence the field version of the dB unit is used here (which uses a
         * linear factor of 20). This function is a bit slow, so it should
         * not be called too frequently.
         *
         * @param linear - sound pressure level as linear ratio value (linear scale)
         * @returns dB value representation
         * @see DecibelToLinRatio() as inverse function
         */
        static float LinRatioToDecibel(float linear) {
            return 20.f * log10f(linear);
        }

        /**
         * Calculates the relatively summed average of a set of values.
         *
         * @param current - the current avaerage value of all previously summed values
         * @param sample - new value to be applied as summed average to the existing values
         * @param n - amount of sample values applied so far
         * @returns new average value of all summed values (including the new @a sample)
         */
        template<typename T_int>
        inline static float RelativeSummedAvg(float current, float sample, T_int n) {
            return current + (sample - current) / float(n);
        }

        /**
         * Clock source to use for getting the current time.
         */
        enum clock_source_t {
            real_clock,    ///< Use this to measure time that passed in reality (no matter if process got suspended).
            process_clock, ///< Use this to measure only the CPU execution time of the current process (if the process got suspended, the clock is paused as well).
            thread_clock,  ///< Use this to measure only the CPU execution time of the current thread (if the process got suspended or another thread is executed, the clock is paused as well).
        };

        /**
         * Returns a time stamp of the current time in microseconds (in
         * probably real-time @b unsafe way). There is no guarantee about
         * what the returned amount of microseconds relates to (i.e.
         * microseconds since epoch, microseconds since system uptime, ...).
         * So you should only use it to calculate time differences between
         * values taken with this method.
         *
         * @b CAUTION: This method may not @b NOT be real-time safe! On some
         * systems it could be RT safe, but there is no guarantee whatsoever!
         * So this method should only be used for debugging, benchmarking and
         * other developing purposes !
         *
         * For creating time stamps in real-time context, use
         * CreateTimeStamp() instead.
         *
         * @param source - the actual clock to use for getting the current
         *                 time, note that the various clock sources may not
         *                 be implemented on all systems
         * @returns time stamp in microseconds
         *
         * @see CreateTimeStamp()
         */
        static usecs_t unsafeMicroSeconds(clock_source_t source);

    private:
        static float* pCentsToFreqTable;

        static float* InitCentsToFreqTable();
};

/** @brief Real Time Math
 *
 * This is a template which provides customized methods for the desired low
 * level implementation. The ASM_X86_MMX_SSE implementation of each method
 * for example doesn't use 387 FPU instruction. This is needed for MMX
 * algorithms which do not allow mixed MMX and 387 instructions.
 */
template<implementation_t IMPL = CPP>
class __RTMath : public RTMathBase {
    public:
        // conversion using truncate
        inline static int Int(const float a) {
            switch (IMPL) {
                #if CONFIG_ASM && ARCH_X86
                case ASM_X86_MMX_SSE: {
                    int ret;
                    asm (
                        "cvttss2si %1, %0  # convert to int\n\t"
                        : "=r" (ret)
                        : "m" (a)
                    );
                    return ret;
                }
                #endif // CONFIG_ASM && ARCH_X86
                default: {
                    return (int) a;
                }
            }
        }

	//for doubles and everything else except floats
        template<class T_a> inline static int Int(const T_a a) {
            return (int) a;
        }

        inline static float Float(const int a) {
            switch (IMPL) {
                #if CONFIG_ASM && ARCH_X86
                case ASM_X86_MMX_SSE: {
                    float ret;
                    asm (
                        "cvtsi2ss %1, %%xmm0  # convert to float\n\t"
                        "movss    %%xmm0,%0   # output\n\t"
                        : "=m" (ret)
                        : "r" (a)
                    );
                    return ret;
                }
                #endif // CONFIG_ASM && ARCH_X86
                default: {
                    return (float) a;
                }
            }
        }

#if 0
        //for everything except ints
        template<class T_a> inline static float Float(T_a a) {
            return (float) a;
        }
#endif

	inline static float Sum(const float& a, const float& b) {
            switch (IMPL) {
                #if CONFIG_ASM && ARCH_X86
                case ASM_X86_MMX_SSE: {
                    float ret;
                    asm (
                        "movss    %1, %%xmm0  # load a\n\t"
                        "addss    %2, %%xmm0  # a + b\n\t"
                        "movss    %%xmm0, %0  # output\n\t"
                        : "=m" (ret)
                        : "m" (a), "m" (b)
                    );
                    return ret;
                }
                #endif // CONFIG_ASM && ARCH_X86
                default: {
                    return (a + b);
                }
            }
        }

        template<class T_a, class T_b> inline static T_a Sum(const T_a a, const T_b b) {
            return (a + b);
        }

        inline static float Sub(const float& a, const float& b) {
            switch (IMPL) {
                #if CONFIG_ASM && ARCH_X86
                case ASM_X86_MMX_SSE: {
                    float ret;
                    asm (
                        "movss    %1, %%xmm0  # load a\n\t"
                        "subss    %2, %%xmm0  # a - b\n\t"
                        "movss    %%xmm0, %0  # output\n\t"
                        : "=m" (ret)
                        : "m" (a), "m" (b)
                    );
                    return ret;
                }
                #endif // CONFIG_ASM && ARCH_X86
                default: {
                    return (a - b);
                }
            }
        }

        template<class T_a, class T_b> inline static T_a Sub(const T_a a, const T_b b) {
            return (a - b);
        }

        inline static float Mul(const float a, const float b) {
            switch (IMPL) {
                #if CONFIG_ASM && ARCH_X86
                case ASM_X86_MMX_SSE: {
                    float ret;
                    asm (
                        "movss    %1, %%xmm0  # load a\n\t"
                        "mulss    %2, %%xmm0  # a * b\n\t"
                        "movss    %%xmm0, %0  # output\n\t"
                        : "=m" (ret)
                        : "m" (a), "m" (b)
                    );
                    return ret;
                }
                #endif // CONFIG_ASM && ARCH_X86
                default: {
                    return (a * b);
                }
            }
        }

        template<class T_a, class T_b> inline static T_a Mul(const T_a a, const T_b b) {
            return (a * b);
        }

        inline static float Div(const float a, const float b) {
            switch (IMPL) {
                #if CONFIG_ASM && ARCH_X86
                case ASM_X86_MMX_SSE: {
                    float ret;
                    asm (
                        "movss    %1, %%xmm0  # load a\n\t"
                        "divss    %2, %%xmm0  # a / b\n\t"
                        "movss    %%xmm0, %0  # output\n\t"
                        : "=m" (ret)
                        : "m" (a), "m" (b)
                    );
                    return ret;
                }
                #endif // CONFIG_ASM && ARCH_X86
                default: {
                    return (a / b);
                }
            }
        }

        template<class T_a, class T_b> inline static T_a Div(const T_a a, const T_b b) {
            return (a / b);
        }

        inline static float Min(const float a, const float b) {
            switch (IMPL) {
                #if CONFIG_ASM && ARCH_X86
                case ASM_X86_MMX_SSE: {
                    float ret;
                    asm (
                        "movss    %1, %%xmm0  # load a\n\t"
                        "minss    %2, %%xmm0  # Minimum(a, b)\n\t"
                        "movss    %%xmm0, %0  # output\n\t"
                        : "=m" (ret)
                        : "m" (a), "m" (b)
                    );
                    return ret;
                }
                #endif // CONFIG_ASM && ARCH_X86
                default: {
                    return std::min(a, b);
                }
            }
        }

        template<class T_a, class T_b> inline static T_a Min(const T_a a, const T_b b) {
            return (b < a) ? b : a;
        }

        inline static float Max(const float a, const float b) {
            switch (IMPL) {
                #if CONFIG_ASM && ARCH_X86
                case ASM_X86_MMX_SSE: {
                    float ret;
                    asm (
                        "movss    %1, %%xmm0  # load a\n\t"
                        "maxss    %2, %%xmm0  # Maximum(a, b)\n\t"
                        "movss    %%xmm0, %0  # output\n\t"
                        : "=m" (ret)
                        : "m" (a), "m" (b)
                    );
                    return ret;
                }
                #endif // CONFIG_ASM && ARCH_X86
                default: {
                    return std::max(a, b);
                }
            }
        }

        template<class T_a, class T_b> inline static T_a Max(const T_a a, const T_b b) {
            return (b > a) ? b : a;
        }

        inline static float Fmodf(const float &a, const float &b) {
            switch (IMPL) {
                #if CONFIG_ASM && ARCH_X86
                case ASM_X86_MMX_SSE: {
                    float ret;
                    asm (
                        "movss    %1, %%xmm0  # load a\n\t"
                        "movss    %2, %%xmm1  # load b\n\t"
                        "movss    %%xmm0,%%xmm2\n\t"
                        "divss    %%xmm1, %%xmm2  # xmm2 = a / b\n\t"
                        "cvttss2si %%xmm2, %%ecx  #convert to int\n\t"
                        "cvtsi2ss %%ecx, %%xmm2  #convert back to float\n\t"
                        "mulss    %%xmm1, %%xmm2  # xmm2 = b * int(a/b)\n\t"
                        "subss    %%xmm2, %%xmm0  #sub a\n\t"
                        "movss    %%xmm0, %0  # output\n\t"
                        : "=m" (ret)
                        : "m" (a), "m" (b)
                        : "%ecx"
                    );
                    return ret;
                }
                #endif // CONFIG_ASM && ARCH_X86
                default: {
                    return fmodf(a, b);
                }
            }
        }
};

/// convenience typedef for using the default implementation (which is CPP)
typedef __RTMath<> RTMath;

#endif // __RT_MATH_H__

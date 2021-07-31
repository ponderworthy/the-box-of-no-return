/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 Christian Schoenebeck                              *
 *   Copyright (C) 2006-2017 Christian Schoenebeck and Andreas Persson     *
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

#ifndef __LS_GIG_FILTER_H__
#define __LS_GIG_FILTER_H__

#include "../../common/global.h"

#if AC_APPLE_UNIVERSAL_BUILD
# include <libgig/gig.h>
#else
# include <gig.h>
#endif

#include <cmath>

/* TODO: This file contains both generic filters (used by the sfz
   engine) and gig specific filters. It should probably be split up,
   and the generic parts should be moved out of the gig directory. */

/*
 * The formulas for the biquad coefficients come from Robert
 * Bristow-Johnson's Audio EQ Cookbook. The one pole filter formulas
 * come from a post on musicdsp.org. The one poles, biquads and
 * cascaded biquads are modeled after output from Dimension LE and SFZ
 * Player. The gig filters are modeled after output from GigaStudio.
 */
namespace LinuxSampler {

    /**
     * Filter state and parameters for a biquad filter.
     */
    class BiquadFilterData {
    public:
        float b0, b1, b2;
        float a1, a2;

        float x1, x2;
        float y1, y2;
    };

    /**
     * Filter state and parameters for cascaded biquad filters and gig
     * engine filters.
     */
    class FilterData : public BiquadFilterData
    {
    public:
        union {
            // gig filter parameters
            struct {
                float a3;
                float x3;
                float y3;

                float scale;
                float b20;
                float y21, y22, y23;
            };
            // cascaded biquad parameters
            struct {
                BiquadFilterData d2;
                BiquadFilterData d3;
            };
        };
    };

    /**
     * Abstract base class for all filter implementations.
     */
    class FilterBase {
    public:
        virtual float Apply(FilterData& d, float x) const = 0;
        virtual void SetParameters(FilterData& d, float fc, float r,
                                   float fs) const = 0;
        virtual void Reset(FilterData& d) const = 0;
    protected:
        void KillDenormal(float& f) const {
            f += 1e-18f;
            f -= 1e-18f;
        }
    };

    /**
     * One-pole lowpass filter.
     */
    class LowpassFilter1p : public FilterBase {
    public:
        LowpassFilter1p() { }

        float Apply(FilterData& d, float x) const {
            float y = x + d.a1 * (x - d.y1); // d.b0 * x - d.a1 * d.y1;
            KillDenormal(y);
            d.y1 = y;
            return y;
        }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            float omega = 2.0 * M_PI * fc / fs;
            float c     = 2 - cos(omega);
            d.a1 = -(c - sqrt(c * c - 1));
            // d.b0 = 1 + d.a1;
        }

        void Reset(FilterData& d) const {
            d.y1 = 0;
        }
    };

    /**
     * One pole highpass filter.
     */
    class HighpassFilter1p : public FilterBase {
    public:
        HighpassFilter1p() { }

        float Apply(FilterData& d, float x) const {
            // d.b0 * x + d.b1 * d.x1 - d.a1 * d.y1;
            float y = d.a1 * (-x + d.x1 - d.y1);
            KillDenormal(y);
            d.x1 = x;
            d.y1 = y;
            return y;
        }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            float omega = 2.0 * M_PI * fc / fs;
            float c     = 2 - cos(omega);
            d.a1 = -(c - sqrt(c * c - 1));
            // d.b0 = -d.a1
            // d.b1 = d.a1
        }

        void Reset(FilterData& d) const {
            d.x1 = 0;
            d.y1 = 0;
        }
    };

    /**
     * Base class for biquad filter implementations.
     */
    class BiquadFilter : public FilterBase {
    protected:
        float ApplyBQ(BiquadFilterData& d, float x) const {
            float y = d.b0 * x + d.b1 * d.x1 + d.b2 * d.x2 + 
                d.a1 * d.y1 + d.a2 * d.y2;
            KillDenormal(y);
            d.x2 = d.x1;
            d.x1 = x;
            d.y2 = d.y1;
            d.y1 = y;
            return y;
        }

    public:
        float Apply(FilterData& d, float x) const {
            return ApplyBQ(d, x);
        }

        void Reset(FilterData& d) const {
            d.x1 = d.x2 = 0;
            d.y1 = d.y2 = 0;
        }
    };

    /**
     * Base class for cascaded double biquad filter (four poles).
     */
    class DoubleBiquadFilter : public BiquadFilter {
    public:
        float Apply(FilterData& d, float x) const {
            return ApplyBQ(d.d2, BiquadFilter::Apply(d, x));
        }

        void Reset(FilterData& d) const {
            BiquadFilter::Reset(d);
            d.d2.x1 = d.d2.x2 = 0;
            d.d2.y1 = d.d2.y2 = 0;
        }
    };

    /**
     * Base class for cascaded triple biquad filter (six poles).
     */
    class TripleBiquadFilter : public DoubleBiquadFilter {
    public:
        float Apply(FilterData& d, float x) const {
            return ApplyBQ(d.d3, DoubleBiquadFilter::Apply(d, x));
        }

        void Reset(FilterData& d) const {
            DoubleBiquadFilter::Reset(d);
            d.d3.x1 = d.d3.x2 = 0;
            d.d3.y1 = d.d3.y2 = 0;
        }
    };


    /** @brief Lowpass Filter
     *
     * Lowpass filter based on biquad filter implementation.
     */
    class LowpassFilter : public BiquadFilter {
    public:
        LowpassFilter() { }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            float omega = 2.0 * M_PI * fc / fs;
            float sn    = sin(omega);
            float cs    = cos(omega);
            float alpha = sn * M_SQRT1_2 * exp(-M_LN10 / 20 * r);
            float a0r = 1.0 / (1.0 + alpha);

            d.b0 = a0r * (1.0 - cs) * 0.5;
            d.b1 = a0r * (1.0 - cs);
            d.b2 = a0r * (1.0 - cs) * 0.5;
            d.a1 = a0r * (2.0 * cs);
            d.a2 = a0r * (alpha - 1.0);
        }
    };

    /** @brief Four pole lowpass filter
     *
     * Lowpass filter based on two cascaded biquad filters.
     */
    class LowpassFilter4p : public DoubleBiquadFilter {
    public:
        LowpassFilter4p() { }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            float omega = 2.0 * M_PI * fc / fs;
            float sn    = sin(omega);
            float cs    = cos(omega);
            float alpha = sn * M_SQRT1_2;
            float a0r = 1.0 / (1.0 + alpha);

            d.b0 = a0r * (1.0 - cs) * 0.5;
            d.b1 = a0r * (1.0 - cs);
            d.b2 = a0r * (1.0 - cs) * 0.5;
            d.a1 = a0r * (2.0 * cs);
            d.a2 = a0r * (alpha - 1.0);

            alpha *= exp(-M_LN10 / 20 * r);
            a0r = 1.0 / (1.0 + alpha);

            d.d2.b0 = a0r * (1.0 - cs) * 0.5;
            d.d2.b1 = a0r * (1.0 - cs);
            d.d2.b2 = a0r * (1.0 - cs) * 0.5;
            d.d2.a1 = a0r * (2.0 * cs);
            d.d2.a2 = a0r * (alpha - 1.0);
        }
    };

    /** @brief Six pole lowpass filter
     *
     * Lowpass filter based on three cascaded biquad filters.
     */
    class LowpassFilter6p : public TripleBiquadFilter {
    public:
        LowpassFilter6p() { }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            float omega = 2.0 * M_PI * fc / fs;
            float sn    = sin(omega);
            float cs    = cos(omega);
            float alpha = sn * M_SQRT1_2;
            float a0r = 1.0 / (1.0 + alpha);

            d.b0 = d.d2.b0 = a0r * (1.0 - cs) * 0.5;
            d.b1 = d.d2.b1 = a0r * (1.0 - cs);
            d.b2 = d.d2.b2 = a0r * (1.0 - cs) * 0.5;
            d.a1 = d.d2.a1 = a0r * (2.0 * cs);
            d.a2 = d.d2.a2 = a0r * (alpha - 1.0);

            alpha *= exp(-M_LN10 / 20 * r);
            a0r = 1.0 / (1.0 + alpha);

            d.d3.b0 = a0r * (1.0 - cs) * 0.5;
            d.d3.b1 = a0r * (1.0 - cs);
            d.d3.b2 = a0r * (1.0 - cs) * 0.5;
            d.d3.a1 = a0r * (2.0 * cs);
            d.d3.a2 = a0r * (alpha - 1.0);
        }
    };

    /** @brief Bandpass filter
     *
     * Bandpass filter based on biquad filter implementation.
     */
    class BandpassFilter : public BiquadFilter {
    public:
        BandpassFilter() { }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            float omega = 2.0 * M_PI * fc / fs;
            float sn    = sin(omega);
            float cs    = cos(omega);
            float alpha = sn * M_SQRT1_2 * exp(-M_LN10 / 20 * r);

            float a0r = 1.0 / (1.0 + alpha);
            d.b0 = a0r * alpha;
            d.b1 = 0.0;
            d.b2 = a0r * -alpha;
            d.a1 = a0r * (2.0 * cs);
            d.a2 = a0r * (alpha - 1.0);
        }
    };

    /** @brief Bandreject filter
     *
     * Bandreject filter based on biquad filter implementation.
     */
    class BandrejectFilter : public BiquadFilter {
    public:
        BandrejectFilter() { }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            float omega = 2.0 * M_PI * fc / fs;
            float sn    = sin(omega);
            float cs    = cos(omega);
            float alpha = sn * M_SQRT1_2 * exp(-M_LN10 / 20 * r);

            float a0r = 1.0 / (1.0 + alpha);
            d.b0 = a0r;
            d.b1 = a0r * (-2.0 * cs);
            d.b2 = a0r;
            d.a1 = a0r * (2.0 * cs);
            d.a2 = a0r * (alpha - 1.0);
        }
    };

    /** @brief Highpass filter
     *
     * Highpass filter based on biquad filter implementation.
     */
    class HighpassFilter : public BiquadFilter {
    public:
        HighpassFilter() { }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            float omega = 2.0 * M_PI * fc / fs;
            float sn    = sin(omega);
            float cs    = cos(omega);
            float alpha = sn * M_SQRT1_2 * exp(-M_LN10 / 20 * r);

            float a0r = 1.0 / (1.0 + alpha);
            d.b0 = a0r * (1.0 + cs) * 0.5;
            d.b1 = a0r * -(1.0 + cs);
            d.b2 = a0r * (1.0 + cs) * 0.5;
            d.a1 = a0r * (2.0 * cs);
            d.a2 = a0r * (alpha - 1.0);
        }
    };

    /** @brief Four pole highpass filter
     *
     * Highpass filter based on three cascaded biquad filters.
     */
    class HighpassFilter4p : public DoubleBiquadFilter {
    public:
        HighpassFilter4p() { }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            float omega = 2.0 * M_PI * fc / fs;
            float sn    = sin(omega);
            float cs    = cos(omega);
            float alpha = sn * M_SQRT1_2;

            float a0r = 1.0 / (1.0 + alpha);
            d.b0 = a0r * (1.0 + cs) * 0.5;
            d.b1 = a0r * -(1.0 + cs);
            d.b2 = a0r * (1.0 + cs) * 0.5;
            d.a1 = a0r * (2.0 * cs);
            d.a2 = a0r * (alpha - 1.0);

            alpha *= exp(-M_LN10 / 20 * r);
            a0r = 1.0 / (1.0 + alpha);

            d.d2.b0 = a0r * (1.0 + cs) * 0.5;
            d.d2.b1 = a0r * -(1.0 + cs);
            d.d2.b2 = a0r * (1.0 + cs) * 0.5;
            d.d2.a1 = a0r * (2.0 * cs);
            d.d2.a2 = a0r * (alpha - 1.0);
        }
    };

    /** @brief Six pole highpass filter
     *
     * Highpass filter based on three cascaded biquad filters.
     */
    class HighpassFilter6p : public TripleBiquadFilter {
    public:
        HighpassFilter6p() { }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            float omega = 2.0 * M_PI * fc / fs;
            float sn    = sin(omega);
            float cs    = cos(omega);
            float alpha = sn * M_SQRT1_2;

            float a0r = 1.0 / (1.0 + alpha);
            d.b0 = d.d2.b0 = a0r * (1.0 + cs) * 0.5;
            d.b1 = d.d2.b1 = a0r * -(1.0 + cs);
            d.b2 = d.d2.b2 = a0r * (1.0 + cs) * 0.5;
            d.a1 = d.d2.a1 = a0r * (2.0 * cs);
            d.a2 = d.d2.a2 = a0r * (alpha - 1.0);

            alpha *= exp(-M_LN10 / 20 * r);
            a0r = 1.0 / (1.0 + alpha);

            d.d3.b0 = a0r * (1.0 + cs) * 0.5;
            d.d3.b1 = a0r * -(1.0 + cs);
            d.d3.b2 = a0r * (1.0 + cs) * 0.5;
            d.d3.a1 = a0r * (2.0 * cs);
            d.d3.a2 = a0r * (alpha - 1.0);
        }
    };

namespace gig {

    /**
     * Base class for the gig engine filters.
     */
    class GigFilter : public FilterBase {
    public:
        void Reset(FilterData& d) const {
            d.x1 = d.x2 = d.x3 = 0;
            d.y1 = d.y2 = d.y3 = 0;
        }
    protected:
        float ApplyA(FilterData& d, float x) const {
            float y = x - d.a1 * d.y1 - d.a2 * d.y2 - d.a3 * d.y3;
            KillDenormal(y);
            d.y3 = d.y2;
            d.y2 = d.y1;
            d.y1 = y;
            return y;
        }
    };

#define GIG_PARAM_INIT                                                  \
    float f1 = fc * 0.0075279;                                          \
    float f2 = f1 - 1 + r * fc * (-5.5389e-5 + 1.1982e-7 * fc);         \
    float scale = r < 51 ? 1.0f : 1.3762f - 0.0075073f * r

    class LowpassFilter : public GigFilter {
    public:
        LowpassFilter() { }

        float Apply(FilterData& d, float x) const {
            return ApplyA(d, d.b0 * x);
        }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            GIG_PARAM_INIT;

            float f1_2 = f1 * f1;
            d.b0 = f1_2 * scale;
            d.a1 = f2;
            d.a2 = f1_2 - 1;
            d.a3 = -f2;
        }
    };

    class BandpassFilter : public GigFilter {
    public:
        BandpassFilter() { }

        float Apply(FilterData& d, float x) const {
            float y = ApplyA(d, d.b0 * x + d.b2 * d.x2);
            d.x2 = d.x1;
            d.x1 = x;
            return y;
        }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            GIG_PARAM_INIT;

            d.b0 = f1 * scale;
            d.b2 = -d.b0;
            d.a1 = f2;
            d.a2 = f1 * f1 - 1;
            d.a3 = -f2;
        }
    };

    class HighpassFilter : public GigFilter {
    public:
        HighpassFilter() { }

        float Apply(FilterData& d, float x) const {
            float y = ApplyA(d, -x + d.x1 + d.x2 - d.x3);
            d.x3 = d.x2;
            d.x2 = d.x1;
            d.x1 = x;
            return y * d.scale;
        }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            GIG_PARAM_INIT;

            d.a1 = f2;
            d.a2 = f1 * f1 - 1;
            d.a3 = -f2;
            d.scale = scale;
        }
    };

    class BandrejectFilter : public GigFilter {
    public:
        BandrejectFilter() { }

        float Apply(FilterData& d, float x) const {
            float y = ApplyA(d, x - d.x1 + d.b2 * d.x2 + d.x3);
            d.x3 = d.x2;
            d.x2 = d.x1;
            d.x1 = x;
            return y * d.scale;
        }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            GIG_PARAM_INIT;

            d.b2 = f1 * f1 - 1;
            d.a1 = f2;
            d.a2 = d.b2;
            d.a3 = -f2;
            d.scale = scale;
        }
    };

    class LowpassTurboFilter : public LowpassFilter {
    public:
        LowpassTurboFilter() { }

        float Apply(FilterData& d, float x) const {
            float y = d.b20 * LowpassFilter::Apply(d, x)
                - d.a1 * d.y21 - d.a2 * d.y22 - d.a3 * d.y23;
            KillDenormal(y);
            d.y23 = d.y22;
            d.y22 = d.y21;
            d.y21 = y;
            return y;
        }

        void SetParameters(FilterData& d, float fc, float r, float fs) const {
            if (fc < 1.f) fc = 1.f; // this lowpass turbo filter cannot cope with cutoff being zero (would cause click sounds)
            LowpassFilter::SetParameters(d, fc, r, fs);
            d.b20 = d.b0 * 0.5;
        }
    };
} //namespace gig


    /**
     * Main filter class.
     */
    class Filter {
        protected:
            static const LowpassFilter1p  lp1p;
            static const LowpassFilter    lp2p;
            static const LowpassFilter4p  lp4p;
            static const LowpassFilter6p  lp6p;
            static const BandpassFilter   bp2p;
            static const BandrejectFilter br2p;
            static const HighpassFilter1p hp1p;
            static const HighpassFilter   hp2p;
            static const HighpassFilter4p hp4p;
            static const HighpassFilter6p hp6p;
            /**
             * These are filters similar to the ones from Gigasampler.
             */
            static const gig::HighpassFilter     HPFilter;
            static const gig::BandpassFilter     BPFilter;
            static const gig::LowpassFilter      LPFilter;
            static const gig::BandrejectFilter   BRFilter;
            static const gig::LowpassTurboFilter LPTFilter;

            FilterData d;
            const FilterBase* pFilter;

        public:
            Filter() {
                // set filter type to 'lowpass' by default
                pFilter = &LPFilter;
                pFilter->Reset(d);
            }

            enum vcf_type_t {
                vcf_type_gig_lowpass = ::gig::vcf_type_lowpass,
                vcf_type_gig_lowpassturbo = ::gig::vcf_type_lowpassturbo,
                vcf_type_gig_bandpass = ::gig::vcf_type_bandpass,
                vcf_type_gig_highpass = ::gig::vcf_type_highpass,
                vcf_type_gig_bandreject = ::gig::vcf_type_bandreject,
                vcf_type_1p_lowpass,
                vcf_type_1p_highpass,
                vcf_type_2p_lowpass,
                vcf_type_2p_highpass,
                vcf_type_2p_bandpass,
                vcf_type_2p_bandreject,
                vcf_type_4p_lowpass,
                vcf_type_4p_highpass,
                vcf_type_6p_lowpass,
                vcf_type_6p_highpass
            };

            void SetType(vcf_type_t FilterType) {
                switch (FilterType) {
                    case vcf_type_gig_highpass:
                        pFilter = &HPFilter;
                        break;
                    case vcf_type_gig_bandreject:
                        pFilter = &BRFilter;
                        break;
                    case vcf_type_gig_bandpass:
                        pFilter = &BPFilter;
                        break;
                    case vcf_type_gig_lowpassturbo:
                        pFilter = &LPTFilter;
                        break;
                    case vcf_type_1p_lowpass:
                        pFilter = &lp1p;
                        break;
                    case vcf_type_1p_highpass:
                        pFilter = &hp1p;
                        break;
                    case vcf_type_2p_lowpass:
                        pFilter = &lp2p;
                        break;
                    case vcf_type_2p_highpass:
                        pFilter = &hp2p;
                        break;
                    case vcf_type_2p_bandpass:
                        pFilter = &bp2p;
                        break;
                    case vcf_type_2p_bandreject:
                        pFilter = &br2p;
                        break;
                    case vcf_type_4p_lowpass:
                        pFilter = &lp4p;
                        break;
                    case vcf_type_4p_highpass:
                        pFilter = &hp4p;
                        break;
                    case vcf_type_6p_lowpass:
                        pFilter = &lp6p;
                        break;
                    case vcf_type_6p_highpass:
                        pFilter = &hp6p;
                        break;
                    default:
                        pFilter = &LPFilter;
                }
                pFilter->Reset(d);
            }

            void SetParameters(float cutoff, float resonance, float fs) {
                pFilter->SetParameters(d, cutoff, resonance, fs);
            }

            void Reset() {
                return pFilter->Reset(d);
            }

            float Apply(float in) {
                return pFilter->Apply(d, in);
            }
    };

} //namespace LinuxSampler

#endif // __LS_GIG_FILTER_H__

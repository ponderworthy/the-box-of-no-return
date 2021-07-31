/*
 * Copyright (c) 2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_FADE_H
#define LS_FADE_H

#include "EaseInEaseOut.h"

namespace LinuxSampler {

#define DEFAULT_FADE_CURVE FADE_CURVE_EASE_IN_EASE_OUT

enum fade_curve_t {
    FADE_CURVE_LINEAR,
    FADE_CURVE_EASE_IN_EASE_OUT,
};

/** @brief Fades a value with a selected curve.
 *
 * Implements a smoothed transition from one value to another value in a
 * requested amount of time, either with an ease-in and ease-out algorithm 
 * (default), or linear.
 */
class Fade : public EaseInEaseOut {
public:
    Fade() : EaseInEaseOut() {
        curveType = DEFAULT_FADE_CURVE;
    }

    /**
     * Select another curve type for this Fade and immediately prepare this
     * Fade for operation with the new curve type.
     */
    void setCurve(fade_curve_t curve, float sampleRate) {
        curveType = curve;
        fadeTo(endValue, duration, sampleRate);
    }

    /**
     * Only set the new curve type. It is your responsibility to prepare this
     * Fade for subsequent operation, i.e. by calling fadeTo() afterwards,
     * otherwise the result of render() calls will be undefined.
     */
    void setCurveOnly(fade_curve_t curve) {
        curveType = curve;
    }

    /**
     * Fade the current value in @duration seconds to the new value reflected
     * by @a endValue, assuming the given @a sampleRate.
     */
    void fadeTo(float endValue, float duration, float sampleRate) {
        if (curveType == FADE_CURVE_EASE_IN_EASE_OUT)
            return EaseInEaseOut::fadeTo(endValue, duration, sampleRate);
        else {
            if (duration <= 0.f) {
                setCurrentValue(endValue);
                return;
            }
            this->endValue = endValue;
            steps = duration * sampleRate;
            c = (endValue - value) / float(steps);
        }
    }

    /**
     * Fade the current value in the currently set default duration to the new
     * value reflected by @a endValue, assuming the given @a sampleRate.
     */
    void fadeTo(float endValue, float sampleRate) {
        fadeTo(endValue, duration, sampleRate);
    }

    /**
     * Proceed transition by exactly one sample point and return the current
     * smoothed value at this stage.
     */
    inline float render() {
        if (curveType == FADE_CURVE_EASE_IN_EASE_OUT)
            return EaseInEaseOut::render();
        if (!steps) return endValue;
        --steps;
        value += c;
        return value;
    }

private:
    fade_curve_t curveType;
};

} // namespace LinuxSampler

#endif // LS_FADE_H

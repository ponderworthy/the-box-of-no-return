/*
 * Copyright (c) 2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_EASEINEASEOUT_H
#define LS_EASEINEASEOUT_H

#include <stdint.h>

namespace LinuxSampler {

/** @brief Value transition with ease-in and ease-out (sinusoidial) curve.
 *
 * Implements a smoothed transition from one value to another value in a
 * requested amount of time with an ease-in and ease-out algorithm,
 * based on a sinusoidial shaped curve.
 */
class EaseInEaseOut {
public:
    EaseInEaseOut() {
        steps = 0;
        value = 1.f;
        endValue = 1.f;
        duration = 1.f;
    }

    /**
     * Set the current value @b immediately to the given value.
     */
    void setCurrentValue(float value) {
        this->value = value;
        endValue = value;
        steps = 0;
    }

    inline float currentValue() const {
        return value;
    }

    /**
     * Sets a new default duration for fadeTo(), which will be used if no
     * duration was passed to fadeTo() calls.
     *
     * @param defaultDuration - default duration for fadeTo() in seconds
     */
    void setDefaultDuration(float defaultDuration) {
        this->duration = defaultDuration;
    }

    /**
     * Fade the current value in @duration seconds to the new value reflected
     * by @a endValue, assuming the given @a sampleRate.
     */
    void fadeTo(float endValue, float duration, float sampleRate) {
        if (duration <= 0.f) {
            setCurrentValue(endValue);
            return;
        }
        this->endValue = endValue;
        steps = duration * sampleRate;
        c = M_PI / duration / sampleRate;
        denormalizer = (value - endValue) * 0.5f;
        offset = endValue + denormalizer;
        real = 1;
        imag = 0;
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
        if (!steps) return endValue;
        --steps;
        value = offset + real * denormalizer;
        real -= c * imag;
        imag += c * real;
        return value;
    }

protected:
    int64_t steps;
    float value;
    float endValue;
    float real;
    float imag;
    float c;
    float denormalizer;
    float offset;
    float duration;
};

} // namespace LinuxSampler

#endif // LS_EASEINEASEOUT_H

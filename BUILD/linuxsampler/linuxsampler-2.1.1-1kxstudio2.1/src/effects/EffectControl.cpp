/*
    Copyright (C) 2010 Christian Schoenebeck
*/

#include "EffectControl.h"

namespace LinuxSampler {

EffectControl::EffectControl() {
    value = 0.0f;
    type = EFFECT_TYPE_BOOL;
}

EffectControl::~EffectControl() {
}

void EffectControl::SetValue(float val) throw (Exception) {
    if (minValue && val < *minValue)
        throw Exception("Effect control value smaller than minimum allowed value");
    if (maxValue && val > *maxValue)
        throw Exception("Effect control value greater than maximum allowed value");
    value = val;
}

float& EffectControl::Value() {
    return value;
}

EffectControl::Type_t EffectControl::Type() const {
    return type;
}

String EffectControl::TypeAsString() const {
    switch (type) {
        case EFFECT_TYPE_FLOAT:
            return "FLOAT";
        case EFFECT_TYPE_INT:
            return "INT";
        case EFFECT_TYPE_BOOL:
            return "BOOL";
        default:
            return "INVALID";
    }
}

String EffectControl::Description() const {
    return description;
}

optional<float> EffectControl::DefaultValue() const {
    return defaultValue;
}

optional<float> EffectControl::MinValue() const {
    return minValue;
}

optional<float> EffectControl::MaxValue() const {
    return maxValue;
}

std::vector<float> EffectControl::Possibilities() const {
    return possibilities;
}

void EffectControl::SetDefaultValue(float val) {
    defaultValue = val;
}

void EffectControl::SetMinValue(float val) {
    minValue = val;
}

void EffectControl::SetMaxValue(float val) {
    maxValue = val;
}

void EffectControl::SetPossibilities(const std::vector<float>& v) {
    possibilities = v;
}

void EffectControl::SetType(Type_t t) {
    type = t;
}

void EffectControl::SetDescription(String s) {
    description = s;
}

} // namespace LinuxSampler

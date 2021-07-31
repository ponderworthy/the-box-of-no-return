/*
    Copyright (C) 2010 Christian Schoenebeck
*/

#ifndef LS_EFFECTCONTROL_H
#define LS_EFFECTCONTROL_H

#include "../common/Exception.h"
#include "../common/optional.h"
#include <vector>

namespace LinuxSampler {

// just symbol prototyping
class Effect;

/**
 * Represents an effect parameter. As the set of parameters an effect offers,
 * varies quite a bit, this class provides necessary informations about the
 * respective effect parameter.
 */
class EffectControl {
public:
    enum Type_t {
        EFFECT_TYPE_FLOAT,
        EFFECT_TYPE_INT,
        EFFECT_TYPE_BOOL
    };

    EffectControl();
    virtual ~EffectControl();
    virtual void SetValue(float val) throw (Exception);
    virtual float& Value();
    Type_t Type() const;
    String TypeAsString() const;
    String Description() const;
    optional<float> DefaultValue() const;
    optional<float> MinValue() const;
    optional<float> MaxValue() const;
    std::vector<float> Possibilities() const;

protected:
    void SetDefaultValue(float val);
    void SetMinValue(float val);
    void SetMaxValue(float val);
    void SetType(Type_t t);
    void SetDescription(String s);
    void SetPossibilities(const std::vector<float>& v);

    friend class Effect;

private:
    float value;
    Type_t type;
    String description;
    optional<float> defaultValue;
    optional<float> minValue;
    optional<float> maxValue;
    std::vector<float> possibilities;
};

} // namespace LinuxSampler

#endif // LS_EFFECTCONTROL_H

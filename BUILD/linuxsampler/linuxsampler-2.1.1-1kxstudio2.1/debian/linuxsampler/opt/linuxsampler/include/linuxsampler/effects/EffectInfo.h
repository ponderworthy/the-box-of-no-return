/*
    Copyright (C) 2010 Christian Schoenebeck
*/

#ifndef _EFFECT_INFO_H_
#define _EFFECT_INFO_H_

namespace LinuxSampler {

/**
 * General informations about a specific effect. At the moment it is mainly
 * used as unique identifier of a certain effect. In future it might also show
 * further informations like amount of input / output channels, copyright
 * informations etc.
 *
 * This abstract base class is implemented by the respective internal effect
 * system.
 *
 * @see EffectFactory::GetEffectInfo()
 */
class EffectInfo {
public:
    /**
     * The underlying effect system the effect is based on, e.g. "LADSPA".
     */
    virtual String EffectSystem() = 0;

    /**
     * Unique name of the effect within its module (plugin DLL).
     */
    virtual String Name() = 0;

    /**
     * Name of the module (usually the DLL file name) that contains the effect.
     */
    virtual String Module() = 0;

    /**
     * Human readable name or description of the effect (not necessarily
     * unique).
     */
    virtual String Description() = 0;

    virtual ~EffectInfo() { }
};

} // namespace LinuxSampler

#endif // _EFFECT_INFO_H_

/*
    Copyright (C) 2010 Christian Schoenebeck
*/

#ifndef LS_EFFECT_FACTORY_H
#define LS_EFFECT_FACTORY_H

#include "Effect.h"
#include "EffectInfo.h"
#include "../common/Exception.h"

namespace LinuxSampler {

/**
 * Manages all sampler internal effects. It offers a list and detailed
 * informations about all internal effects available for the sampler and allows
 * to create and destroy instances of those effects. It provides a general
 * interface for these functions, independent of the respective effect system's
 * implementation details.
 */
class EffectFactory {
public:
    /**
     * Used as search criteria for plugin DLL filenames by GetEffectInfo() .
     * These flags can be bitwise or-ed.
     */
    enum ModuleMatchFlag_t {
        MODULE_MATCH_EXACTLY = 0,    ///< The plugin DLL filename has to match exactly.
        MODULE_IGNORE_CASE = 1,      ///< Ignore upper case / lower case differences in the module names.
        MODULE_IGNORE_EXTENSION = 2, ///< Strip off the file extension (if any) before comparing the DLL filenames.
        MODULE_IGNORE_PATH = 4,      ///< Strip off the path of the plugin DLL filename before comparing.
        MODULE_IGNORE_ALL = -1       ///< Will match all modules successfully, no matter what module name was provided.
    };

    /**
     * Returns comma separated list of all effect systems currently available
     * for this sampler. This list can vary on the exact configuration of the
     * running machine and the options with which the sampler was compiled.
     */
    static String AvailableEffectSystemsAsString();

    /**
     * Returns total amount of effects currently available for the sampler. This
     * value can vary on the exact configuration of the running machine and
     * especially on which effect plugins are currently installed on the machine.
     *
     * @e Note: When this method is called the 1st tim, it can take quite some
     * time to return, since it will trigger a scan to retrieve all effects,
     * currently available on the system. Those informations are then buffered
     * in RAM and won't change on subsequent calls to AvailableEffectsCount()
     * until an update of available effects is forced by calling
     * UpdateAvailableEffects() .
     *
     * @see UpdateAvailableEffects()
     */
    static uint AvailableEffectsCount();

    /**
     * Force to refresh the internal list of available effects and their
     * detailed informations. This might be necessary e.g. when the user
     * installed new effect plugins on his system and doesn't want to restart
     * the whole sampler session just for being able to access the new effects.
     */
    static void UpdateAvailableEffects();

    /**
     * Returns unique identifier and further detailed informations about the
     * requested effect.
     *
     * @param index - index of the effect to retrieve informations about, must
     *                be between 0 and AvailableEffectsCount()
     * @see UpdateAvailableEffects()
     */
    static EffectInfo* GetEffectInfo(uint index);

    /**
     * Returns unique identifier and further detailed informations about the
     * the requested effect. Since the effect plugin's DLL filename (a.k.a.
     * @a module) can vary for the very same effect on different systems, it
     * is possible to use @a iModuleMatchFlags for defining in which way the
     * provided DLL filename should be matched in this search or whether it
     * should even be ignored completely. This allows to reload sampler sessions
     * created on one system, successfully on another system, probably even
     * having a completely different OS (e.g. Linux vs. Windows).
     *
     * @param effectSystem - e.g. 'LADSPA'
     * @param module - usually the DLL filename of the effect plugin
     * @param effectName - unique identifier of the effect within its plugin module
     * @param iModuleMatchFlags - (optional) defines how strong the given
     *                            @a module must match with the actual on the
     *                            system available module name, the flags can
     *                            be bitwise or-ed
     *                            (default: the module name must match exactly)
     * @returns effect info or NULL if there is no such effect
     * @see ModuleMatchFlag_t
     */
    static EffectInfo* GetEffectInfo(String effectSystem, String module, String effectName, int iModuleMatchFlags = MODULE_MATCH_EXACTLY);

    /**
     * Create an instance of the requested effect. You should call Destroy()
     * once the effect is not used anymore.
     *
     * @param pInfo - unique identifier of the effect to create
     * @returns pointer to new effect instance, throws an Exception otherwise
     * @throws Exception - if the requested effect could not be instantiated
     *                     successfully
     */
    static Effect* Create(EffectInfo* pInfo) throw (Exception);

    /**
     * Destroy and free the given effect instance from memory, previously
     * created with Create() .
     *
     * @param pEffect - effect instance to destroy
     * @throws Exception - if effect is still in use
     */
    static void Destroy(Effect* pEffect) throw (Exception);

    /**
     * Returns total amount of effect instances created with Create() .
     */
    static uint EffectInstancesCount();

    /**
     * Returns effect instance for the given index.
     *
     * @param index - index of the effect instance must be between 0 and
     *                EffectInstancesCount()
     * @returns effect instance, or NULL if index out of bounds
     */
    static Effect* GetEffectInstance(uint index);

    /**
     * Returns effect instance with the given effect instance ID.
     *
     * @param id - effect instance ID of sought effect instance
     * @returns effect instance or NULL if no effect instance with that ID
     *          could be found
     */
    static Effect* GetEffectInstanceByID(int id);
};

} // namespace LinuxSampler

#endif // LS_EFFECT_FACTORY_H

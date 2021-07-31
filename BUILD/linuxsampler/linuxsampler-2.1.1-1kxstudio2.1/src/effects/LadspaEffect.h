/*
    Copyright (C) 2010 - 2013 Christian Schoenebeck
*/

#ifndef LS_LADSPAEFFECT_H
#define LS_LADSPAEFFECT_H

#include "Effect.h"
#include "EffectInfo.h"
#include "../common/ladspa.h"
#include "../common/Exception.h"
#include <vector>

namespace LinuxSampler {

class LadspaEffectInfo;
class AudioOutputDevice;

/**
 * Implementation of internal effects using the LADSPA effect system.
 *
 * @e Note: this class is only sampler internal and won't be exported to the
 * external C++ API of the sampler. Use the static class EffectFactory instead
 * for managing LADSPA effects by external applications.
 */
class LadspaEffect : public Effect {
public:
    LadspaEffect(EffectInfo* pInfo) throw (Exception);
   ~LadspaEffect();
    EffectInfo* GetEffectInfo() OVERRIDE;
    void RenderAudio(uint Samples) OVERRIDE;
    void InitEffect(AudioOutputDevice* pDevice) throw (Exception) OVERRIDE;
    static std::vector<EffectInfo*> AvailableEffects();

private:
    LadspaEffectInfo* pInfo;
    void* hDLL;
    const LADSPA_Descriptor* pDescriptor;
    LADSPA_Handle hEffect;
    AudioOutputDevice* pDevice;

    float getLowerB(int iPort) const;
    float getUpperB(int iPort) const;
};

} // namespace LinuxSampler

#endif // LS_LADSPAEFFECT_H

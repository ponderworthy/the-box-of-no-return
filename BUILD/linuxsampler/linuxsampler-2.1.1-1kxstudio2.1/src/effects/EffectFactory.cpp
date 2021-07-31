/*
    Copyright (C) 2010 - 2016 Christian Schoenebeck
*/

#include "EffectFactory.h"
#include "LadspaEffect.h"
#include "../common/Path.h"
#include "../common/IDGenerator.h"
#include <algorithm>

namespace LinuxSampler {

namespace {

////////////////////////////////////////////////////////////////////////////
// class 'EffectInfos'

class EffectInfos {
public:
    EffectInfos();
    ~EffectInfos();
    void Update();
    uint Count();
    EffectInfo* GetEffectInfo(uint index);
private:
    std::vector<EffectInfo*> infos;
    bool bInitialized;
};

EffectInfos::EffectInfos() : bInitialized(false) {
}

EffectInfos::~EffectInfos() {
    for (int i = 0; i < infos.size(); i++) delete infos[i];
}

void EffectInfos::Update() {
    // clear out all old effect infos
    for (int i = 0; i < infos.size(); i++) delete infos[i];

    // scan for LADSPA effects
    infos = LadspaEffect::AvailableEffects();
}

uint EffectInfos::Count() {
    if (!bInitialized) {
        Update();
        bInitialized = true;
    }
    return (uint) infos.size();
}

EffectInfo* EffectInfos::GetEffectInfo(uint index) {
    if (index >= infos.size()) return NULL;
    return infos[index];
}


////////////////////////////////////////////////////////////////////////////
// private static variables

EffectInfos effectInfos;

std::vector<Effect*> vEffectInstances;

IDGenerator idGenerator;

}


////////////////////////////////////////////////////////////////////////////
// class 'EffectFactory'

String EffectFactory::AvailableEffectSystemsAsString() {
    return "LADSPA";
}

uint EffectFactory::AvailableEffectsCount() {
    return effectInfos.Count();
}

EffectInfo* EffectFactory::GetEffectInfo(uint index) {
    return effectInfos.GetEffectInfo(index);
}

static String _toLower(String s) {
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);
    return s;
}

static String _stripFileExtension(String sFileName) {
#if defined(WIN32)
    Path p = Path::fromWindows(sFileName);
    return p.stripLastName() + "\\" + p.getBaseName();
#else
    Path p = Path::fromPosix(sFileName);
    return p.stripLastName() + "/" + p.getBaseName();
#endif
}

static String _stripFilePath(String sFileName) {
#if defined(WIN32)
    Path p = Path::fromWindows(sFileName);
#else
    Path p = Path::fromPosix(sFileName);
#endif
    return p.getName();
}

static bool _moduleNameMatches(String name1, String name2, int iModuleMatchFlags) {
    if (iModuleMatchFlags == EffectFactory::MODULE_IGNORE_ALL) return true;
    if (iModuleMatchFlags == EffectFactory::MODULE_MATCH_EXACTLY) return name1 == name2;
    if (iModuleMatchFlags & EffectFactory::MODULE_IGNORE_CASE) {
        name1 = _toLower(name1);
        name2 = _toLower(name2);
    }
    if (iModuleMatchFlags & EffectFactory::MODULE_IGNORE_EXTENSION) {
        name1 = _stripFileExtension(name1);
        name2 = _stripFileExtension(name2);
    }
    if (iModuleMatchFlags & EffectFactory::MODULE_IGNORE_PATH) {
        name1 = _stripFilePath(name1);
        name2 = _stripFilePath(name2);
    }
    return name1 == name2;
}

EffectInfo* EffectFactory::GetEffectInfo(String effectSystem, String module, String effectName, int iModuleMatchFlags) {
    for (int i = 0; i < effectInfos.Count(); i++) {
        EffectInfo* pEffectInfo = effectInfos.GetEffectInfo(i);
        if (pEffectInfo->EffectSystem() == effectSystem &&
            _moduleNameMatches(pEffectInfo->Module(), module, iModuleMatchFlags) &&
            pEffectInfo->Name() == effectName
        ) {
            return pEffectInfo;
        }
    }
    return NULL;
}

Effect* EffectFactory::Create(EffectInfo* pEffectInfo) throw (Exception) {
    Effect* pEffect = NULL;
    try {
        if (pEffectInfo->EffectSystem() == "LADSPA") {
            pEffect = new LadspaEffect(pEffectInfo);
        } else {
            throw Exception(
                "Effect system '" + pEffectInfo->EffectSystem() +
                "' not supported"
            );
        }
    } catch (Exception e) {
        throw Exception("Could not create effect: " + e.Message());
    } catch (...) {
        throw Exception("Could not create effect: unknown exception");
    }
    if (!pEffect) {
        // should never happen
        throw Exception("Oops, EffectFactory bug: !pEffect");
    }
    
    // stick a new unique effect ID to the effect instance
    const int id = idGenerator.create();
    if (id < 0) {
        delete pEffect;
        throw Exception("Could not generate a new effect ID, whole ID value range is occupied!");
    }
    pEffect->SetId(id);

    vEffectInstances.push_back(pEffect);
    return pEffect;
}

void EffectFactory::Destroy(Effect* pEffect) throw (Exception) {
    // check if effect is still in use
    if (pEffect->Parent())
        throw Exception("effect still in use");

    // now delete effect
    for (int i = 0; i < vEffectInstances.size(); i++) {
        if (vEffectInstances[i] == pEffect) {
            vEffectInstances.erase(vEffectInstances.begin() + i);

            // free the effect's ID
            idGenerator.destroy(pEffect->ID());

            delete pEffect;
        }
    }
}

void EffectFactory::UpdateAvailableEffects() {
    effectInfos.Update();
}

uint EffectFactory::EffectInstancesCount() {
    return (uint) vEffectInstances.size();
}

Effect* EffectFactory::GetEffectInstance(uint index) {
    if (index >= vEffectInstances.size()) return NULL;
    return vEffectInstances[index];
}

Effect* EffectFactory::GetEffectInstanceByID(int id) {
    for (int i = 0; i < vEffectInstances.size(); i++) {
        if (vEffectInstances[i]->ID() == id)
            return vEffectInstances[i];
    }
    return NULL;
}

} // namespace LinuxSampler

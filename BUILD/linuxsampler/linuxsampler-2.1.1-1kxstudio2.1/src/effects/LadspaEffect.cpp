/*
    Copyright (C) 2010 - 2016 Christian Schoenebeck
*/

#include "LadspaEffect.h"
#include "../common/DynamicLibraries.h"
#include "../common/global_private.h"
#include "../common/File.h"
#include "../drivers/audio/AudioOutputDevice.h"
#include <cmath>
#include <sstream>

namespace LinuxSampler {

////////////////////////////////////////////////////////////////////////////
// private helper functions

/// Returns total amount of ports for the given effect and port type.
static unsigned long _getPortCountByType(const LADSPA_Descriptor* psDescriptor, const LADSPA_PortDescriptor iType) {
    unsigned long lCount = 0;
    unsigned long lIndex;
    for (lIndex = 0; lIndex < psDescriptor->PortCount; lIndex++)
        if ((psDescriptor->PortDescriptors[lIndex] & iType) == iType)
            lCount++;

    return lCount;
}

////////////////////////////////////////////////////////////////////////////
// class 'LadspaEffectInfo'

/**
 * Identifier of exactly one LADSPA effect, used as unique key, e.g. for the
 * respective LADSPA effect to be loaded.
 */
class LadspaEffectInfo : public EffectInfo {
public:
    String dll;
    String label;
    String name;

    String EffectSystem() {
        return "LADSPA";
    }

    String Name() {
        return label;
    }

    String Module() {
        return dll;
    }

    String Description() {
        return name;
    }
};

////////////////////////////////////////////////////////////////////////////
// class 'LadspaEffectControl'

/**
 * We just open access to protected members of EffectControl here.
 */
class LadspaEffectControl : public EffectControl {
public:
    using EffectControl::SetDefaultValue;
    using EffectControl::SetMinValue;
    using EffectControl::SetMaxValue;
    using EffectControl::SetType;
    using EffectControl::SetDescription;
    using EffectControl::SetPossibilities;
};

////////////////////////////////////////////////////////////////////////////
// class 'LadspaEffect'

LadspaEffect::LadspaEffect(EffectInfo* pInfo) throw (Exception) : Effect() {
    this->pInfo = dynamic_cast<LadspaEffectInfo*>(pInfo);
    if (!this->pInfo)
        throw Exception("Effect key does not represent a LADSPA effect");

    // DynamicLibraryOpen() and DynamicLibraryClose() maintain a reference
    // count, so its OK to open and close the respective DLL for each effect,
    // even though some effects might share the same DLL.
    hDLL = DynamicLibraryOpen(this->pInfo->dll);
    if (!hDLL)
        throw Exception("Could not open DLL '" + this->pInfo->dll + "' for LADSPA effect");

    LADSPA_Descriptor_Function pDescriptorFunction = (LADSPA_Descriptor_Function)
        DynamicLibraryGetSymbol(hDLL, "ladspa_descriptor");

    if (!pDescriptorFunction)
        throw Exception("'" + this->pInfo->dll + "' is not a LADSPA plugin library");

    // search for requested effect in that LADSPA DLL
    for (long lPluginIndex = 0; true; lPluginIndex++) {
        pDescriptor = pDescriptorFunction(lPluginIndex);
        if (!pDescriptor)
            throw Exception(
                "Effect '" + this->pInfo->label +
                "' could not be found in LADSPA DLL '" + this->pInfo->dll + "'"
            );

        if (pDescriptor->Label == this->pInfo->label)
            break; // found
    }

    // those will be set later in InitEffect()
    hEffect = NULL;
    pDevice = NULL;

    // create control input and control output variables (effect parameters)
    // (they are going to be assigned to the actual LADSPA effect instance
    // later in InitEffect() )
    const int iInControls = (int) _getPortCountByType(
        pDescriptor,
        LADSPA_PORT_CONTROL | LADSPA_PORT_INPUT
    );
    const int iOutControls = (int) _getPortCountByType(
        pDescriptor,
        LADSPA_PORT_CONTROL | LADSPA_PORT_OUTPUT
    );
    vInputControls.resize(iInControls);
    vOutputControls.resize(iOutControls);
    // create LadspaEffectControl instances and determine its informations
    // (value range, description, default value)
    int iInControl = 0;
    int iOutControl = 0;
    for (int iPort = 0; iPort < pDescriptor->PortCount; iPort++) {
        LADSPA_PortDescriptor pPortDescriptor = pDescriptor->PortDescriptors[iPort];
        if (LADSPA_IS_PORT_CONTROL(pPortDescriptor)) {
            if (LADSPA_IS_PORT_INPUT(pPortDescriptor)) {
                LadspaEffectControl* pEffectControl = new LadspaEffectControl();
                vInputControls[iInControl++] = pEffectControl;

                const float lower = getLowerB(iPort);
                const float upper = getUpperB(iPort);

                // determine default value
                float fDefault = 0.5f * lower + 0.5f * upper; // middle value by default
                if (LADSPA_IS_HINT_HAS_DEFAULT(pPortDescriptor)) {
                    if (LADSPA_IS_HINT_DEFAULT_MINIMUM(pPortDescriptor)) {
                        fDefault = lower;
                    } else if (LADSPA_IS_HINT_DEFAULT_LOW(pPortDescriptor)) {
                        if (LADSPA_IS_HINT_LOGARITHMIC(pPortDescriptor)) {
                            fDefault = exp(log(lower) * 0.75 + log(upper) * 0.25);
                        } else {
                            fDefault = lower * 0.75 + upper * 0.25;
                        }
                    } else if (LADSPA_IS_HINT_DEFAULT_MIDDLE(pPortDescriptor)) {
                        if (LADSPA_IS_HINT_LOGARITHMIC(pPortDescriptor)) {
                            fDefault = exp(log(lower) * 0.5 + log(upper) * 0.5);
                        } else {
                            fDefault = 0.5f * lower + 0.5f * upper;
                        }
                    } else if (LADSPA_IS_HINT_DEFAULT_HIGH(pPortDescriptor)) {
                        if (LADSPA_IS_HINT_LOGARITHMIC(pPortDescriptor)) {
                            fDefault = exp(log(lower) * 0.25 + log(upper) * 0.75);
                        } else {
                            fDefault = lower * 0.25 + upper * 0.75;
                        }
                    } else if (LADSPA_IS_HINT_DEFAULT_MAXIMUM(pPortDescriptor)) {
                        fDefault = upper;
                    } else if (LADSPA_IS_HINT_DEFAULT_0(pPortDescriptor)) {
                        fDefault = 0.0f;
                    } else if (LADSPA_IS_HINT_DEFAULT_1(pPortDescriptor)) {
                        fDefault = 1.0f;
                    } else if (LADSPA_IS_HINT_DEFAULT_100(pPortDescriptor)) {
                        fDefault = 100.0f;
                    } else if (LADSPA_IS_HINT_DEFAULT_440(pPortDescriptor)) {
                        fDefault = 440.0f;
                    }

                    pEffectControl->SetDefaultValue(fDefault);
                }
                pEffectControl->SetValue(fDefault);

                // determine value range type
                EffectControl::Type_t type;
                if (LADSPA_IS_HINT_INTEGER(pPortDescriptor)) {
                    type = EffectControl::EFFECT_TYPE_INT;
                } else if (LADSPA_IS_HINT_TOGGLED(pPortDescriptor)) {
                    type = EffectControl::EFFECT_TYPE_BOOL;
                } else {
                    type = EffectControl::EFFECT_TYPE_FLOAT;
                }
                pEffectControl->SetType(type);

                // is there a minimum value?
                if (LADSPA_IS_HINT_BOUNDED_BELOW(pPortDescriptor)) {
                    pEffectControl->SetMinValue(lower);
                }

                // is there a maximum value?
                if (LADSPA_IS_HINT_BOUNDED_ABOVE(pPortDescriptor)) {
                    pEffectControl->SetMaxValue(upper);
                }

                // boolean type?
                if (LADSPA_IS_HINT_TOGGLED(pPortDescriptor)) {
                    std::vector<float> vPossibilities;
                    vPossibilities.push_back(0.0f);
                    vPossibilities.push_back(1.0f);
                    pEffectControl->SetPossibilities(vPossibilities);
                }

                // retrieve human readable description about port
                pEffectControl->SetDescription(pDescriptor->PortNames[iPort]);

            } else if (LADSPA_IS_PORT_OUTPUT(pPortDescriptor)) {
                LadspaEffectControl* pEffectControl = new LadspaEffectControl();
                vOutputControls[iOutControl++] = pEffectControl;

                //TODO: init output controls like input controls above
            }
        }
    }
}

LadspaEffect::~LadspaEffect() {
    if (!hEffect) return;
    if (pDescriptor->deactivate)
        pDescriptor->deactivate(hEffect);
    pDescriptor->cleanup(hEffect);
    DynamicLibraryClose(hDLL);
}

EffectInfo* LadspaEffect::GetEffectInfo() {
    return pInfo;
}

void LadspaEffect::RenderAudio(uint Samples) {
    // (re)assign audio input and audio output buffers
    int iInputPort = 0;
    int iOutputPort = 0;
    for (int iPort = 0; iPort < pDescriptor->PortCount; iPort++) {
        LADSPA_PortDescriptor pPortDescriptor = pDescriptor->PortDescriptors[iPort];
        if (LADSPA_IS_PORT_AUDIO(pPortDescriptor)) {
            if (LADSPA_IS_PORT_INPUT(pPortDescriptor)) {
                pDescriptor->connect_port(hEffect, iPort, vInputChannels[iInputPort++]->Buffer());
            } else if (LADSPA_IS_PORT_OUTPUT(pPortDescriptor)) {
                pDescriptor->connect_port(hEffect, iPort, vOutputChannels[iOutputPort++]->Buffer());
            }
        }
    }

    // let the effect do its job
    pDescriptor->run(hEffect, Samples);
}

void LadspaEffect::InitEffect(AudioOutputDevice* pDevice) throw (Exception) {
    this->pDevice = pDevice;

    const int iInChannels = (int) _getPortCountByType(
        pDescriptor,
        LADSPA_PORT_AUDIO | LADSPA_PORT_INPUT
    );
    const int iOutChannels = (int) _getPortCountByType(
        pDescriptor,
        LADSPA_PORT_AUDIO | LADSPA_PORT_OUTPUT
    );
    //const int iInControls  = (int) vInputControls.size();
    //const int iOutControls = (int) vOutputControls.size();

    // now create the actual LADSPA effect instance ...
    dmsg(1, ("Instantiating LADSPA effect '%s'.\n", pInfo->label.c_str()));
    hEffect = pDescriptor->instantiate(pDescriptor, pDevice->SampleRate());
    if (!hEffect)
        throw Exception("Could not instantiate LADSPA effect '" + pInfo->label + "'");

    // create audio input channels
    vInputChannels.resize(iInChannels);
    for (int i = 0; i < iInChannels; i++) {
        vInputChannels[i] = new AudioChannel(i, pDevice->MaxSamplesPerCycle());
    }

    // create audio output channels
    vOutputChannels.resize(iOutChannels);
    for (int i = 0; i < iOutChannels; i++) {
        vOutputChannels[i] = new AudioChannel(i, pDevice->MaxSamplesPerCycle());
    }
    
    // TODO: recalculate the min and max values that depends on sample rate
    
    // assign (already created and initialized) control input and control
    // output variables (effect parameters)
    int iInControl = 0;
    int iOutControl = 0;
    for (int iPort = 0; iPort < pDescriptor->PortCount; iPort++) {
        LADSPA_PortDescriptor pPortDescriptor = pDescriptor->PortDescriptors[iPort];
        if (LADSPA_IS_PORT_CONTROL(pPortDescriptor)) {
            if (LADSPA_IS_PORT_INPUT(pPortDescriptor)) {
                LadspaEffectControl* pEffectControl =
                    (LadspaEffectControl*) vInputControls[iInControl++];
                pDescriptor->connect_port(hEffect, iPort, &pEffectControl->Value());
            } else if (LADSPA_IS_PORT_OUTPUT(pPortDescriptor)) {
                LadspaEffectControl* pEffectControl =
                    (LadspaEffectControl*) vOutputControls[iOutControl++];
                pDescriptor->connect_port(hEffect, iPort, &pEffectControl->Value());
            }
        }
    }

    // call LADSPA effect's activate function (if there is one)
    if (pDescriptor->activate != NULL)
        pDescriptor->activate(hEffect);

    dmsg(1, ("LADSPA effect '%s' activated.\n", pInfo->label.c_str()));
}

/// Returns lowest allowed value for this LADSPA control port.
float LadspaEffect::getLowerB(int iPort) const {
    float low =
        (pDescriptor->PortRangeHints[iPort].HintDescriptor & LADSPA_HINT_BOUNDED_BELOW)
            ? pDescriptor->PortRangeHints[iPort].LowerBound : 0.0f;

    if (pDescriptor->PortRangeHints[iPort].HintDescriptor & LADSPA_HINT_SAMPLE_RATE)
        low *= float(pDevice == NULL ? 44100 : pDevice->SampleRate());

    return low;
}

/// Returns biggest allowed value for this LADSPA control port.
float LadspaEffect::getUpperB(int iPort) const {
    float up =
        (pDescriptor->PortRangeHints[iPort].HintDescriptor & LADSPA_HINT_BOUNDED_ABOVE)
            ? pDescriptor->PortRangeHints[iPort].UpperBound : 1.0f;

    if (pDescriptor->PortRangeHints[iPort].HintDescriptor & LADSPA_HINT_SAMPLE_RATE)
        up *= float(pDevice == NULL ? 44100 : pDevice->SampleRate());

    return up;
}

static void _foundLadspaDll(String filename, void* hDLL, void* pFunction, void* pCustom) {
    LADSPA_Descriptor_Function fDescriptorFunction = (LADSPA_Descriptor_Function) pFunction;
    std::vector<EffectInfo*>* pV = (std::vector<EffectInfo*>*) pCustom;
    const LADSPA_Descriptor* psDescriptor;
    for (long lIndex = 0; (psDescriptor = fDescriptorFunction(lIndex)) != NULL; lIndex++) {
        //printf("\t%s (%lu/%s)\n", psDescriptor->Name, psDescriptor->UniqueID, psDescriptor->Label);
        LadspaEffectInfo* pInfo = new LadspaEffectInfo;
        pInfo->name = psDescriptor->Name;
        pInfo->label = psDescriptor->Label;
        pInfo->dll = filename;
        pV->push_back(pInfo);
    }
    DynamicLibraryClose(hDLL);
}

static String defaultLadspaDir() {
    #if defined(WIN32)
    const String sysDir =
        getenv("PROGRAMFILES") ? getenv("PROGRAMFILES") : "C:\\Program Files";
    const String searchDirs[] = {
        sysDir + "\\ladspa", //FIXME: hmm... who knows what the common default LADSPA path on Windows is?
        sysDir + "\\Audacity\\Plug-Ins"
    };
    #else
    const String searchDirs[] = {
        "/usr/lib/ladspa",
        "/usr/local/lib/ladspa"
    };
    #endif
    // check if one of the suggested directories exists
    for (int i = 0; i < sizeof(searchDirs) / sizeof(String); i++) {
        File f(searchDirs[i]);
        if (f.Exist() && f.IsDirectory())
            return searchDirs[i];
    }
    // No directory of the list exists? Whatever, return the 1st one of the list.
    return searchDirs[0];
}

std::vector<EffectInfo*> LadspaEffect::AvailableEffects() {
    std::vector<EffectInfo*> v; // will be filled in callback function _foundLadspaDll()

    char* pcLadspaPath = getenv("LADSPA_PATH");
    String ladspaDir = pcLadspaPath ? pcLadspaPath : defaultLadspaDir();

    std::istringstream ss(ladspaDir);
    std::string path;
    while (std::getline(ss, path, File::PathSeparator)) {
        if (!path.empty()) {
            try {
                DynamicLibrariesSearch(path.c_str(), "ladspa_descriptor", _foundLadspaDll, &v);
            } catch (Exception e) {
                std::cerr << "Could not scan LADSPA effects: " << e.Message()
                          << std::endl << std::flush;
            } catch (...) {
                std::cerr << "Could not scan LADSPA effects: unknown exception\n"
                          << std::flush;
            }
        }
    }

    return v;
}

} // namespace LinuxSampler

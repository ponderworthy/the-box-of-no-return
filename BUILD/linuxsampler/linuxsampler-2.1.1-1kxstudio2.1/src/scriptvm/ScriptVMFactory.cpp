/*
 * Copyright (c) 2016 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */ 

#include "ScriptVMFactory.h"
#include <algorithm>
#include "../engines/common/InstrumentScriptVM.h"
#include "../engines/gig/InstrumentScriptVM.h"

namespace LinuxSampler {

ScriptVM* ScriptVMFactory::Create(String EngineName) {
    std::transform(EngineName.begin(), EngineName.end(), EngineName.begin(), ::tolower);
    if (EngineName == "core")
        return new ScriptVM;
    else if (EngineName == "gig")
        return new gig::InstrumentScriptVM;
    else if (EngineName == "sf2")
        return new InstrumentScriptVM;
    else if (EngineName == "sfz")
        return new InstrumentScriptVM;
    else
        return NULL;
}

std::vector<String> ScriptVMFactory::AvailableEngines() {
    std::vector<String> v;
    v.push_back("core");
    v.push_back("gig");
    v.push_back("sf2");
    v.push_back("sfz");
    return v;
}
    
} // namespace LinuxSampler

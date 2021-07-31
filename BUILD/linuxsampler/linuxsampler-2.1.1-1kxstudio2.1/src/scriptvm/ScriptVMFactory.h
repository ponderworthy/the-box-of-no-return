/*
 * Copyright (c) 2016 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */ 

#ifndef LS_SCRIPTVMFACTORY_H

#include "ScriptVM.h"

namespace LinuxSampler {
    
class ScriptVMFactory {
public:
    static ScriptVM* Create(String EngineName);
    static std::vector<String> AvailableEngines();
};

} // namespace LinuxSampler

#endif // LS_SCRIPTVMFACTORY_H

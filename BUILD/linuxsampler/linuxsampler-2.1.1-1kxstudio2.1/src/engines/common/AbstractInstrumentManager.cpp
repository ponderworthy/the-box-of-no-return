/*
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "AbstractInstrumentManager.h"
#include "../AbstractEngine.h"
#include "../AbstractEngineChannel.h"

namespace LinuxSampler {

    VMParserContext* AbstractInstrumentManager::ScriptResourceManager::Create(String Key, InstrumentScriptConsumer* pConsumer, void*& pArg) {
        AbstractEngineChannel* pEngineChannel = dynamic_cast<AbstractEngineChannel*>(pConsumer);
        return pEngineChannel->pEngine->pScriptVM->loadScript(Key);
    }

    void AbstractInstrumentManager::ScriptResourceManager::Destroy(VMParserContext* pResource, void* pArg) {
        delete pResource;
    }

} // namespace LinuxSampler

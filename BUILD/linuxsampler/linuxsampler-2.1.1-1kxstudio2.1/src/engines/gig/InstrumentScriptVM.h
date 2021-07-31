/*
 * Copyright (c) 2014 - 2017 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_GIG_INSTRUMENT_SCRIPT_VM_H
#define LS_GIG_INSTRUMENT_SCRIPT_VM_H

#include "../common/InstrumentScriptVM.h"
#include "InstrumentScriptVMFunctions.h"

namespace LinuxSampler { namespace gig {

    /** @brief Real-time instrument script virtual machine (Giga format).
     *
     * Extends the common sampler format independent InstrumentScriptVM with
     * Giga format specific built-in script variables and functions.
     *
     * Note that this class is currently re-entrant safe, but @b not thread
     * safe! See also comments of base class ScriptVM regarding this issue.
     */
    class InstrumentScriptVM : public LinuxSampler::InstrumentScriptVM {
    public:
        InstrumentScriptVM();
        VMFunction* functionByName(const String& name) OVERRIDE;
        //std::map<String,VMIntRelPtr*> builtInIntVariables() OVERRIDE;
        //std::map<String,VMInt8Array*> builtInIntArrayVariables() OVERRIDE;
        std::map<String,int> builtInConstIntVariables() OVERRIDE;
    protected:
        InstrumentScriptVMFunction_gig_set_dim_zone   m_fnGigSetDimZone;
        InstrumentScriptVMFunction_same_region        m_fnSameRegion;

        friend class InstrumentScriptVMFunction_gig_set_dim_zone;
        friend class InstrumentScriptVMFunction_same_region;
    };

}} // namespace LinuxSampler::gig

#endif // LS_GIG_INSTRUMENT_SCRIPT_VM_H

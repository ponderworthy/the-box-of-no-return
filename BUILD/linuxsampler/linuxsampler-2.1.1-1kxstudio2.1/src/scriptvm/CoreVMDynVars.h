/*
 * Copyright (c) 2016 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef LS_COREVMDYNVARS_H
#define LS_COREVMDYNVARS_H

#include "../common/global.h"
#include "common.h"

namespace LinuxSampler {
    
class ScriptVM;

///////////////////////////////////////////////////////////////////////////
// implementations of core built-in dynamic variables ...

/**
 * Implements the built-in $NKSP_REAL_TIMER script variable.
 */
class CoreVMDynVar_NKSP_REAL_TIMER : public VMDynIntVar {
public:
    bool isAssignable() const OVERRIDE { return false; }
    int evalInt() OVERRIDE;
};

/**
 * Implements the built-in $NKSP_PERF_TIMER script variable.
 */
class CoreVMDynVar_NKSP_PERF_TIMER : public VMDynIntVar {
public:
    bool isAssignable() const OVERRIDE { return false; }
    int evalInt() OVERRIDE;
};

} // namespace LinuxSampler

#endif // LS_COREVMDYNVARS_H

/*
 * Copyright (c) 2016 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "CoreVMDynVars.h"

#include "tree.h"
#include "ScriptVM.h"
#include "../common/RTMath.h"

namespace LinuxSampler {

int CoreVMDynVar_NKSP_REAL_TIMER::evalInt() {
    return (int) RTMath::unsafeMicroSeconds(RTMath::real_clock);
}

int CoreVMDynVar_NKSP_PERF_TIMER::evalInt() {
    return (int) RTMath::unsafeMicroSeconds(RTMath::thread_clock);
}

} // namespace LinuxSampler

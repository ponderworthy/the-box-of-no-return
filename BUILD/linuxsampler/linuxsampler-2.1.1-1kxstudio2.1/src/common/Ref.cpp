/*
 * Copyright (c) 2014 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "Ref.h"

namespace LinuxSampler {

    #if LS_REF_ASSERT_MODE
    std::set<void*> _allRefPtrs;
    #endif

} // namespace LinuxSampler

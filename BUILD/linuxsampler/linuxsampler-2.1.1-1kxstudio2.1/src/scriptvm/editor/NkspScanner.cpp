/*
 * Copyright (c) 2015-2016 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#include "NkspScanner.h"

namespace LinuxSampler {

NkspScanner::NkspScanner(std::istream* is) : CodeScanner(is) {
    createScanner(is);
    processAll();
}

NkspScanner::~NkspScanner() {
    if (scanner) destroyScanner();
}

} // namespace LinuxSampler

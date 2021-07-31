/*
 * Copyright (c) 2015-2016 Christian Schoenebeck
 *
 * http://www.linuxsampler.org
 *
 * This file is part of LinuxSampler and released under the same terms.
 * See README file for details.
 */

#ifndef CRUDEBYTE_NKSP_SCANNER_H
#define CRUDEBYTE_NKSP_SCANNER_H

#include "CodeScanner.h"

namespace LinuxSampler {

class NkspScanner : public CodeScanner {
public:
    NkspScanner(std::istream* is);
    virtual ~NkspScanner();

protected:
    int processScanner();
    void createScanner(std::istream* is);
    void destroyScanner();
};

} // namespace LinuxSampler

#endif // CRUDEBYTE_NKSP_SCANNER_H

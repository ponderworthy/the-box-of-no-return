/*
 *   Copyright (C) 2005, 2007 Christian Schoenebeck
 */

#ifndef __LS_SYNTHESIS_PARAM_H__
#define __LS_SYNTHESIS_PARAM_H__

#include "../../common/global_private.h"
#include "Filter.h"

namespace LinuxSampler { namespace gig {

    struct Loop {
        uint uiStart;
        uint uiEnd;
        uint uiSize;
        uint uiTotalCycles; ///< Number of times the loop should be played (a value of 0 = infinite).
        uint uiCyclesLeft;  ///< In case there is a RAMLoop and it's not an endless loop; reflects number of loop cycles left to be passed
    };

    struct SynthesisParam {
        Filter    filterLeft;
        Filter    filterRight;
        float     fFinalPitch;
        float     fFinalVolumeLeft;
        float     fFinalVolumeRight;
        float     fFinalVolumeDeltaLeft;
        float     fFinalVolumeDeltaRight;
        double    dPos;
        sample_t* pSrc;
        float*    pOutLeft;
        float*    pOutRight;
        uint      uiToGo;
    };

}} // namespace LinuxSampler::gig

#endif // __LS_SYNTHESIS_PARAM_H__

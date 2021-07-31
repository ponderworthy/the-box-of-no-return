/*
    gig::Engine synthesis benchmark

    This is a benchmark for testing the performance of the sampler's current
    Gigasampler format synthesis algorithms. It benchmarks each possible
    "synthesis mode" (that is all possible cases like filter on / off,
    interpolation on / off, stereo / mono). It uses fake sample data and
    fake audio outputs, so we don't have to load a .gig file or care about
    drivers.

    Copyright (C) 2005,2006 Christian Schoenebeck <cuse@users.sf.net>
*/

#include <math.h>
#include <time.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <gig.h>

#include "../src/engines/gig/SynthesisParam.h"
#include "../src/engines/gig/Synthesizer.h"

#define FRAGMENTSIZE	200
#define RUNS            100000

using namespace LinuxSampler;
using namespace LinuxSampler::gig;

int16_t* pSampleInputBuf; // just a buffer with random data

float* pOutputL;
float* pOutputR;

void printmode(int mode) {
    printf("Synthesis Mode: %d ",mode);
    printf("(BITDEPTH=%s,%s,DOLOOP=%s,FILTER=%s,INTERPOLATE=%s)\n",
           (SYNTHESIS_MODE_GET_BITDEPTH24(mode)) ? "24" : "16",
           (SYNTHESIS_MODE_GET_CHANNELS(mode)) ? "STEREO" : "MONO",
           (SYNTHESIS_MODE_GET_LOOP(mode)) ? "y" : "n",
           (SYNTHESIS_MODE_GET_FILTER(mode)) ? "y" : "n",
           (SYNTHESIS_MODE_GET_INTERPOLATE(mode)) ? "y" : "n"
    );
    fflush(stdout);
}

int main() {
    pSampleInputBuf = new int16_t[FRAGMENTSIZE*2 + 100];
    pOutputL = (float*) memalign(16,FRAGMENTSIZE*sizeof(float));
    pOutputR = (float*) memalign(16,FRAGMENTSIZE*sizeof(float));

    // prepare some input data for simulation
    for (int i = 0; i < FRAGMENTSIZE*2; i++) {
        pSampleInputBuf[i] = i;
    }

    SynthesisParam* pParam = new SynthesisParam;
    pParam->filterLeft.SetParameters(5.26f, 127.0f, 44100);
    pParam->filterRight.SetParameters(5.26f, 127.0f, 44100);
    pParam->fFinalPitch = 0.5f;
    pParam->fFinalVolumeLeft = 1.0f;
    pParam->fFinalVolumeRight = 1.0f;
    pParam->fFinalVolumeDeltaLeft = 0;
    pParam->fFinalVolumeDeltaRight = 0;
    pParam->pSrc = pSampleInputBuf;

    // define some loop points
    Loop* pLoop = new Loop;
    pLoop->uiStart = 4;
    pLoop->uiEnd   = 20;
    pLoop->uiSize  = 16;
    pLoop->uiTotalCycles = 0; // infinity

    for (int mode = 0; mode < 32; mode++) {
            // zero out output buffers
            memset(pOutputL,0,FRAGMENTSIZE*sizeof(float));
            memset(pOutputR,0,FRAGMENTSIZE*sizeof(float));

            pParam->filterLeft.Reset();
            pParam->filterRight.Reset();

            printf("Benchmarking ");
            printmode(mode);

            clock_t stop_time;
            clock_t start_time = clock();

            for (uint i = 0; i < RUNS; i++) {
                pParam->dPos      = 0.0;
                pParam->pOutLeft  = pOutputL;
                pParam->pOutRight = pOutputR;
                pParam->uiToGo    = FRAGMENTSIZE;
                // now actually render audio
                RunSynthesisFunction(mode, pParam, pLoop);
            }

            stop_time = clock();
            float elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
            printf("\t: %1.0f ms\n", elapsed_time);
    }
}

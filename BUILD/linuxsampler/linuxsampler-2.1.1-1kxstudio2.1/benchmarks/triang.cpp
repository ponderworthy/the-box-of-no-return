/*
    Triangle wave generator benchmark

    This is a benchmark for comparison between a integer math, table lookup
    and numeric sine wave harmonics solution.

    Copyright (C) 2005 - 2017 Christian Schoenebeck <cuse@users.sf.net>
*/

#include <math.h>
#include <time.h>
#include <stdio.h>

#include "../src/engines/common/LFOTriangleIntMath.h"
#include "../src/engines/common/LFOTriangleIntAbsMath.h"
#include "../src/engines/common/LFOTriangleDiHarmonic.h"

// whether we should not show any messages on the console
#ifndef SILENT
# define SILENT 0
#endif

// set to 1 if you want to output the three calculated waves as RAW files
// you can e.g. open it as RAW file in Rezound
// (32 bit SP-FP PCM, mono, little endian, 44100kHz)
#ifndef OUTPUT_AS_RAW_WAVE
# define OUTPUT_AS_RAW_WAVE	0
#endif

// how many sample points should we calculate in one sequence
#ifndef STEPS
# define STEPS			16384
#endif

// how often should we repeat the benchmark loop of each solution
#ifndef RUNS
# define RUNS			1000
#endif

// whether the wave should have positive and negative range (signed -> 1) or just positive (unsigned -> 0)
#ifndef SIGNED
# define SIGNED			1
#endif

// maximum value of the LFO output (also depends on SIGNED above)
#ifndef MAX
# define MAX			1.0f
#endif

// pro forma
#ifndef SAMPLING_RATE
# define SAMPLING_RATE		44100.0f
#endif

// return value of this benchmark
// to indicate the best performing solution
#define INT_MATH_SOLUTION	2  /* we don't start with 1, as this is reserved for unknown errors */
#define DI_HARMONIC_SOLUTION	3
#define TABLE_LOOKUP_SOLUTION	4  /* table lookup solution is currently disabled in this benchmark, see below */
#define INT_MATH_ABS_SOLUTION   5  /* integer math with abs() */
#define INVALID_RESULT		-1

// we use 32 bit single precision floating point as sample point format
typedef float smpl_t; // (sample_t is already defined as int16_t by global_private.h)

using namespace LinuxSampler;

#if SIGNED
LFOTriangleIntMath<range_signed>*    pIntLFO        = NULL;
LFOTriangleIntAbsMath<range_signed>* pIntAbsLFO     = NULL;
LFOTriangleDiHarmonic<range_signed>* pDiHarmonicLFO = NULL;
#else // unsigned
LFOTriangleIntMath<range_unsigned>*    pIntLFO        = NULL;
LFOTriangleIntAbsMath<range_unsigned>* pIntAbsLFO     = NULL;
LFOTriangleDiHarmonic<range_unsigned>* pDiHarmonicLFO = NULL;
#endif

// integer math solution
float int_math(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    pIntLFO->trigger(frequency, start_level_max, 1200 /* max. internal depth */, 0, false, (unsigned int) SAMPLING_RATE);

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        pIntLFO->updateByMIDICtrlValue(127); // pro forma
        for (int i = 0; i < steps; ++i) {
            pDestinationBuffer[i] = pIntLFO->render() * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    float elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("int math solution elapsed time: %1.0f ms\n", elapsed_time);
    #endif

    return elapsed_time;
}

// integer math abs solution
float int_math_abs(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    pIntAbsLFO->trigger(frequency, start_level_max, 1200 /* max. internal depth */, 0, false, (unsigned int) SAMPLING_RATE);

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        pIntAbsLFO->updateByMIDICtrlValue(0); // pro forma
        for (int i = 0; i < steps; ++i) {
            pDestinationBuffer[i] = pIntAbsLFO->render() * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    float elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("int math abs solution elapsed time: %1.0f ms\n", elapsed_time);
    #endif

    return elapsed_time;
}

// table lookup solution (currently disabled)
//
// This solution is not yet implemented in LinuxSampler yet and probably
// never will, I simply haven't found an architectures / system where this
// turned out to be the best solution and it introduces too many problems
// anyway. If you found an architecture where this seems to be the best
// solution, please let us know!
#if 0
float table_lookup(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    const float r = frequency / SAMPLING_RATE; // frequency alteration quotient
    #if SIGNED
    float c = r * 4.0f;
    #else
    float c = r * 2.0f;
    #endif
    const int wl = (int) (SAMPLING_RATE / frequency); // wave length (in sample points)

    // 'volatile' to avoid the cache to fudge the benchmark result
    volatile float* pPrerenderingBuffer = new float[wl];

    // prerendering of the triangular wave
    {
        float level = 0.0f;
        for (int i = 0; i < wl; ++i) {
            level += c;
            #if SIGNED
            if (level >= 1.0f) {
                c = -c;
                level = 1.0f;
            }
            else if (level <= -1.0f) {
                c = -c;
                level = -1.0f;
            }
            #else
            if (level >= 1.0f) {
                c = -c;
                level = 1.0f;
            }
            else if (level <= 0.0f) {
                c = -c;
                level = 0.0f;
            }
            #endif
            pPrerenderingBuffer[i] = level;
        }
    }

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        for (int i = 0; i < steps; ++i) {
            pDestinationBuffer[i] = pPrerenderingBuffer[i % wl] * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    float elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("Table lookup solution elapsed time: %1.0f ms\n", elapsed_time);
    #endif

    if (pPrerenderingBuffer) delete[] pPrerenderingBuffer;

    return elapsed_time;
}
#endif

// numeric, di-harmonic solution
float numeric_di_harmonic_solution(smpl_t* pDestinationBuffer, float* pAmp, const int steps, const float frequency) {
    // pro forma
    pDiHarmonicLFO->trigger(frequency, start_level_max, 1200 /* max. internal depth */, 0, false, (unsigned int) SAMPLING_RATE);

    clock_t stop_time;
    clock_t start_time = clock();

    for (int run = 0; run < RUNS; run++) {
        pDiHarmonicLFO->updateByMIDICtrlValue(127); // pro forma
        for (int i = 0; i < steps; ++i) {
            pDestinationBuffer[i] = pDiHarmonicLFO->render() * pAmp[i]; // * pAmp[i] just to simulate some memory load
        }
    }

    stop_time = clock();
    float elapsed_time = (stop_time - start_time) / (double(CLOCKS_PER_SEC) / 1000.0);
    #if ! SILENT
    printf("Numeric harmonics solution elapsed time: %1.0f ms\n", elapsed_time);
    #endif

    return elapsed_time;
}

// output calculated values as RAW audio format (32 bit floating point, mono) file
void output_as_raw_file(const char* filename, smpl_t* pOutputBuffer, int steps) {
    FILE* file = fopen(filename, "w");
    if (file) {
        fwrite((void*) pOutputBuffer, sizeof(float), steps, file);
        fclose(file);
    } else {
        fprintf(stderr, "Could not open %s\n", filename);
    }
}

int main() {
    const int steps = STEPS;
    const int sinusoidFrequency = 100; // Hz

    #if ! SILENT
    # if SIGNED
    printf("Signed triangular wave benchmark\n");
    printf("--------------------------------\n");
    # else
    printf("Unsigned triangular wave benchmark\n");
    printf("----------------------------------\n");
    # endif
    #endif

    #if SIGNED
    pIntLFO        = new LFOTriangleIntMath<range_signed>(MAX);
    pIntAbsLFO     = new LFOTriangleIntAbsMath<range_signed>(MAX);
    pDiHarmonicLFO = new LFOTriangleDiHarmonic<range_signed>(MAX);
    #else // unsigned
    pIntLFO        = new LFOTriangleIntMath<range_unsigned>(MAX);
    pIntAbsLFO     = new LFOTriangleIntAbsMath<range_unsigned>(MAX);
    pDiHarmonicLFO = new LFOTriangleDiHarmonic<range_unsigned>(MAX);
    #endif

    // output buffer for the calculated sinusoid wave
    smpl_t* pOutputBuffer = new smpl_t[steps];
    // just an arbitrary amplitude envelope to simulate a bit higher memory bandwidth
    float* pAmplitude  = new float[steps];

    // pro forma - an arbitary amplitude envelope
    for (int i = 0; i < steps; ++i)
        pAmplitude[i] = (float) i / (float) steps;

    // how long each solution took (in seconds)
    float int_math_result, int_math_abs_result, /*table_lookup_result,*/ numeric_di_harmonic_result;

    int_math_result = int_math(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    #if OUTPUT_AS_RAW_WAVE
      output_as_raw_file("bench_int_math.raw", pOutputBuffer, steps);
    #endif

    int_math_abs_result = int_math_abs(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    #if OUTPUT_AS_RAW_WAVE
      output_as_raw_file("bench_int_math_abs.raw", pOutputBuffer, steps);
    #endif
    //table_lookup_result = table_lookup(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    //#if OUTPUT_AS_RAW_WAVE
    //  output_as_raw_file("bench_table_lookup.raw", pOutputBuffer, steps);
    //#endif
    numeric_di_harmonic_result = numeric_di_harmonic_solution(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    #if OUTPUT_AS_RAW_WAVE
      output_as_raw_file("bench_numeric_harmonics.raw", pOutputBuffer, steps);
    #endif

    #if ! SILENT
    printf("\nOK, 2nd try\n\n");
    #endif

    int_math_result            += int_math(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    int_math_abs_result = int_math_abs(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    //table_lookup_result        += table_lookup(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);
    numeric_di_harmonic_result += numeric_di_harmonic_solution(pOutputBuffer, pAmplitude, steps, sinusoidFrequency);

    if (pAmplitude)    delete[] pAmplitude;
    if (pOutputBuffer) delete[] pOutputBuffer;

    if (pIntLFO)        delete pIntLFO;
    if (pDiHarmonicLFO) delete pDiHarmonicLFO;

    if (int_math_abs_result <= int_math_result && int_math_abs_result <= numeric_di_harmonic_result) return INT_MATH_ABS_SOLUTION;
    if (int_math_result <= int_math_abs_result && int_math_result <= numeric_di_harmonic_result) return INT_MATH_SOLUTION;
    if (numeric_di_harmonic_result <= int_math_abs_result && numeric_di_harmonic_result <= int_math_result) return DI_HARMONIC_SOLUTION;

    return INVALID_RESULT; // error
}

/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2016 Christian Schoenebeck                       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the Free Software           *
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston,                 *
 *   MA  02111-1307  USA                                                   *
 ***************************************************************************/

#include "RTMath.h"

// for unsafeMicroSeconds() implementation
#if !defined(WIN32) && !defined(__APPLE__)
# include <time.h>
#endif

static float CentsToFreqTable[CONFIG_MAX_PITCH * 1200 * 2 + 1]; // +-1200 cents per octave

float* RTMathBase::pCentsToFreqTable(InitCentsToFreqTable());

#if defined(__APPLE__)
#include <mach/mach_time.h>
typedef uint64_t time_stamp_t;
#endif

#if defined(__arm__) || defined(__aarch64__)
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/perf_event.h>

static int fddev = -1; 
__attribute__((constructor))
static void __init_z(void)
{
    static struct perf_event_attr attr;
    attr.type = PERF_TYPE_HARDWARE;
    attr.config = PERF_COUNT_HW_CPU_CYCLES;
    fddev = syscall(__NR_perf_event_open, &attr, 0, -1, -1, 0); 
}

__attribute__((destructor))
static void __fini_z(void)
{
    close(fddev);
}
#endif

/*
 * Creates a real time stamp for the current moment. Out of efficiency this
 * is implemented in inline assembly for each CPU independently; we currently
 * don't use a generic solution for CPUs that are not yet covered by the
 * assembly code, instead an error message is prompted on compile time, forcing
 * the user to contact us.
 */
RTMathBase::time_stamp_t RTMathBase::CreateTimeStamp() {
    #if defined(__i386__) || defined(__x86_64__)
    uint64_t t;
    __asm__ __volatile__ ("rdtsc" : "=A" (t));
    return time_stamp_t(t >> 8);
    #elif defined(__ia64__)
    time_stamp_t t;
    __asm__ __volatile__ ("mov %0=ar.itc" : "=r"(t));
    return t;
    #elif defined(__powerpc__)
    time_stamp_t t;
    __asm__ __volatile__ (
        "98:	mftb %0\n"
        "99:\n"
        ".section __ftr_fixup,\"a\"\n"
        "	.long %1\n"
        "	.long 0\n"
        "	.long 98b\n"
        "	.long 99b\n"
        ".previous"
        : "=r" (t) : "i" (0x00000100)
    );
    return t;
    #elif defined(__alpha__)
    time_stamp_t t;
    __asm__ __volatile__ ("rpcc %0" : "=r"(t));
    return t;
    #elif defined(__APPLE__)
    return (time_stamp_t) mach_absolute_time();
    #elif defined(__arm__) || defined(__aarch64__)
    long long result = 0;
    if (fddev == -1 || read(fddev, &result, sizeof(result)) < sizeof(result))
        return 0;
    return result;
    #else // we don't want to use a slow generic solution
    #  error "Sorry, LinuxSampler lacks time stamp code for your system."
    #  error "Please report this error and the CPU you are using to the LinuxSampler developers mailing list!"
    #endif
}

RTMathBase::usecs_t RTMathBase::unsafeMicroSeconds(clock_source_t source) {
    #if defined(WIN32)
    LARGE_INTEGER t;
    LARGE_INTEGER f;
    QueryPerformanceCounter(&t);
    if (!QueryPerformanceFrequency(&f)) return 0;
    return usecs_t( double(t.QuadPart) / double(f.QuadPart) * 1000000.0 );
    #elif defined(__APPLE__)
    static mach_timebase_info_data_t tb;
    double t = mach_absolute_time();
    // if this method is run for the first time, get the internal time base
    if (!tb.denom) mach_timebase_info(&tb); // get nanoseconds per tick
    // convert from internal (abstract) time value to microseconds
    return usecs_t( t * double(tb.numer) / double(tb.denom) / 1000.0 );
    #else
    timespec t;
    clockid_t cid;
    switch (source) {
        case process_clock: cid = CLOCK_PROCESS_CPUTIME_ID; break;
        case thread_clock:  cid = CLOCK_THREAD_CPUTIME_ID; break;
        case real_clock:    cid = CLOCK_MONOTONIC; break;
        default:            return 0;
    }
    clock_gettime(cid, &t);
    return usecs_t( (double(t.tv_sec) * 1000000000.0 + double(t.tv_nsec)) / 1000.0 );
    #endif
}

/**
 * Will automatically be called once to initialize the 'Cents to frequency
 * ratio' table.
 */
float* RTMathBase::InitCentsToFreqTable() {
    float* pMiddleOfTable = &CentsToFreqTable[CONFIG_MAX_PITCH * 1200];
    for (int i = -CONFIG_MAX_PITCH * 1200; i <= CONFIG_MAX_PITCH * 1200; i++) {
        pMiddleOfTable[i] = pow(TWELVEHUNDREDTH_ROOT_OF_TWO, i);
    }
    return pMiddleOfTable;
}

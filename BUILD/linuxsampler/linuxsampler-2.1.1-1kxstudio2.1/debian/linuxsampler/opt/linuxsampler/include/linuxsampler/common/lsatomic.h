/***************************************************************************
 *                                                                         *
 *   Copyright (C) 2008-2013 Andreas Persson                               *
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
 *   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston,                *
 *   MA  02110-1301  USA                                                   *
 ***************************************************************************/

#ifndef LSATOMIC_H
#define LSATOMIC_H

/** @file
 *
 * Implementation of a small subset of the C++11 atomic operations.
 *
 * Note: When working with multithreading on modern CPUs, it's
 * important not only to make sure that concurrent access to shared
 * variables is made atomically, but also to be aware of the order the
 * stores get visible to the loads in other threads. For example, if x
 * and y are shared variables with initial values of 0, the following
 * program:
 *
 * @code
 *  // thread 1:
 *  x.store(1, memory_order_relaxed);
 *  r1 = y.load(memory_order_relaxed);
 *
 *  // thread 2:
 *  y.store(1, memory_order_relaxed);
 *  r2 = x.load(memory_order_relaxed);
 * @endcode
 *
 * would have a possible outcome of r1 == 0 and r2 == 0. The threads
 * might for example run on separate CPU cores with separate caches,
 * and the propagation of the store to the other core might be delayed
 * and done after the loads. In that case, both loads will read the
 * original value of 0 from the core's own cache.
 *
 * The C++11 style operations use the memory_order parameter to let
 * the programmer control the way shared memory stores get visible to
 * loads in other threads. In the example above, relaxed order was
 * used, which allows the CPU and compiler to reorder the memory
 * accesses very freely. If memory_order_seq_cst had been used
 * instead, the r1 == 0 and r2 == 0 outcome would have been
 * impossible, as sequential consistency means that the execution of
 * the program can be modeled by simply interleaving the instructions
 * of the threads.
 *
 * The default order is memory_order_seq_cst, as it is the easiest one
 * to understand. It is however also the slowest. The relaxed order is
 * the fastest, but it can't be used if the shared variable is used to
 * synchronize threads for any other shared data. The third order is
 * acquire/release, where an acquire-load is synchronizing with a
 * release-store to the same variable.
 *
 * See for example http://gcc.gnu.org/wiki/Atomic/GCCMM/AtomicSync for
 * more information about the memory order parameter.
 *
 * The supported operations of the implementation in this file are:
 *
 * - fences (acquire, release and seq_cst)
 *
 * - load and store of atomic<int> with relaxed, acquire/release or
 *   seq_cst memory ordering
 *
 * The supported architectures are x86, powerpc and ARMv7.
 */


// if C++11 and gcc 4.7 or later is used, then use the standard
// implementation
#if __cplusplus >= 201103L && \
    (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))

#include <atomic>

namespace LinuxSampler {
    using std::memory_order_relaxed;
    using std::memory_order_acquire;
    using std::memory_order_release;
    using std::memory_order_seq_cst;
    using std::atomic_thread_fence;
    using std::atomic;
}

#else


namespace LinuxSampler {
    enum memory_order {
        memory_order_relaxed, memory_order_acquire,
        memory_order_release, memory_order_seq_cst
    };

    inline void atomic_thread_fence(memory_order order) {
        switch (order) {
        case memory_order_relaxed:
            break;

        case memory_order_acquire:
        case memory_order_release:
#ifdef _ARCH_PPC64
            asm volatile("lwsync" : : : "memory");
#elif defined(_ARCH_PPC)
            asm volatile("sync" : : : "memory");
#elif defined(__ARM_ARCH_7A__)
            asm volatile("dmb" : : : "memory");
#else
            asm volatile("" : : : "memory");
#endif
            break;

        case memory_order_seq_cst:
#ifdef _ARCH_PPC
            asm volatile("sync" : : : "memory");
#elif defined(__i386__)
            asm volatile("lock; addl $0,0(%%esp)" : : : "memory");
#elif defined(__x86_64__)
            asm volatile("mfence" : : : "memory");
#elif defined(__ARM_ARCH_7A__)
            asm volatile("dmb" : : : "memory");
#else
            asm volatile("" : : : "memory");
#endif
            break;
        }
    }

    template<typename T> class atomic;
    template<> class atomic<int> { // int is the only implemented type
    public:
        atomic() { }
        explicit atomic(int m) : f(m) { }
        int load(memory_order order = memory_order_seq_cst) const volatile {
            int m;
            switch (order) {
            case memory_order_relaxed:
                m = f;
                break;

            case memory_order_seq_cst:
            case memory_order_release: // (invalid)
#ifdef _ARCH_PPC
                atomic_thread_fence(memory_order_seq_cst);
#endif
                // fall-through

            case memory_order_acquire:
#ifdef _ARCH_PPC
                // PPC load-acquire: artificial dependency + isync
                asm volatile(
                    "lwz%U1%X1 %0,%1\n\t"
                    "cmpw %0,%0\n\t"
                    "bne- 1f\n\t"
                    "1: isync"
                    : "=r" (m)
                    : "m"  (f)
                    : "memory", "cr0");
#else
                m = f;
                atomic_thread_fence(memory_order_acquire);
#endif
                break;
            }
            return m;
        }

        void store(int m, memory_order order = memory_order_seq_cst) volatile {
            switch (order) {
            case memory_order_relaxed:
                f = m;
                break;

            case memory_order_release:
                atomic_thread_fence(memory_order_release);
                f = m;
                break;

            case memory_order_seq_cst:
            case memory_order_acquire: // (invalid)
#ifdef _ARCH_PPC
                atomic_thread_fence(memory_order_seq_cst);
                f = m;
#else
                atomic_thread_fence(memory_order_release);
                f = m;
                atomic_thread_fence(memory_order_seq_cst);
#endif
                break;
            }
        }
    private:
        int f;
        atomic(const atomic&); // not allowed
        atomic& operator=(const atomic&); // not allowed
    };
}
#endif
#endif

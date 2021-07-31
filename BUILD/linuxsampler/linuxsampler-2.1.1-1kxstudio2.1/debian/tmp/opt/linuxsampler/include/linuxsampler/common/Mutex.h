/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2017 Christian Schoenebeck                       *
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

#ifndef __MUTEX_H__
#define __MUTEX_H__

#if defined(WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

namespace LinuxSampler {

/** @brief Mutual exclusive objects
 *
 * This class provides the classical thread / process synchronisation
 * technique called Mutex. It is used to protect critical sections, that is
 * resources (typically data structures) from being used at the same time by
 * different threads or processes which otherwise might turn into undefined
 * and of course undesired behavior.
 *
 * Note: as this technique might block the calling thread and also implies
 * a system call, this should not be used directly in realtime sensitive
 * threads!
 */
class Mutex {
    public:
        enum type_t {
            RECURSIVE,
            NON_RECURSIVE
        };

        /** @brief Constructor
         *
         * Creates a new Mutex object. The optional @a type argument defines
         * the fundamental behavior of the Mutex object:
         *
         * - If @c RECURSIVE is passed (which is the default type) then the
         *   mutex will manage an additional lock count such that it allows the
         *   same thread to call Lock() multiple times; each time that thread
         *   calls Lock() the lock count will be increased by one, each time it
         *   calls Unlock() it will be decreased by one, and other threads will
         *   only be unblocked once the lock count fell to zero again.
         *
         * - If @c NON_RECURSIVE is passed then it is considered to be an error
         *   if the same thread calls Lock() while already owning the lock, and
         *   likewise it is considered to be an error if Unlock() is called if
         *   the calling thread hasn't locked the mutex.
         *
         * You should invest the required time to review your design in order to
         * decide which mutex behavior fits to your design. Even though it might
         * be tempting to stick with the lazy approach by using the @c RECURSIVE
         * type, using the @c NON_RECURSIVE type does make sense if your design
         * does not require a recursive mutex, because modern developer tools
         * assist you spotting potential threading bugs in your code while using
         * the @c NON_RECURSIVE type which can avoid developers' biggest fear of
         * undefined behavior, plus also keep in mind that certain OS APIs are
         * not compatible with recursive mutexes at all!
         *
         * @param type - optional: the fundamental behavior type for this mutex
         *               (default: @c RECURSIVE)
         */    
        Mutex(type_t type = RECURSIVE);

        /**
         * Destructor
         */
        virtual ~Mutex();

        /** @brief Lock this Mutex.
         *
         * If this Mutex object is currently be locked by another thread,
         * then the calling thread will be blocked until the other thread
         * unlocks this Mutex object. The calling thread though can safely
         * call this method several times without danger to be blocked
         * himself.
         *
         * The calling thread should call Unlock() as soon as the critical
         * section was left.
         */      
        void Lock();

        /** @brief Try to lock this Mutex.
         *
         * Same as Lock() except that this method won't block the calling
         * thread in case this Mutex object is currently locked by another
         * thread. So this call will always immediately return and the
         * return value has to be checked if the locking request was
         * successful or not.
         *
         * @returns  true if the Mutex object could be locked, false if the
         *           Mutex is currently locked by another thread
         */
        bool Trylock();

        /** @brief Unlock this Mutex.
         *
         * If other threads are currently blocked and waiting due to a
         * Lock() call, one of them will be awaken.
         */
        void Unlock();

    protected:
    #if defined(WIN32)
        HANDLE hMutex;
    #else
        pthread_mutex_t     __posix_mutex;
        pthread_mutexattr_t __posix_mutexattr;
    #endif
        type_t type;
};

// Lock guard for exception safe locking
class LockGuard {
public:
    LockGuard(Mutex& m) : pm(&m) {
        m.Lock();
    }

    /**
     * Empty LockGuard. This constructor can be used to implement conditional
     * mutex protection like:
     * @code
     * Mutex m;
     * LockGuard g;
     * if (requiresMutexProtection()) g = LockGuard(m);
     * @endcode
     */
    LockGuard() : pm(NULL) {
    }

    LockGuard(const LockGuard& g) : pm(g.pm) {
        if (pm) pm->Lock();
    }

    ~LockGuard() {
        if (pm) pm->Unlock();
    }
private:
    Mutex* pm;
};

} // namespace LinuxSampler

#endif // __MUTEX_H__

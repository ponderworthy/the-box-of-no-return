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

#include <sys/time.h>

#include "Condition.h"

#include "global_private.h"

#if CONFIG_PTHREAD_TESTCANCEL
#include "Thread.h"
#endif

namespace LinuxSampler {

// *************** Internal data types (for Windows only) ***************
// *

#if defined(WIN32)

typedef HANDLE win32thread_mutex_t;

//NOTE: since pthread_condattr_t is not needed in the Condition class so just set it to int
typedef int win32thread_condattr_t;

typedef struct {
    unsigned long tv_sec;
    unsigned long tv_nsec;
} win32_timespec;



// *************** ConditionInternal (for Windows only) ***************
// *

class ConditionInternal {
public:

static
int win32thread_cond_init (Condition::win32thread_cond_t *cv, const win32thread_condattr_t *)
{
    dmsg(7,("win32thread_cond_init:\n"));
    cv->waiters_count_ = 0;
    cv->was_broadcast_ = 0;
    cv->sema_ = CreateSemaphore (NULL,       // no security
                                 0,          // initially 0
                                 0x7fffffff, // max count
                                 NULL);      // unnamed
    dmsg(7,("win32thread_cond_init: after CreateSemaphore retval=%d\n",cv->sema_));
    InitializeCriticalSection (&cv->waiters_count_lock_);
    cv->waiters_done_ = CreateEvent (NULL,  // no security
                                   FALSE, // auto-reset
                                   FALSE, // non-signaled initially
                                   NULL); // unnamed
    dmsg(7,("win32thread_cond_init: after CreateEvent retval=%d\n",cv->waiters_done_));

    return 0;
}

static
int win32thread_cond_destroy(Condition::win32thread_cond_t *cv) {
  dmsg(7,("win32thread_cond_destroy ptr=%d\n",cv));
  CloseHandle(cv->waiters_done_);
  DeleteCriticalSection(&cv->waiters_count_lock_);
  CloseHandle(cv->sema_);
  return 0;
}

static
int win32thread_cond_timedwait (Condition::win32thread_cond_t *cv, win32thread_mutex_t *external_mutex, win32_timespec *timeout)
{
    dmsg(7,("win32thread_cond_timedwait: external mutex=%d BEGIN!\n",external_mutex));
    // Avoid race conditions.
    dmsg(7,("win32thread_cond_timedwait: before EnterCriticalSection (&cv->waiters_count_lock_) cv->waiters_count_=%d\n",cv->waiters_count_));
    EnterCriticalSection (&cv->waiters_count_lock_);
    cv->waiters_count_++;
    LeaveCriticalSection (&cv->waiters_count_lock_);
    dmsg(7,("win32thread_cond_timedwait: after LeaveCriticalSection (&cv->waiters_count_lock_) cv->waiters_count_=%d\n",cv->waiters_count_));


    // This call atomically releases the mutex and waits on the
    // semaphore until <pthread_cond_signal> or <pthread_cond_broadcast>
    // are called by another thread.

    DWORD dwMilliseconds;
    if(timeout->tv_sec || timeout->tv_nsec) {
        dwMilliseconds = timeout->tv_sec * 1000 + timeout->tv_nsec / 1000000;
    }
    else {
        dmsg(7,("win32thread_cond_timedwait: dwMilliseconds = INFINITE\n"));
        dwMilliseconds = INFINITE;
    }
    dmsg(7,("win32thread_cond_timedwait: before SignalObjectAndWait(*external_mutex, cv->sema_, dwMilliseconds, FALSE), dwMilliseconds=%d\n",dwMilliseconds));
    DWORD res;
    res = SignalObjectAndWait (*external_mutex, cv->sema_, dwMilliseconds, FALSE);
    dmsg(7,("win32thread_cond_timedwait: after SignalObjectAndWait, res=%d\n",res));
    if(res == WAIT_TIMEOUT) return -1;

    // Reacquire lock to avoid race conditions.
    dmsg(7,("win32thread_cond_timedwait: before EnterCriticalSection (2) (&cv->waiters_count_lock_) cv->waiters_count=%d\n",cv->waiters_count_));
    EnterCriticalSection (&cv->waiters_count_lock_);
    dmsg(7,("win32thread_cond_timedwait: after EnterCriticalSection (2) (&cv->waiters_count_lock_) cv->waiters_count=%d\n",cv->waiters_count_));

    // We're no longer waiting...
    cv->waiters_count_--;

    // Check to see if we're the last waiter after <pthread_cond_broadcast>.
    int last_waiter = cv->was_broadcast_ && cv->waiters_count_ == 0;

    LeaveCriticalSection (&cv->waiters_count_lock_);
    dmsg(7,("win32thread_cond_timedwait: after LeaveCriticalSection (2) (&cv->waiters_count_lock_) last_waiter=%d\n",last_waiter));

    // If we're the last waiter thread during this particular broadcast
    // then let all the other threads proceed.
    if (last_waiter) {
        // This call atomically signals the <waiters_done_> event and waits until
        // it can acquire the <external_mutex>.  This is required to ensure fairness.
        dmsg(7,("win32thread_cond_timedwait: before SignalObjectAndWait (cv->waiters_done_, *external_mutex, dwMilliseconds, FALSE) \n"));
        res = SignalObjectAndWait (cv->waiters_done_, *external_mutex, dwMilliseconds, FALSE);
        dmsg(7,("win32thread_cond_timedwait: after SignalObjectAndWait (cv->waiters_done_, *external_mutex, dwMilliseconds, FALSE) res=%d\n",res));
        if(res == WAIT_TIMEOUT) {
            dmsg(7,("win32thread_cond_timedwait: after SignalObjectAndWait: WAIT_TIMEOUT! returning -1\n"));
            return -1;
        }
    }
    else {
        // Always regain the external mutex since that's the guarantee we
        // give to our callers.
        dmsg(7,("win32thread_cond_timedwait: before WaitForSingleObject (*external_mutex, dwMilliseconds)\n"));
        res = WaitForSingleObject (*external_mutex, dwMilliseconds);
        dmsg(7,("win32thread_cond_timedwait: after WaitForSingleObject (*external_mutex, dwMilliseconds) res=%d\n",res));
        if(res == WAIT_TIMEOUT) {
            dmsg(7,("win32thread_cond_timedwait: after WaitForSingleObject: WAIT_TIMEOUT ! returning -1\n"));
            return -1;
        }
    }
    dmsg(7,("win32thread_cond_timedwait: all OK. returning 0\n"));
    return 0;
}

static
int win32thread_cond_broadcast (Condition::win32thread_cond_t *cv)
{
    DWORD res;
    dmsg(7,("win32thread_cond_broadcast: cv=%d\n",cv));
    dmsg(7,("win32thread_cond_broadcast: before EnterCriticalSection (&cv->waiters_count_lock_)\n"));
    // This is needed to ensure that <waiters_count_> and <was_broadcast_> are
    // consistent relative to each other.
    EnterCriticalSection (&cv->waiters_count_lock_);
    int have_waiters = 0;

    dmsg(7,("win32thread_cond_broadcast: cv->waiters_count_=%d\n",cv->waiters_count_));
    if (cv->waiters_count_ > 0) {
        dmsg(7,("win32thread_cond_broadcast: cv->waiters_count > 0 !\n"));
        // We are broadcasting, even if there is just one waiter...
        // Record that we are broadcasting, which helps optimize
        // <pthread_cond_wait> for the non-broadcast case.
        cv->was_broadcast_ = 1;
        have_waiters = 1;
    }
    dmsg(7,("win32thread_cond_broadcast: cv->was_broadcast_=%d  have_waiters=%d\n",cv->was_broadcast_,have_waiters));

    if (have_waiters) {
        // Wake up all the waiters atomically.
        dmsg(7,("win32thread_cond_broadcast: have_waiters ! before ReleaseSemaphore (cv->sema_, cv->waiters_count_, 0);\n"));
        ReleaseSemaphore (cv->sema_, cv->waiters_count_, 0);

        LeaveCriticalSection (&cv->waiters_count_lock_);
        dmsg(7,("win32thread_cond_broadcast: have_waiters ! after LeaveCriticalSection (&cv->waiters_count_lock_)\n"));

        // Wait for all the awakened threads to acquire the counting
        // semaphore.
        dmsg(7,("win32thread_cond_broadcast: before WaitForSingleObject (cv->waiters_done_, INFINITE)\n"));
        res = WaitForSingleObject (cv->waiters_done_, INFINITE);
        dmsg(7,("win32thread_cond_broadcast: after WaitForSingleObject (cv->waiters_done_, INFINITE) res=%d\n",res));
        // This assignment is okay, even without the <waiters_count_lock_> held
        // because no other waiter threads can wake up to access it.
        cv->was_broadcast_ = 0;
    }
    else {
        LeaveCriticalSection (&cv->waiters_count_lock_);
        dmsg(7,("win32thread_cond_broadcast: after LeaveCriticalSection (&cv->waiters_count_lock_)\n"));
    }
    dmsg(7,("win32thread_cond_broadcast: all OK, returning 0.\n"));
    return 0;
}

/* TODO: the following two functions are currently not used yet
static
int win32thread_cond_signal (Condition::win32thread_cond_t *cv)
{

    dmsg(7,("win32thread_cond_signal cv=%d\n",cv));
    dmsg(7,("win32thread_cond_signal before EnterCriticalSection (&cv->waiters_count_lock_)\n"));
    EnterCriticalSection (&cv->waiters_count_lock_);
    int have_waiters = cv->waiters_count_ > 0;
    LeaveCriticalSection (&cv->waiters_count_lock_);
    dmsg(7,("win32thread_cond_signal after LeaveCriticalSection (&cv->waiters_count_lock_)\n"));

    dmsg(7,("win32thread_cond_signal have_waiters=%d\n",have_waiters));
    // If there aren't any waiters, then this is a no-op.
    if (have_waiters) {
        dmsg(7,("win32thread_cond have_waiters is TRUE,  before ReleaseSemaphore (cv->sema_, 1, 0)\n"));
        ReleaseSemaphore (cv->sema_, 1, 0);
    }
    dmsg(7,("win32thread_cond_signal: all OK. returning 0\n"));
    return 0;
}

static
int win32thread_cond_wait (Condition::win32thread_cond_t *cv, win32thread_mutex_t *external_mutex) {
    dmsg(7,("win32thread_cond_wait: (calls win32thread_cond_timedwait)  cv=%d  external_mutex=%d\n",cv, external_mutex));
    win32_timespec timeout;
    timeout.tv_sec = 0;
    timeout.tv_nsec = 0;
    return win32thread_cond_timedwait (cv, external_mutex, &timeout);
}
*/

}; // class ConditionInternal

#endif // WIN32



// *************** Condition ***************
// *

Condition::Condition(bool bInitialCondition, Mutex::type_t mutexType)
    : Mutex(mutexType)
{
dmsg(7,("Condition:: constructor, bInitialCondition=%d\n", bInitialCondition));
#if defined(WIN32)
    ConditionInternal::win32thread_cond_init(&__win32_true_condition, NULL);
    ConditionInternal::win32thread_cond_init(&__win32_false_condition, NULL);
#else
    pthread_cond_init(&__posix_true_condition, NULL);
    pthread_cond_init(&__posix_false_condition, NULL);
#endif
    bCondition = bInitialCondition;
}

Condition::~Condition() {
#if defined(WIN32)
    ConditionInternal::win32thread_cond_destroy(&__win32_true_condition);
    ConditionInternal::win32thread_cond_destroy(&__win32_false_condition);
#else
    pthread_cond_destroy(&__posix_true_condition);
    pthread_cond_destroy(&__posix_false_condition);
#endif
}

#ifndef WIN32
namespace {
    // If the thread is cancelled while waiting for the condition
    // variable, the mutex will be locked. It needs to be unlocked so
    // the Condition can be reused if the thread is restarted.
    void condition_cleanup(void* c) {
        static_cast<Condition*>(c)->Unlock();
    }
}
#endif

int Condition::WaitIfInternal(bool bLock, bool bCondition, long TimeoutSeconds, long TimeoutNanoSeconds) {
    dmsg(7,("Condition::WaitIfInternal: bCondition=%d  TimeoutSeconds=%ld  TimeoutNanoSeconds=%ld\n",bCondition, TimeoutSeconds, TimeoutNanoSeconds));
    if (bLock) {
        dmsg(7,("Condition::WaitIfInternal() -> LOCK()\n"));
        Lock();
        dmsg(7,("Condition::WaitIfInternal() -> LOCK() passed\n"));
    }
    int res = 0;
    #ifndef WIN32
    pthread_cleanup_push(condition_cleanup, this);
    #endif
    if (this->bCondition == bCondition) {
        if (bCondition) { // wait until condition turned 'false'
            #if defined(WIN32)
            win32_timespec timeout;
            timeout.tv_sec  = TimeoutSeconds;
            timeout.tv_nsec = TimeoutNanoSeconds;
            dmsg(7,("Condition::WaitIfInternal() -> waiting for 'false' condition with timeout\n"));
            res = ConditionInternal::win32thread_cond_timedwait(&__win32_false_condition, &hMutex, &timeout);
            dmsg(7,("Condition::WaitIfInternal() -> awakened from 'false' condition waiting\n"));
            #else
            if (TimeoutSeconds || TimeoutNanoSeconds) { // wait with timeout
                struct timeval now;
                gettimeofday(&now, 0);
                timespec timeout;
                timeout.tv_sec  = now.tv_sec + TimeoutSeconds;
                timeout.tv_nsec = now.tv_usec * 1000 + TimeoutNanoSeconds;
                dmsg(7,("Condition::WaitIfInternal() -> waiting for 'false' condition with timeout\n"));
                res = pthread_cond_timedwait(&__posix_false_condition, &__posix_mutex, &timeout);
                dmsg(7,("Condition::WaitIfInternal() -> awakened from 'false' condition waiting\n"));
            }
            else { // wait without timeout
                dmsg(7,("Condition::WaitIfInternal() -> waiting for 'false' condition\n"));
                pthread_cond_wait(&__posix_false_condition, &__posix_mutex);
                dmsg(7,("Condition::WaitIfInternal() -> awakened from 'false' condition waiting\n"));
            }
            #endif
        }
        else { // wait until condition turned 'true'
            #if defined(WIN32)
            win32_timespec timeout;
            timeout.tv_sec  = TimeoutSeconds;
            timeout.tv_nsec = TimeoutNanoSeconds;
            dmsg(7,("Condition::WaitIfInternal() -> waiting for 'true' condition with timeout\n"));
            res = ConditionInternal::win32thread_cond_timedwait(&__win32_true_condition, &hMutex, &timeout);
            dmsg(7,("Condition::WaitIfInternal() -> awakened from 'true' condition waiting\n"));
            #else
            if (TimeoutSeconds || TimeoutNanoSeconds) { // wait with timeout
                struct timeval now;
                gettimeofday(&now, 0);
                timespec timeout;
                timeout.tv_sec  = now.tv_sec + TimeoutSeconds;
                timeout.tv_nsec = now.tv_usec * 1000 + TimeoutNanoSeconds;
                dmsg(7,("Condition::WaitIfInternal() -> waiting for 'true' condition with timeout\n"));
                res = pthread_cond_timedwait(&__posix_true_condition, &__posix_mutex, &timeout);
                dmsg(7,("Condition::WaitIfInternal() -> awakened from 'true' condition waiting\n"));
            }
            else { // wait without timeout
                dmsg(7,("Condition::WaitIfInternal() -> waiting for 'true' condition\n"));
                pthread_cond_wait(&__posix_true_condition, &__posix_mutex);
                dmsg(7,("Condition::WaitIfInternal() -> awakened from 'true' condition waiting\n"));
            }
            #endif
        }
    }
    #ifndef WIN32
    pthread_cleanup_pop(0);
    #endif
    return res;
}

int Condition::WaitIf(bool bCondition, long TimeoutSeconds, long TimeoutNanoSeconds) {
    return WaitIfInternal(true/*do lock*/, bCondition, TimeoutSeconds, TimeoutNanoSeconds);
}

int Condition::PreLockedWaitIf(bool bCondition, long TimeoutSeconds, long TimeoutNanoSeconds) {
    return WaitIfInternal(false/*don't lock*/, bCondition, TimeoutSeconds, TimeoutNanoSeconds);
}

int Condition::WaitAndUnlockIf(bool bCondition, long TimeoutSeconds, long TimeoutNanoSeconds) {
    int res = WaitIfInternal(true/*do lock*/, bCondition, TimeoutSeconds, TimeoutNanoSeconds);
    dmsg(7,("Condition::WaitAndUnlockIf() -> UNLOCK()\n"));
    Unlock();
    dmsg(7,("Condition::WaitAndUnlockIf() -> UNLOCK() passed\n"));
    return res;
}

int Condition::PreLockedWaitAndUnlockIf(bool bCondition, long TimeoutSeconds, long TimeoutNanoSeconds) {
    int res = WaitIfInternal(false/*don't lock*/, bCondition, TimeoutSeconds, TimeoutNanoSeconds);
    dmsg(7,("Condition::PreLockedWaitAndUnlockIf() -> UNLOCK()\n"));
    Unlock();
    dmsg(7,("Condition::PreLockedWaitAndUnlockIf() -> UNLOCK() passed\n"));
    return res;
}

void Condition::SetInternal(bool bLock, bool bCondition) {
    dmsg(7,("Condition::Set() -> LOCK()\n"));
    LockGuard lock = (bLock) ? LockGuard(*this) : LockGuard();
    dmsg(7,("Condition::Set() -> LOCK() passed\n"));
    if (this->bCondition != bCondition) {
        this->bCondition = bCondition;
        if (bCondition) {
            dmsg(7,("Condition::Set() -> broadcasting 'true' condition\n"));
            #if defined(WIN32)
            ConditionInternal::win32thread_cond_broadcast(&__win32_true_condition);
            #else
            pthread_cond_broadcast(&__posix_true_condition);
            #endif
        }
        else {
            dmsg(7,("Condition::Set() -> broadcasting 'false' condition\n"));
            #if defined(WIN32)
            ConditionInternal::win32thread_cond_broadcast(&__win32_false_condition);
            #else
            pthread_cond_broadcast(&__posix_false_condition);
            #endif
        }
    }
}

void Condition::Set(bool bCondition) {
    SetInternal(true/*do lock*/, bCondition);
}

void Condition::PreLockedSet(bool bCondition) {
    SetInternal(false/*don't lock*/, bCondition);
}

bool Condition::GetUnsafe() {
    return bCondition;
}

#ifdef WIN32
void Condition::Reset() {
    __win32_true_condition.waiters_count_ = 0;
    __win32_true_condition.was_broadcast_ = 0;
    __win32_false_condition.waiters_count_ = 0;
    __win32_false_condition.was_broadcast_ = 0;
}
#endif

} // namespace LinuxSampler

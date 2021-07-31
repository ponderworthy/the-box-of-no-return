/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2019 Christian Schoenebeck                       *
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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#if defined(__APPLE__)
# include <sys/cdefs.h> // defines the system macros checked below
#endif

#ifndef _GNU_SOURCE
# define _GNU_SOURCE 1 /* so _XOPEN_SOURCE will be defined by features.h */
#endif

#ifdef HAVE_FEATURES_H
# include <features.h>
#endif

#if !defined(WIN32)
# if !defined(_XOPEN_SOURCE) || _XOPEN_SOURCE < 500
#  undef _XOPEN_SOURCE
#  define _XOPEN_SOURCE 500 /* to define PTHREAD_MUTEX_RECURSIVE */
#  if (!defined(POSIX_C_SOURCE) || POSIX_C_SOURCE < 199801L) && !__DARWIN_UNIX03
#   warning "Seems you don't have a UNIX98 compatible system."
#   warning "Please run LinuxSampler's selftest to make sure this won't be a problem!"
#   warning "(compile tests with 'make tests', run them with 'src/testcases/linuxsamplertest')"
#  endif
# endif
#endif

#include <iostream>
#include <errno.h>
#include <stdlib.h> /* for exit(int) */

#include "Mutex.h"

namespace LinuxSampler {

Mutex::Mutex(type_t type) {
#if defined(WIN32)
    hMutex = CreateMutex( NULL, FALSE, NULL);
    if (hMutex == NULL) {
        std::cerr << "Mutex Constructor: Fatal error - CreateMutex error " << GetLastError() << "\n";
        exit(1);
    }
#else
    pthread_mutexattr_init(&__posix_mutexattr);
    // pthread_mutexattr_settype() only works on UNIX98 compatible systems
    switch (type) {
    case RECURSIVE:
        if (pthread_mutexattr_settype(&__posix_mutexattr, PTHREAD_MUTEX_RECURSIVE)) {
            std::cerr << "Mutex Constructor: Fatal error - unable to pthread_mutexattr_settype(PTHREAD_MUTEX_RECURSIVE)\n" << std::flush;
            exit(-1);
        }
        break;
    case NON_RECURSIVE:
        if (pthread_mutexattr_settype(&__posix_mutexattr, PTHREAD_MUTEX_ERRORCHECK)) {
            std::cerr << "Mutex Constructor: Fatal error - unable to pthread_mutexattr_settype(PTHREAD_MUTEX_ERRORCHECK)\n" << std::flush;
            exit(-1);
        }
        break;
    default:
        std::cerr << "Mutex Constructor: Fatal error - Unknown mutex type requested\n" << std::flush;
        exit(-1);
        break;
    }
    pthread_mutex_init(&__posix_mutex, &__posix_mutexattr);
#endif
}

Mutex::~Mutex() {
#if defined(WIN32)
    CloseHandle(hMutex);
#else
    pthread_mutex_destroy(&__posix_mutex);
    pthread_mutexattr_destroy(&__posix_mutexattr);
#endif
}

void Mutex::Lock() {
#if defined(WIN32)
    WaitForSingleObject(hMutex, INFINITE);
#else
    pthread_mutex_lock(&__posix_mutex);
#endif
}

bool Mutex::Trylock() {
#if defined(WIN32)
    if( WaitForSingleObject(hMutex, 0) == WAIT_TIMEOUT) return false;
    return true;
#else
    if (pthread_mutex_trylock(&__posix_mutex) == EBUSY)
        return false;
    return true;
#endif
}

void Mutex::Unlock() {
#if defined(WIN32)
    ReleaseMutex(hMutex);
#else
    pthread_mutex_unlock(&__posix_mutex);
#endif
}

} // namespace LinuxSampler

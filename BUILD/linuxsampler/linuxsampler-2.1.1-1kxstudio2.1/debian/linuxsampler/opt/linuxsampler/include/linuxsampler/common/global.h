/***************************************************************************
 *                                                                         *
 *   LinuxSampler - modular, streaming capable sampler                     *
 *                                                                         *
 *   Copyright (C) 2003, 2004 by Benno Senoner and Christian Schoenebeck   *
 *   Copyright (C) 2005 - 2013 Christian Schoenebeck                       *
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

// All application global declarations that HAVE to be exposed to the C++
// API are defined here.

#ifndef __LS_GLOBAL_H__
#define __LS_GLOBAL_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include <string>

typedef std::string String;

#if defined(WIN32)
#include <windows.h>

// modern MinGW has usleep
#if (__MINGW32_MAJOR_VERSION < 3 ||                                     \
     (__MINGW32_MAJOR_VERSION == 3 && __MINGW32_MINOR_VERSION < 15)) && \
    !defined(__MINGW64) && !defined(__MINGW64_VERSION_MAJOR)
#define usleep(a) Sleep(a/1000)
#endif

#define sleep(a) Sleep(a*1000)
typedef unsigned int uint;
// FIXME: define proper functions which do proper alignement under Win32
#define alignedMalloc(a,b) malloc(b)
#define alignedFree(a) free(a)
#else
// needed for usleep under POSIX
#include <stdio.h>
// for uint
#include <sys/types.h>
#endif

#ifdef __GNUC__
#define DEPRECATED_API __attribute__ ((deprecated))
#else
#define DEPRECATED_API
#endif

// whether compiler is C++11 standard compliant
#if defined(__cplusplus) && __cplusplus >= 201103L
# define IS_CPP11 1
#endif

// C++ "override" keyword introduced with C++11 standard
#if IS_CPP11
# define OVERRIDE override
#else
# define OVERRIDE
#endif

#endif // __LS_GLOBAL_H__

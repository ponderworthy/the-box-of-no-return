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

// All application global declarations, that are not going to be exposed to
// the C++ API are defined here.

#ifndef __LS_GLOBAL_PRIVATE_H__
#define __LS_GLOBAL_PRIVATE_H__

#include "global.h"
#include "Exception.h"
#include <sstream>
#include <algorithm>
#include <math.h>

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if CONFIG_DEBUG_LEVEL > 0
#  define dmsg(debuglevel,x)	if (CONFIG_DEBUG_LEVEL >= debuglevel) {printf x; fflush(stdout);}
#else
#  define dmsg(debuglevel,x)
#endif // CONFIG_DEBUG_LEVEL > 0

#define EMMS __asm__ __volatile__ ("emms" ::: "st", "st(1)", "st(2)", "st(3)", "st(4)", "st(5)", "st(6)", "st(7)", "mm0", "mm1", "mm2", "mm3", "mm4", "mm5", "mm6", "mm7")

/// defines globally the bit depth of used samples
typedef int16_t sample_t;

// macro to check for a certain version (or newer) of GCC
#define GNUC_VERSION_PREREQ(major,minor) (__GNUC__ > (major) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))

#if ( GNUC_VERSION_PREREQ(3,3) && ( defined(__i386__) || defined(__x86_64__) || defined(_ARCH_PPC) ) )
#define HAVE_GCC_VECTOR_EXTENSIONS 1
#endif

#if HAVE_GCC_VECTOR_EXTENSIONS
// v4sf is used by some routines that make use of GCC vector extensions (ie AudioChannel.cpp)
typedef float v4sf __attribute__ ((vector_size(16)));
#endif

// circumvents a bug in GCC 4.x which causes a sizeof() expression applied
// on a class member to throw a compiler error, i.e. with GCC 4.4:
// "object missing in reference to 'LinuxSampler::AbstractEngineChannel::ControllerTable'")
// or with GCC 4.0:
// "invalid use of non-static data member 'LinuxSampler::AbstractEngineChannel::ControllerTable'"
#define _MEMBER_SIZEOF(T_Class, Member) sizeof(((T_Class*)NULL)->Member)

/**
 * Whether a function / method call was successful, or if warnings or even an
 * error occured.
 */
enum result_type_t {
    result_type_success,
    result_type_warning,
    result_type_error
};

/**
 * Used whenever a detailed description of the result of a function / method
 * call is needed.
 */
struct result_t {
    result_type_t type;     ///< success, warning or error
    int           code;     ///< warning or error code
    String        message;  ///< detailed warning or error message
};

template<class T> inline String ToString(T o) {
	std::stringstream ss;
	ss << o;
	return ss.str();
}

inline int ToInt(const std::string& s) throw(LinuxSampler::Exception) {
    int i;
    std::istringstream iss(s);
    if(!(iss >> i)) throw LinuxSampler::Exception("Not an integer");
    return i;
}

inline float ToFloat(const std::string& s) throw(LinuxSampler::Exception) {
    float i;
    std::istringstream iss(s);
    if(!(iss >> i)) throw LinuxSampler::Exception("Not a floating-point number");
    return i;
}

inline std::string ltrim(std::string s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}

inline std::string rtrim(std::string s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}

inline std::string trim(std::string s) {
    return ltrim(rtrim(s));
}

class Runnable {
    public:
        virtual ~Runnable() { }
        virtual void Run() = 0;
};

extern double GLOBAL_VOLUME;
extern int GLOBAL_MAX_VOICES;
extern int GLOBAL_MAX_STREAMS;

//TODO: (hopefully) just a temporary nasty hack for launching gigedit on the main thread on Mac (see comments in gigedit.cpp for details)
#if defined(__APPLE__)
extern bool g_mainThreadCallbackSupported;
extern void (*g_mainThreadCallback)(void* info);
extern void* g_mainThreadCallbackInfo;
extern bool g_fireMainThreadCallback;
#endif

// I read with some Linux kernel versions (between 2.4.18 and 2.4.21)
// sscanf() might be buggy regarding parsing of hex characters, so ...
int hexToNumber(char hex_digit);
int hexsToNumber(char hex_digit0, char hex_digit1 = '0');

#endif // __LS_GLOBAL_PRIVATE_H__

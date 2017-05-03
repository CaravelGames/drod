// $Id$

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Deadly Rooms of Death.
 *
 * The Initial Developer of the Original Code is
 * Caravel Software.
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//Type declarations.

#ifndef TYPES_H
#define TYPES_H

#include "PortsBase.h"

#ifdef USE_CXX11
#define STATIC_ASSERT(x) static_assert(x, #x)
#else
#define STATIC_ASSERT(x) typedef char _static_assert[(x)?1:-1]  // for compile-time checks
#endif

//Make sure I have the basics, like NULL.  I don't know how portable this 
//include is.
#  include <ctime>
#  include <climits>
#  include <cstdio>
#  include <cstdlib>

#if defined(_MSC_VER) && _MSC_VER < 1600
typedef signed __int8  int8_t;
typedef signed __int16 int16_t;
typedef signed __int32 int32_t;
typedef signed __int64 int64_t;
typedef unsigned __int8  uint8_t;
typedef unsigned __int16 uint16_t;
typedef unsigned __int32 uint32_t;
typedef unsigned __int64 uint64_t;
#else
#define HAVE_STDINT_H 1
#include <stdint.h>
#endif

#ifndef _WINDOWS_ //If <windows.h> wasn't included.
typedef uint8_t BYTE;
typedef uint16_t USHORT;
typedef unsigned int UINT;  // (currently, must match Wchar.h)
typedef unsigned long ULONG;
#endif //...#ifndef _WINDOWS_

typedef unsigned long long ULONGLONG;

typedef uint64_t QWORD;

#ifndef __APPLE__
	//good random number picking (rely more on high order bits)
	#define RAND(a)   (UINT)(((ULONGLONG)rand() * (a)) / ((ULONGLONG)RAND_MAX+1))
	//random float value in the range [0,a]
	#define fRAND(a) ((a) * (rand() / (float)RAND_MAX))
	//uniform random value in the range [-a,+a]
	#define fRAND_MID(a) ((2*(a) * (rand() / (float)RAND_MAX)) - (a))
#else  // Mac's rand() is spectacularly bad.
	// good random number picking (rely more on a better generator)
	#define RAND(a) ((a) ? random() % (a) : 0)
	// random float value in the range [0,a]
	#define fRAND(a) (1.0 * random() / (1ULL<<31) * (a))
	// uniform random value in the range [-a,+a]
	#define fRAND_MID(a) (fRAND(2.0*(a)) - (a))
#endif

STATIC_ASSERT(sizeof(BYTE) == 1);
STATIC_ASSERT(sizeof(USHORT) == 2);
STATIC_ASSERT(sizeof(UINT) == 4);
STATIC_ASSERT(sizeof(QWORD) == 8);
STATIC_ASSERT(sizeof(ULONG) >= 4);
STATIC_ASSERT(sizeof(ULONGLONG) >= 8);

#ifdef WIN32
static inline long long atoll (const char* str) { return _atoi64(str); }
#endif

// valid inputs: "-2147483648".."4294967295"
static inline UINT convertToUINT (const char* str) { return (UINT)atoll(str); }
static inline int  convertToInt  (const char* str) { return (int)atoll(str); }

static inline time_t convertToTimeT (const char* str) {
	return (sizeof(time_t) == 4) ? (time_t)atoi(str) : (time_t)atoll(str);
}

static inline bool convertIntStrToBool (const char* str) { return atoi(str) != 0; }

// write number to buf, return buf
char* writeInt32 (char* buf, size_t bufsize, int32_t i);
char* writeTimeT (char* buf, size_t bufsize, time_t i);
static inline char* writeInt32 (char* buf, size_t bufsize, uint32_t i) { return writeInt32(buf, bufsize, (int32_t)i); }

#ifdef HAVE_ATTR_DEPRECATED
// let's not use atol
extern "C" long atol (const char*) ATTR_DEPRECATED("use convertTo* from BackEndLib/Types.h");
#endif

#endif //...#ifndef TYPES_H

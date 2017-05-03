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

//Type declarations for common Windows types.  I'm just putting in the ones
//that are being used.

#ifndef PORTS_H
#define PORTS_H

#include "PortsBase.h" //must be first include

#if defined(WIN32) && !defined(__GNU__)
//VS doesn't have std::min/max
#ifndef min
#	define min(a,b) (((a) < (b)) ? (a) : (b))
#endif
#ifndef max
#	define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
//template<typename T> static inline T& min (T &a, T &b) { return a < b ? a : b; }
//template<typename T> static inline T& max (T &a, T &b) { return a > b ? a : b; }
#else
#include <algorithm>
using std::min;
using std::max;
using std::abs;
#endif

#ifndef WIN32

#include "Types.h"
#include "Wchar.h"

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
#  include <ctype.h>
// strcasecmp is a BSD 4.4 function, and some systems might actually have
// stricmp. Should check for both and use a custom version if none exist
#  define stricmp strcasecmp
#  define _stricmp strcasecmp
#  define strnicmp strncasecmp
#  define _strnicmp strncasecmp
#  define _snprintf snprintf
#endif

WCHAR* _itow(int value, WCHAR* str, int radix);
char* _itoa(int value, char* pBuffer, int radix);

#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
void LittleToBig(char *pBuffer, int bytesPer, int count);
void LittleToBig(USHORT *pBuffer, int count = 1);
void LittleToBig(UINT *pBuffer, int count = 1);
void LittleToBig(int *pBuffer, int count = 1);
void LittleToBig(WCHAR *pBuffer, int count = 1);
#endif // big endian

#  ifdef __APPLE__  //don't use the macros defined in <_wctype.h>
#    undef iswlower
#    undef iswupper
#    undef iswspace
#    undef iswdigit
#    undef iswxdigit
#    undef iswalpha
#    undef iswalnum
#  endif

#ifdef USE_CXX11
#include <cwctype>
#else
WCHAR towlower(const WCHAR ch);
bool iswlower(const WCHAR ch);
bool iswupper(const WCHAR ch);
bool iswspace(const WCHAR ch);
bool iswdigit(const WCHAR ch);
bool iswxdigit(const WCHAR ch);
bool iswalpha(const WCHAR ch);
bool iswalnum(const WCHAR ch);
#endif

#endif //...#ifndef WIN32

#endif //...#ifndef PORTS_H

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
 * Portions created by the Initial Developer are Copyright (C) 2006
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef PORTSBASE_H
#define PORTSBASE_H

#if __cplusplus >= 201103L  // c++11
#  define USE_CXX11 1
#endif

#if (__GNUC__ >= 3) || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
// A pure function has no side effects (ie only effect is the return value)
// and only relies on its arguments and/or global variables.
#	define FUNCATTR_PURE __attribute__((__pure__))
// For unused arguments, etc
#	define UNUSED __attribute__((__unused__))
#else
#	define FUNCATTR_PURE
#	define UNUSED
#endif

#if defined(__clang__)
#  define HAVE_ATTR_DEPRECATED 1
#  if __has_extension(attribute_deprecated_with_message)
#    define ATTR_DEPRECATED(x) __attribute__((__deprecated__(x)))
#  else
#    define ATTR_DEPRECATED(x) __attribute__((__deprecated__))
#  endif
#elif defined(__GNUC__)
#  define HAVE_ATTR_DEPRECATED 1
#  if __GNUC__ * 100 + __GNUC_MINOR__ >= 405
#    define ATTR_DEPRECATED(x) __attribute__((__deprecated__(x)))
#  else
#    define ATTR_DEPRECATED(x) __attribute__((__deprecated__))
#  endif
#endif

// Find the byte-order of the host
#define GAME_BYTEORDER_LITTLE (1)
#define GAME_BYTEORDER_BIG    (2)
// Autodetect
#if (defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && defined(BIG_ENDIAN))
#  if (BYTE_ORDER == LITTLE_ENDIAN)
#     define GAME_BYTEORDER GAME_BYTEORDER_LITTLE
#  elif (BYTE_ORDER == BIG_ENDIAN)
#     define GAME_BYTEORDER GAME_BYTEORDER_BIG
#  endif
#elif (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && defined(__BIG_ENDIAN))
#  if (__BYTE_ORDER == __LITTLE_ENDIAN)
#     define GAME_BYTEORDER GAME_BYTEORDER_LITTLE
#  elif (__BYTE_ORDER == __BIG_ENDIAN)
#     define GAME_BYTEORDER GAME_BYTEORDER_BIG
#  endif
#endif
// If that failed, try to recognize the system
#ifndef GAME_BYTEORDER
#  if (defined(WIN32) || defined(__i386__) || defined(__x86_64__))
#     define GAME_BYTEORDER GAME_BYTEORDER_LITTLE
#  elif (defined(__APPLE__) || defined(__sgi))
#     define GAME_BYTEORDER GAME_BYTEORDER_BIG
#  else
#     error Unknown byte order. Please add your system above.
#  endif
#endif

#endif //...#ifndef PORTSBASE_H

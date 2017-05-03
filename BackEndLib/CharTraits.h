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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef CHARTRAITS_H
#define CHARTRAITS_H

#include <ext/pod_char_traits.h>  // From libstdc++ CVS -- put in ../ext

using namespace std;
using namespace __gnu_cxx;

// Some useful extras
template<typename V, typename I, typename S, typename U>
static inline bool operator==(const character<V,I,S>& lhs, U rhs)
	{ return lhs.value == rhs; }

template<typename V, typename I, typename S>
static inline bool operator!=(const character<V,I,S>& lhs, const character<V,I,S>& rhs)
	{ return lhs.value != rhs.value; }

template<typename V, typename I, typename S, typename U>
static inline bool operator!=(const character<V,I,S>& lhs, U rhs)
	{ return lhs.value != rhs; }

template<typename V, typename I, typename S, typename U>
static inline bool operator>=(const character<V,I,S>& lhs, U rhs)
	{ return lhs.value >= rhs; }

template<typename V, typename I, typename S, typename U>
static inline bool operator<=(const character<V,I,S>& lhs, U rhs)
	{ return lhs.value <= rhs; }

template<typename V, typename I, typename S, typename U>
static inline bool operator>(const character<V,I,S> lhs, U rhs)
	{ return lhs.value > rhs; }

template<typename V, typename I, typename S, typename U>
static inline bool operator<(const character<V,I,S> lhs, U rhs)
	{ return lhs.value < rhs; }

template<typename V, typename I, typename S>
static inline bool operator&&(const character<V,I,S>& lhs, bool rhs)
	{ return lhs.value && rhs; }

template<typename V, typename I, typename S>
static inline bool operator||(const character<V,I,S>& lhs, bool rhs)
	{ return lhs.value || rhs; }

template<typename V, typename I, typename S>
static inline bool operator!(const character<V,I,S>& lhs)
	{ return !lhs.value; }

typedef unsigned short          WCHAR_t;
typedef character<WCHAR_t, int> WCHAR; //wc, 16-bit UNICODE character

static inline WCHAR toWCHAR(wchar_t code) { return std::char_traits<WCHAR>::to_char_type(code); }

#define WCv(x)  ((x).value)
#define pWCv(x) ((x)->value)
#define W_t(x)  ((WCHAR){(WCHAR_t)(x)})
#if defined(__linux__) || defined(__APPLE__) || defined(__FreeBSD__)
#if defined(__GNUC__) && __GNUC__ >= 4
#define We(x)   {x}
#else
#define We(x)   W_t(x)
#endif
#else
#error Need to define We(x) for this platform
#endif

#endif //CHARTRAITS_H

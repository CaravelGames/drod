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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996,
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Matt Schikore (Schik)
 *
 * ***** END LICENSE BLOCK ***** */

//Common utility functions.

#ifndef UTILFUNCS_H
#define UTILFUNCS_H

//*****************************************************************************************
template<class T, class I> T SafePostDec(T& obj, I beginIt)
// A shortcut function to decrement an iterator, but only if it's not already at begin().
{
   T ret = obj;

   if (obj != beginIt)
		--obj;

   return ret;
}

//*****************************************************************************************
template<class T, class I> T SafePostInc(T& obj, I endIt)
// A shortcut function to increment an iterator, but only if it's not already at end().
{
   T ret = obj;

   if (obj != endIt)
		++obj;

   return ret;
}

//*****************************************************************************************
template<class T, class I> T SafePreDec(T& obj, I beginIt)
// A shortcut function to decrement an iterator, but only if it's not already at begin().
{
   if (obj != beginIt)
		--obj;

   return obj;
}

//*****************************************************************************************
template<class T, class I> T SafePreInc(T& obj, I endIt)
// A shortcut function to increment an iterator, but only if it's not already at end().
{
   if (obj != endIt)
		++obj;

   return obj;
}

//Returns sign of a number as -1, 0, or 1.
static inline int sgn(const int x)
{
	if (x<0)
		return -1;
	return !x ? 0 : 1;
}

#endif //...#ifndef UTILFUNCS_H

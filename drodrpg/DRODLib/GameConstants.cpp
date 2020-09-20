// $Id: GameConstants.cpp 9637 2011-07-18 19:28:55Z mrimer $

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
 * 1997, 2000, 2001, 2002, 2005, 2011, 2016 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "GameConstants.h"

#ifdef DROD_AUTODETECT_REVISION
#include "drod-revision.h"
#endif

//Current game version.  Update as needed.
const char szDROD[] = "drodrpg";
const WCHAR wszDROD[] = { We('d'),We('r'),We('o'),We('d'),We('r'),We('p'),We('g'),We(0) };
const WCHAR wszDROD_VER[] = { We('1'),We('_'),We('0'),We(0) };

const UINT VERSION_NUMBER = 406; //data format version -- increment when exported data format changes
const UINT NEXT_VERSION_NUMBER = 500;

#ifdef DROD_VERSION_REVISION
// (the WS macro requires c++11 or windows)
const WCHAR wszVersionReleaseNumber[] = WS("1.2.7.") WS(STRFY_EXPAND(DROD_VERSION_REVISION));
#else
const WCHAR wszVersionReleaseNumber[] = {
	We('1'),We('.'),We('2'),We('.'),We('7'),We('.'),We('3'),We('1'),We('6'),We(0)   // 1.2.7.* -- full version number plus build number
};
#endif

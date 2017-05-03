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
 * Gerry Jo Jellestad (trick)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef PNGHANDLER_H
#define PNGHANDLER_H

#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>

struct SDL_Surface;
struct SDL_Rect;
class CPNGHandler
{
public:
	CPNGHandler() {}

	static SDL_Surface* CreateSurface(const void* buffer, const UINT length);
	static SDL_Surface* CreateSurface(const WCHAR* wszFile);

	static bool Write(const WCHAR* wszFile, SDL_Surface* pSurface, SDL_Rect* pRect=NULL);
};

#endif // #ifndef PNGHANDLER_H

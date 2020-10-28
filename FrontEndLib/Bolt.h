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

#ifndef BOLT_H
#define BOLT_H

#include "BitmapManager.h"
#include <vector>


struct BoltSegment {
	BoltSegment(SDL_Rect* pSourceRect, SDL_Rect destRect)
		: pSourceRect(pSourceRect), destRect(destRect)
	{}

	SDL_Rect* pSourceRect; // Not owned by this struct
	SDL_Rect destRect;
};

typedef std::vector<BoltSegment> BOLT_SEGMENTS;

void GenerateBolt(int xBegin, int yBegin, int xEnd, int yEnd, const UINT DISPLAY_SIZE, BOLT_SEGMENTS& drawSegments, vector<SDL_Rect>& dirtyRects);
void DrawBolt(BOLT_SEGMENTS& segments, SDL_Surface& pPartsSurface, SDL_Surface& pDestSurface);

void DrawBolt(int xBegin, int yBegin, int xEnd, int yEnd, const UINT DISPLAY_SIZE, SDL_Surface* pPartsSurface, SDL_Surface* pDestSurface, vector<SDL_Rect>& dirtyRects);

#endif //...#ifndef BOLT_H

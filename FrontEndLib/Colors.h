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

#ifndef COLORS_H
#define COLORS_H

#include <SDL.h>

//Apple doesn't need any special pixel surface handling with SDL2

#define PIXEL_FUDGE_FACTOR (0)

struct SURFACECOLOR 
{
	Uint8 byt1;
	Uint8 byt2; 
	Uint8 byt3;
};

SURFACECOLOR   GetSurfaceColor(const SDL_Surface *pSurface, Uint8 bytRed, 
		Uint8 bytGreen, Uint8 bytBlue);

static inline void EnableSurfaceBlending(SDL_Surface *surface, Uint8 opacity = 255)
{
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_BLEND);
	SDL_SetSurfaceAlphaMod(surface, opacity);
}
static inline void DisableSurfaceBlending(SDL_Surface *surface)
{
	SDL_SetSurfaceBlendMode(surface, SDL_BLENDMODE_NONE);
}
void AutodetectSurfaceBlending(SDL_Surface *surface, Uint8 opacity = 255);

void SetColorKey(SDL_Surface *surface, int flag, Uint32 key);

#define MAKE_SDL_RECT(x,y,w,h) { (int)(x), (int)(y), (int)(w), (int)(h) }

#endif //#ifndef COLORS_H

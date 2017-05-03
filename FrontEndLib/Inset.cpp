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

#include "Inset.h"
#include "Colors.h" // for MAKE_SDL_RECT
#include "Screen.h" // for PresentRect
#include <BackEndLib/Assert.h>
#include <SDL.h>

#define DISABLED_COLOR 190,181,165 //medium brown

//********************************************************************************
void DrawInset(
//Draw inset area to a surface.
//
//Params:
	const int nX, const int nY,            //(in)   Dest coords.
	const UINT wW, const UINT wH,       //(in)   Dimension of area to draw.
	SDL_Surface *pPartsSurface,   //(in)   Contains parts need to draw inset.
	SDL_Surface *pDestSurface,    //(in)   Surface to draw to.
	const bool bDrawCenter,       //(in) Set to false if center of bevel is to
											//     not be drawn [default=true].
	const bool bUpdateRect,       //(in) Set to true if caller won't be updating
											//     the rect [default = false].
	const bool bDisabled)         //(in) Gray out the center [default=false]
{
	ASSERT(pPartsSurface);
	ASSERT(pDestSurface);

	//Source coords and dimensions within parts surface.
	const int X_LEFT_BEVEL = 302;
	const int Y_TOP_BEVEL = 0;
	const UINT CX_CENTER = 26;
	const UINT CY_CENTER = 26;
	const int X_RIGHT_BEVEL = 330;
	const int Y_BOTTOM_BEVEL = 28;
	const int X_CENTER = 304;
	const int Y_CENTER = 2;

	//Dest coords and dimensions.
	const int xLeftBevel = nX;
	const int xRightBevel = nX + wW - CX_INSET_BORDER;
	const int yTopBevel = nY;
	const int yBottomBevel = nY + wH - CY_INSET_BORDER;
	const int xCenter = xLeftBevel + CX_INSET_BORDER;
	const int yCenter = yTopBevel + CY_INSET_BORDER;

	//Draw top-left corner.
	SDL_Rect src = MAKE_SDL_RECT(X_LEFT_BEVEL, Y_TOP_BEVEL, CX_INSET_BORDER, CY_INSET_BORDER);
	SDL_Rect dest = MAKE_SDL_RECT(xLeftBevel, yTopBevel, CX_INSET_BORDER, CY_INSET_BORDER);
	SDL_BlitSurface(pPartsSurface, &src, pDestSurface, &dest);

	//Draw top-right corner.
	src.x = X_RIGHT_BEVEL;
	dest.x = xRightBevel;
	SDL_BlitSurface(pPartsSurface, &src, pDestSurface, &dest);

	//Draw bottom-right corner.
	src.y = Y_BOTTOM_BEVEL;
	dest.y = yBottomBevel;
	SDL_BlitSurface(pPartsSurface, &src, pDestSurface, &dest);

	//Draw bottom-left corner.
	src.x = X_LEFT_BEVEL;
	dest.x = xLeftBevel;
	SDL_BlitSurface(pPartsSurface, &src, pDestSurface, &dest);

	//Draw bottom bevel.
	src.x = X_LEFT_BEVEL + CX_INSET_BORDER;
	dest.w = src.w = CX_CENTER;
	for (dest.x = xLeftBevel + CX_INSET_BORDER; dest.x < xRightBevel; dest.x += CX_CENTER)
	{
		if (dest.x + CX_CENTER > (UINT)xRightBevel)
			dest.w = src.w = xRightBevel - dest.x; //Clip the bevel to remaining width.
		SDL_BlitSurface(pPartsSurface, &src, pDestSurface, &dest);
	}

	//Draw top bevel.
	dest.y = yTopBevel;
	src.y = Y_TOP_BEVEL;
	dest.w = src.w = CX_CENTER;
	for (dest.x = xLeftBevel + CX_INSET_BORDER; dest.x < xRightBevel; dest.x += CX_CENTER)
	{
		if (dest.x + CX_CENTER > (UINT)xRightBevel)
			dest.w = src.w = xRightBevel - dest.x; //Clip the bevel to remaining width.
		SDL_BlitSurface(pPartsSurface, &src, pDestSurface, &dest);
	}

	//Draw left bevel.
	dest.x = xLeftBevel;
	src.x = X_LEFT_BEVEL;
	src.y = Y_TOP_BEVEL + CY_INSET_BORDER;
	src.w = dest.w = CX_INSET_BORDER;
	src.h = dest.h = CY_CENTER;
	for (dest.y = yTopBevel + CY_INSET_BORDER; dest.y < yBottomBevel; dest.y += CY_CENTER)
	{
		if (dest.y + CY_CENTER > (UINT)yBottomBevel)
			dest.h = src.h = yBottomBevel - dest.y; //Clip the bevel to remaining height.
		SDL_BlitSurface(pPartsSurface, &src, pDestSurface, &dest);
	}

	//Draw right bevel.
	dest.x = xRightBevel;
	src.x = X_RIGHT_BEVEL;
	src.h = dest.h = CY_CENTER;
	for (dest.y = yTopBevel + CY_INSET_BORDER; dest.y < yBottomBevel; dest.y += CY_CENTER)
	{
		if (dest.y + CY_CENTER > (UINT) yBottomBevel)
			dest.h = src.h = yBottomBevel - dest.y; //Clip the bevel to remaining height.
		SDL_BlitSurface(pPartsSurface, &src, pDestSurface, &dest);
	}

	//Draw center.
	if (!bDrawCenter) return;
	src.x = X_CENTER;
	src.y = Y_CENTER;
	src.h = dest.h = CY_CENTER;
	if (bDisabled)
	{
		//Gray out center area.
		SDL_Rect center = MAKE_SDL_RECT(xCenter, yCenter, xRightBevel - xCenter, yBottomBevel - yCenter);
		const SURFACECOLOR dc = GetSurfaceColor(pDestSurface, DISABLED_COLOR);
		const Uint32 color = SDL_MapRGB(pDestSurface->format, dc.byt1, dc.byt2, dc.byt3);
		SDL_FillRect(pDestSurface, &center, color);
	}
	else
	{
		for (dest.y = yCenter; dest.y < yBottomBevel; dest.y += CY_CENTER)
		{
			src.w = dest.w = CX_CENTER;
			for (dest.x = xCenter; dest.x < xRightBevel; dest.x += CX_CENTER)
			{
				if (dest.x + CX_CENTER > (UINT)xRightBevel)
					dest.w = src.w = xRightBevel - dest.x; //Clip the blit to remaining width.
				if (dest.y + CY_CENTER > (UINT)yBottomBevel)
					dest.h = src.h = yBottomBevel - dest.y; //Clip the blit to remaining height.
				SDL_BlitSurface(pPartsSurface, &src, pDestSurface, &dest);
			}
		}
	}

	if (bUpdateRect)
		PresentRect(pDestSurface, xLeftBevel, yTopBevel, wW, wH);
}

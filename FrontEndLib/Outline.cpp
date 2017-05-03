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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005, 2006
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include <SDL.h>

#include "Colors.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/CoordIndex.h>

void AddOutline_3BPP(SDL_Surface *pSurface, const SURFACECOLOR& OutlineColor, 
		const SURFACECOLOR& BackgroundColor, const UINT wWidth);
void AddOutline_1BPP(SDL_Surface *pSurface, const Uint8 OutlineColor, 
		const Uint8 BackgroundColor, const UINT wWidth);

//************************************************************************************
void AddOutline(
//Call either 1-bpp or 3-bpp routine to add outline to pixels on a surface.  Background
//color must be set by color key on the surface.
//
//Params:
	SDL_Surface *pSurface,        //(in)   Surface to add outline to.
	SDL_Color OutlineColor,       //(in)   Color of outline pixels.
	UINT wWidth)               //(in)   Width in pixels to make the outline.  Default is 1.
{
	//Lock surface.
	if (SDL_MUSTLOCK(pSurface)) 
		while (SDL_LockSurface(pSurface) < 0)
			SDL_Delay(10);

	//Get surface colors and make the call.
	switch (pSurface->format->BytesPerPixel)
	{
		case 1:
		{
			Uint32 BackgroundSurfaceColor;
			SDL_GetColorKey(pSurface, &BackgroundSurfaceColor);
			Uint32 OutlineSurfaceColor = SDL_MapRGB(pSurface->format, OutlineColor.r,
					OutlineColor.g, OutlineColor.b);

			//set outline color as second color in surface palette
			SDL_Color colors[1] = {OutlineColor};
			SDL_SetPaletteColors(pSurface->format->palette, colors, 2, 1);
			OutlineSurfaceColor = 2;

			AddOutline_1BPP(pSurface, (Uint8)OutlineSurfaceColor, (Uint8)BackgroundSurfaceColor, wWidth);
		}
		break;

		case 3:
		{
			SURFACECOLOR OutlineSurfaceColor = GetSurfaceColor(pSurface, OutlineColor.r, 
					OutlineColor.g, OutlineColor.b);       
			Uint8 r, g, b;
			Uint32 colorkey;
			SDL_GetColorKey(pSurface, &colorkey);
			SDL_GetRGB(colorkey, pSurface->format, &r, &g, &b);
			SURFACECOLOR BackgroundSurfaceColor = GetSurfaceColor(pSurface, r, g, b);
			AddOutline_3BPP(pSurface, OutlineSurfaceColor, BackgroundSurfaceColor, wWidth);
		}
		break;

		default:
			ASSERT(!"No implementation for this BPP.");
		break;
	}

	//Unlock surface.
	if (SDL_MUSTLOCK(pSurface)) SDL_UnlockSurface(pSurface);
}

#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
#define ISCOLOR(pPixel,SurfaceColor) \
	((*reinterpret_cast<Uint32*>(pPixel) & pSurface->format->Rmask) >> pSurface->format->Rshift == (SurfaceColor).byt1 && \
	(*reinterpret_cast<Uint32*>(pPixel) & pSurface->format->Gmask) >> pSurface->format->Rshift == (SurfaceColor).byt2 && \
	(*reinterpret_cast<Uint32*>(pPixel) & pSurface->format->Bmask) >> pSurface->format->Rshift == (SurfaceColor).byt3)
#else
#define ISCOLOR(pPixel,SurfaceColor) \
	((pPixel)[0] == (SurfaceColor).byt1 && \
	(pPixel)[1] == (SurfaceColor).byt2 && \
	(pPixel)[2] == (SurfaceColor).byt3)
#endif

//************************************************************************************
void AddOutline_3BPP(
//Adds an outline around the perimeter of non-background pixels in surface.
//
//Params:
	SDL_Surface *pSurface,        //(in)   Surface to add outline to.
	const SURFACECOLOR& OutlineColor,    //(in)   Color of outline pixels.
	const SURFACECOLOR& BackgroundColor, //(in)   Indicates which pixels are the background.
	const UINT wWidth)            //(in)   Width in pixels to make the outline.
{
	//Index will hold all the outline plots to make.
	CCoordIndex OutlinePlots(pSurface->w, pSurface->h);
	ASSERT(OutlinePlots.GetArea() == (UINT)pSurface->w * (UINT)pSurface->h);

	const BYTE *pbytIndex = OutlinePlots.GetIndex();
	const BYTE *pbytStop = pbytIndex + (pSurface->w * pSurface->h);
	const BYTE *pbytSeek;
	UINT x, y;
	Uint8 *pPixel;
	UINT wBPP = pSurface->format->BytesPerPixel;
	ASSERT(wBPP >= 3);
	UINT wRowOffset = pSurface->pitch - (pSurface->w * wBPP);

	//Each iteration draws one pixel-width outline.
	for (UINT wWidthI = 0; wWidthI < wWidth; ++wWidthI)
	{
		//Each iteration finds outline pixels for one row.
		pPixel = static_cast<Uint8 *>(pSurface->pixels);
		for (y = 0; y < static_cast<UINT>(pSurface->h); ++y)
		{
			//Each iteration checks a pixel to see if outline pixels go around it.
			for (x = 0; x < static_cast<UINT>(pSurface->w); ++x)
			{
				if (!ISCOLOR(pPixel, BackgroundColor)) //Found a non-background pixel.
				{
					//Add outline pixels to plot index.
					if (x == 0)
						OutlinePlots.Add(x, y); //if text abuts edge, draw outline over it
					else if (ISCOLOR(pPixel - wBPP, BackgroundColor))
						OutlinePlots.Add(x - 1, y);
					if (x == static_cast<UINT>(pSurface->w) - 1)
						OutlinePlots.Add(x, y); //if text abuts edge, draw outline over it
					else if (ISCOLOR(pPixel + wBPP, BackgroundColor))
						OutlinePlots.Add(x + 1, y);
					if (y > 0 &&
							ISCOLOR(pPixel - pSurface->pitch, BackgroundColor))
						OutlinePlots.Add(x, y - 1);
					if (y < static_cast<UINT>(pSurface->h) - 1 &&
							ISCOLOR(pPixel + pSurface->pitch, BackgroundColor))
						OutlinePlots.Add(x, y + 1);
				}
				pPixel += wBPP;
			}
			pPixel += wRowOffset;
		}

		//Plot all the outline pixels.
		pbytSeek = pbytIndex;
		while (pbytSeek != pbytStop)
		{
			if (*pbytSeek)
			{
				y = (pbytSeek - pbytIndex) / pSurface->w;
				x = (pbytSeek - pbytIndex) - (y * pSurface->w);
				pPixel = static_cast<Uint8 *>(pSurface->pixels) + 
						(y * pSurface->pitch) + (x * wBPP);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
				ASSERT(!"needs to be tested.");
				Uint32 *pPixel32 = reinterpret_cast<Uint32*>(pPixel);
				*pPixel32 = SDL_MapRGB(pSurface->format, OutlineColor.byt1, OutlineColor.byt2, OutlineColor.byt3);
#else
				pPixel[0] = OutlineColor.byt1;
				pPixel[1] = OutlineColor.byt2;
				pPixel[2] = OutlineColor.byt3;
#endif
			}
			++pbytSeek;
		}

		OutlinePlots.Clear();
	}
}

//************************************************************************************
void AddOutline_1BPP(
//1-bpp version of above routine.
//
//Params:
	SDL_Surface *pSurface,        //(in)   Surface to add outline to.
	const Uint8 OutlineColor,     //(in)   Color of outline pixels.
	const Uint8 BackgroundColor,  //(in)   Indicates which pixels are the background.
	const UINT wWidth)            //(in)   Width in pixels to make the outline.
{
	ASSERT(OutlineColor != BackgroundColor);

	//Index will hold all the outline plots to make.
	CCoordIndex OutlinePlots(pSurface->w, pSurface->h);
	ASSERT(OutlinePlots.GetArea() == (UINT)pSurface->w * (UINT)pSurface->h);

	const BYTE *pbytIndex = OutlinePlots.GetIndex();
	const BYTE *pbytStop = pbytIndex + (pSurface->w * pSurface->h);
	const BYTE *pbytSeek;
	UINT x, y;
	Uint8 *pPixel;
	ASSERT(pSurface->format->BytesPerPixel == 1);
	const UINT wRowOffset = pSurface->pitch - pSurface->w;

	//Each iteration draws one pixel-width outline.
	for (UINT wWidthI = 0; wWidthI < wWidth; ++wWidthI)
	{
		//Each iteration finds outline pixels for one row.
		pPixel = static_cast<Uint8 *>(pSurface->pixels);
		for (y = 0; y < static_cast<UINT>(pSurface->h); ++y)
		{
			//Each iteration checks a pixel to see if outline pixels go around it.
			for (x = 0; x < static_cast<UINT>(pSurface->w); ++x)
			{
				if (*pPixel != BackgroundColor) //Found a non-background pixel.
				{
					//Add outline pixels to plot index.
					if (x == 0)
						OutlinePlots.Add(x, y); //if text abuts edge, draw outline over it
					else if (pPixel[-1] == BackgroundColor)
						OutlinePlots.Add(x - 1, y);
					if (x == static_cast<UINT>(pSurface->w) - 1)
						OutlinePlots.Add(x, y); //if text abuts edge, draw outline over it
					else if (pPixel[1] == BackgroundColor)
						OutlinePlots.Add(x + 1, y);
					if (y > 0 && pPixel[-int(pSurface->pitch)] == BackgroundColor)
						OutlinePlots.Add(x, y - 1);
					if (y < static_cast<UINT>(pSurface->h) - 1 &&
							pPixel[pSurface->pitch] == BackgroundColor)
						OutlinePlots.Add(x, y + 1);
				}
				++pPixel;
			}
			pPixel += wRowOffset;
		}

		//Plot all the outline pixels.
		pbytSeek = pbytIndex;
		while (pbytSeek != pbytStop)
		{
			if (*pbytSeek)
			{
				y = (pbytSeek - pbytIndex) / pSurface->w;
				x = (pbytSeek - pbytIndex) - (y * pSurface->w);
				pPixel = static_cast<Uint8 *>(pSurface->pixels) +
						(y * pSurface->pitch) + x;
				*pPixel = OutlineColor;
			}
			++pbytSeek;
		}

		OutlinePlots.Clear();
	}
}

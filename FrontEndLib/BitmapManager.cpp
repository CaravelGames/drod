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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef WIN32 //Many things will not compile w/o WIN32 API.  Fix them if you are porting.
#  include <windows.h> //Should be first include.
#  pragma warning(disable:4786)
#endif

#define INCLUDED_FROM_BITMAPMANAGER_CPP
#include "BitmapManager.h"
#undef INCLUDED_FROM_BITMAPMANAGER_CPP

#include "Screen.h"

#include "JpegHandler.h"
#include "PNGHandler.h"

#include <BackEndLib/Assert.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Ports.h>

#include <SDL.h>
#include <math.h>

using namespace std;

//Holds the only instance of CBitmapManager for the app.
CBitmapManager *g_pTheBM = NULL;

UINT CBitmapManager::CX_TILE = 0;
UINT CBitmapManager::CY_TILE = 0;
UINT CBitmapManager::BITS_PER_PIXEL = 32;  //default - RGBA
UINT CBitmapManager::BYTES_PER_PIXEL = 4;
bool CBitmapManager::bAlpha = true;
BYTE CBitmapManager::eyeCandy = 1;
float CBitmapManager::fLightLevel = 1.0f;
bool CBitmapManager::bGameHasFocus = true;

Uint8 CBitmapManager::TransColor[3] = {192,192,192};	//(r,g,b) color that signifies transparent pixels: default=192-gray

static const WCHAR wcszBitmaps[] = { We(SLASH),We('B'),We('i'),We('t'),We('m'),We('a'),We('p'),We('s'),We(SLASH),We(0) };
static const WCHAR wszTIM[] = { We('.'),We('t'),We('i'),We('m'),We(0) };

const UINT wNumImageFormats = 3;
const WCHAR imageExtension[wNumImageFormats][5] = {
	{ We('.'),We('p'),We('n'),We('g'),We(0) },   //.png
	{ We('.'),We('j'),We('p'),We('g'),We(0) },   //.jpg
	{ We('.'),We('b'),We('m'),We('p'),We(0) }    //.bmp
};

const UINT imageFormat[wNumImageFormats] = {
	DATA_PNG, DATA_JPG, DATA_BMP
};

//Full shadow is 75% of normal.
const float fShadowConvFactor = 1.0f / (256.0f*4.0f); //4*256 = optimization

Uint8 g_shadowCalc[256][256];
Uint8 g_shadowCalcBlend[256][256];

const int g_darkenSteps = 100;
const float g_DarkenStepIncrement = 1.0f / float(g_darkenSteps);
Uint8 g_darkenCalc[g_darkenSteps][256];

//
//Public methods.
//

//**********************************************************************************
CBitmapManager::CBitmapManager()
	: TileImageTypes(NULL)
	, wTileCount(0), wNextCustomTileNo(0)
	, bAlwaysUpdateScreen(false)
//Constructor.
{
	CBitmapManager::BYTES_PER_PIXEL = CBitmapManager::BITS_PER_PIXEL/8;
#ifdef GAME_RENDERING_OFFSET
	ASSERT(CBitmapManager::BYTES_PER_PIXEL == 4);
#endif

	int i, j;
	for (i = 0; i < 256; ++i)
	{
		const float fPercent = 1.0f - ((255 - i) * fShadowConvFactor);
		Uint8 *row = g_shadowCalc[i];
		for (j = 0; j < 256; ++j)
		{
			row[j] = static_cast<Uint8>(j * fPercent);
		}

		const float fFactor = 1.0f - (i * fShadowConvFactor);
		row = g_shadowCalcBlend[i];
		for (j = 0; j < 256; ++j)
		{
			row[j] = static_cast<Uint8>(j * fFactor);
		}
	}

	for (i = 0; i < g_darkenSteps; ++i)
	{
		const float fFactor = g_DarkenStepIncrement * i;
		for (j = 0; j < 256; ++j)
		{
			g_darkenCalc[i][j] = static_cast<Uint8>(j * fFactor);
		}
	}
}

//**********************************************************************************
CBitmapManager::~CBitmapManager()
//Destructor.
{
	//If this assertion fires, there has been a call to GetBitmapSurface() without
	//a matching ReleaseBitmapSurface().
	ASSERT(this->LoadedBitmaps.size()==0);

	for (UINT i=this->pTileSurfaces.size(); i--; )
	{
		ASSERT(this->pTileSurfaces[i]);
		SDL_FreeSurface(this->pTileSurfaces[i]);
	}

	delete[] this->TileImageTypes;
	this->wTileCount = this->wNextCustomTileNo = 0;

	for (CustomTileMap::iterator iter = this->customTiles.begin();
			iter != this->customTiles.end(); ++iter)
	{
		SDL_Surface *pSurface = iter->second.pSurface;
		ASSERT(pSurface);
		SDL_FreeSurface(pSurface);
	}
}

//**********************************************************************************
void CBitmapManager::AddMask(
//Uses the mask surface values*fFactor as additive multipliers of the destination surface pixels.
//
//Params:
	SDL_Surface *pMaskSurface, SDL_Rect src,
	SDL_Surface *pDestSurface, SDL_Rect dest,
	const float fFactor)  //multiplicative factor [default=1.0]
{
	//Do nothing for invalid range values.
	if (fFactor <= 1.0f)
		return;

	//Bounds clipping.
	ClipSrcAndDestToRect(src, dest, pDestSurface->w, pDestSurface->h);
	ASSERT(src.w == dest.w);
	ASSERT(src.h == dest.h);
	if (!src.w || !src.h || !dest.w || !dest.h)
		return; //not in view area -- do nothing

	if (SDL_MUSTLOCK(pDestSurface))
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"AddMask::Lock surface failed.");
			return;
		}

	const float fScaledFactor = (fFactor - 1.0f) / 255.0f;

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
	ASSERT(pMaskSurface->format->Rmask == 0xff0000);
	ASSERT(pMaskSurface->format->Gmask == 0x00ff00);
	ASSERT(pMaskSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wMaskPixelByteNo = src.y * pMaskSurface->pitch + (src.x * wMaskBPP);
	const UINT wDestPixelByteNo = dest.y * pDestSurface->pitch + (dest.x * wBPP);
	const UINT dwRowWidth = dest.w * wBPP;
	const UINT dwMaskRowOffset = pMaskSurface->pitch - src.w * wMaskBPP;
	const UINT dwDestRowOffset = pDestSurface->pitch - dwRowWidth;

	Uint8 *pMask = (Uint8 *)pMaskSurface->pixels + wMaskPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *pDest = (Uint8 *)pDestSurface->pixels + wDestPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pDest + (dest.h * pDestSurface->pitch);

	while (pDest != pStop)
	{
		ASSERT(pDest < pStop);
		Uint8 *const pEndOfRow = pDest + dwRowWidth;

		//Each iteration modifies one pixel.
		while (pDest != pEndOfRow)
		{
			//ASSUME for speed: values don't exceed limit
			pDest[0] = static_cast<Uint8>(pDest[0] * (1.0f + fScaledFactor * (float)pMask[0]));
			pDest[1] = static_cast<Uint8>(pDest[1] * (1.0f + fScaledFactor * (float)pMask[1]));
			pDest[2] = static_cast<Uint8>(pDest[2] * (1.0f + fScaledFactor * (float)pMask[2]));

			pDest += wBPP;
			pMask += wMaskBPP;
		}
		pDest += dwDestRowOffset;
		pMask += dwMaskRowOffset;
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::AddMaskAdditive(
//Increment the values of the destination surface pixels by the mask surface values*fFactor.
//When bUseColorKeyMask is set, non-transparent keyed pixels in the mask indicate
//where to lighten the dest surface.  Otherwise, all mask pixels are added
//to the dest pixels.
//
//Params:
	SDL_Surface *pMaskSurface, SDL_Rect src,
	SDL_Surface *pDestSurface, SDL_Rect dest,
	float fFactor,  //multiplicative factor
	const bool bUseColorKeyMask) //use transparent color-keyed pixels as a mask
	                             //or blend mask pixels with dest pixels
{
	//Do nothing for invalid range values.
	if (fFactor <= 0.0f)
		return;
	if (fFactor > 1.0f)
		fFactor = 1.0f;

	//Bounds clipping.
	ClipSrcAndDestToRect(src, dest, pDestSurface->w, pDestSurface->h);
	ASSERT(src.w == dest.w);
	ASSERT(src.h == dest.h);
	if (!src.w || !src.h || !dest.w || !dest.h)
		return; //not in view area -- do nothing

	if (SDL_MUSTLOCK(pDestSurface))
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"AddMask::Lock surface failed.");
			return;
		}

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
	ASSERT(pMaskSurface->format->Rmask == 0xff0000);
	ASSERT(pMaskSurface->format->Gmask == 0x00ff00);
	ASSERT(pMaskSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wMaskPixelByteNo = src.y * pMaskSurface->pitch + (src.x * wMaskBPP);
	const UINT wDestPixelByteNo = dest.y * pDestSurface->pitch + (dest.x * wBPP);
	const UINT dwRowWidth = dest.w * wBPP;
	const UINT dwMaskRowOffset = pMaskSurface->pitch - src.w * wMaskBPP;
	const UINT dwDestRowOffset = pDestSurface->pitch - dwRowWidth;

	Uint8 *pMask = (Uint8 *)pMaskSurface->pixels + wMaskPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *pDest = (Uint8 *)pDestSurface->pixels + wDestPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pDest + (dest.h * pDestSurface->pitch);

	if (bUseColorKeyMask)
	{
		Uint16 val;
		while (pDest != pStop)
		{
			ASSERT(pDest < pStop);
			Uint8 *const pEndOfRow = pDest + dwRowWidth;

			//Each iteration modifies one pixel.
			while (pDest != pEndOfRow)
			{
				if (pMask[0] != TransColor[0] || pMask[1] != TransColor[1] || pMask[2] != TransColor[2])
				{
					val = static_cast<Uint16>(pDest[0]) + static_cast<Uint16>(fFactor * (float)pMask[0]);
					pDest[0] = (val >= 255 ? 255 : static_cast<Uint8>(val));
					val = static_cast<Uint16>(pDest[1]) + static_cast<Uint16>(fFactor * (float)pMask[1]);
					pDest[1] = (val >= 255 ? 255 : static_cast<Uint8>(val));
					val = static_cast<Uint16>(pDest[2]) + static_cast<Uint16>(fFactor * (float)pMask[2]);
					pDest[2] = (val >= 255 ? 255 : static_cast<Uint8>(val));
				}

				pDest += wBPP;
				pMask += wMaskBPP;
			}
			pDest += dwDestRowOffset;
			pMask += dwMaskRowOffset;
		}
	} else {
		while (pDest != pStop)
		{
			ASSERT(pDest < pStop);
			Uint8 *const pEndOfRow = pDest + dwRowWidth;

			//Each iteration modifies one pixel.
			while (pDest != pEndOfRow)
			{
				//ASSUME for speed: values don't exceed limit
				pDest[0] += static_cast<Uint8>(fFactor * (float)pMask[0]);
				pDest[1] += static_cast<Uint8>(fFactor * (float)pMask[1]);
				pDest[2] += static_cast<Uint8>(fFactor * (float)pMask[2]);

				pDest += wBPP;
				pMask += wMaskBPP;
			}
			pDest += dwDestRowOffset;
			pMask += dwMaskRowOffset;
		}
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::SetSurfaceAlpha(
// Adds alpha to a bitmap, whether it's surface alpha or per-pixel alpha
	SDL_Surface* pSurface,	//(in/out) The surface
	Uint8 alpha)			//(in) the alpha to apply
{
	SDL_PixelFormat *fmt = pSurface->format;

	// If surface has no alpha channel, just set the surface alpha.
	if (fmt->Amask == 0) {
		EnableSurfaceBlending(pSurface, alpha);
	} else {
		// Change the alpha of each pixel.
		const Uint8 bpp = fmt->BytesPerPixel;
		ASSERT(bpp == BYTES_PER_PIXEL);

		// Scaling factor to clamp alpha to [0, alpha].
		const float scale = alpha / 255.0f;

		if (SDL_MUSTLOCK(pSurface))
			SDL_LockSurface(pSurface);

		Uint8 *pPixel = (Uint8 *)pSurface->pixels;
		Uint8 *const pStop = pPixel + (pSurface->h * pSurface->pitch);

		const UINT dwRowOffset = pSurface->pitch - (pSurface->w * bpp);
		while (pPixel != pStop)
		{
			ASSERT(pPixel < pStop);
			Uint8 *const pEndOfRow = pPixel + (pSurface->w * bpp);
			while (pPixel != pEndOfRow)
			{
				// Get a pointer to the current pixel.
				Uint32* pixel_ptr = (Uint32*)pPixel;

				// Get the old pixel components.
				Uint8 r, g, b, a;
				SDL_GetRGBA(*pixel_ptr, fmt, &r, &g, &b, &a);

				// Set the pixel with the new alpha.
				*pixel_ptr = SDL_MapRGBA(fmt, r, g, b, Uint8(scale * a));

				pPixel += bpp;
			}

			pPixel += dwRowOffset;
		}   

		if (SDL_MUSTLOCK(pSurface))
			SDL_UnlockSurface(pSurface);
	}       
}

//**********************************************************************************
void CBitmapManager::BlitRGBAtoRGBA(
//Blits a source surface with alpha to the dest surface with alpha, blending alphas appropriately.
//
//Params:
	SDL_Surface *pSrcSurface, SDL_Rect *srcIn,   //(in) source
	SDL_Surface *pDestSurface, SDL_Rect *destIn) //(in/out) dest
{
	EnableSurfaceBlending(pSrcSurface, 255);
	SDL_BlitSurface(pSrcSurface, srcIn, pDestSurface, destIn);
	DisableSurfaceBlending(pSrcSurface);
}

//**********************************************************************************
void CBitmapManager::BlitAlphaSurfaceWithTransparency(
//Blits a source surface with alpha to the dest surface, using the specified opacity.
//
//Params:
	SDL_Rect src, SDL_Surface *pSrcSurface,   //(in) source
	SDL_Rect dest, SDL_Surface *pDestSurface, //(in/out) dest
	const Uint8 opacity)
{
	EnableSurfaceBlending(pSrcSurface, opacity);
	SDL_BlitSurface(pSrcSurface, &src, pDestSurface, &dest);
	DisableSurfaceBlending(pSrcSurface);
}

//**********************************************************************************
void CBitmapManager::BlitSurface(
	SDL_Surface *pSrcSurface, SDL_Rect* src,
	SDL_Surface *pDestSurface, SDL_Rect* dest,
	const Uint8 nOpacity)   //(in)   Tile opacity (0 = transparent, 255 = opaque).
{
	if (nOpacity<255)
	{
		EnableSurfaceBlending(pSrcSurface, nOpacity);
		SDL_BlitSurface(pSrcSurface, src, pDestSurface, dest);
		DisableSurfaceBlending(pSrcSurface);
	} else {
		SDL_BlitSurface(pSrcSurface, src, pDestSurface, dest);
	}
}

//**********************************************************************************
void CBitmapManager::BlitTileImagePart(
//Blits all or part of a tile image to a surface.
//This is useful when only a portion of the tile is non-transparent pixels,
//and the pixels outside of this region are all transparent and can be ignored.
//
//Params:
	const UINT wTileImageNo,         //(in)   Tile image to blit.
	const UINT x, const UINT y,      //(in)   Coords on dest surface to blit to.
	const UINT xSource, const UINT ySource, //(in) relative offset
	const UINT cxSize, const UINT cySize, //(in) Amount of tile to blit.
	SDL_Surface *pDestSurface,       //(in)   Surface to blit to.
	const bool bUseLightLevel, //(in) whether to assign light level [default=false]
	const Uint8 nOpacity)   //(in)   Tile opacity (0 = transparent, 255 = opaque).
{
	ASSERT(wTileImageNo < this->wNextCustomTileNo);

	ASSERT(xSource + cxSize <= (UINT)CX_TILE);
	ASSERT(ySource + cySize <= (UINT)CY_TILE);

	//Set coords and size.
	SDL_Rect src = MAKE_SDL_RECT(xSource,ySource,cxSize,cySize);
	SDL_Rect dest = MAKE_SDL_RECT(x,y,cxSize,cySize);

	//Get surface.
	SDL_Surface *pTileImagesSurface;
	if (wTileImageNo < this->wTileCount)
	{
		const UINT wSurfaceIndex = GetTileSurfaceNumber(wTileImageNo);
		pTileImagesSurface = this->pTileSurfaces[wSurfaceIndex];
		src.y += GetTileSurfaceY(wTileImageNo);

		//Set color key as appropriate.
		SetSurfaceColorKey(wTileImageNo, wSurfaceIndex, pTileImagesSurface);
	} else {
		//Custom tile.
		const CustomTiles *pCustomTiles = GetCustomTilesFor(wTileImageNo);
		if (!pCustomTiles)
			return; //some bookkeeping error occurred
		pTileImagesSurface = pCustomTiles->pSurface;

		const UINT wIndexInSurface = wTileImageNo - pCustomTiles->wTileStartNo;
		const UINT wTilesInRow = pTileImagesSurface->w/CBitmapManager::CX_TILE;
		src.x += (wIndexInSurface % wTilesInRow) * CBitmapManager::CX_TILE;
		src.y += (wIndexInSurface / wTilesInRow) * CBitmapManager::CY_TILE;
	}

	//Set transparency level if requested.  Blit.
	if (nOpacity<255)
		EnableSurfaceBlending(pTileImagesSurface, nOpacity);
	SDL_BlitSurface(pTileImagesSurface, &src, pDestSurface, &dest);
	if (nOpacity<255) //remove transparency level
		DisableSurfaceBlending(pTileImagesSurface);

	//Darken blitted area.
	if (bUseLightLevel)
	{
		const UINT wXOffset = x >= xSource ? xSource : xSource - x;
		const UINT wYOffset = y >= ySource ? ySource : ySource - y;
		DarkenTileWithMask(wTileImageNo, wXOffset, wYOffset,
				dest.x, dest.y, dest.w, dest.h, pDestSurface,
				CBitmapManager::fLightLevel + ((1.0f - CBitmapManager::fLightLevel) * ((255-nOpacity)/255.0f)));
	}
}

//**********************************************************************************
void CBitmapManager::BlitTileShadows(
//Darkens the destination location with the ORed area of the shadow tiles as a mask.
//
//Params:
	const UINT* pwShadowTIs, const UINT wNumShadows,   //(in)   Tile shadow mask(s)
	const UINT x, const UINT y,   //(in)   Coords on surface to blit.
	SDL_Surface *pDestSurface)    //(in)   Surface to blit to.
{
	ASSERT(wNumShadows > 0);

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"CBitmapManager::BlitTileShadows -- Lock surface failed.");
			return;
		}
	}

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
	ASSERT(this->pTileSurfaces[0]->format->Rmask == 0xff0000);
	ASSERT(this->pTileSurfaces[0]->format->Gmask == 0x00ff00);
	ASSERT(this->pTileSurfaces[0]->format->Bmask == 0x0000ff);
#endif
	const UINT wSrcBPP = this->pTileSurfaces[0]->format->BytesPerPixel;
	const UINT dwSrcRowOffset = this->pTileSurfaces[0]->pitch - (CX_TILE * wSrcBPP);
	const UINT wPixelByteNo = y * pDestSurface->pitch + (x * wBPP);
	const UINT dwRowOffset = pDestSurface->pitch - (CX_TILE * wBPP);

	//Find shadow tile masks.
	static const UINT wMaxShadows = 10;
	Uint8 *pSrc[wMaxShadows];
	ASSERT(wNumShadows < wMaxShadows);
	UINT wIndex;
	for (wIndex=wNumShadows; wIndex--; )
	{
		ASSERT(pwShadowTIs[wIndex] < this->wTileCount); //not custom tiles
		pSrc[wIndex] = GetTileSurfacePixel(pwShadowTIs[wIndex]) + PIXEL_FUDGE_FACTOR;
	}

//make pixel 75% as bright
#define ShadePixel(pDest) \
		pDest[0] -= (pDest[0] >> 2);\
		pDest[1] -= (pDest[1] >> 2);\
		pDest[2] -= (pDest[2] >> 2)

	Uint8 *pDest = (Uint8 *)pDestSurface->pixels + wPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pDest + (CY_TILE * pDestSurface->pitch);
	while (pDest != pStop)
	{
		ASSERT(pDest < pStop);
		Uint8 *const pEndOfRow = pDest + (CX_TILE * wBPP);

		if (wNumShadows == 1)
		{
			//A simple implementation for handling only one shadow mask.
			Uint8 *pSingleSrc = pSrc[0];
			while (pDest != pEndOfRow)
			{
				//Each iteration modifies one pixel.
				switch (*pSingleSrc) //examine only first color value
				{
					case 0: //full shadow
						ShadePixel(pDest);
					break;
					case 255: break; //no shadow
					default: //Partial shadow.
					{
						const Uint8* const shadowSrc = g_shadowCalc[*pSingleSrc];
						pDest[0] = shadowSrc[pDest[0]];
						pDest[1] = shadowSrc[pDest[1]];
						pDest[2] = shadowSrc[pDest[2]];
					}
					break;
				}
				pDest += wBPP;
				pSingleSrc += wSrcBPP;
			}
			pSrc[0] = pSingleSrc + dwSrcRowOffset;
		} else {
			//Check multiple shadow masks.
			while (pDest != pEndOfRow)
			{
				//Sum the amount of shadowing on this pixel.
				UINT sum=0;
				for (wIndex=0; wIndex<wNumShadows; ++wIndex)
				{
					switch (*pSrc[wIndex]) //examine only first color value
					{
						case 0: //full shadow
							sum = 255;
							wIndex = wNumShadows; //end loop now
						break;
						case 255: //no shadow
						break;
						default: //partial shadow
							sum += 255 - *pSrc[wIndex];
						break;
					}
				}
				if (sum >= 255)  //Full shadow
				{
					ShadePixel(pDest);
				} else if (sum > 0) {
					const Uint8* const shadowSrc = g_shadowCalcBlend[sum];
					pDest[0] = shadowSrc[pDest[0]];
					pDest[1] = shadowSrc[pDest[1]];
					pDest[2] = shadowSrc[pDest[2]];
				}
				for (wIndex=wNumShadows; wIndex--; )
					pSrc[wIndex] += wSrcBPP;
				pDest += wBPP;
			}
			for (wIndex=wNumShadows; wIndex--; )
				pSrc[wIndex] += dwSrcRowOffset;
		}
		pDest += dwRowOffset;
	}
#undef ShadePixel

	if (SDL_MUSTLOCK(pDestSurface))
		SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::BlitTileShadowsWithTileMask(
//Darkens the destination location with the ORed area of the shadow tiles as a mask.
//
//Params:
	const UINT* pwShadowTIs, const UINT wNumShadows,   //(in)   Tile shadow mask(s)
	const UINT wTIMask,           //(in) Shadow masks are intersected with
	                              //     non-transparent pixels in this tile mask
	const UINT x, const UINT y,   //(in) Coords on surface to blit.
	SDL_Surface *pDestSurface,    //(in) Surface to blit to.
	SDL_Rect *crop,   //if not NULL, defines a sub-tile crop area
	int maskStartX, int maskStartY)
{
	ASSERT(wNumShadows > 0);

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"CBitmapManager::BlitTileShadowsWithTileMask -- Lock surface failed.");
			return;
		}
	}

	UINT xStart=0, yStart=0, w=CX_TILE, h=CY_TILE;
	if (crop) {
		xStart = crop->x;
		yStart = crop->y;
		w = crop->w;
		h = crop->h;
	}

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
	ASSERT(this->pTileSurfaces[0]->format->Rmask == 0xff0000);
	ASSERT(this->pTileSurfaces[0]->format->Gmask == 0x00ff00);
	ASSERT(this->pTileSurfaces[0]->format->Bmask == 0x0000ff);
#endif
	const UINT wSrcBPP = this->pTileSurfaces[0]->format->BytesPerPixel;
	const UINT dwSrcRowOffset = this->pTileSurfaces[0]->pitch - (w * wSrcBPP);

	const UINT dwRowOffset = pDestSurface->pitch - (w * wBPP);
	SDL_Surface *pMaskSurface = GetTileSurface(wTIMask);
	ASSERT(pMaskSurface);
	const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;

	//Default indicates to match any crop rect.
	if (maskStartX == -1)
		maskStartX = xStart;
	if (maskStartY == -1)
		maskStartY = yStart;
	ASSERT(maskStartX + w <= CX_TILE);
	ASSERT(maskStartY + h <= CY_TILE);

	const UINT dwMaskRowOffset = pMaskSurface->pitch - (w * wMaskBPP);
	Uint8 *pMask = GetTileSurfacePixel(wTIMask, maskStartX, maskStartY) + PIXEL_FUDGE_FACTOR;
	ASSERT(pMask);

	//Find shadow tile masks.
	vector<Uint8 *>pSrc;
	UINT wIndex;
	for (wIndex=0; wIndex<wNumShadows; ++wIndex)
	{
		ASSERT(pwShadowTIs[wIndex] < this->wTileCount); //not custom tiles
		pSrc.push_back(GetTileSurfacePixel(pwShadowTIs[wIndex], xStart, yStart) + PIXEL_FUDGE_FACTOR);
	}

//make pixel 75% as bright
#define ShadePixel(pDest) \
		pDest[0] -= (pDest[0] >> 2);\
		pDest[1] -= (pDest[1] >> 2);\
		pDest[2] -= (pDest[2] >> 2)

	const UINT wPixelByteNo = y * pDestSurface->pitch + (x * wBPP);
	Uint8 *pDest = (Uint8 *)pDestSurface->pixels + wPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pDest + (h * pDestSurface->pitch);
	while (pDest != pStop)
	{
		ASSERT(pDest < pStop);
		Uint8 *const pEndOfRow = pDest + (w * wBPP);

		if (wNumShadows == 1)
		{
			//A simple implementation for handling only one shadow mask.
			Uint8 *pSingleSrc = pSrc[0];
			while (pDest != pEndOfRow)
			{
				//Each iteration modifies one pixel.

				//Skip color-keyed transparent pixels.
				if (pMask[0] != TransColor[0] || pMask[1] != TransColor[1] || pMask[2] != TransColor[2])
				{
					switch (*pSingleSrc) //examine only first color value
					{
						case 0: //full shadow
							ShadePixel(pDest);
						break;
						case 255: break; //no shadow
						default: //Partial shadow.
						{
							const Uint8* const shadowSrc = g_shadowCalc[*pSingleSrc];
							pDest[0] = shadowSrc[pDest[0]];
							pDest[1] = shadowSrc[pDest[1]];
							pDest[2] = shadowSrc[pDest[2]];
						}
						break;
					}
				}
				pDest += wBPP;
				pMask += wMaskBPP;
				pSingleSrc += wSrcBPP;
			}
			pSrc[0] = pSingleSrc + dwSrcRowOffset;
		} else {
			//Check multiple shadow masks.
			while (pDest != pEndOfRow)
			{
				//Skip color-keyed transparent pixels.
				if (pMask[0] != TransColor[0] || pMask[1] != TransColor[1] || pMask[2] != TransColor[2])
				{
					//Sum the amount of shadowing on this pixel.
					UINT sum=0;
					for (wIndex=0; wIndex<wNumShadows; ++wIndex)
					{
						switch (*pSrc[wIndex]) //examine only first color value
						{
							case 0: //full shadow
								sum = 255;
								wIndex = wNumShadows; //end loop now
							break;
							case 255: //no shadow
							break;
							default: //partial shadow
								sum += 255 - *pSrc[wIndex];
							break;
						}
					}
					if (sum >= 255)  //Full shadow
					{
						ShadePixel(pDest);
					} else if (sum > 0) {
						const Uint8* const shadowSrc = g_shadowCalcBlend[sum];
						pDest[0] = shadowSrc[pDest[0]];
						pDest[1] = shadowSrc[pDest[1]];
						pDest[2] = shadowSrc[pDest[2]];
					}
				}
				pDest += wBPP;
				pMask += wMaskBPP;
				for (wIndex=wNumShadows; wIndex--; )
					pSrc[wIndex] += wSrcBPP;
			}
			for (wIndex=wNumShadows; wIndex--; )
				pSrc[wIndex] += dwSrcRowOffset;
		}
		pMask += dwMaskRowOffset;
		pDest += dwRowOffset;
	}
#undef ShadePixel

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::BlitWithTileMask(
//Blits a tile-sized region from a wrappable source surface to the dest surface, using the
//specified tile as a mask.
//If the source rect exceeds the source surface bounds, the source image is wrapped.
//
//Params:
	const UINT wTIMask,	//(in) tile mask
	SDL_Rect src, SDL_Surface *pSrcSurface,	//(in) source
	SDL_Rect dest, SDL_Surface *pDestSurface,		//(in/out) dest
	const Uint8 opacity)
{
	ASSERT(src.w == (int)CX_TILE);
	ASSERT(src.h == (int)CY_TILE);
	ASSERT(dest.w == (int)CX_TILE);
	ASSERT(dest.h == (int)CY_TILE);
	ASSERT(opacity > 0);	//wasteful for complete transparency

	//Start blitting from within the source image bounds.
	while (src.x < 0) src.x += pSrcSurface->w;
	while (src.y < 0) src.y += pSrcSurface->h;
	src.x = src.x % pSrcSurface->w;
	src.y = src.y % pSrcSurface->h;

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"CBitmapManager::BlitWithTileMask -- Lock surface failed.");
			return;
		}
	}

	SDL_Surface *pMaskSurface = GetTileSurface(wTIMask);
	ASSERT(pMaskSurface);

	const UINT wSrcBPP = pSrcSurface->format->BytesPerPixel;
	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
	ASSERT(pMaskSurface->format->Rmask == 0xff0000);
	ASSERT(pMaskSurface->format->Gmask == 0x00ff00);
	ASSERT(pMaskSurface->format->Bmask == 0x0000ff);
#endif
	const UINT dwMaskRowOffset = pMaskSurface->pitch - (CX_TILE * wMaskBPP);
	const UINT wSrcPixelByteNo = src.y * pSrcSurface->pitch + (src.x * wSrcBPP);
	const UINT wDestPixelByteNo = dest.y * pDestSurface->pitch + (dest.x * wBPP);
	const UINT dwSrcRowOffset = pSrcSurface->pitch - (src.w * wSrcBPP);
	const UINT dwDestRowOffset = pDestSurface->pitch - (dest.w * wBPP);

	Uint8 *pMask = GetTileSurfacePixel(wTIMask) + PIXEL_FUDGE_FACTOR;
	Uint8 *pSrc = (Uint8 *)pSrcSurface->pixels + wSrcPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *pDest = (Uint8 *)pDestSurface->pixels + wDestPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pDest + (dest.h * pDestSurface->pitch);
	const int wPixelsToEndOfSrcRow = pSrcSurface->w - src.x;	//optimization
	int wPixelsToEndOfRow, wRowsToEndOfSrc = pSrcSurface->h - src.y;

	if (opacity == 255)
	{
		while (pDest != pStop)
		{
			ASSERT(pDest < pStop);
			Uint8 *const pEndOfRow = pDest + (dest.w * wBPP);
			wPixelsToEndOfRow = wPixelsToEndOfSrcRow;

			while (pDest != pEndOfRow)
			{
				//Each iteration modifies one pixel.
				if (*pMask == 0)   //examine only first color value: 0 = mask color
				{
					//Copy pixel.
					pDest[0] = pSrc[0];
					pDest[1] = pSrc[1];
					pDest[2] = pSrc[2];
				}
				pMask += wMaskBPP;
				pDest += wBPP;
				pSrc += wSrcBPP;

				if (--wPixelsToEndOfRow == 0)	//If the end of the source row is reached...
					pSrc -= pSrcSurface->w * wSrcBPP;	//Continue from the beginning of same row.
			}
			pMask += dwMaskRowOffset;
			pDest += dwDestRowOffset;
			pSrc += dwSrcRowOffset;

			if (wPixelsToEndOfRow <= 0)	//If we wrapped back to beginning of row...
				pSrc += pSrcSurface->w * wSrcBPP;	//jump back down to next one.
			if (--wRowsToEndOfSrc == 0)	//If end of source image is reached...
				pSrc -= pSrcSurface->h * pSrcSurface->pitch;	//Continue from top row.
		}
	} else {
		//Alpha blending.
		const Uint8 transparency = 256 - opacity;
		while (pDest != pStop)
		{
			ASSERT(pDest < pStop);
			Uint8 *const pEndOfRow = pDest + (dest.w * wBPP);
			wPixelsToEndOfRow = wPixelsToEndOfSrcRow;

			while (pDest != pEndOfRow)
			{
				//Each iteration modifies one pixel.
				if (*pMask == 0)   //examine only first color value: 0 = mask color
				{
					//Copy pixel w/ alpha blending.
					pDest[0] = (pDest[0] * transparency + pSrc[0] * opacity) / 256;
					pDest[1] = (pDest[1] * transparency + pSrc[1] * opacity) / 256;
					pDest[2] = (pDest[2] * transparency + pSrc[2] * opacity) / 256;
				}
				pMask += wMaskBPP;
				pDest += wBPP;
				pSrc += wSrcBPP;

				if (--wPixelsToEndOfRow == 0)	//If the end of the source row is reached...
					pSrc -= pSrcSurface->w * wSrcBPP;	//Continue from the beginning of same row.
			}
			pMask += dwMaskRowOffset;
			pDest += dwDestRowOffset;
			pSrc += dwSrcRowOffset;

			if (wPixelsToEndOfRow <= 0)	//If we wrapped back to beginning of row...
				pSrc += pSrcSurface->w * wSrcBPP;	//jump back down to next one.
			if (--wRowsToEndOfSrc == 0)	//If end of source image is reached...
				pSrc -= pSrcSurface->h * pSrcSurface->pitch;	//Continue from top row.
		}
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::BlitTileWithTileMask(
//Blits a tile to the dest surface, using the first specified tile as a mask.
//Transparent (color-keyed) pixels are skipped.
//
//Params:
	const UINT wTIMask, const UINT wTileNo,	//(in) mask, tile to blit
	SDL_Rect dest, SDL_Surface *pDestSurface,		//(in/out) dest
	const Uint8 opacity)
{
	ASSERT((UINT)dest.w == CX_TILE);
	ASSERT((UINT)dest.h == CY_TILE);
	ASSERT(opacity > 0);	//wasteful for complete transparency

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"CBitmapManager::BlitTileWithTileMask -- Lock surface failed.");
			return;
		}
	}

	SDL_Surface *pSrcSurface = GetTileSurface(wTileNo);
	ASSERT(pSrcSurface);
	SDL_Surface *pMaskSurface = GetTileSurface(wTIMask);
	ASSERT(pMaskSurface);

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
	ASSERT(pMaskSurface->format->Rmask == 0xff0000);
	ASSERT(pMaskSurface->format->Gmask == 0x00ff00);
	ASSERT(pMaskSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wSrcBPP = pSrcSurface->format->BytesPerPixel;
	const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;
	const UINT dwMaskRowOffset = pMaskSurface->pitch - (CX_TILE * wMaskBPP);
	const UINT dwSrcRowOffset = pSrcSurface->pitch - (CX_TILE * wSrcBPP);
	const UINT wDestPixelByteNo = dest.y * pDestSurface->pitch + (dest.x * wBPP);
	const UINT dwDestRowWidth = dest.w * wBPP;
	const UINT dwDestRowOffset = pDestSurface->pitch - dwDestRowWidth;

	Uint8 *pMask = GetTileSurfacePixel(wTIMask) + PIXEL_FUDGE_FACTOR;
	Uint8 *pSrc = GetTileSurfacePixel(wTileNo) + PIXEL_FUDGE_FACTOR;
	Uint8 *pDest = (Uint8 *)pDestSurface->pixels + wDestPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pDest + (dest.h * pDestSurface->pitch);

	if (opacity == 255)
	{
		while (pDest != pStop)
		{
			ASSERT(pDest < pStop);
			Uint8 *const pEndOfRow = pDest + dwDestRowWidth;

			while (pDest != pEndOfRow)
			{
				//Each iteration modifies one pixel.
				if (pMask[0] == 0 && pMask[1] == 0 && pMask[2] == 0)   //0 = mask color
				{
					//Skip color-keyed transparent pixels.
					if (pSrc[0] != TransColor[0] || pSrc[1] != TransColor[1] || pSrc[2] != TransColor[2])
					{
						//Copy pixel.
						pDest[0] = pSrc[0];
						pDest[1] = pSrc[1];
						pDest[2] = pSrc[2];
					}
				}
				pMask += wMaskBPP;
				pDest += wBPP;
				pSrc += wSrcBPP;
			}
			pMask += dwMaskRowOffset;
			pDest += dwDestRowOffset;
			pSrc += dwSrcRowOffset;
		}
	} else {
		const Uint8 transparency = 256 - opacity;
		while (pDest != pStop)
		{
			ASSERT(pDest < pStop);
			Uint8 *const pEndOfRow = pDest + dwDestRowWidth;

			while (pDest != pEndOfRow)
			{
				//Each iteration modifies one pixel.
				if (pMask[0] == 0 && pMask[1] == 0 && pMask[2] == 0)   //0 = mask color
				{
					//Skip color-keyed transparent pixels.
					if (pSrc[0] != TransColor[0] || pSrc[1] != TransColor[1] || pSrc[2] != TransColor[2])
					{
						//Copy pixel w/ alpha blending.
						pDest[0] = (pDest[0] * transparency + pSrc[0] * opacity) / 256;
						pDest[1] = (pDest[1] * transparency + pSrc[1] * opacity) / 256;
						pDest[2] = (pDest[2] * transparency + pSrc[2] * opacity) / 256;
					}
				}
				pMask += wMaskBPP;
				pDest += wBPP;
				pSrc += wSrcBPP;
			}
			pMask += dwMaskRowOffset;
			pDest += dwDestRowOffset;
			pSrc += dwSrcRowOffset;
		}
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::BlitWithMask(
//Blits a tile-sized region from a wrappable source surface to the dest surface, using the
//specified mask surface.
//If the source rect exceeds the source surface bounds, the source image is wrapped.
//
//Params:
	SDL_Rect mask, SDL_Surface *pMaskSurface,	//(in) tile mask
	SDL_Rect src, SDL_Surface *pSrcSurface,	//(in) source
	SDL_Rect dest, SDL_Surface *pDestSurface,		//(in/out) dest
	const Uint8 opacity)
{
	ASSERT(src.w == (int)CX_TILE);
	ASSERT(src.h == (int)CY_TILE);
	ASSERT(dest.w == (int)CX_TILE);
	ASSERT(dest.h == (int)CY_TILE);
	ASSERT(mask.w == (int)CX_TILE);
	ASSERT(mask.h == (int)CY_TILE);
	ASSERT(opacity > 0);	//wasteful for complete transparency

	//Start blitting from within the source image bounds.
	while (src.x < 0) src.x += pSrcSurface->w;
	while (src.y < 0) src.y += pSrcSurface->h;
	src.x = src.x % pSrcSurface->w;
	src.y = src.y % pSrcSurface->h;

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"CBitmapManager::BlitWithTileMask -- Lock surface failed.");
			return;
		}
	}

	const UINT wSrcBPP = pSrcSurface->format->BytesPerPixel;
	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
	ASSERT(pMaskSurface->format->Rmask == 0xff0000);
	ASSERT(pMaskSurface->format->Gmask == 0x00ff00);
	ASSERT(pMaskSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wMaskPixelByteNo = mask.y * pMaskSurface->pitch + (mask.x * wMaskBPP);
	const UINT wSrcPixelByteNo = src.y * pSrcSurface->pitch + (src.x * wSrcBPP);
	const UINT wDestPixelByteNo = dest.y * pDestSurface->pitch + (dest.x * wBPP);
	const UINT dwMaskRowOffset = pMaskSurface->pitch - (mask.w * wMaskBPP);
	const UINT dwSrcRowOffset = pSrcSurface->pitch - (src.w * wSrcBPP);
	const UINT dwDestRowOffset = pDestSurface->pitch - (dest.w * wBPP);

	Uint8 *pMask = (Uint8 *)pMaskSurface->pixels + wMaskPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *pSrc = (Uint8 *)pSrcSurface->pixels + wSrcPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *pDest = (Uint8 *)pDestSurface->pixels + wDestPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pDest + (dest.h * pDestSurface->pitch);
	const int wPixelsToEndOfSrcRow = pSrcSurface->w - src.x;	//optimization
	int wPixelsToEndOfRow, wRowsToEndOfSrc = pSrcSurface->h - src.y;

	if (opacity == 255)
	{
		while (pDest != pStop)
		{
			ASSERT(pDest < pStop);
			Uint8 *const pEndOfRow = pDest + (dest.w * wBPP);
			wPixelsToEndOfRow = wPixelsToEndOfSrcRow;

			while (pDest != pEndOfRow)
			{
				//Each iteration modifies one pixel.
				if (*pMask == 0)   //examine only first color value: 0 = mask color
				{
					//Copy pixel.
					pDest[0] = pSrc[0];
					pDest[1] = pSrc[1];
					pDest[2] = pSrc[2];
				}
				pMask += wMaskBPP;
				pDest += wBPP;
				pSrc += wSrcBPP;

				if (--wPixelsToEndOfRow == 0)	//If the end of the source row is reached...
					pSrc -= pSrcSurface->w * wSrcBPP;	//Continue from the beginning of same row.
			}
			pMask += dwMaskRowOffset;
			pDest += dwDestRowOffset;
			pSrc += dwSrcRowOffset;

			if (wPixelsToEndOfRow <= 0)	//If we wrapped back to beginning of row...
				pSrc += pSrcSurface->w * wSrcBPP;	//jump back down to next one.
			if (--wRowsToEndOfSrc == 0)	//If end of source image is reached...
				pSrc -= pSrcSurface->h * pSrcSurface->pitch;	//Continue from top row.
		}
	} else {
		//Alpha blending.
		const Uint8 transparency = 256 - opacity;
		while (pDest != pStop)
		{
			ASSERT(pDest < pStop);
			Uint8 *const pEndOfRow = pDest + (dest.w * wBPP);
			wPixelsToEndOfRow = wPixelsToEndOfSrcRow;

			while (pDest != pEndOfRow)
			{
				//Each iteration modifies one pixel.
				if (*pMask == 0)   //examine only first color value: 0 = mask color
				{
					//Copy pixel w/ alpha blending.
					pDest[0] = (pDest[0] * transparency + pSrc[0] * opacity) / 256;
					pDest[1] = (pDest[1] * transparency + pSrc[1] * opacity) / 256;
					pDest[2] = (pDest[2] * transparency + pSrc[2] * opacity) / 256;
				}
				pMask += wMaskBPP;
				pDest += wBPP;
				pSrc += wSrcBPP;

				if (--wPixelsToEndOfRow == 0)	//If the end of the source row is reached...
					pSrc -= pSrcSurface->w * wSrcBPP;	//Continue from the beginning of same row.
			}
			pMask += dwMaskRowOffset;
			pDest += dwDestRowOffset;
			pSrc += dwSrcRowOffset;

			if (wPixelsToEndOfRow <= 0)	//If we wrapped back to beginning of row...
				pSrc += pSrcSurface->w * wSrcBPP;	//jump back down to next one.
			if (--wRowsToEndOfSrc == 0)	//If end of source image is reached...
				pSrc -= pSrcSurface->h * pSrcSurface->pitch;	//Continue from top row.
		}
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::BlitTileWithMask(
//Blits a tile to the dest surface, using the first specified tile as a mask.
//Transparent (color-keyed) pixels are skipped.
//
//Params:
	SDL_Rect mask, SDL_Surface *pMaskSurface, const UINT wTileNo,	//(in) mask, tile to blit
	SDL_Rect dest, SDL_Surface *pDestSurface,		//(in/out) dest
	const Uint8 opacity)
{
	ASSERT((UINT)mask.x == CX_TILE);
	ASSERT((UINT)mask.y == CY_TILE);
	ASSERT((UINT)dest.w == CX_TILE);
	ASSERT((UINT)dest.h == CY_TILE);
	ASSERT(opacity > 0);	//wasteful for complete transparency

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"CBitmapManager::BlitTileWithTileMask -- Lock surface failed.");
			return;
		}
	}

	SDL_Surface *pSrcSurface = GetTileSurface(wTileNo);
	ASSERT(pSrcSurface);

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
	ASSERT(pMaskSurface->format->Rmask == 0xff0000);
	ASSERT(pMaskSurface->format->Gmask == 0x00ff00);
	ASSERT(pMaskSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wSrcBPP = pSrcSurface->format->BytesPerPixel;
	const UINT dwSrcRowOffset = pSrcSurface->pitch - (CX_TILE * wSrcBPP);
	const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;
	const UINT wMaskPixelByteNo = mask.y * pMaskSurface->pitch + (mask.x * wMaskBPP);
	const UINT dwMaskRowOffset = pMaskSurface->pitch - (mask.w * wMaskBPP);
	const UINT wDestPixelByteNo = dest.y * pDestSurface->pitch + (dest.x * wBPP);
	const UINT dwDestRowWidth = dest.w * wBPP;
	const UINT dwDestRowOffset = pDestSurface->pitch - dwDestRowWidth;

	Uint8 *pSrc = GetTileSurfacePixel(wTileNo) + PIXEL_FUDGE_FACTOR;
	Uint8 *pMask = (Uint8 *)pMaskSurface->pixels + wMaskPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *pDest = (Uint8 *)pDestSurface->pixels + wDestPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pDest + (dest.h * pDestSurface->pitch);

	if (opacity == 255)
	{
		while (pDest != pStop)
		{
			ASSERT(pDest < pStop);
			Uint8 *const pEndOfRow = pDest + dwDestRowWidth;

			while (pDest != pEndOfRow)
			{
				//Each iteration modifies one pixel.
				if (pMask[0] == 0 && pMask[1] == 0 && pMask[2] == 0)   //0 = mask color
				{
					//Skip color-keyed transparent pixels.
					if (pSrc[0] != TransColor[0] || pSrc[1] != TransColor[1] || pSrc[2] != TransColor[2])
					{
						//Copy pixel.
						pDest[0] = pSrc[0];
						pDest[1] = pSrc[1];
						pDest[2] = pSrc[2];
					}
				}
				pMask += wMaskBPP;
				pDest += wBPP;
				pSrc += wSrcBPP;
			}
			pMask += dwMaskRowOffset;
			pDest += dwDestRowOffset;
			pSrc += dwSrcRowOffset;
		}
	} else {
		const Uint8 transparency = 256 - opacity;
		while (pDest != pStop)
		{
			ASSERT(pDest < pStop);
			Uint8 *const pEndOfRow = pDest + dwDestRowWidth;

			while (pDest != pEndOfRow)
			{
				//Each iteration modifies one pixel.
				if (pMask[0] == 0 && pMask[1] == 0 && pMask[2] == 0)   //0 = mask color
				{
					//Skip color-keyed transparent pixels.
					if (pSrc[0] != TransColor[0] || pSrc[1] != TransColor[1] || pSrc[2] != TransColor[2])
					{
						//Copy pixel w/ alpha blending.
						pDest[0] = (pDest[0] * transparency + pSrc[0] * opacity) / 256;
						pDest[1] = (pDest[1] * transparency + pSrc[1] * opacity) / 256;
						pDest[2] = (pDest[2] * transparency + pSrc[2] * opacity) / 256;
					}
				}
				pMask += wMaskBPP;
				pDest += wBPP;
				pSrc += wSrcBPP;
			}
			pMask += dwMaskRowOffset;
			pDest += dwDestRowOffset;
			pSrc += dwSrcRowOffset;
		}
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::BlitWrappingSurface(
//Blit an area from a wrappable source surface.
//If the source rect exceeds the source surface bounds, the source image is wrapped.
//NOTE: this only wraps once and does not tile an image repeatedly onto a destination.
//
//Params:
	SDL_Surface *pSrcSurface, SDL_Rect src,
	SDL_Surface *pDestSurface, SDL_Rect dest,
	const Uint8 nOpacity) //[default=255]
{
	ASSERT(pSrcSurface);
	ASSERT(pDestSurface);

	if (!src.w || !src.h || !dest.w || !dest.h) return;	//Nothing to blit.

	if (src.w > pSrcSurface->w) src.w = pSrcSurface->w;	//Size checks.
	if (src.h > pSrcSurface->h) src.h = pSrcSurface->h;

	//Start blitting from within the source image bounds.
	while (src.x < 0) src.x += pSrcSurface->w;
	while (src.y < 0) src.y += pSrcSurface->h;
	src.x = src.x % pSrcSurface->w;
	src.y = src.y % pSrcSurface->h;

	//Determine how much blit would go past source surface edge.
	const int dx = (src.x + src.w) - pSrcSurface->w;
	const int dy = (src.y + src.h) - pSrcSurface->h;

	if (nOpacity<255)
		EnableSurfaceBlending(pSrcSurface, nOpacity);

	if (dx <= 0 && dy <= 0)
	{
		//No wrapping.
		SDL_BlitSurface(pSrcSurface, &src, pDestSurface, &dest);
	}
	else if (dx > 0)
	{
		if (dy <= 0)
		{
			//Wrap in x direction.
			SDL_BlitSurface(pSrcSurface, &src, pDestSurface, &dest);
			src.w = dx;		//continue blitting from beginning of source
			dest.x += pSrcSurface->w - src.x;
			src.x = 0;
			SDL_BlitSurface(pSrcSurface, &src, pDestSurface, &dest);
		} else {
			//Wrap source image in x/y directions.
			const Uint16 wOldW = src.w;
			const Sint16 nOldSX = src.x;
			const Sint16 nOldDX = dest.x;
			SDL_BlitSurface(pSrcSurface, &src, pDestSurface, &dest);
			src.w = dx;
			dest.x += pSrcSurface->w - src.x;
			src.x = 0;
			SDL_BlitSurface(pSrcSurface, &src, pDestSurface, &dest);
			src.h = dy;
			dest.y += pSrcSurface->h - src.y;
			src.y = 0;
			SDL_BlitSurface(pSrcSurface, &src, pDestSurface, &dest);
			src.x = nOldSX;
			dest.x = nOldDX;
			src.w = wOldW;
			SDL_BlitSurface(pSrcSurface, &src, pDestSurface, &dest);
		}
	} else {
		//Wrap in y direction.
		ASSERT(dy > 0);
		SDL_BlitSurface(pSrcSurface, &src, pDestSurface, &dest);
		src.h = dy;		//continue blitting from beginning of source
		dest.y += pSrcSurface->h - src.y;
		src.y = 0;
		SDL_BlitSurface(pSrcSurface, &src, pDestSurface, &dest);
	}

	if (nOpacity<255)
		DisableSurfaceBlending(pSrcSurface);
}

//**********************************************************************************
void CBitmapManager::BAndWRect(
//Sets pixels to the black-and-white equivalent of their value.
//
//Params:
	const UINT x, const UINT y,   //(in)   Pixel coords.
	const UINT w, const UINT h,   //
	SDL_Surface *pDestSurface,    //(in)   Surface to blit to.
	UINT tileMask, UINT tileMaskOffsetX, UINT tileMaskOffsetY) //[default=TI_UNSPECIFIED,0,0]
{
	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wPixelByteNo = y * pDestSurface->pitch + (x * wBPP);
	const UINT dwRowWidth = w * wBPP;
	const UINT dwRowOffset = pDestSurface->pitch - dwRowWidth;

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"CBitmapManager::BAndWRect: Lock surface failed.");
			return;
		}
	}

	//Get color index.
	const UINT wR = pDestSurface->format->Rshift / 8;
	const UINT wG = pDestSurface->format->Gshift / 8;
	const UINT wB = pDestSurface->format->Bshift / 8;

	Uint8 *pSeek = (Uint8 *)pDestSurface->pixels + wPixelByteNo;
	Uint8 *const pStop = pSeek + (h * pDestSurface->pitch);

	Uint8 nValue;
	if (tileMask == TI_UNSPECIFIED)
	{
		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			Uint8 *const pEndOfRow = pSeek + dwRowWidth;

			//Each iteration modifies one pixel.
			while (pSeek != pEndOfRow)
			{
				//Set pixel to the black-and-white equivalent of its value.
				// In order to match the perceived differences in brightness
				// between the original and the black-and-white across all colors, the
				// bands should be weighted according to 0.2126 R + 0.7152 G + 0.0722 B.
				// Alternately, 0.299R + 0.587G + 0.114B.
				nValue = (Uint8)(pSeek[wR]*0.3f + pSeek[wG]*0.6f + pSeek[wB]*0.1f);
				pSeek[wR] = pSeek[wG] = pSeek[wB] = nValue;
				pSeek += wBPP;
			}
			pSeek += dwRowOffset;
		}
	} else {
		SDL_Surface *pMaskSurface = GetTileSurface(tileMask);
		ASSERT(pMaskSurface);
		const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;
		const UINT dwMaskRowOffset = pMaskSurface->pitch - (w * wMaskBPP);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		ASSERT(pMaskSurface->format->Rmask == 0xff0000);
		ASSERT(pMaskSurface->format->Gmask == 0x00ff00);
		ASSERT(pMaskSurface->format->Bmask == 0x0000ff);
#endif

		Uint8 *pMask = GetTileSurfacePixel(tileMask, tileMaskOffsetX, tileMaskOffsetY) + PIXEL_FUDGE_FACTOR;

		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			Uint8 *const pEndOfRow = pSeek + dwRowWidth;

			//Each iteration modifies one pixel.
			while (pSeek != pEndOfRow)
			{
				//Skip color-keyed transparent pixels on tile mask.
				if (pMask[0] != TransColor[0] || pMask[1] != TransColor[1] || pMask[2] != TransColor[2])
				{
					//Set pixel to the black-and-white equivalent of its value.
					// In order to match the perceived differences in brightness
					// between the original and the black-and-white across all colors, the
					// bands should be weighted according to 0.2126 R + 0.7152 G + 0.0722 B.
					// Alternately, 0.299R + 0.587G + 0.114B.
					nValue = (Uint8)(pSeek[wR]*0.3f + pSeek[wG]*0.6f + pSeek[wB]*0.1f);
					pSeek[wR] = pSeek[wG] = pSeek[wB] = nValue;
				}
				pSeek += wBPP;
				pMask += wMaskBPP;
			}
			pSeek += dwRowOffset;
			pMask += dwMaskRowOffset;
		}
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::SepiaRect(
//Sets pixels to the sepia equivalent of their value.
//
//Params:
	const UINT x, const UINT y,   //(in)   Pixel coords.
	const UINT w, const UINT h,   //
	SDL_Surface *pDestSurface,    //(in)   Surface to blit to.
	UINT tileMask, UINT tileMaskOffsetX, UINT tileMaskOffsetY) //[default=TI_UNSPECIFIED,0,0]
{
	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wPixelByteNo = y * pDestSurface->pitch + (x * wBPP);
	const UINT dwRowWidth = w * wBPP;
	const UINT dwRowOffset = pDestSurface->pitch - dwRowWidth;

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"CBitmapManager::SepiaRect: Lock surface failed.");
			return;
		}
	}

	//Get color index.
	const UINT wR = pDestSurface->format->Rshift / 8;
	const UINT wG = pDestSurface->format->Gshift / 8;
	const UINT wB = pDestSurface->format->Bshift / 8;

	Uint8 *pSeek = (Uint8 *)pDestSurface->pixels + wPixelByteNo;
	Uint8 *const pStop = pSeek + (h * pDestSurface->pitch);

	//Prebuild sepia color translation table.
	static bool prebuilt = false;
	static Uint8 sepia_translate[3][256];
	if (!prebuilt) {
		prebuilt = true;
		for (int i=0; i<256; ++i) {
			static const int sepiaDepth = 15;
			sepia_translate[0][i] = (Uint8)(min(255,i + sepiaDepth * 2));
			sepia_translate[1][i] = (Uint8)(min(255,i + sepiaDepth));
			sepia_translate[2][i] = (Uint8)(max(0,i - sepiaDepth));
		}
	}

	Uint8 gray;
	if (tileMask == TI_UNSPECIFIED)
	{
		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			Uint8 *const pEndOfRow = pSeek + dwRowWidth;

			//Each iteration modifies one pixel.
			while (pSeek != pEndOfRow)
			{
				//Same as b+w translation.
				//gray = (int(pSeek[wR]) + int(pSeek[wG]) + int(pSeek[wB])) / 3;
				gray = (Uint8)(pSeek[wR]*0.3f + pSeek[wG]*0.6f + pSeek[wB]*0.1f);
				pSeek[wR] = sepia_translate[0][gray];
				pSeek[wG] = sepia_translate[1][gray];
				pSeek[wB] = sepia_translate[2][gray];
				pSeek += wBPP;
			}
			pSeek += dwRowOffset;
		}
	} else {
		SDL_Surface *pMaskSurface = GetTileSurface(tileMask);
		ASSERT(pMaskSurface);
		const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;
		const UINT dwMaskRowOffset = pMaskSurface->pitch - (w * wMaskBPP);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		ASSERT(pMaskSurface->format->Rmask == 0xff0000);
		ASSERT(pMaskSurface->format->Gmask == 0x00ff00);
		ASSERT(pMaskSurface->format->Bmask == 0x0000ff);
#endif

		Uint8 *pMask = GetTileSurfacePixel(tileMask, tileMaskOffsetX, tileMaskOffsetY) + PIXEL_FUDGE_FACTOR;

		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			Uint8 *const pEndOfRow = pSeek + dwRowWidth;

			//Each iteration modifies one pixel.
			while (pSeek != pEndOfRow)
			{
				//Skip color-keyed transparent pixels on tile mask.
				if (pMask[0] != TransColor[0] || pMask[1] != TransColor[1] || pMask[2] != TransColor[2])
				{
					//Same as b+w translation.
					//gray = (int(pSeek[wR]) + int(pSeek[wG]) + int(pSeek[wB])) / 3;
					gray = (Uint8)(pSeek[wR]*0.3f + pSeek[wG]*0.6f + pSeek[wB]*0.1f);
					pSeek[wR] = sepia_translate[0][gray];
					pSeek[wG] = sepia_translate[1][gray];
					pSeek[wB] = sepia_translate[2][gray];
				}
				pSeek += wBPP;
				pMask += wMaskBPP;
			}
			pSeek += dwRowOffset;
			pMask += dwMaskRowOffset;
		}
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::NegativeRect(
//Sets pixels to the negative equivalent of their value.
//
//Params:
	const UINT x, const UINT y,   //(in)   Pixel coords.
	const UINT w, const UINT h,   //
	SDL_Surface *pDestSurface,    //(in)   Surface to blit to.
	UINT tileMask, UINT tileMaskOffsetX, UINT tileMaskOffsetY) //[default=TI_UNSPECIFIED,0,0]
{
	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wPixelByteNo = y * pDestSurface->pitch + (x * wBPP);
	const UINT dwRowWidth = w * wBPP;
	const UINT dwRowOffset = pDestSurface->pitch - dwRowWidth;

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"CBitmapManager::NegativeRect: Lock surface failed.");
			return;
		}
	}

	//Get color index.
	const UINT wR = pDestSurface->format->Rshift / 8;
	const UINT wG = pDestSurface->format->Gshift / 8;
	const UINT wB = pDestSurface->format->Bshift / 8;

	Uint8 *pSeek = (Uint8 *)pDestSurface->pixels + wPixelByteNo;
	Uint8 *const pStop = pSeek + (h * pDestSurface->pitch);

	if (tileMask == TI_UNSPECIFIED)
	{
		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			Uint8 *const pEndOfRow = pSeek + dwRowWidth;

			//Each iteration modifies one pixel.
			while (pSeek != pEndOfRow)
			{
				pSeek[wR] = 255 - pSeek[wR];
				pSeek[wG] = 255 - pSeek[wG];
				pSeek[wB] = 255 - pSeek[wB];
				pSeek += wBPP;
			}
			pSeek += dwRowOffset;
		}
	} else {
		SDL_Surface *pMaskSurface = GetTileSurface(tileMask);
		ASSERT(pMaskSurface);
		const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;
		const UINT dwMaskRowOffset = pMaskSurface->pitch - (w * wMaskBPP);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		ASSERT(pMaskSurface->format->Rmask == 0xff0000);
		ASSERT(pMaskSurface->format->Gmask == 0x00ff00);
		ASSERT(pMaskSurface->format->Bmask == 0x0000ff);
#endif

		Uint8 *pMask = GetTileSurfacePixel(tileMask, tileMaskOffsetX, tileMaskOffsetY) + PIXEL_FUDGE_FACTOR;

		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			Uint8 *const pEndOfRow = pSeek + dwRowWidth;

			//Each iteration modifies one pixel.
			while (pSeek != pEndOfRow)
			{
				//Skip color-keyed transparent pixels on tile mask.
				if (pMask[0] != TransColor[0] || pMask[1] != TransColor[1] || pMask[2] != TransColor[2])
				{
					pSeek[wR] = 255 - pSeek[wR];
					pSeek[wG] = 255 - pSeek[wG];
					pSeek[wB] = 255 - pSeek[wB];
				}
				pSeek += wBPP;
				pMask += wMaskBPP;
			}
			pSeek += dwRowOffset;
			pMask += dwMaskRowOffset;
		}
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::ClipSrcAndDestToRect(
//Updates the dest rectangle to fit properly within the positive view area.
//Translates src to match.
	SDL_Rect& src, SDL_Rect& dest,
	const UINT W, const UINT H) //surface width and height
{
	//Clip source size to fit in destination rect.
	ASSERT(src.w == dest.w);
	ASSERT(src.h == dest.h);

	//Left
	if (dest.x < 0)
	{
		const UINT wDelta = UINT(-dest.x);
		if ((UINT)src.w < wDelta) //rect is outside view area -- shrink to nothing
		{
			src.w = src.h = dest.w = dest.h = 0;
			return;
		}
		dest.w -= wDelta;
		src.w -= wDelta;

		src.x -= dest.x;
		dest.x = 0;
	}

	//Top
	if (dest.y < 0)
	{
		const UINT wDelta = UINT(-dest.y);
		if ((UINT)src.h < wDelta) //a rect is outside view area -- shrink to nothing
		{
			src.w = src.h = dest.w = dest.h = 0;
			return;
		}
		dest.h -= wDelta;
		src.h -= wDelta;

		src.y -= dest.y;
		dest.y = 0;
	}

	//Right
	if ((UINT)dest.x + dest.w > W)
	{
		if ((UINT)dest.x >= W) //rect is outside view area -- shrink to nothing
		{
			src.w = src.h = dest.w = dest.h = 0;
			return;
		}
		const UINT wDelta = dest.x + dest.w - W;
		ASSERT((UINT)dest.w > wDelta);
		dest.w -= wDelta;
		src.w -= wDelta;
	}

	//Bottom
	if ((UINT)dest.y + dest.h > H)
	{
		if ((UINT)dest.y >= H) //rect is outside view area -- shrink to nothing
		{
			src.w = src.h = dest.w = dest.h = 0;
			return;
		}
		const UINT wDelta = dest.y + dest.h - H;
		ASSERT((UINT)dest.h > wDelta);
		dest.h -= wDelta;
		src.h -= wDelta;
	}
}

//*****************************************************************************
SDL_Surface* CBitmapManager::ConvertSurface(SDL_Surface *pSurface)
//Convert surface format to make blits fast (according to graphics hardware).
{
	if (!pSurface) return NULL;

	SDL_Surface *pRet = NULL;
	ASSERT(pSurface->pitch >= pSurface->w * pSurface->format->BytesPerPixel);
	ASSERT(SDL_WasInit(SDL_INIT_VIDEO));
	SDL_PixelFormat *fmt = GetWindowShadowSurface(GetMainWindow())->format;
	Uint32 fmtenum = SDL_MasksToPixelFormatEnum(32, fmt->Rmask, fmt->Gmask, fmt->Bmask,
			(pSurface->format->Amask) ? ~(fmt->Rmask|fmt->Gmask|fmt->Bmask) : 0);
	ASSERT(fmtenum != SDL_PIXELFORMAT_UNKNOWN);
	pRet = SDL_ConvertSurfaceFormat(pSurface, fmtenum, 0);
	if (!pRet) return pSurface;   //maybe no free memory to make new copy of surface
	SDL_FreeSurface(pSurface);

	return pRet;
}

//*****************************************************************************
SDL_Surface* CBitmapManager::CreateNewSurfaceLike(
//Creates a surface of the specified size, having the same properties
//as the source surface.
//
//Params:
	const SDL_Surface *pSrcSurface,
	const int width, const int height)
{
	//Create new surface.
	ASSERT(pSrcSurface->format->BytesPerPixel > 0 && pSrcSurface->format->BytesPerPixel <= 4);
	SDL_Surface* pDestSurface = SDL_CreateRGBSurface(pSrcSurface->flags,
			width, height, pSrcSurface->format->BitsPerPixel,
			pSrcSurface->format->Rmask, pSrcSurface->format->Gmask,
			pSrcSurface->format->Bmask, pSrcSurface->format->Amask);
	if (!pDestSurface)
		return NULL;

	// Copy surface info
	if (pSrcSurface->format->BytesPerPixel==1 && pSrcSurface->format->palette)
		SDL_SetPaletteColors(pDestSurface->format->palette, pSrcSurface->format->palette->colors, 0,
				pSrcSurface->format->palette->ncolors);
	Uint32 ckey;
	if (SDL_GetColorKey((SDL_Surface*)pSrcSurface, &ckey) == 0)
		SetColorKey(pDestSurface, SDL_TRUE, ckey);
	if (pSrcSurface->flags & SDL_RLEACCEL)
		SDL_SetSurfaceRLE(pDestSurface, 1);

	return pDestSurface;
}

//*****************************************************************************
void CBitmapManager::CropToOpaque(
//Starting with the given rect, find the dimensions that tightly bound all
//non-transparent pixels in that region of the specified surface.
//
//Params:
	SDL_Rect &rect,    //(in/out) initial expected dimensions --> cropped dimensions
	SDL_Surface *pSurface, //tile to examine
	Uint8 *pSrc)           //origin pixel for the rect being considered.
	                       //If NULL [default], the origin for the surface will be used
{
	ASSERT(pSurface);

	if (!pSrc)
		pSrc = (Uint8*)pSurface->pixels;

	const UINT wBPP = pSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);

	Uint8 *pOrigin = pSrc + PIXEL_FUDGE_FACTOR;
	Uint8 *pSeek = pOrigin;

	//Find topmost non-transparent pixel.
	const UINT origY = rect.y;
	const UINT dwRowWidth = rect.w * wBPP;
	const UINT dwRowOffset = pSurface->pitch - dwRowWidth;

	//Check rows from top.
	bool bFound = false;
	while (rect.h)
	{
		Uint8 *const pEndOfRow = pSeek + dwRowWidth;
		//Each iteration checks one pixel.
		while (pSeek != pEndOfRow)
		{
			//Stop on non-transparent pixels.
			if (pSeek[0] != TransColor[0] || pSeek[1] != TransColor[1] || pSeek[2] != TransColor[2])
			{
				bFound = true;
				break;
			}
			pSeek += wBPP;
		}

		if (bFound)
			break;

		//This row is only comprised of transparent pixels.
		++rect.y;
		--rect.h;
		pSeek += dwRowOffset;
	}
	if (!rect.h)
	{
		rect.w = 0;
		return; //nothing non-transparent in image
	}

	//Check rows from bottom.
	pSeek = pOrigin + (rect.h-1)*pSurface->pitch;
	bFound = false;
	for (;;)
	{
		ASSERT(rect.h);

		Uint8 *const pEndOfRow = pSeek + dwRowWidth;
		//Each iteration checks one pixel.
		while (pSeek != pEndOfRow)
		{
			if (pSeek[0] != TransColor[0] || pSeek[1] != TransColor[1] || pSeek[2] != TransColor[2])
			{
				bFound = true;
				break;
			}
			pSeek += wBPP;
		}

		if (bFound)
			break;

		//This row is only comprised of transparent pixels.
		--rect.h;
		pSeek += dwRowOffset;
		pSeek -= 2 * pSurface->pitch; //move up to the row before
	}
	ASSERT(rect.h); //if the image wasn't found empty above, it shouldn't be so now

	//Check columns from left.
	pOrigin += (rect.y - origY) * pSurface->pitch;
	pSeek = pOrigin;
	bFound = false;
	int wY;
	for (;;)
	{
		//Check down the left column.
		for (wY = 0; wY < rect.h; ++wY)
		{
			if (pSeek[0] != TransColor[0] || pSeek[1] != TransColor[1] || pSeek[2] != TransColor[2])
			{
				bFound = true;
				break;
			}
			pSeek += pSurface->pitch; //go down a row
		}

		if (bFound)
			break;

		//This column is only comprised of transparent pixels.
		++rect.x;
		--rect.w;

		pSeek -= rect.h * pSurface->pitch; //go to top of next column
		pSeek += wBPP;
	}

	//Check columns from right.
	pSeek = pOrigin + (rect.w-1) * wBPP;
	bFound = false;
	for (;;)
	{
		//Check down the right column.
		for (wY = 0; wY < rect.h; ++wY)
		{
			if (pSeek[0] != TransColor[0] || pSeek[1] != TransColor[1] || pSeek[2] != TransColor[2])
			{
				bFound = true;
				break;
			}
			pSeek += pSurface->pitch; //go down a row
		}

		if (bFound)
			break;

		//This column is only comprised of transparent pixels.
		--rect.w;
		pSeek -= rect.h * pSurface->pitch; //go to top of prev column
		pSeek -= wBPP;
	}
}

//**********************************************************************************
void CBitmapManager::DarkenRect(
//Set pixel intensities to % of original.
//
//Params:
	const UINT x, const UINT y,   //(in)   Pixel coords.
	const UINT w, const UINT h,   //
	const float fLightPercent,    //(in)   % of brightness to retain
	SDL_Surface *pDestSurface)    //(in)   Surface to blit to.
{
	if (fLightPercent < 0.0) return;	//do nothing for invalid range values
	if (fLightPercent >= 1.0) return;

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wPixelByteNo = y * pDestSurface->pitch + (x * wBPP);
	const UINT dwRowWidth = w * wBPP;
	const UINT dwRowOffset = pDestSurface->pitch - dwRowWidth;

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"CBitmapManager::DarkenRect: Lock surface failed.");
			return;
		}
	}

	Uint8 *pSeek = (Uint8 *)pDestSurface->pixels + wPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pSeek + (h * pDestSurface->pitch);

	if (fLightPercent == 0.0f)
	{
		//Optimized 0%.
		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			Uint8 *const pEndOfRow = pSeek + dwRowWidth;

			//Each iteration modifies one pixel.
			while (pSeek != pEndOfRow)
			{
				//Set pixel intensity to % of original.
				pSeek[0] = pSeek[1] = pSeek[2] = 0;
				pSeek += wBPP;
			}
			pSeek += dwRowOffset;
		}
	}
	else if (fLightPercent == 0.5f)
	{
		//Optimized 50%.
		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			Uint8 *const pEndOfRow = pSeek + dwRowWidth;

			//Each iteration modifies one pixel.
			while (pSeek != pEndOfRow)
			{
				//Set pixel intensity to % of original.
				pSeek[0] /= 2;
				pSeek[1] /= 2;
				pSeek[2] /= 2;
				pSeek += wBPP;
			}
			pSeek += dwRowOffset;
		}
	} else {
		const UINT dark = static_cast<UINT>(fLightPercent / g_DarkenStepIncrement);
		const Uint8 *const pDarken = g_darkenCalc[dark];
		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			const Uint8 *const pEndOfRow = pSeek + dwRowWidth;

			while (pSeek != pEndOfRow)
			{
				pSeek[0] = pDarken[pSeek[0]];
				pSeek[1] = pDarken[pSeek[1]];
				pSeek[2] = pDarken[pSeek[2]];
				pSeek += wBPP;
			}
			pSeek += dwRowOffset;
		}
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::DarkenTileWithMask(
//Set pixel intensities to faction of original, using indicated tile as a mask.
	const UINT wTIMask, const UINT wXOffset, const UINT wYOffset,	//(in) mask
	const UINT x, const UINT y, const UINT w, const UINT h,
	SDL_Surface *pDestSurface, const float fLightPercent)
{
	//Do nothing for invalid range values.
	if (fLightPercent >= 1.0)
		return;
	if (fLightPercent < 0.0)
		return;

	if (SDL_MUSTLOCK(pDestSurface))
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"DarkenTileWithMask::Lock surface failed.");
			return;
		}

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
	const UINT wPixelByteNo = y * pDestSurface->pitch + (x * wBPP);
	const UINT dwRowWidth = w * wBPP;
	const UINT dwRowOffset = pDestSurface->pitch - dwRowWidth;

	SDL_Surface *pMaskSurface = GetTileSurface(wTIMask);
	ASSERT(pMaskSurface);
	const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;
	const UINT dwMaskRowOffset = pMaskSurface->pitch - (w * wMaskBPP);

#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
	ASSERT(pMaskSurface->format->Rmask == 0xff0000);
	ASSERT(pMaskSurface->format->Gmask == 0x00ff00);
	ASSERT(pMaskSurface->format->Bmask == 0x0000ff);
#endif
	Uint8 *pSeek = (Uint8 *)pDestSurface->pixels + wPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pSeek + (h * pDestSurface->pitch);
	Uint8 *pMask = GetTileSurfacePixel(wTIMask, wXOffset, wYOffset) + PIXEL_FUDGE_FACTOR;

	const UINT dark = static_cast<UINT>(fLightPercent / g_DarkenStepIncrement);
	const Uint8 *const pDarken = g_darkenCalc[dark];
	while (pSeek != pStop)
	{
		ASSERT(pSeek < pStop);
		Uint8 *const pEndOfRow = pSeek + dwRowWidth;

		//Each iteration modifies one pixel.
		while (pSeek != pEndOfRow)
		{
			//Skip color-keyed transparent pixels on tile mask.
			if (pMask[0] != TransColor[0] || pMask[1] != TransColor[1] || pMask[2] != TransColor[2])
			{
				//Darken pixel.
				pSeek[0] = pDarken[pSeek[0]];
				pSeek[1] = pDarken[pSeek[1]];
				pSeek[2] = pDarken[pSeek[2]];
			}
			pSeek += wBPP;
			pMask += wMaskBPP;
		}
		pSeek += dwRowOffset;
		pMask += dwMaskRowOffset;
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::DarkenWithMask(
//Set pixel intensities to a fraction of the original, using indicated surface as a mask.
//When bUseColorKeyMask is set, non-transparent keyed pixels in the mask indicate
//where to darken the dest surface.  Otherwise, the mask pixels are blended
//with the dest pixels.
//
//Params:
	SDL_Surface *pMaskSurface, SDL_Rect src,
	SDL_Surface *pDestSurface, SDL_Rect dest,
	const float fLightFactor, //resultant pixel values are in range [self*fLightFactor, self]
	const bool bUseColorKeyMask) //use transparent color-keyed pixels as a mask
	                             //or blend mask pixels with dest pixels
{
	//Do nothing for invalid range values.
	if (fLightFactor >= 1.0f)
		return;
	if (fLightFactor < 0.0f)
		return;

	//Bounds clipping.
	ClipSrcAndDestToRect(src, dest, pDestSurface->w, pDestSurface->h);
	ASSERT(src.w == dest.w);
	ASSERT(src.h == dest.h);
	if (!src.w || !src.h || !dest.w || !dest.h)
		return; //not in view area -- do nothing

	if (SDL_MUSTLOCK(pDestSurface))
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"DarkenWithMask::Lock surface failed.");
			return;
		}

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
	ASSERT(pMaskSurface->format->Rmask == 0xff0000);
	ASSERT(pMaskSurface->format->Gmask == 0x00ff00);
	ASSERT(pMaskSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wMaskPixelByteNo = src.y * pMaskSurface->pitch + (src.x * wMaskBPP);
	const UINT wDestPixelByteNo = dest.y * pDestSurface->pitch + (dest.x * wBPP);
	const UINT dwRowWidth = dest.w * wBPP;
	const UINT dwMaskRowOffset = pMaskSurface->pitch - src.w * wMaskBPP;
	const UINT dwDestRowOffset = pDestSurface->pitch - dwRowWidth;

	Uint8 *pMask = (Uint8 *)pMaskSurface->pixels + wMaskPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *pSeek = (Uint8 *)pDestSurface->pixels + wDestPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pSeek + (dest.h * pDestSurface->pitch);

	if (bUseColorKeyMask)
	{
		const UINT dark = static_cast<UINT>(fLightFactor / g_DarkenStepIncrement);
		const Uint8 *const pDarken = g_darkenCalc[dark];
		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			Uint8 *const pEndOfRow = pSeek + dwRowWidth;

			//Skip color-keyed transparent pixels on tile mask.
			while (pSeek != pEndOfRow)
			{
				if (pMask[0] != TransColor[0] || pMask[1] != TransColor[1] || pMask[2] != TransColor[2])
				{
					//Darken pixel where mask is applied.
					pSeek[0] = pDarken[pSeek[0]];
					pSeek[1] = pDarken[pSeek[1]];
					pSeek[2] = pDarken[pSeek[2]];
				}

				pSeek += wBPP;
				pMask += wMaskBPP;
			}
			pSeek += dwDestRowOffset;
			pMask += dwMaskRowOffset;
		}
	} else {
		const float fOneMinusLight = (1.0f - fLightFactor) / 255.0f;
		while (pSeek != pStop)
		{
			ASSERT(pSeek < pStop);
			Uint8 *const pEndOfRow = pSeek + dwRowWidth;

			//Each iteration modifies one pixel.
			while (pSeek != pEndOfRow)
			{
				//Darken pixel to range [self*fLightFactor (when mask=0), self (when mask=255)].
				pSeek[0] = static_cast<Uint8>(pSeek[0] * (fLightFactor + fOneMinusLight * (float)pMask[0]));
				pSeek[1] = static_cast<Uint8>(pSeek[1] * (fLightFactor + fOneMinusLight * (float)pMask[1]));
				pSeek[2] = static_cast<Uint8>(pSeek[2] * (fLightFactor + fOneMinusLight * (float)pMask[2]));

				pSeek += wBPP;
				pMask += wMaskBPP;
			}
			pSeek += dwDestRowOffset;
			pMask += dwMaskRowOffset;
		}
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
UINT CBitmapManager::GetCustomTileNo(
//Searches for this tile set and, if present, returns the calculated index
//of the tile at position (X,Y) in this set
//
//Returns: tile index of tile, else TI_UNSPECIFIED if not found
//
//Params:
	const UINT wTileSet,     //(in)
	UINT wX, UINT wY,        //(in) tile row,column
	const bool bYWraparound) //(in) whether to allow Y row value to wrap around
	                         //     (useful for an animation cycle) [default=false]
{
	//Look up tile set data.
	CustomTileMap::iterator iter = this->customTiles.find(wTileSet);
	if (iter == this->customTiles.end())
		return TI_UNSPECIFIED; //tile set not present

	CustomTiles tiles = iter->second;
	SDL_Surface *pSurface = tiles.pSurface;

	//Bounds check.
	const UINT numRows = pSurface->h/CBitmapManager::CY_TILE;
	if (!numRows)
		return TI_UNSPECIFIED; //no tiles exist
	if (bYWraparound)
		wY = wY % numRows;
	else if (wY >= numRows)
		return TI_UNSPECIFIED;
	const UINT wTilesInRow = pSurface->w/CBitmapManager::CX_TILE;
	if (wX >= wTilesInRow)
		return TI_UNSPECIFIED;

	return tiles.wTileStartNo + wY*wTilesInRow + wX;
}

//**********************************************************************************
const CustomTiles* CBitmapManager::GetCustomTilesFor(const UINT wTileNo) const
//Returns: pointer to the record containing the indicated custom tile number, or NULL if invalid
{
	if (wTileNo < this->wTileCount || wTileNo >= this->wNextCustomTileNo)
		return NULL; //not in the range of loaded custom tiles

	UINT wMaxLowerTileNo = 0;
	const CustomTiles *pFoundTiles = NULL;
	for (CustomTileMap::const_iterator iter = this->customTiles.begin();
			iter != this->customTiles.end(); ++iter)
	{
		const UINT wTileSetStartNo = iter->second.wTileStartNo;
		if (wTileNo >= wTileSetStartNo && wTileSetStartNo > wMaxLowerTileNo)
		{
			wMaxLowerTileNo = wTileSetStartNo;
			pFoundTiles = &(iter->second);
		}
	}
	return pFoundTiles;
}

//**********************************************************************************
void CBitmapManager::LightenRect(
//Set pixel intensities to % of original (multiplicative light addition).
//
//Params:
	SDL_Surface *pDestSurface,    //(in)   Surface to blit to.
	const UINT x, const UINT y,   //(in)   Pixel coords.
	const UINT w, const UINT h,   //
	const float R, const float G,	const float B)  //(in) % brightness to add
{
	ASSERT(R >= 1.0 && R <= 255.0);
	ASSERT(G >= 1.0 && G <= 255.0);
	ASSERT(B >= 1.0 && B <= 255.0);
	if (R == 1.0 && G == 1.0 && B == 1.0) return;

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"LightenRect::Lock surface failed.");
			return;
		}
	}

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wPixelByteNo = y * pDestSurface->pitch + (x * wBPP);
	const UINT dwRowOffset = pDestSurface->pitch - (w * wBPP);

	//Get color index.
	const UINT wR = pDestSurface->format->Rshift / 8;
	const UINT wG = pDestSurface->format->Gshift / 8;
	const UINT wB = pDestSurface->format->Bshift / 8;

	Uint8 *pSeek = (Uint8 *)pDestSurface->pixels + wPixelByteNo;
	Uint8 *const pStop = pSeek + (h * pDestSurface->pitch);
	UINT wVal;

	while (pSeek != pStop)
	{
		ASSERT(pSeek < pStop);
		Uint8 *const pEndOfRow = pSeek + (w * wBPP);

		//Each iteration modifies one pixel.
		while (pSeek != pEndOfRow)
		{
			//Add to pixel intensity.  Cap at 255.
			wVal = (UINT)(pSeek[wR] * R);
			pSeek[wR] = (unsigned char)(wVal < 255 ? wVal : 255);
			wVal = (UINT)(pSeek[wG] * G);
			pSeek[wG] = (unsigned char)(wVal < 255 ? wVal : 255);
			wVal = (UINT)(pSeek[wB] * B);
			pSeek[wB] = (unsigned char)(wVal < 255 ? wVal : 255);
			pSeek += wBPP;
		}
		pSeek += dwRowOffset;
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::LightenRect(
//Set pixel intensities to % of original (multiplicative light addition).
//
//Params:
	SDL_Surface *pDestSurface,    //(in)   Surface to blit to.
	const UINT x, const UINT y,   //(in)   Pixel coords.
	const UINT w, const UINT h,   //
	const UINT* R, const UINT* G,	const UINT* B,  //(in) 4 brightnesses to interpolate across rectangle [0-65535]
	const UINT wTIMask,         //(in) optional tile mask [default=TI_UNSPECIFIED]
	const UINT wXOffset, const UINT wYOffset) //(in) offset into mask [default=(0,0)]
{
	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"LightenRect::Lock surface failed.");
			return;
		}
	}

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
	const UINT wPixelByteNo = y * pDestSurface->pitch + (x * wBPP);
	const UINT dwRowWidth = w * wBPP;
	const UINT dwRowOffset = pDestSurface->pitch - dwRowWidth;

	//Get color index.
	const UINT wR = pDestSurface->format->Rshift / 8;
	const UINT wG = pDestSurface->format->Gshift / 8;
	const UINT wB = pDestSurface->format->Bshift / 8;

	//Bi-linear interpolation.
	int rgbInterp[6][3];   //scratch pad for interpolating values
	rgbInterp[0][0] = (int)R[2]-(int)R[0];
	rgbInterp[0][1] = (int)G[2]-(int)G[0];
	rgbInterp[0][2] = (int)B[2]-(int)B[0];
	rgbInterp[1][0] = (int)R[3]-(int)R[1];
	rgbInterp[1][1] = (int)G[3]-(int)G[1];
	rgbInterp[1][2] = (int)B[3]-(int)B[1];

	const float wOneOverW = 1.0f/(float)w; //optimization
	const float wOneOverH = 1.0f/(float)h;
	float dx, dxInitial = 0.5f*wOneOverW, dy = 0.5f*wOneOverH;

	UINT wVal;
	Uint8 *pMask = NULL;
	UINT dwMaskRowOffset = 0;
	UINT wMaskBPP = 0;
	if (wTIMask != TI_UNSPECIFIED)
	{
		SDL_Surface *pMaskSurface = GetTileSurface(wTIMask);
		ASSERT(pMaskSurface);
		wMaskBPP = pMaskSurface->format->BytesPerPixel;
		pMask = GetTileSurfacePixel(wTIMask, wXOffset, wYOffset) + PIXEL_FUDGE_FACTOR;
		dwMaskRowOffset = pMaskSurface->pitch - w*wMaskBPP;
	}
	Uint8 *pSeek = (Uint8 *)pDestSurface->pixels + wPixelByteNo;
	for (UINT j=0; j<h; ++j)
	{
		Uint8 *const pEndOfRow = pSeek + dwRowWidth;

		//Interpolate along y.
		rgbInterp[2][0] = (int)R[0] + (int)(rgbInterp[0][0]*dy);
		rgbInterp[2][1] = (int)G[0] + (int)(rgbInterp[0][1]*dy);
		rgbInterp[2][2] = (int)B[0] + (int)(rgbInterp[0][2]*dy);
		rgbInterp[3][0] = (int)R[1] + (int)(rgbInterp[1][0]*dy);
		rgbInterp[3][1] = (int)G[1] + (int)(rgbInterp[1][1]*dy);
		rgbInterp[3][2] = (int)B[1] + (int)(rgbInterp[1][2]*dy);

		//Prepare to interpolate along x.
		rgbInterp[4][0] = rgbInterp[3][0]-rgbInterp[2][0];
		rgbInterp[4][1] = rgbInterp[3][1]-rgbInterp[2][1];
		rgbInterp[4][2] = rgbInterp[3][2]-rgbInterp[2][2];

		//Each iteration modifies one pixel.
		dx = dxInitial;
		if (pMask)
		{
			//Use tile mask.
			while (pSeek != pEndOfRow)
			{
				//Skip color-keyed transparent pixels on tile mask.
				if (pMask[0] != TransColor[0] || pMask[1] != TransColor[1] || pMask[2] != TransColor[2])
				{
					//Interpolate along x.
					rgbInterp[5][0] = rgbInterp[2][0] + (int)(rgbInterp[4][0]*dx);
					rgbInterp[5][1] = rgbInterp[2][1] + (int)(rgbInterp[4][1]*dx);
					rgbInterp[5][2] = rgbInterp[2][2] + (int)(rgbInterp[4][2]*dx);

					//Multiply with pixel intensity.  Cap at 255.
					//The color values passed in are represented in fixed point,
					//with 256 equaling a brightness of 1.
					//Dividing by 256 is performed as a speed optimization.
					wVal = (UINT)(pSeek[wR]) * rgbInterp[5][0]/256;
					pSeek[wR] = (Uint8)(wVal < 255 ? wVal : 255);
					wVal = (UINT)(pSeek[wG]) * rgbInterp[5][1]/256;
					pSeek[wG] = (Uint8)(wVal < 255 ? wVal : 255);
					wVal = (UINT)(pSeek[wB]) * rgbInterp[5][2]/256;
					pSeek[wB] = (Uint8)(wVal < 255 ? wVal : 255);
				}
				pSeek += wBPP;
				pMask += wMaskBPP;
				dx += wOneOverW;
			}
			pMask += dwMaskRowOffset;
		} else {
			while (pSeek != pEndOfRow)
			{
				//Interpolate along x.
				rgbInterp[5][0] = rgbInterp[2][0] + (int)(rgbInterp[4][0]*dx);
				rgbInterp[5][1] = rgbInterp[2][1] + (int)(rgbInterp[4][1]*dx);
				rgbInterp[5][2] = rgbInterp[2][2] + (int)(rgbInterp[4][2]*dx);

				//Multiply with pixel intensity.  Cap at 255.
				//The color values passed in are represented in fixed point,
				//with 256 equaling a brightness of 1.
				//Dividing by 256 is performed as a speed optimization.
				wVal = (UINT)(pSeek[wR]) * rgbInterp[5][0]/256;
				pSeek[wR] = (Uint8)(wVal < 255 ? wVal : 255);
				wVal = (UINT)(pSeek[wG]) * rgbInterp[5][1]/256;
				pSeek[wG] = (Uint8)(wVal < 255 ? wVal : 255);
				wVal = (UINT)(pSeek[wB]) * rgbInterp[5][2]/256;
				pSeek[wB] = (Uint8)(wVal < 255 ? wVal : 255);

				pSeek += wBPP;
				dx += wOneOverW;
			}
		}
		pSeek += dwRowOffset;
		dy += wOneOverH;
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::LightenRectWithTileMask(
//Set pixel intensities to % of original (multiplicative light addition).
//Use pixel mask.
//
//Params:
	SDL_Surface *pDestSurface,    //(in)   Surface to blit to.
	const UINT x, const UINT y,   //(in)   Pixel coords.
	const UINT w, const UINT h,   //
	const float R, const float G,	const float B,  //(in) % brightness to add
	const UINT wTIMask, const UINT wXOffset, const UINT wYOffset)	//(in) mask
{
	ASSERT(w <= CX_TILE);
	ASSERT(h <= CY_TILE);

	ASSERT(R >= 0.0 && R <= 255.0);
	ASSERT(G >= 0.0 && G <= 255.0);
	ASSERT(B >= 0.0 && B <= 255.0);
	if (R == 1.0 && G == 1.0 && B == 1.0) return;

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"LightenRectWithTileMask::Lock surface failed.");
			return;
		}
	}

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
	const UINT wPixelByteNo = y * pDestSurface->pitch + (x * wBPP);
	const UINT dwRowWidth = w * wBPP;
	const UINT dwRowOffset = pDestSurface->pitch - dwRowWidth;

	//Start at (wXOffset, wYOffset) on mask.
	SDL_Surface *pMaskSurface = GetTileSurface(wTIMask);
	ASSERT(pMaskSurface);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pMaskSurface->format->Rmask == 0xff0000);
	ASSERT(pMaskSurface->format->Gmask == 0x00ff00);
	ASSERT(pMaskSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wMaskBPP = pMaskSurface->format->BytesPerPixel;
	Uint8 *pMask = GetTileSurfacePixel(wTIMask, wXOffset, wYOffset) + PIXEL_FUDGE_FACTOR;
	const UINT dwMaskRowOffset = pMaskSurface->pitch - (w * wMaskBPP);

	//Get color index.
	const UINT wR = pDestSurface->format->Rshift / 8;
	const UINT wG = pDestSurface->format->Gshift / 8;
	const UINT wB = pDestSurface->format->Bshift / 8;

	Uint8 *pSeek = (Uint8 *)pDestSurface->pixels + wPixelByteNo;
	Uint8 *const pStop = pSeek + (h * pDestSurface->pitch);
	UINT wVal;

	while (pSeek != pStop)
	{
		ASSERT(pSeek < pStop);
		Uint8 *const pEndOfRow = pSeek + dwRowWidth;

		//Each iteration modifies one pixel.
		while (pSeek != pEndOfRow)
		{
			//Skip color-keyed transparent pixels on tile mask.
			if (pMask[0] != TransColor[0] || pMask[1] != TransColor[1] || pMask[2] != TransColor[2])
			{
				//Add to pixel intensity.  Cap at 255.
				wVal = (UINT)(pSeek[wR] * R);
				pSeek[wR] = (unsigned char)(wVal < 255 ? wVal : 255);
				wVal = (UINT)(pSeek[wG] * G);
				pSeek[wG] = (unsigned char)(wVal < 255 ? wVal : 255);
				wVal = (UINT)(pSeek[wB] * B);
				pSeek[wB] = (unsigned char)(wVal < 255 ? wVal : 255);
			}
			pSeek += wBPP;
			pMask += wMaskBPP;
		}
		pSeek += dwRowOffset;
		pMask += dwMaskRowOffset;
	}

	if (SDL_MUSTLOCK(pDestSurface))
		SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
void CBitmapManager::ShadeRect(
//Adds a color shade to tile at (x,y) on surface.
//
//Params:
	const UINT x, const UINT y,   //(in)   Pixel coords.
	const UINT w, const UINT h,   //
	const SURFACECOLOR &ShadeColor,    //(in)   Color to shade tile with.
	SDL_Surface *pDestSurface)    //(in)   Surface to blit to.
{
	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP >= 3);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif
#ifdef GAME_RENDERING_OFFSET
	SURFACECOLOR Color(ShadeColor);
	const int temp = Color.byt3;
	Color.byt3 = Color.byt1;
	Color.byt1 = temp;
#else
	const SURFACECOLOR& Color = ShadeColor;
#endif
	const UINT wPixelByteNo = y * pDestSurface->pitch + (x * wBPP);
	const UINT dwRowOffset = pDestSurface->pitch - (w * wBPP);

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"Lock surface failed.(4)");
			return;
		}
	}

	Uint8 *pSeek = (Uint8 *)pDestSurface->pixels + wPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pSeek + (h * pDestSurface->pitch);

	UINT nHue;
	while (pSeek != pStop)
	{
		ASSERT(pSeek < pStop);
		Uint8 *const pEndOfRow = pSeek + (w * wBPP);

		//Each iteration modifies one pixel.
		while (pSeek != pEndOfRow)
		{
			//Weighted average of current and mixing color (1:1).
			nHue = pSeek[0];
			nHue += Color.byt3;  //big endian order
			pSeek[0] = nHue/2;
			nHue = pSeek[1];
			nHue += Color.byt2;
			pSeek[1] = nHue/2;
			nHue = pSeek[2];
			nHue += Color.byt1;
			pSeek[2] = nHue/2;

			pSeek += wBPP;
		}
		pSeek += dwRowOffset;
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
bool CBitmapManager::SetGamma(const float fGamma)
//Returns: true if operation succeeded
{
#ifdef __APPLE__
	//Gamma setting causes display issues on OSX
	return true;
#else
	//XXX elsewhere too
	(void)fGamma;
	return true;
#endif
}

//**********************************************************************************
void CBitmapManager::SetSurfaceColorKey(const UINT wTileImageNo, const UINT wSurfaceIndex, SDL_Surface *pTileSurface)
//Enables or disables a surface's transparent color key.
{
	ASSERT(this->TileImageTypes[wTileImageNo] != TIT_Unspecified); //Unloaded tile image.
	if (this->TileImageTypes[wTileImageNo] == TIT_Transparent)
	{
		if (!this->bIsColorKeySet[wSurfaceIndex])
		{
			static const Uint32 TranspColor = SDL_MapRGB(pTileSurface->format,
					TransColor[0], TransColor[1], TransColor[2]);

			SetColorKey(pTileSurface, SDL_TRUE, TranspColor);
			this->bIsColorKeySet[wSurfaceIndex] = true;
		}
	}
	else if (this->TileImageTypes[wTileImageNo] == TIT_Opaque && this->bIsColorKeySet[wSurfaceIndex])
	{
		SetColorKey(pTileSurface, 0, 0);
		this->bIsColorKeySet[wSurfaceIndex] = false;
	}
}

//**********************************************************************************
void CBitmapManager::ShadeWithSurfaceMask(
//Use a (non-tiled) source surface as a variable shadow mask, selectively applied
//where shadow mask tiles are NOT present.
//When there is a partial (soft) shadow mask, this mask is applied to the degree
//that the other one was not so that more than full shadowing (i.e. 75% light)
//is never exceeded.
//
//Params:
	SDL_Surface *pSrcSurface, const int srcX, const int srcY,
	SDL_Surface *pDestSurface, const int destX, const int destY,
	const UINT* pwShadowTIs, const UINT wNumShadows)
{
	ASSERT(pSrcSurface);
	ASSERT(pDestSurface);

	if (SDL_MUSTLOCK(pDestSurface))
		if (SDL_LockSurface(pDestSurface) < 0)
			throw CException("CBitmapManager::LightenWithSurfaceMask -- Lock surface failed.");

	const UINT wSrcBPP = pSrcSurface->format->BytesPerPixel;
	const UINT wSrcPixelByteNo = srcY * pSrcSurface->pitch + (srcX * wSrcBPP);
	const UINT dwSrcRowOffset = pSrcSurface->pitch - (CX_TILE * wSrcBPP);
	const UINT wShadowBPP = this->pTileSurfaces[0]->format->BytesPerPixel;
	const UINT dwShadowRowOffset = this->pTileSurfaces[0]->pitch - (CX_TILE * wShadowBPP);
	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
	ASSERT(pSrcSurface->format->Rmask == 0xff0000);
	ASSERT(pSrcSurface->format->Gmask == 0x00ff00);
	ASSERT(pSrcSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wDestPixelByteNo = destY * pDestSurface->pitch + (destX * wBPP);
	const UINT dwDestRowOffset = pDestSurface->pitch - (CX_TILE * wBPP);

	//Find shadow tile masks.
	static const UINT wMaxShadows = 10;
	static Uint8 *pShadow[wMaxShadows];
	ASSERT(wNumShadows < wMaxShadows);
	UINT wIndex;
	for (wIndex=wNumShadows; wIndex--; )
	{
		ASSERT(pwShadowTIs[wIndex] < this->wTileCount); //not custom tiles
		pShadow[wIndex] = GetTileSurfacePixel(pwShadowTIs[wIndex]) + PIXEL_FUDGE_FACTOR;
	}

	//Place shadow (percent based on grayscale src image) on pixels not already shaded by tile shadow masks.
	Uint8 *pSrc = (Uint8 *)pSrcSurface->pixels + wSrcPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *pDest = (Uint8 *)pDestSurface->pixels + wDestPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pDest + (CY_TILE * pDestSurface->pitch);
	while (pDest != pStop)
	{
		ASSERT(pDest < pStop);
		Uint8 *const pEndOfRow = pDest + (CX_TILE * wBPP);

		switch (wNumShadows)
		{
			case 0:	//No shadow masks.
				while (pDest != pEndOfRow)
				{
					//Each iteration modifies one pixel.

					//Darken by amount of shadow source image.
					const Uint8* const shadowSrc = g_shadowCalc[pSrc[0]]; //examine only first color value
					pDest[0] = shadowSrc[pDest[0]];
					pDest[1] = shadowSrc[pDest[1]];
					pDest[2] = shadowSrc[pDest[2]];

					pSrc += wSrcBPP;
					pDest += wBPP;
				}
			break;
			case 1:
				//A simple implementation for handling only one shadow mask.
				while (pDest != pEndOfRow)
				{
					//Examing the shadowing already on this pixel.
					//Don't allow more than the maximum amount.
					//(This may look weird, but the math is right.)
					const int val = (pSrc[0] + *pShadow[0] < 255) ? *pShadow[0] : 255 - pSrc[0];
					ASSERT(val >= 0);

					if (val > 0)
					{
						const Uint8* const shadowSrc = g_shadowCalcBlend[val];
						pDest[0] = shadowSrc[pDest[0]];
						pDest[1] = shadowSrc[pDest[1]];
						pDest[2] = shadowSrc[pDest[2]];
					}
					pSrc += wSrcBPP;
					pDest += wBPP;
					pShadow[0] += wShadowBPP;
				}
				pShadow[0] += dwShadowRowOffset;
			break;
			default:
				//When combining multiple shadow masks.
				while (pDest != pEndOfRow)
				{
					//Sum the amount of shadowing already on this pixel.
					UINT sum = 0;
					for (wIndex=0; wIndex<wNumShadows; ++wIndex)
					{
						switch (*pShadow[wIndex]) //examine only first color value
						{
							case 0: //full shadow
								sum = 255;
								wIndex = wNumShadows; //end loop now
							break;
							case 255: //no shadow
							break;
							default: //partial shadow
								sum += 255 - *pShadow[wIndex];
							break;
						}
					}
					if (sum > 255)
						sum = 255;
					sum = 255 - sum;

					//Don't allow more than the maximum amount of shadow.
					const int val = (pSrc[0] + sum < 255) ? sum : 255 - pSrc[0];
					ASSERT(val >= 0);

					if (val > 0)
					{
						const Uint8* const shadowSrc = g_shadowCalcBlend[val];
						pDest[0] = shadowSrc[pDest[0]];
						pDest[1] = shadowSrc[pDest[1]];
						pDest[2] = shadowSrc[pDest[2]];
					}

					pSrc += wSrcBPP;
					pDest += wBPP;
					for (wIndex=wNumShadows; wIndex--; )
						pShadow[wIndex] += wShadowBPP;
				}
				for (wIndex=wNumShadows; wIndex--; )
					pShadow[wIndex] += dwShadowRowOffset;
			break;
		}
		pSrc += dwSrcRowOffset;
		pDest += dwDestRowOffset;
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//**********************************************************************************
SDL_Surface *pTempSurface = NULL;
void CBitmapManager::ShadeWithWrappingSurfaceMask(
//Wrapper to use a wrappable source surface as a grayscale light mask, selectively
//applied where no shadow pixels are present.
//If the source rect exceeds the source surface bounds, the source image is wrapped.
//NOTE: this only wraps once and does not tile an image repeatedly onto a destination.
//(Copied from BlitWrappingSurface.)
//
//Params:
	SDL_Surface *pSrcSurface, SDL_Rect src,
	SDL_Surface *pDestSurface, SDL_Rect dest,
	const UINT* pwShadowTIs, const UINT wNumShadows)
{
	ASSERT(pSrcSurface);
	ASSERT(pDestSurface);
	ASSERT(!pSrcSurface->format->Amask);	//!!wrapping doesn't work w/ alpha layer

	if (!src.w || !src.h || !dest.w || !dest.h) return;	//Nothing to blit.

	if (src.w > pSrcSurface->w) src.w = pSrcSurface->w;	//Size checks.
	if (src.h > pSrcSurface->h) src.h = pSrcSurface->h;

	//Start blitting from within the source image bounds.
	while (src.x < 0) src.x += pSrcSurface->w;
	while (src.y < 0) src.y += pSrcSurface->h;
	src.x = src.x % pSrcSurface->w;
	src.y = src.y % pSrcSurface->h;

	//Determine how much blit would go past source surface edge.
	const int dx = (src.x + src.w) - pSrcSurface->w;
	const int dy = (src.y + src.h) - pSrcSurface->h;

	if (dx <= 0 && dy <= 0)
	{
		//No wrapping.
		ShadeWithSurfaceMask(pSrcSurface, src.x, src.y,
				pDestSurface, dest.x, dest.y, pwShadowTIs, wNumShadows);
	} else {
		//Wrapping required.  Create a temporary surface from the wrapped image
		//and use it to as the mask.
		//Most blits will be of the same size, so retain this surface between calls for speed.
		if (pTempSurface && (pTempSurface->w < dest.w || pTempSurface->h < dest.h))
		{
			SDL_FreeSurface(pTempSurface);
			pTempSurface = NULL;
		}
		if (!pTempSurface)
		{
			pTempSurface = ConvertSurface(SDL_CreateRGBSurface(SDL_SWSURFACE,
				dest.w, dest.h, CBitmapManager::BITS_PER_PIXEL, 0, 0, 0, 0));
			if (!pTempSurface)
				throw CException("Couldn't allocate memory in LightenWithWrappingSurfaceMask");
		}

		SDL_Rect temp = {0, 0, dest.w, dest.h};
		if (dx > 0)
		{
			if (dy <= 0)
			{
				//Wrap in x direction.
				SDL_BlitSurface(pSrcSurface, &src, pTempSurface, &temp);
				src.w = dx;		//continue blitting this much from beginning of source
				temp.x = pSrcSurface->w - src.x;
				src.x = 0;
				SDL_BlitSurface(pSrcSurface, &src, pTempSurface, &temp);
			} else {
				//Wrap source image in x/y directions.
				const Uint16 wOldW = src.w;
				const Sint16 nOldSX = src.x;
				const Sint16 nOldDX = temp.x;
				SDL_BlitSurface(pSrcSurface, &src, pTempSurface, &temp);
				src.w = dx;
				temp.x = pSrcSurface->w - src.x;
				src.x = 0;
				SDL_BlitSurface(pSrcSurface, &src, pTempSurface, &temp);
				src.h = dy;
				temp.y = pSrcSurface->h - src.y;
				src.y = 0;
				SDL_BlitSurface(pSrcSurface, &src, pTempSurface, &temp);
				src.x = nOldSX;
				temp.x = nOldDX;
				src.w = wOldW;
				SDL_BlitSurface(pSrcSurface, &src, pTempSurface, &temp);
			}
		} else {
			//Wrap in y direction.
			ASSERT(dy > 0);
			SDL_BlitSurface(pSrcSurface, &src, pTempSurface, &temp);
			src.h = dy;		//continue blitting this much from beginning of source
			temp.y = pSrcSurface->h - src.y;
			src.y = 0;
			SDL_BlitSurface(pSrcSurface, &src, pTempSurface, &temp);
		}
		ShadeWithSurfaceMask(pTempSurface, 0, 0, pDestSurface, dest.x, dest.y,
				pwShadowTIs, wNumShadows);
	}
}

//**********************************************************************************
void CBitmapManager::LockTileImagesSurface(const UINT wIndex)
{
	if (SDL_MUSTLOCK(this->pTileSurfaces[wIndex]))
	{
		if (SDL_LockSurface(this->pTileSurfaces[wIndex]) < 0)
			ASSERT(!"LockTileImagesSurface(): Lock surface failed.");
	}
}

//**********************************************************************************
void CBitmapManager::UnlockTileImagesSurface(const UINT wIndex)
{
	if (SDL_MUSTLOCK(this->pTileSurfaces[wIndex]))
		SDL_UnlockSurface(this->pTileSurfaces[wIndex]);
}

//**********************************************************************************
SDL_Surface * CBitmapManager::GetBitmapSurface(
//Gets a bitmap surface.  If bitmap is already loaded, increments a reference count.
//If bitmap isn't already loaded, loads the bitmap.
//
//Note: For every successful call to GetBitmapSurface(), a matching call to
//ReleaseBitmapSurface() must be made, or resources will not be freed and an
//assertion will fire in the destructor.
//
//Params:
	const char *pszName) //(in)   Name of the bitmap, not including extension.
//
//Returns:
//Loaded surface if successful, or NULL if not.
{
	ASSERT(strlen(pszName) <= MAXLEN_BITMAPNAME);

	WSTRING wstr;
	UTF8ToUnicode(pszName, wstr);
	LOADEDBITMAP *pBitmap = FindLoadedBitmap(wstr.c_str());
	if (pBitmap)   //Found it--increment ref count.
	{
		ASSERT(pBitmap->dwRefCount);
		ASSERT(pBitmap->pSurface);

		++pBitmap->dwRefCount;
	}
	else        //Didn't find it--load from file.
	{
		pBitmap = new LOADEDBITMAP;
		pBitmap->pSurface = LoadImageSurface(wstr.c_str());
		if (!pBitmap->pSurface)
		{
			//Couldn't load surface from file.
			delete pBitmap;
			return NULL;
		}
		WCScpy(pBitmap->wszName, wstr.c_str());
		pBitmap->dwRefCount = 1;
		this->LoadedBitmaps.push_back(pBitmap);
	}

	//Success.
	return pBitmap->pSurface;
}

//**********************************************************************************
void CBitmapManager::ReleaseBitmapSurface(
//Releases one reference count of a bitmap, and unloads the bitmap if nobody is
//using it anymore (ref count==0).
//
//Params:
	const char *pszName) //(in)   Name of the bitmap, not including extension.
{
	ASSERT(strlen(pszName) <= MAXLEN_BITMAPNAME);

	WSTRING wstr;
	UTF8ToUnicode(pszName, wstr);
	LOADEDBITMAP *pBitmap = FindLoadedBitmap(wstr.c_str());

	ASSERT(pBitmap);
	ASSERT(pBitmap->dwRefCount > 0);

	//Decrement ref count.
	if (--(pBitmap->dwRefCount)==0)
	{
		//Unload the bitmap.
		SDL_FreeSurface(pBitmap->pSurface);
		this->LoadedBitmaps.remove(pBitmap);
		delete pBitmap;
	}
}

//**********************************************************************************
SDL_Surface* CBitmapManager::Rotate90deg(
//Returns: a new surface, which is a source surface rotated in 90 degree increments.
//
//Params:
	SDL_Surface *pSrcSurface,
	Uint8 *pSrcOrigin,
	const int srcWidth, const int srcHeight,
	const int angle)
{
	int numTurns = (angle / 90) % 4; //get number of turns closest matching destination angle
	while (numTurns<0)
		numTurns += 4;

	int destWidth, destHeight;
	if (!(numTurns % 2))
	{
		destWidth = srcWidth;
		destHeight = srcHeight;
	} else {
		destWidth = srcHeight;
		destHeight = srcWidth;
	}

	SDL_Surface* pDestSurface = CreateNewSurfaceLike(pSrcSurface, destWidth, destHeight);
	if (!pDestSurface)
		return NULL;

	if (SDL_MUSTLOCK(pDestSurface))
		SDL_LockSurface(pDestSurface);

	char *pSrcRow = (char*)pSrcOrigin;
	char *pDstRow = (char*)pDestSurface->pixels;
	int srcStepX, dstStepX;
	srcStepX = dstStepX = pSrcSurface->format->BytesPerPixel;
	int srcStepY = pSrcSurface->pitch;
	int dstStepY = pDestSurface->pitch;

	switch (numTurns)
	{
		//case 0: we don't need to change anything
		case 1:
			pSrcRow += ((srcWidth-1)*srcStepX);
			srcStepY = -srcStepX;
			srcStepX = pSrcSurface->pitch;
		break;
		case 2:
			pSrcRow += ((srcHeight-1)*srcStepY) + ((srcWidth-1)*srcStepX);
			srcStepX = -srcStepX;
			srcStepY = -srcStepY;
		break;
		case 3:
			pSrcRow += ((srcHeight-1)*srcStepY);
			srcStepX = -srcStepY;
			srcStepY = pSrcSurface->format->BytesPerPixel;
		break;
	}

	char *pSrc, *pDst;
	int nX, nY;
	switch (pSrcSurface->format->BytesPerPixel)
	{
		case 1:
		for (nY=destHeight; nY--; )
		{
			pSrc = pSrcRow;
			pDst = pDstRow;
			for (nX=destWidth; nX--; )
			{
				*pDst = *pSrc;
				pSrc += srcStepX;
				pDst += dstStepX;
			}
			pSrcRow += srcStepY;
			pDstRow += dstStepY;
		}
		break;
		case 2:
		for (nY=destHeight; nY--; )
		{
			pSrc = pSrcRow;
			pDst = pDstRow;
			for (nX=destWidth; nX--; )
			{
				*(Uint16*)pDst = *(Uint16*)pSrc;
				pSrc += srcStepX;
				pDst += dstStepX;
			}
			pSrcRow += srcStepY;
			pDstRow += dstStepY;
		}
		break;
		case 3:
		for (nY=destHeight; nY--; )
		{
			pSrc = pSrcRow;
			pDst = pDstRow;
			for (nX=destWidth; nX--; )
			{
				pDst[0] = pSrc[0]; pDst[1] = pSrc[1]; pDst[2] = pSrc[2];
				pSrc += srcStepX;
				pDst += dstStepX;
			}
			pSrcRow += srcStepY;
			pDstRow += dstStepY;
		}
		break;
		case 4:
		for (nY=destHeight; nY--; )
		{
			pSrc = pSrcRow;
			pDst = pDstRow;
			for (nX=destWidth; nX--; )
			{
				*(Uint32*)pDst = *(Uint32*)pSrc;
				pSrc += srcStepX;
				pDst += dstStepX;
			}
			pSrcRow += srcStepY;
			pDstRow += dstStepY;
		}
		break;
	}

	if (SDL_MUSTLOCK(pDestSurface))
		SDL_UnlockSurface(pDestSurface);

	return pDestSurface;
}

//**********************************************************************************
void CBitmapManager::RotateSimple(
//Execute a simple and fast rotation algorithm.
//No interpolation is involved, so we don't need to consider blending
//color-keyed transparent pixels with opaque pixels.
//
//Params:
	SDL_Surface *pSrcSurface,
	Uint8 *pSrcOrigin,
	const int srcWidth, const int srcHeight,
	SDL_Surface *pDestSurface,
	const Uint32 bgColor,
	const double sinAngle, const double cosAngle)
{
	Uint8 *pSrc = (Uint8*)pSrcOrigin;
	Uint8 *pDstRow = (Uint8*)pDestSurface->pixels;
	const int srcPitch = pSrcSurface->pitch;
	const int dstPitch = pDestSurface->pitch;

	const int cy = pDestSurface->h / 2;
	const int xd = (UINT(srcWidth - pDestSurface->w) << 15);
	const int yd = (UINT(srcHeight - pDestSurface->h) << 15);

	const int iSin = (int)(sinAngle*65536);
	const int iCos = (int)(cosAngle*65536);

	const int ax = ((pDestSurface->w) << 15) - (int)(cosAngle * ((pDestSurface->w-1) << 15));
	const int ay = ((pDestSurface->h) << 15) - (int)(sinAngle * ((pDestSurface->w-1) << 15));

	const int ax_plus_xd = ax + xd;
	const int ay_plus_yd = ay + yd;

	const int xMaxVal = (srcWidth << 16) - 1;
	const int yMaxVal = (srcHeight << 16) - 1;

	const int width = pDestSurface->w;
	const int height = pDestSurface->h;
	int y_center_offset = cy;

	int x, y, dx, dy;
	switch (pSrcSurface->format->BytesPerPixel)
	{
		case 1:
			for (y = 0; y < height; ++y, --y_center_offset)
			{
				Uint8 *pDstPos = (Uint8*)pDstRow;
				dx = ax_plus_xd + (iSin * y_center_offset);
				dy = ay_plus_yd - (iCos * y_center_offset);
				for (x = 0; x < width; ++x)
				{
					if (dx<0 || dy<0 || dx>xMaxVal || dy>yMaxVal)
						*pDstPos++ = bgColor;
					else
						*pDstPos++ = *(Uint8*)(pSrc + ((dy>>16) * srcPitch) + (dx>>16));
					dx += iCos;
					dy += iSin;
				}
				pDstRow += dstPitch;
			}
		break;
		case 2:
			for (y = 0; y < height; ++y, --y_center_offset)
			{
				Uint16 *pDstPos = (Uint16*)pDstRow;
				dx = ax_plus_xd + (iSin * y_center_offset);
				dy = ay_plus_yd - (iCos * y_center_offset);
				for (x = 0; x < width; ++x)
				{
					if (dx<0 || dy<0 || dx>xMaxVal || dy>yMaxVal)
						*pDstPos++ = bgColor;
					else
						*pDstPos++ = *(Uint16*)(pSrc + ((dy>>16) * srcPitch) + (dx>>16<<1));
					dx += iCos;
					dy += iSin;
				}
				pDstRow += dstPitch;
			}
		break;
		case 4:
			for (y = 0; y < height; ++y, --y_center_offset)
			{
				Uint32 *pDstPos = (Uint32*)pDstRow;
				dx = ax_plus_xd + (iSin * y_center_offset);
				dy = ay_plus_yd - (iCos * y_center_offset);
				for (x = 0; x < width; ++x)
				{
					if (dx<0 || dy<0 || dx>xMaxVal || dy>yMaxVal)
						*pDstPos++ = bgColor;
					else
						*pDstPos++ = *(Uint32*)(pSrc + ((dy>>16) * srcPitch) + (dx>>16<<2));
					dx += iCos;
					dy += iSin;
				}
				pDstRow += dstPitch;
			}
		break;
		case 3:
		default:
			for (y = 0; y < height; ++y, --y_center_offset)
			{
				Uint8 *pDstPos = (Uint8*)pDstRow;
				dx = ax_plus_xd + (iSin * y_center_offset);
				dy = ay_plus_yd - (iCos * y_center_offset);
				for (x = 0; x < width; ++x)
				{
					if (dx<0 || dy<0 || dx>xMaxVal || dy>yMaxVal)
					{
						pDstPos[0] = ((Uint8*)&bgColor)[0];
						pDstPos[1] = ((Uint8*)&bgColor)[1];
						pDstPos[2] = ((Uint8*)&bgColor)[2];
					} else {
						Uint8* srcpos = (Uint8*)(pSrc + ((dy>>16) * srcPitch) + ((dx>>16) * 3));
						pDstPos[0] = srcpos[0];
						pDstPos[1] = srcpos[1];
						pDstPos[2] = srcpos[2];
					}
					pDstPos += 3;
					dx += iCos;
					dy += iSin;
				}
				pDstRow += dstPitch;
			}
		break;
	}
}

//*****************************************************************************
SDL_Surface* CBitmapManager::RotateSurface(
//Rotate the source surface by the specified amount.
//
//Returns: a new rotated surface
//
//Params:
	SDL_Surface *pSrcSurface, //surface to rotate
	Uint8 *pSrcOrigin,        //origin pixel for operation
	                          //If not NULL, it must point to a completely contained
									  //interior location of the src surface pixel buffer.
	const int width,          //dimensions of area to rotate
	const int height,
	const float fAngle)       //degrees
{
	ASSERT(pSrcSurface);
	ASSERT(pSrcSurface->format->BytesPerPixel > 0 && pSrcSurface->format->BytesPerPixel <= 4);

	if (!pSrcOrigin)
		pSrcOrigin = (Uint8*)pSrcSurface->pixels;

	if (!(((int)fAngle)%90))
		return Rotate90deg(pSrcSurface, pSrcOrigin, width, height, (int)fAngle);

	const double radAngle = fAngle*.01745329251994329; //degrees -> radians
	const double sinAngle = sin(radAngle);
	const double cosAngle = cos(radAngle);

	//Determine dimensions of rotated area.
	double x, y, cx, cy, sx, sy;
	x = width;
	y = height;
	cx = cosAngle*x;
	cy = cosAngle*y;
	sx = sinAngle*x;
	sy = sinAngle*y;
	const int nxmax = (int)(max(max(max(fabs(cx+sy), fabs(cx-sy)), fabs(-cx+sy)), fabs(-cx-sy)));
	const int nymax = (int)(max(max(max(fabs(sx+cy), fabs(sx-cy)), fabs(-sx+cy)), fabs(-sx-cy)));

	SDL_Surface* pDestSurface = CreateNewSurfaceLike(pSrcSurface, nxmax, nymax);
	if (!pDestSurface)
		return NULL;

	if (SDL_MUSTLOCK(pDestSurface))
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"Lock surface failed.");
			SDL_FreeSurface(pDestSurface);
			return NULL;
		}
	// get the background color
	Uint32 bgcolor;
	if (SDL_GetColorKey(pSrcSurface, &bgcolor) != 0)
	{
		//Use the first pixel as the background color.
		switch (pSrcSurface->format->BytesPerPixel)
		{
			case 1: bgcolor = *(Uint8*)pSrcOrigin; break;
			case 2: bgcolor = *(Uint16*)pSrcOrigin; break;
			case 4: bgcolor = *(Uint32*)pSrcOrigin; break;
			case 3:
			default:
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
				bgcolor = (((Uint8*)pSrcOrigin)[0]) +
						(((Uint8*)pSrcOrigin)[1]<<8) + (((Uint8*)pSrcOrigin)[2]<<16);
#else
				bgcolor = (((Uint8*)pSrcSurface->pixels)[2]) +
						(((Uint8*)pSrcOrigin)[1]<<8) + (((Uint8*)pSrcOrigin)[0]<<16);
#endif
			break;
		}
		bgcolor &= ~pSrcSurface->format->Amask;
	}

	RotateSimple(pSrcSurface, pSrcOrigin, width, height,
			pDestSurface, bgcolor, sinAngle, cosAngle);

	if (SDL_MUSTLOCK(pDestSurface))
		SDL_UnlockSurface(pDestSurface);

	return pDestSurface;
}

//*****************************************************************************
void CBitmapManager::ScaleSimple(
//Execute a simple and fast scaling algorithm.
//No interpolation is involved, so we don't need to consider blending
//color-keyed transparent pixels with opaque pixels.
//
//Params:
	const SDL_Surface *pSrcSurface,
	const Uint8 *pSrcOrigin,
	const int srcWidth, const int srcHeight,
	SDL_Surface *pDestSurface)
{
	const Uint8* srcRow = pSrcOrigin;
	Uint8* dstRow = (Uint8*)pDestSurface->pixels;

	const int srcPitch = pSrcSurface->pitch;
	const int dstPitch = pDestSurface->pitch;

	const int dstWidth = pDestSurface->w;
	const int dstHeight = pDestSurface->h;
	const int dstWidth2 = pDestSurface->w << 1;
	const int dstHeight2 = pDestSurface->h << 1;

	const int srcWidth2 = srcWidth << 1;
	const int srcHeight2 = srcHeight << 1;

	int w_err, h_err = srcHeight2 - dstHeight2;

	int wY, wX;
	switch (pSrcSurface->format->BytesPerPixel)
	{
	case 1:
		for (wY = dstHeight; wY--; )
		{
			Uint8 *pSrc = (Uint8*)srcRow, *pDest = (Uint8*)dstRow;
			w_err = srcWidth2 - dstWidth2;
			for (wX = dstWidth; wX--; )
			{
				*pDest++ = *pSrc;
				while (w_err >= 0) {++pSrc; w_err -= dstWidth2;}
				w_err += srcWidth2;
			}
			while (h_err >= 0) {srcRow += srcPitch; h_err -= dstHeight2;}
			dstRow += dstPitch;
			h_err += srcHeight2;
		}
		break;
	case 2:
		for (wY = dstHeight; wY--; )
		{
			Uint16 *pSrc = (Uint16*)srcRow, *pDest = (Uint16*)dstRow;
			w_err = srcWidth2 - dstWidth2;
			for (wX = dstWidth; wX--; )
			{
				*pDest++ = *pSrc;
				while (w_err >= 0) {++pSrc; w_err -= dstWidth2;}
				w_err += srcWidth2;
			}
			while (h_err >= 0) {srcRow += srcPitch; h_err -= dstHeight2;}
			dstRow += dstPitch;
			h_err += srcHeight2;
		}
		break;
	case 3:
		for (wY = dstHeight; wY--; )
		{
			Uint8 *pSrc = (Uint8*)srcRow, *pDest = (Uint8*)dstRow;
			w_err = srcWidth2 - dstWidth2;
			for (wX = dstWidth; wX--; )
			{
				pDest[0] = pSrc[0]; pDest[1] = pSrc[1]; pDest[2] = pSrc[2];
				pDest += 3;
				while (w_err >= 0) {pSrc+=3; w_err -= dstWidth2;}
				w_err += srcWidth2;
			}
			while (h_err >= 0) {srcRow += srcPitch; h_err -= dstHeight2;}
			dstRow += dstPitch;
			h_err += srcHeight2;
		}
		break;
	case 4:
	default:
		for (wY = dstHeight; wY--; )
		{
			Uint32 *pSrc = (Uint32*)srcRow, *pDest = (Uint32*)dstRow;
			w_err = srcWidth2 - dstWidth2;
			for (wX = dstWidth; wX--; )
			{
				*pDest++ = *pSrc;
				while (w_err >= 0) {++pSrc; w_err -= dstWidth2;}
				w_err += srcWidth2;
			}
			while (h_err >= 0) {srcRow += srcPitch; h_err -= dstHeight2;}
			dstRow += dstPitch;
			h_err += srcHeight2;
		}
		break;
	}
}

//*****************************************************************************
SDL_Surface* CBitmapManager::ScaleSurface(
//Scale the source surface to the specified size.
//
//Returns: a new scaled surface
//
//Params:
	const SDL_Surface *pSrcSurface, //surface to rotate
	const Uint8 *pSrcOrigin,        //origin pixel for operation
	                          //If not NULL, it must point to a completely contained
	                          //interior location of the src surface pixel buffer.
	const int width,          //dimensions of area of operation
	const int height,
	const int newWidth, const int newHeight)
{
	ASSERT(pSrcSurface);
	ASSERT(pSrcSurface->format->BytesPerPixel > 0 && pSrcSurface->format->BytesPerPixel <= 4);

	if (!pSrcOrigin)
		pSrcOrigin = (Uint8*)pSrcSurface->pixels;

	if (newWidth <= 0 || newHeight <= 0)
		return NULL;

	SDL_Surface* pDestSurface = CreateNewSurfaceLike(pSrcSurface, newWidth, newHeight);
	if (!pDestSurface)
		return NULL;

	if (SDL_MUSTLOCK(pDestSurface))
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"Lock surface failed.");
			SDL_FreeSurface(pDestSurface);
			return NULL;
		}

	if (width == newWidth && height == newHeight)
	{
		//Trivial case.
		int y, bpp = pSrcSurface->format->BytesPerPixel;
		ASSERT(bpp = pDestSurface->format->BytesPerPixel);

		for (y = 0; y < height; ++y)
			memcpy((Uint8*)pDestSurface->pixels + y * pDestSurface->pitch, pSrcOrigin + y * pSrcSurface->pitch, width * bpp);
	} else {
		ScaleSimple(pSrcSurface, pSrcOrigin, width, height, pDestSurface);
	}

	if (SDL_MUSTLOCK(pDestSurface))
		SDL_UnlockSurface(pDestSurface);

	return pDestSurface;
}

//***********************************************************************************
SDL_Surface* CBitmapManager::GetImage(const CStretchyBuffer &buffer) const
//Attempt loading data as image under supported formats.
//
//Returns: pointer to image surface if the buffer represents valid, supported image data
{
	if (buffer.empty())
		return NULL; //empty -- not a valid image

	//BMP
	SDL_Surface *pSurface = SDL_LoadBMP_RW(SDL_RWFromMem((BYTE*)buffer,buffer.Size()), 0);
	if (pSurface)
		return pSurface;

	//PNG
	pSurface = CPNGHandler::CreateSurface((BYTE*)buffer,buffer.Size());
	if (pSurface)
		return pSurface;

	//JPEG
	BYTE *pImageBuffer = NULL;
	UINT wWidth = 0, wHeight = 0;
	if (CJpegHandler::Decompress((BYTE*)buffer,buffer.Size(), pImageBuffer, wWidth, wHeight))
	{
		//Valid image surface.
		SDL_Surface *pTempSurface = SDL_CreateRGBSurfaceFrom(pImageBuffer, wWidth, wHeight,
				24, wWidth * 3, 0xff, 0xff00, 0xff0000, 0);  //libjpeg uses BGR format by default
		SDL_Surface *pSurface = ConvertSurface(pTempSurface);
		if (pSurface != pTempSurface)
		{
			delete[] pImageBuffer;
		} //else: pImageBuffer shouldn't get deleted, but there will be a memory leak
		if (pSurface)
			return pSurface;
		delete[] pImageBuffer;
	}

	//No supported image formats worked with data.
	return NULL;
}

//***********************************************************************************
UINT CBitmapManager::GetImageType(const CStretchyBuffer &buffer) const
//Attempt loading data as image under supported formats.
//
//Returns: true if the buffer represents valid image data, else false.
{
	if (!buffer.Size())
		return DATA_UNKNOWN;	//empty -- not a valid image

	//BMP
	SDL_Surface *pSurface = SDL_LoadBMP_RW(SDL_RWFromMem((BYTE*)buffer,buffer.Size()), 0);
	if (pSurface)
	{
		//Valid BMP surface.
		SDL_FreeSurface(pSurface);
		return DATA_BMP;
	}

	//PNG
	pSurface = CPNGHandler::CreateSurface((BYTE*)buffer,buffer.Size());
	if (pSurface)
	{
		//Valid BMP/PNG surface.
		SDL_FreeSurface(pSurface);
		return DATA_PNG;
	}

	//JPEG
	BYTE *pImageBuffer = NULL;
	UINT wWidth = 0, wHeight = 0;
	if (CJpegHandler::Decompress((BYTE*)buffer,buffer.Size(), pImageBuffer, wWidth, wHeight))
	{
		//Valid JPEG surface.
		delete[] pImageBuffer;
		return DATA_JPG;
	}

	//No supported image formats worked with data.
	return DATA_UNKNOWN;
}

//**********************************************************************************
CIDSet CBitmapManager::GetSupportedImageFormats()
//Returns: set of supported image formats
{
	CIDSet imageFormats(DATA_JPG);
	imageFormats += DATA_PNG;
	imageFormats += DATA_BMP;
	return imageFormats;
}

//**********************************************************************************
void CBitmapManager::UpdateRects(SDL_Surface *pScreenSurface)
//Updates the screen if regions were marked as damaged.
//The surface passed in should be pointing to the screen surface.
//Call this or UpdateScreen once per frame.
{
	if (!this->rects.size())
	{
#ifdef STEAMBUILD
		//We need to refresh the screen for the Steam overlay to work.
		PresentFrame();
#endif
		return;
	}

	UpdateScreen(pScreenSurface);
}

//**********************************************************************************
void CBitmapManager::UpdateScreen(SDL_Surface *pScreenSurface)
//Updates entire screen.  Resets set of screen regions to update.
//Call once per frame.
{
	PresentRect(pScreenSurface);
	this->rects.clear();
}

//
//Private methods.
//

//**********************************************************************************
bool CBitmapManager::DoesTileImageContainTransparentPixels(
//Scans pixels of a tile image for reserved transparent color key.
//
//Params:
	const UINT wTileImageNo)   //(in)   Tile image to scan.
//
//Returns:
//True if a transparent pixel is found, false if not.
{
	ASSERT(wTileImageNo < this->wTileCount); //not custom tile
	const UINT wSurfaceIndex = GetTileSurfaceNumber(wTileImageNo);
	const SDL_Surface *pSurface = this->pTileSurfaces[wSurfaceIndex];
	const UINT wBPP = pSurface->format->BytesPerPixel;
	ASSERT(wBPP >= 3);
	LockTileImagesSurface(wSurfaceIndex);

	const BYTE bytT1 = this->TransparentColor[wSurfaceIndex].byt1; //optimization
	const BYTE bytT2 = this->TransparentColor[wSurfaceIndex].byt2;
	const BYTE bytT3 = this->TransparentColor[wSurfaceIndex].byt3;

	const UINT dwRowWidth = CBitmapManager::CX_TILE * wBPP;
	const UINT dwRowOffset = pSurface->pitch - dwRowWidth;
	Uint8 *pSeek = GetTileSurfacePixel(wTileImageNo) + PIXEL_FUDGE_FACTOR;
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pSurface->format->Rmask == 0xff0000);
	ASSERT(pSurface->format->Gmask == 0x00ff00);
	ASSERT(pSurface->format->Bmask == 0x0000ff);

	ASSERT(bytT1 == bytT3);
		// I am guessing that the format is in the opposite order from what the code below expects.
		// as long as these bands match, that's no problem...
#endif
	Uint8 *const pStop = pSeek + (CBitmapManager::CY_TILE * pSurface->pitch);
	while (pSeek != pStop)
	{
		ASSERT(pSeek < pStop);

		//Scan a row for transparent pixels.
		Uint8 *const pEndOfRow = pSeek + dwRowWidth;
		while (pSeek != pEndOfRow)
		{
			if (pSeek[0] == bytT1 && pSeek[1] == bytT2 && pSeek[2] == bytT3)
			{
				//Found a transparent pixel.
				UnlockTileImagesSurface(wSurfaceIndex);
				return true;
			}
			pSeek += wBPP;
		}
		pSeek += dwRowOffset;
	}

	UnlockTileImagesSurface(wSurfaceIndex);
	return false;
}

//**********************************************************************************
LOADEDBITMAP * CBitmapManager::FindLoadedBitmap(
//Find a loaded bitmap by name.
//
//Params:
	const WCHAR *wszName) //(in)  Name of the bitmap, not including extension.
//
//Returns:
//Pointer to loaded bitmap structure or NULL if not found.
const
{
	for(list<LOADEDBITMAP *>::const_iterator iSeek = this->LoadedBitmaps.begin();
		iSeek != this->LoadedBitmaps.end(); ++iSeek)
	{
		if (WCScmp(wszName, (*iSeek)->wszName)==0) return *iSeek; //Found it.
	}

	//No match.
	return NULL;
}

//**********************************************************************************
UINT CBitmapManager::GetImageExtensionType(
//Returns: image format, based on file extension, or 0 if extension is not recognized
//
//Params:
	const WCHAR *wszFilePath)  //(out)  Full path to filename.
const
{
	for (UINT i=0; i<wNumImageFormats; ++i)
		if (!WCScmp(wszFilePath + WCSlen(wszFilePath) - WCSlen(imageExtension[i]), imageExtension[i]))
			return imageFormat[i];

	return 0;
}

//**********************************************************************************
WSTRING CBitmapManager::GetImagePath()
{
	CFiles Files;
	WSTRING wstr = Files.GetResPath();
	wstr += wcszBitmaps;
	return wstr;
}

WSTRING CBitmapManager::GetImageModPath()
{
	CFiles Files;
	WSTRING wstr = Files.GetDatPath();
	wstr += wcszBitmaps;
	return wstr;
}

//**********************************************************************************
UINT CBitmapManager::GetImageFilepath(
//Given a basic image filename (minus path and extension), find the first
//extension that is valid (i.e. a file exists with that extension)
//
//Returns: index of file format found, else 0 if none
//
//Params:
	const WCHAR *wszName,   //(in) relative filename (w/o extension)
	WSTRING &wstrFilepath)  //(out) file w/ extension
{
	ASSERT(wszName);
	ASSERT(WCSlen(wszName) > 0);
	ASSERT(wstrFilepath.size() == 0);

	//Try loading from user mod path.
	wstrFilepath = GetImageModPath();
	wstrFilepath += wszName;
	UINT format = GetImageFormat(wstrFilepath);

	if (!format) {
		//Try standard path.
		wstrFilepath = GetImagePath();
		wstrFilepath += wszName;
		format = GetImageFormat(wstrFilepath);
	}

	return format;
}

UINT CBitmapManager::GetImageFormat(WSTRING& wstrFilepath)
{
	WSTRING strPathPlusExt;
	for (UINT i=0; i<wNumImageFormats; ++i)
	{
		strPathPlusExt = wstrFilepath;
		strPathPlusExt += imageExtension[i];
		if (CFiles::DoesFileExist(strPathPlusExt.c_str()))
		{
			wstrFilepath = strPathPlusExt;
			return imageFormat[i];
		}
	}

	//No valid format found.
	return 0;
}

//**********************************************************************************
UINT CBitmapManager::GetImageFilepathExtensionType(
//Given a base image filename (full path, minus extension), find the first
//extension that is valid (i.e. a file exists with that extension)
//
//Returns: index of file format found, else 0 if none
//
//Params:
	const WCHAR *wszName)   //(in) path + filename (w/o extension)
{
   ASSERT(wszName);
	ASSERT(WCSlen(wszName) > 0);

	for (UINT i=0; i<wNumImageFormats; ++i)
	{
		WSTRING strPathPlusExt = wszName;
		strPathPlusExt += imageExtension[i];
		if (CFiles::DoesFileExist(strPathPlusExt.c_str()))
			return imageFormat[i];
	}

	//No valid format found.
	return 0;
}

//**********************************************************************************
WSTRING CBitmapManager::GetTileImageMapFilepath(
//Gets full path to a tile image map that corresponds to a bitmap.
//
//Params:
	const WCHAR *wszName)   //(in)   Name of bitmap without extension.
{
	ASSERT(wszName);
	ASSERT(WCSlen(wszName) > 0);

	WSTRING wstrFilepath = GetImagePath();
	wstrFilepath += wszName;
	wstrFilepath += wszTIM;
	return wstrFilepath;
}

WSTRING CBitmapManager::GetTileImageMapModFilepath(const WCHAR *wszName)
{
	ASSERT(wszName);
	ASSERT(WCSlen(wszName) > 0);

	WSTRING wstrFilepath = GetImageModPath();
	wstrFilepath += wszName;
	wstrFilepath += wszTIM;
	return wstrFilepath;
}

//**********************************************************************************
bool CBitmapManager::GetMappingIndexFromTileImageMap(
//Gets a list of TI_* constants that correspond to tile images in a bitmap.  A
//.tim file with a filename matching the tile image bitmap is loaded and parsed into
//the list.
//
//Params:
	const WCHAR *wszName,      //(in)   Name of the bitmap with no file extension.
	list<UINT> &MappingIndex)  //(out)  List of TI_* constants.  First one is for
								//    topleft square in the bitmap, and then
								//    progresses by column and row.
//
//Returns:
//True if successful, false if not.
const
{
	//Load the .tim file into a buffer.
	CStretchyBuffer Buffer;
	WSTRING wstrFilepath = GetTileImageMapModFilepath(wszName);
	if (!CFiles::ReadFileIntoBuffer(wstrFilepath.c_str(), Buffer)) {
		wstrFilepath = GetTileImageMapFilepath(wszName);
		if (!CFiles::ReadFileIntoBuffer(wstrFilepath.c_str(), Buffer)) {
			return false;
		}
	}

	return ParseTileImageMap((const char *)(BYTE *)Buffer, MappingIndex);
}

//*****************************************************************************
TILEIMAGETYPE CBitmapManager::GetTileType(const UINT wTileNo) const
{
	if (wTileNo < this->wTileCount)
		return this->TileImageTypes[wTileNo];
	return TIT_Transparent; //all custom tiles are assumed to be transparent
}

//*****************************************************************************
void CBitmapManager::Invert(
//Invert color of pixels in area.
//
//Params:
	SDL_Surface *pDestSurface,
	int x, int y, UINT w, UINT h)  //(in) region
{
	if (x>=pDestSurface->w || y>=pDestSurface->h) return;

	//Bounds checking.
	if (w>static_cast<UINT>(pDestSurface->w)) w = pDestSurface->w;
	if (h>static_cast<UINT>(pDestSurface->h)) h = pDestSurface->h;
	if (x<0) {if ((int)w<-x) return; w+=x; x=0;}
	if (y<0) {if ((int)h<-y) return; h+=y; y=0;}
	if (static_cast<int>(x+w) > pDestSurface->w) w=pDestSurface->w-x;
	if (static_cast<int>(y+h) > pDestSurface->h) h=pDestSurface->h-y;

	//Invert all pixels in region.
	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wPixelByteNo = y * pDestSurface->pitch + (x * wBPP);
	const UINT dwRowOffset = pDestSurface->pitch - (w * wBPP);

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"CBitmapManager::Invert: Lock surface failed.");
			return;
		}
	}

	Uint8 *pSeek = (Uint8 *)pDestSurface->pixels + wPixelByteNo + PIXEL_FUDGE_FACTOR;
	Uint8 *const pStop = pSeek + (h * pDestSurface->pitch);

	while (pSeek != pStop)
	{
		ASSERT(pSeek < pStop);
		Uint8 *const pEndOfRow = pSeek + (w * wBPP);

		//Each iteration inverts one pixel.
		while (pSeek != pEndOfRow)
		{
			pSeek[0] = 255 - pSeek[0];
			pSeek[1] = 255 - pSeek[1];
			pSeek[2] = 255 - pSeek[2];
			pSeek += wBPP;
		}
		pSeek += dwRowOffset;
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);
}

//*****************************************************************************
SDL_Surface* CBitmapManager::LoadImageSurface(
//Loads an image from the appropriate location into a new surface.
//
//Params:
	const WCHAR *wszName)   //(in)   Relative name of the image file.
//
//Returns:
//Loaded surface if successful, or NULL if not.
{
	//Get full path to image file.  Determine image format.
	WSTRING wstrFilepath;
	const UINT wFormat = GetImageFilepath(wszName, wstrFilepath);
	if (!wFormat)
		return NULL;

	//Load file according to format.
	return LoadImageSurface(wstrFilepath.c_str(), wFormat);
}

//*****************************************************************************
SDL_Surface* CBitmapManager::LoadImageSurface(
//Loads an image with the specified name and format
//
//Params:
	const WCHAR *wszName,   //(in)   Name+path of the image file.
	const UINT wFormat)     //(in)   Image format.
//
//Returns:
//Loaded surface if successful, or NULL if not.
{
	switch (wFormat)
	{
		case DATA_BMP: return LoadBitmapSurface(wszName);
		case DATA_JPG: return LoadJPEGSurface(wszName);
		case DATA_PNG: return LoadPNGSurface(wszName);
		default: ASSERT(!"Unrecognized image format"); return NULL;
	}
}

//*****************************************************************************
BYTE* CBitmapManager::WriteSurfaceToPixelBuffer(SDL_Surface *pSurface)
//Convert surface to pixel byte buffer.
{
	if (!pSurface) return NULL;

	const UINT wBPP = pSurface->format->BytesPerPixel;
	ASSERT(wBPP == CBitmapManager::BYTES_PER_PIXEL);
	static const UINT PIXELBYTES = 3;

	if (SDL_MUSTLOCK(pSurface))
		if (SDL_LockSurface(pSurface) < 0)
		{
			ASSERT(!"CBitmapManager::WriteSurfaceToPixelBuffer(): lock surface failed.");
			return NULL;
		}

	BYTE *pBuffer = new BYTE[pSurface->w * pSurface->h * PIXELBYTES];
	if (!pBuffer) return NULL;

#if GAME_BYTEORDER == GAME_BYTEORDER_BIG
        const UINT R = 4 - pSurface->format->Rshift / 8;
        const UINT G = 4 - pSurface->format->Gshift / 8;
        const UINT B = 4 - pSurface->format->Bshift / 8;
#else
	const UINT R = pSurface->format->Rshift / 8;
	const UINT G = pSurface->format->Gshift / 8;
	const UINT B = pSurface->format->Bshift / 8;
#endif

	BYTE *pPixel = pBuffer;
	Uint8 *pSeek = (Uint8 *)pSurface->pixels;
	Uint8 *const pStop = pSeek + (pSurface->h * pSurface->pitch);
	const UINT dwRowOffset = pSurface->pitch - (pSurface->w * wBPP);
	while (pSeek != pStop)
	{
		ASSERT(pSeek < pStop);
		Uint8 *const pEndOfRow = pSeek + (pSurface->w * wBPP);

		//Each iteration gets one pixel.
		while (pSeek != pEndOfRow)
		{
			*(pPixel++) = pSeek[R];
			*(pPixel++) = pSeek[G];
			*(pPixel++) = pSeek[B];
			pSeek += wBPP;
		}
		pSeek += dwRowOffset;
		ASSERT(((pPixel - pBuffer) % (pSurface->w * PIXELBYTES)) == 0);
	}
	ASSERT(pPixel == (pBuffer + pSurface->w * pSurface->h * PIXELBYTES));

	if (SDL_MUSTLOCK(pSurface)) SDL_UnlockSurface(pSurface);

	return pBuffer;
}

//**********************************************************************************
SDL_Surface * CBitmapManager::LoadBitmapSurface(
//Loads a bitmap from the appropriate location into a new surface.
//
//Params:
	const WCHAR *wszName)   //(in)   Full name of the bitmap file.
//
//Returns:
//Loaded surface if successful, or NULL if not.
{
	ASSERT(wszName);
	ASSERT(WCSlen(wszName) > 1);

	CStretchyBuffer buffer;
	CFiles::ReadFileIntoBuffer(wszName,buffer,true);
	if (CFiles::FileIsEncrypted(wszName))
	{
		//Unprotect encrypted bitmap file.
		buffer.Decode();
	}
	SDL_Surface *pSurface = SDL_LoadBMP_RW(SDL_RWFromMem((BYTE*)buffer,buffer.Size()), 0);
	if (!pSurface)
	{
		BYTE *u8str = NULL;
		to_utf8(wszName, u8str);
		char szErrMsg[1024];
		_snprintf(szErrMsg, 1024, "SDL_LoadBMP_RW(\"%s\", ...) failed: %s",
				u8str, SDL_GetError());
		delete[] u8str;
		LOGERR(szErrMsg);
		return NULL;
	}

	return ConvertSurface(pSurface);
}

//**********************************************************************************
SDL_Surface* CBitmapManager::LoadJPEGSurface(
//Loads a JPEG image from the appropriate location into a new surface.
//
//Params:
	const WCHAR *wszName)   //(in)   Name of the image, not including extension.
{
	BYTE *pImageBuffer = NULL;
	UINT wWidth = 0, wHeight = 0;
	if (!CJpegHandler::Decompress(wszName, pImageBuffer, wWidth, wHeight))
		return NULL;
#if GAME_BYTEORDER == GAME_BYTEORDER_BIG
        SDL_Surface *pTempSurface = SDL_CreateRGBSurfaceFrom(pImageBuffer, wWidth, wHeight,
                        24, wWidth * 3, 0xff0000, 0xff00, 0xff, 0);  //libjpeg uses BGR format by default
#else
	SDL_Surface *pTempSurface = SDL_CreateRGBSurfaceFrom(pImageBuffer, wWidth, wHeight,
			24, wWidth * 3, 0xff, 0xff00, 0xff0000, 0);  //libjpeg uses BGR format by default
#endif
	SDL_Surface *pSurface = ConvertSurface(pTempSurface);
	if (pSurface != pTempSurface)
	{
		delete[] pImageBuffer;
	} //else: pImageBuffer shouldn't get deleted, but there will be a memory leak
	return pSurface;
}

//**********************************************************************************
SDL_Surface* CBitmapManager::LoadPNGSurface(
//Loads a JPEG image from the appropriate location into a new surface.
//
//Params:
	const WCHAR *wszName)   //(in)   Name of the image, not including extension.
{
	SDL_Surface *pSurface = CPNGHandler::CreateSurface(wszName);
	return ConvertSurface(pSurface);
}

//**********************************************************************************
bool CBitmapManager::ParseTileImageMap(
//Parse the buffer for indices.  Here are the rules:
//
//Each TOKEN is delimited by any combination of ',', SPACE, CR, or LF.
//
//TOKEN is INCLUDE, INCLUDERANGE, or EXCLUDE
//INCLUDE is one or more digits.
//INCLUDERANGE is one or more digits, a hyphen, and one or more digits.
//EXCLUDE is '!' and one or more digits.
//
//Params:
	const char *pszSeek,   //(in)   TIM data.
	list<UINT> &MappingIndex)  //(out) List of TI_* constants.  First one is for
								//    topleft square in the bitmap, and then
								//    progresses by column and row.
const
{
	UINT wBeginTINo, wEndTINo, wExcludeCount;

	//Seek past delimiter chars.
	pszSeek = GetMappingIndexFromTileImageMap_SeekPastDelimiters(pszSeek);
	if (*pszSeek == '\0')
		return false; //No tokens in file, so the format is wrong.
	do
	{
		//Parse token.
		pszSeek = GetMappingIndexFromTileImageMap_ParseToken(pszSeek,
				wBeginTINo, wEndTINo, wExcludeCount);
		if (!pszSeek)
			return false; //No token, so the format is wrong.

		//Was token of EXCLUDE type?
		if (wExcludeCount)
		{
			//Yes--add that number of TI_UNSPECIFIED indices to the index.
			for (UINT wExcludeI = 0; wExcludeI < wExcludeCount; ++wExcludeI)
				MappingIndex.push_back(TI_UNSPECIFIED);
		}
		else
		{
			//No--add range of indices to the index.  There may be only one
			//indice in range.
			for (UINT wIncludeTINo = wBeginTINo; wIncludeTINo <= wEndTINo;
					++wIncludeTINo)
				MappingIndex.push_back(wIncludeTINo);
		}

		//Seek past delimiter chars.
		pszSeek = GetMappingIndexFromTileImageMap_SeekPastDelimiters(pszSeek);
	} while (*pszSeek != '\0');

	return true;
}

//**********************************************************************************
const char *CBitmapManager::GetMappingIndexFromTileImageMap_ParseToken(
//Parse a token from current position in buffer.  Should only be called from
//GetMappingIndexFromTileImageMap().
//
//Params:
	const char *pszSeek,    //(in)      Location to parse from.
	UINT &wBeginTINo,       //(out)     First TI# indicated by INCLUDE or
								//       INCLUDERANGE token.
	UINT &wEndTINo,            //(out)     Second TI# indicated by INCLUDERANGE
								//       token.  For INCLUDE token this will
								//       be same as wBeginTINo.
	UINT &wExcludeCount)    //(out)     Number of tile images to exclude from
								//       map.  Only set for EXCLUDE tokens.
//
//Returns:
//Pointer to char following token or NULL if token couldn't be parsed.
const
{
	const char *pszRet = pszSeek;
	const UINT MAX_DIGITS = 5;
	char szNumber[MAX_DIGITS + 1];
	UINT wCopyI;

	//Skip past any "junk chars".
	while (*pszRet != '\0' && *pszRet != '!' &&
			!(*pszRet >= '0' && *pszRet <= '9'))
		++pszRet;

	//Can't parse if at EOS.
	if (*pszRet == '\0')
		return NULL;

	//Is it an EXCLUDE token?
	if (*pszRet == '!')  //Yes.
	{
		++pszRet;

		//Copy numeric chars into buffer.
		for (wCopyI = 0; wCopyI < MAX_DIGITS; ++wCopyI)
		{
			if (!(*pszRet >= '0' && *pszRet <= '9'))
				break; //End of number found.
			szNumber[wCopyI] = *(pszRet++);
		}
		if (wCopyI == MAX_DIGITS)
			return NULL; //Number is too large.
		szNumber[wCopyI] = '\0';

		//Set return params.
		wExcludeCount = atoi(szNumber);
		if (wExcludeCount == 0)
			return NULL; //Exclude count of 0 is invalid.
		wBeginTINo = wEndTINo = TI_UNSPECIFIED;
	}
	else  //No, it's not an EXCLUDE token.
	{
		wExcludeCount = 0;

		//Copy numeric chars into buffer.
		for (wCopyI = 0; wCopyI < MAX_DIGITS; ++wCopyI)
		{
			if (!(*pszRet >= '0' && *pszRet <= '9'))
				break; //End of number found.
			szNumber[wCopyI] = *(pszRet++);
		}
		if (wCopyI == MAX_DIGITS)
			return NULL; //Number is too large.
		szNumber[wCopyI] = '\0';

		//Set begin TI# param from buffer.
		wBeginTINo = atoi(szNumber);

		//Is the next char a hyphen?
		if (*pszRet == '-')
		{
			//Yes, so this is an INCLUDERANGE token.
			++pszRet;

			//Copy numeric chars into buffer.
			for (wCopyI = 0; wCopyI < MAX_DIGITS; ++wCopyI)
			{
				if (!(*pszRet >= '0' && *pszRet <= '9'))
					break; //End of number found.
				szNumber[wCopyI] = *(pszRet++);
			}
			if (wCopyI == MAX_DIGITS)
				return NULL; //Number is too large.
			szNumber[wCopyI] = '\0';
			if (wCopyI == 0)
				return NULL; //Nothing usuable on right side of hyphen.

			//Set end TI# param from buffer.
			wEndTINo = atoi(szNumber);
		}
		else
			//No, so this is an INCLUDE token.  Set end TI# from begin TI#.
			wEndTINo = wBeginTINo;
	}

	//Skip past "junk chars" that are not delimiter chars.
	while (*pszRet != '\0' && *pszRet != '\r' && *pszRet != '\n' && *pszRet != ' '
			&& *pszRet != ',')
		++pszRet;

	return pszRet;
}

//**********************************************************************************
const char *CBitmapManager::GetMappingIndexFromTileImageMap_SeekPastDelimiters(
//Seek past any delimiter chars from current position in buffer.  Should only be
//called from GetMappingIndexFromTileImageMap().
//
//Params:
	const char *pszSeek)    //(in)      Location to seek from.
//
//Returns:
//Pointer to char following delimiters.
const
{
	const char *pszRet = pszSeek;

	bool comment;
	do {
		comment = false;
		while (isspace(*pszRet) || *pszRet == ',')
			++pszRet;
		if (*pszRet == '#') {
			comment = true;
			while (*pszRet && *pszRet != '\n')
				++pszRet;
		}
	} while (comment);

	return pszRet;
}

//**********************************************************************************
SDL_Surface* CBitmapManager::GetTileSurface(const UINT wTileImageNo) const
//Returns: surface that contains this tile
{
	if (wTileImageNo < this->wTileCount)
		return this->pTileSurfaces[GetTileSurfaceNumber(wTileImageNo)];

	//Custom tile set surface lookup.
	const CustomTiles *pCustomTiles = GetCustomTilesFor(wTileImageNo);
	ASSERT(pCustomTiles);
	if (!pCustomTiles)
		return NULL;
	return pCustomTiles->pSurface;
}

//**********************************************************************************
Uint8* CBitmapManager::GetTileSurfacePixel(
//Returns: memory address of first pixel in indicated tile, with optional vertical offset
//
//Params:
	const UINT wTileImageNo)
const
{
	if (wTileImageNo < this->wTileCount)
	{
		const SDL_Surface* pSurface = GetTileSurface(wTileImageNo);
		return (Uint8*)pSurface->pixels +
			(GetTileSurfaceIndex(wTileImageNo) * CBitmapManager::CY_TILE) * pSurface->pitch;
	}

	//Search custom tile set.
	ASSERT(wTileImageNo < this->wNextCustomTileNo);
	const CustomTiles *pCustomTiles = GetCustomTilesFor(wTileImageNo);
	ASSERT(pCustomTiles);
	SDL_Surface *pSurface = pCustomTiles->pSurface;
	ASSERT(pSurface);
	ASSERT(pSurface->pixels);
	const UINT wIndexInSurface = wTileImageNo - pCustomTiles->wTileStartNo;
	const UINT wTilesInRow = pSurface->w/CBitmapManager::CX_TILE;
	const UINT wX = (wIndexInSurface % wTilesInRow) * CBitmapManager::CX_TILE;
	const UINT wY = (wIndexInSurface / wTilesInRow) * CBitmapManager::CY_TILE;
	return (Uint8*)pSurface->pixels + (wY * pSurface->pitch + wX * pSurface->format->BytesPerPixel);
}

//**********************************************************************************
Uint8* CBitmapManager::GetTileSurfacePixel(
//Copy of the above code, plus coordinate offset.
	const UINT wTileImageNo, const UINT xOffset, const UINT yOffset)
const
{
	if (wTileImageNo < this->wTileCount)
	{
		const SDL_Surface* pSurface = GetTileSurface(wTileImageNo);
		return (Uint8*)pSurface->pixels +
			(GetTileSurfaceIndex(wTileImageNo) * CBitmapManager::CY_TILE + yOffset) * pSurface->pitch +
			xOffset * pSurface->format->BytesPerPixel;
	}

	//Search custom tile set.
	ASSERT(wTileImageNo < this->wNextCustomTileNo);
	const CustomTiles *pCustomTiles = GetCustomTilesFor(wTileImageNo);
	ASSERT(pCustomTiles);
	SDL_Surface *pSurface = pCustomTiles->pSurface;
	ASSERT(pSurface);
	ASSERT(pSurface->pixels);
	const UINT wIndexInSurface = wTileImageNo - pCustomTiles->wTileStartNo;
	const UINT wTilesInRow = pSurface->w/CBitmapManager::CX_TILE;
	const UINT wX = (wIndexInSurface % wTilesInRow) * CBitmapManager::CX_TILE + xOffset;
	const UINT wY = (wIndexInSurface / wTilesInRow) * CBitmapManager::CY_TILE + yOffset;
	return (Uint8*)pSurface->pixels + (wY * pSurface->pitch + wX * pSurface->format->BytesPerPixel);
}

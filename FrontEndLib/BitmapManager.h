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

#ifndef BITMAPMANAGER_H
#define BITMAPMANAGER_H

#include "Colors.h"

#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/IDSet.h>
#include <BackEndLib/StretchyBuffer.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>

#include <list>
#include <map>
#include <string>
#include <vector>
using std::list;
using std::string;
using std::vector;

#define TI_UNSPECIFIED (UINT(-1))
#define TILESTRIP_SIZE (512)	//number of tiles in each tile surface

static const UINT MAXLEN_BITMAPNAME = 256;
struct LOADEDBITMAP
{
	WCHAR       wszName[MAXLEN_BITMAPNAME + 1];
	UINT       dwRefCount;
	SDL_Surface *  pSurface;
};

enum TILEIMAGETYPE
{
	TIT_Unspecified = -1,
	TIT_Opaque,
	TIT_Transparent
};

struct CustomTiles
{
	SDL_Surface *pSurface; //contains some custom tiles
	UINT wTileStartNo;     //starting number of these custom tiles
};

//Used to mask out (could be either to intersect or subtract, depending on use case)
//the region of a moving tile image from a surface location being modified.
//In other words, this mask indicates where to either include or exclude a rendering operation,
//relative to a destination surface region.
struct TweeningTileMask
{
	TweeningTileMask(UINT tile, int xOffset, int yOffset)
		: tile(tile), xOffset(xOffset), yOffset(yOffset)
	{ }
	UINT tile;
	int xOffset, yOffset; //relative tile location offset from top-left origin of the area this mask is being applied to
};

//DataIDs to find custom tile data that the bitmap manager may use
typedef std::map<UINT,CustomTiles> CustomTileMap;

//****************************************************************************
class CBitmapManager
{
public:
	CBitmapManager();
	virtual ~CBitmapManager();

	void        AddMask(SDL_Surface *pMaskSurface, SDL_Rect src,
			SDL_Surface *pDestSurface, SDL_Rect dest, const float fFactor=1.0f);
	void        AddMaskAdditive(SDL_Surface *pMaskSurface, SDL_Rect src,
			SDL_Surface *pDestSurface, SDL_Rect dest, float fFactor, const bool bUseColorKeyMask);

	void        BAndWRect(const UINT x, const UINT y, const UINT w, const UINT h,
			SDL_Surface *pDestSurface, UINT tileMask=TI_UNSPECIFIED, UINT tileMaskOffsetX=0, UINT tileMaskOffsetY=0);
	void        BAndWTile(const UINT x, const UINT y, SDL_Surface *pDestSurface)
		{BAndWRect(x,y,CX_TILE,CY_TILE,pDestSurface);}
	void        BlitAlphaSurfaceWithTransparency(SDL_Rect src, SDL_Surface *pSrcSurface,
			SDL_Rect dest, SDL_Surface *pDestSurface, const Uint8 opacity);
	void        BlitRGBAtoRGBA(SDL_Surface *pSrcSurface, SDL_Rect *src, 
			SDL_Surface *pDestSurface, SDL_Rect *dest);
	void        BlitSurface(SDL_Surface *pSrcSurface, SDL_Rect* src,
			SDL_Surface *pDestSurface, SDL_Rect* dest,	const Uint8 nOpacity=255);
	inline void BlitTileImage(const UINT wTileImageNo,
			const UINT x, const UINT y, SDL_Surface *pDestSurface,
			const bool bUseLightLevel=false, const Uint8 nOpacity=255)
	{BlitTileImagePart(wTileImageNo, x, y, 0, 0, CX_TILE, CY_TILE, pDestSurface, bUseLightLevel, nOpacity);}
	void        BlitTileImagePart(const UINT wTileImageNo, const UINT x,
			const UINT y, const UINT xSource, const UINT ySource,
			const UINT cxSize, const UINT cySize, SDL_Surface *pDestSurface,
			const bool bUseLightLevel=false, const Uint8 nOpacity=255);
	void        BlitTileShadows(const UINT* pwShadowTIs, const UINT wNumShadows,
			const UINT x, const UINT y, SDL_Surface *pDestSurface);
	void        BlitTileShadowsWithTileMask(const UINT* pwShadowTIs, const UINT wNumShadows,
			const UINT wTIMask, const UINT x, const UINT y, SDL_Surface *pDestSurface,
			SDL_Rect *crop=NULL, int maskStartX=-1, int maskStartY=-1);
	void        BlitWithTileMask(const UINT wTIMask,
			SDL_Rect src, SDL_Surface *pSrcSurface,
			SDL_Rect dest, SDL_Surface *pDestSurface, const Uint8 opacity=255);
	void        BlitTileWithTileMask(const UINT wTIMask, const UINT wTileNo,
			SDL_Rect dest, SDL_Surface *pDestSurface, const Uint8 opacity=255);
	void        BlitWithMask(SDL_Rect mask, SDL_Surface *pMaskSurface,
			SDL_Rect src, SDL_Surface *pSrcSurface,
			SDL_Rect dest, SDL_Surface *pDestSurface, const Uint8 opacity=255);
	void        BlitTileWithMask(SDL_Rect mask, SDL_Surface *pMaskSurface, const UINT wTileNo,
			SDL_Rect dest, SDL_Surface *pDestSurface, const Uint8 opacity=255);
	void        BlitWrappingSurface(SDL_Surface *pSrcSurface, SDL_Rect src,
			SDL_Surface *pDestSurface, SDL_Rect dest, const Uint8 nOpacity=255);

	static void ClipSrcAndDestToRect(SDL_Rect& src, SDL_Rect& dest,
			const UINT W, const UINT H);
	static SDL_Surface* ConvertSurface(SDL_Surface *pSurface);
	static SDL_Surface* CreateNewSurfaceLike(const SDL_Surface *pSrcSurface,
			const int width, const int height);
	void        CropToOpaque(SDL_Rect &rect, SDL_Surface *pSurface, Uint8 *pSrc=NULL);

	void        DarkenRect(const UINT x, const UINT y, const UINT w, const UINT h,
			const float fLightPercent, SDL_Surface *pDestSurface);
	void        DarkenTile(const UINT x, const UINT y,
			const float fLightPercent, SDL_Surface *pDestSurface)
		{DarkenRect(x,y,CX_TILE,CY_TILE,fLightPercent,pDestSurface);}
	void        DarkenTileWithMask(const UINT wTIMask, const UINT wXOffset, const UINT wYOffset,
			const UINT x, const UINT y, const UINT w, const UINT h,
			SDL_Surface *pDestSurface, const float fLightPercent);
	void        DarkenTileWithMultiTileMask(const vector<TweeningTileMask>& masks,
			const UINT x, const UINT y, const UINT w, const UINT h,
			SDL_Surface* pDestSurface, const float fLightPercent);
	void        DarkenWithMask(SDL_Surface *pMaskSurface, SDL_Rect src,
			SDL_Surface *pDestSurface, SDL_Rect dest, const float fLightFactor,
			const bool bUseColorKeyMask=false);
	SDL_Surface* GetBitmapSurface(const char *wszName);
	virtual UINT GetCustomTileNo(const UINT wTileSet, UINT wX, UINT wY,
			const bool bYWraparound=false);
	const CustomTiles* GetCustomTilesFor(const UINT wTileNo) const;
	UINT        GetImageExtensionType(const WCHAR *wszFilePath) const;
	static UINT GetImageFilepath(const WCHAR *wszName, WSTRING &wstrFilepath);
	static UINT GetImageFilepathExtensionType(const WCHAR *wszName);
	SDL_Surface* GetImage(const CStretchyBuffer &buffer) const;
	UINT			GetImageType(const CStretchyBuffer &buffer) const;
	UINT        GetNextCustomTileNo() const {return this->wNextCustomTileNo;}
	static CIDSet GetSupportedImageFormats();
	static WSTRING GetTileImageMapFilepath(const WCHAR *pszName);
	static WSTRING GetTileImageMapModFilepath(const WCHAR *pszName);

	//Tile surface info lookups.
	SDL_Surface* GetTileSurface(const UINT wTileImageNo) const;
	inline UINT GetTileSurfaceIndex(const UINT wTileImageNo) const
		{return wTileImageNo % TILESTRIP_SIZE;}	//tile's local position in surface
	inline UINT GetTileSurfaceNumber(const UINT wTileImageNo) const
		{return wTileImageNo / TILESTRIP_SIZE;}	//which surface to access
	Uint8*       GetTileSurfacePixel(const UINT wTileImageNo) const;
	Uint8*       GetTileSurfacePixel(const UINT wTileImageNo,
			const UINT xOffset, const UINT yOffset) const;
	inline UINT GetTileSurfaceY(const UINT wTileImageNo) const
		{return GetTileSurfaceIndex(wTileImageNo) * CY_TILE;}
	void SetSurfaceColorKey(const UINT wTileImageNo, const UINT wSurfaceIndex, SDL_Surface *pTileSurface);

	TILEIMAGETYPE GetTileType(const UINT wTileNo) const;
	virtual UINT   Init() {return 0;}
	void        Invert(SDL_Surface *pDestSurface, int x, int y, UINT w, UINT h);

	void        LightenRect(SDL_Surface *pDestSurface,
			const UINT x, const UINT y, const UINT w, const UINT h,
			const float R, const float G, const float B);
	void        LightenRect(SDL_Surface *pDestSurface,
			const UINT x, const UINT y, const UINT w, const UINT h,
			const float lightVal)
		{LightenRect(pDestSurface,x,y,w,h,lightVal,lightVal,lightVal);}
	void        LightenRect(SDL_Surface *pDestSurface,
			const UINT x, const UINT y, const UINT w, const UINT h,
			const UINT* R, const UINT* G,	const UINT* B,
			const UINT wTIMask=TI_UNSPECIFIED, const UINT wXOffset=0, const UINT wYOffset=0);
	void LightenRectWithTileMask(SDL_Surface *pDestSurface,
			const UINT x, const UINT y, const UINT w, const UINT h,
			const float R, const float G,	const float B,
			const UINT wTIMask, const UINT wXOffset, const UINT wYOffset);
	void        LightenTile(SDL_Surface *pDestSurface, const UINT x, const UINT y,
			const float lightVal)
		{LightenRect(pDestSurface,x,y,CX_TILE,CY_TILE,lightVal);}
	void        LightenTile(SDL_Surface *pDestSurface, const UINT x, const UINT y,
			const float R, const float G, const float B)
		{LightenRect(pDestSurface,x,y,CX_TILE,CY_TILE,R,G,B);}

	virtual SDL_Surface *   LoadImageSurface(const WCHAR *wszName);
	SDL_Surface * LoadImageSurface(const WCHAR *wszName, const UINT wFormat);
	void        LockTileImagesSurface(const UINT wIndex);

	void        NegativeRect(const UINT x, const UINT y, const UINT w, const UINT h,
			SDL_Surface *pDestSurface, UINT tileMask=TI_UNSPECIFIED, UINT tileMaskOffsetX=0, UINT tileMaskOffsetY=0);
	void        NegativeTile(const UINT x, const UINT y, SDL_Surface *pDestSurface)
		{NegativeRect(x,y,CX_TILE,CY_TILE,pDestSurface);}
	void        ReleaseBitmapSurface(const char *pszName);
	static SDL_Surface* Rotate90deg(SDL_Surface *pSrcSurface,
		Uint8 *pSrcOrigin, const int srcWidth, const int srcHeight, const int angle);
	static void RotateSimple(SDL_Surface *pSrcSurface,
			Uint8 *pSrcOrigin, const int srcWidth, const int srcHeight,
			SDL_Surface *pDestSurface,
			const Uint32 bgColor, const double sinAngle, const double cosAngle);
	static SDL_Surface* RotateSurface(SDL_Surface *pSrcSurface,
				Uint8 *pSrcOrigin, const int width, const int height, const float fAngle);
	static void ScaleSimple(const SDL_Surface *pSrcSurface, const Uint8 *pSrcOrigin,
			const int srcWidth, const int srcHeight, SDL_Surface *pDestSurface);
	SDL_Surface* ScaleSurface(const SDL_Surface *pSrcSurface, const Uint8 *pSrcOrigin,
			const int width, const int height, const int newWidth, const int newHeight);
	void        SepiaRect(const UINT x, const UINT y, const UINT w, const UINT h,
			SDL_Surface *pDestSurface, UINT tileMask=TI_UNSPECIFIED, UINT tileMaskOffsetX=0, UINT tileMaskOffsetY=0);
	void        SepiaTile(const UINT x, const UINT y, SDL_Surface *pDestSurface)
		{SepiaRect(x,y,CX_TILE,CY_TILE,pDestSurface);}
	void		SetSurfaceAlpha(SDL_Surface* pSurface, Uint8 alpha);
	bool        SetGamma(const float fGamma);
	void        ShadeRect(const UINT x, const UINT y, const UINT w, const UINT h,
			const SURFACECOLOR &Color, SDL_Surface *pDestSurface);
	void        ShadeTile(const UINT x, const UINT y,
			const SURFACECOLOR &Color, SDL_Surface *pDestSurface)
		{ShadeRect(x,y,CX_TILE,CY_TILE,Color,pDestSurface);}
	void			ShadeWithSurfaceMask(SDL_Surface *pSrcSurface, const int srcX, const int srcY,
			SDL_Surface *pDestSurface, const int destX, const int destY,
			const UINT* pwShadowTIs, const UINT wNumShadows);
	void			ShadeWithWrappingSurfaceMask(SDL_Surface *pSrcSurface, SDL_Rect src,
			SDL_Surface *pDestSurface, SDL_Rect dest,
			const UINT* pwShadowTIs, const UINT wNumShadows);
	void        UnlockTileImagesSurface(const UINT wIndex);

	//Marks a screen region as damaged.
	//Call UpdateRects() to update damaged regions on screen.
	inline void UpdateRect(const SDL_Rect &rect) { this->rects.push_back(rect); }

	void        UpdateRects(SDL_Surface *pScreenSurface);
	void        UpdateScreen(SDL_Surface *pScreenSurface);

	static BYTE*			WriteSurfaceToPixelBuffer(SDL_Surface *pSurface);

	static UINT CX_TILE, CY_TILE;
	static UINT BITS_PER_PIXEL;
	static UINT BYTES_PER_PIXEL;
	static bool bAlpha;  //whether alpha blending is enabled
	static BYTE eyeCandy;	//hint for whether graphic-intensive effects should be enabled
	static float fLightLevel; //darken some blits to this level
	static Uint8 TransColor[3]; //the transparent pixel color key
	static bool bGameHasFocus; //whether the game window is focused

protected:
	bool        DoesTileImageContainTransparentPixels(const UINT wTileImageNo);
	LOADEDBITMAP * FindLoadedBitmap(const WCHAR *wszName) const;
	static WSTRING GetImagePath();
	static WSTRING GetImageModPath();
	virtual bool   GetMappingIndexFromTileImageMap(const WCHAR *pszName,
			list<UINT> &MappingIndex) const;
	const char *   GetMappingIndexFromTileImageMap_ParseToken(
			const char *pszSeek, UINT &wBeginTINo, UINT &wEndTINo,
			UINT &wExcludeCount) const;
	const char *   GetMappingIndexFromTileImageMap_SeekPastDelimiters(
			const char *pszSeek) const;

	SDL_Surface *   LoadBitmapSurface(const WCHAR *wszName);
   SDL_Surface *   LoadJPEGSurface(const WCHAR *wszName);
   SDL_Surface *   LoadPNGSurface(const WCHAR *wszName);
	virtual bool   LoadTileImages(const WCHAR* /*pszName*/, CIDSet& /*styleTiles*/,
			CIDSet* /*pLoadTileSet*/=NULL) {return false;}

	bool        ParseTileImageMap(const char *pszSeek, list<UINT> &MappingIndex) const;

	//Tile (sprite) strips.
	//A vector of surfaces is required to overcome SDL surface/rect size limitations.
	vector<SDL_Surface*> pTileSurfaces;
	vector<bool>         bIsColorKeySet;
	vector<SURFACECOLOR> TransparentColor;

	TILEIMAGETYPE *      TileImageTypes;
	list<LOADEDBITMAP *> LoadedBitmaps;

	UINT                 wTileCount; //tiles loaded on startup

	//Custom tiles loaded at any time.
	CustomTileMap customTiles;
	UINT wNextCustomTileNo;

	vector<SDL_Rect> rects; //damaged screen regions
	bool bAlwaysUpdateScreen; //if set, then always perform an entire screen refresh when updating

private:
	static UINT GetImageFormat(WSTRING& wstrFilepath);
};

//Define global pointer to the one and only CBitmapManager object.
#ifndef INCLUDED_FROM_BITMAPMANAGER_CPP
	extern CBitmapManager *g_pTheBM;
#endif

#endif //...#ifndef BITMAPMANAGER_H

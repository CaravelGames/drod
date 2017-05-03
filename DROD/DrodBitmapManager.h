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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2003, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef DRODBITMAPMANAGER_H
#define DRODBITMAPMANAGER_H

#include <FrontEndLib/BitmapManager.h>

#include "TileImageConstants.h"

//Texture mosaics.
enum Textures
{
	FLOOR_ROAD,
	FLOOR_GRASS,
	FLOOR_DIRT,
	FLOOR_ALT,
	FLOOR_CHECKERED,
	FLOOR_MOSAIC,
	PIT_MOSAIC,
	WALL_MOSAIC,
	PITSIDE_MOSAIC,
	PITSIDE_SMALL,
	FLOOR_IMAGE,
	DEEP_MOSAIC,
	SHALLOW_MOSAIC,
	OVERHEAD_IMAGE,

	TEXTURE_COUNT
};

extern const WCHAR wszSKIES[];
extern const WCHAR wszTILES[];
extern const char textureTileNames[TEXTURE_COUNT][13];

#define TRANSPARENT_RGB CBitmapManager::TransColor[0], CBitmapManager::TransColor[1], CBitmapManager::TransColor[2]

//****************************************************************************
class CDrodBitmapManager : public CBitmapManager
{
public:
	CDrodBitmapManager();
	virtual ~CDrodBitmapManager();
	
	static bool ConvertStyle(WSTRING& style);
	inline void FreezeStyle(const bool bFlag=true) {this->bStyleIsFrozen = bFlag;}
	virtual UINT GetCustomTileNo(const UINT wTileSet, UINT wX, UINT wY,
			const bool bYWraparound=false);
	static Language::LANGUAGE GetLanguage();
	inline bool IsStyleFrozen() const {return this->bStyleIsFrozen;}

	virtual UINT   Init();

	bool           LoadCustomTileSet(const UINT wTileSetDataID);
	virtual SDL_Surface* LoadImageSurface(const WCHAR *wszName);
	SDL_Surface*   LoadImageSurface(const WCHAR *wszName, const UINT dwHoldID);
	SDL_Surface*   LoadImageSurface(const UINT dwDataID);

	bool        LoadGeneralTileImages(CIDSet *pLoadTileSet=NULL);
	bool        LoadTileImagesForStyle(WSTRING style, const bool bForceReload=false);

	static UINT DISPLAY_COLS, DISPLAY_ROWS, CX_ROOM, CY_ROOM;
	SDL_Surface *pTextures[TEXTURE_COUNT];

	static BYTE GetGammaOne();
	bool        SetGamma(const BYTE val);
	static const float fMinGamma, fMaxGamma;
	static Uint8 tarstuffAlpha;
	static bool bDisplayCombos;

	UINT        CreateShallowWaterMix(const UINT wLandMask, const UINT wShallowMask,
			const UINT wDeepMix, SDL_Surface *pDestSurface);

protected:
	SDL_Surface *   LoadJPEGSurface(BYTE* imageBuffer, const UINT wSize);
	SDL_Surface *   LoadPNGSurface(BYTE* imageBuffer, const UINT wSize);

	WSTRING loadedStyle;
	bool    bStyleIsFrozen; //don't change style when set
	CIDSet  loadedStyleSpecificTiles; //tiles loaded for this style

private:
	virtual bool   GetMappingIndexFromTileImageMap(const WCHAR *pszName,
			list<UINT> &MappingIndex) const;
	virtual bool   LoadTileImages(const WCHAR *pszName, CIDSet& styleTiles,
			CIDSet *pLoadTileSet=NULL);
	bool           LoadTexture(const UINT wI, const WSTRING& wstrFilename);
	static const UINT NUM_TILEIMAGESURFACES;
};

//Define global pointer to the one and only CDrodBitmapManager object.
#ifndef INCLUDED_FROM_DRODBITMAPMANAGER_CPP
	extern CDrodBitmapManager *g_pTheDBM;
#endif

#endif //...#ifndef DRODBITMAPMANAGER_H

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

#define INCLUDED_FROM_DRODBITMAPMANAGER_CPP
#include "DrodBitmapManager.h"
#undef INCLUDED_FROM_DRODBITMAPMANAGER_CPP

#include "TileImageConstants.h"

#include <FrontEndLib/JpegHandler.h>
#include <FrontEndLib/PNGHandler.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/SettingsKeys.h"

#include "../Texts/MIDs.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Files.h>

//Holds the only instance of CDrodBitmapManager for the app.
CDrodBitmapManager *g_pTheDBM = NULL;

UINT CDrodBitmapManager::DISPLAY_COLS = 38;
UINT CDrodBitmapManager::DISPLAY_ROWS = 32;
UINT CDrodBitmapManager::CX_ROOM = 0;
UINT CDrodBitmapManager::CY_ROOM = 0;
const UINT CDrodBitmapManager::NUM_TILEIMAGESURFACES = 1 + ((TI_COUNT-1) / TILESTRIP_SIZE);

const float CDrodBitmapManager::fMinGamma = 0.5f;
const float CDrodBitmapManager::fMaxGamma = 2.0f;
Uint8 CDrodBitmapManager::tarstuffAlpha = 255;
bool CDrodBitmapManager::bDisplayCombos = true;

const WCHAR wszSKIES[] = { We('S'),We('k'),We('i'),We('e'),We('s'),We(0) };
const WCHAR wszTILES[] = { We('T'),We('i'),We('l'),We('e'),We('s'),We(0) };

const WCHAR wszDefaultStyleName[] = { We('C'),We('i'),We('t'),We('y'),We(0) };

const char textureTileNames[TEXTURE_COUNT][13] = {
	"Road", "Grass", "Dirt", "Alt",
	"Floor", "Mosaic", "Pit", "Wall", "Pitside", "PitsideSmall",
	"DONT_USE",  //no default FLOOR_IMAGE
	"DeepWater", "ShallowWater",
	"Overhead"
};

const UINT TI_DONT_USE = (UINT)-1;

//
//Public methods.
//

//**********************************************************************************
CDrodBitmapManager::CDrodBitmapManager()
//Constructor.
	: CBitmapManager()
	, bStyleIsFrozen(false)
{
	//192-gray is the transparent pixel color key.
	CBitmapManager::TransColor[0] = CBitmapManager::TransColor[1] = CBitmapManager::TransColor[2] = 192;

	CBitmapManager::CX_TILE = 22;
	CBitmapManager::CY_TILE = 22;
	CDrodBitmapManager::CX_ROOM = CBitmapManager::CX_TILE * CDrodBitmapManager::DISPLAY_COLS;
	CDrodBitmapManager::CY_ROOM = CBitmapManager::CY_TILE * CDrodBitmapManager::DISPLAY_ROWS;

	this->wTileCount = TI_COUNT;
	this->wNextCustomTileNo = this->wTileCount;
	this->TileImageTypes = new TILEIMAGETYPE[TI_COUNT];

	UINT wI;
	for (wI = TEXTURE_COUNT; wI--; )
		this->pTextures[wI] = NULL;
	for (wI = TI_COUNT; wI--; )
		this->TileImageTypes[wI] = TIT_Unspecified;

	//User-specified parameter to always refresh entire screen.
	string str;
	this->bAlwaysUpdateScreen =
		CFiles::GetGameProfileString(INISection::Customizing, INIKey::AlwaysFullBlit, str) &&
			atoi(str.c_str()) != 0;
}

//**********************************************************************************
CDrodBitmapManager::~CDrodBitmapManager()
//Destructor.
{
	for (UINT wI = TEXTURE_COUNT; wI--;)
		if (this->pTextures[wI])
			SDL_FreeSurface(this->pTextures[wI]);
}

//*****************************************************************************
Language::LANGUAGE CDrodBitmapManager::GetLanguage()
//Returns current language setting.
{
	 string strLanguage;
	 if (!CFiles::GetGameProfileString(INISection::Localization, INIKey::Language, strLanguage))
#ifdef RUSSIAN_BUILD
		 return Language::Russian;
#else
		 return Language::English;
#endif

	 const UINT eLanguage = Language::Get(strLanguage.c_str());
	 if (eLanguage == Language::Unknown)
		 return Language::English;
	 return eLanguage;
}

//**********************************************************************************
UINT CDrodBitmapManager::Init()
//Initializes bitmap manager so that it can be used.
//
//Returns:
//MID_Success of a message ID describing failure.
{
	LOGCONTEXT("CDrodBitmapManager::Init");

	ASSERT(CDrodBitmapManager::NUM_TILEIMAGESURFACES > 0);

	//Create the tiles surfaces.  They will be large enough to hold exactly TI_COUNT tiles.
	ASSERT(this->pTileSurfaces.empty()); //nothing should have been set yet
	this->pTileSurfaces.resize(CDrodBitmapManager::NUM_TILEIMAGESURFACES);
	this->TransparentColor.resize(CDrodBitmapManager::NUM_TILEIMAGESURFACES);
	this->bIsColorKeySet.resize(CDrodBitmapManager::NUM_TILEIMAGESURFACES);
	for (UINT wIndex=0; wIndex < CDrodBitmapManager::NUM_TILEIMAGESURFACES; ++wIndex)
	{
		//Strips hold TILESTRIP_SIZE number of tiles, except for the last one,
		//which holds the remainder.
		const UINT wTiles = wIndex < CDrodBitmapManager::NUM_TILEIMAGESURFACES-1 ?
				TILESTRIP_SIZE : TI_COUNT % TILESTRIP_SIZE;
		this->pTileSurfaces[wIndex] = ConvertSurface(SDL_CreateRGBSurface(SDL_SWSURFACE,
				CX_TILE, CY_TILE * wTiles, BITS_PER_PIXEL, 0, 0, 0, 0));
		if (!this->pTileSurfaces[wIndex]) return MID_OutOfMemory;

		this->TransparentColor[wIndex] = GetSurfaceColor(this->pTileSurfaces[wIndex], TRANSPARENT_RGB);
		this->bIsColorKeySet[wIndex] = false;
	}

	//Success.
	return MID_Success;
}

//**********************************************************************************
bool CDrodBitmapManager::ConvertStyle(WSTRING& style)
//If proposed style name is not defined, then use the default style.
//
//Returns: whether style is defined or found
{
	ASSERT(!style.empty());

	//If style is explicitly defined, accept it.
	CFiles f;
	list<WSTRING> styles;
	VERIFY(f.GetGameProfileString(INISection::Graphics, INIKey::Style, styles));
	if (std::find(styles.begin(), styles.end(), style) != styles.end())
		return true;

	//Determine whether a file belonging to the specified style exists,
	//either on disk on in the DB.

	//Check for at least one style file (tiles) on disk.
	WSTRING styleTilename = style;
	styleTilename += wszTILES;
	WSTRING wstrFilepath;
	const UINT wFormat = GetImageFilepath(styleTilename.c_str(), wstrFilepath);
	if (wFormat)
		return true;

	//Check for style in DB.
	CDb db;
	const UINT dwDataID = db.Data.FindByName(styleTilename.c_str());
	if (dwDataID != 0) //found
		return true;

	//Style isn't found -- use default style.
	if (!styles.empty())
		style = wszDefaultStyleName;
	return false;
}

//**********************************************************************************
UINT CDrodBitmapManager::GetCustomTileNo(
//Returns: tile index of tile in the indicated special tile set, for specified index and frame
	const UINT wTileSet,
	UINT wX, UINT wY,
	const bool bYWraparound) //[default=false]
{
	//If tile set is already loaded, return tile number.
	UINT wTileNo = CBitmapManager::GetCustomTileNo(wTileSet, wX, wY, bYWraparound);
	if (wTileNo != TI_UNSPECIFIED)
		return wTileNo;

	if (!LoadCustomTileSet(wTileSet)) //wTileSet is a DataID
		return TI_UNSPECIFIED;

	//Now that tile set is loaded, perform search again.
	return CBitmapManager::GetCustomTileNo(wTileSet, wX, wY, bYWraparound);
}

//**********************************************************************************
bool CDrodBitmapManager::LoadCustomTileSet(const UINT wTileSetDataID)
//Returns: whether custom tile set was not loaded and is now loaded
{
	//If the indicated tile set is already loaded,
	//then this means the tile position queried by GetCustomTileNo is invalid
	//and no tile can be retrieved.
	CustomTileMap::iterator iter = this->customTiles.find(wTileSetDataID);
	if (iter != this->customTiles.end())
		return false; //surface already present -- nothing to load now

	//Tile set is not loaded -- load it now.
	CustomTiles tiles;
	tiles.pSurface = LoadImageSurface(wTileSetDataID);
	if (!tiles.pSurface)
		return false; //surface not present -- can't load

	//Consider all tiles as transparent type.
	SDL_Surface *pSurface = tiles.pSurface;
	SDL_SetSurfaceRLE(pSurface, 0); //need to be able to access pixels directly for blits
	if (pSurface->format->Amask)
		DisableSurfaceBlending(pSurface); //make image opaque and use the transparency color
	const Uint32 TranspColor = SDL_MapRGB(pSurface->format, TRANSPARENT_RGB);
	SetColorKey(pSurface, SDL_TRUE, TranspColor);

	tiles.wTileStartNo = this->wNextCustomTileNo;
	this->customTiles[wTileSetDataID] = tiles;

	//Prepare start tile number for next tile set loaded.
	const UINT wTilesInCol = tiles.pSurface->h/CBitmapManager::CY_TILE;
	const UINT wTilesInRow = tiles.pSurface->w/CBitmapManager::CX_TILE;
	this->wNextCustomTileNo += wTilesInCol * wTilesInRow;

	return true;
}

//**********************************************************************************
bool CDrodBitmapManager::GetMappingIndexFromTileImageMap(
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

	//Search for .tim in data dir.
	if (CBitmapManager::GetMappingIndexFromTileImageMap(wszName, MappingIndex))
		return true;

	//Search for image+tim with this name in the DB.
	CDb db;
	const UINT dwDataID = db.Data.FindByName(wszName);
	if (!dwDataID)
		return false;

	CDbDatum *pData = db.Data.GetByID(dwDataID, true);  //only load the TIM data
	const bool bRes = ParseTileImageMap((const char *)(BYTE *)(pData->timData),
			MappingIndex);
	delete pData;
	return bRes;
}

//**********************************************************************************
bool CDrodBitmapManager::LoadGeneralTileImages(
//Load general tiles that apply to every style.
//
//Returns: whether operation was successful
//
//Params:
	CIDSet *pLoadTileSet) //optional set that specifies the only tiles that are to be loaded [default=NULL]
{
	CFiles f;
	list<WSTRING> tiles;
	if (!f.GetGameProfileString(INISection::Graphics, "General", tiles))
		return false;

	CIDSet ignoredTiles;
	list<WSTRING>::const_iterator iStr;
	for (iStr = tiles.begin(); iStr != tiles.end(); ++iStr)
		if (!LoadTileImages(iStr->c_str(), ignoredTiles, pLoadTileSet))
			return false;

	return true;
}

//**********************************************************************************
bool CDrodBitmapManager::LoadTileImagesForStyle(
//Loads tile images corresponding to a style.  Tile images will become available in
//future calls to GetTileImage().
//
//Params:
	WSTRING style, //(in)   Style to load.
	const bool bForceReload) //whether to force reloading this style if it's already loaded
//
//Returns:
//True if successful, false if not.
{
	if (this->bStyleIsFrozen)
		return true;   //return successfully without changing style

	g_pTheDBM->ConvertStyle(style);

	//Don't reload the currently loaded style.
	if (!this->loadedStyle.compare(style) && !bForceReload)
		return true;	//already loaded

	//If forcing a reload, general tiles is loaded first, and then style-specific tiles.
	if (bForceReload && !LoadGeneralTileImages())
		return false;

	//Lookup base filename given for style.
	CFiles f;
	list<WSTRING> tiles;
	if (!f.GetGameProfileString(INISection::Graphics, style.c_str(), tiles))
		tiles.push_back(style);

	//Load the tilesets for this style in listed order.
	ASSERT(!tiles.empty());

	CIDSet styleTiles;
	for (list<WSTRING>::const_iterator iStr = tiles.begin(); iStr != tiles.end(); ++iStr)
	{
		const WSTRING wstrDefault = *iStr + wszTILES; //look for file called "*Tiles"
		if (!LoadTileImages(wstrDefault.c_str(), styleTiles))  //...but if it's not found...
		   LoadTileImages(iStr->c_str(), styleTiles);          //...just load the listed name
	}

	if (!bForceReload)
	{
		//Check whether any tiles were loaded for the previous style, but not this style.
		this->loadedStyleSpecificTiles -= styleTiles;

		//If there are some tiles from the last style that were not replaced
		//by loading the tile files for this style, then the GeneralTiles file
		//must be reloaded to reacquire the default versions of these tiles.
		if (!this->loadedStyleSpecificTiles.empty())
		{
			//Load missing default tiles.
			if (!LoadGeneralTileImages(&this->loadedStyleSpecificTiles))
				return false;
		}
	}

	//These are the tiles that are loaded specifically for this style, outside
	//of what was loaded in GeneralTiles.
	this->loadedStyleSpecificTiles = styleTiles;

	//Load other style-specific textures.
	WSTRING wstrCurrentName;
	list<WSTRING>::reverse_iterator rStr;
	UINT wI;
	for (wI=TEXTURE_COUNT; wI--; )
	{
		if (wI == FLOOR_IMAGE || wI == OVERHEAD_IMAGE)
			continue; //skip these -- they're loaded separately from this style

		//Last images in list override first ones.
		bool bFoundImage = false;
		AsciiToUnicode(textureTileNames[wI], wstrCurrentName);
		for (rStr = tiles.rbegin(); rStr != tiles.rend(); ++rStr)
			if (LoadTexture(wI, *rStr + wstrCurrentName))
			{
				bFoundImage = true;
				break;
			}

		//If a texture doesn't load after scanning all entries, use the default style's texture.
		if (!bFoundImage)
		{
			if (wI == DEEP_MOSAIC || wI == SHALLOW_MOSAIC)
			{
				//Water tiles require a bit more work
				bFoundImage = false;
				AsciiToUnicode(textureTileNames[wI == DEEP_MOSAIC ? PIT_MOSAIC : FLOOR_DIRT], wstrCurrentName);
				for (rStr = tiles.rbegin(); rStr != tiles.rend(); ++rStr)
					if (LoadTexture(wI, *rStr + wstrCurrentName))
					{
						bFoundImage = true;
						break;
					}

				//If we still haven't found water textures, then progress to default style
				if (!bFoundImage)
				{
					WSTRING wstrFilename = wszDefaultStyleName;
					bool bUseBaseStyle = false;

					//Look up the base filename for this style and use that if defined.
					list<WSTRING> styleFilename;
					if (f.GetGameProfileString(INISection::Graphics, wszDefaultStyleName, styleFilename) &&
							!styleFilename.empty())
					{
						wstrFilename = styleFilename.front();
						bUseBaseStyle = true;
					}

					AsciiToUnicode(textureTileNames[wI], wstrCurrentName);
					wstrFilename += wstrCurrentName;
					bFoundImage = LoadTexture(wI, wstrFilename);
					if (!bFoundImage)
					{
						if (bUseBaseStyle)
							wstrFilename = styleFilename.front();
						else
							wstrFilename = wszDefaultStyleName;
						AsciiToUnicode(textureTileNames[wI == DEEP_MOSAIC ? PIT_MOSAIC : FLOOR_DIRT], wstrCurrentName);
						wstrFilename += wstrCurrentName;
						//If this fails, then we've exhausted all alternatives to show this texture.
						//Abort instead.
						if (!LoadTexture(wI, wstrFilename))
							return false;
					}
				}
			} else {
				WSTRING wstrFilename = wszDefaultStyleName;

				//Look up the base filename for this style and use that if defined.
				list<WSTRING> styleFilename;
				if (f.GetGameProfileString(INISection::Graphics, wszDefaultStyleName, styleFilename) &&
						!styleFilename.empty())
					wstrFilename = styleFilename.front();

				wstrFilename += wstrCurrentName;
				if (!LoadTexture(wI, wstrFilename))
					return false;
			}
		}
	}

	//Texture conversions: DEEP_MOSAIC and SHALLOW_MOSAIC must not have alpha channels
	SDL_SetSurfaceBlendMode(this->pTextures[DEEP_MOSAIC], SDL_BLENDMODE_NONE);
	SDL_SetSurfaceBlendMode(this->pTextures[SHALLOW_MOSAIC], SDL_BLENDMODE_NONE);

	//Texture restrictions: Pit side textures must be the same height.
	ASSERT(this->pTextures[PITSIDE_MOSAIC]->h == this->pTextures[PITSIDE_SMALL]->h);

	this->loadedStyle = style;

	return true;
}

//**********************************************************************************
SDL_Surface* CDrodBitmapManager::LoadImageSurface(
//Loads an image from the appropriate location into a new surface.
//
//Params:
	const WCHAR *wszName)   //(in)   Name of the image, not including extension.
{
	return LoadImageSurface(wszName, 0);
}

//**********************************************************************************
SDL_Surface* CDrodBitmapManager::LoadImageSurface(
//Loads an image from the appropriate location into a new surface.
//
//Params:
	const WCHAR *wszName,   //(in)   Name of the image, not including extension.
	const UINT dwHoldID)
//
//Returns:
//Loaded surface if successful, or NULL if not.
{
	ASSERT(wszName);
	ASSERT(WCSlen(wszName) > 1);

	//Search for image in data dir.
	SDL_Surface *pSurface = CBitmapManager::LoadImageSurface(wszName);
	if (pSurface)
		return pSurface;

	//Search for image with this name in the DB.
	if (!CDb::IsOpen())
		return NULL;	//DB not available

	CDb db;
	CIDSet imageFormats(DATA_BMP);
	imageFormats += DATA_JPG;
	imageFormats += DATA_PNG;

	db.Data.FilterByFormat(imageFormats);
	db.Data.FilterByHold(dwHoldID);
	const UINT dwDataID = db.Data.FindByName(wszName);
	return LoadImageSurface(dwDataID);
}

//**********************************************************************************
SDL_Surface* CDrodBitmapManager::LoadImageSurface(const UINT dwDataID)
//Loads an image from the database into a new surface.
{
	if (!dwDataID) return NULL;

	CDbDatum *pData = g_pTheDB->Data.GetByID(dwDataID);
	if (!pData) return NULL;

	SDL_Surface *pSurface = NULL;
	switch (pData->wDataFormat)
	{
		case DATA_BMP:
			pSurface = ConvertSurface(SDL_LoadBMP_RW(SDL_RWFromMem(
					(BYTE*)pData->data,pData->data.Size()), 0));
			break;
		case DATA_JPG:
			pSurface = LoadJPEGSurface((BYTE*)pData->data, pData->data.Size());
			break;
		case DATA_PNG:
			pSurface = LoadPNGSurface((BYTE*)pData->data, pData->data.Size());
			break;
		default:
			ASSERT(!"Unrecognized image format"); break;
	}

	delete pData;
	return pSurface;
}

//**********************************************************************************
BYTE CDrodBitmapManager::GetGammaOne()
//Returns: byte value corresponding to a gamma value of 1.0 in the pre-specified range
{
	return BYTE(255.0 * (1.0 - CDrodBitmapManager::fMinGamma) /
			(CDrodBitmapManager::fMaxGamma-CDrodBitmapManager::fMinGamma));
}

//**********************************************************************************
bool CDrodBitmapManager::SetGamma(const BYTE val)
//Sets gamma value based on pre-specified range.
//
//Returns: whether operation succeeded
{
	const float fGamma = CDrodBitmapManager::fMinGamma + float(val) *
			(CDrodBitmapManager::fMaxGamma-CDrodBitmapManager::fMinGamma)/255.0f;
	return CBitmapManager::SetGamma(fGamma);
}

//**********************************************************************************
UINT CDrodBitmapManager::CreateShallowWaterMix(
//Combines Shallow/Deep Water tile mix and associated masks onto a temporary surface.
//Returns a value indicating which masks were created.
// -1 == Failure
//  1 == Deep Water Mask created
//  2 == Shallow Water Mask created
//  3 == Both Masks created
//
//Params:
	const UINT wLandMask, const UINT wShallowMask, //(in) Land/Water and Deep/Shallow masks
	const UINT wDeepMix,            //(in) Used if we're making a concave Deep Water surface
	SDL_Surface *pDestSurface)      //(in/out) dest
{
	static const UINT CX_TILE_HALF = CX_TILE / 2;
	static const UINT CY_TILE_HALF = CY_TILE / 2;

	if (SDL_MUSTLOCK(pDestSurface))
	{
		if (SDL_LockSurface(pDestSurface) < 0)
		{
			ASSERT(!"CDrodBitmapManager::CreateShallowWaterMix -- Lock surface failed.");
			return -1;
		}
	}

	const bool bUseLandMask = wLandMask != TI_WATER_TOP;
	SDL_Surface *pLMaskSurface = NULL;
	if (bUseLandMask)
	{
		pLMaskSurface = GetTileSurface(wLandMask);
		ASSERT(pLMaskSurface);
	}
	SDL_Surface *pSMaskSurface = GetTileSurface(wDeepMix ? TI_SHALLOW_NSWE : wShallowMask);
	ASSERT(pSMaskSurface);
	SDL_Surface *pWaterSurface = GetTileSurface(TI_WATER_TOP);
	ASSERT(pWaterSurface);
	SDL_Surface *pShllwSurface = GetTileSurface(TI_SHALLOW_TOP);
	ASSERT(pShllwSurface);

	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wBPP == BYTES_PER_PIXEL);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif
	const UINT wLMaskBPP = bUseLandMask ? pLMaskSurface->format->BytesPerPixel : 0;
	const UINT wSMaskBPP = pSMaskSurface->format->BytesPerPixel;
	const UINT wWaterBPP = pWaterSurface->format->BytesPerPixel;
	const UINT wShllwBPP = pShllwSurface->format->BytesPerPixel;

	const UINT dwLMaskRowOffset = bUseLandMask ? pLMaskSurface->pitch - (CX_TILE * wLMaskBPP) : 0;
	const UINT dwSMaskRowOffset = pSMaskSurface->pitch - (CX_TILE * wSMaskBPP);
	const UINT dwWaterRowOffset = pWaterSurface->pitch - (CX_TILE * wWaterBPP);
	const UINT dwShllwRowOffset = pShllwSurface->pitch - (CX_TILE * wShllwBPP);

	const UINT wNextDest        = CY_TILE * pDestSurface->pitch;
	const UINT dwDestRowWidth   = CX_TILE * wBPP;
	const UINT dwDestRowOffset  = pDestSurface->pitch - dwDestRowWidth;

	Uint8 *pLMask = bUseLandMask ? GetTileSurfacePixel(wLandMask) + PIXEL_FUDGE_FACTOR : 0;
	Uint8 *pSMask = GetTileSurfacePixel(wDeepMix ? TI_SHALLOW_NSWE : wShallowMask) + PIXEL_FUDGE_FACTOR;
	Uint8 *pWater = GetTileSurfacePixel(TI_WATER_TOP) + PIXEL_FUDGE_FACTOR;
	Uint8 *pShllw = GetTileSurfacePixel(TI_SHALLOW_TOP) + PIXEL_FUDGE_FACTOR;

	//pDest  is the Water Mixture image {0, 0, CX_TILE, CY_TILE}
	//pDest2 is the Deep Water mask     {0, CY_TILE, CX_TILE, CY_TILE}
	//pDest3 is the Shallow Water mask  {0, 2*CY_TILE, CX_TILE, CY_TILE}
	Uint8 *pDest1 = (Uint8 *)pDestSurface->pixels + PIXEL_FUDGE_FACTOR;
	Uint8 *pDest2 = pDest1 + wNextDest;
	Uint8 *pDest3 = pDest2 + wNextDest;
	Uint8 *const pStop = pDest2;

	UINT wResult = 0;

	//If pixel in pSMask is Transparent,   add TI_SHALLOW_TOP to Mix
	//If pixel in pSMask is Black,         add TI_WATER_TOP to Mix
	//If pixel in pSMask is neither,       add pSMask to Mix
	
	//If pixel in pLMask is not Black,     add Transparent to both Shallow and Deep Water Masks
	//If pixel in pSMask is Transparent,   add Black to Shallow Water Mask
	//If pixel in pSMask is Black,         add Black to Deep Water Mask

	UINT wX, wY;
	wY = 0;
	while (pDest1 != pStop)
	{
		wX = 0;
		ASSERT(pDest1 < pStop);
		Uint8 *const pEndOfRow = pDest1 + dwDestRowWidth;

		while (pDest1 != pEndOfRow)
		{
			bool bLandMask = false;
			if (!bUseLandMask || (pLMask[0] == 0 && pLMask[1] == 0 && pLMask[2] == 0))
				bLandMask = true;
			else
			{
				pDest2[0] = pDest3[0] = TransColor[0];
				pDest2[1] = pDest3[1] = TransColor[1];
				pDest2[2] = pDest3[2] = TransColor[2];
			}

			bool bDeepWater = false;
			if (wDeepMix)
			{
				if (wY < CY_TILE_HALF)
				{
					if (wX < CX_TILE_HALF)
					{
						if (!(wDeepMix & 0x01)) bDeepWater = true;
					} else {
						if (!(wDeepMix & 0x02)) bDeepWater = true;
					}
				} else {
					if (wX < CX_TILE_HALF)
					{
						if (!(wDeepMix & 0x04)) bDeepWater = true;
					} else {
						if (!(wDeepMix & 0x08)) bDeepWater = true;
					}
				}
			}
			if (bDeepWater || (pSMask[0] == 0 && pSMask[1] == 0 && pSMask[2] == 0)) {
				//Mask Color means Deep Water
				pDest1[0] = pWater[0];
				pDest1[1] = pWater[1];
				pDest1[2] = pWater[2];
				if (bLandMask)
				{
					pDest2[0] = pDest2[1] = pDest2[2] = 0;
					pDest3[0] = TransColor[0];
					pDest3[1] = TransColor[1];
					pDest3[2] = TransColor[2];
					wResult |= 1;
				}
			} else {
				if (pSMask[0] == TransColor[0] && pSMask[1] == TransColor[1] && pSMask[2] == TransColor[2])
				{
					//Transparency means Shallow Water
					pDest1[0] = pShllw[0];
					pDest1[1] = pShllw[1];
					pDest1[2] = pShllw[2];
				} else {
					//Otherwise, Shallow Water but custom pixel
					pDest1[0] = pSMask[0];
					pDest1[1] = pSMask[1];
					pDest1[2] = pSMask[2];
				}
				if (bLandMask)
				{
					pDest2[0] = TransColor[0];
					pDest2[1] = TransColor[1];
					pDest2[2] = TransColor[2];
					pDest3[0] = pDest3[1] = pDest3[2] = 0;
					wResult |= 2;
				}
			}
			if (bUseLandMask) pLMask += wLMaskBPP;
			pSMask += wSMaskBPP;
			pWater += wWaterBPP;
			pShllw += wShllwBPP;
			pDest1 += wBPP;
			pDest2 += wBPP;
			pDest3 += wBPP;
			wX++;
		}
		if (bUseLandMask) pLMask += dwLMaskRowOffset;
		pSMask += dwSMaskRowOffset;
		pWater += dwWaterRowOffset;
		pShllw += dwShllwRowOffset;
		pDest1 += dwDestRowOffset;
		pDest2 += dwDestRowOffset;
		pDest3 += dwDestRowOffset;
		wY++;
	}

	if (SDL_MUSTLOCK(pDestSurface)) SDL_UnlockSurface(pDestSurface);

	return wResult;
}

//
// Protected methods.
//

//**********************************************************************************
SDL_Surface* CDrodBitmapManager::LoadJPEGSurface(
//Loads a JPEG image from the appropriate location into a new surface.
//
//Params:
	BYTE* imageBuffer, const UINT wSize)   //(in)   JPEG image data buffer
{
	BYTE *pImageBuffer;
	UINT wWidth, wHeight;
	if (!CJpegHandler::Decompress(imageBuffer, wSize, pImageBuffer, wWidth, wHeight))
		return NULL;
#if GAME_BYTEORDER == GAME_BYTEORDER_BIG
	SDL_Surface *pTempSurface = SDL_CreateRGBSurfaceFrom(pImageBuffer, wWidth, wHeight,
			24, wWidth * 3, 0xff0000, 0x00ff00, 0x0000ff, 0);
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
SDL_Surface* CDrodBitmapManager::LoadPNGSurface(
//Loads a PNG image from the appropriate location into a new surface.
//
//Params:
	BYTE* imageBuffer, const UINT wSize)   //(in)   PNG image data buffer
{
	if (!wSize) return NULL;
	return ConvertSurface(CPNGHandler::CreateSurface(imageBuffer, wSize));
}

//
// Private methods.
//

//**********************************************************************************
bool CDrodBitmapManager::LoadTexture(
//Loads an image by name into the texture enumeration.
//
//Params:
	const UINT wI, const WSTRING& wstrFilename)  //(in) enumeration, filename
{
	static SDL_Rect src = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
	static SDL_Rect dest = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
	SDL_Surface* &texture = this->pTextures[wI];
	if (texture)
		SDL_FreeSurface(texture);
	texture = LoadImageSurface(wstrFilename.c_str(), 0);
	if (!texture)
		return false;

	//Texture dimensions must be a multiple of the tile size.
	//Exception: pitside textures can have an arbitrary height.
	ASSERT((UINT)texture->w >= CX_TILE && (UINT)texture->w % CX_TILE == 0);
	ASSERT(((UINT)texture->h >= CY_TILE &&
			(UINT)texture->h % CY_TILE == 0) || (wI == PITSIDE_MOSAIC || wI == PITSIDE_SMALL));

	//Load a tile from these images for editor to display a sample.
	static const UINT textureTiles[TEXTURE_COUNT] = {
		TI_ROAD, TI_GRASS, TI_DIRT, TI_ALT, TI_FLOOR, TI_FLOOR_M,
		TI_PIT_M, TI_WALL_M, TI_SPIKE, TI_DONT_USE, TI_FLOOR_IMAGE,
		TI_DONT_USE, TI_DONT_USE, TI_OVERHEAD_IMAGE
	};
	const UINT textureTile = textureTiles[wI];
	if (textureTile != TI_DONT_USE) {
		dest.y = GetTileSurfaceY(textureTile);
		SDL_BlitSurface(texture, &src, GetTileSurface(textureTile), &dest);

		this->TileImageTypes[textureTile] = TIT_Opaque;
	}

	return true;
}

//**********************************************************************************
bool CDrodBitmapManager::LoadTileImages(
//Loads a tile image file.  If tile images that it contains are already loaded,
//the tile images will be overwritten in the tile images surface.
//
//Params:
	const WCHAR *wszName,         //(in)   Name of image without extension.
	CIDSet& styleTiles,     //(in/out) tiles loaded by this operation
	CIDSet *pLoadTileSet)   //(in) optional set that specifies the only tiles that are to be loaded [default=NULL]
//
//Returns:
//True if successful, false if not.
{
	if (pLoadTileSet && pLoadTileSet->empty())
		return true; //an empty tile set is to be loaded -- do nothing

	//Load the source bitmap containing tile images.
	UINT wCols, wRows;
	SDL_Surface *pSrcSurface = LoadImageSurface(wszName, 0);
	if (!pSrcSurface) return false;
	ASSERT(pSrcSurface->w % CX_TILE == 0);
	ASSERT(pSrcSurface->h % CY_TILE == 0);
	wCols = (pSrcSurface->w / CX_TILE);
	wRows = (pSrcSurface->h / CY_TILE);

	//Lock now to speed up lock/unlock pairs in called routines.
	UINT wSurfaceIndex = 0;
	for (wSurfaceIndex = CDrodBitmapManager::NUM_TILEIMAGESURFACES; wSurfaceIndex--; )
		LockTileImagesSurface(wSurfaceIndex);

	//Get mapping index from the tile image map file.  Each index specifies
	//which TI_* constant corresponds to the tile image within the source bitmap.
	list<UINT> MappingIndex;
	SDL_Rect src = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
	SDL_Rect dest = MAKE_SDL_RECT(0, 0, CX_TILE, CY_TILE);
	if (GetMappingIndexFromTileImageMap(wszName, MappingIndex))
	{
		list<UINT>::const_iterator iIndex = MappingIndex.begin();

		//Copy source tile images into the manager's tile image surface.  Their
		//position is determined from the mapping index.
		UINT wTileImageNo;
		for (UINT wY = 0; wY < wRows; ++wY)
		{
			for (UINT wX = 0; wX < wCols; ++wX)
			{
				wTileImageNo = *iIndex;
				if (wTileImageNo != (UINT)(TI_UNSPECIFIED) &&
						wTileImageNo < TI_COUNT)	//ensure this is true
				{
					//Only load tiles specified in this set if non-NULL.
					if (!pLoadTileSet || pLoadTileSet->has(wTileImageNo))
					{
						//Blit tile image to manager's tile image surface.
						SDL_Surface *pDestSurface = GetTileSurface(wTileImageNo);
						dest.y = GetTileSurfaceY(wTileImageNo);
						ASSERT(dest.y + CY_TILE <= static_cast<UINT>(pDestSurface->h));
						SDL_BlitSurface(pSrcSurface, &src, pDestSurface, &dest);

						//Set type of tile image.
						if (DoesTileImageContainTransparentPixels(wTileImageNo))
							this->TileImageTypes[wTileImageNo] = TIT_Transparent;
						else
							this->TileImageTypes[wTileImageNo] = TIT_Opaque;

						styleTiles += wTileImageNo; //output tiles that are being loaded
					}
				}
				++iIndex;
				if (iIndex == MappingIndex.end())
					break;

				src.x += CX_TILE;
			}
			if (iIndex == MappingIndex.end())
				break;

			src.x = 0;
			src.y += CY_TILE;
		}     
	}

	for (wSurfaceIndex = CDrodBitmapManager::NUM_TILEIMAGESURFACES; wSurfaceIndex--; )
		UnlockTileImagesSurface(wSurfaceIndex);

	SDL_FreeSurface(pSrcSurface);

	return true;
}

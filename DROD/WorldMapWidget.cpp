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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2012
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "WorldMapWidget.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"
#include "DrodSound.h"
#include "TileImageCalcs.h"

#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/DbHolds.h"
#include "../DRODLib/DbLevels.h"
#include "../DRODLib/DbRooms.h"
#include "../DRODLib/Db.h"

#include <FrontEndLib/EventHandlerWidget.h>
#include <FrontEndLib/ImageWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/TilesWidget.h>
#include <BackEndLib/Assert.h>

#include <string>
using std::string;

const UINT ANIMATED_TILE = static_cast<UINT>(-1);

const UINT s_fontType = F_WorldMapLevel;

//******************************************************************************
CWorldMapWidget::CWorldMapWidget(const UINT dwSetTagNo,
		const int nSetX, const int nSetY, const UINT wSetW, const UINT wSetH)
	: CWidget(WIDGETTYPE(WT_WorldMap), dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, pWorldMapImage(NULL)
	, pLocationImages(NULL)
	, dwLastAnimate(0)
	, pClickedIcon(NULL), pHighlightedIcon(NULL)
{
	this->imageFilenames.push_back(string("LevelStartBackground"));

	AddImageWidget(this->pWorldMapImage);

	this->pLocationImages = new CImageVectorWidget(0);
	this->pLocationImages->Disable(); //not clickable
	AddWidget(this->pLocationImages);

	for (UINT frame=0; frame<NUM_WORLD_MAP_FRAMES; ++frame) {
		AddTilesWidget(this->pTilesWidget[frame]);
	}

	AddTilesWidget(this->pAnimatedCustomCharTilesWidget);

	AddTilesWidget(this->pOverlayTilesWidget);
}

//*****************************************************************************
CWorldMapWidget::~CWorldMapWidget()
//Destructor.
{ 
	ClearMapImages();
}

//******************************************************************************
void CWorldMapWidget::AddImageWidget(CImageWidget* &pImageWidget)
{
	pImageWidget = new CImageWidget(0, 0, 0, (SDL_Surface*)NULL);
	pImageWidget->Disable(); //not clickable
	AddWidget(pImageWidget);
}

void CWorldMapWidget::AddTilesWidget(CTilesWidget* &pTilesWidget)
{
	pTilesWidget = new CTilesWidget(0, 0, 0, GetW(), GetH());
	pTilesWidget->Disable(); //not clickable
	AddWidget(pTilesWidget);
}

//******************************************************************************
void CWorldMapWidget::HandleAnimate()
{
	const Uint32 dwNow = SDL_GetTicks();
	this->dwLastAnimate = dwNow;

	const Uint32 animationRate = 500; //ms
	const Uint32 animationCycleDuration = animationRate * NUM_WORLD_MAP_FRAMES;
	const UINT currentFrame = (dwNow % animationCycleDuration) / animationRate;
	for (UINT frame=0; frame<NUM_WORLD_MAP_FRAMES; ++frame) {
		this->pTilesWidget[frame]->Show(frame == currentFrame);
	}

	//Each animation frame, determine display tiles for animated custom characters.
	this->pAnimatedCustomCharTilesWidget->ClearTiles();
	for (vector<AnimatedCharIcon>::iterator iter=this->animatedChars.begin();
			iter!=this->animatedChars.end(); ++iter)
	{
		const UINT animationFrame = dwNow / iter->animationSpeed;
		const UINT customTileImageNo = g_pTheBM->GetCustomTileNo(
				iter->dataID_Tiles, 0, animationFrame, true);
		if (customTileImageNo)
			this->pAnimatedCustomCharTilesWidget->AddTile(
					customTileImageNo, iter->icon.xPos, iter->icon.yPos);
	}
}

//******************************************************************************
void CWorldMapWidget::Paint(bool bUpdateRect) //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//Drawing code below needs to be modified to accept offsets.
	//Until then, this widget can't be offset.
	ASSERT(!IsScrollOffset());

	SDL_Surface *pDestSurface = GetDestSurface();

	//Blit the background graphic, if needed.
	if (this->pWorldMapImage->GetW() < GetW() ||
			this->pWorldMapImage->GetH() < GetH()) {
		SDL_Rect Dest = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
		SDL_BlitSurface(this->images[0], NULL, pDestSurface, &Dest);
	}

	PaintChildren(bUpdateRect);

	const CLabelWidget *pHighlightedIconLabel = this->pHighlightedIcon ?
		this->pHighlightedIcon->pLabel : NULL;
	for (vector<CLabelWidget*>::iterator iter=this->labels.begin();
			iter!=this->labels.end(); ++iter)
	{
		CLabelWidget *pLabel = *iter;
		const bool bHighlighted = pLabel == pHighlightedIconLabel;
		if (bHighlighted) {
			g_pTheDFM->SetFontColor(s_fontType, Yellow);
			pLabel->SetText(pLabel->GetText().c_str()); //rerender
		}

		pLabel->SetDestSurface(pDestSurface);
		pLabel->Paint(bUpdateRect);

		if (bHighlighted) { //revert
			g_pTheDFM->SetFontColor(s_fontType, AlmostWhite);
			pLabel->SetText(pLabel->GetText().c_str());
		}
	}

	if (bUpdateRect) UpdateRect();
}

//******************************************************************************
void CWorldMapWidget::HandleMouseMotion(const SDL_MouseMotionEvent &Motion)
{
	const WorldMapIconUI *pIcon = GetIconAt(Motion.x, Motion.y);
	if (pIcon != this->pHighlightedIcon) {
		if (pIcon)
			g_pTheSound->PlaySoundEffect(SEID_WORLDMAP_CLICK);

		this->pHighlightedIcon = pIcon;
		RequestPaint();
	}
}

void CWorldMapWidget::HandleMouseUp(
//Called when the mouse is clicked.
//
//Params:
   const SDL_MouseButtonEvent &Button) //(in) Event to handle.
{
	const WorldMapIconUI *pIcon = GetIconAt(Button.x, Button.y);
	if (pIcon) {
		this->pClickedIcon = pIcon;

		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
	}
}

const CWorldMapWidget::WorldMapIconUI* CWorldMapWidget::GetIconAt(UINT x, UINT y)
{
	//Some icons might be overlapping others,
	//so consider the ones drawn on top first.
	for (vector<WorldMapIconUI>::reverse_iterator iter=this->icons.rbegin();
			iter!=this->icons.rend(); ++iter)
	{
		const WorldMapIconUI& icon = *iter;
		ASSERT(icon.imageID == 0);
		if ((icon.xPos <= x) && (x < icon.xPos + CBitmapManager::CX_TILE) &&
			(icon.yPos <= y) && (y < icon.yPos + CBitmapManager::CY_TILE))
		{
			return &icon;
		}
	}

	//Run through images if no icon is found at this location
	for (vector<WorldMapImage>::reverse_iterator iter=this->mapImages.rbegin();
			iter!=this->mapImages.rend(); ++iter)
	{
		const WorldMapImage& image = *iter;
		if (image.icon.displayFlags != ScriptFlag::WMI_ON &&
				image.icon.displayFlags != ScriptFlag::WMI_LOCKED)
			continue;
		if ((image.icon.xPos <= x) && (x < image.icon.xPos + image.width) &&
			(image.icon.yPos <= y) && (y < image.icon.yPos + image.height))
		{
			return &(image.icon);
		}
	}

	return NULL;
}

//*****************************************************************************
void CWorldMapWidget::SetCurrentGame(CCurrentGame *pCurrentGame)
{
	ClearIcons();

	if (!pCurrentGame)
		return;

	ASSERT(pCurrentGame->worldMapID);

	//Set background image.
	const UINT dataID = pCurrentGame->pHold->GetWorldMapDataID(pCurrentGame->worldMapID);
	SDL_Surface *pSurface = g_pTheDBM->LoadImageSurface(dataID);
	this->pWorldMapImage->SetImage(pSurface);

	//Center on screen.
	const UINT width = this->pWorldMapImage->GetW();
	const UINT height = this->pWorldMapImage->GetH();
	const int xPos = width >= GetW() ? 0 : (GetW() - width) / 2;
	const int yPos = height >= GetH() ? 0 : (GetH() - height) / 2;
	this->pWorldMapImage->Move(xPos, yPos);

	AddIcons(pCurrentGame);
}

//*****************************************************************************
void CWorldMapWidget::AddIcons(CCurrentGame *pCurrentGame)
{
	WorldMapsIcons::const_iterator iter = pCurrentGame->worldMapIcons.find(pCurrentGame->worldMapID);
	if (iter == pCurrentGame->worldMapIcons.end())
		return;

	const HoldWorldMap::DisplayType displayType =
			pCurrentGame->pHold->GetWorldMapDisplayType(pCurrentGame->worldMapID);

	const WorldMapIcons& icons = iter->second;
	for (WorldMapIcons::const_iterator iconIt=icons.begin();
			iconIt!=icons.end(); ++iconIt)
	{
		const WorldMapIcon& icon = *iconIt;
		const bool bImage = icon.imageID != 0;
		ASSERT(!bImage || !icon.charID);
		UINT wIconW = CBitmapManager::CX_TILE;
		UINT wIconH = CBitmapManager::CY_TILE;

		bool clickable = true;
		bool showlabel = true;
		UINT coveringIcon = 0;
		switch (icon.displayFlags) {
			case ScriptFlag::WMI_OFF: ASSERT(!"Invalid display flag"); break;
			case ScriptFlag::WMI_ON: break;
			case ScriptFlag::WMI_LEVELSTATE:
			{
				ASSERT(!bImage);
				if (LevelExit::IsWorldMapID(icon.entranceID))
					break;
				CEntranceData *pEntrance = pCurrentGame->pHold->GetEntrance(icon.entranceID);
				if (!pEntrance)
					break;

				if (!pCurrentGame->ExploredRooms.has(pEntrance->dwRoomID)) {
					coveringIcon = TI_WORLDMAP_NEW;
				} else {
					const UINT dwLevelID = CDbRooms::GetLevelIDForRoom(pEntrance->dwRoomID);

					CIDSet requiredRooms;
					CDbLevels::GetRequiredRooms(dwLevelID, requiredRooms);
					if (pCurrentGame->ConqueredRooms.contains(requiredRooms))
						coveringIcon = TI_WORLDMAP_CLEAR;
					else
						coveringIcon = TI_WORLDMAP_IN_PROGRESS;
				}
			}
				break;
			case ScriptFlag::WMI_NOLABEL:
				clickable = false;
				showlabel = false;
				break;
			case ScriptFlag::WMI_DISABLED:
				clickable = false;
				coveringIcon = TI_CHECKPOINT;
				break;
			case ScriptFlag::WMI_LOCKED:
				coveringIcon = TI_WORLDMAP_LOCK;
				break;
		}

		//Check for dangling IDs
		const bool bEntranceIsWorldMap = LevelExit::IsWorldMapID(icon.entranceID);
		if (bEntranceIsWorldMap) {
			if (!pCurrentGame->pHold->DoesWorldMapExist(
					LevelExit::ConvertWorldMapID(icon.entranceID))) {
				clickable = false;
				coveringIcon = TI_CHECKPOINT_L;
			}
		} else {
			CEntranceData *pEntrance = pCurrentGame->pHold->GetEntrance(icon.entranceID);
			if (!pEntrance) {
				clickable = false;
				coveringIcon = TI_CHECKPOINT_L;
			}
		}

		if (bImage)	{
			SDL_Surface *pSurface = g_pTheDBM->LoadImageSurface(icon.imageID);
			//If image doesn't exist, then we can't do anything more with this icon
			if (!pSurface) continue;
			this->pLocationImages->AddImage(pSurface, icon.xPos, icon.yPos);

			WorldMapImage image(icon);
			image.pSurface = pSurface;
			image.width = wIconW = pSurface->w;
			image.height = wIconH = pSurface->h;

			this->mapImages.push_back(image);
		}

		if (!bImage && clickable) {
			this->icons.push_back(icon);
		}

		if (showlabel)
		{
			CLabelWidget *pLabel = AddLabel(icon, displayType, wIconW, wIconH, pCurrentGame);
			if (clickable)
			{
				if (bImage)
					this->mapImages.back().icon.pLabel = pLabel;
				else
					this->icons.back().pLabel = pLabel;
			}
		}

		if (!bImage) {
			AddTile(icon, coveringIcon, pCurrentGame);
		}
	}
}

//*****************************************************************************
CLabelWidget* CWorldMapWidget::AddLabel(
	const WorldMapIcon& icon,
	HoldWorldMap::DisplayType displayType,
	UINT wIconW, UINT wIconH,
	CCurrentGame *pCurrentGame)
{
	if (displayType == HoldWorldMap::NoLabels)
		return NULL;

	WSTRING wstrLabelText = wszQuestionMark;

	const bool bEntranceIsWorldMap = LevelExit::IsWorldMapID(icon.entranceID);
	if (bEntranceIsWorldMap) {
		wstrLabelText = pCurrentGame->pHold->GetWorldMapName(
			LevelExit::ConvertWorldMapID(icon.entranceID));
	} else {
		bool bShowName = true;
		CEntranceData *pEntrance = pCurrentGame->pHold->GetEntrance(icon.entranceID);
		if (pEntrance) {
			if (displayType == HoldWorldMap::LabelsWhenExplored)
				bShowName = pCurrentGame->ExploredRooms.has(pEntrance->dwRoomID);

			if (bShowName) {
				const UINT dwLevelID = CDbRooms::GetLevelIDForRoom(pEntrance->dwRoomID);
				wstrLabelText = CDbLevels::GetLevelName(dwLevelID);
			}
		}
	}

	//Get rendered text width for positioning.
	UINT wTextW, wTextH;
	g_pTheFM->GetTextWidthHeight(s_fontType, wstrLabelText.c_str(), wTextW, wTextH);

	//Center under icon.
	int xPos = icon.xPos + wIconW/2 - wTextW/2;
	if (xPos + wTextW > GetW())
		xPos = GetW() - wTextW;
	if (xPos < 0)
		xPos = 0;

	static const UINT outlineHeightHack = 5;
	CLabelWidget *pLabel = new CLabelWidget(0,
		xPos, icon.yPos + wIconH,
		wTextW, wTextH + outlineHeightHack,
		s_fontType, wstrLabelText.c_str(), false, 0, WT_Label, true, 0);

	this->labels.push_back(pLabel); //for easy cleanup
	return pLabel;
}

//*****************************************************************************
void CWorldMapWidget::AddTile(
	const WorldMapIcon& icon, UINT coveringIcon, CCurrentGame *pCurrentGame)
{
	for (UINT frame=0; frame<NUM_WORLD_MAP_FRAMES; ++frame) {
		const UINT tileNo = GetTileFor(icon.charID, frame, pCurrentGame);
		if (tileNo == ANIMATED_TILE) {
			const HoldCharacter *pChar = pCurrentGame->pHold->GetCharacter(icon.charID);
			this->animatedChars.push_back(AnimatedCharIcon(icon, pChar->dwDataID_Tiles, pChar->animationSpeed));
		} else {
			this->pTilesWidget[frame]->AddTile(tileNo, icon.xPos, icon.yPos);
		}

		if (coveringIcon)
			this->pOverlayTilesWidget->AddTile(coveringIcon, icon.xPos, icon.yPos);
	}
}

//*****************************************************************************
void CWorldMapWidget::ClearIcons()
{
	for (UINT frame=0; frame<NUM_WORLD_MAP_FRAMES; ++frame) {
		this->pTilesWidget[frame]->ClearTiles();
	}
	this->pOverlayTilesWidget->ClearTiles();
	this->pAnimatedCustomCharTilesWidget->ClearTiles();

	ClearLabels();

	this->icons.clear();
	ClearMapImages();

	this->animatedChars.clear();

	this->pClickedIcon = this->pHighlightedIcon = NULL;
}

//*****************************************************************************
void CWorldMapWidget::ClearLabels()
{
	for (vector<CLabelWidget*>::iterator iter=this->labels.begin();
			iter!=this->labels.end(); ++iter)
		RemoveWidget((CWidget*)(*iter));
	this->labels.clear();
}

//*****************************************************************************
void CWorldMapWidget::ClearMapImages()
{
	this->pLocationImages->ClearImages();

	this->mapImages.clear();
}

//*****************************************************************************
UINT CWorldMapWidget::GetTileFor(
	UINT charID, UINT frame, CCurrentGame *pCurrentGame)
const
{
	static const UINT orientation = S;
	if (charID >= CUSTOM_CHARACTER_FIRST) {
		const HoldCharacter *pChar = pCurrentGame->pHold->GetCharacter(charID);
		if (!pChar)
			return GetTileImageForEntityOrDefault(M_CITIZEN1, orientation, frame);

		if (pChar->animationSpeed && pChar->dwDataID_Tiles)
			return ANIMATED_TILE; //calculated during real-time animation

		const UINT tile = g_pTheBM->GetCustomTileNo(pChar->dwDataID_Tiles, orientation, frame);
		if (tile != TI_UNSPECIFIED)
			return tile;

		//Use stock/default character tileset.
		return GetTileImageForEntity(pChar->wType == M_NONE ?
				static_cast<UINT>(CHARACTER_FIRST) : pChar->wType, orientation, frame);
	}

	return GetTileImageForEntityOrDefault(charID, orientation, frame);
}

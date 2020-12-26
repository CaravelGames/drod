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

#include "MapWidget.h"
#include "RoomWidget.h"
#include "DrodSound.h"
#include <FrontEndLib/EventHandlerWidget.h>
#include <FrontEndLib/ScrollableWidget.h>

#include "../DRODLib/GameConstants.h"
#include "../DRODLib/TileConstants.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbRooms.h"
#include "../DRODLib/DbLevels.h"
#include <BackEndLib/Types.h>
#include <BackEndLib/Assert.h>
#include <BackEndLib/Files.h>

//Map color indexes.
enum MapColor
{
	MAP_BACKGROUND,
	MAP_LTBACKGROUND,
	MAP_FLOOR,
	MAP_GOO,
	MAP_STAIRS,
	MAP_BLACK,
	MAP_DKGRAY,
	MAP_MIDGRAY1,
	MAP_GRAY,
	MAP_MIDGRAY2,
	MAP_LTGRAY,
	MAP_BLUE,
	MAP_DKCYAN2,
	MAP_DKRED,
	MAP_RED,
	MAP_MEDRED,
	MAP_LTRED,
	MAP_MAGENTA,
	MAP_BROWN,
	MAP_DKORANGE,
	MAP_DIM_ORANGE,
	MAP_ORANGE,
	MAP_MEDBLUE,
	MAP_LTBLUE,
	MAP_LTBLUE1,
	MAP_LTBLUE2,
	MAP_LTBLUE3,
	MAP_DKGREEN,
	MAP_MIDGREEN,
	MAP_GREEN,
	MAP_LTCYAN,
	MAP_LTMAGENTA,
	MAP_LTYELLOW,
	MAP_LTGREEN,
	MAP_PALERED,
	MAP_PALEGREEN,
	MAP_PALEBLUE,
	MAP_PALECYAN,
	MAP_PALEYELLOW,
	MAP_WHITE,

	MAP_COLOR_COUNT
};

static SURFACECOLOR m_arrColor[MAP_COLOR_COUNT];

//
//Public methods.
//

//*****************************************************************************
CMapWidget::CMapWidget(
//Constructor.
//
//Params:
	UINT dwSetTagNo,                //(in)   Required params for CWidget 
	int nSetX, int nSetY,               //    constructor.
	UINT wSetW, UINT wSetH,             //
	const CCurrentGame *pSetCurrentGame)   //(in) Game to use for drawing the map.
	: CFocusWidget((WIDGETTYPE)WT_Map, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, bVacantRoom(false)
	, bUserMoveable(true)
	, dwClickedRoomX(0), dwClickedRoomY(0)
	, dwLevelID(0L)
	, bIsLevelComplete(false)
	, pCurrentGame(pSetCurrentGame)
	, pLevel(NULL)
	, pMapSurface(NULL)
	, dwSelectedRoomX(0L), dwSelectedRoomY(0L)
	, dwLeftRoomX(0L), dwTopRoomY(0L), dwRightRoomX(0L), dwBottomRoomY(0L)
	, wLastSrcX(0), wLastSrcY(0), wBorderW(0), wBorderH(0)
	, bScrollHorizontal(false), bScrollVertical(false)
	, bPendingExitDrawn(false)
	, bEditing(false), bCopyingRoom(false)
	, bDeletingRoom(false)
	, pRoom(NULL)
{
	//Can't display a map smaller than one room.
	ASSERT(wSetW >= CDrodBitmapManager::DISPLAY_COLS);
	ASSERT(wSetH >= CDrodBitmapManager::DISPLAY_ROWS);
}

//*****************************************************************************
CMapWidget::~CMapWidget()
//Destructor.
{ 
	ASSERT(!this->bIsLoaded);
	ClearState();
}

//*****************************************************************************
void CMapWidget::ClearMap()
//Reset the level so that the map draws nothing.
{
	this->pLevel = NULL;
	RequestPaint();
}

//*****************************************************************************
void CMapWidget::ClearState()
//Call on Unload() to force state reset for reload.
{
	if (this->pMapSurface) 
	{
		SDL_FreeSurface(this->pMapSurface);
		this->pMapSurface = NULL;
	}
	this->pCurrentGame = NULL;
	this->pLevel = NULL;
	this->bEditing = false;
	
	delete this->pRoom;
	this->pRoom = NULL;
}

//*****************************************************************************
void CMapWidget::CopyRoom(const bool bCopy)
//Perform copy of selected room, if any.
{
	if (!this->bEditing || !this->pLevel) return;
	CDbRoom *pCutRoom = g_pTheDB->Rooms.GetByCoords(
			this->pLevel->dwLevelID, this->dwSelectedRoomX, this->dwSelectedRoomY);
	if (!pCutRoom) return;

	//A room exists here to cut/copy -- save it.
	delete this->pRoom;
	this->pRoom = pCutRoom;
	this->bCopyingRoom = bCopy;
	g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);
}

//*****************************************************************************
void CMapWidget::GetSelectedRoomXY(UINT &dwRoomX, UINT &dwRoomY) const
//OUT: selected room's logical coordinates.
{
	dwRoomX = this->dwSelectedRoomX;
	dwRoomY = this->dwSelectedRoomY;
}

//*****************************************************************************
void CMapWidget::GetSelectedRoomRect(SDL_Rect &rect) const
//OUT: selected room's physical pixel area on map surface
{
	rect.x = (Sint16)(this->wBorderW + ((this->dwSelectedRoomX - this->dwLeftRoomX) * 
				CDrodBitmapManager::DISPLAY_COLS)) - 1;
	rect.y = (Sint16)(this->wBorderH + ((this->dwSelectedRoomY - this->dwTopRoomY) * 
				CDrodBitmapManager::DISPLAY_ROWS)) - 1;
	rect.w = CDrodBitmapManager::DISPLAY_COLS + 2;
	rect.h = CDrodBitmapManager::DISPLAY_ROWS + 2;
}

//******************************************************************************
void CMapWidget::GetVisibleMapRect(SDL_Rect &rect) const
//OUT: visible level's physical pixel area on map surface
{
	//Get bounds of explored rooms in level.
	bool bFirstRoom = true;
	list<CDbRoom *>::const_iterator iSeek;
	CIDSet roomIDs = CDb::getRoomsInLevel(this->pLevel->dwLevelID);
	UINT dwLeft=0, dwRight=0, dwTop=0, dwBottom=0;
	for (CIDSet::const_iterator iter=roomIDs.begin(); iter != roomIDs.end(); ++iter)
	{
		CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(*iter, true);  //load only coord data
		const bool bIncludeRoom = this->bEditing ||
				this->DarkenedRooms.has(*iter) ||
				this->pCurrentGame->IsRoomAtCoordsExplored(pRoom->dwRoomX, pRoom->dwRoomY);

		if (bIncludeRoom)
		{
			//Look for expansion of visible level boundaries.
			if (bFirstRoom)
			{
				dwLeft = dwRight = pRoom->dwRoomX;
				dwBottom = dwTop = pRoom->dwRoomY;
				bFirstRoom = false;
			} else {
				if (pRoom->dwRoomX < dwLeft)
					dwLeft = pRoom->dwRoomX;
				else if (pRoom->dwRoomX > dwRight)
					dwRight = pRoom->dwRoomX;
				if (pRoom->dwRoomY < dwTop)
					dwTop = pRoom->dwRoomY;
				else if (pRoom->dwRoomY > dwBottom)
					dwBottom = pRoom->dwRoomY;
			}
		}
		delete pRoom;
	}

	rect.x = (Sint16)(this->wBorderW - 1 + (CDrodBitmapManager::DISPLAY_COLS * (dwLeft - this->dwLeftRoomX)));
	rect.y = (Sint16)(this->wBorderH - 1 + (CDrodBitmapManager::DISPLAY_ROWS * (dwTop - this->dwTopRoomY)));
	rect.w = (Uint16)(CDrodBitmapManager::DISPLAY_COLS * (dwRight - dwLeft + 1) + 2);
	rect.h = (Uint16)(CDrodBitmapManager::DISPLAY_ROWS * (dwBottom - dwTop + 1) + 2);
}

//******************************************************************************
void CMapWidget::HandleKeyDown(
//Processes a keyboard event within the scope of the widget.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
{
	if (!this->bUserMoveable)
		return;

	const SDL_Keycode key = KeyboardEvent.keysym.sym;
	UINT dwRoomX = this->dwSelectedRoomX, dwRoomY = this->dwSelectedRoomY;

	switch (key) {
		//Moving selection to an adjacent room
		case SDLK_DOWN: case SDLK_KP_2:
			if (KeyboardEvent.keysym.mod & KMOD_CTRL) return;
			if (dwRoomY < (this->bEditing ? this->dwBottomRoomY + 1 : this->dwBottomRoomY))
				++dwRoomY;
			break;
		case SDLK_LEFT: case SDLK_KP_4:
			if (KeyboardEvent.keysym.mod & KMOD_CTRL) return;
			if (dwRoomX > (this->bEditing ? this->dwLeftRoomX - 1 : this->dwLeftRoomX))
				--dwRoomX;
			break;
		case SDLK_RIGHT: case SDLK_KP_6:
			if (KeyboardEvent.keysym.mod & KMOD_CTRL) return;
			if (dwRoomX < (this->bEditing ? this->dwRightRoomX + 1 : this->dwRightRoomX))
				++dwRoomX;
			break;
		case SDLK_UP: case SDLK_KP_8:
			if (KeyboardEvent.keysym.mod & KMOD_CTRL) return;
			if (dwRoomY > (this->bEditing ? this->dwRightRoomX - 1 : this->dwTopRoomY))
				--dwRoomY;
			break;

		//Pasting selected room
		case SDLK_v:
			this->bDeletingRoom = false;
			if (!this->bEditing || !this->pLevel || !this->pRoom) return;
			if (KeyboardEvent.keysym.mod & KMOD_CTRL)
			{
				if (this->pRoom->dwRoomX == dwRoomX && this->pRoom->dwRoomY == dwRoomY)
					return;  //putting room at same spot -- don't need to do anything

				this->bDeletingRoom = !this->bVacantRoom;
				//PasteRoom() called through owner screen.
			}
			return;

		default:
			return;
	}

	if (SelectRoomIfValid(dwRoomX,dwRoomY))
	{
		RequestPaint();
		
		//Call OnSelectChange() notifier.
		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) pEventHandler->OnSelectChange(this->GetTagNo());
	}
}

//******************************************************************************
void CMapWidget::HandleMouseDown(
//Processes a mouse event within the scope of the widget.
//
//Params:
	const SDL_MouseButtonEvent &MouseButtonEvent)   //(in) Event to handle.
{
	if (!this->bUserMoveable)
	{
		//Keep track of what room was clicked on in case the parent is interested.
		GetRoomAtCoords(MouseButtonEvent.x, MouseButtonEvent.y, this->dwClickedRoomX, this->dwClickedRoomY);
		return;
	}

	const UINT dwPrevRoomX = this->dwSelectedRoomX,
			dwPrevRoomY = this->dwSelectedRoomY;

	SelectRoomAtCoords(MouseButtonEvent.x, MouseButtonEvent.y);
	RequestPaint();

	if (dwPrevRoomX != this->dwSelectedRoomX || 
			dwPrevRoomY != this->dwSelectedRoomY)
	{
		//Call OnSelectChange() notifier.
		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
	}
}

//*****************************************************************************
void CMapWidget::HighlightRoom(SDL_Rect& dest, const UINT wWidth) //[default=1]
//Draw rectangle of specified width around selected room (dest).
{
	ASSERT(wWidth);
	const BYTE boxColor = (IsSelected() || !this->bEditing ? MAP_ORANGE : MAP_LTGRAY);
	DrawRect(dest, m_arrColor[boxColor]);
	for (UINT width=wWidth-1; width--; )
	{
		--dest.x;
		--dest.y;
		dest.w += 2;
		dest.h += 2;
		DrawRect(dest, m_arrColor[boxColor]);
	}
}

//*****************************************************************************
void CMapWidget::Resize(const UINT wSetW, const UINT wSetH)
//Override CWidget::Resize() to keep map surface set up correctly for new
//size.
{
	if (wSetW == this->w && wSetH == this->h) return;

	//Can't display a map smaller than one room.
	ASSERT(wSetW >= CDrodBitmapManager::DISPLAY_COLS);
	ASSERT(wSetH >= CDrodBitmapManager::DISPLAY_ROWS);

	CWidget::Resize(wSetW, wSetH);

	//Don't allow recursive call to Resize, since LoadMapSurface() below can call Resize().
	if (this->bResizing) return;
	this->bResizing = true;

	ClipWHToDest();

	//Reload the map surface.
	LoadMapSurface();

	this->bResizing = false;
}

//*****************************************************************************
bool CMapWidget::LoadFromCurrentGame(
//Sets current game used by map.
//
//Params:
	const CCurrentGame *pSetCurrentGame,   //(in) New current game.
	const bool bDrawSurface)   //(in) if true [default], draw rooms onto map surface
//
//Returns:
//True if successful, false if not.
{
	ASSERT(pSetCurrentGame);
	this->pCurrentGame = pSetCurrentGame;
	this->pLevel = this->pCurrentGame->pLevel;   //easy access
	this->bEditing = false;

	//Reload the map surface.
	if (bDrawSurface)
	{
		if (!LoadMapSurface(true))	//always provide margin
			return false;
		//Select the current room.
		SelectRoom(this->pCurrentGame->pRoom->dwRoomX, this->pCurrentGame->pRoom->dwRoomY);
	}

	//Now that map is drawn, set vars that reflect state of current game in map.
	this->dwLevelID = this->pLevel->dwLevelID;
	this->bIsLevelComplete = this->pCurrentGame->IsCurrentLevelComplete();
	this->ExploredRooms = this->pCurrentGame->ExploredRooms;
	this->ConqueredRooms = this->pCurrentGame->ConqueredRooms;

	return true;
}

//*****************************************************************************
bool CMapWidget::LoadFromLevel(
//Sets current game used by map.
//
//Params:
	CDbLevel *pLevel) //(in) Level to load from.
//
//Returns:
//True if successful, false if not.
{
	ASSERT(pLevel);
	this->pLevel = pLevel;
	this->pCurrentGame = NULL;
	this->bEditing = true;

	//Reload the map surface.
	if (!LoadMapSurface()) 
		return false;

	//Set vars that show current state of map compared to current game.
	this->dwLevelID = pLevel->dwLevelID;
	this->bIsLevelComplete = false;  //show original state of level

	//Select the first room in the level.
	CIDSet roomIDs = CDb::getRoomsInLevel(this->pLevel->dwLevelID);
	for (CIDSet::const_iterator room = roomIDs.begin(); room != roomIDs.end(); ++room)
	{
		CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(*room, true);
		if (pRoom)
		{
			SelectRoom(pRoom->dwRoomX, pRoom->dwRoomY);
			delete pRoom;
			break;
		}
	}

	//Can't cut room from a different hold, only copy.
	if (this->pRoom && !this->bCopyingRoom &&
			g_pTheDB->Levels.GetHoldIDForLevel(this->pRoom->dwLevelID) != pLevel->dwHoldID)
		this->bCopyingRoom = true;

	return true;
}

//*****************************************************************************
void CMapWidget::UpdateFromCurrentGame()
//Update data so that it matches current game.
{
	ASSERT(this->pCurrentGame);
	ASSERT(this->pCurrentGame->pRoom);
	ASSERT(this->pMapSurface);
	
	UpdateMapSurface(true);
	SelectRoom(this->pCurrentGame->pRoom->dwRoomX, this->pCurrentGame->pRoom->dwRoomY);
}

//*****************************************************************************
void CMapWidget::UpdateFromCurrentLevel()
//Update map to reflect the latest in-game level state.
{
	// No current game if using editor
	if (this->pCurrentGame)
	{
		//Make sure level pointer is current
		if (this->pLevel == this->pCurrentGame->pLevel)
		{
			//Don't need to reload the map -- just update it.
			UpdateMapSurface();
			return;
		}
		this->pLevel = this->pCurrentGame->pLevel;
	}
	LoadMapSurface();
}

//*****************************************************************************
void CMapWidget::Paint(
//Blits the map to the widget area on the screen surface.
//
//Params:
	bool bUpdateRect)       //(in)   If true (default) and destination
								//    surface is the screen, the screen
								//    will be immediately updated in
								//    the widget's rect.
{
	if (!this->pLevel)
	{
		DrawPlaceholder();
		return;
	}

	if (!this->pMapSurface) return;

	//Figure part of map surface that will go in widget area.
	if (this->bScrollHorizontal)
	{
		const UINT wCenterX = this->wBorderW + (this->dwSelectedRoomX - this->dwLeftRoomX) *
				CDrodBitmapManager::DISPLAY_COLS + (CDrodBitmapManager::DISPLAY_COLS / 2);
		this->wLastSrcX = wCenterX - (this->w / 2);
	}
	else
		this->wLastSrcX = 0;
	if (this->bScrollVertical)
	{
		const UINT wCenterY = this->wBorderH + (this->dwSelectedRoomY - this->dwTopRoomY) * 
				CDrodBitmapManager::DISPLAY_ROWS + (CDrodBitmapManager::DISPLAY_ROWS / 2);
		this->wLastSrcY = wCenterY - (this->h / 2);
	}
	else
		this->wLastSrcY = 0;

	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);

	//Blit map surface to screen.
	SDL_Surface *pDestSurface = GetDestSurface();
	SDL_Rect src = MAKE_SDL_RECT(this->wLastSrcX, this->wLastSrcY, this->w, this->h);
	SDL_Rect dest = MAKE_SDL_RECT(this->x + nOffsetX, this->y + nOffsetY, this->w, this->h);
	SDL_BlitSurface(this->pMapSurface, &src, pDestSurface, &dest);

	//Draw selected room rectangle if widget area is large enough.
	if (this->w >= CDrodBitmapManager::DISPLAY_COLS + 2 &&
			this->h >= CDrodBitmapManager::DISPLAY_ROWS + 2)
	{
		//Figure coords for rectangle.
		dest.w = CDrodBitmapManager::DISPLAY_COLS + 2;
		dest.h = CDrodBitmapManager::DISPLAY_ROWS + 2;
		if (this->bScrollHorizontal)
			dest.x = this->x + nOffsetX + (this->w - CDrodBitmapManager::DISPLAY_COLS) / 2 - 1;
		else
			dest.x = static_cast<Sint16>(this->x + nOffsetX + this->wBorderW + 
				((this->dwSelectedRoomX - this->dwLeftRoomX) * 
				CDrodBitmapManager::DISPLAY_COLS) - 1);
		if (this->bScrollVertical)
			dest.y = this->y + nOffsetY + (this->h - CDrodBitmapManager::DISPLAY_ROWS) / 2 - 1;
		else
			dest.y = static_cast<Sint16>(this->y + nOffsetY + this->wBorderH + 
				((this->dwSelectedRoomY - this->dwTopRoomY) * 
				CDrodBitmapManager::DISPLAY_ROWS) - 1);

		bool bHighlightRoom = true;
		if (this->pParent && this->pParent->GetType() == WT_Scrollable)
		{
			SDL_Rect clipRect;
			this->pParent->GetChildClippingRect(clipRect);
			bHighlightRoom = ClipRectToRect(dest, clipRect);
		}

		if (bHighlightRoom)
			HighlightRoom(dest, 2);
	}

	if (bUpdateRect) UpdateRect();
}

//*****************************************************************************
bool CMapWidget::PasteRoom(CDbHold *pHold)
//Pastes this->pRoom to the selected room spot on the map.
//It might have a room there or not.  If so, delete the old one.
//
//If entrance room was cut-and-pasted, update level's entrance room position.
//
//OUT: pHold (hold's entrance list and ScriptID Updated() here)
//ATTN: This makes any instances of level/hold records for this room obsolete,
//and they should be discarded without calls to Update().
//
//Returns: whether a room was pasted
{
	ASSERT(pHold);
	ASSERT(this->pLevel->dwHoldID == pHold->dwHoldID);

	this->bDeletingRoom = false;
	if (!this->bEditing || !this->pLevel || !this->pRoom)
		return false;

	g_pTheSound->PlaySoundEffect(SEID_DOOROPEN);

	const UINT dwRoomX = this->dwSelectedRoomX, dwRoomY = this->dwSelectedRoomY;
	const bool bChangingLevels = this->dwLevelID != this->pRoom->dwLevelID;
	if (!bChangingLevels && this->pRoom->dwRoomX == dwRoomX && this->pRoom->dwRoomY == dwRoomY)
		return false;  //putting room at same spot -- don't need to do anything

	//Check whether level entrances are being moved.
	CIDSet entranceIDs;
	g_pTheDB->Holds.GetEntranceIDsForRoom(this->pRoom->dwRoomID, entranceIDs);
	const bool bMovingLevelEntrance = !this->bCopyingRoom && entranceIDs.size();

	//Is room being moved to a level in a different hold
	// OR
	//Is the level's main entrance room being removed? (Not allowed -- change cut operation to a copy.)
	bool bChangingHolds = false;
	UINT uSrcHoldID = 0;
	if (bChangingLevels)
	{
		CDbLevel *pSrcLevel = g_pTheDB->Levels.GetByID(this->pRoom->dwLevelID, true);
		if (!pSrcLevel)           //if copied room's level is already deleted,
			bChangingHolds = true; //assume the room was from a different hold
		else
		{
			uSrcHoldID = pSrcLevel->dwHoldID;
			if (pHold->dwHoldID != uSrcHoldID)
				bChangingHolds = true;

			if (bMovingLevelEntrance)
			{
				//Is the level's main entrance room being removed to another level?
				UINT dwSX, dwSY;
				pSrcLevel->GetStartingRoomCoords(dwSX, dwSY);
				if (this->pRoom->dwRoomX == dwSX && this->pRoom->dwRoomY == dwSY)
				{
					this->bCopyingRoom = true; //Yes -- not allowed -- perform a copy operation instead of a cut.
					g_pTheSound->PlaySoundEffect(SEID_POTION); //audial cue
				}
			}

			delete pSrcLevel;
		}
	}

	//If copying room, must copy level entrances also.
	CImportInfo info;
	ENTRANCE_VECTOR entrances;
	if (this->bCopyingRoom)
		g_pTheDB->Holds.GetEntrancesForRoom(this->pRoom->dwRoomID, entrances);

	if (this->bVacantRoom)
	{
		//Place room at empty spot.
		this->bVacantRoom = false;
		if (!this->bCopyingRoom)
		{
			//Move room.
			this->pRoom->dwRoomX = dwRoomX;
			this->pRoom->dwRoomY = dwRoomY;
			if (bChangingLevels)
			{
				this->pRoom->dwLevelID = this->dwLevelID;
				this->pRoom->ResetExitIDs();
				if (bMovingLevelEntrance)
					MarkEntrancesAsNotMain(pHold, entranceIDs);
				ASSERT(!bChangingHolds); //this->bCopyingRoom should have been set in this case
				if (bChangingHolds) //robustness guard
					this->pRoom->dwDataID = 0; //reset dangling floor image ID
			}
			this->pRoom->Update();
			this->bCopyingRoom = true; //further pastes will now copy room
		} else {
			//Copy room.
			CDbRoom *pNewRoom = this->pRoom->MakeCopy(info, pHold->dwHoldID);
			if (!pNewRoom) return false;
			pNewRoom->dwLevelID = this->dwLevelID;
			pNewRoom->dwRoomID = 0;   //so new room gets added to DB
			pNewRoom->dwRoomX = dwRoomX;
			pNewRoom->dwRoomY = dwRoomY;
			if (bChangingLevels)
				pNewRoom->ResetExitIDs(); //might call Update

			pNewRoom->Update();
			MergeHoldData(pHold, pNewRoom, entrances, uSrcHoldID, info);
			delete this->pRoom;
			this->pRoom = pNewRoom; //next room copy uses this room to avoid pasting to this location again
		}
	} else {
		{
			//Delete room at this spot.
			CDbRoom *pRoomHere = g_pTheDB->Rooms.GetByCoords(
					this->pLevel->dwLevelID, dwRoomX, dwRoomY);
			ASSERT(pRoomHere);

			//Delete level entrances contained in the room being erased.
			//NOTE: This is also performed in the call to Delete the room, but
			//we want to update *this* copy of the hold record with the changes.
			pHold->DeleteEntrancesForRoom(pRoomHere->dwRoomID);

			g_pTheDB->Rooms.Delete(pRoomHere->dwRoomID);
			if (!this->bCopyingRoom)
				delete pRoomHere;
			else 
			{
				pRoomHere->MakeCopy(*this->pRoom);
				//Copy custom room media.
				CImportInfo unneeded;
				if (bChangingHolds) {
					CDbData::CopyObject(unneeded, pRoomHere->dwDataID, pHold->dwHoldID);
					CDbData::CopyObject(unneeded, pRoomHere->dwOverheadDataID, pHold->dwHoldID);
				}

				delete this->pRoom;
				this->pRoom = pRoomHere;
			}
		}
		//Update room data.
		this->pRoom->dwRoomX = dwRoomX;
		this->pRoom->dwRoomY = dwRoomY;
		this->pRoom->dwLevelID = this->dwLevelID;
		if (bChangingLevels)
			this->pRoom->ResetExitIDs();

		this->pRoom->Update();
		if (this->bCopyingRoom)
		{
			MergeHoldData(pHold, this->pRoom, entrances, uSrcHoldID, info);
		} else {
			this->bCopyingRoom = true; //further pastes will now copy room
		}
	}

	//If entrance room was moved, update level extrance room coords.
	if (bMovingLevelEntrance)
		this->pLevel->bGotStartingRoomCoords = false;

	//Update map.
	LoadMapSurface();

	//Call OnSelectChange() notifier.
	CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
	if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());

	return true;
}

//
//Protected methods.
//

//*****************************************************************************
void CMapWidget::SetDarkenedRooms(
//Set rooms that will appear as darkened.
//
//Params:
	CIDSet &RoomIDs)  //(in) Rooms to darken.
{
	this->DarkenedRooms = RoomIDs;
	LoadMapSurface();
	//Select the current room.
	SelectRoom(this->pCurrentGame->pRoom->dwRoomX, this->pCurrentGame->pRoom->dwRoomY);
}

//*****************************************************************************
void CMapWidget::SetDestSurface(
//Calls CWidget method and fixes this object after the new surface has been set.
//
//Params:
	SDL_Surface *pNewSurface)
{
	CWidget::SetDestSurface(pNewSurface);
	if (IsLoaded()) 
		//The new surface may have different surface colors.
		InitMapColors();
}

//
//Private methods.
//

//*****************************************************************************
void CMapWidget::GetRoomAtCoords(const int nX, const int nY, UINT& dwRoomX, UINT& dwRoomY)
//OUT: the coordinates of the room located at (x,y) (in pixels) on the map surface.
{
	ASSERT(ContainsCoords(nX, nY));
	if (!this->pMapSurface) return;

	//Figure where coords are in relation to source surface.
	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);
	const UINT wSrcX = this->wLastSrcX + (nX - (this->x + nOffsetX));
	const UINT wSrcY = this->wLastSrcY + (nY - (this->y + nOffsetY));
	ASSERT(static_cast<int>(wSrcX) < this->pMapSurface->w);
	ASSERT(static_cast<int>(wSrcY) < this->pMapSurface->h);

	//Figure which room corresponds to source coords.
	const int offsetX = (int)wSrcX - (int)this->wBorderW;
	const int offsetY = (int)wSrcY - (int)this->wBorderH;

	dwRoomX = (int)this->dwLeftRoomX + (offsetX / (int)CDrodBitmapManager::DISPLAY_COLS);
	dwRoomY = (int)this->dwTopRoomY + (offsetY / (int)CDrodBitmapManager::DISPLAY_ROWS);
	if (offsetX < 0)
		--dwRoomX;
	if (offsetY < 0)
		--dwRoomY;
}

//*****************************************************************************
bool CMapWidget::IsAdjacentToValidRoom(
//Returns: true if room coords are adjacent to an existing room, else false
//
//Accepts:
  const UINT dwRoomX, const UINT dwRoomY)  //(in) Room #
{
	if (g_pTheDB->Rooms.FindIDAtCoords(this->pLevel->dwLevelID, dwRoomX-1, dwRoomY))
		return true;
	if (g_pTheDB->Rooms.FindIDAtCoords(this->pLevel->dwLevelID, dwRoomX+1, dwRoomY))
		return true;
	if (g_pTheDB->Rooms.FindIDAtCoords(this->pLevel->dwLevelID, dwRoomX, dwRoomY-1))
		return true;
	if (g_pTheDB->Rooms.FindIDAtCoords(this->pLevel->dwLevelID, dwRoomX, dwRoomY+1))
		return true;

	//No room exists on any side.
	return false;
}

//*****************************************************************************
void CMapWidget::SelectRoom(
//Sets the room that will be selected.
//When using the editor, a vacant spot can be selected.
//
//Accepts:
  const UINT dwRoomX, const UINT dwRoomY)
{
	ASSERT(this->bEditing ||
		IsInRect(dwRoomX, dwRoomY,
			this->dwLeftRoomX, this->dwTopRoomY, this->dwRightRoomX, this->dwBottomRoomY));
	this->bVacantRoom = false; //set to true elsewhere

	this->dwSelectedRoomX = dwRoomX;
	this->dwSelectedRoomY = dwRoomY;

	if (this->pParent && this->pParent->GetType() == WT_Scrollable)
	{
		//Center scrolling container on selected room.
		CScrollableWidget *pContainer = DYN_CAST(CScrollableWidget*, CWidget*, this->pParent);
		const UINT x = (dwRoomX - this->dwLeftRoomX) *
				CDrodBitmapManager::DISPLAY_COLS + CDrodBitmapManager::DISPLAY_COLS / 2 +
				this->wBorderW;
		const UINT y = (dwRoomY - this->dwTopRoomY) *
				CDrodBitmapManager::DISPLAY_ROWS + CDrodBitmapManager::DISPLAY_ROWS / 2 +
				this->wBorderH;
		pContainer->CenterOn(x,y);
	}
}

//*****************************************************************************
bool CMapWidget::SelectRoomIfValid(
//If room exists and has been explored, select it.
//
//Returns: true if valid room was selected, else false
//
//Accepts:
  const UINT dwRoomX, const UINT dwRoomY)  //(in) Room #
{
	//General bounds check.
	if (!this->bEditing &&
			!IsInRect(dwRoomX, dwRoomY,
				this->dwLeftRoomX, this->dwTopRoomY, this->dwRightRoomX, this->dwBottomRoomY))
		return false;

	//Does room exist?
	if (!this->pLevel) return false;
	const UINT dwRoomID = g_pTheDB->Rooms.FindIDAtCoords(this->pLevel->dwLevelID,
			dwRoomX, dwRoomY);
	if (dwRoomID) //Yes.
	{
		//Has it been explored?
		if (this->bEditing || this->ExploredRooms.has(dwRoomID) ||
				this->DarkenedRooms.has(dwRoomID)) //Yes.
		{
			//Select the room.
			SelectRoom(dwRoomX, dwRoomY);
			return true;
		}
	} else {
		if (this->bEditing && IsAdjacentToValidRoom(dwRoomX, dwRoomY))
		{
			//Enforce coords only in [1..99] range.
			if (dwRoomX < 1 || dwRoomX > 99) return false;
			if ((dwRoomY % 100) == 0) return false;   //handles upper and lower bounds
			//Select vacant spot anyway.  New room can be placed there.
			SelectRoom(dwRoomX, dwRoomY);
			this->bVacantRoom = true;
			return true;
		}
	}
	return false;
}

//*****************************************************************************
void CMapWidget::SelectRoomAtCoords(
//Sets the selected room to one currently displayed within widget area 
//containing specified coords.  If coords are invalid, room selection will not
//change.
//
//Accepts:
  const int nX, const int nY) //(in) Coords within widget area.
{
	ASSERT(ContainsCoords(nX, nY));
	if (!this->pMapSurface) return;

	//Figure where coords are in relation to source surface.
	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);
	const UINT wSrcX = this->wLastSrcX + (nX - (this->x + nOffsetX));
	const UINT wSrcY = this->wLastSrcY + (nY - (this->y + nOffsetY));
	ASSERT(static_cast<int>(wSrcX) < this->pMapSurface->w);
	ASSERT(static_cast<int>(wSrcY) < this->pMapSurface->h);

	//Figure which room corresponds to source coords.
	const int offsetX = (int)wSrcX - (int)this->wBorderW;
	const int offsetY = (int)wSrcY - (int)this->wBorderH;
	UINT dwRoomX = (int)this->dwLeftRoomX + (offsetX / (int)CDrodBitmapManager::DISPLAY_COLS),
			dwRoomY = (int)this->dwTopRoomY + (offsetY / (int)CDrodBitmapManager::DISPLAY_ROWS);
	if (offsetX < 0) --dwRoomX;
	if (offsetY < 0) --dwRoomY;

	if (this->dwSelectedRoomX != dwRoomX || this->dwSelectedRoomY != dwRoomY)
		SelectRoomIfValid(dwRoomX, dwRoomY);
}

//**********************************************************************************
void CMapWidget::InitMapColors()
//Set values in color array used for drawing map pixels.
//
//Returns:
//Pointer to new structure of NULL for failure.
{
	//One call is enough, since m_arrColor is static module-scoped.
	static bool bInitOnce = false;
	if (bInitOnce) return;
	bInitOnce = true;

	//Get surface-writable bytes for the colors used for map pixels.
	m_arrColor[MAP_BACKGROUND] = GetSurfaceColor(this->pMapSurface, 102, 51, 51);
	m_arrColor[MAP_LTBACKGROUND] = GetSurfaceColor(this->pMapSurface, 102, 70, 70);
	m_arrColor[MAP_FLOOR] =    GetSurfaceColor(this->pMapSurface, 229, 229, 229);
	m_arrColor[MAP_GOO] =      GetSurfaceColor(this->pMapSurface, 0, 192, 0);
	m_arrColor[MAP_STAIRS] =   GetSurfaceColor(this->pMapSurface, 210, 210, 100);
	m_arrColor[MAP_BLACK] =    GetSurfaceColor(this->pMapSurface, 0,  0,  0);
	m_arrColor[MAP_DKGRAY] =   GetSurfaceColor(this->pMapSurface, 64,64,64);
	m_arrColor[MAP_MIDGRAY1] = GetSurfaceColor(this->pMapSurface, 96, 96, 96);
	m_arrColor[MAP_GRAY] =     GetSurfaceColor(this->pMapSurface, 128,128,128);
	m_arrColor[MAP_MIDGRAY2] = GetSurfaceColor(this->pMapSurface, 160,160,160);
	m_arrColor[MAP_LTGRAY] =   GetSurfaceColor(this->pMapSurface, 200,200,200);
	m_arrColor[MAP_BLUE] =     GetSurfaceColor(this->pMapSurface, 0,  0,  128);
	m_arrColor[MAP_DKCYAN2] =  GetSurfaceColor(this->pMapSurface, 0,  64, 64);
	m_arrColor[MAP_DKRED] =    GetSurfaceColor(this->pMapSurface, 128,0,  0);
	m_arrColor[MAP_RED] =      GetSurfaceColor(this->pMapSurface, 255,0,  0);
	m_arrColor[MAP_MEDRED] =   GetSurfaceColor(this->pMapSurface, 255,64, 64);
	m_arrColor[MAP_LTRED] =    GetSurfaceColor(this->pMapSurface, 255,128,128);
	m_arrColor[MAP_MAGENTA] =  GetSurfaceColor(this->pMapSurface, 128,0,  128);
	m_arrColor[MAP_BROWN] =    GetSurfaceColor(this->pMapSurface, 128,128,0);
	m_arrColor[MAP_DKORANGE] = GetSurfaceColor(this->pMapSurface, 128,64, 0);
	m_arrColor[MAP_DIM_ORANGE]=GetSurfaceColor(this->pMapSurface, 192,96, 0);
	m_arrColor[MAP_ORANGE] =   GetSurfaceColor(this->pMapSurface, 255,128,0);
	m_arrColor[MAP_MEDBLUE] =  GetSurfaceColor(this->pMapSurface, 0,  0,  192);
	m_arrColor[MAP_LTBLUE] =   GetSurfaceColor(this->pMapSurface, 0,  0,  255);
	m_arrColor[MAP_LTBLUE1] =  GetSurfaceColor(this->pMapSurface, 64, 96, 255);
	m_arrColor[MAP_LTBLUE2] =  GetSurfaceColor(this->pMapSurface, 96, 144,255);
	m_arrColor[MAP_LTBLUE3] =  GetSurfaceColor(this->pMapSurface, 128,192,255);
	m_arrColor[MAP_DKGREEN] =  GetSurfaceColor(this->pMapSurface, 0,  128,0);
	m_arrColor[MAP_MIDGREEN] = GetSurfaceColor(this->pMapSurface, 0,  196,0);
	m_arrColor[MAP_GREEN] =    GetSurfaceColor(this->pMapSurface, 0,  255,0);
	m_arrColor[MAP_LTCYAN] =   GetSurfaceColor(this->pMapSurface, 0,  255,255);
	m_arrColor[MAP_LTMAGENTA] =GetSurfaceColor(this->pMapSurface, 255,0,  255);
	m_arrColor[MAP_LTYELLOW] = GetSurfaceColor(this->pMapSurface, 255,255,0);
	m_arrColor[MAP_LTGREEN] =  GetSurfaceColor(this->pMapSurface, 128,255,128);
	m_arrColor[MAP_PALERED] =  GetSurfaceColor(this->pMapSurface, 255,200,200);
	m_arrColor[MAP_PALEGREEN] =GetSurfaceColor(this->pMapSurface, 200,255,200);
	m_arrColor[MAP_PALEBLUE] = GetSurfaceColor(this->pMapSurface, 200,200,255);
	m_arrColor[MAP_PALECYAN] = GetSurfaceColor(this->pMapSurface, 164,255,255);
	m_arrColor[MAP_PALEYELLOW]=GetSurfaceColor(this->pMapSurface, 255,255,164);
	m_arrColor[MAP_WHITE] =    GetSurfaceColor(this->pMapSurface, 255,255,255);
}

//*****************************************************************************
bool CMapWidget::LoadMapSurface(
//Creates map surface resource containing representation of all rooms in level.
//
//Params:
	const bool bForceMargin)	//true when buffer should be shown [default=false]
{
	// Delete the surface if it exists
	if (this->pMapSurface)
	{
		SDL_FreeSurface(this->pMapSurface);
		this->pMapSurface = NULL;
	}

	const bool bInScrollWindow = CanResize();
	ASSERT(!bInScrollWindow || this->pParent);

	if (!this->pLevel)
	{
		//No level to display.
		if (bInScrollWindow)   //shrink to size of container widget
			CWidget::Resize(0, 0);
		//DrawPlaceholder();
		return true;
	}

	CIDSet roomIDs = CDb::getRoomsInLevel(this->pLevel->dwLevelID);
	if (roomIDs.empty())
	{
		//No rooms to display.
		if (bInScrollWindow)  //shrink to size of container widget
			CWidget::Resize(0, 0);
		DrawPlaceholder();
		return true;
	}

	//Load every room in the current level.
	bool bFirstRoom = true;
	list<CDbRoom *> DrawRooms;
	list<CDbRoom *>::const_iterator iSeek;
	for (CIDSet::const_iterator iter=roomIDs.begin(); iter != roomIDs.end(); ++iter)
	{
		CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(*iter, true);  //load only coord data
		const bool bIncludeRoom = this->bEditing ||
				this->DarkenedRooms.has(*iter) ||
				this->pCurrentGame->IsRoomAtCoordsExplored(pRoom->dwRoomX, pRoom->dwRoomY);

		if (!bInScrollWindow || bIncludeRoom)
		{
			//Look for expansion of visible level boundaries.
			if (bFirstRoom)
			{
				this->dwLeftRoomX = this->dwRightRoomX = pRoom->dwRoomX;
				this->dwBottomRoomY = this->dwTopRoomY = pRoom->dwRoomY;
				bFirstRoom = false;
			} else {
				if (pRoom->dwRoomX < this->dwLeftRoomX)
					this->dwLeftRoomX = pRoom->dwRoomX;
				else if (pRoom->dwRoomX > this->dwRightRoomX)
					this->dwRightRoomX = pRoom->dwRoomX;
				if (pRoom->dwRoomY < this->dwTopRoomY)
					this->dwTopRoomY = pRoom->dwRoomY;
				else if (pRoom->dwRoomY > this->dwBottomRoomY)
					this->dwBottomRoomY = pRoom->dwRoomY;
			}
		}

		if (!bIncludeRoom)
			delete pRoom;
		else
		{
			//Keep the rooms that have been explored in a list.
			pRoom->LoadTiles();  //now load tile data
			DrawRooms.push_back(pRoom);
		}
	}

	//Currently, a room's y-coordinate contains a pseudo-level encoding in its 100s place.
	//Therefore, a level's rooms should not have y-coords that overflow into to the next level's room coords.
	ASSERT(this->dwBottomRoomY - this->dwTopRoomY + 1 <= 100);

	//Determine dimensions of map for level and border size.  Border is so 
	//that I can scroll map to edge rooms without worrying about the display.
	const UINT wMapW = CDrodBitmapManager::DISPLAY_COLS *
			(this->dwRightRoomX - this->dwLeftRoomX + 1) + 2;
	const UINT wMapH = CDrodBitmapManager::DISPLAY_ROWS *
			(this->dwBottomRoomY - this->dwTopRoomY + 1) + 2;
	if (bInScrollWindow)
	{
		this->bScrollHorizontal = this->bScrollVertical = false;
		if (this->bEditing)
		{
			//In editor, provide space for adding new rooms on the edge.
			this->wBorderW = CDrodBitmapManager::DISPLAY_COLS + 2;
			this->wBorderH = CDrodBitmapManager::DISPLAY_ROWS + 2;
		} else {
			//Add room for highlighting border.
			this->wBorderW = this->wBorderH = 2;
		}
		//Expand map to fill container.
		ASSERT(this->pParent->GetType() == WT_Scrollable);
		CScrollableWidget *pContainer = DYN_CAST(CScrollableWidget*, CWidget*, this->pParent);
		SDL_Rect rect;
		pContainer->GetRectAvailableForChildren(rect, wMapW + this->wBorderW * 2,
				wMapH + this->wBorderH * 2);
		if ((UINT)rect.w > wMapW + this->wBorderW * 2)
			this->wBorderW = (rect.w - wMapW) / 2;
		if ((UINT)rect.h > wMapH + this->wBorderH * 2)
			this->wBorderH = (rect.h - wMapH) / 2;
	} else {
		//always scroll so as to be able to add empty rooms on the edges in the level editor.
		this->bScrollHorizontal = bForceMargin || (wMapW > this->w) || this->bEditing;
		this->bScrollVertical = bForceMargin || (wMapH > this->h) || this->bEditing;
		if (this->bScrollHorizontal)
			this->wBorderW = this->w - (this->bEditing ? 0 : CDrodBitmapManager::DISPLAY_COLS);
		else
			this->wBorderW = (this->w - wMapW) / 2;
		if (this->bScrollVertical)
			this->wBorderH = this->h - (this->bEditing ? 0 : CDrodBitmapManager::DISPLAY_ROWS);
		else
			this->wBorderH = (this->h - wMapH) / 2;
	}

	//Create the surface
	bool bSuccess = true;
	this->pMapSurface = g_pTheBM->ConvertSurface(SDL_CreateRGBSurface(SDL_SWSURFACE, 
			wMapW + (this->wBorderW * 2), wMapH + (this->wBorderH * 2),
			g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	if (!this->pMapSurface)
	{
		CFiles Files;
		Files.AppendErrorLog("CMapWidget::LoadMapSurface()--SDL_CreateRGBSurface() failed.");
		bSuccess = false;
		goto Cleanup;
	}
	//Get colors for drawing on map surface.
	InitMapColors();

	//If map is in a scrollable container, make it as large as it needs to be.
	if (bInScrollWindow)
		CWidget::Resize(this->pMapSurface->w, this->pMapSurface->h);

	//Fill map surface with background color.
#ifdef GAME_RENDERING_OFFSET
	SDL_FillRect(this->pMapSurface,NULL,SDL_MapRGB(
			this->pMapSurface->format,
			m_arrColor[MAP_BACKGROUND].byt1,
			m_arrColor[MAP_BACKGROUND].byt2,
			m_arrColor[MAP_BACKGROUND].byt3
	));
#else
	SDL_FillRect(this->pMapSurface,NULL,m_arrColor[MAP_BACKGROUND].byt3 << 16 |
			m_arrColor[MAP_BACKGROUND].byt2 << 8 | m_arrColor[MAP_BACKGROUND].byt1);
#endif

	//Draw each room onto the map.
	for (iSeek = DrawRooms.begin(); iSeek != DrawRooms.end(); ++iSeek)
		DrawMapSurfaceFromRoom(*iSeek);

Cleanup:
	for (iSeek = DrawRooms.begin(); iSeek != DrawRooms.end(); ++iSeek)
		delete *iSeek;

	return bSuccess;
}

//*****************************************************************************
void CMapWidget::UpdateMapSurface(const bool bRefreshSelectedRoom)	//[default=false]
//Updates map surface with any differences between map and current game.
{
	bool bDrawCurrentRoom = false;

	//If level changed, then a complete reload is needed.
	ASSERT(this->pCurrentGame);
	if (this->pCurrentGame->pLevel->dwLevelID != this->dwLevelID)
	{
		LoadFromCurrentGame(this->pCurrentGame);
		return;
	}

	//See if all the current game conquered rooms are shown in map.
	ASSERT(this->pCurrentGame->pRoom);
	if (this->pCurrentGame->ConqueredRooms.size() != 
			this->ConqueredRooms.size())
	{
		for (CIDSet::const_iterator iter=this->pCurrentGame->ConqueredRooms.begin();
				iter != this->pCurrentGame->ConqueredRooms.end(); ++iter)
		{
			if (!this->ConqueredRooms.has(*iter))
			{
				//A new conquered room to update my map with.
				this->ConqueredRooms += *iter;
				if (this->pCurrentGame->pRoom->dwRoomID == *iter)
					bDrawCurrentRoom = true;
				else
				{
					//Load the room in.
					CDbRoom *pTempRoom = g_pTheDB->Rooms.GetByID(*iter, true);
					if (pTempRoom)
					{
						pTempRoom->LoadTiles();
						DrawMapSurfaceFromRoom(pTempRoom);
						delete pTempRoom;
					}
					else
						ASSERT(!"Failed to retrieve room");
				}
			}
		}
	} //...if conquered room list sizes don't match.
	
	//See if all the current game explored rooms are shown in map.
	if (this->pCurrentGame->ExploredRooms.size() != 
			this->ExploredRooms.size())
	{
		for (CIDSet::const_iterator iter=this->pCurrentGame->ExploredRooms.begin();
				iter != this->pCurrentGame->ExploredRooms.end(); ++iter)
		{
			if (!this->ExploredRooms.has(*iter))
			{
				//A new explored room to update my map with.
				this->ExploredRooms += *iter;

				//Draw the room if it isn't in the conquered list (already drawn).
				if (!this->ConqueredRooms.has(*iter))
				{
					if (this->pCurrentGame->pRoom->dwRoomID == *iter)
						bDrawCurrentRoom = true;
					else
					{
						//Load the room in.
						CDbRoom *pTempRoom = g_pTheDB->Rooms.GetByID(*iter, true);
						if (pTempRoom)
						{
							pTempRoom->LoadTiles();
							DrawMapSurfaceFromRoom(pTempRoom);
							delete pTempRoom;
						}
						else
							ASSERT(!"Failed to retrieve room");
					}
				}             
			}
		}
	}

	//Used to force redraw of the room just left before a new room is selected.
	//This is done to ensure it is shown in its original state, and not the
	//state the room was in when it was left.
	if (bRefreshSelectedRoom)
	{
		CDbRoom *pSelectedRoom = g_pTheDB->Rooms.GetByCoords(
				this->dwLevelID, this->dwSelectedRoomX, this->dwSelectedRoomY);
		if (pSelectedRoom)
		{
			DrawMapSurfaceFromRoom(pSelectedRoom);
			delete pSelectedRoom;
		}
	}

	//Need to redraw the current room if an exit is pending.
	const bool bPendingExit = (this->pCurrentGame ?
			this->pCurrentGame->IsCurrentRoomPendingExit() : false);
	if (bPendingExit != this->bPendingExitDrawn) {
		bDrawCurrentRoom = true;
		this->bPendingExitDrawn = bPendingExit;
	}

	//New room is the current room.
	if (bDrawCurrentRoom)
		DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom);
}

//*****************************************************************************
void CMapWidget::DrawMapSurfaceFromRoom(
//Loads a room from disk into a specified position in the map surface.
//
//Params:
  const CDbRoom *pRoom) //(in)   Contains coords of room to update on map
						//    as well as the squares of the room to use
						//    in determining pixels.
{
	ASSERT(pRoom);

	const bool bRoomIsCurrentRoom = this->pCurrentGame &&
			pRoom->dwRoomID == this->pCurrentGame->pRoom->dwRoomID;

	//Get variables that affect how map pixels are set.
	const bool bRoomRequired = pRoom->bIsRequired;
	const bool bRoomSecret = pRoom->bIsSecret;
	bool bConquered, bPendingConquer, bDarkened;

	//When there is no current game, then show everything fully.
	if (this->pCurrentGame) {
		if (bRoomIsCurrentRoom && pRoom->IsBeaconActive() && !this->pCurrentGame->AreBeaconsIgnored()) {
			bConquered = false;
		} else {
			bConquered = this->pCurrentGame->IsRoomAtCoordsConquered(pRoom->dwRoomX, pRoom->dwRoomY);
		}
		bDarkened = this->DarkenedRooms.has(pRoom->dwRoomID);
		bPendingConquer = bRoomIsCurrentRoom &&
			this->pCurrentGame->IsCurrentRoomPendingConquer();
	} else {
		bConquered = true;
		bPendingConquer = bDarkened = false;
	}

	//Set colors in map surface to correspond to squares in the room.
	LockMapSurface();

	SURFACECOLOR Color;
	static const UINT wBPP = this->pMapSurface->format->BytesPerPixel;
	ASSERT(wBPP >= 3);
	const UINT dwRowOffset = this->pMapSurface->pitch - (CDrodBitmapManager::DISPLAY_COLS * wBPP);
	Uint8 *pSeek = GetRoomStart(pRoom->dwRoomX, pRoom->dwRoomY);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(this->pMapSurface->format->Rmask == 0xff0000);
	ASSERT(this->pMapSurface->format->Gmask == 0x00ff00);
	ASSERT(this->pMapSurface->format->Bmask == 0x0000ff);
#endif
#ifdef GAME_RENDERING_OFFSET
	pSeek += wBPP-3;  // a crude hack.  the first byte is unused.  start one byte over.
#endif
	Uint8 *pStop = pSeek + (this->pMapSurface->pitch * CDrodBitmapManager::DISPLAY_ROWS);
	UINT wSquareIndex=0;

	//Each iteration draws one row.
	while (pSeek != pStop)
	{
		ASSERT(pSeek < pStop);
		Uint8 *pEndOfRow = pSeek + (CDrodBitmapManager::DISPLAY_COLS * wBPP);

		//Each iteration draws one pixel.
		while (pSeek != pEndOfRow)
		{
			Color = GetMapColorFromTile(
				(unsigned char) pRoom->pszOSquares[wSquareIndex],
				(unsigned char) pRoom->GetTSquare(wSquareIndex),
				bConquered, bDarkened, bPendingConquer, bRoomRequired, bRoomSecret);
			pSeek[0] = Color.byt1;
			pSeek[1] = Color.byt2;
			pSeek[2] = Color.byt3;

			++wSquareIndex;
			pSeek += wBPP;
		}
		pSeek += dwRowOffset;
	}
			
	UnlockMapSurface();
}

//*****************************************************************************
inline void CMapWidget::LockMapSurface()
//Simplified call to SDL_LockSurface().
{
	ASSERT(this->pMapSurface);
	if (SDL_MUSTLOCK(this->pMapSurface)) 
		if (SDL_LockSurface(this->pMapSurface) < 0) 
			ASSERT(!"Failed to lock surface.");
}

//*****************************************************************************
inline void CMapWidget::UnlockMapSurface()
//Simplified call to SDL_LockSurface().
{
	ASSERT(this->pMapSurface);
	if (SDL_MUSTLOCK(this->pMapSurface))
		SDL_UnlockSurface(this->pMapSurface);
}

//*****************************************************************************
inline SURFACECOLOR CMapWidget::GetMapColorFromTile(
//Returns the map image color that corresponds to a given tile#.
//
//Accepts:
	const UINT wOpaqueTile, 
	const UINT wTransparentTile,
	const bool bRoomConquered, 
	const bool bDarkened,
	const bool bPendingConquer,
	const bool bRoomRequired,
	const bool bRoomSecret)
{
	switch (wTransparentTile)
	{
		case T_OBSTACLE:
			return m_arrColor[MAP_GRAY];
		case T_TAR:
			return m_arrColor[MAP_MEDBLUE];
		case T_MUD:
			return m_arrColor[MAP_DKRED];
		case T_GEL:
			return m_arrColor[MAP_DKGREEN];
		case T_FLUFF:
			return m_arrColor[MAP_PALEBLUE];
		case T_BOMB:
		case T_STATION:
			return m_arrColor[MAP_LTGRAY];
		case T_BRIAR_SOURCE:
			return m_arrColor[MAP_MIDGREEN];
	}

	switch (wOpaqueTile)
	{
		case T_WALL: case T_WALL2: case T_WALL_IMAGE:
		case T_WALL_B: case T_WALL_H: 
			return bDarkened ? m_arrColor[MAP_DKCYAN2] : m_arrColor[MAP_BLACK];
		case T_STAIRS: case T_STAIRS_UP:
			return m_arrColor[MAP_STAIRS];
		case T_DOOR_Y:
			return m_arrColor[MAP_LTYELLOW];
		case T_DOOR_YO:
			return m_arrColor[MAP_PALEYELLOW];
		case T_DOOR_B:
			return m_arrColor[MAP_DKGRAY];
		case T_DOOR_BO:
			return m_arrColor[MAP_LTGRAY];
		case T_DOOR_M:
			if (bDarkened)
				return m_arrColor[MAP_LTGRAY];
			return m_arrColor[MAP_GREEN];
		case T_DOOR_GO:
			if (bDarkened)
				return m_arrColor[MAP_LTGRAY];
			return m_arrColor[MAP_LTGREEN];
		case T_DOOR_C:
			if (bDarkened)
				return m_arrColor[MAP_LTGRAY];
			return m_arrColor[MAP_LTCYAN];
		case T_DOOR_CO:
			if (bDarkened)
				return m_arrColor[MAP_LTGRAY];
			return m_arrColor[MAP_PALECYAN];
		case T_TRAPDOOR: case T_TRAPDOOR2:
			return m_arrColor[MAP_LTRED];
		case T_DOOR_R:
			return m_arrColor[MAP_RED];
		case T_DOOR_RO:
			return m_arrColor[MAP_MEDRED];
		case T_GOO:
			return m_arrColor[bPendingConquer && !bRoomConquered ? MAP_LTGREEN : MAP_GOO];
		case T_HOT:
			return m_arrColor[bPendingConquer && !bRoomConquered ? MAP_LTGREEN : MAP_ORANGE];
		case T_PIT: case T_PIT_IMAGE:
			return m_arrColor[MAP_BLUE];
		case T_WATER:
			return m_arrColor[MAP_LTBLUE];
		case T_THINICE:
			return m_arrColor[MAP_LTBLUE1];
		case T_SHALLOW_WATER:
			return m_arrColor[MAP_LTBLUE2];
		case T_THINICE_SH:
			return m_arrColor[MAP_LTBLUE3];
		case T_TUNNEL_N: case T_TUNNEL_S: case T_TUNNEL_E: case T_TUNNEL_W:
			return m_arrColor[MAP_GRAY];
		case T_FLOOR_SPIKES:
			return m_arrColor[MAP_MIDGRAY1];
		case T_FLUFFVENT:
			return m_arrColor[MAP_MIDGRAY2];
		case T_FIRETRAP: case T_FIRETRAP_ON:
			return m_arrColor[bPendingConquer && !bRoomConquered ? MAP_LTGREEN : MAP_DIM_ORANGE];
		case T_PLATFORM_W:
		case T_PLATFORM_P:
			return m_arrColor[MAP_DKORANGE];
		case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
			return m_arrColor[MAP_BROWN];
		case T_WALL_M:
		case T_WALL_WIN:
			return m_arrColor[MAP_ORANGE];
		default:
			if (bDarkened)
				return m_arrColor[MAP_LTBACKGROUND];
			if (this->bEditing)
			{
				if (bRoomRequired)
					return m_arrColor[MAP_PALERED];
				if (bRoomSecret)
					return m_arrColor[MAP_PALEGREEN];
				return m_arrColor[MAP_FLOOR];
			}
			if (bRoomConquered)
				return m_arrColor[MAP_FLOOR];
			if (bPendingConquer)
				return m_arrColor[MAP_LTGREEN];
			if (bRoomRequired)
				return m_arrColor[MAP_RED];
			return m_arrColor[MAP_LTMAGENTA]; //non-required, non-conquered room
	} 
}

//*****************************************************************************
inline Uint8 *CMapWidget::GetRoomStart(
//Gets location in map surface pixels for top lefthand corner of a room.
//
//Accepts:
	const UINT dwRoomX, const UINT dwRoomY)
{
	ASSERT(IsInRect(dwRoomX, dwRoomY,
		this->dwLeftRoomX, this->dwTopRoomY, this->dwRightRoomX, this->dwBottomRoomY));
	ASSERT(this->pMapSurface);

	//Figure x and y inside of pixel map.
	const UINT x = (dwRoomX - this->dwLeftRoomX) *
			CDrodBitmapManager::DISPLAY_COLS + this->wBorderW;
	const UINT y = (dwRoomY - this->dwTopRoomY) *
			CDrodBitmapManager::DISPLAY_ROWS + this->wBorderH;

	//Figure address of pixel.
	return (Uint8 *)this->pMapSurface->pixels + (y * this->pMapSurface->pitch) + 
			(x * this->pMapSurface->format->BytesPerPixel);
}

//*****************************************************************************
void CMapWidget::MarkEntrancesAsNotMain(
//When moving a room to a different level, level entrances in this room
//must not be marked as the main one.
//
//Params:
	CDbHold *pHold, CIDSet &entranceIDs)
const
{
	ASSERT(pHold);
	ASSERT(entranceIDs.size());
	ASSERT(!this->bCopyingRoom);

	CEntranceData *pEntrance;
	for (CIDSet::const_iterator entrance = entranceIDs.begin();
			entrance != entranceIDs.end(); ++entrance)
	{
		pEntrance = pHold->GetEntrance(*entrance);
		ASSERT(pEntrance);
		if (pEntrance->bIsMainEntrance)
		{
			pEntrance->bIsMainEntrance = false;
			pHold->Update();  //can call this here, as it should happen at most once
		}
	}
}

//*****************************************************************************
void CMapWidget::MergeHoldData(
//When copying a room, some global hold data may be present.
//This information must be copied to the destination hold, as follows:
//1. Level entrances in this room must be copied.
//A main level entrance only stays main if there are no other main ones.
//This could happen if the previous level entrance room was copied over.
//2. Ensure the room's character data is copied and updated properly.
//
//Params:
	CDbHold *pHold,      //(in/out) destination hold: entrances, scriptID and character data updated
	CDbRoom *pRoom,      //(in/out) scriptIDs updated for membership in destination hold
	ENTRANCE_VECTOR& entrances, //(in) added to hold
	const UINT uSrcHoldID, //(in) ID of room's source hold
	CImportInfo& info) //(in/out)
const
{
	ASSERT(pHold);
	ASSERT(pRoom);
	ASSERT(pRoom->dwRoomID);
	ASSERT(this->bCopyingRoom);

	//Load source hold if it's known and different from the destination hold.
	CDbHold *pSrcHold = NULL;
	if (pHold->dwHoldID != uSrcHoldID)
		pSrcHold = g_pTheDB->Holds.GetByID(uSrcHoldID);

	//1. Copy level entrances.
	CEntranceData *pEntrance;
	UINT wIndex;
	for (wIndex=0; wIndex<entrances.size(); ++wIndex)
	{
		pEntrance = entrances[wIndex];
		ASSERT(pEntrance);
		//Update entrance's room ID to point to duplicated level's room.
		pEntrance->dwRoomID = pRoom->dwRoomID;

		pHold->AddEntrance(pEntrance, false);
	}

	//2. Updates character information to be correct in destination hold.
	bool bIDsUpdated = false;
	CMonster *pMonster = pRoom->pFirstMonster;
	while (pMonster)
	{
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			pCharacter->ChangeHold(pSrcHold, pHold, info);
			bIDsUpdated = true;
		}
		pMonster = pMonster->pNext;
	}

	delete pSrcHold;
	if (bIDsUpdated)
		pRoom->Update();
	pHold->Update();  //always update timestamp, even if nothing else changes
}

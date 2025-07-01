// $Id: MapWidget.h 9304 2008-10-29 02:38:30Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef MAPWIDGET_H
#define MAPWIDGET_H

#include "DrodWidget.h"
#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/DbHolds.h"

extern const UINT MAP_NOT_MARKED;
extern const UINT MAX_MARKERS;

//******************************************************************************
class CMapWidget : public CFocusWidget
{     
public:
	CMapWidget(UINT dwSetTagNo, int nSetX, int nSetY, UINT wSetW, 
			UINT wSetH, const CCurrentGame *pSetCurrentGame, const UINT sizeMultiplier=1);

	void           ClearMap();
	void           ClearState();
	void           CopyRoom(const bool bCopy);
	void           DrawMapSurfaceFromRoom(const CDbRoom *pRoom, const UINT mapMarker=0);
	SDL_Surface*   GetMapSurface() const {return this->pMapSurface;}
	void           GetRoomAtCoords(const int nX, const int nY, UINT& dwRoomX, UINT& dwRoomY) const;
	void           GetSelectedRoomXY(UINT &dwRoomX, UINT &dwRoomY) const;
	void           GetSelectedRoomRect(SDL_Rect &rect) const;
	void           GetVisibleMapRect(SDL_Rect &rect) const;
	void           HighlightRoom(SDL_Rect& dest, const UINT wWidth=1);
	bool           IsDeletingRoom() const {return this->bDeletingRoom;}
	bool           LoadFromCurrentGame(const CCurrentGame *pSetCurrentGame,
			const bool bDrawSurface=true);
	bool           LoadFromLevel(CDbLevel *pLevel);
	virtual void   Paint(bool bUpdateRect = true);
	bool           PasteRoom(CDbHold *pHold);
	bool           ReadyToPasteRoom() const {return this->pRoom != NULL;}
	void           RefreshRoom(const UINT roomID, const UINT mapMarker=MAP_NOT_MARKED);
	virtual void   Resize(const UINT wSetW, const UINT wSetH);
	void           SelectRoom(const UINT dwRoomX, const UINT dwRoomY);
	void           SelectRoomAtCoords(const int nX, const int nY);
	bool           SelectRoomIfValid(const UINT dwRoomX, const UINT dwRoomY);
	void           SetDestSurface(SDL_Surface *pNewSurface);
	void           UpdateFromCurrentGame();
	void           UpdateFromCurrentLevel();

	bool           bVacantRoom;   //when room at selected coords doesn't exist
	bool           bUserMoveable; //whether user input can change selected room

	//Keeping track of which room was clicked on when no selection can be made.
	UINT              dwClickedRoomX, dwClickedRoomY;

protected:
	virtual ~CMapWidget();

	virtual void   HandleMouseDown(const SDL_MouseButtonEvent &Button);
	virtual void   HandleKeyDown(const SDL_KeyboardEvent &Key);
 
private:
	void           DrawBackground();
	void           DrawGridlines(UINT includedLeft, UINT includedRight,
			UINT includedTop, UINT includedBottom);

	void MergeHoldData(CDbHold *pHold, CDbRoom *pRoom, ENTRANCE_VECTOR& entrances,
			const UINT uSrcHoldID, CImportInfo& info) const;
	inline SURFACECOLOR  GetMapColorFromTile(const UINT wOpaqueTile,
			const UINT wTransparentTile,
			const bool bDarkened,
			const bool bSecret,
			const bool bHasMonsters, const bool bHasItems, const bool bHasClosedDoors);
	UINT           GetRoomShowHeight() const;
	UINT           GetRoomShowWidth() const;
	inline Uint8 *    GetRoomStart(const UINT dwRoomX, const UINT dwRoomY);
	bool           LoadMapSurface(const bool bForceMargin=false);
	void           InitMapColors();
	bool           IsAdjacentToValidRoom(const UINT dwRoomX, const UINT dwRoomY);
	inline void       LockMapSurface();
	void           MarkEntrancesAsNotMain(CDbHold *pHold, CIDSet &entranceIDs) const;
	void           SetNoDetailRooms();
	inline void       UnlockMapSurface();
	void           UpdateMapSurface(const bool bRefreshSelectedRoom = false);

	UINT             dwLevelID;
	CIDSet               ExploredRooms, MappedRooms;
	CIDSet               NoDetailRooms, PreviewedRooms;
	bool              bIsLevelComplete;

	const CCurrentGame * pCurrentGame;  //to show map of a game in progress
	CDbLevel *           pLevel;        //to show map of entire level
	SDL_Surface *        pMapSurface;

	UINT             dwSelectedRoomX, dwSelectedRoomY;
	UINT             dwLeftRoomX, dwTopRoomY;
	UINT             dwRightRoomX, dwBottomRoomY;

	UINT              wLastSrcX, wLastSrcY;
	UINT              wBorderW, wBorderH;
	bool              bScrollHorizontal, bScrollVertical;
	UINT              sizeMultiplier; //to show room areas larger

	bool              bEditing;   //whether level editor is being used
	bool              bCopyingRoom;  //room is being copy-and-pasted
												//(otherwise, it just is being cut)
	bool              bDeletingRoom; //room is get pasted over
	CDbRoom *         pRoom;      //room being cut or copied
};

#endif //#ifndef MAPWIDGET_H

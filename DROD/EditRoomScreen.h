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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996, 
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef EDITROOMSCREEN_H
#define EDITROOMSCREEN_H

#include "RoomScreen.h"
#include "EditRoomWidget.h"
#include "SelectMediaDialogWidget.h"
#include "../DRODLib/DbRooms.h"
#include <BackEndLib/CoordSet.h>

enum PlotType
{
	PLOT_NOTHING,  //nothing was plotted (due to illegal placement)
	PLOT_HEAD,     //only a head was plotted
	PLOT_DONE,     //nothing plotted (and we're done)
	PLOT_SEGMENT,  //a segment was plotted (2+ pieces)
	PLOT_ERROR     //neither a head nor tail was plotted
};

typedef list<vector<CDbBase*> > UndoList;

class CCharacterDialogWidget;
class CDbLevel;
class CMapWidget;
class CMoveCoord;
class CRoomWidget;
class CObjectMenuWidget;
class CTabbedMenuWidget;
class CEntranceSelectDialogWidget;
class CEditRoomScreen : public CRoomScreen
{
friend class CCharacterDialogWidget;
public:
	virtual ~CEditRoomScreen();

	static void    FillInRoomEdges(CDbRoom* const pRoom);

	bool     SetRoom(const UINT dwRoomID, const bool bQuick=false);
	UINT     GetRoomID() const {return (this->pRoom ? this->pRoom->dwRoomID : 0L);}
	UINT     GetLevelID() const {return (this->pRoom ? this->pRoom->dwLevelID : 0L);}

	UINT     GetSavePlayerID() const {return this->dwSavePlayerID;}
	UINT     GetTestPlayerID() const {return this->dwTestPlayerID;}

	UINT     SelectMediaID(const UINT dwSelectedValue, const CSelectMediaDialogWidget::DATATYPE eType);

protected:
	friend class CDrodScreenManager;

	CEditRoomScreen();

	virtual void   ApplyINISettings();
	virtual void   DisplayChatText(const WSTRING& text, const SDL_Color& color);
	virtual bool   IsCommandSupported(int command) const;
	virtual bool   SetForActivate();
	virtual bool   UnloadOnDeactivate() const {return false;}

	enum Change {Room, Hold, RoomAndHold};

private:
	void     AddChatDialog();
	void     AddLevelEntranceDialog();
	void     AddPlotEffect(const UINT wObjectNo);
	UINT     AddRoom(const UINT dwRoomX, const UINT dwRoomY);
	void     ApplyPlayerSettings();

	void     Changing(const Change eChange=Room);
	static void    ClearList(UndoList& List);
	void     ClickRoom();
	const UINT*    DisplaySelection(const UINT wObjectNo) const;
	bool     DeleteLevelEntrance(const UINT wX, const UINT wY);
	void     DisplayChatDialog();
	void     DrawHalphSlayerEntrances();
	void     EditLevelEntrance(const UINT wX, const UINT wY);
	bool     EditLevelEntrance(WSTRING &wstrDescription, bool &bMainEntrance,
			CEntranceData::DescriptionDisplay &eShowEntranceDesc, UINT &dataID);
	void     EditGentryii(CMonster* pMonster);
	void     EditSerpent(CMonster* pMonster);
	void     EditObjects();
	void     EditOrbAgents(const UINT wX, const UINT wY);
	bool     EditScrollText(const UINT wX, const UINT wY);
	bool     EraseAndPlot(const UINT wX, const UINT wY, const UINT wObjectToPlot,
			const bool bFixImmediately=true);
	void     EraseObjects();
	void     EraseObjects(const UINT wLayer, bool& bSomethingPlotted, bool& bSomethingDeleted);
	void     EraseObjectsOnAllLayers(bool& bSomethingPlotted, bool& bSomethingDeleted);
	void     EraseRegion();
	COrbAgentData* FindOrbAgentFor(const UINT wX, const UINT wY, COrbData* pOrb);
	COrbAgentData* FindOrbAgentFor(COrbData* pOrb, CCoordSet &doorCoords);
	void     FixUnstableTar();
	void     FixCorruptStaircase(const UINT wX, const UINT wY);
	CObjectMenuWidget*   GetActiveMenu();
	void           ForceFullStyleReload();

	void     GetCustomImageID(UINT& roomDataID, UINT& imageStartX, UINT& imageStartY, const bool bReselect);
	void           GetFloorImageID(const bool bReselect=false);
	void           GetOverheadImageID(const bool bReselect=false);

	void           GetLevelEntrancesInRoom();
	const UINT*    GetTileImageForMonsterType(const UINT wType, const UINT wO,
			const UINT wAnimFrame) const;
	UINT     getRotatedTile(UINT wObject, bool bCW) const;
	UINT     GetSelectedObject() const;
	void     HighlightPendingPaste();
	UINT     ImportHoldSound();
	UINT     ImportHoldVideo();
	void     IncrementMenuSelection(const bool bForward=true);

	bool     LoadRoom(CDbRoom *pNewRoom);
	bool     LoadRoomAtCoords(const UINT dwRoomX, const UINT dwRoomY, const bool bForceReload=false);

	void     MarkOverheadLayerTiles();

	bool     MergePressurePlate(const UINT wX, const UINT wY, const bool bUpdateType=false);
	CObjectMenuWidget* ObjectMenuForTile(const UINT wTile);
	void     ObstacleFill();
	
	virtual void   OnBetweenEvents();
	virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDeactivate();
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   OnMouseDown(const UINT dwTagNo, const SDL_MouseButtonEvent &Button);
	virtual void   OnMouseMotion(const UINT dwTagNo, const SDL_MouseMotionEvent &Motion);
	virtual void   OnMouseUp(const UINT dwTagNo, const SDL_MouseButtonEvent &Button);
	virtual void   OnMouseWheel(const SDL_MouseWheelEvent &Wheel);
	virtual void   OnSelectChange(const UINT dwTagNo);

	virtual void   Paint(bool bUpdateRect=true);
	void     PaintHighlights();

	void     PasteRegion(const UINT wX, const UINT wY);
	bool     PlaceEdgeMonster(const UINT wObject, const UINT wX, const UINT wY,
			bool& bOpenTile);
	void     PlotLastSerpentSegment(const UINT wTailX, const UINT wTailY,
			const UINT wDirection);
	PlotType    PlotGentryiiSegment();
	PlotType    PlotLongMonsterSegment();
	void     PlotObjects();
	bool     PlotObjectAt(const UINT wX, const UINT wY, UINT wObject,
			const UINT wO);
	void     PlotStaircase(const UINT wStairType);
	void     PopulateItemMenus();
	void     ReadyToPasteRegion();
	void     ReflectRoomX();
	void     ReflectRoomY();
	void     RecalcBrokenObstacle(const UINT wCX, const UINT wCY, const UINT wObIndex);
	void     RemoveChange();
	bool     RemoveMonster(CMonster *pMonster);
	bool     RemoveObjectAt(const UINT wX, const UINT wY, const UINT wPlottedObject,
			bool &bSpecialTileRemoved, bool &bTarRemoved, bool &bStairsRemoved);
	void     RemoveOrbAssociationAt(const UINT wX, const UINT wY);
	void     RepairYellowDoors(const UINT doorType);
	void     ResetAdjacentRooms();
	void     ResetMembers();
	void     RotateClockwise();
	void     RotateCounterClockwise();
	bool     SaveRoom();
	bool     SaveRoomToDB();
	void     SetButtons(const bool bPaint=true);
	void     SetDestinationEntrance(const UINT wX1, const UINT wY1,
			const UINT wX2, const UINT wY2);
	void     SetLightLevel();
	void     SetMenuItem(const UINT wObject, const UINT wNewTile);
	void     SetOrbAgentsForDoor(const UINT wX, const UINT wY);
	void     SetSelectedObject(const UINT wObject);
	void     SetSignTextToCurrentRoom(const bool bMarkAsEntrance=false);
	bool     SetState(const EditState eState, const bool bForce=false);
	bool     SetUnspecifiedPlayerSettings(CDbPackedVars &Settings);
	void     ShowErrors();
	void     ShowPlot();
	bool     ToggleMenuItem(const UINT wObject, const bool bCW=true);
	void     UndoCommand(const bool bUndo);
	void     UniquePlacement(const UINT wX, const UINT wY, const MONSTERTYPE eType);
	void     UnloadPlaytestSession();
	void     UpdateMenuGraphic(const UINT wTile);
	void     WarpToNextLevel(const bool bDown);

	CDbHold *         pHold;
	CDbLevel *        pLevel;
	CDbRoom *         pRoom;
	vector<CMoveCoord*> LevelEntrances;  //level entrances in the current room

	CEditRoomWidget *   pRoomWidget;
	CTabbedMenuWidget * pTabbedMenu;

	CCharacterDialogWidget *      pCharacterDialog;
	CEntranceSelectDialogWidget * pEntranceBox;     //choose from list of levels
	CSelectMediaDialogWidget *    pSelectMediaDialog;
	CDialogWidget *               pLevelEntranceDialog; //for defining level entrances

	UINT              wSelectedObject;  //object selected for placement
	UINT              wSelectedObjectSave; //object selected, while
														//something else is being placed
	UndoList          undoList, redoList;  //sequence of stored hold/room states
	EditState         eState;        //current state of room editing
	CMonster *        pLongMonster;  //long monster being placed
	COrbData *        pOrb;          //orb being modified
	bool              bShowErrors;   //visually display objects with errors
	bool              bAutoSave;     //always save the room when leaving it
	bool              bSafetyOn;     //safe editing
	bool              bRoomDirty;    //whether a save is needed
	int               nUndoSize;     //facilitates dirty checking
	bool              bPaintItemText; //whether item text must be refreshed
	bool              bGroupMenuItems; //whether to collapse similar menu items into one entry

	UINT              wTestX, wTestY, wTestO; //start player here when room testing

	UINT              wO;            //current placement orientation
	UINT              wSelectedObType;      //current obstacle selected
	UINT					wSelectedLightType;   //current light selected
	UINT              wSelectedDarkType;    //current dark level selected
	UINT              wSelOrbType, wSelPlateType; //current orb/pressure plate type selected
	UINT              wSelTokenType;   //token type currently selected
	UINT              wSelWaterType; //current water selected (if ungrouped)

	UINT              wLastFloorSelected;
	UINT              wLastEntranceSelected;  //for warping between levels
	bool              bSelectingImageStart;

	UINT              wSelectedX, wSelectedY; //for multi-square selection

	//Room region duplication.
	CDbRoom          *pCopyRoom;
	UINT              wCopyX1, wCopyY1, wCopyX2, wCopyY2;
	bool              bAreaJustCopied, bCutAndPaste;

	UINT             dwSavePlayerID, dwTestPlayerID;   //ID of temporary player for room testing
	CCoordSet         ignoreCoords; //for efficient door placement

	//Obstacle placement.
	CCoordIndex       obstacles;

	//Error plotting.
	CDbRoom *pAdjRoom[4];
};

#endif //...#ifndef EDITROOMSCREEN_H

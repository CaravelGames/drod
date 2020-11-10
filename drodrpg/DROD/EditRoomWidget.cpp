// $Id: EditRoomWidget.cpp 10126 2012-04-24 05:40:08Z mrimer $

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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "EditRoomWidget.h"
#include "PendingPlotEffect.h"
#include "TileImageCalcs.h"

#include "DrodEffect.h"
#include "RoomEffectList.h"
#include <FrontEndLib/ToolTipEffect.h>
#include <FrontEndLib/TransTileEffect.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/MonsterFactory.h"
#include "../DRODLib/MonsterPiece.h"
#include "../DRODLib/Character.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Coord.h>

const SURFACECOLOR Red = {255, 0, 0};
const SURFACECOLOR BlueGreen = {0, 255, 255};
const SURFACECOLOR PaleYellow = {255, 255, 128};
const SURFACECOLOR Orange = {255, 128, 0};

#define NO_BOLT	(static_cast<UINT>(-1))

//
//Public methods.
//

//*****************************************************************************
CEditRoomWidget::CEditRoomWidget(
//Constructor.
//
//Params:
	UINT dwSetTagNo,                //(in)   Required params for CWidget
	int nSetX, int nSetY,               //    constructor.
	UINT wSetW, UINT wSetH)             //
	: CRoomWidget(dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, wStartX((UINT)-1), wStartY((UINT)-1)
	, wMidX((UINT)-1), wMidY((UINT)-1)
	, wEndX((UINT)-1), wEndY((UINT)-1)
	, bMouseInBounds(false)
	, bSinglePlacement(false)
	, bPlacing(true)
	, eEditState(ES_PLACING)
	, pLevelEntrances(NULL)
	, wOX(NO_BOLT), wOY(NO_BOLT)
	, pPlotted(NULL)
{
}

//*****************************************************************************
CEditRoomWidget::~CEditRoomWidget()
{
	delete[] this->pPlotted;
}

//*****************************************************************************
void CEditRoomWidget::AddToolTipEffect(
//Adds a tool tip to room tile.
//
//Params:
	const UINT wX, const UINT wY, //(in) desired square
	const MESSAGE_ID messageID)
{
	CCoord Coord(wX,wY);
	AddLastLayerEffect(new CToolTipEffect(this, Coord,
			g_pTheDB->GetMessageText(messageID)));
}

//*****************************************************************************
bool CEditRoomWidget::AddOrbEffect(
//Add an effect to display the affect an orb agent has on a door.
//
//Returns: whether the orb agent is on a door
//
//Params:
	COrbAgentData *pOrbAgent)  //(in) Orb agent to display
{
	const UINT wX = pOrbAgent->wX, wY = pOrbAgent->wY;
	ASSERT(this->pRoom->IsValidColRow(wX, wY));

	//Only for orbs and pressure plates.
	const bool bPressPlate = this->pRoom->GetOSquare(wX, wY) == T_PRESSPLATE;
	if (this->pRoom->GetTSquare(wX, wY) != T_ORB && !bPressPlate)
		return false;

	//Set highlight color.
	SURFACECOLOR color;
	switch (pOrbAgent->action)
	{
		case OA_NULL:
			//Just highlight the door (mouse is over it).
			color = PaleYellow;
		break;
		case OA_TOGGLE: color = Orange; break;
		case OA_OPEN: color = BlueGreen; break;
		case OA_CLOSE: color = Red; break;
		default:
			ASSERT(!"AddOrbEffect: Bad orb agent.");
		break;
	}

	//Get set of all tiles to highlight.
	CCoordSet tiles(wX,wY);
	if (bPressPlate)
	{
		CTileMask plateMask(T_PRESSPLATE);
		this->pRoom->GetConnectedTiles(wX, wY, plateMask, false, tiles, NULL);
	}

	for (CCoordSet::const_iterator tile=tiles.begin(); tile!=tiles.end(); ++tile)
		AddShadeEffect(tile->wX, tile->wY, color);

	return true;
}

//*****************************************************************************
void CEditRoomWidget::AddOrbAgentsEffect(
//Add an effect to display the affect an orb has on yellow doors in the room.
//
//Params:
	COrbData *pOrb,   //(in) Orb data to display
	const bool bEditingOrb)//(in) whether an orb is being edited (and should be drawn)
							 //(default = true) if false, a door is being edited; i.e.,
							 //effect of all orbs on one door is being displayed,
							 //not one orb on all its doors.
{
	ASSERT(pOrb);

	//Remove last effect to repaint affected areas
	this->pLastLayerEffects->RemoveEffectsOfType(ETRANSTILE);
	this->pLastLayerEffects->RemoveEffectsOfType(ESHADE);

	const bool bDrawOrb = bEditingOrb &&
			this->pRoom->GetTSquare(pOrb->wX, pOrb->wY) == T_ORB;

	if (!this->pMLayerEffects->ContainsEffectOfType(EORBHIT))
		AddStrikeOrbEffect(*pOrb, bDrawOrb);

	ResetPlot();   //tracks which door squares are highlighted
	//Highlight any door/orb mouse is over.
	COrbAgentData TentativeOrbAgent;
	TentativeOrbAgent.wX = this->wEndX;
	TentativeOrbAgent.wY = this->wEndY;
	TentativeOrbAgent.action = OA_NULL;
	const bool bDrawingSomething = (bEditingOrb ?
		AddDoorEffect(&TentativeOrbAgent) : AddOrbEffect(&TentativeOrbAgent));
	if (bDrawingSomething && pOrb->agents.size())
		ResetPlot();   //call again after door/orb highlight

	if (!pOrb->agents.size())
	{
		//Show lightning from orb to mouse cursor -- ready to place an agent.
		this->wOX = pOrb->wX;
		this->wOY = pOrb->wY;
	} else {
		this->wOX = NO_BOLT;  //don't show lightning to mouse cursor

		//For each orb agent...
		const UINT wNumAgents = pOrb->agents.size();
		for (UINT wIndex=0; wIndex<wNumAgents; ++wIndex)
		{
			COrbAgentData *pAgent = pOrb->agents[wIndex];
			if (bEditingOrb)
				AddDoorEffect(pAgent);
			else
				AddOrbEffect(pAgent);
		}
	}
}

//*****************************************************************************
void CEditRoomWidget::AddOrbAgentToolTip(
//Adds a tool tip near (wX,wY).
//
//Params:
	const UINT wX, const UINT wY, const UINT wAgentType)  //(in)
{
	UINT wNearX, wNearY;
	wNearX = (wX < this->pRoom->wRoomCols - 1 ? wX + 1 : wX);
	wNearY = (wY > 0 ? wY - 1 : wY);

	const int x = this->x + wNearX * CX_TILE;
	const int y = this->y + wNearY * CY_TILE;
	this->pLastLayerEffects->RemoveEffectsOfType(ETOOLTIP);
	switch (wAgentType)
	{
		case OA_NULL: break;
		case OA_TOGGLE: AddToolTipEffect(x, y, MID_OrbAgentToggle); break;
		case OA_OPEN: AddToolTipEffect(x, y, MID_OrbAgentOpen);  break;
		case OA_CLOSE: AddToolTipEffect(x, y, MID_OrbAgentClose);   break;
	}
}

//*****************************************************************************
void CEditRoomWidget::AddMonsterSegmentEffect(
//Add an effect to display a long monster segment to room.
//
//Params:
	const UINT wMonsterType)         //(in)   Monster to be placed.
{
	UINT wX, wY, wTileNo;

   //only long monsters
	ASSERT(wMonsterType == M_SERPENT || wMonsterType == M_SERPENTG || wMonsterType == M_SERPENTB);
	this->monsterSegment.wType = wMonsterType;

	this->pLastLayerEffects->RemoveEffectsOfType(ETRANSTILE);
	this->pLastLayerEffects->RemoveEffectsOfType(ESHADE);

	//Save segment start and end.
	const UINT wSX = this->monsterSegment.wSX;
	const UINT wSY = this->monsterSegment.wSY;
	this->monsterSegment.wEX = this->wEndX;
	this->monsterSegment.wEY = this->wEndY;

	//Only show tiles in a horizontal or vertical direction.
	//Determine which way to display.
	const bool bHorizontal = this->monsterSegment.bHorizontal =
			(abs(static_cast<int>(this->wEndX) - static_cast<int>(wSX))
				>= abs(static_cast<int>(this->wEndY) - static_cast<int>(wSY)));
	const UINT wMinX =
			(bHorizontal ? min(wSX,this->wEndX) : wSX);
	const UINT wMinY =
			(bHorizontal ? wSY : min(wSY,this->wEndY));
	const UINT wMaxX =
			(bHorizontal ? max(wSX,this->wEndX) : wMinX);
	const UINT wMaxY =
			(bHorizontal ? wMinY : max(wSY,this->wEndY));
	const UINT wDirection = this->monsterSegment.wDirection = (bHorizontal ?
			(this->wEndX > wSX ? W : E) :
			(this->wEndY > wSY ? N : S));
	const bool bSegment = (wMaxY != wMinY || wMaxX != wMinX);
	bool bHead;

	//Calculate new pending tail position.
	this->monsterSegment.wTailX = (bHorizontal ? this->wEndX : wMinX);
	this->monsterSegment.wTailY = (bHorizontal ? wMinY : this->wEndY);

	for (wY=wMinY; wY<=wMaxY; ++wY)
		for (wX=wMinX; wX<=wMaxX; ++wX)
		{
			bHead = false;
			//Calculate tile to display.
			if (wX == wSX && wY == wSY)
			{
				if (wX == this->monsterSegment.wHeadX &&
						wY == this->monsterSegment.wHeadY)
				{
					//Show head.
					wTileNo = GetTileImageForEntity(wMonsterType, wDirection, 0);
					bHead = true;
				} else if ((wX == this->monsterSegment.wTailX) &&
						(wY == this->monsterSegment.wTailY))
				{
					//Only one tile -- it's the tail.
					//Since it would paint over where a tail should already be,
					//don't have to do anything here.
					continue;
				} else {
					//Show a twist (a turn where the tail currently is).
					wTileNo = GetSerpentTurnTile(wX,wY,wDirection,true);
				}
			} else if ((wX == this->monsterSegment.wTailX) &&
						(wY == this->monsterSegment.wTailY))
			{
				//Show tail.
				wTileNo = GetSerpentTailTile(wX,wY,wDirection,true);
			} else {
				//Show straight segment.
				//If backtracking, erase.
				wTileNo = GetSerpentStraightTile(wX,wY,wDirection,true);
			}

			if (IsSafePlacement(wMonsterType + M_OFFSET,wX,wY))
			{
				CCoord coord(wX,wY);
				AddLastLayerEffect(new CTransTileEffect(this, coord, wTileNo));
				if (bHead && !bSegment)
					AddShadeEffect(wX,wY,Red); //can't plot only the head
			}
			else
				//Obstacle there -- can't place.
				AddShadeEffect(wX,wY,Red);
		}
}

//*****************************************************************************
UINT CEditRoomWidget::GetSerpentStraightTile(
//Show/plot tail in the proper direction.
//
//Returns: tile to show/plot
//
//Params:
	const UINT wX, const UINT wY, const UINT wDirection,  //(in)
	const bool bShow) //(in) Whether this is for display or room plotting
const
{
	CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
	UINT wTTileNo;
	if (!pMonster || !pMonster->IsPiece())
		wTTileNo = T_NOMONSTER;
	else {
		CMonsterPiece *pMPiece = DYN_CAST(CMonsterPiece*, CMonster*, pMonster);
		ASSERT(pMPiece);
		wTTileNo = pMPiece->wTileNo;
	}
	UINT wTileNo = T_NOMONSTER;
	switch (wDirection)
	{
		case W:
		case E:
			//Horizontal
			switch (wTTileNo)
			{
				case T_SNK_EW: wTileNo = T_NOMONSTER;  break;   //erasing
				case T_SNK_NW: wTileNo = T_SNK_NE;  break;   //changing a turn
				case T_SNK_NE: wTileNo = T_SNK_NW;  break;
				case T_SNK_SW: wTileNo = T_SNK_SE;  break;
				case T_SNK_SE: wTileNo = T_SNK_SW;  break;
				default: wTileNo = T_SNK_EW;  break;         //continuing straight
			}
			break;
		case N:
		case S:
			//Vertical
			switch (wTTileNo)
			{
				case T_SNK_NS: wTileNo = T_NOMONSTER;  break;
				case T_SNK_NW: wTileNo = T_SNK_SW;  break;
				case T_SNK_NE: wTileNo = T_SNK_SE;  break;
				case T_SNK_SW: wTileNo = T_SNK_NW;  break;
				case T_SNK_SE: wTileNo = T_SNK_NE;  break;
				default: wTileNo = T_SNK_NS;  break;
			}
			break;
		default:
			ASSERT(!"Bad serpent tile."); break;
	}

	if (bShow)
		return (wTileNo == T_NOMONSTER ? TI_FLOOR :
				GetTileImageForSerpentPiece(this->monsterSegment.wType, wTileNo));

	return wTileNo;
}

//*****************************************************************************
UINT CEditRoomWidget::GetSerpentTailTile(
//Show/plot tail in the proper direction.
//
//Returns: tile to show/plot
//
//Params:
	const UINT wX, const UINT wY, const UINT wDirection,  //(in)
	const bool bShow) //(in) Whether this is for display or room plotting
const
{
	CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
	UINT wTTileNo;
	if (!pMonster || !pMonster->IsPiece())
		wTTileNo = T_NOMONSTER;
	else {
		CMonsterPiece *pMPiece = DYN_CAST(CMonsterPiece*, CMonster*, pMonster);
		ASSERT(pMPiece);
		wTTileNo = pMPiece->wTileNo;
	}
	UINT wTileNo;
	switch (wDirection)
	{
		case N:
			switch (wTTileNo)
			{
				case T_SNK_NS: wTileNo = T_SNKT_N;  break;
				case T_SNK_SE: wTileNo = T_SNKT_E;  break;
				case T_SNK_SW: wTileNo = T_SNKT_W;  break;
				default: wTileNo = T_SNKT_S;  break;
			}
			break;
		case S:
			switch (wTTileNo)
			{
				case T_SNK_NS: wTileNo = T_SNKT_S;  break;
				case T_SNK_NE: wTileNo = T_SNKT_E;  break;
				case T_SNK_NW: wTileNo = T_SNKT_W;  break;
				default: wTileNo = T_SNKT_N;  break;
			}
			break;
		case E:
			switch (wTTileNo)
			{
				case T_SNK_EW: wTileNo = T_SNKT_E;  break;
				case T_SNK_NW: wTileNo = T_SNKT_N;  break;
				case T_SNK_SW: wTileNo = T_SNKT_S;  break;
				default: wTileNo = T_SNKT_W;  break;
			}
			break;
		case W:
			switch (wTTileNo)
			{
				case T_SNK_EW: wTileNo = T_SNKT_W;  break;
				case T_SNK_NE: wTileNo = T_SNKT_N;  break;
				case T_SNK_SE: wTileNo = T_SNKT_S;  break;
				default: wTileNo = T_SNKT_E;  break;
			}
			break;
		default: ASSERT(!"Bad serpent orientation."); return 0;
	}

	if (bShow)
		return GetTileImageForSerpentPiece(this->monsterSegment.wType, wTileNo);

	return wTileNo;
}

//*****************************************************************************
UINT CEditRoomWidget::GetSerpentTurnTile(
//Show/plot a turn in a serpent in the proper direction.
//(Change a tail piece into a turn.  Erase if backing up.)
//
//Returns: tile to show/plot
//
//Params:
	const UINT wX, const UINT wY, const UINT wDirection,  //(in)
	const bool bShow) //(in) Whether this is for display or room plotting
const
{
	UINT wTileNo = T_NOMONSTER;
	CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX,wY);
	UINT wTTileNo;
	if (!pMonster)
		wTTileNo = T_NOMONSTER;
	else {
		CMonsterPiece *pMPiece = DYN_CAST(CMonsterPiece*, CMonster*, pMonster);
		ASSERT(pMPiece);
		wTTileNo = pMPiece->wTileNo;
	}
	switch (wTTileNo)
	{
	case T_SNKT_S:
		switch (wDirection)
		{
			case N: wTileNo = T_SNK_NS;   break;   //continue
			case S: wTileNo = T_NOMONSTER;   break;   //erase
			case E: wTileNo = T_SNK_SE;   break;   //turn
			case W: wTileNo = T_SNK_SW;   break;
		}
		break;
	case T_SNKT_N:
		switch (wDirection)
		{
			case N: wTileNo = T_NOMONSTER;   break;
			case S: wTileNo = T_SNK_NS;   break;
			case E: wTileNo = T_SNK_NE;   break;
			case W: wTileNo = T_SNK_NW;   break;
		}
		break;
	case T_SNKT_E:
		switch (wDirection)
		{
			case N: wTileNo = T_SNK_NE;   break;
			case S: wTileNo = T_SNK_SE;   break;
			case E: wTileNo = T_NOMONSTER;   break;
			case W: wTileNo = T_SNK_EW;   break;
		}
		break;
	case T_SNKT_W:
		switch (wDirection)
		{
			case N: wTileNo = T_SNK_NW;   break;
			case S: wTileNo = T_SNK_SW;   break;
			case E: wTileNo = T_SNK_EW;   break;
			case W: wTileNo = T_NOMONSTER;   break;
		}
		break;
	default: ASSERTP(bShow, "Bad serpent turn tile."); if (!bShow) return 0;
		break;
	}

	if (bShow)
		return (wTileNo == T_NOMONSTER ? TI_FLOOR :
				GetTileImageForSerpentPiece(this->monsterSegment.wType, wTileNo));

	return wTileNo;
}

//*****************************************************************************
void CEditRoomWidget::AddPendingPasteEffect(
//Adds shade effects to room's top effect layer to show where a pending room region
//paste will occur, originating at the indicated location.
//Erases previous shade effects.
//
//Params:
	const UINT wX1, const UINT wY1, //top-left corner of pending paste region
	const UINT wXSize, const UINT wYSize,  //(in) Dimensions of region
	const SURFACECOLOR& color,
	const bool bEraseOldShading)
{
	if (bEraseOldShading)
		this->pLastLayerEffects->RemoveEffectsOfType(ESHADE);

	for (UINT wY=0; wY<=wYSize; ++wY)
		for (UINT wX=0; wX<=wXSize; ++wX)
			AddShadeEffect(wX1 + wX, wY1 + wY, color);
}

//*****************************************************************************
void CEditRoomWidget::AddPendingPlotEffect(
//Adds a PendingPlot effect on top of the room to show where a pending item plot is marked.
//Erases previous PendingPlot effects.
//
//Params:
	const UINT wObjectNo,         //(in)   Object to be placed.
	const UINT* pwTileImageNo,    //(in)   Image of object to place.
	const UINT wXSize, const UINT wYSize,  //(in) Dimensions of object being displayed
											//(default: 1x1)
	const bool bSinglePlacement,  //(in)   Show only one object [default=false]
	const UINT wO)                //(in)   Orientation of object [default=NO_ORIENTATION]
{
	this->pLastLayerEffects->RemoveEffectsOfType(EPENDINGPLOT);
	AddLastLayerEffect(
		new CPendingPlotEffect(this, wObjectNo, pwTileImageNo, wXSize, wYSize, wO));
	this->bSinglePlacement = bSinglePlacement;
}

//*****************************************************************************
void CEditRoomWidget::IsLevelStartAt(
//
//
//Params:
	const UINT wX, const UINT wY,       //(in) position to check
	bool &bSwordsmanAt, bool &bSwordAt) //(out)
const
{
	bSwordsmanAt = bSwordAt = false;
	ASSERT(this->pLevelEntrances);
	CMoveCoord *pEntrance;
	for (UINT wIndex=0; wIndex<this->pLevelEntrances->size(); ++wIndex)
	{
		pEntrance = (*this->pLevelEntrances)[wIndex];
		if (wX == pEntrance->wX && wY == pEntrance->wY)
			bSwordsmanAt = true;
		if (wX == pEntrance->wX + nGetOX(pEntrance->wO) &&
				wY == pEntrance->wY + nGetOY(pEntrance->wO))
			bSwordAt = true;
	}
}

//*****************************************************************************
bool CEditRoomWidget::IsObjectReplaceable(
//Returns: whether this object can be put down on (replacing) itself
//
//Params:
	const UINT wObject,     //(in) object being placed
	const UINT wTileLayer,  //(in) object layer
	const UINT wTileNo)     //(in) object on tile layer
const
{
	switch (wTileLayer)
	{
	case 0:
		//Floor types can replace themselves,
		//except for doors: to facilitate editing them when clicked on.
		if (bIsPlainFloor(wTileNo)) return true; //anything can replace floor
		if (wTileNo == T_HOT || wTileNo == T_GOO) return true;
		if (bIsWall(wTileNo) && bIsWall(wObject)) return true;
		if (bIsCrumblyWall(wTileNo) && bIsCrumblyWall(wObject)) return true;
		if (bIsTunnel(wTileNo) && bIsTunnel(wObject)) return true;
		if ((bIsPit(wTileNo) || bIsWater(wTileNo) || bIsTrapdoor(wTileNo) || bIsPlatform(wTileNo) || bIsBridge(wTileNo)) &&
			 (bIsPit(wObject) || bIsWater(wObject) || bIsTrapdoor(wObject) || bIsPlatform(wObject) || bIsBridge(wObject)))
			return true;
		if (bIsDoor(wObject) || bIsOpenDoor(wObject))
			return false;
		if (wObject == T_PRESSPLATE)
			return false;
		return wObject == wTileNo;
	case 3:
		//Arrows can replace arrows.
		if (bIsArrow(wObject) && bIsArrow(wTileNo))
			return true;
		if (wObject == T_WALLLIGHT)
			return false;
		return wObject == wTileNo;
	case 1:
		//Can't replace customizable objects: scrolls, obstacles and lights.
		if (wObject == T_SCROLL || wObject == T_OBSTACLE || bIsLight(wObject))
			return false;
		if (wObject == T_BOMB)
			return false; //facilitate explosion range highlighting
		if (bIsBriar(wTileNo) && bIsBriar(wObject))
			return true;
		if (bIsHealth(wTileNo) && bIsHealth(wObject))
			return true;
		return wObject == wTileNo;
	case 2:
		//Same type of monster can replace itself, except long monsters and special character.
		if (bIsSerpent(wObject) || wObject == T_CHARACTER || wObject == T_ROCKGIANT)
			return false;
		if (wObject >= TILE_COUNT && wObject < TOTAL_TILE_COUNT)
			return wObject == wTileNo;
		return false;
	default: ASSERT(!"Bad tile layer."); return false;
	}
}

//*****************************************************************************
inline bool bIsEmptyTile(const UINT wObject, const UINT wLayer)
{
	switch (wLayer)
	{
		case LAYER_OPAQUE: return bIsPlainFloor(wObject) || wObject == T_CHECKPOINT;
		case LAYER_TRANSPARENT: return wObject == T_EMPTY;
		case LAYER_MONSTER: return wObject == T_NOMONSTER;
		case LAYER_FLOOR: return wObject == T_EMPTY || wObject == T_EMPTY_F ||
				wObject == T_LIGHT_CEILING || wObject == T_DARK_CEILING ||
				wObject == T_WALLLIGHT;
		default: ASSERT(!"bIsEmptyTile: Bad layer"); return false;
	}
}

//*****************************************************************************
bool CEditRoomWidget::IsSafePlacement(
//For room editing.
//
//Returns: whether placement of currently selected object is allowed at
//specified room square or not.
//
//Params:
	const UINT wSelectedObject,      //(in)   Object to be placed
	const UINT wX, const UINT wY,    //(in)   Coords where placement is pending.
	const UINT /*wO*/,               //(in)   Orientation of object (default = NO_ORIENTATION)
	const bool bAllowSelf)           //(in)   In general, don't allow plotting
									//over same object (default = false).  A value of
									//true is to check the room state for errors,
									//when placement isn't actually occurring.
const
{
	if (!this->pRoom->IsValidColRow(wX,wY))
		return false;  //out of bounds

	if (this->pPlotted[this->pRoom->ARRAYINDEX(wX,wY)])
		return true;   //this part can be replaced

	//Get what's on the other layers to see if they're compatible.
	UINT wTileNo[4];
	const UINT wTileLayer = TILE_LAYER[wSelectedObject];
	const UINT wSquareIndex = this->pRoom->ARRAYINDEX(wX,wY);
	wTileNo[LAYER_OPAQUE] = this->pRoom->pszOSquares[wSquareIndex];
	wTileNo[LAYER_TRANSPARENT] = this->pRoom->pszTSquares[wSquareIndex];
	const CMonster *pMonster = this->pRoom->pMonsterSquares[wSquareIndex];
	wTileNo[LAYER_MONSTER] = pMonster ? pMonster->wType : T_NOMONSTER;
	wTileNo[LAYER_FLOOR] = this->pRoom->pszFSquares[wSquareIndex];

	//Don't allow objects to clobber existing objects on their layer.
	if (wTileLayer == LAYER_MONSTER && wSelectedObject != T_NOMONSTER)
	{
		//Monster can't overwrite a monster of a different type.
		if (!bAllowSelf && pMonster)
			if (pMonster->wType != wSelectedObject - TILE_COUNT ||
				 pMonster->wType == M_CHARACTER) //character can't overwrite itself
				return false;
	} else {
		//Universal placement rules:
		//1. "Empty" tiles can be placed on anything to erase them.
		//2. If the tile being placed is different than what's already there,
		//    it can only be placed if what's there is "empty".
		//3. If the tile being placed is the same as the one already there,
		//    it can be placed if all that's changing is the object's orientation.
		if (!bIsEmptyTile(wSelectedObject, wTileLayer) &&
				(!bAllowSelf || (wTileNo[wTileLayer] != wSelectedObject)) &&
				!bIsEmptyTile(wTileNo[wTileLayer], wTileLayer) &&
				!IsObjectReplaceable(wSelectedObject, wTileLayer, wTileNo[wTileLayer]))
			return false;
	}

	//Don't allow placing anything unsafe under swordsman's starting position.
	bool bSwordsmanAt, bSwordIgnored;
	IsLevelStartAt(wX, wY, bSwordsmanAt, bSwordIgnored);
	if (bSwordsmanAt)
	{
		UINT wTile;
		wTile = wTileLayer == LAYER_OPAQUE ? wSelectedObject : wTileNo[LAYER_OPAQUE];
		if (!(bIsFloor(wTile) || bIsOpenDoor(wTile) || bIsDoor(wTile) ||
				bIsCrumblyWall(wTile) || bIsTunnel(wTile) || bIsPlatform(wTile)))
			return false;

		wTile = wTileLayer == LAYER_TRANSPARENT ? wSelectedObject : wTileNo[LAYER_TRANSPARENT];
		if (bIsTar(wTile) || wTile == T_ORB || wTile == T_BOMB ||
				bIsBriar(wTile) || wTile == T_MAP)
			return false;
		if (pMonster || (wSelectedObject == T_SWORDSMAN && !bAllowSelf))
			return false;
	}

	//Checks on other layers.
	switch (wSelectedObject)
	{
		//Unique layers.
		case T_CHECKPOINT:
		case T_LIGHT_CEILING:
		case T_DARK_CEILING:
			//Can go anywhere.
			return true;

		case T_WALLLIGHT:
			//Can go anywhere except on a light.
			return !bIsWallLightValue(this->pRoom->tileLights.GetAt(wX,wY));

		case T_EMPTY: case T_EMPTY_F:
			//Empty stuff goes with anything.
			return true;

		//O-layer stuff
		case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
		case T_PLATFORM_W: case T_PLATFORM_P:
		case T_TRAPDOOR: case T_TRAPDOOR2:
		case T_GOO:
		case T_HOT:
			//Anything can be on these.
			return true;
		case T_FLOOR: case T_FLOOR_M:
		case T_FLOOR_ROAD: case T_FLOOR_GRASS:
		case T_FLOOR_DIRT: case T_FLOOR_ALT:
		case T_FLOOR_IMAGE:
			//Floor textures don't replace walls.
			if (bIsWall(wTileNo[0]))
				return false;
			if (wTileNo[0] == T_GOO || bIsWater(wTileNo[0]))
				return true;
			break;
		case T_TUNNEL_N: case T_TUNNEL_S: case T_TUNNEL_E: case T_TUNNEL_W:
			//Tunnels -- can only replace other tunnels.
			if (pMonster && wTileNo[2] != M_CHARACTER)
				return false;
			if (IsObjectReplaceable(wSelectedObject, wTileLayer, wTileNo[wTileLayer]))
				return true;
			break;
		case T_PIT: case T_PIT_IMAGE:
			//Pit -- flying monsters can be on it.
			if (bIsPit(wTileNo[0]))
				return true;
			return (wTileNo[1] == T_EMPTY || wTileNo[1] == T_FUSE || wTileNo[1] == T_OBSTACLE ||
					  wTileNo[1] == T_TOKEN || bIsLight(wTileNo[1])) &&
					(!pMonster || wTileNo[2] == M_WWING || wTileNo[2] == M_FEGUNDO || wTileNo[2] == M_CHARACTER);
		case T_WATER:
			//Water -- flying+water monsters can be on it.
			if (bIsWater(wTileNo[0]))
				return true;
			return (wTileNo[1] == T_EMPTY || wTileNo[1] == T_FUSE ||
						wTileNo[1] == T_OBSTACLE || wTileNo[1] == T_TOKEN || bIsLight(wTileNo[1])) &&
					(!pMonster || wTileNo[2] == M_WWING || wTileNo[2] == M_FEGUNDO ||
					 wTileNo[2] == M_CHARACTER || wTileNo[2] == M_WATERSKIPPER);// || wTileNo[2] == M_SKIPPERNEST);
		case T_STAIRS:
		case T_STAIRS_UP:
			//Don't allow stair juxtaposition w/ t-layer items (except tar and obstacles).
			if (!(wTileNo[1] == T_EMPTY || bIsTar(wTileNo[1]) || wTileNo[1] == T_OBSTACLE))
				return false;
			//Allowing stairs under a level start can trap the player in a loop of stairs.
			if (bSwordsmanAt)
				return false;
			if (bIsTar(wTileNo[1]))
				return true;
			break;
		case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_RO:
		case T_DOOR_CO: case T_DOOR_BO: case T_DOOR_MONEYO:
			//Open doors can't have orbs on them, but can have tar.
			if (wTileNo[1] == T_ORB)// || wTileNo[1] == T_STATION)
				return false;
			if (bIsTar(wTileNo[1]) || bIsBriar(wTileNo[1]))
				return true;
			if (IsObjectReplaceable(wSelectedObject, wTileLayer, wTileNo[wTileLayer]))
				return true;
			break;
		case T_PRESSPLATE:
			//Can't place pressure plate under orb (conflicting room data).
			if (wTileNo[1] == T_ORB)
				return false;
			if (bIsTar(wTileNo[1]))
				return true;
			break;
		case T_WALL:
		case T_WALL2:
		case T_WALL_IMAGE:
			if (bIsWall(wTileNo[0]))
				return true;
			// FALL-THROUGH
		case T_WALL_B:
		case T_WALL_H:
			if (bIsBriar(wTileNo[1]))
				return false;
			if (wTileNo[2] != T_NOMONSTER && wTileNo[2] != M_SEEP &&
					!bIsMother(wTileNo[2]) && //tarstuff mothers allowed on walls
					wTileNo[2] != M_CHARACTER)
				return false;
			if (bSwordsmanAt) return false;
			if (wTileNo[1] == T_ORB)
				return false;
			break;
		case T_DOOR_MONEY:
		case T_DOOR_Y:	case T_DOOR_G:	case T_DOOR_C:	case T_DOOR_R:	case T_DOOR_B:
			//Doors can't have orbs on them.
			//But can have tar.
			if (wTileNo[1] == T_ORB)// || wTileNo[1] == T_STATION)
				return false;
			if (bIsTar(wTileNo[1])) return true;
			if (IsObjectReplaceable(wSelectedObject, wTileLayer, wTileNo[wTileLayer]))
				return true;
			break;

		//F-layer stuff
		case T_NODIAGONAL:
			//On anything.
			return true;
		case T_ARROW_NW: case T_ARROW_N: case T_ARROW_NE: case T_ARROW_W:
		case T_ARROW_E: case T_ARROW_SW: case T_ARROW_S: case T_ARROW_SE:
			//Not on stairs.
			return !bIsStairs(wTileNo[0]);

		//T-layer stuff
		case T_ORB:
			//Can't place orbs on pressure plates (conflicting room data).
			if (wTileNo[0] == T_PRESSPLATE)
				return false;
			//On normal floor, wall, goo, tunnels, or pressure plates.
			return !bSwordsmanAt &&
					(bIsPlainFloor(wTileNo[0]) || bIsTrapdoor(wTileNo[0]) ||
						bIsWall(wTileNo[0]) || bIsCrumblyWall(wTileNo[0]) ||
						bIsBridge(wTileNo[0]) || wTileNo[0] == T_HOT ||
						wTileNo[0] == T_GOO || bIsTunnel(wTileNo[0]) ||
						wTileNo[0] == T_PRESSPLATE || bIsPlatform(wTileNo[0])) &&
					(!pMonster || wTileNo[2] == M_CHARACTER);
		case T_BOMB:
			//On normal floor, wall, goo, tunnels, or pressure plates.
			return !bSwordsmanAt &&
				(bIsPlainFloor(wTileNo[0]) || bIsTrapdoor(wTileNo[0]) ||
					bIsWall(wTileNo[0]) || bIsCrumblyWall(wTileNo[0]) ||
					bIsDoor(wTileNo[0]) || bIsOpenDoor(wTileNo[0]) ||
					bIsBridge(wTileNo[0]) || wTileNo[0] == T_HOT ||
					wTileNo[0] == T_GOO || bIsTunnel(wTileNo[0]) ||
					wTileNo[0] == T_PRESSPLATE || bIsPlatform(wTileNo[0])) &&
				(!pMonster || wTileNo[2] == M_CHARACTER);
		case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
			//On normal floor, platforms, goo or water.
			return !bSwordsmanAt &&
					(bIsPlainFloor(wTileNo[0]) || bIsTrapdoor(wTileNo[0]) ||
						bIsOpenDoor(wTileNo[0]) || bIsBridge(wTileNo[0]) ||
						bIsPlatform(wTileNo[0]) ||
						wTileNo[0] == T_HOT || wTileNo[0] == T_GOO ||
						bIsWater(wTileNo[0])) &&
					(!pMonster || wTileNo[2] == M_CHARACTER);
		case T_MIRROR:
			//Only on floor, open doors or platforms.
			return !bSwordsmanAt &&
					(bIsFloor(wTileNo[0]) || bIsWall(wTileNo[0]) || bIsCrumblyWall(wTileNo[0]) ||
						bIsOpenDoor(wTileNo[0]) || bIsDoor(wTileNo[0]) ||
						bIsTunnel(wTileNo[0]) || bIsPlatform(wTileNo[0]) ||
						wTileNo[0] == T_GOO) &&
						(!pMonster || wTileNo[2] == M_CHARACTER);
		case T_LIGHT:
			//Light -- only on floor, walls, pit.
			return !bSwordsmanAt &&
					(bIsFloor(wTileNo[0]) || bIsPit(wTileNo[0]) || bIsWater(wTileNo[0]) ||
					 bIsWall(wTileNo[0]) || bIsCrumblyWall(wTileNo[0])) &&
					!pMonster;
		case T_OBSTACLE:
			//Obstacles -- Can't have monsters on them.
			return !pMonster || wTileNo[2] == M_CHARACTER;
		case T_FUSE:
			//Not on monsters that use/affect the t-layer.
			return !bIsMother(wTileNo[2]);
		case T_HEALTH_SM: case T_HEALTH_MED: case T_HEALTH_BIG:
		case T_DEF_UP:	case T_ATK_UP:
		case T_SCROLL:
		case T_MAP:
			//Not on monsters that use/affect the t-layer.
			if (bIsMother(wTileNo[2])) return false;
			//Can't go on things player never steps on.
			return !(bIsWall(wTileNo[0]) || bIsPit(wTileNo[0]) || bIsWater(wTileNo[0]) ||
					bIsStairs(wTileNo[0]));
		case T_TOKEN:
			//Not on stairs or monsters that use/affect the t-layer.
			return !(bIsStairs(wTileNo[0]) || bIsMother(wTileNo[2]));
		case T_KEY:
		case T_SWORD: case T_SHIELD: case T_ACCESSORY:
			//Not on stairs, pits/water, or monsters that use/affect the t-layer.
			return !(bIsStairs(wTileNo[0]) || bIsPit(wTileNo[0]) || bIsWater(wTileNo[0]) ||
					bIsMother(wTileNo[2]));

		case T_TAR:
			//Can't go on a pit or water.
			//Only tar mother can be on tar.
			if (pMonster && wTileNo[2] != M_TARMOTHER && wTileNo[2] != M_CHARACTER)
				return false;
			if (bSwordsmanAt) return false;
			if (!bAllowSelf && (SDL_GetModState() & KMOD_SHIFT) != 0 &&
					(bIsWall(wTileNo[0]) || bIsCrumblyWall(wTileNo[0]) || bIsDoor(wTileNo[0])))
				return false;	//hold shift while placing to disallow placing on walls
			return !(bIsPit(wTileNo[0]) || bIsWater(wTileNo[0]));
		case T_MUD:
			//Same as tar.
			if (pMonster && wTileNo[2] != M_MUDMOTHER && wTileNo[2] != M_CHARACTER)
				return false;
			if (bSwordsmanAt) return false;
			if (!bAllowSelf && (SDL_GetModState() & KMOD_SHIFT) != 0 &&
					(bIsWall(wTileNo[0]) || bIsCrumblyWall(wTileNo[0]) || bIsDoor(wTileNo[0])))
				return false;	//hold shift while placing to disallow placing on walls
			return !(bIsPit(wTileNo[0]) || bIsWater(wTileNo[0]));
		case T_GEL:
			//Same as tar.
			if (pMonster && wTileNo[2] != M_GELMOTHER && wTileNo[2] != M_CHARACTER)
				return false;
			if (bSwordsmanAt) return false;
			if (!bAllowSelf && (SDL_GetModState() & KMOD_SHIFT) != 0 &&
					(bIsWall(wTileNo[0]) || bIsCrumblyWall(wTileNo[0]) || bIsDoor(wTileNo[0])))
				return false;	//hold shift while placing to disallow placing on walls
			return !(bIsPit(wTileNo[0]) || bIsWater(wTileNo[0]));

		//M-layer stuff
		case T_NOMONSTER:
			//Can always erase monsters.
			return true;
		case T_SERPENT:	//Can be placed on force arrows and scrolls, although they won't move there.
		case T_SERPENTG:
		case T_SERPENTB:
			if (!(wTileNo[1] == T_EMPTY ||
					wTileNo[1] == T_FUSE || wTileNo[1] == T_TOKEN || wTileNo[1] == T_SCROLL))
				return false;
			//Serpents can never overwrite serpents.
			if (pMonster && !bAllowSelf)
				return false;
			//NO BREAK
		case T_ROACH:
		case T_QROACH:
		case T_GOBLIN: case T_GOBLINKING:
		case T_EYE: case T_MADEYE:
		case T_TARBABY: case T_MUDBABY: case T_GELBABY:
		case T_BRAIN:
		case T_SPIDER:
		case T_NEATHER:
		case T_ROCKGOLEM:
		case T_AUMTLICH:
		case T_WUBBA:
		case T_GUARD: case T_PIRATE:
		case T_CLONE: case T_DECOY: case T_MIMIC:
		case T_CITIZEN:
		case T_SKIPPERNEST:
		case T_HALPH: case T_SLAYER:
			//Ground movement types
			if (bSwordsmanAt) return false;
			return (bIsFloor(wTileNo[0]) || bIsDoor(wTileNo[0]) || bIsOpenDoor(wTileNo[0]) ||
					bIsPlatform(wTileNo[0])) &&
					!(wTileNo[1] == T_ORB || bIsTar(wTileNo[1]) || wTileNo[1] == T_BOMB ||
							wTileNo[1] == T_OBSTACLE || bIsBriar(wTileNo[1]));// || wTileNo[1] == T_STATION);
		case T_ROCKGIANT:
			if (bSwordsmanAt) return false;
			//Can't go on (other) monsters.
			if (pMonster && !bAllowSelf)
				return false;
			return (bIsFloor(wTileNo[0]) || bIsDoor(wTileNo[0]) || bIsOpenDoor(wTileNo[0]) ||
					bIsPlatform(wTileNo[0])) &&
					!(wTileNo[1] == T_ORB || bIsTar(wTileNo[1]) || wTileNo[1] == T_BOMB ||
							wTileNo[1] == T_OBSTACLE || bIsBriar(wTileNo[1]));// || wTileNo[1] == T_STATION);
		case T_TARMOTHER:
			//Also can (should) be on tar.
			if (bSwordsmanAt) return false;
			return !(bIsPit(wTileNo[0]) || bIsWater(wTileNo[0])) && //same as for tar
					(wTileNo[1] == T_TAR || wTileNo[1] == T_EMPTY);
		case T_MUDMOTHER:
			//Also can (should) be on mud.
			if (bSwordsmanAt) return false;
			return !(bIsPit(wTileNo[0]) || bIsWater(wTileNo[0])) && //same as for mud
					(wTileNo[1] == T_MUD || wTileNo[1] == T_EMPTY);
		case T_GELMOTHER:
			//Also can (should) be on gel.
			if (bSwordsmanAt) return false;
			return !(bIsPit(wTileNo[0]) || bIsWater(wTileNo[0])) && //same as for gel
					(wTileNo[1] == T_GEL || wTileNo[1] == T_EMPTY);
		case T_WWING:
		case T_FEGUNDO:
			//Air movement types
			if (bSwordsmanAt) return false;
			return (bIsFloor(wTileNo[0]) || bIsDoor(wTileNo[0]) || bIsOpenDoor(wTileNo[0]) || bIsPlatform(wTileNo[0]) ||
					bIsPit(wTileNo[0]) || bIsWater(wTileNo[0])) &&
					!(wTileNo[1] == T_ORB || bIsTar(wTileNo[1]) || wTileNo[1] == T_BOMB ||
							wTileNo[1] == T_OBSTACLE ||
							bIsBriar(wTileNo[1]) || wTileNo[1] == T_LIGHT || wTileNo[1] == T_MIRROR);// || wTileNo[1] == T_STATION);

		case T_SEEP:
			//Wall movement types
			if (bSwordsmanAt) return false;
			if (bIsTar(wTileNo[1])) return false;
			return bIsWall(wTileNo[0]) || bIsCrumblyWall(wTileNo[0]) || bIsDoor(wTileNo[0]);

		case T_WATERSKIPPER:
//		case T_SKIPPERNEST:
			//Water movement types -- allow them to be placed on water or land.
			if (bSwordsmanAt) return false;
			return bIsWater(wTileNo[0]) ||
					((bIsFloor(wTileNo[0]) || bIsOpenDoor(wTileNo[0]) || bIsPlatform(wTileNo[0])) &&
					!(wTileNo[1] == T_ORB || bIsTar(wTileNo[1]) || wTileNo[1] == T_BOMB ||
							wTileNo[1] == T_OBSTACLE || //wTileNo[1] == T_STATION ||
							bIsBriar(wTileNo[1]) || wTileNo[1] == T_LIGHT || wTileNo[1] == T_MIRROR));

		case T_CHARACTER:
			//Can't go on monsters.
			if (pMonster && !bAllowSelf)
				return false;
			return !bSwordsmanAt;
			//Can go on any o- and t-layer tiles.

		case T_SWORDSMAN:
		{
			//copied from above
			if (!(bIsFloor(wTileNo[0]) || bIsTunnel(wTileNo[0]) ||
					bIsDoor(wTileNo[0]) || bIsOpenDoor(wTileNo[0]) ||
					bIsTunnel(wTileNo[0]) || bIsPlatform(wTileNo[0]) ||
					bIsWall(wTileNo[0]) || bIsCrumblyWall(wTileNo[0]) ||
					bIsPit(wTileNo[0]) || bIsWater(wTileNo[0])) ||
					wTileNo[1] == T_ORB || bIsTar(wTileNo[1]) ||
					wTileNo[1] == T_BOMB || wTileNo[1] == T_OBSTACLE || bIsBriar(wTileNo[1]) || //wTileNo[1] == T_STATION ||
					pMonster)
				return false;
			return true;
		}

		default: break;
	}

	if (bIsSerpentTile(wSelectedObject))
	{
		//Serpent pieces -- can't be plotted over monsters or pieces.
		return wTileNo[1] == T_EMPTY || bIsFloor(wTileNo[0]) || bIsOpenDoor(wTileNo[0]);
	}

	//Allow placement if square is empty.
	if (wTileNo[wTileLayer] == wSelectedObject &&
			(bAllowSelf || IsObjectReplaceable(wSelectedObject, wTileLayer, wTileNo[wTileLayer])))
		return true;
	return bIsEmptyTile(wTileNo[wTileLayer], wTileLayer);
}

//*****************************************************************************
bool CEditRoomWidget::LoadFromRoom(
//Loads widget from a room.
//
//Params:
	CDbRoom *pRoom, vector<CMoveCoord*>* pLevelEntrances)
//
//Returns:
//True if successful, false if not.
{
	ASSERT(pRoom);
	this->pRoom = pRoom;
	this->pLevelEntrances = pLevelEntrances;
	this->pRoom->GetDoubleSwordCoords(this->swords);

	//Update vars used for comparison of widget to current game.
	this->style = pRoom->style;
	this->dwRoomX = pRoom->dwRoomX;
	this->dwRoomY = pRoom->dwRoomY;

	this->pCurrentGame = NULL;
	HidePlayer();
	ClearEffects();

	//Load tile images.
	if (!g_pTheDBM->LoadTileImagesForStyle(this->style))
	{
		this->pRoom = NULL;
		this->pLevelEntrances = NULL;
		return false;
	}

	LoadRoomImages();

	delete[] this->pPlotted;
	this->pPlotted = new bool[pRoom->CalcRoomArea()];
	ResetPlot();

	//Set tile image arrays to new current room.
	ResetForPaint();

	return true;
}

//*****************************************************************************
void CEditRoomWidget::Paint(
//Plots current room to display.
//Displays editing state.
//
//Params:
	bool bUpdateRect)       //(in)   If true (default) and destination
								//    surface is the screen, the screen
								//    will be immediately updated in
								//    the widget's rect.
{
	//Drawing code below needs to be modified to accept offsets.  Until then,
	//this widget can't be offset.
	ASSERT(!IsScrollOffset());

	if (!this->pRoom)
	{
		DrawPlaceholder();
		return;
	}

	//1a. Render room image now, if requested.
	if (this->bRenderRoom)
	{
		RenderRoom(this->wShowCol, this->wShowRow);
		this->bRenderRoom = false;
	}

	//1b. Blit pre-rendered room.
	SDL_Surface *pDestSurface = GetDestSurface();
	ASSERT(this->pRoomSnapshotSurface);
	ASSERT(this->pTileImages);
	TileImages *pTI = this->pTileImages;
	TileImages *const pTIStop = pTI +
			CDrodBitmapManager::DISPLAY_ROWS * CDrodBitmapManager::DISPLAY_COLS;
	if (this->bAllDirty)
	{
		//Re-blit entire room.
		SDL_Rect src = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
		SDL_Rect dest = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
		SDL_BlitSurface(this->pRoomSnapshotSurface, &src, pDestSurface, &dest);

		//Undirty all room tiles.
		while (pTI != pTIStop)
			(pTI++)->dirty = 0;
	} else {
		//Blit only dirtied room tiles from last turn to save time.
		//Undirty them (they will be marked as "damaged" to indicate they must be
		//updated on the screen below).
		while (pTI != pTIStop)
			(pTI++)->damaged = 0;
		BlitDirtyRoomTiles(false);
	}
	pTI = this->pTileImages;

	//1c. Render dynamic stuff that shows over the static room image.
	RenderFogInPit(pDestSurface);
	DrawTLayer(pDestSurface, true);

	//2. Draw effects that go on top of room image, under monsters/swordsman.
	this->pTLayerEffects->UpdateAndDrawEffects();
	this->pTLayerEffects->DirtyTiles();

	//3. Repaint monsters.
	AnimateMonsters();
	if (this->bAllDirty)
	{
		//Draw all monsters.
		DrawMonsters(this->pRoom->pFirstMonster, pDestSurface, false);
	} else {
		//Paint monsters whose tiles have been repainted.
		DrawDamagedMonsters(pDestSurface);
	}

	//4. Draw level entrance positions.
	DrawLevelEntrances(pDestSurface);

	//5a. Draw effects that go on top monsters/swordsman.
	this->pMLayerEffects->UpdateAndDrawEffects();
	this->pMLayerEffects->DirtyTiles();

	//5b. Draw effects that go on top of everything else drawn in the room.
	this->pLastLayerEffects->UpdateAndDrawEffects();
	this->pLastLayerEffects->DirtyTiles();

	//Orb/door editing: show lightning from orb/door to mouse cursor.
	if (this->wOX != NO_BOLT)
	{
		static const UINT CX_TILE_HALF = CX_TILE / 2;
		static const UINT CY_TILE_HALF = CY_TILE / 2;
		DrawBoltInRoom(this->x + this->wOX * CX_TILE + CX_TILE_HALF,
				this->y + this->wOY * CY_TILE + CY_TILE_HALF,
				this->x + this->wEndX * CX_TILE + CX_TILE_HALF,
				this->y + this->wEndY * CY_TILE + CY_TILE_HALF);
		const UINT xBuffer = this->wOX < this->wEndX ? 2 : -2;
		const UINT yBuffer = this->wOY < this->wEndY ? 3 : -3;
		DirtyTileRect(this->wOX - xBuffer, this->wOY - yBuffer,
				this->wEndX + xBuffer, this->wEndY + yBuffer);
	}

	//If any widgets are attached to this one, draw them now.
	PaintChildren();

	//6. Show changes on screen.
	if (bUpdateRect)
		UpdateRoomRects();

	//Everything has been (re)painted by now.
	this->bAllDirty = false;
}

//*****************************************************************************
bool CEditRoomWidget::Plotted(const UINT wCol, const UINT wRow) const
{
	ASSERT(this->pPlotted);
	return this->pPlotted[this->pRoom->ARRAYINDEX(wCol,wRow)];
}

//*****************************************************************************
void CEditRoomWidget::ResetPlot()
//Reset plotted flag on all squares.
{
	memset(this->pPlotted, false, this->pRoom->CalcRoomArea() * sizeof(bool));
}

//*****************************************************************************
void CEditRoomWidget::ResetRoom()
//Unload a loaded room.
{
	this->pRoom = NULL;
   this->pCurrentGame = NULL;
}

//*****************************************************************************
void CEditRoomWidget::SetPlot(
//Mark a square as having been plotted.
//
//Params:
	const UINT wCol, const UINT wRow)   //(in)   Square to set plot flag for.
{
	ASSERT(this->pRoom->IsValidColRow(wCol,wRow));

	this->pPlotted[this->pRoom->ARRAYINDEX(wCol,wRow)] = true;
}

//*****************************************************************************
bool CEditRoomWidget::SkyWillShow() const
//Returns: true, to always be prepared to show sky while editing the room
{
	return true;
}

//
// Protected methods
//

//*****************************************************************************
void CEditRoomWidget::DrawCharacter(
//Draws character as Negotiator for editor, or in-game type when Alt is held.
//
//Params:
	CCharacter *pCharacter,    //(in)   Pointer to CCharacter monster.
	const bool bDrawRaised,    //(in)   Draw Character raised above floor?
	SDL_Surface *pDestSurface, //(in)   Surface to draw to.
	const bool /*bMoveInProgress*/)
{
	const bool bAlt = (SDL_GetModState() & KMOD_ALT) != 0;
	const UINT wIdentity = pCharacter->GetIdentity();
	UINT wFrameIndex = this->pTileImages[this->pRoom->ARRAYINDEX(pCharacter->wX, pCharacter->wY)].animFrame % ANIMATION_FRAMES;
	if (wIdentity != M_NONE && IsAnimated() && bAlt)
	{
		//Draw NPC's actual in-game image.
		ASSERT(wIdentity < MONSTER_COUNT ||
				(wIdentity >= CHARACTER_FIRST && wIdentity < CHARACTER_TYPES) ||
				wIdentity == M_NONE);
		const UINT wO = wIdentity == M_BRAIN || wIdentity == M_SKIPPERNEST ?
				NO_ORIENTATION : pCharacter->wO;

		//If a sword-wielding character is swordless, try to get its swordless frame.
		//However, if that frame is not defined or otherwise, then show the current animation frame.
		UINT wSX, wSY;
		const bool bHasSword = pCharacter->GetSwordCoords(wSX, wSY);
		const UINT wFrame = bHasSword || !bEntityHasSword(wIdentity) ? wFrameIndex : SWORDLESS_FRAME;
		const UINT wTileImageNo = GetEntityTile(wIdentity, pCharacter->wLogicalIdentity, wO, wFrameIndex); //wFrame?

		//Draw character.
		const Uint8 opacity = pCharacter->IsVisible() ? 255 : 128;
		TileImageBlitParams blit(pCharacter->wX, pCharacter->wY, wTileImageNo, 0, 0, true, bDrawRaised);
		blit.nOpacity = opacity;
		blit.nAddColor = pCharacter->getColor();
		DrawTileImage(blit, pDestSurface);

		//Draw character with sword.
		if (bHasSword) {
			blit.wCol = wSX;
			blit.wRow = wSY;
			blit.nAddColor = -1;
			DrawSwordFor(pCharacter, wIdentity, blit, pDestSurface);
		}
	} else {
		//Draw generic NPC image.
		const UINT wTileImageNo = GetTileImageForEntity(pCharacter->wType, pCharacter->wO, wFrameIndex);
		TileImageBlitParams blit(pCharacter->wX, pCharacter->wY, wTileImageNo, 0, 0, true, wIdentity != M_NONE ? bDrawRaised : false);
		if (bAlt && !pCharacter->IsVisible())
			blit.nOpacity = 128;
		DrawTileImage(blit, pDestSurface);
	}
}

bool CEditRoomWidget::DrawingSwordFor(const CMonster *pMonster) const {
	switch (pMonster->wType) {
		case M_CHARACTER:
		{
			const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pMonster);
			return pCharacter->GetIdentity() != M_NONE && IsAnimated();
		}
	}

	return CRoomWidget::DrawingSwordFor(pMonster);
}

//
// Private methods
//

//*****************************************************************************
void CEditRoomWidget::DrawLevelEntrances(SDL_Surface *pDestSurface)
{
	if (!this->pLevelEntrances) return;

	CSwordsman swordsman;
	swordsman.st.clear();
//	swordsman.bIsVisible = true;

	CMoveCoord *pEntrance;
	for (UINT wIndex=0; wIndex<this->pLevelEntrances->size(); ++wIndex)
	{
		pEntrance = (*this->pLevelEntrances)[wIndex];
		swordsman.wX = pEntrance->wX;
		swordsman.wY = pEntrance->wY;
//		swordsman.wAppearance = M_BEETHRO;
		swordsman.SetOrientation(pEntrance->wO);
		TileImageBlitParams blit(swordsman.wX, swordsman.wY, TI_CHECKPOINT, 0, 0, true);
		DrawTileImage(blit, pDestSurface);
		DrawPlayer(swordsman, pDestSurface);
	}
}

//*****************************************************************************
void CEditRoomWidget::DrawMonsters(
//Draws monsters.
//
//Params:
	CMonster *const pMonsterList, //(in)   Monsters to draw.
	SDL_Surface *pDestSurface,
	const bool bMoveInProgress)   //
{
	vector<CMonster*> drawnMonsters;
	CMonster *pMonster = pMonsterList;
	while (pMonster)
	{
		drawnMonsters.push_back(pMonster);

		DrawMonster(pMonster, this->pRoom, pDestSurface, bMoveInProgress);
		pMonster = pMonster->pNext;
	}

	DrawSwordsFor(drawnMonsters, pDestSurface);
}

//******************************************************************************
inline void GetMinMax(
//Calculates the min and max of two vars.
//
//Params:
	const UINT v1, const UINT v2, //(in)
	UINT &vMin, UINT &vMax)       //(out)
{
	vMin = min(v1,v2);
	vMax = max(v1,v2);
}

//*****************************************************************************
void CEditRoomWidget::HandleMouseDown(
//Handles a mouse down event.
//
//Params:
	const SDL_MouseButtonEvent &Button) //(in)   Event to handle.
{
	this->wDownX = this->wStartX = this->wMidX = this->wEndX = (Button.x - this->x) / CX_TILE;
	this->wDownY = this->wStartY = this->wMidY = this->wEndY = (Button.y - this->y) / CY_TILE;
}

//*****************************************************************************
void CEditRoomWidget::HandleDrag(
//Handles a mouse drag event.
//
//Params:
	const SDL_MouseMotionEvent &Motion)
{
	//Don't update coords when outside widget.
	if (Motion.x < this->x || Motion.y < this->y) return;
	if ((UINT)Motion.x >= this->x + this->w || (UINT)Motion.y >= this->y + this->h) return;

	//When placing only one object, keep it located at mouse cursor.
	if (this->bSinglePlacement)
	{
		this->wStartX = (Motion.x - this->x) / CX_TILE;
		this->wStartY = (Motion.y - this->y) / CY_TILE;
	}

	//Preserve mouse down position.
	GetMinMax(this->wStartX, (Motion.x - this->x) / CX_TILE,
			this->wMidX, this->wEndX);
	GetMinMax(this->wStartY, (Motion.y - this->y) / CY_TILE,
			this->wMidY, this->wEndY);
}

//*****************************************************************************
void CEditRoomWidget::HandleMouseMotion(
//Handles a mouse motion event.
//
//Params:
	const SDL_MouseMotionEvent &Motion)
{
	if (this->bPlacing) return;

	//Don't update coords when outside widget.
	if (Motion.x < this->x || Motion.y < this->y) return;
	if ((UINT)Motion.x >= this->x + this->w || (UINT)Motion.y >= this->y + this->h) return;

	this->wEndX = (Motion.x - this->x) / CX_TILE;
	this->wEndY = (Motion.y - this->y) / CY_TILE;
}

//*****************************************************************************
void CEditRoomWidget::HandleMouseUp(
//Handles a mouse up event.
//
//Params:
	const SDL_MouseButtonEvent &Button) //(in)   Event to handle.
{
	ASSERT(this->pRoom);

	//Don't update coords when outside widget.
	this->bMouseInBounds = false;
	if (Button.x < this->x || Button.y < this->y) return;
	if ((UINT)Button.x >= this->x + this->w || (UINT)Button.y >= this->y + this->h) return;

	const bool bCtrlKey = (SDL_GetModState() & KMOD_CTRL) != 0;

	this->bMouseInBounds = true;
	const UINT x1 = this->wStartX;
	const UINT y1 = this->wStartY;
	const UINT x2 = (Button.x - this->x) / CX_TILE;
	const UINT y2 = (Button.y - this->y) / CY_TILE;
	ASSERT(this->pRoom->IsValidColRow(x2,y2));

	if (bCtrlKey)
	{
		//Query only.
		DisplayRoomCoordSubtitle(x2,y2);
	} else {
		//Placing objects -- w/o Ctrl key only.
		GetMinMax(x1,x2,this->wStartX,this->wEndX);
		GetMinMax(y1,y2,this->wStartY,this->wEndY);
	}

	//Left mouse button shows special states.
	if (Button.button != SDL_BUTTON_LEFT) return;
	if (!this->bPlacing)
		return;
	if (this->eEditState != ES_PLACING)
		return;

/*
	CMonster *pMonster = this->pRoom->GetMonsterAtSquare(x2,y2);
	if (pMonster)
		switch (pMonster->wType)
		{
			case M_EYE:
				//Show all eye gazes in editor.
				this->pLastLayerEffects->RemoveEffectsOfType(EEVILEYEGAZE);

				pMonster = this->pRoom->pFirstMonster;
				while (pMonster)
				{
					if (pMonster->wType == M_EYE)
						AddLastLayerEffect(new CEvilEyeGazeEffect(
								this,pMonster->wX,pMonster->wY,pMonster->wO,
								bCtrlKey ? 0 : 1500)); //permanent or not
					pMonster = pMonster->pNext;
				}
			return;
			default: break;
		}
*/

	switch (this->pRoom->GetTSquare(x2,y2))
	{
		case T_BOMB:
		{
			//Show all explosions caused by this bomb.
			this->pLastLayerEffects->RemoveEffectsOfType(ESHADE);

			CCueEvents CueEvents;
			CDbRoom room(*this->pRoom);
			room.InitRoomStats();
			CCoordStack bombs(x2,y2);
			room.BombExplode(CueEvents, bombs);

			CCoordSet coords;
			const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*,
				CueEvents.GetFirstPrivateData(CID_Explosion));
			while (pCoord)
			{
				coords.insert(pCoord->wX, pCoord->wY);
				pCoord = DYN_CAST(const CCoord*, const CAttachableObject*,
						CueEvents.GetNextPrivateData());
			}
			static const SURFACECOLOR ExpColor = {224, 160, 0};
			for (CCoordSet::const_iterator coord=coords.begin(); coord!=coords.end(); ++coord)
				AddShadeEffect(coord->wX, coord->wY, ExpColor);
		}
		return;
		default: break;
	}
}

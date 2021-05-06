// $Id: EditRoomWidget.h 8463 2008-01-08 03:47:01Z mrimer $

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

#ifndef EDITROOMWIDGET_H
#define EDITROOMWIDGET_H

//This derived class provides editing support in the CRoomWidget.

#include "RoomWidget.h"

//Conditions of the editing process.
enum EditState
{
	ES_PLACING,    //standard state: placing objects
	ES_ORB,        //setting orb agents
	ES_DOOR,       //setting orb agents for one door
	ES_SCROLL,     //entering scroll text
	ES_TESTROOM,   //selecting position to begin playing room
	ES_LONGMONSTER,//laying long monster segments
	ES_GETSQUARE,  //query for a room square
	ES_GETRECT,    //query for a room rectangle
	ES_GETMAPROOM, //query for a room from the minimap
	ES_PASTING     //next click pastes a copied room region
};

//Plotting a long monster segment in the editor.
struct MonsterSegment {
	UINT wHeadX, wHeadY; //head position of long monster
	UINT wTailX, wTailY; //current tail position of long monster
	UINT wSX, wSY;       //start of segment
	UINT wEX, wEY;       //end of segment
	UINT wDirection;     //direction of segment, from head to tail
	bool bHorizontal;    //whether segment is horiz. or vert.
	UINT wType;          //monster type
};

class CMoveCoord;
class CEditRoomWidget : public CRoomWidget
{
public:
	CEditRoomWidget(UINT dwSetTagNo, int nSetX, int nSetY, UINT wSetW, UINT wSetH);

	void           AddOrbAgentsEffect(COrbData *pOrb, const bool bEditingOrb = true);
	bool           AddOrbEffect(COrbAgentData *pOrbAgent);
	void           AddOrbAgentToolTip(const UINT wX, const UINT wY,
			const UINT wAgentType);
	void           AddMonsterSegmentEffect(const CMonster* pPlottingMonster, const UINT wMonsterType);
	void           AddPendingPasteEffect(const UINT wX1, const UINT wY1,
			const UINT wXSize, const UINT wYSize, const SURFACECOLOR& color,
			const bool bEraseOldShading);
	void           AddPendingPlotEffect(const UINT wObjectNo,
			const UINT* pwTileImageNo, const UINT wXSize=1, const UINT wYSize=1,
			const bool bSinglePlacement=false, const UINT wO=NO_ORIENTATION);
	void           AddToolTipEffect(const UINT wX, const UINT wY,
			const MESSAGE_ID messageID);

	virtual void   DrawMonsters(CMonster *const pMonsterList,
			SDL_Surface *pDestSurface, const bool bMoveInProgress=false);

	UINT           GetSerpentStraightTile(const UINT wX, const UINT wY,
			const UINT wDirection, const bool bShow) const;
	UINT           GetSerpentTailTile(const UINT wX, const UINT wY,
			const UINT wDirection, const bool bShow) const;
	UINT           GetSerpentTurnTile(const UINT wX, const UINT wY,
			const UINT wDirection, const bool bShow) const;

	void           IsLevelStartAt(const UINT wX, const UINT wY,
			bool &bSwordsmanAt, bool &bSwordAt) const;
	bool           IsObjectReplaceable(const UINT wObject, const UINT wTileLayer,
			const UINT wTileNo) const;
	virtual bool   IsPlayerLightRendered() const {return false;}
	bool           IsSafePlacement(const UINT wSelectedObject,
			const UINT wX, const UINT wY, const UINT wO=NO_ORIENTATION,
			const bool bAllowSelf=false) const;

	bool           LoadFromRoom(CDbRoom *pRoom, vector<CMoveCoord*>* pLevelEntrances);

	virtual void   Paint(bool bUpdateRect = true);

	bool           Plotted(const UINT wCol, const UINT wRow) const;
	void           ResetPlot();
	void           ResetRoom();
	virtual void   SetPlot(const UINT wCol, const UINT wRow);
	virtual bool	SkyWillShow() const;

	UINT           wDownX, wDownY;   //where mouse is when button is pressed
	UINT           wStartX, wStartY; //a starting tile coord
	UINT           wMidX, wMidY;     //set to upper-left corner during drag
	UINT           wEndX, wEndY;     //an ending tile coord
	bool           bMouseInBounds;   //whether plot is active
	bool           bSinglePlacement; //whether only one object is to be placed
	bool           bPlacing;         //in placing object state
	EditState      eEditState;       //used in connection with room editor

	vector<CMoveCoord*>  *pLevelEntrances;  //entrances in the current room

	UINT           wOX, wOY;         //selected orb position
	MonsterSegment monsterSegment;   //long monster being plotted
	CCoordIndex    swords;           //double swords in room

protected:
	virtual  ~CEditRoomWidget();

	virtual void   DrawCharacter(CCharacter *pCharacter, const bool bDrawRaised,
			SDL_Surface *pDestSurface, const bool bMoveInProgress);
	virtual bool   DrawingSwordFor(const CMonster *pMonster) const;

	virtual void   HandleAnimate() {if (this->pRoom) Paint(false);}   //parent must handle calling UpdateRect()

private:
	void           DrawLevelEntrances(SDL_Surface *pDestSurface);

	virtual void   HandleMouseDown(const SDL_MouseButtonEvent &Button);
	virtual void   HandleDrag(const SDL_MouseMotionEvent &Motion);
	virtual void   HandleMouseMotion(const SDL_MouseMotionEvent &Motion);
	virtual void   HandleMouseUp(const SDL_MouseButtonEvent &Button);

	bool *            pPlotted;   //level editor: where an object is placed
};

#endif //#ifndef EDITROOMWIDGET_H

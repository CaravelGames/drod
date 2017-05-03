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
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef BRIAR_H
#define BRIAR_H

#include <BackEndLib/CoordIndex.h>
#include <BackEndLib/CoordSet.h>
#include <BackEndLib/CoordStack.h>
#include "CueEvents.h"
#include <list>
#include <utility>
#include <vector>

class CDbRoom;

typedef std::pair<CCoord,CCoord> CoordPair;

//*****************************************************************************
class CBriars;
class CBriar
{
public:
	CBriar(CBriars *pBriars, const UINT wBriarIndex, const UINT wX, const UINT wY);

	bool process();

	UINT wComponentIndex; //index of component this source is connected to
	UINT wX, wY;     //location of this source
	bool bStuck;     //has filled all available area and can't expand further
	bool bDone;      //has marked an edge tile this turn

private:
	CBriars *pBriars;  //parent object
};

//*****************************************************************************
class CBriars
{
public:
	CBriars();
	~CBriars();

	void clear();
	bool empty() const;
	
	void forceRecalc() { bRecalc = true; }

	const CCoordSet& getEdgeTilesFor(const UINT wX, const UINT wY) const;
	UINT getIndexAt(const UINT wX, const UINT wY) const;
	UINT getNumSourcesWithIndex(const UINT index) const;
	void initLiveTiles();
	void insert(const UINT wX, const UINT wY);
	void plotted(const UINT wX, const UINT wY, const UINT wTileNo);
	void process(CCueEvents &CueEvents);
	void removeSource(const UINT wX, const UINT wY);
	void setMembersForRoom(const CBriars& src, CDbRoom* pRoom);
	void setRoom(CDbRoom* pRoom);

private:
	void expand(CCueEvents &CueEvents, const UINT wIndex, CCoordSet &killedPuffs,
			CCoordStack &powder_kegs);

	friend class CBriar;
	CDbRoom     *pRoom;
	std::list<CBriar*> briars;   //sources in the room
	CCoordIndex_T<USHORT>  briarIndices;  //quick access to which component is at what tile
	std::vector<CCoordSet> briarComponents, briarEdge; //connected components
	std::vector<CoordPair> connectedBriars;     //tile pairs where two components connect
	CCoordSet pressurePlates;  //set of pressure plate tiles depressed on a turn
	bool bRecalc;              //indicates components must be reconstructed
};

#endif //...#ifndef BRIAR_H

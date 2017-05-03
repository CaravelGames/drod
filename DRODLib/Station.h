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

//Class for handling station game logic.

#ifndef STATION_H
#define STATION_H

#include <BackEndLib/Types.h>
#include <BackEndLib/CoordIndex.h>

class CDbRoom;
class CStation
{
public:
	CStation(const UINT wX, const UINT wY, CDbRoom* const pRoom);
	CStation(const CStation& src, CDbRoom* pRoom);

	UINT GetDirectionFrom(const UINT wX, const UINT wY) const;
	UINT GetDistanceFrom(const UINT wX, const UINT wY) const;
	inline UINT GetType() const {return this->wType;}
	void RecalcPathmap();
	bool UpdateTurn(const UINT wTurnNo);
	inline UINT X() const {return this->wX;}
	inline UINT Y() const {return this->wY;}

private:
	void CalcPathmap();
	inline bool IsObstacle(const UINT wDestF, const UINT wX, const UINT wY,
			const UINT wOrientation) const;

	CDbRoom *pRoom;
	UINT wX, wY;
	UINT wType;     //to designate sets of stations

	CCoordIndex_T<UINT> pathmap, distance;

	static UINT wLastTurnInit; //speed optimization
	static CCoordIndex swords;
	bool bRecalcPathmap;
};

#endif //...#ifndef STATION_H

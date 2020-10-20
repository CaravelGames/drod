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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef CVERMINEFFECT_H
#define CVERMINEFFECT_H

#include "DrodEffect.h"
#include <BackEndLib/Coord.h>

struct VERMIN
{
	float fX, fY, fAngle;   //position, direction (in radians)
	float duration;         //how long the vermin will be alive
	enum ACCELERATION
	{
		HardLeft=0,
		Left,
		Forward,
		Right,
		HardRight,
		NUM_ACCELERATIONS
	};
	ACCELERATION acceleration; //change in direction
	UINT wSize;          //sprite size
	UINT wTileNo;        //picture of vermin displayed
	bool bActive;
};

//*****************************************************************************
class CRoomWidget;
class CDbRoom;
class CVerminEffect : public CEffect
{
public:
	CVerminEffect(CWidget *pSetWidget, const CMoveCoord &origin,
		const UINT wNumVermin=5, const bool bSlayer=false);

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& pDestSurface);

private:
	void MarkVerminInactive(const UINT wIndex);
	bool OutOfBounds(const VERMIN &v) const;
	bool HitsObstacle(const CDbRoom *pRoom, const VERMIN &v) const;
	void UpdateDirection(VERMIN &v);

	vector<VERMIN> vermin;
	CRoomWidget *pRoomWidget;
	SDL_Rect screenRect;
	bool bSlayer;	//slayer pieces

	static UINT dwMaxDuration;
	static UINT dwDurationSway; // By how much to spread the max duration of each vermin
};

#endif   //...#ifndef CVERMINEFFECT_H

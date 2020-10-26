// $Id: VerminEffect.cpp 9972 2012-03-20 03:00:05Z mrimer $

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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//Adds a "vermin" to the display that will move in a specified pattern.

#include "VerminEffect.h"
#include "DrodBitmapManager.h"
#include "RoomWidget.h"
#include "../DRODLib/GameConstants.h"
#include "../DRODLib/TileConstants.h"
#include "../DRODLib/DbRooms.h"
#include <math.h>

UINT CVerminEffect::dwMaxDuration = 3000;
UINT CVerminEffect::dwDurationSway = 1000;

//*****************************************************************************
CVerminEffect::CVerminEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,       //(in) Should be a room widget.
	const CMoveCoord &origin,  //(in) Location and initial direction of movement.
	const UINT wNumVermin,     //(in) [default=5]
	const bool bSlayer)			//(in) [default=false]
	: CEffect(pSetWidget, EVERMIN)
	, bSlayer(bSlayer)
{
	ASSERT(pSetWidget->GetType() == WT_Room);
	this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);
	ASSERT(this->pRoomWidget);

	pSetWidget->GetRect(this->screenRect);

	//Generate randomly oriented vermin.
	const int nX = this->screenRect.x + origin.wX*CBitmapManager::CX_TILE + CBitmapManager::CX_TILE/2;
	const int nY = this->screenRect.y + origin.wY*CBitmapManager::CY_TILE + CBitmapManager::CY_TILE/2;
	for (UINT wIndex=wNumVermin; wIndex--; )
	{
		VERMIN v;
		v.fX = static_cast<float>(nX);
		v.fY = static_cast<float>(nY);
		v.fAngle = fRAND(TWOPI);
		v.duration = float(CVerminEffect::dwMaxDuration) - RAND(CVerminEffect::dwDurationSway);
		v.acceleration = (VERMIN::ACCELERATION)RAND(VERMIN::NUM_ACCELERATIONS);
		v.wTileNo = bSlayer ? TI_SLAYERDEBRIS : RAND(2) == 0 ? TI_VERMIN_1 : TI_VERMIN_2;
		v.wSize = bSlayer ? 8 : v.wTileNo == TI_VERMIN_1 ? 6 : 4;
		v.bActive = true;

		this->vermin.push_back(v);
		//Bounding box is always of these dimensions.
		SDL_Rect rect = MAKE_SDL_RECT(v.fX, v.fY, v.wSize, v.wSize);
		this->dirtyRects.push_back(rect);
	}
}

//*****************************************************************************
bool CVerminEffect::Draw(SDL_Surface* pDestSurface)
//Draw the effect.
//
//Returns:
//True if effect should continue, or false if effect is done.
{
	if (!pDestSurface) pDestSurface = GetDestSurface();
  
	const Uint32 timeElapsed = TimeElapsed();
	if (timeElapsed > CVerminEffect::dwMaxDuration)
		return false;

	const Uint32 dwNow = SDL_GetTicks();
	Uint32 dwFrameTime = dwNow <= this->dwTimeOfLastMove ? 1 :
			dwNow - this->dwTimeOfLastMove;
	this->dwTimeOfLastMove = dwNow;
	const float fMultiplier = (dwFrameTime < 50 ? dwFrameTime : 50) / 8.0f;

	const CDbRoom *pRoom = this->pRoomWidget->GetCurrentGame()->pRoom;
	ASSERT(pRoom);

	for (UINT wIndex=this->vermin.size(); wIndex--; )
	{
		VERMIN& v = this->vermin[wIndex];

		if (!v.bActive)
			continue;

		if (timeElapsed > v.duration) {
			MarkVerminInactive(wIndex);
			continue;
		}

		v.fX += static_cast<float>(cos(v.fAngle)) * fMultiplier;
		v.fY += static_cast<float>(sin(v.fAngle)) * fMultiplier;

		if (OutOfBounds(v) || HitsObstacle(pRoom, v))
		{
			MarkVerminInactive(wIndex);
			continue;
		}

		UpdateDirection(v);

		g_pTheBM->BlitTileImagePart(v.wTileNo, static_cast<UINT>(v.fX), static_cast<UINT>(v.fY),
											 0, 0, v.wSize, v.wSize, pDestSurface, true);

		//Update bounding box position.
		this->dirtyRects[wIndex].x = static_cast<Sint16>(v.fX);
		this->dirtyRects[wIndex].y = static_cast<Sint16>(v.fY);
	}

	return true;
}

//*****************************************************************************
void CVerminEffect::MarkVerminInactive(const UINT wIndex)
{
	this->vermin[wIndex].bActive = false;

	//Don't need to dirty anything for inactive vermin.
	this->dirtyRects[wIndex].w = this->dirtyRects[wIndex].h = 0;
}

//*****************************************************************************
inline bool CVerminEffect::OutOfBounds(const VERMIN &v) const
{
	return (v.fX < this->screenRect.x || v.fY < this->screenRect.y ||
			v.fX >= this->screenRect.x + this->screenRect.w - v.wSize ||
			v.fY >= this->screenRect.y + this->screenRect.h - v.wSize);
}

//*****************************************************************************
inline bool CVerminEffect::HitsObstacle(const CDbRoom *pRoom, const VERMIN &v) const
//Returns: whether the vermin is standing on tiles it will run over (false), or
//is on something it will disappear under (true)
{
	const UINT wX = ((Sint16)v.fX - screenRect.x) / CBitmapManager::CX_TILE;
	const UINT wY = ((Sint16)v.fY - screenRect.y) / CBitmapManager::CY_TILE;
	switch (pRoom->GetOSquare(wX,wY))
	{
		case T_FLOOR: case T_FLOOR_M:	case T_FLOOR_ROAD: case T_FLOOR_GRASS:
		case T_FLOOR_DIRT: case T_FLOOR_ALT: case T_FLOOR_IMAGE:
		case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO:	case T_DOOR_RO: case T_DOOR_BO: case T_DOOR_MONEYO:
		case T_TRAPDOOR: case T_TRAPDOOR2:
		case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
		case T_GOO:
		case T_PRESSPLATE: case T_PLATFORM_P: case T_PLATFORM_W:
			break;  //vermin can move on these things
		case T_HOT: //slayer particles may move on hot tiles and closed doors
		case T_DOOR_Y: case T_DOOR_G: case T_DOOR_C: case T_DOOR_R: case T_DOOR_B: case T_DOOR_MONEY:
			if (!this->bSlayer)
				return true;
			break;
		default:
			return true;
	}
	switch (pRoom->GetTSquare(wX,wY))
	{
		case T_OBSTACLE:
		case T_TAR:	case T_MUD: case T_GEL:
			return true;	//vermin won't go through these things
		default:
			return false;
	}
}

//*****************************************************************************
inline void CVerminEffect::UpdateDirection(VERMIN &v)
//Change movement direction according to current acceleration.
{
	ASSERT(v.acceleration < VERMIN::NUM_ACCELERATIONS);
	switch (v.acceleration)
	{
		case VERMIN::HardLeft: v.fAngle -= 0.2f; break;
		case VERMIN::Left: v.fAngle -= 0.1f; break;
		case VERMIN::Forward: break;
		case VERMIN::Right: v.fAngle += 0.1f; break;
		case VERMIN::HardRight: v.fAngle += 0.2f; break;
		case VERMIN::NUM_ACCELERATIONS: break;
	}

	//Randomly change acceleration state according to Markov process.
	static const UINT markovProbs[2][VERMIN::NUM_ACCELERATIONS][VERMIN::NUM_ACCELERATIONS] = {
	{
		//Normal Vermin
		{55, 25,  5, 10,  5},
		{15, 55,  5, 15, 10},
		{ 0, 30, 40, 30,  0},
		{10, 15,  5, 55, 15},
		{ 5, 10,  5, 25, 55}
	},{
		//Slayer pieces
		{30, 40,  5, 15, 10},
		{15, 45,  5, 25, 10},
		{ 0, 30, 40, 30,  0},
		{10, 25,  5, 45, 15},
		{10, 15,  5, 40, 30}
	}};

	const UINT wRand = RAND(100);
	const UINT wType = this->bSlayer ? 1 : 0;
	UINT wProbSum=markovProbs[wType][v.acceleration][0], wProbIndex=0;
	while (wProbSum < wRand)
		wProbSum += markovProbs[wType][v.acceleration][++wProbIndex];
	ASSERT(wProbIndex < VERMIN::NUM_ACCELERATIONS);

	v.acceleration = (VERMIN::ACCELERATION)wProbIndex;
}

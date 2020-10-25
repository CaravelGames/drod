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
 *
 * ***** END LICENSE BLOCK ***** */

#include "StrikeOrbEffect.h"
#include "RoomWidget.h"
#include "TileImageConstants.h"
#include "DrodBitmapManager.h"
#include <FrontEndLib/Bolt.h>
#include "../DRODLib/GameConstants.h"
#include "../DRODLib/CurrentGame.h"
#include <BackEndLib/Assert.h>

const UINT StrikeOrbEffectDuration = 220;

//********************************************************************************
CStrikeOrbEffect::CStrikeOrbEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,       //(in)   Should be a room widget.
	const COrbData &SetOrbData,      //(in)   Orb being struck.
	SDL_Surface *pSetPartsSurface,   //(in)   Preloaded surface containing bolt parts.
	const bool bSetDrawOrb)          //(in)   True if the orb should be drawn
	: CEffect(pSetWidget, StrikeOrbEffectDuration, EORBHIT)
{
	ASSERT(pSetWidget->GetType() == WT_Room);
	ASSERT(pSetPartsSurface);

	this->pPartsSurface = pSetPartsSurface;
	this->bDrawOrb = bSetDrawOrb;

	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	this->wOrbX = OwnerRect.x + (SetOrbData.wX * CBitmapManager::CX_TILE);
	this->wOrbY = OwnerRect.y + (SetOrbData.wY * CBitmapManager::CY_TILE);
	this->eOrbType = SetOrbData.eType;

	//Create bolt array.
	CRoomWidget *pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);
	const CCurrentGame *pGame = pRoomWidget->GetCurrentGame();
	CDbRoom *pRoom = pGame ? pGame->pRoom : NULL;

	static const UINT CX_TILE_HALF = CBitmapManager::CX_TILE / 2;
	static const UINT CY_TILE_HALF = CBitmapManager::CY_TILE / 2;
	const int xOrbCenter = this->wOrbX + CX_TILE_HALF;
	const int yOrbCenter = this->wOrbY + CY_TILE_HALF;

	//Calc start and end coords for each bolt.
	for (UINT wBoltI = SetOrbData.agents.size(); wBoltI--; )
	{
		COrbAgentData *pAgent = SetOrbData.agents[wBoltI];
		if (pRoom && bIsLight(pRoom->GetTSquare(pAgent->wX,pAgent->wY)))
		{
			//don't show lightning bolts going to lights
			continue;
		}

		BOLT bolt;
		bolt.xBegin = xOrbCenter;
		bolt.yBegin = yOrbCenter;
		bolt.xEnd = OwnerRect.x + pAgent->wX * CBitmapManager::CX_TILE + CX_TILE_HALF;
		bolt.yEnd = OwnerRect.y + pAgent->wY * CBitmapManager::CY_TILE + CY_TILE_HALF;
		this->bolts.push_back(bolt);
		this->drawBolts.push_back(new BOLT_SEGMENTS());
	}
}

//********************************************************************************
CStrikeOrbEffect::~CStrikeOrbEffect()
{
	for (UINT wBoltI = this->bolts.size(); wBoltI--; )
	{
		delete this->drawBolts[wBoltI];
	}
	this->bolts.clear();
}

//********************************************************************************
bool CStrikeOrbEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	this->dirtyRects.clear();  //Reset dirty regions.

	if (this->eOrbType == OT_BROKEN && RAND(5) != 0)
		return true; //flicker when broken

	if (bDrawOrb)
		this->dirtyRects.push_back(MAKE_SDL_RECT(this->wOrbX, this->wOrbY, CBitmapManager::CX_TILE, CBitmapManager::CY_TILE));

	if (g_pTheBM->bAlpha)
		this->nOpacity = (Uint8)(GetRemainingFraction() * 255.0);


	//Draw energy bolts.
	for (UINT wBoltI = this->bolts.size(); wBoltI--; )
	{
		const BOLT& bolt = this->bolts[wBoltI];
		BOLT_SEGMENTS* segments = this->drawBolts[wBoltI];
		segments->clear();
		GenerateBolt(bolt.xBegin, bolt.yBegin, bolt.xEnd, bolt.yEnd,
			CDrodBitmapManager::DISPLAY_COLS, *segments, this->dirtyRects);
	}

	return true;
}

//********************************************************************************
void CStrikeOrbEffect::Draw(SDL_Surface& pDestSurface)
{
	//Draw activated orb tile.
	if (bDrawOrb)
	    g_pTheBM->BlitTileImage(TI_ORB_L, this->wOrbX, this->wOrbY, &pDestSurface, false);

	//Clip screen surface to widget because bolt segments will go all over the place.
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	SDL_SetClipRect(&pDestSurface, &OwnerRect);

	//Set transparency level: bolt fades out over life of effect.
	if (g_pTheBM->bAlpha)
		EnableSurfaceBlending(this->pPartsSurface, this->nOpacity);

	//Draw energy bolts.
	for (UINT wBoltI = this->drawBolts.size(); wBoltI--; )
	{
		const BOLT_SEGMENTS* boltSegments = this->drawBolts[wBoltI];
		DrawBolt(*(this->drawBolts.at(wBoltI)), *this->pPartsSurface, pDestSurface);
	}

	//Remove transparency level.
	DisableSurfaceBlending(this->pPartsSurface);

	//Unclip screen surface.
	SDL_SetClipRect(&pDestSurface, NULL);
}

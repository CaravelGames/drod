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

#include "BumpObstacleEffect.h"
#include "BitmapManager.h"
#include <BackEndLib/Assert.h>

#include <SDL.h>

//Get relative horizontal/vertical position from orientation#.
#define nGetOX(o)                                 ( ((o) % 3) - 1 )
#define nGetOY(o)                                 ( ((o) / 3) - 1 )

//********************************************************************************
CBumpObstacleEffect::CBumpObstacleEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	UINT wSetCol, UINT wSetRow, //(in)  Square containing obstacle.
	UINT wSetBumpO)            //(in)   Direction to bump square.
	: CEffect(pSetWidget, BUMP_OBSTACLE_EFFECT_DURATION, EFFECTLIB::EBUMPOBSTACLE)
	, pEraseSurface(NULL)
{
	ASSERT(pSetWidget->GetType() == WT_Room);

	//Get width and height of bump blit area.
	const int dx = nGetOX(wSetBumpO);
	const int dy = nGetOY(wSetBumpO);
	this->src.w = this->dest.w = CBitmapManager::CX_TILE;
	this->src.h = this->dest.h = CBitmapManager::CY_TILE;
	
	//Set source and dest rects for bump blit.
	SDL_Rect rectWidget;
	this->pOwnerWidget->GetRect(rectWidget);
	this->src.x = this->dest.x = rectWidget.x + (wSetCol * CBitmapManager::CX_TILE);
	this->src.y = this->dest.y = rectWidget.y + (wSetRow * CBitmapManager::CY_TILE);
	
	this->dest.x += dx;
	this->dest.y += dy;

	//Area covered by effect.
	this->dirtyRects.push_back(this->dest);
}

//********************************************************************************
CBumpObstacleEffect::~CBumpObstacleEffect()
{
	if (this->pEraseSurface)
		SDL_FreeSurface(this->pEraseSurface);
}

//********************************************************************************
bool CBumpObstacleEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	if (!this->pEraseSurface) {
		this->pEraseSurface = g_pTheBM->ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, this->src.w, this->src.h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	}
	return true;
}
//********************************************************************************
void CBumpObstacleEffect::Draw(SDL_Surface& destSurface)
{
	SDL_Rect EraseRect = MAKE_SDL_RECT(0, 0, this->src.w, this->src.h);
	SDL_Rect DestRect = this->dest;
	SDL_BlitSurface(&destSurface, &this->src, this->pEraseSurface, &EraseRect);

	//Blit the bumped area.
	SDL_Rect ClipRect;
	this->pOwnerWidget->GetRect(ClipRect);

	// Manually clip West and North edges of the room, because otherwise the effect will just not
	// draw if bumping against Row/Col 0 - don't know why but that's how it works
	if (DestRect.x < ClipRect.x) {
		const int delta = ClipRect.x - DestRect.x;
		if (delta >= DestRect.w || delta >= EraseRect.w)
			return; // Nothing would be drawn in this case
		
		DestRect.x += delta;
		DestRect.w -= delta;
		EraseRect.x += delta;
		EraseRect.w -= delta;
	}
	if (DestRect.y < ClipRect.y) {
		const int delta = ClipRect.y - DestRect.y;
		if (delta >= DestRect.h || delta >= EraseRect.h)
			return; // Nothing would be drawn in this case

		DestRect.y += delta;
		DestRect.h -= delta;
		EraseRect.y += delta;
		EraseRect.h -= delta;
	}

	SDL_SetClipRect(&destSurface, &ClipRect);
	SDL_BlitSurface(this->pEraseSurface, &EraseRect, &destSurface, &DestRect);
	SDL_SetClipRect(&destSurface, NULL);
}

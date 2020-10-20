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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2003, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "SnowflakeEffect.h"
#include "TileImageConstants.h"
#include <FrontEndLib/BitmapManager.h>
#include <FrontEndLib/Screen.h>

//All instanced snowflakes have a similar horizontal drift
float CSnowflakeEffect::fXDrift = 0.0;

const UINT SNOW_TYPES = 2;

const UINT EffectDuration = 5000;

//*****************************************************************************
CSnowflakeEffect::CSnowflakeEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget)       //(in) parent widget
		: CEffect(pSetWidget, EffectDuration, ESNOWFLAKE),
		nOpacity(255), wTileNo(TI_TEMPTY), wDrawSize(0)
{
	ASSERT(pSetWidget);
	pSetWidget->GetRect(this->screenRect);

	static const UINT wVerticalFill = 50; //to better fill top
	const int nX = this->screenRect.x + RAND(this->screenRect.w);
	const int nY = this->screenRect.y + RAND(this->screenRect.h + wVerticalFill) - wVerticalFill;
	this->fX = static_cast<float>(nX);
	this->fY = static_cast<float>(nY);

	this->wType = RAND(SNOW_TYPES);

	//There is always one dirty rect.
	SDL_Rect rect = {static_cast<Sint16>(this->fX), static_cast<Sint16>(this->fY), 0, 0};
	this->dirtyRects.push_back(rect);

	RequestRetainOnClear(); //this effect doesn't depend on room state
}

//*****************************************************************************
bool CSnowflakeEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	UpdateWind();

	//Downward drift movement pattern.
	//Snowflake appears to move slower as it falls down.
	static const Uint32 dwBuffer = dwDuration / 4;
	const float fMultiplier = (50 * wDeltaTime) / (float)(dwBuffer + wDeltaTime);
	this->fY += fMultiplier;   //float downward
	this->fX += fMultiplier * CSnowflakeEffect::fXDrift;	//wind blows sideways

	if (OutOfBounds()) 
		return false;

	this->nOpacity = g_pTheBM->bAlpha
		? (BYTE)(GetRemainingFraction() * this->fOpacity * 255.0)
		: 255;
}

//*****************************************************************************
void CSnowflakeEffect::UpdateWind()
{
	static Uint32 lastUpdatePresentCount = 0;

	// Already updated this frame
	if (CScreen::dwPresentsCount == lastUpdatePresentCount)
		return;

	//Sideways wind movement.
	//Wind changes gradually.  Occasionally, velocity changes sharply.
	static const float fMaxDrift = 2.0;
	if (RAND(20000) == 0)
		CSnowflakeEffect::fXDrift += fRAND_MID(0.5f);
	else
		CSnowflakeEffect::fXDrift += fRAND_MID(0.005f);

	if (CSnowflakeEffect::fXDrift < -fMaxDrift)
		CSnowflakeEffect::fXDrift = -fMaxDrift;
	else if (CSnowflakeEffect::fXDrift > fMaxDrift)
		CSnowflakeEffect::fXDrift = fMaxDrift;
}

//*****************************************************************************
void CSnowflakeEffect::UpdateFrame(float elapsedFraction)
{
	static const UINT NUM_SPRITES = 4;        //Sprites in animation
	static const UINT SpriteNum[SNOW_TYPES][NUM_SPRITES] = {	//two types
		{TI_SNOWFLAKE_a1, TI_SNOWFLAKE_a2, TI_SNOWFLAKE_a3, TI_SNOWFLAKE_a4},
		{TI_SNOWFLAKE_b1, TI_SNOWFLAKE_b2, TI_SNOWFLAKE_b3, TI_SNOWFLAKE_b4}
	};
	static const UINT SpriteSize[SNOW_TYPES][NUM_SPRITES] = {
		{8, 6, 4, 3},
		{7, 5, 4, 3}
	};

	UINT wSpriteNo = elapsedFraction * NUM_SPRITES;
	ASSERT(wSpriteNo < NUM_SPRITES);

	this->wTileNo = SpriteNum[this->wType][wSpriteNo];
	this->wDrawSize = SpriteSize[this->wType][wSpriteNo];

	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].x = static_cast<Sint16>(this->fX);
	this->dirtyRects[0].y = static_cast<Sint16>(this->fY);
	this->dirtyRects[0].w = this->dirtyRects[0].h = this->wDrawSize;
}

//*****************************************************************************
void CSnowflakeEffect::Draw(SDL_Surface& pDestSurface)
{
	const UINT wX = static_cast<UINT>(this->fX), wY = static_cast<UINT>(this->fY);
	const UINT wSize = this->wDrawSize;
	if (wX >= (UINT)this->screenRect.x && 
			wY >= (UINT)this->screenRect.y &&
			wX < this->screenRect.x + this->screenRect.w - wSize &&
			wY < this->screenRect.y + this->screenRect.h - wSize)
		g_pTheBM->BlitTileImagePart(
			this->wTileNo, 
			wX, wY,
			0, 0, wSize, wSize, 
			&pDestSurface, true, nOpacity);
}

//*****************************************************************************
inline bool CSnowflakeEffect::OutOfBounds() const
{
	return this->fY >= this->screenRect.y + this->screenRect.h;
}

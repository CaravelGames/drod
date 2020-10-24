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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2003, 2004, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "SwordSwingEffect.h"
#include "DrodEffect.h"
#include "TileImageConstants.h"
#include "../DRODLib/Weapons.h"
#include <FrontEndLib/BitmapManager.h>

const UINT EffectDuration = 250;

//********************************************************************************
CSwordSwingEffect::CSwordSwingEffect(
//Constructor.
//
//Params:
	CWidget *pSetWidget,    //(in)   Should be a room widget.
	const CCoord &SetCoord,    //(in)   Location of axis of rotation.
	const UINT wO,
	const UINT weaponType)
	: CEffect(pSetWidget, EffectDuration, ESWORDSWING)
	, wO(wO)
	, weaponType(weaponType)
	, nOpacity(255), wTileNo(TI_TEMPTY)
{
	ASSERT(pSetWidget->GetType() == WT_Room);

	//Determine where to place graphic.
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	this->wX = OwnerRect.x + (SetCoord.wX * CBitmapManager::CX_TILE);
	this->wY = OwnerRect.y + (SetCoord.wY * CBitmapManager::CY_TILE);

	//Calculate offset.
	switch (wO)
	{
		case NW: this->wX -= CBitmapManager::CX_TILE/2; this->wY -= CBitmapManager::CY_TILE; break;
		case N: this->wX += CBitmapManager::CX_TILE/2; this->wY -= CBitmapManager::CY_TILE; break;
		case NE: this->wX += CBitmapManager::CX_TILE; this->wY -= CBitmapManager::CY_TILE/2; break;
		case E: this->wX += CBitmapManager::CX_TILE; this->wY += CBitmapManager::CY_TILE/2; break;
		case SE: this->wX += CBitmapManager::CX_TILE/2; this->wY += CBitmapManager::CY_TILE; break;
		case S: this->wX -= CBitmapManager::CX_TILE/2; this->wY += CBitmapManager::CY_TILE; break;
		case SW: this->wX -= CBitmapManager::CX_TILE; this->wY += CBitmapManager::CY_TILE/2; break;
		case W: this->wX -= CBitmapManager::CX_TILE; this->wY -= CBitmapManager::CY_TILE/2; break;
	}

	//Area of effect.
	SDL_Rect rect = MAKE_SDL_RECT(this->wX, this->wY,
			CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);
	this->dirtyRects.push_back(rect);

	CalculateFrame();
}

//********************************************************************************
bool CSwordSwingEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	this->nOpacity = static_cast<Uint8>(GetRemainingFraction() * 255);

	return true;
}
//********************************************************************************
void CSwordSwingEffect::Draw(SDL_Surface& pDestSurface)
{
	//Clip screen surface to owner widget in case effect goes outside.
	SDL_Rect OwnerRect;
	this->pOwnerWidget->GetRect(OwnerRect);
	SDL_SetClipRect(&pDestSurface, &OwnerRect);

	g_pTheBM->BlitTileImage(this->wTileNo, this->wX, this->wY, &pDestSurface, true, this->nOpacity);

	//Unclip screen surface.
	SDL_SetClipRect(&pDestSurface, NULL);
}

void CSwordSwingEffect::CalculateFrame()
{
	ASSERT(IsValidOrientation(this->wO));
	ASSERT(this->wO != NO_ORIENTATION);
	static const UINT sword_tiles[ORIENTATION_COUNT] = {
		TI_ROTATE_NW_N, TI_ROTATE_N_NE, TI_ROTATE_NE_E,
		TI_ROTATE_W_NW, TI_TEMPTY,	    TI_ROTATE_E_SE,
		TI_ROTATE_SW_W, TI_ROTATE_S_SW, TI_ROTATE_SE_S
	};
	static const UINT pickaxe_tiles[ORIENTATION_COUNT] = {
		TI_ROTATE_PICKAXE_NW_N, TI_ROTATE_PICKAXE_N_NE, TI_ROTATE_PICKAXE_NE_E,
		TI_ROTATE_PICKAXE_W_NW, TI_TEMPTY,	            TI_ROTATE_PICKAXE_E_SE,
		TI_ROTATE_PICKAXE_SW_W, TI_ROTATE_PICKAXE_S_SW, TI_ROTATE_PICKAXE_SE_S
	};
	static const UINT spear_tiles[ORIENTATION_COUNT] = {
		TI_ROTATE_SPEAR_NW_N, TI_ROTATE_SPEAR_N_NE, TI_ROTATE_SPEAR_NE_E,
		TI_ROTATE_SPEAR_W_NW, TI_TEMPTY,	        TI_ROTATE_SPEAR_E_SE,
		TI_ROTATE_SPEAR_SW_W, TI_ROTATE_SPEAR_S_SW, TI_ROTATE_SPEAR_SE_S
	};
	static const UINT staff_tiles[ORIENTATION_COUNT] = {
		TI_ROTATE_STAFF_NW_N, TI_ROTATE_STAFF_N_NE, TI_ROTATE_STAFF_NE_E,
		TI_ROTATE_STAFF_W_NW, TI_TEMPTY,	        TI_ROTATE_STAFF_E_SE,
		TI_ROTATE_STAFF_SW_W, TI_ROTATE_STAFF_S_SW, TI_ROTATE_STAFF_SE_S
	};
	static const UINT dagger_tiles[ORIENTATION_COUNT] = {
		TI_ROTATE_DAGGER_NW_N, TI_ROTATE_DAGGER_N_NE, TI_ROTATE_DAGGER_NE_E,
		TI_ROTATE_DAGGER_W_NW, TI_TEMPTY,	          TI_ROTATE_DAGGER_E_SE,
		TI_ROTATE_DAGGER_SW_W, TI_ROTATE_DAGGER_S_SW, TI_ROTATE_DAGGER_SE_S
	};

	switch (this->weaponType) {
		default:
		case WT_Sword: this->wTileNo = sword_tiles[this->wO]; break;
		case WT_Pickaxe: this->wTileNo = pickaxe_tiles[this->wO]; break;
		case WT_Spear: this->wTileNo = spear_tiles[this->wO]; break;
		case WT_Staff: this->wTileNo = staff_tiles[this->wO]; break;
		case WT_Dagger: this->wTileNo = dagger_tiles[this->wO]; break;
		case WT_Caber: this->wTileNo = staff_tiles[this->wO]; break;
	}
}

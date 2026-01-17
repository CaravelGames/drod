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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "ImageOverlayEffect.h"
#include "RoomEffectList.h"
#include "RoomWidget.h"
#include <BackEndLib/Assert.h>

//*****************************************************************************
void CRoomEffectList::Clear(
//Clears all effects from the effect list.
//
//Params:
	const bool bRepaint, //(in)   Touch up affected room areas before deleting
								//(default = false)
	const bool bForceClearAll) //if set [default=true], delete all effects,
	                     //including those that request to be retained
{
	list<CEffect*> retained;
	for (list<CEffect *>::const_iterator iSeek = this->Effects.begin();
		iSeek != this->Effects.end(); ++iSeek)
	{
		CEffect *pEffect = *iSeek;
		ASSERT(pEffect);
		if (pEffect->RequestsRetainOnClear() && !bForceClearAll)
		{
			retained.push_back(pEffect);
			continue;
		}
		if (bRepaint)
			DirtyTilesForRects(pEffect->dirtyRects);
		delete pEffect;
	}
	this->Effects = retained;
}

//*****************************************************************************
void CRoomEffectList::DirtyTiles() const
//Dirties room tiles within the effects' area of effect.
{
	for (list<CEffect *>::const_iterator iSeek = this->Effects.begin();
		iSeek != this->Effects.end(); ++iSeek)
			DirtyTilesForRects((*iSeek)->dirtyRects);
}

//*****************************************************************************
void CRoomEffectList::DirtyTilesForRects(
//Dirties room tiles within the effects' area of effect.
//
//Params:
	vector<SDL_Rect>& dirtyRects)
const
{
	UINT wIndex, xStart, yStart, xEnd, yEnd;
	for (wIndex=dirtyRects.size(); wIndex--; )
	{
		SDL_Rect rect = dirtyRects[wIndex];
		if (!rect.w || !rect.h) continue;

		//Non-zero effect area -- dirty the tiles this effect covers.

		//Remove offset to calculate tiles.
		rect.x -= this->pOwnerWidget->GetX();
		rect.y -= this->pOwnerWidget->GetY();
		//Ignore parts out of bounds.
		if (rect.x < 0) {rect.w += rect.x; rect.x = 0;}
		if (rect.y < 0) {rect.h += rect.y; rect.y = 0;}
		if ((rect.x + (Sint16)rect.w <= 0) || (rect.y + (Sint16)rect.h <= 0) ||
				(rect.x >= (int)this->pOwnerWidget->GetW()) ||
				(rect.y >= (int)this->pOwnerWidget->GetH())) continue;
		if (rect.x + rect.w > static_cast<int>(this->pOwnerWidget->GetW()))
			{rect.w = this->pOwnerWidget->GetW() - rect.x;}
		if (rect.y + rect.h > static_cast<int>(this->pOwnerWidget->GetH()))
			{rect.h = this->pOwnerWidget->GetH() - rect.y;}

		//Calculate tiles covered.
		xStart = rect.x / CBitmapManager::CX_TILE;
		yStart = rect.y / CBitmapManager::CY_TILE;
		xEnd = (rect.x+rect.w-1) / CBitmapManager::CX_TILE;
		yEnd = (rect.y+rect.h-1) / CBitmapManager::CY_TILE;
		ASSERT(xEnd>=xStart);
		ASSERT(yEnd>=yStart);
		//Dirty these tiles.  Overlapping regions are fine.
		DirtyTilesInRect(xStart,yStart,xEnd,yEnd);
	}
}

//*****************************************************************************
void CRoomEffectList::DirtyTilesInRect(
//Mark all tiles in bounding box as dirty.
//
//Params:
	const UINT xStart, const UINT yStart, const UINT xEnd, const UINT yEnd) //(in)
const
{
	if (!this->pOwnerWidget->pRoom)
		return;
	if (!this->pOwnerWidget->pTileImages)
		return;

	const UINT wCols = this->pOwnerWidget->pRoom->wRoomCols;
	UINT xIndex, yIndex;
	TileImages *pTInfo;
	for (yIndex=yStart; yIndex<=yEnd; ++yIndex)
	{
		pTInfo = this->pOwnerWidget->pTileImages + (yIndex * wCols + xStart);
		for (xIndex=xStart; xIndex<=xEnd; ++xIndex)
			(pTInfo++)->dirty = 1;
	}
}

//*****************************************************************************
void CRoomEffectList::RemoveEffectsOfType(
//Removes all effects of given type from the list.
//
//Params:
	const UINT eEffectType, //(in)   Type of effect to remove.
	const bool bForceClearAll) //if set [default=true], delete all effects,
	                     //including those that request to be retained
{
	//Clear list of given effect type.
	list<CEffect *>::const_iterator iSeek = this->Effects.begin();
	while (iSeek != this->Effects.end())
	{
		if (eEffectType == (*iSeek)->GetEffectType())
		{
			//Remove from list.
			CEffect *pDelete = *iSeek;
			ASSERT(pDelete);
			if (pDelete->RequestsRetainOnClear() && !bForceClearAll)
			{
				++iSeek;
				continue;
			}

			DirtyTilesForRects(pDelete->dirtyRects);  //touch up area before deleting
			++iSeek;
			this->Effects.remove(pDelete);
			delete pDelete;
		}
		else
			++iSeek;
	}
}

void CRoomEffectList::RemoveOverlayEffectsInGroup(
//Removes all image overlay effects in the given group from the list.
//
//Params:
	const int clearGroup,//(in) Overlay group to remove
	const bool bForceClearAll) //if set [default=true], delete all effects,
	                     //including those that request to be retained
{
	list<CEffect*>::const_iterator iSeek = this->Effects.begin();
	while (iSeek != this->Effects.end())
	{
		if ((*iSeek)->GetEffectType() == EIMAGEOVERLAY)
		{
			//Remove from list.
			CEffect* pDelete = *iSeek;
			ASSERT(pDelete);
			if (pDelete->RequestsRetainOnClear() && !bForceClearAll)
			{
				++iSeek;
				continue;
			}

			CImageOverlayEffect* pDeleteOverlay = DYN_CAST(CImageOverlayEffect*, CEffect*, pDelete);

			if (pDeleteOverlay->getGroup() == clearGroup) {
				DirtyTilesForRects(pDelete->dirtyRects);  //touch up area before deleting
				++iSeek;
				this->Effects.remove(pDelete);
				delete pDelete;
			}
		}
		else
			++iSeek;
	}
}

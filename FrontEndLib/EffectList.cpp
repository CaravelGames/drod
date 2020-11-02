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

#include "EffectList.h"
#include <BackEndLib/Assert.h>
#include "Screen.h"
#include "AnimatedTileEffect.h"

#include <limits.h>

using namespace std;

//*****************************************************************************
CEffectList::CEffectList(CWidget *pOwnerWidget)
	: pOwnerWidget(pOwnerWidget)
	, pOwnerScreen(NULL)
	, bIsFrozen(false)
{
	if (pOwnerWidget && pOwnerWidget->eType == WT_Screen)
		this->pOwnerScreen = DYN_CAST(CScreen*, CWidget*, pOwnerWidget);
}

CEffectList::~CEffectList()
{
	Clear(false, true);
}

//*****************************************************************************
void CEffectList::AddEffect(
//Adds an effect to the effect list.
//
//Params:
	CEffect *pEffect)          //(in)   Effect to add.
{
	ASSERT(pEffect);
	if (!pEffect) return;   //just-in-case handling

	//List is sorted by draw sequence--smaller values are at beginning.
	//Add effect in front of the first element with a larger or equal draw
	//sequence.
	for (list<CEffect *>::iterator iSeek = this->Effects.begin();
		iSeek != this->Effects.end(); ++iSeek)
	{
		CEffect *pSeekEffect = *iSeek;
		if (pSeekEffect->GetDrawSequence() >= pEffect->GetDrawSequence())
		{
			this->Effects.insert(iSeek, pEffect);
			return;
		}
	}

	//Effect has the largest draw sequence value so it goes at end of list.
	this->Effects.push_back(pEffect);
}

//*****************************************************************************
void CEffectList::Clear(
//Clears all effects from the effect list.
//
//Params:
	const bool bRepaint, //(in)   Touch up affected screen areas before deleting
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
		if (pEffect->bRequestRetainOnClear && !bForceClearAll)
		{
			retained.push_back(pEffect);
			continue;
		}
		if (bRepaint)
		{
			this->pOwnerWidget->RequestPaint();
			if (this->pOwnerScreen)
			{
				for (UINT wIndex=pEffect->dirtyRects.size(); wIndex--; )
					this->pOwnerScreen->UpdateRect(pEffect->dirtyRects[wIndex]);
			}
		}
		delete pEffect;
	}
	this->Effects = retained;
	this->bIsFrozen = false;
}

//*****************************************************************************
bool CEffectList::ContainsEffect(CEffect *pEffect) const
//Returns whether this effect is in the list.
{
	for (list<CEffect *>::const_iterator iSeek = this->Effects.begin();
		iSeek != this->Effects.end(); ++iSeek)
	{
		if (pEffect == (*iSeek))
			return true;
	}

	return false;
}

//*****************************************************************************
bool CEffectList::ContainsEffectOfType(
//Returns true if effect list contains an effect of stated type, else false.
//
//Params:
	const UINT eEffectType) //(in)
const
{
	return GetEffectOfType(eEffectType) != NULL;
}

//*****************************************************************************
bool CEffectList::ContainsEffectOfTypeAt(
//Returns true if effect list contains an effect of stated type at the
//stated position, else false.
//
//Pre-Cond: effect searched for must be derived from an CAnimatedTileEffect
//
//Params:
	const UINT eEffectType, //(in)
	const UINT wX, const UINT wY) //(in) position
const
{
	CAnimatedTileEffect *pTileEffect;
	for (list<CEffect *>::const_iterator iSeek = this->Effects.begin();
		iSeek != this->Effects.end(); ++iSeek)
	{
		if (eEffectType == (*iSeek)->GetEffectType())
		{
			pTileEffect = DYN_CAST(CAnimatedTileEffect*, CEffect*, *iSeek);
			if (wX == pTileEffect->GetX() && wY == pTileEffect->GetY())
				return true;
		}
	}

	return false;
}

//*****************************************************************************
void CEffectList::UpdateEffects(
//Draws list of effects.
//
//Params:
	const bool bUpdateScreen,  //(in) Whether screen is updated where effects are drawn
                               //[default = false]
	const UINT eDrawnType)     //(in) When given effect type will only draw effects of that type
	                           //[default = -1] - Draw all effect types
{
	list<CEffect *>::const_iterator iSeek = this->Effects.begin();

	bool bRepaintScreen = false;
	while (iSeek != this->Effects.end())
	{
		CEffect *pEffect = *iSeek;
		++iSeek;

		if (eDrawnType != DRAW_ALL_EFFECTS_TYPE && pEffect->GetEffectType() != eDrawnType)
			continue;

		if (!UpdateEffect(pEffect, bUpdateScreen))
			bRepaintScreen = true;
	}

	if (bRepaintScreen && this->pOwnerScreen)
		this->pOwnerScreen->RequestPaint();  //refresh area of removed effects
}

//*****************************************************************************
void CEffectList::DrawEffects(
//Draws list of effects.
//
//Params:
	SDL_Surface* pDestSurface, //(in) where to draw effects (default = NULL)
	const UINT eDrawnType)     //(in) When given effect type will only draw effects of that type
							   //[default = -1] - Draw all effect types
{
	list<CEffect*>::const_iterator iSeek = this->Effects.begin();

	bool bRepaintScreen = false;
	while (iSeek != this->Effects.end())
	{
		CEffect* pEffect = *iSeek;
		++iSeek;

		if (eDrawnType != DRAW_ALL_EFFECTS_TYPE && pEffect->GetEffectType() != eDrawnType)
			continue;

		DrawEffect(pEffect, pDestSurface);
	}
}

//*****************************************************************************
void CEffectList::UpdateAndDrawEffects(
//Draws list of effects.
//
//Params:
	const bool bUpdateScreen,  //(in) Whether screen is updated where effects are drawn
							   //[default = false]
	SDL_Surface* pDestSurface, //(in) where to draw effects (default = NULL)
	const UINT eDrawnType)     //(in) When given effect type will only draw effects of that type
							   //[default = -1] - Draw all effect types
{
	UpdateEffects(bUpdateScreen, eDrawnType);
	DrawEffects(pDestSurface, eDrawnType);
}

//*****************************************************************************
//Updates a single effect
bool CEffectList::UpdateEffect(
	CEffect* pEffect,          //(in) Effect to draw
	const bool bUpdateScreen)  //(in) Whether screen is updated where effects are drawn
							   //[default = false]
// Returns: true if the effect is still running, false if it's finished
{

	ASSERT(pEffect);

	UINT wDeltaTime = this->bIsFrozen
		? 0
		: CScreen::dwLastRenderTicks - pEffect->dwTimeOfLastMove;

	const bool bDoesEffectRemain = pEffect->Update(wDeltaTime);
	if (!this->bIsFrozen)
		pEffect->dwTimeOfLastMove = CScreen::dwLastRenderTicks;

	if (bDoesEffectRemain)
	{
		if (bUpdateScreen && this->pOwnerScreen)
			for (UINT wIndex = pEffect->dirtyRects.size(); wIndex--; )
				this->pOwnerScreen->UpdateRect(pEffect->dirtyRects[wIndex]);
	}
	else {
		//Effect is finished--remove from list.
		this->Effects.remove(pEffect);
		delete pEffect;
	}

	return bDoesEffectRemain;
}

//*****************************************************************************
//Draws a single effect
void CEffectList::DrawEffect(
	CEffect* pEffect,          //(in) Effect to draw
	SDL_Surface* pDestSurface) //(in) where to draw effects (default = NULL)
// Returns: true if the effect is still running, false if it's finished
{
	
	ASSERT(pEffect);
	pEffect->Draw(pDestSurface);
}

//*****************************************************************************
void CEffectList::EraseEffects(
//Erases the area drawn over by the effect list by repainting a given
//background in its place.
//Call when effects are painted over a background that is otherwise never repainted,
//like over a CScreen that is normally painted only once.
//
//Params:
	SDL_Surface* pBackground, const SDL_Rect& rect,	//(in) background under effects
	const bool bUpdate)	//(in) [default=false]
{
	ASSERT(pBackground);

	for (list<CEffect *>::const_iterator iSeek = this->Effects.begin();
		iSeek != this->Effects.end(); ++iSeek)
	{
		CEffect *pEffect = *iSeek;
		ASSERT(pEffect);
		ASSERT(pEffect->pOwnerWidget);
		SDL_Surface *pDestSurface = pEffect->pOwnerWidget->GetDestSurface();
		for (UINT wIndex=pEffect->dirtyRects.size(); wIndex--; )
		{
			//Repaint background over this dirty region.
			SDL_Rect& dirty = pEffect->dirtyRects[wIndex];
			SDL_Rect srcRect = MAKE_SDL_RECT(rect.x + dirty.x, rect.y + dirty.y,
					dirty.w, dirty.h);
			SDL_BlitSurface(pBackground, &srcRect, pDestSurface, &dirty);
			if (bUpdate && this->pOwnerScreen)
				this->pOwnerScreen->UpdateRect(dirty);
		}
	}
}

//*****************************************************************************
CEffect* CEffectList::GetEffectOfType(const UINT eEffectType) const
//Returns: pointer to first effect of specified type if exists, else NULL
{
	for (list<CEffect *>::const_iterator iSeek = this->Effects.begin();
		iSeek != this->Effects.end(); ++iSeek)
	{
		if (eEffectType == (*iSeek)->GetEffectType())
			return *iSeek;
	}

	return NULL;
}

//*****************************************************************************
//Returns: a rectangle of semantic format (x1,y1,x2,y2)
//         containing the total area of the indicated effect type
SDL_Rect CEffectList::GetBoundingBoxForEffectsOfType(const UINT eEffectType) const
{
	SDL_Rect bbox = {SHRT_MAX, SHRT_MAX, 0, 0};

	bool bNone = true;
	for (list<CEffect *>::const_iterator iSeek = this->Effects.begin();
		iSeek != this->Effects.end(); ++iSeek)
	{
		const CEffect *pEffect = *iSeek;
		if (eEffectType == pEffect->GetEffectType()) {
			bNone = false;
			for (UINT i=0; i<pEffect->dirtyRects.size(); ++i) {
				const SDL_Rect& rect = pEffect->dirtyRects[i];
				if (rect.x < bbox.x)
					bbox.x = rect.x;
				if (rect.y < bbox.y)
					bbox.y = rect.y;
				const int x2 = rect.x + rect.w;
				if (x2 > bbox.w)
					bbox.w = x2;
				const int y2 = rect.y + rect.h;
				if (y2 > bbox.h)
					bbox.h = y2;
			}
		}
	}

	if (bNone)
		bbox.x = bbox.y = 0;
	return bbox;
}

//*****************************************************************************
void CEffectList::SetOpacityForEffectsOfType(const UINT eEffectType, float fOpacity) const
{
	for (list<CEffect *>::const_iterator iSeek = this->Effects.begin();
		iSeek != this->Effects.end(); ++iSeek)
	{
		if (eEffectType == (*iSeek)->GetEffectType())
			(*iSeek)->SetOpacity(fOpacity);
	}
}

//*****************************************************************************
void CEffectList::RemoveEffectsOfType(
//Removes all effects of given type from the list.
//
//Params:
	const UINT eEffectType) //(in)   Type of effect to remove.
{
	bool bRepaint = false;

	//Clear list of given effect type.
	list<CEffect *>::const_iterator iSeek = this->Effects.begin();
	while (iSeek != this->Effects.end())
	{
		CEffect *pEffect = *iSeek;
		ASSERT(pEffect);
		++iSeek;
		if (eEffectType == pEffect->GetEffectType())
		{
			//Remove from list.

			//Damage screen area.
			bRepaint = true;
			if (this->pOwnerScreen)
				for (UINT wIndex=pEffect->dirtyRects.size(); wIndex--; )
					this->pOwnerScreen->UpdateRect(pEffect->dirtyRects[wIndex]);

			this->Effects.remove(pEffect);
			delete pEffect;
		}
	}

	if (bRepaint)
		this->pOwnerWidget->RequestPaint();
}

//*****************************************************************************
void CEffectList::SetEffectsFrozen(const bool bIsFrozen)
	//Sets whether the effects should animate or just render statically
	//When set to on, all effects will run with 0 delta time
	//When set to off, all effects will have their internal state changed so that
	// the next Draw() call will run with 0 delta, but will continue to execute
	// as normal afterwards
{
	if (bIsFrozen == this->bIsFrozen)
		return;

	this->bIsFrozen = bIsFrozen;

	if (!this->bIsFrozen) {
		// Unfreeze the effects, resetting their last render time to current time
		// so that next draw call assumes only 0 or 1 frames has passed since they were last drawn
		for (list<CEffect*>::const_iterator iSeek = this->Effects.begin(); iSeek != this->Effects.end(); ++iSeek)
			(*iSeek)->dwTimeOfLastMove = CScreen::dwLastRenderTicks;
	}
}

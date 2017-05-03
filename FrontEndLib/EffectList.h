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

#ifndef EFFECTLIST_H
#define EFFECTLIST_H

#include "Effect.h"

//****************************************************************************************
class CScreen;
class CEffectList
{
public:
	CEffectList(CWidget *pOwnerWidget);
	virtual ~CEffectList();

	void           AddEffect(CEffect *pEffect);
	virtual void   Clear(const bool bRepaint=false, const bool bForceClearAll=true);
	bool           ContainsEffect(CEffect *pEffect) const;
	bool           ContainsEffectOfType(const UINT eEffectType) const;
	bool           ContainsEffectOfTypeAt(const UINT eEffectType, const UINT wX, const UINT wY) const;
	void           DrawEffects(const bool bUpdateScreen=false,
			const bool bFreezeEffects=false, SDL_Surface *pDestSurface=NULL);
	void           DrawEffectsOfType(const UINT eEffectType, const bool bUpdateScreen=false,
			const bool bFreezeEffects=false, SDL_Surface *pDestSurface=NULL);
	void           EraseEffects(SDL_Surface* pBackground, const SDL_Rect& rect, const bool bUpdate=false);
	CEffect*       GetEffectOfType(const UINT eEffectType) const;
	SDL_Rect       GetBoundingBoxForEffectsOfType(const UINT eEffectType) const;
	virtual void   RemoveEffectsOfType(const UINT eEffectType);
	void           SetOpacityForEffectsOfType(const UINT eEffectType, float fOpacity) const;

	list<CEffect *> Effects;

protected:
	CWidget *   pOwnerWidget;
	CScreen *   pOwnerScreen;

	//Save time when effects are temporarily stopped
	Uint32         dwTimeEffectsWereFrozen;
};

#endif //...#ifndef EFFECTLIST_H

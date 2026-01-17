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

#ifndef ROOMEFFECTLIST_H
#define ROOMEFFECTLIST_H

#include <FrontEndLib/EffectList.h>
#include "RoomWidget.h"

//****************************************************************************************
class CRoomEffectList : public CEffectList
{
public:
	CRoomEffectList(CRoomWidget* pRoom)
		: CEffectList(pRoom), pOwnerWidget(pRoom) {}

	virtual void   Clear(const bool bRepaint=false, const bool bForceClearAll=true);
	void           DirtyTiles() const;
	void           DirtyTilesForRects(vector<SDL_Rect>& dirtyRects) const;
	void           DirtyTilesInRect(const UINT xStart, const UINT yStart,
			const UINT xEnd, const UINT yEnd) const;
	virtual void   RemoveEffectsOfType(const UINT eEffectType, const bool bForceClearAll=true);
	virtual void   RemoveOverlayEffectsInGroup(const int clearGroup, const bool bForceClearAll=true);

protected:
	CRoomWidget *pOwnerWidget;
};

#endif //...#ifndef ROOMEFFECTLIST_H

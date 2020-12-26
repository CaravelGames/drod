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
 * Portions created by the Initial Developer are Copyright (C) 2006
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef VARMONITOREFFECT_H
#define VARMONITOREFFECT_H

#include "DrodEffect.h"
#include "../DRODLib/CurrentGame.h"

//*****************************************************************************
class CRoomWidget;
class CVarMonitorEffect : public CEffect
{
public:
	CVarMonitorEffect(CWidget *pSetWidget);
	~CVarMonitorEffect();

	void SetText(const WCHAR* pText);

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);
	
private:
	void UpdateOpacity(const Uint32 dwTimeElapsed);
	void SetTextForNewTurn();

	int x, y;          //pixel position
	UINT lastTurn;      //last turn var query was made

	VARMAP lastVars;   //latest displayed var state

	CRoomWidget *pRoomWidget;

	SDL_Surface *pTextSurface;  //text to display

	Uint8 nOpacity;
};

#endif //...#ifndef VARMONITOREFFECT_H

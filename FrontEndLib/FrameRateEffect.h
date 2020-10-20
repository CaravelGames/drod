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

#ifndef FRAMERATEEFFECT_H
#define FRAMERATEEFFECT_H

#include "Effect.h"

//****************************************************************************************
class CFrameRateEffect : public CEffect
{
public:
	CFrameRateEffect(CWidget *pSetWidget, const UINT eType=EFFECTLIB::EFRAMERATE);
	virtual ~CFrameRateEffect();

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& pDestSurface);

	virtual void PrepDisplay();
	void         SetText(const WCHAR* pText);

	int      x, y;
	UINT dwLastDrawTime;
	UINT  wFrameCount;

private:
	SDL_Surface *pTextSurface;  //text to display
};

#endif //...#ifndef FRAMERATEEFFECT_H

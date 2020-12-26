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

#ifndef TOOLTIPEFFECT_H
#define TOOLTIPEFFECT_H

#include "FontManager.h"
#include "Effect.h"
#include <BackEndLib/Coord.h>

#include <string>
#include <SDL.h>

//******************************************************************************
class CToolTipEffect : public CEffect
{
public:
	CToolTipEffect(CWidget *pSetWidget, const CCoord &SetCoord,
			const WCHAR *pwczSetText, const Uint32 dwDisplayDuration=5000,
			const UINT eSetFontType=FONTLIB::F_Small);
	virtual ~CToolTipEffect();

	virtual long   GetDrawSequence() const {return 1000L;}   //draw last

	UINT           GetFontType() const {return this->eFontType;}
	void           GetTextWidthHeight(UINT &wW, UINT &wH) const;
	void           SetDuration(const Uint32 dwDuration)
			{this->dwDuration = dwDuration;}
	void           SetText(const WCHAR *pwczSetText, bool bResizeToFit=false);
	

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);

private:
	UINT           x, y, w, h;
	WSTRING           wstrText;
	UINT           eFontType;

	SDL_Surface *  pToolTipSurface;
};

#endif //#ifndef TOOLTIPEFFECT_H

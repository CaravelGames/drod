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
 * Contributor(s): mrimer
 *
 * ***** END LICENSE BLOCK ***** */

//SUMMARY
//
//CTextEffect is a class for displaying text to be drawn on top of widgets.
//Effects are temporary animations drawn by the owner widget.  The screen surface 
//update is performed by the owner widget.
//

#ifndef CTEXTEFFECT_H
#define CTEXTEFFECT_H

#include "Effect.h"

#include <BackEndLib/Wchar.h>
#include <string>

//******************************************************************************
class CTextEffect : public CEffect
{
public:
	CTextEffect(CWidget *pSetWidget, const WCHAR* text, const UINT eFont,
			const Uint32 dwFadeIn = 0, const Uint32 dwDuration = 0,  //ms
			const bool bFadeOut = false, const bool bPerPixelTransparency=false);
	virtual ~CTextEffect();

	void     FadeOut(const bool bVal=true) {this->bFadeOut = bVal;}
	int		X() const {return this->nX;}
	int		Y() const {return this->nY;}
	void		Move(const int nX, const int nY);
	void     SetText(const WCHAR *text, const UINT eFont);

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);

private:
	SDL_Surface * pTextSurface;  //text to display
	SDL_Rect screenRect;
	WSTRING wstrText;
	Uint32 dwDuration, dwFadeIn;

	int nX, nY;
	bool bFadeOut;
	bool bPerPixelTransparency;

	Uint8 nOpacity;
	SDL_Rect drawRect;
};

#endif   //...#ifndef CTEXTEFFECT_H

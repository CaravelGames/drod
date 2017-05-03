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
//CFlashMessageEffect is a class for displaying text to be drawn on top of widgets.
//Effects are temporary animations drawn by the owner widget.  The screen surface 
//update is performed by the owner widget.
//

#ifndef CFLASHMESSAGEEFFECT_H
#define CFLASHMESSAGEEFFECT_H

#include "Effect.h"

#include <BackEndLib/Wchar.h>
#include <string>

//******************************************************************************
class CWidget;
class CFlashMessageEffect : public CEffect
{
public:
	CFlashMessageEffect(CWidget *pSetWidget, const WCHAR* text, 
			const int yOffset = 0, const Uint32 wDuration = 3000, const Uint32 fadeTime = 1000);
	virtual ~CFlashMessageEffect();

	virtual bool Draw(SDL_Surface* pDestSurface=NULL);

	void SetColor(int r, int g, int b);
	void SetMovement(bool val) { this->movement = val; }
	void SlowExpansion(bool val=true) { this->bSlowExpansion = val; }

private:
	void FreeTextSurface();
	void RenderText();

	SDL_Surface *pTextSurface;  //text to display
	SDL_Rect base_size;
	SDL_Rect screenRect;
	WSTRING wstrText;
	int yOffset;
	Uint32 wDuration, fadeTime;
	bool movement;
	bool bSlowExpansion;

	bool bBlendedFontRender;

	bool bCustomColor;
	SDL_Color customColor;
};

#endif   //...#ifndef CFLASHMESSAGEEFFECT_H

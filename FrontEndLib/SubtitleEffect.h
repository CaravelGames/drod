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

#ifndef SUBTITLEEFFECT_H
#define SUBTITLEEFFECT_H

#include "Effect.h"
#include "FontManager.h"
#include <BackEndLib/Coord.h>
#include <BackEndLib/Wchar.h>

#include <string>
#include <map>

class CSubtitleEffect;
typedef std::map<CMoveCoord*, CSubtitleEffect*> SUBTITLES;

struct TEXTLINE {
	TEXTLINE() : color({0, 0, 0, 0}) {} //black
	TEXTLINE(const WCHAR* pText) : text(pText), color({0, 0, 0, 0}) {}
	WSTRING text;
	SDL_Color color;
};

//******************************************************************************
class CSubtitleEffect : public CEffect
{
public:
	CSubtitleEffect(CWidget *pSetWidget, CMoveCoord *const pCoord,
			const WCHAR *pwczSetText, const SDL_Color& FGColor,
			const SURFACECOLOR& BGColor,
			const Uint32 dwDuration=2000,
			const UINT wDisplayLines=3,
			const UINT eSetFontType=FONTLIB::F_Small,
			const UINT maxWidth=0);
	virtual ~CSubtitleEffect();

	void           AddToSubtitles(SUBTITLES &subtitles);
	void           RemoveFromSubtitles();

	virtual bool   Draw(SDL_Surface* pDestSurface=NULL);
	void           FollowCoord(CMoveCoord *const pCoord, const bool bAttachedCoord=false);
	Uint32         GetDisplayTimeRemaining() const;
	virtual long   GetDrawSequence() const;

	UINT           GetFontType() const {return this->eFontType;}
	void           GetTextWidthHeight(UINT &wW, UINT &wH) const;
	void           AddTextLine(const WCHAR* pwczSetText, const Uint32 dwDuration, const SDL_Color& color=Black,
			const bool bSyncTextOffsetUpward=false);
	void           SetToText(const WCHAR* pwczSetText, const Uint32 dwDuration, const SDL_Color& color=Black);

	void           SetAlpha(const Uint8 opacity);
	void           SetEffectType(const UINT eType) {this->eEffectType = eType;}
	void           SetFadeDuration(const Uint32 duration) {this->dwFadeDuration = duration;}
	void           SetOffset(const int nX, const int nY);
	void           SetMaxWidth(const UINT width) {this->maxWidth = width;}

	bool           bAttachedCoord;   //whether pCoord should be deleted on destruction

private:
	void           PrepWidget();
	void           SetLocation();

	CMoveCoord      * pCoord;  //dynamic location to place effect near to
	SUBTITLES       * pSubtitles; //set of subtitle effects maintained by my parent

	int              xOffset, yOffset;
	UINT              x, y, w, h;
	vector<TEXTLINE>  texts;
	UINT              eFontType;
	UINT              wDisplayLines;
	UINT              maxWidth;
	Uint32            dwWhenEnabled, dwDuration, dwFadeDuration;

	SDL_Surface *  pTextSurface;
	SURFACECOLOR   BGColor;
	Uint8          opacity;
};

#endif //#ifndef SUBTITLEEFFECT_H

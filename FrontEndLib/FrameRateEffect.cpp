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

#include "FrameRateEffect.h"
#include "BitmapManager.h"
#include "FontManager.h"
#include "Sound.h"
#include <BackEndLib/Assert.h>

//
//Public methods.
//

//*****************************************************************************
CFrameRateEffect::CFrameRateEffect(CWidget *pSetWidget, const UINT eType)
	: CEffect(pSetWidget, (UINT)-1, eType)
	, x(pOwnerWidget->GetX())
	, y(pOwnerWidget->GetY())
	, wFrameCount(0)
	, pTextSurface(NULL)
//Constructor.
{
	//Even when other effects are cleared, it's generally okay to leave this one.
	RequestRetainOnClear();

	SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y, 0, 0);
	this->dirtyRects.push_back(rect);

	SetText(wszPeriod);
}

CFrameRateEffect::~CFrameRateEffect()
{
	if (this->pTextSurface)
		SDL_FreeSurface(this->pTextSurface);
}

//*****************************************************************************
bool CFrameRateEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	PrepDisplay();

	return true;
}
//*****************************************************************************
void CFrameRateEffect::Draw(SDL_Surface& destSurface)
{
	ASSERT(this->pTextSurface);
	SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y, this->pTextSurface->w, this->pTextSurface->h);
	SDL_BlitSurface(this->pTextSurface, NULL, &destSurface, &rect);
}

//*****************************************************************************
void CFrameRateEffect::PrepDisplay()
{
	//Calc the frame rate based on the amount of times this method is called
	//over the duration of one second.
	++this->wFrameCount;

	const UINT dwNow = SDL_GetTicks();
	static const Uint32 INTERVAL = 1000; //ms
	if (dwNow >= this->dwLastDrawTime + INTERVAL)
	{
		float fOneSecondFrameCount = this->wFrameCount * (INTERVAL / float(dwNow-this->dwLastDrawTime));
		fOneSecondFrameCount *= 10.f;
		this->dwLastDrawTime = dwNow;
		this->wFrameCount = 0;

		WSTRING wStr;
		WCHAR wczNum[10];
		wStr += _itoW(int(fOneSecondFrameCount) / 10, wczNum, 10);
		wStr += wszPeriod;
		wStr += _itoW(int(fOneSecondFrameCount) % 10, wczNum, 10);

		//Show some other stats too.
		if (g_pTheSound)
		{
			wStr += wszSpace;
			wStr += _itoW(g_pTheSound->GetMemoryUsage(), wczNum, 10);  //sound lib memory usage
		}

		SetText(wStr.c_str());
	}
}

//*****************************************************************************
void CFrameRateEffect::SetText(const WCHAR* pText)
//Sets text being displayed.
{
	const WCHAR *text = pText ? pText : wszEmpty;

	//Get area.
	UINT w, h;
	g_pTheFM->GetTextWidthHeight(FONTLIB::F_FrameRate, text, w, h);

	if (!w) w = 1; //to avoid SDL 1.2 crashes with empty text
	if (!h) h = 1;

	if (this->pTextSurface) {
		SDL_FreeSurface(this->pTextSurface);
		this->pTextSurface = NULL;
	}

	this->pTextSurface = CBitmapManager::ConvertSurface(
		SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, g_pTheBM->BITS_PER_PIXEL, 0x000000ff, 0x0000ff00, 0x00ff0000, 0xff000000));
	const Uint32 color = SDL_MapRGBA(this->pTextSurface->format, 0, 0, 0, SDL_ALPHA_TRANSPARENT);
	SDL_FillRect(this->pTextSurface, NULL, color);

	g_pTheFM->DrawTextToRect(FONTLIB::F_FrameRate, text,
			0, 0, w, h, this->pTextSurface);

	//Get area of effect.
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].w = w;
	this->dirtyRects[0].h = h;
}

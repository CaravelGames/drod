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

#include "VarMonitorEffect.h"
#include "DrodFontManager.h"
#include "RoomWidget.h"
#include "../DRODLib/DbHolds.h"
#include <BackEndLib/Assert.h>

//
//Public methods.
//

//*****************************************************************************
CVarMonitorEffect::CVarMonitorEffect(CWidget *pSetWidget)
	: CEffect(pSetWidget, (UINT)-1, EVARMONITOR)
	, x(pOwnerWidget->GetX())
	, y(pOwnerWidget->GetY())
	, lastTurn((UINT)-1)
	, pTextSurface(NULL)
//Constructor.
{
	//Even when other effects are cleared, it's generally okay to leave this one.
	RequestRetainOnClear();

	SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y, 0, 0);
	this->dirtyRects.push_back(rect);

	this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);

	//Get current var state.
	CCurrentGame *pGame = (CCurrentGame*)pRoomWidget->GetCurrentGame();
	if (pGame)
		pGame->GetVarValues(this->lastVars);
}

CVarMonitorEffect::~CVarMonitorEffect()
{
	if (pTextSurface)
		SDL_FreeSurface(this->pTextSurface);
}

//*****************************************************************************
void CVarMonitorEffect::SetTextForNewTurn()
{
	const UINT turn = this->pRoomWidget->GetLastTurn();
	if (turn == this->lastTurn)
		return;

	//Check vars for updated state.
	CCurrentGame *pGame = (CCurrentGame*)pRoomWidget->GetCurrentGame();
	VARMAP curVars;
	set<VarNameType> diff, diff2;
	if (pGame)
	{
		pGame->GetVarValues(curVars);
		pGame->DiffVarValues(this->lastVars,curVars,diff);
		pGame->DiffVarValues(curVars,this->lastVars,diff2);
		diff.insert(diff2.begin(), diff2.end());
	}
	if (!diff.empty())
	{
		//Vars have changed.  Update display.
		WSTRING newText;
		for (set<VarNameType>::const_iterator var = diff.begin(); var != diff.end(); ++var)
		{
			//Print changed var names and values.
			WSTRING temp;
			const char *pVarName = var->c_str();
			AsciiToUnicode(pVarName, temp);
			char *varID = pGame->pHold->getVarAccessToken(temp.c_str());
			newText += temp;
			newText += wszSpace;
			newText += wszEqual;
			newText += wszSpace;
			const UNPACKEDVARTYPE vType = pGame->stats.GetVarType(varID);
			const bool bValidInt = vType == UVT_int || vType == UVT_unknown;
			if (bValidInt)
			{
				WCHAR temp[16];
				const int val = pGame->stats.GetVar(varID, int(0));
				newText += _itoW(val, temp, 10);
			} else {
				newText += pGame->stats.GetVar(varID, wszEmpty);
			}
			newText += wszCRLF;
		}
		this->lastVars = curVars;

		SetText(newText.c_str());
	}

	this->lastTurn = turn;
	this->dwTimeElapsed = 0;
}

//*****************************************************************************
bool CVarMonitorEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	SetTextForNewTurn();

	//Fade out after a while.
	UpdateOpacity(dwTimeElapsed);

	this->dirtyRects[0].w = this->pTextSurface->w;
	this->dirtyRects[0].h = this->pTextSurface->h;

	return true;
}

//*****************************************************************************
void CVarMonitorEffect::UpdateOpacity(const Uint32 dwTimeElapsed)
{
	static const UINT timeToFadeBegin = 2000, timeToFadeEnd = 3000; //ms

	this->nOpacity = 255;
	if (dwTimeElapsed > timeToFadeEnd)
		this->nOpacity = 0;

	else if (g_pTheBM->bAlpha && dwTimeElapsed > timeToFadeBegin)
		this->nOpacity = (timeToFadeEnd - dwTimeElapsed) * 255 / (timeToFadeEnd - timeToFadeBegin);
}

//*****************************************************************************
void CVarMonitorEffect::Draw(SDL_Surface& pDestSurface)
{

	ASSERT(this->dirtyRects.size() == 1);
	if (!this->nOpacity) {
		this->dirtyRects[0].w = 0;

	} else {
		ASSERT(this->pTextSurface);
		g_pTheBM->SetSurfaceAlpha(this->pTextSurface, this->nOpacity);

		SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y, this->pTextSurface->w, this->pTextSurface->h);
		SDL_BlitSurface(this->pTextSurface, NULL, &pDestSurface, &rect);

	}
}

//*****************************************************************************
void CVarMonitorEffect::SetText(const WCHAR* pText)
//Sets text being displayed.
{
	const WCHAR *text = pText ? pText : wszEmpty;

	//Get area.
	UINT w, h;
	g_pTheFM->GetTextRectHeight(F_FrameRate, text, this->pRoomWidget->GetW(), w, h);

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

	g_pTheFM->DrawTextToRect(F_FrameRate, text,
			0, 0, w, h, this->pTextSurface);

	this->dwTimeElapsed = 0;
}

// $Id$

/****** BEGIN LICENSE BLOCK *****
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

#include "NoticeEffect.h"
#include <FrontEndLib/BitmapManager.h>

#include <BackEndLib/Assert.h>
#include <BackEndLib/UtilFuncs.h>

//Background tool tip color.
const SURFACECOLOR Border = {0, 0, 0}; //black

const UINT wBorder = 2; //border buffer (pixels)
const UINT widthBuffer = (wBorder+1)*2; //how much width is unavailable for text rendering

const UINT NOTICE_BOX_MINIMUM_WIDTH = 170;
const UINT NOTICE_BOX_MINIMUM_HEIGHT = 50;

const float SLIDE_FACTOR = 0.1f;

//
//Public methods.
//

//*****************************************************************************
CNoticeEffect::CNoticeEffect(
	CWidget *pSetWidget, //(in)   Required params for CEffect 
	const WCHAR* pTitle, const WCHAR* pText,
	const UINT wYOffset, //[default=0]
	const UINT eType)
	: CEffect(pSetWidget, (UINT)-1, eType)
	, wYOffset(wYOffset)
	, eFontType(FONTLIB::F_Small)
	, x(0), y(0)
	, dwDuration(3000) //ms
	, state(NS_Init)
	, pNoticeSurface(NULL)
	, opacity(225)	//88%
{
	ASSERT(this->pOwnerWidget);
	this->pOwnerWidget->GetRect(this->screenRect);

	this->BGColor.byt1 = 255;
	this->BGColor.byt2 = 255;
	this->BGColor.byt3 = 255;

	PrepWidget(pTitle, pText);
}

//*****************************************************************************
CNoticeEffect::~CNoticeEffect()
{
	if (this->pNoticeSurface) {
		SDL_FreeSurface(this->pNoticeSurface);
		this->pNoticeSurface = NULL;
	}
}

//*****************************************************************************
void CNoticeEffect::PrepWidget(const WCHAR* pTitle, const WCHAR* pText)
//Set widget dimensions and prepare surface to render.
{
	UINT wW1, wW2, wH1, wH2;
	g_pTheFM->GetTextWidthHeight(this->eFontType, pTitle, wW1, wH1);
	g_pTheFM->GetTextWidthHeight(this->eFontType, pText, wW2, wH2);

	this->w = max(NOTICE_BOX_MINIMUM_WIDTH, max(wW1, wW2) + widthBuffer);
	this->w = min(this->w, UINT(this->screenRect.w));

	this->h = max(NOTICE_BOX_MINIMUM_HEIGHT, wH1 + wH2 + widthBuffer);

	ASSERT(this->w >= widthBuffer);
	ASSERT(this->h >= widthBuffer);

	if (this->pNoticeSurface)
		SDL_FreeSurface(this->pNoticeSurface);
	this->pNoticeSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, this->w, this->h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	ASSERT(this->pNoticeSurface);

	//Ensure color is in same pixel color order as surface.
	const SURFACECOLOR BGColor = GetSurfaceColor(this->pNoticeSurface,
			this->BGColor.byt1, this->BGColor.byt2, this->BGColor.byt3);

	//Fill rectangle and draw border.
	SDL_Rect BorderRect = MAKE_SDL_RECT(0, 0, this->w, this->h);
	this->pOwnerWidget->DrawFilledRect(BorderRect, BGColor, this->pNoticeSurface);
	this->pOwnerWidget->DrawRect(BorderRect, Border, this->pNoticeSurface);

	//Draw text.
	SDL_Rect TextRect = MAKE_SDL_RECT(wBorder, wBorder, this->w-widthBuffer, this->h-widthBuffer);
#ifdef RUSSIAN_BUILD
	static const int YOFFSET = 1;   //this font draws a tad too low
#else
	static const int YOFFSET = 4;   //this font always draws too low
#endif
	UINT wY = TextRect.y - YOFFSET;

	g_pTheFM->DrawTextXY(this->eFontType, pTitle, this->pNoticeSurface,
			TextRect.x+wBorder, wY, TextRect.w);

	g_pTheFM->DrawTextXY(this->eFontType, pText, this->pNoticeSurface,
			TextRect.x+wBorder, wY + wH1 + wBorder + 1, TextRect.w);

	if (g_pTheBM->bAlpha)
		SetAlpha(this->opacity);

	//Area of effect.
	this->dirtyRects.clear();
	SDL_Rect rect = MAKE_SDL_RECT(0, 0, this->w, this->h);
	this->dirtyRects.push_back(rect);
}

//*****************************************************************************
void CNoticeEffect::SetAlpha(const Uint8 opacity)
//Sets opacity of effect.
{
	this->opacity = opacity;
	if (!this->pNoticeSurface)
		return;

	AutodetectSurfaceBlending(this->pNoticeSurface, this->opacity);
}

//*****************************************************************************
bool CNoticeEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	if (this->state == NS_Done)
		return false;

	SetLocation(dwTimeElapsed);

	//Update area of effect.
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].x = this->x;
	this->dirtyRects[0].y = this->y;

	return true;
}
//*****************************************************************************
void CNoticeEffect::Draw(SDL_Surface& pDestSurface)
{
	SDL_Rect SrcRect = MAKE_SDL_RECT(0, 0, this->w, this->h);
	SDL_Rect ScreenRect = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);

	SDL_SetClipRect(&pDestSurface, &this->screenRect);
	SDL_BlitSurface(this->pNoticeSurface, &SrcRect, &pDestSurface, &ScreenRect);
	SDL_SetClipRect(&pDestSurface, NULL);
}

//*****************************************************************************
void CNoticeEffect::SetLocation(const Uint32 dwTimeElapsed)
{
	switch (this->state) {
		case NS_Init:
			// Set initial position: out of bounds to the left
			this->x = this->screenRect.x - int(this->w);
			this->y = this->screenRect.y + this->wYOffset;

			this->state = NS_SlideIn;
			this->targetX = this->screenRect.x;
			this->targetY = this->y;
			break;
		case NS_SlideIn:
			if (this->x >= this->targetX) {
				this->x = this->targetX;
				this->state = NS_DisplayPause;
			}
			break;
		case NS_DisplayPause:
			if (dwTimeElapsed >= this->dwDuration) {
				this->state = NS_SlideOut;
				this->targetX = this->x;
				this->targetY = this->screenRect.y - this->h;
			}
			break;
		case NS_SlideOut:
			if (this->y <= this->targetY)
				this->state = NS_Done;
			break;
		default: break;
	}

	// Move the effect towards the target.
	const int deltaX = this->targetX-this->x;
	const int deltaY = this->targetY-this->y;
	const int incX = int(SLIDE_FACTOR * deltaX);
	const int incY = int(SLIDE_FACTOR * deltaY);
	this->x += !incX ? sgn(deltaX) : incX;
	this->y += !incY ? sgn(deltaY) : incY;
}


//*****************************************************************************
CCaravelNetNoticeEffect::CCaravelNetNoticeEffect(
	CWidget *pSetWidget,                //(in)   Required params for CEffect 
	const CNetNotice &notice,          //(in)   Text that label will display.
	NOTICES &notices)
	: CNoticeEffect(pSetWidget, notice.title.c_str(), notice.text.c_str(), 0, ECNETNOTICE)
	, notices(notices)
{
	if (this->notices.size() > 0) {
		CNoticeEffect* pLast = this->notices.back();
		this->y = pLast->y + pLast->h;
	} else {
		this->y = this->screenRect.y;
	}

	this->notices.push_back(this);

	// Add a little buffer so multiple notices don't try to disappear at the same time.
	this->dwDuration += this->notices.size() * 100;
}

//*****************************************************************************
CCaravelNetNoticeEffect::~CCaravelNetNoticeEffect()
{
	RemoveFromNotices();
}

//*****************************************************************************
void CCaravelNetNoticeEffect::RemoveFromNotices()
{
}

//*****************************************************************************
void CCaravelNetNoticeEffect::SetLocation(const Uint32 dwTimeElapsed)
{
	switch (this->state) {
		case NS_Init:
			// Set initial position: offscreen to the right below all other visible notices
			this->x = this->screenRect.x + this->screenRect.w;
			//this->y was set previously
			this->state = NS_SlideIn;
			this->targetX = this->screenRect.x + this->screenRect.w - this->w;
			this->targetY = this->y;
			break;

		case NS_SlideIn:
			if (this->x + this->w <= (UINT)(this->screenRect.x + this->screenRect.w)) {
				this->state = NS_DisplayPause;
				this->dwTimeElapsed = 0;
			}
			break;

		case NS_DisplayPause:
			if (dwTimeElapsed > this->dwDuration) {
				this->state = NS_SlideOut;
				this->targetX = this->x;
				this->targetY = this->screenRect.y < int(this->h) ? 0 : this->screenRect.y - this->h;
				bool bFound = false;
				for (NOTICES::iterator i = this->notices.begin(); i != this->notices.end(); i++) {
					if (*i == this) {
						bFound = true;
					} else if (bFound) {
						(*i)->state = NS_MoveUp;
					}
				}
			}
			break;

		case NS_MoveUp:
			// A notice has been deleted.  Move this one up.
			this->targetY = this->y < int(this->h) ? 0 : this->y - this->h;
			this->targetX = this->x;
			this->state = NS_MovingUp;
			this->dwDuration += 500;
			break;

		case NS_MovingUp:
			if (this->y == this->targetY) {
				this->state = NS_DisplayPause;
			}
			break;

		case NS_SlideOut:
			if (this->y == this->targetY) {
				// Done!
				this->state = NS_Done;
				NOTICES::iterator delMe = this->notices.end();
				for (NOTICES::iterator i = this->notices.begin(); i != this->notices.end(); i++) {
					if (*i == this) {
						delMe = i;
					}
				}
				if (delMe != this->notices.end()) {
					this->notices.erase(delMe);
				}
			}
			break;

		case NS_Done:
			break;
	}

	// Move the effect towards the target.
	const int deltaX = this->targetX-this->x;
	const int deltaY = this->targetY-this->y;
	const int incX = int(SLIDE_FACTOR * deltaX);
	const int incY = int(SLIDE_FACTOR * deltaY);
	this->x += !incX ? sgn(deltaX) : incX;
	this->y += !incY ? sgn(deltaY) : incY;
}

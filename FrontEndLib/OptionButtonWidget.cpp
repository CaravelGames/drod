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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "OptionButtonWidget.h"
#include "BitmapManager.h"
#include "FontManager.h"
#include "EventHandlerWidget.h"
#include "Sound.h"
#include <BackEndLib/Exception.h>

//Source coords and dimensions within parts surface.
const int X_OB_C = 272;
const int Y_OB_C = 30;
const int X_OB_UC = 272;
const int Y_OB_UC = 52;
const UINT CX_OB = 22;
const UINT CY_OB = 22;
const UINT CX_FOCUS = CX_OB + 4; //2 pixel buffer
const UINT CY_FOCUS = CY_OB + 4;

//
//Public methods.
//

//******************************************************************************
COptionButtonWidget::COptionButtonWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,             //(in)   Required params for CWidget 
	const int nSetX, const int nSetY,   //    constructor.
	const UINT wSetW, const UINT wSetH, //
	const WCHAR *pwczSetCaption,        //(in)   Caption for button.  "" = none.
	const bool bSetChecked,             //(in)   Button begins checked?
	const bool bWhiteText)              //(in)   Make text white (default=false).
	: CFocusWidget(WT_OptionButton, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, bFocusRegionsSaved(false)
	, bIsChecked(bSetChecked)
	, bWhiteText(bWhiteText)
{
	SetCaption(pwczSetCaption);

	//Create surface to save screen surface bits where focus is drawn.
	this->pFocusSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, CX_FOCUS, CY_FOCUS, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	if (!this->pFocusSurface) throw CException();

	this->imageFilenames.push_back(string("Dialog"));
}

//******************************************************************************
COptionButtonWidget::~COptionButtonWidget()
{
	if (this->pFocusSurface)
	{
		SDL_FreeSurface(this->pFocusSurface);
		this->pFocusSurface = NULL;
	}
}

//******************************************************************************
void COptionButtonWidget::SetChecked(
	const bool bSetChecked)
{
	if (bSetChecked)  //Mac port fix (weird, but necessary)
		this->bIsChecked = true;
	else
		this->bIsChecked = false;
}

//******************************************************************************
void COptionButtonWidget::SetChecked(
	const int nSetChecked)
{
	if (nSetChecked != 0) //Mac port fix (weird, but necessary)
		this->bIsChecked = true;
	else
		this->bIsChecked = false;
}

//******************************************************************************
void COptionButtonWidget::ToggleCheck(
//Changes checked status.
//
//Params:
	const bool bShowEffects)   //(in) default = false
{
	this->bIsChecked = !this->bIsChecked;

	//Call OnSelectChange() notifier.
	CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
	if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());

	if (bShowEffects)
	{
		g_pTheSound->PlaySoundEffect(SOUNDLIB::SEID_BUTTON);
		RequestPaint();
	}
}

//******************************************************************************
void COptionButtonWidget::SetCaption(
//Sets caption that appears on button.
//
//Params:
	const WCHAR *pwczSetCaption)  //(in)   New text.
{
	ASSERT(pwczSetCaption);

	this->wstrCaption = pwczSetCaption;
}

//******************************************************************************
void COptionButtonWidget::Paint(
//Paint widget area.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//Drawing code below needs to be modified to accept offsets.  Until then,
	//this widget can't be offset.
	ASSERT(!IsScrollOffset());

	SDL_Rect FocusScreenRect = MAKE_SDL_RECT(this->x, this->y + (this->h - CY_OB) / 2 - 2,
			CX_FOCUS, CY_FOCUS);
	SDL_Rect FocusRect = MAKE_SDL_RECT(0, 0, CX_FOCUS, CY_FOCUS);

	ASSERT(this->w >= CX_OB && this->h >= CY_OB);

	//Save spot where focus box is drawn.
	SDL_Surface *pDestSurface = GetDestSurface();
	if (!this->bFocusRegionsSaved)
	{
		SDL_BlitSurface(pDestSurface, &FocusScreenRect,
				this->pFocusSurface, &FocusRect);
		this->bFocusRegionsSaved = true;
	}

	//Draw with or without focus.
	if (this->IsSelected())
		DrawFocused();
	else
		SDL_BlitSurface(this->pFocusSurface, &FocusRect, pDestSurface, &FocusScreenRect);

	//Draw checkbox.
	if (this->bIsChecked)
		DrawChecked();
	else
		DrawUnchecked();

	//Draw caption.
	if (this->wstrCaption.size())
	{
		static const UINT CX_CAPTION_SPACE = 6; //4 + 2 pixels for focus graphic
#ifdef RUSSIAN_BUILD
		static const int Y_OFFSET = 0;
#else
		static const int Y_OFFSET = -2;	//kludge -- font is too low otherwise
#endif
		UINT wIgnored, wTextHeight;
		g_pTheFM->GetTextRectHeight(FONTLIB::F_Button, 
				this->wstrCaption.c_str(), this->w - CX_OB - CX_CAPTION_SPACE, wIgnored,
				wTextHeight);
		const bool bFitsH = wTextHeight <= this->h;
		const int yOffset = (bFitsH ? ((this->h - wTextHeight) / 2) : 0) + Y_OFFSET;
		g_pTheFM->DrawTextXY(!IsEnabled() ? FONTLIB::F_Button_Disabled :
				this->bWhiteText ? FONTLIB::F_ButtonWhite : FONTLIB::F_Button,
				this->wstrCaption.c_str(), GetDestSurface(),
				this->x + CX_OB + CX_CAPTION_SPACE,
				this->y + yOffset,
				this->w - CX_OB - CX_CAPTION_SPACE,
				this->h - (yOffset > 0 ? yOffset : 0));
	}

	PaintChildren();

	if (bUpdateRect) UpdateRect();
}

//
//Protected methods.
//

//******************************************************************************
void COptionButtonWidget::HandleKeyDown(
//Processes a keyboard event within the scope of the widget.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
//
//Returns:
//True.
{
	const SDL_Keycode key = KeyboardEvent.keysym.sym;

	switch (key) {
		case SDLK_SPACE:
			ToggleCheck(true);
		break;

		default: break;
	}
}

//******************************************************************************
void COptionButtonWidget::HandleMouseDown(
//Handles a mouse down event.
//
//Params:
	const SDL_MouseButtonEvent &/*Button*/)   //(in)   Event to handle.
{
	ToggleCheck(true);
}

//
//Private methods.
//

//******************************************************************************
void COptionButtonWidget::DrawChecked()
//Draw button in its checked state.
{
	static SDL_Rect Src = {X_OB_C, Y_OB_C, CX_OB, CY_OB};
	static SDL_Rect Dest = {0, 0, CX_OB, CY_OB};
	Dest.x = this->x + 2;
	Dest.y = this->y + (this->h - CY_OB) / 2;
	SDL_BlitSurface(this->images[0], &Src, GetDestSurface(), &Dest);
}

//******************************************************************************
void COptionButtonWidget::DrawUnchecked()
//Draw button in its unchecked state.
{
	static SDL_Rect Src = {X_OB_UC, Y_OB_UC, CX_OB, CY_OB};
	static SDL_Rect Dest = {0, 0, CX_OB, CY_OB};
	Dest.x = this->x + 2;
	Dest.y = this->y + (this->h - CY_OB) / 2;
	SDL_BlitSurface(this->images[0], &Src, GetDestSurface(), &Dest);
}

//******************************************************************************
void COptionButtonWidget::DrawFocused()
//Draw focus.
{
	SDL_Surface *pDestSurface = GetDestSurface();

	const SURFACECOLOR FocusColor = GetSurfaceColor(pDestSurface, RGB_FOCUS);
	const UINT DRAWX = this->x;
	const UINT DRAWY = this->y + (this->h - CY_OB) / 2 - 2;
	const SDL_Rect rect = MAKE_SDL_RECT(DRAWX, DRAWY, CX_FOCUS, CY_FOCUS);

	//draw box around checkbox
	DrawRect(rect,FocusColor,pDestSurface);
}

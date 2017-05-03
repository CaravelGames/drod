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

#include "ButtonWidget.h"
#include "BitmapManager.h"
#include "FontManager.h"
#include "EventHandlerWidget.h"
#include "Sound.h"

#include <BackEndLib/Types.h>
#include <BackEndLib/Assert.h>
#include <BackEndLib/Ports.h>

//Source coords and dimensions within parts surface.
const UINT X_LEFT_BEVEL = 272;
const UINT Y_TOP_BEVEL = 0;
const UINT CX_CORNER = 2;
const UINT CY_CORNER = 2;
const UINT CX_CENTER = 26;
const UINT CY_CENTER = 26;
const UINT X_RIGHT_BEVEL = X_LEFT_BEVEL + CX_CORNER + CX_CENTER;
const UINT Y_BOTTOM_BEVEL = Y_TOP_BEVEL + CY_CORNER + CY_CENTER;
const UINT X_CENTER = X_LEFT_BEVEL + CX_CORNER;
const UINT Y_CENTER = Y_TOP_BEVEL + CY_CORNER;

//
//Public methods.
//

//******************************************************************************
CButtonWidget::CButtonWidget(
//Constructor.
//
//Params:
	UINT dwSetTagNo,                //(in)   Required params for CWidget
	int nSetX, int nSetY,               //    constructor.
	UINT wSetW, UINT wSetH,             //
	const WCHAR *pwczSetCaption)        //(in)   Caption for button.  "" = none.
	: CFocusWidget(WT_Button, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, bIsPressed(false)
	, bIsSilent(false)
{
	this->imageFilenames.push_back(string("Dialog"));

	SetCaption(pwczSetCaption);

	SetHotkeyFromText(pwczSetCaption);
}

//******************************************************************************
CButtonWidget::~CButtonWidget()
//Destructor.
//
{
}

//******************************************************************************
void CButtonWidget::HandleKeyDown(
//Processes a keyboard event within the scope of the widget.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
{
	const SDL_Keycode key = KeyboardEvent.keysym.sym;

	switch (key) {
		case SDLK_SPACE:
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
		{
			Press();
			RequestPaint();
			if (!this->bIsSilent)
				g_pTheSound->PlaySoundEffect(SOUNDLIB::SEID_BUTTON);
			SDL_Delay(200);
			Unpress();
			RequestPaint();
			SDL_Delay(50);

			//Call OnClick() notifier.
			CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
			if (pEventHandler) pEventHandler->OnClick(GetTagNo());
		}
		return;

		default: break;
	}
}

//******************************************************************************
void CButtonWidget::HandleDrag(
//Handles mouse motion occurring after a mousedown event in this widget.
//
//Params:
	const SDL_MouseMotionEvent &Motion) //(in) Event to handle.
{
	//The mouse button goes up or down depending on whether the cursor is
	//inside the widget.
	bool bWasPressed = this->bIsPressed;
	if (ContainsCoords(Motion.x, Motion.y))
		Press();
	else
		Unpress();

	//If there was a change, repaint the button.
	if (this->bIsPressed != bWasPressed)
	{
		if (this->bIsPressed && !this->bIsSilent)
				g_pTheSound->PlaySoundEffect(SOUNDLIB::SEID_BUTTON);
		RequestPaint();
	}
}

//******************************************************************************
void CButtonWidget::HandleMouseDown(
//Handles mouse down event.
//
//Params:
	const SDL_MouseButtonEvent &/*Button*/)   //(in) Event to handle.
{
	//The mouse button goes down.
	bool bWasPressed = this->bIsPressed;
	Press();

	//If there was a change, repaint the button.
	if (this->bIsPressed != bWasPressed)
	{
		if (!this->bIsSilent)
			g_pTheSound->PlaySoundEffect(SOUNDLIB::SEID_BUTTON);
		RequestPaint();
	}
}

//******************************************************************************
void CButtonWidget::HandleMouseUp(
//Handles mouse up event.
//
//Params:
	const SDL_MouseButtonEvent &/*Button*/)   //(in) Event to handle.
{
	//The mouse button goes down.
	bool bWasPressed = this->bIsPressed;
	Unpress();

	//If there was a change, repaint the button.
	if (this->bIsPressed != bWasPressed)
		RequestPaint();
}

//******************************************************************************
void CButtonWidget::SetCaption(
//Sets caption that appears on button.
//
//Params:
	const WCHAR *pwczSetCaption)  //(in)   New text.
{
	ASSERT(pwczSetCaption);

	this->wstrCaption = pwczSetCaption;
}

//******************************************************************************
void CButtonWidget::SetHotkeyFromText(
//Sets a hotkey to correspond with the text that appears on button.
//
//Params:
	const WCHAR *pwczSetCaption)  //(in)   New text.
{
	RemoveAllHotkeys();

	//Search for a (the first) hotkey.
	const UINT wLength = WCSlen(pwczSetCaption);
	for (UINT wIndex=0; wIndex<wLength; ++wIndex)
		if (pwczSetCaption[wIndex] == L'&' && wIndex+1 < wLength)
		{
			AddHotkey(static_cast<SDL_Keycode>(WCv(towlower(pwczSetCaption[wIndex+1]))),
					GetTagNo());   //no uppercase mappings exist
			break;
		}
}

//******************************************************************************
void CButtonWidget::Paint(
//Paint widget area.
//
//Params:
	bool bUpdateRect)       //(in)   If true (default) and destination
								//    surface is the screen, the screen
								//    will be immediately updated in
								//    the widget's rect.
{
	ASSERT(this->w > CX_CORNER * 2);
	ASSERT(this->h > CY_CORNER * 2);

	if (this->bIsPressed)
	{
		DrawPressed();
	} else {
		DrawNormal();
	}

	PaintChildren();
	
	if (bUpdateRect) UpdateRect();
}

//
//Private methods.
//

//******************************************************************************
void CButtonWidget::DrawButtonText(
//Draw button text.
//
//Params:
	const int nXOffset, const int nYOffset, const int yTextOffset)  //(in) Number of pixels to drop text.
{
	UINT wTextW, wTextH;
	int xDraw, yDraw;
	const UINT buttonFont = IsEnabled() ? FONTLIB::F_Button : FONTLIB::F_Button_Disabled;

	//Get a copy of caption text without any ampersands.
	WCHAR *pwczPlainCaption = new WCHAR[this->wstrCaption.size() + 1];
	if (!pwczPlainCaption) {ASSERT(!"Alloc failed."); return;}
	const WCHAR *pwczRead = this->wstrCaption.c_str();
	WCHAR *pwczWrite = pwczPlainCaption;
	while (*pwczRead != '\0')
	{
		if (*pwczRead != '&')
			*(pwczWrite++) = *pwczRead;
		++pwczRead;
	}
	pWCv(pwczWrite) = '\0';

	//Get dimensions of plain caption to be used for centering drawn text.
	g_pTheFM->GetTextWidthHeight(buttonFont, pwczPlainCaption, wTextW, wTextH);
	if (wTextH + yTextOffset > this->h)
		return; //No room for text.
	wTextW = g_pTheFM->GetHotkeyTextLineWidth(buttonFont, this->wstrCaption.c_str()); //correct calculation for hotkey text width
	if (wTextW > this->w)
		xDraw = this->x;
	else
		xDraw = this->x + (this->w - wTextW) / 2;
	yDraw = this->y + (this->h - wTextH) / 2;
#ifdef RUSSIAN_BUILD
		static const int DY_TEXT = -1; //kludge for better centering -- this font is drawn a bit too low on button otherwise
#else
		static const int DY_TEXT = -4; //kludge for better centering -- this font is drawn too low on button otherwise
#endif
	yDraw += DY_TEXT;

	//Draw the text.
	if (IsEnabled())
		g_pTheFM->DrawHotkeyTextToLine(buttonFont, this->wstrCaption.c_str(),
			xDraw + nXOffset, yDraw + nYOffset + yTextOffset, this->w, GetDestSurface());
	else
		g_pTheFM->DrawTextToRect(buttonFont, pwczPlainCaption,
			xDraw + nXOffset, yDraw + nYOffset + yTextOffset, this->w, this->h, GetDestSurface());

	delete[] pwczPlainCaption;
}

//******************************************************************************
void CButtonWidget::DrawFocused(
//Draw button with focus.
//
//Params:
	const int nXOffset, const int nYOffset) //(in) Number of pixels to offset focus indicator.
{
	SDL_Surface *pDestSurface = GetDestSurface();

	const SURFACECOLOR FocusColor = GetSurfaceColor(pDestSurface, RGB_FOCUS);
	static const UINT wIndent = 2;   //# pixels in from beveled edge that box shows
	const int DrawX = this->x + nXOffset;
	const int DrawY = this->y + nYOffset;
	const SDL_Rect rect = MAKE_SDL_RECT(DrawX + CX_CORNER + wIndent, DrawY + wIndent + 1,
		this->w - (wIndent + CX_CORNER) * 2, this->h - (wIndent + CY_CORNER) * 2 + 1);

	//draw box inside button edge
	DrawRect(rect,FocusColor,pDestSurface);
}

//******************************************************************************
void CButtonWidget::DrawNormal()
//Draw button in its normal state.
{
	SDL_Surface *pDestSurface = GetDestSurface();

	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);

	//Dest coords and dimensions.
	const int xLeftBevel = this->x + nOffsetX;
	const int xRightBevel = xLeftBevel + this->w - CX_CORNER;
	const int yTopBevel = this->y + nOffsetY;
	const int yBottomBevel = yTopBevel + this->h - CY_CORNER;
	const int xCenter = xLeftBevel + CX_CORNER;
	const int yCenter = yTopBevel + CY_CORNER;

	//Draw top-left corner.
	SDL_Rect src = MAKE_SDL_RECT(X_LEFT_BEVEL, Y_TOP_BEVEL, CX_CORNER, CY_CORNER);
	SDL_Rect dest = MAKE_SDL_RECT(xLeftBevel, yTopBevel, CX_CORNER, CY_CORNER);
	SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);

	//Draw top-right corner.
	src.x = X_RIGHT_BEVEL;
	dest.x = xRightBevel;
	SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);

	//Draw bottom-right corner.
	src.y = Y_BOTTOM_BEVEL;
	dest.y = yBottomBevel;
	SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);

	//Draw bottom-left corner.
	src.x = X_LEFT_BEVEL;
	dest.x = xLeftBevel;
	SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);

	//Draw bottom bevel.
	src.x = X_CENTER;
	dest.w = src.w = CX_CENTER;
	for (dest.x = xCenter; dest.x < static_cast<int>(xRightBevel); dest.x += CX_CENTER)
	{
		if (dest.x + int(CX_CENTER) > xRightBevel)
			dest.w = src.w = xRightBevel - dest.x; //Clip the bevel to remaining width.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw top bevel.
	dest.y = yTopBevel;
	src.y = Y_TOP_BEVEL;
	dest.w = src.w = CX_CENTER;
	for (dest.x = xCenter; dest.x < static_cast<int>(xRightBevel); dest.x += CX_CENTER)
	{
		if (dest.x + int(CX_CENTER) > xRightBevel)
			dest.w = src.w = xRightBevel - dest.x; //Clip the bevel to remaining width.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw left bevel.
	dest.x = xLeftBevel;
	src.x = X_LEFT_BEVEL;
	src.y = Y_CENTER;
	src.w = dest.w = CX_CORNER;
	src.h = dest.h = CY_CENTER;
	for (dest.y = yCenter; dest.y < static_cast<int>(yBottomBevel); dest.y += CY_CENTER)
	{
		if (dest.y + int(CY_CENTER) > yBottomBevel)
			dest.h = src.h = yBottomBevel - dest.y; //Clip the bevel to remaining height.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw right bevel.
	dest.x = xRightBevel;
	src.x = X_RIGHT_BEVEL;
	src.h = dest.h = CY_CENTER;
	for (dest.y = yCenter; dest.y < static_cast<int>(yBottomBevel); dest.y += CY_CENTER)
	{
		if (dest.y + int(CY_CENTER) > yBottomBevel)
			dest.h = src.h = yBottomBevel - dest.y; //Clip the bevel to remaining height.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw center.
	src.x = X_CENTER;
	src.y = Y_CENTER;
	src.h = dest.h = CY_CENTER;
	for (dest.y = yCenter; dest.y < static_cast<int>(yBottomBevel); dest.y += CY_CENTER)
	{
		src.w = dest.w = CX_CENTER;
		for (dest.x = xCenter; dest.x < static_cast<int>(xRightBevel); dest.x += CX_CENTER)
		{
			if (dest.x + int(CX_CENTER) > xRightBevel)
				dest.w = src.w = xRightBevel - dest.x; //Clip the blit to remaining width.
			if (dest.y + int(CY_CENTER) > yBottomBevel)
				dest.h = src.h = yBottomBevel - dest.y; //Clip the blit to remaining height.
			SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
		}
	}

	if (IsSelected())
		DrawFocused(nOffsetX, nOffsetY);

	DrawButtonText(nOffsetX, nOffsetY, 0);
}

//******************************************************************************
void CButtonWidget::DrawPressed()
//Draw button in its pressed state.
{
	SDL_Surface *pDestSurface = GetDestSurface();

	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);

	//Dest coords and dimensions.
	const int xLeftBevel = this->x + nOffsetX;
	const int xRightBevel = xLeftBevel + this->w - CX_CORNER;
	const int yTopBevel = this->y + nOffsetY;
	const int yBottomBevel = yTopBevel + this->h - CY_CORNER;
	const int xCenter = xLeftBevel + CX_CORNER;
	const int yCenter = yTopBevel + CY_CORNER;

	//If the button bevel is enlarged beyond 1 pixel, then it is no longer
	//visually correct to blit the same top-right and bottom-left corner as
	//DrawNormal() does.
	//!!(I'm going to let this slide for 2 pixels.)
	ASSERT(CX_CORNER <= 2 && CY_CORNER <= 2);

	//Draw top-left corner--use bottom-right corner for source.
	SDL_Rect src = MAKE_SDL_RECT(X_RIGHT_BEVEL, Y_BOTTOM_BEVEL, CX_CORNER, CY_CORNER);
	SDL_Rect dest = MAKE_SDL_RECT(xLeftBevel, yTopBevel, CX_CORNER, CY_CORNER);
	SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);

	//Draw top-right corner.
	src.y = Y_TOP_BEVEL;
	dest.x = xRightBevel;
	SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);

	//Draw bottom-right corner--use top-left corner for source.
	src.x = X_LEFT_BEVEL;
	dest.y = yBottomBevel;
	SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);

	//Draw bottom-left corner.
	src.y = Y_BOTTOM_BEVEL;
	dest.x = xLeftBevel;
	SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);

	//Draw top bevel--use bottom bevel for source.
	src.x = X_CENTER;
	dest.y = yTopBevel;
	dest.w = src.w = CX_CENTER;
	for (dest.x = xCenter; dest.x < static_cast<int>(xRightBevel); dest.x += CX_CENTER)
	{
		if (dest.x + int(CX_CENTER) > xRightBevel)
			dest.w = src.w = xRightBevel - dest.x; //Clip the bevel to remaining width.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw bottom bevel--use top bevel for source.
	src.y = Y_TOP_BEVEL;
	dest.y = yBottomBevel;
	dest.w = src.w = CX_CENTER;
	for (dest.x = xCenter; dest.x < static_cast<int>(xRightBevel); dest.x += CX_CENTER)
	{
		if (dest.x + int(CX_CENTER) > xRightBevel)
			dest.w = src.w = xRightBevel - dest.x; //Clip the bevel to remaining width.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw left bevel--use right bevel for source.
	dest.x = xLeftBevel;
	src.x = X_RIGHT_BEVEL;
	src.y = Y_CENTER;
	src.w = dest.w = CX_CORNER;
	src.h = dest.h = CY_CENTER;
	for (dest.y = yCenter; dest.y < static_cast<int>(yBottomBevel); dest.y += CY_CENTER)
	{
		if (dest.y + int(CY_CENTER) > yBottomBevel)
			dest.h = src.h = yBottomBevel - dest.y; //Clip the bevel to remaining height.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw right bevel--use left bevel for source.
	dest.x = xRightBevel;
	src.x = X_LEFT_BEVEL;
	src.h = dest.h = CY_CENTER;
	for (dest.y = yCenter; dest.y < static_cast<int>(yBottomBevel); dest.y += CY_CENTER)
	{
		if (dest.y + int(CY_CENTER) > yBottomBevel)
			dest.h = src.h = yBottomBevel - dest.y; //Clip the bevel to remaining height.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw top row of center.
	src.x = X_CENTER;
	src.y = Y_CENTER + CY_CENTER - 1;
	src.h = dest.h = 1;
	src.w = dest.w = CX_CENTER;
	dest.y = yCenter;
	for (dest.x = xCenter; dest.x < static_cast<int>(xRightBevel); dest.x += CX_CENTER)
	{
		if (dest.x + int(CX_CENTER) > xRightBevel)
			dest.w = src.w = xRightBevel - dest.x; //Clip the blit to remaining width.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw rest of center.
	src.y = Y_CENTER;
	src.h = dest.h = CY_CENTER;
	for (dest.y = yCenter + 1; dest.y < static_cast<int>(yBottomBevel); dest.y += CY_CENTER)
	{
		src.w = dest.w = CX_CENTER;
		for (dest.x = xCenter; dest.x < static_cast<int>(xRightBevel); dest.x += CX_CENTER)
		{
			if (dest.x + int(CX_CENTER) > xRightBevel)
				dest.w = src.w = xRightBevel - dest.x; //Clip the blit to remaining width.
			if (dest.y + int(CY_CENTER) > yBottomBevel)
				dest.h = src.h = yBottomBevel - dest.y; //Clip the blit to remaining height.
			SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
		}
	}

	if (IsSelected())
		DrawFocused(nOffsetX, nOffsetY + 1);

	//Draw text -- one pixel lower than for not pressed.
	DrawButtonText(nOffsetX, nOffsetY, 1);
}

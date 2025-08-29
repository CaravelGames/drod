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

#include "DialogWidget.h"
#include "BitmapManager.h"
#include "ButtonWidget.h"
#include "ListBoxWidget.h"
#include "MenuWidget.h"
#include "Screen.h"
#include "TextBoxWidget.h"
#include <BackEndLib/Types.h>
#include <BackEndLib/Assert.h>

//Source coords and dimensions.
const UINT CX_CORNER = 8;
const UINT CY_CORNER = 8;
const UINT X_LEFT_BEVEL = 0;
const UINT X_RIGHT_BEVEL = 264;
const UINT Y_TOP_BEVEL = 0;
const UINT Y_BOTTOM_BEVEL = 264;
const UINT X_CENTER = X_LEFT_BEVEL + CX_CORNER;
const UINT Y_CENTER = Y_TOP_BEVEL + CY_CORNER;
const UINT CX_CENTER = 256;
const UINT CY_CENTER = 256;

//
//Public methods.
//

//******************************************************************************
CDialogWidget::CDialogWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget 
	const int nSetX, const int nSetY,      //    constructor.
	const UINT wSetW, const UINT wSetH,    //
	const bool bListBoxDoubleClickReturns) //[default = false]
	: CEventHandlerWidget(WT_Dialog, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, dwDeactivateValue(TAG_UNSPECIFIED)
	, dwReqTextField(0L)
	, bListBoxDoubleClickReturns(bListBoxDoubleClickReturns)
{
	this->imageFilenames.push_back(string("Dialog"));
	this->centerRect.x = this->centerRect.y = this->centerRect.w = this->centerRect.h = 0;
}

//*****************************************************************************
void CDialogWidget::Center()
//Centers this widget inside its parent area.
{
	if (this->centerRect.w && this->centerRect.h)
	{
		const int nNewX = this->centerRect.x + (static_cast<int>(this->centerRect.w) - static_cast<int>(this->w)) / 2;
		const int nNewY = this->centerRect.y + (static_cast<int>(this->centerRect.h) - static_cast<int>(this->h)) / 2;
		const int dx = nNewX - this->x;
		const int dy = nNewY - this->y;
		if (dx || dy)
		{
			this->x = nNewX;
			this->y = nNewY;

			MoveChildren(dx, dy);
		}
	} else {
		CWidget::Center();
	}
}

//******************************************************************************
UINT CDialogWidget::Display(const bool bSelectFirstWidget) //[default=true]
//All-in-one method to show a dialog, paint it, activate it, and hide the 
//dialog again.  Caller should probably repaint parent widget of dialog to 
//erase the dialog.
//
//Returns:
//Tag# indicating reason for exit--either TAG_QUIT or widget tag that was clicked.
{
	//Allow dialogs to appear during screen activation.
	const bool bActivatingScreen = g_pTheSM->bTransitioning;
	g_pTheSM->bTransitioning = false;
	Show();

	SetForActivate();
	if (bSelectFirstWidget)
		SelectFirstWidget(false);
	ASSERT(IsLoaded());  //dialog must be loaded before painted
	RequestPaint();

	Activate();

	//Re-enable the OK button if it was disabled for mandatory text entry.
	if (this->dwReqTextField)
	{
		CButtonWidget *pOKButton = DYN_CAST(CButtonWidget*, CWidget *,
				GetWidget(TAG_OK));
		if (pOKButton)
			pOKButton->Enable();
	}

	Hide();

	g_pTheSM->bTransitioning = bActivatingScreen;
	return this->dwDeactivateValue;
}

//******************************************************************************
void CDialogWidget::Paint(
//Paint widget area.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//NOTE: It is not necessary to apply scrolling offsets to dialog coords.
	//Dialogs float on top of their parents in the same position, regardless
	//of scrolling offsets.

	SDL_Surface *pDestSurface = GetDestSurface();

	ASSERT(this->w > CX_CORNER * 2);
	ASSERT(this->h > CY_CORNER * 2);

	//Dest coords and dimensions.
	UINT xLeftBevel = this->x;
	UINT xRightBevel = this->x + this->w - CX_CORNER;
	UINT yTopBevel = this->y;
	UINT yBottomBevel = this->y + this->h - CY_CORNER;
	UINT xCenter = xLeftBevel + CX_CORNER;
	UINT yCenter = yTopBevel + CY_CORNER;

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
		if (dest.x + CX_CENTER > xRightBevel)
			dest.w = src.w = xRightBevel - dest.x; //Clip the bevel to remaining width.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw top bevel.
	dest.y = yTopBevel;
	src.y = Y_TOP_BEVEL;
	dest.w = src.w = CX_CENTER;
	for (dest.x = xCenter; dest.x < static_cast<int>(xRightBevel); dest.x += CX_CENTER)
	{
		if (dest.x + CX_CENTER > xRightBevel)
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
		if (dest.y + CY_CENTER > yBottomBevel)
			dest.h = src.h = yBottomBevel - dest.y; //Clip the bevel to remaining height.
		SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
	}

	//Draw right bevel.
	dest.x = xRightBevel;
	src.x = X_RIGHT_BEVEL;
	src.h = dest.h = CY_CENTER;
	for (dest.y = yCenter; dest.y < static_cast<int>(yBottomBevel); dest.y += CY_CENTER)
	{
		if (dest.y + CY_CENTER > yBottomBevel)
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
			if (dest.x + CX_CENTER > xRightBevel)
				dest.w = src.w = xRightBevel - dest.x; //Clip the blit to remaining width.
			if (dest.y + CY_CENTER > yBottomBevel)
				dest.h = src.h = yBottomBevel - dest.y; //Clip the blit to remaining height.
			SDL_BlitSurface(this->images[0], &src, pDestSurface, &dest);
		}
	}

	PaintChildren();

	if (bUpdateRect) UpdateRect();
}

//
//Protected methods.
//

//******************************************************************************
bool CDialogWidget::SetForActivate()
//Called before dialog is painted and activated.
//
//Returns:
//True if activation should continue, false if not.
{
	CheckTextBox();

	this->dwDeactivateValue=TAG_UNSPECIFIED;
	return true;
}

//*****************************************************************************
void CDialogWidget::OnClick(
//Handles click event.
//
//Params:
	const UINT dwTagNo)          //(in)   Widget that received event.
{
	if (!dwTagNo) return;

	CWidget *pWidget = GetWidget(dwTagNo);
	switch (pWidget->GetType())
	{
		//Clicking only these widgets will return from the dialog.
		case WT_Button:
			this->dwDeactivateValue = dwTagNo;
			Deactivate();
		break;
		default: break;
	}
}

//*****************************************************************************
void CDialogWidget::OnDoubleClick(
//Handles double click event.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	if (dwTagNo)
	{
		CWidget *pWidget = GetWidget(dwTagNo);
		if (pWidget && pWidget->GetType() == WT_ListBox && this->bListBoxDoubleClickReturns)
		{
			CListBoxWidget *pListBox = DYN_CAST(CListBoxWidget*, CWidget*, pWidget);
			if (pListBox->ClickedSelection())
			{
				//Double-clicking list box selection on the dialog activates OK button
				this->dwDeactivateValue = TAG_OK;
				Deactivate();
			}
		}
	}
}

//*****************************************************************************
void CDialogWidget::OnSelectChange(
//Handles selection change event.
//Pass event on to parent, if exists.
//
//Params:
	const UINT dwTagNo)
{
	if (!dwTagNo) return;

	//Selecting a menu option will return from the dialog.
	CWidget *pWidget = GetWidget(dwTagNo);
	switch (pWidget->GetType())
	{
		case WT_Menu:
		{
			CMenuWidget *pMenu = DYN_CAST(CMenuWidget*, CWidget*, pWidget);
			const UINT dwVal = pMenu->GetSelectedOption();
			if (dwVal != static_cast<UINT>(-1))
			{
				this->dwDeactivateValue = dwVal;
				Deactivate();
			}
		}
		break;
		default: break;
	}

	CEventHandlerWidget *pEventHandler = GetParentEventHandlerWidget();
	if (pEventHandler) pEventHandler->OnSelectChange(dwTagNo);
}

//*****************************************************************************
bool CDialogWidget::OnQuit()
//Handles SDL_QUIT event.
{
	if (this->pParent && this->pParent->GetType() == WT_Screen)
	{
		CScreen *const pScreen = static_cast<CScreen *const>(this->pParent);
		if (!pScreen->OnQuit())
		{
			if (!pScreen->IsDeactivating())
			{
				RequestPaint();	//might need to redraw the widget after a cancelled quit prompt
				return false;
			}
		}
	}

	this->dwDeactivateValue = TAG_QUIT;
	Deactivate();
	return true;
}

//*****************************************************************************
void CDialogWidget::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT /*dwTagNo*/,         //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Event.
{
	switch (Key.keysym.sym)
	{
		case SDLK_KP_ENTER:
			if ((Key.keysym.mod & KMOD_CTRL) != 0 &&
				this->dwReqTextField) {
				this->dwDeactivateValue = TAG_OK;
				Deactivate();
				return;
			}
		break;

		case SDLK_RETURN:
			if ((Key.keysym.mod & KMOD_CTRL) != 0 &&
				this->dwReqTextField) {
				this->dwDeactivateValue = TAG_OK;
				Deactivate();
				return;
			}

			if ((Key.keysym.mod & KMOD_ALT) == 0) break;
		//NO BREAK

		case SDLK_F10:
			if (this->pParent && this->pParent->GetType() == WT_Screen)
			{
				CScreen *const pScreen = static_cast<CScreen *const>(this->pParent);
				pScreen->ToggleScreenSize();
				RequestPaint();
			}
		break;

		case SDLK_F4:  //Activator will need to look for TAG_QUIT and handle it.
#if defined(__linux__) || defined(__FreeBSD__)
		case SDLK_PAUSE:
#endif
			if (Key.keysym.mod & (KMOD_ALT | KMOD_CTRL))
			{
				OnQuit();
			}
		break;

		case SDLK_ESCAPE:
			this->dwDeactivateValue = TAG_ESCAPE;
			Deactivate();
		return;

		default: break;
	}

	CheckTextBox();
}

//*****************************************************************************
void CDialogWidget::OnTextInput(
//Called when widget receives SDL_TEXTINPUT event.
	const UINT /*dwTagNo*/,     //(in)   Widget that event applied to.
	const SDL_TextInputEvent &) //(in)   Event.
{
	CheckTextBox();
}


//
// Private methods
//

//*****************************************************************************
void CDialogWidget::CheckTextBox()
//When this->dwReqTextField is set to the tag ID of a text box,
//this implies text must be entered.  To enforce this, the OK button is
//enabled only when the text box is non-empty.
{
	CButtonWidget *pOKButton = DYN_CAST(CButtonWidget*, CWidget *,
			GetWidget(TAG_OK));
	if (!pOKButton) return; //OK button is not present or hidden

	if (this->dwReqTextField)
	{
		CTextBoxWidget *pTextBox = DYN_CAST(CTextBoxWidget*, CWidget *,
				GetWidget(this->dwReqTextField));
		ASSERT(pTextBox);

		pOKButton->Enable(WCSlen(pTextBox->GetText()) > 0);
		pOKButton->RequestPaint();
	}
}


//*****************************************************************************
void CDialogWidget::OnWindowEvent_GetFocus()
{
	// As a rule, dialogs appear as part of a screen, and the screen may need to do special handling
	// when focus is restored/lost, so just pass the responsibility to them
	ASSERT(this->pParent);
	CEventHandlerWidget* pParent = FindEventHandlerParent();
	if (pParent)
		pParent->OnWindowEvent_GetFocus();
	else
		CEventHandlerWidget::OnWindowEvent_GetFocus();
}

//*****************************************************************************
void CDialogWidget::OnWindowEvent_LoseFocus()
{
	// See OnWindowEvent_GetFocus() for explanation
	CEventHandlerWidget* pParent = FindEventHandlerParent();
	if (pParent)
		pParent->OnWindowEvent_LoseFocus();
	else
		CEventHandlerWidget::OnWindowEvent_LoseFocus();
}

//*****************************************************************************
CEventHandlerWidget* CDialogWidget::FindEventHandlerParent() const
{
	ASSERT(this->pParent);
	CWidget* pParent = this->pParent;

	while (pParent) {
		CEventHandlerWidget* pEventHandlerParent = dynamic_cast<CEventHandlerWidget*>(pParent);
		if (pEventHandlerParent)
			return pEventHandlerParent;

		pParent = pParent->GetParent();
	}

	ASSERT(!"Dialogs should always exist as a child of CEventHandlerWidget, will fallback!");
	return NULL;
}

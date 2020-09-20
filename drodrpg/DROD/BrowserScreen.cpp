// $Id: BrowserScreen.cpp 8102 2007-08-15 14:55:40Z trick $

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

#include "BrowserScreen.h"
#include "DrodFontManager.h"

#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/HyperLinkWidget.h>

#include "../DRODLib/Db.h"
#include <BackEndLib/Browser.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>

// Widget tags.
const UINT TAG_HTML_AREA = 2000;
const UINT TAG_BACK_BUTTON = 2001;
const UINT TAG_FORWARD_BUTTON = 2002;
const UINT TAG_RETURN_BUTTON = 2003;
const UINT TAG_INDEX_BUTTON = 2004;
const UINT TAG_HELP_STATUS_BAR = 2005;

const UINT TAG_HYPERLINK_START = 10000;

// Help location (added to resource path)
const WCHAR wszHelpPath[] = {We(SLASH),We('H'),We('e'),We('l'),We('p'),We(SLASH),We(0)};
const WCHAR wszIndexPage[] = {We('c'),We('o'),We('n'),We('t'),We('e'),We('n'),We('t'),We('s'),We('.'),We('h'),We('t'),We('m'),We('l'),We(0)};

WSTRING CBrowserScreen::m_wstrPageToLoad;

void CBrowserScreen::SetPageToLoad(const char* wszPageName)
//Specify a page to load on activation.
//A null string indicates a request to load the index page.
{
	if (!wszPageName)
		m_wstrPageToLoad = wszEmpty;
	else
		AsciiToUnicode(wszPageName, m_wstrPageToLoad);
}

//******************************************************************************
CBrowserScreen::CBrowserScreen ()
	: CDrodScreen(SCR_Browser)
{
	// Background image
	imageFilenames.push_back(string("Background"));

	// Title
	static const UINT CX_SPACE = 10;
	static const UINT CY_SPACE = 10;
	static const UINT CX_TITLE = w - 2 * CX_SPACE;
	static const UINT CY_TITLE = 50;
	static const UINT CY_TITLE_SPACE = 12;
	static const int X_TITLE = CX_SPACE;
	static const int Y_TITLE = CY_TITLE_SPACE;

	// Buttons
	static const UINT CX_BACK_BUTTON = 120;
	static const UINT CY_BACK_BUTTON = CY_STANDARD_BUTTON;
	static const UINT CX_FORWARD_BUTTON = CX_BACK_BUTTON;
	static const UINT CY_FORWARD_BUTTON = CY_BACK_BUTTON;
	static const UINT CX_INDEX_BUTTON = CX_BACK_BUTTON;
	static const UINT CY_INDEX_BUTTON = CY_BACK_BUTTON;
	static const UINT CX_RETURN_BUTTON = CX_BACK_BUTTON;
	static const UINT CY_RETURN_BUTTON = CY_BACK_BUTTON;
	static const int X_BACK_BUTTON = CX_SPACE;
	static const int Y_BACK_BUTTON = h - CY_BACK_BUTTON - CY_SPACE;
	static const int X_FORWARD_BUTTON = X_BACK_BUTTON + CX_BACK_BUTTON + 2;
	static const int Y_FORWARD_BUTTON = Y_BACK_BUTTON;
	static const int X_RETURN_BUTTON = w - CX_RETURN_BUTTON - CX_SPACE;
	static const int Y_RETURN_BUTTON = Y_BACK_BUTTON;
	static const int X_INDEX_BUTTON = X_RETURN_BUTTON - CX_INDEX_BUTTON - 2;
	static const int Y_INDEX_BUTTON = Y_BACK_BUTTON;

	// Status bar
	static const UINT CX_STATUS = X_INDEX_BUTTON - X_FORWARD_BUTTON - CX_FORWARD_BUTTON;
	static const UINT CY_STATUS = CY_BACK_BUTTON;
	static const int X_STATUS = X_FORWARD_BUTTON + CX_FORWARD_BUTTON;
	static const int Y_STATUS = Y_BACK_BUTTON + 2;

	// The browser
	static const int X_BROWSER = CX_SPACE;
	static const int Y_BROWSER = CY_TITLE + CY_SPACE + 4;
	static const UINT CX_BROWSER = w - 2 * CX_SPACE;
	static const UINT CY_BROWSER = Y_BACK_BUTTON - Y_BROWSER - CY_SPACE;

	// Widgets
	this->pTitle = new CLabelWidget(0, X_TITLE, Y_TITLE,
			CX_TITLE, CY_TITLE, F_Title, NULL);
	this->pTitle->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(this->pTitle);
	CButtonWidget *pButton = new CButtonWidget(TAG_BACK_BUTTON,
			X_BACK_BUTTON, Y_BACK_BUTTON,
			CX_BACK_BUTTON, CY_BACK_BUTTON,
			g_pTheDB->GetMessageText(MID_Back));
	pButton->Disable();
	AddWidget(pButton);
	pButton = new CButtonWidget(TAG_FORWARD_BUTTON,
			X_FORWARD_BUTTON, Y_FORWARD_BUTTON,
			CX_FORWARD_BUTTON, CY_FORWARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Forward));
	pButton->Disable();
	AddWidget(pButton);
	AddWidget(new CButtonWidget(TAG_INDEX_BUTTON,
			X_INDEX_BUTTON, Y_INDEX_BUTTON,
			CX_INDEX_BUTTON, CY_INDEX_BUTTON,
			g_pTheDB->GetMessageText(MID_Index)));
	AddWidget(new CButtonWidget(TAG_RETURN_BUTTON,
			X_RETURN_BUTTON, Y_RETURN_BUTTON,
			CX_RETURN_BUTTON, CY_RETURN_BUTTON,
			g_pTheDB->GetMessageText(MID_Return)));
	this->pStatus = new CLabelWidget(TAG_HELP_STATUS_BAR,
			X_STATUS, Y_STATUS,
			CX_STATUS, CY_STATUS,
			F_Button, g_pTheDB->GetMessageText(MID_BrowserName));
	this->pStatus->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(this->pStatus);

	//Add scrollable view window.  Set size of HTML widget so container will
	//have a vertical scroll bar and no horizontal one, since most help pages
	//will be long and require vertical scrolling, while horizontal scrolling
	//is annoying.
	this->pBrowser = new CScrollableWidget(0,
		X_BROWSER, Y_BROWSER, CX_BROWSER, CY_BROWSER);
	SDL_Rect rect;
	this->pBrowser->GetRectAvailableForChildren(rect, 0, this->pBrowser->GetH());
	AddWidget(this->pBrowser);

	this->pHTML = new CHTMLWidget(TAG_HTML_AREA, 0, 0, rect.w, rect.h+1, 5,
			wszIndexPage, TAG_HYPERLINK_START);
	this->pHTML->SetStatus(this->pStatus->GetText().c_str());
	this->pBrowser->AddWidget(this->pHTML);
}

//******************************************************************************
CBrowserScreen::~CBrowserScreen()
{
}

//******************************************************************************
bool CBrowserScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	int language = (int)Language::GetLanguage();
	bool bTriedDefault = false;
	bool bSuccess = false;
	while (!bSuccess && !bTriedDefault) {
		//Load a specific (relative) page if requested, else load the designated index page.
		CFiles Files;
		WSTRING wstrPage = Files.GetResPath();
		WCHAR wszIBuf[16];
		wstrPage += wszHelpPath;
		wstrPage += _itoW(language, wszIBuf, 10);
		wstrPage += wszSlash;
		if (m_wstrPageToLoad.size())
			wstrPage += m_wstrPageToLoad;
		else
			wstrPage += wszIndexPage;

		bSuccess = this->pHTML->LoadFile(wstrPage.c_str());

		//If help files for a language other than the default were looked up
		//and failed, try loading the default language.
		bTriedDefault = language == Language::English;
		if (!bTriedDefault)
			language = Language::English;
	}

	return bSuccess;
}

//*****************************************************************************
void CBrowserScreen::OnClick(
//Called when widget receives a click event.
//
//Params:
	const UINT dwTagNo) //(in)   Widget that event applied to.
{
	if (dwTagNo >= TAG_HYPERLINK_START)
	{
		CHyperLinkWidget *pHyperLink = DYN_CAST(CHyperLinkWidget*, CWidget*,
				GetWidget(dwTagNo));
		ASSERTP(pHyperLink != NULL, "Missing hyperlink: GetWidget returned NULL");
		if (pHyperLink->IsExternal())
		{
			string strLink;
			UnicodeToAscii(pHyperLink->GetLink(), strLink);
			SetFullScreen(false);
			OpenExtBrowser(strLink.c_str());
		}
		else
		{
			this->pHTML->LoadFile(pHyperLink->GetLink().c_str());
			RequestPaint();
		}
	} else {
		switch (dwTagNo)
		{
			case TAG_ESCAPE:
			case TAG_RETURN_BUTTON:
				GoToScreen(SCR_Return);
				break;
			case TAG_BACK_BUTTON:
				this->pHTML->GoBack();
				RequestPaint();
				break;
			case TAG_FORWARD_BUTTON:
				this->pHTML->GoForward();
				RequestPaint();
				break;
			case TAG_INDEX_BUTTON:
				this->pHTML->GoHome();
				RequestPaint();
				break;
			default: break;
		}
	}

	UpdateWidgets();
}

//*****************************************************************************
void CBrowserScreen::UpdateWidgets()
{
	//Set back/forward status.
	CButtonWidget *pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_BACK_BUTTON));
	ASSERT(pButton);
	pButton->Enable(this->pHTML->GetBackCount() > 0);
	SelectFirstWidget();
	pButton->RequestPaint();
	pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_FORWARD_BUTTON));
	ASSERT(pButton);
	pButton->Enable(this->pHTML->GetForwardCount() > 0);
	SelectFirstWidget();
	pButton->RequestPaint();
}

//*****************************************************************************
void CBrowserScreen::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,          //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Key event.
{
	CScreen::OnKeyDown(dwTagNo, Key);

	switch (Key.keysym.sym)
	{
		case SDLK_BACKSPACE:
			this->pHTML->GoBack();
			UpdateWidgets();
			break;

		//Scroll browser window.
		case SDLK_UP:  case SDLK_KP8:
			this->pHTML->ScrollUp();
			break;
		case SDLK_HOME:  case SDLK_KP7:
		case SDLK_PAGEUP:    case SDLK_KP9:
			this->pHTML->ScrollUpPage();
			break;
		case SDLK_DOWN:  case SDLK_KP2:
			this->pHTML->ScrollDown();
			break;
		case SDLK_END:  case SDLK_KP1:
		case SDLK_PAGEDOWN:  case SDLK_KP3:
			this->pHTML->ScrollDownPage();
			break;
		default: break;
	}
}

//*****************************************************************************
void CBrowserScreen::OnMouseWheel(
//Called when a mouse wheel event is received.
//
//Params:
	const SDL_MouseButtonEvent &Button)
{
	//Mouse wheel scrolls browser widget (if it doesn't already have focus).
	ASSERT(this->pBrowser);
	if (!this->pBrowser->IsSelected())
	{
		ASSERT(this->pHTML);
		if (Button.button == SDL_BUTTON_WHEELDOWN)
			this->pHTML->ScrollDown();
		else if (Button.button == SDL_BUTTON_WHEELUP)
			this->pHTML->ScrollUp();
	}
}

//******************************************************************************
void CBrowserScreen::Paint(
//Paint the screen.
//
//Params:
	bool bUpdateRect)          //(in)   If true (default) and destination
										//       surface is the screen, the screen
										//       will be immediately updated in
										//       the widget's rect.
{
	//Since browser takes up most of the screen region, don't draw BG over it.
	//This also avoids the browser "flickering" when screen is repainted rapidly.
	ASSERT(this->pBrowser);
	SDL_Rect rect = MAKE_SDL_RECT(0, 0, this->w, this->pBrowser->GetY());
	SDL_BlitSurface(images[0], &rect, GetDestSurface(), &rect);	//top
	rect.y = this->pBrowser->GetY() + this->pBrowser->GetH();
	rect.h = this->h - rect.y;
	SDL_BlitSurface(images[0], &rect, GetDestSurface(), &rect);	//bottom
	rect.y = 0;
	rect.w = this->pBrowser->GetX();
	rect.h = this->h;
	SDL_BlitSurface(images[0], &rect, GetDestSurface(), &rect);	//left
	rect.x = this->pBrowser->GetX() + this->pBrowser->GetW();
	rect.w = this->w - rect.x;
	SDL_BlitSurface(images[0], &rect, GetDestSurface(), &rect);	//right

	this->pTitle->SetText(this->pHTML->GetTitle());
	this->pStatus->SetText(this->pHTML->GetStatus());

	PaintChildren();
	if (bUpdateRect) UpdateRect();
}


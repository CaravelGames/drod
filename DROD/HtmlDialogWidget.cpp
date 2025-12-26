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

#include "HtmlDialogWidget.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"

#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/HyperLinkWidget.h>

#include "../DRODLib/Db.h"
#include <BackEndLib/Browser.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>

 // Widget tags.
const UINT TAG_HTML_AREA = 2000;
const UINT TAG_OK_BUTTON = 2001;

const UINT TAG_HYPERLINK_START = 10000;

// Help location (added to resource path)
const WCHAR wszHelpPath[] = { We(SLASH),We('H'),We('e'),We('l'),We('p'),We(SLASH),We(0) };
const WCHAR wszIndexPage[] = { We('c'),We('o'),We('n'),We('t'),We('e'),We('n'),We('t'),We('s'),We('.'),We('h'),We('t'),We('m'),We('l'),We(0) };

//******************************************************************************
CHtmlDialogWidget::CHtmlDialogWidget(const UINT dwTagNo, const UINT wWidth, const UINT wHeight)
	: CDialogWidget(dwTagNo, 0, 0, wWidth, wHeight)
{
	const UINT DIALOG_W = wWidth;
	const UINT DIALOG_H = wHeight;
	const UINT SPACE_W = 10;
	const UINT SPACE_H = 10;

	// Title
	const int TITLE_X = SPACE_W;
	const int TITLE_Y = SPACE_H;
	const int TITLE_W = DIALOG_W;
	const int TITLE_H = CY_LABEL_FONT_HEADER;
	const int TITLE_BOTTOM = TITLE_Y + TITLE_H + SPACE_H;
	

	// Buttons
	const int OK_BUTTON_W = 128;
	const int OK_BUTTON_H = CY_STANDARD_BUTTON;
	const int OK_BUTTON_X = (DIALOG_W - OK_BUTTON_W) / 2;
	const int OK_BUTTON_Y = DIALOG_H - OK_BUTTON_H - SPACE_H;

	// The browser
	const int BROWSER_X = SPACE_W * 2;
	const int BROWSER_Y = TITLE_BOTTOM;
	const int BROWSER_W = DIALOG_W - 4 * SPACE_W;
	const int BROWSER_H = OK_BUTTON_Y - SPACE_H - TITLE_BOTTOM;

	// Widgets
	{
		WSTRING titleStr(g_pTheDB->GetMessageText(MID_Help));
		titleStr.erase(std::remove(titleStr.begin(), titleStr.end(), '&'), titleStr.end());
		this->pTitle = new CLabelWidget(0,
			TITLE_X, TITLE_Y, TITLE_W, TITLE_H, F_Header, titleStr.c_str());
		this->pTitle->SetAlign(CLabelWidget::TA_CenterGroup);
		AddWidget(this->pTitle);
	}

	{
		CButtonWidget *pButton = new CButtonWidget(TAG_OK_BUTTON,
			OK_BUTTON_X, OK_BUTTON_Y, OK_BUTTON_W, OK_BUTTON_H,
			g_pTheDB->GetMessageText(MID_Close));
		AddWidget(pButton);
	}

	//Add scrollable view window.  Set size of HTML widget so container will
	//have a vertical scroll bar and no horizontal one, since most help pages
	//will be long and require vertical scrolling, while horizontal scrolling
	//is annoying.
	this->pBrowser = new CScrollableWidget(0,
		BROWSER_X, BROWSER_Y, BROWSER_W, BROWSER_H);
	SDL_Rect rect;
	this->pBrowser->GetRectAvailableForChildren(rect, 0, this->pBrowser->GetH());
	AddWidget(this->pBrowser);

	this->pHTML = new CHTMLWidget(TAG_HTML_AREA, 0, 0, rect.w, rect.h + 1, 5,
		wszIndexPage, TAG_HYPERLINK_START);
	this->pBrowser->AddWidget(this->pHTML);
}

//******************************************************************************
CHtmlDialogWidget::~CHtmlDialogWidget()
{
}

//*****************************************************************************
void CHtmlDialogWidget::OnClick(
	//Called when widget receives a click event.
	//
	//Params:
	const UINT dwTagNo) //(in)   Widget that event applied to.
{
	if (dwTagNo >= TAG_HYPERLINK_START)
	{
		CHyperLinkWidget *pHyperLink = DYN_CAST(CHyperLinkWidget *, CWidget *,
			GetWidget(dwTagNo));
		ASSERTP(pHyperLink != NULL, "Missing hyperlink: GetWidget returned NULL");
		if (pHyperLink->IsExternal())
		{
			const string strLink = UnicodeToUTF8(pHyperLink->GetLink());
			OpenExtBrowser(strLink.c_str());
		}
		else
		{
			this->pHTML->LoadFile(pHyperLink->GetLink().c_str());
			RequestPaint();
		}
	}
	else {
		switch (dwTagNo)
		{
		case TAG_ESCAPE:
		case TAG_OK_BUTTON:
			Deactivate();
			break;
		default: break;
		}
	}
}

//*****************************************************************************
void CHtmlDialogWidget::OnKeyDown(
	//Called when widget receives SDL_KEYDOWN event.
	//
	//Params:
	const UINT dwTagNo,          //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Key event.
{
	switch (Key.keysym.sym)
	{
	case SDLK_ESCAPE:
		this->Deactivate();
		break;

		//Scroll browser window.
	case SDLK_UP:  case SDLK_KP_8:
		this->pHTML->ScrollUp();
		break;
	case SDLK_HOME:  case SDLK_KP_7:
	case SDLK_PAGEUP:    case SDLK_KP_9:
		this->pHTML->ScrollUpPage();
		break;
	case SDLK_DOWN:  case SDLK_KP_2:
		this->pHTML->ScrollDown();
		break;
	case SDLK_END:  case SDLK_KP_1:
	case SDLK_PAGEDOWN:  case SDLK_KP_3:
		this->pHTML->ScrollDownPage();
		break;
	default: break;
	}
}

//*****************************************************************************
void CHtmlDialogWidget::OnMouseWheel(
	//Called when a mouse wheel event is received.
	//
	//Params:
	const SDL_MouseWheelEvent &Wheel)
{
	//Mouse wheel scrolls browser widget (if it doesn't already have focus).
	ASSERT(this->pBrowser);
	if (!this->pBrowser->IsSelected())
	{
		ASSERT(this->pHTML);
		if (Wheel.y < 0)
			this->pHTML->ScrollDown();
		else if (Wheel.y > 0)
			this->pHTML->ScrollUp();
	}
}

//******************************************************************************
void CHtmlDialogWidget::SetPageToLoad(const char *wszPageName)
{
	if (!wszPageName)
		this->wstrPageToLoad = wszEmpty;
	else
		UTF8ToUnicode(wszPageName, this->wstrPageToLoad);

	LoadPage();
}

//******************************************************************************
void CHtmlDialogWidget::LoadPage()
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
		if (this->wstrPageToLoad.size())
			wstrPage += this->wstrPageToLoad;
		else
			wstrPage += wszIndexPage;

		bSuccess = this->pHTML->LoadFile(wstrPage.c_str());

		//If help files for a language other than the default were looked up
		//and failed, try loading the default language.
		bTriedDefault = language == Language::English;
		if (!bTriedDefault)
			language = Language::English;
	}
}
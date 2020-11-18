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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996, 
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "RoomScreen.h"
#include "DrodFontManager.h"
#include "MapWidget.h"
#include "RoomWidget.h"
#include "SettingsScreen.h"

#include "../DRODLib/GameConstants.h"
#include "../DRODLib/SettingsKeys.h"
#include "../DRODLib/Db.h"

#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/ListBoxWidget.h>

#include <BackEndLib/Exception.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Browser.h>

#define BG_SURFACE      (0)
#define PARTS_SURFACE   (1)

const string demoDefaultMusic = "Deep Spaces";

static const UINT CX_LEFT_SIGN = 65;
static const UINT CX_MIDDLE_SIGN = 36;
static const UINT CX_RIGHT_SIGN = 68;
static const UINT CX_SIGN = 779;
static const UINT CY_SIGN = 36;
static const int X_LEFT_SIGN_SRC = 1;
static const int X_MIDDLE_SIGN_SRC = 69;
static const int X_RIGHT_SIGN_SRC = 118;
static const int Y_SIGN_SRC = 384;
static const int X_SIGN = 163;
static const int Y_SIGN = 2;
static SDL_Rect LeftSignSource = {X_LEFT_SIGN_SRC, Y_SIGN_SRC, CX_LEFT_SIGN, CY_SIGN};
static SDL_Rect MiddleSignSource = {X_MIDDLE_SIGN_SRC, Y_SIGN_SRC, CX_MIDDLE_SIGN, CY_SIGN};
static SDL_Rect RightSignSource = {X_RIGHT_SIGN_SRC, Y_SIGN_SRC, CX_RIGHT_SIGN, CY_SIGN};

const UINT TAG_NOTICESBOX = 1040;
const UINT TAG_NOTICESNOTICE = 1041;
const UINT TAG_NOTICESDELETE = 1042;
const UINT TAG_NOTICESLIST = 1043;

//*****************************************************************************
CRoomScreen::CRoomScreen(
//Base constructor.
//
//Params:
	const SCREENTYPE eSetType)
	: CDrodScreen(eSetType)
	, pMapWidget(NULL)
	, pScrollLabel(NULL)
	, pNoticesDialog(NULL)
	, pNoticesList(NULL)
	, dwLastNotice(0)
	, bIsScrollVisible(false)
{
	this->imageFilenames.push_back(string("GameScreen"));
	this->imageFilenames.push_back(string("GameScreenParts"));

	this->signColor = Black;

	static const int X_MAP = 15;
	static const int Y_MAP = 578;
	static const UINT CX_MAP = 130;
	static const UINT CY_MAP = 138;
	static const int X_SCROLL_LABEL = 13;
#ifdef RUSSIAN_BUILD
	static const int Y_SCROLL_LABEL = 215;
#else
	static const int Y_SCROLL_LABEL = 211;
#endif
	static const UINT CX_SCROLL_LABEL = 132;
	static const UINT CY_SCROLL_LABEL = 340;

	//Add widgets.
	this->pMapWidget = new CMapWidget(TAG_MAP, X_MAP, Y_MAP, CX_MAP, CY_MAP, NULL);
	if (!this->pMapWidget) throw CException("CRoomScreen: Couldn't allocate resources");
	this->pMapWidget->Disable();
	AddWidget(this->pMapWidget);

	this->pScrollLabel = new CLabelWidget(0L, X_SCROLL_LABEL, Y_SCROLL_LABEL, 
			CX_SCROLL_LABEL, CY_SCROLL_LABEL, F_Scroll, wszEmpty);
	AddWidget(this->pScrollLabel);
	this->pScrollLabel->Hide();
}

//*****************************************************************************
void CRoomScreen::SetMusicStyle(
//Changes the music to match style.  If music is already matching style, nothing
//will happen.
//
//Params:
	WSTRING style, const UINT wMood, const UINT fadeDuration) //[default=3000ms]
{
	if (g_pTheDBM->IsStyleFrozen())
		return; //don't change music if style is not to change

	ASSERT(g_pTheDBM);
	g_pTheDBM->ConvertStyle(style);

	WSTRING wMoodText;
	switch (wMood)
	{
		case SONG_AMBIENT: case SONG_ATTACK: case SONG_PUZZLE: case SONG_EXIT: case SONG_EDITOR:
			UTF8ToUnicode(moodText[wMood], wMoodText);
		break;
		default: ASSERT(!"Invalid style mood"); break;
	}
	style += wMoodText;

	//Fade to next song in list and update play order.
	CFiles f;
	list<WSTRING> songlist;
	if (f.GetGameProfileString(INISection::Songs, style.c_str(), songlist))
	{
		ASSERT(g_pTheSound);
		g_pTheSound->CrossFadeSong(&songlist, fadeDuration);
		f.WriteGameProfileString(INISection::Songs, style.c_str(), songlist);
	} else {
		//Play default music when a graphics style is present, but music is not.
		static WSTRING wstrDemoDefaultMusic;
		UTF8ToUnicode(demoDefaultMusic.c_str(), wstrDemoDefaultMusic);
		if (WCScmp(style.c_str(), wstrDemoDefaultMusic.c_str()) != 0)
			SetMusicStyle(wstrDemoDefaultMusic, wMood, fadeDuration);
	}
}

//*****************************************************************************
void CRoomScreen::SetSignText(
//Set text that appears on sign.
//
//Params:
	const WCHAR *pwczSetText)  //(in)   New text.  NULL will make the sign 
								//    disappear on next paint.
{
	this->wstrSignText = pwczSetText ? pwczSetText : wszEmpty;
}

//*****************************************************************************
void CRoomScreen::PaintBackground()
//Paint background.
{
	ASSERT(this->images[BG_SURFACE]);
	SDL_BlitSurface(this->images[BG_SURFACE], NULL, GetDestSurface(), NULL);
}

//*****************************************************************************
void CRoomScreen::PaintScroll()
//Paint the scroll.
{
	static const int X_SCROLL = 6;
	static const int Y_SCROLL = 187;
	static const UINT CX_SCROLL = 154;
	static const UINT CY_SCROLL = 380;
	static const int X_SRC_SCROLL = 2;
	static const int Y_SRC_SCROLL = 2;
	static SDL_Rect ScreenRect = {X_SCROLL, Y_SCROLL, CX_SCROLL, CY_SCROLL};
	static SDL_Rect ScrollRect = {X_SRC_SCROLL, Y_SRC_SCROLL, CX_SCROLL, CY_SCROLL};

	ASSERT(this->images[BG_SURFACE]);
	ASSERT(this->images[PARTS_SURFACE]);

	SDL_Surface *pDestSurface = GetDestSurface();

	if (this->bIsScrollVisible)
	{
		ASSERT(this->images[PARTS_SURFACE]);
		SDL_BlitSurface(this->images[PARTS_SURFACE], &ScrollRect, 
				pDestSurface, &ScreenRect);
		this->pScrollLabel->Show();
		this->pScrollLabel->Paint();
	}
	else
	{
		ASSERT(this->images[BG_SURFACE]);
		SDL_BlitSurface(this->images[BG_SURFACE], &ScreenRect, 
				pDestSurface, &ScreenRect);
		this->pScrollLabel->Hide();
	}
	
	UpdateRect(ScreenRect);
}

//*****************************************************************************
SDL_Rect& CRoomScreen::GetEntireSignRect() const
{
	static SDL_Rect EntireSign = {X_SIGN, Y_SIGN, CX_SIGN, CY_SIGN};

	return EntireSign;
}

//*****************************************************************************
void CRoomScreen::PaintSign()
//Paint the sign.
{
	SDL_Rect& EntireSign = GetEntireSignRect();

	ASSERT(this->images[BG_SURFACE]);
	ASSERT(this->images[PARTS_SURFACE]);

	SDL_Surface *pDestSurface = GetDestSurface();

	//Blit background over the entire possible area the sign could cover.
	SDL_BlitSurface(this->images[BG_SURFACE], &EntireSign, pDestSurface, &EntireSign);

	//Is there text to display?
	if (this->wstrSignText.size()) //Yes.
	{
		UINT wMiddleCount, wTextWidth, wTextHeight;

		//Figure out how wide it will be.
		g_pTheFM->GetTextWidthHeight(F_Sign, this->wstrSignText.c_str(), wTextWidth, wTextHeight);
		ASSERT(wTextWidth > 0);
			
		//Figure how many middle sign parts will be needed to display the text.
		wMiddleCount = (wTextWidth / CX_MIDDLE_SIGN);
		if (wTextWidth % CX_MIDDLE_SIGN != 0) ++wMiddleCount; //Round up.

		//Is the text too large to fit?
		UINT wSignWidth = CX_LEFT_SIGN + (wMiddleCount * CX_MIDDLE_SIGN) +
				CX_RIGHT_SIGN;
		if (wSignWidth > CX_SIGN)
		{
			//Sign width too large -- truncate sign text to fit.
			wSignWidth = CX_SIGN;
			wMiddleCount = (CX_SIGN - CX_LEFT_SIGN - CX_RIGHT_SIGN) / CX_MIDDLE_SIGN;
		}

		//Blit left part of sign.
		SDL_Rect Dest = MAKE_SDL_RECT(X_SIGN + ((CX_SIGN - wSignWidth) / 2), Y_SIGN, 
				CX_LEFT_SIGN, CY_SIGN);
		Uint32 TransparentColor = SDL_MapRGB(this->images[PARTS_SURFACE]->format, 226, 0, 0);
		SetColorKey(this->images[PARTS_SURFACE], SDL_TRUE, TransparentColor);
		SDL_BlitSurface(this->images[PARTS_SURFACE], &LeftSignSource, pDestSurface, &Dest);
		SetColorKey(this->images[PARTS_SURFACE], 0, TransparentColor);

		//Blit middle parts of sign.
		Dest.x += CX_LEFT_SIGN;
		Dest.w = CX_MIDDLE_SIGN;
		for (UINT wI = 0; wI < wMiddleCount; ++wI)
		{
			SDL_BlitSurface(this->images[PARTS_SURFACE], &MiddleSignSource, pDestSurface, &Dest);
			Dest.x += CX_MIDDLE_SIGN;
		}

		//Blit right part of sign.
		Dest.w = CX_RIGHT_SIGN;
		SetColorKey(this->images[PARTS_SURFACE], SDL_TRUE, TransparentColor);
		SDL_BlitSurface(this->images[PARTS_SURFACE], &RightSignSource, pDestSurface, &Dest);

		//Draw text on sign.
		int xText = X_SIGN + (int(CX_SIGN - wTextWidth) / 2);
		if (xText < X_SIGN + (int)CX_LEFT_SIGN/2)
			xText = X_SIGN + CX_LEFT_SIGN/2;
		int yText = Y_SIGN + ((CY_SIGN - wTextHeight) / 2);
		if (yText < Y_SIGN)
			yText = Y_SIGN;
		yText -= 3;     //kludge -- this font text is normally drawn too low

		//Set color.
		const SDL_Color origColor = g_pTheFM->GetFontColor(F_Sign);
		g_pTheFM->SetFontColor(F_Sign, this->signColor);

		g_pTheFM->DrawTextXY(F_Sign, this->wstrSignText.c_str(), pDestSurface, xText, yText,
				CX_SIGN - CX_LEFT_SIGN);

		g_pTheFM->SetFontColor(F_Sign, origColor);
		SetColorKey(this->images[PARTS_SURFACE], 0, TransparentColor);
	}

	UpdateRect(EntireSign);
}

//*****************************************************************************
SDL_Keycode CRoomScreen::GetKeysymForCommand(const UINT wCommand) const
//Returns: keysym currently set for indicated command
{
	for (std::map<SDL_Keycode,int>::const_iterator it = KeysymToCommandMap.begin(); it != KeysymToCommandMap.end(); ++it)
		if (it->second == (int)wCommand)
			return it->first;

	ASSERT(!"Command not assigned");
	return SDLK_UNKNOWN;
}

//*****************************************************************************
void CRoomScreen::InitKeysymToCommandMap(
//Set the keysym-to-command map with values from player settings that will determine
//which commands correspond to which keys.
//
//Params:
	CDbPackedVars &PlayerSettings)   //(in)   Player settings to load from.
{
	//Clear the map.
	this->KeysymToCommandMap.clear();

	//Check whether default is for desktop or notebook keyboard configuration.
	CFiles Files;
	string strKeyboard;
	UINT wKeyboard = 0;	//default to numpad
	if (Files.GetGameProfileString(INISection::Localization, INIKey::Keyboard, strKeyboard))
	{
		wKeyboard = atoi(strKeyboard.c_str());
		if (wKeyboard > 1)
			wKeyboard = 0; //invalid setting
	}

	static const int commands[InputCommands::DCMD_Count] = {
		CMD_NW, CMD_N, CMD_NE, CMD_W, CMD_WAIT, CMD_E, CMD_SW, CMD_S, CMD_SE,
		CMD_C, CMD_CC,
		CMD_RESTART, CMD_UNDO, CMD_BATTLE_KEY, CMD_EXEC_COMMAND, CMD_CLONE
	};

	//Get values from current player settings.
	for (UINT wIndex = 0; wIndex < InputCommands::DCMD_Count; ++wIndex) {
		const int nKey = PlayerSettings.GetVar(InputCommands::COMMANDNAME_ARRAY[wIndex],
				COMMANDKEY_ARRAY[wKeyboard][wIndex]);
		const bool bInvalidSDL1mapping = nKey >= 128 && nKey <= 323;
		this->KeysymToCommandMap[bInvalidSDL1mapping ? COMMANDKEY_ARRAY[wKeyboard][wIndex] : nKey] = commands[wIndex];
	}
}

//*****************************************************************************
int CRoomScreen::GetCommandForKeysym(const SDL_Keycode& sym) const
{
	std::map<SDL_Keycode,int>::const_iterator it = this->KeysymToCommandMap.find(sym);
	if (it != this->KeysymToCommandMap.end())
		return it->second;

	return CMD_UNSPECIFIED;
}

//*****************************************************************************
void CRoomScreen::AddNoticesDialog()
{
	static const UINT CX_DIALOG = 500;
	static const UINT CY_DIALOG = 510;

	static const int Y_HEADER = 15;
	static const int X_HEADER = 20;
	static const UINT CX_HEADER = CX_DIALOG - 2*X_HEADER;
	static const UINT CY_HEADER = 36;

	static const UINT CX_SPACE = 12;
	static const UINT CY_SPACE = 12;

	static const UINT CY_OK_BUTTON = CY_STANDARD_BUTTON;
	static const int Y_OK_BUTTON = CY_DIALOG - CY_OK_BUTTON - CY_SPACE*2;

	static const UINT CX_GO_BUTTON = 80;
	static const UINT CY_GO_BUTTON = CY_OK_BUTTON;
	static const int X_GO_BUTTON = 100;
	static const int Y_GO_BUTTON = Y_OK_BUTTON;

	static const UINT CX_DELETE_BUTTON = 80;
	static const UINT CY_DELETE_BUTTON = CY_OK_BUTTON;
	static const int X_DELETE_BUTTON = X_GO_BUTTON + CX_GO_BUTTON + CX_SPACE*2;
	static const int Y_DELETE_BUTTON = Y_OK_BUTTON;

	static const UINT CX_OK_BUTTON = 80;
	static const int X_OK_BUTTON = X_DELETE_BUTTON + CX_DELETE_BUTTON + CX_SPACE*2;

	static const int X_NOTICESLIST = X_HEADER;
	static const int Y_NOTICESLIST = Y_HEADER+30;
	static const UINT CX_NOTICESLIST = CX_DIALOG - X_NOTICESLIST*2;
	static const UINT CY_NOTICESLIST = CY_DIALOG - Y_HEADER*2 - CY_SPACE * 4 - CY_OK_BUTTON;

	this->pNoticesDialog = new CDialogWidget(TAG_NOTICESBOX, 0, 0, CX_DIALOG, CY_DIALOG);

	CLabelWidget *pLabel = new CLabelWidget(0L, X_HEADER, Y_HEADER, CX_HEADER,
			CY_HEADER, F_Header, g_pTheDB->GetMessageText(MID_Notices));
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pNoticesDialog->AddWidget(pLabel);

	this->pNoticesList = new CListBoxWidget(TAG_NOTICESLIST, X_NOTICESLIST, Y_NOTICESLIST, CX_NOTICESLIST, CY_NOTICESLIST);
	this->pNoticesDialog->AddWidget(this->pNoticesList);
	// New notices will be added to pNoticesList.

	CButtonWidget *pButton;
	pButton = new CButtonWidget(TAG_NOTICESNOTICE, X_GO_BUTTON, Y_GO_BUTTON,
			CX_GO_BUTTON, CY_GO_BUTTON, g_pTheDB->GetMessageText(MID_Watch)); //!!new text
	this->pNoticesDialog->AddWidget(pButton);

	pButton = new CButtonWidget(TAG_NOTICESDELETE, X_DELETE_BUTTON, Y_DELETE_BUTTON,
			CX_DELETE_BUTTON, CY_DELETE_BUTTON, g_pTheDB->GetMessageText(MID_Delete));
	this->pNoticesDialog->AddWidget(pButton);

	pButton = new CButtonWidget(TAG_OK, X_OK_BUTTON, Y_OK_BUTTON,
			CX_OK_BUTTON, CY_OK_BUTTON, g_pTheDB->GetMessageText(MID_Okay));
	this->pNoticesDialog->AddWidget(pButton);
	
	AddWidget(this->pNoticesDialog,true);
	this->pNoticesDialog->Center();
	this->pNoticesDialog->Hide();
}

//*****************************************************************************
void CRoomScreen::OpenNoticesBox(CRoomWidget* pRoomWidget)
{
	this->pNoticesDialog->SetBetweenEventsHandler(GetEventHandlerWidget()); //keep updating room effects
	UINT returnTag = 0;
	do {
		if (returnTag == TAG_NOTICESDELETE) {
			UINT selected = this->pNoticesList->GetSelectedItem();
			this->pNoticesList->RemoveItem(selected);
			this->notices.erase(this->notices.find(selected));
		}
		returnTag = this->pNoticesDialog->Display(false);
		this->UpdateNoticesButton();

	} while (returnTag == TAG_NOTICESDELETE);

	switch (returnTag) {
	case TAG_NOTICESNOTICE:
		// This button should go to whatever the notice was about. 
		UINT selected = this->pNoticesList->GetSelectedItem();
		std::map<UINT, CNetNotice>::const_iterator item = this->notices.find(selected);
		if (item != this->notices.end()) {
			// Found it!
			switch (item->second.type) {
			case NOTICE_NEWHOLD:
			case NOTICE_UPDATEDHOLD:
				// go to the hold select screen so the user can update it
				g_pTheNet->SetDownloadHold(item->second.dwServerHoldId);
				GoToScreen(SCR_HoldSelect);
				break;

			case NOTICE_GENERICURL:
				// open a web browser to the specified URL
				OpenExtBrowser(item->second.url.c_str());
				break;

			case NOTICE_ROOMSPECIFIC:
				// Go to the hold/level/room specified.  
				break;
			}
		}
		break;
	}

	this->pNoticesDialog->SetBetweenEventsHandler(NULL);

	pRoomWidget->DirtyRoom();
	pRoomWidget->Paint();
}

//*****************************************************************************
void CRoomScreen::UpdateNoticesButton()
{
	const int num = this->notices.size();
	CButtonWidget* pButton = dynamic_cast<CButtonWidget*>(this->GetWidget(TAG_OPEN_NOTICES));
	string label;
	WSTRING wLabel;
	if (num >= 10) {
		label = "...";
	}
	else {
		char buffer[5];
		sprintf(buffer, "%d", num);
		label = buffer;
	}
	UTF8ToUnicode(label, wLabel);
	pButton->SetCaption(wLabel.c_str());
	pButton->RequestPaint();
}

//*****************************************************************************
void CRoomScreen::GrabNewNotices(CRoomWidget* pRoomWidget)
{
	vector<CNetNotice> notices;
	g_pTheNet->QueryNotices(notices, NOTICE_ALL, this->dwLastNotice);
	for (vector<CNetNotice>::const_iterator i = notices.begin(); i != notices.end(); i++) {
		this->dwLastNotice = i->id;
		CCaravelNetNoticeEffect *pEffect = new CCaravelNetNoticeEffect(pRoomWidget, *i, pRoomWidget->notices);
		pRoomWidget->AddLastLayerEffect(pEffect);
		this->pNoticesList->AddItem(i->id, i->text.c_str());
		this->notices[i->id] = *i;

	}
	this->UpdateNoticesButton();
}

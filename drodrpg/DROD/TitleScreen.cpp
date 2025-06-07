// $Id: TitleScreen.cpp 10126 2012-04-24 05:40:08Z mrimer $

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
 *
 * ***** END LICENSE BLOCK ***** */

#include "TitleScreen.h"
#include "BrowserScreen.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"
#include "DrodSound.h"
#include "DemoScreen.h"
#include "BloodEffect.h"

#include <FrontEndLib/ImageWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/HyperLinkWidget.h>
#include <FrontEndLib/MenuWidget.h>

#include "../Texts/MIDs.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/SettingsKeys.h"

#include <BackEndLib/Browser.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Internet.h>
#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/Ports.h>

#include <math.h>

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#define TITLE_BACKGROUND (0)
#define LIGHT_MASK       (4)
#define TITLE_SHADOW     (5)

const UINT TAG_MENU = 1000;
const UINT TAG_PLAYMENU = 1001;
const UINT TAG_TITLE_IMG = 1002;
const UINT TAG_INTERNET_ICON = 1010;
const UINT TAG_CARAVEL_LOGO_SW = 1012;
const UINT TAG_HYPERLINK_START = 10000;

const UINT dwDisplayDuration = 60000;  //ms

const WCHAR wszTitle[] = {
	We('T'),We('i'),We('t'),We('l'),We('e'),We('D'),We('R'),We('O'),We('D'),We(0)
};

const WCHAR wszCaravelLogo[] = {
	We('C'),We('a'),We('r'),We('a'),We('v'),We('e'),We('l'),We('L'),We('o'),We('g'),We('o'),We('S'),We('W'),We(0)
};

//Internet connection state icons.
const WCHAR wszSignalNo[] = {We('S'),We('i'),We('g'),We('n'),We('a'),We('l'),We('N'),We('o'),We(0)};
const WCHAR wszSignalYes[] = {We('S'),We('i'),We('g'),We('n'),We('a'),We('l'),We('Y'),We('e'),We('s'),We(0)};
const WCHAR wszSignalBad[] = {We('S'),We('i'),We('g'),We('n'),We('a'),We('l'),We('B'),We('a'),We('d'),We(0)};
const WCHAR wszSignalGood[] = {We('S'),We('i'),We('g'),We('n'),We('a'),We('l'),We('G'),We('o'),We('o'),We('d'),We(0)};

const int X_TITLE = 280; //centered: ((CScreen::CX_SCREEN - 460) / 2)
const int Y_TITLE = 110;

const int X_TITLE_SHADOW = X_TITLE - 30;
const int Y_TITLE_SHADOW = Y_TITLE + 5;

const float fDarkFactor = 0.20f;

#define PI (3.1415926535f)
#define TWOPI (2.0f * PI)

enum VerminTypes {
	MV_ROACH,
	MV_WW,
	MV_EYE,
	VERMIN_TYPES
};

//const int CX_CARAVEL_LOGO = 132;
const int CY_CARAVEL_LOGO = 132;

//
//Protected methods.
//

//******************************************************************************
CTitleScreen::CTitleScreen() : CDrodScreen(SCR_Title)
	, verminEffects(NULL)
	, bExtraCritters(false), critterKills(0), bBackwards(false)
	, bReloadDemos(true)
	, dwNonTutorialHoldID(0)
	, pMenu(NULL), pPlayMenu(NULL)
	, dwFirstPaint(0)
	, bSavedGameExists(false)
	, wNewsHandle(0)
	, pMarqueeWidget(NULL)
	, bWaitingForHoldlist(false)
	, bPredarken(true), bReloadGraphics(false)
	, bNewGamePrompt(true)
//Constructor.
{
	//Load image assets
	this->imageFilenames.push_back(string("TitleBG"));
	this->imageFilenames.push_back(string("TitleBG1"));
	this->imageFilenames.push_back(string("TitleBG2"));
	this->imageFilenames.push_back(string("TitleBGTunnel"));
	this->imageFilenames.push_back(string("TitleLightMask"));
	this->imageFilenames.push_back(string("TitleShadow"));

	//Game title logo.
	AddWidget(new CImageWidget(TAG_TITLE_IMG, X_TITLE, Y_TITLE, wszTitle));

	//Caravel Games logo
	AddWidget(new CImageWidget(TAG_CARAVEL_LOGO_SW, 0, CScreen::CY_SCREEN - CY_CARAVEL_LOGO, wszCaravelLogo));

	g_pTheDBM->LoadGeneralTileImages();

	this->currentDemo = this->ShowSequenceDemoIDs.end();

	const bool bDemo = !IsGameFullVersion();

	//Option menu position based on BG image.
	const int CX_MENU = 355;
	const int CY_MENU = 360;
	const int X_MENU = GetMenuXPosition(CX_MENU); //lower center, right
	const int Y_MENU = 340;

	this->pMenu = new CMenuWidget(TAG_MENU, X_MENU, Y_MENU, CX_MENU, CY_MENU,
			F_TitleMenu, F_TitleMenuActive, F_TitleMenuSelected);
	AddWidget(this->pMenu);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitlePlayMenu), MNU_PLAYMENU);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitlePlayer), MNU_WHO);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitleSettings), MNU_SETTINGS);
//	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitleDemo), MNU_DEMO);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitleBuild), MNU_BUILD);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitleHelp), MNU_HELP);
	if (bDemo)
		this->pMenu->AddText(g_pTheDB->GetMessageText(MID_BuyNow), MNU_BUY);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitleQuit), MNU_QUIT);

	//Play sub-menu.
	this->pPlayMenu = new CMenuWidget(TAG_PLAYMENU, X_MENU, Y_MENU, CX_MENU, CY_MENU,
			F_TitleMenu, F_TitleMenuActive, F_TitleMenuSelected);
	AddWidget(this->pPlayMenu);
	//this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleTutorial), MNU_TUTORIAL);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleNewGame), MNU_NEWGAME);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleContinue), MNU_CONTINUE);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleRestore), MNU_RESTORE);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleWhere), MNU_WHERE);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleChat), MNU_CHAT);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleMainMenu), MNU_MAINMENU);
	this->pPlayMenu->Hide();

	//Draw version number in top right corner.
	UINT wVersionW, wVersionH;
	g_pTheFM->GetTextWidthHeight(F_TitleMarquee, wszVersionReleaseNumber, wVersionW, wVersionH);
	static const UINT CY_VERSION_LABEL = wVersionH;
	static const int X_VERSION_LABEL = CX_SCREEN - wVersionW - 8;
	static const int Y_VERSION_LABEL = 0;
   AddWidget(new CLabelWidget(0, X_VERSION_LABEL, Y_VERSION_LABEL,
         wVersionW, CY_VERSION_LABEL, F_TitleMarquee, wszVersionReleaseNumber, true, 0, WT_Label, true));

	//Internet connection status icon.
	static const UINT CX_INTERNET_ICON = 32;
	static const int X_INTERNET_ICON = CX_SCREEN - CX_INTERNET_ICON - 8;
	static const int Y_INTERNET_ICON = Y_VERSION_LABEL + CY_VERSION_LABEL + 9;
	CImageWidget *pInternetIcon = new CImageWidget(TAG_INTERNET_ICON,
			X_INTERNET_ICON, Y_INTERNET_ICON, wszSignalNo);
	pInternetIcon->SetAlpha(150);
	AddWidget(pInternetIcon);

	//Scroll text in marquee in bottom center.
	static const UINT MARQUEE_MARGIN = 0;
	static const UINT CX_MARQUEE = CX_SCREEN - 2*MARQUEE_MARGIN - 133;
	static const int X_MARQUEE = MARQUEE_MARGIN + 133;
	static const UINT CY_MARQUEE = 30;
	static const int Y_MARQUEE = CY_SCREEN - CY_MARQUEE - 10;
	this->pMarqueeWidget = new CMarqueeWidget(0, X_MARQUEE, Y_MARQUEE,
			CX_MARQUEE, CY_MARQUEE, 10);
	AddWidget(this->pMarqueeWidget);

	SetTitleScreenSkin();
}

//******************************************************************************
bool CTitleScreen::IsRPG1BG() const
{
	return this->imageNum >= 0 && this->imageNum <= 2;
}

//******************************************************************************
int CTitleScreen::GetMenuXPosition(const int width) const
{
	return IsRPG1BG() ? (CScreen::CX_SCREEN - width) / 2 : 680; //lower center, right;
}

//******************************************************************************
void CTitleScreen::LoadDemos()
//Load demo show sequence for currently selected hold.
{
	if (!this->bReloadDemos)
		this->bReloadDemos = true; //reload them next time by default, but not this time
	else
	{
		//Get all show demos in the active hold.
		this->ShowSequenceDemoIDs.clear();
		CDb db;
		db.Demos.FilterByShow();
		CIDSet demoIDs = db.Demos.GetIDs();
		const UINT activeHoldID = g_pTheDB->GetHoldID();
		for (CIDSet::const_iterator demo = demoIDs.begin(); demo != demoIDs.end(); ++demo)
		{
			const UINT dwDemoHoldID = CDb::getHoldOfDemo(*demo);
			if (dwDemoHoldID == activeHoldID)
				this->ShowSequenceDemoIDs += *demo;
		}
		this->currentDemo = this->ShowSequenceDemoIDs.end();
	}
}

//******************************************************************************
bool CTitleScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	if (UnloadOnDeactivate())
		this->bPredarken = true; //need to redarken background if assets are being reloaded
	this->bReloadGraphics = false;

	SetCursor(CUR_Wait);

	//There ought to always be an active player at the title screen.
	const UINT dwPlayerID = g_pTheDB->GetPlayerID();
	ASSERT(dwPlayerID);

	//If the tutorial was just played, then select the previous non-tutorial hold.
	if (this->dwNonTutorialHoldID)
	{
		//If a tutorial game was being played, unload it.
		CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
				g_pTheSM->GetScreen(SCR_Game));
		ASSERT(pGameScreen);
		if (pGameScreen->IsGameLoaded())
			pGameScreen->UnloadGame();

		g_pTheDB->SetHoldID(this->dwNonTutorialHoldID);
		this->dwNonTutorialHoldID = 0;
	}

	SetTitleScreenSkin();

	LoadDemos();

	const CDbPackedVars settings = g_pTheDB->GetCurrentPlayerSettings();
	g_pTheSound->bNoFocusPlaysMusic = settings.GetVar(Settings::NoFocusPlaysMusic, false);

	this->bNewGamePrompt = settings.GetVar(Settings::NewGamePrompt, true);

	ASSERT(!this->bWaitingForHoldlist); //no previous transaction should be left uncompleted
	if (g_pTheNet->IsEnabled()) //if CaravelNet connection hasn't been disabled
	{
		CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
		if (pPlayer)
		{
			//If net connect is set, get news text.
			if (pPlayer->Settings.GetVar(useInternetStr, false))
			{
				if (this->wstrNewsText.empty())	//query news text only once
					RequestNews();
			}

			delete pPlayer;
		}
	}
	SetMenuOptionStatus();

	g_pTheSound->PlaySong(IsRPG1BG() ? SONGID_INTRO : SONGID_TITLE_2);

	SetCursor();

	this->dwFirstPaint = SDL_GetTicks();
	this->pMenu->ResetSelection();
	this->pPlayMenu->ResetSelection();

   return CDrodScreen::SetForActivate();
}

//
//Private methods.
//

//*****************************************************************************
void CTitleScreen::SetTitleScreenSkin()
{
	this->hold_status = GetHoldStatus();
	if (!CDbHold::IsOfficialHold(this->hold_status)) {
		//Check CaravelNet data to determine which game version the hold is from.
		const UINT holdID = g_pTheDB->GetHoldID();
		if (holdID) {
			CDbHold* pHold = g_pTheDB->Holds.GetByID(holdID, true);
			if (pHold) {
				vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();
				const int nIndex = g_pTheNet->getIndexForName((const WCHAR*)pHold->NameText);
				if (nIndex >= 0) {
					CNetMedia* pHoldData = cNetMedia[nIndex];
					const UINT version = pHoldData->wVersion;
					if (version < 500)
						this->hold_status = CDbHold::Tendry;
					else if (version < 600)
						this->hold_status = CDbHold::ACR;
				}
				delete pHold;
			}
		}

		//To provide consistency w/o CaravelNet, set skin based on the newest installed official hold.
		if (!CDbHold::IsOfficialHold(this->hold_status)) {
			this->hold_status = CDbHolds::GetNewestInstalledOfficialHoldStatus();
		}
	}

	switch (this->hold_status) {
		case CDbHold::Tendry:
			this->imageNum = rand() % 3;
		break;
		case CDbHold::ACR:
		default:
			this->imageNum = 3;
	}

	this->backgroundIndex = TITLE_BACKGROUND + this->imageNum;
	CWidget* titleImage = GetWidget(TAG_TITLE_IMG);
	ASSERT(titleImage);

	//Manage distinct screen assets with skittering roaches when showing a DROD RPG 1 background image
	if (IsRPG1BG()) {
		titleImage->Show();
		string str;
		this->bExtraCritters = CFiles::GetGameProfileString(INISection::Startup, "ExtraCritters", str);

		time_t t = time(NULL);
		tm* pLocalTime = localtime(&t);
		if (pLocalTime->tm_mon == 3 && pLocalTime->tm_mday == 1)
			this->bBackwards = true; //critters move backwards
	} else {
		titleImage->Hide();
	}

	CWidget* mainMenu = GetWidget(TAG_MENU);
	ASSERT(mainMenu);
	mainMenu->Move(GetMenuXPosition(mainMenu->GetW()), mainMenu->GetY());

	CWidget* playMenu = GetWidget(TAG_PLAYMENU);
	ASSERT(playMenu);
	playMenu->Move(GetMenuXPosition(playMenu->GetW()), playMenu->GetY());
}

//*****************************************************************************
void CTitleScreen::OnBetweenEvents()
//Handle events
{
	Animate();

	PollForNews();

	PollForHoldList();

	ImportQueuedFiles(); //import any files specified for import now

	CDrodScreen::OnBetweenEvents();

	//Show tool tip for highlighted menu option.
	if (this->pMenu->IsVisible())
		switch (this->pMenu->GetOnOption())
		{
			case MNU_SETTINGS: RequestToolTip(MID_SettingsTip); break;
			case MNU_HELP: RequestToolTip(MID_HelpTip); break;
			case MNU_DEMO: RequestToolTip(MID_DemoTip); break;
			case MNU_QUIT: RequestToolTip(MID_QuitTip); break;
			case MNU_BUILD: RequestToolTip(MID_BuildTip); break;
			case MNU_WHO: RequestToolTip(MID_WhoTip); break;
			case MNU_PLAYMENU: RequestToolTip(MID_PlayMenuTip); break;
			case MNU_BUY: RequestToolTip(MID_BuyNowTip); break;
			default: break;
		}
	else
		switch (this->pPlayMenu->GetOnOption())
		{
			//case MNU_TUTORIAL: RequestToolTip(MID_TutorialTip); break;
			case MNU_NEWGAME: RequestToolTip(MID_NewGameTip); break;
			case MNU_CONTINUE: RequestToolTip(MID_ContinueTip); break;
			case MNU_RESTORE: RequestToolTip(MID_RestoreTip); break;
			case MNU_WHERE: RequestToolTip(MID_WhereTip); break;
			case MNU_CHAT: RequestToolTip(MID_ChatTip); break;
			case MNU_MAINMENU: RequestToolTip(MID_MainMenuTip); break;
			default: break;
		}

	//Go to demo screen after a map BG cycle.
	if (SDL_GetTicks() - this->dwFirstPaint > dwDisplayDuration)
	{
		const UINT dwDemoID = GetNextDemoID();
		if (dwDemoID)
		{
			CDemoScreen *pDemoScreen = DYN_CAST(CDemoScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_Demo));
			if (!pDemoScreen || !pDemoScreen->LoadDemoGame(dwDemoID))
			{
				ShowOkMessage(MID_LoadGameFailed);
				this->dwFirstPaint = SDL_GetTicks();
			} else {
				this->bReloadDemos = false;
				pDemoScreen->SetReplayOptions(false);
				GoToScreen(SCR_Demo);
			}
		}
	}
}

//*****************************************************************************
void CTitleScreen::OnClick(
//Called when widget receives a click event.
//
//Params:
	const UINT dwTagNo) //(in)   Widget that event applied to.
{
	if (dwTagNo >= TAG_HYPERLINK_START)
	{
		CHyperLinkWidget *pHyperLink = DYN_CAST(CHyperLinkWidget*, CWidget*,
				this->pMarqueeWidget->GetWidgetPart(dwTagNo));
		ASSERTP(pHyperLink != NULL, "Missing hyperlink: GetWidget returned NULL");
		ASSERT(pHyperLink->IsExternal());

		const string strLink = UnicodeToUTF8(pHyperLink->GetLink());
		SetFullScreen(false);
		OpenExtBrowser(strLink.c_str());
	}
}

//*****************************************************************************
void CTitleScreen::OnDeactivate()
{
	//Must wait for pending transactions to complete before exiting.
	WCHAR temp[16];
	UINT dwStart = SDL_GetTicks();
	while (!PollForNews())
	{
		//News has not come in yet.  Wait until it does.
		SetCursor(CUR_Internet);

		//Show seconds that have passed.
		const UINT wSeconds = (SDL_GetTicks() - dwStart) / 1000;
		if (wSeconds) //don't show for first second to minimize distraction
		{
			WSTRING wstr = _itoW(wSeconds, temp, 10);
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_TryingToConnectWithDisconnectPrompt);
			CScreen::ShowStatusMessage(wstr.c_str());
		}

		SDL_Delay(20);
		if (PollForCNetInterrupt())
		{
			//User has opted to disconnect from CaravelNet.
			g_pTheNet->Disable();
			this->wNewsHandle = 0; //ignore response
			break;
		}
	}
	dwStart = SDL_GetTicks();
	while (!PollForHoldList())
	{
		SetCursor(CUR_Internet);

		//Show seconds that have passed.
		const UINT wSeconds = (SDL_GetTicks() - dwStart) / 1000;
		if (wSeconds) //don't show for first second to minimize distraction
		{
			WSTRING wstr = _itoW(wSeconds, temp, 10);
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_TryingToConnectWithDisconnectPrompt);
			CScreen::ShowStatusMessage(wstr.c_str());
		}

		SDL_Delay(20);
		if (PollForCNetInterrupt())
		{
			g_pTheNet->Disable();
			this->bWaitingForHoldlist = false; //ignore response
			break;
		}
	}

	HideStatusMessage();
	SetCursor();
}

//*****************************************************************************
void CTitleScreen::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT /*dwTagNo*/,         //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Key event.
{
	TitleSelection wSetPos;
	switch (Key.keysym.sym)
	{
		case SDLK_RETURN:
			if (!(Key.keysym.mod & KMOD_ALT))
			{
				wSetPos = MNU_CONTINUE; break;
			}
			//else going to next case
		case SDLK_F10:
			ToggleScreenSize();
		return;
		case SDLK_F11:
			SaveSurface();
		return;

		case SDLK_F4:
#if defined(__linux__) || defined(__FreeBSD__)
		case SDLK_PAUSE:
#endif
			if (Key.keysym.mod & (KMOD_ALT | KMOD_CTRL))
				GoToScreen(SCR_None);   //boss key -- exit immediately
		return;

		case SDLK_ESCAPE:
			//Pressing ESC will quit without confirmation dialog from the main menu.
			if (this->pMenu->IsVisible())
				GoToScreen(SCR_None);
			else {
				//Just go back to the main menu.
				this->pMenu->Show();
				this->pPlayMenu->Hide();
				SelectFirstWidget(false);
				Paint();
			}
		return;

		//Menu selections.
		case SDLK_F1: wSetPos = MNU_HELP; break;

		default:
		return;
	}

	//One of the menu items was chosen.
	SCREENTYPE eNextScreen = ProcessMenuSelection(wSetPos);
	if (eNextScreen != SCR_Title)
		GoToScreen(eNextScreen);

	//Delay showing a demo when the user is doing things.
	this->dwFirstPaint = SDL_GetTicks();
}

//*****************************************************************************
void CTitleScreen::OnMouseDown(const UINT /*dwTagNo*/, const SDL_MouseButtonEvent &Button)
{
	clickParticles(Button.x, Button.y);
}

//*****************************************************************************
void CTitleScreen::OnMouseMotion(
//Handle unhighlighting menu options when mouse is not over menu widget.
//
//Params:
	const UINT dwTagNo,
	const SDL_MouseMotionEvent &MotionEvent)
{
	if (dwTagNo != TAG_MENU && dwTagNo != TAG_PLAYMENU)
	{
		if (this->pMenu->IsVisible())
			this->pMenu->ResetSelection();
		else
			this->pPlayMenu->ResetSelection();
	}
	CDrodScreen::OnMouseMotion(dwTagNo, MotionEvent);

	//Delay showing a demo when the user is doing things.
	if (MotionEvent.xrel || MotionEvent.yrel)
		this->dwFirstPaint = SDL_GetTicks();
}

//*****************************************************************************
void CTitleScreen::OnSelectChange(const UINT dwTagNo)
//Called when a menu selection has occurred.
{
	if (IsDeactivating()) return;

	TitleSelection wSetPos;
	switch (dwTagNo)
	{
		case TAG_MENU:
			wSetPos = (TitleSelection)this->pMenu->GetSelectedOption();
		break;
		case TAG_PLAYMENU:
			wSetPos = (TitleSelection)this->pPlayMenu->GetSelectedOption();
		break;
		default : return;
	}

	const SCREENTYPE eNextScreen = ProcessMenuSelection(wSetPos);
	if (eNextScreen != SCR_Title)
		GoToScreen(eNextScreen);
}

//*****************************************************************************
void CTitleScreen::Paint(
//Paint the whole screen.
//
//Params:
	bool bUpdateRect)          //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//Preprocess: darken border surface.
	if (this->bPredarken)
	{
		//Use darker screen if high quality graphics are set.
		const float fValue = g_pTheBM->bAlpha || g_pTheBM->eyeCandy ? fDarkFactor : 0.75f;
		g_pTheBM->DarkenRect(0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN,
			fValue, this->images[TITLE_BACKGROUND]);
		g_pTheBM->DarkenRect(0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN,
			fValue, this->images[TITLE_BACKGROUND + 1]);
		g_pTheBM->DarkenRect(0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN,
			fValue, this->images[TITLE_BACKGROUND + 2]);

		this->bPredarken = false;
	}

	//Draw background.
	g_pTheBM->BlitSurface(this->images[this->backgroundIndex], NULL, GetDestSurface(), NULL);

	//Draw the screen.
	RedrawScreen(bUpdateRect);
}

//*****************************************************************************
bool CTitleScreen::PollForHoldList()
//If requested, waits for CaravelNet hold list to become available
//as an indication of the user being logged in.
//
//Returns: true if ready/done, false if still waiting
{
	if (!this->bWaitingForHoldlist)
		return true;

	//If we don't have a connection, do nothing here.
	if (g_pTheNet->Busy() || !g_pTheNet->IsEnabled())
		return false;

	//Ready to query hold list.
	this->bWaitingForHoldlist = false;

	//Now check for a downloaded hold list as an indication of being logged in.
	CImageWidget *pInternetIcon = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_INTERNET_ICON));
	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();
	if (!cNetMedia.empty())
	{
		pInternetIcon->SetImage(wszSignalGood);
		this->pPlayMenu->Enable(MNU_CHAT, true);
		
		if (pPlayer)
		{
			//Prompt the user if the player has unsent scores for holds that might be on CaravelNet.
			const CIDSet& holdIds = g_pTheNet->GetLocalHolds();
			if (!holdIds.empty() && pPlayer->Settings.GetVar(playerScoresOld, false) != 0)
			{
				CLabelWidget *pText = new CLabelWidget(0, 0, 0, 0, 50, F_TitleMarquee, wszEmpty, false, 0, WT_Label, true);
				pText->SetText(g_pTheDB->GetMessageText(MID_UnsentCaravelNetProgress),
						false, 0, true);
				this->pMarqueeWidget->AddPart(pText);

				pText = new CLabelWidget(0, 0, 0, 100, 50, F_TitleMarquee, wszEmpty, false, 0, WT_Label, true);
				this->pMarqueeWidget->AddPart(pText);
				this->pMarqueeWidget->Reset();
			}
		}
	} else {
		//Determine whether player is a registered CaravelNet user.
		if (pPlayer)
		{
			//Since news feed was downloaded, this mean player has internet connectivity set.
			//So, if a username and key are set, that means they are invalid
			//since the CaravelNet hold list is empty.
			if (WCSlen((const WCHAR*)pPlayer->CNetNameText) &&
					WCSlen((const WCHAR*)pPlayer->CNetPasswordText))
			{
				pInternetIcon->SetImage(wszSignalBad);

				//Ambient text prompt to update password.
				CLabelWidget *pText = new CLabelWidget(0, 0, 0, 0, 50, F_TitleMarquee, wszEmpty, false, 0, WT_Label, true);
				pText->SetText(g_pTheDB->GetMessageText(MID_InvalidCaravelNetKey),
						false, 0, true);
				this->pMarqueeWidget->AddPart(pText);

				pText = new CLabelWidget(0, 0, 0, 100, 50, F_TitleMarquee, wszEmpty, false, 0, WT_Label, true);
				this->pMarqueeWidget->AddPart(pText);
				this->pMarqueeWidget->Reset();
			}
		}
	}
	delete pPlayer;

	return true;
}

//*****************************************************************************
bool CTitleScreen::PollForNews()
//Query for news feed from CaravelNet.
//
//Returns: true if ready/done, false if still waiting
{
	if (!this->wNewsHandle)
		return true;

	const int nStatus = CInternet::GetStatus(this->wNewsHandle);
	if (nStatus < 0)
		return false;

	CImageWidget *pInternetIcon = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_INTERNET_ICON));
	const UINT wHandle = this->wNewsHandle;
	this->wNewsHandle = 0;
	CStretchyBuffer *pBuffer = CInternet::GetResults(wHandle);

	if (!pBuffer) {
		//News request did not return successfully.
		pInternetIcon->SetImage(wszSignalNo);

		//Check for special problems.
		if (nStatus == CURLE_OPERATION_TIMEDOUT)
		{
			//Not connecting.  Maybe no Internet connection or server is down?

			//Try to get news again.  Disable server requests until news is received.
			g_pTheNet->Disable();
			RequestNews();
			return false;  //still waiting for request
		}
	} else {
		//There is an Internet connection.
		pInternetIcon->SetImage(wszSignalYes);
		g_pTheNet->Enable();

		//Update news text.
		(*pBuffer) += (UINT)0;  //null terminate
		const char *pszFromWeb = (char*)(BYTE*)*pBuffer;
 		ASSERT(pszFromWeb);

		//Check whether news is new.
		string strFromWeb;
		if (!pszFromWeb)
			strFromWeb = "Couldn't get news";
		else
		{
			strFromWeb = pszFromWeb;
			string str;
			CFiles f;
			f.GetGameProfileString(INISection::Startup, "LastNews", str);
			if (strFromWeb.size() && str.compare(strFromWeb.c_str()) != 0)
			{
				//New news!
				f.WriteGameProfileString(INISection::Startup, "LastNews", strFromWeb.c_str());
				g_pTheSound->PlaySoundEffect(SEID_WISP);
			}
		}

		delete pBuffer;

		UTF8ToUnicode(strFromWeb.c_str(), this->wstrNewsText);
		SetNewsText();

		//Wait to query hold list until no delay will be incurred.
		this->bWaitingForHoldlist = true;
	}
	return true;
}

//*****************************************************************************
SCREENTYPE CTitleScreen::ProcessMenuSelection(
//Processes menu selection.  No UI-related performed here.
//
//Params:
	TitleSelection wMenuPos)   //(in) One of the MNU_* constants.
//
//Returns:
//Screen to go to next or SCR_Title to remain at title screen.
{
	switch (wMenuPos)
	{
		case MNU_MAINMENU:
			this->pMenu->Show();
			this->pPlayMenu->Hide();
			SelectFirstWidget(false);
		break;

		case MNU_PLAYMENU:
			this->pMenu->Hide();
			this->pPlayMenu->Show();
			SelectFirstWidget(false);
		break;

		case MNU_TUTORIAL:
		{
			//Select tutorial hold here.
			this->dwNonTutorialHoldID = g_pTheDB->GetHoldID();

			//If a tutorial game was being played, unload it before changing hold ID.
			CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_Game));
			ASSERT(pGameScreen);
			if (pGameScreen->IsGameLoaded())
				pGameScreen->UnloadGame();

			//Select tutorial hold ID.
			g_pTheDB->SetHoldID(g_pTheDB->Holds.GetHoldIDWithStatus(CDbHold::Tutorial));
		}
		//NO BREAK

		case MNU_NEWGAME:
		{
			const UINT dwCurrentHoldID = g_pTheDB->GetHoldID();
			if (!dwCurrentHoldID) return SCR_Title;
			CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_Game));
			ASSERT(pGameScreen);
			const UINT dwContinueID = g_pTheDB->SavedGames.FindByContinue();
			if (pGameScreen->IsGameLoaded() || dwContinueID) {
				if (!ConfirmNewGame()) return SCR_Title;
				pGameScreen->UnloadGame();
			}
			if (!pGameScreen->LoadNewGame(dwCurrentHoldID))
			{
				ShowOkMessage(MID_LoadGameFailed);
				return SCR_Title;
			}
			ASSERT(pGameScreen->IsGameLoaded());

			this->bReloadDemos = false;
			const bool bShow = pGameScreen->ShouldShowLevelStart();
			pGameScreen->MarkCurrentEntranceExplored();
			return bShow ? SCR_LevelStart : SCR_Game;
		}

		case MNU_CONTINUE:
		{
			const UINT dwCurrentHoldID = g_pTheDB->GetHoldID();
			if (!dwCurrentHoldID) return SCR_Title;
			CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_Game));
			ASSERT(pGameScreen);
			if (!pGameScreen->IsGameLoaded())
			{
				if (!pGameScreen->LoadContinueGame())
				{
					//Probable continue slot is missing.
					if (!pGameScreen->LoadNewGame(dwCurrentHoldID))
					{
						ShowOkMessage(MID_LoadGameFailed);
						return SCR_Title;
					}
				}
			}
			ASSERT(pGameScreen->IsGameLoaded());

			//If game is continued via Enter from main menu, then go to play
			//submenu at the same time play begins so that the user returns
			//their when returning to this screen.
			this->pMenu->Hide();
			this->pPlayMenu->Show();
			SelectFirstWidget(false);

			this->bReloadDemos = false;
			return pGameScreen->SelectGotoScreen();
		}

		case MNU_HELP:
			CBrowserScreen::SetPageToLoad(NULL);
			this->bReloadDemos = false;
		return SCR_Browser;

		case MNU_SETTINGS:
			this->bReloadDemos = false;
			ResetCNetStatus();
		return SCR_Settings;

		case MNU_RESTORE:
			ASSERT(this->bSavedGameExists);
			this->bReloadDemos = false;
		return SCR_Restore;

		case MNU_DEMO:
		{
			const UINT dwDemoID = GetNextDemoID();
			if (dwDemoID)
			{
				CDemoScreen *pDemoScreen = DYN_CAST(CDemoScreen*, CScreen*,
						g_pTheSM->GetScreen(SCR_Demo));
				if (!pDemoScreen || !pDemoScreen->LoadDemoGame(dwDemoID))
					ShowOkMessage(MID_LoadGameFailed);
				else {
					this->bReloadDemos = false;
					pDemoScreen->SetReplayOptions(false);
					return SCR_Demo;
				}
			}
		}
		return SCR_Title;

		case MNU_BUILD:
		{
			if (!g_pTheDB->Holds.EditableHoldExists())
			{
				SetCursor();
#ifndef ENABLE_CHEATS
				if (ShowYesNoMessage(MID_CreateHoldPrompt) != TAG_YES)
					break;
#endif
			}

			//Editing room of current game could break it -- so unload it now.
			CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_Game));
			ASSERT(pGameScreen);
			pGameScreen->UnloadGame();
		}
		return SCR_EditSelect;

		case MNU_QUIT:
			//Don't need a sell screen if this is the full game.
			if (IsGameFullVersion())
			{
				this->bQuitPrompt = true;
				const UINT ret = ShowYesNoMessage(MID_ReallyQuit);
				this->bQuitPrompt = false;
				if (ret != TAG_NO)
					return SCR_None;
				break;
			}
		return SCR_Sell;

		case MNU_WHO:
		{
			//Changing player will make the current game invalid -- so unload it now.
			CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_Game));
			ASSERT(pGameScreen);
			pGameScreen->UnloadGame();

			this->bReloadDemos = false;
			ResetCNetStatus();
		}
		return SCR_SelectPlayer;

		case MNU_WHERE:
		{
			//Deleting hold of current game would cause it to save a continue
			//slot for a room that no longer exists -- so unload the game now.
			CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_Game));
			ASSERT(pGameScreen);
			pGameScreen->UnloadGame();
		}
		return SCR_HoldSelect;

		case MNU_BUY:
			GoToBuyNow();
		break;

		case MNU_CHAT:
		return SCR_Chat;

		default: break;
	}

	return SCR_Title;
}

//*****************************************************************************
void CTitleScreen::Animate()
//Animates the screen.
{
	//Redraw the screen.
	RedrawScreen();
}

//*****************************************************************************
bool CTitleScreen::ConfirmNewGame()
//Returns: If player really wants to start a new game
{
	//New game confirmation is disabled in player settings
	if (!this->bNewGamePrompt)
		return true;

	return (ShowYesNoMessage(g_pTheDB->GetMessageText(MID_ReallyStartNewGame)) == TAG_YES);
}

//*****************************************************************************
UINT CTitleScreen::GetNextDemoID()
//Returns:
//DemoID of next demo in sequence to show or 0L if there are no demos to show.
{
	if (this->currentDemo == this->ShowSequenceDemoIDs.end())
	{
		//Start from the first demo.
		if (this->ShowSequenceDemoIDs.empty())
			return 0L; //No demos to show.
		this->currentDemo = this->ShowSequenceDemoIDs.begin();
	}

	const UINT dwFirstDemoID = *this->currentDemo;
	UINT dwRetDemoID;   //This demo ID is returned.

	//Advance to next demo ID (for play next time), looping back to start if needed.
	//NOTE: Only select demos from currently selected hold.
	const UINT dwCurrentHoldID = g_pTheDB->GetHoldID();
	UINT dwDemoHoldID;
	do {
		//Skip demos not belonging to the current hold.
		dwRetDemoID = *this->currentDemo;
		dwDemoHoldID = g_pTheDB->Demos.GetHoldIDofDemo(dwRetDemoID);

		//Get next demo marked for show (for next time).
		++this->currentDemo;
		if (this->currentDemo == this->ShowSequenceDemoIDs.end())
			this->currentDemo = this->ShowSequenceDemoIDs.begin();
	} while (dwDemoHoldID != dwCurrentHoldID &&
			*this->currentDemo != dwFirstDemoID);  //haven't wrapped around

	if (*this->currentDemo == dwFirstDemoID &&
			dwDemoHoldID != dwCurrentHoldID)
		dwRetDemoID = 0L; //No demos for this hold were found.

	return dwRetDemoID;
}

//*****************************************************************************
void CTitleScreen::RedrawScreen(const bool bUpdate) //[default=true]
//Updates the title screen graphics.
{
	if (IsRPG1BG()) {
		DrawRPG1Screen();
	} else {
		DrawRPG2Screen();
	}

	if (this->pStatusDialog->IsVisible())
		this->pStatusDialog->Paint();

	if (bUpdate)
		UpdateRect();
}

//*****************************************************************************
void CTitleScreen::DrawRPG1Screen()
{
	static const int nShadowMaskW = this->images[TITLE_SHADOW]->w;
	static const int nShadowMaskH = this->images[TITLE_SHADOW]->h;
	static const float fOffsetFactor = 0.12f;

	//Draw light mask if higher quality graphics are enabled.
	const bool bAlpha = g_pTheBM->bAlpha || g_pTheBM->eyeCandy;

	SDL_Surface* pDestSurface = GetDestSurface();

	int nMouseX, nMouseY;
	GetMouseState(&nMouseX, &nMouseY);

	//Blit the title background.
	if (bAlpha) {
		g_pTheBM->BlitSurface(this->images[this->backgroundIndex], NULL, pDestSurface, NULL);
	}
	else {
		//Selectively damage region around title graphic and shadow.
		SDL_Rect redrawRect = MAKE_SDL_RECT(0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN);
		redrawRect.x = X_TITLE_SHADOW - static_cast<int>(CScreen::CX_SCREEN * fOffsetFactor);
		redrawRect.y = Y_TITLE_SHADOW - static_cast<int>((CScreen::CY_SCREEN - Y_TITLE_SHADOW) * fOffsetFactor);
		redrawRect.w = nShadowMaskW + static_cast<int>(CScreen::CX_SCREEN / 2 * fOffsetFactor) + 125;
		redrawRect.h = nShadowMaskH + static_cast<int>(CScreen::CY_SCREEN / 2 * fOffsetFactor) + 58;
		g_pTheBM->BlitSurface(this->images[this->backgroundIndex], &redrawRect, pDestSurface, &redrawRect);
		UpdateRect(redrawRect);

		//Erase effects drawn last frame.
		SDL_Rect src = MAKE_SDL_RECT(0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN);
		EraseChildren(this->images[this->backgroundIndex], src, true);
		this->pEffects->EraseEffects(this->images[this->backgroundIndex], src, true);
	}

	if (bAlpha)
	{
		//Roaches.
		g_pTheDBM->fLightLevel = fDarkFactor;
		addParticle();
		updateParticles(pDestSurface, nMouseX, nMouseY);
		verminEffects.UpdateAndDrawEffects(false, pDestSurface);

		//Light mask centered on mouse cursor.
		//Bounded random walk for light jitter.
		static int nXOffset = 0, nYOffset = 0;
		static const int MAX_OFFSET = 2;
		if (RAND(2) && nXOffset < MAX_OFFSET)
			++nXOffset;
		else if (nXOffset > -MAX_OFFSET)
			--nXOffset;
		if (RAND(2) && nYOffset < MAX_OFFSET)
			++nYOffset;
		else if (nYOffset > -MAX_OFFSET)
			--nYOffset;

		{
			static const int nLightMaskW = this->images[LIGHT_MASK]->w;
			static const int nLightMaskH = this->images[LIGHT_MASK]->h;
			SDL_Rect src = MAKE_SDL_RECT(0, 0, nLightMaskW, nLightMaskH);
			SDL_Rect dest = MAKE_SDL_RECT(nMouseX + nXOffset - nLightMaskW / 2, nMouseY + nYOffset - nLightMaskH / 2,
				nLightMaskW, nLightMaskH);
			g_pTheBM->AddMask(this->images[LIGHT_MASK], src, pDestSurface, dest, 1.0f / fDarkFactor + 0.002f * RAND(100));
		}
	}

	//Title shadow cast from direction of mouse cursor.
	{
		static SDL_Rect src = MAKE_SDL_RECT(0, 0, nShadowMaskW, nShadowMaskH);
		SDL_Rect dest = MAKE_SDL_RECT(
			X_TITLE_SHADOW - static_cast<Sint16>((nMouseX - (int)(X_TITLE_SHADOW + nShadowMaskW / 2)) * fOffsetFactor),
			Y_TITLE_SHADOW - static_cast<Sint16>((nMouseY - (int)(Y_TITLE_SHADOW + nShadowMaskH / 2)) * fOffsetFactor),
			nShadowMaskW, nShadowMaskH);
		g_pTheBM->DarkenWithMask(this->images[TITLE_SHADOW], src, pDestSurface, dest, 0.2f);
	}

	PaintChildren();

	AnimateCaravelLogo(pDestSurface);

	this->pEffects->UpdateAndDrawEffects(!bAlpha);
}

//*****************************************************************************
void CTitleScreen::DrawRPG2Screen()
{
	//Blit the title background.
	SDL_Surface* pDestSurface = GetDestSurface();
	g_pTheBM->BlitSurface(this->images[this->backgroundIndex], NULL, pDestSurface, NULL);

	//Lantern light.
	{
		//Light mask centered on lantern in bg image.
		const int nLanternX = 525, nLanternY = 345;
		static const int nLightMaskW = this->images[LIGHT_MASK]->w;
		static const int nLightMaskH = this->images[LIGHT_MASK]->h;
		SDL_Rect src = MAKE_SDL_RECT(0, 0, nLightMaskW, nLightMaskH);
		SDL_Rect dest = MAKE_SDL_RECT(nLanternX - nLightMaskW / 2, nLanternY - nLightMaskH / 2,
			nLightMaskW, nLightMaskH);

		//Slowing pulsing light.
		static const Uint32 pulseInterval = 2000; //ms
		static const float fBrightnessFactor = 0.3f; //196 --> 255 at full increase
		Uint32 t = SDL_GetTicks() % pulseInterval;
		const float fPulseFactor = (1.0f + cos((t / static_cast<float>(pulseInterval)) * TWOPI)) / 2.0f;
		const float fLightFactor = 1.0f + fBrightnessFactor * fPulseFactor;
		g_pTheBM->AddMask(this->images[LIGHT_MASK], src, pDestSurface, dest, fLightFactor);
	}

	PaintChildren();

	AnimateCaravelLogo(pDestSurface);

	this->pEffects->UpdateAndDrawEffects();
}

//*****************************************************************************
void CTitleScreen::AnimateCaravelLogo(SDL_Surface* pDestSurface)
{
	static const Uint32 FPS = 18;
	static const Uint32 updateMS = 1000 / FPS;

	static Uint32 dwTimeOfLastUpdate = 0;
	const Uint32 dwNow = SDL_GetTicks();

	bool update = false;
	if (dwNow - dwTimeOfLastUpdate >= updateMS) {
		dwTimeOfLastUpdate = dwNow;
		update = true;
	}

	AnimateWaves(pDestSurface, update);
	AnimateFlag(pDestSurface, update);
}

//*****************************************************************************
void CTitleScreen::AnimateWaves(SDL_Surface* pDestSurface, bool update)
//Animates the waves in the Caravel logo.
{
	//Waves area.
	int X_WAVES = 33; //for logo in SW corner
	const int Y_WAVES = 94;
	const UINT CX_WAVES = 44;
	const UINT CY_WAVES = 3;

	CImageWidget* pCaravelLogo = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_CARAVEL_LOGO_SW));

	static UINT wIndex = 0;

	if (update) {
		++wIndex;
		if (wIndex == CX_WAVES) wIndex = 0;
	}

	//Draw left side of waves.
	SDL_Rect Src = MAKE_SDL_RECT(X_WAVES + wIndex, Y_WAVES, CX_WAVES - wIndex, CY_WAVES);
	SDL_Rect Dest = MAKE_SDL_RECT(pCaravelLogo->GetX() + X_WAVES, pCaravelLogo->GetY() + Y_WAVES, CX_WAVES - wIndex, CY_WAVES);
	SDL_BlitSurface(pCaravelLogo->GetImageSurface(), &Src, pDestSurface, &Dest);
	UpdateRect(Dest);

	//Draw right side of waves.
	if (wIndex)
	{
		Src.x = X_WAVES;
		Src.w = wIndex;
		Dest.x = pCaravelLogo->GetX() + X_WAVES + CX_WAVES - wIndex;
		Dest.w = wIndex;
		SDL_BlitSurface(pCaravelLogo->GetImageSurface(), &Src, pDestSurface, &Dest);
		UpdateRect(Dest);
	}
}

//*****************************************************************************
void CTitleScreen::AnimateFlag(SDL_Surface* pDestSurface, bool update)
//Animates the flag in the Caravel logo.
{
	//Flag area.
	int X_FLAG = 50; //for logo in SW corner
	const int Y_FLAG = 16;
	const UINT CX_FLAG = 11;
	const UINT CY_FLAG = 4;

	CImageWidget* pCaravelLogo = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_CARAVEL_LOGO_SW));

	static UINT wIndex = 0;

	if (update) {
		++wIndex;
		if (wIndex == CX_FLAG) wIndex = 0;
	}

	//Draw left side of flag.
	SDL_Rect Src = MAKE_SDL_RECT(X_FLAG + wIndex, Y_FLAG, CX_FLAG - wIndex, CY_FLAG);
	SDL_Rect Dest = MAKE_SDL_RECT(pCaravelLogo->GetX() + X_FLAG, pCaravelLogo->GetY() + Y_FLAG, CX_FLAG - wIndex, CY_FLAG);
	SDL_BlitSurface(pCaravelLogo->GetImageSurface(), &Src, pDestSurface, &Dest);
	UpdateRect(Dest);

	//Draw right side of flag.
	if (wIndex)
	{
		Src.x = X_FLAG;
		Src.w = wIndex;
		Dest.x = pCaravelLogo->GetX() + X_FLAG + CX_FLAG - wIndex;
		Dest.y = pCaravelLogo->GetY() + Y_FLAG + 1;
		Dest.w = wIndex;
		SDL_BlitSurface(pCaravelLogo->GetImageSurface(), &Src, pDestSurface, &Dest);
		UpdateRect(Dest);
	}
}

//*****************************************************************************
void CTitleScreen::ResetCNetStatus()
//CaravelNet and internet settings might change on other screens where player
//data and/or the active player are altered.
//Invoke this method to query CaravelNet status again on return to this screen.
{
	CImageWidget *pInternetIcon = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_INTERNET_ICON));
	pInternetIcon->SetImage(wszSignalNo);
	this->wstrNewsText = wszEmpty;
	this->pMarqueeWidget->RemoveParts();

	this->pPlayMenu->Enable(MNU_CHAT, false);

	//When screen is exited, unload assets so they will be reloaded next activation.
	this->bReloadGraphics = true;
}

//*****************************************************************************
void CTitleScreen::RequestNews()
//Send Internet request for news text.
{
	ASSERT(!this->wNewsHandle);

	string newsQuery = "http://forum.caravelgames.com/gamenews.php?game=";
	newsQuery += szDROD;
	newsQuery += "&version=";
	newsQuery += UnicodeToUTF8(wszVersionReleaseNumber);
#ifdef BETA
	newsQuery += "-BETA-";
	newsQuery += __DATE__;
#endif
	newsQuery += "&language=";
	newsQuery += Language::GetCode(Language::GetLanguage());
	newsQuery += "&OS=";
#ifdef WIN32
	newsQuery += "Windows";
#elif defined(__linux__)
	newsQuery += "Linux";
#elif defined(__FreeBSD__)
	newsQuery += "FreeBSD";
#elif defined(__APPLE__)
	newsQuery += "Apple";
#else
	newsQuery += "Unknown";
#endif

#if defined (BETA)
	int nSpacePos;
	while ((nSpacePos = newsQuery.find(' ', 0)) != -1)
		newsQuery.replace(nSpacePos, 1, "-");
#endif

	CInternet::HttpGet(newsQuery, &this->wNewsHandle);
}

//*****************************************************************************
void CTitleScreen::SetMenuOptionStatus()
//Sets whether menu options are available.
//Post-Cond: sets this->bSavedGameExists
{
	CDb db;

	this->pMenu->Enable(MNU_DEMO, !this->ShowSequenceDemoIDs.empty());

	const UINT dwPlayerID = db.GetPlayerID();
	this->pMenu->Enable(MNU_PLAYMENU, dwPlayerID != 0);
	this->pMenu->Enable(MNU_SETTINGS, dwPlayerID != 0);

	//Check for Tutorial hold.
	//this->pPlayMenu->Enable(MNU_TUTORIAL, g_pTheDB->Holds.GetHoldIDWithStatus(CDbHold::Tutorial) != 0);

	db.SavedGames.FilterByHold(g_pTheDB->GetHoldID());
	db.SavedGames.FilterByPlayer(g_pTheDB->GetPlayerID());
	db.SavedGames.FindHiddens(true);
	CIDSet SavedGameIDs = db.SavedGames.GetIDs();
	this->bSavedGameExists = !SavedGameIDs.empty();
	this->pPlayMenu->Enable(MNU_RESTORE, this->bSavedGameExists);

	CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_Game));
	ASSERT(pGameScreen);
	const UINT dwHoldID = db.GetHoldID();	//sets hold ID, if needed
	const UINT dwContinueID = g_pTheDB->SavedGames.FindByContinue();
	const bool bGameInProgress = pGameScreen->IsGameLoaded() || dwContinueID;
	this->pPlayMenu->Enable(MNU_NEWGAME, dwHoldID != 0);
	this->pPlayMenu->Enable(MNU_CONTINUE, bGameInProgress);

	this->pPlayMenu->Enable(MNU_CHAT, g_pTheNet->IsLoggedIn());
}

//*****************************************************************************
void CTitleScreen::SetNewsText()
//Parse news text to add the proper widgets to the marquee.
{
	this->pMarqueeWidget->RemoveParts();
	this->pMarqueeWidget->AddPart(MP_Speed, 20);

	static const WCHAR www[] = {We('w'),We('w'),We('w'),We('.'),We(0)};
	static const WCHAR http[] = {We('h'),We('t'),We('t'),We('p'),We(':'),We('/'),We('/'),We(0)};
	static const WCHAR https[] = {We('h'),We('t'),We('t'),We('p'),We('s'),We(':'),We('/'),We('/'),We(0)};

	//Tokenize text based on semantic context.
	const WSTRING& wStr = this->wstrNewsText;
	int nIndex = 0, nFound, nSkipSize;
	CLabelWidget *pText;
	CHyperLinkWidget *pURL;
	UINT dwHyperLinkTag = TAG_HYPERLINK_START;
	do {
		//Search for internet links.
		nSkipSize = 0;
		if ((nFound = wStr.find(http, nIndex)) >= 0)
			nSkipSize = WCSlen(http);
		else if ((nFound = wStr.find(https, nIndex)) >= 0)
			nSkipSize = WCSlen(https);
		else nFound = wStr.find(www, nIndex);

		if (nFound < 0)
		{
			//Nothing special -- add rest of text.
			pText = new CLabelWidget(0, 0, 0, 0, 50, F_TitleMarquee, wszEmpty, false, 0, WT_Label, true);
			pText->SetText(wStr.c_str() + nIndex, false,
					nIndex ? g_pTheFM->GetSpaceWidth(F_TitleMarquee) : 0, true);
			this->pMarqueeWidget->AddPart(pText);
			break;
		}

		//URL found.  Add any text before it, then add it.
		ASSERT(nFound >= nIndex);
		if (nFound > nIndex)
		{
			pText = new CLabelWidget(0, 0, 0, 0, 50, F_TitleMarquee, wszEmpty, false, 0, WT_Label, true);
			pText->SetText(wStr.substr(nIndex, nFound - nIndex).c_str(),
					false, nIndex ? g_pTheFM->GetSpaceWidth(F_TitleMarquee) : 0, true);
			this->pMarqueeWidget->AddPart(pText);
		}

		//Find where URL ends.
		nIndex = nFound + nSkipSize;
		while (nIndex < (int)wStr.length() && !iswspace(wStr[nIndex]))
			++nIndex;
		pURL = new CHyperLinkWidget(dwHyperLinkTag++, 0, 0, 0, 50,
				F_TitleMarqueeHyperlink, F_TitleMarqueeActiveHyperlink, wszEmpty,
				wStr.substr(nFound, nIndex - nFound).c_str(), false,
				nFound > nIndex ? g_pTheFM->GetSpaceWidth(F_TitleMarquee) : 0, true);
		pURL->SetText(wStr.substr(nFound + nSkipSize, nIndex - (nFound + nSkipSize)).c_str(),
				false, 0, true);
		this->pMarqueeWidget->AddPart(pURL);

		//Skip trailing whitespace.
		while (nIndex < (int)wStr.length() && iswspace(wStr[nIndex]))
			++nIndex;
	} while (nIndex < (int)wStr.length());

	//Add some space at the end.  Init.
	pText = new CLabelWidget(0, 0, 0, 100, 50, F_TitleMarquee, wszEmpty, false, 0, WT_Label, true);
	this->pMarqueeWidget->AddPart(pText);
	this->pMarqueeWidget->Reset();
}

//*****************************************************************************
bool CTitleScreen::UnloadOnDeactivate() const
{
	return this->bReloadGraphics;
}

//*****************************************************************************
void CTitleScreen::addParticle()
//Adds a particle to the set.
{
	static const UINT MAX_VERMIN = 5;
	if (vermin.size() < MAX_VERMIN)
	{
		VERMIN v;
		resetParticle(v);
		vermin.push_back(v);
	}
}

//*****************************************************************************
void CTitleScreen::clickParticles(const int x, const int y)
//Sends a click at (x,y) to the particles.
{
	for (UINT wIndex=vermin.size(); wIndex--; )
	{
		VERMIN& v = vermin[wIndex];
		if (v.fX <= x && x <= v.fX + v.wSize &&
				v.fY <= y && y <= v.fY + v.wSize)
		{
			//Hit.
			v.bActive = false;
			++this->critterKills;

			g_pTheSound->PlaySoundEffect(SEID_SPLAT);
			CMoveCoord coord(x / CBitmapManager::CX_TILE, y / CBitmapManager::CY_TILE, NO_ORIENTATION);
			verminEffects.AddEffect(new CBloodEffect(this, coord, 16, 7, 4));
		}
	}
}

//*****************************************************************************
void CTitleScreen::resetParticle(VERMIN& v)
//Reinits given particle.
{
	v.fAngle = float(fRAND(TWOPI));
	v.fX = CScreen::CX_SCREEN/2 - (CScreen::CX_SCREEN*3/4)*cos(v.fAngle);
	v.fY = CScreen::CY_SCREEN/2 - (CScreen::CY_SCREEN*3/4)*sin(v.fAngle);
	v.acceleration = (VERMIN::ACCELERATION)RAND(VERMIN::NUM_ACCELERATIONS);
	v.wSize = UINT(CBitmapManager::CX_TILE * (0.5 + fRAND(1.0))); //various sizes
	v.bActive = true;
	v.wTileNo = MV_ROACH;
	//More monster types if set.
	if (this->bExtraCritters)
	{
		switch (RAND(7)) //determine monster type
		{
			default: v.wTileNo = MV_ROACH; break;
			case 0: v.wTileNo = MV_WW; break;
			case 1: v.wTileNo = MV_EYE; break;
		}
	}
}

//*****************************************************************************
void CTitleScreen::updateDirection(VERMIN &v)
//Change movement direction according to current acceleration.
{
	ASSERT(v.acceleration < VERMIN::NUM_ACCELERATIONS);
	switch (v.acceleration)
	{
		case VERMIN::HardLeft: v.fAngle -= 0.2f; break;
		case VERMIN::Left: v.fAngle -= 0.05f; break;
		case VERMIN::Forward: break;
		case VERMIN::Right: v.fAngle += 0.05f; break;
		case VERMIN::HardRight: v.fAngle += 0.2f; break;
		case VERMIN::NUM_ACCELERATIONS: break;
	}

	//Randomly change acceleration state according to Markov process.
	static const UINT markovProbs[VERMIN::NUM_ACCELERATIONS][VERMIN::NUM_ACCELERATIONS] = {
		//tend toward the center
		{30, 40, 10, 15,  5},
		{15, 40, 10, 30,  5},
		{ 0, 25, 50, 25,  0},
		{ 5, 30, 10, 40, 15},
		{ 5, 15, 10, 40, 30}
	};

	const UINT wRand = RAND(100);
	UINT wProbSum=markovProbs[v.acceleration][0], wProbIndex=0;
	while (wProbSum < wRand)
		wProbSum += markovProbs[v.acceleration][++wProbIndex];
	ASSERT(wProbIndex < VERMIN::NUM_ACCELERATIONS);

	v.acceleration = (VERMIN::ACCELERATION)wProbIndex;
}

//*****************************************************************************
void CTitleScreen::updateParticles(
//Advance all the particles according to their rules.
//
//Params:
	SDL_Surface *pDestSurface, const int nLightX, const int nLightY)
{
	ASSERT(pDestSurface);

	const Uint32 dwNow = SDL_GetTicks();
	static Uint32 dwTimeOfLastMove = dwNow;
	Uint32 dwFrameTime = dwNow <= dwTimeOfLastMove ? 1 : dwNow - dwTimeOfLastMove;
	static const Uint32 MAX_FRAME_TIME = 50;
	static const int SCREEN_BORDER = 200;
	float fMultiplier = (dwFrameTime < MAX_FRAME_TIME ?
		dwFrameTime : MAX_FRAME_TIME) / 7.0f; //larger divisor makes them slower
	dwTimeOfLastMove = dwNow;

	for (UINT wIndex=vermin.size(); wIndex--; )
	{
		VERMIN& v = vermin[wIndex];
		if (!v.bActive)
			resetParticle(v);

		if (v.fX < -SCREEN_BORDER || v.fX > CScreen::CX_SCREEN + SCREEN_BORDER ||
				v.fY < -SCREEN_BORDER || v.fY > CScreen::CY_SCREEN + SCREEN_BORDER) //out of bounds
			resetParticle(v);

		float fMult = fMultiplier * CBitmapManager::CX_TILE / (float)v.wSize;

		//Is monster in light?
		static const int nLightMaskRadW = this->images[LIGHT_MASK]->w / 2;
		static const int nLightMaskRadH = this->images[LIGHT_MASK]->h / 2;
		const bool bInLight = abs(int(v.fX - nLightX)) < nLightMaskRadW &&
				abs(int(v.fY - nLightY)) < nLightMaskRadH;
		if (bInLight || v.wTileNo == MV_WW)
		{
			float fAngleToLight = atan2(v.fY - nLightY, v.fX - nLightX) - v.fAngle;
			while (fAngleToLight < 0.0)
				fAngleToLight += TWOPI;
			while (fAngleToLight >= TWOPI)
				fAngleToLight -= TWOPI;
			if (v.wTileNo == MV_ROACH || (v.wTileNo == MV_WW && bInLight))
			{
				//away from light
				if (fAngleToLight < 0.1 || fAngleToLight > TWOPI - 0.1)
					v.acceleration = VERMIN::Forward;
				else
					v.acceleration = fAngleToLight > PI ? VERMIN::HardLeft : VERMIN::HardRight;
			}
			else if ((v.wTileNo == MV_EYE && bInLight) || (v.wTileNo == MV_WW && !bInLight))
			{
				//toward light
				v.acceleration = fAngleToLight < PI ? VERMIN::HardLeft : VERMIN::HardRight;
				if (v.wTileNo == MV_EYE) //eyes only turn when in light
					updateDirection(v);
			}
			if (bInLight)
				fMult *= 1.4f; //move faster
		}

		v.fX += cos(v.fAngle) * fMult;
		v.fY += sin(v.fAngle) * fMult;

		//Update angle.
		if (v.wTileNo != MV_EYE) //eyes move in straight line
			updateDirection(v);

		//If sprite won't be displayed on screen, then do nothing.
		if (v.fX < -(float)v.wSize * 1.25f || //.25 is for diagonal rotation, which increases base dimensions
			 v.fY < -(float)v.wSize * 1.25f ||
			 v.fX >= CScreen::CX_SCREEN + v.wSize * 0.25f ||
			 v.fY >= CScreen::CY_SCREEN + v.wSize * 0.25f)
			continue;

		//Set sprite image for angle.
		static const UINT ROTATION_FRAMES = 8;
		static const float fOrientationInterval = TWOPI / (float)ROTATION_FRAMES;
		static const float fHalfOrientation = fOrientationInterval / 2.0f;
		v.fAngle += fHalfOrientation; //center angle on orientation
		while (v.fAngle < 0.0)
			v.fAngle += TWOPI;
		while (v.fAngle >= TWOPI)
			v.fAngle -= TWOPI;
		UINT wOrientation = static_cast<UINT>(v.fAngle * ROTATION_FRAMES / TWOPI);
		v.fAngle -= fHalfOrientation;

		//Determine how far off sprite orientation is from real angle.
		const float fRotAngle = (float)wOrientation * fOrientationInterval - v.fAngle;
		ASSERT(fabs(fRotAngle) < fHalfOrientation);

		if (this->bBackwards)
			wOrientation += ROTATION_FRAMES / 2;
		while (wOrientation >= ROTATION_FRAMES)
			wOrientation -= ROTATION_FRAMES;
		ASSERT(wOrientation < ROTATION_FRAMES);

		static const UINT frame[VERMIN_TYPES][ROTATION_FRAMES] = {
			{TI_ROACH_E, TI_ROACH_SE, TI_ROACH_S, TI_ROACH_SW, TI_ROACH_W, TI_ROACH_NW, TI_ROACH_N, TI_ROACH_NE},
			{TI_WW_E, TI_WW_SE, TI_WW_S, TI_WW_SW, TI_WW_W, TI_WW_NW, TI_WW_N, TI_WW_NE},
			{TI_EYE_E, TI_EYE_SE, TI_EYE_S, TI_EYE_SW, TI_EYE_W, TI_EYE_NW, TI_EYE_N, TI_EYE_NE}
		};
		const UINT wTileNo = frame[v.wTileNo][wOrientation];

		//Draw tile rotated to its exact angle.
		SDL_Surface *pSrcSurface = g_pTheDBM->GetTileSurface(wTileNo);
		Uint8 *pSrcPixel = g_pTheDBM->GetTileSurfacePixel(wTileNo);
		//Set to transparent tile colorkey.
		const UINT wSurfaceIndex = g_pTheDBM->GetTileSurfaceNumber(wTileNo);
		g_pTheBM->SetSurfaceColorKey(wTileNo, wSurfaceIndex, pSrcSurface);

		//Scale to expected size.
		SDL_Surface *pScaledSurface = g_pTheDBM->ScaleSurface(pSrcSurface, pSrcPixel,
				CDrodBitmapManager::CX_TILE, CDrodBitmapManager::CY_TILE,
				v.wSize, v.wSize);
		if (!pScaledSurface)
			continue;
		SDL_Rect srcRect = MAKE_SDL_RECT(0, 0, pScaledSurface->w, pScaledSurface->h);
		g_pTheBM->CropToOpaque(srcRect, pScaledSurface);
		pSrcPixel = (Uint8*)pScaledSurface->pixels + srcRect.y * pScaledSurface->pitch +
				srcRect.x * pScaledSurface->format->BytesPerPixel;

		SDL_Surface *pRotatedSurface = g_pTheDBM->RotateSurface(pScaledSurface, pSrcPixel,
				srcRect.w, srcRect.h, fRotAngle * (180.0f / PI));
		SDL_FreeSurface(pScaledSurface);
		if (!pRotatedSurface)
			continue;

		//Handle partial blits at screen edge.
		UINT cx = pRotatedSurface->w, cy = pRotatedSurface->h;
		float fX = v.fX + float((int)v.wSize - (int)cx) / 2.0f; //dest of rotated image (keep centered)
		float fY = v.fY + float((int)v.wSize - (int)cy) / 2.0f;
		UINT x = static_cast<UINT>(fX), y = static_cast<UINT>(fY);
		UINT xSource = 0, ySource = 0;
		if (fX < 0.0 || fY < 0.0)
		{
			//Blit partial sprite at top/left edges.
			if (fX < 0.0)
			{
				x = 0;
				xSource = UINT(-fX);
				if (xSource >= cx)
					continue; //out of view
				cx -= xSource;
			}
			if (fY < 0.0)
			{
				y = 0;
				ySource = UINT(-fY);
				if (ySource >= cy)
					continue; //out of view
				cy -= ySource;
			}
		}

		SDL_Rect src = MAKE_SDL_RECT(xSource, ySource, cx, cy);
		SDL_Rect dest = MAKE_SDL_RECT(x, y, cx, cy);
		g_pTheDBM->BlitSurface(pRotatedSurface, &src, pDestSurface, &dest);

		//Darken sprite.
		if (dest.w && dest.h)
		{
			ASSERT(src.w >= dest.w);
			ASSERT(src.h >= dest.h);
			src.w = dest.w; //update to blitted area
			src.h = dest.h;
			g_pTheDBM->DarkenWithMask(pRotatedSurface, src,
					pDestSurface, dest, CBitmapManager::fLightLevel, true);
		}

		SDL_FreeSurface(pRotatedSurface);
	}
}


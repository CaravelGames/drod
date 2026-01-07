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
#include <FrontEndLib/ProgressBarWidget.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/SettingsKeys.h"
#include "../Texts/MIDs.h"
#include "../DRODLib/DbXML.h"

#include <BackEndLib/Browser.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Internet.h>
#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/Ports.h>
#include <BackEndLib/Base64.h>

#include <math.h>
#include <sstream>

#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

const UINT TAG_MENU = 1000;
const UINT TAG_PLAYMENU = 1001;
const UINT TAG_INTERNET_ICON = 1010;

const UINT TAG_CARAVEL_LOGO_SE = 1011;
const UINT TAG_CARAVEL_LOGO_SW = 1012;

const UINT TAG_TITLE_GATEB = 1020;
const UINT TAG_TITLE_JTRH = 1021;
const UINT TAG_TITLE_KDD = 1022;
const UINT TAG_TITLE_TCB = 1023;
const UINT TAG_TITLE_TSS = 1024;
const UINT TAG_TITLE_TSS_2 = 1025;
const UINT TAG_VERSION_NUMBER = 1026;
const UINT TAG_MARQUEE = 1027;

const UINT TAG_HYPERLINK_START = 10000;

const UINT dwDisplayDuration = 60000;  //ms
const UINT dwMapCycleDuration = 90000;  //ms

//Internet connection state icons.
const WCHAR wszSignalNo[] = {We('S'),We('i'),We('g'),We('n'),We('a'),We('l'),We('N'),We('o'),We(0)};
const WCHAR wszSignalYes[] = {We('S'),We('i'),We('g'),We('n'),We('a'),We('l'),We('Y'),We('e'),We('s'),We(0)};
const WCHAR wszSignalBad[] = {We('S'),We('i'),We('g'),We('n'),We('a'),We('l'),We('B'),We('a'),We('d'),We(0)};
const WCHAR wszSignalGood[] = {We('S'),We('i'),We('g'),We('n'),We('a'),We('l'),We('G'),We('o'),We('o'),We('d'),We(0)};
const WCHAR wszSignalCloud[] = {We('S'),We('i'),We('g'),We('n'),We('a'),We('l'),We('C'),We('l'),We('o'),We('u'),We('d'),We(0)};

//GatEB
#define LIGHT_MASK               (0)
#define TITLE_BACKGROUND_GATEB   (1)
#define FOREGROUND_IMAGE_GATEB   (2)
#define WINDOW_LIGHTS_MASK_GATEB (3)
//JtRH
#define TITLE_BORDER_JTRH        (4)
#define TITLE_MAP_JTRH           (5)
//KDD
#define TITLE_BACKGROUND_KDD     (6)
//TCB
#define TITLE_BACKGROUND_TCB     (7)
#define TITLE_SHADOW_TCB         (8)
//TSS
#define TITLE_BACKGROUND_TSS     (9)
//Spinning debris
#define TSS_DEBRIS_CHAIR         (10)
#define TSS_DEBRIS_COG           (11)
#define TSS_DEBRIS_CRATE         (12)
#define TSS_DEBRIS_FENCE         (13)
#define TSS_DEBRIS_TUB           (14)
#define TSS_DEBRIS_WALL          (15)
#define TSS_DEBRIS_STAIRS        (16)
#define TSS_DEBRIS_TREE          (17)
#define TSS_DEBRIS_PIPE          (18)
#define TSS_DEBRIS_LIGHTHOUSE    (19)
#define TSS_CHIMNEY              (20)
//Non-spinning
#define TSS_DEBRIS_ROCKS1        (21)
#define TSS_DEBRIS_ROCKS2        (22)

const int CX_CARAVEL_LOGO = 132;
const int CY_CARAVEL_LOGO = 132;
const int CY_MARQUEE = 30;

const WCHAR wszCaravelLogoSE[] = {
	We('C'),We('a'),We('r'),We('a'),We('v'),We('e'),We('l'),We('L'),We('o'),We('g'),We('o'),We('S'),We('E'),We(0)
};
const WCHAR wszCaravelLogoSW[] = {
	We('C'),We('a'),We('r'),We('a'),We('v'),We('e'),We('l'),We('L'),We('o'),We('g'),We('o'),We('S'),We('W'),We(0)
};

const WCHAR wszTitleLogoGatEB[] = {
	We('g'),We('e'),We('b'),We('_'),We('l'),We('o'),We('g'),We('o'),We(0)
};
const WCHAR wszTitleLogoJtRH[] = {
	We('T'),We('i'),We('t'),We('l'),We('e'),We('L'),We('o'),We('g'),We('o'),We('_'),We('J'),We('t'),We('R'),We('H'),We(0)
};
const WCHAR wszTitleLogoKDD[] = {
	We('T'),We('i'),We('t'),We('l'),We('e'),We('L'),We('o'),We('g'),We('o'),We('_'),We('K'),We('D'),We('D'),We(0)
};
const WCHAR wszTitleLogoTCB[] = {
	We('T'),We('i'),We('t'),We('l'),We('e'),We('L'),We('o'),We('g'),We('o'),We('_'),We('T'),We('C'),We('B'),We(0)
};
const WCHAR wszTitleLogoTSS[] = {
	We('T'),We('i'),We('t'),We('l'),We('e'),We('L'),We('o'),We('g'),We('o'),We('_'),We('T'),We('S'),We('S'),We(0)
};
const WCHAR wszTitleLogoTSS_2[] = {
	We('T'),We('i'),We('t'),We('l'),We('e'),We('L'),We('o'),We('g'),We('o'),We('_'),We('T'),We('S'),We('S'),We('_'),We('2'),We(0)
};

//KDD/JtRH
const int X_TITLE_KDD = (CScreen::CX_SCREEN - 493) / 2; //center
const int Y_TITLE_KDD = 110;
const int X_TITLE_JTRH = X_TITLE_KDD;
const int Y_TITLE_JTRH = Y_TITLE_KDD;

const int X_MAP_JTRH = 69;
const int Y_MAP_JTRH = 69;
const UINT wFramedMapWidth = 888;
const UINT wFramedMapHeight = 631;

//TCB
const int X_TITLE_TCB = 280; //centered: ((CScreen::CX_SCREEN - 460) / 2)
const int Y_TITLE_TCB = 110;

const int X_TITLE_SHADOW = X_TITLE_TCB - 30;
const int Y_TITLE_SHADOW = Y_TITLE_TCB + 5;

enum VerminTypes {
	MV_ROACH,
	MV_WW,
	MV_EYE,
	VERMIN_TYPES
};

//GatEB/TSS
float getDarkFactorByTimeOfDay()
{
	time_t t = time(NULL);
	tm* pLocalTime = localtime(&t);
	const float time_of_day = pLocalTime->tm_hour + pLocalTime->tm_min/60.f;
	static const float MIN_DARK_FACTOR = 0.1f;
	static const float DAWN = 6.f, MORNING = 8.f, EVENING = 19.f, NIGHT = 21.f;
	if (time_of_day <= DAWN || time_of_day >= NIGHT)
		return MIN_DARK_FACTOR; //full night
	else if (time_of_day >= MORNING && time_of_day <= EVENING)
		return 1.0f; //full day
	else if (time_of_day < MORNING) //sunrise
		return MIN_DARK_FACTOR + (((time_of_day - DAWN) / (MORNING - DAWN)) * (1.f - MIN_DARK_FACTOR));
	else //sunset
		return MIN_DARK_FACTOR + (((NIGHT - time_of_day) / (NIGHT - EVENING)) * (1.f - MIN_DARK_FACTOR));
}
bool areLightsOn(float darkFactor)
{
	return darkFactor < 0.5f;
}

//TSS
const int X_TITLE_TSS = 282; //centered: ((CScreen::CX_SCREEN - 460) / 2)
const int Y_TITLE_TSS = 50;
const int X_TITLE_TSS_2 = X_TITLE_TSS;
const int Y_TITLE_TSS_2 = Y_TITLE_TSS + 177;

#define TSS_DEBRIS_TILE_TYPES (2)
#define TSS_DEBRIS_SPINNING (11)
#define TSS_DEBRIS_NON_SPINNING (2)

//-----

#define PI (3.1415926535f)
#define TWOPI (2.0f * PI)

const float SQRT_TWO = sqrt(2.0f);
const float RADIANS_TO_DEGREES = 180.0f / PI;

//
//Protected methods.
//

//******************************************************************************
CTitleScreen::CTitleScreen() : CDrodScreen(SCR_Title)
	, bReloadDemos(true)
	, dwNonTutorialHoldID(0)
	, pMenu(NULL), pPlayMenu(NULL)
	, dwFirstPaint(0), dwDemoWaitTimer(0)
	, bSavedGameExists(false)
	, wNewsHandle(0)
	, pMarqueeWidget(NULL)
	, bWaitingForHoldlist(false)
	, bPredarken(true), bReloadGraphics(false)
	, bForcePaintOnNextAnimate(false)
	, fDarkFactor(1.0f)

	, wMapX(0), wMapY(0)
	, dwMapCycleStart(0)
	, multiplier(1)
	, verminEffects(NULL)
	, bExtraCritters(false), critterKills(0), bBackwards(false)
//Constructor.
{
	//Load image assets.
	this->imageFilenames.push_back(string("TitleLightMask"));

	//GatEB
	this->imageFilenames.push_back(string("gunthro_title_back"));
	this->imageFilenames.push_back(string("gunthro_title_front"));
	this->imageFilenames.push_back(string("gunthro_title_windows"));

	//JtRH
	this->imageFilenames.push_back(string("TitleBorderJtRH"));
	this->imageFilenames.push_back(string("TitleMap"));

	//KDD
	this->imageFilenames.push_back(string("TitleBG_KDD"));

	//TCB
	string TitleBG;
	switch (rand() % 3)
	{
		default:
		case 0: TitleBG = "TitleTCB_BG"; break;
		case 1: TitleBG = "TitleTCB_BG1"; break;
		case 2: TitleBG = "TitleTCB_BG2"; break;
	}
	this->imageFilenames.push_back(TitleBG);
	this->imageFilenames.push_back(string("TitleTCB_Shadow"));

	//TSS
	std::ostringstream tss_str;
	tss_str << "TitleTSS_BG" << ((rand() % 6) + 1);
	this->imageFilenames.push_back(tss_str.str());

	//Debris
	//Spinning
	this->imageFilenames.push_back(string("TitleTSSChair"));
	this->imageFilenames.push_back(string("TitleTSSCog"));
	this->imageFilenames.push_back(string("TitleTSSCrate"));
	this->imageFilenames.push_back(string("TitleTSSFence"));
	this->imageFilenames.push_back(string("TitleTSSTub"));
	this->imageFilenames.push_back(string("TitleTSSWall"));
	this->imageFilenames.push_back(string("TitleTSSStairs"));
	this->imageFilenames.push_back(string("TitleTSSTree"));
	this->imageFilenames.push_back(string("TitleTSSPipe"));
	this->imageFilenames.push_back(string("TitleTSSLighthouse"));
	this->imageFilenames.push_back(string("TitleTSSChimney"));

	//Non-spinning
	this->imageFilenames.push_back(string("TitleTSSRocks1"));
	this->imageFilenames.push_back(string("TitleTSSRocks2"));

	g_pTheDBM->LoadGeneralTileImages();

	this->currentDemo = this->ShowSequenceDemoIDs.end();

	const bool bDemo = !IsGameFullVersion();

	//Game title logo.
	//GatEB
	const int X_TITLE_GATEB = 50;
	const int Y_TITLE_GATEB = 50;
	AddWidget(new CImageWidget(TAG_TITLE_GATEB, X_TITLE_GATEB, Y_TITLE_GATEB, wszTitleLogoGatEB));

	//KDD/JtRH
	const int X_TITLE = (CScreen::CX_SCREEN - 493) / 2; //center
	const int Y_TITLE = 110;
	AddWidget(new CImageWidget(TAG_TITLE_KDD, X_TITLE_KDD, Y_TITLE_KDD, wszTitleLogoKDD));
	AddWidget(new CImageWidget(TAG_TITLE_JTRH, X_TITLE_JTRH, Y_TITLE_JTRH, wszTitleLogoJtRH));

	//TCB
	AddWidget(new CImageWidget(TAG_TITLE_TCB, X_TITLE_TCB, Y_TITLE_TCB, wszTitleLogoTCB));

	//TSS
	CImageWidget *pImage = new CImageWidget(TAG_TITLE_TSS, X_TITLE_TSS, Y_TITLE_TSS, wszTitleLogoTSS);
	pImage->SetAlpha(1, true);
	AddWidget(pImage);

	//TSS subtitle
	pImage = new CImageWidget(TAG_TITLE_TSS_2, X_TITLE_TSS_2, Y_TITLE_TSS_2, wszTitleLogoTSS_2);
	pImage->SetAlpha(1, true);
	AddWidget(pImage);

	//Caravel logo.
	AddWidget(new CImageWidget(TAG_CARAVEL_LOGO_SE,
		CScreen::CX_SCREEN - CX_CARAVEL_LOGO, CScreen::CY_SCREEN - CY_CARAVEL_LOGO, wszCaravelLogoSE));
	AddWidget(new CImageWidget(TAG_CARAVEL_LOGO_SW,
		0, CScreen::CY_SCREEN - CY_CARAVEL_LOGO, wszCaravelLogoSW));

	//Option menu.
#ifdef RUSSIAN_BUILD
	const int CX_MENU = 441;
	const int CX_MENU2 = 375;
#else
	const int CX_MENU = 355;
	const int CX_MENU2 = 362;
#endif
	const int CY_MENU = 360;

	this->pMenu = new CMenuWidget(TAG_MENU, 0, 0, CX_MENU, CY_MENU,
			F_TitleMenu, F_TitleMenuActive, F_TitleMenuSelected);
	this->pMenu->SetButtonSound(SEID_SWORDS);
	AddWidget(this->pMenu);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitlePlayMenu), MNU_PLAYMENU);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitlePlayer), MNU_WHO);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitleSettings), MNU_SETTINGS);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitleDemo), MNU_DEMO);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitleBuild), MNU_BUILD);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitleHelp), MNU_HELP);
	if (bDemo)
		this->pMenu->AddText(g_pTheDB->GetMessageText(MID_BuyNow), MNU_BUY);
	this->pMenu->AddText(g_pTheDB->GetMessageText(MID_TitleQuit), MNU_QUIT);

	//Play sub-menu.
	this->pPlayMenu = new CMenuWidget(TAG_PLAYMENU, 0, 0, CX_MENU2, CY_MENU,
			F_TitleMenu, F_TitleMenuActive, F_TitleMenuSelected);
	this->pPlayMenu->SetButtonSound(SEID_SWORDS);
	AddWidget(this->pPlayMenu);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleTutorial), MNU_TUTORIAL);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleNewGame), MNU_NEWGAME);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleContinue), MNU_CONTINUE);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleRestore), MNU_RESTORE);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleWhere), MNU_WHERE);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleChat), MNU_CHAT);
	this->pPlayMenu->AddText(g_pTheDB->GetMessageText(MID_TitleMainMenu), MNU_MAINMENU);
	this->pPlayMenu->Hide();

	//Version number.
	CLabelWidget *pLabel = new CLabelWidget(TAG_VERSION_NUMBER, 0, 0, 300, 0,
		 F_TitleMarquee, wszVersionReleaseNumber, true, 0, WT_Label, true);
	AddWidget(pLabel);

	//Internet connection status icon.
	pInternetIcon = new CImageWidget(TAG_INTERNET_ICON, 0, 0, wszSignalNo);
	pInternetIcon->SetAlpha(150);
	AddWidget(pInternetIcon);

	//Scroll news text in marquee.
	this->pMarqueeWidget = new CMarqueeWidget(TAG_MARQUEE, 0, 0, 1, CY_MARQUEE, 10);
	AddWidget(this->pMarqueeWidget);

	string str;
	this->bExtraCritters = CFiles::GetGameProfileString(INISection::Startup, "ExtraCritters", str);

	// April Fool's
	time_t t = time(NULL);
	tm* pLocalTime = localtime(&t);
	if (pLocalTime->tm_mon == 3 && pLocalTime->tm_mday == 1) {
		this->multiplier = -1;
		this->bBackwards = true; //critters move backwards
	}
}

//******************************************************************************
CTitleScreen::~CTitleScreen()
{
	clearDebris();
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
		CDb db;
		db.Demos.FilterByShow(g_pTheDB->GetHoldID());
		this->ShowSequenceDemoIDs = db.Demos.GetIDs();
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

	SetVisibleWidgetsForHold();

	if (UnloadOnDeactivate())
		this->bPredarken = true; //need to redarken background if assets are being reloaded
	this->bReloadGraphics = false;

	switch (this->hold_status) {
		case CDbHold::KDD:
		case CDbHold::JtRH:
			this->fDarkFactor = 1.0f; break;
		case CDbHold::TCB:
			this->fDarkFactor = 0.2f; break;
		case CDbHold::GatEB:
		case CDbHold::TSS:
		default:
			this->fDarkFactor = getDarkFactorByTimeOfDay(); break;
	}

	LoadDemos();

	const CDbPackedVars settings = g_pTheDB->GetCurrentPlayerSettings();
	g_pTheSound->bNoFocusPlaysMusic = settings.GetVar(Settings::NoFocusPlaysMusic, false);
	g_pTheDBM->bDisplayCombos = settings.GetVar(Settings::DisplayCombos, true);

	ASSERT(!this->bWaitingForHoldlist); //no previous transaction should be left uncompleted
	if (g_pTheNet->IsEnabled()) //if CaravelNet connection hasn't been disabled
	{
		//If net connect is set, get news text.
		if (this->wstrNewsText.empty())	//query news text only once
		{
			if (settings.GetVar(Settings::ConnectToInternet, false))
				RequestNews();
		}
	}
	SetMenuOptionStatus();

	PlayTitleSong();

	SetCursor();

	this->dwDemoWaitTimer = SDL_GetTicks();
	if (!this->dwFirstPaint)
		this->dwFirstPaint = this->dwDemoWaitTimer;
	this->pMenu->ResetSelection();
	this->pPlayMenu->ResetSelection();

	clearDebris();

	return CDrodScreen::SetForActivate();
}

//
//Private methods.
//

//******************************************************************************
void CTitleScreen::PlayTitleSong()
{
	SONGID songid;
	switch (this->hold_status) {
		case CDbHold::GatEB: songid = SONGID_INTRO_GATEB; break;
		case CDbHold::JtRH: songid = SONGID_INTRO_JTRH; break;
		case CDbHold::KDD: songid = SONGID_INTRO_KDD; break;
		case CDbHold::TCB: songid = SONGID_INTRO_TCB; break;
		case CDbHold::TSS: songid = SONGID_INTRO_TSS; break;
		default: songid = SONGID_INTRO_TSS; break;
	}

	g_pTheSound->PlaySong(songid);
}

void CTitleScreen::SetTitleScreenSkin()
{
	this->hold_status = GetHoldStatus();
	if (!CDbHold::IsOfficialHold(this->hold_status)) {
		//Check CaravelNet data to determine which game version the hold is from.
		const UINT holdID = g_pTheDB->GetHoldID();
		if (holdID) {
			CDbHold *pHold = g_pTheDB->Holds.GetByID(holdID, true);
			if (pHold) {
				vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();
				const int nIndex = g_pTheNet->getIndexForName((const WCHAR*)pHold->NameText);
				if (nIndex >= 0) {
					CNetMedia *pHoldData = cNetMedia[nIndex];
					const UINT version = pHoldData->wVersion;
					if (version < 200)
						this->hold_status = CDbHold::KDD;
					else if (version < 300)
						this->hold_status = CDbHold::JtRH;
					else if (version < 400)
						this->hold_status = CDbHold::TCB;
					else if (version < 500)
						this->hold_status = CDbHold::GatEB;
				}
				delete pHold;
			}
		}

		//To provide consistency w/o CaravelNet, set skin based on the newest installed official hold.
		if (!CDbHold::IsOfficialHold(this->hold_status)) {
			this->hold_status = CDbHolds::GetNewestInstalledOfficialHoldStatus();
		}
	}
}

//******************************************************************************
void CTitleScreen::SetVisibleWidgetsForHold()
{
	vector<UINT> all_widgets, active_widgets;
	all_widgets.push_back(TAG_TITLE_GATEB);
	all_widgets.push_back(TAG_TITLE_JTRH);
	all_widgets.push_back(TAG_TITLE_KDD);
	all_widgets.push_back(TAG_TITLE_TCB);
	all_widgets.push_back(TAG_TITLE_TSS);
	all_widgets.push_back(TAG_TITLE_TSS_2);
	all_widgets.push_back(TAG_CARAVEL_LOGO_SE);
	all_widgets.push_back(TAG_CARAVEL_LOGO_SW);

	int x_version_label, y_version_label;
	int x_menu, y_menu;
	SDL_Rect marquee = MAKE_SDL_RECT(0, CScreen::CY_SCREEN - CY_MARQUEE - 10, CScreen::CX_SCREEN, CY_MARQUEE);

	switch (this->hold_status) {
		case CDbHold::GatEB:
		{
			active_widgets.push_back(TAG_TITLE_GATEB);
			active_widgets.push_back(TAG_CARAVEL_LOGO_SE);
			x_version_label = 6;
			y_version_label = 0;
			x_menu = 80;
			y_menu = 340;

			marquee.w = CScreen::CX_SCREEN - (CX_CARAVEL_LOGO+1);
			break;
		}
		case CDbHold::JtRH:
		{
			active_widgets.push_back(TAG_TITLE_JTRH);
			x_version_label = 137;
			y_version_label = Y_MAP_JTRH + 4;
			x_menu = 325;
			y_menu = 340;

			const UINT MARQUEE_MARGIN = CX_CARAVEL_LOGO+1;
			marquee.x = MARQUEE_MARGIN;
			marquee.w = CScreen::CX_SCREEN - 2*marquee.x;

			//Init background map.
			this->fInitialMapPos = fRAND(1.0f);
			this->dwMapCycleStart = SDL_GetTicks();
			UpdateMapViewPosition();
			if (!g_pTheBM->bAlpha)
				this->multiplier = 0;	//low-quality graphics: no map rotation
			else if (this->multiplier == 0)
				this->multiplier = 1;
		}
		break;
		case CDbHold::KDD:
		{
			active_widgets.push_back(TAG_TITLE_KDD);
			x_version_label = 137;
			y_version_label = Y_MAP_JTRH + 4;
			x_menu = 325;
			y_menu = 340;

			const UINT MARQUEE_MARGIN = CX_CARAVEL_LOGO+1;
			marquee.x = MARQUEE_MARGIN;
			marquee.w = CScreen::CX_SCREEN - 2*marquee.x;
		}
		break;
		case CDbHold::TCB:
		{
			active_widgets.push_back(TAG_TITLE_TCB);
			active_widgets.push_back(TAG_CARAVEL_LOGO_SW);
			x_version_label = 6;
			y_version_label = 0;
			x_menu = 325;
			y_menu = 340;

			const UINT MARQUEE_MARGIN = CX_CARAVEL_LOGO+1;
			marquee.x = MARQUEE_MARGIN;
			marquee.w = CScreen::CX_SCREEN - (CX_CARAVEL_LOGO+1);
		}
		break;

		default:
		case CDbHold::TSS:
			active_widgets.push_back(TAG_TITLE_TSS);
			active_widgets.push_back(TAG_TITLE_TSS_2);
			active_widgets.push_back(TAG_CARAVEL_LOGO_SE);
			x_version_label = 6;
			y_version_label = 0;
			x_menu = 340;
			y_menu = 320;

			marquee.w = CScreen::CX_SCREEN - (CX_CARAVEL_LOGO+1);
		break;
	}

	CWidget *pWidget;

	//Show widgets.
	UINT i;
	for (i=0; i<all_widgets.size(); ++i) {
		pWidget = GetWidget(all_widgets[i]);
		pWidget->Hide();
	}
	for (i=0; i<active_widgets.size(); ++i) {
		pWidget = GetWidget(active_widgets[i]);
		pWidget->Show();
	}

	//Move widgets.
	pWidget = GetWidget(TAG_VERSION_NUMBER);
	pWidget->Move(x_version_label, y_version_label);

	int x_internet_icon, y_internet_icon;
	x_internet_icon = x_version_label;
	y_internet_icon = y_version_label + pWidget->GetH() + 3;

	pWidget = GetWidget(TAG_INTERNET_ICON);
	pWidget->Move(x_internet_icon, y_internet_icon);

	pWidget = GetWidget(TAG_MENU);
	pWidget->Move(x_menu, y_menu);

	pWidget = GetWidget(TAG_PLAYMENU);
	pWidget->Move(x_menu, y_menu);

	pWidget = GetWidget(TAG_MARQUEE);
	pWidget->Move(marquee.x, marquee.y);
	pWidget->SetWidth(marquee.w);
}

//*****************************************************************************
void CTitleScreen::OnBetweenEvents()
//Handle events
{
	Animate();

	PollForNews();

	PollForHoldList();

	ImportQueuedFiles();

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
			case MNU_TUTORIAL: RequestToolTip(MID_TutorialTip); break;
			case MNU_NEWGAME: RequestToolTip(MID_NewGameTip); break;
			case MNU_CONTINUE: RequestToolTip(MID_ContinueTip); break;
			case MNU_RESTORE: RequestToolTip(MID_RestoreTip); break;
			case MNU_WHERE: RequestToolTip(MID_WhereTip); break;
			case MNU_CHAT: RequestToolTip(MID_ChatTip); break;
			case MNU_MAINMENU: RequestToolTip(MID_MainMenuTip); break;
			default: break;
		}

	//Go to demo screen after a map BG cycle.
	if (SDL_GetTicks() - this->dwDemoWaitTimer > dwDisplayDuration)
	{
		const UINT dwDemoID = GetNextDemoID();
		if (dwDemoID)
		{
			CDemoScreen *pDemoScreen = DYN_CAST(CDemoScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_Demo));
			if (!pDemoScreen || !pDemoScreen->LoadDemoGame(dwDemoID))
			{
				ShowOkMessage(MID_LoadGameFailed);
				this->dwDemoWaitTimer = SDL_GetTicks();
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
	this->dwDemoWaitTimer = SDL_GetTicks();
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
		this->dwDemoWaitTimer = SDL_GetTicks();
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
	SDL_Surface *pDestSurface = GetDestSurface();

	switch (this->hold_status) {
		case CDbHold::GatEB:
			//Preprocess: darken border surface.
			if (this->bPredarken)
			{
				//Use darker screen if high quality graphics are set.
				const float fValue = IsShowingAlphaEffects() ? this->fDarkFactor : 0.75f;
				g_pTheBM->DarkenRect(0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN,
						fValue, this->images[TITLE_BACKGROUND_GATEB]);
				this->bPredarken = false;
			}

			//Draw background.
			g_pTheBM->BlitSurface(this->images[TITLE_BACKGROUND_GATEB], NULL, pDestSurface, NULL);
		break;
		case CDbHold::JtRH:
			DrawJtRHBorder(pDestSurface);
			RedrawMapArea();
		break;
		case CDbHold::KDD:
		{
			DrawJtRHBorder(pDestSurface);

			//Blit the title image in three strips to fill in area framed by border.
			const UINT pieces = 3;
			SDL_Rect src[pieces] = {
				{64, 0, 759, 64},
				{0, 64, wFramedMapWidth, wFramedMapHeight-64-64},
				{64, 567, 759, 64}
			};
			SDL_Rect dest[pieces] = {
				{X_MAP_JTRH+64, Y_MAP_JTRH, 759, 64},
				{X_MAP_JTRH, Y_MAP_JTRH+64, wFramedMapWidth, wFramedMapHeight-64-64},
				{X_MAP_JTRH+64, Y_MAP_JTRH+567, 759, 64}
			};
			for (UINT i=0; i<pieces; ++i)
				SDL_BlitSurface(this->images[TITLE_BACKGROUND_KDD], src+i, pDestSurface, dest+i);
		}
		break;
		case CDbHold::TCB:
			//Preprocess: darken background surface.
			if (this->bPredarken)
			{
				//Use darker screen if high quality graphics are set.
				const float fValue = g_pTheBM->bAlpha || g_pTheBM->eyeCandy ? this->fDarkFactor : 0.75f;
				g_pTheBM->DarkenRect(0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN,
						fValue, this->images[TITLE_BACKGROUND_TCB]);
				this->bPredarken = false;
			}

			//Draw background.
			g_pTheBM->BlitSurface(this->images[TITLE_BACKGROUND_TCB], NULL, GetDestSurface(), NULL);
		break;

		default:
		case CDbHold::TSS:
			//Evening/night sky is darker.
			if (this->bPredarken)
			{
				const float fValue = (1.0f + 4*this->fDarkFactor) / 5.0f; //soften dark level somewhat
				g_pTheBM->DarkenRect(0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN,
						fValue, this->images[TITLE_BACKGROUND_TSS]);
				this->bPredarken = false;
			}

			g_pTheBM->BlitSurface(this->images[TITLE_BACKGROUND_TSS], NULL, pDestSurface, NULL);
		break;
	}

	this->bForcePaintOnNextAnimate = false;

	//Draw the screen.
	RedrawScreen(bUpdateRect);
}

//*****************************************************************************
void CTitleScreen::DrawJtRHBorder(SDL_Surface *pDestSurface)
{
	const UINT pieces = 4;
	SDL_Rect src[pieces] = {
		{0, 0, 1024, 132},
		{0, 132, X_MAP_JTRH, 503},
		{957, 132, X_MAP_JTRH, 503},
		{0, 636, 1024, 132}
	};

	for (UINT i=0; i<pieces; ++i) {
		SDL_Rect dest = src[i];
		SDL_BlitSurface(this->images[TITLE_BORDER_JTRH], src+i, pDestSurface, &dest);
	}
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
			if (!holdIds.empty() && pPlayer->Settings.GetVar(Settings::CNetProgressIsOld, (BYTE)0) != 0)
			{
				AddTextToMarquee(g_pTheDB->GetMessageText(MID_UnsentCaravelNetProgress));
			}
#ifdef ENABLE_CLOUDSYNC
			if (pPlayer->Settings.GetVar(Settings::CloudActivated, false)) {
				pInternetIcon->SetImage(wszSignalCloud);
				// Get any pending cloud demo updates
				const CIDSet updatedDemos = g_pTheNet->GetUpdatedCloudDemosHolds();
				float fStartProgress = 0.1f;
				const float perHoldProgressIncrement = (1.0f - fStartProgress) / (updatedDemos.size() ? updatedDemos.size() : 1.0f);

				for (CIDSet::const_iterator hold = updatedDemos.begin(); hold != updatedDemos.end(); ++hold) {
					const UINT progressHandle = g_pTheNet->CloudDownloadProgress(*hold);
					GenericNetTransactionWait(progressHandle, MID_CloudSynchInProgress,
						fStartProgress, fStartProgress + perHoldProgressIncrement);
					if (g_pTheNet->GetStatus(progressHandle) == CURLE_OK) {
						CNetResult* pDemos = g_pTheNet->GetResults(progressHandle);
						if (pDemos) {
							if (pDemos->pJson && pDemos->pJson->isMember("data")) {
								const string demoXmlB64 = pDemos->pJson->get("data", "").asString();
								const UINT version = pDemos->pJson->get("demosVersion", 0).asUInt();
								string compressedXml;
								Base64::decode(demoXmlB64, compressedXml);
								const MESSAGE_ID result = CDbXML::ImportXMLRaw(compressedXml, CImportInfo::DemosAndSavedGames, true);
								g_pTheNet->SetCloudVersionID(Settings::CloudHoldDemosVersion, *hold, version);
								g_pTheNet->SetCloudDemosCurrent(*hold);
							}
							delete pDemos;
						}
					}
				}
				ExportCleanup();
				Paint();
			}
#endif // ENABLE_CLOUDSYNC
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
				AddTextToMarquee(g_pTheDB->GetMessageText(MID_InvalidCaravelNetKey));
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
			f.GetGameProfileString(INISection::Startup, INIKey::LastNews, str);
			if (strFromWeb.size() && str.compare(strFromWeb.c_str()) != 0)
			{
				//New news!
				f.WriteGameProfileString(INISection::Startup, INIKey::LastNews, strFromWeb.c_str());
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
			this->bForcePaintOnNextAnimate = true;
			SelectFirstWidget(false);
		break;

		case MNU_PLAYMENU:
			this->pMenu->Hide();
			this->pPlayMenu->Show();
			this->bForcePaintOnNextAnimate = true;
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

			//Select tutorial based on keyboard configuration.
			UINT dwTutorialHoldID = g_pTheDB->Holds.GetHoldIDWithStatus(CDbHold::Tutorial);
			CFiles Files;
			string strKeyboard;
			if (Files.GetGameProfileString(INISection::Localization, INIKey::Keyboard, strKeyboard))
				if (atoi(strKeyboard.c_str()) == 1)
					++dwTutorialHoldID;	//ATTN: tutorial holds must be in consecutive order

			g_pTheDB->SetHoldID(dwTutorialHoldID);
		}
		//NO BREAK

		case MNU_NEWGAME:
		{
			const UINT dwCurrentHoldID = g_pTheDB->GetHoldID();
			if (!dwCurrentHoldID) return SCR_Title;
			CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_Game));
			ASSERT(pGameScreen);
			if (pGameScreen->IsGameLoaded())
				pGameScreen->UnloadGame();
			if (!pGameScreen->LoadNewGame(dwCurrentHoldID))
			{
				ShowOkMessage(MID_LoadGameFailed);
				return SCR_Title;
			}
			ASSERT(pGameScreen->IsGameLoaded());

			this->bReloadDemos = false;
			const SCREENTYPE firstScreen = pGameScreen->SelectGotoScreen();
			pGameScreen->MarkCurrentEntranceExplored();
			return firstScreen;
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
		return SelectSellScreen();

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
void CTitleScreen::AddSpaceToMarquee()
{
	CLabelWidget *pLabel = new CLabelWidget(0, 0, 0, 100, 50, F_TitleMarquee, wszEmpty);
	this->pMarqueeWidget->AddPart(pLabel);
	this->pMarqueeWidget->Reset();
}

void CTitleScreen::AddTextToMarquee(const WCHAR *pText)
{
	ASSERT(pText);

	CLabelWidget *pLabel = new CLabelWidget(0, 0, 0, 0, 50, F_TitleMarquee, wszEmpty, false, 0, WT_Label, true);
	pLabel->SetText(pText, false, 0, true);
	this->pMarqueeWidget->AddPart(pLabel);

	AddSpaceToMarquee();
}

//*****************************************************************************
void CTitleScreen::Animate()
//Animates the screen.
{
	switch (this->hold_status) {
		case CDbHold::KDD:
		{
			SDL_Surface *pBackground = this->images[TITLE_BACKGROUND_KDD];
			SDL_Surface *pDestSurface = GetDestSurface();
			SDL_Rect dest;

			//Refresh version # area.
			CWidget *pWidget = GetWidget(TAG_VERSION_NUMBER);
			pWidget->GetRect(dest);
			RefreshBackground(pBackground, pDestSurface, dest, X_MAP_JTRH, Y_MAP_JTRH);

			//Refresh title logo area to preserve transparency.
			pWidget = GetWidget(TAG_TITLE_KDD);
			pWidget->GetRect(dest);
			RefreshBackground(pBackground,pDestSurface, dest, X_MAP_JTRH, Y_MAP_JTRH);

			//Refresh menu display area.
			CWidget *pMenuWidget = GetWidget(TAG_PLAYMENU); //larger than TAG_MENU
			pMenuWidget->GetRect(dest);
			RefreshBackground(pBackground, pDestSurface, dest, X_MAP_JTRH, Y_MAP_JTRH);

			//Erase old marquee text.
			this->pMarqueeWidget->GetRect(dest);
			RefreshBackground(this->images[TITLE_BORDER_JTRH], pDestSurface, dest);
		}
		break;
		case CDbHold::JtRH:
		{
			RedrawMapArea(false);

			//Erase old marquee text.
			SDL_Surface *pBackground = this->images[TITLE_BORDER_JTRH];
			SDL_Rect dest;
			this->pMarqueeWidget->GetRect(dest);
			RefreshBackground(pBackground, GetDestSurface(), dest);
		}
		break;

		case CDbHold::TCB:
		case CDbHold::GatEB:
		break;

		default:
		case CDbHold::TSS:
		{
			SDL_Surface *pBackground = this->images[TITLE_BACKGROUND_TSS];
			SDL_Surface *pDestSurface = GetDestSurface();
			SDL_Rect dest;

			//Refresh version # area.
			CWidget *pWidget = GetWidget(TAG_VERSION_NUMBER);
			pWidget->GetRect(dest);
			RefreshBackground(pBackground, pDestSurface, dest);

			//Fade in title logo.
			CImageWidget *pImage = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_TITLE_TSS));
			CImageWidget *pImage2 = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_TITLE_TSS_2));
			const Uint32 time_spent = SDL_GetTicks() - this->dwFirstPaint;

			static const Uint32 logo_fadein = 2000; //ms
			if (time_spent >= logo_fadein) {
				pImage->SetAlpha(255);
			} else {
				Uint8 logo_transparency = 255 * time_spent / logo_fadein;
				if (!logo_transparency)
					logo_transparency = 1;
				pImage->SetAlpha(logo_transparency, true);
			}

			//Fade in subtitle a bit delayed.
			static const Uint32 subtitle_delay = 1000;
			if (time_spent < subtitle_delay) {
				pImage2->Hide();
			} else {
				pImage2->Show();
				static const Uint32 subtitle_end_fadein = subtitle_delay + logo_fadein;
				if (time_spent >= subtitle_end_fadein) {
					pImage2->SetAlpha(255);
				} else {
					Uint8 logo_transparency = 255 * (time_spent-subtitle_delay) / logo_fadein;
					if (!logo_transparency)
						logo_transparency = 1;
					pImage2->SetAlpha(logo_transparency, true);
				}
			}

			//Refresh title logo area to preserve transparency.
			pImage->GetRect(dest);
			RefreshBackground(pBackground, pDestSurface, dest);
			pImage2->GetRect(dest);
			RefreshBackground(pBackground, pDestSurface, dest);

			//Refresh menu display area.
			{
				CWidget *pMenuWidget = GetWidget(TAG_PLAYMENU); //larger than TAG_MENU
				pMenuWidget->GetRect(dest);
				RefreshBackground(pBackground, pDestSurface, dest);
			}

			//Refresh caravelnet status area
			this->pInternetIcon->GetRect(dest);
			RefreshBackground(pBackground, pDestSurface, dest);

			//Erase old marquee text.
			this->pMarqueeWidget->GetRect(dest);
			RefreshBackground(pBackground, pDestSurface, dest);

			eraseDebrisFromScreen();
		}
		break;
	}

	//Redraw the screen.
	if (this->bForcePaintOnNextAnimate)
	{
		Paint();
	}
	else
	{
		RedrawScreen();
	}
}

//*****************************************************************************
void CTitleScreen::RefreshBackground(
	SDL_Surface* pBackground, SDL_Surface* pDestSurface, SDL_Rect& dest,
	int x_offset, int y_offset)
{
	SDL_Rect src = dest;
	src.x -= x_offset;
	src.y -= y_offset;
	SDL_BlitSurface(pBackground, &src, pDestSurface, &dest);
	UpdateRect(dest);
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

bool CTitleScreen::IsShowingAlphaEffects() const
{
	return g_pTheBM->bAlpha || g_pTheBM->eyeCandy;
}

//*****************************************************************************
void CTitleScreen::RedrawMapArea(const bool bAlwaysRedraw)	//[default=true]
//Updates the scrolling background map's position.
//Repaints everything on this area (optional).
{
	bool bRedrawBackground = UpdateMapViewPosition();
	bRedrawBackground |= bAlwaysRedraw;

	if (bRedrawBackground)
	{
		//Blit map in three strips to fill in area framed by border.
		SDL_Rect src[3] = {
			MAKE_SDL_RECT(this->wMapX+64, this->wMapY, 759, 64),
			MAKE_SDL_RECT(this->wMapX, this->wMapY+64, wFramedMapWidth, wFramedMapHeight-64-64),
			MAKE_SDL_RECT(this->wMapX+64, this->wMapY+567, 759, 64)
		};
		SDL_Rect dest[3] = {
			MAKE_SDL_RECT(X_MAP_JTRH+64, Y_MAP_JTRH, 759, 64),
			MAKE_SDL_RECT(X_MAP_JTRH, Y_MAP_JTRH+64, wFramedMapWidth, wFramedMapHeight-64-64),
			MAKE_SDL_RECT(X_MAP_JTRH+64, Y_MAP_JTRH+567, 759, 64)
		};
		for (UINT i=0; i<3; ++i)
			SDL_BlitSurface(this->images[TITLE_MAP_JTRH], src+i, GetDestSurface(), dest+i);
	} else {
		//Not redrawing the whole background.
		//Only erase stuff that was drawn over the background last frame.
		SDL_Rect src = MAKE_SDL_RECT(this->wMapX - X_MAP_JTRH, this->wMapY - Y_MAP_JTRH, wFramedMapWidth, wFramedMapHeight);
		EraseChildren(this->images[TITLE_MAP_JTRH], src, !bAlwaysRedraw);
		this->pEffects->EraseEffects(this->images[TITLE_MAP_JTRH], src, !bAlwaysRedraw);
	}

	if (!bAlwaysRedraw && bRedrawBackground)
	{
		//If the screen won't be updated by caller...
		PaintChildren();

		this->pEffects->UpdateAndDrawEffects();

		//Update the map area here.
		SDL_Rect wholeDest = {X_MAP_JTRH, Y_MAP_JTRH, wFramedMapWidth, wFramedMapHeight};
		UpdateRect(wholeDest);
	}
}

//*****************************************************************************
void CTitleScreen::RedrawScreen(const bool bUpdate) //[default=true]
//Updates the title screen graphics.
{
	//Draw light mask if higher quality graphics are enabled.
	const bool bAlpha = IsShowingAlphaEffects();

	SDL_Surface *pDestSurface = GetDestSurface();

	int nMouseX, nMouseY;
	GetMouseState(&nMouseX, &nMouseY);

	switch (this->hold_status) {
		case CDbHold::GatEB:
		{
			g_pTheBM->BlitSurface(this->images[TITLE_BACKGROUND_GATEB], NULL, pDestSurface, NULL);

			const bool bLightsOn = bAlpha && areLightsOn(this->fDarkFactor);
			if (bLightsOn)
				DrawLightMask(pDestSurface, nMouseX, nMouseY, 0.85f/this->fDarkFactor + 0.003f * RAND(100));

			g_pTheBM->BlitSurface(this->images[FOREGROUND_IMAGE_GATEB], NULL, pDestSurface, NULL);

			//Lights turn on when the environment becomes too dark.
			if (bLightsOn) {
				//Light in windows flickers.
				static const float fLightWavering = 0.2f;
				static const float fDimLights = 1.0f - fLightWavering;
				static float fLightValue = 0.0f;

				static Uint32 dwTimeOfLastFlicker = 0;
				static const Uint32 flickerFPS = 12;
				static const Uint32 flickerMS = 1000/flickerFPS;
				const Uint32 dwNow = SDL_GetTicks();
				if (dwNow - dwTimeOfLastFlicker >= flickerMS) {
					dwTimeOfLastFlicker = dwNow;
					fLightValue = fDimLights + fRAND(fLightWavering);
				}

				static const SDL_Rect lightsRectLeft = {245,230, 154,122};
				static const SDL_Rect lightsRectRight = {612,140, 287,362};
				g_pTheBM->AddMaskAdditive(this->images[WINDOW_LIGHTS_MASK_GATEB], lightsRectLeft, pDestSurface, lightsRectLeft, fLightValue, true);
				g_pTheBM->AddMaskAdditive(this->images[WINDOW_LIGHTS_MASK_GATEB], lightsRectRight, pDestSurface, lightsRectRight, fLightValue, true);
			}
		}
		break;

		case CDbHold::JtRH: break;
		case CDbHold::KDD: break;
		case CDbHold::TCB:
		{
			static const int nShadowMaskW = this->images[TITLE_SHADOW_TCB]->w;
			static const int nShadowMaskH = this->images[TITLE_SHADOW_TCB]->h;
			static const float fOffsetFactor = 0.12f;

			//Blit the title background.
			if (bAlpha)
				g_pTheBM->BlitSurface(this->images[TITLE_BACKGROUND_TCB], NULL, pDestSurface, NULL);
			else
			{
				//Selectively damage region around title graphic and shadow.
				SDL_Rect redrawRect = MAKE_SDL_RECT(0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN);
				redrawRect.x = X_TITLE_SHADOW - static_cast<int>(CScreen::CX_SCREEN * fOffsetFactor);
				redrawRect.y = Y_TITLE_SHADOW - static_cast<int>((CScreen::CY_SCREEN - Y_TITLE_SHADOW) * fOffsetFactor);
				redrawRect.w = nShadowMaskW + static_cast<int>(CScreen::CX_SCREEN/2 * fOffsetFactor) + 125;
				redrawRect.h = nShadowMaskH + static_cast<int>(CScreen::CY_SCREEN/2 * fOffsetFactor) + 58;
				g_pTheBM->BlitSurface(this->images[TITLE_BACKGROUND_TCB], &redrawRect, pDestSurface, &redrawRect);
				UpdateRect(redrawRect);

				//Erase effects drawn last frame.
				SDL_Rect src = MAKE_SDL_RECT(0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN);
				EraseChildren(this->images[TITLE_BACKGROUND_TCB], src, true);
				this->pEffects->EraseEffects(this->images[TITLE_BACKGROUND_TCB], src, true);
			}

			if (bAlpha)
			{
				//Roaches.
				g_pTheDBM->fLightLevel = this->fDarkFactor;
				addParticle();
				updateParticles(pDestSurface, nMouseX, nMouseY);
				verminEffects.UpdateAndDrawEffects(false, pDestSurface);

				DrawLightMask(pDestSurface, nMouseX, nMouseY, 1.0f/this->fDarkFactor + 0.002f * RAND(100));
			}

			//Title shadow cast from direction of mouse cursor.
			{
				static SDL_Rect src = MAKE_SDL_RECT(0, 0, nShadowMaskW, nShadowMaskH);
				SDL_Rect dest = MAKE_SDL_RECT(
						X_TITLE_SHADOW - static_cast<Sint16>((nMouseX-(int)(X_TITLE_SHADOW + nShadowMaskW/2)) * fOffsetFactor),
						Y_TITLE_SHADOW - static_cast<Sint16>((nMouseY-(int)(Y_TITLE_SHADOW + nShadowMaskH/2)) * fOffsetFactor),
						nShadowMaskW, nShadowMaskH);
				g_pTheBM->DarkenWithMask(this->images[TITLE_SHADOW_TCB], src, pDestSurface, dest, 0.2f);
			}
		}
		break;

		default:
		case CDbHold::TSS:
			g_pTheDBM->fLightLevel = this->fDarkFactor;

			addDebris();
			updateDebris(pDestSurface);
		break;
	}

	PaintChildren();

	AnimateCaravelLogo(pDestSurface);

	this->pEffects->UpdateAndDrawEffects(!bAlpha);

	if (this->pStatusDialog->IsVisible())
		this->pStatusDialog->Paint();

	if (bUpdate)
	{
		UpdateRect();
	}
}

//*****************************************************************************
void CTitleScreen::AnimateCaravelLogo(SDL_Surface *pDestSurface)
{
	static const Uint32 FPS = 18;
	static const Uint32 updateMS = 1000/FPS;

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
void CTitleScreen::AnimateWaves(SDL_Surface *pDestSurface, bool update)
//Animates the waves in the Caravel logo.
{
	//Waves area.
	int X_WAVES = 38; //for logo in SE corner
	const int Y_WAVES = 94;
	const UINT CX_WAVES = 44;
	const UINT CY_WAVES = 3;

	CImageWidget *pCaravelLogo = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_CARAVEL_LOGO_SE));
	if (!pCaravelLogo->IsVisible()) {
		pCaravelLogo = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_CARAVEL_LOGO_SW));
		X_WAVES = 33;
	}

	static UINT wIndex=0;

	if (update) {
		++wIndex;
		if (wIndex==CX_WAVES) wIndex=0;
	}

	//Draw left side of waves.
	SDL_Rect Src = MAKE_SDL_RECT(X_WAVES+wIndex, Y_WAVES, CX_WAVES-wIndex, CY_WAVES);
	SDL_Rect Dest = MAKE_SDL_RECT(pCaravelLogo->GetX() + X_WAVES, pCaravelLogo->GetY() + Y_WAVES, CX_WAVES-wIndex, CY_WAVES);
	SDL_BlitSurface(pCaravelLogo->GetImageSurface(), &Src, pDestSurface, &Dest);
	UpdateRect(Dest);

	//Draw right side of waves.
	if (wIndex)  
	{
		Src.x = X_WAVES;
		Src.w = wIndex;
		Dest.x = pCaravelLogo->GetX() + X_WAVES+CX_WAVES-wIndex;
		Dest.w = wIndex;
		SDL_BlitSurface(pCaravelLogo->GetImageSurface(), &Src, pDestSurface, &Dest);
		UpdateRect(Dest);
	}
}

//*****************************************************************************
void CTitleScreen::AnimateFlag(SDL_Surface *pDestSurface, bool update)
//Animates the flag in the Caravel logo.
{
	//Flag area.
	int X_FLAG = 55; //for logo in SE corner
	const int Y_FLAG = 16;
	const UINT CX_FLAG = 11;
	const UINT CY_FLAG = 4;

	CImageWidget *pCaravelLogo = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_CARAVEL_LOGO_SE));
	if (!pCaravelLogo->IsVisible()) {
		pCaravelLogo = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_CARAVEL_LOGO_SW));
		X_FLAG = 50;
	}

	static UINT wIndex=0;

	if (update) {
		++wIndex;
		if (wIndex==CX_FLAG) wIndex=0;
	}

	//Draw left side of flag.
	SDL_Rect Src = MAKE_SDL_RECT(X_FLAG+wIndex, Y_FLAG, CX_FLAG-wIndex, CY_FLAG);
	SDL_Rect Dest = MAKE_SDL_RECT(pCaravelLogo->GetX() + X_FLAG, pCaravelLogo->GetY() + Y_FLAG, CX_FLAG-wIndex, CY_FLAG);
	SDL_BlitSurface(pCaravelLogo->GetImageSurface(), &Src, pDestSurface, &Dest);
	UpdateRect(Dest);

	//Draw right side of flag.
	if (wIndex)  
	{
		Src.x = X_FLAG;
		Src.w = wIndex;
		Dest.x = pCaravelLogo->GetX() + X_FLAG+CX_FLAG-wIndex;
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

	string newsQuery = CNetInterface::cNetBaseURL + "gamenews.php?game=";
	newsQuery += szDROD;
	newsQuery += "&version=";
	const string ver = UnicodeToUTF8(wszVersionReleaseNumber);
	newsQuery += ver;
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
	this->pPlayMenu->Enable(MNU_TUTORIAL, g_pTheDB->Holds.GetHoldIDWithStatus(CDbHold::Tutorial) != 0);

	const UINT holdID = g_pTheDB->GetHoldID();
	if (!holdID) {
		this->bSavedGameExists = false;
	} else {
		db.SavedGames.FilterByHold(holdID);
		db.SavedGames.FilterByPlayer(g_pTheDB->GetPlayerID());
		const CIDSet SavedGameIDs = db.SavedGames.GetIDs();
		//Saved games for continue slot and demos won't be counted since they are hidden.
		this->bSavedGameExists = !SavedGameIDs.empty();
	}
	this->pPlayMenu->Enable(MNU_RESTORE, this->bSavedGameExists);

	CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_Game));
	ASSERT(pGameScreen);
	if (!holdID) {
		this->pPlayMenu->Enable(MNU_NEWGAME, false);
		this->pPlayMenu->Enable(MNU_CONTINUE, false);
	} else {
		const UINT dwHoldID = db.GetHoldID();	//side-effect: call sets hold ID (still needed?)
		const UINT dwContinueID = g_pTheDB->SavedGames.FindByContinue();
		const bool bGameInProgress = pGameScreen->IsGameLoaded() || dwContinueID;
		this->pPlayMenu->Enable(MNU_NEWGAME, dwHoldID != 0);
		this->pPlayMenu->Enable(MNU_CONTINUE, bGameInProgress);
	}

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
					false, nIndex ? g_pTheFM->GetSpaceWidth(F_ButtonWhite) : 0, true);
			this->pMarqueeWidget->AddPart(pText);
		}

		//Find where URL ends.
		nIndex = nFound + nSkipSize;
		while (nIndex < (int)wStr.length() && !iswspace(wStr[nIndex]))
			++nIndex;
		pURL = new CHyperLinkWidget(dwHyperLinkTag++, 0, 0, 0, 50,
				F_TitleMarqueeHyperlink, F_TitleMarqueeActiveHyperlink, wszEmpty,
				wStr.substr(nFound, nIndex - nFound).c_str(), false,
				nFound > nIndex ? g_pTheFM->GetSpaceWidth(F_HeaderWhite) : 0, true);
		pURL->SetText(wStr.substr(nFound + nSkipSize, nIndex - (nFound + nSkipSize)).c_str(),
				false, 0, true);
		this->pMarqueeWidget->AddPart(pURL);

		//Skip trailing whitespace.
		while (nIndex < (int)wStr.length() && iswspace(wStr[nIndex]))
			++nIndex;
	} while (nIndex < (int)wStr.length());

	//Add some space at the end.  Init.
	AddSpaceToMarquee();
}

//*****************************************************************************
bool CTitleScreen::UnloadOnDeactivate() const
//Whether to unload graphical assets so they are reloaded next activation.
{
	return true; //always reload assets on screen reentrance in order to change light level
}

//*****************************************************************************
void CTitleScreen::DrawLightMask(SDL_Surface *pDestSurface, int nMouseX, int nMouseY, float fFactor)
{
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
		SDL_Rect dest = MAKE_SDL_RECT(nMouseX + nXOffset - nLightMaskW/2, nMouseY + nYOffset - nLightMaskH/2,
				nLightMaskW, nLightMaskH);
		g_pTheBM->AddMask(this->images[LIGHT_MASK], src, pDestSurface, dest, fFactor);
	}
}

//*****************************************************************************
bool CTitleScreen::UpdateMapViewPosition()
//Move the background view in a cycle.
//
//Returns: whether the view position has changed
{
	if (this->multiplier == 0)
		return false;

	const UINT wCycleRadWidth = (this->images[TITLE_MAP_JTRH]->w - wFramedMapWidth) / 2;
	const UINT wCycleRadHeight = (this->images[TITLE_MAP_JTRH]->h - wFramedMapHeight) / 2;

	const UINT dwElapsed = SDL_GetTicks() - this->dwMapCycleStart;
	const double fRadians = TWOPI * ((dwElapsed/(double)dwMapCycleDuration) + this->fInitialMapPos);
	const UINT wOldX = this->wMapX, wOldY = this->wMapY;
	this->wMapX = wCycleRadWidth + static_cast<UINT>(wCycleRadWidth * cos(fRadians * this->multiplier));
	this->wMapY = wCycleRadHeight + static_cast<UINT>(wCycleRadHeight * sin(fRadians * this->multiplier));

	ASSERT(this->wMapX + wFramedMapWidth <= static_cast<UINT>(this->images[TITLE_MAP_JTRH]->w));
	ASSERT(this->wMapY + wFramedMapHeight <= static_cast<UINT>(this->images[TITLE_MAP_JTRH]->h));

	return this->wMapX != wOldX || this->wMapY != wOldY;
}

//*****************************************************************************
//TCB
void CTitleScreen::addParticle()
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
	v.wSize = 22;
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
		case VERMIN::Left: v.fAngle -= 0.1f; break;
		case VERMIN::Forward: break;
		case VERMIN::Right: v.fAngle += 0.1f; break;
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
	const float fMultiplier = (dwFrameTime < MAX_FRAME_TIME ?
		dwFrameTime : MAX_FRAME_TIME) / 10.0f; //larger divisor makes them slower
	dwTimeOfLastMove = dwNow;

	for (UINT wIndex=vermin.size(); wIndex--; )
	{
		VERMIN& v = vermin[wIndex];
		if (!v.bActive)
			resetParticle(v);

		if (v.fX < -SCREEN_BORDER || v.fX > CScreen::CX_SCREEN + SCREEN_BORDER ||
				v.fY < -SCREEN_BORDER || v.fY > CScreen::CY_SCREEN + SCREEN_BORDER) //out of bounds
			resetParticle(v);

		float fMult = fMultiplier;

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
		if (v.fX < -(float)CDrodBitmapManager::CX_TILE * 1.25f ||
			 v.fY < -(float)CDrodBitmapManager::CY_TILE * 1.25f ||
			 v.fX >= CScreen::CX_SCREEN + CDrodBitmapManager::CX_TILE * 0.25f ||
			 v.fY >= CScreen::CY_SCREEN + CDrodBitmapManager::CY_TILE * 0.25f)
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
		ASSERT(fabs(fRotAngle) < fOrientationInterval); //Should be less than fHalfOrientation, but can't guarantee due to float precision

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

		SDL_Surface *pRotatedSurface = g_pTheDBM->RotateSurface(pSrcSurface, pSrcPixel,
				CDrodBitmapManager::CX_TILE, CDrodBitmapManager::CY_TILE, fRotAngle * (180.0f / PI));
		if (!pRotatedSurface)
			continue;

		//Handle partial blits at screen edge.
		UINT cx = pRotatedSurface->w, cy = pRotatedSurface->h;
		float fX = v.fX + float((int)CDrodBitmapManager::CX_TILE - (int)cx) / 2.0f; //dest of rotated image (keep centered)
		float fY = v.fY + float((int)CDrodBitmapManager::CY_TILE - (int)cy) / 2.0f;
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

//******************************************************************************
//TSS
void CTitleScreen::addDebris()
{
	static const UINT MAX_DEBRIS = 10;
	static const UINT DEBRIS_APPEAR_RATE = 20;
	if (debris.size() < MAX_DEBRIS && (RAND(DEBRIS_APPEAR_RATE) == 0))
	{
		DEBRIS *d = new DEBRIS();
		resetDebris(*d);
		debris.push_back(d);
		sort_debris_by_distance();
	}
}

void CTitleScreen::clearDebris()
{
	for (DEBRISLIST::iterator iter=debris.begin(); iter!=debris.end(); ++iter)
	{
		DEBRIS *d = *iter;

		if (d->pScaledSurface)
			SDL_FreeSurface(d->pScaledSurface);
		delete d;
	}

	debris.clear();
}

void CTitleScreen::eraseDebrisFromScreen()
{
	for (DEBRISLIST::iterator iter=debris.begin(); iter!=debris.end(); ++iter)
	{
		DEBRIS& d = *(*iter);
		SDL_Rect& dest = d.damaged;
		SDL_Rect src = dest;
		SDL_BlitSurface(this->images[TITLE_BACKGROUND_TSS], &src, GetDestSurface(), &dest);
		if (dest.w && dest.h)
			UpdateRect(dest);
		d.clear_damaged();
	}
}

UINT CTitleScreen::get_debris_image_index(DEBRIS& d) const
{
	switch (d.type) {
		case DT_SPINNING: return TSS_DEBRIS_CHAIR + d.type_num;
		case DT_NON_SPINNING: return TSS_DEBRIS_ROCKS1 + d.type_num;
		default: return d.type_num;
	}
}

void CTitleScreen::resetDebris(DEBRIS& d)
{
	d.clear_damaged();

	if (d.pScaledSurface) {
		SDL_FreeSurface(d.pScaledSurface);
		d.pScaledSurface = NULL;
	}

	select_unique_custom_debris(d);

	switch (d.type) {
		case DT_SPRITE:
			d.distance_multiplier = 0.1f + fRAND(1.4f);
		break;
		case DT_SPINNING:
			d.distance_multiplier = 0.01f + fRAND(0.99f);
		break;
		case DT_NON_SPINNING:
			d.distance_multiplier = 0.05f + fRAND(0.95f);
		break;

		case DEBRIS_TYPES: break;
	}

	UINT x_orig, y_orig;
	const UINT image_index = get_debris_image_index(d);
	const bool custom_image = d.type != DT_SPRITE;
	if (custom_image) {
		//Set size values.
		const SDL_Surface *pSurface = this->images[image_index];
		x_orig = pSurface->w;
		y_orig = pSurface->h;
	} else {
		x_orig = CDrodBitmapManager::CX_TILE;
		y_orig = CDrodBitmapManager::CY_TILE;
	}

	d.xSize = UINT(ceil(x_orig * d.distance_multiplier));
	d.ySize = UINT(ceil(y_orig * d.distance_multiplier));

	if (custom_image) {
		//Prescale
		const SDL_Surface *pSurface = this->images[image_index];
		const Uint8 *pSrcPixel = (Uint8*)pSurface->pixels;
		d.pScaledSurface = g_pTheDBM->ScaleSurface(pSurface, pSrcPixel,
			x_orig, y_orig,	d.xSize, d.ySize);
		if (!d.pScaledSurface) {
			d.bActive = false; //can't display
			return;
		}

		//Set darkness on same image.
		if (CBitmapManager::fLightLevel < 1.0f) {
			SDL_Rect src = MAKE_SDL_RECT(0, 0, d.pScaledSurface->w, d.pScaledSurface->h);
			g_pTheBM->CropToOpaque(src, d.pScaledSurface);
			SDL_Rect dest = src;
			g_pTheDBM->DarkenWithMask(d.pScaledSurface, src,
				d.pScaledSurface, dest, CBitmapManager::fLightLevel, true);
		}

		const float momentum = 1000.0f / (pSurface->w * pSurface->h);
		d.angularRotation = fRAND_MID(momentum);
	} else {
		d.angularRotation = fRAND_MID(0.05f);
	}

	d.fAngle = d.type == DT_NON_SPINNING ? 0.0f : float(fRAND(TWOPI));

	const int x_size_half = int(d.xSize/2);
	d.fX = float(fRAND(CScreen::CX_SCREEN + x_size_half) - x_size_half);
	d.fY = -float(d.ySize) * SQRT_TWO; //just off top of screen at any rotation

	d.bActive = true;
}

void CTitleScreen::select_unique_custom_debris(DEBRIS& d) const
{
	const int rnd = RAND(100);
	if (rnd < 10) {
		d.type = DT_NON_SPINNING;
	} else if (rnd < 25) {
		d.type = DT_SPINNING;
	} else {
		d.type = DT_SPRITE;
	}

	set<UINT> candidate_types;
	size_t i;

	switch (d.type) {
		case DT_SPINNING:
			for (i=0; i<TSS_DEBRIS_SPINNING; ++i)
				candidate_types.insert(i);
		break;
		case DT_NON_SPINNING:
			for (i=0; i<TSS_DEBRIS_NON_SPINNING; ++i)
				candidate_types.insert(i);
		break;
		default: break;
	}

	//Cull any currently displayed images.
	for (DEBRISLIST::const_iterator iter=debris.begin(); iter!=debris.end(); ++iter)
	{
		DEBRIS& d_comp = *(*iter);
		if (d_comp.bActive && d_comp.type == d.type) {
			candidate_types.erase(d_comp.type_num);
		}
	}

	if (candidate_types.empty())
		d.type = DT_SPRITE; //default

	if (d.type == DT_SPRITE) {
		d.type_num = RAND(TSS_DEBRIS_TILE_TYPES); //no uniqueness required
	} else {
		const UINT index = RAND(candidate_types.size());
		set<UINT>::const_iterator iter = candidate_types.begin();
		for (i=0; i<index; ++i)
			++iter;
		d.type_num = *iter;
	}
}

void CTitleScreen::sort_debris_by_distance()
{
	std::sort(debris.begin(), debris.end(), CompareDebrisPtr());
}

void CTitleScreen::updateDebris(SDL_Surface *pDestSurface)
{
	ASSERT(pDestSurface);

	const Uint32 dwNow = SDL_GetTicks();
	static Uint32 dwTimeOfLastMove = dwNow;
	Uint32 dwFrameTime = dwNow <= dwTimeOfLastMove ? 1 : dwNow - dwTimeOfLastMove;
	static const Uint32 MAX_FRAME_TIME = 50;
	const float fMultiplier = (dwFrameTime < MAX_FRAME_TIME ?
		dwFrameTime : MAX_FRAME_TIME) / 10.0f; //larger divisor makes them slower
	dwTimeOfLastMove = dwNow;

	for (DEBRISLIST::iterator iter=debris.begin(); iter!=debris.end(); ++iter)
	{
		DEBRIS& d = *(*iter);

		if (!d.bActive)
			resetDebris(d);

		//Out of bounds
		if (d.fY > float(CScreen::CY_SCREEN) + d.ySize)
			resetDebris(d);

		if (d.type != DT_NON_SPINNING)
			d.fAngle += d.angularRotation * fMultiplier;

		const bool custom_image = d.type != DT_SPRITE;

		//Closer objects fall more quickly.
		const float y_delta = fMultiplier * d.distance_multiplier * (custom_image ? 3.0f : 1.0f);
		d.fY += y_delta;

		//If sprite won't be displayed on screen at any rotation, then do nothing.
		const float max_tile_image_height = float(d.ySize) * SQRT_TWO;
		if (d.fY < -max_tile_image_height || d.fY >= CScreen::CY_SCREEN + max_tile_image_height)
		{
			d.clear_damaged();
			continue;
		}

		SDL_Surface *pSrcSurface, *pScaledSurface = NULL;
		Uint8 *pSrcPixel;
		SDL_Rect srcRect = {0,0,0,0};
		float fRotAngle;

		if (custom_image) {
			pSrcSurface = d.pScaledSurface;
			fRotAngle = d.fAngle;
		} else {
			//Sprite tile.

			//Set angle and determine image by orientation.
			static const UINT ROTATION_FRAMES = 8;
			static const float fOrientationInterval = TWOPI / (float)ROTATION_FRAMES;
			static const float fHalfOrientation = fOrientationInterval / 2.0f;
			d.fAngle += fHalfOrientation; //center angle on orientation
			while (d.fAngle < 0.0)
				d.fAngle += TWOPI;
			while (d.fAngle >= TWOPI)
				d.fAngle -= TWOPI;
			UINT wOrientation = static_cast<UINT>(d.fAngle * ROTATION_FRAMES / TWOPI);
			d.fAngle -= fHalfOrientation;

			//Determine how far off sprite orientation is from real angle.
			fRotAngle = (float)wOrientation * fOrientationInterval - d.fAngle;
			ASSERT(fabs(fRotAngle) < fOrientationInterval); //Should be less than fHalfOrientation, but can't guarantee due to float precision

			while (wOrientation >= ROTATION_FRAMES)
				wOrientation -= ROTATION_FRAMES;
			ASSERT(wOrientation < ROTATION_FRAMES);

			static const UINT frame[TSS_DEBRIS_TILE_TYPES][ROTATION_FRAMES] = {
				{TI_ROACH_E, TI_ROACH_SE, TI_ROACH_S, TI_ROACH_SW, TI_ROACH_W, TI_ROACH_NW, TI_ROACH_N, TI_ROACH_NE},
				{TI_QROACH_E, TI_QROACH_SE, TI_QROACH_S, TI_QROACH_SW, TI_QROACH_W, TI_QROACH_NW, TI_QROACH_N, TI_QROACH_NE}
			};
			const UINT wTileNo = frame[d.type_num][wOrientation];

			pSrcSurface = g_pTheDBM->GetTileSurface(wTileNo);
			pSrcPixel = g_pTheDBM->GetTileSurfacePixel(wTileNo);

			//Set to transparent tile colorkey.
			const UINT wSurfaceIndex = g_pTheDBM->GetTileSurfaceNumber(wTileNo);
			g_pTheBM->SetSurfaceColorKey(wTileNo, wSurfaceIndex, pSrcSurface);

			//Scale based on distance.
			pScaledSurface = g_pTheDBM->ScaleSurface(pSrcSurface, pSrcPixel,
					CDrodBitmapManager::CX_TILE, CDrodBitmapManager::CY_TILE,
					d.xSize, d.ySize);
			if (!pScaledSurface) {
				d.clear_damaged();
				continue;
			}

			pSrcSurface = pScaledSurface;
		}

		//Minimize blitted area.
		srcRect.w = pSrcSurface->w;
		srcRect.h = pSrcSurface->h;

		g_pTheBM->CropToOpaque(srcRect, pSrcSurface);
		pSrcPixel = (Uint8*)pSrcSurface->pixels + srcRect.y * pSrcSurface->pitch +
				srcRect.x * pSrcSurface->format->BytesPerPixel;

		//Draw rotated.
		SDL_Surface *pRotatedSurface = g_pTheDBM->RotateSurface(pSrcSurface, pSrcPixel,
				srcRect.w, srcRect.h, fRotAngle * RADIANS_TO_DEGREES);
		if (pScaledSurface)
			SDL_FreeSurface(pScaledSurface);
		if (!pRotatedSurface) {
			d.clear_damaged();
			continue;
		}

		//Handle partial blits at screen edge.
		UINT cx = pRotatedSurface->w, cy = pRotatedSurface->h;
		float fX = d.fX + float((int)d.xSize - (int)cx) / 2.0f; //dest of rotated image (keep centered)
		float fY = d.fY + float((int)d.ySize - (int)cy) / 2.0f;
		UINT x = static_cast<UINT>(fX), y = static_cast<UINT>(fY);
		UINT xSource = 0, ySource = 0;
		//Blit partial sprite at left/top edge.
		if (fX < 0.0)
		{
			x = 0;
			xSource = UINT(-fX);
			if (xSource >= cx) {
				d.clear_damaged();
				SDL_FreeSurface(pRotatedSurface);
				continue; //out of view
			}
			cx -= xSource;
		}
		if (fY < 0.0)
		{
			y = 0;
			ySource = UINT(-fY);
			if (ySource >= cy) {
				d.clear_damaged();
				SDL_FreeSurface(pRotatedSurface);
				continue; //out of view
			}
			cy -= ySource;
		}

		SDL_Rect src = MAKE_SDL_RECT(xSource, ySource, cx, cy);
		SDL_Rect dest = MAKE_SDL_RECT(x, y, cx, cy);
		g_pTheDBM->BlitSurface(pRotatedSurface, &src, pDestSurface, &dest);

		if (!custom_image) {
			SDL_Rect dest = MAKE_SDL_RECT(x, y, cx, cy); //need to reset to original size for darkening
			g_pTheDBM->DarkenWithMask(pRotatedSurface, src,
				pDestSurface, dest, CBitmapManager::fLightLevel, true);
		}

		SDL_FreeSurface(pRotatedSurface);

		d.damaged = dest;
	}

	sort_debris_by_distance();
}

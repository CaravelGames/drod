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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "DemosScreen.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"
#include "DrodSound.h"
#include "BrowserScreen.h"
#include "DemoScreen.h"
#include "RoomWidget.h"

#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/OptionButtonWidget.h>
#include <FrontEndLib/ScalerWidget.h>
#include <FrontEndLib/ButtonWidget.h>

#include "../DRODLib/CueEvents.h"
#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbXML.h"
#include "../DRODLib/SettingsKeys.h"
#include "../Texts/MIDs.h"

#include <BackEndLib/Assert.h>
#include <BackEndLib/Base64.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Ports.h>

//#define ENABLE_CHEATS

//Widget tag constants.
const UINT TAG_WATCH = 1000;
const UINT TAG_DELETE = 1001;
const UINT TAG_HELP = 1002;
const UINT TAG_RETURN = 1003;
const UINT TAG_DEMO_LBOX = 1004;
const UINT TAG_CNETDEMO_LBOX = 1005;
const UINT TAG_DISPLAYOPTION = 1006;
const UINT TAG_EXPORT = 1007;
const UINT TAG_EXPORT_ALL = 1008;
const UINT TAG_IMPORT = 1009;
const UINT TAG_SHOWLEVEL = 1010;
const UINT TAG_DOWNLOAD = 1011;
const UINT TAG_DELETEALL = 1012;
const UINT TAG_MINIROOM = 1013;
const UINT TAG_UPLOAD = 1014;

const UINT MAXLEN_ITEMTEXT = 100;

//
//Protected methods.
//

//************************************************************************************
CDemosScreen::CDemosScreen() : CDrodScreen(SCR_Demos)
	, pDemoListBoxWidget(NULL), pCNetDemoListBoxWidget(NULL)
	, pRoomWidget(NULL)
	, pDemoCurrentGame(NULL)
	, pAuthorWidget(NULL), pCreatedWidget(NULL), pDurationWidget(NULL)
	, pDescriptionWidget(NULL)
	, pLBoxHeaderWidget(NULL), pNoDemoWidget(NULL)
	, pDetailsFrame(NULL)
	, dwRoomID(0), dwLevelID(0)
	, wDemoHandle(0)
	, bHoldPublished(false), bDownloadDemos(false), bIsAuthor(false)
	, bViewRecordsOnly(true)
	, dwDownloadDemosForRoomID(0)
	, bRetainDemos(false)
//Constructor.
{
	SetKeyRepeat(66);

	this->imageFilenames.push_back(string("Background"));

	static const UINT CX_SPACE = 8;
	static const UINT CY_SPACE = 8;
	static const UINT CY_TITLE = CY_LABEL_FONT_TITLE;
	static const UINT CY_TITLE_SPACE = 14;
	static const int Y_TITLE = CY_TITLE_SPACE;

	//Buttons
	static const UINT CY_SHOWLEVEL = CY_STANDARD_OPTIONBUTTON;
	const int Y_SHOWLEVEL = this->h - CY_SHOWLEVEL - CY_STANDARD_OPTIONBUTTON - CY_SPACE - 3;
	static const UINT CX_SHOWLEVEL = 400;
	static const int X_SHOWLEVEL = CX_SPACE;

	static const UINT CY_WATCH = CY_STANDARD_BUTTON;
	const int Y_WATCH = Y_SHOWLEVEL + CY_SHOWLEVEL;
	static const UINT CX_WATCH = 90;
	static const int X_WATCH = CX_SPACE;

	static const int X_HELP_BUTTON = X_WATCH + CX_WATCH + CX_SPACE;
	const int Y_HELP_BUTTON = Y_WATCH;
	static const UINT CX_HELP_BUTTON = CX_WATCH;
	static const UINT CY_HELP_BUTTON = CY_WATCH;

	static const int X_DELETE = X_HELP_BUTTON + CX_HELP_BUTTON + CX_SPACE;
	const int Y_DELETE = Y_WATCH;
	static const UINT CX_DELETE = CX_WATCH;
	static const UINT CY_DELETE = CY_WATCH;

	static const int X_DELETEALL = X_DELETE + CX_DELETE + CX_SPACE;
	const int Y_DELETEALL = Y_WATCH;
	static const UINT CX_DELETEALL = 100;
	static const UINT CY_DELETEALL = CY_WATCH;

	static const int X_EXPORT = X_DELETEALL + CX_DELETEALL + CX_SPACE;
	const int Y_EXPORT = Y_WATCH;
	static const UINT CX_EXPORT = CX_WATCH;
	static const UINT CY_EXPORT = CY_WATCH;

	static const int X_EXPORT_ALL = X_EXPORT + CX_EXPORT + CX_SPACE;
	static const UINT CX_EXPORT_ALL = 110;

	static const int X_IMPORT = X_EXPORT_ALL + CX_EXPORT_ALL + CX_SPACE;
	const int Y_IMPORT = Y_WATCH;
	static const UINT CX_IMPORT = CX_EXPORT;
	static const UINT CY_IMPORT = CY_WATCH;

	static const int X_DOWNLOAD = X_IMPORT + CX_IMPORT + CX_SPACE;
	const int Y_DOWNLOAD = Y_WATCH;
	static const UINT CX_DOWNLOAD = 100;
	static const UINT CY_DOWNLOAD = CY_WATCH;

	static const UINT CX_RETURN = 160;
	const int X_RETURN = this->w - CX_SPACE - CX_RETURN;
	const int Y_RETURN = Y_WATCH;
	static const UINT CY_RETURN = CY_WATCH;

	static const UINT CY_UPLOAD = CY_STANDARD_BUTTON;
	const int Y_UPLOAD = Y_TITLE + CY_TITLE;
	static const UINT CX_UPLOAD = 90;

	//List boxes.
	static const UINT CY_LBOX_HEADER = 24;
	const UINT CX_LBOX_HEADER = 410;
	static const int X_LBOX_HEADER = CX_SPACE;
	static const int Y_LBOX_HEADER = Y_TITLE + CY_TITLE + CY_TITLE_SPACE;

	static const int X_UPLOAD = X_LBOX_HEADER + CX_LBOX_HEADER;

	const UINT CX_DEMO_LBOX = ((this->w - CX_SPACE) / 2) - CX_SPACE;
	static const int X_DEMO_LBOX = X_LBOX_HEADER;
	static const UINT CY_DEMO_LBOX = 378;
	static const int Y_DEMO_LBOX = Y_LBOX_HEADER + CY_LBOX_HEADER;

	const UINT CX_CNET_HEADER = CX_DEMO_LBOX;
	static const UINT CY_CNET_HEADER = CY_LBOX_HEADER;
	static const int X_CNET_HEADER = X_LBOX_HEADER;
	static const int Y_CNET_HEADER = Y_DEMO_LBOX + CY_DEMO_LBOX;

	static const UINT CX_CNETDEMO_LBOX = CX_DEMO_LBOX;
	static const int X_CNETDEMO_LBOX = X_DEMO_LBOX;
	static const UINT CY_CNETDEMO_LBOX = 180;
	static const int Y_CNETDEMO_LBOX = Y_CNET_HEADER + CY_CNET_HEADER;

	static const UINT CX_DETAILS_FRAME = CX_DEMO_LBOX;
	static const int Y_DETAILS_FRAME = Y_LBOX_HEADER;
	const UINT CY_DETAILS_FRAME = this->h - CY_SPACE - CY_STANDARD_BUTTON - CY_SPACE -
			Y_DETAILS_FRAME;
	static const int X_DETAILS_FRAME = X_DEMO_LBOX + CX_DEMO_LBOX + CX_SPACE;

	//Details frame widgets.
	static const UINT CX_MINIROOM = CX_DETAILS_FRAME - CX_SPACE - CX_SPACE;
	const UINT CY_MINIROOM = (CX_MINIROOM * CDrodBitmapManager::CY_ROOM) /
			CDrodBitmapManager::CX_ROOM;
	static const int X_MINIROOM = CX_SPACE;
	const int Y_MINIROOM = CY_DETAILS_FRAME - CY_SPACE - CY_MINIROOM;

	static const int X_AUTHOR_LABEL = X_MINIROOM;
	static const int Y_AUTHOR_LABEL = CY_SPACE;
	static const UINT CX_AUTHOR_LABEL = 100;
	static const UINT CY_AUTHOR_LABEL = 25;

	static const int X_AUTHOR = X_AUTHOR_LABEL + CX_AUTHOR_LABEL;
	static const int Y_AUTHOR = Y_AUTHOR_LABEL;
	static const UINT CX_AUTHOR = CX_DETAILS_FRAME - CX_SPACE - X_AUTHOR_LABEL - CX_AUTHOR_LABEL;
	static const UINT CY_AUTHOR = CY_AUTHOR_LABEL;

	static const int X_CREATED_LABEL = X_AUTHOR_LABEL;
	static const UINT CX_CREATED_LABEL = CX_AUTHOR_LABEL;
	static const UINT CY_CREATED_LABEL = CY_AUTHOR_LABEL;
	static const int Y_CREATED_LABEL = Y_AUTHOR_LABEL + CY_AUTHOR_LABEL;

	static const int X_CREATED = X_AUTHOR;
	static const int Y_CREATED = Y_CREATED_LABEL;
	static const UINT CX_CREATED = CX_AUTHOR;
	static const UINT CY_CREATED = CY_CREATED_LABEL;

	static const int X_DURATION_LABEL = X_AUTHOR_LABEL;
#ifdef RUSSIAN_BUILD
	static const UINT CX_DURATION_LABEL = 180;
	static const UINT CX_DURATION = CX_DETAILS_FRAME - CX_SPACE - X_DURATION_LABEL - CX_DURATION_LABEL;
#else
	static const UINT CX_DURATION_LABEL = CX_AUTHOR_LABEL;
	static const UINT CX_DURATION = CX_AUTHOR;
#endif
	static const UINT CY_DURATION_LABEL = CY_AUTHOR_LABEL;
	static const int Y_DURATION_LABEL = Y_CREATED_LABEL + CY_CREATED_LABEL;

	static const int X_DURATION = X_DURATION_LABEL + CX_DURATION_LABEL;
	static const int Y_DURATION = Y_DURATION_LABEL;
	static const UINT CY_DURATION = CY_DURATION_LABEL;

	static const int X_DESC_LABEL = X_AUTHOR_LABEL;
	static const UINT CX_DESC_LABEL = CX_AUTHOR_LABEL;
	static const UINT CY_DESC_LABEL = CY_AUTHOR_LABEL;
	static const int Y_DESC_LABEL = Y_DURATION_LABEL + CY_DURATION_LABEL;

	static const int X_DESC = X_AUTHOR;
	static const int Y_DESC = Y_DESC_LABEL;
	static const UINT CX_DISPLAYOPTION = CX_AUTHOR;
	static const UINT CY_DISPLAYOPTION = CY_STANDARD_OPTIONBUTTON;
	static const UINT CX_DESC = CX_AUTHOR;
	const UINT CY_DESC = Y_MINIROOM - CY_DISPLAYOPTION - Y_DESC;
	static const int X_DISPLAYOPTION = X_AUTHOR_LABEL;
	const int Y_DISPLAYOPTION = Y_DESC_LABEL + CY_DESC;

	static const int X_NODEMO_LABEL = X_AUTHOR_LABEL;
	static const int Y_NODEMO_LABEL = Y_AUTHOR_LABEL;
	static const UINT CX_NODEMO_LABEL = CX_DETAILS_FRAME - CX_SPACE - CX_SPACE;
	static const UINT CY_NODEMO_LABEL = CY_AUTHOR_LABEL;

	//
	//Add widgets to screen.
	//

	//Title.
	CLabelWidget *pTitle = new CLabelWidget(0L, 0, Y_TITLE,
			this->w, CY_TITLE, F_Title, g_pTheDB->GetMessageText(MID_Demos));
	pTitle->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(pTitle);

	//Demo list box.
	this->pLBoxHeaderWidget = new CLabelWidget(0L, X_LBOX_HEADER,
			Y_LBOX_HEADER, CX_LBOX_HEADER, CY_LBOX_HEADER, F_Small, wszEmpty);
	AddWidget(this->pLBoxHeaderWidget);
	this->pDemoListBoxWidget = new CListBoxWidget(TAG_DEMO_LBOX,
			X_DEMO_LBOX, Y_DEMO_LBOX, CX_DEMO_LBOX, CY_DEMO_LBOX);
	AddWidget(this->pDemoListBoxWidget);

	//CaravelNet demo list box.
	AddWidget(new CLabelWidget(0L, X_CNET_HEADER,
			Y_CNET_HEADER, CX_CNET_HEADER, CY_CNET_HEADER, F_Small,
			g_pTheDB->GetMessageText(MID_ForumDemos)));
	this->pCNetDemoListBoxWidget = new CListBoxWidget(TAG_CNETDEMO_LBOX,
			X_CNETDEMO_LBOX, Y_CNETDEMO_LBOX, CX_CNETDEMO_LBOX, CY_CNETDEMO_LBOX);
	this->pDemoListBoxWidget->SelectMultipleItems(true);
	AddWidget(this->pCNetDemoListBoxWidget);

	//Details frame.
	this->pDetailsFrame = new CFrameWidget(0L, X_DETAILS_FRAME,
			Y_DETAILS_FRAME, CX_DETAILS_FRAME, CY_DETAILS_FRAME,
			g_pTheDB->GetMessageText(MID_Details));
	AddWidget(this->pDetailsFrame);

	//Details frame widgets.
	this->pDetailsFrame->AddWidget(new CLabelWidget(0L, X_AUTHOR_LABEL, Y_AUTHOR_LABEL,
					CX_AUTHOR_LABEL, CY_AUTHOR_LABEL, F_Small,
					g_pTheDB->GetMessageText(MID_Author)));
	this->pAuthorWidget = new CLabelWidget(0L, X_AUTHOR, Y_AUTHOR,
			CX_AUTHOR, CY_AUTHOR, F_Small, wszEmpty);
	this->pDetailsFrame->AddWidget(this->pAuthorWidget);
	this->pDetailsFrame->AddWidget(new CLabelWidget(0L, X_CREATED_LABEL, Y_CREATED_LABEL,
					CX_CREATED_LABEL, CY_CREATED_LABEL, F_Small,
					g_pTheDB->GetMessageText(MID_Created)));
	this->pCreatedWidget = new CLabelWidget(0L, X_CREATED, Y_CREATED,
			CX_CREATED, CY_CREATED, F_Small, wszEmpty);
	this->pDetailsFrame->AddWidget(this->pCreatedWidget);
	this->pDetailsFrame->AddWidget(new CLabelWidget(0L, X_DURATION_LABEL, Y_DURATION_LABEL,
					CX_DURATION_LABEL, CY_DURATION_LABEL, F_Small,
					g_pTheDB->GetMessageText(MID_Duration)));
	this->pDurationWidget = new CLabelWidget(0L, X_DURATION, Y_DURATION,
			CX_DURATION, CY_DURATION, F_Small, wszEmpty);
	this->pDetailsFrame->AddWidget(this->pDurationWidget);
	this->pDetailsFrame->AddWidget(new CLabelWidget(0L, X_DESC_LABEL, Y_DESC_LABEL,
					CX_DESC_LABEL, CY_DESC_LABEL, F_Small,
					g_pTheDB->GetMessageText(MID_Description)));
	this->pDescriptionWidget = new CLabelWidget(0L, X_DESC, Y_DESC,
			CX_DESC, CY_DESC, F_Small, wszEmpty);
	this->pDetailsFrame->AddWidget(this->pDescriptionWidget);

	this->pShowButton = new COptionButtonWidget(
			TAG_DISPLAYOPTION, X_DISPLAYOPTION, Y_DISPLAYOPTION, CX_DISPLAYOPTION,
			CY_DISPLAYOPTION, g_pTheDB->GetMessageText(MID_DisplayDemo), false);
	this->pDetailsFrame->AddWidget(this->pShowButton);

	CScalerWidget *pScaledRoomWidget = new CScalerWidget(TAG_MINIROOM,
			X_MINIROOM, Y_MINIROOM, CX_MINIROOM, CY_MINIROOM, false);
	this->pDetailsFrame->AddWidget(pScaledRoomWidget);
	this->pRoomWidget = new CRoomWidget(0L, 0, 0, CDrodBitmapManager::CX_ROOM,
			CDrodBitmapManager::CY_ROOM);
	pScaledRoomWidget->AddScaledWidget(this->pRoomWidget);

	this->pNoDemoWidget = new CLabelWidget(0L, X_NODEMO_LABEL,
			Y_NODEMO_LABEL, CX_NODEMO_LABEL, CY_NODEMO_LABEL, F_Small,
			g_pTheDB->GetMessageText(MID_NoDemoSpecified));
	this->pDetailsFrame->AddWidget(this->pNoDemoWidget);
	this->pNoDemoWidget->Hide();

	//Bottom buttons.
	this->pShowFromLevel = new COptionButtonWidget(
			TAG_SHOWLEVEL, X_SHOWLEVEL, Y_SHOWLEVEL, CX_SHOWLEVEL,
			CY_SHOWLEVEL,  g_pTheDB->GetMessageText(MID_DisplayLevelDemos), false);
	AddWidget(this->pShowFromLevel);

	CButtonWidget *pButton;
	pButton = new CButtonWidget(TAG_UPLOAD, X_UPLOAD, Y_UPLOAD,
			CX_UPLOAD, CY_UPLOAD, g_pTheDB->GetMessageText(MID_Upload));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_WATCH, X_WATCH, Y_WATCH,
			CX_WATCH, CY_WATCH, g_pTheDB->GetMessageText(MID_Watch));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_HELP, X_HELP_BUTTON, Y_HELP_BUTTON,
			CX_HELP_BUTTON, CY_HELP_BUTTON, g_pTheDB->GetMessageText(MID_Help));
	AddWidget(pButton);
	AddHotkey(SDLK_F1,TAG_HELP);

	pButton = new CButtonWidget(TAG_DELETE, X_DELETE, Y_DELETE,
			CX_DELETE, CY_DELETE, g_pTheDB->GetMessageText(MID_Delete));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_DELETEALL, X_DELETEALL, Y_DELETEALL,
			CX_DELETEALL, CY_DELETEALL, g_pTheDB->GetMessageText(MID_DeleteAll));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_EXPORT, X_EXPORT, Y_EXPORT,
		CX_EXPORT, CY_EXPORT, g_pTheDB->GetMessageText(MID_Export));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_EXPORT_ALL, X_EXPORT_ALL, Y_EXPORT,
		CX_EXPORT_ALL, CY_EXPORT, g_pTheDB->GetMessageText(MID_ExportAll));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_IMPORT, X_IMPORT, Y_IMPORT,
		CX_IMPORT, CY_IMPORT, g_pTheDB->GetMessageText(MID_Import));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_DOWNLOAD, X_DOWNLOAD, Y_DOWNLOAD,
			CX_DOWNLOAD, CY_DOWNLOAD, g_pTheDB->GetMessageText(MID_Download));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_RETURN, X_RETURN, Y_RETURN,
			CX_RETURN, CY_RETURN, g_pTheDB->GetMessageText(MID_ReturnToGame));
	AddWidget(pButton);
}

//************************************************************************************
CDemosScreen::~CDemosScreen()
{
	delete this->pDemoCurrentGame;
}

//************************************************************************************
bool CDemosScreen::IsCommandSupported(int command) const
{
	return command == CMD_EXTRA_EDITOR_DELETE;
}

//******************************************************************************
bool CDemosScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	//Retain prior screen state if requested.
	//This will avoid unneeded server hits and demo reconstruction delay.
	if (this->bRetainDemos)
	{
		this->bRetainDemos = false;
		if (this->pDemoCurrentGame) {
			this->pRoomWidget->LoadFromCurrentGame(this->pDemoCurrentGame);
			CCueEvents Ignored;
			this->pRoomWidget->DisplayPersistingImageOverlays(Ignored);
		}
		return true;
	}

	const CDbPackedVars settings = g_pTheDB->GetCurrentPlayerSettings();

	this->pLBoxHeaderWidget->SetText(wszEmpty);
	this->pAuthorWidget->SetText(wszEmpty);
	this->pCreatedWidget->SetText(wszEmpty);
	this->pDurationWidget->SetText(wszEmpty);
	this->pDescriptionWidget->SetText(wszEmpty);

	//Ensure room has been specified for display.
	ASSERT(this->dwRoomID);

	//Prepare display of CaravelNet room demos.
	this->pShowFromLevel->SetChecked(settings.GetVar(Settings::ShowDemosFromLevel, false));
	this->pCNetDemoListBoxWidget->Clear();	//call before PopulateDemoListBox sets widget states
	this->dwDownloadDemosForRoomID = 0;	//no downloaded demos are listed now

	this->pDemoListBoxWidget->Clear();	//reset list first
	PopulateDemoListBox();

	//Check whether this room is in a published hold.
	this->bDownloadDemos = this->bHoldPublished = false;
	const UINT dwHoldID = g_pTheDB->Rooms.GetHoldIDForRoom(this->dwRoomID);
	vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();
	//Local holds should never have something in it if CaravelNet holds is empty.
	ASSERT(!cNetMedia.empty() || g_pTheNet->GetLocalHolds().empty());

	if (!g_pTheNet->IsLocalHold(dwHoldID))  //hold is not listed on CaravelNet
	{
		this->pCNetDemoListBoxWidget->AddItem(0, g_pTheDB->GetMessageText(
				cNetMedia.empty() ? MID_NotConnected : MID_HoldNotPublished), true);
		return true;
	}

	//Only download demos when player has opted to do so.
	this->bDownloadDemos = settings.GetVar(Settings::ConnectToInternet, false);
	if (!this->bDownloadDemos)
	{
		this->pCNetDemoListBoxWidget->AddItem(0, g_pTheDB->GetMessageText(
				MID_NotConnected), true);
		return true;
	}

	//Get demos if player can watch them.
	this->bHoldPublished = true;
	this->bIsAuthor = g_pTheDB->GetPlayerID() == g_pTheDB->Holds.GetAuthorID(dwHoldID);
	DownloadRoomDemos(this->dwRoomID);

	{
		CDbPlayer* pPlayer = g_pTheDB->GetCurrentPlayer();
		InitKeysymToCommandMap(pPlayer->Settings);
		delete pPlayer;
	}

	return true;
}

//
//Private methods.
//

//*****************************************************************************
void CDemosScreen::AdvanceCurrentDemoTurn()
{
	AdvanceDemoPlayback(this->pDemoCurrentGame, this->pRoomWidget, TAG_MINIROOM);
}

//******************************************************************************
void CDemosScreen::DownloadRoomDemos(const UINT dwRoomID)
//Requests the recorded CaravelNet demos for the specified room.
{
	//Don't download demos if they were just downloaded for this room.
	if (dwRoomID && this->dwDownloadDemosForRoomID == dwRoomID)
		return;

	//New room -- clear any old room's demo list immediately in preparation for new room.
	ClearCNetDemos();

	//If hold is not published, or player doesn't want to download demos, then stop.
	if (!this->bHoldPublished) return;
	if (!this->bDownloadDemos) return;

	//Update CaravelNet demo list box with new room status.
	this->pCNetDemoListBoxWidget->Clear();
	if (!dwRoomID)
	{
		this->pCNetDemoListBoxWidget->AddItem(0, g_pTheDB->GetMessageText(
				MID_NoScoresForRoom), true);
		return;
	}

	//Only display CaravelNet demos when the player is the hold author or
	//the current room has been conquered.
	//Also, see beta hold demo stats viewing criteria below.
	this->bViewRecordsOnly = false;
#ifdef ENABLE_CHEATS
	const bool bDownloadCriteriaMet = true;
#else
	const bool bDownloadCriteriaMet = this->bIsAuthor ||
			g_pTheDB->SavedGames.FindByConqueredRoom(dwRoomID);
	if (!bDownloadCriteriaMet)
	{
		//Room demos in beta holds can have their stats viewed, but not downloaded.
		const UINT dwHoldID = g_pTheDB->Rooms.GetHoldIDForRoom(this->dwRoomID);
		this->bViewRecordsOnly = g_pTheNet->IsBetaHold(dwHoldID);
	}
#endif

	//Request to download demos (maybe only if room is marked conquered on server).
	this->pCNetDemoListBoxWidget->AddItem(0, g_pTheDB->GetMessageText(
			MID_DownloadingData), true);

	CNetRoom netRoom;
	g_pTheDB->Rooms.ExportNetRoom(dwRoomID, netRoom);

	this->wDemoHandle = g_pTheNet->DownloadRecords(netRoom,
			!bDownloadCriteriaMet && !this->bViewRecordsOnly);
	if (this->wDemoHandle)
		this->dwDownloadDemosForRoomID = dwRoomID;	//d/ling demos for this room
	else
	{
		//Download request was rejected -- user is not logged in.
		this->pCNetDemoListBoxWidget->Clear();
		this->pCNetDemoListBoxWidget->AddItem(0, g_pTheDB->GetMessageText(
				MID_MustRegisterToViewScores), true);
	}
}

//******************************************************************************
void CDemosScreen::DownloadSelectedDemo()
//Downloads and imports demo selected in CaravelNet list box.
{
	const UINT dwDemoID = this->pCNetDemoListBoxWidget->GetSelectedItem();
	if (!dwDemoID) return;
#ifndef ENABLE_CHEATS
	if (!this->pCNetDemoListBoxWidget->IsItemEnabled(dwDemoID)) return;
#endif

	//Download CaravelNet demo.
	CScreen::SetCursor(CUR_Internet);
	ShowStatusMessage(MID_DownloadingData);

	const UINT handle = g_pTheNet->DownloadDemo(dwDemoID);
	g_pTheDB->Commit();  //commit any outstanding changes while waiting for request
	if (!handle) {
		HideStatusMessage(); SetCursor(); return;
	}
	while (g_pTheNet->GetStatus(handle) < 0)
		SDL_Delay(20); // wait until received

	CNetResult *pBuffer = g_pTheNet->GetResults(handle);

	if (!pBuffer || !pBuffer->pJson) {
		// error - can't download!
		HideStatusMessage(); SetCursor(); return;
	}

	//Import selected CaravelNet demo.
	ShowStatusMessage(MID_ImportingData);
	string decodedDemo, encodedDemo = pBuffer->pJson->get("demo", "").asString();
	Base64::decode(encodedDemo, decodedDemo);
	CStretchyBuffer demoBuffer(decodedDemo);
	MESSAGE_ID result = CDbXML::ImportXML(demoBuffer, CImportInfo::Demo);
	if (!ImportConfirm(result)) {delete pBuffer; HideStatusMessage(); SetCursor(); return;}
	delete pBuffer;

	//Display result.
	HideStatusMessage();
	SetCursor();
	ShowOkMessage(result);


	this->pCNetDemoListBoxWidget->DisableItem(dwDemoID);
	PopulateDemoListBox();

	//Set room and other widgets to downloaded demo.
	this->pDemoListBoxWidget->SelectItem(CDbXML::info.dwDemoImportedID);
	SetWidgetsToDemo(this->pDemoListBoxWidget->GetSelectedItem());

	Paint();
}

//******************************************************************************
void CDemosScreen::DeleteAllDemos()
//Prompt user to delete all demos in list, and optionally in the hold.
{
	if (!this->pDemoListBoxWidget->GetItemCount()) return;

	UINT dwRet = ShowYesNoMessage(MID_ReallyDeleteAllDemos);
	if (dwRet == TAG_QUIT)
	{
		OnQuit();
		return;
	}
	if (dwRet != TAG_YES)
		return;

	//Delete the demos.
	for (UINT i=this->pDemoListBoxWidget->GetItemCount(); i--; )
		g_pTheDB->Demos.Delete(this->pDemoListBoxWidget->GetKeyAtLine(i));

	//Update widgets to show no demos.
	PopulateDemoListBox();
	ListCNetDemos();	//update state
	SetWidgetsToDemo(0);
	Paint();

	//Now that these demos are deleted, prompt to delete all other demos in hold too.
	dwRet = ShowYesNoMessage(MID_DeleteAllHoldDemosPrompt);
	if (dwRet == TAG_QUIT)
	{
		OnQuit();
		return;
	}
	if (dwRet != TAG_YES)
		return;

	//Get demos in this hold.
	//If player is the hold author, all demos are deleted.
	//Otherwise, only demos created by the player are deleted.
	const UINT dwPlayerID = g_pTheDB->GetPlayerID();
	const UINT dwHoldID = g_pTheDB->Rooms.GetHoldIDForRoom(this->dwRoomID);
	const bool bPlayerIsHoldAuthor = dwPlayerID == g_pTheDB->Holds.GetAuthorID(dwHoldID);
	CDb db;
	db.Demos.FilterByHold(dwHoldID);
	CDbDemo *pDemo = db.Demos.GetFirst();
	while (pDemo)
	{
		if (IsDemoAccessible(pDemo, dwPlayerID, bPlayerIsHoldAuthor))
			g_pTheDB->Demos.Delete(pDemo->dwDemoID);
		delete pDemo;
		pDemo = db.Demos.GetNext();
	}
}

//******************************************************************************
void CDemosScreen::DeleteDemo()
{
	const CIDSet demoIDs = this->pDemoListBoxWidget->GetSelectedItems();
	if (!demoIDs.getMax()) return; //no demos selected

	UINT dwRet = ShowYesNoMessage(MID_ReallyDeleteDemo);
	if (dwRet == TAG_QUIT)
	{
		OnQuit();
		return;
	}
	if (dwRet != TAG_YES)
		return;
	
	const UINT dwPlayerID = g_pTheDB->GetPlayerID();

	bool bAsk = true;
	for (CIDSet::const_iterator demo = demoIDs.begin();
			demo != demoIDs.end(); ++demo)
	{
		//If demo is part of a multi-room sequence, prompt to delete all trailing
		//demos in the sequence.
		bool bDeleteChildDemos = false;
		UINT dwDemoID = *demo, dwNextDemoID = 0;
		do {
			CDbDemo *pDemo = g_pTheDB->Demos.GetByID(dwDemoID);
			if (!pDemo) break;
			dwNextDemoID = pDemo->dwNextDemoID;
			if (dwNextDemoID && bAsk)
			{
				dwRet = ShowYesNoMessage(MID_DeleteChildDemosPrompt);
				if (dwRet == TAG_QUIT)
				{
					OnQuit();
					delete pDemo;
					return;
				}
				if (dwRet == TAG_YES)
					bDeleteChildDemos = true;
				bAsk = false;
			}

			//Hide demo only if its state represents a completed challenge for the active player.
			bool bHideDemo = false;
			if (pDemo->IsFlagSet(CDbDemo::CompletedChallenge) && pDemo->GetAuthorID() == dwPlayerID) {
				bHideDemo = true;
				pDemo->bIsHidden = true;
				pDemo->Update();
			}

			delete pDemo;
			
			//Delete the demo.
			//Update demo list to no longer show this demo.
			if (!bHideDemo)
				g_pTheDB->Demos.Delete(dwDemoID);
			this->pDemoListBoxWidget->RemoveItem(dwDemoID);

			dwDemoID = dwNextDemoID;
		} while (dwNextDemoID && bDeleteChildDemos);
	}
	this->pDemoListBoxWidget->SelectLine(0);

	ListCNetDemos();	//update state
	const UINT dwDemoID = this->pDemoListBoxWidget->GetSelectedItem();
	SetWidgetsToDemo(dwDemoID);
	Paint();
}

//*****************************************************************************
void CDemosScreen::ExportDemos()
//Export all demos in list/hold.
{
	const UINT dwRet = ShowYesNoMessage(MID_ExportAllHoldDemosPrompt);
	if (dwRet == TAG_QUIT)
	{
		OnQuit();
		return;
	}

	CIDSet demoIDs;
	WSTRING wstrExportFile;

	if (dwRet == TAG_YES)
	{
		//Gather all player-accessible demos in hold.
		const UINT dwHoldID = g_pTheDB->Rooms.GetHoldIDForRoom(this->dwRoomID);
		const UINT dwPlayerID = g_pTheDB->GetPlayerID();
		const bool bPlayerIsHoldAuthor = dwPlayerID == g_pTheDB->Holds.GetAuthorID(dwHoldID);
		CIDSet demosInHold = CDb::getDemosInHold(dwHoldID);
		for (CIDSet::const_iterator demo = demosInHold.begin(); demo != demosInHold.end(); ++demo)
		{
			CDbDemo *pDemo = g_pTheDB->Demos.GetByID(*demo);
			ASSERT(pDemo);
			if (IsDemoAccessible(pDemo, dwPlayerID, bPlayerIsHoldAuthor))
				demoIDs += pDemo->dwDemoID;
			delete pDemo;
		}

		if (demoIDs.empty())
		{
			ShowOkMessage(MID_NoDemosToExport);
			return;
		}

		CDbHold *pHold = g_pTheDB->Holds.GetByID(dwHoldID, true);
		ASSERT(pHold);
		wstrExportFile = (const WCHAR*)pHold->NameText;
		delete pHold;
	} else {
		//Gather demos in list box.
		if (this->pDemoListBoxWidget->IsEmpty())
		{
			ShowOkMessage(MID_NoDemosToExport);
			return;
		}
		for (UINT wIndex=0; wIndex<this->pDemoListBoxWidget->GetItemCount(); ++wIndex)
			demoIDs += this->pDemoListBoxWidget->GetKeyAtLine(wIndex);
		wstrExportFile = this->pDemoListBoxWidget->GetTextAtLine(0);
	}

	//Export demos.
	if (ExportSelectFile(MID_SaveDemoPath, wstrExportFile, EXT_DEMO))
	{
		if (CDbXML::ExportXML(V_Demos, demoIDs, wstrExportFile.c_str()))
			ShowOkMessage(MID_DemoFileSaved);
		else
			ShowOkMessage(MID_DemoFileNotSaved);
	}
}

//*****************************************************************************
bool CDemosScreen::IsDemoAccessible(
//Returns: whether this player has access to this demo record.
	CDbDemo* pDemo, const UINT dwPlayerID, const bool bPlayerIsHoldAuthor) const
{
#ifndef ENABLE_CHEATS
	ASSERT(pDemo);
	if (!pDemo)
		return false; //robustness
	if (pDemo->wShowSequenceNo)
	{
		const UINT dwAuthorID = pDemo->GetAuthorID();
		if (dwAuthorID != dwPlayerID && !bPlayerIsHoldAuthor)
		{
			//Show-demo made by another player.
			//Can't access this demo unless the hold is owned by the player.
			return false;
		}
	}
#endif
	return true;
}

//*****************************************************************************
void CDemosScreen::OnBetweenEvents()
//Called between events.
{
	if (this->wDemoHandle)
	{
		if (g_pTheNet->GetStatus(this->wDemoHandle) >= 0)
		{
			CNetResult* pResults = g_pTheNet->GetResults(this->wDemoHandle);
			this->wDemoHandle = 0;	//only try to get once
			if (pResults) {
				GetCNetDemos(pResults);
				delete pResults;
			}
		} else {
			g_pTheDB->Commit(); //commit prior key updates while waiting
		}
	}

	AdvanceCurrentDemoTurn();
}

//******************************************************************************
void CDemosScreen::OnClick(
//Handles click events.
//
//Params:
	const UINT dwTagNo)                      //(in) Widget receiving event.
{
	switch (dwTagNo)
	{
		case TAG_HELP:
			CBrowserScreen::SetPageToLoad("demos.html");
			GoToScreen(SCR_Browser);
		break;

		case TAG_WATCH:
		case TAG_MINIROOM:
		{
			const UINT dwDemoID = this->pDemoListBoxWidget->GetSelectedItem();
			if (!dwDemoID) break;
			CDemoScreen *pDemoScreen = DYN_CAST(CDemoScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_Demo));
			pDemoScreen->SetReplayOptions(true);
			if (!pDemoScreen->LoadDemoGame(dwDemoID))
				break;
			GoToScreen(SCR_Demo);
			this->bRetainDemos = true;	//don't reload lists when coming back to this screen
		}
		break;

		case TAG_DELETE:
			DeleteDemo();
		break;

		case TAG_DELETEALL:
			DeleteAllDemos();
		break;

		case TAG_EXPORT:
		{
			const CIDSet demoIDs = this->pDemoListBoxWidget->GetSelectedItems();
			if (!demoIDs.getMax())
				break; //no demos selected

			CDbDemo *pDemo = g_pTheDB->Demos.GetByID(demoIDs.getFirst());
			if (pDemo)
			{
				//Default filename is demo description.
				WSTRING wstrExportFile = demoIDs.size() == 1 ?
					(WSTRING)pDemo->DescriptionText : GetSelectedDemosDescription(demoIDs, pDemo);
				if (ExportSelectFile(MID_SaveDemoPath, wstrExportFile, EXT_DEMO))
				{
					//Write the demo file.
					if (CDbXML::ExportXML(V_Demos, demoIDs, wstrExportFile.c_str()))
						ShowOkMessage(MID_DemoFileSaved);
					else
						ShowOkMessage(MID_DemoFileNotSaved);
				}
				delete pDemo;
			}
		}
		break;

		case TAG_EXPORT_ALL:
			ExportDemos();
		break;

		case TAG_IMPORT:
		{
			//Import demo data file(s).
			CIDSet importedIDs;
			set<WSTRING> importedStyles;
			Import(EXT_DEMO, importedIDs, importedStyles);
			if (CDbXML::WasImportSuccessful() && !importedIDs.empty())
			{
				//Update in case a demo was added.
				PopulateDemoListBox();
				Paint();
			}
			ASSERT(importedStyles.empty());
		}
		break;

		case TAG_RETURN:
			ResetRoom();
			Deactivate();
		break;

		case TAG_DISPLAYOPTION:
		{
			//Toggle demo display at title screen.
			const UINT dwDemoID = this->pDemoListBoxWidget->GetSelectedItem();
			CDbDemo *pDemo = g_pTheDB->Demos.GetByID(dwDemoID);
			ASSERT(pDemo);
			if (this->pShowButton->IsChecked())
			{
				pDemo->wShowSequenceNo = g_pTheDB->Demos.GetNextSequenceNo();
				pDemo->Update();
			}
			else
				g_pTheDB->Demos.RemoveShowSequenceNo(pDemo->wShowSequenceNo);
			delete pDemo;
		}
		break;

		case TAG_DOWNLOAD:
			DownloadSelectedDemo();
		break;

		case TAG_UPLOAD:
			UploadSelectedDemo();
		break;

		//Give focus affordances.
		case TAG_DEMO_LBOX:
		{
			CWidget *pButton = GetWidget(TAG_DOWNLOAD);
			pButton->Disable();
			pButton->Paint();

			const bool bLocalDemoSelected = this->pDemoListBoxWidget->GetSelectedItem() != 0;
			pButton = GetWidget(TAG_WATCH);
			pButton->Enable(bLocalDemoSelected);
			pButton->Paint();
			pButton = GetWidget(TAG_DELETE);
			pButton->Enable(bLocalDemoSelected);
			pButton->Paint();
			pButton = GetWidget(TAG_EXPORT);
			pButton->Enable(bLocalDemoSelected);
			pButton->Paint();
		}
		break;
		case TAG_CNETDEMO_LBOX:
		{
			CWidget *pButton = GetWidget(TAG_WATCH);
			pButton->Disable();
			pButton->Paint();
			pButton = GetWidget(TAG_DELETE);
			pButton->Disable();
			pButton->Paint();
			pButton = GetWidget(TAG_EXPORT);
			pButton->Disable();
			pButton->Paint();

			pButton = GetWidget(TAG_DOWNLOAD);
			const UINT dwID = this->pCNetDemoListBoxWidget->GetSelectedItem();
#ifdef ENABLE_CHEATS
			pButton->Enable(dwID != 0);
#else
			pButton->Enable(dwID != 0 && this->pCNetDemoListBoxWidget->IsItemEnabled(dwID));
#endif
			pButton->Paint();
		}
		break;
	}
}

//*****************************************************************************
void CDemosScreen::OnDeactivate()
{
	if (this->wDemoHandle) {
#ifdef DEBUG_CINTERNET
		char buffer[16];
		CFiles files;
		files.AppendErrorLog("Canceling request...");
		files.AppendErrorLog(itoa(this->wDemoHandle, buffer, 10));
#endif
		CInternet::CancelRequest(this->wDemoHandle);
	}
}

//*****************************************************************************
void CDemosScreen::OnDoubleClick(const UINT dwTagNo)
{
	switch (dwTagNo)
	{
		case TAG_DEMO_LBOX:
			if (this->pDemoListBoxWidget->ClickedSelection())
				RenameDemo();
		break;

		case TAG_CNETDEMO_LBOX:
			if (this->pCNetDemoListBoxWidget->ClickedSelection())
				DownloadSelectedDemo();
		break;
	}
}

//*****************************************************************************
void CDemosScreen::OnKeyDown(
//Handles a keydown event.
//
//Params:
	const UINT dwTagNo, const SDL_KeyboardEvent &Key)
{
	CScreen::OnKeyDown(dwTagNo, Key);

	switch (Key.keysym.sym)
	{
		case SDLK_DELETE:
		{
			CWidget *pWidget = GetSelectedWidget();
			switch (pWidget->GetTagNo())
			{
				case TAG_DEMO_LBOX:
					DeleteDemo();
				break;
			}
		}
		break;

		default: break;
	}
}

//******************************************************************************
void CDemosScreen::OnSelectChange(
//Handles selection changes.
//
//Params:
	const UINT dwTagNo) //(in)   Widget affected by event.
{
	switch (dwTagNo)
	{
		case TAG_DEMO_LBOX:
		{
			const UINT dwDemoID = this->pDemoListBoxWidget->GetSelectedItem();
			if (dwDemoID)
			{
				SetWidgetsToDemo(dwDemoID);
				Paint();
			}
		}
		break;

		case TAG_CNETDEMO_LBOX:
		{
			CButtonWidget *pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_DOWNLOAD));
			const UINT dwID = this->pCNetDemoListBoxWidget->GetSelectedItem();
			pButton->Enable(dwID != 0 && this->pCNetDemoListBoxWidget->IsItemEnabled(dwID));
			pButton->RequestPaint();
		}
		break;

		case TAG_SHOWLEVEL:
		{
			PopulateDemoListBox();
			Paint();

			//Update view preference in player settings.
			CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
			if (pCurrentPlayer)
			{
				pCurrentPlayer->Settings.SetVar(Settings::ShowDemosFromLevel,
					this->pShowFromLevel->IsChecked());
				pCurrentPlayer->Update();
				delete pCurrentPlayer;
			}
		}
		break;

		default: break;
	}
}

//************************************************************************************
bool CDemosScreen::GetItemTextForDemo(
//Gets text used to summarize one demo in the list box.
//
//Params:
	UINT dwDemoID,      //(in)   Demo to summarize.
	WSTRING &wstrText,   //(out)  Receives item text.
	UINT &dwRoomID)		//(out)  Room demo is played in.
//
//Returns:
//True if demo is not hidden, false if demo is hidden.
const
{
	ASSERT(dwDemoID);
	dwRoomID = 0;

	//Get the demo.
	CDbDemo *pDemo = g_pTheDB->Demos.GetByID(dwDemoID);
	if (!pDemo) return false;
	if (pDemo->bIsHidden) {delete pDemo; return false;}

	//Get saved game for demo.
	CDbSavedGame *pSavedGame = g_pTheDB->SavedGames.GetByID(pDemo->dwSavedGameID);
	if (!pSavedGame) {delete pDemo; return false;}

	//Copy date/time to beginning of item.
	CDate date = pSavedGame->Created;
	CDbPlayer* pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	if (pCurrentPlayer)
	{
		date.SetDateFormat((CDate::DateFormat)pCurrentPlayer->Settings.GetVar(Settings::DemoDateFormat, CDate::MDY));
		delete pCurrentPlayer;
	}
	date.GetLocalFormattedText(DF_SHORT_DATE | DF_SHORT_TIME, wstrText);
	dwRoomID = pSavedGame->dwRoomID;
	delete pSavedGame;

	//If this demo belongs to a different player than the active one, say whose it is.
	if (pDemo->GetAuthorID() != g_pTheDB->GetPlayerID())
	{
		wstrText += wszSpace;
		wstrText += pDemo->GetAuthorText();
	}

	//Copy description to item.
	const WCHAR *pwczDescription = pDemo->DescriptionText;
	UINT wDescriptionLen = WCSlen(pwczDescription);
	if (wDescriptionLen)
	{
		wstrText += wszSpace;
		wstrText += wszQuote;
		if (wstrText.size() + wDescriptionLen + 1 /* """ */ > MAXLEN_ITEMTEXT)
		{
			//Append truncated description.
			UINT wCopyLen = MAXLEN_ITEMTEXT - wstrText.size() - 4; // "...""
			wstrText.append(pwczDescription, wCopyLen);
			wstrText += wszElipsis;
			wstrText += wszQuote;
		}
		else
		{
			//Append full description.
			wstrText += pwczDescription;
			wstrText += wszQuote;
		}
	}

	delete pDemo;
	return true;
}

//*****************************************************************************
WSTRING CDemosScreen::GetSelectedDemosDescription(
// Gets text to describe a group of demos. Assumes they're all in the same level.
// If something goes wrong, it returns the description of pDemo
	const CIDSet demoIDs,
	const CDbDemo* pDemo) const
{
	ASSERT(pDemo);
	//Get the level and hold IDs
	UINT roomID = CDbSavedGames::GetRoomIDofSavedGame(pDemo->dwSavedGameID);
	UINT levelID = CDbRooms::GetLevelIDForRoom(roomID);
	UINT holdID = CDbRooms::GetHoldIDForRoom(roomID);

	//Hold name.
	WSTRING descText;
	WSTRING holdName = CDbHolds::GetHoldName(holdID);
	WSTRING abbrevHoldName;
	static const UINT MAX_HOLD_NAME = 8;
	if (holdName.size() <= MAX_HOLD_NAME)
		descText += holdName;
	else
	{
		//Try to abbreviate by taking only the first letter from each word
		abbrevHoldName = filterFirstLettersAndNumbers(holdName);
		descText += abbrevHoldName;
	}
	descText += wszColon;
	descText += wszSpace;

	//Level name.
	descText += CDbLevels::GetLevelName(levelID);
	descText += wszSpace;

	//Append total number of demos
	descText += std::to_wstring(demoIDs.size());
	descText += wszSpace;
	descText += g_pTheDB->GetMessageText(MID_Demos);

	return descText;
}

//*****************************************************************************
void CDemosScreen::ListCNetDemos()
{
	this->pCNetDemoListBoxWidget->Clear();

	//Add received list of CaravelNet demos.
	if (this->cNetDemos.empty())
	{
		this->pCNetDemoListBoxWidget->AddItem(0,
				g_pTheDB->GetMessageText(this->bHoldPublished ? MID_NoScoresForRoom :
					g_pTheNet->IsLoggedIn() ? MID_HoldNotPublished : MID_NotConnected), true);
	} else {
		CNetDemoData *pDemoData;
		WSTRING wstr;
		WCHAR temp[20];
		for (UINT wIndex=0; wIndex<this->cNetDemos.size(); ++wIndex)
		{
			pDemoData = this->cNetDemos[wIndex];
			ASSERT(pDemoData);
			if (pDemoData->bRoomNotScorable) {
				//This room was is marked as not scorable on CaravelNet
				this->pCNetDemoListBoxWidget->AddItem(0, g_pTheDB->GetMessageText(
						MID_RoomNotScorable), true);
				break;
			}
			if (pDemoData->bRoomNotConquered)
			{
				//This room was not conquered locally, and the server has confirmed
				//the player isn't marked as conquering the room on CaravelNet either.
				this->pCNetDemoListBoxWidget->AddItem(0, g_pTheDB->GetMessageText(
						MID_RoomNotConquered), true);
				break;
			}

			const CDate timeReceived(time_t(pDemoData->wTimeReceived));
			const CDate timeCreated(time_t(pDemoData->wCreated));

			//Check whether demo is already installed.
			const UINT dwDemoID = g_pTheDB->Demos.GetDemoID(
					this->dwDownloadDemosForRoomID, timeCreated, pDemoData->playerName);

			//Print demo description.
			_itoW(pDemoData->wNumMoves, temp, 10);
			wstr = temp;
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_Moves);
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += CDate::FormatTime(pDemoData->wTimeElapsed / 100);   //convert: 10ms --> s
			wstr += wszRightParen;
			wstr += wszColon;
			wstr += wszSpace;
			wstr += pDemoData->userName;
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += pDemoData->playerName;
			wstr += wszRightParen;
			wstr += wszComma;
			wstr += wszSpace;
			timeReceived.GetLocalFormattedText(DF_LONG_DATE, wstr);
			this->pCNetDemoListBoxWidget->AddItem(pDemoData->lDemoID, wstr.c_str(),
					!pDemoData->lDemoID || this->bViewRecordsOnly || dwDemoID != 0);
		}
	}

   Paint();
}

//************************************************************************************
void CDemosScreen::Paint(
//Paint the screen.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	SDL_BlitSurface(this->images[0], NULL, GetDestSurface(), NULL);

	PaintChildren();
	if (bUpdateRect) UpdateRect();
}

//*****************************************************************************
void CDemosScreen::PopulateDemoListBox()
//Compile a list of all demos recorded in this room/level.
//Demos from all players are shown, unless they belong to another player and
//are marked for show from the title screen.
{
	//Continue showing the currently selected demo, if there is one.
	UINT dwDemoID = this->pDemoListBoxWidget->GetSelectedItem();

	this->pDemoListBoxWidget->Clear();

	ASSERT(this->dwRoomID);

	const bool bShowLevelDemos = this->pShowFromLevel->IsChecked();

	//List box header to show level position description.
	WSTRING wstrHeader = (const WCHAR *)CDbMessageText(MID_DemosFor);
	wstrHeader += wszSpace;
	CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(this->dwRoomID, true);
	ASSERT(pRoom);
	this->dwLevelID = pRoom->dwLevelID;
	ASSERT(this->dwLevelID);
	CDbLevel *pLevel = g_pTheDB->Levels.GetByID(this->dwLevelID);
	ASSERT(pLevel);
	wstrHeader += (const WCHAR *)pLevel->NameText;
	const UINT dwHoldID = pLevel->dwHoldID;
	ASSERT(dwHoldID);
	delete pLevel;
	if (!bShowLevelDemos)
	{
		wstrHeader += wszSpace;
		wstrHeader += wszColon;
		wstrHeader += wszSpace;
		pRoom->GetLevelPositionDescription(wstrHeader);
	}
	delete pRoom;
	this->pLBoxHeaderWidget->SetText(wstrHeader.c_str());

	//Determine whether player is author of this hold.
	const UINT dwPlayerID = g_pTheDB->GetPlayerID();
	const bool bPlayerIsHoldAuthor = dwPlayerID == g_pTheDB->Holds.GetAuthorID(dwHoldID);

	//Get demos in current room/level.
	CIDSet demoIDs = bShowLevelDemos ? CDb::getDemosInLevel(this->dwLevelID) :
			CDb::getDemosInRoom(this->dwRoomID);

	//Add items for demos into list box.
	WSTRING wstrItemText;
	UINT dwRoomID;
	for (CIDSet::const_iterator demo = demoIDs.begin(); demo != demoIDs.end(); ++demo)
	{
		CDbDemo *pDemo = g_pTheDB->Demos.GetByID(*demo);
		ASSERT(pDemo);
		if (IsDemoAccessible(pDemo, dwPlayerID, bPlayerIsHoldAuthor))
		{
			wstrItemText = wszEmpty;
			if (GetItemTextForDemo(pDemo->dwDemoID, wstrItemText, dwRoomID))
			{
				this->pDemoListBoxWidget->AddItem(pDemo->dwDemoID, wstrItemText.c_str());

				//Ensure that when showing demos from whole level that a demo
				//from the current room gets selected if at least one exists.
				if (!dwDemoID && dwRoomID == this->dwRoomID)
					dwDemoID = pDemo->dwDemoID;
			}
		}
		delete pDemo;
	}
	if (this->pDemoListBoxWidget->IsEmpty())
		this->pDemoListBoxWidget->AddItem(0L, g_pTheDB->GetMessageText(MID_NoDemosForRoom), true);
	this->pDemoListBoxWidget->SelectLine(0);
	//If previously highlighted demo is still in list, keep it selected.
	this->pDemoListBoxWidget->SelectItem(dwDemoID);

	//Set room and other widgets to selected demo.
	SetWidgetsToDemo(this->pDemoListBoxWidget->GetSelectedItem());

	SelectFirstWidget(false);
}

//*****************************************************************************
void CDemosScreen::RenameDemo()
{
	const UINT dwDemoID = this->pDemoListBoxWidget->GetSelectedItem();
	if (!dwDemoID) return;
	CDbDemo *pDemo = g_pTheDB->Demos.GetByID(dwDemoID);
	ASSERT(pDemo);
	if (!pDemo) return;
	WSTRING wstr = (const WCHAR*)pDemo->DescriptionText;
	const UINT dwAnswerTagNo = ShowTextInputMessage(MID_DescribeDemo, wstr);
	if (dwAnswerTagNo == TAG_OK)
	{
		pDemo->DescriptionText = wstr.c_str();
		pDemo->Update();
		PopulateDemoListBox();
		SetWidgetsToDemo(dwDemoID);
		Paint();
	}
	delete pDemo;
}

//*****************************************************************************
void CDemosScreen::SetWidgetsToDemo(
//Sets widgets to correspond to a specified demo.
//
//Params:
	UINT dwDemoID)   //(in)   If 0L then no demo will be selected.
{
	CWidget *pButton;

	pButton = GetWidget(TAG_DOWNLOAD);
	const UINT dwID = this->pCNetDemoListBoxWidget->GetSelectedItem();
	pButton->Enable(dwID != 0 && this->pCNetDemoListBoxWidget->IsItemEnabled(dwID));

	if (dwDemoID == 0L)  //No demo selected.
	{
		this->pDetailsFrame->HideChildren();
		this->pNoDemoWidget->Show();

		pButton = GetWidget(TAG_DELETE);
		pButton->Disable();
		pButton = GetWidget(TAG_DELETEALL);
		pButton->Disable();
		pButton = GetWidget(TAG_WATCH);
		pButton->Disable();
		pButton = GetWidget(TAG_EXPORT);
		pButton->Disable();
		pButton = GetWidget(TAG_EXPORT_ALL);
		pButton->Disable();
		pButton = GetWidget(TAG_UPLOAD);
		pButton->Disable();

		//Show server demos from current room.
		DownloadRoomDemos(this->dwRoomID);

		return;
	}

	//
	//A demo is selected.
	//

	this->pDetailsFrame->ShowChildren();
	this->pNoDemoWidget->Hide();

	pButton = GetWidget(TAG_DELETE);
	pButton->Enable();
	pButton = GetWidget(TAG_DELETEALL);
	pButton->Enable();
	pButton = GetWidget(TAG_WATCH);
	pButton->Enable();
	pButton = GetWidget(TAG_EXPORT);
	pButton->Enable();
	pButton = GetWidget(TAG_EXPORT_ALL);
	pButton->Enable();

	//Get saved game for the demo.
	CDbDemo *pDemo = g_pTheDB->Demos.GetByID(dwDemoID);
	if (!pDemo) {ASSERT(!"Could not retrieve demo."); return;} //Probably corrupted db.
	const UINT dwSavedGameID = pDemo->dwSavedGameID;

	SetCursor(CUR_Wait);

	//Get new current game.
	WSTRING wstrCreated;
	CCueEvents Ignored;
	delete this->pDemoCurrentGame;
	this->pDemoCurrentGame = g_pTheDB->GetSavedCurrentGame(dwSavedGameID, Ignored, true,
			true); //don't save anything to DB during playback
	if (this->pDemoCurrentGame)
	{
		//Show server demos from the same room as the selected demo.
		DownloadRoomDemos(this->pDemoCurrentGame->dwRoomID);

		//Load room widget from current game.
		this->pDemoCurrentGame->SetAutoSaveOptions(ASO_NONE);
		if (!pDemo->wBeginTurnNo)
			this->pDemoCurrentGame->Commands.GetFirst(); //quick replay from beginning
		this->pDemoCurrentGame->SetTurn(pDemo->wBeginTurnNo, Ignored);
		this->pRoomWidget->LoadFromCurrentGame(this->pDemoCurrentGame);
		this->pRoomWidget->DisplayPersistingImageOverlays(Ignored);

		//Since there is no way to flag hold completion or mastery in a demo,
		//assume the player can go through hold completion and master walls in demos.
		//This is a safe assumption to make: if it's wrong, the player wouldn't
		//be making any moves to go through master walls anyway (observe that
		//bumps against intact master walls are not allowed in CCurrentGame::ProcessPlayer.)
		this->pDemoCurrentGame->bHoldCompleted = true;
		this->pDemoCurrentGame->bHoldMastered = true;

		//Set text for labels.
		this->pDemoCurrentGame->Created.GetLocalFormattedText(DF_LONG_DATE | DF_SHORT_TIME,
				wstrCreated);
		this->pCreatedWidget->SetText(wstrCreated.c_str());
	}

	//Enable upload button if this is a CaravelNet hold.
	pButton = GetWidget(TAG_UPLOAD);
	pButton->Enable(g_pTheNet->IsLocalHold(this->pDemoCurrentGame->pHold->dwHoldID));

	//Get author text from a couple of lookups.
	const WCHAR *pwczAuthor = pDemo->GetAuthorText();
	this->pAuthorWidget->SetText((pwczAuthor) ? pwczAuthor : wszEmpty);

	//Get demo duration from a couple of lookups.
	WSTRING wstrDuration;
	if (pDemo->GetTimeElapsed(wstrDuration))
	{
		wstrDuration += wszSpace;
		wstrDuration += wszSpace;
	}

	//Set description text from narrative analysis of the demo.
	UINT wEndTurnNo = 0, wMoves = 0;
	WSTRING wstrNarrationText;
	pDemo->GetNarrationText(wstrNarrationText, wEndTurnNo,
			true, true); //consider completed and mastered
	this->pDescriptionWidget->SetText(wstrNarrationText.c_str());

	WCHAR dummy[20];
	wMoves = wEndTurnNo - this->pDemoCurrentGame->wPlayerTurn;
	wstrDuration += _itoW(wMoves, dummy, 10);
	wstrDuration += wszSpace;
	wstrDuration += g_pTheDB->GetMessageText(wMoves == 1 ? MID_Move : MID_Moves);
	this->pDurationWidget->SetText(wstrDuration.c_str());

	//Set display option box.
	this->pShowButton->SetChecked(pDemo->wShowSequenceNo != 0);

	//Enable option: setting demos for display from title screen
	//only if current player authored this hold.
	const UINT dwHoldID = g_pTheDB->Levels.GetHoldIDForLevel(this->dwLevelID);
	ASSERT(dwHoldID);
	this->pShowButton->Show(g_pTheDB->GetPlayerID() == g_pTheDB->Holds.GetAuthorID(dwHoldID));

	delete pDemo;

	SetCursor();
}

//*****************************************************************************
void CDemosScreen::UploadSelectedDemo()
//Submit selected demos authored by current player for CaravelNet scoring.
{
	const CIDSet demoIDs = this->pDemoListBoxWidget->GetSelectedItems();
	if (!demoIDs.getMax())
		return; //no demos selected

	const UINT currentPlayerID = g_pTheDB->GetPlayerID();
	if (!currentPlayerID)
		return;

	CIDSet uploadDemoIDs;
	for (CIDSet::const_iterator demo = demoIDs.begin(); demo != demoIDs.end(); ++demo)
	{
		//Filter by demos authored by current player.
		const UINT savedGameID = g_pTheDB->Demos.GetSavedGameIDofDemo(*demo);
		if (g_pTheDB->SavedGames.GetPlayerIDofSavedGame(savedGameID) != currentPlayerID)
			continue;

		//Don't submit this demo during future batch uploads.
		CDbDemo *pDemo = g_pTheDB->Demos.GetByID(*demo);
		ASSERT(pDemo);
		if (pDemo)
		{
			uploadDemoIDs += *demo;
			pDemo->SetFlag(CDbDemo::TestedForUpload);
			pDemo->Update();
			delete pDemo;
		}
	}
	if (!uploadDemoIDs.empty())
	{
		SetCursor(CUR_Wait);
		ShowStatusMessage(MID_Exporting);

		string text;
		UINT handle = 0;
		if (CDbXML::ExportXML(V_Demos, uploadDemoIDs, text, UINT(-1))) //no multi-room demos
		{
			SetCursor(CUR_Internet);
			handle = g_pTheNet->UploadDemos(text);
		}
		HideStatusMessage();
		SetCursor();
		ShowOkMessage(handle > 0 ? MID_ScoresUploaded : MID_ScoresNotUploaded);
	}
}

//*****************************************************************************
void CDemosScreen::ClearCNetDemos()
{
	UINT wIndex;
	for (wIndex=this->cNetDemos.size(); wIndex--; )
		delete this->cNetDemos[wIndex];
	this->cNetDemos.clear();
}

//*****************************************************************************
void CDemosScreen::GetCNetDemos(CNetResult* pResults)
//Adds demos featured on CaravelNet to CaravelNet demos list box.
{
	ClearCNetDemos();

	ASSERT(pResults);
	if (!pResults->pJson) {
		ListCNetDemos();
		return;
	}

	if (pResults->pJson->get("status", false).asBool() != true)	//check for invalid data
	{
		//Key not valid.
		this->pCNetDemoListBoxWidget->Clear();
		this->pCNetDemoListBoxWidget->AddItem(0, g_pTheDB->GetMessageText(
				MID_MustRegisterToViewScores), true);
	} else {
		Json::Value demos = (*(pResults->pJson))["scores"];
		if ((pResults->pJson)->get("RoomNotScorable", false).asBool() == true) {
			CNetDemoData* pDemo = new CNetDemoData;
			pDemo->bRoomNotScorable = true;
			this->cNetDemos.push_back(pDemo);
		} else if ((pResults->pJson)->get("RoomNotConquered", false).asBool() == true) {
			CNetDemoData* pDemo = new CNetDemoData;
			pDemo->bRoomNotConquered = true;
			this->cNetDemos.push_back(pDemo);
		} else {
			for (Json::Value::ArrayIndex i = 0; i < demos.size(); i++) {
				CNetDemoData* pDemo = new CNetDemoData;
				pDemo->lDemoID = demos[i].get("id", 0).asInt();
				pDemo->wNumMoves = demos[i].get("numMoves", 0).asUInt();
				pDemo->wTimeElapsed = demos[i].get("realTime", 0).asUInt();
				pDemo->wTimeReceived = demos[i].get("time", 0).asUInt();
				UTF8ToUnicode(demos[i].get("user", "Unknown").asString().c_str(), pDemo->userName);
				Base64::decode(demos[i].get("playerName", "").asString(), pDemo->playerName);
				pDemo->wCreated = demos[i].get("createdTime", 0).asUInt();

				this->cNetDemos.push_back(pDemo);
			}
		}
	}

	ListCNetDemos();
}

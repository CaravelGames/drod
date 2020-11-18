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
 * Contributors:
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//#define ENABLE_CHEATS

#include "HoldSelectScreen.h"
#include "BrowserScreen.h"
#include "DrodFontManager.h"
#include "EditSelectScreen.h"
#include "GameScreen.h"
#include "RoomWidget.h"
#include "TitleScreen.h"

#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/ImageWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/OptionButtonWidget.h>
#include <FrontEndLib/ProgressBarWidget.h>
#include <FrontEndLib/ScalerWidget.h>
#include <FrontEndLib/SliderWidget.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/DbXML.h"
#include "../DRODLib/SettingsKeys.h"

#include "../Texts/MIDs.h"
#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Base64.h>
#include <BackEndLib/Ports.h>

#include <map>
typedef std::multimap<double, UINT> HoldSortMap; //one or more values at each position

const UINT TAG_LIST_BOX = 1100;
const UINT TAG_CANCEL = 1101;
const UINT TAG_EXPORT = 1102;
const UINT TAG_IMPORT = 1103;
const UINT TAG_DELETEHOLD = 1104;
const UINT TAG_DOWNLOAD = 1105;
const UINT TAG_HELP = 1106;

const UINT TAG_RATEHOLD = 1110;
const UINT TAG_RATEDIFFICULTY = 1111;
const UINT TAG_RATEOVERALL = 1112;
const UINT TAG_DIFFNUM_LABEL = 1113;
const UINT TAG_OVERALLNUM_LABEL = 1114;
const UINT TAG_NOT_LOGGED_IN = 1115;
const UINT TAG_NOTRATED = 1116;
const UINT TAG_CNET_DIFFICULTY_LABEL = 1117;
const UINT TAG_CNET_OVERALL_LABEL = 1118;
const UINT TAG_NOTPUBLISHED = 1119;
const UINT TAG_BETA = 1120;
const UINT TAG_HOLDCNETAUTHOR = 1121;
const UINT TAG_UPLOADHOLDSCORES = 1122;
const UINT TAG_CNETSORTER_LABEL = 1123;
const UINT TAG_CNETSORTER = 1124;

const UINT TAG_MINIROOM = 1130;
const UINT TAG_POSITION_LABEL = 1131;
const UINT TAG_HOLDFILTER = 1132;
const UINT TAG_THUMBNAIL = 1133;

const UINT gameTurnTick = 100; //for the miniroom animation (ms)

const SDL_Color Aqua = {0, 136, 204, 0};

inline bool hold_is_installed(UINT holdID) { return int(holdID) > 0; }

bool CanDeleteHold(const CDbHold::HoldStatus status)
{
	//Player is not allowed to delete pre-installed holds.
	if (status == CDbHold::Tutorial)
		return false;

#ifdef STEAMBUILD
	if (CDbHold::IsOfficialHold(status) || status == CDbHold::Official) //DLC is managed via Steam UI
		return false;
#else
	if (status == CDbHold::GetOfficialHoldStatus())
		return false;
#endif

	return true;
}

//
//Public methods.
//

//*****************************************************************************
CHoldSelectScreen::CHoldSelectScreen()
	: CDrodScreen(SCR_HoldSelect)
	, pHoldListBoxWidget(NULL), pFullHoldList(NULL), pCNetHoldSortList(NULL)
	, pHoldDesc(NULL), pAuthorName(NULL)
	, pDetailsFrame(NULL)
	, pRoomWidget(NULL)
	, pOKButton(NULL), pExportButton(NULL), pDeleteButton(NULL), pDownloadButton(NULL)
	, wProcessingHoldLine(0)
	, dwLastNotice(0)
	, bDownloadList(false)
	, pCurrentRestoreGame(NULL)
	, filter(F_ALL)
	, pSelCNetHold(NULL)
//Constructor
{
	this->imageFilenames.push_back(string("Background"));

	static const UINT CY_SPACE = 15;
	static const UINT CX_SPACE = 15;

	static const UINT CX_BUTTON = 100;
	static const UINT CY_BUTTON = CY_STANDARD_BUTTON;
	static const UINT CX_BUTTON_SPACE = CX_SPACE;

	static const UINT CY_TITLE_SPACE = 14;
#ifdef RUSSIAN_BUILD
	static const UINT CX_TITLE = 440;
#else
	static const UINT CX_TITLE = 300;
#endif
	static const UINT CY_TITLE = 52;
	const int X_TITLE = (this->w - CX_TITLE) / 2;
	static const int Y_TITLE = CY_TITLE_SPACE;

	//List box.
	static const int X_LBOX_HEADER = CX_SPACE;
	static const UINT CX_LBOX_HEADER = 200;

#ifdef RUSSIAN_BUILD
	static const int Y_LBOX_HEADER = Y_TITLE + CY_TITLE;
	static const UINT CY_LBOX_HEADER = 54;
#else
	static const int Y_LBOX_HEADER = Y_TITLE + CY_TITLE + 22;
	static const UINT CY_LBOX_HEADER = 32;
#endif

	static const int X_HOLDLISTBOX = X_LBOX_HEADER;
	static const int Y_HOLDLISTBOX = Y_LBOX_HEADER + CY_LBOX_HEADER + 3;
	const UINT CX_HOLDLISTBOX = ((this->w - CX_SPACE) / 2) - CX_SPACE;
	static const UINT CY_HOLDLISTBOX = 356;   //16 items * 22 + 4

	static const UINT CX_HOLDFILTER_LIST = 220;
	static const UINT CY_HOLDFILTER_LIST = 53;
	static const int X_HOLDFILTER_LIST = X_HOLDLISTBOX + CX_HOLDLISTBOX - CX_HOLDFILTER_LIST;
	static const int Y_HOLDFILTER_LIST = Y_TITLE + CY_TITLE;

#ifdef RUSSIAN_BUILD
	static const UINT CX_HOLDFILTER_LABEL = 70;
#else
	static const UINT CX_HOLDFILTER_LABEL = 50;
#endif
	static const int X_HOLDFILTER_LABEL = X_HOLDFILTER_LIST - CX_HOLDFILTER_LABEL;
	static const int Y_HOLDFILTER_LABEL = Y_HOLDFILTER_LIST;
	static const UINT CY_HOLDFILTER_LABEL = CY_BUTTON;

	//Buttons.
	const int X_IMPORT_BUTTON = X_HOLDLISTBOX;
	const int Y_IMPORT_BUTTON = Y_HOLDLISTBOX + CY_HOLDLISTBOX + CY_SPACE/2;
	static const int X_EXPORT_BUTTON = X_IMPORT_BUTTON + CX_BUTTON + CX_BUTTON_SPACE;
	static const int Y_EXPORT_BUTTON = Y_IMPORT_BUTTON;
	static const int X_DELETEHOLD_BUTTON = X_EXPORT_BUTTON + CX_BUTTON + CX_BUTTON_SPACE;
	static const int Y_DELETEHOLD_BUTTON = Y_IMPORT_BUTTON;
	static const UINT CX_DELETEHOLD_BUTTON = 120;
	static const int X_DOWNLOAD_BUTTON = X_DELETEHOLD_BUTTON + CX_DELETEHOLD_BUTTON + CX_BUTTON_SPACE;
	static const int Y_DOWNLOAD_BUTTON = Y_IMPORT_BUTTON;
	static const int CX_DOWNLOAD_BUTTON = 116;

	const int X_OK_BUTTON = (this->w / 2) + 75;
	const int Y_OK_BUTTON = this->h - CY_BUTTON - CY_SPACE;
	const int X_HELP_BUTTON = X_OK_BUTTON + CX_BUTTON + CX_BUTTON_SPACE;
	const int Y_HELP_BUTTON = Y_OK_BUTTON;
	const int X_CANCEL_BUTTON = X_HELP_BUTTON + CX_BUTTON + CX_BUTTON_SPACE;
	const int Y_CANCEL_BUTTON = Y_OK_BUTTON;

	//CaravelNet frame.
	static const int X_CNET_FRAME = CX_SPACE;
	static const int Y_CNET_FRAME = Y_DOWNLOAD_BUTTON + CY_BUTTON + CY_SPACE;
	const UINT CX_CNET_FRAME = CX_HOLDLISTBOX;
	const UINT CY_CNET_FRAME = this->h - Y_CNET_FRAME - CY_SPACE;

	//CaravelNet hold sorting.
	static const int X_CNETHOLDSORT_LABEL = CX_SPACE;
	static const UINT CX_CNETHOLDSORT_LABEL = 200;
	static const int Y_CNETHOLDSORT_LABEL = CY_SPACE;
	static const UINT CY_CNETHOLDSORT_LABEL = 26 + CY_SPACE/2;

	static const int X_CNETHOLDSORT_LIST = X_CNETHOLDSORT_LABEL + CX_CNETHOLDSORT_LABEL;
	static const UINT CX_CNETHOLDSORT_LIST = CX_HOLDLISTBOX - CX_CNETHOLDSORT_LABEL - CX_SPACE*2;
	static const int Y_CNETHOLDSORT_LIST = Y_CNETHOLDSORT_LABEL;
	static const UINT CY_CNETHOLDSORT_LIST = 53;

	//Hold rating.
	static const int X_DIFFICULTY_LABEL = CX_SPACE;
	static const UINT CX_DIFFICULTY_LABEL = 100;
	static const int Y_DIFFICULTY_LABEL = Y_CNETHOLDSORT_LIST + CY_CNETHOLDSORT_LIST + CY_SPACE/2;
	static const UINT CY_DIFFICULTY_LABEL = 26 + CY_SPACE/2;

	static const int X_DIFFICULTY_SLIDER = X_DIFFICULTY_LABEL + CX_DIFFICULTY_LABEL;
	static const UINT CX_DIFFICULTY_SLIDER = 310;
	static const int Y_DIFFICULTY_SLIDER = Y_DIFFICULTY_LABEL;
	static const UINT CY_DIFFICULTY_SLIDER = CY_STANDARD_SLIDER;

	static const int X_DIFFNUM_LABEL = X_DIFFICULTY_SLIDER + CX_DIFFICULTY_SLIDER + CX_SPACE;
	static const UINT CX_DIFFNUM_LABEL = 50;
	static const int Y_DIFFNUM_LABEL = Y_DIFFICULTY_SLIDER;
	static const UINT CY_DIFFNUM_LABEL = CY_DIFFICULTY_LABEL;

	static const int X_OVERALL_LABEL = X_DIFFICULTY_LABEL;
	static const UINT CX_OVERALL_LABEL = CX_DIFFICULTY_LABEL;
	static const int Y_OVERALL_LABEL = Y_DIFFICULTY_LABEL + CY_DIFFICULTY_LABEL;
	static const UINT CY_OVERALL_LABEL = CY_DIFFICULTY_LABEL;

	static const int X_OVERALL_SLIDER = X_OVERALL_LABEL + CX_OVERALL_LABEL;
	static const UINT CX_OVERALL_SLIDER = CX_DIFFICULTY_SLIDER;
	static const int Y_OVERALL_SLIDER = Y_OVERALL_LABEL;
	static const UINT CY_OVERALL_SLIDER = CY_DIFFICULTY_SLIDER;

	static const int X_OVERALLNUM_LABEL = X_OVERALL_SLIDER + CX_OVERALL_SLIDER + CX_SPACE;
	static const UINT CX_OVERALLNUM_LABEL = CX_DIFFNUM_LABEL;
	static const int Y_OVERALLNUM_LABEL = Y_OVERALL_SLIDER;
	static const UINT CY_OVERALLNUM_LABEL = CY_OVERALL_LABEL;

	static const UINT CX_UPLOADSCORES_BUTTON = 180;
	static const UINT CY_UPLOADSCORES_BUTTON = CY_STANDARD_BUTTON;
	const int X_UPLOADSCORES_BUTTON = 20;
	static const int Y_UPLOADSCORES_BUTTON = CY_CNET_FRAME - CY_UPLOADSCORES_BUTTON - CY_SPACE;

	static const UINT CX_RATEHOLD_BUTTON = 150;
	static const UINT CY_RATEHOLD_BUTTON = CY_STANDARD_BUTTON;
	const int X_RATEHOLD_BUTTON = X_UPLOADSCORES_BUTTON + CX_UPLOADSCORES_BUTTON + 75;
	static const int Y_RATEHOLD_BUTTON = Y_UPLOADSCORES_BUTTON;

	//CaravelNet status texts.
	static const int X_NOTLOGGEDIN = CX_SPACE;
	static const UINT CX_NOTLOGGEDIN = CX_CNET_FRAME - X_NOTLOGGEDIN*2;
	static const int Y_NOTLOGGEDIN = CY_SPACE;
	static const UINT CY_NOTLOGGEDIN = 26 + CY_SPACE/2;

	static const int X_NOTRATED = X_NOTLOGGEDIN;
	static const UINT CX_NOTRATED = CX_NOTLOGGEDIN;
	static const int Y_NOTRATED = Y_OVERALL_LABEL + CY_OVERALL_LABEL;
	static const UINT CY_NOTRATED = CY_NOTLOGGEDIN;

	static const int X_NOTPUBLISHED = X_NOTLOGGEDIN;
	static const UINT CX_NOTPUBLISHED = CX_NOTLOGGEDIN;
	static const int Y_NOTPUBLISHED = Y_CNETHOLDSORT_LIST + CY_CNETHOLDSORT_LIST + CY_SPACE;
	static const UINT CY_NOTPUBLISHED = CY_NOTLOGGEDIN;

	static const int X_BETA = X_NOTLOGGEDIN;
	static const UINT CX_BETA = CX_NOTLOGGEDIN;
	static const int Y_BETA = Y_NOTPUBLISHED;
	static const UINT CY_BETA = CY_NOTLOGGEDIN;

	//Details frame.
	static const UINT CX_DETAILS_FRAME = CX_HOLDLISTBOX;
	static const int Y_DETAILS_FRAME = Y_TITLE + CY_TITLE + 10;
	const UINT CY_DETAILS_FRAME = this->h - CY_SPACE - CY_STANDARD_BUTTON - CY_SPACE -
			Y_DETAILS_FRAME;
	static const int X_DETAILS_FRAME = X_HOLDLISTBOX + CX_HOLDLISTBOX + CX_SPACE;

	//Hold info.
	static const int X_LABEL = CX_SPACE;
	const UINT CX_LABEL = CX_DETAILS_FRAME - X_LABEL*2;
	static const int Y_AUTHOR_LABEL = CY_SPACE;
	static const UINT CY_AUTHOR_LABEL = 26 + CY_SPACE/2;
	static const int Y_DESC_LABEL = Y_AUTHOR_LABEL + CY_AUTHOR_LABEL;
	static const UINT CY_DESC_LABEL = 126;  //6 lines of text * 21

	//Miniroom.
	static const UINT CX_MINIROOM = CX_DETAILS_FRAME - CX_SPACE - CX_SPACE;
	const UINT CY_MINIROOM = (CX_MINIROOM * CDrodBitmapManager::CY_ROOM) /
			CDrodBitmapManager::CX_ROOM;
	static const int X_MINIROOM = CX_SPACE;
	const int Y_MINIROOM = CY_DETAILS_FRAME - CY_SPACE - CY_MINIROOM;

	static const int X_POSITION_LABEL = X_MINIROOM;
	static const UINT CX_POSITION_LABEL= CX_MINIROOM;
	static const UINT CY_POSITION_LABEL = 25;
	static const int Y_POSITION_LABEL = Y_MINIROOM - CY_POSITION_LABEL;

	//Title.
	AddWidget(new CLabelWidget(0L, X_TITLE, Y_TITLE,
			CX_TITLE, CY_TITLE, F_Title, g_pTheDB->GetMessageText(MID_HoldManagementTitle)));

	//Hold list box.
	AddWidget(new CLabelWidget(0L, X_LBOX_HEADER, Y_LBOX_HEADER, CX_LBOX_HEADER,
			CY_LBOX_HEADER, F_Header, g_pTheDB->GetMessageText(MID_InstalledHolds)));

	this->pHoldListBoxWidget = new CListBoxWidget(TAG_LIST_BOX,
			X_HOLDLISTBOX, Y_HOLDLISTBOX, CX_HOLDLISTBOX, CY_HOLDLISTBOX, false,
			false, true);
	this->pHoldListBoxWidget->SetHotkeyItemSelection(true);
	this->pHoldListBoxWidget->IgnoreLeadingArticlesInSort();
	AddWidget(this->pHoldListBoxWidget);
	//Used for storing hold info.  Not added to the screen as a child widget.
	this->pFullHoldList = new CListBoxWidget(0, X_HOLDLISTBOX, Y_HOLDLISTBOX,
			CX_HOLDLISTBOX, CY_HOLDLISTBOX, true, false, true);
	this->pFullHoldList->IgnoreLeadingArticlesInSort();

	AddWidget(new CLabelWidget(0L, X_HOLDFILTER_LABEL, Y_HOLDFILTER_LABEL, CX_HOLDFILTER_LABEL,
			CY_HOLDFILTER_LABEL, F_Small, g_pTheDB->GetMessageText(MID_HoldFilter)));
	CListBoxWidget *pHoldFilterList = new CListBoxWidget(TAG_HOLDFILTER,
			X_HOLDFILTER_LIST, Y_HOLDFILTER_LIST, CX_HOLDFILTER_LIST, CY_HOLDFILTER_LIST);
	pHoldFilterList->AddItem(F_ALL, g_pTheDB->GetMessageText(MID_HoldFilterAll));
	pHoldFilterList->AddItem(F_OFFICIAL, g_pTheDB->GetMessageText(MID_HoldFilterOfficial));
	pHoldFilterList->AddItem(F_CNET, g_pTheDB->GetMessageText(MID_HoldFilterCaravelNet));
	pHoldFilterList->AddItem(F_MINE, g_pTheDB->GetMessageText(MID_HoldFilterMine));
	pHoldFilterList->AddItem(F_NEW, g_pTheDB->GetMessageText(MID_HoldFilterNew));
	pHoldFilterList->AddItem(F_INPROGRESS, g_pTheDB->GetMessageText(MID_HoldFilterInProgress));
	pHoldFilterList->AddItem(F_CONQUERED, g_pTheDB->GetMessageText(MID_HoldFilterConquered));
	pHoldFilterList->AddItem(F_UNCONQUERED, g_pTheDB->GetMessageText(MID_HoldFilterUnconquered));
	pHoldFilterList->AddItem(F_CONQUERED_NOT_MASTERED, g_pTheDB->GetMessageText(MID_HoldFilterConqueredNotMastered));
	pHoldFilterList->AddItem(F_MASTERED, g_pTheDB->GetMessageText(MID_HoldFilterMastered));
	pHoldFilterList->AddItem(F_UNMASTERED, g_pTheDB->GetMessageText(MID_HoldFilterUnmastered));
	pHoldFilterList->AddItem(F_NONCNET, g_pTheDB->GetMessageText(MID_HoldFilterNonCaravelNet));
	pHoldFilterList->AddItem(F_BETA, g_pTheDB->GetMessageText(MID_HoldFilterBeta));
	pHoldFilterList->AddItem(F_UPDATED, g_pTheDB->GetMessageText(MID_HoldFilterUpdated));
	pHoldFilterList->SelectLine(0);
	AddWidget(pHoldFilterList);

	//Hold-specific buttons.
	CButtonWidget *pImportButton = new CButtonWidget(
			TAG_IMPORT, X_IMPORT_BUTTON, Y_IMPORT_BUTTON, CX_BUTTON, CY_BUTTON,
			g_pTheDB->GetMessageText(MID_Import));
	AddWidget(pImportButton);
	this->pExportButton = new CButtonWidget(
			TAG_EXPORT, X_EXPORT_BUTTON, Y_EXPORT_BUTTON, CX_BUTTON, CY_BUTTON,
			g_pTheDB->GetMessageText(MID_Export));
	AddWidget(this->pExportButton);
	this->pDeleteButton = new CButtonWidget(TAG_DELETEHOLD,
			X_DELETEHOLD_BUTTON, Y_DELETEHOLD_BUTTON, CX_DELETEHOLD_BUTTON, CY_BUTTON,
			g_pTheDB->GetMessageText(MID_DeleteHold));
	AddWidget(this->pDeleteButton);
	this->pDownloadButton = new CButtonWidget(
			TAG_DOWNLOAD, X_DOWNLOAD_BUTTON, Y_DOWNLOAD_BUTTON, CX_DOWNLOAD_BUTTON,
			CY_BUTTON, g_pTheDB->GetMessageText(MID_Download));
	AddWidget(this->pDownloadButton);

	//CaravelNet operations.
	CFrameWidget *pCNetFrame = new CFrameWidget(0L, X_CNET_FRAME, Y_CNET_FRAME,
			CX_CNET_FRAME, CY_CNET_FRAME, g_pTheDB->GetMessageText(MID_CaravelNet));
	AddWidget(pCNetFrame);

	CLabelWidget *pLabel = new CLabelWidget(TAG_CNETSORTER_LABEL,
			X_CNETHOLDSORT_LABEL, Y_CNETHOLDSORT_LABEL,
			CX_CNETHOLDSORT_LABEL, CY_CNETHOLDSORT_LABEL, F_Small,
			g_pTheDB->GetMessageText(MID_CaravelNetSortHoldsBy));
	pCNetFrame->AddWidget(pLabel);
	this->pCNetHoldSortList = new CListBoxWidget(TAG_CNETSORTER,
			X_CNETHOLDSORT_LIST, Y_CNETHOLDSORT_LIST,
			CX_CNETHOLDSORT_LIST, CY_CNETHOLDSORT_LIST);
	this->pCNetHoldSortList->AddItem(S_ALPHA, g_pTheDB->GetMessageText(MID_HoldOrderAlpha));
	this->pCNetHoldSortList->AddItem(S_RATING, g_pTheDB->GetMessageText(MID_HoldOrderRating));
	this->pCNetHoldSortList->AddItem(S_DIFFICULTY, g_pTheDB->GetMessageText(MID_HoldOrderDifficulty));
	this->pCNetHoldSortList->AddItem(S_VERSION, g_pTheDB->GetMessageText(MID_HoldOrderVersion));
	this->pCNetHoldSortList->AddItem(S_VOTES, g_pTheDB->GetMessageText(MID_HoldOrderVotes));
	this->pCNetHoldSortList->SelectLine(0);
	pCNetFrame->AddWidget(this->pCNetHoldSortList);

	pLabel = new CLabelWidget(TAG_NOT_LOGGED_IN, X_NOTLOGGEDIN,
			Y_NOTLOGGEDIN, CX_NOTLOGGEDIN, CY_NOTLOGGEDIN,
			F_Small, g_pTheDB->GetMessageText(MID_CaravelNetNotLoggedIn));
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	pCNetFrame->AddWidget(pLabel);
	pLabel = new CLabelWidget(TAG_NOTRATED, X_NOTRATED, Y_NOTRATED, CX_NOTRATED,
			CY_NOTRATED, F_Small, g_pTheDB->GetMessageText(MID_HoldNotRated));
	pCNetFrame->AddWidget(pLabel);
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	pLabel = new CLabelWidget(TAG_HOLDCNETAUTHOR, X_NOTRATED, Y_NOTRATED, CX_NOTRATED,
			CY_NOTRATED, F_Small, g_pTheDB->GetMessageText(MID_HoldIsYours));
	pCNetFrame->AddWidget(pLabel);
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	pLabel = new CLabelWidget(TAG_NOTPUBLISHED, X_NOTPUBLISHED, Y_NOTPUBLISHED, CX_NOTPUBLISHED,
			CY_NOTPUBLISHED, F_Small, g_pTheDB->GetMessageText(MID_HoldNotPublished));
	pCNetFrame->AddWidget(pLabel);
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	pLabel = new CLabelWidget(TAG_BETA, X_BETA, Y_BETA, CX_BETA,
			CY_BETA, F_Small, g_pTheDB->GetMessageText(MID_HoldInBeta));
	pCNetFrame->AddWidget(pLabel);
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);

	//Hold rating.
	pCNetFrame->AddWidget(new CLabelWidget(TAG_CNET_DIFFICULTY_LABEL, X_DIFFICULTY_LABEL, Y_DIFFICULTY_LABEL,
			CX_DIFFICULTY_LABEL, CY_DIFFICULTY_LABEL, F_Small,
			g_pTheDB->GetMessageText(MID_HoldDifficulty)));
	pCNetFrame->AddWidget(new CLabelWidget(TAG_CNET_OVERALL_LABEL, X_OVERALL_LABEL, Y_OVERALL_LABEL,
			CX_OVERALL_LABEL, CY_OVERALL_LABEL, F_Small,
			g_pTheDB->GetMessageText(MID_HoldOverall)));
	pCNetFrame->AddWidget(new CSliderWidget(TAG_RATEDIFFICULTY, X_DIFFICULTY_SLIDER, Y_DIFFICULTY_SLIDER,
			CX_DIFFICULTY_SLIDER, CY_DIFFICULTY_SLIDER, 10, 20));
	pCNetFrame->AddWidget(new CSliderWidget(TAG_RATEOVERALL, X_OVERALL_SLIDER, Y_OVERALL_SLIDER,
			CX_OVERALL_SLIDER, CY_OVERALL_SLIDER, 5, 10));
	pCNetFrame->AddWidget(new CLabelWidget(TAG_DIFFNUM_LABEL, X_DIFFNUM_LABEL, Y_DIFFNUM_LABEL,
			CX_DIFFNUM_LABEL, CY_DIFFNUM_LABEL, F_Small, wszEmpty));
	pCNetFrame->AddWidget(new CLabelWidget(TAG_OVERALLNUM_LABEL, X_OVERALLNUM_LABEL, Y_OVERALLNUM_LABEL,
			CX_OVERALLNUM_LABEL, CY_OVERALLNUM_LABEL, F_Small, wszEmpty));
	pCNetFrame->AddWidget(new CButtonWidget(TAG_RATEHOLD, X_RATEHOLD_BUTTON, Y_RATEHOLD_BUTTON,
			CX_RATEHOLD_BUTTON, CY_RATEHOLD_BUTTON, g_pTheDB->GetMessageText(MID_RateHold)));
	pCNetFrame->AddWidget(new CButtonWidget(TAG_UPLOADHOLDSCORES, X_UPLOADSCORES_BUTTON, Y_UPLOADSCORES_BUTTON,
			CX_UPLOADSCORES_BUTTON, CY_UPLOADSCORES_BUTTON, g_pTheDB->GetMessageText(MID_UploadHoldScores)));

	//Details frame.
	this->pDetailsFrame = new CFrameWidget(0L, X_DETAILS_FRAME,
			Y_DETAILS_FRAME, CX_DETAILS_FRAME, CY_DETAILS_FRAME,
			g_pTheDB->GetMessageText(MID_Details));
	AddWidget(this->pDetailsFrame);

	this->pAuthorName = new CLabelWidget(0L, X_LABEL, Y_AUTHOR_LABEL, CX_LABEL,
			CY_AUTHOR_LABEL, F_Small, wszEmpty);
	this->pAuthorName->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pDetailsFrame->AddWidget(this->pAuthorName);
	this->pHoldDesc = new CLabelWidget(0L, X_LABEL, Y_DESC_LABEL, CX_LABEL,
			CY_DESC_LABEL, F_Small, wszEmpty);
	this->pHoldDesc->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pDetailsFrame->AddWidget(this->pHoldDesc);

	this->pDetailsFrame->AddWidget(new CLabelWidget(TAG_POSITION_LABEL,
			X_POSITION_LABEL, Y_POSITION_LABEL, CX_POSITION_LABEL, CY_POSITION_LABEL,
			F_Small, wszEmpty));

	CScalerWidget *pScaledRoomWidget = new CScalerWidget(TAG_MINIROOM,
			X_MINIROOM, Y_MINIROOM, CX_MINIROOM, CY_MINIROOM, false);
	this->pDetailsFrame->AddWidget(pScaledRoomWidget);
	this->pRoomWidget = new CRoomWidget(0L, 0, 0, CDrodBitmapManager::CX_ROOM,
			CDrodBitmapManager::CY_ROOM);
	this->pRoomWidget->SetMoveDuration(gameTurnTick);
	pScaledRoomWidget->AddScaledWidget(this->pRoomWidget);
	this->pDetailsFrame->AddWidget(new CImageWidget(TAG_THUMBNAIL,
			X_MINIROOM, Y_MINIROOM, (const WCHAR*)NULL));

	//General buttons.
	this->pOKButton = new CButtonWidget(
			TAG_OK, X_OK_BUTTON, Y_OK_BUTTON, CX_BUTTON, CY_BUTTON,
			g_pTheDB->GetMessageText(MID_Okay));
	AddWidget(this->pOKButton);
	CButtonWidget *pHelpButton = new CButtonWidget(TAG_HELP,
			X_HELP_BUTTON, Y_HELP_BUTTON, CX_BUTTON,
			CY_BUTTON, g_pTheDB->GetMessageText(MID_Help));
	AddWidget(pHelpButton);
	AddHotkey(SDLK_F1,TAG_HELP);
	CButtonWidget *pCancelButton = new CButtonWidget(
			TAG_CANCEL, X_CANCEL_BUTTON, Y_CANCEL_BUTTON, CX_BUTTON, CY_BUTTON,
			g_pTheDB->GetMessageText(MID_Cancel));
	AddWidget(pCancelButton);
}

//*****************************************************************************
CHoldSelectScreen::~CHoldSelectScreen()
//Destructor.
{
	delete this->pCurrentRestoreGame;
	delete this->pFullHoldList;
}

//*****************************************************************************
void CHoldSelectScreen::Paint(
//Overridable method to paint the screen.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//Blit the background graphic.
	SDL_BlitSurface(this->images[0], NULL, GetDestSurface(), NULL);

	PaintChildren();

	if (this->pProgressWidget->IsVisible())
		this->pProgressWidget->Paint();

	if (bUpdateRect)
		UpdateRect();
}

//*****************************************************************************
UINT CHoldSelectScreen::GetSelectedItem() const
//Returns: tag of item selected in the list box widget.
{
	if (this->pHoldListBoxWidget->GetSelectedLineCount() > 1)
	{
		if (this->pHoldListBoxWidget->IsCursorLineSelected())
			return this->pHoldListBoxWidget->GetKeyAtCursor();
	}
	return this->pHoldListBoxWidget->GetSelectedItem();
}

//*****************************************************************************
WSTRING CHoldSelectScreen::GetSelectedItemText() const
//Returns: tag of item selected in the list box widget.
{
	if (this->pHoldListBoxWidget->GetSelectedLineCount() > 1)
	{
		if (this->pHoldListBoxWidget->IsCursorLineSelected())
		{
			const WCHAR *pText = this->pHoldListBoxWidget->GetTextAtCursor();
			ASSERT(pText);
			return WSTRING(pText);
		}
	}
	return this->pHoldListBoxWidget->GetSelectedItemText();
}

//*****************************************************************************
bool CHoldSelectScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	if (!pCurrentPlayer)
		return false;

	this->dwLastNotice = pCurrentPlayer->Settings.GetVar(Settings::LastNotice, this->dwLastNotice);

	vector<CNetNotice> updates;
	g_pTheNet->QueryNotices(updates, NOTICE_NEWHOLD|NOTICE_UPDATEDHOLD, this->dwLastNotice);
	if (!updates.empty()) {
		this->bDownloadList = true;
		this->dwLastNotice = updates.back().id;
	}

	pCurrentPlayer->Settings.SetVar(Settings::LastNotice, this->dwLastNotice);
	pCurrentPlayer->Update();
	delete pCurrentPlayer;

	this->pRoomWidget->UnloadCurrentGame();
	delete this->pCurrentRestoreGame;
	this->pCurrentRestoreGame = NULL;

	PopulateHoldListBox();
	SetHoldDesc();

	//Never return to the win screen (for rating a hold).  Yank it out of the return list so
	//that we go back to the title screen instead.
	if (g_pTheSM->GetReturnScreenType() == SCR_WinStart)
		g_pTheSM->RemoveReturnScreen();

	return true;
}

//
// Private methods
//

//*****************************************************************************
void CHoldSelectScreen::AdvanceRestoreGameTurn()
{
	AdvanceDemoPlayback(this->pCurrentRestoreGame, this->pRoomWidget, TAG_MINIROOM);
}

//*****************************************************************************
void CHoldSelectScreen::ClearCachedHoldProgress(UINT holdID)
{
	const UINT dwCurrentPlayerID = g_pTheDB->GetPlayerID();
	HoldProgressCache& holdProgressCache = this->playerHoldProgressCache[dwCurrentPlayerID];
	holdProgressCache.erase(holdID);
}

//*****************************************************************************
bool CHoldSelectScreen::DecorateHoldText()
//Add indicators to the local holds in the list if the current player has finished the hold.
//Process one at a time to avoid a large delay when there are many holds.
//
//Returns: true if hold was processed, false if all holds in list have been processed
{
	ASSERT(this->pHoldListBoxWidget);

	const UINT dwCurrentPlayerID = g_pTheDB->GetPlayerID();
	HoldProgressCache& holdProgressCache = this->playerHoldProgressCache[dwCurrentPlayerID];

	CDb db;

	const Uint32 dwNow = SDL_GetTicks();
	static const Uint32 MAX_SCAN_INTERVAL_MS = 100;

	const UINT holdListCount = this->pFullHoldList->GetItemCount();
	while (this->wProcessingHoldLine < holdListCount)
	{
		SetCursor(CUR_Wait);

		const UINT wLineNo = this->wProcessingHoldLine++;
		const UINT dwHoldID = this->pFullHoldList->GetKeyAtLine(wLineNo);
		if (!hold_is_installed(dwHoldID))
			continue; //hold not installed -- change nothing

		ASSERT(this->holdInfo.count(dwHoldID) > 0);
		HoldInfo& h = this->holdInfo[dwHoldID];

		//Cache info for this hold ID.
		const bool cached_data_exists = holdProgressCache.count(dwHoldID) > 0;
		HoldProgress& holdProgress = holdProgressCache[dwHoldID];
		if (cached_data_exists) {
			//Retrieve cached info.
			h.bInProgress = holdProgress.inProgress;
			h.bConquered = holdProgress.conquered;
			h.bMastered = holdProgress.mastered;
			SetHoldNameText(dwHoldID, holdProgress.holdNameWithProgress);
			continue;
		}

		//Look up data about this player's hold progress.
		WSTRING wStr = this->pFullHoldList->GetTextAtLine(wLineNo);
		holdProgress.holdNameWithProgress = wStr; //default

		//Mark whether hold is in progress.
		const UINT dwContinueGameID = g_pTheDB->SavedGames.FindByContinue(dwHoldID);
		if (dwContinueGameID) {
			h.bInProgress = holdProgress.inProgress = true;
		}

		//Mark whether hold is conquered.
		const UINT dwEndHoldID = db.SavedGames.FindByEndHold(dwHoldID);
		if (!dwEndHoldID)
			continue; //not conquered -- change nothing

		h.bConquered = holdProgress.conquered = true;

		//Compile set of secret rooms found by player.
		HoldStats stats;
		const UINT wSecretsConquered = db.Holds.GetSecretsDone(stats,
				dwHoldID, g_pTheDB->GetPlayerID(), true);
		wStr += wszSpace;
		wStr += wszLeftParen;
		const UINT wPercent = stats.secretRooms.size() ?
				(wSecretsConquered*100)/stats.secretRooms.size() : 100;
		WCHAR temp[16];
		_itoW(wPercent, temp, 10);
		wStr += temp;
		wStr += wszPercent;
		wStr += wszRightParen;

		if (wPercent == 100) {
			h.bMastered = holdProgress.mastered = true;
		}

		holdProgress.holdNameWithProgress = wStr;
		SetHoldNameText(dwHoldID, wStr);

		//UI responsiveness.
		if (SDL_GetTicks() - dwNow >= MAX_SCAN_INTERVAL_MS)
			break;
	}

	SetCursor();

	return this->wProcessingHoldLine < holdListCount;
}

//*****************************************************************************
void CHoldSelectScreen::SetHoldNameText(UINT dwHoldID, const WSTRING& wStr)
{
	//Update hold text in full list and visible (filtered) list.
	this->pFullHoldList->SetItemText(dwHoldID, wStr.c_str());
	this->pHoldListBoxWidget->SetItemText(dwHoldID, wStr.c_str());
	this->pHoldListBoxWidget->RequestPaint();
	UpdateRect();
}

//*****************************************************************************
void CHoldSelectScreen::DeleteSelectedHolds()
{
	SetCursor(CUR_Wait);

	const CIDSet holdIDs = this->pHoldListBoxWidget->GetSelectedItems();
	for (CIDSet::const_iterator id = holdIDs.begin(); id != holdIDs.end(); ++id)
	{
		if (PollForOperationInterrupt())
			break;

		const UINT holdID = *id;

		if (g_pTheNet->IsEnabled()) {
			const CDbPackedVars settings = g_pTheDB->GetCurrentPlayerSettings();
			if (settings.GetVar(Settings::CloudActivated, false)) {
				while (g_pTheNet->IsHoldInCloudQueue(holdID))
					SDL_Delay(20);
				g_pTheNet->CloudSetHoldInstalled(holdID, false);
			}
		}

		CDbHold *pHold = g_pTheDB->Holds.GetByID(holdID, true);
		ASSERT(pHold);
#ifndef ENABLE_CHEATS
		if (!CanDeleteHold(pHold->status))
		{
			delete pHold;
			continue;
		}
#endif
		WSTRING wstr = g_pTheDB->GetMessageText(MID_Deleting);
		wstr += wszSpace;
		wstr += (const WCHAR *)pHold->NameText;
		delete pHold;
		CScreen::ShowStatusMessage(wstr.c_str());

		g_pTheDB->Holds.Delete(holdID);

		ClearCachedHoldProgress(holdID);
	}
	g_pTheDB->SavedGames.CleanupPlayerTallies();

	HideStatusMessage();
	g_pTheNet->MatchCNetHolds();
	PopulateHoldListBox();
	this->pHoldListBoxWidget->SelectLine(0);
	g_pTheDB->SetHoldID(GetSelectedItem());
	SetHoldDesc();
	SetCursor();
	Paint();
}

//*****************************************************************************
void CHoldSelectScreen::DownloadNewRoomStyles(set<WSTRING>& importedStyles)
//Following a hold import, download published room styles not yet installed
{
	if (importedStyles.empty())
		return; //no styles to import

	//Get set of local styles.
	list<WSTRING> styles;
	if (!CFiles::GetGameProfileString(INISection::Graphics, INIKey::Style, styles))
		return;

	//Remove local styles from set of imported styles.
	for (list<WSTRING>::const_iterator localStyle = styles.begin();
			localStyle != styles.end(); ++localStyle)
		importedStyles.erase(*localStyle);

	if (importedStyles.empty())
		return; //no new styles to import

	//Download and import all remaining styles.
	vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();
	for (set<WSTRING>::const_iterator newStyle=importedStyles.begin();
			newStyle!=importedStyles.end(); ++newStyle)
	{
		//Determine whether style is published on CaravelNet.
		const int nIndex = g_pTheNet->getIndexForName((const WCHAR*)newStyle->c_str(), MT_Style);
		if (nIndex < 0)
			continue; //style not found and won't be imported
		const UINT wExpectedSize = atoi(cNetMedia[nIndex]->filesizebytes.c_str());

		//Init.
		SetCursor(CUR_Internet);
		CDrodScreen::callbackContext = *newStyle;
		Callback(MID_DownloadingMedia);

		//Download style.
		const UINT handle = g_pTheNet->DownloadStyle(newStyle->c_str());
		g_pTheDB->Commit();  //commit any outstanding changes right after request is sent
		if (!handle) {
			HideStatusMessage(); SetCursor(); continue;
		}
		while (g_pTheNet->GetStatus(handle) < 0)
		{
			const UINT wBytes = g_pTheNet->GetBytesCompleted(handle);
			const float p = float(wBytes)/float(wExpectedSize);
			CDrodScreen::Callbackf(p);
			SDL_Delay(10); // just wait.
		}
		CNetResult* pResults = g_pTheNet->GetResults(handle);
		if (!pResults || !pResults->pBuffer || pResults->pBuffer->Size() < 10) {
			// error - can't download!
			delete pResults; HideStatusMessage(); SetCursor(); continue;
		}

		//Import style.
		SetCursor(CUR_Wait);
		Callback(MID_ImportingData);
		CDbXML::SetCallback(this);
		MESSAGE_ID result = CDbXML::ImportXML(*pResults->pBuffer, CImportInfo::Data);
		if (ImportConfirm(result)) {
			// Write the header info to the INI
			WSTRING wstr;
			Base64::decode(CDbXML::info.headerInfo, wstr);
			CFiles f;
			f.WriteGameProfileBuffer(wstr,false,false);
		}
		delete pResults;
	}

	//Done.
	g_pTheDB->Commit();
	ExportCleanup();
	CDrodScreen::callbackContext.resize(0);
}

//*****************************************************************************
void CHoldSelectScreen::DownloadSelectedHolds()
//Downloads and installs the selected CaravelNet hold(s).
{
	if (!g_pTheDB->GetPlayerID())
		return;

	const vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();

	bool bFirstErrorMessage = true;
	MESSAGE_ID result;
	CIDSet importedHoldIDs;
	set<WSTRING> importedStyles;
	const CIDSet lineNos = this->pHoldListBoxWidget->GetSelectedLineNumbers();
	for (CIDSet::const_iterator line = lineNos.begin(); line != lineNos.end(); ++line)
	{
		if (PollForOperationInterrupt())
			break;

		//Get download URL for selected hold.
		const UINT dwKey = this->pHoldListBoxWidget->GetKeyAtLine(*line);
		WSTRING holdName;
		if (!hold_is_installed(dwKey)) {
			holdName = this->pHoldListBoxWidget->GetTextAtLine(*line);
#ifdef STEAMBUILD
//TODO: attempting to download official DLC holds from Steam should open a browser to the Steam game purchase page
#endif
		} else {
			//Get original name of hold (without extra text decorations).
			holdName = g_pTheDB->Holds.GetHoldName(dwKey);
		}

		//Get hold info.
		const int nIndex = g_pTheNet->getIndexForName(holdName.c_str());
		if (nIndex < 0)
			continue; //not found in CNet list
		const long lHoldID = cNetMedia[nIndex]->lHoldID;
		const UINT wExpectedSize = atoi(cNetMedia[nIndex]->filesizebytes.c_str());

		//Init.
		SetCursor(CUR_Internet);
		CDrodScreen::callbackContext = holdName;
		Callback(MID_DownloadingHold);

		//Download hold.
		const UINT handle = g_pTheNet->DownloadHold(lHoldID);
		g_pTheDB->Commit();  //commit any outstanding changes right after request is sent
		if (!handle) {
			HideStatusMessage(); SetCursor(); continue;
		}
		while (g_pTheNet->GetStatus(handle) < 0)
		{
			const UINT wBytes = g_pTheNet->GetBytesCompleted(handle);
			const float p = float(wBytes)/float(wExpectedSize);
			CDrodScreen::Callbackf(p);
			SDL_Delay(10); // just wait.
		}

		CNetResult* pResults = g_pTheNet->GetResults(handle);

		if (!pResults || !pResults->pBuffer || pResults->pBuffer->Size() < 10) {
			// error - can't download!
			delete pResults;
			ExportCleanup();
			if (bFirstErrorMessage)
			{
				ShowOkMessage(MID_DownloadFailed);
				bFirstErrorMessage = false;
			}
			continue;
		}

		//Import hold.
		SetCursor(CUR_Wait);
		Callback(MID_ImportingData);
		CDbXML::SetCallback(this);
		result = CDbXML::ImportXML(*pResults->pBuffer, CImportInfo::Hold);
		if (!ImportConfirm(result)) {
			delete pResults;
			ExportCleanup();
			continue;
		}
		delete pResults;

		//If hold import was not ignored...
		if (CDbXML::info.dwHoldImportedID)
		{
			//Mark hold as imported.
			if (g_pTheNet->IsEnabled()) {
				const CDbPackedVars settings = g_pTheDB->GetCurrentPlayerSettings();
				if (settings.GetVar(Settings::CloudActivated, false)) {
					g_pTheNet->CloudSetHoldInstalled(CDbXML::info.dwHoldImportedID, true);
				}
			}
			importedHoldIDs += CDbXML::info.dwHoldImportedID;
			for (set<WSTRING>::const_iterator iter = CDbXML::info.roomStyles.begin();
					iter != CDbXML::info.roomStyles.end(); ++iter)
				importedStyles.insert(*iter);
			CDbHold *pHold = g_pTheDB->Holds.GetByID(CDbXML::info.dwHoldImportedID);
			ASSERT(pHold);
			pHold->bCaravelNetMedia = true;
			CDb::FreezeTimeStamps(true);
			pHold->Update();
			CDb::FreezeTimeStamps(false);
			delete pHold;
		}
	}

	//Done.
	g_pTheDB->Commit();
	ExportCleanup();
	CDrodScreen::callbackContext.resize(0);
	if (!importedHoldIDs.empty())
	{
		ShowOkMessage(result);

		//Download non-present room styles encountered during import.
		DownloadNewRoomStyles(importedStyles);

		//Show hold once any new styles are imported.
		SelectImportedHold(importedHoldIDs);
	}
}

//*****************************************************************************
void CHoldSelectScreen::OnBetweenEvents()
{
	const Uint32 dwNow = SDL_GetTicks();

	//Carefully-timed events.
	static const UINT requiredIdleTime = 750; //ms
	static const UINT decorationTick = 100; //ms
	static UINT lastGameTurnTick=0, lastDecorationTick=0;

	if (this->bDownloadList) {
		CScreen::ShowStatusMessage(g_pTheDB->GetMessageText(MID_UpdatingHoldList));
		UINT handle = g_pTheNet->DownloadHoldList();
		while (g_pTheNet->Busy()) {
			SDL_Delay(50);
		}
		// Done downloading the list.
		CScreen::HideStatusMessage();
		this->bDownloadList = false;

		this->pSelCNetHold = NULL;
		PopulateHoldListBox();

		UINT caravelNetSelectHoldId = g_pTheNet->GetDownloadHold();
		if (caravelNetSelectHoldId != 0) {
			// User got here from a CaravelNet new/update hold notice.  Find the hold and select it.
			this->pHoldListBoxWidget->SelectItem(caravelNetSelectHoldId);
			g_pTheNet->SetDownloadHold(0);
		}


		SetHoldDesc();
		Paint();
	}

	//Whenever the user isn't doing anything, perform a CPU-intensive query
	//that tallies the player's hold completion progress.
	if (dwNow - GetTimeOfLastUserInput() >= requiredIdleTime &&
			dwNow - lastDecorationTick >= decorationTick)
	{
		DecorateHoldText();
		lastDecorationTick = dwNow;
	}

	//If hold image thumbnail is pending, request it until received.
	if (this->pSelCNetHold)
		RequestThumbnail(this->pSelCNetHold);

	//Advance play of the featured saved game.
	if (dwNow - lastGameTurnTick >= gameTurnTick)
	{
		AdvanceRestoreGameTurn();
		lastGameTurnTick = dwNow;
	} else {
		//Animate the widget's last turn.
		CWidget *pRoomDisplay = GetWidget(TAG_MINIROOM);
		ASSERT(pRoomDisplay);
		if (pRoomDisplay->IsVisible())
			pRoomDisplay->RequestPaint();
	}
}

//*****************************************************************************
void CHoldSelectScreen::OnClick(
//Handles a button click.
//
//Params:
	const UINT dwTagNo)       //(in)   Widget that event applied to.
{
	const UINT dwSelectedHoldID = GetSelectedItem();
	switch (dwTagNo)
	{
		case TAG_OK:
		case TAG_MINIROOM:
		{
			//Select this hold for playing.
			//Don't change the selected playing hold when visiting this screen from the editor.
			if (g_pTheSM->GetReturnScreenType() != SCR_EditSelect)
				SelectHold(dwSelectedHoldID);
			GoToScreen(SCR_Return);
		}
		break;

		case TAG_DELETEHOLD:
		{
			//Delete hold on confirmation.
			const CIDSet ids = this->pHoldListBoxWidget->GetSelectedItems();
			if (ShowYesNoMessage(ids.size() > 1 ? MID_DeleteHoldsPrompt : MID_DeleteHoldPrompt) != TAG_YES)
				break;

			DeleteSelectedHolds();
		}
		break;

		case TAG_EXPORT:
			ExportHold(dwSelectedHoldID);
		break;

		case TAG_HELP:
			CBrowserScreen::SetPageToLoad("holdselect.html");
			GoToScreen(SCR_Browser);
		break;

		case TAG_IMPORT:
		{
			//Import a hold data file.
			CIDSet importedHoldIDs;
			set<WSTRING> importedStyles;
			if (Import(EXT_HOLD | EXT_XML, importedHoldIDs, importedStyles) == MID_ImportSuccessful)
			{
				CFiles f;
				WSTRING wstr;
				Base64::decode(CDbXML::info.headerInfo, wstr);
				if (!wstr.empty())
					f.WriteGameProfileBuffer(wstr,false,false);
			}
			if (!importedHoldIDs.empty())
			{
				if (g_pTheNet->IsEnabled()) {
					const CDbPackedVars settings = g_pTheDB->GetCurrentPlayerSettings();
					if (settings.GetVar(Settings::CloudActivated, false)) {
						for (CIDSet::const_iterator i = importedHoldIDs.begin(); i != importedHoldIDs.end(); i++) {
							g_pTheNet->CloudSetHoldInstalled(*i, true);
						}
					}
				}
				if (!importedStyles.empty())
				{
					//Automatically download and import new referenced room styles
					//from CaravelNet, if available.
					if (g_pTheNet->IsLoggedIn())
						DownloadNewRoomStyles(importedStyles);
				}
				//Show hold once any new styles are imported.
				SelectImportedHold(importedHoldIDs);
			}
		}
		break;

		case TAG_DOWNLOAD:
			DownloadSelectedHolds();
		break;

		case TAG_RATEHOLD:
			RateHold();
		break;

		case TAG_UPLOADHOLDSCORES:
			UploadHoldScores();
		break;

		case TAG_CANCEL:
		case TAG_ESCAPE:
			GoToScreen(SCR_Return);
		break;
		case TAG_QUIT:
			this->bQuitPrompt = true;
			if (ShowYesNoMessage(MID_ReallyQuit) != TAG_NO)
				GoToScreen(SCR_None);
			this->bQuitPrompt = false;
		break;
		default: break;
	}
}

//*****************************************************************************
void CHoldSelectScreen::OnDeactivate()
{
	//Player will likely play the selected hold,
	//so cached data might not be accurate when returning to this screen.
	ClearCachedHoldProgress(g_pTheDB->GetHoldID());
}

//*****************************************************************************
void CHoldSelectScreen::OnDoubleClick(
//Handles double click event.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	switch (dwTagNo)
	{
		case TAG_LIST_BOX:
		{
			if (!this->pHoldListBoxWidget->ClickedSelection()) break;

			const UINT dwSelectedHoldID = GetSelectedItem();
			if (hold_is_installed(dwSelectedHoldID))
			{
				SelectHold(dwSelectedHoldID);
				GoToScreen(SCR_Return);
			} else {
				DownloadSelectedHolds();
			}
		}
		break;
	}
}

//*****************************************************************************
void CHoldSelectScreen::OnKeyDown(
//Handles a keydown event.
//
//Params:
	const UINT dwTagNo, const SDL_KeyboardEvent &Key)
{
	CScreen::OnKeyDown(dwTagNo, Key);

	//Fast hold select.
	if (Key.keysym.mod & KMOD_CTRL)
	{
		const WCHAR wc = TranslateUnicodeKeysym(Key.keysym);
		if (this->pHoldListBoxWidget->SelectLineStartingWith(wc))
		{
			SetHoldDesc();
			Paint();
		}
	}
}

//*****************************************************************************
void CHoldSelectScreen::OnSelectChange(
//Handles a selection change.
//
//Params:
	const UINT dwTagNo)       //(in)   Widget that event applied to.
{
	switch (dwTagNo)
	{
		case TAG_LIST_BOX:
			//Update widgets with this hold's info.
			SetHoldDesc();
			Paint();
		break;
		case TAG_HOLDFILTER:
		{
			while (DecorateHoldText()) ; //complete this to gather all info before setting a filter

			CListBoxWidget *pFilterList = DYN_CAST(CListBoxWidget*, CWidget*, GetWidget(TAG_HOLDFILTER));
			this->filter = HoldTypeFilter(pFilterList->GetSelectedItem());

			SetHoldFilter();
			SetHoldDesc();
			Paint();
		}
		break;

		case TAG_CNETSORTER:
		{
			const WSTRING wstrHoldName = GetSelectedItemText();
			PopulateHoldListBox();  //list holds in new order
			//Reselect highlighted line at its new position.
			const int line = this->pHoldListBoxWidget->GetLineWithText(wstrHoldName.c_str());
			if (line >= 0)
				this->pHoldListBoxWidget->SelectLine(line);
			SetHoldDesc();
			Paint();
		}
		break;

		case TAG_RATEDIFFICULTY:
		case TAG_RATEOVERALL:
			SetHoldRatingLabel(dwTagNo);
			Paint();
		break;
		default: break;
	}
}

//*****************************************************************************
bool CHoldSelectScreen::PollForOperationInterrupt()
//Returns: true if user has requested 
{
	//Get any events waiting in the queue.
	//Prompt to halt the process on a key press.
	bool bInterrupt = false;
	SDL_Event event;
	while (PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_WINDOWEVENT:
				OnWindowEvent(event.window);
				this->pStatusDialog->Paint();	//make sure this stays on top
			break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
					case SDLK_SPACE:
					case SDLK_RETURN:	case SDLK_KP_ENTER:
					case SDLK_F4:
#if defined(__linux__) || defined(__FreeBSD__)
					case SDLK_PAUSE:
#endif
						SetCursor();
						if (ShowYesNoMessage(MID_HaltOperationPrompt) != TAG_NO)
							bInterrupt = true;
						this->pStatusDialog->Paint();	//make sure this stays on top
						SetCursor(CUR_Wait);
					break;
					default: break;
				}
			break;
			case SDL_QUIT:
				SetCursor();
				if (ShowYesNoMessage(MID_HaltOperationPrompt) != TAG_NO)
					bInterrupt = true;
				this->pStatusDialog->Paint();	//make sure this stays on top
				SetCursor(CUR_Wait);
			break;
			default: break;
		}
	}
	return bInterrupt;
}

//*****************************************************************************
void CHoldSelectScreen::RateHold()
//Upload rating for selected hold to server.
{
	//Get original name of hold (without extra text decorations).
	const WSTRING holdName = g_pTheDB->Holds.GetHoldName(GetSelectedItem());

	CSliderWidget *pSlider = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_RATEDIFFICULTY));
	float fDifficulty = ((1+pSlider->GetValue())/(float)pSlider->GetNumTicks()) * 10; //(0,10]
	pSlider = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_RATEOVERALL));
	float fOverall = ((1+pSlider->GetValue())/(float)pSlider->GetNumTicks()) * 10; //(0,10]
	const UINT wCaravelNetRequest = g_pTheNet->RateHold(holdName.c_str(), fDifficulty, fOverall);
	if (!wCaravelNetRequest)
	{
		ShowOkMessage(MID_NotConnected);
		return;
	}

	g_pTheDB->Commit(); //commit outstanding changes just after request is sent

	GenericNetTransactionWait(wCaravelNetRequest, MID_RatingHold);

	CNetResult* pBuffer = g_pTheNet->GetResults(wCaravelNetRequest);
	if (!pBuffer || !pBuffer->pJson) {
		ShowOkMessage(MID_CaravelNetUnreachable);
		delete pBuffer;
		return;
	}

	// Buffer possibilities:
	//   '1' : Okay.  Rating Sent.
	//   '2' : Hold not on CaravelNet.
	switch (pBuffer->pJson->get("status", 1).asInt()) {
		case 1:
		{
			ShowOkMessage(MID_HoldRatingSent);

			//Update local personal hold rating.
			vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();
			const int nIndex = g_pTheNet->getIndexForName(holdName.c_str());
			CNetMedia& cNetHold = *(cNetMedia[nIndex]);
			char temp[16];
			sprintf(temp, "%.1f", fDifficulty);
			cNetHold.myDifficulty = temp;
			sprintf(temp, "%.1f", fOverall);
			cNetHold.myRating = temp;

			SetHoldDesc();
			Paint();
		}
		break;
		case 2:
			ShowOkMessage(MID_HoldNotFoundOnCaravelNet);
		break;
		default: break;
	}

	delete pBuffer;
}

//*****************************************************************************
void CHoldSelectScreen::RequestThumbnail(CNetMedia *pMedia)
//Request thumbnail image.  When received, show it.
{
	CStretchyBuffer *pBuffer = pMedia->RequestThumbnail();
	if (pBuffer)
	{
		this->pSelCNetHold = NULL; //pending request is completed

		//Show image thumbnail if valid image data.
		SDL_Surface *pImage = g_pTheBM->GetImage(*pBuffer);
		if (pImage)
		{
			CImageWidget *pThumbnail = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_THUMBNAIL));
			pThumbnail->SetImage(pImage);
			pThumbnail->Show();
			Paint();
			return;
		}
	}

	//Image not available yet.  Start/continue requesting it.
	this->pSelCNetHold = pMedia;
}

//*****************************************************************************
void CHoldSelectScreen::SelectHold(const UINT dwSelectedHoldID)
//Activates the selected hold.
{
	const UINT dwCurrentHoldID = g_pTheDB->GetHoldID();
	if (dwCurrentHoldID != dwSelectedHoldID)
	{
		//Unload any game being played in another hold.
		CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*,
				g_pTheSM->GetScreen(SCR_Game));
		ASSERT(pGameScreen);
		pGameScreen->UnloadGame();

		g_pTheDB->SetHoldID(dwSelectedHoldID);
	}
}

//*****************************************************************************
void CHoldSelectScreen::SelectImportedHold(const CIDSet& importedHoldIDs)
{
	if (CDbXML::WasImportSuccessful() && !importedHoldIDs.empty())
	{
		g_pTheNet->MatchCNetHolds();
		PopulateHoldListBox();  //Update in case a new hold was added.

		if (!this->pHoldListBoxWidget->HasKeys(importedHoldIDs))
		{
			//Remove filter to show all imported holds in list.
			this->filter = F_ALL;
			CListBoxWidget *pFilterList = DYN_CAST(CListBoxWidget*, CWidget*, GetWidget(TAG_HOLDFILTER));
			pFilterList->SelectItem(this->filter);
			PopulateHoldListBox();  //Update in case a hold was added.
		}

		//Select imported holds.
		this->pHoldListBoxWidget->SelectItems(importedHoldIDs);
		g_pTheDB->SetHoldID(this->pHoldListBoxWidget->GetKeyAtCursor());
		SetHoldDesc();
		Paint();
	}
}

//*****************************************************************************
void CHoldSelectScreen::SetCNetHoldDesc(CNetMedia *pHoldData)
//Set hold description text from specified hold data.
{
	ASSERT(pHoldData);

	//Show thumbnail if available.
	RequestThumbnail(pHoldData);

	WSTRING wstrAuthorNames = g_pTheDB->GetMessageText(MID_LevelBy);
	wstrAuthorNames += wszSpace;
	wstrAuthorNames += (const WCHAR*)pHoldData->AuthorText;
	wstrAuthorNames += wszSpace;
	wstrAuthorNames += wszLeftParen;
	wstrAuthorNames += (const WCHAR*)pHoldData->CNetNameText;
	wstrAuthorNames += wszRightParen;
	this->pAuthorName->SetText(wstrAuthorNames.c_str());

	WSTRING holdRating = g_pTheDB->GetMessageText(MID_Difficulty);
	WSTRING temp;
	holdRating += wszSpace;
	UTF8ToUnicode(pHoldData->difficulty.c_str(), temp);
	holdRating += temp;
	holdRating += wszCRLF;
	holdRating += g_pTheDB->GetMessageText(MID_Rating);
	holdRating += wszSpace;
	UTF8ToUnicode(pHoldData->rating.c_str(), temp);
	holdRating += temp;
	holdRating += wszCRLF;
	holdRating += g_pTheDB->GetMessageText(MID_Votes);
	holdRating += wszSpace;
	UTF8ToUnicode(pHoldData->numVotes.c_str(), temp);
	holdRating += temp;
	holdRating += wszCRLF;
	holdRating += g_pTheDB->GetMessageText(MID_FileSize);
	holdRating += wszSpace;
	UTF8ToUnicode(pHoldData->filesize.c_str(), temp);
	holdRating += temp;
	holdRating += wszCRLF;
	holdRating += g_pTheDB->GetMessageText(MID_Version);
	holdRating += wszSpace;
	holdRating += g_pTheDB->GetMessageText(GetVersionMID(pHoldData->wVersion));
	this->pHoldDesc->SetText(holdRating.c_str());
}

//*****************************************************************************
void CHoldSelectScreen::SetHoldDesc()
//Updates name/description fields of selected hold.
//Sets export permissions and download availability.
{
	const UINT dwHoldID = GetSelectedItem();
	vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();
	const bool bIsCaravelNetHold = g_pTheNet->IsLocalHold(dwHoldID);

	this->pDownloadButton->Enable(g_pTheNet->IsUpdatedHold(dwHoldID));
	this->pOKButton->Enable();
	ShowActiveRoom(dwHoldID);

	CDbHold* pHold = g_pTheDB->Holds.GetByID(dwHoldID, true);
	ShowCaravelNetWidgets(!cNetMedia.empty(), pHold != NULL, bIsCaravelNetHold,
			g_pTheNet->IsBetaHold(dwHoldID));
	if (!pHold)
	{
		//This is not a locally installed hold.
		this->pExportButton->Disable();
		this->pHoldDesc->SetText(wszEmpty);

		if (!hold_is_installed(dwHoldID))
		{
			//This CaravelNet hold hasn't been imported yet.
			const WSTRING holdName = GetSelectedItemText();
			//Get hold author name.
			this->pAuthorName->SetText(wszEmpty);
			bool bCanDownloadHold = false;
			const int nIndex = g_pTheNet->getIndexForName(holdName.c_str());
			if (nIndex >= 0)
			{
				//Show hold's CaravelNet info.
				SetCNetHoldDesc(cNetMedia[nIndex]);
				bCanDownloadHold = true;
			}
			this->pDeleteButton->Disable();
			this->pDownloadButton->Enable(bCanDownloadHold);
			if (this->pHoldListBoxWidget->GetSelectedLineCount() && !bCanDownloadHold)
				this->pHoldDesc->SetText(g_pTheDB->GetMessageText(MID_OrderHoldAtCaravelGames));
			this->pOKButton->Disable();
		}

		return;
	}

	//Display hold info.
	WSTRING wstrAuthorText = g_pTheDB->GetMessageText(MID_LevelBy);
	wstrAuthorText += wszSpace;
	wstrAuthorText += pHold->GetAuthorText();
	this->pAuthorName->SetText(wstrAuthorText.c_str());
	this->pHoldDesc->SetText(pHold->DescriptionText);

#ifdef ENABLE_CHEATS
	this->pExportButton->Enable();
	this->pDeleteButton->Enable();
#else
	//Player can only publish holds and export texts from holds they authored.
	const bool bHoldAuthor = pHold->dwPlayerID == g_pTheDB->GetPlayerID();
	this->pExportButton->Enable(bHoldAuthor);
	//Only authored and non-preinstalled holds may be deleted.
	this->pDeleteButton->Enable(bHoldAuthor || CanDeleteHold(pHold->status));
#endif
	delete pHold;

	//Can rate hold if it is an installed CaravelNet hold.
	if (!bIsCaravelNetHold)
		return;

	//Show how the player has previously rated this CaravelNet hold.
	//
	//However, if the active player's CaravelNet user is the publisher of this
	//hold on CaravelNet, then show the average hold rating and don't allow voting.
	//Get original name of hold (without extra text decorations).
	const WSTRING holdName = g_pTheDB->Holds.GetHoldName(GetSelectedItem());
	const int nIndex = g_pTheNet->getIndexForName(holdName.c_str());
	if (nIndex >= 0)
	{
		CNetMedia& cNetHold = *(cNetMedia[nIndex]);

		double fRating;
		BYTE sliderIndex;
		bool bPreviouslyRated = false;

		CSliderWidget *pSlider = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_RATEDIFFICULTY));
		{
			//Convert string to float.
			fRating = atof(cNetHold.bPlayerIsAuthor ? cNetHold.difficulty.c_str() : cNetHold.myDifficulty.c_str());
			if (fRating >= 0.0)
			{
				sliderIndex = BYTE((fRating * pSlider->GetNumTicks() / 10.0) - 1); //(0,10] -> enum
				bPreviouslyRated = true;
			}
			else sliderIndex = pSlider->GetNumTicks() / 2;
		}
		pSlider->SetValue(sliderIndex);
		SetHoldRatingLabel(TAG_RATEDIFFICULTY);

		pSlider = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_RATEOVERALL));
		{
			fRating = atof(cNetHold.bPlayerIsAuthor ? cNetHold.rating.c_str() : cNetHold.myRating.c_str());
			if (fRating >= 0.0)
			{
				sliderIndex = BYTE((fRating * pSlider->GetNumTicks() / 10.0) - 1); //(0,10] -> enum
				bPreviouslyRated = true;
			}
			else sliderIndex = pSlider->GetNumTicks() / 2;
		}
		pSlider->SetValue(sliderIndex);
		SetHoldRatingLabel(TAG_RATEOVERALL);

		CWidget *pWidget = GetWidget(TAG_HOLDCNETAUTHOR);
		pWidget->Show(cNetHold.bPlayerIsAuthor);

		pWidget = GetWidget(TAG_RATEHOLD);
		pWidget->Enable(!cNetHold.bPlayerIsAuthor);
		CButtonWidget *pButton = DYN_CAST(CButtonWidget*, CWidget*, pWidget);
		pButton->SetCaption(g_pTheDB->GetMessageText(pWidget->IsEnabled() && bPreviouslyRated ?
				MID_ChangeHoldRating : MID_RateHold));

		pWidget = GetWidget(TAG_NOTRATED);
		pWidget->Show(!bPreviouslyRated && !cNetHold.bPlayerIsAuthor);
	}
}

//*****************************************************************************
void CHoldSelectScreen::SetHoldRatingLabel(const UINT dwTagNo)
//Set the numeric labels depicting hold rating.
{
	char temp[16];
	CSliderWidget *pSlider = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(dwTagNo));
	const UINT wVal = UINT(((1+pSlider->GetValue())/(float)pSlider->GetNumTicks()) * 100); //(0,100]
	string str = _itoa(wVal/10, temp, 10);
	str += ".";
	str += _itoa(wVal%10, temp, 10);
	WSTRING wstr;
	UTF8ToUnicode(str.c_str(), wstr);
	CLabelWidget *pLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(
			dwTagNo == TAG_RATEDIFFICULTY ? TAG_DIFFNUM_LABEL : TAG_OVERALLNUM_LABEL));
	pLabel->SetText(wstr.c_str());
}

//*****************************************************************************
void CHoldSelectScreen::SetHoldFilter()
{
	//Filter full hold list to visible display.
	this->pHoldListBoxWidget->Clear();
	for (vector<UINT>::const_iterator holdOrderIt = this->holdOrder.begin();
			holdOrderIt != this->holdOrder.end(); ++holdOrderIt)
	{
		const UINT holdID = *holdOrderIt;

		ASSERT(this->holdInfo.count(holdID) > 0);
		const HoldInfo& h = this->holdInfo[holdID];

		if (showHoldInList(h))
		{
			const UINT dwKey = holdID;
			const bool bLocalHold = this->pFullHoldList->IsItemEnabled(dwKey);
			const UINT insertedAtIndex = this->pHoldListBoxWidget->AddItem(dwKey,
					this->pFullHoldList->GetTextForKey(dwKey),
					!bLocalHold, -1, true); //place non-local holds last
			this->pHoldListBoxWidget->EnableItemAtIndex(insertedAtIndex);

			if (h.status == CDbHold::Official || CDbHold::IsOfficialHold(CDbHold::HoldStatus(h.status))) //make official holds stand out
			{
				static const SDL_Color Purple = {96, 0, 96, 0};
				this->pHoldListBoxWidget->SetItemColorAtLine(insertedAtIndex, Purple);
			} else if (h.bBeta) { //make beta holds stand out
				this->pHoldListBoxWidget->SetItemColorAtLine(insertedAtIndex, Maroon);
			} else if (!bLocalHold)	{
				//Color-code holds on CaravelNet by game version.
				static const SDL_Color DarkGold = {114, 85, 2, 0};
				if (h.version < 200) //JtRH
					this->pHoldListBoxWidget->SetItemColorAtLine(insertedAtIndex, Gray);
				else if (h.version < 300) //TCB
					this->pHoldListBoxWidget->SetItemColorAtLine(insertedAtIndex, DarkGold);
				else if (h.version < 400) //GatEB
					this->pHoldListBoxWidget->SetItemColorAtLine(insertedAtIndex, DarkBlue);
				else if (h.version < 500) //TSS
					this->pHoldListBoxWidget->SetItemColorAtLine(insertedAtIndex, DarkGreen);
				else if (h.version < NEXT_VERSION_NUMBER) //unknown version
					this->pHoldListBoxWidget->SetItemColorAtLine(insertedAtIndex, Aqua);
				else
					this->pHoldListBoxWidget->SetItemColorAtLine(insertedAtIndex, DarkBrown);
			}
		}
	}
	this->pHoldListBoxWidget->SelectLine(0); //default

	//Select current hold if in the current filter.
	UINT dwSelectHoldID = g_pTheDB->GetHoldID();
	if (g_pTheSM->GetReturnScreenType() == SCR_EditSelect)
	{
		//If coming from the level editor, select the hold being edited
		CEditSelectScreen *pESS = DYN_CAST(CEditSelectScreen*, CScreen*,
				g_pTheSM->GetScreen(SCR_EditSelect));
		if (pESS->GetPrevHoldID())
			dwSelectHoldID = pESS->GetPrevHoldID();
	}

	this->pHoldListBoxWidget->SelectItem(dwSelectHoldID);

	this->pHoldListBoxWidget->RequestPaint();
}

//*****************************************************************************
bool CHoldSelectScreen::showHoldInList(const HoldInfo& h) const
{
	switch (this->filter)
	{
		default:
		case F_ALL: return true;
		case F_MINE: return h.bMine;
		case F_NEW: return !h.bInProgress && h.bLocalHold;
		case F_INPROGRESS: return h.bInProgress;
		case F_CONQUERED: return h.bConquered;
		case F_UNCONQUERED: return !h.bConquered && h.bLocalHold;
		case F_CONQUERED_NOT_MASTERED: return h.bConquered && !h.bMastered;
		case F_MASTERED: return h.bMastered;
		case F_UNMASTERED: return !h.bMastered && h.bLocalHold;
		case F_CNET: return h.bCaravelNetHold;
		case F_NONCNET: return !h.bCaravelNetHold;
		case F_BETA: return h.bBeta;
		case F_UPDATED: return h.bUpdated;
		case F_OFFICIAL: return h.status != CDbHold::Homemade &&
				h.status != CDbHold::Tutorial && h.status != CDbHold::NoStatus;
	}
}

//*****************************************************************************
double CHoldSelectScreen::getSortVal(const UINT eSortType, const CNetMedia* pHoldData, const UINT wIndex)
//Returns: this hold's value according to indicated sorting criterion.
{
	static const double OVER_MAXVAL = 9999999;
	double fVal = 0.0;
	switch (eSortType)
	{
		default:
		case S_ALPHA:
			//Insert in order -- the list widget will sort items alphabetically.
			fVal = float(wIndex);
		break;
		case S_RATING:
		{
			if (pHoldData)
				fVal = atof(pHoldData->rating.c_str());
			if (!fVal)
				fVal = OVER_MAXVAL; //non-CNet (incl. BETA) hold -- set at max value to put at top of list
		}
		break;
		case S_DIFFICULTY:
		{
			if (pHoldData)
				fVal = atof(pHoldData->difficulty.c_str());
			if (!fVal)
				fVal = OVER_MAXVAL; //non-CNet (incl. BETA) hold -- set at max value to put at top of list
		}
		break;
		case S_VERSION:
			if (pHoldData)
				fVal = float(pHoldData->wVersion);
			else
				fVal = float(wIndex); //unknown -- will yield the order of local installation
		break;
		case S_VOTES:
		{
			if (pHoldData)
				fVal = atof(pHoldData->numVotes.c_str());
			if (!fVal)
				fVal = OVER_MAXVAL; //non-CNet (incl. BETA) -- set at max value to put at top of list
		}
		break;
	}
	return fVal;
}

//*****************************************************************************
void CHoldSelectScreen::PopulateHoldListBox()
//Put the holds in the DB into the hold list box.
{
	SetCursor(CUR_Wait);
	this->pFullHoldList->Clear();
	this->holdInfo.clear();
	this->holdOrder.clear();
	this->wProcessingHoldLine = 0;

	PopulateHoldListWithLocalHolds();
	PopulateHoldListWithNonLocalCaravelNetHolds();

	this->pFullHoldList->SortAlphabetically(true);

	if (this->filter != F_ALL)
		while (DecorateHoldText()) ; //complete this to gather all info before setting a filter
	SetHoldFilter();

	SetCursor();
}

//*****************************************************************************
void CHoldSelectScreen::PopulateHoldListWithLocalHolds()
{
	const UINT playerID = g_pTheDB->GetPlayerID();

	const CIDSet& localCNetHolds = g_pTheNet->GetLocalHolds();
	const CIDSet& updatedHolds = g_pTheNet->GetUpdatedHolds();
	const CIDSet& betaHolds = g_pTheNet->GetBetaHolds();
	const vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();

	HoldSortMap holdSortMap;
	const UINT eSortType = this->pCNetHoldSortList->GetSelectedItem();
	this->pFullHoldList->SortAlphabetically(eSortType == S_ALPHA);

	//Add sort list of local holds.
	vector<HoldInfo> localHoldInfo;
	CDb db;

	for (CDbHold *pHold = db.Holds.GetFirst(true); pHold != NULL; pHold = db.Holds.GetNext())
	{
#ifndef ENABLE_CHEATS
		//Don't show tutorial hold in list.
		if (pHold->status == CDbHold::Tutorial)
		{
			delete pHold;
			continue;
		}
#endif

		HoldInfo localhold;
		localhold.bMine = pHold->dwPlayerID == playerID;
		localhold.bCaravelNetHold = localCNetHolds.has(pHold->dwHoldID);
		localhold.bBeta = betaHolds.has(pHold->dwHoldID);
		localhold.bLocalHold = true;
		localhold.status = pHold->status;
		localhold.bUpdated = updatedHolds.has(pHold->dwHoldID);
		localhold.localHoldID = pHold->dwHoldID;
		localhold.holdNameText = static_cast<WSTRING>(pHold->NameText);
		//localhold.bConquered = localhold.bMastered = localhold.bInProgress = unknown;
		//localhold.version = unknown;
		delete pHold;

		const UINT index = localHoldInfo.size();
		localHoldInfo.push_back(localhold);

		//Cross-reference hold's CaravelNet rating.
		CNetMedia *pHoldData = NULL;
		const int cNetMediaIndex = g_pTheNet->getMediaIndexWithLocalHoldID(localhold.localHoldID);
		if (cNetMediaIndex >= 0)
			pHoldData = cNetMedia[cNetMediaIndex];

		//Sort.
		const double fVal = getSortVal(eSortType, pHoldData, index);
		holdSortMap.insert(std::make_pair(fVal, index));
	}

	//Display local holds in sorted (descending) order.
	HoldSortMap::reverse_iterator hold;
	for (hold = holdSortMap.rbegin(); hold != holdSortMap.rend(); ++hold)
	{
		//Get all items with this value.
		UINT index = hold->second;
		const HoldInfo& localhold = localHoldInfo[index];

		WSTRING wStr = localhold.holdNameText;
		//This local hold matches a published beta CaravelNet hold?
		if (localhold.bBeta)
		{
			//Notify this is a hold in beta.
			wStr += wszSpace;
			wStr += wszLeftParen;
			wStr += g_pTheDB->GetMessageText(MID_BetaHold);
			wStr += wszRightParen;
		}
		//There is an updated version of this local hold on CaravelNet?
		if (localhold.bUpdated)
		{
			//Notify an update to this CaravelNet hold is available.
			wStr += wszSpace;
			wStr += wszLeftParen;
			wStr += g_pTheDB->GetMessageText(MID_UpdatedHoldExists);
			wStr += wszRightParen;
		}

		const UINT holdID = localhold.getID();
		const UINT wInsertedAtIndex = this->pFullHoldList->AddItem(holdID, wStr.c_str());
		this->holdOrder.insert(this->holdOrder.begin() + wInsertedAtIndex, holdID);

		this->holdInfo.insert(make_pair(holdID, localhold));
	}
}

//*****************************************************************************
void CHoldSelectScreen::PopulateHoldListWithNonLocalCaravelNetHolds()
{
	const vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();

	//Add sorted list of holds on CaravelNet.
	HoldSortMap holdSortMap;
	const UINT eSortType = this->pCNetHoldSortList->GetSelectedItem();

	UINT wIndex;
	for (wIndex=0; wIndex<cNetMedia.size(); ++wIndex)
	{
		const CNetMedia& cNetHold = *(cNetMedia[wIndex]);

		//Skip non-holds.
		if (cNetHold.mediaType != MT_Hold) continue;

		//Skip any holds without a server ID.
		if (!cNetHold.lHoldID) continue;

		//Skip holds from a future version of the game.
		if (cNetHold.wVersion >= NEXT_VERSION_NUMBER) continue;

		//Show non-installed CaravelNet holds only.
		if (!cNetHold.localHoldID)
		{
			//Sort.
			const double fVal = getSortVal(eSortType, &cNetHold, wIndex);
			holdSortMap.insert(std::make_pair(fVal, wIndex));
		}
		//else it was already added to list as a local hold above
	}

	//Display non-local holds (in descending order).
	for (HoldSortMap::reverse_iterator hold = holdSortMap.rbegin(); hold != holdSortMap.rend(); ++hold)
	{
		//Get all items with this value.
		UINT index = hold->second;
		const CNetMedia& cNetHold = *(cNetMedia[index]);
		WSTRING holdName = (const WCHAR*)cNetHold.HoldNameText;
		HoldInfo holdinfo;
		holdinfo.bCaravelNetHold = true;
		holdinfo.bBeta = cNetHold.bBeta;
		holdinfo.status = cNetHold.status;
		holdinfo.version = cNetHold.wVersion;
		holdinfo.cnetID = cNetHold.lHoldID;

		const UINT holdID = holdinfo.getID();
		const UINT wInsertedAtIndex = this->pFullHoldList->AddItem(holdID, holdName.c_str(), true, -1, true); //place after local holds
		this->holdOrder.insert(this->holdOrder.begin() + wInsertedAtIndex, holdID);

		this->holdInfo.insert(make_pair(holdID, holdinfo));
	}
}

//*****************************************************************************
void CHoldSelectScreen::ShowActiveRoom(const UINT dwHoldID)
//Shows the room currently being played by the current player in this hold.
//Shows the hold entrance room if none.
{
	//Update room position label.
	WSTRING wstrDesc;
	CLabelWidget *pPositionLabel = DYN_CAST(CLabelWidget*, CWidget*,
			GetWidget(TAG_POSITION_LABEL));

	const UINT dwContinueGameID = dwHoldID ? g_pTheDB->SavedGames.FindByContinue(dwHoldID) : 0;

	//Show game from the hold's continue slot.
	CCueEvents Ignored;
	delete this->pCurrentRestoreGame;
	this->pCurrentRestoreGame = dwContinueGameID ?
			g_pTheDB->GetSavedCurrentGame(dwContinueGameID, Ignored, true,
			true //don't save anything to DB during playback
			) : NULL;
	if (!this->pCurrentRestoreGame)
	{
		//No continue slot yet, load from beginning of hold.
		this->pCurrentRestoreGame = g_pTheDB->GetNewCurrentGame(dwHoldID, Ignored, ASO_NONE);
	}
	if (this->pCurrentRestoreGame)
		this->pCurrentRestoreGame->Commands.GetFirst();

	//Show room if valid.
	CWidget *pRoomDisplay = GetWidget(TAG_MINIROOM);
	const bool bGame = this->pCurrentRestoreGame != NULL;
	pRoomDisplay->Show(bGame);
	pPositionLabel->Show(bGame);

	//Hide room thumbnail by default.
	pRoomDisplay = GetWidget(TAG_THUMBNAIL);
	pRoomDisplay->Hide();
	this->pSelCNetHold = NULL; //no thumbnail pending

	if (bGame)
	{
		VERIFY(this->pRoomWidget->LoadFromCurrentGame(this->pCurrentRestoreGame));
		CCueEvents Ignored;
		this->pRoomWidget->DisplayPersistingImageOverlays(Ignored);

		this->pRoomWidget->RequestPaint();
		wstrDesc += (const WCHAR*)this->pCurrentRestoreGame->pLevel->NameText;
		wstrDesc += wszColon;
		wstrDesc += wszSpace;
		this->pCurrentRestoreGame->pRoom->GetLevelPositionDescription(wstrDesc);
		pPositionLabel->SetText(wstrDesc.c_str());
	} else {
		this->pRoomWidget->UnloadCurrentGame();
	}
}

//*****************************************************************************
void CHoldSelectScreen::ShowCaravelNetWidgets(
//Shows or hides CaravelNet widgets based on status.
	const bool bLoggedIn, const bool bLocalHold, const bool bCaravelNetHold, const bool bBetaHold)
{
	//Show these widgets when not logged in to CaravelNet.
	CWidget *pWidget = GetWidget(TAG_NOT_LOGGED_IN);
	pWidget->Show(!bLoggedIn);

	pWidget = GetWidget(TAG_CNETSORTER_LABEL);
	pWidget->Show(bLoggedIn);
	pWidget = GetWidget(TAG_CNETSORTER);
	pWidget->Show(bLoggedIn);

	pWidget = GetWidget(TAG_NOTPUBLISHED);
	pWidget->Show(bLoggedIn && bLocalHold && !bCaravelNetHold);

	pWidget = GetWidget(TAG_BETA);
	pWidget->Show(bLoggedIn && bCaravelNetHold && bBetaHold);

	//Show these widgets for a CaravelNet release hold.
	static const UINT wNumShowWidgets = 10;
	const UINT dwShowWidgets[wNumShowWidgets] = {
		TAG_RATEHOLD, TAG_RATEDIFFICULTY, TAG_RATEOVERALL, TAG_DIFFNUM_LABEL,
		TAG_OVERALLNUM_LABEL, TAG_NOTRATED, TAG_CNET_OVERALL_LABEL,
		TAG_CNET_DIFFICULTY_LABEL, TAG_HOLDCNETAUTHOR, TAG_UPLOADHOLDSCORES
	};
	for (UINT i=wNumShowWidgets; i--; )
	{
		pWidget = GetWidget(dwShowWidgets[i]);
		pWidget->Show(bCaravelNetHold && !bBetaHold);
	}
}

//*****************************************************************************
void CHoldSelectScreen::UploadHoldScores()
//Submits all player scores for the selected hold to CaravelNet.
{
	const UINT dwHoldID = GetSelectedItem();
	const bool bIsCaravelNetHold = g_pTheNet->IsLocalHold(dwHoldID);
	if (!bIsCaravelNetHold)
		return;

	SetCursor(CUR_Wait);
	g_pTheDB->Commit();  //commit any outstanding changes now

	SetCursor(CUR_Internet);
	ShowStatusMessage(MID_Exporting);
	CDrodScreen::Callbackf(0.0f);

	//Part 1. Upload general hold progress.
	const UINT dwPlayerID = g_pTheDB->GetPlayerID();
	string text;
	ExportHoldProgressForUpload(dwHoldID, dwPlayerID, text);
	if (!text.empty())
		g_pTheNet->UploadExploredRooms(text);
	CDrodScreen::Callbackf(0.1f);
	g_pTheDB->Commit(); //save current key

	//Part 2. Upload hold victory demos.
	CDb db;
	db.Demos.FilterByPlayer(dwPlayerID);
	db.Demos.FilterByHold(dwHoldID);
	db.Demos.FindHiddens(true);
	CIDSet uploadedDemoIDs, allDemoIDs = db.Demos.GetIDs();

	//We are searching for victory demos, so don't upload death demos.
	CIDSet::const_iterator demo;
	for (demo = allDemoIDs.begin(); demo != allDemoIDs.end(); ++demo)
	{
		CDbDemo *pDemo = db.Demos.GetByID(*demo);
		ASSERT(pDemo);
		if (!pDemo) continue;
		if (!pDemo->IsFlagSet(CDbDemo::Death))
			uploadedDemoIDs += *demo;
		delete pDemo;
	}
	if (!uploadedDemoIDs.empty())
	{
		string text;
		if (CDbXML::ExportXML(V_Demos, uploadedDemoIDs, text, (UINT)-1)) //no multi-room demos
		{
			const UINT handle = g_pTheNet->UploadDemos(text);
			if (handle)
			{
				const UINT MAX_TRIES = 200; //200 * 50ms = 10s total wait time for each response
				UINT tries;
				for (tries = 0; tries < MAX_TRIES; ++tries)
				{
					const int status = g_pTheNet->GetStatus(handle);
					if (status < 0)
					{
						SDL_Delay(50);
						continue;
					}

					CNetResult* pBuffer = g_pTheNet->GetResults(handle);
					if (pBuffer) {
						delete pBuffer;

						//Now flag all uploaded demos.
						for (demo = uploadedDemoIDs.begin(); demo != uploadedDemoIDs.end(); ++demo)
						{
							CDbDemo* pDemo = db.Demos.GetByID(*demo);
							ASSERT(pDemo);
							pDemo->SetFlag(CDbDemo::TestedForUpload); //don't submit this demo next time
							pDemo->Update();
							delete pDemo;
						}
					} else {
						ShowOkMessage(MID_CaravelServerError);
					}
					break;
				}
			}
		}
	}

	CDrodScreen::Callbackf(1.0f);
	g_pTheDB->Commit(); //save current key
	ExportCleanup();
	Paint();
}

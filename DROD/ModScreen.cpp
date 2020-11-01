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
 * Portions created by the Initial Developer are Copyright (C) 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributors:
 *
 * ***** END LICENSE BLOCK ***** */

#include "ModScreen.h"
#include "BrowserScreen.h"
#include "EditSelectScreen.h"
#include "DrodFontManager.h"
#include "GameScreen.h"

#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/ImageWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/OptionButtonWidget.h>
#include <FrontEndLib/ProgressBarWidget.h>
#include <FrontEndLib/SliderWidget.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/DbXML.h"
#include "../DRODLib/SettingsKeys.h"

#include "../Texts/MIDs.h"
#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Base64.h>
#include <BackEndLib/Ports.h>
#include <sstream>

const UINT TAG_LIST_BOX = 1100;
const UINT TAG_CANCEL = 1101;
const UINT TAG_EXPORT = 1102;
const UINT TAG_IMPORT = 1103;
const UINT TAG_DOWNLOAD = 1104;
const UINT TAG_DELETE = 1105;
const UINT TAG_HELP = 1106;

const UINT TAG_RATE = 1110;
const UINT TAG_RATESLIDER = 1111;
const UINT TAG_RATINGNUM_LABEL = 1112;
const UINT TAG_NOT_LOGGED_IN = 1113;
const UINT TAG_NOTRATED = 1114;
const UINT TAG_CNET_RATING_LABEL = 1115;
const UINT TAG_NOTPUBLISHED = 1116;

const UINT TAG_MODFILTER = 1130;
const UINT TAG_THUMBNAIL = 1131;

//
//Public methods.
//

//*****************************************************************************
CModScreen::CModScreen()
	: CDrodScreen(SCR_Mod)
	, pModListBoxWidget(NULL), pFullModList(NULL)
	, pDesc(NULL), pAuthorName(NULL)
	, pDetailsFrame(NULL)
	, pExportButton(NULL), pDownloadButton(NULL), pDeleteButton(NULL)
	, filter(F_ALL)
	, pSelCNetMod(NULL)
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
	static const UINT CX_TITLE = 400;
#else
	static const UINT CX_TITLE = 300;
#endif
	static const UINT CY_TITLE = 52;
	const int X_TITLE = (this->w - CX_TITLE) / 2;
	static const int Y_TITLE = CY_TITLE_SPACE;

	//List box.
	static const int X_LBOX_HEADER = CX_SPACE;
	const UINT CX_LBOX_HEADER = 200;

#ifdef RUSSIAN_BUILD
	static const int Y_LBOX_HEADER = Y_TITLE + CY_TITLE;
	static const UINT CY_LBOX_HEADER = 54;
#else
	static const int Y_LBOX_HEADER = Y_TITLE + CY_TITLE + 22;
	static const UINT CY_LBOX_HEADER = 32;
#endif

	static const int X_MODLISTBOX = X_LBOX_HEADER;
	static const int Y_MODLISTBOX = Y_LBOX_HEADER + CY_LBOX_HEADER + 3;
	const UINT CX_MODLISTBOX = ((this->w - CX_SPACE) / 2) - CX_SPACE;
	static const UINT CY_MODLISTBOX = 400;   //18 items * 22 + 4

	static const UINT CX_FILTER_LIST = 160;
	static const UINT CY_FILTER_LIST = 53;
	static const int X_FILTER_LIST = X_MODLISTBOX + CX_MODLISTBOX - CX_FILTER_LIST;
	static const int Y_FILTER_LIST = Y_TITLE + CY_TITLE;

#ifdef RUSSIAN_BUILD
	static const UINT CX_FILTER_LABEL = 70;
#else
	static const UINT CX_FILTER_LABEL = 50;
#endif
	static const int X_FILTER_LABEL = X_FILTER_LIST - CX_FILTER_LABEL;
	static const int Y_FILTER_LABEL = Y_FILTER_LIST;
	static const UINT CY_FILTER_LABEL = CY_BUTTON;

	//Buttons.
	const int X_IMPORT_BUTTON = X_MODLISTBOX;
	const int Y_IMPORT_BUTTON = Y_MODLISTBOX + CY_MODLISTBOX + CY_SPACE;
	static const int X_EXPORT_BUTTON = X_IMPORT_BUTTON + CX_BUTTON + CX_BUTTON_SPACE;
	static const int Y_EXPORT_BUTTON = Y_IMPORT_BUTTON;
	static const int X_DOWNLOAD_BUTTON = X_EXPORT_BUTTON + CX_BUTTON + CX_BUTTON_SPACE;
	static const int Y_DOWNLOAD_BUTTON = Y_IMPORT_BUTTON;
	static const UINT CX_DOWNLOAD_BUTTON = 116;
	static const int X_DELETE_BUTTON = X_DOWNLOAD_BUTTON + CX_DOWNLOAD_BUTTON + CX_BUTTON_SPACE;
	static const int Y_DELETE_BUTTON = Y_IMPORT_BUTTON;
	static const UINT CX_DELETE_BUTTON = CX_DOWNLOAD_BUTTON;

	const int X_OK_BUTTON = (this->w / 2) + 75;
	const int Y_OK_BUTTON = this->h - CY_BUTTON - CY_SPACE;

	const int X_HELP_BUTTON = X_OK_BUTTON + CX_BUTTON + CX_BUTTON_SPACE;
	const int Y_HELP_BUTTON = Y_OK_BUTTON;
	const int X_CANCEL_BUTTON = X_HELP_BUTTON + CX_BUTTON + CX_BUTTON_SPACE;
	const int Y_CANCEL_BUTTON = Y_OK_BUTTON;

	//CaravelNet frame.
	static const int X_CNET_FRAME = CX_SPACE;
	static const int Y_CNET_FRAME = Y_DOWNLOAD_BUTTON + CY_BUTTON + CY_SPACE;
	const UINT CX_CNET_FRAME = CX_MODLISTBOX;
	const UINT CY_CNET_FRAME = this->h - Y_CNET_FRAME - CY_SPACE;

	//Rating.
	static const int X_RATING_LABEL = CX_SPACE;
	static const UINT CX_RATING_LABEL = 100;
	static const int Y_RATING_LABEL = CY_SPACE;
	static const UINT CY_RATING_LABEL = 26 + CY_SPACE/2;

	static const int X_RATING_SLIDER = X_RATING_LABEL + CX_RATING_LABEL;
	static const UINT CX_RATING_SLIDER = 310;
	static const int Y_RATING_SLIDER = Y_RATING_LABEL;
	static const UINT CY_RATING_SLIDER = CY_STANDARD_SLIDER;

	static const int X_RATINGNUM_LABEL = X_RATING_SLIDER + CX_RATING_SLIDER + CX_SPACE;
	static const UINT CX_RATINGNUM_LABEL = 50;
	static const int Y_RATINGNUM_LABEL = Y_RATING_SLIDER;
	static const UINT CY_RATINGNUM_LABEL = CY_RATING_LABEL;

	static const UINT CX_RATE_BUTTON = 150;
	static const UINT CY_RATE_BUTTON = CY_STANDARD_BUTTON;
	const int X_RATE_BUTTON = (CX_CNET_FRAME - CX_RATE_BUTTON) / 2;
	static const int Y_RATE_BUTTON = CY_CNET_FRAME - CY_RATE_BUTTON - CY_SPACE;

	//CaravelNet status texts.
	static const int X_NOTLOGGEDIN = CX_SPACE;
	static const UINT CX_NOTLOGGEDIN = CX_CNET_FRAME - X_NOTLOGGEDIN*2;
	static const int Y_NOTLOGGEDIN = CY_SPACE;
	static const UINT CY_NOTLOGGEDIN = 26 + CY_SPACE/2;

	static const int X_NOTRATED = X_NOTLOGGEDIN;
	static const UINT CX_NOTRATED = CX_NOTLOGGEDIN;
	static const int Y_NOTRATED = Y_RATING_LABEL + CY_RATING_LABEL;
	static const UINT CY_NOTRATED = CY_NOTLOGGEDIN;

	static const int X_NOTPUBLISHED = X_NOTLOGGEDIN;
	static const UINT CX_NOTPUBLISHED = CX_NOTLOGGEDIN;
	static const int Y_NOTPUBLISHED = Y_NOTLOGGEDIN;
	static const UINT CY_NOTPUBLISHED = CY_NOTLOGGEDIN;

	//Details frame.
	static const UINT CX_DETAILS_FRAME = CX_MODLISTBOX;
	static const int Y_DETAILS_FRAME = Y_TITLE + CY_TITLE + 10;
	const UINT CY_DETAILS_FRAME = this->h - CY_SPACE - CY_STANDARD_BUTTON - CY_SPACE -
			Y_DETAILS_FRAME;
	static const int X_DETAILS_FRAME = X_MODLISTBOX + CX_MODLISTBOX + CX_SPACE;

	//Mod info.
	static const int X_LABEL = CX_SPACE;
	const UINT CX_LABEL = CX_DETAILS_FRAME - X_LABEL;
	static const int Y_AUTHOR_LABEL = CY_SPACE;
	static const UINT CY_AUTHOR_LABEL = 26 + CY_SPACE/2;
	static const int Y_DESC_LABEL = Y_AUTHOR_LABEL + CY_AUTHOR_LABEL;
	static const UINT CY_DESC_LABEL = 126;  //6 lines of text * 21

	//Thumbnail image (same size as miniroom).
	static const UINT CX_MINIROOM = CX_DETAILS_FRAME - CX_SPACE - CX_SPACE;
	const UINT CY_MINIROOM = (CX_MINIROOM * CDrodBitmapManager::CY_ROOM) /
			CDrodBitmapManager::CX_ROOM;
	static const int X_MINIROOM = CX_SPACE;
	const int Y_MINIROOM = CY_DETAILS_FRAME - CY_SPACE - CY_MINIROOM;

	//Title.
	AddWidget(new CLabelWidget(0L, X_TITLE, Y_TITLE,
			CX_TITLE, CY_TITLE, F_Title, g_pTheDB->GetMessageText(MID_ModManagementTitle)));

	//Mod list box.
	AddWidget(new CLabelWidget(0L, X_LBOX_HEADER, Y_LBOX_HEADER, CX_LBOX_HEADER,
			CY_LBOX_HEADER, F_Header, g_pTheDB->GetMessageText(MID_InstalledMods)));

	this->pModListBoxWidget = new CListBoxWidget(TAG_LIST_BOX,
			X_MODLISTBOX, Y_MODLISTBOX, CX_MODLISTBOX, CY_MODLISTBOX, true,
			false, true);
	AddWidget(this->pModListBoxWidget);
	//Used for storing mod info.  Not added to the screen as a child widget.
	this->pFullModList = new CListBoxWidget(0, X_MODLISTBOX, Y_MODLISTBOX,
			CX_MODLISTBOX, CY_MODLISTBOX, false, false, true);

	AddWidget(new CLabelWidget(0L, X_FILTER_LABEL, Y_FILTER_LABEL, CX_FILTER_LABEL,
			CY_FILTER_LABEL, F_Small, g_pTheDB->GetMessageText(MID_HoldFilter)));
	CListBoxWidget *pModFilterList = new CListBoxWidget(TAG_MODFILTER,
			X_FILTER_LIST, Y_FILTER_LIST, CX_FILTER_LIST, CY_FILTER_LIST);
	pModFilterList->AddItem(F_ALL, g_pTheDB->GetMessageText(MID_HoldFilterAll));
	pModFilterList->AddItem(F_CNET, g_pTheDB->GetMessageText(MID_HoldFilterCaravelNet));
	pModFilterList->AddItem(F_NONCNET, g_pTheDB->GetMessageText(MID_HoldFilterNonCaravelNet));
	pModFilterList->SelectLine(0);
	AddWidget(pModFilterList);

	//Buttons.
	CButtonWidget *pImportButton = new CButtonWidget(
			TAG_IMPORT, X_IMPORT_BUTTON, Y_IMPORT_BUTTON, CX_BUTTON, CY_BUTTON,
			g_pTheDB->GetMessageText(MID_Import));
	AddWidget(pImportButton);
	this->pExportButton = new CButtonWidget(
			TAG_EXPORT, X_EXPORT_BUTTON, Y_EXPORT_BUTTON, CX_BUTTON, CY_BUTTON,
			g_pTheDB->GetMessageText(MID_Export));
	AddWidget(this->pExportButton);
	this->pDownloadButton = new CButtonWidget(
			TAG_DOWNLOAD, X_DOWNLOAD_BUTTON, Y_DOWNLOAD_BUTTON, CX_DOWNLOAD_BUTTON,
			CY_BUTTON, g_pTheDB->GetMessageText(MID_Download));
	AddWidget(this->pDownloadButton);
	this->pDeleteButton = new CButtonWidget(
			TAG_DELETE, X_DELETE_BUTTON, Y_DELETE_BUTTON, CX_DELETE_BUTTON,
			CY_BUTTON, g_pTheDB->GetMessageText(MID_DeleteNoHotkey));
	AddWidget(this->pDeleteButton);

	//CaravelNet operations.
	CFrameWidget *pCNetFrame = new CFrameWidget(0L, X_CNET_FRAME, Y_CNET_FRAME,
			CX_CNET_FRAME, CY_CNET_FRAME, g_pTheDB->GetMessageText(MID_CaravelNet));
	AddWidget(pCNetFrame);

	CLabelWidget *pLabel = new CLabelWidget(TAG_NOT_LOGGED_IN, X_NOTLOGGEDIN,
			Y_NOTLOGGEDIN, CX_NOTLOGGEDIN, CY_NOTLOGGEDIN,
			F_Small, g_pTheDB->GetMessageText(MID_CaravelNetNotLoggedIn));
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	pCNetFrame->AddWidget(pLabel);
	pLabel = new CLabelWidget(TAG_NOTRATED, X_NOTRATED, Y_NOTRATED, CX_NOTRATED,
			CY_NOTRATED, F_Small, g_pTheDB->GetMessageText(MID_ModNotRated));
	pCNetFrame->AddWidget(pLabel);
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	pLabel = new CLabelWidget(TAG_NOTPUBLISHED, X_NOTPUBLISHED, Y_NOTPUBLISHED, CX_NOTPUBLISHED,
			CY_NOTPUBLISHED, F_Small, g_pTheDB->GetMessageText(MID_MediaNotPublished));
	pCNetFrame->AddWidget(pLabel);
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);

	//Rating.
	pCNetFrame->AddWidget(new CLabelWidget(TAG_CNET_RATING_LABEL, X_RATING_LABEL, Y_RATING_LABEL,
			CX_RATING_LABEL, CY_RATING_LABEL, F_Small,
			g_pTheDB->GetMessageText(MID_ModRating)));
	pCNetFrame->AddWidget(new CSliderWidget(TAG_RATESLIDER, X_RATING_SLIDER, Y_RATING_SLIDER,
			CX_RATING_SLIDER, CY_RATING_SLIDER, 0, 10));
	pCNetFrame->AddWidget(new CLabelWidget(TAG_RATINGNUM_LABEL, X_RATINGNUM_LABEL, Y_RATINGNUM_LABEL,
			CX_RATINGNUM_LABEL, CY_RATINGNUM_LABEL, F_Small, wszEmpty));
	pCNetFrame->AddWidget(new CButtonWidget(TAG_RATE, X_RATE_BUTTON, Y_RATE_BUTTON,
			CX_RATE_BUTTON, CY_RATE_BUTTON, g_pTheDB->GetMessageText(MID_RateMod)));

	//Details frame.
	this->pDetailsFrame = new CFrameWidget(0L, X_DETAILS_FRAME,
			Y_DETAILS_FRAME, CX_DETAILS_FRAME, CY_DETAILS_FRAME,
			g_pTheDB->GetMessageText(MID_Details));
	AddWidget(this->pDetailsFrame);

	this->pAuthorName = new CLabelWidget(0L, X_LABEL, Y_AUTHOR_LABEL, CX_LABEL,
			CY_AUTHOR_LABEL, F_Small, wszEmpty);
	this->pAuthorName->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pDetailsFrame->AddWidget(this->pAuthorName);
	this->pDesc = new CLabelWidget(0L, X_LABEL, Y_DESC_LABEL, CX_LABEL,
			CY_DESC_LABEL, F_Small, wszEmpty);
	this->pDesc->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pDetailsFrame->AddWidget(this->pDesc);
	this->pDetailsFrame->AddWidget(new CImageWidget(TAG_THUMBNAIL,
			X_MINIROOM, Y_MINIROOM, (const WCHAR*)NULL));

	//General buttons.
	CButtonWidget *pOKButton = new CButtonWidget(
			TAG_OK, X_OK_BUTTON, Y_OK_BUTTON, CX_BUTTON, CY_BUTTON,
			g_pTheDB->GetMessageText(MID_Okay));
	AddWidget(pOKButton);
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
CModScreen::~CModScreen()
//Destructor.
{
	delete this->pFullModList;
}

//*****************************************************************************
void CModScreen::Paint(
//Overridable method to paint the screen.
//
//Params:
	bool bUpdateRect)	//(in)   If true (default) and destination
						//    surface is the screen, the screen
						//    will be immediately updated in
						//    the widget's rect.
{
	//Blit the background graphic.
	SDL_BlitSurface(this->images[0], NULL, GetDestSurface(), NULL);

	PaintChildren();

	if (this->pProgressWidget->IsVisible())
		this->pProgressWidget->Paint();

	if (bUpdateRect) UpdateRect();
}

//*****************************************************************************
UINT CModScreen::GetSelectedItem()
//Returns: tag of item selected in the list box widget.
{
	return this->pModListBoxWidget->GetSelectedItem();
}

//*****************************************************************************
bool CModScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	PopulateModListBox();
	SetDesc();

	return true;
}

//
// Private methods
//

//*****************************************************************************
void CModScreen::DeleteSelectedMods()
//Deletes the selected locally-installed CaravelNet mod(s).
{
	CFiles f;

	const CIDSet keys = this->pModListBoxWidget->GetSelectedItems();
	for (CIDSet::const_iterator key = keys.begin(); key != keys.end(); ++key)
	{
		WSTRING wModName = this->pModListBoxWidget->GetTextForKey(*key);

		//Delete mod from DB.
		CIDSet modDataIDs = g_pTheDB->Data.GetIDsForMod(wModName.c_str());
		for (CIDSet::const_iterator id = modDataIDs.begin(); id != modDataIDs.end(); ++id)
			if (g_pTheDB->Data.Exists(*id))
				g_pTheDB->Data.Delete(*id);

		//Remove style entry from INI also.
		string styleName = UnicodeToUTF8(wModName.c_str());
		f.DeleteINIEntry(INISection::Graphics, INIKey::Style, styleName.c_str());
		f.DeleteINIEntry(INISection::Graphics, styleName.c_str(), NULL);

		//Delete style skies entry, if exists.
		wModName += wszSpace;
		wModName += wszSKIES;
		styleName = UnicodeToUTF8(wModName.c_str());
		f.DeleteINIEntry(INISection::Graphics, styleName.c_str(), NULL);
	}

	PopulateModListBox();
	SetDesc();
	Paint();

	//Refresh active room style display when mods are deleted.
	if (GetDestScreenType() == SCR_EditSelect)
	{
		CEditSelectScreen *pEditSelectScreen = DYN_CAST(CEditSelectScreen*, CScreen*,
				g_pTheSM->GetScreen(SCR_EditSelect));
		ASSERT(pEditSelectScreen);
		const WSTRING style = pEditSelectScreen->GetSelectedStyle();
		if (!style.empty())
			g_pTheDBM->LoadTileImagesForStyle(style, true);
	}
}

//*****************************************************************************
void CModScreen::DownloadSelectedMods()
//Downloads and installs the selected CaravelNet mod(s).
{
	if (!g_pTheDB->GetPlayerID())
		return;

	const vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();
	CFiles f;

	MESSAGE_ID result = MID_Okay;
	const CIDSet lineNos = this->pModListBoxWidget->GetSelectedLineNumbers();
	WSTRING firstImportedName;
	for (CIDSet::const_iterator line = lineNos.begin(); line != lineNos.end(); ++line)
	{
		if (PollForOperationInterrupt())
			break;

		//Get download URL for selected media.
		const UINT dwKey = this->pModListBoxWidget->GetKeyAtLine(*line);
		WSTRING name = this->pModListBoxWidget->GetTextAtLine(*line);
		if (dwKey)
		{
			//Get original name of mod (without extra texts appended).
			name = g_pTheDB->Data.GetNameFor(dwKey);
		}

		//Get access info.
		const int nIndex = g_pTheNet->getIndexForName(name.c_str(), MT_Style);
		if (nIndex < 0)
			continue; //not found in CNet list
		const UINT wExpectedSize = atoi(cNetMedia[nIndex]->filesizebytes.c_str());

		//Init.
		SetCursor(CUR_Internet);
		CDrodScreen::callbackContext = name;
		Callback(MID_DownloadingMedia);

		//Download.
		const UINT handle = g_pTheNet->DownloadStyle(name.c_str());
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

		CNetResult* pBuffer = g_pTheNet->GetResults(handle);
		if (!pBuffer || !pBuffer->pBuffer || pBuffer->pBuffer->Size() < 10) {
			// error - can't download!
			delete pBuffer; HideStatusMessage(); SetCursor(); continue;
		}

		//Import.
		SetCursor(CUR_Wait);
		Callback(MID_ImportingData);
		CDbXML::SetCallback(this);
		result = CDbXML::ImportXML(*pBuffer->pBuffer, CImportInfo::Data);
		if (!ImportConfirm(result)) {delete pBuffer; HideStatusMessage(); SetCursor(); continue;}

		// Write the header info to the INI
		WSTRING wstr;
		Base64::decode(CDbXML::info.headerInfo, wstr);
		f.WriteGameProfileBuffer(wstr,false,false);
		delete pBuffer;
		if (firstImportedName.empty())
			firstImportedName = name.c_str();
	}

	//Done.
	g_pTheDB->Commit();
	ExportCleanup();
	ShowOkMessage(result);
	CDrodScreen::callbackContext.resize(0);

	this->filter = F_ALL; //ensure downloaded mod is shown in list
	PopulateModListBox();
	const UINT importedLine = this->pModListBoxWidget->GetLineWithText(firstImportedName.c_str());
	if (importedLine)
		this->pModListBoxWidget->SelectLine(importedLine);
	SetDesc();
	Paint();
}

//*****************************************************************************
bool CModScreen::IsCNetMod(const WCHAR* pName) const
//Returns: whether name belongs to a CNet mod (whether local or remote)
{
	//Is name listed in the CNet media list?
	return g_pTheNet->getIndexForName(pName, MT_Style) >= 0;
}

//*****************************************************************************
bool CModScreen::IsImportedMod(const WCHAR* pName) const
//Returns: whether name belongs to a mod set imported into the DB
{
	//Is mod in DB?
	CDb db;
	const CIDSet modIDs = db.Data.GetIDsForMod(pName);
	return !modIDs.empty();
}

//*****************************************************************************
bool CModScreen::IsLocalMod(const WCHAR* pName) const
//Returns: whether name belongs to a local mod (whether on disk or in DB)
{
	//Is room style listed?
	CFiles f;
	list<WSTRING> styles;
	f.GetGameProfileString(INISection::Graphics, INIKey::Style, styles);
	for (list<WSTRING>::const_iterator style = styles.begin(); style != styles.end(); ++style)
		if (!WCScmp(style->c_str(), pName))
			return true; //that means it's on disk

	//Is mod in DB?
	return IsImportedMod(pName);
}

//*****************************************************************************
void CModScreen::OnBetweenEvents()
{
	//If image thumbnail is pending, request it until received.
	if (this->pSelCNetMod)
		RequestThumbnail(this->pSelCNetMod);
}

//*****************************************************************************
void CModScreen::OnClick(
//Handles a button click.
//
//Params:
	const UINT dwTagNo)       //(in)   Widget that event applied to.
{
	switch (dwTagNo)
	{
	case TAG_OK:
		GoToScreen(SCR_Return);
		break;

	case TAG_EXPORT:
		ExportStyle(this->pModListBoxWidget->GetSelectedItemText());
		break;

	case TAG_HELP:
		CBrowserScreen::SetPageToLoad("modscreen.html");
		GoToScreen(SCR_Browser);
		break;

	case TAG_IMPORT:
		{
			//Import a data file.
			CIDSet importedIDs;
			set<WSTRING> importedStyles;
			if (Import(EXT_DATA, importedIDs, importedStyles) == MID_ImportSuccessful)
			{
				CFiles f;
				WSTRING wstr;
				Base64::decode(CDbXML::info.headerInfo, wstr);
				f.WriteGameProfileBuffer(wstr,false,false);
			}
			if (!importedIDs.empty())
			{
				const WSTRING modName = g_pTheDB->Data.GetModNameFor(importedIDs.getFirst());
				SelectImportedMod(modName);
			}
		}
		break;

	case TAG_DELETE:
		DeleteSelectedMods();
		break;

	case TAG_DOWNLOAD:
		DownloadSelectedMods();
		break;

	case TAG_RATE:
		RateMod();
		break;

	case TAG_CANCEL:
	case TAG_ESCAPE:
		GoToScreen(SCR_Return);
		break;
	case TAG_QUIT:
		if (ShowYesNoMessage(MID_ReallyQuit) != TAG_NO)
			GoToScreen(SCR_None);
		break;
	default: break;
	}
}

//*****************************************************************************
void CModScreen::OnDoubleClick(
//Handles double click event.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	switch (dwTagNo)
	{
	case TAG_LIST_BOX:
		{
			if (!this->pModListBoxWidget->ClickedSelection()) break;

			const UINT dwSelectedID = GetSelectedItem();
			if (!dwSelectedID)
			{
				DownloadSelectedMods();
			}
		}
		break;
	}
}

//*****************************************************************************
void CModScreen::OnKeyDown(
//Handles a keydown event.
//
//Params:
	const UINT dwTagNo, const SDL_KeyboardEvent &Key)
{
	CScreen::OnKeyDown(dwTagNo, Key);

	//Fast select.
	WCHAR wc = TranslateUnicodeKeysym(Key.keysym);
	if (Key.keysym.mod & KMOD_CTRL && iswalnum(wc))
	{
		if (this->pModListBoxWidget->SelectLineStartingWith(wc))
		{
			SetDesc();
			Paint();
		}
	}
}

//*****************************************************************************
void CModScreen::OnSelectChange(
//Handles a selection change.
//
//Params:
	const UINT dwTagNo)       //(in)   Widget that event applied to.
{
	switch (dwTagNo)
	{
	case TAG_LIST_BOX:
		//Update widgets with this mod's info.
		SetDesc();
		Paint();
		break;
	case TAG_MODFILTER:
		{
			CListBoxWidget *pFilterList = DYN_CAST(CListBoxWidget*, CWidget*, GetWidget(TAG_MODFILTER));
			this->filter = ModTypeFilter(pFilterList->GetSelectedItem());

			SetFilter();
			SetDesc();
			Paint();
		}
		break;

	case TAG_RATESLIDER:
		SetRatingLabel(dwTagNo);
		Paint();
		break;
	default: break;
	}
}

//*****************************************************************************
bool CModScreen::PollForOperationInterrupt()
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
void CModScreen::RateMod()
//Upload rating for selected mod to server.
{
	const WSTRING name = this->pModListBoxWidget->GetSelectedItemText();
	CSliderWidget *pSlider = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_RATESLIDER));
	float fOverall = ((1+pSlider->GetValue())/(float)pSlider->GetNumTicks()) * 10; //(0,10]
	const UINT wCaravelNetRequest = g_pTheNet->RateMod(name.c_str(), fOverall);
	if (!wCaravelNetRequest)
	{
		ShowOkMessage(MID_NotConnected);
		return;
	}

	g_pTheDB->Commit(); //commit outstanding changes just after request is sent

	GenericNetTransactionWait(wCaravelNetRequest, MID_RatingMod);

	CNetResult* pBuffer = g_pTheNet->GetResults(wCaravelNetRequest);
	if (!pBuffer || !pBuffer->pJson) {
		ShowOkMessage(MID_CaravelNetUnreachable);
		return;
	}
	if (pBuffer->pJson->get("status", false).asBool()) {
		ShowOkMessage(MID_RatingReceived);
	} else {
		ShowOkMessage(MID_ModNotFoundOnCaravelNet);
	}

	delete pBuffer;
}

//*****************************************************************************
void CModScreen::RequestThumbnail(CNetMedia *pMedia)
//Request thumbnail image.  When received, show it.
{
	CStretchyBuffer *pBuffer = pMedia->RequestThumbnail();

	CImageWidget *pThumbnail = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_THUMBNAIL));
	if (pBuffer)
	{
		this->pSelCNetMod = NULL; //pending request is completed

		//Show image thumbnail if valid image data.
		SDL_Surface *pImage = g_pTheBM->GetImage(*pBuffer);
		if (pImage)
		{
			pThumbnail->SetImage(pImage);
			pThumbnail->Show();
			Paint();
			return;
		}
	}

	//Image not available yet.  Start/continue requesting it.
	this->pSelCNetMod = pMedia;
}

//*****************************************************************************
void CModScreen::SelectImportedMod(const WSTRING& modName)
{
	if (CDbXML::WasImportSuccessful() && !modName.empty())
	{
		//Select imported mod.
		this->filter = F_ALL; //make sure to show imported mod in list
		PopulateModListBox();  //refresh
		this->pModListBoxWidget->SelectLineWithText(modName.c_str());
		SetDesc();
		Paint();
	}
}

//*****************************************************************************
void CModScreen::SetCNetModDesc(CNetMedia *pData)
//Set description text from specified data.
{
	ASSERT(pData);

	//Show thumbnail if available.
	RequestThumbnail(pData);

	WSTRING wstrAuthorNames = g_pTheDB->GetMessageText(MID_LevelBy);
	wstrAuthorNames += wszSpace;
	wstrAuthorNames += (const WCHAR*)pData->AuthorText;
	wstrAuthorNames += wszSpace;
	wstrAuthorNames += wszLeftParen;
	wstrAuthorNames += (const WCHAR*)pData->CNetNameText;
	wstrAuthorNames += wszRightParen;
	this->pAuthorName->SetText(wstrAuthorNames.c_str());

	WSTRING temp, rating = g_pTheDB->GetMessageText(MID_Rating);
	rating += wszSpace;
	AsciiToUnicode(pData->rating.c_str(), temp);
	rating += temp;
	rating += wszCRLF;
	rating += g_pTheDB->GetMessageText(MID_Votes);
	rating += wszSpace;
	AsciiToUnicode(pData->numVotes.c_str(), temp);
	rating += temp;
	rating += wszCRLF;
	rating += g_pTheDB->GetMessageText(MID_FileSize);
	rating += wszSpace;
	AsciiToUnicode(pData->filesize.c_str(), temp);
	rating += temp;
	rating += wszCRLF;
	rating += g_pTheDB->GetMessageText(MID_Version);
	rating += wszSpace;
	rating += g_pTheDB->GetMessageText(GetVersionMID(pData->wVersion));

	this->pDesc->SetText(rating.c_str());
}

//*****************************************************************************
void CModScreen::SetDesc()
//Updates name/description fields of selected item.
//Sets export permissions and download availability.
{
	vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();
	const WSTRING modName = this->pModListBoxWidget->GetSelectedItemText();
	const bool bIsCaravelNetMod = IsCNetMod(modName.c_str());
	const bool bIsImportedMod = IsImportedMod(modName.c_str());

	//Is mod embedded in data files?
	this->pDeleteButton->Enable(bIsImportedMod);    //if in .dats
	this->pDownloadButton->Enable(bIsCaravelNetMod && !bIsImportedMod); //if not in .dats

	//Export button is active when the style's image files are in the user's Bitmaps dir on disk.
	const WSTRING name = this->pModListBoxWidget->GetSelectedItemText();
	list<WSTRING> styleNames, skies;
	CFiles::GetGameProfileString(INISection::Graphics, name.c_str(), styleNames);
	this->pExportButton->Enable(IsStyleOnDisk(styleNames, skies));

	ShowCaravelNetWidgets(!cNetMedia.empty(), bIsImportedMod, bIsCaravelNetMod);
	this->pAuthorName->SetText(wszEmpty);
	if (!bIsImportedMod)
	{
		//This is not an imported mod.
		this->pDesc->SetText(wszEmpty);

		if (bIsCaravelNetMod)
		{
			//This mod is on CaravelNet (and hasn't been imported yet).
			bool bCanDownload = false;
			const int nIndex = g_pTheNet->getIndexForName(name.c_str(), MT_Style);
			if (nIndex >= 0)
			{
				//Show mod's CaravelNet info.
				SetCNetModDesc(cNetMedia[nIndex]);
				bCanDownload = true;
			}
			this->pDownloadButton->Enable(bCanDownload);
			if (this->pModListBoxWidget->GetItemCount() && !bCanDownload)
				this->pDesc->SetText(g_pTheDB->GetMessageText(MID_OrderAtCaravelGames));
		}
		//else the mod is stored locally as files on disk

		return;
	}

	//Display mod info.
	this->pDesc->SetText(modName.c_str());

	//Can rate if it is an installed CaravelNet mod.
	if (!bIsCaravelNetMod)
		return;

	//Show how the player has previously rated this CaravelNet mod.
	const int nIndex = g_pTheNet->getIndexForName(modName.c_str());
	if (nIndex >= 0)
	{
		CNetMedia& mediaData = *(cNetMedia[nIndex]);

		BYTE sliderIndex;
		bool bPreviouslyRated = false;
		float fRating;
		CSliderWidget *pSlider = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_RATESLIDER));
		{
			std::istringstream st(mediaData.myRating);
			st >> fRating;
			ASSERT(st);
			if (fRating >= 0.0)
			{
				sliderIndex = BYTE((fRating * pSlider->GetNumTicks() / 10.0) - 1); //(0,10] -> enum
				bPreviouslyRated = true;
			}
			else sliderIndex=0;
		}
		pSlider->SetValue(sliderIndex);
		SetRatingLabel(TAG_RATESLIDER);
		CWidget *pNotRatedLabel = GetWidget(TAG_NOTRATED);
		pNotRatedLabel->Show(!bPreviouslyRated);
	}
}

//*****************************************************************************
void CModScreen::SetRatingLabel(const UINT dwTagNo)
//Set the numeric labels depicting hold rating.
{
	char temp[16];
	CSliderWidget *pSlider = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(dwTagNo));
	const UINT wVal = UINT(((1+pSlider->GetValue())/(float)pSlider->GetNumTicks()) * 100); //(0,100]
	string str = _itoa(wVal/10, temp, 10);
	str += ".";
	str += _itoa(wVal%10, temp, 10);
	WSTRING wstr;
	AsciiToUnicode(str.c_str(), wstr);
	CLabelWidget *pLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAG_RATINGNUM_LABEL));
	pLabel->SetText(wstr.c_str());
}

//*****************************************************************************
void CModScreen::SetFilter()
{
	//Filter full list to visible display.
	this->pModListBoxWidget->Clear();
	for (UINT i=0; i<this->modInfo.size(); ++i)
	{
		const ModInfo& h = this->modInfo[i];
		bool bShow = true;

		switch (this->filter)
		{
		default:
		case F_ALL: break;
		case F_CNET: bShow = h.bCaravelNetMod; break;
		case F_NONCNET: bShow = !h.bCaravelNetMod; break;
		}
		if (bShow)
		{
			const UINT dwKey = this->pFullModList->GetKeyAtLine(i);
			this->pModListBoxWidget->AddItem(dwKey,
				this->pFullModList->GetTextAtLine(i),
				!this->pFullModList->IsItemEnabled(dwKey), -1, true);
		}
	}

	this->pModListBoxWidget->SelectLine(0);
	this->pModListBoxWidget->RequestPaint();
}

//*****************************************************************************
void CModScreen::PopulateModListBox()
//Put the mods in the DB into the list box.
{
	SetCursor(CUR_Wait);
	this->pFullModList->Clear();
	this->modInfo.clear();
	vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();

	CDb db;
	set<WSTRING> loadedStyles = db.Data.GetModNames();

	//Also list any styles in the INI file.
	{
		CFiles f;
		list<WSTRING> ini_styles;
		if (f.GetGameProfileString(INISection::Graphics, INIKey::Style, ini_styles)) {
			for (list<WSTRING>::const_iterator ini_style = ini_styles.begin(); ini_style != ini_styles.end(); ++ini_style)
				loadedStyles.insert(*ini_style);
		}
	}

	UINT unusedDataID=-int(loadedStyles.size());
	for (set<WSTRING>::const_iterator style=loadedStyles.begin(); style!=loadedStyles.end(); ++style)
	{
		ModInfo modinfo;
		modinfo.bLocalMod = true;
		modinfo.bImportedMod = IsImportedMod(style->c_str());
		modinfo.bCaravelNetMod = IsCNetMod(style->c_str());
		this->modInfo.push_back(modinfo);

		this->pFullModList->AddItem(unusedDataID++, style->c_str());
	}

	//Add list of mods on CaravelNet.
	for (UINT wIndex=0; wIndex<cNetMedia.size(); ++wIndex)
	{
		CNetMedia& mod = *(cNetMedia[wIndex]);

		//Skip non-mods.
		if (mod.mediaType != MT_Style) continue;

		//Skip mods from a future version of the game.
		if (mod.wVersion >= NEXT_VERSION_NUMBER)
			continue;

		//Show only mods on CaravelNet that aren't currently installed.
		if (!IsLocalMod((const WCHAR*)mod.HoldNameText))
		{
			//This mod hasn't been installed locally -- add to list.
			WSTRING name = (const WCHAR*)mod.HoldNameText;
			const UINT wInsertedAtIndex = this->pFullModList->AddItem(
				0, name.c_str(), true, -1, true);
			ModInfo modinfo;
			modinfo.bCaravelNetMod = true;
			this->modInfo.insert(this->modInfo.begin() + wInsertedAtIndex, modinfo);
		}
	}

	SetCursor();
	SetFilter();
}

//*****************************************************************************
void CModScreen::ShowCaravelNetWidgets(
//Shows or hides CaravelNet widgets based on status.
	const bool bLoggedIn, const bool bEmbeddedMod, const bool bCaravelNetHold)
{
	//Show these widgets when not logged in to CaravelNet.
	CWidget *pWidget = GetWidget(TAG_NOT_LOGGED_IN);
	pWidget->Show(!bLoggedIn);

	pWidget = GetWidget(TAG_NOTPUBLISHED);
	pWidget->Show(bLoggedIn && bEmbeddedMod && !bCaravelNetHold);

	//Show these widgets for a CaravelNet release hold.
	static const UINT wNumShowWidgets = 5;
	const UINT dwShowWidgets[wNumShowWidgets] = {
		TAG_RATE, TAG_RATESLIDER, TAG_RATINGNUM_LABEL,
		TAG_NOTRATED, TAG_CNET_RATING_LABEL
	};
	for (UINT i=wNumShowWidgets; i--; )
	{
		pWidget = GetWidget(dwShowWidgets[i]);
		pWidget->Show(bCaravelNetHold && bEmbeddedMod);
	}
}

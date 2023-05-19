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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "SelectMediaDialogWidget.h"
#include "DrodBitmapManager.h"
#include "DrodDialogs.h"
#include "DrodFontManager.h"
#include "DrodSound.h"
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/OptionButtonWidget.h>
#include <BackEndLib/Clipboard.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Ports.h>

#include "../DRODLib/Character.h"
#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbLevels.h"
#include "../DRODLib/DbHolds.h"
#include "../DRODLib/SettingsKeys.h"
#include "../Texts/MIDs.h"

 //NOTE: tag #'s should not conflict with other widgets on screen
const UINT TAG_ENTRANCES_LISTBOX = 990;
const UINT TAG_REPLACE = 991;
const UINT TAG_DELETE = 992;
const UINT TAG_UNDELETE = 993;
const UINT TAG_CANCEL = 994;

const UINT DIALOG_W = 700;
const UINT DIALOG_H = 610;


//*****************************************************************************
CSelectMediaDialogWidget::CSelectMediaDialogWidget(
	//Constructor.
	//
	//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget 
	const int nSetX, const int nSetY)         //    constructor.
	: CDialogWidget(dwSetTagNo, nSetX, nSetY, DIALOG_W, DIALOG_H, false)   //double-click on list box disables
	, pListBoxWidget(NULL)
	, pSourceHold(NULL)
	, pHeaderLabel(NULL)
{
	static const UINT SPACE_W = 15;
	static const UINT SPACE_H = 10;

	static const int HEADER_Y = SPACE_H;
	static const int HEADER_H = 60;

	static const int BUTTON_W = 90;
	static const int BUTTON_Y = DIALOG_H - CY_STANDARD_BUTTON - SPACE_H * 3 / 2;

	static const int LISTBOX_X = SPACE_W;
	static const int LISTBOX_Y = HEADER_Y + HEADER_H;
	static const int LISTBOX_W = DIALOG_W - LISTBOX_X - SPACE_W;
	static const int LISTBOX_H = BUTTON_Y - LISTBOX_Y - SPACE_H;

	static const int BUTTONS_TOTAL_WIDTH = 5 * BUTTON_W + 4 * SPACE_W;
	static const int BUTTON_OK_X = (DIALOG_W - 2 * SPACE_W - BUTTONS_TOTAL_WIDTH) / 2 + SPACE_W;
	static const int BUTTON_DELETE_X = BUTTON_OK_X + BUTTON_W + SPACE_W;
	static const int BUTTON_REPLACE_X = BUTTON_DELETE_X + BUTTON_W + SPACE_W;
	static const int BUTTON_UNDELETE_X = BUTTON_REPLACE_X + BUTTON_W + SPACE_W;
	static const int BUTTON_CANCEL_X = BUTTON_UNDELETE_X + BUTTON_W + SPACE_W;

	{
		this->pHeaderLabel = new CLabelWidget(0L, 0, HEADER_Y, this->w, HEADER_H,
			F_Small, wszEmpty);
		this->pHeaderLabel->SetAlign(CLabelWidget::TA_CenterGroup);
		AddWidget(this->pHeaderLabel);
	}

	CButtonWidget *pButton;

	{ // OK button
		pButton = new CButtonWidget(
			TAG_OK, BUTTON_OK_X, BUTTON_Y, BUTTON_W, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Okay));
		AddWidget(pButton);
	}

	{ // DELETE button
		pButton = new CButtonWidget(
			TAG_DELETE, BUTTON_DELETE_X, BUTTON_Y, BUTTON_W, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Delete));
		AddWidget(pButton);
	}

	{ // REPLACE button
		pButton = new CButtonWidget(
			TAG_REPLACE, BUTTON_REPLACE_X, BUTTON_Y, BUTTON_W, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_ReplaceFileButton));
		AddWidget(pButton);
	}

	{ // UNDELETE button
		pButton = new CButtonWidget(
			TAG_UNDELETE, BUTTON_UNDELETE_X, BUTTON_Y, BUTTON_W, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Undelete));
		AddWidget(pButton);
	}

	{ // CANCEL button
		pButton = new CButtonWidget(
			TAG_CANCEL, BUTTON_CANCEL_X, BUTTON_Y, BUTTON_W, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Cancel));
		AddWidget(pButton);
	}
	{
		this->pListBoxWidget = new CListBoxWidget(TAG_ENTRANCES_LISTBOX,
			LISTBOX_X, LISTBOX_Y, LISTBOX_W, LISTBOX_H);
		AddWidget(this->pListBoxWidget);
	}
}

//*****************************************************************************
void CSelectMediaDialogWidget::OnClick(
	//Handles click event.  Overrides CDialogWidget::OnClick().
	//
	//Params:
	const UINT dwTagNo)          //(in)   Widget that received event.
{
	if (!dwTagNo) return;

	CWidget *pWidget = GetWidget(dwTagNo);
	if (pWidget->GetType() == WT_Button)   //only buttons will return from dialog
	{
		switch (dwTagNo)
		{
		case TAG_OK: 
			if (GetSelectedItem()) {
				this->dwDeactivateValue = TAG_OK;
				Deactivate();
			}
			else {
				const UINT wResult = ImportFile(this->eDatatype);
				if (wResult) {
					RefreshList();
					this->pListBoxWidget->SelectItem(wResult);
					RefreshButtons();
				}
			}
			break;
		case TAG_DELETE: 
			if (GetSelectedItem()) {
				DeleteMedia(GetSelectedItem());
				RefreshButtons();
			}
			break;
		case TAG_REPLACE: 
			if (GetSelectedItem()) {
				const UINT wResult = ImportFile(this->eDatatype, GetSelectedItem());
				if (wResult) {
					RefreshList();
					this->pListBoxWidget->SelectItem(wResult);
					RefreshButtons();
				}
			}
			break;
		case TAG_UNDELETE: 
			if (GetSelectedItem()) {
				UndeleteMedia(GetSelectedItem());
				RefreshButtons();
			}
			break;
		case TAG_CANCEL: 
			this->dwDeactivateValue = TAG_ESCAPE; 
			Deactivate();
			break;
		}
	}

	RequestPaint();
}

//*****************************************************************************
void CSelectMediaDialogWidget::OnDoubleClick(
//Handles double click event.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	CDialogWidget::OnDoubleClick(dwTagNo);

	if (GetSelectedItem()) {
		this->dwDeactivateValue = TAG_OK;
		Deactivate();
	} else {
		const UINT wResult = ImportFile(this->eDatatype);
		if (wResult) {
			RefreshList();
			this->pListBoxWidget->SelectItem(wResult);
			RefreshButtons();
		}

		RequestPaint();
	}
}

//*****************************************************************************
void CSelectMediaDialogWidget::OnSelectChange(
	//Handles selection change event.
	//Pass event on to parent, if exists.
	//
	//Params:
	const UINT dwTagNo)
{
	RefreshButtons();

	CDialogWidget::OnSelectChange(dwTagNo);
}

//*****************************************************************************
void CSelectMediaDialogWidget::SetForDisplay(
	const MESSAGE_ID headerMID,
	CDbHold *pHold,
	const DATATYPE eDatatype) //[default=Entrances]
//Populate list box with all of a certain data type in hold.
{
	ASSERT(pHold);

	this->pHeaderLabel->SetText(g_pTheDB->GetMessageText(headerMID));
	this->pSourceHold = pHold;
	this->eDatatype = eDatatype;

	RefreshList();

	RefreshButtons();
}

//*****************************************************************************
void CSelectMediaDialogWidget::RefreshList()
{
	this->pListBoxWidget->Clear();

	this->pListBoxWidget->SortAlphabetically(false);

	BEGIN_DBREFCOUNT_CHECK;
	switch (this->eDatatype)
	{
	case Images:
		this->pListBoxWidget->AddItem(0L, g_pTheDB->GetMessageText(MID_ImportImage));
		break;

	case Sounds:
		this->pListBoxWidget->AddItem(0L, g_pTheDB->GetMessageText(MID_ImportSoundOption));
		break;

	case Videos:
		this->pListBoxWidget->AddItem(0L, g_pTheDB->GetMessageText(MID_ImportVideoOption));
		break;

	default: break;
	}

	this->pListBoxWidget->SortAlphabetically(true);

	{ // Ensure CDb is deleted before refcount check ends
		CDb db;
		db.Data.FilterByFormat(GetSupportedFormats());
		db.Data.FilterByHold(this->pSourceHold->dwHoldID);

		CIDSet dataIDs = db.Data.GetIDs();
		CIDSet deletedDataIDs = this->pSourceHold->GetDeletedDataIDs();
		for (CIDSet::const_iterator iter = dataIDs.begin(); iter != dataIDs.end(); ++iter)
		{
			this->pListBoxWidget->AddItem(*iter, GetMediaName(*iter, &deletedDataIDs).c_str());
		}
	}
	END_DBREFCOUNT_CHECK;
}

//*****************************************************************************
const WSTRING CSelectMediaDialogWidget::GetMediaName(const UINT wMediaId) const
{
	CIDSet deletedDataIDs = this->pSourceHold->GetDeletedDataIDs();
	return GetMediaName(wMediaId, &deletedDataIDs);
}

//*****************************************************************************
const WSTRING CSelectMediaDialogWidget::GetMediaName(const UINT wMediaId, CIDSet *deletedDataIDs) const
{

	WSTRING name = g_pTheDB->Data.GetNameFor(wMediaId);
	if (deletedDataIDs->has(wMediaId)) {
		name += L" ";
		name += g_pTheDB->GetMessageText(MID_FilePendingDeletionSuffix);
	}

	return name.c_str();
}

//*****************************************************************************
void CSelectMediaDialogWidget::RefreshMediaName(const UINT wMediaId)
{
	this->pListBoxWidget->SetItemText(wMediaId, GetMediaName(wMediaId).c_str());
}

//*****************************************************************************
void CSelectMediaDialogWidget::DeleteMedia(const UINT wMediaId)
{
	this->pSourceHold->MarkDataForDeletion(wMediaId);
	RefreshMediaName(wMediaId);
	RefreshButtons();
}

//*****************************************************************************
void CSelectMediaDialogWidget::UndeleteMedia(const UINT wMediaId)
{
	this->pSourceHold->UnmarkDataForDeletion(wMediaId);
	RefreshMediaName(wMediaId);
	RefreshButtons();
}

//*****************************************************************************
UINT CSelectMediaDialogWidget::ImportFile(
	DATATYPE eType,        //(in) Type of the file to import
	UINT wReplacedMediaId) //(in) Optional ID of the existing media file to replace 
{
	const char *pathSetting = NULL;
	CIDSet deletedDataIDs = this->pSourceHold->GetDeletedDataIDs();
	const CIDSet supportedFormats = GetSupportedFormats();
	UINT fileDialogHeaderMID;
	UINT allowedExtensionsFlags;

	switch (eType) {
		case DATATYPE::Images:
			pathSetting = &(Settings::ImportImagePath)[0];
			fileDialogHeaderMID = MID_ImageSelectPrompt;
			allowedExtensionsFlags = EXT_JPEG | EXT_PNG;
			break;
		case DATATYPE::Sounds:
			pathSetting = &(Settings::ImportSoundPath)[0];
			fileDialogHeaderMID = MID_SoundSelectPrompt;
			allowedExtensionsFlags = EXT_WAV | EXT_OGG;
			break;
		case DATATYPE::Videos:
			pathSetting = &(Settings::ImportVideoPath)[0];
			fileDialogHeaderMID = MID_VideoSelectPrompt;
			allowedExtensionsFlags = EXT_THEORA;
			break;
		default:
			return 0;
	}

	CFiles Files;
	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	WSTRING wstrImportPath = pCurrentPlayer ?
		pCurrentPlayer->Settings.GetVar(pathSetting, Files.GetDatPath().c_str()) :
		Files.GetDatPath();

	CDb db;
	db.Data.FilterByHold(this->pSourceHold->dwHoldID);

	UINT wResult = 0;
	while (wResult == 0) {
		// 1. Ask for the file
		WSTRING wstrSelectedFile;
		const UINT dwSelectFileResult = g_pTheDialogs->SelectFile(
			wstrImportPath,
			wstrSelectedFile,
			fileDialogHeaderMID,
			false,
			allowedExtensionsFlags
		);
		if (dwSelectFileResult != TAG_OK)
			break;

		//2. Update the path in player settings, so next time dialog comes up it will have the same path.
		if (pCurrentPlayer)
		{
			pCurrentPlayer->Settings.SetVar(pathSetting, wstrImportPath.c_str());
			pCurrentPlayer->Update();
		}

		// 3. If file with this name already exists...
		const WSTRING filename = getFilenameFromPath(wstrSelectedFile.c_str());
		const UINT wExistingImageId = db.Data.FindByName(filename.c_str());

		// 3a. If we are already replacing a file, it's impossible
		if (wExistingImageId && wReplacedMediaId && wExistingImageId != wReplacedMediaId) {
			const CDbDatum *pReplacedFile = db.Data.GetByID(wReplacedMediaId);
			WSTRING message = g_pTheDB->GetMessageText(MID_ErrorCannotReplaceWithDifferentExistingFile);
			message = WCSReplace(message, L"%fileBase%", pReplacedFile->DataNameText);
			message = WCSReplace(message, L"%fileSelected%", filename);
			delete pReplacedFile;
			g_pTheDialogs->ShowOkMessage(message.c_str());
			continue;
		}

		// 3b. If we are importing a new file, ask whether user wants to replace it
		if (wExistingImageId && !wReplacedMediaId) {
			WSTRING message = g_pTheDB->GetMessageText(MID_ReplaceMediaWithAnother);
			message = WCSReplace(message, L"%file%", filename);

			if (g_pTheDialogs->ShowYesNoMessage(message.c_str(), MID_Rename, MID_Cancel) != TAG_YES)
				continue;
		}

		// 4. Read the file
		CStretchyBuffer buffer;
		if (!Files.ReadFileIntoBuffer(wstrSelectedFile.c_str(), buffer, true)) {
			g_pTheDialogs->ShowOkMessage(MID_FileNotFound);
			continue;
		}

		// 5. Verify the file
		UINT wDataFormat;
		switch (eType) {
			case DATATYPE::Images:
				wDataFormat = g_pTheBM->GetImageType(buffer);
				break;
			case DATATYPE::Sounds:
				if (!g_pTheSound->VerifySound(buffer)) {
					g_pTheDialogs->ShowOkMessage(MID_FileNotValid);
					continue;
				}
				wDataFormat = DATA_OGG;
				break;
			case DATATYPE::Videos:
				g_pTheDialogs->ShowStatusMessage(MID_VerifyingMedia);
				if (!ValidateVideo(buffer)) {
					g_pTheDialogs->ShowOkMessage(MID_FileNotValid);
					continue;
				}
				g_pTheDialogs->HideStatusMessage();
				wDataFormat = DATA_THEORA;
				break;
		}

		// 6. Import the file
		CDbDatum *pDatum;
		if (wReplacedMediaId)
			pDatum = db.Data.GetByID(wReplacedMediaId);
		else if (wExistingImageId)
			pDatum = db.Data.GetByID(wExistingImageId);
		else
			pDatum = db.Data.GetNew();

		pDatum->wDataFormat = wDataFormat;
		pDatum->data.Set((const BYTE *)buffer, buffer.Size());
		pDatum->DataNameText = getFilenameFromPath(wstrSelectedFile.c_str());
		pDatum->dwHoldID = this->pSourceHold->dwHoldID;
		pDatum->Update();
		wResult = pDatum->dwDataID;
		delete pDatum;
	}

	delete pCurrentPlayer;
	return wResult;
}

//*****************************************************************************
CIDSet CSelectMediaDialogWidget::GetSupportedFormats() const
{
	CIDSet supportedFormats;
	switch (this->eDatatype)
	{
		case Images:
			supportedFormats += CBitmapManager::GetSupportedImageFormats();
			break;

		case Sounds:
			supportedFormats += DATA_OGG; //all supported sound formats
			supportedFormats += DATA_WAV;
			break;

		case Videos:
			supportedFormats += DATA_THEORA;
			break;

		default: 
			ASSERT(!"Tried to get supported types for unknown datatype.");
			break;
	}
	return supportedFormats;
}
//*****************************************************************************
UINT CSelectMediaDialogWidget::GetSelectedItem() const
{
	return this->pListBoxWidget->GetSelectedItem();
}

//*****************************************************************************
void CSelectMediaDialogWidget::SelectItem(
	const UINT dwTagNo) //(in)
{
	this->pListBoxWidget->SelectItem(dwTagNo);

	RefreshButtons();
}

//*****************************************************************************
void CSelectMediaDialogWidget::RefreshButtons()
{
	CIDSet deletedDataIDs = this->pSourceHold->GetDeletedDataIDs();

	const UINT wSelectedID = GetSelectedItem();
	const bool wIsDeleted = wSelectedID > 0 ? deletedDataIDs.has(wSelectedID) : false;

	//Disable delete & replaces button when selecting a virtual value
	CButtonWidget *pOkButton = DYN_CAST(CButtonWidget *, CWidget *, GetWidget(TAG_OK));
	CButtonWidget *pDeleteButton = DYN_CAST(CButtonWidget *, CWidget *, GetWidget(TAG_DELETE));
	CButtonWidget *pReplaceButton = DYN_CAST(CButtonWidget *, CWidget *, GetWidget(TAG_REPLACE));
	CButtonWidget *pUndeleteButton = DYN_CAST(CButtonWidget *, CWidget *, GetWidget(TAG_UNDELETE));

	pOkButton->Enable(!wIsDeleted);
	pDeleteButton->Enable(GetSelectedItem() != 0L && !wIsDeleted);
	pReplaceButton->Enable(GetSelectedItem() != 0L && !wIsDeleted);
	pUndeleteButton->Enable(GetSelectedItem() != 0L && wIsDeleted);

	pOkButton->RequestPaint();
	pDeleteButton->RequestPaint();
	pReplaceButton->RequestPaint();
	pUndeleteButton->RequestPaint();
}

//*****************************************************************************
bool CSelectMediaDialogWidget::ValidateVideo(CStretchyBuffer &buffer)
{
	CDrodScreen *pDrodScreen = GetParentDrodScreen();

	if (pDrodScreen) {
		return pDrodScreen->ValidateVideo(buffer);
	}

	return false;
}

//*****************************************************************************
CDrodScreen *CSelectMediaDialogWidget::GetParentDrodScreen() const
{
	CWidget *pParent = this->pParent;

	while (pParent) {
		CDrodScreen *pParentAsScreen = dynamic_cast<CDrodScreen *>(pParent);

		if (pParentAsScreen) {
			return pParentAsScreen;
		}
		pParent = pParent->GetParent();
	}

	ASSERT(!"CSelectMediaDialogWidget must have CDrodScreen somewhere in its parents");
	return NULL;
}

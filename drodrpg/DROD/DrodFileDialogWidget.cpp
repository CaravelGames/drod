// $Id: DrodFileDialogWidget.cpp 8102 2007-08-15 14:55:40Z trick $

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
 * Portions created by the Initial Developer are Copyright (C)
 * 2002, 2005, 2007 Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "DrodFileDialogWidget.h"
#include "DrodFontManager.h"
#include "../DRODLib/Db.h"
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include "../Texts/MIDs.h"

const UINT TAG_CANCEL = 1090;
const UINT TAG_SHOW_HIDDEN = 1085; //same value as in FrontEndLib/FileDialogWidget.cpp

//DROD-specific file extension filters.
const MESSAGE_ID fileExtension[FileExtension::EXT_COUNT] =
{
	MID_FilePlayerExt,
	MID_FileHoldExt,
	MID_FileDemoExt,
	MID_Ogg,
	MID_HTML,
	MID_Jpeg,
	MID_PNG,
	MID_Wav,
	MID_Data,
	MID_XML,
	MID_Ogg,
	MID_SaveExt
};

const MESSAGE_ID fileExtensionDesc[FileExtension::EXT_COUNT] =
{
	MID_FilePlayerExtDesc,
	MID_FileHoldExtDesc,
	MID_FileDemoExtDesc,
	MID_OggDesc,
	MID_HTMLDesc,
	MID_JpegDesc,
	MID_PNGDesc,
	MID_WavDesc,
	MID_DataDesc,
	MID_XMLDesc,
	MID_TheoraDesc,
	MID_SaveExtDesc
};

//*****************************************************************************
CDrodFileDialogWidget::CDrodFileDialogWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget
	const int nSetX, const int nSetY)         //    constructor.
	: CFileDialogWidget(dwSetTagNo, nSetX, nSetY)
{
	static const UINT CX_SPACE = 15;
	static const UINT CY_SPACE = 8;

	static const UINT CX_BUTTON = 90;
	static const int X_BUTTON = CX_FILEDIALOG - CX_BUTTON - CX_SPACE;
	static const int Y_CANCEL_BUTTON = CY_FILEDIALOG - CY_STANDARD_BUTTON - CY_SPACE * 2;
	static const int Y_OK_BUTTON = Y_CANCEL_BUTTON - CY_STANDARD_BUTTON - CY_SPACE;

	static const int X_DIR_TEXT_BOX = 80; //ref from CFileDialogWidget
	static const UINT CX_CURDIR = X_DIR_TEXT_BOX - CX_SPACE;
	static const UINT CX_FILENAME = 85;
	static const UINT CX_FILETYPE = 120;

	//Add labels (requiring DB text to access).
	AddWidget(new CLabelWidget(0L, CX_SPACE, 47, CX_CURDIR, 60, F_Small,
			g_pTheDB->GetMessageText(MID_CurrentDirectory)));
	AddWidget(new CLabelWidget(0L, CX_SPACE, 570, CX_FILENAME, 30, FONTLIB::F_Small,
			g_pTheDB->GetMessageText(MID_FileName)));
	AddWidget(new CLabelWidget(0L, CX_SPACE, 610, CX_FILETYPE, 30, FONTLIB::F_Small,
			g_pTheDB->GetMessageText(MID_FileType)));

	//Buttons.
	CButtonWidget *pOKButton = new CButtonWidget(
			TAG_OK, X_BUTTON, Y_OK_BUTTON, CX_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Okay));
	AddWidget(pOKButton);
	CButtonWidget *pCancelButton = new CButtonWidget(
			TAG_CANCEL, X_BUTTON, Y_CANCEL_BUTTON, CX_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Cancel));
	AddWidget(pCancelButton);

	COptionButtonWidget *pShowHidden = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_SHOW_HIDDEN));
	pShowHidden->SetCaption(g_pTheDB->GetMessageText(MID_ShowHiddenFiles));
}

//*****************************************************************************
UINT CDrodFileDialogWidget::GetExtensionType(const WSTRING& wFilename)
//Determine file type based on file name extension.
{
	UINT ext;
	for (ext = 0; ext < FileExtension::EXT_COUNT; ++ext)
	{
		WSTRING wstrExt = wszPeriod;
		wstrExt += g_pTheDB->GetMessageText(fileExtension[ext]);
		if (!WCSicmp(wFilename.c_str() + WCSlen(wFilename.c_str()) - wstrExt.size(), wstrExt.c_str()))
		{
			//Recognized file extension.
			return ext;
		}
	}

	//On older OSes, only 3-character extensions are recognized.
	//If none of the above matches worked, try matching against shortened extensions.
	for (ext = 0; ext < FileExtension::EXT_COUNT; ++ext)
	{
		WSTRING wstrExt = wszPeriod;
		wstrExt += g_pTheDB->GetMessageText(fileExtension[ext]);
		if (wstrExt.length() > 4) //don't retest extensions that were already short enough
		{
			wstrExt.resize(4); //.ext
			if (!WCSicmp(wFilename.c_str() + WCSlen(wFilename.c_str()) - wstrExt.size(), wstrExt.c_str()))
			{
				//Recognized file extension.
				return ext;
			}
		}
	}

	return FileExtension::EXT_COUNT; //extension not recognized
}

//*****************************************************************************
void CDrodFileDialogWidget::SetExtensions(
//Sets the file extension filter.
//
//Params:
	const UINT extensionTypes)  //(in)
{
	ClearExtensionList();

	//Set extensions and extension filter appearing in the extension list box.
	for (UINT ext = 0, bit = 1; ext < FileExtension::EXT_COUNT; ++ext, bit <<= 1)
		if (extensionTypes & bit)
		{
			AddExtensionDesc(g_pTheDB->GetMessageText(fileExtensionDesc[ext]));
			this->extensions.push_back(g_pTheDB->GetMessageText(fileExtension[ext]));
		}
}

//*****************************************************************************
void CDrodFileDialogWidget::SetPrompt(
//Sets the file selection prompt.
//
//Params:
	const MESSAGE_ID messageID)   //(in)
{
	const WCHAR *pwczText = g_pTheDB->GetMessageText(messageID);
	CFileDialogWidget::SetPrompt(pwczText);
}

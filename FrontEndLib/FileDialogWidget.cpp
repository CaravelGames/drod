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
#ifdef WIN32
#  include <windows.h> //Should be first include.
#endif

#include "FileDialogWidget.h"
#include "BitmapManager.h"
#include "ButtonWidget.h"
#include "FrameWidget.h"
#include "LabelWidget.h"
#include "ListBoxWidget.h"
#include "OptionButtonWidget.h"
#include "TextBoxWidget.h"

#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>

#include <set>
#include <fstream>

using namespace std;

#ifdef WIN32
#include <io.h>
#include <direct.h>  //for _chdrive
#else
#include <stdio.h>
#include <dirent.h>
#endif
#include <sys/stat.h>

#if defined __linux__ || defined __FreeBSD__
#include <unistd.h> //for chdir
#endif

const UINT TAG_DIR_LBOX = 1080;
const UINT TAG_FILE_LBOX = 1081;
const UINT TAG_EXTENSION_LBOX = 1082;
const UINT TAG_DIRNAME_BOX = 1083;
const UINT TAG_FILENAME_BOX = 1084;
const UINT TAG_SHOW_HIDDEN = 1085;

#ifdef WIN32
	const UINT numDrives = 26; //A - Z
	bool bDrives[numDrives] = {false};
	bool bDrivesChecked = false;
#endif

//*****************************************************************************
CFileDialogWidget::CFileDialogWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget
	const int nSetX, const int nSetY)         //    constructor.
	: CDialogWidget(dwSetTagNo, nSetX, nSetY, CX_FILEDIALOG, CY_FILEDIALOG)
	, pDirNameTextBox(NULL)
	, pHeaderLabel(NULL)
	, pDirListBoxWidget(NULL)
	, pFileListBoxWidget(NULL)
	, pExtensionListBoxWidget(NULL)
	, pFilenameTextBox(NULL)
	, bWriting(false)
{
	CheckDrives();

	static const UINT CX_SPACE = 15;
	static const UINT CY_SPACE = 12;

	static const int X_FRAME = CX_SPACE;
	static const int Y_FRAME = CY_SPACE;
	static const UINT CX_FRAME = CX_FILEDIALOG - CX_SPACE*2;
	static const UINT CY_PROMPT_LABEL = 22;
	static const UINT CY_FRAME = CY_PROMPT_LABEL + CY_SPACE;

	static const int Y_PROMPT_LABEL = 0;

	static const int X_DIR_TEXT_BOX = 80;
	static const int Y_DIR_TEXT_BOX = Y_FRAME + CY_FRAME + CY_SPACE;
	static const UINT CX_DIR_TEXT_BOX = CX_FILEDIALOG - X_DIR_TEXT_BOX - CX_SPACE;
	static const UINT CY_DIR_TEXT_BOX = 32;

	static const int Y_DIR_LIST_BOX = Y_DIR_TEXT_BOX + CY_DIR_TEXT_BOX + CY_SPACE;
	static const UINT CY_DIR_LIST_BOX = 180;
	static const UINT CX_FILEBOX = CX_FRAME;

	static const int Y_FILE_LIST_BOX = Y_DIR_LIST_BOX + CY_DIR_LIST_BOX + CY_SPACE;
	static const UINT CY_FILE_LIST_BOX = 268;

	static const int X_FILE_TEXT_BOX = 100;
	static const int Y_FILE_TEXT_BOX = Y_FILE_LIST_BOX + CY_FILE_LIST_BOX + CY_SPACE;
	static const UINT CX_FILE_TEXT_BOX = CX_FILEDIALOG - X_FILE_TEXT_BOX - CX_SPACE;
	static const UINT CY_FILE_TEXT_BOX = CY_DIR_TEXT_BOX;

	static const UINT CX_FILE_EXT_BOX = 320;
	static const UINT CX_SHOW_HIDDEN = CX_FILE_EXT_BOX;
	static const UINT CY_SHOW_HIDDEN = CY_STANDARD_OPTIONBUTTON;
	static const int X_SHOW_HIDDEN = CX_SPACE;
	static const int Y_SHOW_HIDDEN = CY_FILEDIALOG - CY_SHOW_HIDDEN - CY_SPACE;
	static const UINT CY_FILE_EXT_BOX = 53;
	static const int Y_FILE_EXT_BOX = CY_FILEDIALOG - CY_FILE_EXT_BOX - CY_SHOW_HIDDEN - CY_SPACE;

	//Prompt.
	CFrameWidget *pFrame = new CFrameWidget(0L, X_FRAME, Y_FRAME, CX_FRAME, CY_FRAME, NULL);
	this->pHeaderLabel = new CLabelWidget(0L, 0, Y_PROMPT_LABEL,
			CX_FRAME, CY_PROMPT_LABEL, FONTLIB::F_Small, wszEmpty);
	this->pHeaderLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	pFrame->AddWidget(this->pHeaderLabel);
	AddWidget(pFrame);

	//Current directory.
	this->pDirNameTextBox = new CTextBoxWidget(TAG_DIRNAME_BOX, X_DIR_TEXT_BOX,
			Y_DIR_TEXT_BOX, CX_DIR_TEXT_BOX, CY_DIR_TEXT_BOX, 255, false);
	AddWidget(this->pDirNameTextBox);

	this->pDirListBoxWidget = new CListBoxWidget(TAG_DIR_LBOX,
			CX_SPACE, Y_DIR_LIST_BOX, CX_FILEBOX, CY_DIR_LIST_BOX);
	this->pDirListBoxWidget->SetHotkeyItemSelection(true);
	AddWidget(this->pDirListBoxWidget);

	//Current file.
	this->pFileListBoxWidget = new CListBoxWidget(TAG_FILE_LBOX,
			CX_SPACE, Y_FILE_LIST_BOX, CX_FILEBOX, CY_FILE_LIST_BOX);
	this->pFileListBoxWidget->SetHotkeyItemSelection(true);
	AddWidget(this->pFileListBoxWidget);

	this->pFilenameTextBox = new CTextBoxWidget(TAG_FILENAME_BOX,
			X_FILE_TEXT_BOX, Y_FILE_TEXT_BOX, CX_FILE_TEXT_BOX, CY_FILE_TEXT_BOX,
			255, true);
	this->pFilenameTextBox->SetFilenameSafe();
	AddWidget(this->pFilenameTextBox);
	SetEnterText(TAG_FILENAME_BOX);

	//Extension filter.
	this->pExtensionListBoxWidget = new CListBoxWidget(TAG_EXTENSION_LBOX,
			CX_SPACE, Y_FILE_EXT_BOX, CX_FILE_EXT_BOX, CY_FILE_EXT_BOX);
	AddWidget(this->pExtensionListBoxWidget);

	//Hidden files toggle.
	this->pShowHiddenOptionWidget = new COptionButtonWidget(TAG_SHOW_HIDDEN,
			X_SHOW_HIDDEN, Y_SHOW_HIDDEN, CX_SHOW_HIDDEN, CY_SHOW_HIDDEN,
			wszEmpty, false);
	AddWidget(this->pShowHiddenOptionWidget);
}

//*****************************************************************************
void CFileDialogWidget::CheckDrives()
//Checks which drives are available.
{
#ifdef WIN32
	if (!bDrivesChecked)
	{
		for (char drive=0; drive<numDrives; ++drive)
		{
			char buffer[4] = { drive+'a', ':', '\\', 0};
			UINT uType = GetDriveTypeA(buffer); //Use instead of GetDriveType() for pre-NT compatibility.
			bDrives[drive] = ((uType != DRIVE_UNKNOWN) && (uType != DRIVE_NO_ROOT_DIR));
		}
		bDrivesChecked = true;
	}
#endif
}

//*****************************************************************************
void CFileDialogWidget::OnClick(
//Handles click event.
//
//Params:
	const UINT dwTagNo)          //(in)   Widget that received event.
{
	switch (dwTagNo)
	{
		case TAG_OK:
			if (pFileListBoxWidget->GetItemCount() == 0 && !this->bWriting &&
					!WCSlen(this->pFilenameTextBox->GetText()))
			{
				//No files to select for reading.  Select highlighted directory instead.
				GoToDirectory();
				return;
			}
		break;

		case TAG_SHOW_HIDDEN:
			PopulateDirList();
			PopulateFileList();
			RequestPaint();
			break;
	}

	CDialogWidget::OnClick(dwTagNo);
}

//*****************************************************************************
void CFileDialogWidget::OnDoubleClick(
//Called when widget receives a mouse click.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	CDialogWidget::OnDoubleClick(dwTagNo);

	switch (dwTagNo)
	{
		case TAG_DIR_LBOX:
			if (this->pDirListBoxWidget->ClickedSelection())
				GoToDirectory();
		break;

		case TAG_FILE_LBOX:
			if (this->pFileListBoxWidget->ClickedSelection())
			{
				//Selects the file.
				this->dwDeactivateValue = TAG_OK;
				Deactivate();
			}
		break;

      default: break;
	}
}

//*****************************************************************************
void CFileDialogWidget::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget event applies to.
	const SDL_KeyboardEvent &Key) //(in)   Event.
{
	CDialogWidget::OnKeyDown(dwTagNo, Key);

	switch (dwTagNo)
	{
		case TAG_DIRNAME_BOX:
			switch (Key.keysym.sym)
			{
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					SetDirectory(this->pDirNameTextBox->GetText());
				break;
				default: break;
			}
		break;
		case TAG_DIR_LBOX:
		{
			switch (Key.keysym.sym)
			{
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					GoToDirectory();
				break;
				default: break;
			}
		}
		break;

		case TAG_FILENAME_BOX:
			switch (Key.keysym.sym)
			{
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					OnClick(TAG_OK);
				break;
				default: break;
			}
		break;

		case TAG_EXTENSION_LBOX:
			if (this->pExtensionListBoxWidget->GetItemCount() > 1)
			{
				//There might have been a change in extension.  Update file list.
				PopulateFileList();
				if (!this->bWriting) SelectFile();
				RequestPaint();
			}
		break;
		default: break;
	}
}

//*****************************************************************************
void CFileDialogWidget::OnSelectChange(
//Handles a selection change.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	switch (dwTagNo)
	{
		case TAG_FILE_LBOX:
			SelectFile();
			RequestPaint();
		break;

		case TAG_EXTENSION_LBOX:
			PopulateFileList();
			if (!this->bWriting) SelectFile();
			RequestPaint();
		break;
	}
}

//*****************************************************************************
void CFileDialogWidget::PopulateDirList()
{
	this->pDirListBoxWidget->Clear();

	UINT wCount = 0;
	vector<WSTRING> wstrDirs;
	CFiles::GetDirectoryList(this->dirpath.c_str(), wstrDirs, pShowHiddenOptionWidget->IsChecked());

	vector<WSTRING>::iterator wstr;
	for (wstr = wstrDirs.begin(); wstr != wstrDirs.end(); ++wstr) {
      this->pDirListBoxWidget->AddItem(++wCount, wstr->c_str());
	}

	if (CFiles::GetDriveList(wstrDirs))
	{
		for (wstr = wstrDirs.begin(); wstr != wstrDirs.end(); ++wstr) {
			this->pDirListBoxWidget->AddItem(++wCount, wstr->c_str());
		}
	}
}

//*****************************************************************************
void CFileDialogWidget::PopulateFileList()
//Populate list box with all files in current directory.
{
	this->pFileListBoxWidget->Clear();

	UINT wCount = 0;

	vector<WSTRING> wstrFiles;

	// If no extensions are selected, GetSelectedItem returns 0, ie. first one
	CFiles::GetFileList(this->dirpath.c_str(),
			this->extensions[this->pExtensionListBoxWidget->GetSelectedItem()],
			wstrFiles, false, pShowHiddenOptionWidget->IsChecked());

	vector<WSTRING>::iterator wstr;

	// No Drives to list here, so we just add all files at the end.
	for (wstr = wstrFiles.begin(); wstr != wstrFiles.end(); ++wstr) {
      this->pFileListBoxWidget->AddItem(++wCount, wstr->c_str());
	}
}

//*****************************************************************************
void CFileDialogWidget::ClearExtensionList()
//Clear extensions list (!)
{
	this->extensions.clear();
	this->pExtensionListBoxWidget->Clear();
}

//*****************************************************************************
void CFileDialogWidget::AddExtensionDesc(
//Add a new file extension description.  The new extension must be pushed to
//extensions *after* a call to this.
//
//Params:
	const WCHAR *wszDesc)  //(in) A description of this extension.
{
	if (!this->pExtensionListBoxWidget->AddItem(this->extensions.size(), wszDesc))
		this->pExtensionListBoxWidget->SelectLine(0);	//select first by default
}

//*****************************************************************************
void CFileDialogWidget::GoToDirectory()
//Go to selected directory.
{
	const WSTRING wstrDirname = this->pDirListBoxWidget->GetSelectedItemText();
	if (wstrDirname.empty()) return; //nothing is selected
	if (!WCScmp(wszParentDir,wstrDirname.c_str()))
	{
		//Go up a directory.
		const int nSlashLoc=this->dirpath.rfind(wszSlash);
		if (nSlashLoc<0)
			this->dirpath.resize(0);
#ifndef WIN32
		else if (nSlashLoc == 0)
			this->dirpath.resize(1); // go to root dir
#endif
		else
			this->dirpath.resize(nSlashLoc);
#if defined(WIN32) || defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
	} else if (wstrDirname[0] == W_t('[')) {
		//Switch drives.
		WSTRING newDrive = wstrDirname.c_str() + 2;
		newDrive.resize(newDrive.size()-2);
		SetDirectory(newDrive.c_str());
		return;
#endif
	} else {
		//Go down a directory.
		if (this->dirpath.c_str() &&
				(this->dirpath.c_str()[this->dirpath.length()-1] != wszSlash[0]))
			this->dirpath += wszSlash;
		this->dirpath += wstrDirname;
	}

	SetDirectory();
}

//*****************************************************************************
void CFileDialogWidget::SetDirectory(
//Go to selected directory.
//
//Params:
	const WCHAR *wDirPath)  //(in) default = NULL (means to use this->dirpath)
{
	//Change to new directory?
	if (wDirPath && CFiles::IsValidPath(wDirPath)) //Yes.
		this->dirpath = wDirPath;
	else if (!CFiles::IsValidPath(this->dirpath.c_str()))
	{
		// Default path is invalid! Set to dat path.
		CFiles files;
		this->dirpath = files.GetDatPath();
	}

	//Update affected widgets.
	this->pDirNameTextBox->SetText(this->dirpath.c_str());
	PopulateDirList();
	PopulateFileList();
	if (!this->bWriting)
	{
		if (!this->pFileListBoxWidget->ItemIsSelected())
			this->pFileListBoxWidget->SelectLine(0);
		SelectFile();
	}

	//Repaint the dialog to show changes.
	RequestPaint();
}

//*****************************************************************************
WSTRING CFileDialogWidget::GetSelectedFileName() const
//Returns: the full file name
{
	WSTRING filename = this->dirpath;
	filename += wszSlash;
	filename += this->pFilenameTextBox->GetText();

	//Add extension if it's missing.
	WSTRING ext;
	UINT i;
	for (i=0; i<pExtensionListBoxWidget->GetItemCount(); ++i)
	{
		ext = wszPeriod;
		ext += this->extensions[pExtensionListBoxWidget->GetKeyAtLine(i)];
		if (!WCSicmp(filename.c_str() + WCSlen(filename.c_str()) - ext.size(), ext.c_str()))
			break;
	}
	if (i==pExtensionListBoxWidget->GetItemCount())
	{
		//Extension is missing.  Add the selected one.
		ext = wszPeriod;
		ext += this->extensions[pExtensionListBoxWidget->GetSelectedItem()];
		filename += ext.c_str();
	}

	return filename;
}

//*****************************************************************************
vector<WSTRING> CFileDialogWidget::GetSelectedFileNames() const
//Returns: a vector of full file names
{
	vector<WSTRING> wFilenames = this->pFileListBoxWidget->GetSelectedItemTexts();
	if (wFilenames.empty())
	{
		//No files selected, so use the written text only.
		wFilenames.push_back(GetSelectedFileName());
		return wFilenames;
	}

	for (vector<WSTRING>::iterator wFilename = wFilenames.begin();
			wFilename != wFilenames.end(); ++wFilename)
	{
		WSTRING filename = this->dirpath;
		filename += wszSlash;
		filename += *wFilename;

		//Add extension if it's missing.
		WSTRING ext;
		UINT i;
		for (i=0; i<pExtensionListBoxWidget->GetItemCount(); ++i)
		{
			ext = wszPeriod;
			ext += this->extensions[pExtensionListBoxWidget->GetKeyAtLine(i)];
			if (!WCSicmp(filename.c_str() + WCSlen(filename.c_str()) - ext.size(), ext.c_str()))
				break;
		}
		if (i==pExtensionListBoxWidget->GetItemCount())
		{
			//Extension is missing.  Add the selected one.
			ext = wszPeriod;
			ext += this->extensions[pExtensionListBoxWidget->GetSelectedItem()];
			filename += ext.c_str();
		}

		(*wFilename) = filename;
	}

	return wFilenames;
}

//*****************************************************************************
void CFileDialogWidget::SelectFile()
//Selects the file selected in the list box.
{
	const WSTRING wFilename = this->pFileListBoxWidget->GetSelectedItemText();
	if (!wFilename.empty())
		this->pFilenameTextBox->SetText(wFilename.c_str());
	CheckTextBox();
}

//*****************************************************************************
void CFileDialogWidget::SelectMultipleFiles(const bool bVal)
//Set whether multiple filenames can be selected.
{
	this->pFileListBoxWidget->SelectMultipleItems(bVal);
}

//*****************************************************************************
void CFileDialogWidget::SetFilename(
//Sets the string appearing in the filename text box.
//
//Params:
	const WCHAR *wFilename)
{
	this->pFilenameTextBox->SetText(wFilename);
}

//*****************************************************************************
void CFileDialogWidget::SetPrompt(
//Sets the file selection prompt.
//
//Params:
	const WCHAR *pwczText)  //(in)
{
	this->pHeaderLabel->SetText(pwczText);
}

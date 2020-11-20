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

//This is a CDialogWidget that has been augmented to show a list of files.

#ifndef FILEDIALOGWIDGET_H
#define FILEDIALOGWIDGET_H

#include "DialogWidget.h"
#include "OptionButtonWidget.h"
#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/Wchar.h>

static const UINT CX_FILEDIALOG = 500;
static const UINT CY_FILEDIALOG = 732;

class CLabelWidget;
class CListBoxWidget;
class CTextBoxWidget;
class CFileDialogWidget : public CDialogWidget
{
public:
	CFileDialogWidget(const UINT dwSetTagNo, const int nSetX=0,
			const int nSetY=0);

	WSTRING     GetSelectedDirectory() const {return this->dirpath;}
	WSTRING     GetSelectedFileName() const;
	vector<WSTRING> GetSelectedFileNames() const;
	void     PopulateDirList();
	void     PopulateFileList();
	void     SetDirectory(const WCHAR *wDirPath=NULL);
	virtual void   SetExtensions(const UINT extensionType)=0;
	void     SetFilename(const WCHAR *wFilename);
	void     SetPrompt(const WCHAR *pwczText);
	void     SelectFile();
	void     SelectMultipleFiles(const bool bVal);
	void     SetWriting(const bool bWriting) {this->bWriting = bWriting;}

protected:
	virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDoubleClick(const UINT dwTagNo);
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);
	virtual void   OnSelectChange(const UINT dwTagNo);

	void     ClearExtensionList(void);
	void     AddExtensionDesc(const WCHAR *wszDesc);

	vector<WSTRING>  extensions;  //file extension filters (first is primary).
	CTextBoxWidget *pDirNameTextBox;

private:
	static void  CheckDrives();

	void     GoToDirectory();

	CLabelWidget *pHeaderLabel;
	CListBoxWidget *pDirListBoxWidget, *pFileListBoxWidget, *pExtensionListBoxWidget;
	CTextBoxWidget *pFilenameTextBox;
	COptionButtonWidget *pShowHiddenOptionWidget;

	WSTRING  dirpath;    //current directory
	bool     bWriting;   //whether file selected will be written to disk or not (i.e. read in)
};

#endif

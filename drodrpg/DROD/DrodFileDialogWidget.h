// $Id: DrodFileDialogWidget.h 8102 2007-08-15 14:55:40Z trick $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//This is a CFileDialogWidget that has been specialized for DROD use.

#ifndef DRODFILEDIALOGWIDGET_H
#define DRODFILEDIALOGWIDGET_H

#include <FrontEndLib/FileDialogWidget.h>

namespace FileExtension
{
	enum Extensions
	{
		PLAYER=0,
		HOLD=1,
		DEMO=2,
		OGG=3,
		HTML=4,
		JPEG=5,
		PNG=6,
		WAV=7,
		DATA=8,
		XML=9,
		THEORA=10,
		SAVE=11,
		EXT_COUNT,
		MAX_EXTENSIONS=32 //bits
	};
};

#define EXT_PLAYER (1 << FileExtension::PLAYER)
#define EXT_HOLD   (1 << FileExtension::HOLD)
#define EXT_DEMO   (1 << FileExtension::DEMO)
#define EXT_OGG    (1 << FileExtension::OGG)
#define EXT_HTML   (1 << FileExtension::HTML)
#define EXT_JPEG   (1 << FileExtension::JPEG)
#define EXT_PNG    (1 << FileExtension::PNG)
#define EXT_WAV    (1 << FileExtension::WAV)
#define EXT_DATA   (1 << FileExtension::DATA)
#define EXT_XML    (1 << FileExtension::XML)
#define EXT_THEORA (1 << FileExtension::THEORA)
#define EXT_SAVE   (1 << FileExtension::SAVE)

class CDrodFileDialogWidget : public CFileDialogWidget
{
public:
	CDrodFileDialogWidget(const UINT dwSetTagNo, const int nSetX=0,
			const int nSetY=0);

	static UINT    GetExtensionType(const WSTRING& wFilename);

	virtual void   SetExtensions(const UINT extensionTypes);
	void     SetPrompt(const MESSAGE_ID messageID);
};

#endif

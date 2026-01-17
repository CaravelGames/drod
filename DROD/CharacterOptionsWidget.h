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

//This is a CDialog that has been augmented to customize options of a character monster

#ifndef CHARACTEROPTIONSDIALOGWIDGET_H
#define CHARACTEROPTIONSDIALOGWIDGET_H

#include "EditRoomScreen.h"
#include <FrontEndLib/DialogWidget.h>
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/LabelWidget.h>

#include "../Texts/MIDs.h"
#include "../DRODLib/Db.h"

class CCharacterOptionsDialog: public CDialogWidget
{
public:
	static const UINT TAG_SAVE = 996;
	static const UINT TAG_CANCEL = 995;

	CCharacterOptionsDialog();

	CTextBoxWidget* pSpeechColorRedTextBox;
	CTextBoxWidget* pSpeechColorBlueTextBox;
	CTextBoxWidget* pSpeechColorGreenTextBox;
	CTextBoxWidget *pSequenceTextBox;

	void SetCharacter(const CCharacter *pCharacter);
	void SetCharacter(HoldCharacter *pCharacter);
	UINT GetSpeechColor();
	UINT GetProcessSequence();

protected:
	virtual void OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);

private:
	void SetSpeechColorTexts(UINT color);

	static const UINT CY_SPACE = 15;
	static const UINT CX_SPACE = 15;

	static const int CX_DIALOG = 400;
	static const int CY_DIALOG = 467;

	static const int CX_TITLE = 200;
	static const int CY_TITLE = CY_LABEL_FONT_HEADER;
	static const int X_TITLE = (CX_DIALOG - CX_TITLE) / 2;
	static const int Y_TITLE = CY_SPACE;

	static const int CX_SPEECHCOLORLABEL = 150;
	static const int X_SPEECHCOLORLABEL = CX_SPACE;
	static const int Y_SPEECHCOLORLABEL = Y_TITLE + CY_TITLE + CY_SPACE;

	static const int CX_SPEECHCOLORTEXT = 45;
	static const int X_SPEECHCOLORTEXT_RED = X_SPEECHCOLORLABEL + CX_SPEECHCOLORLABEL + CX_SPACE;
	static const int X_SPEECHCOLORTEXT_GREEN = X_SPEECHCOLORTEXT_RED + CX_SPEECHCOLORTEXT + CX_SPACE / 2;
	static const int X_SPEECHCOLORTEXT_BLUE = X_SPEECHCOLORTEXT_GREEN + CX_SPEECHCOLORTEXT + CX_SPACE / 2;
	static const int Y_SPEECHCOLORTEXT = Y_SPEECHCOLORLABEL;

	static const int CX_SEQUENCELABEL = 150;
	static const int CY_SEQUENCELABEL = CY_STANDARD_BUTTON;
	static const int X_SEQUENCELABEL = CX_SPACE;
	static const int Y_SEQUENCELABEL = Y_SPEECHCOLORLABEL + CY_STANDARD_BUTTON + CY_SPACE;

	static const int CX_SEQUENCEHELP = 250;
	static const int CY_SEQUENCEHELP = 270;
	static const int X_SEQUENCEHELP = CX_SPACE;
	static const int Y_SEQUENCEHELP = Y_SEQUENCELABEL + CY_SEQUENCELABEL + CY_SPACE;

	static const int CX_SEQUENCETEXT = 130;
	static const int CY_SEQUENCETEXT = CY_STANDARD_BUTTON;
	static const int X_SEQUENCETEXT = X_SEQUENCELABEL + CX_SEQUENCELABEL + CX_SPACE;
	static const int Y_SEQUENCETEXT = Y_SEQUENCELABEL;

	static const int CX_SAVE = 100;
	static const int CY_SAVE = CY_STANDARD_BUTTON;
	static const int X_SAVE = CX_DIALOG / 2 - CX_SAVE - CX_SPACE / 2;
	static const int Y_SAVE = CY_DIALOG - CY_SAVE - CY_SPACE;

	static const int CX_CANCEL = 100;
	static const int CY_CANCEL = CY_STANDARD_BUTTON;
	static const int X_CANCEL = CX_DIALOG / 2 + CX_SPACE / 2;
	static const int Y_CANCEL = CY_DIALOG - CY_SAVE - CY_SPACE;

	static const int SPEECHCOLOR_MAX_LENGTH = 3;
	static const int PROCESSING_SEQUENCE_MAX_LENGTH = 9;
};

#endif
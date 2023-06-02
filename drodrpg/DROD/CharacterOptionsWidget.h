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

#include "../Texts/MIDs.h"
#include "../DRODLib/Db.h"

class CCharacterOptionsDialog: public CDialogWidget
{
public:
	static const UINT TAG_SAVE = 996;
	static const UINT TAG_CANCEL = 995;

	CCharacterOptionsDialog();

	CTextBoxWidget *pSequenceTextBox;

	void SetCharacter(const CCharacter *pCharacter);
	void SetCharacter(HoldCharacter *pCharacter);
	UINT GetProcessSequence();

protected:
	virtual void OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);

private:
	static const UINT SPACE_CY = 15;
	static const UINT SPACE_CX = 15;

	static const int DIALOG_CX = 400;
	static const int DIALOG_CY = 230;

	static const int TITLE_CX = 200;
	static const int TITLE_CY = 30;
	static const int TITLE_X = (DIALOG_CX - TITLE_CX) / 2;
	static const int TITLE_Y = SPACE_CY;

	static const int SEQUENCELABEL_CX = 150;
	static const int SEQUENCELABEL_CY = CY_STANDARD_BUTTON;
	static const int SEQUENCELABEL_X = SPACE_CX;
	static const int SEQUENCELABEL_Y = TITLE_Y + TITLE_CY + SPACE_CY;

	static const int SEQUENCEHELP_CX = 370;
	static const int SEQUENCEHELP_CY = 270;
	static const int SEQUENCEHELP_X = SPACE_CX;
	static const int SEQUENCEHELP_Y = SEQUENCELABEL_Y + SEQUENCELABEL_CY + SPACE_CY;

	static const int SEQUENCETEXT_CX = 130;
	static const int SEQUENCETEXT_CY = CY_STANDARD_BUTTON;
	static const int SEQUENCETEXT_X = SEQUENCELABEL_X + SEQUENCELABEL_CX + SPACE_CX;
	static const int SEQUENCETEXT_Y = SEQUENCELABEL_Y;

	static const int SAVE_CX = 100;
	static const int SAVE_CY = CY_STANDARD_BUTTON;
	static const int SAVE_X = DIALOG_CX / 2 - SAVE_CX - SPACE_CX / 2;
	static const int SAVE_Y = DIALOG_CY - SAVE_CY - SPACE_CY;

	static const int CANCEL_CX = 100;
	static const int CANCEL_CY = CY_STANDARD_BUTTON;
	static const int CANCEL_X = DIALOG_CX / 2 + SPACE_CX / 2;
	static const int CANCEL_Y = DIALOG_CY - SAVE_CY - SPACE_CY;

	static const int PROCESSING_SEQUENCE_MAX_LENGTH = 9;
};

#endif
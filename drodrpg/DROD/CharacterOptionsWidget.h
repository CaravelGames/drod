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

	CTextBoxWidget *pColorTextBox;
	CTextBoxWidget *pSpeechColorRedTextBox;
	CTextBoxWidget *pSpeechColorBlueTextBox;
	CTextBoxWidget *pSpeechColorGreenTextBox;
	CTextBoxWidget *pSequenceTextBox;

	COptionButtonWidget *pGhostDisplayCheckbox;
	COptionButtonWidget *pMinimapTreasureCheckbox;

	void SetCharacter(const CCharacter *pCharacter);
	void SetCharacter(HoldCharacter *pCharacter);
	UINT GetColor();
	bool GetGhostDisplay();
	UINT GetSpeechColor();
	UINT GetProcessSequence();
	bool GetMinimapTreasure();

protected:
	virtual void OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);

private:
	void SetSpeechColorTexts(UINT color);

	static const UINT SPACE_CY = 15;
	static const UINT SPACE_CX = 15;

	static const int DIALOG_CX = 400;
	static const int DIALOG_CY = 390;

	static const int LABEL_CX = 150;
	static const int LABEL_CY = CY_STANDARD_BUTTON;
	static const int TEXT_CX = 150;
	static const int TEXT_CY = CY_STANDARD_BUTTON;

	static const int TITLE_CX = 200;
	static const int TITLE_CY = 30;
	static const int TITLE_X = (DIALOG_CX - TITLE_CX) / 2;
	static const int TITLE_Y = SPACE_CY;

	static const int GHOSTDISPLAY_BUTTON_X = SPACE_CX;
	static const int GHOSTDISPLAY_BUTTON_Y = TITLE_Y + TITLE_CY + SPACE_CY;

	static const int TREASURE_BUTTON_X = SPACE_CX;
	static const int TREASURE_BUTTON_Y = GHOSTDISPLAY_BUTTON_Y + TITLE_CY + SPACE_CY / 2;

	static const int COLORLABEL_CX = 150;
	static const int COLORLABEL_X = SPACE_CX;
	static const int COLORLABEL_Y = TREASURE_BUTTON_Y + TITLE_CY + SPACE_CY;

	static const int COLORTEXT_X = COLORLABEL_X + COLORLABEL_CX + SPACE_CX;
	static const int COLORTEXT_Y = COLORLABEL_Y;

	static const int SPEECHCOLORLABEL_CX = 150;
	static const int SPEECHCOLORLABEL_X = SPACE_CX;
	static const int SPEECHCOLORLABEL_Y = COLORLABEL_Y + TITLE_CY + SPACE_CY;

	static const int SPEECHCOLORTEXT_CX = 45;
	static const int SPEECHCOLORTEXTRED_X = SPEECHCOLORLABEL_X + SPEECHCOLORLABEL_CX + SPACE_CX;
	static const int SPEECHCOLORTEXTGREEN_X = SPEECHCOLORTEXTRED_X + SPEECHCOLORTEXT_CX + SPACE_CX/2;
	static const int SPEECHCOLORTEXTBLUE_X = SPEECHCOLORTEXTGREEN_X + SPEECHCOLORTEXT_CX + SPACE_CX/2;
	static const int SPEECHCOLORTEXT_Y = SPEECHCOLORLABEL_Y;

	static const int SEQUENCELABEL_X = SPACE_CX;
	static const int SEQUENCELABEL_Y = SPEECHCOLORLABEL_Y + TITLE_CY + SPACE_CY;

	static const int SEQUENCEHELP_CX = 370;
	static const int SEQUENCEHELP_CY = 270;
	static const int SEQUENCEHELP_X = SPACE_CX;
	static const int SEQUENCEHELP_Y = SEQUENCELABEL_Y + LABEL_CY + SPACE_CY;

	static const int SEQUENCETEXT_X = SEQUENCELABEL_X + LABEL_CX + SPACE_CX;
	static const int SEQUENCETEXT_Y = SEQUENCELABEL_Y;

	static const int SAVE_CX = 100;
	static const int SAVE_CY = CY_STANDARD_BUTTON;
	static const int SAVE_X = DIALOG_CX / 2 - SAVE_CX - SPACE_CX / 2;
	static const int SAVE_Y = DIALOG_CY - SAVE_CY - SPACE_CY;

	static const int CANCEL_CX = 100;
	static const int CANCEL_CY = CY_STANDARD_BUTTON;
	static const int CANCEL_X = DIALOG_CX / 2 + SPACE_CX / 2;
	static const int CANCEL_Y = DIALOG_CY - SAVE_CY - SPACE_CY;

	static const int COLOR_MAX_LENGTH = 6;
	static const int SPEECHCOLOR_MAX_LENGTH = 3;
	static const int PROCESSING_SEQUENCE_MAX_LENGTH = 9;
};

#endif
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

#include "CharacterOptionsWidget.h"
#include <FrontEndLib/TextBoxWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include "DrodFontManager.h"

//******************************************************************************
CCharacterOptionsDialog::CCharacterOptionsDialog(
//Constructor.
)
: CDialogWidget(0L, 0, 0, CCharacterOptionsDialog::DIALOG_CX, CCharacterOptionsDialog::DIALOG_CY, false)
{
	AddWidget(new CButtonWidget(TAG_SAVE,
		SAVE_X, SAVE_Y,
		SAVE_CX, SAVE_CY,
		g_pTheDB->GetMessageText(MID_Save)));

	AddWidget(new CButtonWidget(CCharacterOptionsDialog::TAG_CANCEL,
		CCharacterOptionsDialog::CANCEL_X, CCharacterOptionsDialog::CANCEL_Y,
		CCharacterOptionsDialog::CANCEL_CX, CCharacterOptionsDialog::CANCEL_CY,
		g_pTheDB->GetMessageText(MID_Cancel)));

	AddWidget(new CLabelWidget(0L, TITLE_X, TITLE_Y,
		TITLE_CX, TITLE_CY, F_Header, g_pTheDB->GetMessageText(MID_CharOptionsTitle)));


	AddWidget(new CLabelWidget(0L, SEQUENCELABEL_X, SEQUENCELABEL_Y,
		SEQUENCELABEL_CX, SEQUENCELABEL_CY, F_Small, g_pTheDB->GetMessageText(MID_ProcessingSequence)));

	AddWidget(new CLabelWidget(0L, SEQUENCEHELP_X, SEQUENCEHELP_Y,
		SEQUENCEHELP_CX, SEQUENCEHELP_CY, F_Small, g_pTheDB->GetMessageText(MID_ProcessingSequenceDescription)));

	this->pSequenceTextBox = new CTextBoxWidget(0L, SEQUENCETEXT_X, SEQUENCETEXT_Y,
		SEQUENCETEXT_CX, SEQUENCETEXT_CY, PROCESSING_SEQUENCE_MAX_LENGTH, TAG_OK);

	this->pSequenceTextBox->SetDigitsOnly(true);
	this->pSequenceTextBox->SetAllowNegative(false);
	this->pSequenceTextBox->AddHotkey(SDLK_RETURN, TAG_SAVE);

	AddWidget(this->pSequenceTextBox);
}

//*****************************************************************************
void CCharacterOptionsDialog::SetCharacter(
	const CCharacter *pCharacter
){
	const UINT bufferLength = PROCESSING_SEQUENCE_MAX_LENGTH + 1; // Added space for null-termination
	WCHAR temp[bufferLength];

	_itoW(pCharacter->wProcessSequence, temp, 10, bufferLength);

	this->pSequenceTextBox->SetText(temp);
}

//*****************************************************************************
void CCharacterOptionsDialog::SetCharacter(
	HoldCharacter *pCharacter
){
	const UINT bufferLength = PROCESSING_SEQUENCE_MAX_LENGTH + 1; // Added space for null-termination
	WCHAR temp[bufferLength];

	_itoW(pCharacter->ExtraVars.GetVar(ParamProcessSequenceStr, 9999), temp, 10, bufferLength);

	this->pSequenceTextBox->SetText(temp);
}

//*****************************************************************************
UINT CCharacterOptionsDialog::GetProcessSequence(){
	return (UINT) this->pSequenceTextBox->GetNumber();
}

//*****************************************************************************
void CCharacterOptionsDialog::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT /*dwTagNo*/,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Event.
{
	switch (Key.keysym.sym) 
	{
		case SDLK_ESCAPE:
			Deactivate();
			dwDeactivateValue = CCharacterOptionsDialog::TAG_CANCEL;
			break;
	}
}

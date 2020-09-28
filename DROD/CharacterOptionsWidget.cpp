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
: CDialogWidget(0L, 0, 0, CCharacterOptionsDialog::CX_DIALOG, CCharacterOptionsDialog::CY_DIALOG, false)
{
	AddWidget(new CButtonWidget(TAG_SAVE,
		X_SAVE, Y_SAVE,
		CX_SAVE, CY_SAVE,
		g_pTheDB->GetMessageText(MID_Save)));

	AddWidget(new CButtonWidget(CCharacterOptionsDialog::TAG_CANCEL,
		CCharacterOptionsDialog::X_CANCEL, CCharacterOptionsDialog::Y_CANCEL,
		CCharacterOptionsDialog::CX_CANCEL, CCharacterOptionsDialog::CY_CANCEL,
		g_pTheDB->GetMessageText(MID_Cancel)));

	AddWidget(new CLabelWidget(0L, X_TITLE, Y_TITLE,
		CX_TITLE, CY_TITLE, F_Header, g_pTheDB->GetMessageText(MID_CharOptionsTitle)));

	AddWidget(new CLabelWidget(0L, X_SEQUENCELABEL, Y_SEQUENCELABEL,
		CX_SEQUENCELABEL, CY_SEQUENCELABEL, F_Small, g_pTheDB->GetMessageText(MID_ProcessingSequenceDesc)));

	AddWidget(new CLabelWidget(0L, X_SEQUENCEHELP, Y_SEQUENCEHELP,
		CX_SEQUENCEHELP, CY_SEQUENCEHELP, F_Small, g_pTheDB->GetMessageText(MID_SetProcessingSequenceDescription)));

	this->pSequenceTextBox = new CTextBoxWidget(0L, X_SEQUENCETEXT, Y_SEQUENCETEXT,
		CX_SEQUENCETEXT, CY_SEQUENCETEXT, PROCESSING_SEQUENCE_MAX_LENGTH, TAG_OK);

	this->pSequenceTextBox->SetDigitsOnly(true);
	this->pSequenceTextBox->SetAllowNegative(false);

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

	_itoW(pCharacter->ExtraVars.GetVar(ParamProcessSequenceStr, SPD_CHARACTER), temp, 10, bufferLength);

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
	//Trap ESC so it doesn't close the parent dialog.
	if (Key.keysym.sym == SDLK_ESCAPE)
	{
		//CWidget *pWidget = GetWidget(TAG_OK2);
		//if (!pWidget)
		//	pWidget = GetWidget(TAG_OK);
		//if (pWidget)
		//	OnClick(pWidget->GetTagNo()); //deactivate
		//return;
	}
}

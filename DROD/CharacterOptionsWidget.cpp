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

	AddWidget(new CLabelWidget(0L, X_SPEECHCOLORLABEL, Y_SPEECHCOLORLABEL,
		CX_SPEECHCOLORLABEL, CY_SEQUENCELABEL, F_Small, g_pTheDB->GetMessageText(MID_CustomSpeechColor)));

	AddWidget(new CLabelWidget(0L, X_SEQUENCELABEL, Y_SEQUENCELABEL,
		CX_SEQUENCELABEL, CY_SEQUENCELABEL, F_Small, g_pTheDB->GetMessageText(MID_ProcessingSequenceDesc)));

	AddWidget(new CLabelWidget(0L, X_SEQUENCEHELP, Y_SEQUENCEHELP,
		CX_SEQUENCEHELP, CY_SEQUENCEHELP, F_Small, g_pTheDB->GetMessageText(MID_SetProcessingSequenceDescription)));

	this->pSpeechColorRedTextBox = new CTextBoxWidget(0L, X_SPEECHCOLORTEXT_RED, Y_SPEECHCOLORTEXT, 
		CX_SPEECHCOLORTEXT, CY_STANDARD_TBOX, SPEECHCOLOR_MAX_LENGTH);
	this->pSpeechColorGreenTextBox = new CTextBoxWidget(0L, X_SPEECHCOLORTEXT_GREEN, Y_SPEECHCOLORTEXT, 
		CX_SPEECHCOLORTEXT, CY_STANDARD_TBOX, SPEECHCOLOR_MAX_LENGTH);
	this->pSpeechColorBlueTextBox = new CTextBoxWidget(0L, X_SPEECHCOLORTEXT_BLUE, Y_SPEECHCOLORTEXT, 
		CX_SPEECHCOLORTEXT, CY_STANDARD_TBOX, SPEECHCOLOR_MAX_LENGTH);

	this->pSpeechColorRedTextBox->SetDigitsOnly(true);
	this->pSpeechColorGreenTextBox->SetDigitsOnly(true);
	this->pSpeechColorBlueTextBox->SetDigitsOnly(true);

	this->pSpeechColorRedTextBox->SetAllowNegative(false);
	this->pSpeechColorGreenTextBox->SetAllowNegative(false);
	this->pSpeechColorBlueTextBox->SetAllowNegative(false);

	AddWidget(this->pSpeechColorRedTextBox);
	AddWidget(this->pSpeechColorGreenTextBox);
	AddWidget(this->pSpeechColorBlueTextBox);

	this->pSequenceTextBox = new CTextBoxWidget(0L, X_SEQUENCETEXT, Y_SEQUENCETEXT,
		CX_SEQUENCETEXT, CY_SEQUENCETEXT, PROCESSING_SEQUENCE_MAX_LENGTH, TAG_OK);

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

	SetSpeechColorTexts(pCharacter->GetCustomSpeechColor());
}

//*****************************************************************************
void CCharacterOptionsDialog::SetCharacter(
	HoldCharacter *pCharacter
){
	const UINT bufferLength = PROCESSING_SEQUENCE_MAX_LENGTH + 1; // Added space for null-termination
	WCHAR temp[bufferLength];

	_itoW(pCharacter->ExtraVars.GetVar(ParamProcessSequenceStr, SPD_CHARACTER), temp, 10, bufferLength);

	this->pSequenceTextBox->SetText(temp);

	SetSpeechColorTexts(pCharacter->ExtraVars.GetVar(ParamSpeechColorStr, 0));
}

//*****************************************************************************
void CCharacterOptionsDialog::SetSpeechColorTexts(UINT color)
{
	UINT r = (color >> 16) & 255;
	UINT g = (color >> 8) & 255;
	UINT b = color & 255;

	const UINT bufferLength = SPEECHCOLOR_MAX_LENGTH + 1; // Added space for null-termination
	WCHAR temp[bufferLength];

	_itoW(r, temp, 10, bufferLength);
	this->pSpeechColorRedTextBox->SetText(temp);
	_itoW(g, temp, 10, bufferLength);
	this->pSpeechColorGreenTextBox->SetText(temp);
	_itoW(b, temp, 10, bufferLength);
	this->pSpeechColorBlueTextBox->SetText(temp);
}

//*****************************************************************************
UINT CCharacterOptionsDialog::GetSpeechColor()
{
	UINT r = min((UINT)this->pSpeechColorRedTextBox->GetNumber(), 255U);
	UINT g = min((UINT)this->pSpeechColorGreenTextBox->GetNumber(), 255U);
	UINT b = min((UINT)this->pSpeechColorBlueTextBox->GetNumber(), 255U);

	UINT value = (r << 16) + (g << 8) + b;

	return value;
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

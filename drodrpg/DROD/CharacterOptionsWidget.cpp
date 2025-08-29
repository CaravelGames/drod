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

	AddWidget(new CLabelWidget(0L, HUELABEL_X, HUELABEL_Y,
		LABEL_CX, LABEL_CY, F_Small, g_pTheDB->GetMessageText(MID_VarMonsterHue)));

	this->pHueTextBox = new CTextBoxWidget(0L, HUETEXT_X, HUETEXT_Y,
		TEXT_CX, TEXT_CY, HUE_STATURATION_MAX_LENGTH, TAG_OK);
	AddWidget(pHueTextBox);

	AddWidget(new CLabelWidget(0L, SATURATIONLABEL_X, SATURATIONLABEL_Y,
		LABEL_CX, LABEL_CY, F_Small, g_pTheDB->GetMessageText(MID_VarMonsterSaturation)));

	this->pSaturationTextBox = new CTextBoxWidget(0L, SATURATIONTEXT_X, SATURATIONTEXT_Y,
		TEXT_CX, TEXT_CY, HUE_STATURATION_MAX_LENGTH, TAG_OK);
	AddWidget(pSaturationTextBox);

	AddWidget(new CLabelWidget(0L, COLORLABEL_X, COLORLABEL_Y,
		LABEL_CX, LABEL_CY, F_Small, g_pTheDB->GetMessageText(MID_VarMonsterColor)));

	this->pColorTextBox = new CTextBoxWidget(0L, COLORTEXT_X, COLORTEXT_Y,
		TEXT_CX, TEXT_CY, COLOR_MAX_LENGTH, TAG_OK);

	this->pColorTextBox->SetDigitsOnly(true);
	this->pColorTextBox->SetAllowNegative(false);
	this->pColorTextBox->AddHotkey(SDLK_RETURN, TAG_SAVE);
	AddWidget(this->pColorTextBox);

	AddWidget(new CLabelWidget(0L, SPEECHCOLORLABEL_X, SPEECHCOLORLABEL_Y,
		LABEL_CX, LABEL_CY, F_Small, g_pTheDB->GetMessageText(MID_CustomSpeechColor)));

	this->pSpeechColorRedTextBox = new CTextBoxWidget(0L, SPEECHCOLORTEXTRED_X, SPEECHCOLORTEXT_Y,
		SPEECHCOLORTEXT_CX, TEXT_CY, SPEECHCOLOR_MAX_LENGTH, TAG_OK);
	this->pSpeechColorGreenTextBox = new CTextBoxWidget(0L, SPEECHCOLORTEXTGREEN_X, SPEECHCOLORTEXT_Y,
		SPEECHCOLORTEXT_CX, TEXT_CY, SPEECHCOLOR_MAX_LENGTH, TAG_OK);
	this->pSpeechColorBlueTextBox = new CTextBoxWidget(0L, SPEECHCOLORTEXTBLUE_X, SPEECHCOLORTEXT_Y,
		SPEECHCOLORTEXT_CX, TEXT_CY, SPEECHCOLOR_MAX_LENGTH, TAG_OK);

	this->pSpeechColorRedTextBox->SetDigitsOnly(true);
	this->pSpeechColorGreenTextBox->SetDigitsOnly(true);
	this->pSpeechColorBlueTextBox->SetDigitsOnly(true);

	this->pSpeechColorRedTextBox->SetAllowNegative(false);
	this->pSpeechColorGreenTextBox->SetAllowNegative(false);
	this->pSpeechColorBlueTextBox->SetAllowNegative(false);

	AddWidget(this->pSpeechColorRedTextBox);
	AddWidget(this->pSpeechColorGreenTextBox);
	AddWidget(this->pSpeechColorBlueTextBox);

	AddWidget(new CLabelWidget(0L, SEQUENCELABEL_X, SEQUENCELABEL_Y,
		LABEL_CX, LABEL_CY, F_Small, g_pTheDB->GetMessageText(MID_ProcessingSequence)));

	AddWidget(new CLabelWidget(0L, SEQUENCEHELP_X, SEQUENCEHELP_Y,
		SEQUENCEHELP_CX, SEQUENCEHELP_CY, F_Small, g_pTheDB->GetMessageText(MID_ProcessingSequenceDescription)));

	this->pSequenceTextBox = new CTextBoxWidget(0L, SEQUENCETEXT_X, SEQUENCETEXT_Y,
		TEXT_CX, TEXT_CY, PROCESSING_SEQUENCE_MAX_LENGTH, TAG_OK);

	this->pSequenceTextBox->SetDigitsOnly(true);
	this->pSequenceTextBox->SetAllowNegative(false);
	this->pSequenceTextBox->AddHotkey(SDLK_RETURN, TAG_SAVE);

	AddWidget(this->pSequenceTextBox);

	this->pGhostDisplayCheckbox = new COptionButtonWidget(0L, GHOSTDISPLAY_BUTTON_X, GHOSTDISPLAY_BUTTON_Y,
		SEQUENCEHELP_CX, LABEL_CY, g_pTheDB->GetMessageText(MID_NPCGhostDisplay));
	AddWidget(this->pGhostDisplayCheckbox);

	this->pMinimapTreasureCheckbox = new COptionButtonWidget(0L, TREASURE_BUTTON_X, TREASURE_BUTTON_Y,
		SEQUENCEHELP_CX, LABEL_CY, g_pTheDB->GetMessageText(MID_MinimapTreasure));
	AddWidget(this->pMinimapTreasureCheckbox);

	this->pPreviewTile = new CTilesWidget(0U, PREVIEW_TILE_X, PREVIEW_TILE_Y,
		CDrodBitmapManager::CX_TILE, CDrodBitmapManager::CY_TILE);
	AddWidget(this->pPreviewTile);

	this->previewTileNumber = TI_UNSPECIFIED;
}

//*****************************************************************************
void CCharacterOptionsDialog::SetCharacter(
	const CCharacter *pCharacter, const HoldCharacter* pHoldCharacter
){
	const UINT bufferLength = PROCESSING_SEQUENCE_MAX_LENGTH + 1; // Added space for null-termination
	WCHAR temp[bufferLength];

	_itoW(pCharacter->wProcessSequence, temp, 10, bufferLength);
	this->pSequenceTextBox->SetText(temp);

	_itoW(pCharacter->getColor(), temp, 10, bufferLength);
	this->pColorTextBox->SetText(temp);

	_itoW(pCharacter->getHue(), temp, 10, bufferLength);
	this->pHueTextBox->SetText(temp);

	_itoW(pCharacter->getSaturation(), temp, 10, bufferLength);
	this->pSaturationTextBox->SetText(temp);

	SetSpeechColorTexts(pCharacter->GetCustomSpeechColor());

	this->pGhostDisplayCheckbox->SetChecked(pCharacter->IsGhostImage());
	this->pMinimapTreasureCheckbox->SetChecked(pCharacter->IsMinimapTreasure());

	if (pHoldCharacter) {
		CalculatePreviewTile(pHoldCharacter);
	} else {
		const UINT wIdentity = pCharacter->wLogicalIdentity;
		this->previewTileNumber = GetTileImageForEntity(wIdentity == M_NONE ?
			static_cast<UINT>(CHARACTER_FIRST) : wIdentity, S, 0);
	}
	UpdatePreview();
}

//*****************************************************************************
void CCharacterOptionsDialog::SetCharacter(
	HoldCharacter *pCharacter
){
	const UINT bufferLength = PROCESSING_SEQUENCE_MAX_LENGTH + 1; // Added space for null-termination
	WCHAR temp[bufferLength];

	_itoW(pCharacter->ExtraVars.GetVar(ParamProcessSequenceStr, 9999), temp, 10, bufferLength);
	this->pSequenceTextBox->SetText(temp);

	_itoW(pCharacter->ExtraVars.GetVar(ColorStr, 0), temp, 10, bufferLength);
	this->pColorTextBox->SetText(temp);

	_itoW(pCharacter->ExtraVars.GetVar(HueStr, 0), temp, 10, bufferLength);
	this->pHueTextBox->SetText(temp);

	_itoW(pCharacter->ExtraVars.GetVar(SaturationStr, 0), temp, 10, bufferLength);
	this->pSaturationTextBox->SetText(temp);

	SetSpeechColorTexts(pCharacter->ExtraVars.GetVar(ParamSpeechColorStr, 0));

	this->pGhostDisplayCheckbox->SetChecked(pCharacter->ExtraVars.GetVar(GhostImageStr, false));
	this->pMinimapTreasureCheckbox->SetChecked(pCharacter->ExtraVars.GetVar(MinimapTreasureStr, false));

	CalculatePreviewTile(pCharacter);
	UpdatePreview();
}

//*****************************************************************************
void CCharacterOptionsDialog::CalculatePreviewTile(const HoldCharacter* pCharacter)
{
	this->previewTileNumber == TI_UNSPECIFIED;
	static const UINT tileIndex = 4; //index for south-facing tile
	this->previewTileNumber = g_pTheBM->GetCustomTileNo(pCharacter->dwDataID_Tiles, tileIndex, 0);
	if (this->previewTileNumber == TI_UNSPECIFIED) {
		this->previewTileNumber = g_pTheBM->GetCustomTileNo(pCharacter->dwDataID_Tiles, 0, 0);
		if (this->previewTileNumber == TI_UNSPECIFIED) {
			this->previewTileNumber = GetTileImageForEntity(pCharacter->wType == M_NONE ?
				static_cast<UINT>(CHARACTER_FIRST) : pCharacter->wType, S, 0);
		}
	}
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
void CCharacterOptionsDialog::UpdatePreview()
{
	this->pPreviewTile->ClearTiles();
	if (this->previewTileNumber == TI_UNSPECIFIED)
		return;

	float r, g, b;
	CRoomWidget::TranslateMonsterColor(GetColor(), r, g, b);

	CCharacter character(NULL);
	character.SetHue(GetHue());
	character.SetSaturation(GetSaturation());

	this->pPreviewTile->AddTile(previewTileNumber, 0, 0, character.getHSV(),
		r, g, b);
	Paint();
}

//*****************************************************************************
UINT CCharacterOptionsDialog::GetColor(){
	return (UINT)this->pColorTextBox->GetNumber();
}

//*****************************************************************************
UINT CCharacterOptionsDialog::GetHue()
{
	return (UINT)this->pHueTextBox->GetNumber();
}

//*****************************************************************************
UINT CCharacterOptionsDialog::GetSaturation()
{
	return (UINT)this->pSaturationTextBox->GetNumber();
}

//*****************************************************************************
bool CCharacterOptionsDialog::GetGhostDisplay()
{
	return this->pGhostDisplayCheckbox->IsChecked();
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
bool CCharacterOptionsDialog::GetMinimapTreasure()
{
	return this->pMinimapTreasureCheckbox->IsChecked();
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

//*****************************************************************************
void CCharacterOptionsDialog::OnKeyUp(
	const UINT dwTagNo, const SDL_KeyboardEvent& Key)
{
	//We update the preview image on key up as this will happen after the text
	//input widgets have updated their content in response to a key down event.
	UpdatePreview();
}

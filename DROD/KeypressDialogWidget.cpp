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
 * ***** END LICENSE BLOCK ***** */

#include "KeypressDialogWidget.h"
#include "DrodFontManager.h"

#include <DRODLib/Db.h>
#include "../Texts/MIDs.h"

const UINT DIALOG_W = 500;
const UINT DIALOG_H = 145;

//********************************************************************************
CKeypressDialogWidget::CKeypressDialogWidget(
//Constructor.
//
//Params:
	UINT dwSetTagNo) //(in)
	: CDialogWidget(dwSetTagNo, 0, 0, DIALOG_W, DIALOG_H),
	bAllowModifiers(false)
{
	this->Key = SDLK_UNKNOWN;

	static const UINT PADDING_W = 10;
	static const UINT PADDING_H = 10;

	static const UINT LABEL_W = DIALOG_W - PADDING_W * 2;
	static const UINT LABEL_H = 25;

	static const int HEADER_X = PADDING_W;
	static const int HEADER_Y = PADDING_H;
	static const int HEADER_BOTTOM = HEADER_Y + LABEL_H + PADDING_H;
	static const int COMMAND_X = PADDING_W;
	static const int COMMAND_Y = HEADER_BOTTOM;
	static const int COMMAND_BOTTOM = COMMAND_Y + LABEL_H + PADDING_H;
	static const int INSTRUCTIONS_X = PADDING_W;
	static const int INSTRUCTIONS_Y = COMMAND_BOTTOM;
	static const int INSTRUCTIONS_H = LABEL_H * 2;

	{ //Header
		CLabelWidget *pLabelWidget = new CLabelWidget(0L,
			HEADER_X, HEADER_Y, LABEL_W, LABEL_H,
			F_Small, g_pTheDB->GetMessageText(MID_GetKeyCommand));
		pLabelWidget->SetAlign(CLabelWidget::TA_CenterGroup);
		this->AddWidget(pLabelWidget);
	}

	{ // Current command
		this->pCommandLabel = new CLabelWidget(0L,
			COMMAND_X, COMMAND_Y, LABEL_W, LABEL_H,
			F_Header, wszEmpty);
		this->pCommandLabel->SetAlign(CLabelWidget::TA_CenterGroup);
		this->AddWidget(this->pCommandLabel);
	}

	{ // Instructions
		this->pInstructionsLabel = new CLabelWidget(0L,
			INSTRUCTIONS_X, INSTRUCTIONS_Y, LABEL_W, INSTRUCTIONS_H,
			F_Small, wszEmpty);
		pInstructionsLabel->SetAlign(CLabelWidget::TA_CenterGroup);
		this->AddWidget(pInstructionsLabel);
	}

	this->bAllowRepeating = false;
}

//********************************************************************************
void CKeypressDialogWidget::OnKeyDown(
//Called when widget receives SDL_KEYUP event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Event.
{
	//Let base class handle alt-F4 and maybe other things.
	CDialogWidget::OnKeyDown(dwTagNo, Key);
	if (IsDeactivating()) return;

	const bool bIsModifierKey = Key.keysym.sym == SDLK_LSHIFT
		|| Key.keysym.sym == SDLK_RSHIFT
		|| Key.keysym.sym == SDLK_LALT
		|| Key.keysym.sym == SDLK_RALT
		|| Key.keysym.sym == SDLK_LCTRL
		|| Key.keysym.sym == SDLK_RCTRL;

	if (bIsModifierKey)
		return; // Registering a modifier is handled in OnKeyUp

	//Remember what key was pressed and deactivate.  The activator can retrieve
	//the key with GetKey() method.
	this->Key = BuildInputKey(Key);
	Deactivate();
}


//********************************************************************************
void CKeypressDialogWidget::OnKeyUp(
	//Called when widget receives SDL_KEYUP event.
	//
	//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Event.
{
	//Let base class handle alt-F4 and maybe other things.
	CDialogWidget::OnKeyUp(dwTagNo, Key);
	if (IsDeactivating()) return;

	const bool bIsModifierKey = Key.keysym.sym == SDLK_LSHIFT
		|| Key.keysym.sym == SDLK_RSHIFT
		|| Key.keysym.sym == SDLK_LALT
		|| Key.keysym.sym == SDLK_RALT
		|| Key.keysym.sym == SDLK_LCTRL
		|| Key.keysym.sym == SDLK_RCTRL;

	if (bIsModifierKey && !this->bAllowModifiers)
		return; // Silently ignore lone modiifer keypresses when not allowing modifiers for better usability

	//Remember what key was pressed and deactivate.  The activator can retrieve
	//the key with GetKey() method.
	this->Key = BuildInputKey(Key);
	Deactivate();
}

//********************************************************************************
void CKeypressDialogWidget::SetupDisplay(const MESSAGE_ID eCommandMid, const bool bAllowModifiers)
{
	this->bAllowModifiers = bAllowModifiers;
	this->pCommandLabel->SetText(g_pTheDB->GetMessageText(eCommandMid));
	this->pInstructionsLabel->SetText(g_pTheDB->GetMessageText(bAllowModifiers ? MID_GetKeyDescription_YesModifiers : MID_GetKeyDescription_NoModifiers));
}
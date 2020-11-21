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

//********************************************************************************
CKeypressDialogWidget::CKeypressDialogWidget(
//Constructor.
//
//Params:
	UINT dwSetTagNo, int nSetX, int nSetY, UINT wSetW, UINT wSetH) //(in)
	: CDialogWidget(dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
{
	this->Key = UNKNOWN_INPUT_KEY;
}

//********************************************************************************
void CKeypressDialogWidget::OnKeyUp(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Event.
{
	//Let base class handle alt-F4 and maybe other things.
	CDialogWidget::OnKeyDown(dwTagNo, Key);
	if (IsDeactivating()) return;

	//Remember what key was pressed and deactivate.  The activator can retrieve
	//the key with GetKey() method.
	this->Key = BuildInputKey(Key);
	Deactivate();
}

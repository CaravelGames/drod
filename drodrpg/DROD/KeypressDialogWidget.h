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

//This is just a CDialog that has been changed so that it deactivates when a 
//keydown event is received.

#ifndef KEYPRESSDIALOGWIDGET_H
#define KEYPRESSDIALOGWIDGET_H

#include <FrontEndLib/DialogWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <BackEndLib/InputKey.h>

#include <SDL.h>

class CKeypressDialogWidget : public CDialogWidget
{
public:
	CKeypressDialogWidget(UINT dwSetTagNo);

	InputKey    GetKey(void) const {return this->Key;}
	void        SetupDisplay(const MESSAGE_ID wCommand, const bool bAllowModifiers);

private:
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);
	virtual void   OnKeyUp(const UINT dwTagNo, const SDL_KeyboardEvent &Key);

	InputKey    Key;
	bool bAllowModifiers;
	CLabelWidget* pCommandLabel;
	CLabelWidget* pInstructionsLabel;
};

#endif

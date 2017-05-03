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
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef BUTTONWIDGET_H
#define BUTTONWIDGET_H

#include "FocusWidget.h"

#include <BackEndLib/Wchar.h>

//Standard height of buttons.
static const UINT CY_STANDARD_BUTTON = 32;

//******************************************************************************
class CButtonWidget : public CFocusWidget
{
public:
	CButtonWidget(UINT dwSetTagNo, int nSetX, int nSetY, UINT wSetW,
			UINT wSetH, const WCHAR *pwczSetCaption);
	virtual ~CButtonWidget();

	virtual void   Paint(bool bUpdateRect = true);
	void           Press() {this->bIsPressed=true;}
	WSTRING        GetCaption() const { return this->wstrCaption;}
	void           SetCaption(const WCHAR *pwczSetCaption);
	void           SetHotkeyFromText(const WCHAR *pwczSetCaption);
	void           SetSilent(const bool bVal=true) {this->bIsSilent = bVal;}
	void           Unpress() {this->bIsPressed=false;}

protected:
	virtual void   HandleKeyDown(const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   HandleMouseDown(const SDL_MouseButtonEvent &Button);
	virtual void   HandleMouseUp(const SDL_MouseButtonEvent &Button);
	virtual void   HandleDrag(const SDL_MouseMotionEvent &Motion);
	virtual bool   IsDoubleClickable() const {return false;}

private:
	void           DrawButtonText(const int wXOffset, const int wYOffset, const int yTextOffset);
	void           DrawFocused(const int wXOffset, const int wYOffset);
	void           DrawNormal();
	void           DrawPressed();

	bool           bIsPressed;
	WSTRING        wstrCaption;
	bool           bIsSilent; //does sound effect play when pressed?
};

#endif //#ifndef BUTTONWIDGET_H

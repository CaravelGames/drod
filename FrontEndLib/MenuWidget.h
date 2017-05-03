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

#ifndef MENUWIDGET_H
#define MENUWIDGET_H

#include "FontManager.h"
#include "FocusWidget.h"
#include <BackEndLib/Wchar.h>

#include <string>
#include <vector>
using std::string;
using std::vector;

enum MenuFont {
	UNSELECTED=0,
	SELECTED,
	MOUSEOVER,
	MENUFONTCOUNT
};

struct MenuOption
{
	MenuOption()
		: dwTag(0)
		, bEnabled(true)
		, pNormal(NULL), pSelected(NULL), pDisabled(NULL)
	{ }
	~MenuOption() {
		if (pNormal)
			SDL_FreeSurface(pNormal);
		if (pSelected)
			SDL_FreeSurface(pSelected);
		if (pDisabled)
			SDL_FreeSurface(pDisabled);
	}

	WSTRING text;  //text to display
	SDL_Rect rect; //location of option in widget
	WCHAR hotkey; //keypress that activates this option
	UINT dwTag;   //ID reference
	bool bEnabled; //option is selectable
	SDL_Surface* pNormal;
	SDL_Surface* pSelected;
	SDL_Surface* pDisabled;
};

//******************************************************************************
class CMenuWidget : public CFocusWidget
{
public:
	CMenuWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY, const UINT wSetW,
			const UINT wSetH, const UINT eUnselectedFont,
			const UINT eMouseOverFont, const UINT eSelectedFont);
	virtual ~CMenuWidget();

	void        AddText(const WCHAR* wszText, const UINT dwTag);
	void        clear();
	void        Enable(const UINT dwTag, const bool bVal);
	UINT		GetOnOption();
	UINT		GetSelectedOption();
	UINT        DispHeight() const {return this->wNextY;}
	virtual void   Paint(bool bUpdateRect = true);
	void        ResetSelection();
	void        SetButtonSound(UINT seid) {this->seid = seid;}
	void        SetFontYOffset(int offset) { y_font_offset = offset; }

protected:
	virtual void   HandleKeyDown(const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   HandleMouseMotion(const SDL_MouseMotionEvent &Motion);
	virtual void   HandleMouseUp(const SDL_MouseButtonEvent &Button);

private:
	bool        InOption(const UINT wIndex, const UINT wX, const UINT wY) const;
	void        SelectOption(const UINT wIndex);

	vector<MenuOption*>  options;
	UINT           eFontType[MENUFONTCOUNT];
	UINT           wNextY;
	int            nSelectedOption, nOnOption;
	int            y_font_offset;
	
	UINT seid;
};

#endif //#ifndef MENUWIDGET_H

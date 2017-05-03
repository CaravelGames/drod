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

#ifndef TABBEDMENUWIDGET_H
#define TABBEDMENUWIDGET_H

#include "FocusWidget.h"
#include <list>
#include <string>
#include <vector>

//******************************************************************************
class CTabbedMenuWidget : public CFocusWidget
{     
public:
	CTabbedMenuWidget(const UINT dwSetTagNo, const int nSetX,
			const int nSetY, const UINT wSetW, const UINT wSetH,
			const UINT wTabs, const UINT wTabsHeight, const BYTE R, const BYTE G, const BYTE B);
	virtual ~CTabbedMenuWidget();

	virtual CWidget *    AddWidgetToTab(CWidget *pNewWidget, const UINT wTab,
			bool bLoad = false);

	virtual void   Paint(bool bUpdateRect = true);
	void           PaintBGImage(SDL_Rect& rect, SDL_Surface* pDestSurface);
	UINT           GetSelectedTab() const {return wSelectedTab;}
	void           SelectTab(const UINT wTab);
	void           SetBGImage(const std::string& filename, const Uint8 opacity=255);
	void           SetTabTile(const UINT wTab, const UINT wTileNo);
	void           SetTabText(const UINT wTab, const WCHAR *pText);

	virtual void      HandleKeyDown(const SDL_KeyboardEvent &KeyboardEvent);

protected:
	virtual void      HandleMouseDown(const SDL_MouseButtonEvent &Button);

private:
	void           DrawTabs();
	Uint32         GetBGColorValue() const;

	std::list<CWidget *>*  plChildWidgetsForTab;
	UINT              wNumTabs;
	UINT              wSelectedTab;
	vector<UINT>      tabTileNo, tabWidths;
	std::vector<WSTRING> tabText;
	SURFACECOLOR		BGColor;

	UINT tabHeight;

	//Optional background image opacity.
	Uint8   BGimageOpacity;
};

#endif //#ifndef TABBEDMENUWIDGET_H

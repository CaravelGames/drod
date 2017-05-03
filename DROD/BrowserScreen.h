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

#ifndef BROWSERSCREEN_H
#define BROWSERSCREEN_H

#include "DrodScreen.h"

#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/HTMLWidget.h>
#include <FrontEndLib/ScrollableWidget.h>

class CBrowserScreen : public CDrodScreen
{
public:
	CHTMLWidget* GetHTMLBrowser() {return this->pHTML;}
	static void SetPageToLoad(const char* wszPageName);

protected:
	friend class CDrodScreenManager;

	CBrowserScreen();
	virtual ~CBrowserScreen();

	virtual bool SetForActivate();

private:
	virtual void OnClick(const UINT dwTagNo);
	virtual void OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);
	virtual void OnMouseWheel(const SDL_MouseWheelEvent &Wheel);
	virtual void Paint(bool bUpdateRect = true);
	void  UpdateWidgets();

	CLabelWidget *pTitle;
	CLabelWidget *pStatus;
	CScrollableWidget *pBrowser;
	CHTMLWidget  *pHTML;

	static WSTRING m_wstrPageToLoad;
};

#endif //...#ifndef BROWSERSCREEN_H

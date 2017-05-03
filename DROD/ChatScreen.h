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
 * Portions created by the Initial Developer are Copyright (C) 2006
 * Caravel Software. All Rights Reserved.
 *
 * Contributors:
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef CHATSCREEN_H
#define CHATSCREEN_H

#include "DrodScreen.h"

class CButtonWidget;
class CListBoxWidget;
class CTextBox2DWidget;
class CChatScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CChatScreen();
	virtual ~CChatScreen();

	virtual void Paint(bool bUpdateRect=true);
	virtual bool SetForActivate();

	virtual void OnBetweenEvents();
	virtual void OnClick(const UINT dwTagNo);
   virtual void OnDeactivate();
	virtual void OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &KeyboardEvent);
	virtual void OnSelectChange(const UINT dwTagNo);

private:
	void     DeleteSelectedMessages();
	void     IgnoreSelectedUsers();
	void     ProcessReceivedData();
	void     SendText(const WCHAR* pText, const CListBoxWidget *pListBox);

	CListBoxWidget *pChatListBox, *pUsersListBox;
	CTextBox2DWidget *pTextWidget;
	CButtonWidget *pSendButton, *pDeleteButton, *pIgnoreButton;
};

#endif //...#ifndef CHATSCREEN_H

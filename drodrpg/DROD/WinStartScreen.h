// $Id: WinStartScreen.h 8102 2007-08-15 14:55:40Z trick $

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
 * Portions created by the Initial Developer are Copyright (C) 
 * 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef WINSTARTSCREEN_H
#define WINSTARTSCREEN_H

#include "DrodScreen.h"
#include <FrontEndLib/LabelWidget.h>

#include <BackEndLib/Types.h>

class CWinStartScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CWinStartScreen();

	virtual void   Paint(bool bUpdateRect=true);
	virtual bool   SetForActivate();

private:
	bool           IsMainDungeon() const;
	virtual void	OnBetweenEvents();
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);
	virtual void   OnMouseUp(const UINT dwTagNo, const SDL_MouseButtonEvent &Button);
	bool           UserWillRateHold();

	bool bCaravelNetHold;
};

#endif //...#ifndef WINSTARTSCREEN_H


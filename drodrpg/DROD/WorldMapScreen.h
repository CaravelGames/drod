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
 * Portions created by the Initial Developer are Copyright (C)
 * 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef WORLDMAPSCREEN_H
#define WORLDMAPSCREEN_H

#include "DrodScreen.h"

#include <BackEndLib/Types.h>

class CCurrentGame;
class CWorldMapWidget;
class CWorldMapScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CWorldMapScreen();

	virtual void   OnBetweenEvents();

	virtual void   Paint(bool bUpdateRect=true);
	virtual bool   SetForActivate();

private:
	void Animate();

	CCurrentGame* GetCurrentGame() const;

	virtual void   OnSelectChange(const UINT dwTagNo);

	void SetMusic(CCurrentGame *pCurrentGame);

	CWorldMapWidget *pWorldMapWidget;
};

#endif //...#ifndef WORLDMAPSCREEN_H

// $Id: ModScreen.h 8370 2007-10-20 16:33:59Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributors:
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef MODSCREEN_H
#define MODSCREEN_H

#include "DrodScreen.h"
#include <vector>

class CButtonWidget;
class CFrameWidget;
class CLabelWidget;
class CListBoxWidget;
class COptionButtonWidget;
class CCurrentGame;

class CModScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CModScreen();
	virtual ~CModScreen();

	virtual void   Paint(bool bUpdateRect=true);
	UINT    GetSelectedItem();

	virtual bool   SetForActivate();

	struct ModInfo {
		ModInfo() : bCaravelNetMod(false), bImportedMod(false), bLocalMod(false) {}
		bool bCaravelNetMod, bImportedMod, bLocalMod;
	};

	enum ModTypeFilter {
		F_ALL,
		F_CNET,
		F_NONCNET,
		F_COUNT
	};

private:
	void     DeleteSelectedMods();
	void     DownloadSelectedMods();
	bool     IsCNetMod(const WCHAR* pName) const;
	bool     IsImportedMod(const WCHAR* pName) const;
	bool     IsLocalMod(const WCHAR* pName) const;
	virtual void OnBetweenEvents();
	virtual void OnClick(const UINT dwTagNo);
	virtual void OnDoubleClick(const UINT dwTagNo);
	virtual void OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);
	virtual void OnSelectChange(const UINT dwTagNo);
	bool     PollForOperationInterrupt();
	void     PopulateModListBox();
	void     RateMod();
	void     RequestThumbnail(CNetMedia *pMedia);
	void     SelectImportedMod(const WSTRING& modName);
	void     SetDesc();
	void     SetCNetModDesc(CNetMedia *pHoldData);
	void     SetRatingLabel(const UINT dwTagNo);
	void     SetFilter();
	void     ShowActiveMod(const UINT dwModID);
	void     ShowCaravelNetWidgets(const bool bLoggedIn, const bool bEmbeddedMod,
			const bool bCaravelNetMod);

	CListBoxWidget *pModListBoxWidget, *pFullModList;
	CLabelWidget *pDesc;
	CLabelWidget *pAuthorName;
	CFrameWidget *pDetailsFrame;
	CButtonWidget *pExportButton, *pDownloadButton, *pDeleteButton;

	std::vector<ModInfo> modInfo;
	ModTypeFilter filter;
	CNetMedia *pSelCNetMod; //set if thumbnail image pending
};

#endif


// $Id: HoldSelectScreen.h 8370 2007-10-20 16:33:59Z mrimer $

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
 * Contributors:
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef HOLDSELECTSCREEN_H
#define HOLDSELECTSCREEN_H

#include "DrodScreen.h"
#include <vector>

class CButtonWidget;
class CFrameWidget;
class CLabelWidget;
class CListBoxWidget;
class COptionButtonWidget;
class CRoomWidget;
class CCurrentGame;

class CHoldSelectScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CHoldSelectScreen();
	virtual ~CHoldSelectScreen();

	virtual void   Paint(bool bUpdateRect=true);
	UINT    GetSelectedItem() const;
	WSTRING  GetSelectedItemText() const;

	virtual bool   SetForActivate();

private:
	struct HoldInfo {
		HoldInfo()
			: bCaravelNetHold(false), bBeta(false), bUpdated(false)
			, bConquered(false), bMastered(false), bInProgress(false)
			, bLocalHold(false), bMine(false)
			, status(0), version(0)
			, localHoldID(0)
			, cnetID(0)
		{}
		UINT getID() const {
			if (bLocalHold)
				return localHoldID;
			const int cnetid = int(cnetID);
			return cnetid < 0 ? cnetid : -cnetid; //return negative number
		}

		bool bCaravelNetHold, bBeta, bUpdated;
		bool bConquered, bMastered, bInProgress;
		bool bLocalHold, bMine;
		long status;
		UINT version;
		UINT localHoldID;
		UINT cnetID;
		WSTRING holdNameText;
	};

	enum HoldTypeFilter {
		F_ALL,
		F_MINE,
		F_NEW,
		F_INPROGRESS,
		F_CONQUERED,
		F_UNCONQUERED,
		F_MASTERED,
		F_UNMASTERED,
		F_CNET,
		F_NONCNET,
		F_BETA,
		F_UPDATED,
		F_OFFICIAL,
		F_COUNT
	};

	enum CNetHoldOrder {
		S_ALPHA,
		S_RATING,
		S_DIFFICULTY,
		S_VOTES,
		S_VERSION,
		S_COUNT
	};

	struct HoldProgress {
		HoldProgress() : inProgress(false), conquered(false), mastered(false) { }

		bool inProgress;
		bool conquered;
		bool mastered;
		WSTRING holdNameWithProgress;
	};
	typedef std::map<UINT, HoldProgress> HoldProgressCache;
	typedef std::map<UINT, HoldProgressCache> PlayerHoldProgressCache;

	PlayerHoldProgressCache playerHoldProgressCache;

	void     AdvanceRestoreGameTurn();
	bool     DecorateHoldText();
	void     DeleteSelectedHolds();
	void     DownloadNewRoomStyles(set<WSTRING>& importedStyles);
	void     DownloadSelectedHolds();
	void     GetHoldID(UINT &dwHoldID);
	double   getSortVal(const UINT eSortType, const CNetMedia* pHoldData, const UINT wIndex);
	virtual void OnBetweenEvents();
	virtual void OnClick(const UINT dwTagNo);
	virtual void OnDoubleClick(const UINT dwTagNo);
	virtual void OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);
	virtual void OnSelectChange(const UINT dwTagNo);
	bool     PollForOperationInterrupt();
	void     PopulateHoldListBox();
	void     PopulateHoldListWithLocalHolds();
	void     PopulateHoldListWithNonLocalCaravelNetHolds();
	void     RateHold();
	void     RequestThumbnail(CNetMedia *pMedia);
	void     SelectHold(const UINT dwHoldID);
	void     SelectImportedHold(const CIDSet& importedHoldIDs);
	void     SetCNetHoldDesc(CNetMedia *pHoldData);
	void     SetHoldDesc();
	void     SetHoldNameText(UINT dwHoldID, const WSTRING& wStr);
	void     SetHoldRatingLabel(const UINT dwTagNo);
	void     SetHoldFilter();
	void     ShowActiveRoom(const UINT dwHoldID);
	void     ShowCaravelNetWidgets(const bool bLoggedIn, const bool bLocalHold,
			const bool bCaravelNetHold, const bool bBetaHold);
	bool     showHoldInList(const HoldInfo& h) const;
	void     UploadHoldScores();

	CListBoxWidget *pHoldListBoxWidget, *pFullHoldList, *pCNetHoldSortList;
	CLabelWidget *pHoldDesc;
	CLabelWidget *pAuthorName;
	CFrameWidget *pDetailsFrame;
	CRoomWidget *pRoomWidget;
	CButtonWidget *pOKButton, *pExportButton, *pDeleteButton, *pDownloadButton;

	std::map<UINT, HoldInfo> holdInfo;
	std::vector<UINT> holdOrder; //order that hold IDs should be listed
	UINT wProcessingHoldLine; //for hold name-status decoration
	CCurrentGame *pCurrentRestoreGame;
	HoldTypeFilter filter;
	CNetMedia *pSelCNetHold; //set if thumbnail image pending
};

#endif


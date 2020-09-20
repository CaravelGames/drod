// $Id: DemosScreen.h 8370 2007-10-20 16:33:59Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996, 
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef DEMOSSCREEN_H
#define DEMOSSCREEN_H

#include "DrodScreen.h"

#include <BackEndLib/Wchar.h>
#include <BackEndLib/Types.h>

#include <expat.h>

#include <string>
using std::string;

struct CNetDemoData
{
	CNetDemoData()
		: lDemoID(0), wNumMoves(0), wTimeElapsed(0), wTimeReceived(0), wCreated(0)
	{}
	long lDemoID;  // for downloading
	UINT wNumMoves, wTimeElapsed, wTimeReceived, wCreated;
	WSTRING userName, playerName;
};

class CCurrentGame;
class CFrameWidget;
class CLabelWidget;
class CListBoxWidget;
class COptionButtonWidget;
class CRoomWidget;

class CDemosScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CDemosScreen();
	virtual ~CDemosScreen();

	virtual bool   SetForActivate();
	void     ResetRoom() {this->dwRoomID = this->dwLevelID = 0;}

public:
	void     ShowRoom(const UINT dwRoomID) {this->dwRoomID = dwRoomID;}

	//For XML parsing.
	void         StartElement(const XML_Char *name, const XML_Char **atts);

private:
	void     AdvanceCurrentDemoTurn();
	void		DeleteAllDemos();
	void     DeleteDemo();
	void		DownloadRoomDemos(const UINT dwRoomID);
	void		DownloadSelectedDemo();
	void     ExportDemos();
	void		GetCNetDemos(CStretchyBuffer &buffer);
	bool     GetItemTextForDemo(UINT dwDemoID,  WSTRING &wstrText, UINT& dwRoomID) const;
	bool     IsDemoAccessible(CDbDemo* pDemo, const UINT dwPlayerID,
			const bool bPlayerIsHoldAuthor) const;
	void		ListCNetDemos();
	virtual void   OnBetweenEvents();
	virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDoubleClick(const UINT dwTagNo);
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);
	virtual void   OnSelectChange(const UINT dwTagNo);
	virtual void   Paint(bool bUpdateRect = true);
	void     PopulateDemoListBox();
	void     RenameDemo();
	void     SetWidgetsToDemo(UINT dwDemoID);
	void		UploadSelectedDemo();
	
	CListBoxWidget *  pDemoListBoxWidget, *pCNetDemoListBoxWidget;
	CRoomWidget *     pRoomWidget;
	CCurrentGame *    pDemoCurrentGame;
	CLabelWidget *    pAuthorWidget, *pCreatedWidget, *pDurationWidget;
	CLabelWidget *    pDescriptionWidget;
	COptionButtonWidget *pShowButton;
	CLabelWidget *    pLBoxHeaderWidget;
	CLabelWidget *    pNoDemoWidget;
	CFrameWidget *    pDetailsFrame;
	COptionButtonWidget *pShowFromLevel;

	UINT    dwRoomID, dwLevelID;   //room/level to show on activation

	//Enumeration of supported demo fields.
	enum DEMOTagType
	{
		First_Demo_Tag=0,
		DemoID=First_Demo_Tag,
		NumMoves,		//# player moves
		TimeElapsed,	//real play time
		TimeReceived,	//CaravelNet received time
		UserName,		//player's CaravelNet name
		PlayerName,		//player's in-game name
		Created,			//demo creation time
		Demo_Tag_Count,
		Unknown_Demo_Tag
	};
	DEMOTagType    ParseTagField(const char *str) const;

	void     ClearCNetDemos();
	UINT		wDemoHandle;	//net interface
	vector<CNetDemoData*> cNetDemos;
	bool     bHoldPublished, bDownloadDemos, bIsAuthor;	//of hold
	bool     bViewRecordsOnly;
	UINT		dwDownloadDemosForRoomID;
	bool		bRetainDemos;	//on next activation
};

#endif //...#ifndef DEMOSSCREEN_H


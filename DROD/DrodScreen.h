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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2003, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef DRODSCREEN_H
#define DRODSCREEN_H

#include "DrodScreenManager.h"
#include "Chat.h"
#include "DrodFileDialogWidget.h"
#include "EntranceSelectDialogWidget.h"
#include <FrontEndLib/Screen.h>
#include "../DRODLib/DbHolds.h"
#include "../DRODLib/NetInterface.h"

#include <vector>

#define TAG_OK_WHOLELEVEL	(TAG_OK + 1)
#define TAG_CNETNAME (1061)
#define TAG_CNETPASSWORD (1062)

#define TAG_EMPTYCHATUSERLIST (UINT(0))

#define UNDO_LEVEL_INCREMENTS (8)
extern const UINT UNDO_SETTING_LEVELS[UNDO_LEVEL_INCREMENTS];

//******************************************************************************
class CDbHold;
class CProgressBarWidget;
class CRoomWidget;
class CCurrentGame;
struct VisualEffectInfo;
class CDrodScreen : public CScreen
{
public:
	CDrodScreen(const SCREENTYPE eSetType);
	virtual ~CDrodScreen() {}
	virtual void   Callback(long val);
	virtual void   Callbackf(float fVal);
	virtual void   CallbackText(const WCHAR* wpText);

	UINT    ImportHoldImage(const UINT holdID, const UINT extensionFlags=EXT_JPEG|EXT_PNG);

	static WSTRING getStatsText(const RoomStats& st);
	static MESSAGE_ID GetVersionMID(const UINT wVersion);
	static bool    IsGameFullVersion();

	UINT           PublicShowOkMessage(const MESSAGE_ID dwMessageID) {return ShowOkMessage(dwMessageID);}
	void           PublicShowStatusMessage(const MESSAGE_ID dwMessageID) {ShowStatusMessage(dwMessageID);}
	UINT           PublicShowYesNoMessage(const MESSAGE_ID dwMessageID) {return ShowYesNoMessage(dwMessageID);}
	void           PublicHideProgressWidget();

	static vector<WSTRING> importFiles;
	WSTRING        callbackContext; //set to provide more contextual callback messages to user

	static UINT    EntrancesInFullVersion();

protected:
	void     AddCloudDialog();
	void     AddDamageEffect(CRoomWidget* pRoomWidget, const CCurrentGame* pGame,
			const UINT monsterType, const CMoveCoord& coord);
	void     AddVisualEffect(const VisualEffectInfo* pEffect, CRoomWidget* pRoomWidget, const CCurrentGame* pGame);
	void     AddVisualCues(CCueEvents& CueEvents, CRoomWidget* pRoomWidget, const CCurrentGame* pGame);
	void     AdvanceDemoPlayback(CCurrentGame* pGame, CRoomWidget* pRoomWidget,
			const UINT containerWidgetTag);

	void BrowseForRoomHints(CDbRoom* pRoom);

	void ProcessImageEvents(CCueEvents& CueEvents, CRoomWidget* pRoomWidget, const CCurrentGame* pGame);

	static UINT    GetEffectDuration(const CCurrentGame* pGame, const UINT baseDuration);
	static UINT    GetParticleSpeed(const CCurrentGame* pGame, const UINT baseSpeed);
	static CDbHold::HoldStatus GetHoldStatus();
	virtual void   ApplyINISettings();
	virtual void   ChatPolling(const UINT tagUserList);
	virtual void   DisplayChatText(const WSTRING& /*text*/, const SDL_Color& /*color*/) {}

	void           ExportCleanup();
	void           ExportHold(const UINT dwHoldID);
	void           ExportHoldScripts(CDbHold *pHold);
	void           ExportHoldTexts(CDbHold *pHold);
	bool           ExportSelectFile(const MESSAGE_ID messageID,
			WSTRING &wstrExportFile, const UINT extensionTypes);
	void           ExportStyle(const WSTRING& style);
	void           GenericNetTransactionWait(const UINT handle, MESSAGE_ID mid,
			const float progressStart=0.0f, const float progressEnd=1.0f);
	void           CloudTransactionWait(MESSAGE_ID mid,
			const float progressStart=0.0f, const float progressEnd=1.0f);
	void           GoToBuyNow();
	void           GoToForum();
	MESSAGE_ID     Import(const UINT extensionTypes, CIDSet& importedIDs, set<WSTRING>& importedStyles);
	bool           ImportConfirm(MESSAGE_ID& result, const WSTRING* pwFilename=NULL);
	MESSAGE_ID     ImportFiles(const vector<WSTRING>& wstrImportFiles,
			CIDSet& importedIDs, set<WSTRING>& importedStyles);
	void           ImportMedia();
	bool           ImportQueuedFiles();
	bool           IsStyleOnDisk(list<WSTRING>& styleName, list<WSTRING>& skies);

	void           EnablePlayerSettings(const UINT dwPlayerID);
	virtual bool   OnQuit();
	bool           ParseConsoleCommand(const WCHAR *pText);
	virtual bool   PlayVideo(const WCHAR *pFilename, const UINT dwHoldID, const int x=0, const int y=0);
	bool           PlayVideo(const UINT dwDataID, const int x=0, const int y=0);
	void           PopulateChatUserList(const UINT tagUserList);
	void           ProcessReceivedChatData(const UINT tagUserList);
	bool           PollForCNetInterrupt();
	void           ReformatChatText(const UINT chatInputTextTag, const bool bNextOption=false);
	void           RequestToolTip(const MESSAGE_ID messageID);
	virtual void   SaveSurface(SDL_Surface *pSurface=NULL);
	CEntranceSelectDialogWidget::BUTTONTYPE SelectListID(
			CEntranceSelectDialogWidget *pEntranceBox,
			CDbHold *pHold, UINT &dwListItemID, const MESSAGE_ID messagePromptID,
			const CEntranceSelectDialogWidget::DATATYPE datatype=CEntranceSelectDialogWidget::Entrances);
	UINT          SelectFile(WSTRING& filePath, WSTRING& fileName,
			const MESSAGE_ID messagePromptID, const bool bWrite,
			const UINT extensionTypes);
	UINT          SelectFiles(WSTRING& filePath, vector<WSTRING>& fileName,
			const MESSAGE_ID messagePromptID, const UINT extensionTypes);
	SCREENTYPE    SelectSellScreen() const;
	virtual void  ShowChatHistory(CEntranceSelectDialogWidget *pBox);
	UINT          ShowGetCloudPlayerDialog();
	UINT          ShowOkMessage(const MESSAGE_ID dwMessageID);
	UINT          ShowOkMessage(const WCHAR *pwczText);
	void          ShowStatusMessage(const MESSAGE_ID dwMessageID);
	UINT          ShowTextInputMessage(const MESSAGE_ID dwMessageID,
			WSTRING &wstrUserInput, const bool bMultiLineText=false,
			const bool bMustEnterText=true);
	virtual UINT  ShowYesNoMessage(const MESSAGE_ID dwMessageID) {return ShowYesNoMessage(dwMessageID, MID_Yes, MID_No);}
	virtual UINT  ShowYesNoMessage(const MESSAGE_ID dwMessageID,
			const MESSAGE_ID dwYesButtonText, const MESSAGE_ID dwNoButtonText);
	UINT  ShowYesNoMessage(const WCHAR* pwczText,
			const MESSAGE_ID dwYesButtonText=MID_Yes, const MESSAGE_ID dwNoButtonText=MID_No);
	virtual void   ToggleScreenSize();
	void           ExportHoldProgressForUpload(const UINT holdID, const UINT playerID, string& outText);

	CProgressBarWidget *    pProgressWidget;
	CDialogWidget  *pGetCloudPlayerDialog;

	bool           bQuitPrompt;

	//Chat.
	bool        bEnableChat; //whether to receive chats while playing
	bool        bReceiveWhispersOnly; //whether to ignore global chats
public:
	static CNetChat::Interface chat;
	static void    logoutFromChat();

private:
	CDbHold::HoldStatus GetInstalledOfficialHold();
	void PrepareDatumForExport(const WSTRING& modName, const WSTRING& wstrFile, CIDSet& ids, const UINT dataType);

	UINT ShowMessage(const MESSAGE_ID dwMessageID);
	UINT ShowMessage(const WCHAR *pwczText);
};

#endif //...#ifndef DRODSCREEN_H

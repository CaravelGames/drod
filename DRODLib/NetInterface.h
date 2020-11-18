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
* Contributor(s): Matt Schikore (Schik)
*
* ***** END LICENSE BLOCK ***** */

#ifndef NETINTERFACE_H
#define NETINTERFACE_H

#include "DbMessageText.h"
#include <BackEndLib/Date.h>
#include <BackEndLib/Internet.h>
#include <BackEndLib/IDSet.h>

#include <string>
#include <vector>
using std::string;
using std::vector;

enum MediaType {
	MT_Hold=0,
	MT_Style=1,
	MT_Music=2,
	MT_Actor=3
};

struct CNetMedia
{
	CNetMedia()
		: wVersion(0), lHoldID(0), status(0), localHoldID(0), bBeta(false), bPlayerIsAuthor(false)
		, mediaType(MT_Hold), wThumbnailHandle(0)
	{}

	UINT wVersion;
	CDate Created, LastModified;
	CDbMessageText OrigAuthorText, AuthorText, CNetNameText, HoldNameText;
	long lHoldID, status;
	UINT localHoldID;

	string difficulty, rating, numVotes, filesize, filesizebytes;
	string myDifficulty, myRating;
	bool bBeta, bPlayerIsAuthor;

	MediaType mediaType;

	CStretchyBuffer* RequestThumbnail();
	string thumbnailURL;
	CStretchyBuffer thumbnail;
	UINT wThumbnailHandle;

	UINT wCloudDemosVer;
	bool bCloudInstalled;

};

struct SendData
{
	SendData(const WSTRING& text)
		: text(text)
	{}
	SendData(const WSTRING& text, const CIDSet& userIDs)
		: text(text), userIDs(userIDs)
	{}
	WSTRING text;
	CIDSet userIDs;
};

class CNetResult {
public:
	CNetResult() { pJson = NULL;  pBuffer = NULL; }
	~CNetResult() { if (pJson) delete pJson; if (pBuffer) delete pBuffer; }

	Json::Value* pJson;
	CStretchyBuffer* pBuffer;
};

// new hold released
#define NOTICE_NEWHOLD		0x00000001
// existing hold updated
#define NOTICE_UPDATEDHOLD	0x00000002
// Anything with a URL.  new PM, post notification?  I don't know.  TBD.
#define NOTICE_GENERICURL	0x00000004
// Notices of this type will send you to the room in question when activated
#define NOTICE_ROOMSPECIFIC	0x00000008
// Notice that new cloud data is available and should be downloaded when possible
#define NOTICE_CLOUDUPDATE	0x00000010
// Bitmask that means all types of notices
#define NOTICE_ALL			0xffffffff


class CNetRoom {
public:
	CNetRoom() { bValid = false; }
	CNetRoom(const class CDbDemo* pDemo);
	CNetRoom(const class CDbRoom* pRoom);
	CNetRoom(const class CDbHold* pHold);
	Json::Value* ToJson() const;
	bool FillFromDemo(UINT dwDemoID);
	bool FillFromDemo(const CDbDemo* pDemo);
	bool FillFromRoom(UINT dwRoomID);
	bool FillFromRoom(const CDbRoom* pRoom);
	bool FillFromHold(UINT dwHoldID);
	bool FillFromHold(const CDbHold* pHold);

	bool bValid;
	time_t gidCreated;
	time_t lastUpdated;
	UINT wGidLevelIndex;
	string strArchitect;
	int dx;
	int dy;
};

class CNetNotice {
public:
	// Generic data
	UINT id;
	UINT type;
	WSTRING title;	// "Hold updated", "#1 score beaten", etc.
	WSTRING text;	// Specifics for the notice.  What hold, what room, what is the URL for, etc.
	// URL
	string url;		// Only relevant for NOTICE_GENERICURL
	// HOLD/ROOM
	int dwServerHoldId;
	CNetRoom room;	// Used for hold specific and room specific notices
	// CLOUD
	UINT dwCloudId;	// The given ID should be downloaded and imported.
};

enum CloudDataType {
	CLOUD_HOLD,
	CLOUD_DEMO,
	CLOUD_PLAYER
};

//****************************************************************************
class CNetInterface {
public:
	CNetInterface();
	virtual ~CNetInterface() {}

	virtual bool Busy() const {return false;}
	virtual void ClearActiveAction();
	void Disable() {Enable(false);}
	virtual void Enable(const bool bVal=true);
	bool IsEnabled() const {return this->bEnabled;}

	virtual bool ChatLogout();
	virtual UINT DownloadDemo(const long);
	virtual UINT DownloadHold(const long);
	virtual UINT DownloadHoldList();
	virtual UINT DownloadRecords(const CNetRoom&, const bool);
	virtual UINT DownloadStyle(const WCHAR*);
	virtual UINT HasRegisteredAccess();
	virtual void MatchCNetHolds() {}
	virtual UINT RateHold(const WCHAR*, const float, const float);
	virtual UINT RateMod(const WCHAR*, const float);
	virtual UINT RequestNewKey(const string& strUser);
	virtual UINT SendChatText(const SendData&, const int, const UINT);
	virtual UINT SendChatText(const vector<SendData>&, const int, const UINT);
	virtual UINT UploadDemo(const CNetRoom&, const WSTRING&, const string&,
			const UINT, const UINT, const UINT);
	virtual UINT UploadDemos(const string& text);
	virtual bool UploadExploredRooms(const string& buffer);
	virtual bool UploadExploredRoom(const CNetRoom& room);
	virtual UINT UploadChallengeDemo(const CNetRoom&, const string&) {return 0;}

	UINT GetBytesCompleted(const UINT handle);
	virtual CNetResult* GetResults(const UINT handle);
	virtual int  GetStatus(const UINT handle);

	virtual vector<CNetMedia*>& GetCNetMedia() {return this->cNetMedia;}
	virtual CIDSet& GetBetaHolds() {return this->betaHolds;}
	virtual CIDSet& GetLocalHolds() {return this->localHolds;}
	virtual CIDSet& GetUpdatedHolds() {return this->updatedHolds;}
	virtual CIDSet& GetUpdatedCloudDemosHolds() {return this->updatedCloudDemosHolds;}
	int     getMediaIndexWithLocalHoldID(const UINT holdID);
	bool    IsBetaHold(const UINT dwID);
	bool    IsLocalHold(const UINT dwID);
	bool    IsUpdatedHold(const UINT dwID);
	virtual bool IsLoggedIn();

	// Cloud stuff
	virtual UINT CloudUploadProgress(const UINT /*dwHoldID*/) {return 0;}
	virtual UINT CloudDownloadProgress(const UINT /*dwHoldID*/) {return 0;}
	virtual UINT CloudUploadMyHold(const UINT /*dwHoldId*/) {return 0;}
	virtual UINT CloudDownloadHold(/*id*/) {return 0;}
	virtual UINT CloudUploadGeneralHold(const UINT /*dwHoldId*/) { return 0; }
	virtual UINT CloudDownloadGeneralHold(const Json::Value&) { return 0; }
	virtual UINT CloudUploadData(CloudDataType /*type*/, const CNetRoom& /*room*/, const int /*version*/, const std::string& /*data*/) { return 0; }
	virtual UINT CloudInitialize() { return 0; }
	virtual UINT CloudGetPlayer(const WCHAR* /*pUsername*/ = NULL, const WCHAR* /*pKey*/ = NULL) { return 0; }
	virtual UINT CloudSetHoldInstalled(const UINT /*dwHoldId*/, bool /*bInstalled*/) { return 0; }
	virtual bool IsHoldInCloudQueue(const UINT /*holdID*/) const { return false; }
	virtual UINT CloudQueueSize() const { return 0; }

	virtual UINT GetCloudVersionID(const char* /*settingsKey*/, const UINT /*id*/) const { return 0; }
	virtual bool SetCloudVersionID(const char* /*settingsKey*/, const UINT /*id*/, const UINT /*version*/) const { return true; }

	void SetCloudDemosCurrent(const UINT holdID) { this->updatedCloudDemosHolds -= holdID; }

	// Notices
	virtual void SetLastNotice(const UINT /*dwNoticeID*/) {}
	virtual void QueryNotices(vector<CNetNotice> &/*notices*/, UINT /*typeMask*/, UINT /*lastId*/) const {}
	virtual void SetDownloadHold(const int /*dwServerHoldId*/) {}
	virtual int GetDownloadHold() const { return 0; }

	int getIndexForName(const WCHAR* pName, const MediaType mediaType=MT_Hold);

	static const UINT GLOBAL_CHATROOM;
	static const string cNetBaseURL;

protected:
	void         ClearCNetHolds();
	virtual CPostData* SetCredentials(bool bVerify=true);
	virtual CPostData* SetCredentials(const string& strUser, const string& strKey, bool bVerify = true);
	virtual bool VerifyKeyFormat(const string& strKey);

	vector<CNetMedia*> cNetMedia;  //CaravelNet published media
	CIDSet betaHolds;    //CaravelNet beta holds
	CIDSet localHolds;   //CaravelNet holds installed locally
	CIDSet updatedHolds; //installed holds that have a newer version on CaravelNet
	CIDSet updatedCloudDemosHolds; // installed holds that have a newer version of cloud demos

	static CIDSet currentHandles;
	bool bEnabled;
};

//Define global pointer to the one and only net interface object.
#  ifndef INCLUDED_FROM_NETINTERFACE_CPP
extern CNetInterface *g_pTheNet;
#  endif

#endif //...#ifndef NETINTERFACE_H

// $Id: NetInterface.h 9656 2011-08-25 16:13:27Z mrimer $

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

extern const char useInternetStr[];
extern const char playerScoresOld[];

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

//****************************************************************************
class CDbSavedGame;
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
	virtual UINT DownloadRecords(const string&, const bool);
	virtual UINT DownloadStyle(const WCHAR*);
	virtual UINT HasRegisteredAccess();
	virtual void MatchCNetHolds() {}
	virtual UINT RateHold(const WCHAR*, const float, const float);
	virtual UINT RateMod(const WCHAR*, const float);
	virtual UINT RequestNewKey(const string& strUser);
	virtual UINT SendChatText(const SendData&, const int, const UINT);
	virtual UINT SendChatText(const vector<SendData>&, const int, const UINT);
	virtual UINT UploadDemo(const string& buffer, const UINT, const UINT);
	virtual bool UploadDemos(const string& text);
	virtual bool UploadExploredRooms(const string& buffer);
	virtual UINT UploadScore(const string&, const WSTRING&, const UINT) {return 0;}

	UINT    GetBytesCompleted(const UINT handle);
	virtual CStretchyBuffer* GetResults(const UINT handle);
	virtual int  GetStatus(const UINT handle);

	virtual string GetChecksum(CDbSavedGame*, const UINT=0) const;
	virtual bool   VerifyChecksum(CDbSavedGame*, const UINT=0) const;

	virtual vector<CNetMedia*>& GetCNetMedia() {return this->cNetMedia;}
	virtual CIDSet& GetBetaHolds() {return this->betaHolds;}
	virtual CIDSet& GetLocalHolds() {return this->localHolds;}
	virtual CIDSet& GetUpdatedHolds() {return this->updatedHolds;}
	int     getMediaIndexWithLocalHoldID(const UINT holdID);
	bool    IsBetaHold(const UINT dwID);
	bool    IsLocalHold(const UINT dwID);
	bool    IsUpdatedHold(const UINT dwID);
	virtual bool IsLoggedIn();

	int getIndexForName(const WCHAR* pName, const MediaType mediaType=MT_Hold);

	static const UINT GLOBAL_CHATROOM;

protected:
	void         ClearCNetHolds();
	virtual bool SetCookies(bool bVerify=true);
	virtual bool SetCookies(const string& strUser, const string& strKey, bool bVerify = true);
	virtual bool VerifyKeyFormat(const string& strKey);

	vector<CNetMedia*> cNetMedia;  //CaravelNet published media
	CIDSet betaHolds;    //CaravelNet beta holds
	CIDSet localHolds;   //CaravelNet holds installed locally
	CIDSet updatedHolds; //installed holds that have a newer version on CaravelNet

	static UINT wCurrentHandle;
	bool bEnabled;
};

//Define global pointer to the one and only net interface object.
#  ifndef INCLUDED_FROM_NETINTERFACE_CPP
extern CNetInterface *g_pTheNet;
#  endif

#endif //...#ifndef NETINTERFACE_H


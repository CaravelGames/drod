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

//NetInterface.cpp.
//Base implementation of CNetInterface

#define INCLUDED_FROM_NETINTERFACE_CPP
#include "NetInterface.h"
#undef INCLUDED_FROM_NETINTERFACE_CPP

#include "Db.h"
#include "DbDemos.h"
#include "CurrentGame.h"

#include <BackEndLib/Base64.h>

//Holds the only instance of the net interface for the app.
CNetInterface *g_pTheNet = NULL;

CIDSet CNetInterface::currentHandles;

const UINT CNetInterface::GLOBAL_CHATROOM = 0;

const string CNetInterface::cNetBaseURL = "http://forum.caravelgames.com/";

//
//Public methods.
//

//*****************************************************************************
CStretchyBuffer* CNetMedia::RequestThumbnail()
//Downloads thumbnail if not available and not already requested.
//If requested, then check on response status of request.
//
//Returns: pointer to thumbnail buffer if available, else NULL.
{
	if (!this->thumbnail.empty())
		return &this->thumbnail; //thumbnail is available

	if (this->wThumbnailHandle)
	{
		//Request sent previously -- check status.
		const int nStatus = CInternet::GetStatus(this->wThumbnailHandle);
		if (nStatus < 0)
			return NULL; //not arrived yet

		//Arrived.
		CStretchyBuffer *pBuffer = CInternet::GetResults(this->wThumbnailHandle);
		this->wThumbnailHandle = 0; //reset
		if (pBuffer)
		{
			this->thumbnail = *pBuffer;
			delete pBuffer;
		}
		return &this->thumbnail;
	}

	//Thumbnail not requested yet -- request now.
	if (!this->thumbnailURL.empty())
		CInternet::HttpGet(this->thumbnailURL, &this->wThumbnailHandle);

	return NULL;
}

//*****************************************************************************
Json::Value* CNetRoom::ToJson() const
{
	Json::Value* pValue = new Json::Value;

	(*pValue)["created"] = (Json::Value::Int64)this->gidCreated;
	(*pValue)["lastUpdated"] = (Json::Value::Int64)this->lastUpdated;
	(*pValue)["levelIndex"] = this->wGidLevelIndex;
	(*pValue)["dx"] = this->dx;
	(*pValue)["dy"] = this->dy;
	(*pValue)["architect"] = this->strArchitect;

	return pValue;
}

//*****************************************************************************
bool CNetRoom::FillFromDemo(UINT dwDemoID) {
	bool bStatus = false;
	CDbDemo* pDemo = g_pTheDB->Demos.GetByID(dwDemoID);
	if (pDemo) {
		bStatus = this->FillFromDemo(pDemo);
		delete pDemo;
	}
	return bStatus;
}

//*****************************************************************************
bool CNetRoom::FillFromDemo(const CDbDemo* pDemo) {
	bool bStatus = false;
	if (pDemo) {
		CCurrentGame* pCurrentGame = pDemo->GetCurrentGame();
		if (pCurrentGame) {
			CDbRoom* pRoom = pCurrentGame->GetRoom();
			if (pRoom) {
				bStatus = this->FillFromRoom(pRoom);
				delete pRoom;
			}
			delete pCurrentGame;
		}
	}
	return bStatus;
}

//*****************************************************************************
bool CNetRoom::FillFromRoom(UINT dwRoomID) {
	bool bStatus = false;
	CDbRoom* pRoom = g_pTheDB->Rooms.GetByID(dwRoomID);
	if (pRoom) {
		bStatus = this->FillFromRoom(pRoom);
		delete pRoom;
	}
	return bStatus;
}

//*****************************************************************************
bool CNetRoom::FillFromRoom(const CDbRoom* pRoom) {
	bool bStatus = false;
	if (pRoom) {
		CDbLevel* pLevel = pRoom->GetLevel();
		if (pLevel) {
			CDbHold* pHold = pLevel->GetHold();
			if (pHold) {
				CDbPlayer* pPlayer = g_pTheDB->Players.GetByID(pHold->dwPlayerID);
				this->strArchitect = Base64::encode((WSTRING)pPlayer->NameText);
				delete pPlayer;
				this->gidCreated = pHold->GetCreated();
				this->lastUpdated = pHold->LastUpdated;
				this->wGidLevelIndex = pLevel->dwLevelIndex;
				pRoom->GetPositionInLevel(this->dx, this->dy);
				bStatus = true;
				delete pHold;
			}
			delete pLevel;
		}
	}
	return bStatus;
}

//*****************************************************************************
bool CNetRoom::FillFromHold(const CDbHold* pHold) {
	bool bStatus = false;
	if (pHold) {
		CDbPlayer* pPlayer = g_pTheDB->Players.GetByID(pHold->dwPlayerID);
		this->strArchitect = Base64::encode((WSTRING)pPlayer->NameText);
		delete pPlayer;
		this->gidCreated = pHold->GetCreated();
		this->lastUpdated = pHold->LastUpdated;
		this->wGidLevelIndex = 0;
		this->dx = this->dy = 0;
		bStatus = true;
	}
	return bStatus;
}

//*****************************************************************************
CNetRoom::CNetRoom(const CDbDemo *pDemo) {
	this->bValid = this->FillFromDemo(pDemo);
}

//*****************************************************************************
CNetRoom::CNetRoom(const CDbRoom* pRoom) {
	this->bValid = this->FillFromRoom(pRoom);
}

//*****************************************************************************
CNetRoom::CNetRoom(const CDbHold* pHold) {
	this->bValid = this->FillFromHold(pHold);
}

//*****************************************************************************
CNetInterface::CNetInterface()
	: bEnabled(true)
{
}

//*****************************************************************************
void CNetInterface::ClearActiveAction()
{
}

//*****************************************************************************
void CNetInterface::ClearCNetHolds()
{
	UINT wIndex;
	for (wIndex=this->cNetMedia.size(); wIndex--; )
		delete this->cNetMedia[wIndex];
	this->cNetMedia.clear();
	this->betaHolds.clear();
	this->localHolds.clear();
	this->updatedHolds.clear();
}

//*****************************************************************************
int CNetInterface::getMediaIndexWithLocalHoldID(const UINT holdID)
//Returns: index in media list of first item with indicated localHoldID, or -1 if none
{
	for (UINT wIndex=0; wIndex<this->cNetMedia.size(); ++wIndex)
		if (this->cNetMedia[wIndex]->localHoldID == holdID)
			return wIndex;
	return -1;
}

//*****************************************************************************
bool CNetInterface::IsBetaHold(const UINT dwID) {
	const CIDSet& betaHolds = GetBetaHolds();
	return betaHolds.has(dwID);
}
bool CNetInterface::IsLocalHold(const UINT dwID) {
	const CIDSet& localHolds = GetLocalHolds();
	return localHolds.has(dwID);
}
bool CNetInterface::IsUpdatedHold(const UINT dwID) {
	const CIDSet& updatedHolds = GetUpdatedHolds();
	return updatedHolds.has(dwID);
}

//*****************************************************************************
void CNetInterface::Enable(const bool bVal) //[default=true]
{
	this->bEnabled = bVal;
}

//*****************************************************************************
bool CNetInterface::ChatLogout()
//Returns: True if request was sent successfully, false on failure.
{
	return false;
}

//*****************************************************************************
UINT CNetInterface::DownloadDemo(
//Returns: A handle to use to get the response, or 0 on failure.
//
//Params:
	const long)
{
	return 0;
}

//*****************************************************************************
UINT CNetInterface::DownloadHold(
//Returns: A handle to use to get the response, or 0 on failure.
//
//Params:
   const long)
{
   return 0;
}

//*****************************************************************************
UINT CNetInterface::DownloadHoldList()
//Returns: A handle to use to get the response, or 0 on failure.
{
	return 0;
}

//*****************************************************************************
UINT CNetInterface::DownloadStyle(const WCHAR*)
//Returns: A handle to use to get the response, or 0 on failure.
{
	return 0;
}

//*****************************************************************************
UINT CNetInterface::DownloadRecords(const CNetRoom&, const bool)
//Returns: A handle to use to get the response, or 0 on failure.
{
	return 0;
}

//*****************************************************************************
UINT CNetInterface::HasRegisteredAccess()
//Returns: A handle to use to get the response, or 0 on failure.
{
	return 0;
}

//*****************************************************************************
bool CNetInterface::IsLoggedIn()
//Returns: Whether user is logged in to CaravelNet
{
	return false;
}

//*****************************************************************************
UINT CNetInterface::RateHold(const WCHAR*, const float, const float)
//Returns: A handle to use to get the response, or 0 on failure.
{
	return 0;
}

//*****************************************************************************
UINT CNetInterface::RateMod(const WCHAR*, const float)
//Returns: A handle to use to get the response, or 0 on failure.
{
	return 0;
}

//*****************************************************************************
UINT CNetInterface::RequestNewKey(const string& /*strUser*/)
//Returns: A handle to use to get the response, or 0 on failure.
{
	return 0;
}

//*****************************************************************************
UINT CNetInterface::SendChatText(const SendData&, const int, const UINT)
//Returns: A handle to use to get the response, or 0 on failure.
{
	return 0;
}

//*****************************************************************************
UINT CNetInterface::SendChatText(const vector<SendData>&, const int, const UINT)
//Returns: A handle to use to get the response, or 0 on failure.
{
	return 0;
}

//*****************************************************************************
UINT CNetInterface::UploadDemo(const CNetRoom&, const WSTRING&, const string&,
			const UINT, const UINT, const UINT)
//Returns: A handle to use to get the response, or 0 on failure.
{
	return 0;
}

//*****************************************************************************
UINT CNetInterface::UploadDemos(const string& /*text*/)
//Returns: True if upload was sent successfully, false on failure.
{
	return 0;
}

//*****************************************************************************
bool CNetInterface::UploadExploredRooms(const string& /*buffer*/)
//Returns: True if upload was sent successfully, false on failure.
{
	return false;
}

//*****************************************************************************
bool CNetInterface::UploadExploredRoom(const CNetRoom& /*buffer*/)
//Returns: True if upload was sent successfully, false on failure.
{
	return false;
}

//*****************************************************************************
UINT CNetInterface::GetBytesCompleted(
//Retrieve the buffer that corresponds to the given handle
//
//Returns: A handle to use to get the response, or 0 on failure.
//
//Params:
   const UINT handle) // (in) handle of the download to query the status of
{
   return CInternet::GetBytesCompleted(handle);
}

//*****************************************************************************
CNetResult* CNetInterface::GetResults(
//Retrieve the buffer that corresponds to the given handle
//
//Returns: A buffer sent as response to the specified request, or NULL on failure.
//
//Params:
	const UINT /*handle*/) // (in) handle of the download to get the results of
{
	return NULL;
}

//*****************************************************************************
int CNetInterface::GetStatus(
//Returns: Error code returned if handle response received (0 indicating success), else -1
//
//Params:
	const UINT handle) // (in) handle of the download to query the status of
{
	return CInternet::GetStatus(handle);
}

//*****************************************************************************
int CNetInterface::getIndexForName(const WCHAR* pName, const MediaType mediaType) //[default=0]
//Returns: index of name in record list, or -1 if not found
{
	const vector<CNetMedia*>& media = GetCNetMedia();
	for (int nIndex=media.size(); nIndex--; )
	{
		CNetMedia& datum = *(media[nIndex]);
		if (datum.mediaType == mediaType &&
				!WCScmp(pName, (const WCHAR*)datum.HoldNameText))
			return nIndex; //found it
	}

	return -1; //not found
}

//
//Protected methods.
//

//*****************************************************************************
CPostData* CNetInterface::SetCredentials(bool /*bVerify*/)
//Returns: false
{
	return NULL;
}

//*****************************************************************************
CPostData* CNetInterface::SetCredentials(const string& /*strUser*/, const string& /*strKey*/, bool /*bVerify*/)
//Returns: false
{
	return NULL;
}

//*****************************************************************************
bool CNetInterface::VerifyKeyFormat(
//Returns: true if the key is valid, false otherwise
//
//Params:
	const string& /*strKey*/) // (in)  The key to check
{
	return false;
}

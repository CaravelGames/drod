// $Id: NetInterface.cpp 10108 2012-04-22 04:54:24Z mrimer $

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

//Holds the only instance of the net interface for the app.
CNetInterface *g_pTheNet = NULL;

UINT CNetInterface::wCurrentHandle = 0;

const UINT CNetInterface::GLOBAL_CHATROOM = 0;

const char useInternetStr[] = "ConnectToInternet";
const char playerScoresOld[] = "CNetProgressIsOld";

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
//Returns: A handle to use to get the response, or 0 on failure.
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
UINT CNetInterface::DownloadRecords(const string&, const bool)
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
UINT CNetInterface::UploadDemo(const string&, const UINT, const UINT)
//Returns: A handle to use to get the response, or 0 on failure.
{
	return 0;
}

//*****************************************************************************
bool CNetInterface::UploadDemos(const string& /*text*/)
//Returns: A handle to use to get the response, or 0 on failure.
{
	return false;
}

//*****************************************************************************
bool CNetInterface::UploadExploredRooms(const string& /*buffer*/)
//Returns: A handle to use to get the response, or 0 on failure.
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
string CNetInterface::GetChecksum(CDbSavedGame*, const UINT) const
//Returns: a checksum string for the specified data
{
	return string();
}

//*****************************************************************************
bool CNetInterface::VerifyChecksum(CDbSavedGame*, const UINT) const
//Returns: whether the checksum string for the specified data is verified as accurate
{
	return false;
}

//*****************************************************************************
CStretchyBuffer* CNetInterface::GetResults(
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
bool CNetInterface::SetCookies(bool /*bVerify*/)
//Returns: false
{
	return false;
}

//*****************************************************************************
bool CNetInterface::SetCookies(const string& /*strUser*/, const string& /*strKey*/, bool /*bVerify*/)
//Returns: false
{
	return false;
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


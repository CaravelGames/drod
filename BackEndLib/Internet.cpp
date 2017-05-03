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
* Contributor(s):
*
* ***** END LICENSE BLOCK ***** */

#include "Internet.h"
#include "Assert.h"
#include "Files.h"
#include "Ports.h"
#include <json/json.h>

//#define DEBUG_CINTERNET

using namespace std; //for VS6

bool CInternet::bInit = false;
UINT CInternet::nHandles = 1;
CURL* CInternet::pHandle = NULL;
string CInternet::strCookies;
map<UINT, CInternet_Thread_Info*> CInternet::threadInfo;
std::vector<CInternet_Thread_Info*> CInternet::ignoredThreads;
std::vector<CURL*> CInternet::usedHandles;
string CInternet::userAgent;
CIDSet CInternet::canceledHandles;

bool bQuitNow = false;

//
//Public methods.
//

//*****************************************************************************
size_t CInternet_HandleDataStretchyBuffer(
//Handler for getting a URL into a stretchy buffer.  Used as a callback
// in CInternet_HTTP_Get_Worker().
//
//Params:
	void* pPtr,    //(in) Data to be appended to buffer
	size_t size,   //(in) size of each element
	size_t num,    //(in) number of elements
	void* pStream) //(in/out) The CStretchyBuffer to append to
//
//Returns: The size of the appended data
{
	ASSERT(size > 0);
   CInternet_Thread_Info* pInfo = static_cast<CInternet_Thread_Info*>(pStream);
	if (num > 0)
		pInfo->pBuffer->Append(static_cast<const BYTE*>(pPtr), size*num);
   pInfo->bytesComplete = pInfo->pBuffer->Size();

	return size*num;
}

//*****************************************************************************
int CInternet_HTTP_Get_Worker(void* pPtr)
{
	CInternet_Thread_Info* pInfo = (CInternet_Thread_Info *)pPtr;
	const curl_httppost* pPost = pInfo->pPostData ? pInfo->pPostData->PostData() : NULL;
//	curl_httppost* pEnd = CInternet::pFormPostEnd;

	//Send HTTP post.
	static const UINT MAX_ATTEMPTS = 2;
	UINT wAttempts = 0;
	CURLcode code;	//error code
	do
	{
		//Retrieve a handle for transfer.
		CURL* pHandle = curl_easy_init();

		//Set transfer options.
		curl_easy_setopt(pHandle, CURLOPT_NOPROGRESS, 1);
		curl_easy_setopt(pHandle, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(pHandle, CURLOPT_CONNECTTIMEOUT, 5);  //fail if connect takes longer than this (s)
		curl_easy_setopt(pHandle, CURLOPT_LOW_SPEED_LIMIT, 1); //the client must receive at least 1 byte...
		curl_easy_setopt(pHandle, CURLOPT_LOW_SPEED_TIME, 10); //...every 10 seconds, otherwise fail
		curl_easy_setopt(pHandle, CURLOPT_DNS_CACHE_TIMEOUT, -1);	//preserve name resolves forever

		//URL and response.
		curl_easy_setopt(pHandle, CURLOPT_WRITEDATA, pInfo);
		curl_easy_setopt(pHandle, CURLOPT_URL, pInfo->strUrl.c_str());
		curl_easy_setopt(pHandle, CURLOPT_WRITEFUNCTION, CInternet_HandleDataStretchyBuffer);
		curl_easy_setopt(pHandle, CURLOPT_USERAGENT, CInternet::userAgent.c_str());
		curl_easy_setopt(pHandle, CURLOPT_ERRORBUFFER, pInfo->errorBuffer);

		string cookies = CInternet::GetCookies();
		if (!cookies.empty())
			curl_easy_setopt(pHandle, CURLOPT_COOKIE, cookies.c_str());

		if (pPost)
			curl_easy_setopt(pHandle, CURLOPT_HTTPPOST, pPost);

		//Send.
		code = curl_easy_perform(pHandle);
		if (code == CURLE_OK)
			curl_easy_getinfo(pHandle, CURLINFO_RESPONSE_CODE, &pInfo->responseCode);

		//Done.
		curl_easy_cleanup(pHandle);

		//Try sending a few times on timeout.
	} while (code == CURLE_OPERATION_TIMEDOUT && ++wAttempts < MAX_ATTEMPTS && !bQuitNow);
	if (pInfo->pPostData)
		delete pInfo->pPostData;

	pInfo->eRetVal = code;

	return int(code);
}

//*****************************************************************************
void CInternet::Deinit()
//Deinitializes the curl library.
{
	if (!CInternet::bInit) return;

	bQuitNow = true;

	//Wait for active threads to finish.
	UINT i;
	for (i=0; i<CInternet::ignoredThreads.size(); ++i)
	{
		CInternet_Thread_Info* pInfo = CInternet::ignoredThreads[i];
		SDL_WaitThread(pInfo->pThread, NULL); //SDL 1.2.7 bug: must call this after thread procedure completes to avoid memory leaks/hangs
		delete pInfo->pBuffer;
		delete pInfo;
	}
	CInternet::ignoredThreads.clear();

	for (std::map<UINT, CInternet_Thread_Info*>::const_iterator iter = CInternet::threadInfo.begin();
			iter != CInternet::threadInfo.end(); ++iter)
	{
		CInternet_Thread_Info* pInfo = iter->second;
		ASSERT(pInfo->pThread);
		SDL_WaitThread(pInfo->pThread, NULL);
		delete pInfo->pBuffer;
		delete pInfo;
	}
	CInternet::threadInfo.clear();

	for (i=0; i<CInternet::usedHandles.size(); ++i)
		curl_easy_cleanup(CInternet::usedHandles[i]);
	CInternet::usedHandles.clear();

	curl_global_cleanup();

	CInternet::bInit = false;
}

//*****************************************************************************
bool CInternet::Init(
	const char* pUserAgent,
	const bool bSSL) //[default=false]
//Initializes the curl library.
{
	if (CInternet::bInit) return true;

	bQuitNow = false;

	ASSERT(pUserAgent);
	CInternet::userAgent = pUserAgent;

	CInternet::bInit =
		curl_global_init(bSSL ? CURL_GLOBAL_ALL : CURL_GLOBAL_WIN32) == CURLE_OK;

	return CInternet::bInit;
}

//*****************************************************************************
int CInternet::GetStatus(const UINT handle)
//Returns: Error code returned if handle response received (0 indicating success), else -1
{
	ASSERT(handle > 0 && handle < CInternet::nHandles);

	std::map<UINT, CInternet_Thread_Info*>::iterator found = CInternet::threadInfo.find(handle);
	if (found == CInternet::threadInfo.end())
		return -2;	//handle not found, results already retrieved?

	return found->second->eRetVal;
}

//*****************************************************************************
UINT CInternet::GetBytesCompleted(const UINT handle)
{
	ASSERT(handle > 0 && handle < CInternet::nHandles);

	std::map<UINT, CInternet_Thread_Info*>::iterator found = CInternet::threadInfo.find(handle);
   if (found == CInternet::threadInfo.end())
		return 0;

	return (UINT)found->second->bytesComplete;
}


//*****************************************************************************
CStretchyBuffer* CInternet::GetResults(const UINT handle, const bool bSkipClean)
{
	ASSERT(handle > 0 && handle < CInternet::nHandles);

	// Check for any stale results that can be removed.
	if (!bSkipClean) CleanCanceled();

	std::map<UINT, CInternet_Thread_Info*>::iterator found = CInternet::threadInfo.find(handle);
#ifdef DEBUG_CINTERNET
CFiles files;
char buffer[16];
#endif
	if (found == CInternet::threadInfo.end()) {
#ifdef DEBUG_CINTERNET
files.AppendErrorLog("GetResults can't find #");
files.AppendErrorLog(itoa(handle, buffer, 10));
files.AppendErrorLog(NEWLINE);
#endif
		return NULL;
	}

	CInternet_Thread_Info* pInfo = found->second;
	const bool bSuccess = pInfo->eRetVal == CURLE_OK && pInfo->responseCode == 200;
#ifdef DEBUG_CINTERNET
	if (!bSuccess) {
		files.AppendErrorLog("GetResults bSuccess = false! #");
		files.AppendErrorLog(itoa(handle, buffer, 10));
		files.AppendErrorLog("(");
		files.AppendErrorLog(itoa(pInfo->eRetVal, buffer, 10));
		files.AppendErrorLog(", ");
		files.AppendErrorLog(itoa(pInfo->responseCode, buffer, 10));
		files.AppendErrorLog(")");
		files.AppendErrorLog(NEWLINE);
	}
#endif
	CStretchyBuffer* pBuffer = pInfo->pBuffer;
	int status;
	ASSERT(pInfo->pThread);
	SDL_WaitThread(pInfo->pThread, &status);	//SDL 1.2.7 bug: must call this after thread procedure completes to avoid memory leaks/hangs
	delete pInfo;
	CInternet::threadInfo.erase(found);

	if (bSuccess) {
#ifdef DEBUG_CINTERNET
	files.AppendErrorLog("GetResults #");
	files.AppendErrorLog(itoa(handle, buffer, 10));
	files.AppendErrorLog(NEWLINE);
#endif

/*
	const char *pBufCopy = (char*)pBuffer->GetCopy();
	string str(pBufCopy ? pBufCopy : "");
	delete pBufCopy;
	CFiles f;
	f.AppendErrorLog("CINTERNET GETRESULTS\n");
	f.AppendErrorLog(str.c_str());
*/
	
		return pBuffer;
	}

	//Error occurred -- return nothing.
	delete pBuffer;
	return NULL;
}

//*****************************************************************************
void CInternet::OutputError(const UINT handle)
//Output error if some problem occurred in CInternet_HTTP_Get_Worker.
{
	ASSERT(handle > 0 && handle < CInternet::nHandles);

	std::map<UINT, CInternet_Thread_Info*>::iterator found = CInternet::threadInfo.find(handle);
	if (found == CInternet::threadInfo.end()) return;

	if (found->second->eRetVal > 0)
	{
		char errorStr[256];
		_snprintf(errorStr, 256, "Internet warning %d: %s\n", found->second->eRetVal, found->second->errorBuffer);
		CFiles Files;
		Files.AppendErrorLog((char *)errorStr);
	}
}

//*****************************************************************************
void CInternet::AddCookie(
//Adds a cookie to all HTTP requests
//
//Params:
	const string& strName,     //(in) Cookie name
	const string& strValue)    //(in) Cookie value
{
	if (CInternet::strCookies != "") CInternet::strCookies += "; ";
	CInternet::strCookies += strName;
	CInternet::strCookies += "=";
	CInternet::strCookies += strValue;
}

//*****************************************************************************
void CInternet::CleanCanceled()
// Clean up any canceled requests that have finished.
{
	CIDSet toRemove;
	CIDSet::iterator i;
#ifdef DEBUG_CINTERNET
	CFiles files;
	char buffer[16];
#endif
	for (i = CInternet::canceledHandles.begin(); i != CInternet::canceledHandles.end(); i++) {
		if (GetStatus(*i) >= 0) {
			CStretchyBuffer* pBuffer = GetResults(*i, true);
			if (pBuffer) {
#ifdef DEBUG_CINTERNET
	files.AppendErrorLog("Removing canceled request #");
	files.AppendErrorLog(itoa(*i, buffer, 10));
	files.AppendErrorLog(NEWLINE);
#endif
				toRemove += *i;
				delete pBuffer;
			} else {
#ifdef DEBUG_CINTERNET
	files.AppendErrorLog("Tried to remove request #");
	files.AppendErrorLog(itoa(*i, buffer, 10));
	files.AppendErrorLog(", but it returned a null buffer");
	files.AppendErrorLog(NEWLINE);
#endif
			}
		}
	}
	for (i = toRemove.begin(); i != toRemove.end(); i++) {
		CInternet::canceledHandles -= *i;
	}
}

//*****************************************************************************
bool CInternet::CancelRequest(
//Cancel a HTTP request.  We don't actually kill any threads, we wait for them to finish and then discard.
//
//Params:
	const UINT handle			//(in) The handle of the request to cancel
	)
//Returns:
//True if the handle exists and is canceled, false otherwise
{
	ASSERT(handle > 0 && handle < CInternet::nHandles);

	if (!CInternet::bInit) return false;

	std::map<UINT, CInternet_Thread_Info*>::iterator found = CInternet::threadInfo.find(handle);
	if (found != CInternet::threadInfo.end()) {
		CInternet::canceledHandles += handle;
		return true;
	} else {
		// Can't find the handle
		return false;
	}
}

//*****************************************************************************
bool CInternet::HttpGet(
//Gets a file via HTTP
//
//Params:
	const string& strUrl,       //(in) URL to retrieve
	UINT* handle,               //(in) if NULL, response doesn't matter and won't be checked [default=NULL]
	CPostData* pPostData)       //(in) if NULL, there is no POST data [default=NULL]
//Returns:
//True if the operation has been initialized.
{
	if (!CInternet::bInit) return false;

	CInternet_Thread_Info* pInfo = new CInternet_Thread_Info;
	pInfo->pBuffer = new CStretchyBuffer;
	pInfo->pBuffer->Alloc(1000);
	pInfo->strUrl = strUrl;
	pInfo->bIgnoreResponse = (bool)(!handle);
	pInfo->pPostData = pPostData;

	SDL_Thread* pThread = SDL_CreateThread(CInternet_HTTP_Get_Worker, "httpget", pInfo);
	if (!pThread) {delete pInfo->pBuffer; delete pInfo; return false;} //couldn't create thread
	pInfo->pThread = pThread;

	if (handle)
	{
		if (CInternet::threadInfo.size() > 0) {
			int dummy = 5;
		}
		*handle = CInternet::nHandles++;
		CInternet::threadInfo[*handle] = pInfo;
#ifdef DEBUG_CINTERNET
	CFiles files;
	char buffer[16];
	files.AppendErrorLog("HttpGet #");
	files.AppendErrorLog(itoa(*handle, buffer, 10));
	files.AppendErrorLog(strUrl.c_str());
	files.AppendErrorLog(NEWLINE);
#endif
	} else
		CInternet::ignoredThreads.push_back(pInfo);

	return true;
}


//*****************************************************************************
bool CInternet::HttpGet(
//Gets a file via HTTP
//
//Params:
	const string& strUrl,       //(in) URL to retrieve
	Json::Value& json,          //(in) if NULL, there is no POST data [default=NULL]
	UINT* handle)               //(in) if NULL, response doesn't matter and won't be checked [default=NULL]
//Returns:
//True if the operation has been initialized.
{
	// We wrap up the json data in a post var named "json".
	Json::StyledWriter writer;
	CPostData* pPost = new CPostData();
	pPost->Add("json", writer.write(json));
	return CInternet::HttpGet(strUrl, handle, pPost);
}

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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * John Wm. Wicks (j_wicks)
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#if defined(STEAMBUILD)
#  include <steam_api.h>
#  include <stdlib.h>

static bool OpenSteamOverlayBrowser(const char* pszURI)
{
	if (SteamUtils() && SteamUtils()->IsSteamInBigPictureMode() && SteamUtils()->IsOverlayEnabled())
	{
		SteamFriends()->ActivateGameOverlayToWebPage(pszURI);
		return true;
	}
	return false;
}

#endif

#if defined(WIN32)
#  include <windows.h> //Should be first include.

#  include "Files.h"
#  include <stdio.h>

extern HWND m_hwndWin;
HWND m_hBrwsWin=NULL;

//*****************************************************************************
void Error()
//Shows error message if file can't be opened.
{
	 MessageBeep(0);

	 LPVOID lpMsgBuf;
	 FormatMessage(
		  FORMAT_MESSAGE_ALLOCATE_BUFFER |
		  FORMAT_MESSAGE_FROM_SYSTEM |
		  FORMAT_MESSAGE_IGNORE_INSERTS,
		  NULL,
		  GetLastError(),
		  0, // Default language
		  (LPTSTR) &lpMsgBuf,
		  0,
		  NULL
		  );
	 OutputDebugStringA((char *)lpMsgBuf);
	 CFiles Files;
	 Files.AppendErrorLog("Browser.cpp -- Error: ");
	 Files.AppendErrorLog((char *)lpMsgBuf);
	 // Free the buffer.
	 LocalFree( lpMsgBuf );
}

//*****************************************************************************
static bool OpenSystemBrowser(
//Uses the system default browser to show an external URL.
//
//Returns: true if successful
//
//Params:
	const char *pszURL)  //(in) The full URI, e.g. "http://www.drod.net/"
{
	WSTRING wstrURI = UTF8ToUnicode(pszURL);

	static const WCHAR wszOpen[] = {'o', 'p', 'e', 'n', 0 };
	static const char pszOpen[] = {'o', 'p', 'e', 'n', 0 };

	if (m_hBrwsWin && (!IsWindow(m_hBrwsWin))) m_hBrwsWin = NULL;

	if (CFiles::WindowsCanBrowseUnicode())
	{
		SHELLEXECUTEINFOW myShExInfo;
		//Open URL in browser window.
		myShExInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		//myShExInfo.fMask = (SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS);
		myShExInfo.hwnd = m_hBrwsWin;
		myShExInfo.fMask = NULL;
		myShExInfo.lpParameters = NULL;
		myShExInfo.lpDirectory = NULL;
		myShExInfo.nShow = SW_SHOWNORMAL;
		myShExInfo.hInstApp = NULL;
		myShExInfo.lpVerb = wszOpen;
		myShExInfo.lpFile = wstrURI.c_str();
		if(!ShellExecuteExW(&myShExInfo))
			Error();
      if( (int)myShExInfo.hInstApp > 32 ) m_hBrwsWin = myShExInfo.hwnd;
		else return false;
	}
	else
	{
		SHELLEXECUTEINFOA myShExInfo;
		//Open file in browser window.
		myShExInfo.cbSize = sizeof(SHELLEXECUTEINFO);
		//myShExInfo.fMask = (SEE_MASK_FLAG_NO_UI | SEE_MASK_NOCLOSEPROCESS);
		myShExInfo.hwnd = m_hBrwsWin;
		myShExInfo.fMask = NULL;
		myShExInfo.lpParameters = NULL;
		myShExInfo.lpDirectory = NULL;
		myShExInfo.nShow = SW_SHOWNORMAL;
		myShExInfo.hInstApp = NULL;
		myShExInfo.lpVerb = pszOpen;
		myShExInfo.lpFile = pszURL;
		if(!ShellExecuteExA(&myShExInfo))
			Error();
		if( (int)myShExInfo.hInstApp > 32 ) m_hBrwsWin = myShExInfo.hwnd;
		else return false;
	}

	// If we were successful then save the HWND for later use
	return true;
}

#elif defined(__linux__) || defined(__FreeBSD__)
#  include "Files.h"
#  include <stdlib.h>

static bool OpenSystemBrowser(
//Uses the system default browser to show an external URL.
//
//Params:
	const char *pszURI)  //(in) The full URI, e.g. "http://www.drod.net/"
{
	string cmd;
#ifdef STEAMBUILD
	cmd += "env STEAM_RUNTIME=0 ";
	if (getenv("SYSTEM_PATH"))
		cmd += "LD_LIBRARY_PATH=\"$SYSTEM_LD_LIBRARY_PATH\" PATH=\"$SYSTEM_PATH\" ";
#endif
	cmd += "xdg-open '"; // xdg-open handles all the nasty details for us
	cmd.reserve(cmd.length() + strlen(pszURI) + 10);
	for (int i = 0; pszURI[i]; ++i)  // escape single quotes
	{
		if (pszURI[i] != '\'')
			cmd += pszURI[i];
		else
			cmd += "'\"'\"'";
	}
	cmd += "' &";
	return system(cmd.c_str()) != -1;  //let's hope for the best =P
}

#elif defined(__APPLE__)
#include <string>
#include <cstdlib>

static bool OpenSystemBrowser(
//Uses the system default browser to show an external URL.
//
//Params:
	const char *pszURI)  //(in) The full URI, e.g. "http://www.drod.net/"
//
//Returns: true if successful
{
	std::string cmd = "open '";
	cmd.reserve(cmd.length() + strlen(pszURI) + 10);
	for (int i = 0; pszURI[i]; ++i)  // escape single quotes
	{
		if (pszURI[i] != '\'')
			cmd += pszURI[i];
		else
			cmd += "'\"'\"'";
	}
	cmd += "' &";
	system(cmd.c_str());
	return true;
}

#else
#  error "missing code to launch external browser for this platform"
#endif

#include "Assert.h"

bool OpenExtBrowser (const char *pszURI)
{
	ASSERT(pszURI);

#ifdef STEAMBUILD
	if (OpenSteamOverlayBrowser(pszURI))
		return true;
#endif

	return OpenSystemBrowser(pszURI);
}

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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996,
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Matt Schikore (Schik), Gerry JJ
 *
 * ***** END LICENSE BLOCK ***** */

#include "Assert.h"
#include "Clipboard.h"
#include "SDL_clipboard.h"

//******************************************************************************
bool CClipboard::SetString(
//Copies the given string into the system clipboard
//Returns: true on success, false otherwise.
//
//Params:
	const string& sClip)  //(in)
{
	return !SDL_SetClipboardText(sClip.c_str());
}

//******************************************************************************
bool CClipboard::GetString(
//Copies the system clipboard string into sClip
//Returns: true on success, false otherwise.
//
//Params:
	string& sClip)  //(out)
{
	char* clip = SDL_GetClipboardText();
	if (clip)
	{
		sClip = clip;
		SDL_free(clip);
	}
	return clip != 0;
}

//******************************************************************************
bool CClipboard::GetString(
//Copies the system clipboard string into sClip
//Returns: true on success, false otherwise.
//
//Params:
	WSTRING& sClip) //(out)
{
	string u8clip;
	bool bSuccess;
	if ((bSuccess = GetString(u8clip)))
		UTF8ToUnicode(u8clip.c_str(), u8clip.length(), sClip);
	return bSuccess;
}

//******************************************************************************
bool CClipboard::SetString(
//Copies the given string into the system clipboard
//Returns: true on success, false otherwise.
//
//Params:
	const WSTRING& sClip)  //(in)
{
	bool bSuccess = false;
	BYTE *pbOutStr = NULL;
	if (to_utf8(sClip.c_str(), pbOutStr))
		bSuccess = SetString((const char*)pbOutStr);
	delete[] pbOutStr;
	return bSuccess;
}

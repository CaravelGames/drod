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
 *
 * ***** END LICENSE BLOCK ***** */

#include "Assert.h"

#ifdef _DEBUG

//Assertion checking module.
#ifdef WIN32
#  include <windows.h> //Should be first include.
#endif
#include "Types.h"

#include "Files.h"
#include <assert.h>
#include <stdio.h>

bool bLogErrors = true;

//**************************************************************************************
#ifndef WIN32
void OutputDebugStringA(const char *pszMessage)
// Outputs a string to stderr (Non-WIN32 only)
{
	const string gameName = UnicodeToAscii(CFiles::wGameName);
	fprintf(stderr, "%s: %s", gameName.c_str(), pszMessage);
}
#endif

//**************************************************************************************
#ifdef USE_LOGCONTEXT
CLogContext::CLogContext(const char *pszDesc) : strDesc(pszDesc) {CFiles::PushLogContext(pszDesc);}
CLogContext::~CLogContext() {CFiles::PopLogContext(strDesc.c_str());}
#endif

//**************************************************************************************
void SetLogErrors(const bool bVal)
//Enable/disable error log output
{
	bLogErrors = bVal;
}

//**************************************************************************************
void LogErr(const char *pszMessage)
//Logs an error to the error log.
{
	assert(pszMessage);

#ifdef WIN32
	MessageBeep(0);
#endif
	OutputDebugStringA(pszMessage);
	CFiles Files;
	Files.AppendErrorLog(pszMessage);
}

//**************************************************************************************
void AssertErr(const char *pszFile, int nLine, const char *pszDesc)
//Logs an assertion error to the error log.
{
	if (bLogErrors)
	{
		assert(pszFile);
		assert(nLine > 0);
		assert(pszDesc);
		char szMessage[500];
		sprintf(szMessage, "Assertion error in line %d of %s: \"%s\"" NEWLINE, nLine, pszFile, pszDesc);
		LogErr(szMessage);
#ifndef RELEASE_WITH_DEBUG_INFO //Don't debugbreak() in a release build, because it will just crash.
#	ifdef WIN32
		DebugBreak();
#	endif
#endif
	}
}

//**************************************************************************************
void DebugPrint(const char *pszMessage)
//Send message to debug output.
{
	assert(pszMessage);
	char szChunk[256];
	szChunk[255] = '\0'; //Last char will always be term zero.
	const char *pszSeek = pszMessage;
	char *psWrite = szChunk;
	UINT wWriteChars = 0;

	//Copy every 255 chars to temp buffer and send in chunks.  Each iteration
	//processes one char from message.
	while (*pszSeek != '\0')
	{
		*(psWrite++) = *(pszSeek++);
		if ((++wWriteChars) == 255) //Chunk is full and ready to send.
		{
			OutputDebugStringA(szChunk);

			//Reset write position.
			psWrite = szChunk;
			wWriteChars = 0;
		}
	}

	//Write whatever is left over in the chunk.
	if (wWriteChars)
	{
		*psWrite = '\0';
		OutputDebugStringA(szChunk);
	}
}

#endif //...#ifdef _DEBUG

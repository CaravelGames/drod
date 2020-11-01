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
 *
 * ***** END LICENSE BLOCK ***** */

//OptionList.cpp.
//Implementation of COptionList.

#include "OptionList.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Ports.h>

#include <stdio.h>
#include <string.h>
#include <ctype.h>

const WCHAR isNotValid[] = {{'i'},{'s'},{' '},{'n'},{'o'},{'t'},{' '},{'v'},{'a'},{'l'},{'i'},{'d'},{0}};
static const WSTRING wstrIsNotValid(isNotValid);
const WCHAR wasSpecifiedMoreThanOnce[] = {
	{'w'},{'a'},{'s'},{' '},{'s'},{'p'},{'e'},{'c'},{'i'},
	{'f'},{'i'},{'e'},{'d'},{' '},{'m'},{'o'},{'r'},{'e'},{' '},
	{'t'},{'h'},{'a'},{'n'},{' '},{'o'},{'n'},{'c'},{'e'},{0}};
static const WSTRING wstrWasSpecifiedMoreThanOnce(wasSpecifiedMoreThanOnce);

static void OptionError (
//Print an error message about wrong option usage.
//
//Params:
	const WSTRING &wstrErr, //(in)   The error.
	const WCHAR   *pwczOpt) //(in)   The option this applies to.
{
	ASSERT(pwczOpt);

#ifdef HAS_UNICODE
	wprintf(L"The \"-%s\" option %s.\n", pwczOpt, wstrErr.c_str());
#else
	WSTRING wstrOpt = pwczOpt;
	const string opt = UnicodeToUTF8(wstrOpt);
	const string err = UnicodeToUTF8(wstrErr);
	printf("The \"-%s\" option %s.\n", opt.c_str(), err.c_str());
#endif
}

//
//Public methods.
//

//******************************************************************************
bool COptionList::Add(
//Add option to list.
//
//Params:
	const WCHAR *pszOption,    //(in)   Option char(s) w/o preceding "-".
	const WCHAR *pszAttributes)   //(in)   Attributes associated with option.  ""
								//    is default.
//
//Returns:
//True if successfully added, false if not.
{
	ASSERT(pszOption);
	ASSERT(pszAttributes);

	//Check for valid lengths.
	if (WCSlen(pszOption) > MAXLEN_OPTION ||
		WCSlen(pszAttributes) > MAXLEN_ATTRIBUTES)
	{
		OptionError(wstrIsNotValid, pszOption);
		return false;
	}

	//Check for duplicate attribute.
	if (Exists(pszOption))
	{
		OptionError(wstrWasSpecifiedMoreThanOnce, pszOption);
		return false;
	}

	//Create new option node.
	OPTIONNODE *pNew = new OPTIONNODE;
	pNew->pNext = NULL;
	WCScpy(pNew->szAttributes, pszAttributes);
	WCScpy(pNew->szOption, pszOption);

	//Add to end of list.
	ASSERT(this->pLast->pNext == NULL);
	this->pLast->pNext = pNew;
	this->pLast = pNew;
	++(this->wSize);

	return true;
}

//******************************************************************************
bool COptionList::AreOptionsValid(
//Do options currently loaded into object all match a list of valid options?
//
//Params:
	const WCHAR *pszValidOptions) //(in)   Lower-case comma-delimited options
									//    to check against.
//
//Returns:
//True if they do, false if not.
const
{
	ASSERT(pszValidOptions);

	//If no object options, then there is nothing to validate.
	if (this->wSize == 0) return true;

	//Do a cheap handling of the case where there are no valid options.
	if (pszValidOptions[0] == '\0' && this->wSize > 0)
	{
		//Just say the first option is wrong.
		ASSERT(this->Sentry.pNext->szOption);
		OptionError(wstrIsNotValid, this->Sentry.pNext->szOption);
		return false;
	}

	//Each iteration checks one object option against valid options.
	OPTIONNODE *pSeek = this->Sentry.pNext;
	while (pSeek)
	{
		const WCHAR *pszCompare = pszValidOptions;
		const WCHAR *pszSeekOption = pSeek->szOption;
		while (pszCompare != '\0')
		{
			if (*pszCompare != *pszSeekOption) //Options do not match.
			{
				//Skip ahead to next valid option.
				while (*pszCompare != ',' && *pszCompare != '\0') ++pszCompare;
				if (*pszCompare == '\0') //No more valid options.
				{
					OptionError(wstrIsNotValid, pSeek->szOption);
					return false;
				}
				++pszCompare;
				pszSeekOption = pSeek->szOption;
			}
			else //Options match so far.
			{
				++pszCompare;
				++pszSeekOption;
				if (*pszSeekOption == '\0')
				{
					//I'm at end of object option.  If I'm also at end of valid
					//option, then this is a match.
					if (*pszCompare == '\0' || *pszCompare == ',')
						break; //Found a matching valid option, so stop checking.
				}
			}
		}

		//Get next object option.
		pSeek = pSeek->pNext;
	}
	return true;
}

//******************************************************************************
void COptionList::Clear()
//Frees resources and resets members.
{
	OPTIONNODE *pSeek = this->Sentry.pNext, *pDelete;
	while (pSeek)
	{
		pDelete = pSeek;
		pSeek = pSeek->pNext;
		delete pDelete;
	}
	this->Sentry.pNext = this->pLast = NULL;
	this->wSize = 0;
}

//******************************************************************************
bool COptionList::Exists(
//Does an option exist?
//
//Params:
	const WCHAR *pszOption) //(in)   Option to look for w/o preceding "-".
							//    Comparisons will be case insensitive.
//
//Returns:
//True if it does, false if not.
const
{
	ASSERT(pszOption);
	return (Get(pszOption) != NULL);
}

//******************************************************************************
OPTIONNODE *COptionList::Get(
//Get an option.
//
//Params:
	const WCHAR *pszOption) //(in)   Option to look for w/o preceding "-".
							//    Comparisons will be case insensitive.
//
//Returns:
//Pointer to option or NULL if not match.
const
{
	ASSERT(pszOption);

	OPTIONNODE *pSeek = this->Sentry.pNext;
	while (pSeek)
	{
		if (WCSicmp(pSeek->szOption, pszOption)==0) return pSeek; //Found it.
		pSeek = pSeek->pNext;
	}
	return NULL; //Didn't find it.
}

//******************************************************************************
bool COptionList::Set(
//Loads the class with options from a command line.
//
//Params:
	int argc,      //(in)   Command line arg count.
	WCHAR *argv[]) //(in)   Command line arguments passed to main().
//
//Returns:
//True if options were in a parsable format, false if not.
{
	//The accepted format of DRODUtil commands is:
	//DRODUTIL Command [Options] [Params]
	//An option is distinguished from a param by being preceded with a "-".

	//Each iteration parses one argument.
	for (int nArgI = 2; nArgI < argc; ++nArgI)
	{
		//If first param found, then no more options are expected.
		if (argv[nArgI][0] != '-') return true;

		//Copy option into buffer.
		WCHAR szOption[MAXLEN_OPTION + 1];
		WCHAR *pWrite = &(szOption[0]);
		WCHAR *pSeek = argv[nArgI] + 1, *pStop = pSeek + MAXLEN_OPTION;
		while (*pSeek != ':' && *pSeek != '\0' && pSeek < pStop)
			*(pWrite++) = towlower(*(pSeek++));
		if (*pSeek != ':' && *pSeek != '\0') //Option is too long.
		{
			OptionError(wstrIsNotValid, argv[nArgI]);
			return false;
		}
		pWCv(pWrite) = '\0';

		//If found a colon then copy attributes into a separate buffer.
		WCHAR szAttributes[MAXLEN_ATTRIBUTES + 1];
		if (*pSeek == ':')
		{
			WCHAR *pWrite = &(szAttributes[0]);
			++pSeek;
			pStop = pSeek + MAXLEN_ATTRIBUTES;
			while (*pSeek != '\0' && pSeek < pStop)
				*(pWrite++) = *(pSeek++); //Note: not converted to lower because it could be a filename or path.
			if (*pSeek != '\0') //Option is too long.
			{
				OptionError(wstrIsNotValid, szOption);
				return false;
			}
			pWCv(pWrite) = '\0';
		}
		else
			//No attributes with this option.
			WCv(szAttributes[0]) = '\0';

		//Add option to list.
		if (!Add(szOption, szAttributes)) return false;
	}

	//Parsed all arguments into options and did not encounter parameters.
	return true;
}

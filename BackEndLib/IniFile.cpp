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
 * Matt Schikore (Schik)
 *
 * ***** END LICENSE BLOCK ***** */

// Apple and Linux keep isspace() in ctype.h.
#if defined __APPLE__ || defined __linux__ || defined __FreeBSD__
#  include <ctype.h>
#endif

#include "IniFile.h"
#include "Assert.h"
#include "Files.h"
#include "Ports.h"

#include <cstdio>

using namespace std;

//******************************************************************************
void parseEntries(string& str, list<string>& entries)
//Parses a string into a list of semicolon-separated entries
{
	if (str.empty())
		return;

	const char *pszStart = str.c_str();
	for (char *pszSeek = &(*str.begin()); *pszSeek != '\0'; ++pszSeek)
	{
		if (*pszSeek == ';') //Found delineator
		{
			//Add to list.
			*pszSeek = '\0'; //Null-terminate the string.
			entries.push_back(pszStart);
			pszStart = pszSeek + 1;
		}
	}

	//Add remainder.
	entries.push_back(pszStart);
}

//******************************************************************************
CIniSection::CIniSection(
//Constructor.
//
//Params:
	const char *pszSetName)  //(in)
{
	ASSERT(pszSetName != NULL);
	this->strName = pszSetName;
}

//******************************************************************************
CIniSection::CIniSection()
//Default constructor.
{
}

//******************************************************************************
bool CIniSection::GetString(
//Gets a string from this section
//
//Params:
	const char* pszKey,      //(in)
	string& strBuffer)    //(out)
//
//Returns:
//True if the string exists, false if not
{
	strBuffer = "";
	map<string,IniEntry>::iterator iter = this->entries.find(pszKey);
	if (iter == this->entries.end())
		return false;

	bool bFirst = true;
	for (IniEntry::const_iterator i=iter->second.begin(); i!=iter->second.end(); ++i)
	{
		//Concatenate multiple entries, delineate with a semicolon.
		if (bFirst)
			bFirst = false;
		else
			strBuffer += ";";

		//Strip excess line-ending-characters and other spaces
		//(needed to parse dos-style drod.ini in linux)
		string str = *i;
		int buflen = str.length();
		if (buflen) {
			do
			{--buflen;}
			while (buflen && isspace(str[buflen]));
			str.resize(buflen + 1);
		}
		strBuffer += str;
	}
	return true;
}

//******************************************************************************
bool CIniSection::GetString(
//OUT: list of delineated string segments
	const char* pszKey, list<string>& strBuffer)
{
	strBuffer.clear();
	string str;
	if (!GetString(pszKey, str))
		return false;

	parseEntries(str, strBuffer);

	return true;
}

//******************************************************************************
bool CIniSection::GetString(
//OUT: list of delineated string segments
	const char* pszKey, list<WSTRING>& wstrBuffer)
{
	wstrBuffer.clear();
	list<string> strs;
	if (!GetString(pszKey, strs))
		return false;

	WSTRING wstr;
	for (list<string>::const_iterator iStr = strs.begin(); iStr != strs.end(); ++iStr)
	{
		AsciiToUnicode(iStr->c_str(), wstr);
		wstrBuffer.push_back(wstr);
	}
	return true;
}

//******************************************************************************
void CIniSection::WriteString(
//Writes a string to this section and key
//
//Params:
  const char *pszKey,   //(in)
  const char *pszValue, //(in)
  const bool bOverwrite)//if true, replace all entries for this key
{
	IniEntry& texts = this->entries[(string)pszKey];
	if (bOverwrite)
		texts.clear();
	texts.push_back((string)pszValue);
}

//****************************
void CIniSection::WriteString(
	const char *pszKey, const list<WSTRING>& wstrValue, const bool bOverwrite)
{
	IniEntry& texts = this->entries[(string)pszKey];
	if (bOverwrite)
		texts.clear();
	for (list<WSTRING>::const_iterator iStr = wstrValue.begin();
			iStr != wstrValue.end(); ++iStr)
	{
		texts.push_back(UnicodeToUTF8(iStr->c_str()));
	}
}

//******************************************************************************
void CIniSection::Save(
//Saves the section to the specified (already opened) file
//
//Params:
  FILE* const pFile)       //(in)
{
	ASSERT(pFile);
	fprintf(pFile, "[%s]\n", strName.c_str());

	for (map<string,IniEntry>::iterator entry = this->entries.begin(); entry != this->entries.end(); ++entry)
		for (IniEntry::const_iterator i=entry->second.begin(); i!=entry->second.end(); ++i)
			fprintf(pFile, "%s=%s\n", entry->first.c_str(), i->c_str());
	fprintf(pFile, "\n");
}

//******************************************************************************
CIniFile::CIniFile()
//Constructor.
	: bLoaded(false)
	, bDirty(false)
{
}

//******************************************************************************
CIniFile::~CIniFile()
//Destructor.
{
	if (this->bLoaded && this->bDirty)
	{
		FILE* pFile = CFiles::Open(wstrFilename.c_str(), "w");
		if (pFile != NULL)
		{
			for (map<string, CIniSection>::iterator i = this->sections.begin();
					i != this->sections.end();	++i)
			{
				i->second.Save(pFile);
			}
			fclose(pFile);
		}
	}
}

//******************************************************************************
bool CIniFile::DeleteEntry(const char *pszSection, const char *pszKey, const char *pszValue)
//Deletes entry 'value', if it exists, from section-key (not case sensitive).
//If value is NULL, then all values for this key are removed.
//If key is NULL, then all entries in this section are removed.
//If section is NULL, then all entries are removed.
//
//Returns: whether value/key/section was found to delete; true if everything is deleted
{
	if (!pszSection)
	{
		this->sections.clear();
		return true;
	}

	map<string, CIniSection>::iterator sec = this->sections.find(pszSection);
	if (sec == this->sections.end())
		return false; //section doesn't exist

	map<string, IniEntry>& section = sec->second.entries;
	if (!pszKey)
	{
		section.clear();
		return true;
	}

	map<string, IniEntry>::iterator entryiter = section.find(pszKey);
	if (entryiter == section.end())
		return false; //key entry doesn't exist

	IniEntry& entry = entryiter->second;
	if (!pszValue)
	{
		entry.clear();
		return true;
	}

	//Remove this value.
	bool bFound = false;
	for (IniEntry::iterator iStr = entry.begin(); iStr != entry.end(); )
	{
		if (!_stricmp(iStr->c_str(), pszValue))
		{
			bFound = true;
			iStr = entry.erase(iStr);
		}
		else
			++iStr;
	}

	//If no values remain, remove this key entry also.
	if (bFound && entry.empty())
		section.erase(entryiter);

	return bFound;
}

//******************************************************************************
bool CIniFile::Load(
//Loads the INI file
//
//Params:
  const WCHAR *wszSetFilename) //(in)
{
	this->bLoaded = true;
	this->bDirty = false;
	wstrFilename = wszSetFilename;

	CStretchyBuffer text;
	if (!CFiles::ReadFileIntoBuffer(wstrFilename.c_str(), text))
		return false;

	const bool bRes = LoadText((char*)(BYTE*)text);
	this->bDirty = false; //reset again here
	return bRes;
}

//******************************************************************************
bool CIniFile::LoadText(
//Parses text and adds entries to existing data structures.
//Does not add duplicate values to entries for an existing key.
	string strText,            //(in) text to add
	const bool bOverwrite,     //if true, replace key entries with this text [default=false]
	const bool bAddDuplicates) //if true, multiple identical values may exist in a key.
	                           //Otherwise, matches are not reinserted. [default=true]
{
	string curSection;
	char *pStrText = (char*)(strText.c_str());

	//Parse a line of text at a time.
	for (char *pLine = strtok(pStrText, "\n"); pLine != NULL; pLine = strtok(NULL, "\n"))
	{
		if (pLine[0] == '[')
		{
			string sectionName(&pLine[1]);
			string::size_type end = sectionName.find_first_of(']');
			if (end == string::npos)
			{
				// Can't find the closing ] in a section name
				return false;
			}

			curSection = sectionName.substr(0, end);
		} else {
			string line(pLine);
			bool bIsWhitespace = true;
			for (string::iterator c = line.begin(); c != line.end(); c++)
			{
				if (!isspace(*c))
				{
					bIsWhitespace = false;
					break;
				}
			}
			if (bIsWhitespace) continue;

			// It's a key/value pair, not a section name
			string::size_type equalsPos = line.find_first_of('=');
			if (equalsPos == string::npos)
			{
				// No = in the line
				return false;
			}

			string key = line.substr(0, equalsPos);
			string value = line.substr(equalsPos+1);
			string::iterator last = value.end();
			last--;
			if (*last == '\r')
				value.erase(last);

			// Now add the pair to the correct section.
			// See if the section already exists.
			map<string, CIniSection>::iterator sec;
			sec = sections.find(curSection);
			if (sec == sections.end())
			{
				// Section doesn't exist - make a new one
				CIniSection newSection(curSection.c_str());
				sec=sections.insert(make_pair(curSection, newSection)).first;
			}

			//Section exists.  Get list of values for this key.
			list<string> existingValues;
			if (!sec->second.GetString(key.c_str(), existingValues) || bAddDuplicates)
			{
				//Key either doesn't exist or duplicate entries may be added.  Add key and values.
				sec->second.WriteString(key.c_str(), value.c_str(), bOverwrite);
				this->bDirty = true;
			} else {
				//Key exists.  Don't re-add duplicates values.
				list<string> newValues;
				parseEntries(value, newValues);
				bool bFirst = true;
				for (list<string>::const_iterator newVal=newValues.begin();
						newVal!=newValues.end(); ++newVal)
				{
					//Check each new value to be added for a matching existing entry.
					bool bValExists = false;
					for (list<string>::const_iterator val=existingValues.begin();
							val!=existingValues.end(); ++val)
						if (!val->compare(*newVal))
						{
							bValExists = true;
							break; //value exists for this key -- don't re-add.
						}
					if (bValExists)
						continue;
					sec->second.WriteString(key.c_str(), newVal->c_str(), bOverwrite && bFirst);
					bFirst = false; //only overwrite when first entry of this list is inserted.  The rest are appended to this.
					this->bDirty = true;
				}
			}
		}
	}

	return true;
}

//**********************************************
bool CIniFile::LoadText(const WSTRING& wstrText, const bool bOverwrite, const bool bAddDuplicates)
{
	return LoadText(UnicodeToUTF8(wstrText), bOverwrite, bAddDuplicates);
}

//******************************************************************************
bool CIniFile::GetString(
//Gets the value of the specified section/key
//
//Params:
	const char* pszSection,  //(in)
	const char* pszKey,      //(in)
	string& strBuffer)       //(out)
//
//Returns:
//True if the section/key exists, false if not
{
	map<string, CIniSection>::iterator sec = this->sections.find(pszSection);
	if (sec == this->sections.end())
	{
		// section does not exist
		strBuffer = "";
		return false;
	}

	return sec->second.GetString(pszKey, strBuffer);
}

//**************************
bool CIniFile::GetString(
	const char* pszSection, const char* pszKey, list<string>& strBuffer)
{
	map<string, CIniSection>::iterator sec = this->sections.find(pszSection);
	if (sec == this->sections.end())
	{
		// section does not exist
		strBuffer.clear();
		return false;
	}

	return sec->second.GetString(pszKey, strBuffer);
}

//**************************
bool CIniFile::GetString(
	const char* pszSection, const char* pszKey, list<WSTRING>& wstrBuffer)
{
	map<string, CIniSection>::iterator sec = this->sections.find(pszSection);
	if (sec == this->sections.end())
	{
		// section does not exist
		wstrBuffer.clear();
		return false;
	}

	return sec->second.GetString(pszKey, wstrBuffer);
}

//******************************************************************************
void CIniFile::WriteString(
//Writes a key/value to a section
//
//Params:
  const char *pszSection,  //(in)
  const char *pszKey,      //(in)
  const char *pszValue,    //(in)
  const bool bOverwrite)   //(in) if true, replace all strings under this key [default=true]
{
	ASSERT(this->bLoaded);
	this->bDirty = true;

	map<string, CIniSection>::iterator sec = sections.find(pszSection);
	if (sec == sections.end())
	{
		// Section does not exist yet -- create it.
		CIniSection newSection(pszSection);
		sec = this->sections.insert(make_pair(pszSection, newSection)).first;
	}
	sec->second.WriteString(pszKey, pszValue, bOverwrite);
}

//******************************************************************************
void CIniFile::WriteString(const char *pszSection, const char *pszKey,
	const list<WSTRING>& wstrBuffer, const bool bOverwrite)
{
	ASSERT(this->bLoaded);
	this->bDirty = true;
	map<string, CIniSection>::iterator sec = sections.find(pszSection);
	if (sec == sections.end())
	{
		// Section does not exist yet -- create it.
		CIniSection newSection(pszSection);
		sec = this->sections.insert(make_pair(pszSection, newSection)).first;
	}
	sec->second.WriteString(pszKey, wstrBuffer, bOverwrite);
}

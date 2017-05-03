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

//IniFile
//Declarations for CIniFile.
//Class for accessing and modifying DROD ini files.

#ifndef INIFILE_H
#define INIFILE_H
#ifdef WIN32
#  pragma warning(disable:4786)
#endif

#include "Wchar.h"

#include <list>
#include <map>
#include <string>
using std::list;
using std::map;
using std::string;

typedef list<string> IniEntry;

//******************************************************************************************
class CIniFile;
class CIniSection
{
public:
	CIniSection(const char *pszSetName);
	CIniSection();

	bool GetString(const char *pszKey, string& strBuffer);
	bool GetString(const char *pszKey, list<string>& strBuffer);
	bool GetString(const char *pszKey, list<WSTRING>& wstrBuffer);
	void WriteString(const char *pszKey, const char *pszValue, const bool bOverwrite);
	void WriteString(const char *pszKey, const list<WSTRING>& wstrValue, const bool bOverwrite);

	void Save(FILE* const pFile);

private:
	friend class CIniFile;

	string strName;
	map<string, IniEntry> entries; //one or more strings per key
};

//******************************************************************************************
class CIniFile
{
public:
	CIniFile();
	~CIniFile();

	bool DeleteEntry(const char *pszSection, const char *pszKey, const char *pszValue);

	bool Load(const WCHAR *wszSetFilename);
	bool LoadText(string strText, const bool bOverwrite=false, const bool bAddDuplicates=true);
	bool LoadText(const WSTRING& wstrText, const bool bOverwrite=false, const bool bAddDuplicates=true);
	
	//Use this method to access data in the INI file
	bool GetString(const char *pszSection, const char *pszKey, string& strBuffer);
	bool GetString(const char *pszSection, const char *pszKey, list<string>& strBuffer);
	bool GetString(const char *pszSection, const char *pszKey, list<WSTRING>& wstrBuffer);
	
	//Use this method to add/modify data in the INI file
	void WriteString(const char *pszSection, const char *pszKey,
			const char *pszValue, const bool bOverwrite=true);
	void WriteString(const char *pszSection, const char *pszKey,
			const list<WSTRING>& wstrBuffer, const bool bOverwrite=true);

private:
	WSTRING wstrFilename;
	bool  bLoaded;
	bool  bDirty;
	map <string, CIniSection> sections;
};

#endif //...#ifndef INIFILE_H

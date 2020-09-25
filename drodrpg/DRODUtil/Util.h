// $Id: Util.h 8102 2007-08-15 14:55:40Z trick $

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

//Util.h
//Declarations for CUtil base class.

#ifndef UTIL_H
#define UTIL_H

#include "OptionList.h"

#ifdef WIN32 //Many things will not compile w/o WIN32 API.  Fix them if you are porting.
#  pragma warning(disable:4786)
#endif

//To add new version, update VERSION_COUNT, tagVersion, and g_szarrVersions.  Versions
//are expected to be in sequence--newer versions go on the end.
const UINT MAXLEN_VERSION = 10;
enum VERSION
{
	v1_11c = 0,
	v1_5,
	v1_6,
	v2_0,
	v3_0,

	VERSION_COUNT
};
#ifndef INCLUDED_FOR_UTIL_CPP
extern WCHAR g_szarrVersions[VERSION_COUNT][MAXLEN_VERSION + 1];
#endif

//*****************************************************************************************
class CUtil
{
public:
	CUtil(VERSION eSetVersion, const WCHAR* pszSetPath);
	virtual ~CUtil() {}

	static bool CopyViewDef(const WCHAR *pwzSrcFilepath, const WCHAR *pwzDestFilepath,
			const char *pszViewDefName);
	bool           DoesFileExist(const WCHAR* pszFilepath) const;
	VERSION        GetVersion() const {return this->eVersion;}
	static bool IsPathValid(const WCHAR* pszPath);

	virtual bool   PrintCreate(const COptionList &/*Options*/) const {PrintNotImplemented(); return false;}
	virtual bool   PrintDelete(const COptionList &/*Options*/) const {PrintNotImplemented(); return false;}
	virtual bool   PrintDemo(const COptionList &/*Options*/, UINT /*dwDemoID*/) const {PrintNotImplemented(); return false;}
	virtual bool   PrintExport(const COptionList &/*Options*/, const WCHAR* /*pszDestPath*/) const {PrintNotImplemented(); return false;}
	virtual bool   PrintHold(const COptionList &/*Options*/, UINT /*dwHoldID*/) const {PrintNotImplemented(); return false;}
	virtual bool   PrintLevel(const COptionList &/*Options*/, UINT /*dwLevelID*/) const {PrintNotImplemented(); return false;}
	virtual bool   PrintImport(const COptionList &/*Options*/, const WCHAR* /*pszSrcPath*/, VERSION /*eSrcVersion*/) const {PrintNotImplemented(); return false;}
	virtual bool   PrintMysql(const COptionList &/*Options*/, UINT /*dwRoomID*/) const {PrintNotImplemented(); return false;}
	virtual bool   PrintRoom(const COptionList &/*Options*/, UINT /*dwRoomID*/) const {PrintNotImplemented(); return false;}
	virtual bool   PrintSummary(const COptionList &/*Options*/) const {PrintNotImplemented(); return false;}
	virtual bool   PrintTest(const COptionList &/*Options*/, UINT /*dwDemoID*/) const {PrintNotImplemented(); return false;}

protected:
	WSTRING        strPath;
	VERSION        eVersion;

private:
	void        PrintNotImplemented() const;
};

#endif //..#ifndef UTIL_H


// $Id: Util3_0.h 8144 2007-08-26 17:07:39Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2003, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//Util3_0.h
//Declarations for CUtil3_0.

#ifndef UTIL3_0_H
#define UTIL3_0_H

#include "Util.h"

#include <BackEndLib/MessageIDs.h>
using namespace Language;

#include <mk4.h>

#include <list>
#include <map>
using std::list;
using std::map;

class CUtil3_0 : public CUtil
{
private:
	 typedef map<string,UINT> ASSIGNEDMIDS;
public:
	CUtil3_0(const WCHAR* pszSetPath) : CUtil(v3_0, pszSetPath) { };
	
	virtual bool  PrintCreate(const COptionList &Options) const;
	virtual bool  PrintDelete(const COptionList &Options) const;
	virtual bool  PrintImport(const COptionList &Options, const WCHAR* pszSrcPath, VERSION eSrcVersion) const;
	virtual bool  PrintRoom(const COptionList &Options, UINT dwRoomID) const;
	virtual bool  PrintLevel(const COptionList &Options, UINT dwLevelID) const;
	virtual bool  PrintTest(const COptionList &Options, UINT dwDemoID) const;

private:
	static void AddMessageText(c4_Storage &TextStorage, const UINT dwMessageID,
		  const Language::LANGUAGE eLanguage, const WCHAR *pwszText);
	bool createDatabase(const WSTRING& wstrFilepath) const;
	static bool DeleteDat(const WCHAR *pwszFilepath);
	void        GetAssignedMIDs(const WCHAR *pwzMIDFilepath, ASSIGNEDMIDS &AssignedMIDs, 
				UINT &dwMaxMessageID) const;
	void        GetHoldFilepath(WSTRING &wstrFilepath) const;
	void        GetPlayerFilepath(WSTRING &wstrFilepath) const;
	bool        ImportBasicMessages(const WCHAR *pwzSrcPath, c4_Storage &TextStorage) const;
	bool        ImportMessageTexts(c4_Storage &SourceStorage, c4_Storage &DestStorage, 
				const list<UINT> &NeededMessages) const;
	bool        ImportUNI(const WCHAR *pwzFilepath, c4_Storage &TextStorage, const ASSIGNEDMIDS &AssignedMIDs,
				UINT &dwMaxMessageID, string &strMIDs) const;
};

#endif //...#ifndef UTIL3_0_H


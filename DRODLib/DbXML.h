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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//DbXML.h
//Declarations for CDbXML.
//Interface for exporting/importing database data to/from disk.

#ifndef DBXML_H
#define DBXML_H

#include "Db.h"
#include "../Texts/MIDs.h"

#include <expat.h>
#include <zlib.h>

#include <map>
#include <vector>
using std::vector;

extern const char szDRODVersion[];

struct DEMO_UPLOAD
{
	DEMO_UPLOAD(const CNetRoom& room, const string& buffer, const UINT wTurn, const UINT dwTimeElapsed,
		const UINT dwDemoID, const UINT dwCreatedTime, CDbDemo::DemoFlag flag, const WSTRING& wsPlayerName = WSTRING())
		: room(room), wsPlayerName(wsPlayerName), buffer(buffer), wTurn(wTurn), dwTimeElapsed(dwTimeElapsed)
		, dwDemoID(dwDemoID), dwCreatedTime(dwCreatedTime), flag(flag)
	{ }
	CNetRoom room;
	WSTRING wsPlayerName;
	string buffer;
	UINT wTurn;
	UINT dwTimeElapsed;
	UINT dwDemoID;
	UINT dwCreatedTime;
	CDbDemo::DemoFlag flag;
};

struct streamingOutParams
{
	streamingOutParams()
		: pOutBuffer(NULL)
		, pGzf(NULL)
	{ }
	void reset() {
		pOutBuffer = NULL;
		pGzf = NULL;
	}
	void set(string* str, gzFile* gzf)
	{
		pOutBuffer = str;
		pGzf = gzf;
	}
	bool flush(const ULONG maxSizeThreshold = 0);

	string* pOutBuffer;
	gzFile* pGzf;
};

//*****************************************************************************
struct ImportBuffer;
class CDbXML : public CDb
{
public:
	typedef std::map<VIEWTYPE, CIDSet> ViewIDMap;

	//Importing and exporting methods.
	static void CleanUp();
	static MESSAGE_ID ImportXML(const WCHAR *pszFilename, const CImportInfo::ImportType type);
	static MESSAGE_ID ImportXML(CStretchyBuffer &buffer, const CImportInfo::ImportType type);
	static MESSAGE_ID ImportXML(const string& xml);
	static MESSAGE_ID ImportXMLRaw(const string& buf, const CImportInfo::ImportType type, const bool bUncompress=false);
	static MESSAGE_ID ImportXML();	//continue import already in progress
	static bool ExportXML(const VIEWTYPE vType,
			const UINT dwPrimaryKey, const WCHAR *pszFilename);
	static bool ExportXML(const VIEWTYPE vType,
			const CIDSet& primaryKeys, const WCHAR *pszFilename);
	static bool ExportXML(const VIEWTYPE vType, const CIDSet& primaryKeys,
			string &text, const UINT eSaveType=0);
	static bool ExportXML(const ViewIDMap& viewExportIDs,
			string &text, const UINT eSaveType=0);

	static MESSAGE_ID Uncompress(BYTE* buffer, UINT size);

	static UINT GetActiveSpeechID();

	static string getXMLheader(const string *pString=NULL);
	static string getXMLfooter();

	//For XML parsing.
	static void StartElement(void *userData, const char *name, const char **atts);
	static void TallyElement(void *userData, const char *name, const char **atts);
	static void EndElement(void *userData, const char *name);

	static bool WasImportSuccessful();

	static bool ExportSavedGames(const UINT dwHoldID);

	static void SetCallback(CAttachableObject *pObject);
	static void PerformCallback(long val);
	static void PerformCallbackf(float fVal);
	static void PerformCallbackText(const WCHAR* wpText);

	static CImportInfo info;
	static vector<DEMO_UPLOAD> upgradedHoldVictoryDemos;
	static RecordMap exportInfo;

private:
	static void AddRowsForPendingRecords();
	static bool ContinueImport(const MESSAGE_ID status = MID_ImportSuccessful);
	static bool ExportXMLRecords(CDbRefs& dbRefs, const CIDSet& primaryKeys, string &text);

	static CDbBase * GetNewRecord(const VIEWTYPE vType);

	static MESSAGE_ID ImportXML(ImportBuffer* pBuffer);
	static void Import_Init();
	static void Import_TallyRecords(ImportBuffer* pBuffer);
	static void Import_TallyRecords(const string& xml);
	static void Import_ParseRecords(ImportBuffer* pBuffer);
	static void Import_ParseRecords(const string& xml);
	static void Import_Resolve();

	static VIEWTYPE ParseViewType(const char *str);
	static VIEWPROPTYPE ParseViewpropType(const char *str);
	static PROPTYPE ParsePropType(const char *str);

	static bool UpdateLocalIDs();

	static void ImportSavedGames();
	static void VerifySavedGames();

	static vector <CDbBase*> dbRecordStack;   //stack of records being parsed
	static vector <UINT> dbImportedRecordIDs;  //imported record IDs (primary keys)
	static vector <VIEWTYPE> dbRecordTypes;   //record types
	static vector <VIEWPROPTYPE> vpCurrentType;  //stack of viewprops being parsed
	static vector <bool>  SaveRecord;   //whether record should be saved to the DB

	static streamingOutParams streamingOut;
};

#endif //...#ifndef DBMXL_H

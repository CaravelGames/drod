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
 * Portions created by the Initial Developer are Copyright (C)
 * 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//DbData.h
//Declarations for CDbData and CDbDatum.
//Classes for accessing misc. embedded data types from database.

#ifndef DBDATA_H
#define DBDATA_H

#include "DbVDInterface.h"
#include <BackEndLib/StretchyBuffer.h>

#include <set>

//*****************************************************************************
class CDb;
class CDbDatum;
class CDbData : public CDbVDInterface<CDbDatum>
{
protected:
	friend class CDb;

	CDbData()
		: CDbVDInterface<CDbDatum>(V_Data, p_DataID)
		, dwFilterByHoldID(0)
	{}

public:
	static void      CopyObject(CImportInfo& info, UINT& dataID,
			const UINT newHoldID, bool attemptToMatchOnName=true);
	static void      SkipIDsFromHoldForCopy(CImportInfo& info, const UINT holdID);

	virtual void     Delete(const UINT dwDataID);
	virtual void     ExportXML(const UINT dwDataID, CDbRefs &dbRefs, string &str, const bool bRef=false);
	void             FilterByFormat(const CIDSet& setFilterByFormat);
	void             FilterByHold(const UINT dwSetFilterByHoldID);
	UINT             FindByName(const WCHAR *pwczName);
	CIDSet           GetIDsForMod(const WCHAR* pModName);
	static   WSTRING GetModNameFor(const UINT dwDataID);
	std::set<WSTRING> GetModNames();
	static   WSTRING GetNameFor(const UINT dwDataID);
	static   UINT    GetRawDataForID(const UINT dwDataID, BYTE* &pData);
	static   bool    GetRawDataForID(const UINT dwDataID, CStretchyBuffer& buffer);

private:
	virtual void     LoadMembership();

	UINT    dwFilterByHoldID;
	CIDSet     filterByFormat;
};

//*****************************************************************************
class CDbDatum : public CDbBase
{
protected:
	friend class CDbData;
	friend class CDbVDInterface<CDbDatum>;
	CDbDatum()
		: CDbBase()
		, dwDataID(0)
	{
		Clear();
	}

public:
	CDbDatum(const CDbDatum& that, const bool bReplicateData=false);
	UINT        GetPrimaryKey() const {return this->dwDataID;}
	bool        Load(const UINT dwDataID, const bool bQuick=false);
	virtual MESSAGE_ID   SetProperty(const PROPTYPE pType, char* const str,
			CImportInfo &info, bool &bSaveRecord);
	virtual bool   Update();

	UINT       dwDataID;
	UINT        wDataFormat;
	WSTRING     DataNameText, modName;
	CStretchyBuffer data;
	CStretchyBuffer timData;   //tile image specific data
	UINT       dwHoldID;      //what hold owns this record, if any

private:
   void        Clear();
	UINT       GetLocalID() const;
	bool        UpdateNew();
	bool        UpdateExisting();
};

#endif //...#ifndef DBDATA_H

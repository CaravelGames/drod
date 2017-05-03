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
 * 2003, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "DbData.h"
#include "Db.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Base64.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Ports.h>

//
//CDbData public methods.
//

//*****************************************************************************
void CDbData::CopyObject(
//Call to make a copy of an owned media object in the DB,
//e.g. an image embedded in a hold,
//and update the ID for the record pointing to the copy of the original.
//
//Params:
	CImportInfo& info, //(in/out) media ID pairing info -- makes sure each object is copied no more than once
	UINT& dataID, //(in/out) set to ID of object copy
	const UINT newHoldID, //if set, use as new holdID
	bool attemptToMatchOnName) //if set, use any preexisting data object with the same name as the source data
{
	if (!dataID)
		return;

	PrimaryKeyMap::iterator copyID = info.DataIDMap.find(dataID);
	if (copyID != info.DataIDMap.end()) {
		dataID = copyID->second; //set ID reference to previously copied object
		return;
	}

	if (attemptToMatchOnName) {
		//Make copy of room image if hold doesn't already have an image object with matching name.
		const WSTRING dataName = GetNameFor(dataID);
		CDb db;
		db.Data.FilterByHold(newHoldID);
		const UINT dataCopyID = db.Data.FindByName(dataName.c_str());
		if (dataCopyID) {
			dataID = dataCopyID;
			return;
		}
	}

	//Make new instance of data object in DB.
	const CDbDatum *pData = g_pTheDB->Data.GetByID(dataID);
	ASSERTP(pData,"Dangling media ID");
	if (!pData) {
		dataID = 0; //no currently existing data object to reference
	} else {
		CDbDatum *pNewData = new CDbDatum(*pData, true);
		if (newHoldID)
			pNewData->dwHoldID = newHoldID;
		pNewData->Update();
		info.DataIDMap[dataID] = pNewData->dwDataID;
		dataID = pNewData->dwDataID;
		delete pData;
		delete pNewData;
	}
}

//*******************************************************************************
void CDbData::SkipIDsFromHoldForCopy(CImportInfo& info, const UINT holdID)
//Adds all dataIDs belonging to this hold to 'info' and maps them to themselves,
//so that when CopyObject is called on these ids, they remain unchanged
//and no data is copied.
{
	CDbData data;
	data.FilterByHold(holdID);
	CIDSet holdDataIDs = data.GetIDs();
	for (CIDSet::const_iterator id = holdDataIDs.begin(); id != holdDataIDs.end(); ++id)
		info.DataIDMap[*id] = *id;
}

//*******************************************************************************
void CDbData::Delete(
//Deletes a data record.
//
//Params:
	const UINT dwDataID)   //(in)   Datum to delete.
{
	ASSERT(dwDataID);

	c4_View DataView;
	const UINT dwDataRowI = LookupRowByPrimaryKey(dwDataID, V_Data, DataView);
	if (dwDataRowI==ROW_NO_MATCH)
		return; //dangling ID reference

	DataView.RemoveAt(dwDataRowI);
	CDb::deleteData(dwDataID);

	//After object is deleted, membership might change, so reset the flag.
	this->bIsMembershipLoaded = false;
}

//*****************************************************************************
void CDbData::ExportXML(
//Returns: string containing XML text describing data with this ID
//
//Pre-condition: dwDataID is valid
//
//Params:
	const UINT dwDataID,   //(in)
	CDbRefs &dbRefs,        //(in/out)
	string &str,            //(in/out)
	const bool bRef)    //(in) Only export GUID reference (default = false)
{
#define STARTTAG(vType,pType) "<"; str += ViewTypeStr(vType); str += " "; str += PropTypeStr(pType); str += "='"
#define PROPTAG(pType) "' "; str += PropTypeStr(pType); str += "='"
#define CLOSETAG "'/>\n"
#define INT32TOSTR(val) writeInt32(dummy, sizeof(dummy), (val))

	if (dbRefs.IsSet(V_Data,dwDataID))
		return;

	dbRefs.Set(V_Data,dwDataID);

	CDbDatum *pData = GetByID(dwDataID);
	if (!pData)
		return; //there may be dangling dataIDs

#ifdef EXPORTNOSPEECH
	if (pData->wDataFormat == DATA_WAV || pData->wDataFormat == DATA_OGG)
		return;
#endif

	char dummy[32];
	str += STARTTAG(V_Data, P_DataFormat);
	str += INT32TOSTR(pData->wDataFormat);
	str += PROPTAG(P_DataNameText);
	str += Base64::encode(pData->DataNameText);
	if (!bRef) {
		if (pData->data.Size() > 0)
		{
			str += PROPTAG(P_RawData);
			str += Base64::encode((const BYTE*)pData->data,pData->data.Size());
		}
		if (pData->timData.Size() > 0)
		{
			str += PROPTAG(P_TimData);
			str += Base64::encode((const BYTE*)pData->timData,pData->timData.Size());
		}
	}
	if (pData->modName.size())
	{
		str += PROPTAG(P_ModName);
		str += Base64::encode(pData->modName);
	}
	if (pData->dwHoldID != 0 || dbRefs.vTypeBeingExported == V_Holds)
	{
		str += PROPTAG(P_HoldID);
		//When exporting a hold, ensure all data objects being exported are
		//marked as belonging to the hold.  (Fixes pre-2.0.15 bug.)
		if (!dbRefs.exportingIDs.empty() && !bRef)
			str += INT32TOSTR(dbRefs.exportingIDs.getFirst());
		else
			str += INT32TOSTR(pData->dwHoldID);
	}

	//Put primary key last, so all fields have been set by the
	//time Update() is called during import.
	str += PROPTAG(P_DataID);
	str += INT32TOSTR(pData->dwDataID);
	str += CLOSETAG;

	delete pData;

#undef STARTTAG
#undef PROPTAG
#undef CLOSETAG
#undef INT32TOSTR
}

//*****************************************************************************
void CDbData::FilterByFormat(
//Changes filter so that GetFirst() and GetNext() will return data for a
//specified data format.
//
//Params:
	const CIDSet& setFilterByFormat)   //(in)   Format to filter by.
												//Set to 0 for all formats.
{
	if (this->bIsMembershipLoaded && !this->filterByFormat.contains(setFilterByFormat))
	{
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	this->filterByFormat = setFilterByFormat;
}

//*****************************************************************************
void CDbData::FilterByHold(
//Changes filter so that GetFirst() and GetNext() will return data for a
//specified hold.
//
//Params:
	const UINT dwSetFilterByHoldID) //(in)   Hold ID to filter by.
												//Set to 0 for all holds.
{
	if (dwSetFilterByHoldID != this->dwFilterByHoldID && this->bIsMembershipLoaded)
	{
		//Membership is invalid.
		this->bIsMembershipLoaded = false;
	}

	this->dwFilterByHoldID = dwSetFilterByHoldID;
}

//*****************************************************************************
CIDSet CDbData::GetIDsForMod(const WCHAR* pModName)
//Returns: a set of DataIDs having the indicated mod name
{
	ASSERT(IsOpen());

	//Each iteration checks the modName field of a record.
	CIDSet ids;
	const UINT dwDataCount = GetViewSize();
	for (UINT dwDataI = 0; dwDataI < dwDataCount; ++dwDataI)
	{
		c4_RowRef row = GetRowRef(V_Data, dwDataI);
		c4_Bytes ModNameBytes = p_ModName(row);
		WSTRING modName;
		GetWString(modName, ModNameBytes);
		if (!WCScmp(pModName, modName.c_str()))
			ids += UINT(p_DataID(row));
	}
	return ids;
}

//*****************************************************************************
set<WSTRING> CDbData::GetModNames()
//Returns: all stored mod names
{
	set<WSTRING> names;

	ASSERT(IsOpen());

	const UINT dwDataCount = GetViewSize();
	for (UINT dwDataI = 0; dwDataI < dwDataCount; ++dwDataI)
	{
		c4_RowRef row = GetRowRef(V_Data, dwDataI);
		c4_Bytes ModNameBytes = p_ModName(row);
		WSTRING modName;
		GetWString(modName, ModNameBytes);
		if (!modName.empty())
			names.insert(modName);
	}

	return names;
}

//*****************************************************************************
WSTRING CDbData::GetModNameFor(const UINT dwDataID)
//Returns: mod field text for record with indicated dataID
{
	ASSERT(IsOpen());

	//Find record with matching Data ID.
	c4_View DataView;
	const UINT dwDataI = LookupRowByPrimaryKey(dwDataID, V_Data, DataView);
	if (dwDataI == ROW_NO_MATCH) return wszEmpty;

	WSTRING modName;
	c4_Bytes ModNameBytes = p_ModName(DataView[dwDataI]);
	GetWString(modName, ModNameBytes);
	return modName;
}

//*****************************************************************************
WSTRING CDbData::GetNameFor(const UINT dwDataID)
//Returns: name field text for record with indicated dataID
{
	ASSERT(IsOpen());

	//Find record with matching Data ID.
	c4_View DataView;
	const UINT dwDataI = LookupRowByPrimaryKey(dwDataID, V_Data, DataView);
	if (dwDataI == ROW_NO_MATCH) return wszEmpty;

	WSTRING name;
	c4_Bytes NameTextBytes = p_DataNameText(DataView[dwDataI]);
	GetWString(name, NameTextBytes);
	return name;
}

//*****************************************************************************
UINT CDbData::GetRawDataForID(const UINT dwDataID, BYTE* &pData)
//OUT: an allocated buffer of the raw data stored in the specified record.
//Buffer must be deleted by the caller.
//
//Returns: size of data
{
	pData = NULL;

	ASSERT(IsOpen());

	//Find record with matching Data ID.
	c4_View DataView;
	const UINT dwDataI = LookupRowByPrimaryKey(dwDataID, V_Data, DataView);
	if (dwDataI == ROW_NO_MATCH) return 0;

	const c4_Bytes &data = p_RawData(DataView[dwDataI]);
	const UINT dwSize = data.Size();
	pData = new BYTE[dwSize];
	if (!pData) return 0;
	memcpy(pData, data.Contents(), dwSize);
	return dwSize;
}

//*****************************************************************************
bool CDbData::GetRawDataForID(const UINT dwDataID, CStretchyBuffer& buffer)
//OUT: an allocated buffer of the raw data stored in the specified record.
//
//Returns: whether operation succeeded
{
	ASSERT(IsOpen());

	//Find record with matching Data ID.
	c4_View DataView;
	const UINT dwDataI = LookupRowByPrimaryKey(dwDataID, V_Data, DataView);
	if (dwDataI == ROW_NO_MATCH) return false;

	const c4_Bytes &data = p_RawData(DataView[dwDataI]);
	const UINT dwSize = data.Size();
	buffer.Set(data.Contents(), dwSize);
	return true;
}

//
//CDbData private methods.
//

//*****************************************************************************
void CDbData::LoadMembership()
//Load the membership list with all data IDs.
{
	this->MembershipIDs.clear();

	//Each iteration gets a data ID and puts in membership list.
	ASSERT(IsOpen());
	const UINT dwDataCount = GetViewSize();
	for (UINT dwDataI = 0; dwDataI < dwDataCount; ++dwDataI)
	{
		c4_RowRef row = GetRowRef(V_Data, dwDataI);
		const UINT dwHoldID = p_HoldID(row);
		if (!this->dwFilterByHoldID || dwHoldID == this->dwFilterByHoldID)
		{
			const UINT wDataFormat = p_DataFormat(row);
			if (this->filterByFormat.empty() || this->filterByFormat.has(wDataFormat))
				this->MembershipIDs += p_DataID(row);
		}
	}
	this->currentRow = this->MembershipIDs.begin();
	this->bIsMembershipLoaded = true;
}

//*****************************************************************************
UINT CDbData::FindByName(
//Returns the ID of the data record with this name, and matching format holdID, and filter (if specified).
//
//Returns:
//DataID if a match was found, 0L for no match.
//
//Params:
	const WCHAR *pwczName)
{
	ASSERT(IsOpen());

	//Each iteration checks an image name from one record.
	const UINT dwDataCount = GetViewSize();
	for (UINT dwDataI = 0; dwDataI < dwDataCount; ++dwDataI)
	{
		c4_RowRef row = GetRowRef(V_Data, dwDataI);
		c4_Bytes NameTextBytes = p_DataNameText(row);
		WSTRING name;
		GetWString(name, NameTextBytes);
		if (!WCScmp(pwczName, name.c_str()))
		{
			//Name matches -- check for hold+format filter match.
			const UINT dwHoldID = p_HoldID(row);
			if (dwHoldID == this->dwFilterByHoldID)
			{
				const UINT wDataFormat = p_DataFormat(row);
				if (this->filterByFormat.empty() || this->filterByFormat.has(wDataFormat))
				{
					return (UINT) p_DataID(row); //Found it.
				}
			}
		}
	}

	//Name not found.
	return 0;
}

//
//CDbDatum methods
//

//*****************************************************************************
CDbDatum::CDbDatum(const CDbDatum& that, const bool bReplicateData)  //[false]
	: CDbBase()
	, wDataFormat(that.wDataFormat)
	, DataNameText(that.DataNameText), modName(that.modName)
	, data(that.data)
	, timData(that.timData)
	, dwHoldID(that.dwHoldID)
{
	if (!bReplicateData)
	{
		this->dwDataID = that.dwDataID;
	} else {
		this->dwDataID = 0;
	}
}

//*****************************************************************************
void CDbDatum::Clear()
//Clears members of object.
{
	this->bPartialLoad = false;

	this->dwDataID = this->dwHoldID = 0L;
	this->wDataFormat = 0;
	this->DataNameText = wszEmpty;
	this->timData.Clear();
	this->modName = wszEmpty;
}

//*****************************************************************************
UINT CDbDatum::GetLocalID() const
//Compares this object's GID fields against those of the records in the DB.
//ASSUME: dwHoldID has already been set to the local record ID
//
//Returns: local ID if a record in the DB matches this object's GID, else 0
{
	//Only universal data objects may be replaced.
	//That is, there must be not be owned by any hold.
	if (this->dwHoldID)
		return 0;

	ASSERT(IsOpen());

	//Each iteration checks a record's GIDs.
	const UINT dwDataCount = GetViewSize(V_Data);
	for (UINT dwDataI = 0; dwDataI < dwDataCount; ++dwDataI)
	{
		//Check name.
		c4_RowRef row = GetRowRef(V_Data, dwDataI);
		c4_Bytes NameTextBytes = p_DataNameText(row);
		WSTRING name;
		GetWString(name, NameTextBytes);

		if (!WCScmp(this->DataNameText.c_str(), name.c_str()))
		{
			//Compare format.
			const UINT wDataFormat = p_DataFormat(row);
			if (this->wDataFormat == wDataFormat)
			{
				//See remark above about no owning hold.
				if (p_HoldID(row) == 0)
				{
					//GUIDs match.  Return this record's local ID.
					return UINT(p_DataID(row));
				}
			}
		}
	}

	//No match.
	return 0;
}

//*****************************************************************************
bool CDbDatum::Load(
//Loads an embedded data record from database into this object.
//
//Params:
	const UINT dwDataID,   //(in) DataID of data to load.
	const bool bQuick) //(in) [default=false] load only quick data members
//
//Returns:
//True if successful, false if not.
{
	try {
		Clear();

		//Find record with matching Data ID.
		ASSERT(IsOpen());
		c4_View DataView;
		const UINT dwDataI = LookupRowByPrimaryKey(dwDataID, V_Data, DataView);
		if (dwDataI == ROW_NO_MATCH) throw CException("CDbDatum::Load");
		c4_RowRef row = DataView[dwDataI];

		//Load in props from Data record.
		this->dwDataID = (UINT) (p_DataID(row));
		c4_Bytes NameTextBytes = p_DataNameText(row);
		GetWString(this->DataNameText, NameTextBytes);
		this->wDataFormat = (UINT) (p_DataFormat(row));
		this->dwHoldID = (UINT) (p_HoldID(row));
		c4_Bytes ModNameBytes = p_ModName(row);
		GetWString(this->modName, ModNameBytes);

		this->bPartialLoad = bQuick;
		if (!bQuick)
			this->data = p_RawData(row);

		//Tile image specific data.  (Only these two data formats are used for storing tile data.)
		if (this->wDataFormat == DATA_BMP || this->wDataFormat == DATA_PNG)
			this->timData = p_TimData(row);
	}
	catch (CException&)
	{
		Clear();
		return false;
	}
	return true;
}

//*****************************************************************************
bool CDbDatum::Update()
//Updates database with this data.
{
	if (this->bPartialLoad)
	{
		ASSERT(false); //don't try to update partially-loaded records
		return false;
	}

	if (this->dwDataID == 0)
	{
		//Insert a new data record.
		return UpdateNew();
	}

	//Update existing data.
	return UpdateExisting();
}

//*****************************************************************************
bool CDbDatum::UpdateNew()
//Insert a new Data record.
{
	LOGCONTEXT("CDbDatum::UpdateNew");
	ASSERT(this->dwDataID == 0);
	ASSERT(IsOpen());

	this->dwDataID = GetIncrementedID(p_DataID);

	//Prepare data.
	c4_Bytes DataBytes((const BYTE*)this->data, this->data.Size());
	c4_Bytes TimBytes((const BYTE*)this->timData, this->timData.Size());

	//Add record.
	c4_RowRef row = g_pTheDB->Data.GetNewRow();
	p_DataID(row) = this->dwDataID;
	p_DataFormat(row) = this->wDataFormat;
	p_HoldID(row) = this->dwHoldID;
	p_DataNameText(row) = PutWString(this->DataNameText);
	p_RawData(row) = DataBytes;
	p_TimData(row) = TimBytes;
	p_ModName(row) = PutWString(this->modName);

	CDb::addDataToHold(this->dwDataID, this->dwHoldID);
	return true;
}

//*****************************************************************************
bool CDbDatum::UpdateExisting()
//Update an existing Data record.
{
	LOGCONTEXT("CDbDatum::UpdateExisting");
	ASSERT(this->dwDataID != 0);
	ASSERT(IsOpen());

	//Lookup data record.
	c4_View DataView;
	const UINT dwDataI = LookupRowByPrimaryKey(this->dwDataID,	V_Data, DataView);
	if (dwDataI == ROW_NO_MATCH)
	{
		ASSERT(!"DataID is bad.");
		return false;
	}

	//Ensure the datum is indexed under the hold it is currently owned by.
	c4_RowRef row = DataView[dwDataI];
	CDb::moveData(this->dwDataID, UINT(p_HoldID(row)), this->dwHoldID);

	//Prepare data.
	c4_Bytes DataBytes((const BYTE*)this->data, this->data.Size());
	c4_Bytes TimBytes((const BYTE*)this->timData, this->timData.Size());

	//Update record.
	p_DataID(row) = this->dwDataID;
	p_DataFormat(row) = this->wDataFormat;
	p_HoldID(row) = this->dwHoldID;
	p_DataNameText(row) = PutWString(this->DataNameText);
	p_RawData(row) = DataBytes;
	p_TimData(row) = TimBytes;
	p_ModName(row) = PutWString(this->modName);

	CDbBase::DirtyData();
	return true;
}

//*****************************************************************************
MESSAGE_ID CDbDatum::SetProperty(
//Used during XML data import.
//According to pType, convert string to proper datatype and member
//
//Returns: whether operation was successful
//
//Params:
	const PROPTYPE pType,   //(in) property (data member) to set
	char* const str,        //(in) string representation of value
	CImportInfo &info,      //(in/out) Import info
	bool &bSaveRecord)      //(out) whether record should be saved
{
	switch (pType)
	{
		case P_DataID:
		{
			this->dwDataID = convertToUINT(str);
			if (!this->dwDataID)
				return MID_FileCorrupted;  //corrupt file

			//Look up local ID.
			PrimaryKeyMap::const_iterator localID = info.DataIDMap.find(this->dwDataID);
			if (localID != info.DataIDMap.end())
				//Error - this data should not have been imported already.
				return MID_FileCorrupted;

			//Look up data member in the DB if it's not data members local to a hold being imported.
			UINT dwLocalDataID;
			switch (info.typeBeingImported)
			{
				case CImportInfo::Hold: dwLocalDataID = 0; break;
				case CImportInfo::Player:
				case CImportInfo::SavedGame:
				case CImportInfo::DemosAndSavedGames:
				{
					CDb db;
					db.Data.FilterByHold(this->dwHoldID);
					dwLocalDataID = db.Data.FindByName(this->DataNameText.c_str());

					//whether a mapping is found or not, we don't want to create a new data record for this reference
					bSaveRecord = false;
				}
				break;
				default: dwLocalDataID = GetLocalID(); break;
			}

			if (dwLocalDataID)
			{
				//Data object found in DB.
				info.DataIDMap[this->dwDataID] = dwLocalDataID;
				this->dwDataID = dwLocalDataID;
				bSaveRecord = false;
			} else {
				if (bSaveRecord)
				{
					//Add a new data (ignore old local ID).
					const UINT dwOldLocalID = this->dwDataID;
					this->dwDataID = 0;
					Update();
					info.DataIDMap[dwOldLocalID] = this->dwDataID;
					info.dwDataImportedID = this->dwDataID;  //mark that a data record was imported
				} else {
					//This data is being ignored.
					info.DataIDMap[this->dwDataID] = 0;   //skip records with refs to this data ID
				}
			}
		}
		break;

		case P_DataFormat:
			this->wDataFormat = convertToUINT(str);
		break;
		case P_HoldID:
		{
			this->dwHoldID = convertToUINT(str);
			//Set to local ID.
			PrimaryKeyMap::const_iterator localID = info.HoldIDMap.find(this->dwHoldID);
			if (localID == info.HoldIDMap.end())
				return MID_HoldNotFound;   //record should have been loaded already
			this->dwHoldID = localID->second;
			if (!this->dwHoldID)
			{
				//Records for this hold are being ignored.  Don't save this data.
				bSaveRecord = false;
			}
		}
		break;
		case P_DataNameText:
		case P_DataNameTextID:  //deprecated (used in 2.0 pre-beta)
			Base64::decode(str,this->DataNameText);
		break;
		case P_RawData:
		{
			BYTE *data;
			const UINT size = Base64::decode(str,data);
			this->data.Set(data, size);
			delete[] data;
		}
		break;
		case P_TimData:
		{
			BYTE *data;
			const UINT size = Base64::decode(str,data);
			this->timData.Set(data, size);
			delete[] data;
		}
		break;
		case P_ModName:
			Base64::decode(str,this->modName);
		break;

		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

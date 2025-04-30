// $Id: DbXML.cpp 10113 2012-04-22 05:40:36Z mrimer $

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

//DbXML.cpp
//Implementation of CDbXML.
//
//Used for importing/exporting records from the DB to XML files.

#include "DbXML.h"
#include "CurrentGame.h"
#include <BackEndLib/Exception.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/StretchyBuffer.h>
#include <BackEndLib/Ports.h>

#include <cstdio>

const char gzID[] = "\x1f\x8b"; //gzip file header id
const UINT EXPORT_MAX_SIZE_THRESHOLD = 10 * 1024 * 1024; //10 MB
const int XML_PARSER_BUFF_SIZE = 128 * 1024; //uncompressed buffer chunk size for import

//Static vars
CImportInfo CDbXML::info;
//vector<DEMO_UPLOAD> CDbXML::upgradedHoldVictoryDemos;
RecordMap CDbXML::exportInfo;

vector <CDbBase*> CDbXML::dbRecordStack;
vector <UINT> CDbXML::dbImportedRecordIDs;
vector <VIEWTYPE> CDbXML::dbRecordTypes;
vector <bool>  CDbXML::SaveRecord;
vector <VIEWPROPTYPE> CDbXML::vpCurrentType;
CAttachableObject* pCallbackObject = NULL;

streamingOutParams CDbXML::streamingOut;

//Local vars
static XML_Parser parser = NULL;
static bool bImportComplete = false;

const char szDRODVersion[] = "Version";
static const char szDRODHeaderInfo[] = "Info";

//Metakit requires a call to Commit when committing a large number of rows
//to the DB or a subsequent call to Add may fail on memory allocation,
//causing a crash.  This number specifies the maximum number of records
//that may be added during import before a call to Commit.
#define MAX_RECORDS_BEFORE_COMMIT (10000)

struct roomSet {CIDSet explored;};

typedef map<string,int> StringMap;
StringMap::iterator tokenIter;
StringMap viewMap, viewPropMap, propMap;

//*****************************************************************************
//This structure is used to provide data to the XML parser for iterative parsing.
//It supports incremental uncompression from a buffer of compressed data (zlib/gzip).
struct ImportBuffer
{
	ImportBuffer()
		: compressedBuffer(NULL), compressedSize(0)
		, uncompressedBuffer(NULL), uncompressedSize(0)
		, buf(NULL)
		, totalParseSize(0)
		, d_stream(NULL)
	{
	}
	~ImportBuffer() { clear(); }

	void clear()
	{
		delete[] compressedBuffer;
		compressedBuffer = NULL;
		compressedSize = 0;

		delete[] uncompressedBuffer;
		uncompressedBuffer = NULL;
		uncompressedSize = 0;

		buf = NULL;

		totalParseSize = 0;

		closeStream();

		ASSERT(isReset());
	}
	void closeStream()
	{
		if (d_stream)
			inflateEnd(d_stream);
		d_stream = NULL;
	}
	//Returns: true if object is clear
	bool isReset() const
	{
		if (compressedBuffer)
			return false;
		if (compressedSize)
			return false;
		if (uncompressedBuffer)
			return false;
		if (uncompressedSize)
			return false;
		if (buf)
			return false;
		if (totalParseSize)
			return false;
		if (d_stream)
			return false;

		return true;
	}
	//Returns: true if object is ready to use for parsing
	bool isPopulated() const
	{
		//compressedBuffer and compressedSize are optional
		if (!uncompressedBuffer)
			return false;
		if (!uncompressedSize)
			return false;
		if (!buf)
			return false;
		//d_stream use is optional

		return true;
	}

	//Call once to allocate a buffer of (size+1) for uncompressing data.
	//Returns: whether buffer was allocated.
	bool allocUncompressionBuffer()
	{
		ASSERT(!uncompressedBuffer);
		const int size = XML_PARSER_BUFF_SIZE; //implementation expects buffer of this size

		try {
			uncompressedBuffer = new BYTE[size + 1]; //allow null-termination
		}
		catch (std::bad_alloc&) {
			uncompressedBuffer = NULL;
			return false;
		}
		ASSERT(uncompressedBuffer);

		uncompressedBuffer[size] = '\0'; //Null-terminate text string so it's readable if we want to look at it

		buf = (char*)uncompressedBuffer; //Point 'buf' at where data to parse will be written during uncompression

		return true;
	}

	//Call once to make an internal copy of compressed data.
	//The copy will be used for incremental uncompression.
	bool copyCompressedData(const BYTE* buffer, const UINT size)
	{
		ASSERT(!compressedBuffer);
		ASSERT(!compressedSize);

		ASSERT(buffer);
		ASSERT(size);

		try {
			compressedBuffer = new BYTE[size];
		}
		catch (std::bad_alloc&) {
			compressedBuffer = NULL;
			return false;
		}

		memcpy(compressedBuffer, buffer, size);
		compressedSize = size;
		return true;
	}

	//Call this once when data to parse is already uncompressed
	//and no memory needs to be allocated for uncompression.
	void passthruUncompressedData(const string& text)
	{
		ASSERT(isReset());

		buf = const_cast<char*>(text.c_str());
		totalParseSize = uncompressedSize = text.size();

		uncompressedBuffer = new BYTE[1]; //dummy data, will be freed on clear
	}

	//Call to begin decompression of 'compressedBuffer'.
	//
	//Returns: MID indicating status of operation (e.g., MID_Success)
	UINT initStream()
	{
		ASSERT(!d_stream);

		z_stream* ds = new z_stream; // decompression stream (gzip/zlib format)
		ds->zalloc = (alloc_func)0;
		ds->zfree = (free_func)0;
		ds->opaque = (voidpf)0;

		int err = inflateInit(ds);
		if (err == Z_OK) {
			//gzip format: MAX_WBITS | 16
			//zlib format: MAX_WBITS
			//deflate format: -MAX_WBITS
			const bool bGzip = !strncmp((char*)this->compressedBuffer, gzID, 2);
			const int windowBits = bGzip ? MAX_WBITS | 16 : MAX_WBITS;
			err = inflateReset2(ds, windowBits);
		}
		if (err != Z_OK) {
			delete ds;
			return MID_OutOfMemory;
		}

		ds->next_in = compressedBuffer; // all compressed data are in the input buffer
		ds->avail_in = compressedSize;  // total size of data to uncompress
		d_stream = ds;

		return uncompressChunk(); //prepare first chunk of uncompressed data
	}

	//Call to uncompress another chunk of data.
	//Returns: MID indicating success or failure
	UINT uncompressChunk()
	{
		ASSERT(d_stream);

		d_stream->next_out = uncompressedBuffer; // pointer to the resulting uncompressed data buffer
		d_stream->avail_out = XML_PARSER_BUFF_SIZE;        // size of the uncompressed data buffer

		const int err = inflate(d_stream, Z_SYNC_FLUSH); //fill output buffer as much as possible
		switch (err) {
		case Z_STREAM_END: //all data was decompressed into the buffer
		case Z_OK: //partial data was decompressed
			uncompressedSize = (XML_PARSER_BUFF_SIZE - d_stream->avail_out); //amount of data made available to parse
			return MID_Success;
		case Z_MEM_ERROR:
			return MID_NoMoreMemory;
		case Z_BUF_ERROR:
		case Z_DATA_ERROR:
		case Z_STREAM_ERROR:
		case Z_VERSION_ERROR:
		default: //unknown error -- assume failure
			return MID_FileCorrupted;
		}
	}

	BYTE* compressedBuffer;   //copy of compressed data; free on completion
	uLongf compressedSize;    //size of uncompressed data

	BYTE* uncompressedBuffer; //allocated to store uncompressed data
	uLongf uncompressedSize;  //amount of data available to parse

	char* buf;                //pointer to data to parse; points to uncompressedBuffer when uncompressing data, otherwise points elsewhere

	uLongf totalParseSize;    //indicates size of all data being parsed (e.g., size of the uncompressed version of what's in compressedBuffer)

	z_stream* d_stream;       //if set, used to stream chunks of uncompressed data
};
ImportBuffer importBuf;

//*****************************************************************************
void InitTokens()
//Call before parsing to prepare set of recognized tokens.
{
	if (viewMap.empty())
		for (int vType=V_First; vType<V_Count; ++vType)
			viewMap[string(viewTypeStr[vType])] = vType;
	if (viewPropMap.empty())
		for (int vpType=VP_First; vpType<VP_Count; ++vpType)
			viewPropMap[string(viewpropTypeStr[vpType])] = vpType;
	if (propMap.empty())
		for (int pType=P_First; pType<P_Count; ++pType)
			propMap[string(propTypeStr[pType])] = pType;
}

//If buffer has more data than indicated amount, flush it to the file.
bool streamingOutParams::flush(const ULONG maxSizeThreshold) //[default=0]
{
	if (pOutBuffer) {
		const ULONG srcLen = (ULONG)(pOutBuffer->size() * sizeof(char));
		if (!srcLen || srcLen < maxSizeThreshold)
			return true;

		const ULONG bytesWritten = gzwrite(*this->pGzf, (const BYTE*)pOutBuffer->c_str(), (unsigned int)srcLen);
		pOutBuffer->erase(0, bytesWritten);
		return bytesWritten == srcLen;
	}
	return true; //no-op
}

//
//CDbXML private methods.
//

//*****************************************************************************
void CDbXML::AddRowsForPendingRecords()
//Call at the end of preparsing to preallocate the expected number of new rows.
{
	//Rows of these types are only added during hold import.
	//Hence, we don't need to add empty rows just for reference GIDs.
	//However, if this is not true in the future, this assumption may cause
	//memory fragmentation and/or slowdown during import, but not failure.
	if (CDbXML::info.typeBeingImported == CImportInfo::Hold)
	{
		g_pTheDB->Data.EnsureEmptyRows(CDbXML::info.nData);
		g_pTheDB->Holds.EnsureEmptyRows(CDbXML::info.nHolds);
		g_pTheDB->Levels.EnsureEmptyRows(CDbXML::info.nLevels);
		g_pTheDB->Rooms.EnsureEmptyRows(CDbXML::info.nRooms);
		g_pTheDB->Speech.EnsureEmptyRows(CDbXML::info.nSpeech);
	}

	g_pTheDB->Demos.EnsureEmptyRows(CDbXML::info.nDemos);
	g_pTheDB->Players.EnsureEmptyRows(CDbXML::info.nPlayers);
	g_pTheDB->SavedGames.EnsureEmptyRows(CDbXML::info.nSavedGames);
	g_pTheDB->HighScores.EnsureEmptyRows(CDbXML::info.nHighScore);
}

//*****************************************************************************
CDbBase* CDbXML::GetNewRecord(
//Class factory for classes derived from CDbBase.  New record will need its
//members set before it is used.
//
//Params:
	const VIEWTYPE vType)      //(in)   One of the DB record views.
//
//Returns:
//Pointer to a new instance of class derived from CDbBase.
{
	switch (vType)
	{
		//These are the only classes we are considering.
		case V_Data:
			return g_pTheDB->Data.GetNew();
		case V_Demos:
			return g_pTheDB->Demos.GetNew();
		case V_Holds:
			return g_pTheDB->Holds.GetNew();
		case V_Levels:
			return g_pTheDB->Levels.GetNew();
		case V_Players:
			return g_pTheDB->Players.GetNew();
		case V_Rooms:
			return g_pTheDB->Rooms.GetNew();
		case V_SavedGames:
			return g_pTheDB->SavedGames.GetNew();
		case V_SavedGameMoves:
			return g_pTheDB->SavedGameMoves.GetNew();
		case V_Speech:
			return g_pTheDB->Speech.GetNew();
		case V_LocalHighScores:
			return g_pTheDB->HighScores.GetNew();

		default:
			ASSERT(!"Unexpected view type.");
			return NULL;
	}
}

//*****************************************************************************
VIEWTYPE CDbXML::ParseViewType(const char *str)
//Returns: enumeration corresponding to string
{
	tokenIter = viewMap.find(string(str));
	return tokenIter == viewMap.end() ? V_Invalid : VIEWTYPE(tokenIter->second);
}

//*****************************************************************************
VIEWPROPTYPE CDbXML::ParseViewpropType(const char *str)
//Returns: enumeration corresponding to string
{
	tokenIter = viewPropMap.find(string(str));
	return tokenIter == viewPropMap.end() ? VP_Invalid : VIEWPROPTYPE(tokenIter->second);
}

//*****************************************************************************
PROPTYPE CDbXML::ParsePropType(const char *str)
//Returns: enumeration corresponding to string
{
	tokenIter = propMap.find(string(str));
	return tokenIter == propMap.end() ? P_Invalid : PROPTYPE(tokenIter->second);
}

//*****************************************************************************
bool CDbXML::WasImportSuccessful()
//Returns: whether the import procedure has only parsed good (correct) data
//that might be added to the database
{
	return
		info.ImportStatus == MID_ImportSuccessful ||
      info.ImportStatus == MID_DemoIgnored ||
//      info.ImportStatus == MID_HoldIgnored || //no data will be added to the DB for these
//      info.ImportStatus == MID_HoldIdenticalIgnored ||
//      info.ImportStatus == MID_HoldNotIdenticalIgnored ||
//      info.ImportStatus == MID_PlayerIgnored ||
		info.ImportStatus == MID_DowngradeHoldPrompt ||
		info.ImportStatus == MID_PlayerSavesIgnored ||
		info.ImportStatus == MID_OverwritePlayerPrompt ||
		info.ImportStatus == MID_OverwriteHoldPrompt;
}

//*****************************************************************************
bool CDbXML::ContinueImport(const MESSAGE_ID status)
//Returns: whether the import procedure should continue the current pass.
//
//It should not continue if the user is being prompted for input
//or if the data to import is being ignored.
{
	//If a diagonstic/error message has been passed in, record it.
	if (status != MID_ImportSuccessful)
		info.ImportStatus = status;

	return
		info.ImportStatus == MID_ImportSuccessful ||
		info.ImportStatus == MID_DemoIgnored ||
		info.ImportStatus == MID_PlayerSavesIgnored;
}

//*****************************************************************************
//Provide hooks to report progress.
void CDbXML::SetCallback(CAttachableObject *pObject) {pCallbackObject = pObject;}
void CDbXML::PerformCallback(long val) {
	if (pCallbackObject)
		pCallbackObject->Callback(val);
}
void CDbXML::PerformCallbackf(float fVal) {
	if (pCallbackObject)
		pCallbackObject->Callbackf(fVal);

	const bool successIgnored = CDbXML::streamingOut.flush(EXPORT_MAX_SIZE_THRESHOLD);
}
void CDbXML::PerformCallbackText(const WCHAR* wpText) {
	if (pCallbackObject)
		pCallbackObject->CallbackText(wpText);
}

//*****************************************************************************
extern "C" void CDbXMLTallyElementCDecl(void * ud, const char * name, const char ** atts) { CDbXML::TallyElement(ud, name, atts); }
void CDbXML::TallyElement(
//Expat callback function
//
//Tally XML start tags.
//
//That is, it is called to count the number of records of each object type that
//are encounted in the XML data payload.
//This number of records will later be pre-allocated for each object type.
//
//Params:
	void* /*userData*/, const char *name, const char ** /*atts*/)
{
	//Use callback for progress indicator, etc.
	if (pCallbackObject)
	{
		const uLongf sizeEstimate = importBuf.compressedSize * 15; //heuristic to show a reasonable prediction on the progress bar
		const float fProgress = XML_GetCurrentByteIndex(parser) / (float)(sizeEstimate);
		PerformCallbackf(fProgress);
	}

	//Get view type.
	const VIEWTYPE vType = ParseViewType(name);
	switch (vType)
	{
		case V_Data: ++CDbXML::info.nData; break;
		case V_Demos: ++CDbXML::info.nDemos; break;
		case V_Holds: ++CDbXML::info.nHolds; break;
		case V_Levels: ++CDbXML::info.nLevels; break;
		case V_Players: ++CDbXML::info.nPlayers; break;
		case V_Rooms: ++CDbXML::info.nRooms; break;
		case V_SavedGames: ++CDbXML::info.nSavedGames; break;
		case V_Speech: ++CDbXML::info.nSpeech; break;
		case V_LocalHighScores: ++CDbXML::info.nHighScore; break;
		default: break;
	}
}

//*****************************************************************************
extern "C" void CDbXMLStartElementCDecl(void * ud, const char * name, const char ** atts) { CDbXML::StartElement(ud, name, atts); }
void CDbXML::StartElement(
//Expat callback function
//
//Process XML start tag, and attributes.
//
//This method is used to populate fields of new database records via the
//polymorphic SetProperty call, defined in each DB sub-class.
//The 'info' object is used to identify record GUIDs, remap foreign keys,
//merge imported data into any pre-existing records, and resolve semantic collisions
//of any pre-existing records, potentially overwriting pre-existing records
//with the field values being imported here.
//
//Params:
	void * /*userData*/, const char *name, const char **atts)
{
	CDbBase *pDbBase;
	int i;

	if (!ContinueImport())
		return;  //ignore the rest

	//Use callback for progress indicator, etc.
	const bool bLanguageMod = info.typeBeingImported == CImportInfo::LanguageMod;
	if (pCallbackObject)
	{
		ASSERT(importBuf.totalParseSize);
		const float fEndPercent = bLanguageMod ? 1.00f : 0.50f;
		float fProgress = (XML_GetCurrentByteIndex(parser) / (float)(importBuf.totalParseSize)) * fEndPercent;
		if (fProgress > fEndPercent) fProgress = fEndPercent;
		PerformCallbackf(fProgress);
	}

	//Get view type.
	const VIEWTYPE vType = ParseViewType(name);
	if (vType != V_Invalid)
	{
		//Create new object (record) to insert into DB.
		pDbBase = GetNewRecord(vType);
		bool bSaveRecord = !bLanguageMod;
		MESSAGE_ID status;

		//Set members.
		for (i = 0; atts[i]; i += 2) {
			const PROPTYPE pType = ParsePropType(atts[i]);
			if (pType == P_Invalid)
			{
				//Invalid tag -- Fail import
				ContinueImport(MID_FileCorrupted);
				delete pDbBase;
				return;
			}
			status = pDbBase->SetProperty(pType, (char* const)atts[i + 1], info, bSaveRecord);
			if (!ContinueImport(status))
			{
				delete pDbBase;
				return;
			}
		}

		SaveRecord.push_back(bSaveRecord);
		dbRecordStack.push_back(pDbBase);
		return;
	}

	//Get subview type.
	const VIEWPROPTYPE vpType = ParseViewpropType(name);
	if (vpType != VP_Invalid)
	{
		//Create object member (sub-record).

		//Notify sub-view that members will be set.
		CDbBase *pBase = dbRecordStack.back();
		MESSAGE_ID status = pBase->SetProperty(vpType, P_Start, NULL, info);
		if (!ContinueImport(status))
			return;

		//Set members.
		for (i = 0; atts[i]; i += 2) {
			const PROPTYPE pType = ParsePropType(atts[i]);
			status = pBase->SetProperty(vpType, pType, (char* const)atts[i + 1], info);
			if (!ContinueImport(status))
				return;
		}

		vpCurrentType.push_back(vpType);
		return;
	}

	//Get property type.  (Format only valid for language modules.)
	if (bLanguageMod)
	{
		const PROPTYPE pType = ParsePropType(name);
		if (pType != P_Invalid)
		{
			//Object properties.
			CDbBase *pBase = dbRecordStack.back();
			MESSAGE_ID status = pBase->SetProperty(pType, atts, info);
			ContinueImport(status); //checks status
			return;
		}
	}

	//Look for root tag.
	if (!strcmp(szDROD,name))
	{
		//Check for version number.
		info.wVersion = 0;
		if (atts[0] && !strcmp(szDRODVersion, atts[0]))
		{
			//Version 2.0+
			info.wVersion = atoi(atts[1]);

			//Don't import data from a version newer than this one.
			if (info.wVersion > VERSION_NUMBER)
				ContinueImport(MID_CantImportLaterVersion);

			if (atts[2] && !strcmp(szDRODHeaderInfo, atts[2]))
			{
				//Version 3.0+
				ASSERT(info.wVersion >= 300);
				info.headerInfo = atts[3];
			}
		}
		return;
	}

	//Invalid tag -- Fail import
	ContinueImport(MID_ImportWrongRootTag);
}

//*****************************************************************************
extern "C" void CDbXMLEndElementCDecl(void * ud, const char * name) { CDbXML::EndElement(ud, name); }
void CDbXML::EndElement(
//Expat callback function
//
//Process XML end tag.
//
//This method is reached when the parser completes reading of all field values
//and sub-tables (subviews) that will be written into a database record.
//At this point, the new record is staged for saving to the database
//via the Update() call.
//
//If an unrecoverable error is encountered at any point in the import process,
//all (pending) database updates made here will be rolled back instead of committed.
//
//Params:
	void* /*userData*/, const char* name)
{
	if (!ContinueImport())
		return;  //ignore the rest

	//Verify matching tags.

	//End of record.
	const VIEWTYPE vType = ParseViewType(name);
	if (vType != V_Invalid)
	{
		//Remove record object from the stack.
		CDbBase *pDbBase = dbRecordStack.back();
		dbRecordStack.pop_back();
		if (!pDbBase)
		{
			//Unmatched tags -- Fail import
			ContinueImport(MID_FileCorrupted);
			return;
		}

		if (SaveRecord.back())
		{
			ASSERT(pDbBase->GetPrimaryKey()); //record should have already been assigned a key if it is being saved to the DB
			VERIFY(pDbBase->Update());  //save any viewprop modifications

			//Keep track of record type+ID for second-pass updates, if needed.
			dbImportedRecordIDs.push_back(pDbBase->GetPrimaryKey());
			dbRecordTypes.push_back(vType);
		}
		delete pDbBase;
		SaveRecord.pop_back();
		return;
	}

	//End of sub-view.
	const VIEWPROPTYPE vpType = ParseViewpropType(name);
	if (vpType != VP_Invalid)
	{
		if (vpType != vpCurrentType.back())
		{
			//Mismatched tags -- Fail import
			ContinueImport(MID_FileCorrupted);
			return;
		}

		const MESSAGE_ID status =
				(dbRecordStack.back())->SetProperty(vpType, P_End, NULL, info);
		if (!ContinueImport(status))
			return;

		vpCurrentType.pop_back();
		return;
	}

	if (info.typeBeingImported == CImportInfo::LanguageMod)
	{
		const PROPTYPE pType = ParsePropType(name);
		if (pType != P_Invalid)
		{
			//End property tag.
			//Currently, do nothing here.
			return;
		}
	}

	if (!strcmp(szDROD,name))  //ignore top-level header
	{
		bImportComplete = true;
		return;
	}

	//Bad tag -- Fail import
	ContinueImport(MID_FileCorrupted);
}

//*****************************************************************************
UINT CDbXML::GetActiveSpeechID()
//Facilitates adding language mods to speech members of data objects.
{
	//Get the active room object.
	//When this is called, it should be the second object from the top (end).
	if (dbRecordStack.size() < 2)
		return 0;

	CDbBase *pBase = dbRecordStack[dbRecordStack.size() - 2];
	CDbRoom *pRoom = dynamic_cast<CDbRoom*>(pBase);
	if (!pRoom)
		return 0;

	return pRoom->GetImportCharacterSpeechID();
}

//
//CDbXML private methods.
//

//*****************************************************************************
bool CDbXML::UpdateLocalIDs()
//For some imported records, old local IDs couldn't be resolved.
//Once all records have been read in, this method is called to transform
//remaining old local IDs to new local IDs.
//
//Returns: whether saved game records were committed to the DB
{
	PrimaryKeyMap::iterator localID;

	CDb::FreezeTimeStamps(true);

	bool bSavedGameUpdated = false;
	BEGIN_DBREFCOUNT_CHECK;
	{
	CDbHold *pHoldForRooms = NULL;	//all imported rooms should be in a single hold
//	UINT dwHoldForRoomsID = 0;

	UINT wIndex, wCount = 1;	//start at 1
	const UINT wNumRecords = dbImportedRecordIDs.size();
	ASSERT(dbRecordTypes.size() == wNumRecords);	//must have a 1:1 correspondence
	for (wIndex=wNumRecords;
			wIndex--; 	//process records in reverse order so
					//owner records are handled first
					//(i.e. first a demo record, and then its owned saved game record)
			++wCount)	//count records processed
	{
		//Process one record.
		const VIEWTYPE vType = dbRecordTypes[wIndex];

		switch (vType)
		{
			case V_Holds:
			{
            CDbHold *pHold = g_pTheDB->Holds.GetByID(dbImportedRecordIDs[wIndex]);
				ASSERT(pHold);

				if (0 == (time_t)pHold->LastUpdated)
				{
					//Implies this is only a hold reference -- it should already
					//exist in the DB and shouldn't be updated.
					ASSERT(!"Trying to Update a hold reference");
					delete pHold;
					continue;
				}

				//Update first level ID.
				localID = info.LevelIDMap.find(pHold->dwLevelID);
				if (localID == info.LevelIDMap.end())
				{
					//Entrance Level ID not found -- Fail import.
					ContinueImport(MID_FileCorrupted);
/*
					//...probably due to a bug caused during 1.6 beta.
					//Repair this hold after levels are handled.
					pHold->dwLevelID = 0;
					info.dwRepairHoldID = pHold->dwHoldID;
*/
				} else {
					pHold->dwLevelID = localID->second;
				}

				UINT wI;
/*
				//Backwards compatibility:
				//1.6 format -- move level entrances into hold's Entrances table
				for (wI=0; wI<info.LevelEntrances.size(); ++wI)
				{
					CEntranceData *pEntrance = info.LevelEntrances[wI];
					ASSERT(pEntrance->bIsMainEntrance);
					pEntrance->dwEntranceID = pHold->Entrances.size() == 0 ? 1 :
							(*(pHold->Entrances.rbegin()))->dwEntranceID + 1;
					pHold->Entrances.push_back(pEntrance);
				}
#ifdef _DEBUG
				dwStartingDbRefCount -= info.LevelEntrances.size(); //ref counting: these message texts get deleted
#endif
				info.LevelEntrances.clear();
*/
				//Update entrances' room IDs.
				const UINT wCount = pHold->Entrances.size();
				for (wI=0; wI<wCount; ++wI)
				{
					localID = info.RoomIDMap.find(pHold->Entrances[wI]->dwRoomID);
					if (localID == info.RoomIDMap.end())
					{
						//ID not found -- Fail import
						ContinueImport(MID_FileCorrupted);
						break;
					}
					pHold->Entrances[wI]->dwRoomID = localID->second;
				}
				pHold->Update();
				delete pHold;
			}
			break;

			case V_Levels:
			{
            CDbLevel *pLevel = g_pTheDB->Levels.GetByID(dbImportedRecordIDs[wIndex]);
				ASSERT(pLevel);

				if (!pLevel->dwPlayerID)
				{
					//Implies this is only a level reference -- it should already
					//exist in the DB and shouldn't be updated.
					ASSERT(!"Trying to update a level reference");
					delete pLevel;
					continue;
				}

				//Provide default level ordering if no order is given.
				if (!pLevel->dwOrderIndex)
				{
					pLevel->dwOrderIndex = pLevel->dwLevelIndex;
					pLevel->Update();
				}
				delete pLevel;
			}
			break;

			case V_Rooms:
/*
				//Backwards compatibility.
				if (info.wVersion < 200)
				{
					CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(dbImportedRecordIDs[wIndex]);
					ASSERT(pRoom);

					if (!pRoom->pszOSquares)
					{
						//Implies this is only a room reference -- it should already
						//exist in the DB and shouldn't be updated.
						delete pRoom;
						continue;
					}

					//pre 2.0 used 8-neighbor yellow door connections
					pRoom->AddDiagonalDoorAssociations();

					//Update exit IDs.
					const UINT wCount = pRoom->Exits.size();
					for (UINT wIndex=0; wIndex<wCount; ++wIndex)
					{
						//Translate level ID into level's main entrance ID.
						CExitData *pExit = pRoom->Exits[wIndex];
						if (pExit->dwEntranceID > DROD1_6EXITFORMAT)
						{
							//Translate level ID into level's main entrance ID.
							pExit->dwEntranceID -= DROD1_6EXITFORMAT;
							localID = info.LevelIDMap.find(pExit->dwEntranceID);
							if (localID == info.LevelIDMap.end())
							{
								//ID not found -- Fail import
								ContinueImport(MID_LevelNotFound);
								break;
							}
							const UINT dwLevelID = localID->second;
							if (!dwHoldForRoomsID)
							{
								//Get hold once.  Exits should only point to levels in one hold.
								//If they don't, they will be reset here.
								dwHoldForRoomsID = g_pTheDB->Levels.GetHoldIDForLevel(dwLevelID);
								ASSERT(!pHoldForRooms);
								pHoldForRooms = g_pTheDB->Holds.GetByID(dwHoldForRoomsID);
								//For beta 1.6 builds, data might be corrupted.
								if (!pHoldForRooms) dwHoldForRoomsID = 0;
							}
							pExit->dwEntranceID = pHoldForRooms ?
									pHoldForRooms->GetMainEntranceIDForLevel(dwLevelID) : 0;
						}
					}
					pRoom->Update();
					delete pRoom;
				}
*/
			break;

			case V_SavedGames:
			{
            CDbSavedGame *pSavedGame = g_pTheDB->SavedGames.GetByID(dbImportedRecordIDs[wIndex]);
				ASSERT(pSavedGame);

				//When importing saved games for merging into current player record,
				//not during a hold upgrade,
				//only keep those saved games in spots that the player doesn't have.
				bool bSkipRecord = false;
				if (info.typeBeingImported == CImportInfo::SavedGame &&
						!info.bImportingSavedGames) //not during a hold upgrade
				{
					//These saved game records should now all belong to the current player.
					ASSERT(pSavedGame->dwPlayerID == g_pTheDB->GetPlayerID());

					switch (pSavedGame->eType)
					{
						case ST_Unknown:
							bSkipRecord = true;	//these types always get skipped
						break;
						case ST_Demo:		//when importing a player over itself, import demo saves as usual
						{
							const UINT dwSavedGameID = CDbSavedGames::GetSavedGameID(
								pSavedGame->dwRoomID, pSavedGame->Created, pSavedGame->dwPlayerID);
							if (dwSavedGameID && dwSavedGameID != pSavedGame->dwSavedGameID)
								bSkipRecord = true;	//this demo saved game already exists -- skip
						}
						break;

						case ST_Manual:
						case ST_EndHold:
						case ST_Continue:
						case ST_Autosave:
						case ST_Quicksave:
							//These types should have already been verified against existing
							//saves when each record was imported.
						break;

						default: break;	//probably include any other types w/o check
					}
					if (bSkipRecord)
					{
						//Shouldn't save this record to the DB.
						//If it was already saved, remove it.
						if (pSavedGame->eType == ST_Demo)
						{
							const UINT dwDemoID = g_pTheDB->Demos.
									GetDemoIDforSavedGameID(pSavedGame->dwSavedGameID);
							if (dwDemoID)	//this demo record was just imported and Updated
								g_pTheDB->Demos.Delete(dwDemoID);
						} else {
							if (pSavedGame->dwSavedGameID)
								g_pTheDB->SavedGames.Delete(pSavedGame->dwSavedGameID);
						}

						delete pSavedGame;
						continue;
					}
				}

				//Handle new ExploredRoom records.
				//for (UINT i=0; i<pSavedGame->ExploredRooms.size; ++i)
				//{
				//	ExploredRoom& room = *(pSavedGame->ExploredRooms[i]);
				//	//do anything here?
				//}

/*
				//Update IDs for ExploredRooms and ConqueredRooms.
				CIDSet temp;
				CIDSet::const_iterator iter;
				for (iter = pSavedGame->ExploredRooms.begin();
						iter != pSavedGame->ExploredRooms.end(); ++iter)
				{
					localID = info.RoomIDMap.find(*iter);
					if (localID == info.RoomIDMap.end())
					{
						//Room ID not mapped -- skip it.
						//(Room no longer exists in revised hold version.
						//Don't include reference to deleted room.)
						continue;
					}
					if (localID->second)
						temp += localID->second;
				}
				pSavedGame->ExploredRooms = temp;

				temp.clear();
				for (iter = pSavedGame->ConqueredRooms.begin();
						iter != pSavedGame->ConqueredRooms.end(); ++iter)
				{
					localID = info.RoomIDMap.find(*iter);
					if (localID == info.RoomIDMap.end())
					{
						//Room ID not mapped -- skip it.
						//(Room no longer exists in revised hold version.
						//Don't include reference to deleted room.)
						continue;
					}
					if (localID->second)
						temp += localID->second;
				}
				pSavedGame->ConqueredRooms = temp;
*/
				//Room ID processing that gets done once all the IDs have been resolved.
				switch (pSavedGame->eType)
				{
/*
					case ST_RoomBegin:
						//Never assume the current room is conquered on first entrance.
						//If the room is really clean, then this will be taken care of
						//when restoring to this saved game.
						pSavedGame->ConqueredRooms -= pSavedGame->dwRoomID;
					break;
*/
					case ST_PlayerTotal:
					{
						const UINT playerID = g_pTheDB->GetPlayerID();
						const UINT dwSavedGameID = g_pTheDB->SavedGames.FindByType(
								ST_PlayerTotal, playerID, false);
						if (dwSavedGameID && dwSavedGameID != pSavedGame->dwSavedGameID)
						{
							//Only one instance of this saved game record type exists
							//per player.  Merge room IDs into the existing record
							//and delete this new one.
							g_pTheDB->SavedGames.AddRoomsToPlayerTally(playerID, pSavedGame->GetExploredRooms());

							if (pSavedGame->dwSavedGameID) //don't keep this one around
								g_pTheDB->SavedGames.Delete(pSavedGame->dwSavedGameID);

							bSavedGameUpdated = true;
							bSkipRecord = true;
						}
					}
					break;
					default: break;
				}

				if (!bSkipRecord)
				{
					if (pSavedGame->Update())
						bSavedGameUpdated = true;	//saved game record saved to DB
				}
				//else: nothing needed to be updated
				delete pSavedGame;
			}
			break;

			case V_Players:
			case V_Demos:
			case V_Data:
			case V_SavedGameMoves:
			case V_Speech:
			case V_LocalHighScores:
				break;   //these types need no fix-ups

			default:
				ASSERT(!"CDbXML::UpdateLocalIDs: Unexpected view type");
			break;
		}

		//Show progress.
		static const float fStartPercent = 0.50f;
		static const float fLocalPercent = 1.0f - fStartPercent;
		const float fProgress = fStartPercent + (wCount/(float)wNumRecords) * fLocalPercent;
		PerformCallbackf(fProgress);
	}

	//Free imported records.
	dbImportedRecordIDs.clear();
	dbRecordTypes.clear();
	delete pHoldForRooms;

/*
	//Attempt to repair corrupted data (from buggy 1.6 beta versions).
	if (info.dwRepairHoldID)
	{
		CDbHold *pHold = g_pTheDB->Holds.GetByID(info.dwRepairHoldID);
		ASSERT(pHold);
		if (pHold->Repair())
		{
			pHold->Update();
		} else {
			info.ImportStatus = MID_LevelNotFound;	//abort import
		}
		delete pHold;
	}
*/
	}
	END_DBREFCOUNT_CHECK;

	CDb::FreezeTimeStamps(false);

	return bSavedGameUpdated;
}

//*****************************************************************************
bool CDbXML::ExportSavedGames(
//Temporarily export all saved games in the hold being upgraded.
//
//This is done on import when a hold is being overwritten and saved games will
//be erased.  These saved games will be re-imported once the new hold version
//has been imported.
//This is the simplest way to save as much saved game progress as possible
//without the import process becoming too complicated.
//
//Returns: true if any saved games were exported, else false
//
//Params:
	const UINT dwHoldID)   //(in) hold to export saved games from
{
	ASSERT(dwHoldID);

	bool bSomethingExported = false;
	CAttachableObject *pSaveCallbackObject = pCallbackObject;   //don't reset this

	//Compile list of all (non-hidden) demo IDs in hold.
	//The demo export will include saved game records attached to demos.
	CIDSet demoIDs = CDb::getDemosInHold(dwHoldID);

	if (ExportXML(V_Demos, demoIDs, info.exportedDemos))
		bSomethingExported = true;
	else info.exportedDemos.resize(0);
	pCallbackObject = pSaveCallbackObject;

	//Compile list of all saved game IDs in hold.
	//The saved game export will exclude saved game records attached to demos.
	CIDSet savedGameIDs = CDb::getSavedGamesInHold(dwHoldID);

	if (ExportXML(V_SavedGames, savedGameIDs, info.exportedSavedGames))
		bSomethingExported = true;
	else info.exportedSavedGames.resize(0);

	pCallbackObject = pSaveCallbackObject;

	//Compile list of all high score IDs for hold
	CIDSet highScoreIDs = CDb::getLocalHighscoresForHold(dwHoldID);
	if (ExportXML(V_LocalHighScores, highScoreIDs, info.exportedHighScores)) {
		bSomethingExported = true;
	} else {
		info.exportedHighScores.resize(0);
	}

	pCallbackObject = pSaveCallbackObject;

	return bSomethingExported;
}

//*****************************************************************************
void CDbXML::ImportSavedGames()
//Re-import temporarily exported demos and saved game records.
//This is done on import when a hold has been overwritten with a newer version
//and attached demo/saved games records were erased.  This will restore all
//demos/saved games that players had for the hold.
/*
	//  Validation checks are performed in VerifySavedGames().
*/
{
	if (info.bImportingSavedGames)
		return; //prevent infinite loop in ImportXML().
	info.bImportingSavedGames = true;

	const CImportInfo::ImportType importType = info.typeBeingImported;
	const MESSAGE_ID importState = info.ImportStatus;
	info.typeBeingImported = CImportInfo::Demo;
	VERIFY(ImportXML(info.exportedDemos) == MID_ImportSuccessful);
	info.exportedDemos.resize(0);

	info.ImportStatus = importState; //ignore import state changes in saved game restoration
	info.typeBeingImported = CImportInfo::SavedGame;
	VERIFY(ImportXML(info.exportedSavedGames) == MID_ImportSuccessful);
	info.exportedSavedGames.resize(0);

	info.ImportStatus = importState; //ignore import state changes in high score restoration
	info.typeBeingImported = CImportInfo::HighScore;
	VERIFY(ImportXML(info.exportedHighScores) == MID_ImportSuccessful);
	info.exportedHighScores.resize(0);

	info.typeBeingImported = importType;
	info.ImportStatus = importState;
	info.bImportingSavedGames = false;
}

//*****************************************************************************
void CDbXML::CleanUp()
{
	importBuf.clear();

	info.Clear(true);
	pCallbackObject = NULL; //release hook
}

//Makes a copy of input data and initiates uncompression
//
//Post-conditions: on successful completion, importBuf.isPopulated() is true
//  and ready to begin handing off data to the XML parser
MESSAGE_ID CDbXML::Uncompress(
	BYTE* buffer, //(in) may be either in zlib or gzip formats
	UINT size)    //(in) size of compressed data in buffer
{
	ASSERT(importBuf.isReset());

	//Allocate a buffer for uncompressing data into
	if (!importBuf.allocUncompressionBuffer())
		return MID_NoMoreMemory;

	//Copy input for iterative uncompression during parsing
	if (!importBuf.copyCompressedData(buffer, size))
	{
		importBuf.clear();
		return MID_NoMoreMemory;
	}

	const UINT mid = importBuf.initStream();
	if (mid == MID_Success) {
		importBuf.totalParseSize += importBuf.uncompressedSize;
		ASSERT(importBuf.isPopulated());
	}

	return mid;
}

//*****************************************************************************
MESSAGE_ID CDbXML::ImportXML(
//Import an XML file into one or more tables.
//
//Params:
	const WCHAR *wszFilename,  //(in)   XML filename, not including extension or path
	const CImportInfo::ImportType type)		//(in)	Type of data being imported
//
//Returns:
//Message ID giving the concluding status of the operation, i.e.
// MID_ImportSuccessful (etc) if it worked,
// MID_* (giving the reason for failure) if not.
//If not successful, then no changes are made to the database.
{
	ASSERT(wszFilename);

	// Read the compressed data stream
	CStretchyBuffer buffer;
	if (!CFiles::ReadFileIntoBuffer(wszFilename, buffer)) {
		info.ImportStatus = MID_FileNotFound;
		return MID_FileNotFound;
	}

	return ImportXML(buffer, type);
}

//*****************************************************************************
MESSAGE_ID CDbXML::ImportXML(
//Import XML data from a buffer.
//Buffer is cleared on successful uncompression.
//
//Params:
	CStretchyBuffer &buffer,   //(in/out) buffer of XML text -> cleared buffer
	const CImportInfo::ImportType type)		//(in)	Type of data being imported
{
	ASSERT(importBuf.isReset());

	if (type == CImportInfo::LanguageMod)
	{
		//Plain text buffer.
		importBuf.totalParseSize = importBuf.uncompressedSize = buffer.Size();
		buffer += (BYTE)0; //null-terminate
		importBuf.uncompressedBuffer = buffer.GetCopy();
		importBuf.buf = (char*)importBuf.uncompressedBuffer;
	} else {
		//Compressed data -- uncompress into buffer.
		BYTE* bytes = (BYTE*)buffer;
		if (strncmp((char*)bytes, gzID, 2) != 0) {
			//Prior zlib format, not gzip -- decode encoded zlib buffer.
			buffer.Decode(); //in-place decoding; 'bytes' remains current
		}

		const MESSAGE_ID result = Uncompress(bytes, buffer.Size());
		if (result != MID_Success) {
			info.ImportStatus = result;
			return result;
		}
	}

	ASSERT(importBuf.isPopulated());

	buffer.Clear(); //don't need any more

	//Set type of data being imported.
	CDbXML::info.typeBeingImported = type;

	return ImportXML();
}

//*****************************************************************************
//Import an XML file into one or more tables
MESSAGE_ID CDbXML::ImportXML(const string& xml) //(in) string of XML text
{
	if (xml.empty())
		return info.ImportStatus;

	Import_Init();

	BEGIN_DBREFCOUNT_CHECK;
	try
	{
		//Parser init.
		InitTokens();

		//Free any parsing job that had started and been interrupted.
		if (parser)
			XML_ParserFree(parser);

		parser = XML_ParserCreate(NULL);

		//First pass through data
		Import_TallyRecords(xml);

		//Second pass through data
		PerformCallback(MID_ImportingData);
		VERIFY(XML_ParserReset(parser, NULL) == XML_TRUE);

		importBuf.closeStream();
		VERIFY(importBuf.initStream() == MID_Success); //reset and reinitialize compression object for second pass

		Import_ParseRecords(xml);

		//Free parser.
		XML_ParserFree(parser);
		parser = NULL;

		Import_Resolve();
	}	//try
	catch (CException&)
	{
		return MID_FileCorrupted;
	}
	END_DBREFCOUNT_CHECK;

	return info.ImportStatus;
}

//*****************************************************************************
MESSAGE_ID CDbXML::ImportXML()
{
	info.ImportStatus = ImportXML(&importBuf);

	//Clean up.
	//If an import is interrupted in the middle by a request for user input,
	//the XML buffer read in from a file will be retained so it can be resumed
	//later without having to read in the file and decompress the data again.
	if (bImportComplete)
		CleanUp();

	return info.ImportStatus;
}

//*****************************************************************************
MESSAGE_ID CDbXML::ImportXML(
//Import an XML file into one or more tables (see above method for notes).
//
//Params:
ImportBuffer* pBuffer)  //(in) buffer of XML text
{
	//IMPORT XML FORMAT
	//
	//The XML file will be legit XML 1.0 data.  Expat is being used to parse it.
	//Details of the expected format are determined by ExportXML().
	//See derived classes of CDbBase for reference.
	//
	//Note that the XML file may contain records for more than one table.  The
	//top-level tag in the hierarchy (under DROD) will indicate which table to
	//insert records into.  For example, the following XML would indicate to
	//insert one record into Players, two records into Saved Games, and
	//two records into Demos (some fields have been left out for brevity):
	//
	// <?xml version="1.0" encoding="ISO8859-1" ?>
	// <DROD>
	//  <Players PlayerID='1' Name='Bubba X' />
	//
	//  <SavedGames SavedGameID='12' Commands='CQAAAExhbmd1Y...' />
	//
	//  <SavedGames SavedGameID='13' Commands='TXVzaWMABAAAAAEAAAA...' />
	//
	//  <Demos DemoID='1' PlayerID='1' SavedGameID='12' />
	//
	//  <Demos DemoID='2' PlayerID='1' SavedGameID='13' />
	// </DROD>
	//
	//The second-level tags, like "PlayerID" and "Name" specify the names of
	//fields that will receive values in the inserted record.  If an
	//XML-specified table (AKA "viewdef") does not exist in the database, or
	//any XML-specified field does not exist in the table, then the import
	//should fail.

	//FAILING THE IMPORT
	//
	//When the import fails, all changes to data should roll back, leaving the
	//database in its initial state.

	//REMAPPING PRIMARY AND FOREIGN KEYS
	//
	//Look back at the previous XML example, and you may see that there is a
	//potential problem with importing the data as-is.  The database probably
	//already has a Players record with a PlayerID of 1.  Same goes for demo IDs
	//and saved game IDs.
	//
	//The new Players record will need a new PlayerID value that is not already
	//used by any record currently in the Players table.  The Demos records need
	//to have their PlayerID values updated to match the new PlayerID value.
	//
	//Terminology:
	//Primary key - A primary key is a field value that uniquely identifies a record
	//              within a table.  Within a table, each record's primary key must be
	//              unique against all the other records.  "PlayerID" is a primary
	//          key within the "Players" table.  "DemoID" is a primary key within
	//              the "Demos" table.
	//Foreign key - A foreign key is a field value that identifies a record from a
	//              foreign table (a table other than the one that the foreign key
	//              appears within).  "PlayerID" is a foreign key within the "Demos"
	//              table.  In the above XML example, it indicates that the player with
	//          PlayerID of 1 is the author of the demos with demo IDs of 1 and 2.
	//          "PlayerID" is a foreign key within the "Demos" table (but not the
	//          "Players" table).  "SavedGameID" is also a foreign key within the
	//          "Demos" table.
	//
	//Metakit has no internal designation for primary or foreign keys.  You can
	//determine primary and foreign keys at run-time using the following rules:
	//
	//1. If the field name ends in "ID" and is the first field in a table, that field
	//   is the primary key for the table.
	//2. If the field name ends in "ID" and is the second or later field in a table,
	//   that field is a foreign key for the table.
	//
	//A table may not have more than one primary key (implied from above rules).  A
	//table does not necessarily have a primary or foreign keys.
	//
	//You can always expect an XML file to contain a complete
	//set of records for the import.  The above XML example is a complete set.
	//It would not be a complete set if the two SavedGames records were left
	//out, because the Demos records would have two meaningless "SavedGameID"
	//foreign keys that are left without corresponding SavedGames records.
	//
	//To construct GUIDs, fields are used to uniquely identify records on import.
	//If a record's GUID matches the GUID of a local record of that type,
	//the data will either be merged (for Players) or replace currently existing
	//data (for Holds, Levels, and Rooms).

	LOGCONTEXT("CDbXML::ImportXML");

	ASSERT(pBuffer);
	if (!pBuffer->uncompressedSize) {
		bImportComplete = true;
		return MID_ImportSuccessful;   //nothing to import
	}
	ASSERT(pBuffer->isPopulated());

	Import_Init();

	BEGIN_DBREFCOUNT_CHECK;
	try
	{
		//Parser init.
		InitTokens();

		//Free any parsing job that had started and been interrupted.
		if (parser)
			XML_ParserFree(parser);

		parser = XML_ParserCreate(NULL);

		//First pass through data
		Import_TallyRecords(pBuffer);

		//Second pass through data
		PerformCallback(MID_ImportingData);
		VERIFY(XML_ParserReset(parser, NULL) == XML_TRUE);

		importBuf.closeStream();
		VERIFY(importBuf.initStream() == MID_Success); //reset and reinitialize compression object for second pass

		Import_ParseRecords(pBuffer);

		importBuf.closeStream();
		VERIFY(importBuf.initStream() == MID_Success);

		//Free parser.
		XML_ParserFree(parser);
		parser = NULL;

		Import_Resolve();
	}	//try
	catch (CException&)
	{
		return MID_FileCorrupted;
	}
	END_DBREFCOUNT_CHECK;

	return info.ImportStatus;
}

//*****************************************************************************
//Ensure everything used for import is reset and ready.
void CDbXML::Import_Init()
{
	ASSERT(dbRecordStack.empty());
	ASSERT(dbImportedRecordIDs.empty());
	ASSERT(dbRecordTypes.empty());
	ASSERT(vpCurrentType.empty());

	//Import init.
	if (!info.bImportingSavedGames)
	{
		info.ImportStatus = MID_ImportSuccessful;
		info.dwPlayerImportedID = info.dwHoldImportedID = info.dwDemoImportedID = 0;
		info.roomStyles.clear();
	}
	CDbRoom::ResetForImport();
	CDbSavedGame::ResetForImport();
	bImportComplete = false;
}

//*****************************************************************************
//Pre-parse the XML payload.
//Determine the upper limit of how many new records will be added to the DB
//and add empty rows for them now to minimize memory address fragmentation during import.
void CDbXML::Import_TallyRecords(
	ImportBuffer* pBuffer)  //(in) buffer of XML text
{
	ASSERT(parser); //pre-condition: parser is inited and ready to parse new data
	ASSERT(pBuffer);

	if (CDbXML::info.bPreparsed ||
		CDbXML::info.typeBeingImported == CImportInfo::LanguageMod) //only adding text records
		return;

	PerformCallback(MID_ImportTallying);
	//This callback method is invoked by the XML parser for each record
	//encountered in the data payload.
	//It is used to tally the number of records existing of each type.
	//See the functions's comments for more detail.
	XML_SetElementHandler(parser, CDbXMLTallyElementCDecl, NULL);

	//Parse the XML in chunks to keep memory overhead low.
	while (info.ImportStatus == MID_ImportSuccessful)
	{
		//Allocate (next) buffer for parsing.
		void* buff = XML_GetBuffer(parser, XML_PARSER_BUFF_SIZE);
		if (!buff)
		{
			info.ImportStatus = MID_NoMoreMemory;
			break;
		}

		const int bytesRead = pBuffer->uncompressedSize;
		ASSERT(bytesRead >= 0);
		ASSERT(bytesRead <= XML_PARSER_BUFF_SIZE);
		memcpy(buff, pBuffer->buf, bytesRead);

		//Parse data chunk.
		if (XML_ParseBuffer(parser, bytesRead, bytesRead == 0) == XML_STATUS_ERROR)
		{
			//Some problem occurred.
			info.ImportStatus = MID_FileCorrupted;

			char errorStr[256];
			_snprintf(errorStr, 256,
				"Import Parse Error: %s at line %u:%u\n",
				XML_ErrorString(XML_GetErrorCode(parser)),
				(UINT)XML_GetCurrentLineNumber(parser),
				(UINT)XML_GetCurrentColumnNumber(parser));
			CFiles Files;
			Files.AppendErrorLog((char*)errorStr);
		}

		if (bytesRead == 0) //done
		{
			AddRowsForPendingRecords();
			CDbXML::info.bPreparsed = true;
			break;
		}

		//Get next chunk of data to parse.
		if (pBuffer->d_stream) {
			//uncompress more data until input is exhausted
			const UINT mid = pBuffer->uncompressChunk();
			if (mid != MID_Success) {
				info.ImportStatus = mid;
				break;
			}

			pBuffer->totalParseSize += pBuffer->uncompressedSize; //sum during first data pass
		}
		else if (bImportComplete) {
			//we aren't decompressing data and we hit an end tag
			//therefore, we must not have anything else to parse
			break;
		}
	}
}

//*****************************************************************************
//Pre-parse the XML payload.
//Determine the upper limit of how many new records will be added to the DB
//and add empty rows for them now to minimize memory address fragmentation during import.
void CDbXML::Import_TallyRecords(const string& xml) //[in] string containg xml to parse
{
	ASSERT(parser); //pre-condition: parser is inited and ready to parse new data

	if (CDbXML::info.bPreparsed ||
		CDbXML::info.typeBeingImported == CImportInfo::LanguageMod) //only adding text records
		return;

	PerformCallback(MID_ImportTallying);
	//This callback method is invoked by the XML parser for each record
	//encountered in the data payload.
	//It is used to tally the number of records existing of each type.
	//See the functions's comments for more detail.
	XML_SetElementHandler(parser, CDbXMLTallyElementCDecl, NULL);

	const char* pXml = xml.c_str();
	size_t remainingSize = xml.length();

	//Parse the XML in chunks to keep memory overhead low.
	while (info.ImportStatus == MID_ImportSuccessful)
	{
		//Allocate (next) buffer for parsing.
		void* buff = XML_GetBuffer(parser, XML_PARSER_BUFF_SIZE);
		if (!buff)
		{
			info.ImportStatus = MID_NoMoreMemory;
			break;
		}

		const int bytesRead = min((int)remainingSize, XML_PARSER_BUFF_SIZE);
		ASSERT(bytesRead >= 0);
		ASSERT(bytesRead <= XML_PARSER_BUFF_SIZE);
		memcpy(buff, pXml, bytesRead);

		//Parse data chunk.
		if (XML_ParseBuffer(parser, bytesRead, bytesRead == 0) == XML_STATUS_ERROR)
		{
			//Some problem occurred.
			info.ImportStatus = MID_FileCorrupted;

			char errorStr[256];
			_snprintf(errorStr, 256,
				"Import Parse Error: %s at line %u:%u\n",
				XML_ErrorString(XML_GetErrorCode(parser)),
				(UINT)XML_GetCurrentLineNumber(parser),
				(UINT)XML_GetCurrentColumnNumber(parser));
			CFiles Files;
			Files.AppendErrorLog((char*)errorStr);
		}

		if (bytesRead == 0) //done
		{
			AddRowsForPendingRecords();
			CDbXML::info.bPreparsed = true;
			break;
		}

		//Advance pointer by amount of data that has been parsed
		pXml += bytesRead;
		remainingSize -= bytesRead;
	}
}

//*****************************************************************************
//Parse the XML data payload, populating database records and staging them
//for commit at the end of parsing.
void CDbXML::Import_ParseRecords(
	ImportBuffer* pBuffer)
{
	ASSERT(parser); //pre-condition: parser is inited and ready to parse new data

	//These callback methods are invoked as XML records and attributes are parsed.
	//Internal database records will be constructed incrementally as these
	//values are read in. The 'EndElement' callback is used to indicate the
	//completion of parsing a record, at which point it is staged for write
	//to the database.  Read their in-place comments for more detail.
	XML_SetElementHandler(parser, CDbXMLStartElementCDecl, CDbXMLEndElementCDecl);

	CDb::FreezeTimeStamps(true);

	while (ContinueImport())
	{
		//Allocate buffer for parsing.
		void* buff = XML_GetBuffer(parser, XML_PARSER_BUFF_SIZE);
		if (!buff) {
			if (!bImportComplete)
				info.ImportStatus = MID_NoMoreMemory;
			break;
		}

		//Get next chunk of data to parse.
		const int bytesRead = pBuffer->uncompressedSize;
		ASSERT(bytesRead >= 0);
		ASSERT(bytesRead <= XML_PARSER_BUFF_SIZE);
		memcpy(buff, pBuffer->buf, bytesRead);

		//Parse data chunk.
		if (XML_ParseBuffer(parser, bytesRead, bytesRead == 0) == XML_STATUS_ERROR)
		{
			//Invalidate import only if data was corrupted somewhere before the end tag.
			//If something afterwards happens to be wrong, just ignore it.
			if (!bImportComplete)
				info.ImportStatus = MID_FileCorrupted;

			char errorStr[256];
			_snprintf(errorStr, 256,
				"Import Parse Error: %s at line %u:%u\n",
				XML_ErrorString(XML_GetErrorCode(parser)),
				(UINT)XML_GetCurrentLineNumber(parser),
				(UINT)XML_GetCurrentColumnNumber(parser));
			CFiles Files;
			Files.AppendErrorLog((char*)errorStr);
		}

		if (bytesRead == 0)
			break; //done

		if (pBuffer->d_stream) {
			//uncompress more data until input is exhausted
			const UINT mid = pBuffer->uncompressChunk();
			if (mid != MID_Success) {
				info.ImportStatus = mid;
				break;
			}
		}
		else if (bImportComplete) {
			//we aren't decompressing data and we hit an end tag
			//therefore, we must not have anything else to parse
			break;
		}
	}

	CDb::FreezeTimeStamps(false);
}

//*****************************************************************************
//Parse the XML data payload, populating database records and staging them
//for commit at the end of parsing.
void CDbXML::Import_ParseRecords(const string& xml) //[in] string containg xml to parse
{
	ASSERT(parser); //pre-condition: parser is inited and ready to parse new data

	//These callback methods are invoked as XML records and attributes are parsed.
	//Internal database records will be constructed incrementally as these
	//values are read in. The 'EndElement' callback is used to indicate the
	//completion of parsing a record, at which point it is staged for write
	//to the database.  Read their in-place comments for more detail.
	XML_SetElementHandler(parser, CDbXMLStartElementCDecl, CDbXMLEndElementCDecl);

	CDb::FreezeTimeStamps(true);

	const char* pXml = xml.c_str();
	size_t remainingSize = xml.length();

	while (ContinueImport()) {
		//Allocate buffer for parsing.
		void* buff = XML_GetBuffer(parser, XML_PARSER_BUFF_SIZE);
		if (!buff) {
			if (!bImportComplete)
				info.ImportStatus = MID_NoMoreMemory;
			break;
		}

		//Get next chunk of data to parse.
		const int bytesRead = min((int)remainingSize, XML_PARSER_BUFF_SIZE);
		ASSERT(bytesRead >= 0);
		ASSERT(bytesRead <= XML_PARSER_BUFF_SIZE);
		memcpy(buff, pXml, bytesRead);

		//Parse data chunk.
		if (XML_ParseBuffer(parser, bytesRead, bytesRead == 0) == XML_STATUS_ERROR)
		{
			//Invalidate import only if data was corrupted somewhere before the end tag.
			//If something afterwards happens to be wrong, just ignore it.
			if (!bImportComplete)
				info.ImportStatus = MID_FileCorrupted;

			char errorStr[256];
			_snprintf(errorStr, 256,
				"Import Parse Error: %s at line %u:%u\n",
				XML_ErrorString(XML_GetErrorCode(parser)),
				(UINT)XML_GetCurrentLineNumber(parser),
				(UINT)XML_GetCurrentColumnNumber(parser));
			CFiles Files;
			Files.AppendErrorLog((char*)errorStr);
		}

		if (bytesRead == 0)
			break; //done

		//Advance pointer by amount of data that has been parsed
		pXml += bytesRead;
		remainingSize -= bytesRead;
	}

	CDb::FreezeTimeStamps(false);
}

//*****************************************************************************
//Following parsing of the import data payload,
//assess what happened, resolve pending foreign key updates,
//verify data integrity and freshness,
//integrate new data with pre-existing data, as appropriate,
//and tear down and reset all parsing structures.
void CDbXML::Import_Resolve()
{
	//Confirm something was actually imported.  If not, mention this.
	if (ContinueImport() && !info.bImportingSavedGames)
		switch (info.typeBeingImported)
		{
		case CImportInfo::Demo:
		case CImportInfo::SavedGame:
		case CImportInfo::HighScore:
			break;
		case CImportInfo::Hold:
			if (info.dwHoldImportedID == 0)
				ContinueImport(MID_HoldIgnored);
			break;
		case CImportInfo::Player:
			if (info.dwPlayerImportedID == 0)
				ContinueImport(MID_PlayerIgnored);
			break;
		case CImportInfo::LanguageMod: break;
		default: break;
		}

	if (WasImportSuccessful())
	{
		//Only save data if we're not exiting the import to prompt the user
		//on what to do.
		if (ContinueImport())
		{
			//Now that all the records have been read in, we can
			//update all remaining old local keys (IDs) to new local keys.
			const bool bSavedGamesUpdated = UpdateLocalIDs();

			if (ContinueImport())	//UpdateLocalIDs() might have marked a data corruption
			{
/*
				if (info.localHoldIDs.size() > 0 &&
					bSavedGamesUpdated &&	//only check saved game records if some
					//records were actually saved to DB
					//don't verify after both demo and saved game re-import:
					//only do it once both types are imported
					(!info.bImportingSavedGames || info.typeBeingImported == CImportInfo::SavedGame))
					VerifySavedGames(); //performs internal Commits
*/
				g_pTheDB->Commit();
			}
		}
		g_pTheDB->ResetMembership();
	}
	if (!WasImportSuccessful())   //check again, in case something was flagged on the final pass
	{
		bImportComplete = true; //Import is impossible or ignored -- won't perform another pass
		g_pTheDB->Rollback(); //remove records added since last commit

		//Free any remaining DB objects.
		UINT wIndex;
		for (wIndex = dbRecordStack.size(); wIndex--; )
			delete dbRecordStack[wIndex];
		dbRecordStack.clear();
		dbImportedRecordIDs.clear();
		dbRecordTypes.clear();
		vpCurrentType.clear();
	}

	//Clean up.
	ASSERT(dbRecordStack.empty());
	ASSERT(dbImportedRecordIDs.empty());
	ASSERT(dbRecordTypes.empty());
	ASSERT(vpCurrentType.empty());

	if (bImportComplete)
	{
		if (WasImportSuccessful()) //don't need to reimport any peripheral data if main import failed
		{
			if (info.bReplaceOldHolds)
				g_pTheDB->SavedGames.UpdatePlayerTallies(info);
			info.Clear(true);		//only partial clear before saved game import
			ImportSavedGames();
		}

		//Output any log notes user might find useful.
		if (!info.userMessages.empty())
		{
			CFiles f;
			WSTRING wstrLog;
			AsciiToUnicode(".log", wstrLog);
			WSTRING wstrLogFilename = f.GetDatPath();
			wstrLogFilename += wszSlash;
			wstrLogFilename += CFiles::wGameName;
			wstrLogFilename += wstrLog;

			info.userMessages += wszCRLF;
			const string strLog = UnicodeToUTF8(info.userMessages);
			const CStretchyBuffer text(strLog);
			if (f.WriteBufferToFile(wstrLogFilename.c_str(), text, true))
				info.userMessages.resize(0);
		}
	}

	info.Clear(!bImportComplete || info.bImportingSavedGames);
}

//*****************************************************************************
bool CDbXML::ExportXML(
//Export a table record to an XML file.
//
//Params:
	const VIEWTYPE vType,  //(in)   Table to export.
	const UINT dwPrimaryKey,  //(in)   Key to look up in that table.
	const WCHAR *wszFilename)  //(in)   XML path + filename.
//
//Returns:
//True if export was successful, false if not.
{
	return ExportXML(vType, CIDSet(dwPrimaryKey), wszFilename);
}

#ifndef ZLIB_VERNUM
	// older versions of zlib, such as the system-provided one on Mac OS X,
	// do not provide compressBound.
size_t compressBound(size_t sourceLength) {
	return size_t((1.01 * sourceLength) + 13);
}
#endif

//*****************************************************************************
bool CDbXML::ExportXML(
//Export a table record to an XML file.
//
//Params:
	const VIEWTYPE vType,  //(in)   Table to export.
	const CIDSet& primaryKeys, //(in)   Key to look up in that table.
	const WCHAR *wszFilename)  //(in)   XML path + filename.
//
//Returns:
//True if export was successful, false if not.
{
	//Generate data for export.
	bool bRes=false;
	BYTE *dest = NULL;
	ULONG destLen = 0;
	{
		// Compress the data in gzip format (previously, zlib format w/ stretchy buffer encoding).
#ifdef WIN32
		gzFile gzf = gzopen_w(wszFilename, "wb");
#else
		const string filename = UnicodeToUTF8(wszFilename);
		gzFile gzf = gzopen(filename.c_str(), "wb");
#endif

		string text; // only in scope until compressed
		CDbXML::streamingOut.set(&text, &gzf);
		if (!ExportXML(vType, primaryKeys, text)) {
			CDbXML::streamingOut.reset();
			return false;
		}
		g_pTheDB->Close(); //reset memory used by DB during export lookups
		g_pTheDB->Open();

		const bool success = CDbXML::streamingOut.flush();

		const int closeval = gzclose(gzf);
		CDbXML::streamingOut.reset();
		bRes = success && closeval == 0;
	}

	return bRes;
}

//*****************************************************************************
bool CDbXML::ExportXML(
//Export a table record to an XML file.
//
//Params:
	const VIEWTYPE vType,		//(in)   Table view to export.
	const CIDSet& primaryKeys, //(in)   Key to look up in that table.
	string &text,              //(out)  Data being exported.
	const UINT eSaveType)		//(in) Export only a specific saved game type (during player export) [default=0]
//
//Returns:
//True if export was successful, false if not.  If false, the export file will not
//be present.
{
	BEGIN_DBREFCOUNT_CHECK;
	bool bSomethingExported = false;
	try {

	CDbRefs dbRefs(vType, primaryKeys, eSaveType);
	CDbXML::exportInfo.clear();

	text = getXMLheader(&(CDbXML::info.headerInfo));
	text.reserve(1000000); //speed optimization
	const UINT headerSize = text.size();

	UINT wCount = 0, wTotal = primaryKeys.size();
	for (CIDSet::const_iterator iter = primaryKeys.begin(); iter != primaryKeys.end(); ++iter, ++wCount)
	{
		const UINT dwPrimaryKey = *iter;
		c4_View DBView;	//DB table view
		const UINT dwIndex = LookupRowByPrimaryKey(dwPrimaryKey, vType, DBView);
		if (dwIndex == ROW_NO_MATCH) {ASSERT(!"Row lookup failed."); continue;}

		switch (vType)
		{
			//All record types that can be exported independently are here.
			case V_Data:
				g_pTheDB->Data.ExportXML(dwPrimaryKey, dbRefs, text);
				CDbXML::PerformCallbackf(wCount/(float)wTotal);
				break;
			case V_Demos:
				g_pTheDB->Demos.ExportXML(dwPrimaryKey, dbRefs, text);
				CDbXML::PerformCallbackf(wCount/(float)wTotal);
				break;
			case V_Holds:
				g_pTheDB->Holds.ExportXML(dwPrimaryKey, dbRefs, text);
				break;
			case V_Players:
				g_pTheDB->Players.ExportXML(dwPrimaryKey, dbRefs, text);
				break;
			case V_SavedGames:
				g_pTheDB->SavedGames.ExportXML(dwPrimaryKey, dbRefs, text);
				CDbXML::PerformCallbackf(wCount/(float)wTotal);
				break;
			case V_LocalHighScores:
				g_pTheDB->HighScores.ExportXML(dwPrimaryKey, dbRefs, text);
			break;

			default:
				ASSERT(!"CDbXML::ExportXML: Unexpected view type.");
				return false;
		}
	}
	pCallbackObject = NULL; //release hook
	bSomethingExported = (text.size() > headerSize);

	text += getXMLfooter();

	}	//try
	catch (CException&)
	{
		return false;
	}
	
	//Reset one-use export fields.
//	CDbXML::info.bQuickPlayerExport = false;
	CDbXML::info.headerInfo.resize(0);

	END_DBREFCOUNT_CHECK;

	return bSomethingExported;
}

//*****************************************************************************
string CDbXML::getXMLheader(const string *pString) //[default=NULL]
//Returns: XML header, top-level tag required by Expat, and optional header info.
{
	string text = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?>" NEWLINE;
	text += "<";
	text += szDROD;
	text += " ";
	text += szDRODVersion;
	text += "='";
	char temp[10];
	text += _itoa(VERSION_NUMBER, temp, 10);
	if (pString && !pString->empty()) //include optional header info
	{
		text += "' ";
		text += szDRODHeaderInfo;
		text += "='";
		text += *pString;
	}
	text += "'>" NEWLINE;
	return text;
}

//*****************************************************************************
string CDbXML::getXMLfooter()
//Returns: top level end tag required by Expat
{
	string text = "</";
	text += szDROD;
	text += ">" NEWLINE;
	return text;
}

//*****************************************************************************
bool CDbXML::z_compress(
//Calls zlib compress API.
//
//Returns: whether operation succeeded
//
//Params:
	const BYTE* src, const ULONG srcSize, //(in) data to compress
	BYTE*& dest, ULONG& destLen)          //(out) allocated buffer with compressed data and its size
	                                      //      buffer must be deleted by caller
{
	destLen = compressBound(srcSize);
	dest = new BYTE[destLen];
	uLongf dSize = (uLongf)destLen;
	const int res = compress(dest, &dSize, src, srcSize);
	destLen = dSize;
	return res == Z_OK;
}

//*****************************************************************************
MESSAGE_ID CDbXML::z_uncompress(
//Calls zlib uncompress API.
//
//Returns: MID_Success on success, or MID for type of error encountered on failure
//
//Params:
	ULONG& decodedSize,   //(in/out) # of bytes in uncompressed data.
	                      //       Set to 0 on error.
	BYTE*& decodedBuf,    //(out) pointer to buffer for uncompressed data, or NULL on error.
	                      //       Will be realloced if not large enough
	                      //       Must be deleted by caller.
	const BYTE* inBuffer, //(in) contains compressed data
	const UINT inBufferSize) //(in) # of bytes of compressed data
{
	ASSERT(inBufferSize);
	if (!decodedSize) //if not provided...
		decodedSize = static_cast<ULONG>(inBufferSize * 1.6);  //start with a reasonably-sized buffer

	//Attempt uncompression until we have a large enough buffer to hold it.
	if (!decodedBuf) //if not already allocated
		decodedBuf = new BYTE[decodedSize+1]; //allow null-termination
	if (!decodedBuf)
	{
		decodedSize = 0;
		return MID_OutOfMemory;
	}

	int res;
	do {
		uLongf dSize = (uLongf)decodedSize;
		res = uncompress(decodedBuf, &dSize, inBuffer, inBufferSize);
		decodedSize = dSize;
		switch (res)
		{
			case Z_BUF_ERROR:
				//This wasn't enough memory to decode the data to,
				//so double the buffer size.
				decodedSize *= 2;
				delete[] decodedBuf;
				decodedBuf = new BYTE[decodedSize+1]; //allow null-termination
				if (!decodedBuf)
				{
					decodedSize = 0;
					return MID_OutOfMemory;
				}
				break;

			case Z_DATA_ERROR:
				delete[] decodedBuf;
				decodedBuf = NULL;
				decodedSize = 0;
				return MID_FileCorrupted;

			case Z_MEM_ERROR:
				delete[] decodedBuf;
				decodedBuf = NULL;
				decodedSize = 0;
				return MID_OutOfMemory;
		}
	} while (res != Z_OK);  //success

	//Upon successful completion, decodedSize is the actual size of the uncompressed buffer.

	return MID_Success;
}

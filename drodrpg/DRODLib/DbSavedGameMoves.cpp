// $Id: DbSavedGameMoves.cpp 9179 2008-08-29 13:26:56Z mrimer $

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

#include "DbSavedGameMoves.h"

#include "DbXML.h"
#include "GameConstants.h"
#include <BackEndLib/Base64.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Ports.h>

#define CURRENT_VERSION (2)

//
//CDbSavedGameMove public methods.
//

//*****************************************************************************
void CDbSavedGameMove::Append(
//Adds command data (sans timing info) to MoveSequence.
//Call this whenever a room is exited.
//
//Params:
	CDbCommands& commands,   //(in) commands to append
	const bool bEndOfSequence) //[default=false] set to true if no more commands will be added
{
	UINT wX, wY;
	for (CDbCommands::const_iterator comIter = commands.GetFirst();
			comIter != commands.end(); comIter = commands.GetNext())
	{
		const UINT command = comIter->command;
		this->MoveSequence += command;
		if (commands.GetData(wX,wY))	//get optional command parameters
		{
			this->MoveSequence += wX;
			this->MoveSequence += wY;
		}
	}

	//Mark the point at which the room is exited or sequence is ended.
	this->MoveSequence += (bEndOfSequence ? (UINT)CMD_ENDMOVE : (UINT)CMD_EXITROOM);
}

//*****************************************************************************
void CDbSavedGameMove::AppendWorldMapCommand(UINT wEntrance, ExitType exitType)
//Adds command data for world map interaction
{
	this->MoveSequence += CMD_WORLD_MAP;
	this->MoveSequence += wEntrance;
	this->MoveSequence += (UINT)exitType;
}

//*****************************************************************************
bool CDbSavedGameMove::Load(
//Load move sequence (previous to current room) for the indicated saved game.
//
//Params:
	const UINT loadSavedGameID, //(in)
	const bool /*bQuick*/) //[default=false]
{
	this->MoveSequence.Clear();

	//Find record.  It has the same savedGameID as its owning record,
	//as they have a 1:1 relation.
	c4_View SavedGameMovesView;
	const UINT dwSavedGameMovesI = LookupRowByPrimaryKey(loadSavedGameID,
			V_SavedGameMoves, SavedGameMovesView);
	if (dwSavedGameMovesI == ROW_NO_MATCH)
		return false; //before version 1.1 doesn't have this data
	c4_RowRef row = SavedGameMovesView[dwSavedGameMovesI];

	//Load in props from SavedGameMoves record.
	this->savedGameID = loadSavedGameID;
	this->version = (UINT)(p_Version(row));
	if (this->version > CURRENT_VERSION)
		return false; //unsupported

	c4_Bytes commandBytes = p_Commands(row);
	const UINT numInBytes = commandBytes.Size();
	if (!numInBytes)
		return true; //nothing else needed

	//Unpack data.
	ULONG decodedSize = (ULONG)(p_NumBytes(row));
	const BYTE *bytes = commandBytes.Contents();
	if (!decodedSize)
	{
		//0 size field indicates data were not packed, so don't need to uncompress.
		this->MoveSequence.Set(bytes, numInBytes);
		return true; //done
	}

	BYTE *decodedBuf = NULL; //called method will allocate memory
	MESSAGE_ID ret = CDbXML::z_uncompress(decodedSize, decodedBuf, bytes, numInBytes);
	if (ret != MID_Success)
	{
		ASSERT(!decodedBuf);
		ASSERT(!"Can't unpack move data");
		throw CException("CDbSavedGame::LoadMoveSequence");
	}

	//Success.
	ASSERT(decodedSize); //if set to 0, there was an error in uncompression
	this->MoveSequence.Set(decodedBuf, decodedSize);
	delete[] decodedBuf;

	//Upgrade older data to new version.
	if (this->version == 1)
	{
		ConvertDataVersion_1_to_2(this->MoveSequence);
		this->version = CURRENT_VERSION;
	}

	return true;
}

//*****************************************************************************
MESSAGE_ID CDbSavedGameMove::SetProperty(
//Used during XML data import.
//According to pType, convert string to proper datatype and member
//
//Returns: whether operation was successful
//
//Params:
	const PROPTYPE pType,   //(in) property (data member) to set
	char* const str,        //(in) string representation of value
	CImportInfo &info,      //(in/out) Import data
	bool &bSaveRecord)      //(out) whether record should be saved
{
	static PrimaryKeyMap::iterator localID;
	switch (pType)
	{
		//SavedGame field is parsed last.
		//By the time we get here, we should have all info needed to save the new record to the DB.
		case P_SavedGameID:
			this->savedGameID = convertToUINT(str);
			if (!this->savedGameID)
				return MID_FileCorrupted;  //corrupt data

			//Look up local ID for owning saved game record.
			//It should have just been imported.
			localID = info.SavedGameIDMap.find(this->savedGameID);
			if (localID == info.SavedGameIDMap.end())
				return MID_FileCorrupted;

			this->savedGameID = localID->second;
			if (!this->savedGameID)
			{
				//Records for this saved game are being ignored.  Don't save this record.
				bSaveRecord = false;
			}
			if (bSaveRecord)
			{
				//Add this new record to the DB.
				Update();
			}
			break;
		case P_Version:
			this->version = convertToUINT(str);
			break;
		case P_Commands:
		{
			BYTE *data = NULL;
			const UINT size = Base64::decode(str,data);
			if (size)
				this->MoveSequence.Set(data, size);
			delete[] data;
		}
		break;
		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
bool CDbSavedGameMove::Update()
//Load move sequence previous to current room for this saved game.
//
//Returns: whether operation succeeded
{
	//Find whether a moves record for this savedGameID already exists
	ASSERT(IsOpen());
	c4_View SavedGameMovesView;
	UINT savedGameI = LookupRowByPrimaryKey(this->savedGameID,
			V_SavedGameMoves, SavedGameMovesView);

	//If not, add one and get its row index.
	if (savedGameI == ROW_NO_MATCH)
	{
		c4_RowRef newRow = g_pTheDB->SavedGameMoves.GetNewRow();
		p_SavedGameID(newRow) = this->savedGameID;
		g_pTheDB->SavedGameMoves.SortLastRow(); //rows must remain in sorted order so lookup by primary key will work

		savedGameI = LookupRowByPrimaryKey(this->savedGameID,
			V_SavedGameMoves, SavedGameMovesView);
		ASSERT(savedGameI != ROW_NO_MATCH);
	}

	c4_RowRef row = SavedGameMovesView[savedGameI];
	p_Version(row) = this->version;

	//Populate move sequence fields.
	BYTE *packedBuf = NULL;
	ULONG packedSize = 0;

	if (!this->MoveSequence.empty())
	{
		if (!this->MoveSequence.Compress(packedBuf, packedSize))
		{
			//Store unpacked data on failure.
			c4_Bytes bytes((BYTE*)this->MoveSequence, MoveSequence.Size());
			p_Commands(row) = bytes;
			p_NumBytes(row) = 0; //0 signals data is not packed
			return false;
		}
	}
	c4_Bytes bytes(packedBuf, packedSize);
	p_Commands(row) = bytes;
	p_NumBytes(row) = packedSize;

	delete[] packedBuf;

	return true;
}

//*******************************************************************************
void CDbSavedGameMoves::Delete(
//Deletes a saved game moves record.
//NOTE: Don't call this directly to delete a moves object belonging to a saved game.
//Instead call CDbSavedGames::Delete() to delete the saved game and associated moves record.
//
//Params:
	const UINT savedGameID) //(in)   Saved game parent to delete for.
{
	ASSERT(savedGameID);

	c4_View SavedGameMovesView;
	const UINT savedGameMovesRowI = LookupRowByPrimaryKey(savedGameID,
			V_SavedGameMoves, SavedGameMovesView);
	if (savedGameMovesRowI==ROW_NO_MATCH)
		return; //before version 1.1, these records didn't exist

	SavedGameMovesView.RemoveAt(savedGameMovesRowI);

	//After object is deleted, membership might change, so reset the flag.
	this->bIsMembershipLoaded = false;
}

//*****************************************************************************
void CDbSavedGameMoves::ExportXML(
//Returns: string containing XML text describing the move sequence for the saved game with this ID
//
//Pre-condition: dwSavedGameID is valid
//
//Params:
	const UINT savedGameID, //(in)
	CDbRefs& /*dbRefs*/,        //(in/out)
	string &str,            //(in/out)
	const bool /*bRef*/)    //(in)
{
#define STARTTAG(vType,pType) "<"; str += ViewTypeStr(vType); str += " "; str += PropTypeStr(pType); str += "='"
#define PROPTAG(pType) "' "; str += PropTypeStr(pType); str += "='"
#define ENDTAG(vType) "</"; str += ViewTypeStr(vType); str += ">\n"
#define CLOSETAG "'/>\n"
#define INT32TOSTR(val) writeInt32(dummy, sizeof(dummy), (val))

	CDbSavedGameMove *pSavedGameMoves = GetByID(savedGameID);
	if (!pSavedGameMoves)
		return; //saves before version 1.1 didn't have this data

	//Prepare data.
	char dummy[32];

	str += STARTTAG(V_SavedGameMoves, P_Version);
	str += INT32TOSTR(pSavedGameMoves->version);
	str += PROPTAG(P_Commands);
	str += Base64::encode((BYTE*)pSavedGameMoves->MoveSequence, pSavedGameMoves->MoveSequence.Size());
	str += PROPTAG(P_SavedGameID);
	str += INT32TOSTR(pSavedGameMoves->savedGameID); //put last
	str += CLOSETAG;

	delete pSavedGameMoves;

#undef STARTTAG
#undef PROPTAG
#undef ENDTAG
#undef CLOSETAG
#undef INT32TOSTR
}

//
// Protected methods.
//

//*****************************************************************************
CDbSavedGameMove::CDbSavedGameMove(bool /*bClear*/) //[default=true]
	: savedGameID(0)
	, version(CURRENT_VERSION) //current format version
{}

//
// Private methods.
//

//*****************************************************************************
void CDbSavedGameMove::ConvertDataVersion_1_to_2(CStretchyBuffer& buffer) //(in/out)
//Change BYTE command entries to UINTs.
{
	CStretchyBuffer newBuf;
	const UINT oldBufSize = buffer.Size();
	newBuf.Realloc(oldBufSize * 3); //guessing a pretty close value w/o going over

	UINT index=0;
	while (index<oldBufSize)
	{
		//Convert 1 byte --> UINT
		const BYTE byt = buffer[index++];
		newBuf += static_cast<UINT>(byt);

		if (bIsComplexCommand(static_cast<int>(byt)))
		{
			//Keep complex command params as 2 UINTs.
			for (UINT i=8; i--; ) //copy 8 bytes directly
				newBuf += buffer[index++];
		}
	}

	buffer = newBuf;
}

//*****************************************************************************
bool CDbSavedGameMove::SetMembers(const CDbSavedGameMove &that)
{
	this->savedGameID = that.savedGameID;
	this->version = that.version;
	this->MoveSequence = that.MoveSequence;
	return true;
}

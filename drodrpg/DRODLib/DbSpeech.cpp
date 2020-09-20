// $Id: DbSpeech.cpp 10113 2012-04-22 05:40:36Z mrimer $

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

#include "DbSpeech.h"
#include "Db.h"
#include "DbXML.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Base64.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Ports.h>

//*******************************************************************************
void CDbSpeeches::Delete(
//Deletes a speech record.
//
//Params:
	const UINT dwSpeechID) //(in)   Speech to delete.
{
	ASSERT(dwSpeechID);

	c4_View SpeechView;
	const UINT dwSpeechRowI = LookupRowByPrimaryKey(dwSpeechID, V_Speech, SpeechView);
	if (dwSpeechRowI==ROW_NO_MATCH)
		return; //row for this speech ID was already deleted

	c4_RowRef row = SpeechView[dwSpeechRowI];
	CDbBase::DirtyHold();

	//Remove any data record tied to speech record.
	const UINT dwTextMID = (UINT) (p_MessageID(row));
	if (dwTextMID)
		DeleteMessage(static_cast<MESSAGE_ID>(dwTextMID));
	const UINT dwDataID = (UINT) (p_DataID(row));
	if (dwDataID)
		g_pTheDB->Data.Delete(dwDataID);

	SpeechView.RemoveAt(dwSpeechRowI);

	//After object is deleted, membership might change, so reset the flag.
	this->bIsMembershipLoaded = false;
}

//*****************************************************************************
#define STARTTAG(vType,pType) "<"; str += ViewTypeStr(vType); str += " "; str += PropTypeStr(pType); str += "='"
#define PROPTAG(pType) "' "; str += PropTypeStr(pType); str += "='"
#define CLOSETAG "'/>" NEWLINE
#define CLOSESTARTTAG "'>" NEWLINE
#define ENDTAG(vType) "</"; str += ViewTypeStr(vType); str += ">" NEWLINE
#define EXPORTTEXT(pType, messageText) messageText.ExportText(str, PropTypeStr(pType))
#define INT32TOSTR(val) writeInt32(dummy, sizeof(dummy), (val))

//*****************************************************************************
bool CDbSpeeches::ExportText(const UINT dwSpeechID, CDbRefs &dbRefs, CStretchyBuffer &str)
//Serialize a hold reference, its texts, and the texts of every child object.
{
	if (dbRefs.IsSet(V_Speech,dwSpeechID))
		return false;

	dbRefs.Set(V_Speech,dwSpeechID);

	CDbSpeech *pSpeech = GetByID(dwSpeechID);
	ASSERT(pSpeech);
	if (!pSpeech)
		return false; //shouldn't happen -- but this is more robust

	//GID (empty).
	str += "<";
	str += ViewTypeStr(V_Speech);
	str += ">" NEWLINE;

	EXPORTTEXT(P_Message, pSpeech->MessageText);

	str += ENDTAG(V_Speech);

	delete pSpeech;

	return true;
}

//*****************************************************************************
void CDbSpeeches::ExportXML(
//Returns: string containing XML text describing Speech with this ID
//
//Pre-condition: dwSpeechID is valid
//
//Params:
	const UINT dwSpeechID, //(in)
	CDbRefs &dbRefs,        //(in/out)
	string &str,            //(in/out)
	const bool bRef)    //(in) Only export GUID reference (default = false)
{
	if (dbRefs.IsSet(V_Speech,dwSpeechID))
		return;

	dbRefs.Set(V_Speech,dwSpeechID);

	CDbSpeech *pSpeech = GetByID(dwSpeechID);
	ASSERT(pSpeech);
	if (!pSpeech)
		return; //shouldn't happen -- but this is more robust

	//Include corresponding sound data with speech.
	const bool bDataExists = g_pTheDB->Data.Exists(pSpeech->dwDataID);
	if (bDataExists)
		g_pTheDB->Data.ExportXML(pSpeech->dwDataID, dbRefs, str, bRef);

	//Prepare data.
	WSTRING const wTextStr = (WSTRING)pSpeech->MessageText;
	char dummy[32];

	str += STARTTAG(V_Speech, P_Character);
	str += INT32TOSTR(pSpeech->wCharacter);
	str += PROPTAG(P_Mood);
	str += INT32TOSTR(pSpeech->wMood);
	str += PROPTAG(P_Message);
	str += Base64::encode(wTextStr);
	str += PROPTAG(P_Delay);
	str += INT32TOSTR(pSpeech->dwDelay);
	//Only save attached data if it's not a dangling pointer.
	if (bDataExists)
	{
		str += PROPTAG(P_DataID);
		str += INT32TOSTR(pSpeech->dwDataID);
	}
	//Put primary key last, so all fields have been set by the
	//time Update() is called during import.
	str += PROPTAG(P_SpeechID);
	str += INT32TOSTR(pSpeech->dwSpeechID);
	str += CLOSETAG;

	delete pSpeech;
}
#undef STARTTAG
#undef PROPTAG
#undef CLOSETAG
#undef CLOSESTARTTAG
#undef ENDTAG
#undef EXPORTTEXT
#undef INT32TOSTR

//
//CDbSpeech methods
//

//*****************************************************************************
CDbSpeech::CDbSpeech(const CDbSpeech& that, const bool bReplicateData)  //[false]
	: CDbBase()
	, wCharacter(that.wCharacter)
	, wMood(that.wMood)
	, dwDelay(that.dwDelay)
	, dwDataID(that.dwDataID)
	, pSound(NULL) //this object is never copied in-game where pSound is used
	, bDirtySound(that.bDirtySound)
{
	if (that.pSound && !that.dwDataID)
	{
		//Sound record isn't stored in DB -- make copy of it (memory intensive).
		this->pSound = new CDbDatum(*that.pSound);
	}

	if (!bReplicateData)
	{
		this->dwSpeechID = that.dwSpeechID;
		//Use same message ID, if any.
		if (that.MessageText.GetMessageID())
			this->MessageText.Bind(that.MessageText.GetMessageID());
	} else {
		this->dwSpeechID = 0;
	}
	//Copy text.
	this->MessageText = that.MessageText;
}

//*****************************************************************************
void CDbSpeech::Clear()
//Clears members of object.
{
	this->bPartialLoad = false;

	this->dwSpeechID = this->dwDataID = this->dwDelay = 0L;
	this->wCharacter = this->wMood = 0;
	this->MessageText.Clear();

	UnloadSound();
	this->bDirtySound = false;
}

//*****************************************************************************
bool CDbSpeech::Load(
//Loads an speech record from database into this object.
//
//Params:
	const UINT dwSpeechID, //(in) SpeechID of speech to load.
	const bool /*bQuick*/) //(in) [default=false] load only quick data members
//
//Returns:
//True if successful, false if not.
{
	try {
		Clear();

		//Find record with matching Speech ID.
		ASSERT(IsOpen());
		c4_View SpeechView;
		const UINT dwSpeechI = LookupRowByPrimaryKey(dwSpeechID, V_Speech,
				SpeechView);
		if (dwSpeechI == ROW_NO_MATCH) throw CException("CDbSpeech::Load");
		c4_RowRef row = SpeechView[dwSpeechI];

		//Load in props from Speech record.
		this->dwSpeechID = (UINT) (p_SpeechID(row));
		this->wCharacter = (UINT) (p_Character(row));
		this->wMood = (UINT) (p_Mood(row));
		this->dwDataID = (UINT) (p_DataID(row));
		const UINT dwTextID = (UINT) (p_MessageID(row));
		if (dwTextID)
			this->MessageText.Bind(dwTextID);
		this->dwDelay = (UINT) (p_Delay(row));
	}
	catch (CException&)
	{
		Clear();
		return false;
	}
	return true;
}

//*****************************************************************************
bool CDbSpeech::Update()
//Updates database with this data.
{
	if (this->bPartialLoad)
	{
		ASSERT(false); //don't try to update partially-loaded records
		return false;
	}

	CDbBase::DirtyHold();
	if (this->dwSpeechID == 0)
		//Insert a new speech record.
		return UpdateNew();

	//Update existing data.
	return UpdateExisting();
}

//*****************************************************************************
bool CDbSpeech::UpdateNew()
//Insert a new Speech record.
{
	LOGCONTEXT("CDbSpeech::UpdateNew");
	ASSERT(this->dwSpeechID == 0);
	ASSERT(IsOpen());

	this->dwSpeechID = GetIncrementedID(p_SpeechID);

	//Save text.
	const UINT dwMessageMID = this->MessageText.UpdateText();
	ASSERT(dwMessageMID);

	//Save new sound clip, if set.
	if (this->pSound)
	{
		ASSERT(!this->dwDataID);   //no sound data should be associated with this speech object yet
		ASSERT(this->bDirtySound); //dirty bit for sound should be set
		if (this->pSound->Update())
			this->dwDataID = this->pSound->dwDataID;
	}

	//Add record.
	c4_RowRef row = g_pTheDB->Speech.GetNewRow();
	p_SpeechID(row) = this->dwSpeechID;
	p_Character(row) = this->wCharacter;
	p_Mood(row) = this->wMood;
	p_DataID(row) = this->dwDataID;
	p_MessageID(row) = dwMessageMID;
	p_Delay(row) = this->dwDelay;

	return true;
}

//*****************************************************************************
bool CDbSpeech::UpdateExisting()
//Update an existing Speech record.
{
	LOGCONTEXT("CDbSpeech::UpdateExisting");
	ASSERT(this->dwSpeechID != 0);
	ASSERT(IsOpen());

	//Lookup speech record.
	c4_View SpeechView;
	const UINT dwSpeechI = LookupRowByPrimaryKey(this->dwSpeechID,
			V_Speech, SpeechView);
	if (dwSpeechI == ROW_NO_MATCH)
	{
		//This previously saved speech row was removed -- readd it.
		this->dwSpeechID = 0;
		if (!this->pSound)     //...and if no sound object is attached at this point,
			this->dwDataID = 0; //this value should be reset
		return UpdateNew();
	}

	//Save text.
	const UINT dwMessageMID = this->MessageText.UpdateText();
	ASSERT(dwMessageMID);

	//Save sound clip, if set.
	if (this->pSound && this->bDirtySound)
		if (this->pSound->Update())
		{
			this->dwDataID = this->pSound->dwDataID;
			this->bDirtySound = false;
		}

	//Update record.
	c4_RowRef row = SpeechView[dwSpeechI];
	p_SpeechID(row) = this->dwSpeechID;
	p_Character(row) = this->wCharacter;
	p_Mood(row) = this->wMood;
	p_DataID(row) = this->dwDataID;
	p_MessageID(row) = dwMessageMID;
	p_Delay(row) = this->dwDelay;

	return true;
}

//*****************************************************************************
MESSAGE_ID CDbSpeech::SetProperty(
//Used during XML import of language modules.
	const PROPTYPE pType,
	const char** atts,
	CImportInfo &/*info*/)
{
	switch (pType)
	{
		case P_Message:
		{
			//Determine which speech object is being considered.
			if (!this->dwSpeechID)
			{
				const UINT dwActiveSpeechID = CDbXML::GetActiveSpeechID();
				if (!dwActiveSpeechID)
					return MID_FileCorrupted; //speech message can't be placed
				Load(dwActiveSpeechID);
			}
			this->MessageText.ImportText(atts);
		}
		break;
		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
MESSAGE_ID CDbSpeech::SetProperty(
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
	static PrimaryKeyMap::iterator localID;
	switch (pType)
	{
		case P_SpeechID:
			this->dwSpeechID = convertToUINT(str);
			if (!this->dwSpeechID)
				return MID_FileCorrupted;  //corrupt file

			//Look up local ID.
			localID = info.SpeechIDMap.find(this->dwSpeechID);
			if (localID != info.SpeechIDMap.end())
				//Error - this data should not have been imported already.
				return MID_FileCorrupted;

			if (bSaveRecord)
			{
				//Add a new speech (ignore old local ID).
				const UINT dwOldLocalID = this->dwSpeechID;
				this->dwSpeechID = 0;
				Update();
				info.SpeechIDMap[dwOldLocalID] = this->dwSpeechID;
			} else {
				//This data is being ignored.
				info.SpeechIDMap[this->dwSpeechID] = 0;   //skip records with refs to this speech ID
			}
			break;

		case P_Character:
			this->wCharacter = convertToUINT(str);
			break;
		case P_Mood:
			this->wMood = convertToUINT(str);
			break;
		case P_DataID:
			this->dwDataID = convertToUINT(str);
			if (this->dwDataID)
			{
				//Set to local ID.
				localID = info.DataIDMap.find(this->dwDataID);
				if (localID == info.DataIDMap.end())
					return MID_FileCorrupted;  //record should have been loaded already
				this->dwDataID = localID->second;
			}
			break;
		case P_Message:
		{
			WSTRING text;
			Base64::decode(str,text);
			this->MessageText = text.c_str();
			break;
		}
		case P_Delay:
			this->dwDelay = convertToUINT(str);
			break;
		default:
			return MID_FileCorrupted;
	}

	return MID_ImportSuccessful;
}

//*****************************************************************************
const CDbDatum* CDbSpeech::GetSound()
//Returns: const pointer to sound data object
{
	if (!this->pSound && this->dwDataID)
		this->pSound = g_pTheDB->Data.GetByID(this->dwDataID);
	return this->pSound;
}

//*****************************************************************************
void CDbSpeech::SetSound(
//Sets or replaces the owned sound clip.
//Doesn't delete any old sound clip from the DB.
//
//Params:
	CDbDatum *pSound, const bool bDeleteSound)  //[default=true]
{
	this->bDirtySound = true;  //dirty sound even if pointer is identical
	if (this->pSound == pSound) return; //same object -- nothing to do
	if (bDeleteSound)
		delete this->pSound;
	this->pSound = pSound;
	this->dwDataID = pSound ? pSound->dwDataID : 0;
}

//*****************************************************************************
void CDbSpeech::UnloadSound()
//Deletes any allocated sound object.
//Doesn't change member info so sound may be loaded again.
{
	delete this->pSound;
	this->pSound = NULL;
}


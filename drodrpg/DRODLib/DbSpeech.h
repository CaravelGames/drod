// $Id: DbSpeech.h 8310 2007-10-13 20:44:34Z mrimer $

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

//DbSpeech.h
//Declarations for CDbSpeeches and CDbSpeech.
//Classes for accessing scripted speech commands from database.

#ifndef DBSPEECH_H
#define DBSPEECH_H

#include "DbVDInterface.h"
#include "DbData.h"

//*****************************************************************************
class CDb;
class CDbSpeech;
class CDbSpeeches : public CDbVDInterface<CDbSpeech>
{
protected:
	friend class CDb;

	CDbSpeeches()
		: CDbVDInterface<CDbSpeech>(V_Speech, p_SpeechID)
	{}

public:
	void            Delete(const UINT dwSpeechID);
	virtual bool    ExportText(const UINT dwSpeechID, CDbRefs &dbRefs, CStretchyBuffer &str);
	virtual void    ExportXML(const UINT dwSpeechID, CDbRefs &dbRefs, string &str, const bool bRef=false);
};

//*****************************************************************************
class CDbSpeech : public CDbBase
{
protected:
	friend class CDbSpeeches;
	friend class CDbVDInterface<CDbSpeech>;
	CDbSpeech()
		: CDbBase()
		, dwSpeechID(0)
		, pSound(NULL)
	{
		Clear();
	}

public:
	CDbSpeech(const CDbSpeech& that, const bool bReplicateData=false);
	virtual ~CDbSpeech()
	{
		delete this->pSound;
	}

	UINT        GetPrimaryKey() const {return this->dwSpeechID;}
	bool        Load(const UINT dwSpeechID, const bool bQuick=false);
	virtual MESSAGE_ID SetProperty(const PROPTYPE pType, const char** atts,
			CImportInfo &info);
	virtual MESSAGE_ID SetProperty(const PROPTYPE pType, char* const str,
			CImportInfo &info, bool &bSaveRecord);
	virtual bool   Update();

	const CDbDatum*   GetSound();
	void        SetSound(CDbDatum *pSound, const bool bDeleteSound=true);
	void        UnloadSound();

	UINT       dwSpeechID;
	UINT        wCharacter; //character type giving speech
	UINT        wMood;      //character mood
	UINT       dwDelay;    //delay after script ends (ms)
	UINT       dwDataID;   //sound clip reference
	CDbMessageText MessageText;   //text of sound clip

private:
   void        Clear();
	bool        UpdateNew();
	bool        UpdateExisting();

	CDbDatum   *pSound;     //optionally loaded sound clip w/ dwDataID
	bool        bDirtySound;     //whether attached sound has changed
};

#endif //...#ifndef DBSPEECH_H


// $Id: DbSavedGameMoves.h 9179 2008-08-29 13:26:56Z mrimer $

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

//DbSavedGameMoves.h
//Declarations for CDbSavedGameMoves and CDbSavedGameMove.
//Classes for accessing saved game move sequence data from database.

#ifndef DBSAVEDGAMEMOVES_H
#define DBSAVEDGAMEMOVES_H

#include "DbVDInterface.h"
#include "DbCommands.h"

//*****************************************************************************
//ver. 1.1+
//Objects of this type are owned by objects of type CDbSavedGame.
//There is a 1:1 relation.
class CDbSavedGameMove : public CDbBase
{
protected:
	friend class CDbSavedGame;
	friend class CDbSavedGameMoves;
	friend class CDbVDInterface<CDbSavedGameMove>;

	CDbSavedGameMove(bool bClear=true);

public:
	CDbSavedGameMove(CDbSavedGameMove &Src) : CDbBase() {SetMembers(Src);}
	CDbSavedGameMove &operator= (const CDbSavedGameMove &Src) {
		SetMembers(Src);
		return *this;
	}
	virtual ~CDbSavedGameMove() {}

	void Append(CDbCommands& commands, const bool bEndOfSequence=false);
	void clear() {this->MoveSequence.Clear();}
	bool empty() const {return this->MoveSequence.empty();}
	const CStretchyBuffer& getMoves() const {return this->MoveSequence;}

	bool Load(const UINT loadSavedGameID, const bool bQuick=false);

	virtual UINT GetPrimaryKey() const {return this->savedGameID;}
	virtual MESSAGE_ID   SetProperty(const PROPTYPE pType, char* const str,
			CImportInfo &info, bool &bSaveRecord);
	virtual bool Update();

private:
	static void ConvertDataVersion_1_to_2(CStretchyBuffer& oldVersionBuf);
	bool SetMembers(const CDbSavedGameMove &that);

	UINT savedGameID; //of owning record
	UINT version;
	CStretchyBuffer MoveSequence;
};

//*****************************************************************************
class CDbSavedGameMoves : public CDbVDInterface<CDbSavedGameMove>
{
protected:
	friend class CDb;
	CDbSavedGameMoves()
		: CDbVDInterface<CDbSavedGameMove>(V_SavedGameMoves, p_SavedGameID)
	{}

public:
	virtual void Delete(const UINT savedGameID);
	virtual void ExportXML(const UINT dwVDID, CDbRefs &dbRefs, string &str, const bool bRef=false);
};

#endif //...#ifndef DBSAVEDGAMEMOVES_H

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
 * Portions created by the Initial Developer are Copyright (C) 2025
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//DbLocalHighScores.h
//Declarations for DbLocalHighScores and DbLocalHighScore.
//Classes for accessing local highscore data from database.

#ifndef DB_LOCALHIGHSCORES
#define DB_LOCALHIGHSCORES

#include "DbVDInterface.h"
#include "ImportInfo.h"

//*****************************************************************************
class CDbLocalHighScore : public CDbBase {
protected:
	friend class CDbVDInterface<CDbLocalHighScore>;
	CDbLocalHighScore();

public:
	CDbLocalHighScore(CDbLocalHighScore& Src) : CDbBase() { SetMembers(Src); }
	CDbLocalHighScore& operator= (const CDbLocalHighScore& Src) {
		SetMembers(Src);
		return *this;
	}

	UINT GetPrimaryKey() const { return this->dwHighScoreID; }

	bool Load(const UINT dwLocalHighScoreID, const bool bQuick = false);
	virtual MESSAGE_ID SetProperty(const PROPTYPE pType, char* const str,
		CImportInfo& info, bool& bSaveRecord);

	virtual bool Update();

	UINT dwHighScoreID;
	UINT dwHoldID;
	UINT dwPlayerID;
	int  score;
	WSTRING scorepointName;
	CDbPackedVars stats;

private:
	void Clear();

	UINT GetLocalID() const;

	void SetMembers(const CDbLocalHighScore& src);

	bool UpdateExisting();
	bool UpdateNew();
};

//*****************************************************************************
class CDbLocalHighScores : public CDbVDInterface<CDbLocalHighScore> {
protected:
	friend class CDb;
	CDbLocalHighScores()
		: CDbVDInterface<CDbLocalHighScore>(V_LocalHighScores, p_HighScoreID),
		dwFilterByHoldID(0), dwFilterByPlayerID(0)
	{}

public:

	virtual void Delete(UINT dwLocalHighScoreID);

	virtual void ExportXML(
		const UINT dwLocalHighScoreID,CDbRefs& dbRefs, string& str, const bool bRef = false);
	void FilterByHold(UINT dwFilterByHoldID);
	void FilterByPlayer(UINT dwFilterByPlayerID);
	UINT GetIDForScorepoint(WSTRING scorepointName);
	bool HasScorepoint(WSTRING scorepointName);

private:
	virtual void LoadMembership();

	UINT dwFilterByHoldID;
	UINT dwFilterByPlayerID;

	std::multimap<WSTRING, UINT> scorepointNameToID;
};

#endif // #ifndef DB_LOCALHIGHSCORES

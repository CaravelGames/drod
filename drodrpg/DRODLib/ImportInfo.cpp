// $Id: ImportInfo.cpp 8650 2008-02-23 17:29:55Z mrimer $

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

#include "ImportInfo.h"
#include "DbHolds.h"
#include "../Texts/MIDs.h"

//*****************************************************************************
CImportInfo::CImportInfo()
	: bPreparsed(false)
	, nData(0), nDemos(0), nHolds(0), nLevels(0), nPlayers(0), nRooms(0), nSavedGames(0), nSpeech(0)
	, bReplaceOldPlayers(false), bReplaceOldHolds(false)
	, bImportingSavedGames(false)
//	, bQuickPlayerExport(false)
	, typeBeingImported(NoImport)
	, bAllowHoldUpgrade(true), bAllowHoldDowngrade(false)
	, dwPlayerImportedID(0), dwHoldImportedID(0), dwDemoImportedID(0), importedSavedGameID(0), dwDataImportedID(0)
	, ImportStatus(MID_ImportSuccessful)
	, wVersion(0)
//	, pEntrance(NULL)
	, pImportEntrance(NULL)
{}

CImportInfo::~CImportInfo() {
//	delete pEntrance;
	delete pImportEntrance;
}

//*****************************************************************************
void CImportInfo::Clear(
//
//Params:
	const bool bPartialClear)  //set this to true when import was
										//interrupted and will be performed again
{
	this->DataIDMap.clear();
	this->DemoIDMap.clear();
	this->EntranceIDMap.clear();
	this->HoldIDMap.clear();
	this->LevelIDMap.clear();
	this->PlayerIDMap.clear();
	this->RoomIDMap.clear();
	this->SavedGameIDMap.clear();
	this->SpeechIDMap.clear();

	this->nData = this->nDemos = this->nHolds = this->nLevels =
		this->nPlayers = this->nRooms = this->nSavedGames = this->nSpeech = 0;

	this->bAllowHoldUpgrade = true;
	this->bAllowHoldDowngrade = false;

	//Leave the following intact for query following import
	//this->dwPlayerImportedID = this->dwHoldImportedID = this->dwDemoImportedID = this->importedSavedGameID = this->dwDataImportedID = 0;
	//this->bImportingSavedGames = false;
	//this->roomStyles.clear();

	if (!bPartialClear)
	{
		this->bPreparsed = false;

		this->bReplaceOldHolds = false;
		this->bReplaceOldPlayers = false;
//		this->bQuickPlayerExport = false;
		this->typeBeingImported = NoImport;

		this->localHoldIDs.clear();
		this->exportedDemos.resize(0);
		this->exportedSavedGames.resize(0);
	}

	//Backwards compatibility.
/*
	//1.6
	for (UINT wIndex=this->LevelEntrances.size(); wIndex--; )
		delete this->LevelEntrances[wIndex];
	this->LevelEntrances.clear();
	this->dwRepairHoldID = 0;
*/
	this->wVersion = 0;

	//Used while importing.
	importVar.clear();
	importChar.clear();
//	delete pEntrance;
//	pEntrance = NULL;
	delete pImportEntrance;
	pImportEntrance = NULL;
}


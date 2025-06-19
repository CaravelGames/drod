// $Id: DbPlayers.h 8310 2007-10-13 20:44:34Z mrimer $

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

//DbPlayers.h
//Declarations for CDbPlayers and CDbPlayer.
//Classes for accessing player data from database.

#ifndef DBPLAYERS_H
#define DBPLAYERS_H

#include "DbVDInterface.h"
#include "DbPackedVars.h"
#include <BackEndLib/Date.h>

//*****************************************************************************
class CDb;
class CDbPlayer;
class CDbPlayers : public CDbVDInterface<CDbPlayer>
{
protected:
	friend class CDb;

	CDbPlayers()
		: CDbVDInterface<CDbPlayer>(V_Players, p_PlayerID)
	{}

public:
	virtual void   Delete(const UINT dwPlayerID, const bool bRetainRef=true);
	static bool    Exists(const UINT playerID);
	virtual bool   ExportText(const UINT dwPlayerID, CDbRefs &dbRefs, CStretchyBuffer &str);
	virtual void   ExportXML(const UINT dwPlayerID, CDbRefs &dbRefs, string &str, const bool bRef=false);
	void     FilterByLocal(const bool bVar=true);
	static UINT    FindByName(const WCHAR *pwczName);
	static CDbPackedVars GetSettings(const UINT dwPlayerID);
	static bool    IsLocal(const UINT dwPlayerID);

private:
	virtual void    LoadMembership();

	bool     bFilterByLocal;
};

//*****************************************************************************
class CDbPlayer : public CDbBase
{
protected:
	friend class CDbPlayers;
	friend class CDbVDInterface<CDbPlayer>;
	CDbPlayer();

public:
	UINT        GetPrimaryKey() const {return this->dwPlayerID;}
	bool        Load(const UINT dwLoadPlayerID, const bool bQuick=false);
	virtual MESSAGE_ID   SetProperty(const PROPTYPE pType, char* const str,
			CImportInfo &info, bool &bSaveRecord);
	virtual bool   Update();

	UINT       dwPlayerID;
	bool        bIsLocal;
	CDbMessageText NameText;
	CDbMessageText CNetNameText;
	CDbMessageText CNetPasswordText;
	CDbPackedVars  Settings;

	//GUID fields.  Don't modify these fields externally.
	CDbMessageText OriginalNameText;
	CDate       Created, LastUpdated;

	static void ConvertInputSettings(CDbPackedVars& settings);

private:
	CIDSet      GetLocalIDs();
	bool        UpdateExisting();
	bool        UpdateNew();
	void        Clear();

	void        UpgradeKeyDefintions();
};

#endif //...#ifndef DBPLAYERS_H


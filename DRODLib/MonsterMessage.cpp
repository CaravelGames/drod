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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//MonsterMessage.cpp

#include "MonsterMessage.h"
#include "Db.h"

//*****************************************************************************
CMonsterMessage::CMonsterMessage(MONSTER_MESSAGE_TYPE eSetType,
		MESSAGE_ID eSetMessageID, CMonster *pSetSender)
	: CAttachableObject()
	, pSender(pSetSender)
	, eMessageID(eSetMessageID)
	, message()
	, eType(eSetType)
{}

//*****************************************************************************
CMonsterMessage::CMonsterMessage(MONSTER_MESSAGE_TYPE eSetType,
		const WCHAR* pwczText, CMonster *pSetSender)
	: CAttachableObject()
	, pSender(pSetSender)
	, eMessageID(0)
	, message()
	, eType(eSetType)
{
	ASSERT(pwczText);
	this->message = pwczText;
}

//*****************************************************************************
CMonsterMessage::CMonsterMessage(const CMonsterMessage &Src) 
	: CAttachableObject()
	, pSender(Src.pSender)
	, eMessageID(Src.eMessageID)
	, message(Src.message)
	, eType(Src.eType)
{}

//*****************************************************************************
const WCHAR* CMonsterMessage::GetMessageText() const
{
	return this->eMessageID != (MESSAGE_ID)0 ?
		g_pTheDB->GetMessageText(this->eMessageID) : this->message.c_str();
}

//$Log: MonsterMessage.cpp,v $
//Revision 1.3  2005/05/10 04:57:32  mrimer
//Removed old logs.

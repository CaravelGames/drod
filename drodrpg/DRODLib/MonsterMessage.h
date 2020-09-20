// $Id: MonsterMessage.h 8102 2007-08-15 14:55:40Z trick $

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
 *
 * ***** END LICENSE BLOCK ***** */

//MonsterMessage.h
//Declarations for CMonsterMessage. 
//Class for holding monster message data.

#ifndef MONSTERMESSAGE_H
#define MONSTERMESSAGE_H

#include <BackEndLib/AttachableObject.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/MessageIDs.h>

//*****************************************************************************
enum MONSTER_MESSAGE_TYPE
{
	MMT_OK,
	MMT_YESNO,
	MMT_MENU
};

//*****************************************************************************
class CMonster;
class CMonsterMessage : public CAttachableObject
{
public:
	CMonsterMessage(const MONSTER_MESSAGE_TYPE eSetType, const MESSAGE_ID eSetMessageID,
			CMonster *pSetSender);
	CMonsterMessage(const MONSTER_MESSAGE_TYPE eSetType, const WCHAR* pwczText,
			CMonster *pSetSender);
	CMonsterMessage(const CMonsterMessage &Src);

	const WCHAR* GetMessageText() const;

	CMonster *            pSender;      //Monster that sent the message.
	MESSAGE_ID            eMessageID;   //Message sent.
	WSTRING					 message;
	MONSTER_MESSAGE_TYPE  eType;        //Type of message.
};

#endif //...#ifndef MONSTERMESSAGE_H


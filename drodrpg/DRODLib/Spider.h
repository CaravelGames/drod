// $Id: Spider.h 9202 2008-09-08 00:15:38Z mrimer $

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

//Spider.h
//Declarations for CSpider.
//Class for handling spider monster game logic.

#ifndef SPIDER_H
#define SPIDER_H

#include "Monster.h"
#include "MonsterFactory.h"

class CSpider : public CMonsterFacesTarget
{
public:
	CSpider(CCurrentGame *pSetCurrentGame = NULL) : CMonsterFacesTarget(M_SPIDER, pSetCurrentGame) {}
	IMPLEMENT_CLONE_REPLICATE(CMonster, CSpider);
};

#endif //...#ifndef SPIDER_H


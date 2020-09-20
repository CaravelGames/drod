// $Id: RedSerpent.h 8102 2007-08-15 14:55:40Z trick $

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

#ifndef REDSERPENT_H
#define REDSERPENT_H

#include "Serpent.h"

class CRedSerpent : public CSerpent
{
public:
	CRedSerpent(CCurrentGame *pSetCurrentGame = NULL);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CRedSerpent);

//	virtual bool OnStabbed(CCueEvents &/*CueEvents*/, const UINT /*wX*/=(UINT)-1, const UINT /*wY*/=(UINT)-1);
	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
};

#endif //...#ifndef REDSERPENT_H


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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996, 
 * 1997, 2000, 2001, 2002, 2005, 2012 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef TEMPORALCLONE_H
#define TEMPORALCLONE_H

#include "Mimic.h"
#include <queue>

class CTemporalClone : public CMimic
{
public:
	CTemporalClone(CCurrentGame *pSetCurrentGame = NULL);
	IMPLEMENT_CLONE_REPLICATE(CMonster, CTemporalClone);

	virtual bool CanDropTrapdoor(const UINT oTile) const;
	virtual UINT GetIdentity() const;
	virtual bool IsAttackableTarget() const;
	virtual bool IsFlying() const;
	virtual bool IsHiding() const;
	virtual bool IsMonsterTarget() const;
	virtual bool IsTarget() const;
	virtual void Process(const int nLastCommand, CCueEvents &CueEvents);
	virtual void PushInDirection(int dx, int dy, bool bStun, CCueEvents &CueEvents);
	virtual bool SetWeaponSheathed();
	void SetMovementType();
	virtual void Stun(CCueEvents &CueEvents, UINT val=1);
	virtual bool CanMoveOntoTunnelAt(UINT /*col*/, UINT /*row*/) const { return true; }

	int getNextCommand() const;
	void InputCommands(const vector<int>& commands);

	//player stats on creation
	UINT wIdentity;
	bool bInvisible;
	bool bIsTarget;

private:
	bool CanLightFuses() const;

	std::deque<int> commands;
};
	
#endif //...#ifndef TEMPORALCLONE_H

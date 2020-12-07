// $Id: Combat.h 9526 2011-01-05 13:10:01Z TFMurphy $

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
 * Portions created by the Initial Developer are Copyright (C) 2006
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef COMBAT_H
#define COMBAT_H

#include "MonsterFactory.h"
#include "PlayerStats.h"
#include <BackEndLib/AttachableObject.h>

extern const UINT MON_HP[MONSTER_TYPES];
extern const UINT MON_ATK[MONSTER_TYPES];
extern const UINT MON_DEF[MONSTER_TYPES];
extern const UINT MON_GOLD[MONSTER_TYPES];


enum CombatEffectType {
	CET_NODAMAGE,  //attack was blocked
	CET_HARM,      //entity hurt by attack
	CET_HEAL,      //entity healed
	CET_GOLD,      //entity received gold
	CET_XP,        //entity received XP
	CET_ATK,       //entity's attack power changed
	CET_DEF,       //entity's defense power changed
	CET_STRONGHIT, //entity was hit hard
	CET_NODEFENSEHIT, //entity was hit with a DEF-nullifying attack
	CET_YKEY,      //entity's YKEYs was affected
	CET_GKEY,      //entity's GKEYs was affected
	CET_BKEY,      //entity's BKEYs was affected
	CET_SKEY       //entity's SKEYs was affected
};

struct CombatInfo
{
	CombatInfo(
			CMonster* pMonster, const UINT wFromX, const UINT wFromY,
			const UINT wX, const UINT wY, const bool bPlayerHitsFirst,
			const bool bDefeatToStabTarTile)
		: pMonster(pMonster), wFromX(wFromX), wFromY(wFromY), wX(wX), wY(wY)
		, bPlayerHitsFirst(bPlayerHitsFirst), bDefeatToStabTarTile(bDefeatToStabTarTile)
	{}

	CMonster *pMonster;
	UINT wFromX, wFromY;
	UINT wX, wY;
	bool bPlayerHitsFirst;
	bool bDefeatToStabTarTile;
};

class CCurrentGame;
class CCombatEffect : public CAttachableObject
{
public:
	CCombatEffect(CEntity *pEntity, const CombatEffectType type, const int amount=0, const int originalAmount=0)
	: CAttachableObject()
	, pEntity(pEntity)
	, eType(type)
	, amount(amount)
	, originalAmount(originalAmount)
	{}

	CEntity *pEntity;       //who is affected
	CombatEffectType eType; //in what way
	int amount;             //how much
	int originalAmount;     //'amount', in case of damage, is taken off this original value
};

//Maintain state of one-on-one combat between player and a monster.
class CCombat
{
public:
	CCombat(CCurrentGame* pCurrentGame, CMonster* pMonster,
			const bool bPlayerHitsFirst, const UINT wFromX=UINT(-1), const UINT wFromY=UINT(-1),
			const UINT wX=UINT(-1), const UINT wY=UINT(-1),
			const bool bDefeatToStabTarTile=false);

	bool Advance(CCueEvents& CueEvents, bool bQuickResolution);

	bool AttackIsFromBehindMonster(int &dx, int &dy) const;
	int  GetExpectedDamage();
	UINT GetMonsterSingleAttackDamage() const;
	UINT GetMonsterAttacksMade() const { return monsterAttacksMade; }
	static UINT GetMonsterType(CMonster *pMonster);
	UINT GetProjectedPlayerATKIncreaseForFasterWin() const;

	void InitMonsterStats(const bool bCombatStart=true);
	bool IsCurrent() const;
	bool IsFighting(const CMonster* pMonster) const;
	void MonsterAttacksPlayerOnce(CCueEvents& CueEvents);
	bool MonsterCanHarmPlayer(CMonster *pMonster) const;
	static UINT MonsterATKDoubles(const CCurrentGame* pGame);
	bool PlayerAttacksFirst(const bool bPlayerHitWithWeapon) const;
	bool PlayerCanHarmMonster(const CMonster *pMonster) const;
	bool PlayerCanHarmQueuedMonster() const;
	CMonster* PlayerCantHarmAQueuedMonster() const;
	bool PlayerDoesStrongHit(const CMonster* pMonster) const;
	bool QueueMonster(CMonster* pMonster, const bool bPlayerHitsFirst,
			const UINT wFromX, const UINT wFromY,
			const UINT wX, const UINT wY,
			const bool bDefeatToStabTarTile=false);
	static UINT getMonsterATK(CMonster* pMonster, const CCurrentGame* pGame, bool& bMonsterDoesStrongHit);

	CCurrentGame *pGame;
	UINT wTurnOfCombat;   //on which game turn combat is
	bool bSimulated;      //true if combat shouldn't actually affect game state

	static UINT hitTicks; //how many ticks must be amassed for a hit to occur
	UINT noDamageHits;    //number of consecutive hits doing no damage by either party

	//player stats
	int plATK, plDEF;
	UINT plStartingHP, nonSimulatedPlayerHP;

	//monster stats
	CMonster *pMonster;
	UINT wFromX, wFromY; //location of player or entity initiating attack on monster
	UINT wX, wY;  //on what tile attack on monster occurs
	UINT monATK, monDEF;

	//special attack indicators
	bool bPlayerDoesStrongHit, bMonsterDoesStrongHit;
	bool bMonsterDoesNoDefenseHit, bPlayerDoesNoDefenseHit;
	bool bPlayerBackstabs; //first attack surprise from behind

	//intended combat results
	bool bDefeatToStabTarTile; //when battle is won, tarstuff is removed from this tile

	vector<CombatInfo> queuedCombat; //additional battles to be fought after current monster is defeated
	CMonster *pDefeatedMonster; //most recent monster of this combat sequence to be defeated
	bool bFightNextMonsterInQueue;
	bool bEndCombat; //if set, combat against the current monster terminates whether someone was defeated or not

	//combat progress
	UINT playerTicks, monsterTicks; //progress toward next hit

private:
	UINT playerAttacksMade, monsterAttacksMade;
	UINT monsterHPOnFinalRound;

	bool BeginFightingNextQueuedMonster(CCueEvents& CueEvents);
	bool FightNextMonster(CCueEvents& CueEvents);

	int  getPlayerATK();
	int  getPlayerDEF();
};

#endif //COMBAT_H

// $Id: Combat.cpp 10108 2012-04-22 04:54:24Z mrimer $

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

#include "Combat.h"
#include "Character.h"
#include "CurrentGame.h"
#include "Swordsman.h"

#include <math.h>

#define MAX_HP (INT_MAX)

UINT CCombat::hitTicks = 1000;

//Stats of predefined monster types.

//roach  qroach egg  gob    neat ww     eye
//snk[r] tm     tb   brain  mim  spi
//snk[g] snk[b] rock ws     nest aumt
//clo    dec    wub  seep   stal
//hal    sla    feg  fegAsh gua  (NPC)
//mm     mb     gm   gb     cit  rgiant
//madeye gking  con  fb
const UINT MON_HP[MONSTER_TYPES] = {
	  45,   50,   1, 320, 100,  35,  60,
	2500,  100, 320,  35, 210,  55,
	1500, 1200,  20, 220,  60, 230,
	 100,  220,  10, 200, 180,
	 100, 4500, 200,   1, 160,   0,
	  50,  130, 180, 360, 260, 800,
	 100,  400, 500, 100
};
const UINT MON_ATK[MONSTER_TYPES] = {
	  20,   42,   0, 120,  65,  38,  32,
	 550,  180, 140,   9, 200,  52,
	 600,  180, 100, 370, 100, 450,
	 680,  180,   0, 380, 430,
	  65,  560, 390,   0, 230,   0,
	  48,   60, 460, 310,  85, 500,
	  95,  199, 400, 100
};
const UINT MON_DEF[MONSTER_TYPES] = {
	   2,    6,  23,  15,  15,   3,   8,
	 350,  110,  20,   1,  65,  12,
	 250,   20,  68, 110,   8, 100,
	  50,   30, 320, 130, 210,
	  15,  310,  90,  80, 105,   0,
	  22,    3, 360,  20,   5, 100,
	  30,   66, 120,   0
};
const UINT MON_GOLD[MONSTER_TYPES] = {
	   2,    6,   0,  30,  25,   3,  5,
	 900,  100,  30,   1,  45,   8,
	 800,  100,  28,  80,  12, 100,
	  55,   35, 100,  90, 120,
	  25, 1000,  50,   0,  65,   0,
	  12,    8, 200,  40,  18, 500,
	  22,  144, 125,  10
};

//monster attack speed is always uniform
const UINT MON_SPEED = 100;

//roach egg     1/0/19       0   has high defense, i.e. spawned eggs can't be broken without a weapon
//fegundo ashes 1/0/80?      0   (not used)

/*
Green Slime     35/18/1      1   brain (is given 9 ATK since it does x2)
Red Slime       45/20/2      2   roach
Bat             35/38/3      3   wwing
Priest          60/32/8      5   eye
Skeleton C      50/42/6      6   roach queen
Skeleton B      55/52/12     8   spider

Big Slime       130/60/3     8   mud baby
Big Bat         60/100/8     12  skipper nest
Zombie          260/85/5     18  citizen
Superion Priest 100/95/30    22  mad eye
Rock            20/100/68    28  rock golem
Zombie Knight   320/120/15   30  goblin

Slime Man       320/140/20   30  tar baby
Ghost Soldier   220/180/30   35  decoy
Soldier         210/200/65   45  mimic
Swordsman       100/680/50   55  clone
Knight          160/230/105  65  guard

Slime Lord      360/310/20   40  gel baby
Vampire Bat     200/390/90   50  fegundo
Magician B      220/370/110  80  water skipper
Magician A      200/380/130  90  seep
Magic Sergeant  230/450/100  100 stalwart
Dark Knight     180/430/210  120 aumtlich

Gate-keeper C   50/48/22     12  mud mother
Gate-keeper B   100/180/110  100 tar mother
Gate-keeper A   180/460/360  200 gel mother

?               10/0/320     100 wubba
?               2500/550/350 900 serpent

Bosses
====================================
Skeleton A      100/65/15    30        halph / 'Neather
Vampire         444/199/66   144       goblin king
Gold Knight     120/150/50   100       pirate captain
Octopus         1200/180/20  100+50*   rattlesnake
Zeno            800/500/100  500       rock giant
Dragon          1500/600/250 800       adder
Magic Master    4500/560/310 1000      slayer
*/

//*****************************************************************************
inline void DecrementInt(int& var, const UINT amt)
//Reduces a variable to no less than zero
{
	var -= amt;
	if (var < 0)
		var = 0;
}

//*****************************************************************************
inline void DecrementUINT(UINT& var, const UINT amt)
//Reduces a variable to no less than zero
{
	if (var <= amt)
		var = 0;
	else
		var -= amt;
}

//*****************************************************************************
inline bool doubleWithClampUINT(UINT& val)
//Doubles a UINT, ensuring values don't overflow the signed int limit.
//
//Returns: false if actual result can't be given (i.e. value overflowed), otherwise true
{
	if (val >= INT_MAX)
	{
		val = INT_MAX;
		return false;
	}
	val *= 2;

	if (val > INT_MAX)
	{
		val = INT_MAX;
		return false;
	}

	return true;
}

//*****************************************************************************
inline bool doubleWithClamp(int& val)
//Doubles an int, ensuring values don't overflow.
//
//Returns: false if actual result can't be given (i.e. value overflowed), otherwise true
{
	const double newVal = val * 2.0;
	if (newVal > INT_MAX)
	{
		val = INT_MAX;
		return false;
	}
	if (newVal < INT_MIN)
	{
		val = INT_MIN;
		return false;
	}
	val *= 2;
	return true;
}

//*****************************************************************************
CCombat::CCombat(
		CCurrentGame* pCurrentGame, CMonster* pMonster,
		const bool bPlayerHitsFirst,
		const UINT wFromX, const UINT wFromY, //[-1,-1]
		const UINT wX, const UINT wY,    //[-1,-1]
		const bool bDefeatToStabTarTile) //[false]
	: pGame(pCurrentGame)
	, bSimulated(false)
	, noDamageHits(0)
	, nonSimulatedPlayerHP(0)
	, pMonster(pMonster)
	, wFromX(wFromX), wFromY(wFromY)
	, wX(wX), wY(wY)
	, bDefeatToStabTarTile(bDefeatToStabTarTile)
	, pDefeatedMonster(NULL)
	, bFightNextMonsterInQueue(false)
	, playerTicks(0), monsterTicks(0)
	, playerStalls(0), monsterStalls(0)
	, bCombatStalled(false)
	, playerAttacksMade(0), monsterAttacksMade(0)
	, monsterHPOnFinalRound(0)
	, bExpectedDamageIsApproximate(false)
{
	ASSERT(pCurrentGame);
	this->wTurnOfCombat = this->pGame->wTurnNo;

	ASSERT(pMonster);
	InitMonsterStats();

	//First attack occurs immediately.
	this->playerTicks = this->monsterTicks = CCombat::hitTicks;

	//Who gets to make the first hit?
	if (PlayerAttacksFirst(bPlayerHitsFirst))
		this->monsterTicks /= 2; //player hits sooner
	else
		this->playerTicks /= 2; //monster hits sooner
}

//*****************************************************************************
bool CCombat::PlayerAttacksFirst(const bool bPlayerHitWithWeapon) const
//Returns: whether player makes the first hit in combat, or the enemy
{
	ASSERT(this->pGame);
	ASSERT(this->pMonster);

	//1. If the player has the "attack first" attribute, then he makes first hit.
	//   If the enemy also has the "attack first" attribute, then they cancel out,
	//   in which case, the player hits first if he used his weapon to engage in combat.
	const bool bPlayerAlwaysAttacksFirst = this->pGame->DoesPlayerAttackFirst();
	const bool bEnemyAlwaysAttacksFirst = this->pMonster->CanAttackFirst();
	if (bPlayerAlwaysAttacksFirst)
	{
		if (bEnemyAlwaysAttacksFirst)
			return bPlayerHitWithWeapon; //decide like normal
		return true;
	}

	//2. If the player has the "attack last" attribute, then he makes last hit.
	//   If the enemy also has the "attack last" attribute, then they cancel out.
	const bool bPlayerAlwaysAttacksLast = this->pGame->DoesPlayerAttackLast();
	const bool bEnemyAlwaysAttacksLast = this->pMonster->CanAttackLast();
	if (bPlayerAlwaysAttacksLast)
	{
		if (bEnemyAlwaysAttacksLast)
			return bPlayerHitWithWeapon; //decide like normal
		return false;
	}
	
	//3. Player has no special combat initiative attributes.
	//Decide based on enemy initiative attributes, if any,
	//otherwise, the player hits first if he used his weapon to engage in combat.
	if (bEnemyAlwaysAttacksFirst)
		return false;
	if (bEnemyAlwaysAttacksLast)
		return true;
	return bPlayerHitWithWeapon;
}

//*****************************************************************************
bool CCombat::AttackIsFromBehindMonster(int &dx, int &dy) const
//Returns: whether the attack is coming from behind the monster
{
	ASSERT(this->pGame);
	const CSwordsman& player = *this->pGame->pPlayer;
	if (player.wAppearance == M_NONE)
		return false; //player not in room isn't attacking from anywhere

	//Direction toward attack.
	dx = sgn(int(this->wFromX - this->pMonster->wX));
	dy = sgn(int(this->wFromY - this->pMonster->wY));

	return
		//attack is coming from a direction opposite monster's orientation
		((dx && nGetOX(this->pMonster->wO) == -dx) || (dy && nGetOY(this->pMonster->wO) == -dy)) &&
		//and not diagonally to the side
		!(dx && dy && (nGetOX(this->pMonster->wO) == dx || nGetOY(this->pMonster->wO) == dy));
}

//*****************************************************************************
UINT CCombat::GetMonsterType(CMonster *pMonster)
//Returns: the type of this monster (or monster piece).
//         For characters, the base type is returned.
{
	UINT type = pMonster->wType;
	if (type == M_CHARACTER)
	{
		CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
		if (pCharacter->wIdentity < MONSTER_TYPES)
			type = pCharacter->wIdentity;
	}
	ASSERT(type < MONSTER_TYPES);
	return type;
}

//*****************************************************************************
int CCombat::getPlayerATK()
{
	ASSERT(this->pGame);

	const CSwordsman& player = *this->pGame->pPlayer;

	int atk = this->pGame->getPlayerATK(); //includes equipment modifiers

	//Does player have special attacks?
	this->bPlayerDoesStrongHit = this->bPlayerDoesNoDefenseHit = false;
	if (player.IsInvisible())
	{
		doubleWithClamp(atk); //doubles effective attack power
		this->bPlayerDoesStrongHit = true;
	}
	if (PlayerDoesStrongHit(this->pMonster))
	{
		doubleWithClamp(atk); //doubles effective attack power
		this->bPlayerDoesStrongHit = true;
	}

	if (player.HasSword() && this->pGame->DoesPlayerItemHaveNoEnemyDefense(ScriptFlag::Weapon))
	{
		//Player is wielding a NoDefense weapon.  Monster cannot protect against it.
		//if (this->monDEF > 0) this->monDEF = 0; //don't modify stat here, so monster's DEF display will be more useful
		this->bPlayerDoesNoDefenseHit = true;
	}

	return atk;
}

//*****************************************************************************
int CCombat::getPlayerDEF()
{
	ASSERT(this->pGame);

	int def = this->pGame->getPlayerDEF();

	//"No defense" attribute overrides DEF stats above.
	this->bMonsterDoesNoDefenseHit = false;
	if (this->pMonster->HasNoEnemyDefense() &&
			!this->pGame->DoesPlayerItemHaveNoEnemyDefense(ScriptFlag::Armor))
	{
		//Monster has a NoDefense attack that is not protected against by
		//player's NoDefense armor.
		if (def > 0)
			def = 0;
		this->bMonsterDoesNoDefenseHit = true;
	} else if (PlayerHasStrongShield(this->pMonster)){
		// Shield with monster weakness doubles effective defense
		doubleWithClamp(def);
	}

	return def;
}

//*****************************************************************************
void CCombat::InitMonsterStats(const bool bCombatStart) //[default=true]
//Sets initial stats of the monster player is engaging in combat.
{
	ASSERT(this->pMonster);
	ASSERT(this->pGame);

	CSwordsman& player = *this->pGame->pPlayer;
	PlayerStats& ps = player.st;
	if (bCombatStart)
	{
		this->plStartingHP = ps.HP;
		//Certain things should only be set at start of combat
		this->noDamageHits = 0;
		this->bPlayerBackstabs = this->pGame->DoesPlayerBackstab();
		this->playerAttacksMade = this->monsterAttacksMade = 0;
		this->monsterHPOnFinalRound = 0;
		this->playerStalls = this->monsterStalls = 0;
		this->bCombatStalled = false;
	}

	//Monster's attack stats and attributes.
	this->bMonsterDoesStrongHit = false;
	this->monATK = (int)getMonsterATK(this->pMonster, this->pGame, this->bMonsterDoesStrongHit);
	this->monDEF = this->pMonster->getDEF();

	this->plATK = getPlayerATK();
	this->plDEF = getPlayerDEF();

	this->bEndCombat = false;
}

//*****************************************************************************
bool CCombat::IsCurrent() const
//Returns: whether this combat object is for fighting monsters this turn
{
	return this->pGame->wTurnNo == this->wTurnOfCombat;
}

//*****************************************************************************
bool CCombat::IsFighting(const CMonster* pMonster) const
//Returns: whether this monster is queued for fighting
{
	if (this->pMonster == pMonster)
		return true;

	for (UINT i=0; i<this->queuedCombat.size(); ++i)
		if (this->queuedCombat[i].pMonster == pMonster)
			return true;

	return false;
}

//*****************************************************************************
UINT CCombat::MonsterATKDoubles(const CCurrentGame* pGame)
//Returns: N, where N is exponent in 2^N -- the monster's ATK multiplier against player
{
	ASSERT(pGame);
	return pGame->pRoom->GetBrainsPresent(); //each brain present doubles monster's ATK
}

//*****************************************************************************
bool CCombat::PlayerDoesStrongHit(const CMonster* pMonster) const
//Returns: whether player does x2 ATK against this monster
{
	ASSERT(pMonster);

	if (!this->pGame->IsPlayerSwordDisabled())
	{
		if (this->pGame->IsSwordStrongAgainst(pMonster))
			return true;
	}
	if (!this->pGame->IsPlayerAccessoryDisabled())
	{
		if (this->pGame->IsEquipmentStrongAgainst(pMonster, ScriptFlag::Accessory)) {
			return true;
		}
	}

	return false;
}

//*****************************************************************************
bool CCombat::PlayerHasStrongShield(const CMonster* pMonster) const
//Returns: whether player gets x2 DEF against this monster
{
	ASSERT(pMonster);

	if (this->pGame->IsPlayerShieldDisabled()) {
		return false;
	}

	return this->pGame->IsEquipmentStrongAgainst(pMonster, ScriptFlag::Armor);
}

//*****************************************************************************
UINT CCombat::getMonsterATK(CMonster* pMonster, const CCurrentGame* pGame, bool& bMonsterDoesStrongHit)
//Returns: the strength of the specified monster's ATK
{
	ASSERT(pMonster);
	UINT atk = pMonster->getATK();

	bMonsterDoesStrongHit = false;
	const UINT wMonsterATKDoubles = MonsterATKDoubles(pGame);
	if (wMonsterATKDoubles)
	{
		for (UINT n=wMonsterATKDoubles; n--; )
		{
			if (!doubleWithClampUINT(atk))
				break; //can't double any more
		}
		bMonsterDoesStrongHit = true;
	}

	return atk;
}

//*****************************************************************************
UINT CCombat::GetMonsterSingleAttackDamage() const
{
	return (int)this->monATK > this->plDEF ? (int)this->monATK - this->plDEF : 0;
}

//*****************************************************************************
UINT CCombat::GetProjectedPlayerATKIncreaseForFasterWin() const
//Returns: simple projection of how many more ATK the player needs to defeat this
//         enemy in fewer hits, or 0 if cannot defeat enemy faster
//
//Pre-condition: GetExpectedDamage() was called previously
{
	if (this->playerAttacksMade <= 1)
		return 0;

	UINT extraDamagePerATK = this->playerAttacksMade-1;
	const CSwordsman& player = *this->pGame->pPlayer;
	if (player.IsInvisible())
		extraDamagePerATK *= 2;
	if (PlayerDoesStrongHit(this->pMonster))
		extraDamagePerATK *= 2;

	return UINT(ceil(float(this->monsterHPOnFinalRound) / float(extraDamagePerATK)));
}

//*****************************************************************************
bool CCombat::Advance(
//Take another turn of combat, or all turns until resolution if bQuickResolution is set.
//
//Returns: whether combat should still continue
//
//Params:
	CCueEvents& CueEvents, //(in/out) events occurring during fight turn(s)
	bool bQuickResolution, //if true, then calculate all turns immediately until resolution
	const UINT maxStrikesToPerform) //if set, then evaluate up to this many combat strikes [default=0]
{
	bool bContinue = true; //true while combat rounds should continue being processed this call

	ASSERT(this->pGame);
	CSwordsman& player = *this->pGame->pPlayer;
	PlayerStats& ps = player.st;

	//Player should not be dead.
	ASSERT(ps.HP > 0);
	ASSERT(ps.speed);

	//No monster has been defeated yet this tick.
	this->pDefeatedMonster = NULL;

	if (this->bFightNextMonsterInQueue)
	{
		if (!FightNextMonster(CueEvents))
			return false;
	}

	CMonster *pMonsterBeingFought = this->pMonster;
	ASSERT(pMonsterBeingFought);

	UINT monHP = pMonsterBeingFought->getHP();
	ASSERT(monHP); //monster should not be dead at this point

	//If monster has become not combatable, fighting can no longer continue.
	if (!pMonsterBeingFought->IsCombatable() || !pMonsterBeingFought->IsAlive())
	{
		this->bEndCombat = true;
		BeginFightingNextQueuedMonster(CueEvents);
		return false;
	}

	UINT damageToMonster = 0, damageToPlayer = 0;
	bool bPlayerShielded = false, bStrongHit = false, bStrongHitOnPlayer = false;
	bool bNoDefenseHit = false, bNoDefenseHitOnPlayer = false;
	bool bPlayerDied = false;

	bool bQuick = bQuickResolution;
	UINT playerStrikesPerformed = 0;
	do {
		//Process a tick of combat resolution.

		//Add some progress toward when each entity hits again.
		this->playerTicks += ps.speed * (player.IsHasted() ? 2 : 1);
		this->monsterTicks += MON_SPEED;

		//Update enemy HP if in non-simulated quick combat
		if (bQuick && !this->bSimulated) {
			monHP = pMonsterBeingFought->getHP();
		}

		//Does either the player or enemy perform a strike this iteration?
		const bool bStrike = (this->playerTicks >= CCombat::hitTicks) ||
				(this->monsterTicks >= CCombat::hitTicks);

		if (bStrike)
		{
			//Re-evaluate player's current ATK+DEF.
			this->plATK = getPlayerATK();
			this->plDEF = getPlayerDEF();

			//While player can harm enemy without damage, combine combat rounds into one.
			bQuick = bQuickResolution ||
				(this->plDEF >= (int)this->monATK && this->plATK > (int)this->monDEF);

			//Does player attack now?
			if (this->playerTicks >= CCombat::hitTicks)
			{
				UINT playerHPBefore = ps.HP;
				UINT monsterHPBefore = pMonsterBeingFought->HP;

				int monCombatDEF = (int)this->monDEF;
				int playerATK = this->plATK;

				if (this->bPlayerDoesNoDefenseHit)
				{
					if (monCombatDEF > 0)
						monCombatDEF = 0;
				}
				if (this->bPlayerBackstabs)
				{
					//Double-strong backstab hit on first attack.
					int dx, dy;
					if (AttackIsFromBehindMonster(dx, dy))
					{
						doubleWithClamp(playerATK);
						bStrongHit = true;
					}
					this->bPlayerBackstabs = false;
				}

				//Player swings at monster.
				++this->playerAttacksMade;
				if (playerATK <= monCombatDEF)
				{
					//Monster not hurt
					CueEvents.Add(CID_EntityAffected, new CCombatEffect(pMonsterBeingFought, CET_NODAMAGE), true);
					++this->noDamageHits;

					if (this->bSimulated)
					{
						//When simulating a fight and the player would have run out of HP
						//when fighting a monster that cannot be harmed on this combat turn,
						//we will assume the player will never be able to defeat the monster,
						//even if custom scripting might have changed things,
						//in order to avoid the possibility of an endless simulation.
						if (damageToPlayer >= this->nonSimulatedPlayerHP)
						{
							player.st.HP = 0;
							bContinue = false;
						}
					} else {
						//Process special instructions when the monster is hit, even if
						//no damage is inflicted.
						//NOTE: Only process this when not simulating a combat outcome
						//to avoid script commands that change the room state being executed.

						//Process player's custom equipment scripts.
						CCharacter* pCharacter = this->pGame->getCustomEquipment(ScriptFlag::Weapon);
						if (pCharacter)
							pCharacter->ProcessAfterAttack(CueEvents);

						pCharacter = this->pGame->getCustomEquipment(ScriptFlag::Armor);
						if (pCharacter)
							pCharacter->ProcessAfterAttack(CueEvents);

						pCharacter = this->pGame->getCustomEquipment(ScriptFlag::Accessory);
						if (pCharacter)
							pCharacter->ProcessAfterAttack(CueEvents);

						if (!pMonsterBeingFought->ProcessAfterDefend(CueEvents))
						{
							bEndCombat = true;
							BeginFightingNextQueuedMonster(CueEvents);
						}

						//Track when combatant HP goes up as a result of script effects
						//(or monster HP failed to go down overall as result of player turn)
						if (ps.HP > playerHPBefore) {
							++this->playerStalls;
						}
						if (pMonsterBeingFought->HP >= monsterHPBefore) {
							++this->monsterStalls;
						}
					}
				} else {
					//Apply damage to monster.
					const UINT monsterHPAtRoundStart = monHP;
					const UINT delta = playerATK - monCombatDEF;
					if (delta > 0)
						this->noDamageHits = 0;
					DecrementUINT(monHP, delta);  //avoid negative values
					damageToMonster += delta;
					if (this->bPlayerDoesStrongHit)
						bStrongHit = true;
					if (this->bPlayerDoesNoDefenseHit)
						bNoDefenseHit = true;

					//Process special instructions when the monster is hit.
					if (!this->bSimulated)
					{
						//Update the monster's HP if this is an actual fight.
						pMonsterBeingFought->HP = monHP;

						//Process player's custom equipment scripts.
						CCharacter* pCharacter = this->pGame->getCustomEquipment(ScriptFlag::Weapon);
						if (pCharacter)
							pCharacter->ProcessAfterAttack(CueEvents);

						pCharacter = this->pGame->getCustomEquipment(ScriptFlag::Armor);
						if (pCharacter)
							pCharacter->ProcessAfterAttack(CueEvents);

						pCharacter = this->pGame->getCustomEquipment(ScriptFlag::Accessory);
						if (pCharacter)
							pCharacter->ProcessAfterAttack(CueEvents);

						if (!pMonsterBeingFought->ProcessAfterDefend(CueEvents))
						{
							bEndCombat = true;
							BeginFightingNextQueuedMonster(CueEvents);
						}

						//Track when combatant HP goes up as a result of script effects
						//(or monster HP failed to go down overall as result of player turn)
						if (ps.HP > playerHPBefore) {
							++this->playerStalls;
						}
						if (pMonsterBeingFought->HP >= monsterHPBefore) {
							++this->monsterStalls;
						}
					}

					//Check for monster death after scripts are processed.
					if (!monHP)
					{
						//Monster died.
						bContinue = false;

						this->monsterHPOnFinalRound = monsterHPAtRoundStart;
						if (!bPlayerDied)
						{
							//spoils of war (could be negative if defined that way)
							const int monGOLD = pMonsterBeingFought->getGOLD() * player.getGoldMultiplier();
							const int monXP = pMonsterBeingFought->getXP() * player.getXPMultiplier();
							incintValueWithBounds(ps.GOLD, monGOLD);
							CueEvents.Add(CID_EntityAffected, new CCombatEffect(&player, CET_GOLD, monGOLD), true);

							incintValueWithBounds(ps.XP, monXP);
							if (monGOLD != monXP) //only show this if it's different from gold received
								CueEvents.Add(CID_EntityAffected, new CCombatEffect(&player, CET_XP, monXP), true);

							player.bInvisible = false; //player loses any invisibility once combat is completed

							//This was a grueling fight.
							if (ps.HP < this->plStartingHP * 3 / 4)
								CueEvents.Add(CID_SwordsmanTired);

							//Keep track of which monster just died.
							this->pDefeatedMonster = pMonsterBeingFought;

							//If other monsters are queued to fight, set up to handle next one.
							BeginFightingNextQueuedMonster(CueEvents);
						}
					}
				}

				this->playerTicks -= CCombat::hitTicks; //begin counting time to next strike

				//Interrupt combat evaluation if we've exceeded the maximum strikes to calculate in this call.
				if (maxStrikesToPerform && ++playerStrikesPerformed >= maxStrikesToPerform && monHP > 0) {
					bContinue = false;
					this->bExpectedDamageIsApproximate = true;
					this->monsterHPOnFinalRound = monHP;
				}
			}

			//Does monster attack now?
			if (this->monsterTicks >= CCombat::hitTicks && bContinue)
			{
				UINT playerHPBefore = ps.HP;
				UINT monsterHPBefore = pMonsterBeingFought->HP;

				bool bSkipTurn = false;
				if (pMonsterBeingFought->TurnToFacePlayerWhenFighting())
				{
					if (pMonsterBeingFought->HasOrientation()) //directionless monsters can't be surprised from behind
					{
						int dx, dy;
						const bool bAttackIsBehindMonster = AttackIsFromBehindMonster(dx, dy);
						if (bAttackIsBehindMonster)
							bSkipTurn = true;
						if (player.wAppearance != M_NONE) {
							pMonsterBeingFought->SetOrientation(dx, dy);
						}
					}
				}

				if (!bSkipTurn)
				{
					//Monster swings at player.
					++this->monsterAttacksMade;
					if ((int)this->monATK <= this->plDEF)
					{
						//Player not hurt.
						bPlayerShielded = true;
						++this->noDamageHits;
					} else {
						//Apply damage to player.
						const UINT delta = GetMonsterSingleAttackDamage();
						if (delta > 0)
							this->noDamageHits = 0;
						DecrementUINT(ps.HP, delta);
						damageToPlayer += delta;
						if (this->bMonsterDoesStrongHit)
							bStrongHitOnPlayer = true;
						if (this->bMonsterDoesNoDefenseHit)
							bNoDefenseHitOnPlayer = true;
					}

					//Process special instructions when the monster attacks,
					//whether or not it inflicts damage.
					if (!this->bSimulated)
					{
						if (!pMonsterBeingFought->ProcessAfterAttack(CueEvents)) {
							this->bEndCombat = true;
							BeginFightingNextQueuedMonster(CueEvents);
						}

						//Process player's custom equipment scripts.
						CCharacter* pCharacter = this->pGame->getCustomEquipment(ScriptFlag::Armor);
						if (pCharacter)
							pCharacter->ProcessAfterDefend(CueEvents);

						pCharacter = this->pGame->getCustomEquipment(ScriptFlag::Weapon);
						if (pCharacter)
							pCharacter->ProcessAfterDefend(CueEvents);

						pCharacter = this->pGame->getCustomEquipment(ScriptFlag::Accessory);
						if (pCharacter)
							pCharacter->ProcessAfterDefend(CueEvents);

						//Track when combatant HP goes up as a result of script effects
						//(or player HP failed to go down overall as result of monster turn)
						if (ps.HP >= playerHPBefore) {
							++this->playerStalls;
						}
						if (pMonsterBeingFought->HP > monsterHPBefore) {
							++this->monsterStalls;
						}
					}

					//Check for player death after scripts are processed.
					if (!ps.HP)
					{
						bPlayerDied = true;
						//In simulated combat, figure out how much damage the player would take
						//before killing the monster, even when the player actually dies first.
						bContinue = this->bSimulated;
					}
				}

				this->bPlayerBackstabs = false; //if monster gets first hit, backstab attack fails

				this->monsterTicks -= CCombat::hitTicks;
			}

			//If the enemy's not dead and neither combatant can harm
			//the other, then end the combat.
			if (this->noDamageHits >= 3 && monHP)
			{
				bContinue = false;

				//If other monsters are queued to fight, handle next one now.
				BeginFightingNextQueuedMonster(CueEvents);
			}

			//The combat isn't making a timely progression towards a conclusion
			if (this->playerStalls > 25 && this->monsterStalls > 25)
			{
				bContinue = false;
				this->bCombatStalled = true;
			}
		}

		if (!this->bSimulated) {
			//in a real encounter, check for flags interrupting combat
			if (this->bEndCombat ||
				!this->pGame->bIsGameActive) //if a script executed something like a level exit, combat must stop now
			{
				bContinue = false; //combat has been interrupted prior to defeat
			}
		}
	} while (bContinue && bQuick); //in quick combat, sum up all rounds at once until someone dies

	//Finished.
	//
	//Output events for what happened.

	//only show damage to monster when combat rounds are shown separately.
	//Otherwise, if the monster died, the number is meaningless and just gets in the way.
	if (damageToMonster && !(bQuick && this->pDefeatedMonster))
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(pMonsterBeingFought,
				CET_HARM, damageToMonster, monHP), true);

	if (damageToPlayer)
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(&player,
				CET_HARM, damageToPlayer, player.st.HP), true);
	if (bPlayerShielded)
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(&player, CET_NODAMAGE), true);
	if (bStrongHit)
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(pMonsterBeingFought, CET_STRONGHIT), true);
	if (bNoDefenseHit)
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(pMonsterBeingFought, CET_NODEFENSEHIT), true);
	if (bStrongHitOnPlayer)
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(&player, CET_STRONGHIT), true);
	if (bNoDefenseHitOnPlayer)
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(&player, CET_NODEFENSEHIT), true);

	return bContinue;
}

//*****************************************************************************
bool CCombat::BeginFightingNextQueuedMonster(CCueEvents& CueEvents)
//When combat is done, this method is called to begin fighting the next queued opponent.
//
//Returns: true if a new opponent is now being fought, else false
{
	if (this->queuedCombat.empty())
	{
		//Ensure fight with this monster does not continue
		//(important for monsters with multiple pieces).
		this->bEndCombat = true;
		return false;
	}

	CombatInfo& combat = this->queuedCombat.back();

	//don't refight a dead monster
	if (!combat.pMonster->IsAlive())
		return false;

//	ASSERT(combat.pMonster != this->pDefeatedMonster); //no -- it's possible to hit monsters from multiple tiles, so this check should be handled when the monster is queued up
	CueEvents.Add(CID_MonsterEngaged, combat.pMonster);
	this->bFightNextMonsterInQueue = true; //next time this method is called

	return true;
}

//*****************************************************************************
bool CCombat::FightNextMonster(CCueEvents& CueEvents)
//Player defeated one monster and now has another battle queued to fight.
{
	CombatInfo& combat = this->queuedCombat.back();

	//If monster was already defeated (i.e. simultaneous attacks on one turn), don't fight again.
	if (!combat.pMonster->getHP() || !combat.pMonster->IsAlive())
	{
		this->queuedCombat.pop_back();

		//Advance to fight any other queued monsters.
		this->bEndCombat = true;
		BeginFightingNextQueuedMonster(CueEvents);
		return false;
	}

	this->pMonster = combat.pMonster;

	this->wFromX = combat.wFromX;
	this->wFromY = combat.wFromY;
	this->wX = combat.wX;
	this->wY = combat.wY;
	this->bDefeatToStabTarTile = combat.bDefeatToStabTarTile;
	InitMonsterStats();
	this->playerTicks = this->monsterTicks = 0;
	if (combat.bPlayerHitsFirst)
		this->playerTicks = CCombat::hitTicks / 2; //First attack occurs after a half-round delay.
	else
		this->monsterTicks = CCombat::hitTicks / 2;

	this->queuedCombat.pop_back();
	this->bFightNextMonsterInQueue = false;

	return true;
}

//*****************************************************************************
void CCombat::MonsterAttacksPlayerOnce(CCueEvents& CueEvents)
//Monster attacks the player one time.
{
	ASSERT(this->pGame);
	ASSERT(this->pGame->pPlayer);
	ASSERT(this->pMonster);
	CSwordsman& player = *this->pGame->pPlayer;
	PlayerStats& ps = player.st;

	if ((int)this->monATK <= this->plDEF)
	{
		//Player not hurt.
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(&player, CET_NODAMAGE), true);
	} else {
		//Apply normal damage.
		const UINT delta = this->monATK - this->plDEF;
		DecrementUINT(ps.HP, delta);
		CueEvents.Add(CID_EntityAffected, new CCombatEffect(&player, CET_HARM, delta), true);
		CueEvents.Add(CID_SwordsmanAfraid);
	}

	//Process special instructions when the monster attacks,
	//whether or not it inflicts damage.
	ASSERT(!this->bSimulated);
	this->pMonster->ProcessAfterAttack(CueEvents);

	//Process player's custom equipment scripts.
	CCharacter* pCharacter = this->pGame->getCustomEquipment(ScriptFlag::Armor);
	if (pCharacter)
		pCharacter->ProcessAfterDefend(CueEvents);

	pCharacter = this->pGame->getCustomEquipment(ScriptFlag::Weapon);
	if (pCharacter)
		pCharacter->ProcessAfterDefend(CueEvents);

	pCharacter = this->pGame->getCustomEquipment(ScriptFlag::Accessory);
	if (pCharacter)
		pCharacter->ProcessAfterDefend(CueEvents);

	//Check for player death after scripts are processed.
	if (!ps.HP)
		CueEvents.Add(CID_MonsterKilledPlayer, this->pMonster);
}

//*****************************************************************************
// Performs a simulation of this combat to forecast total damage the player will take.
// If combat requires too many rounds, it will be short-circuited and an estimate returned.
int CCombat::GetExpectedDamage()
//Returns: amount of damage player will suffer if striking this monster first;
//         -1: player cannot harm monster
{
	if (!PlayerCanHarmMonster(this->pMonster))
		return -1;

	if (!this->pMonster->IsCombatable())
		return -1;

	//Simulate the combat without changing any player stats.
	CCueEvents Ignored;
	CSwordsman& player = *this->pGame->pPlayer;
	this->nonSimulatedPlayerHP = player.st.HP;
	CSwordsman player_ = player;
	const UINT wMonsterO = this->pMonster->wO;
	this->bSimulated = true; //don't alter room state
	this->bExpectedDamageIsApproximate = false;

	player.st.HP = MAX_HP; //give player max HP for total damage calculation
	static const UINT maxStrikesToPerform = 1000;
	Advance(Ignored, true, maxStrikesToPerform);
	UINT uDamage = UINT(MAX_HP) - player.st.HP;

	if (this->bExpectedDamageIsApproximate) {
		//Combat damage took too long to predict -- extrapolate based on partial outcome.
		ASSERT(this->monsterHPOnFinalRound);

		const double fPercentMonsterHPTaken = 1.0f - (this->monsterHPOnFinalRound / double(this->pMonster->getHP()));
		ASSERT(0.0f <= fPercentMonsterHPTaken && fPercentMonsterHPTaken <= 1.0f);
		if (fPercentMonsterHPTaken > 0.0f) {
			const double fFactorToCompleteCombat = 1.0f / fPercentMonsterHPTaken;
			uDamage = UINT(ceil(double(uDamage) * fFactorToCompleteCombat));
			this->monsterAttacksMade = UINT(ceil(double(this->monsterAttacksMade) * fFactorToCompleteCombat));
		} else {
			uDamage = INT_MAX;
		}
	}

	const int damage = uDamage >= INT_MAX ? INT_MAX : uDamage;

	//Restore original state.
	this->bSimulated = false;
	this->pMonster->wO = wMonsterO;
	player = player_;

	return damage;
}

//*****************************************************************************
bool CCombat::PlayerCanHarmMonster(const CMonster *pMonster) const
//Returns: whether player has the power to harm this monster
{
	ASSERT(pMonster);

	if (!pMonster->IsCombatable())
		return false;

	ASSERT(this->pGame);
	const CSwordsman& player = *this->pGame->pPlayer;
	int atk = this->pGame->getPlayerATK(); //includes equipment modifiers
	UINT monDef = pMonster->getDEF();

	//Does player have special attacks?
	if (player.IsInvisible())
		doubleWithClamp(atk); //doubles effective attack power
	if (PlayerDoesStrongHit(pMonster))
		doubleWithClamp(atk); //doubles effective attack power
	if (player.HasSword() && this->pGame->DoesPlayerItemHaveNoEnemyDefense(ScriptFlag::Weapon))
		monDef = 0;

	int dx, dy;
	if (this->pGame->DoesPlayerBackstab() &&
			AttackIsFromBehindMonster(dx, dy))
		doubleWithClamp(atk);

	return atk > (int)monDef;
}

//*****************************************************************************
bool CCombat::PlayerCanHarmQueuedMonster() const
//Returns: whether player can harm any monster queued for combat
{
	if (this->pMonster && PlayerCanHarmMonster(this->pMonster))
		return true;

	for (vector<CombatInfo>::const_iterator combat = this->queuedCombat.begin();
			combat != this->queuedCombat.end(); ++combat)
		if (PlayerCanHarmMonster(combat->pMonster))
			return true;

	return false;
}

//*****************************************************************************
CMonster* CCombat::PlayerCantHarmAQueuedMonster() const
//Returns: if the player can't harm any queued monster, then return a pointer
// to the first of these.  If no unharmable queued monsters exist, return NULL.
{
	if (this->pMonster && !PlayerCanHarmMonster(this->pMonster))
		return this->pMonster;

	for (vector<CombatInfo>::const_iterator combat = this->queuedCombat.begin();
			combat != this->queuedCombat.end(); ++combat)
		if (!PlayerCanHarmMonster(combat->pMonster))
			return combat->pMonster;

	return NULL;
}

//*****************************************************************************
bool CCombat::MonsterCanHarmPlayer(CMonster *pMonster) const
//Returns: whether player is vulnerable to this monster's attack
{
	ASSERT(pMonster);
	ASSERT(this->pGame);
	int def = this->pGame->getPlayerDEF(); //includes equipment modifiers

	if (pMonster->HasNoEnemyDefense() &&
			!this->pGame->DoesPlayerItemHaveNoEnemyDefense(ScriptFlag::Armor))
	{
		//Monster has a NoDefense attack that is not protected against by
		//player's NoDefense armor.
		if (def > 0)
			def = 0;
	}

	ASSERT(pMonster->IsCombatable());
	bool bIgnored;
	UINT monATK = getMonsterATK(pMonster, this->pGame, bIgnored);
	return def < (int)monATK;
}

//*****************************************************************************
bool CCombat::QueueMonster(
//Adds another fight to this turn's combat queue.
//
//Returns: whether the player is able to damage this monster
	CMonster *pMonster,
	const bool bPlayerHitsFirst,
	const UINT wFromX, const UINT wFromY,
	const UINT wX, const UINT wY,
	const bool bDefeatToStabTarTile) //[default=false]
{
	//Don't add a fight on a tile where a fight is already queued.
	if (this->wX == wX && this->wY == wY)
		return true; //already fighting on this tile.  Assume player can harm the monster already being fought.

	for (UINT i=0; i<this->queuedCombat.size(); ++i)
		if (this->queuedCombat[i].wX == wX && this->queuedCombat[i].wY == wY)
			return true; //already set to fight here

	this->queuedCombat.push_back(CombatInfo(pMonster, wFromX, wFromY, wX, wY,
			bPlayerHitsFirst, bDefeatToStabTarTile));

	return PlayerCanHarmMonster(pMonster);
}

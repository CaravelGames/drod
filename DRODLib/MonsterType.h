#ifndef MONSTERTYPE_H
#define MONSTERTYPE_H

#include <BackEndLib/Types.h>

//Monster types.  Keep this numbering in order to preserve their integer values.
enum MONSTERTYPE {
	M_ROACH=0,
	M_QROACH,
	M_REGG,
	M_GOBLIN,
	M_NEATHER,
	M_WWING,
	M_EYE,
	M_SERPENT,
	M_TARMOTHER,
	M_TARBABY,
	M_BRAIN,
	M_MIMIC,
	M_SPIDER,

	//2.0 and 3.0 monsters
	M_SERPENTG,
	M_SERPENTB,
	M_ROCKGOLEM,
	M_WATERSKIPPER,
	M_SKIPPERNEST,
	M_AUMTLICH,
	M_CLONE,
	M_DECOY,
	M_WUBBA,
	M_SEEP,
	M_STALWART,
	M_HALPH,
	M_SLAYER,
	M_FEGUNDO,
	M_FEGUNDOASHES,
	M_GUARD,
	M_CHARACTER,
	M_MUDMOTHER,
	M_MUDBABY,
	M_GELMOTHER,
	M_GELBABY,
	M_CITIZEN,
	M_ROCKGIANT,
	M_HALPH2,  //3.0 "skin" variant
	M_SLAYER2, //3.0 "skin" variant

	//4.0 monsters
	M_STALWART2, //4.0 "skin" variant

	//5.0 monsters
	M_ARCHITECT, //"upgraded" citizen
	M_CONSTRUCT,
	M_GENTRYII,
	M_TEMPORALCLONE,
	M_FLUFFBABY,

	MONSTER_TYPES, //end of real monster types

	//Character pseudo-types (speakers)
	CHARACTER_FIRST=10000,
	M_NEGOTIATOR=CHARACTER_FIRST,
	M_CITIZEN1,
	M_CITIZEN2,
	M_GOBLINKING,
	M_INSTRUCTOR, //deprecated 2.0 type
	M_MUDCOORDINATOR,
	M_TARTECHNICIAN,
	M_EYE_ACTIVE,
	M_BEETHRO,
	M_CITIZEN3,
	M_CITIZEN4,
	M_BEETHRO_IN_DISGUISE,
	M_GUNTHRO,

	CHARACTER_TYPES,

	CUSTOM_CHARACTER_FIRST=20000, //for custom character IDs

	M_PLAYER = static_cast<UINT>(-3),
	M_CUSTOM = static_cast<UINT>(-2),
	M_NONE = static_cast<UINT>(-1)
};

static inline bool IsValidMonsterType(const UINT mt) {return (mt<MONSTER_TYPES);}

static inline bool bIsMother(const UINT mt) {
	switch (mt)
	{
		case M_TARMOTHER: case M_MUDMOTHER: case M_GELMOTHER: return true;
		default: return false;
	}
}

static inline bool bIsMonsterTarstuff(const UINT mt) {
	switch (mt)
	{
		case M_TARMOTHER: case M_MUDMOTHER: case M_GELMOTHER:
		case M_TARBABY: case M_MUDBABY: case M_GELBABY:
			return true;
		default: return false;
	}
}

//The standard player character roles
static inline bool bIsSmitemaster(const UINT mt){
	switch (mt)
	{
		case M_BEETHRO: case M_GUNTHRO:
			return true;
		default:
			return false;
	}
}

static inline bool bIsBeethroDouble(const UINT mt) {
	switch (mt)
	{
		case M_MIMIC: case M_CLONE: case M_DECOY:
		case M_BEETHRO: case M_GUNTHRO:
		case M_TEMPORALCLONE:
			return true;
		//Don't include M_BEETHRO_IN_DISGUISE because that type is functionally
		//different from Beethro.
		default:
			return false;
	}
}

static inline bool bIsStalwart(const UINT mt){
	switch (mt)
	{
		case M_STALWART: case M_STALWART2:
			return true;
		default:
			return false;
	}
}

//Any sworded monster type.
static inline bool bEntityHasSword(const UINT mt) {
	switch (mt)
	{
		case M_MIMIC: case M_CLONE: case M_DECOY:
		case M_SLAYER:	case M_SLAYER2:
		case M_GUARD: case M_STALWART: case M_STALWART2:
		case M_BEETHRO: case M_BEETHRO_IN_DISGUISE:
		case M_GUNTHRO:
		case M_TEMPORALCLONE:
			return true;
		default:
			return false;
	}
}

//All human types.
static inline bool bIsHuman(const UINT mt) {
	switch (mt)
	{
		case M_MIMIC: case M_CLONE: case M_DECOY:
		case M_SLAYER:	case M_SLAYER2:
		case M_GUARD: case M_STALWART: case M_STALWART2:
		case M_BEETHRO: case M_BEETHRO_IN_DISGUISE: case M_GUNTHRO:
		case M_TEMPORALCLONE:
		case M_HALPH: case M_HALPH2: case M_NEATHER:
		case M_CITIZEN: case M_ARCHITECT:
		case M_CITIZEN1: case M_CITIZEN2: case M_CITIZEN3: case M_CITIZEN4:
		case M_INSTRUCTOR: case M_NEGOTIATOR:
		case M_MUDCOORDINATOR: case M_TARTECHNICIAN:
			return true;
		default:
			return false;
	}
}

//All non-Beethro types that monsters can sense as an enemy.
static inline bool bIsMonsterTarget(const UINT mt) {
	switch (mt)
	{
		case M_BEETHRO: case M_GUNTHRO:
		case M_CLONE: case M_DECOY: case M_TEMPORALCLONE:
		case M_STALWART: case M_STALWART2:
			return true;
		default:
			return false;
	}
}

static inline bool bCanEntityStepOnMonsters(const UINT mt) {
	//Human roles can't step on monsters.
	//Exception: Slayer
	switch (mt)
	{
		case M_NEGOTIATOR: case M_INSTRUCTOR:
		case M_CITIZEN: case M_ARCHITECT:
		case M_CITIZEN1: case M_CITIZEN2:
		case M_CITIZEN3: case M_CITIZEN4:
		case M_MUDCOORDINATOR: case M_TARTECHNICIAN:
		case M_WUBBA:
		case M_HALPH: case M_HALPH2:
		case M_CLONE: case M_DECOY: case M_MIMIC:
		case M_TEMPORALCLONE:
		case M_GUARD: case M_STALWART: case M_STALWART2:
		case M_BEETHRO: case M_BEETHRO_IN_DISGUISE:
		case M_GUNTHRO:
		case M_NONE:
			return false;
		default:
			//All other (monster) roles can step on other monsters.
			return true;
	}
}

static inline bool bIsEntityTypeVulnerableToHeat(const UINT mt) {
	switch (mt)
	{
		case M_TARMOTHER: case M_TARBABY:
		case M_MUDMOTHER: case M_MUDBABY:
		case M_GELMOTHER: case M_GELBABY:
		case M_NONE:
			return false;
		default:
			return true;
	}
}

static inline bool bIsEntityFlying(const UINT mt) {
	switch (mt)
	{
	case M_WWING: case M_FEGUNDO: case M_FLUFFBABY:
		return true;
	default:
		return false;
	}
}

static inline bool bCanEntityPressPressurePlates(const UINT mt) {
	switch (mt)
	{
	case M_WWING: case M_FEGUNDO: case M_FLUFFBABY: case M_SEEP:
		return false;
	default:
		return true;
	}
}

static inline bool bIsEntitySwimming(const UINT mt) {
	return mt == M_WATERSKIPPER || mt == M_SKIPPERNEST;
}

static inline bool bIsSerpent(const UINT mt) {return mt==M_SERPENT || mt==M_SERPENTG || mt==M_SERPENTB;}
static inline bool bIsSerpentOrGentryii(const UINT mt) { return bIsSerpent(mt) || mt == M_GENTRYII; }

static inline bool bIsRockGolemType(const UINT mt) { return mt==M_ROCKGOLEM || mt==M_CONSTRUCT; }

static inline bool bCanEntityHideInShallowWater(const UINT mt) {
	switch(mt)
	{
		case M_BEETHRO: case M_GUNTHRO:
		case M_DECOY: case M_CLONE: case M_MIMIC:
		case M_TEMPORALCLONE:
			return true;
		default:
			return false;
	}
}

static inline bool bCanEntityWadeInShallowWater(const UINT mt) {
	return mt != M_STALWART && mt != M_HALPH &&
			(bIsHuman(mt) || bIsSerpentOrGentryii(mt) || mt == M_BRAIN || mt == M_SPIDER
			|| mt == M_GELBABY || bIsRockGolemType(mt) || mt == M_ROCKGIANT);
}

static inline bool bCanFluffTrack(const UINT mt) {
	switch(mt) {
		case M_TARMOTHER: case M_MUDMOTHER: case M_GELMOTHER:
		case M_ROCKGOLEM: case M_ROCKGIANT: case M_CONSTRUCT:
		case M_GENTRYII:  case M_FLUFFBABY: case M_SEEP:
		case M_FEGUNDOASHES:
			return false;
		default:
			return true;
	}	
}
static inline bool bCanFluffKill(const UINT mt) {
	switch(mt) {
		case M_ROCKGOLEM: case M_ROCKGIANT: case M_CONSTRUCT:
		case M_GENTRYII: case M_FLUFFBABY: case M_SEEP:
			return false;
		default:
			return true;
	}	
}

static inline bool bMonsterCanActivateCheckpoint(const UINT mt) {
	switch (mt) {
		case M_MIMIC:
		case M_FEGUNDO:
		case M_TEMPORALCLONE:
			return true;
		default:
			return false;
	}
}

#endif
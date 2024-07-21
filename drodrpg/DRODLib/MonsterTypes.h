#ifndef MONSTERTYPES_H
#define MONSTERTYPES_H

#include <BackEndLib/Types.h>

//Monster types.  Keep this in order to preserve their integer values.
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
	M_PIRATE,
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
	M_MADEYE, //moved from pseudo type for RPG
	M_GOBLINKING, //moved from pseudotype for RPG

	MONSTER_TYPES, //end of real monster types

	//Character pseudo-types (speakers)
	CHARACTER_FIRST=10000,
	M_NEGOTIATOR=CHARACTER_FIRST, //2.0 deprecated
	M_CITIZEN1,
	M_CITIZEN2,
	M_INSTRUCTOR, //2.0 deprecated
	M_MUDCOORDINATOR, //2.0 deprecated
	M_TARTECHNICIAN, //2.0 deprecated
	M_BEETHRO,
	M_CITIZEN3,
	M_CITIZEN4,
	M_BEETHRO_IN_DISGUISE,
	M_STALWART,
	M_ARCHIVIST,

	CHARACTER_TYPES,

	CUSTOM_CHARACTER_FIRST=20000, //for custom character IDs
	CUSTOM_EQUIPMENT=CUSTOM_CHARACTER_FIRST,

	M_PLAYER = static_cast<UINT>(-3),
	M_CUSTOM = static_cast<UINT>(-2),
	M_NONE = static_cast<UINT>(-1)
};

static inline bool bIsCustomEquipment(const UINT t) {return t >= CUSTOM_EQUIPMENT;}

static inline MONSTERTYPE defaultPlayerType() {return M_STALWART;}

#endif //MONSTERTYPES_H

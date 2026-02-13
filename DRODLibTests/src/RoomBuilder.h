#ifndef ROOM_BUILDER_H
#define ROOM_BUILDER_H

#include "CTestDb.h"
#include "../../DRODLib/Character.h"
#include "../../DRODLib/Serpent.h"
#include "../../DRODLib/Gentryii.h"
#include "../../DRODLib/DbRooms.h"
#include <BackEndLib/Types.h>

class RoomBuilder{
public:
	static void ClearRoom();
	static CCharacter* AddCharacter(const UINT wX, const UINT wY, const UINT wO = N, const UINT identity = M_ROACH);
	static CCharacter* AddVisibleCharacter(const UINT wX, const UINT wY, const UINT wO = N, const UINT identity = M_ROACH);
	static CMonster* AddMonster(const UINT wType, const UINT wX, const UINT wY, const UINT wO = N);
	static CCharacter* AddMonsterWithWeapon(const UINT wType, const WeaponType weaponType, const UINT wX, const UINT wY, const UINT wO = N);
	static void AddSerpentPiece(CSerpent* serpent, const UINT pieceX, const UINT pieceY);
    static void AddGentryiiPiece(CGentryii* gentryii, const UINT pieceX, const UINT pieceY);
	static void AddWallLight(const UINT wX, const UINT wY, const UINT type);
	static void AddCeilingLight(const UINT wX, const UINT wY, const UINT type);
	static void AddCeilingDarkness(const UINT wX, const UINT wY, const UINT type);
	static void SaveRoom();
	static void AddCommand(CCharacter* character, CCharacterCommand::CharCommand command, UINT x = 0, UINT y = 0, UINT w = 0, UINT h = 0, UINT flags = 0);
	static void AddCommand(CCharacter* character, CCharacterCommand::CharCommand command, UINT x, UINT y, UINT w, UINT h, UINT flags, WSTRING label);
	static void AddCommand(CCharacter* character, CCharacterCommand::CharCommand command, UINT x, UINT y, UINT w, UINT h, UINT flags, const char * label);
	static COrbData* AddOrbDataToTile(const UINT wX, const UINT wY, const OrbType eOrbType = OT_NORMAL);
	static void LinkOrb(const UINT wOrbX, const UINT wOrbY, const UINT wDoorX, const UINT wDoorY, const OrbAgentType eLinkType);
	static void Plot(const UINT tileType, const UINT wX, const UINT wY);
	static void PlotToken(const RoomTokenType tokenType, const UINT wX, const UINT wY);
	static void PlotStation(const UINT wX, const UINT wY, const UINT stationType);
	static void PlotRect(const UINT tileType, const UINT startX, const UINT startY, const UINT endX, const UINT endY);

private:
	static CDbRoom* GetRoom();

	static UINT GetSerpentTile(const UINT prevTileX, const UINT prevTileY,
		const UINT thisTileX, const UINT thisTileY,
		const UINT nextTileX = UINT(-1), const UINT nextTileY = UINT(-1));

	static CMonsterPiece* GetPieceAtIndex(MonsterPieces piecesList, const UINT index);
};

#endif
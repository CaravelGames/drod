#ifndef ROOM_BUILDER_H
#define ROOM_BUILDER_H

#include "CTestDb.h"
#include "../../DRODLib/Character.h"
#include "../../DRODLib/Serpent.h"
#include "../../DRODLib/DbRooms.h"
#include <BackEndLib/Types.h>

class RoomBuilder {

	class OrbLinker
	{
	public:
		OrbLinker Link(UINT wDoorX, UINT wDoorY, OrbAgentType eLinkType = OA_TOGGLE) const;

	private:
		friend class RoomBuilder;
		explicit OrbLinker(COrbData* pOrbData);
		COrbData* pOrbData;
	};

	class LongMonsterBuilder
	{
	public:
		LongMonsterBuilder GrowIn(UINT wO, UINT wTiles = 1) const;
		LongMonsterBuilder GrowTo(UINT wX, UINT wY) const;
		LongMonsterBuilder Grow(int deltaX, int deltaY) const;
		void End() const {};

		CMonster* ToMonster() const;

	private:
		friend class RoomBuilder;
		CMonster& pMonster;
		void Plot(UINT wPieceX, UINT wPieceY) const;
		void PlotSerpentPiece(UINT wPieceX, UINT wPieceY) const;
		void PlotGentryiiPiece(UINT wPieceX, UINT wPieceY) const;
		CMonsterPiece* GetPieceAtIndex(int index) const;
		static UINT GetSerpentTile(UINT prevTileX, UINT prevTileY,
		                           UINT thisTileX, UINT thisTileY,
		                           UINT nextTileX = static_cast<UINT>(-1),
		                           UINT nextTileY = static_cast<UINT>(-1));
		CCoord GetTail() const;
		explicit LongMonsterBuilder(CMonster* pMonster);
		explicit LongMonsterBuilder(CMonster& pMonster);
	};


public:
	static void ClearRoom();
	static CCharacter* AddCharacter(UINT wX, UINT wY, UINT wO = N, UINT identity = M_ROACH);
	static CCharacter* AddVisibleCharacter(UINT wX, UINT wY, UINT wO = N, UINT identity = M_ROACH);
	static CMonster* AddMonster(UINT wType, UINT wX, UINT wY, UINT wO = N);
	static CCharacter* AddMonsterWithWeapon(UINT wType, WeaponType weaponType, UINT wX, UINT wY, UINT wO = N);
	static LongMonsterBuilder AddLongMonster(UINT wType, UINT wX, UINT wY, UINT wO = E);
	static void AddWallLight(UINT wX, UINT wY, UINT type);
	static void AddCeilingLight(UINT wX, UINT wY, UINT type);
	static void AddCeilingDarkness(UINT wX, UINT wY, UINT type);
	static void SaveRoom();
	static void AddCommand(CCharacter* character, CCharacterCommand::CharCommand command, UINT x = 0, UINT y = 0, UINT w = 0, UINT h = 0, UINT flags = 0);
	static void AddCommand(CCharacter* character, CCharacterCommand::CharCommand command, UINT x, UINT y, UINT w, UINT h, UINT flags, WSTRING label);
	static void AddCommand(CCharacter* character, CCharacterCommand::CharCommand command, UINT x, UINT y, UINT w, UINT h, UINT flags, const char * label);
	static COrbData* AddOrbDataToTile(UINT wX, UINT wY, OrbType eOrbType = OT_NORMAL);
	static void LinkOrb(UINT wOrbX, UINT wOrbY, UINT wDoorX, UINT wDoorY, OrbAgentType eLinkType);
	static void Plot(UINT tileType, UINT wX, UINT wY);
	static OrbLinker PlotPlate(UINT wX, UINT wY, OrbType eOrbType = OT_NORMAL);
	static OrbLinker PlotPlate(UINT wX, UINT wY, UINT wEndX, UINT wEndY, OrbType eOrbType = OT_NORMAL);
	static void PlotToken(RoomTokenType tokenType, UINT wX, UINT wY);
	static void PlotObstacle(UINT obstacleType, UINT wX, UINT wY);
	static void PlotObstacle(UINT obstacleType, UINT startX, UINT startY, UINT endX, UINT endY);
	static void PlotStation(UINT wX, UINT wY, UINT stationType);
	static void PlotRect(UINT tileType, UINT startX, UINT startY, UINT endX, UINT endY);

private:
	static CDbRoom* GetRoom();
	static CDbRoom& GetRoomRef();
};

#endif
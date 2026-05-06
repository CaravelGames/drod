#include "RoomBuilder.h"

#include "catch.hpp"
#include "../../DRODLib/DbRooms.h"
#include "../../DRODLib/MonsterPiece.h"

constexpr UINT uint_n1 = static_cast<UINT>(-1);

// -------------------------------------------------------------------
// ORB LINKER
// -------------------------------------------------------------------

RoomBuilder::OrbLinker RoomBuilder::OrbLinker::Link(const UINT wDoorX, const UINT wDoorY,
	const OrbAgentType eLinkType) const
{
	REQUIRE(pOrbData->GetAgentAt(wDoorX, wDoorY) == nullptr);

	pOrbData->AddAgent(wDoorX, wDoorY, eLinkType);

	return OrbLinker(pOrbData);
}

RoomBuilder::OrbLinker::OrbLinker(COrbData* pOrbData) : pOrbData(pOrbData)
{
}


// -------------------------------------------------------------------
// LONG MONSTER BUILDER
// -------------------------------------------------------------------

/**
 * Grow the tail in the direction `wO` by `wTiles` tiles.
 * @param wO Direction in which to grow
 * @param wTiles Number of tiles to grow.
 */
RoomBuilder::LongMonsterBuilder RoomBuilder::LongMonsterBuilder::GrowIn(
	const UINT wO,
	const UINT wTiles) const
{
	for (UINT i = 0; i < wTiles; i++)
	{
		const auto tail = GetTail();
		Plot(tail.wX + nGetOX(wO), tail.wY + nGetOY(wO));
	}

	return LongMonsterBuilder(this->pMonster);
}

/**
 * Grow the tail to position `wX`, `wY`. It has to be either perfectly orthogonally or
 * perfectly diagonally from the tail's end.
 * @param wX X coordinate of the position to which to grow the tail.
 * @param wY Y coordinate of the position to which to grow the tail.
 */
RoomBuilder::LongMonsterBuilder RoomBuilder::LongMonsterBuilder::GrowTo(
	const UINT wX,
	const UINT wY) const
{
	const auto tail = GetTail();

	return this->Grow(
		static_cast<int>(wX) - static_cast<int>(tail.wX),
		static_cast<int>(wY) - static_cast<int>(tail.wY)
	);
}

/**
 * Grow the tail by `wX` and `wY`. It has to be either perfectly orthogonally or
 * perfectly diagonally from the tail's end.
 * @param deltaX X coordinate of the position to which to grow the tail.
 * @param deltaY Y coordinate of the position to which to grow the tail.
 */
RoomBuilder::LongMonsterBuilder RoomBuilder::LongMonsterBuilder::Grow(
	const int deltaX,
	const int deltaY) const
{
	const int distanceX = abs(deltaX);
	const int distanceY = abs(deltaY);

	REQUIRE((distanceX == 0 || distanceY == 0 || distanceX == distanceY));

	return GrowIn(
		nGetO(sgn(deltaX), sgn(deltaY)),
		max(distanceX, distanceY)
	);
}

CMonster* RoomBuilder::LongMonsterBuilder::ToMonster() const
{
	return &this->pMonster;
}

void RoomBuilder::LongMonsterBuilder::Plot(const UINT wPieceX, const UINT wPieceY) const
{
	if (this->pMonster.wType == M_GENTRYII)
	{
		PlotGentryiiPiece(wPieceX, wPieceY);
	} else
	{
		PlotSerpentPiece(wPieceX, wPieceY);
	}
}

void RoomBuilder::LongMonsterBuilder::PlotSerpentPiece(UINT wPieceX, UINT wPieceY) const
{
	// Sorry for this mess of a function, but it works at least!

	REQUIRE((this->pMonster.wType == M_SERPENT
		|| this->pMonster.wType == M_SERPENTB
		|| this->pMonster.wType == M_SERPENTG));
	REQUIRE(GetRoomRef().IsValidColRow(wPieceX, wPieceY));
	REQUIRE(GetRoomRef().GetMonsterTypeAt(wPieceX, wPieceY) == M_NONE);

	UINT newTileType;
	UINT ultimateTileType = uint_n1;
	UINT ultimateTileX;
	UINT ultimateTileY;

	if (this->pMonster.Pieces.empty()){
		REQUIRE(abs((int)wPieceX - (int)this->pMonster.wX) + abs((int)wPieceY - (int)this->pMonster.wY) == 1);
		newTileType = GetSerpentTile(this->pMonster.wX, this->pMonster.wY, wPieceX, wPieceY);
	}
	else if (this->pMonster.Pieces.size() == 1){
		CMonsterPiece* ultimatePiece = this->GetPieceAtIndex(-1);
		REQUIRE(abs((int)wPieceX - (int)ultimatePiece->wX) + abs((int)wPieceY - (int)ultimatePiece->wY) == 1);

		ultimateTileType = this->GetSerpentTile(
			this->pMonster.wX, this->pMonster.wY,
			ultimatePiece->wX, ultimatePiece->wY,
			wPieceX, wPieceY
		);
		newTileType = GetSerpentTile(ultimatePiece->wX, ultimatePiece->wY, wPieceX, wPieceY);

		ultimateTileX = ultimatePiece->wX;
		ultimateTileY = ultimatePiece->wY;

		GetRoom()->RemoveMonsterFromTileArray(ultimatePiece);
		this->pMonster.Pieces.pop_back();
		ultimatePiece->Clear();
		delete ultimatePiece;
	}
	else
	{
		CMonsterPiece* ultimatePiece = this->GetPieceAtIndex(-1);
		CMonsterPiece* penultimatePiece = this->GetPieceAtIndex(-2);
		REQUIRE(abs((int)wPieceX - (int)ultimatePiece->wX) + abs((int)wPieceY - (int)ultimatePiece->wY) == 1);

		ultimateTileType = GetSerpentTile(penultimatePiece->wX, penultimatePiece->wY, ultimatePiece->wX, ultimatePiece->wY, wPieceX, wPieceY);
		newTileType = GetSerpentTile(ultimatePiece->wX, ultimatePiece->wY, wPieceX, wPieceY);

		ultimateTileX = ultimatePiece->wX;
		ultimateTileY = ultimatePiece->wY;

		GetRoom()->RemoveMonsterFromTileArray(ultimatePiece);
		this->pMonster.Pieces.pop_back();
		ultimatePiece->Clear();
		delete ultimatePiece;
	}

	if (ultimateTileType != uint_n1 && ultimateTileX != uint_n1 && ultimateTileY != uint_n1) {
		const auto ultimatePiece = new CMonsterPiece(&this->pMonster, ultimateTileType, ultimateTileX, ultimateTileY);
		GetRoom()->SetMonsterSquare(ultimatePiece);
		this->pMonster.Pieces.push_back(ultimatePiece);
	}

	if (newTileType != uint_n1){
		const auto newPiece = new CMonsterPiece(&this->pMonster, newTileType, wPieceX, wPieceY);
		GetRoom()->SetMonsterSquare(newPiece);
		this->pMonster.Pieces.push_back(newPiece);
	}
}

void RoomBuilder::LongMonsterBuilder::PlotGentryiiPiece(UINT wPieceX, UINT wPieceY) const
{
	REQUIRE(this->pMonster.wType == M_GENTRYII);

	const auto newPiece = new CMonsterPiece(&this->pMonster, T_GENTRYII, wPieceX, wPieceY);
	GetRoom()->SetMonsterSquare(newPiece);
	this->pMonster.Pieces.push_back(newPiece);
}

CMonsterPiece* RoomBuilder::LongMonsterBuilder::GetPieceAtIndex(int index) const
{
	if (index < 0)
	{
		index = this->pMonster.Pieces.size() + index;
	}

	REQUIRE(index < this->pMonster.Pieces.size());

	auto it = this->pMonster.Pieces.begin();
	std::advance(it, index);

	return *it;
}

UINT RoomBuilder::LongMonsterBuilder::GetSerpentTile(
	const UINT prevTileX, const UINT prevTileY,
	const UINT thisTileX, const UINT thisTileY,
	const UINT nextTileX, const UINT nextTileY)
{
	const bool hasNextTile = (nextTileX != uint_n1 && nextTileY != uint_n1);
	if (!hasNextTile){
		if (thisTileX == prevTileX)
			return thisTileY > prevTileY ? T_SNKT_S : T_SNKT_N;
		if (thisTileY == prevTileY)
			return thisTileX > prevTileX ? T_SNKT_E : T_SNKT_W;
	}
	else if (prevTileX == thisTileX && thisTileX == nextTileX){
		return T_SNK_NS;
	}
	else if (prevTileY == thisTileY && thisTileY == nextTileY){
		return T_SNK_EW;
	}
	else if (thisTileX == prevTileX){
		if (thisTileY > prevTileY)
			return thisTileX > nextTileX ? T_SNK_NW : T_SNK_NE;
		return thisTileX > nextTileX ? T_SNK_SW : T_SNK_SE;
	}
	else if (thisTileX == nextTileX){
		if (thisTileY > nextTileY)
			return thisTileX > prevTileX ? T_SNK_NW : T_SNK_NE;
		return thisTileX > prevTileX ? T_SNK_SW : T_SNK_SE;
	}

	throw std::invalid_argument("invalid serpent positions");
}

CCoord RoomBuilder::LongMonsterBuilder::GetTail() const
{
	if (this->pMonster.Pieces.empty())
	{
		return CCoord(this->pMonster.wX, this->pMonster.wY);
	} else
	{
		const auto pTail = this->pMonster.Pieces.back();
		return CCoord(pTail->wX, pTail->wY);
	}
}

RoomBuilder::LongMonsterBuilder::LongMonsterBuilder(CMonster* pMonster): pMonster(*pMonster)
{
}
RoomBuilder::LongMonsterBuilder::LongMonsterBuilder(CMonster& pMonster): pMonster(pMonster)
{
}

// -------------------------------------------------------------------
// Room Builder
// -------------------------------------------------------------------

void RoomBuilder::ClearRoom(){
	CTestDb::RegenerateRoom();
}

CCharacter* RoomBuilder::AddCharacter(const UINT wX, const UINT wY, const UINT wO, const UINT identity){
	CMonster *pMonster = GetRoom()->AddNewMonster(M_CHARACTER, wX, wY, false);
	pMonster->wO = wO;

	const auto pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
	pCharacter->dwScriptID = CTestDb::hold->GetNewScriptID();
	pCharacter->wLogicalIdentity = identity;
	pCharacter->wIdentity = identity;

	return pCharacter;
}


CCharacter* RoomBuilder::AddVisibleCharacter(const UINT wX, const UINT wY, const UINT wO, const UINT identity){
	CCharacter *pCharacter = RoomBuilder::AddCharacter(wX, wY, wO, identity);
	pCharacter->bVisible = true;

	GetRoom()->SetMonsterSquare(pCharacter);

	return pCharacter;
}


CMonster* RoomBuilder::AddMonster(const UINT wType, const UINT wX, const UINT wY, const UINT wO){
	CMonster *pMonster = GetRoom()->AddNewMonster(wType, wX, wY);
	pMonster->wO = wO;
	return pMonster;
}

CCharacter* RoomBuilder::AddMonsterWithWeapon(const UINT wType, const WeaponType weaponType, const UINT wX, const UINT wY, const UINT wO){
	CCharacter *pCharacter = AddVisibleCharacter(wX, wY, wO, wType);
	AddCommand(pCharacter, CCharacterCommand::CC_VarSet, ScriptVars::P_MONSTER_WEAPON, ScriptVars::Assign, weaponType);
	AddCommand(pCharacter, CCharacterCommand::CC_TurnIntoMonster);

	return pCharacter;
}

/**
 * Start creating a long monster. This places its head and returns a builder you can use to grow
 * the tail.
 */
RoomBuilder::LongMonsterBuilder RoomBuilder::AddLongMonster(
	const UINT wType, const UINT wX, const UINT wY, const UINT wO)
{
	REQUIRE((wType == M_SERPENT
		|| wType == M_SERPENTB
		|| wType == M_SERPENTG
		|| wType == M_GENTRYII));

	return LongMonsterBuilder(AddMonster(wType, wX, wY, wO));
}


void RoomBuilder::SaveRoom(){
	GetRoom()->Update();
}

void RoomBuilder::AddCommand(
	CCharacter *character,
	CCharacterCommand::CharCommand command,
	UINT x, UINT y,
	UINT w, UINT h,
	UINT flags)
{
	WSTRING string;
	RoomBuilder::AddCommand(character, command, x, y, w, h, flags, string);
}
void RoomBuilder::AddCommand(
	CCharacter *character,
	CCharacterCommand::CharCommand command,
	UINT x, UINT y,
	UINT w, UINT h,
	UINT flags,
	WSTRING label)
{
	CCharacterCommand pCommand;
	pCommand.command = command;
	pCommand.x = x;
	pCommand.y = y;
	pCommand.w = w;
	pCommand.h = h;
	pCommand.flags = flags;
	pCommand.label = std::move(label);

	character->commands.push_back(pCommand);
}
void RoomBuilder::AddCommand(
	CCharacter *character,
	CCharacterCommand::CharCommand command,
	UINT x, UINT y,
	UINT w, UINT h,
	UINT flags,
	const char *label)
{
	AddCommand(character, command, x, y, w, h, flags, UTF8ToUnicode(label));
}

COrbData *RoomBuilder::AddOrbDataToTile(const UINT wX, const UINT wY, const OrbType eOrbType)
{
	auto& room = GetRoomRef();
	auto pOrbData = room.GetOSquare(wX, wY) == T_PRESSPLATE
		? room.GetPressurePlateAtCoords(wX, wY)
		: room.GetOrbAtCoords(wX, wY);

	if (pOrbData != nullptr) {
		REQUIRE(pOrbData->eType == eOrbType);
	} else {
		pOrbData = room.AddOrbToSquare(wX, wY);
		pOrbData->eType = eOrbType;
	}

	return pOrbData;
}

void RoomBuilder::AddWallLight(const UINT wX, const UINT wY, const UINT type){
	if (type >= NUM_LIGHT_TYPES){
		throw std::invalid_argument("Invalid light type selected");
	}

	GetRoom()->tileLights.Add(wX, wY, WALL_LIGHT + type);
}

void RoomBuilder::AddCeilingLight(const UINT wX, const UINT wY, const UINT type){
	if (type >= NUM_LIGHT_TYPES){
		throw std::invalid_argument("Invalid light type selected");
	}

	GetRoom()->tileLights.Add(wX, wY, type + 1);
}

void RoomBuilder::AddCeilingDarkness(const UINT wX, const UINT wY, const UINT type){
	if (type >= NUM_DARK_TYPES){
		throw std::invalid_argument("Invalid light type selected");
	}

	GetRoom()->tileLights.Add(wX, wY, LIGHT_OFF + type + 1);
}

void RoomBuilder::LinkOrb(const UINT wOrbX, const UINT wOrbY, const UINT wDoorX, const UINT wDoorY, const OrbAgentType eLinkType){
	COrbData *pEditOrb;
	if (GetRoom()->GetOSquare(wOrbX, wOrbY) == T_PRESSPLATE){
		pEditOrb = GetRoom()->GetPressurePlateAtCoords(wOrbX, wOrbY);
	} else {
		pEditOrb = GetRoom()->GetOrbAtCoords(wOrbX, wOrbY);
	}

	if (pEditOrb == nullptr){
		throw std::invalid_argument("No orb on given tile");
	}

	pEditOrb->AddAgent(wDoorX, wDoorY, eLinkType);
}

void RoomBuilder::Plot(const UINT tileType, const UINT wX, const UINT wY){
	PlotRect(tileType, wX, wY, wX, wY);
}

RoomBuilder::OrbLinker RoomBuilder::PlotPlate(UINT wX, UINT wY, OrbType eOrbType)
{
	return PlotPlate(wX, wY, wX, wY, eOrbType);
}

RoomBuilder::OrbLinker RoomBuilder::PlotPlate(const UINT wX, const UINT wY, const UINT wEndX, const UINT wEndY, OrbType eOrbType)
{
	auto& room = GetRoomRef();
	REQUIRE(room.IsValidColRow(wX, wY));
	REQUIRE(room.IsValidColRow(wEndX, wEndY));

	PlotRect(T_PRESSPLATE, wX, wY, wEndX, wEndY);

	auto pOrbData = AddOrbDataToTile(wX, wY, eOrbType);

	REQUIRE(pOrbData->eType == eOrbType);

	return OrbLinker(pOrbData);
}

void RoomBuilder::PlotObstacle(const UINT obstacleType, const UINT wX, const UINT wY) {
	PlotObstacle(obstacleType, wX, wY, wX, wY);
}

void RoomBuilder::PlotObstacle(const UINT obstacleType, const UINT startX, const UINT startY, const UINT endX, const UINT endY) {
	for (UINT x = startX; x <= endX; ++x){
		for (UINT y = startY; y <= endY; ++y){
			GetRoom()->Plot(x, y, T_OBSTACLE);
			GetRoom()->SetTParam(x, y, obstacleType);
		}
	}
}

void RoomBuilder::PlotToken(const RoomTokenType tokenType, const UINT wX, const UINT wY){
	PlotRect(T_TOKEN, wX, wY, wX, wY);
	GetRoom()->SetTParam(wX, wY, tokenType);
}

void RoomBuilder::PlotStation(const UINT wX, const UINT wY, const UINT stationType)
{
	PlotRect(T_STATION, wX, wY, wX, wY);
	GetRoom()->stations.push_back(new CStation(wX, wY, GetRoom()));
	GetRoom()->SetTParam(wX, wY, stationType);
	GetRoom()->stations.back()->UpdateType();
}

void RoomBuilder::PlotRect(const UINT tileType, const UINT startX, const UINT startY, const UINT endX, const UINT endY){
	for (UINT x = startX; x <= endX; ++x){
		for (UINT y = startY; y <= endY; ++y){
			GetRoomRef().Plot(x, y, tileType);
		}
	}
}

CDbRoom* RoomBuilder::GetRoom(){
	return CTestDb::room;
}

CDbRoom& RoomBuilder::GetRoomRef(){
	return *(CTestDb::room);
}
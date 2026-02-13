#include "RoomBuilder.h"
#include "../../DRODLib/DbRooms.h"
#include "../../DRODLib/MonsterPiece.h"

void RoomBuilder::ClearRoom(){
	CTestDb::RegenerateRoom();
}

CCharacter* RoomBuilder::AddCharacter(const UINT wX, const UINT wY, const UINT wO, const UINT identity){
	CMonster *pMonster = GetRoom()->AddNewMonster(M_CHARACTER, wX, wY, false);
	pMonster->wO = wO;

	CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
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

void RoomBuilder::AddSerpentPiece(CSerpent* serpent, const UINT pieceX, const UINT pieceY){
	UINT newTileType = UINT(-1);
	UINT ultimateTileType = UINT(-1);
	UINT ultimateTileX = UINT(-1);
	UINT ultimateTileY = UINT(-1);

	if (serpent->Pieces.size() == 0){
		newTileType = GetSerpentTile(serpent->wX, serpent->wY, pieceX, pieceY);
	}
	else if (serpent->Pieces.size() == 1){
		CMonsterPiece* ultimatePiece = GetPieceAtIndex(serpent->Pieces, serpent->Pieces.size() - 1);
		ultimateTileType = GetSerpentTile(serpent->wX, serpent->wY, ultimatePiece->wX, ultimatePiece->wY, pieceX, pieceY);
		newTileType = GetSerpentTile(ultimatePiece->wX, ultimatePiece->wY, pieceX, pieceY);

		ultimateTileX = ultimatePiece->wX;
		ultimateTileY = ultimatePiece->wY;

		GetRoom()->RemoveMonsterFromTileArray(ultimatePiece);
		serpent->Pieces.pop_back();
		ultimatePiece->Clear();
		delete ultimatePiece;
	}
	else
	{
		CMonsterPiece* ultimatePiece = GetPieceAtIndex(serpent->Pieces, serpent->Pieces.size() - 1);
		CMonsterPiece* penultimatePiece = GetPieceAtIndex(serpent->Pieces, serpent->Pieces.size() - 2);
		ultimateTileType = GetSerpentTile(penultimatePiece->wX, penultimatePiece->wY, ultimatePiece->wX, ultimatePiece->wY, pieceX, pieceY);
		newTileType = GetSerpentTile(ultimatePiece->wX, ultimatePiece->wY, pieceX, pieceY);

		ultimateTileX = ultimatePiece->wX;
		ultimateTileY = ultimatePiece->wY;

		GetRoom()->RemoveMonsterFromTileArray(ultimatePiece);
		serpent->Pieces.pop_back();
		ultimatePiece->Clear();
		delete ultimatePiece;
	}

	if (ultimateTileType != UINT(-1)){
		CMonsterPiece *ultimatePiece = new CMonsterPiece(serpent, ultimateTileType, ultimateTileX, ultimateTileY);
		GetRoom()->SetMonsterSquare(ultimatePiece);
		serpent->Pieces.push_back(ultimatePiece);
	}

	if (newTileType != UINT(-1)){
		CMonsterPiece *newPiece = new CMonsterPiece(serpent, newTileType, pieceX, pieceY);
		GetRoom()->SetMonsterSquare(newPiece);
		serpent->Pieces.push_back(newPiece);
	}
}

void RoomBuilder::AddGentryiiPiece(CGentryii* gentryii, const UINT pieceX, const UINT pieceY){
    CMonsterPiece* newPiece = new CMonsterPiece(gentryii, T_GENTRYII, pieceX, pieceY);
    GetRoom()->SetMonsterSquare(newPiece);
    gentryii->Pieces.push_back(newPiece);
}

CCharacter* RoomBuilder::AddMonsterWithWeapon(const UINT wType, const WeaponType weaponType, const UINT wX, const UINT wY, const UINT wO){
	CCharacter *pCharacter = AddVisibleCharacter(wX, wY, wO, wType);
	AddCommand(pCharacter, CCharacterCommand::CC_VarSet, ScriptVars::P_MONSTER_WEAPON, ScriptVars::Assign, weaponType);
	AddCommand(pCharacter, CCharacterCommand::CC_TurnIntoMonster);

	return pCharacter;
}


void RoomBuilder::SaveRoom(){
	GetRoom()->Update();
}

void RoomBuilder::AddCommand(CCharacter* character, CCharacterCommand::CharCommand command, UINT x, UINT y, UINT w, UINT h, UINT flags){
	WSTRING string;
	RoomBuilder::AddCommand(character, command, x, y, w, h, flags, string);
}
void RoomBuilder::AddCommand(CCharacter* character, CCharacterCommand::CharCommand command, UINT x, UINT y, UINT w, UINT h, UINT flags, WSTRING label){
	CCharacterCommand pCommand;
	pCommand.command = command;
	pCommand.x = x;
	pCommand.y = y;
	pCommand.w = w;
	pCommand.h = h;
	pCommand.flags = flags;
	pCommand.label = label;

	character->commands.push_back(pCommand);
}
void RoomBuilder::AddCommand(CCharacter *character, CCharacterCommand::CharCommand command, UINT x, UINT y, UINT w, UINT h, UINT flags, const char *label)
{
	AddCommand(character, command, x, y, w, h, flags, UTF8ToUnicode(label).c_str());
}

COrbData *RoomBuilder::AddOrbDataToTile(const UINT wX, const UINT wY, const OrbType eOrbType)
{
    COrbData* pOrbData = GetRoom()->AddOrbToSquare(wX, wY);
	pOrbData->eType = eOrbType;

	return pOrbData;
}

void RoomBuilder::AddWallLight(const UINT wX, const UINT wY, const UINT type){
	if (type < 0 || type >= NUM_LIGHT_TYPES){
		throw "Invalid light type selected";
	}

	GetRoom()->tileLights.Add(wX, wY, WALL_LIGHT + type);
}

void RoomBuilder::AddCeilingLight(const UINT wX, const UINT wY, const UINT type){
	if (type < 0 || type >= NUM_LIGHT_TYPES){
		throw "Invalid light type selected";
	}

	GetRoom()->tileLights.Add(wX, wY, type + 1);
}

void RoomBuilder::AddCeilingDarkness(const UINT wX, const UINT wY, const UINT type){
	if (type < 0 || type >= NUM_DARK_TYPES){
		throw "Invalid darkness type selected";
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

	if (pEditOrb == NULL){
		throw "No orb on given tile";
	}

	pEditOrb->AddAgent(wDoorX, wDoorY, eLinkType);
}

void RoomBuilder::Plot(const UINT tileType, const UINT wX, const UINT wY){
	PlotRect(tileType, wX, wY, wX, wY);
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
			GetRoom()->Plot(x, y, tileType);
		}
	}
}

CDbRoom* RoomBuilder::GetRoom(){
	return CTestDb::room;
}

UINT RoomBuilder::GetSerpentTile(const UINT prevTileX, const UINT prevTileY,
	const UINT thisTileX, const UINT thisTileY,
	const UINT nextTileX, const UINT nextTileY)
{
	const bool hasNextTile = (nextTileX != UINT(-1) && nextTileY != UINT(-1));
	if (!hasNextTile){
		if (thisTileX == prevTileX)
			return thisTileY > prevTileY ? T_SNKT_S : T_SNKT_N;
		else if (thisTileY == prevTileY)
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
		else
			return thisTileX > nextTileX ? T_SNK_SW : T_SNK_SE;
	}
	else if (thisTileX == nextTileX){
		if (thisTileY > nextTileY)
			return thisTileX > prevTileX ? T_SNK_NW : T_SNK_NE;
		else
			return thisTileX > prevTileX ? T_SNK_SW : T_SNK_SE;
	}

	throw "invalid serpent positions";
}

CMonsterPiece* RoomBuilder::GetPieceAtIndex(MonsterPieces piecesList, const UINT index){
	std::list<CMonsterPiece*>::iterator it = piecesList.begin();
	std::advance(it, index);

	return *it;
}
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
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Michael Welsh Duggan (md5i), Richard Cookney (timeracer), John Wm. Wicks (j_wicks),
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//CurrentGame.cpp.
//Implementation of CCurrentGame.

#include "CurrentGame.h"
#include "Db.h"
#include "DbPlayers.h"
#include "DbProps.h"
#include "DbXML.h"
#include "CueEvents.h"
#include "Construct.h"
#include "GameConstants.h"
#include "Halph.h"
#include "I18N.h"
#include "Monster.h"
#include "MonsterFactory.h"
#include "MonsterPiece.h"
#include "Character.h"
#include "Clone.h"
#include "EvilEye.h"
#include "FegundoAshes.h"
#include "Mimic.h"
#include "Pathmap.h"
#include "RockGiant.h"
#include "TemporalClone.h"
#include "TileConstants.h"
#include "NetInterface.h"
#include "SettingsKeys.h"
#include "Waterskipper.h"

#include "../Texts/MIDs.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/CoordStack.h>
#include <BackEndLib/Exception.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Ports.h>
#include <BackEndLib/SysTimer.h>
#include <BackEndLib/Wchar.h>

#define TAG_ESCAPE   (UINT)(-2)    //same as in FrontEndLib/Widget.*

#define NO_CHECKPOINT (static_cast<UINT>(-1))

queue<DEMO_UPLOAD*> CCurrentGame::demosForUpload;

//Game character/monster constant that speaker refers to
const UINT SpeakerConstant[Speaker_Count] = {
	M_BEETHRO, M_HALPH, M_SLAYER, M_NEGOTIATOR, UINT(M_CUSTOM), UINT(M_CUSTOM),
	M_CITIZEN1, M_CITIZEN2, M_GOBLINKING, M_GOBLIN,
	M_INSTRUCTOR, M_MUDCOORDINATOR, M_ROCKGOLEM, M_TARTECHNICIAN, M_GUARD, M_EYE,
	M_CITIZEN, M_STALWART, M_ROACH, M_QROACH, M_REGG, M_WWING, M_EYE,
	M_SERPENT, M_TARMOTHER, M_TARBABY, M_BRAIN, M_MIMIC, M_SPIDER,
	M_SERPENTG, M_SERPENTB, M_WATERSKIPPER, M_SKIPPERNEST, M_AUMTLICH, M_CLONE,
	M_DECOY, M_WUBBA, M_SEEP, M_FEGUNDO, M_FEGUNDOASHES, M_MUDMOTHER,
	M_MUDBABY, M_GELMOTHER, M_GELBABY, M_ROCKGIANT, M_CITIZEN3, M_CITIZEN4,
	M_BEETHRO_IN_DISGUISE, M_SLAYER2, M_HALPH2, UINT(M_CUSTOM), M_GUNTHRO, UINT(M_PLAYER), M_STALWART2,
	M_ARCHITECT, M_CONSTRUCT, M_GENTRYII, M_TEMPORALCLONE, M_FLUFFBABY
};

//*****************************************************************************
TemporalSplitData::TemporalSplitData()
	: queuingTurn(-1)
	, x(-1), y(-1)
{ }

void TemporalSplitData::add_move(int command, bool moved)
{
	ASSERT(queuing());

	switch (command) {
		case CMD_CC: case CMD_C: case CMD_WAIT:
			this->player_commands.push_back(command);
		break;

		case CMD_NW: case CMD_N: case CMD_NE: case CMD_W:
		case CMD_E: case CMD_SW: case CMD_S: case CMD_SE:
			this->player_commands.push_back(moved ? command : ConvertToBumpCommand(command));
		break;

		default: break;
	}
}

void TemporalSplitData::clear() {
	queuingTurn = -1;
	x = y = -1;
	player_commands.clear();
}

bool TemporalSplitData::start(int turn, UINT x, UINT y) {
	//Can only activate one split point at a time.
	if (queuing())
		return false;

	queuingTurn = turn;
	this->x = x;
	this->y = y;
	player_commands.clear();

	return true;
}
bool TemporalSplitData::queuing() const {
	return queuingTurn >= 0;
}

//
//Protected methods.
//

//*****************************************************************************
CCurrentGame::CCurrentGame()
//Constructor.
	: pRoom(NULL)
	, pLevel(NULL)
	, pHold(NULL)
	, bNoSaves(false) // Clear() does not set this
	, pSnapshotGame(NULL)
{
	//Zero resource members before calling Clear().
	Clear();
}

//
//Public methods.
//

//*****************************************************************************
CCurrentGame::~CCurrentGame()
//Destructor.
{
	if (this->bIsDemoRecording)
	{
		if (!EndDemoRecording())
		{
			CFiles f;
			f.AppendErrorLog("Failed to save a demo when recording ended." NEWLINE);
		}
	}

	if (this->Commands.IsFrozen()) this->Commands.Unfreeze();

	Clear();
}

//*****************************************************************************
WSTRING CCurrentGame::AbbrevRoomLocation()
	//Prepend room position to non-empty (player viewable) demo descriptions.
{
	WSTRING descText;

	//Hold name.
	WSTRING holdName = static_cast<const WCHAR *>(this->pHold->NameText);
	WSTRING abbrevHoldName;
	static const UINT MAX_HOLD_NAME = 8;
	if (holdName.size() <= MAX_HOLD_NAME)
		descText += holdName;
	else
	{
		//Try to abbreviate by taking only the first letter from each word
		abbrevHoldName = filterFirstLettersAndNumbers(holdName);
		descText += abbrevHoldName;
	}
	descText += wszColon;
	descText += wszSpace;

	//Level name.
	descText += this->pLevel->NameText;
	descText += wszColon;
	descText += wszSpace;

	//Room name.
	WSTRING abbrevRoomPosition;
	this->pRoom->GetLevelPositionDescription(abbrevRoomPosition, true);
	descText += abbrevRoomPosition;

	return descText;
}

//*****************************************************************************
CMonster* CCurrentGame::AddNewEntity(
//Adds a new monster in the room of the indicated type.
//
//Params:
	CCueEvents& CueEvents,
	const UINT identity, const UINT wX, const UINT wY, const UINT wO)
{
	if (!IsValidOrientation(wO))
		return NULL; //invalid
	if (bIsSerpentOrGentryii(identity) || identity == M_FEGUNDOASHES || identity == M_CHARACTER)
		return NULL; //not supported
	if (IsValidMonsterType(identity))
	{
		CMonster *pMonster = this->pRoom->AddNewMonster(identity, wX, wY);
		if (identity != M_REGG) //must retain the constructed orientation
		{
			const bool bHasOrientation = pMonster->HasOrientation();
			pMonster->wO = bHasOrientation ? wO : NO_ORIENTATION;
			if (bHasOrientation && pMonster->wO == NO_ORIENTATION)
				pMonster->wO = NW; //default
		}
		pMonster->bIsFirstTurn = true;

		if (bIsBeethroDouble(identity) && this->swordsman.IsInRoom()) {
			CPlayerDouble *pDouble = DYN_CAST(CPlayerDouble*, CMonster*, pMonster);
			pDouble->weaponType = this->swordsman.GetActiveWeapon();
			pDouble->SetWeaponSheathed();
		}

		//Affect tile being placed on.
		if (this->pRoom->GetOSquare(wX, wY) == T_PRESSPLATE && pMonster->CanPressPressurePlates())
			this->pRoom->ActivateOrb(wX, wY, CueEvents, OAT_PressurePlate);
		this->pRoom->CreatePathMaps();
		return pMonster;
	}

	if (identity == M_NONE)
	{
		//Remove any entity occupying this tile.
		this->pRoom->KillMonsterAtSquare(wX, wY, CueEvents, true);
		return NULL;
	}

	if (identity >= CUSTOM_CHARACTER_FIRST &&
			!this->pHold->GetCharacter(identity))
		return NULL; //do nothing if this is an invalid custom character type

	//Add NPC to the room.
	CMonster *pNew = CMonsterFactory::GetNewMonster((MONSTERTYPE)M_CHARACTER);
	ASSERT(pNew);
	pNew->wX = pNew->wPrevX = wX;
	pNew->wY = pNew->wPrevY = wY;
	pNew->wO = pNew->wPrevO = wO;
	pNew->bIsFirstTurn = true; //don't reprocess until next turn

	//Set up NPC info.
	CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pNew);
	pCharacter->wLogicalIdentity = identity;
	pCharacter->SetCurrentGame(this); //will assign the default script for custom NPCs
	pCharacter->dwScriptID = this->pHold->GetNewScriptID();
	pCharacter->bNewEntity = true;

	//Place in room if visible.
	bool bVisible = pCharacter->IsVisible();
	if (bVisible &&
			(this->pRoom->GetMonsterAtSquare(wX, wY) || IsPlayerAt(wX, wY)))
	{
		//When the room tile is already occupied, the NPC must begin invisible.
		if (pCharacter->commands.empty())
		{
			//There is no default script for the NPC, so it would never appear.
			//Just pretend it was never added to the room.
			delete pCharacter;
			return NULL;
		}

		pCharacter->bVisible = false;
		bVisible = false;
	}
	this->pRoom->LinkMonster(pNew, bVisible);

	//Affect tile being placed on.
	if (bVisible && this->pRoom->GetOSquare(wX, wY) == T_PRESSPLATE && !pCharacter->CanPressPressurePlates())
		this->pRoom->ActivateOrb(wX, wY, CueEvents, OAT_PressurePlate);

	//Set up the NPC's properties by running the default script.
	//Guard against creating new scripted entities that immediately
	//create new scripted entities, ad infinitum.
	static const UINT MAX_ADD_NEW_ENTITY_RECURSE_DEPTH = 3;
	if (this->wAddNewEntityDepth >= MAX_ADD_NEW_ENTITY_RECURSE_DEPTH)
	{
		//Log the occurrence and don't let the new NPC run any script commands
		//so that the recursion won't continue.
		string str = "AddNewEntity: Recursive depth limit reached.  ID = ";
		char temp[16];
		str += _itoa(identity, temp, 10);
		str += NEWLINE;
		CFiles f;
		f.AppendUserLog(str.c_str());
	} else {
		const bool bExec = ExecutingNoMoveCommands();
		SetExecuteNoMoveCommands();

		++this->wAddNewEntityDepth;
		pCharacter->Process(CMD_WAIT, CueEvents);
		--this->wAddNewEntityDepth;

		SetExecuteNoMoveCommands(bExec);
	}

	return pCharacter;
}

//*****************************************************************************
void CCurrentGame::BeginDemoRecording(
//Begins demo recording.  Database writes to store the demo will occur when the
//swordsman exits a room or EndDemoRecording() is called.
//
//Params:
	const WCHAR *pwczSetDescription, //(in)  Description stored with demo record(s).
	const bool bUseCurrentTurnNo)    //[default=true]
{
	ASSERT(pwczSetDescription);
	ASSERT(!this->bIsDemoRecording);

	//Commands should not be frozen while recording.  Recording requires commands to
	//be added for each call to ProcessCommand().  It is possible to have some carefully
	//thought-out mixture of the two states, in which case this assertion can be changed.
	ASSERT(!this->Commands.IsFrozen());

	//Set recording information.
	this->DemoRecInfo.clear();
	this->DemoRecInfo.dwDescriptionMessageID = AddMessageText(pwczSetDescription);
	this->DemoRecInfo.wBeginTurnNo = bUseCurrentTurnNo ? this->wTurnNo : 0;
	this->DemoRecInfo.SetFlag(CDbDemo::Usermade);

	this->bIsDemoRecording = true;
}

//*****************************************************************************
void CCurrentGame::Clear(
//Frees resources associated with this object and zeros members.
//
//Params:
	const bool bNewGame)  //(in)   whether new game is starting [default=true]
{
	CDbSavedGame::Clear(bNewGame);  //Resets the Explored and Conquered room lists.

	delete this->pRoom;
	this->pRoom = NULL;

	delete this->pLevel;
	this->pLevel = NULL;

	if (bNewGame)
	{
		//Only reset the hold, room lists, and mastery if a new game is beginning
		delete this->pHold;
		this->pHold=NULL;

		this->bHoldMastered = this->bHoldCompleted = false;
	}

	this->pEntrance = NULL;

	this->swordsman.Clear();

	this->wSpawnCycleCount = this->wPlayerTurn = this->wTurnNo = 0;
	this->dwStartTime = 0;
	this->bHalfTurn = false;

	this->ambientSounds.clear();
	for (vector<SpeechLog>::const_iterator iter = this->roomSpeech.begin();
			iter != this->roomSpeech.end(); ++iter)
		delete iter->pSpeechCommand;
	this->roomSpeech.clear();

	this->wMonsterKills = this->wMonsterKillCombo = 0;

	this->wMonstersKilledRecently = 0;
	this->bLotsOfMonstersKilled = false;

	this->bBrainSensesSwordsman = false;
	this->wLastCheckpointX = this->wLastCheckpointY = NO_CHECKPOINT;
	this->checkpointTurns.clear();

	this->bRoomExitLocked = false;
	this->conquerTokenTurn = NO_CONQUER_TOKEN_TURN;

	this->bSwordsmanOutsideRoom = false;
	this->dwAutoSaveOptions = ASO_DEFAULT;
	this->bIsNewRoom = false;
	this->bExecuteNoMoveCommands = false;
	this->wAddNewEntityDepth = 0;

	this->bIsGameActive = this->bIsDemoRecording = false;

	this->dwCutScene = 0;
	this->bWaitedOnHotFloorLastTurn = false;
	this->bWasRoomConqueredAtTurnStart = false;
	this->bIsLeavingLevel = false;

	this->DemoRecInfo.clear();

	this->UnansweredQuestions.clear();
	this->customRoomLocationText.clear();
	this->displayFilter = ScriptFlag::D_Normal;
	this->threatClockDisplay = 0;
	this->scriptReturnX = this->scriptReturnY = this->scriptReturnF = 0;
	this->playerLight = this->playerLightType = 0;
	this->persistingImageOverlays.clear();
	this->imageOverlayNextID = 0;

	this->temporalSplit.clear();
	this->activatingTemporalSplit = 0;

	this->pDyingEntity = NULL;
	this->pKillingEntity = NULL;

	delete this->pSnapshotGame;
	this->pSnapshotGame = NULL;
	this->dwComputationTime = 0;
	this->numSnapshots = 0;
	this->dwComputationTimePerSnapshot = 500; //ms

	ResetCutSceneStartTurn();
	this->bMusicStyleFrozen = false;
	this->music.reset();
}

//*****************************************************************************
void CCurrentGame::DiffVarValues(const VARMAP& vars1, const VARMAP& vars2, set<VarNameType>& diff)
//Outputs the set of vars that are different between vars1 and vars2.
//
//ASSUME: vars2 is a superset of vars1
{
	diff.clear();

	for (VARMAP::const_iterator var2 = vars2.begin(); var2 != vars2.end(); ++var2)
	{
		VARMAP::const_iterator var1 = vars1.find(var2->first);
		if (var1 != vars1.end())
		{
			//Var is present in both maps.  Check type and value.
			const VarMapInfo& v1 = var1->second;
			const VarMapInfo& v2 = var2->second;
			if (v1.bInteger == v2.bInteger)
			{
				//Same type.  Check value.
				if (v1.bInteger)
				{
					if (v1.val == v2.val)
						continue; //same
				} else {
					if (!v1.wstrVal.compare(v2.wstrVal))
						continue; //same
				}
			}
		} else {
			//Var is absent from vars1, but if value in vars2 is set to the default, then ignore it.
			const VarMapInfo& v2 = var2->second;
			if (v2.bInteger)
			{
				if (!v2.val)
					continue; //default value -- ignore
			} else {
				if (v2.wstrVal.empty())
					continue; //default value -- ignore
			}
		}

		//This var is different.
		diff.insert(var2->first);
	}
}

//*****************************************************************************
UINT CCurrentGame::EndDemoRecording()
//Ends demo recording, which may cause database to be updated with demo information.
//
//Returns:
//DemoID of first and maybe only demo in series of demos recorded (one per room) or
//0 if no commands recorded.
{
	ASSERT(this->bIsDemoRecording);

	//Commands should not be frozen while recording.  Recording requires commands to
	//be added for each call to ProcessCommand().  It is possible to have some carefully
	//thought-out mixture of the two states, in which case this assertion can be changed.
	ASSERT(!this->Commands.IsFrozen());

	this->bIsDemoRecording = false;

	//If no commands were recorded, no database update for the current room is needed.
	if (this->DemoRecInfo.wBeginTurnNo >= this->wTurnNo)
		return TAG_ESCAPE;

	//Save the current room state and commands to a demo.
	WriteCurrentRoomDemo(this->DemoRecInfo, false, false);

	return this->DemoRecInfo.dwFirstDemoID;
}


//***************************************************************************************
//Evaluate a calculated function in context of the current game
int CCurrentGame::EvalPrimitive(ScriptVars::PrimitiveType ePrimitive, const vector<int>& params)
{
	ASSERT(params.size() == ScriptVars::getPrimitiveRequiredParameters(ePrimitive));

	switch (ePrimitive) {
		case ScriptVars::P_Abs:
			return abs(params[0]);
		case ScriptVars::P_Orient:
		{
			const int dx = sgn(params[0]);
			const int dy = sgn(params[1]);
			return nGetO(dx, dy);
		}
		case ScriptVars::P_Facing:
		{
			int dx = params[0];
			int dy = params[1];
			//If one of the four compass directions is more direct than a diagonal,
			//snap to it.
			const int absDx = abs(dx), absDy = abs(dy);
			if (absDx > 2 * absDy)
				dy = 0;
			else if (absDy > 2 * absDx)
				dx = 0;
			return nGetO(sgn(dx), sgn(dy));
		}
		case ScriptVars::P_OrientX:
		case ScriptVars::P_OrientY:
		case ScriptVars::P_RotateCW:
		case ScriptVars::P_RotateCCW:
		{
			const int o = params[0];
			if (!IsValidOrientation(o))
				return o;
			switch (ePrimitive) {
				case ScriptVars::P_OrientX: return nGetOX(o);
				case ScriptVars::P_OrientY: return nGetOY(o);
				case ScriptVars::P_RotateCW: return nNextCO(o);
				case ScriptVars::P_RotateCCW: return nNextCCO(o);
			}
		}
		case ScriptVars::P_RotateDist:
		{
			UINT wO1 = params[0];
			UINT wO2 = params[1];
			UINT wTurns = 0;

			if (!(IsValidOrientation(wO1) && IsValidOrientation(wO2)) ||
				wO1 == NO_ORIENTATION || wO2 == NO_ORIENTATION) {
				return 0;
			}

			while (wO1 != wO2) {
				wO1 = nNextCO(wO1);
				++wTurns;
				ASSERT(wTurns < 8);
			}

			return wTurns <= 4 ? wTurns : 8 - wTurns;
		}
		case ScriptVars::P_Min:
			return min(params[0], params[1]);
		case ScriptVars::P_Max:
			return max(params[0], params[1]);
		case ScriptVars::P_Dist0: //L-infinity norm
		{
			const int deltaX = abs(params[2] - params[0]);
			const int deltaY = abs(params[3] - params[1]);
			return max(deltaX, deltaY);
		}
		case ScriptVars::P_Dist1: //L-1 norm (Manhattan distance)
		{
			const int deltaX = params[2] - params[0];
			const int deltaY = params[3] - params[1];
			return abs(deltaX) + abs(deltaY);
		}
		case ScriptVars::P_Dist2: //L-2 norm (Euclidean distance)
		{
			const int deltaX = params[2] - params[0];
			const int deltaY = params[3] - params[1];
			return int(sqrt(deltaX * deltaX + deltaY * deltaY));
		}
		case ScriptVars::P_ArrowDir:
		{
			const UINT tile = this->pRoom->GetFSquare(params[0], params[1]);
			return getForceArrowDirection(tile);
		}
		case ScriptVars::P_RoomTile:
		{
			switch (params[2]) {
				case 0: return this->pRoom->GetOSquare(params[0], params[1]);
				case 1: return this->pRoom->GetFSquare(params[0], params[1]);
				case 2: return this->pRoom->GetTSquare(params[0], params[1]);
				default: return 0;
			}
		}
		case ScriptVars::P_MonsterType:
		{
			CMonster* pMonster = this->pRoom->GetMonsterAtSquare(params[0], params[1]);
			if (pMonster) {
				return pMonster->wType;
			}
			return -1;
		}
		break;
	}

	return 0;
}

//*****************************************************************************
WSTRING CCurrentGame::ExpandText(const WCHAR* wText,
	CCharacter *pCharacter) //character, to query its local vars [default=NULL]
//Translate escape sequences embedded in text string to literal values.
{
	if (!wText)
		return WSTRING();

	WSTRING wStr, wEscapeStr;
	UINT wIndex;
	bool bEscapeSeq = false;
	for (wIndex=0; wText[wIndex] != 0; ++wText)
	{
		if (wText[wIndex] == '$')
		{
			if ((bEscapeSeq = !bEscapeSeq))
				continue; //just started what looks like an escape sequence
			if (wEscapeStr.empty())
				wStr += wszDollarSign; //$$
			else
			{
				//Process special escape sequences or variables.
				static const WCHAR newlineEscape[] = { We('\\'),We('n'),We(0) };
				if (!WCScmp(newlineEscape, wEscapeStr.c_str()))
				{
					wStr += wszCRLF;
				} else {
					//Resolve var name.
					WCHAR wIntText[20];
					//Is it a predefined var?
					const ScriptVars::Predefined eVar = ScriptVars::parsePredefinedVar(wEscapeStr);
					InputCommands::DCMD reserved_lookup_id;
					if (eVar != ScriptVars::P_NoVar)
					{
						//Yes.
						if (pCharacter) {
							wStr += pCharacter->getPredefinedVar(eVar);
						} else {
							if (ScriptVars::IsStringVar(eVar)) {
								wStr += getStringVar(eVar);
							} else {
								const UINT val = getVar(eVar);
								wStr += _itoW(int(val), wIntText, 10);
							}
						}
					} else if (ScriptVars::IsCharacterLocalVar(wEscapeStr) && pCharacter) {
						wStr += pCharacter->getLocalVarString(wEscapeStr);
					} else if ((reserved_lookup_id = InputCommands::getCommandIDByVarName(wEscapeStr)) < InputCommands::DCMD_Count) {
						//Is it a player input button?
						wStr += getTextForInputCommandKey(reserved_lookup_id);
					} else {
						//Is it a hold-local var?
						const UINT varID = this->pHold->GetVarID(wEscapeStr.c_str());
						if (varID)
						{
							//Yes -- get its value, if defined.
							char *varName = this->pHold->getVarAccessToken(wEscapeStr.c_str());
							const UNPACKEDVARTYPE vType = this->stats.GetVarType(varName);
							const bool bExistingIntValue = vType == UVT_int || vType == UVT_uint;
							if (bExistingIntValue)
							{
								//Integer.
								const int nVal = this->stats.GetVar(varName, int(0));
								wStr += _itoW(nVal, wIntText, 10);
							} else if (vType == UVT_wchar_string || vType == UVT_unknown) {
								//A text string.
								wStr += this->stats.GetVar(varName, wszEmpty);
							}
						} else {
							//Might be a complex expression.  Does it parse?
							UINT index=0;
							if (CCharacter::IsValidExpression(wEscapeStr.c_str(), index, this->pHold))
							{
								index=0;
								const int nVal = CCharacter::parseExpression(wEscapeStr.c_str(), index, this);
								wStr += _itoW(nVal, wIntText, 10);
							}
						}
					}
				}
				wEscapeStr.resize(0);
			}
		} else {
			if (bEscapeSeq)
				wEscapeStr += wText[wIndex];
			else
				wStr += wText[wIndex];
		}
	}

	//If there are non-paired var name delimiters, just add this text to the string.
	if (!wEscapeStr.empty())
		wStr += wEscapeStr;

	return wStr;
}

//*****************************************************************************
bool CCurrentGame::GetNearestEntranceForHorn(
	UINT wHornX, UINT wHornY, UINT wSummonType, UINT& wX, UINT& wY
)
// Find the nearest room edge tile for horn-summoned entity.
// Returns true if an edge is found, and outputs the location to (wX, wY).
// If no room edge can be reached from the horn tile, return false.
{
	MovementType eMovement = GROUND_AND_SHALLOW_WATER_FORCE;
	if (wSummonType == M_CLONE)
		eMovement = GetHornMovementType(this->swordsman.GetMovementType());

	return this->pRoom->GetNearestEntranceTo(wHornX, wHornY, eMovement, wX, wY);
}

//*****************************************************************************
UINT CCurrentGame::GetNextImageOverlayID()
{
	return imageOverlayNextID++;
}

//*****************************************************************************
WSTRING CCurrentGame::getTextForInputCommandKey(InputCommands::DCMD id) const
{
	ASSERT(id < InputCommands::DCMD_Count);

	const CDbPackedVars settings = g_pTheDB->GetCurrentPlayerSettings();
	const InputCommands::KeyDefinition *keyDefinition = InputCommands::GetKeyDefinition(id);

	const InputKey inputKey = settings.GetVar(keyDefinition->settingName, 0);

	return I18N::DescribeInputKey(inputKey);
}

//*****************************************************************************
void CCurrentGame::FreezeCommands()
//Disallow modification of command list, i.e. adding commands, clearing, or truncating.
//Assertions will fire in CDbCommands if this is violated.
{
	//Commands should not be frozen while recording.  Recording requires commands to
	//be added for each call to ProcessCommand().  It is possible to have some carefully
	//thought-out mixture of the two states, in which case this assertion can be changed.
	ASSERT(!this->bIsDemoRecording);

	this->Commands.Freeze();
}

//*****************************************************************************
UINT CCurrentGame::GetChecksum()
//Gets a checksum representing the current game state.  This checksum is meant to
//stay the same even if DROD's implementation changes radically, so only things that
//I'm confident will be consistently measurable in future versions are included.
//
//Returns:
//The checksum.
const
{
	const UINT dwSum =

		//Swordsman position.
		BYTE(this->swordsman.wX) * 0x00000001 +
		BYTE(this->swordsman.wY) * 0x00000010 +
		BYTE(this->swordsman.wO) * 0x00000100 +

		//Number of monsters.
		BYTE(this->pRoom->wMonsterCount) * 0x00010000 +

		//Turn count.
		BYTE(this->wTurnNo) * 0x00100000 +

		//Conquered rooms.
		BYTE(this->ConqueredRooms.size()) * 0x01000000 +

		//Explored rooms.
		BYTE(this->ExploredRooms.size()) * 0x10000000;

	return dwSum;
}

//*****************************************************************************
void CCurrentGame::GetLevelStats(CDbLevel *pLevel)
//Extract stats for this level from packed vars.
//If no saved stats exist for this level, values default to zero.
{
	ASSERT(pLevel);
	ASSERT(pLevel->dwLevelIndex);

	char levelIndex[10];
	char varName[20];
	_itoa(pLevel->dwLevelIndex, levelIndex, 10);

	strcpy(varName, levelIndex);
	strcat(varName, "d");
	this->dwLevelDeaths = this->stats.GetVar(varName, (UINT)0);
	strcpy(varName, levelIndex);
	strcat(varName, "k");
	this->dwLevelKills = this->stats.GetVar(varName, (UINT)0);
	strcpy(varName, levelIndex);
	strcat(varName, "m");
	this->dwLevelMoves = this->stats.GetVar(varName, (UINT)0);
	strcpy(varName, levelIndex);
	strcat(varName, "t");
	this->dwLevelTime = this->stats.GetVar(varName, (UINT)0);
}

//*****************************************************************************
UINT CCurrentGame::GetRoomExitDirection(
//Returns: direction swordsman should exit the current room,
//based on his move and position.
//
//Params:
	const UINT wMoveO)      //(in)   Direction swordsman is exiting.
const
{
	switch (wMoveO)
	{
		case N: return N;

		case S: return S;

		case W: return W;

		case E: return E;

		case NW:
		return this->swordsman.wY == 0 ? N : W;

		case NE:
		return this->swordsman.wY == 0 ? N : E;

		case SW:
		return this->swordsman.wY == this->pRoom->wRoomRows - 1 ? S : W;

		case SE:
		return this->swordsman.wY == this->pRoom->wRoomRows - 1 ? S : E;

		default:
			return NO_ORIENTATION;  //bad orientation
	}
}

//*****************************************************************************
//Returns: pointer to interpolated text of scroll at indicated room tile
WSTRING CCurrentGame::GetScrollTextAt(const UINT wX, const UINT wY)
{
	const WCHAR *pText = pRoom->GetScrollTextAtSquare(wX, wY);
	return ExpandText(pText);
}

//*****************************************************************************
CEntity* CCurrentGame::getSpeakingEntity(CFiredCharacterCommand *pFiredCommand)
//Returns: pointer to the best entity to speak the specified scripted speech command.
//This may be a dead monster, so this should be checked for by the caller.
{
	ASSERT(pFiredCommand);
	ASSERT(pFiredCommand->pCommand);
	CDbSpeech *pSpeech = pFiredCommand->pCommand->pSpeech;
	ASSERT(pSpeech);
	if (!pSpeech)
		return NULL; //robustness

	//Determine monster speaker identity.
	SPEAKER speaker = (SPEAKER)pSpeech->wCharacter;
	//a custom speaker should already have been resolved before calling this
	if (speaker == Speaker_Custom)
		speaker = Speaker_None;

	//Determine whether a custom speaker is set.
	HoldCharacter *pCustomChar = NULL;
	if ((UINT)speaker >= (UINT)CUSTOM_CHARACTER_FIRST && (UINT)speaker != (UINT)M_NONE)
	{
		pCustomChar = this->pHold->GetCharacter(speaker);
		if (!pCustomChar) //custom ID not defined (bad data?) -- default to no speaker
			speaker = Speaker_None;
	}

	CMonster *pMonster = pFiredCommand->pSpeakingEntity; //assume the attached entity is speaking
	CEntity *pEntity = pMonster;
	if (speaker != Speaker_Self && speaker != Speaker_None)
	{
		bool bFoundSpeaker = false;
		if (pMonster->wType == M_CHARACTER)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->IsVisible())
			{
				if ((UINT(speaker) >= UINT(CUSTOM_CHARACTER_FIRST) && UINT(speaker) != UINT(M_NONE) &&
						(UINT(speaker) == pCharacter->wLogicalIdentity || UINT(speaker) == pCharacter->wIdentity)) || //custom characters match (either ID or in appearance)
						(speaker == getSpeakerType(MONSTERTYPE(pCharacter->GetIdentity())))) //stock speaker types match
				bFoundSpeaker = true;   //So we conclude the attached monster is speaking
			}
		}

		if (!bFoundSpeaker)
		{
			//The attached monster is not the one speaking.  Find the speaking monster.
			pEntity = this->pRoom->GetSpeaker(speaker >= Speaker_Count ?
					UINT(speaker) : SpeakerConstant[speaker]); //custom vs. stock character speaker
			//If none of the indicated monster type are present...
			if (!pEntity)
			{
				//...and a custom speaker is indicated,
				//look for an entity that matches its base type
				if (pCustomChar)
					pEntity = this->pRoom->GetSpeaker(pCustomChar->wType, true);
			}
			//Otherwise, default to having the attached monster speak.
			if (!pEntity)
				pEntity = pMonster;
		}
	}

	ASSERT(pEntity);
	return pEntity;
}

//*****************************************************************************
bool CCurrentGame::GetSwordsman(UINT& wSX, UINT& wSY, const bool bIncludeNonTarget) const
//OUT: position of Beethro (or equivalent targeted player) in the room, or (-1000,-1000) if absent
//
//Returns: whether Beethro or player target is present
{
	if (bIncludeNonTarget || this->swordsman.IsTarget())
	{
		//Player is Beethro or an equivalent target.
		wSX = this->swordsman.wX;
		wSY = this->swordsman.wY;
		return true;
	}

	//Check whether an NPC is posing as the swordsman.
	CMonster *pNPCBeethro = this->pRoom->GetNPCBeethro();
	if (pNPCBeethro && pNPCBeethro->IsTarget())
	{
		wSX = pNPCBeethro->wX;
		wSY = pNPCBeethro->wY;
		return true;
	}

	//Swordsman not present.
	wSX = wSY = static_cast<UINT>(-1000);
	return false;
}

//*****************************************************************************
UINT CCurrentGame::getVar(const UINT varIndex) const
//Gets the value of a global var.
{
	switch (varIndex) {
		case (UINT)ScriptVars::P_PLAYER_X:
			return this->swordsman.wX;
		case (UINT)ScriptVars::P_PLAYER_Y:
			return this->swordsman.wY;
		case (UINT)ScriptVars::P_PLAYER_O:
			return this->swordsman.wO;
		case (UINT)ScriptVars::P_ROOMIMAGE_X:
			return this->pRoom->wImageStartX;
		case (UINT)ScriptVars::P_ROOMIMAGE_Y:
			return this->pRoom->wImageStartY;
		case (UINT)ScriptVars::P_OVERHEADIMAGE_X:
			return this->pRoom->wOverheadImageStartX;
		case (UINT)ScriptVars::P_OVERHEADIMAGE_Y:
			return this->pRoom->wOverheadImageStartY;
		case (UINT)ScriptVars::P_THREATCLOCK:
			return this->threatClockDisplay;
		case (UINT)ScriptVars::P_PLAYERLIGHT:
			return this->playerLight;
		case (UINT)ScriptVars::P_PLAYERLIGHTTYPE:
			return this->playerLightType;
		case (UINT)ScriptVars::P_RETURN_X:
			return this->scriptReturnX;
		case (UINT)ScriptVars::P_RETURN_Y:
			return this->scriptReturnY;
		case (UINT)ScriptVars::P_ROOM_X:
		case (UINT)ScriptVars::P_ROOM_Y:
		{
			int dX, dY;
			this->pRoom->GetPositionInLevel(dX, dY);
			return varIndex == (UINT)ScriptVars::P_ROOM_X ? dX : dY;
		}
		default:
			return 0;
	}
}

//*****************************************************************************
WSTRING CCurrentGame::getStringVar(const UINT varIndex) const
{
	switch (varIndex) {
		case (UINT)ScriptVars::P_LEVELNAME:
			return WSTRING(this->pLevel->NameText);
		default:
			return WSTRING();
	}
}

//*****************************************************************************
void CCurrentGame::ProcessCommandSetVar(
//Called when some predefined variables are changed,
//e.g. via CMD_SETVAR command and CCharacter::SetPredefinedVar.
//It is used to alter the state values of the current game directly.
	const UINT itemID, //ScriptVars Predefined vars enum
	UINT newVal)
{
	switch (itemID) {
		case (UINT)ScriptVars::P_PLAYER_X:
		{
			CCueEvents CueEvents;
			TeleportPlayer(newVal, this->swordsman.wY, CueEvents);
		}
		break;
		case (UINT)ScriptVars::P_PLAYER_Y:
		{
			CCueEvents CueEvents;
			TeleportPlayer(this->swordsman.wX, newVal, CueEvents);
		}
		break;
		case (UINT)ScriptVars::P_PLAYER_O:
			if (IsValidOrientation(newVal) && newVal != NO_ORIENTATION)
				this->swordsman.SetOrientation(newVal);
		break;
		case (UINT)ScriptVars::P_ROOMIMAGE_X:
			if (newVal != this->pRoom->wImageStartX) {
				this->pRoom->wImageStartX = newVal;
				this->pRoom->MarkPlotImageTiles();
			}
		break;
		case (UINT)ScriptVars::P_ROOMIMAGE_Y:
			if (newVal != this->pRoom->wImageStartY) {
				this->pRoom->wImageStartY = newVal;
				this->pRoom->MarkPlotImageTiles();
			}
		break;
		case (UINT)ScriptVars::P_OVERHEADIMAGE_X:
			if (newVal != this->pRoom->wOverheadImageStartX) {
				this->pRoom->wOverheadImageStartX = newVal;
				this->pRoom->MarkPlotOverheadTiles();
			}
		break;
		case (UINT)ScriptVars::P_OVERHEADIMAGE_Y:
			if (newVal != this->pRoom->wOverheadImageStartY) {
				this->pRoom->wOverheadImageStartY = newVal;
				this->pRoom->MarkPlotOverheadTiles();
			}
		break;
		case (UINT)ScriptVars::P_THREATCLOCK:
			this->threatClockDisplay = int(newVal);
		break;
		case (UINT)ScriptVars::P_PLAYERLIGHT:
			this->playerLight = int(newVal);
		break;
		case (UINT)ScriptVars::P_PLAYERLIGHTTYPE:
			this->playerLightType = int(newVal);
		break;
		case (UINT)ScriptVars::P_RETURN_X:
			this->scriptReturnX = int(newVal);
		break;
		case (UINT)ScriptVars::P_RETURN_Y:
			this->scriptReturnY = int(newVal);
		break;
		default:
		break;
	}
}

//*****************************************************************************
void CCurrentGame::GetVarValues(VARMAP& vars)
//Outputs a mapping of var name to value info.
{
	vars.clear();

	for (UNPACKEDVAR *pVar = this->stats.GetFirst(); pVar != NULL; pVar = this->stats.GetNext())
	{
		//Skip non-vars (i.e. not of format "v<varID>".
		if (pVar->name[0] != 'v')
			continue;

		//Get var name.
		const UINT wVarID = atoi(pVar->name.c_str() + 1); //skip the "v"
		const string varName = UnicodeToUTF8(this->pHold->GetVarName(wVarID));

		const bool bInteger = pVar->eType == UVT_int;
		VarMapInfo info;
		if (bInteger)
		{
			info.bInteger = true;
			info.val = this->stats.GetVar(pVar->name.c_str(), (int)0);
		} else {
			info.bInteger = false;
			info.wstrVal = this->stats.GetVar(pVar->name.c_str(), wszEmpty);
		}
		vars[varName] = info;
	}
}

//*****************************************************************************
void CCurrentGame::GotoLevelEntrance(
//Leaves the level and goes to the level with the indicated entrance.
//
//Params:
	CCueEvents& CueEvents, const UINT wEntrance, const bool bSkipEntranceDisplay)
{
	ProcessPlayer_HandleLeaveLevel(CueEvents,
		LevelExit(LevelExit::SpecificID, wEntrance), bSkipEntranceDisplay);
}

//*****************************************************************************
bool CCurrentGame::IsCurrentLevelComplete() const
//For the current level, has the player conquered all required rooms for the
//completion doors to open?
//
//Returns:
//True if so, false if not.
{
	ASSERT(this->pLevel);
	CIDSet requiredRooms;
	this->pLevel->GetRequiredRooms(requiredRooms);
	return CDbSavedGame::ConqueredRooms.contains(requiredRooms);
}

//*****************************************************************************
bool CCurrentGame::IsCurrentRoomConquered() const
{
	return CDbSavedGame::ConqueredRooms.has(this->pRoom->dwRoomID);
}

//*****************************************************************************
bool CCurrentGame::IsCurrentRoomExplored() const
{
	return CDbSavedGame::ExploredRooms.has(this->pRoom->dwRoomID);
}

//*****************************************************************************
bool CCurrentGame::IsCurrentRoomPendingExit() const
//Returns: whether this room has passed the requirements for basic conquering
{
	 return this->pRoom->wMonsterCount == 0 &&
		 (!this->pRoom->bHasConquerToken || this->conquerTokenTurn != NO_CONQUER_TOKEN_TURN);
}

//*****************************************************************************
bool CCurrentGame::IsCurrentRoomPendingConquer() const
//Returns: whether this room will be marked as conquered when the player exits
{
	// See WasRoomConqueredOnThisVisit() for more details
	if (this->bWasRoomConqueredAtTurnStart && this->bIsLeavingLevel)
		return !IsCurrentRoomConquered();

	 return IsCurrentRoomPendingExit() && !this->pRoom->IsBeaconActive();
}

//*****************************************************************************
bool CCurrentGame::AreBeaconsIgnored() const
//Returns: whether there are any circumstances causing the beacons to be ignored when determing if room is being conquered right now
{

	return this->bWasRoomConqueredAtTurnStart && this->bIsLeavingLevel;
}

//*****************************************************************************
void CCurrentGame::SetWorldMapIcon(
	UINT worldMapID,
	UINT xPos, UINT yPos,
	UINT entranceID,
	UINT charID,
	UINT imageID,
	UINT displayFlags)
{
	ASSERT(!charID || !imageID);

	if (!this->pHold->DoesWorldMapExist(worldMapID))
		return;

	WorldMapsIcons::iterator iter = this->worldMapIcons.find(worldMapID);
	if (displayFlags == ScriptFlag::WMI_OFF) {
		if (iter != this->worldMapIcons.end()) {
			//Remove all icons for this entranceID on this world map
			WorldMapIcons& icons = iter->second;
			WorldMapIcons newIcons;
			for (WorldMapIcons::const_iterator iconIt=icons.begin();
				iconIt!=icons.end(); ++iconIt)
			{
				if (iconIt->entranceID != entranceID)
					newIcons.push_back(*iconIt);
			}

			icons = newIcons;
		}
	} else {
		//Only for valid entrance IDs
		if (this->pHold->GetEntrance(entranceID) ||
				(LevelExit::IsWorldMapID(entranceID) &&
					this->pHold->DoesWorldMapExist(LevelExit::ConvertWorldMapID(entranceID))))
		{
			if (iter != this->worldMapIcons.end()) {
				//Replace any icons at the same coords on this world map.
				WorldMapIcons& icons = iter->second;
				WorldMapIcons newIcons;
				bool bReplaced = false;
				for (WorldMapIcons::const_iterator iconIt=icons.begin();
					iconIt!=icons.end(); ++iconIt)
				{
					const WorldMapIcon& icon = *iconIt;
					if (icon.xPos == xPos && icon.yPos == yPos) {
						bReplaced = true; //remove icon from the copied data
					} else {
						newIcons.push_back(icon);
					}
				}

				if (bReplaced)
					icons = newIcons;
			}

			//Can have multiple icons with the same destination on a world map
			this->worldMapIcons[worldMapID].push_back(
				WorldMapIcon(entranceID, xPos, yPos, charID, imageID, displayFlags));
		}
	}
}

//*****************************************************************************
//The world map music specifications are stored, only for lookup in the front end.
string GetWorldMapVarNamePrefix(UINT worldMapID)
{
	string prefix = "wm";

	char worldMap[12];
	_itoa(worldMapID, worldMap, 10);

	prefix += worldMap;

	return prefix;
}

string GetWorldMapMusicVarNamePrefix(UINT worldMapID) {
	return GetWorldMapVarNamePrefix(worldMapID) + "m";
}

string GetWorldMapMusicCustomIDNamePiece() {
	return string("c");
}

void CCurrentGame::SetWorldMapMusic(UINT worldMapID, const WSTRING& songlist)
{
	if (this->pHold->DoesWorldMapExist(worldMapID)) {
		const string varName = GetWorldMapMusicVarNamePrefix(worldMapID);
		this->stats.SetVar(varName.c_str(), songlist.c_str());
	}
}

void CCurrentGame::SetWorldMapMusic(UINT worldMapID, const UINT songID, const UINT customID)
{
	if (this->pHold->DoesWorldMapExist(worldMapID)) {
		string varName = GetWorldMapMusicVarNamePrefix(worldMapID);
		this->stats.SetVar(varName.c_str(), songID);

		varName += GetWorldMapMusicCustomIDNamePiece();
		this->stats.SetVar(varName.c_str(), customID);
	}
}

WorldMapMusic CCurrentGame::GetWorldMapMusic(UINT worldMapID)
{
	string varName = GetWorldMapMusicVarNamePrefix(worldMapID);
	if (this->stats.GetVarType(varName.c_str()) == UVT_wchar_string) {
		const WCHAR *songlist = this->stats.GetVar(varName.c_str(), (WCHAR*)(wszEmpty));
		return WorldMapMusic(WSTRING(songlist));
	}

	const UINT songID = this->stats.GetVar(varName.c_str(), UINT(0));

	varName += GetWorldMapMusicCustomIDNamePiece();
	const UINT customID = this->stats.GetVar(varName.c_str(), UINT(0));

	return WorldMapMusic(songID, customID);
}

//*****************************************************************************
bool CCurrentGame::ShouldSaveRoomBegin(
//Checks to see if the specified room has a Room Begin save that is worse than
//the current game state.
//
//Params:
	const UINT dwRoomID) const
{
	const UINT dwRoomBeginSavedGameID =
			g_pTheDB->SavedGames.FindByRoomBegin(dwRoomID);
	if (!dwRoomBeginSavedGameID)   //Save game for newly explored room.
		return true;
	else if (!CDbSavedGame::ConqueredRooms.has(dwRoomID))
	{
		//Allow resaving room being entered if it's still not conquered
		//AND
		//has >= the # of conquered rooms as the existing room start saved game.
		CDbSavedGame *pSavedGame = g_pTheDB->SavedGames.GetByID(dwRoomBeginSavedGameID);
		ASSERT(pSavedGame);
		const UINT dwConqueredCount = pSavedGame->ConqueredRooms.size();
		delete pSavedGame;
		if (this->ConqueredRooms.size() >= dwConqueredCount)
			return true;
	}
	return false;

}

//*****************************************************************************
bool CCurrentGame::ShowLevelStart() const
//Show the current game show a level entrance.
{
	if (this->wTurnNo != 0)
		return false; //not at an entrance if room play is already in progress

	//At a level entrance position that should be shown?
	CEntranceData *pEntrance = this->pHold->GetEntranceAt(this->pRoom->dwRoomID,
			this->swordsman.wX, this->swordsman.wY);
	if (!pEntrance)
		return false;

	switch (pEntrance->eShowDescription)
	{
		default:
		case CEntranceData::DD_No: return false;
		case CEntranceData::DD_Always: return true;
		case CEntranceData::DD_Once: return !this->entrancesExplored.has(pEntrance->dwEntranceID);
	}
}

//*****************************************************************************
bool CCurrentGame::IsPlayerAt(const UINT wX, const UINT wY) const
//Returns: whether player is at (x,y)
{
	return this->swordsman.IsAt(wX, wY);
}

//*****************************************************************************
bool CCurrentGame::IsPlayerEntranceValid() const
//Entrances might have changed since a previous version of the hold.
//
//Returns: whether the player's entrance into the room is a current valid location.
{
	//Is the player entrance on a room edge?
	if (this->wStartRoomX == 0 || this->wStartRoomX == this->pRoom->wRoomCols-1)
		return true;
	if (this->wStartRoomY == 0 || this->wStartRoomY == this->pRoom->wRoomRows-1)
		return true;

	//Is the player entrance at a level entrance position?
	const CEntranceData *pEntrance = this->pHold->GetEntranceAt(this->pRoom->dwRoomID,
			this->wStartRoomX, this->wStartRoomY);
	return (pEntrance != NULL) && (pEntrance->wO == this->wStartRoomO);
}

//*****************************************************************************
bool CCurrentGame::IsPlayerDying() const
//Returns: whether the current state indicates the player is dying
{
	return this->swordsman.bIsDying;
}

//*****************************************************************************
bool CCurrentGame::IsPlayerWeaponAt(const UINT wX, const UINT wY, const bool bIgnoreDagger) const
//Returns: whether player's sword is at (x,y)
{
	if (bIgnoreDagger && this->swordsman.GetActiveWeapon() == WT_Dagger)
		return false;

	return this->swordsman.IsWeaponAt(wX, wY);
}

//*****************************************************************************
bool CCurrentGame::IsRoomAtCoordsConquered(
//Determines if a room in the current level has been conquered in the current game.
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY) //(in) Coords of room to check.
//
//Returns:
//True if it has, false if it hasn't.
const
{
	const UINT dwRoomID = this->pLevel->GetRoomIDAtCoords(dwRoomX, dwRoomY);
	return CDbSavedGame::ConqueredRooms.has(dwRoomID);
}

//*****************************************************************************
bool CCurrentGame::IsRoomAtCoordsExplored(
//Determines if a room in the current level has been explored in the current game.
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY) //(in) Coords of room to check.
const
//
//Returns:
//True if it has, false if it hasn't.
{
	const UINT dwRoomID = this->pLevel->GetRoomIDAtCoords(dwRoomX, dwRoomY);
	return CDbSavedGame::ExploredRooms.has(dwRoomID);
}

//*****************************************************************************
bool CCurrentGame::LoadFromHold(
//Loads current game from the starting level and room of a specified hold.
//
//Params:
	const UINT dwHoldID,   //(in) Identifies hold that game will begin in.
	CCueEvents &CueEvents)  //(out)  Cue events generated by swordsman's first
							//    step into the room.
//
//Returns:
//True if successful, false if not.
{
	LOGCONTEXT("CCurrentGame::LoadFromHold");
	bool bSuccess=true;

	LoadPrep();

	this->worldMapID = 0;

	//Load the hold.
	this->pHold = new CDbHold();
	if (!this->pHold) {bSuccess=false; goto Cleanup;}
	bSuccess = this->pHold->Load(dwHoldID);
	if (!bSuccess) goto Cleanup;

	//Load the first level of hold.
	this->pLevel = this->pHold->GetStartingLevel();
	if (!this->pLevel) {bSuccess=false; goto Cleanup;}
	this->pEntrance = this->pHold->GetMainEntranceForLevel(this->pLevel->dwLevelID);
	ASSERT(this->pEntrance);
	this->wStartRoomX = pEntrance->wX;
	this->wStartRoomY = pEntrance->wY;
	this->wStartRoomO = pEntrance->wO;

	//defaults
	this->wStartRoomAppearance = M_BEETHRO;
	this->bStartRoomSwordOff = false;
	this->wStartRoomWaterTraversal = WTrv_AsPlayerRole;
	this->wStartRoomWeaponType = WT_Sword;

	this->pLevel->dwStartingRoomID = pEntrance->dwRoomID;

	//Load the first room of level.
	this->pRoom = this->pLevel->GetStartingRoom();
	if (!this->pRoom) {bSuccess=false; goto Cleanup;}

	//Set swordsman to beginning of room.
	SetPlayerToRoomStart();

	//Save to level-begin and room-begin slots.
	//ATTN: Do this before SetMembersAfterRoomLoad changes anything.
	if ((this->dwAutoSaveOptions & ASO_LEVELBEGIN)==ASO_LEVELBEGIN)
		SaveToLevelBegin();
	if ((this->dwAutoSaveOptions & ASO_ROOMBEGIN)==ASO_ROOMBEGIN)
	{
		SaveToRoomBegin();

		//Upload explored room data
		if (!IsCurrentRoomExplored())
			UploadExploredRoom();
	}


	//Get members ready.
	SetMembersAfterRoomLoad(CueEvents);

Cleanup:
	if (!bSuccess) Clear();
	return bSuccess;
}

//*****************************************************************************
bool CCurrentGame::LoadFromLevelEntrance(
//Loads current game from an entrance of a specified level.
//Pre-Cond: Hold specified is the same as the one already in memory.
//
//Params:
	const UINT dwEntranceID,
	CCueEvents &CueEvents)
{
	LOGCONTEXT("CCurrentGame::LoadFromLevelEntrance");

	bool bSuccess=true;
	CIDSet roomsInLevel;
	bool bNewLevel;

	SaveExitedLevelStats();

	{
		//Retain these var states.
		const UINT wIdentity_ = this->swordsman.wIdentity;
		const bool bWeaponOff_ = this->swordsman.bWeaponOff;
		const UINT wWaterTraversal_ = this->swordsman.wWaterTraversal;
		const WeaponType weaponType_ = this->swordsman.weaponType;

		LoadPrep(false);

		SetPlayerRole(wIdentity_, CueEvents);
		this->swordsman.bWeaponOff = bWeaponOff_;
		this->swordsman.wWaterTraversal = wWaterTraversal_;
		this->swordsman.weaponType = weaponType_;
	}

	this->pEntrance = this->pHold->GetEntrance(dwEntranceID);
	ASSERTP(pEntrance, "Dangling level entrance ID");   //Corrupted DB if NULL
	if (!this->pEntrance) {bSuccess=false; goto Cleanup;}
	this->worldMapID = 0;

	//Load the room of the level entrance.
	this->pRoom = new CDbRoom();
	if (!this->pRoom) {bSuccess=false; goto Cleanup;}
	bSuccess = this->pRoom->Load(pEntrance->dwRoomID);
	if (!this->pRoom) {bSuccess=false; goto Cleanup;}

	//Load the level.
	this->pLevel = new CDbLevel();
	if (!this->pLevel) {bSuccess=false; goto Cleanup;}
	bSuccess = this->pLevel->Load(this->pRoom->dwLevelID);
	if (!bSuccess) goto Cleanup;

	//Set room start vars.
	this->swordsman.wX = this->swordsman.wPrevX = pEntrance->wX;
	this->swordsman.wY = this->swordsman.wPrevY = pEntrance->wY;
	this->swordsman.SetOrientation(pEntrance->wO);
	SetRoomStartToPlayer();

	//Replace a previous LevelBegin saved game record if the level is being
	//entered for the first time in the current game, meaning if none of
	//the rooms in the level has ever been visited in the current game.
	roomsInLevel = CDb::getRoomsInLevel(this->pLevel->dwLevelID);
	bNewLevel = !this->ExploredRooms.containsAny(roomsInLevel);

	//Set swordsman to beginning of room.
	SetPlayerToRoomStart();

	//Prepare game stats for this level.
	GetLevelStats(this->pLevel);

	//Save to level-begin and room-begin slots.
	//ATTN: Do this before SetMembersAfterRoomLoad changes anything.
	//Allow resaving each time level is entered fresh.
	if ((this->dwAutoSaveOptions & ASO_LEVELBEGIN)==ASO_LEVELBEGIN && bNewLevel)
		SaveToLevelBegin();
	if ((this->dwAutoSaveOptions & ASO_ROOMBEGIN)==ASO_ROOMBEGIN &&
		ShouldSaveRoomBegin(pRoom->dwRoomID))
	{
		SaveToRoomBegin();

		//Upload explored room data
		if (!IsCurrentRoomExplored())
			UploadExploredRoom();
	}

	//Get members ready.
	SetMembersAfterRoomLoad(CueEvents);

Cleanup:
	if (!bSuccess) Clear();
	return bSuccess;
}

bool CCurrentGame::LoadFromWorldMap(const UINT worldMapID)
{
	LOGCONTEXT("CCurrentGame::LoadFromWorldMap");

	SaveExitedLevelStats();
	SetRoomStartToPlayer();
	SetPlayerToRoomStart(); //reset some vars so saving game works properly

	if (!this->pHold->DoesWorldMapExist(worldMapID))
		return false;

	this->worldMapID = worldMapID;

	this->bIsGameActive = true;
	this->bIsNewRoom = false;

	if ((this->dwAutoSaveOptions & ASO_LEVELBEGIN)==ASO_LEVELBEGIN)
		SaveToWorldMap();

	return true;
}

//*****************************************************************************
bool CCurrentGame::LoadFromRoom(
//Loads current game from specified room (for playtesting).
//Hold and level are implied by the specified room.
//
//Params:
	const UINT dwRoomID,   //(in) Identifies room that game will begin in.
	CCueEvents &CueEvents,  //(out)  Cue events generated by swordsman's first
							//    step into the room.
	const UINT wX, const UINT wY, const UINT wO, //(in) Player starting position
	const bool bNoSaves)    //whether DB saves should be prevented [default=false]
//
//Returns:
//True if successful, false if not.
{
	LOGCONTEXT("CCurrentGame::LoadFromRoom");
	ASSERT(dwRoomID);

	bool bSuccess=true;

	this->bNoSaves = bNoSaves;
	LoadPrep();

	//Only checkpoint saves enabled during testing.
	this->dwAutoSaveOptions = ASO_CHECKPOINT;

	//Load the room.
	this->pRoom = new CDbRoom;
	if (!this->pRoom) {bSuccess=false; goto Cleanup;}
	bSuccess = this->pRoom->Load(dwRoomID);
	if (!bSuccess) goto Cleanup;

	//Load level associated with room.
	this->pLevel = new CDbLevel();
	if (!this->pLevel) {bSuccess=false; goto Cleanup;}
	bSuccess = this->pLevel->Load(this->pRoom->dwLevelID);
	if (!bSuccess) goto Cleanup;

	//Load hold associated with level.
	this->pHold = new CDbHold();
	if (!this->pHold) {bSuccess=false; goto Cleanup;}
	bSuccess = this->pHold->Load(this->pLevel->dwHoldID);
	if (!bSuccess) goto Cleanup;

	//Set entrance to the main level entrance.
	this->pEntrance = this->pHold->GetMainEntranceForLevel(this->pLevel->dwLevelID);
	this->worldMapID = 0;

	//Set room start vars.
	this->wStartRoomO = wO;
	this->wStartRoomX = wX;
	this->wStartRoomY = wY;

	//Defaults.
	this->wStartRoomAppearance = M_BEETHRO;
	this->bStartRoomSwordOff = false;
	this->wStartRoomWaterTraversal = WTrv_AsPlayerRole;
	this->wStartRoomWeaponType = WT_Sword;

	this->swordsman.wX = this->swordsman.wPrevX = CDbSavedGame::wStartRoomX;
	this->swordsman.wY = this->swordsman.wPrevY = CDbSavedGame::wStartRoomY;
	this->swordsman.SetOrientation(CDbSavedGame::wStartRoomO);
	SetRoomStartToPlayer();

	//Set player to beginning of room.
	SetPlayerToRoomStart();

	//Get members ready.
	SetMembersAfterRoomLoad(CueEvents);

Cleanup:
	if (!bSuccess) Clear();
	return bSuccess;
}

//*****************************************************************************
bool CCurrentGame::LoadFromSavedGame(
//Loads current game from a saved game.
//
//Params:
	const UINT dwSavedGameID, //(in) Identifies saved game to load from.
	CCueEvents &CueEvents,     //(out)  Cue events generated by swordsman's last
								//    step in the room (which may be the first step).
	bool bRestoreAtRoomStart,  //(in)   If true, current game will be loaded to
								//    beginning of room in saved game.  If false (default),
								//    current game will be loaded to the exact room
								//    state specified in the saved game.
	const bool bNoSaves)    //whether DB saves should be prevented [default=false]
//
//Returns:
//True if successful, false if not.
{
	LOGCONTEXT("CCurrentGame::LoadFromSavedGame");

	try {

	this->bNoSaves = bNoSaves;
	LoadPrep();
	CueEvents.Clear(); //ensure object starts empty

	//Load the saved game.
	if (!CDbSavedGame::Load(dwSavedGameID))
		throw CException("CCurrentGame::LoadFromSavedGame");

	//Load the room.
	this->pRoom = CDbSavedGame::GetRoom();
	if (!this->pRoom) throw CException("CCurrentGame::LoadFromSavedGame");

	//Load the level.
	this->pLevel = this->pRoom->GetLevel();
	if (!this->pLevel) throw CException("CCurrentGame::LoadFromSavedGame");

	//Load the hold.
	this->pHold = this->pLevel->GetHold();
	if (!this->pHold) throw CException("CCurrentGame::LoadFromSavedGame");

	//Set room start vars.
	this->swordsman.wX = this->swordsman.wPrevX = CDbSavedGame::wStartRoomX;
	this->swordsman.wY = this->swordsman.wPrevY = CDbSavedGame::wStartRoomY;
	this->swordsman.SetOrientation(CDbSavedGame::wStartRoomO);
	SetPlayerRole(CDbSavedGame::wStartRoomAppearance, CueEvents);
	this->swordsman.bWeaponOff = CDbSavedGame::bStartRoomSwordOff;
	this->swordsman.wWaterTraversal = CDbSavedGame::wStartRoomWaterTraversal;
	this->swordsman.SetWeaponType(CDbSavedGame::wStartRoomWeaponType);
	SetRoomStartToPlayer();

	if (this->worldMapID)
		return true;

	const bool bAtRoomStart = bRestoreAtRoomStart || this->Commands.Empty();

	//Determine level entrance where player is.
	//This is currently used only to show level entrance texts and entrance warp cheat.
	this->pEntrance = NULL;
	if (bAtRoomStart)
	{
		//At a level entrance position that should be shown?
		CEntranceData *pEntrance = this->pHold->GetEntranceAt(this->pRoom->dwRoomID,
				this->swordsman.wX, this->swordsman.wY);
		if (pEntrance)
			this->pEntrance = pEntrance;
	}
	if (!this->pEntrance)
		this->pEntrance = this->pHold->GetMainEntranceForLevel(this->pLevel->dwLevelID);

	//Put room in correct beginning state and get cue events for the
	//last step the player has taken.
	if (bAtRoomStart)
	{
		//Cue events come from first step into the room.
		SetMembersAfterRoomLoad(CueEvents, false);
	} else {
		//Cue events come from processing of last command below.
		//Ignore cue events from first step into the room.
		CCueEvents IgnoredCueEvents;
		SetMembersAfterRoomLoad(IgnoredCueEvents, false);

		//Play through any commands from the saved game.
		//Truncate any commands that cannot be played back.
		if (!PlayAllCommands(CueEvents, true))
		{
			//There was an error encountered during playback.
#ifdef _DEBUG
			char szMsg[80];
			sprintf(szMsg, "Commands in saved game ID#%u could not be played back." NEWLINE,
					dwSavedGameID);
			CFiles Files;
			Files.AppendErrorLog(szMsg);
#endif
		}

		if (!this->bIsGameActive)
		{
			//Playing back the saved game didn't work as expected.
			//Undo to the last command when the game was active.
			UndoCommands(1, CueEvents);
		}

		UpdatePrevCoords();  //start everything moving from current positions
	}

	} //try
	catch (CException&)
	{
		Clear();
		return false;
	}
	return true;
}

//*****************************************************************************
bool CCurrentGame::LoadNewRoomForExit(
//Loads room adjacent to the current room in the direction of the swordsman's
//exit of the current room.  Swordsman coords are also updated so that he wraps to
//other side in new room.
//
//Params:
	const UINT wExitO,      //(in)   Direction swordsman is exiting.
	CCueEvents &CueEvents)  //(out)  Cue events generated by swordsman's first step
							//    into the room.
{
	switch (wExitO)
	{
		case N:
		if (!LoadNorthRoom())
			return false;
		break;
		case S:
		if (!LoadSouthRoom())
			return false;
		break;

		case W:
		if (!LoadWestRoom())
			return false;
		break;

		case E:
		if (!LoadEastRoom())
			return false;
		break;

		default:
			ASSERT(!"Bad orientation value.");
		return false;
	}

	//Set start room members.
	SetRoomStartToPlayer();
	SetPlayerToRoomStart(); //reset some vars so saving game works properly

	//ATTN: Save before any state vars are modified in SetMembersAfterRoomLoad.
	if ((GetAutoSaveOptions() & ASO_ROOMBEGIN)==ASO_ROOMBEGIN)
		if (!g_pTheDB->SavedGames.FindByRoomBegin(this->pRoom->dwRoomID))
			SaveToRoomBegin();

	SetMembersAfterRoomLoad(CueEvents);

	return true;
}

//*****************************************************************************
void CCurrentGame::LoadPrep(
//What needs to be done before loading (i.e. changing) a current game.
//
//Params:
	const bool bNewGame)  //(in)   whether new game is starting [default=true]
{
	if (bNewGame && !this->bNoSaves)
	{
		//On new game, end and save a demo if in the middle of recording.
		//Otherwise, keep recording a multi-room(level) demo.
		if (IsDemoRecording() && !EndDemoRecording())
		{
			CFiles Files;
			Files.AppendErrorLog("Failed to save a demo when recording ended." NEWLINE);
		}
	}

	//Unload an already loaded current game if there is one.
	{
		const UINT dwAutoSaveOptionsB4 = this->dwAutoSaveOptions;
		const DEMO_REC_INFO demoInfo = this->DemoRecInfo;
		const bool bDemoRecording = this->bIsDemoRecording;
		Clear(bNewGame);
		this->dwAutoSaveOptions = dwAutoSaveOptionsB4;
		if (!bNewGame)
		{
			this->DemoRecInfo = demoInfo;
			this->bIsDemoRecording = bDemoRecording;
		}
	}
}

//***************************************************************************************
bool CCurrentGame::PlayAllCommands(
//Play back stored commands to change the game state.
//Assumes that the room has been freshly loaded when replaying from turn zero.
//
//Params:
	CCueEvents &CueEvents,     //(out)  Cue events generated by last processed command.
	const bool bTruncateInvalidCommands)   //(in) delete any commands that cannot be played back [default=false]
//
//Returns:
//True if commands were successfully played without putting the game into an
//unexpected state, false if not.
{
	if (this->Commands.Empty())
		return true; //no commands to process

	if (this->eType == ST_WorldMap)
		return true; //robustness -- ignore any spurious commands in a world map saved game

	//While processing the command list, I don't want to take any actions that
	//will modify the command list.
	FreezeCommands();

	CDbCommands::const_iterator comIter;
	UINT wCommandI=this->wTurnNo, wX=(UINT)-1, wY=(UINT)-1;
	CDbCommands commands(this->Commands); //must have local copy for temporal split undo handling
	for (comIter = commands.Get(wCommandI);
	     comIter != commands.end();
	     comIter = commands.GetNext(), ++wCommandI)
	{
		DeleteLeakyCueEvents(CueEvents);
		CueEvents.Clear();

		int nCommand = comIter->bytCommand;
		//Note that Answer commands can legitimately turn up at the start of a Double sequence
		if (this->swordsman.wPlacingDoubleType && nCommand != CMD_DOUBLE && !bIsAnswerCommand(nCommand))
		{
			//Upgrade 2.0 double placement move sequence with a single CMD_DOUBLE.
			UnfreezeCommands();
			ReplaceDoubleCommands();
			FreezeCommands();
			commands = this->Commands; //refresh copy -- safe for pre-5.0 plays
			comIter = commands.GetCurrent();
			if (comIter == commands.end())
				break; //double placement never completed -- nothing else to advance
			nCommand = comIter->bytCommand;
		}

		if (bIsComplexCommand(nCommand)) //handle multi-part commands here
			VERIFY(commands.GetData(wX,wY));
		ProcessCommand(nCommand, CueEvents, wX, wY);

		//Check for game states that indicate further commands would be invalid.
		//Note: a possible reason for getting these errors is that the current version
		//of the app is not compatible with a game previously saved.
		if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerLeftRoom), CIDA_PlayerLeftRoom))
		{
			//command ending room play executed successfully, but replay stops now
			comIter = commands.GetNext();
			++wCommandI;
			break;
		}
		if (!this->bIsGameActive)
			break;
		if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied))
			break;
	}
	this->swordsman.wPrevX = this->swordsman.wX;
	this->swordsman.wPrevY = this->swordsman.wY;

	//Allow modification of command list again.
	UnfreezeCommands();

	//If not all commands were processed, optionally truncate commands that
	//cannot be played back.
	const bool bProcessedAll = (comIter == commands.end());
	if (!bProcessedAll && bTruncateInvalidCommands)
		this->Commands.Truncate(wCommandI);

	//Successful return if I processed all the commands.
	return bProcessedAll;
}

//***************************************************************************************
bool CCurrentGame::PlayCommandsToTurn(
//Play back stored commands to change the game state.
//Assumes that the room has been freshly loaded when replaying from turn zero.
//
//Params:
	const UINT wEndTurnNo, //(in) Play commands until this turn number.
	CCueEvents &CueEvents) //(out)  Cue events generated by last processed command.
//
//Returns:
//True if commands were successfully played to specified turn without putting
//the game into an unexpected state, false if not.
{
	ASSERT(wEndTurnNo <= this->Commands.Count());

	if (!wEndTurnNo)
		return true; //no commands to be played

	//While processing the command list, I don't want to take any actions that
	//will modify the command list.
	const bool freeze = ShouldFreezeCommandsDuringSetTurn();
	if (freeze)
		FreezeCommands();

	//Replay turns starting from the turn # of the current game state.
	CDbCommands::const_iterator comIter = this->Commands.Get(this->wTurnNo);
	UINT wX, wY;
	while (this->wTurnNo < wEndTurnNo)
	{
		ASSERT(comIter != this->Commands.end());
		DeleteLeakyCueEvents(CueEvents);
		CueEvents.Clear();

		const int nCommand = comIter->bytCommand;
		if (this->swordsman.wPlacingDoubleType && !bIsAnswerCommand(nCommand))
		{
			//If we tried fixing up double commands with
			//ReplaceDoubleCommands() here, it would no longer be
			//clear what wEndTurnNo means.  But this method is only
			//used for rewinding (undo/restore) to a specific move,
			//from SetTurn().  Instead, any situation which might
			//rewind commands needs to fix old style double
			//commands during forward command playback.
			//
			//Hence, we assert that a 3.0 double placement command is already in place.
			ASSERT(nCommand == CMD_DOUBLE);
		}
		wX = wY = UINT(-1);
		if (bIsComplexCommand(nCommand)) //handle multi-part commands here
			VERIFY(this->Commands.GetData(wX,wY));
		ProcessCommand(nCommand, CueEvents, wX, wY);
		//Check for game states that indicate further commands would be invalid.
		//Note: a possible reason for getting these errors is that the current version
		//of the app is not compatible with a game previously saved.
		if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerLeftRoom), CIDA_PlayerLeftRoom))
		{
			//command ending room play executed successfully, and demo stops now
			break;
		}
		if (!this->bIsGameActive)
			break;
		if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied))
			break;

		if (nCommand == CMD_CLONE) {
			//rewinding to a temporal split point will desync the commands iterator
			//so here we need to re-sync it
			comIter = this->Commands.Get(this->wTurnNo);
		} else {
			comIter = this->Commands.GetNext();
		}
	}
	this->swordsman.wPrevX = this->swordsman.wX;
	this->swordsman.wPrevY = this->swordsman.wY;

	//Allow modification of command list again.
	if (freeze)
		UnfreezeCommands();

	//Successful return if I processed all the commands.
	return this->wTurnNo == wEndTurnNo;
}

//*****************************************************************************
void CCurrentGame::PostProcessCharacter(CCharacter* pCharacter, CCueEvents& CueEvents)
//Call after processing a character's turn.
{
	if (!pCharacter->stunned)
		pCharacter->bWasPushed = false;

	pCharacter->bPreventMoveAfterPush = false;

	//Mark when script is completed.
	if (pCharacter->bScriptDone)
		ScriptCompleted(pCharacter);
	if (pCharacter->bReplaced)
	{
		//Character "reverted" to the monster matching its appearance.
		CMonster *pMonster = this->pRoom->GetMonsterAtSquare(
				pCharacter->wX, pCharacter->wY);
		//The monster may have died during the replacement
		//ASSERT(pMonster); 

		//The new monster might require changes to room state (e.g. pathmapping).
		this->pRoom->CreatePathMaps();

		if (pMonster)
		{
			//Beethro Doubles should light fuse ends after the character's "turn"
			if (bIsBeethroDouble(pMonster->wType))
				this->pRoom->LightFuseEnd(CueEvents, pCharacter->wX, pCharacter->wY);
			else if (CueEvents.HasOccurredWith(CID_FegundoToAsh, pMonster))
				FegundoToAsh(pMonster, CueEvents);
		}
	}
}

//*****************************************************************************
void CCurrentGame::ProcessCommand(
//Processes a game command, causing game data and current room to be updated
//in response.  Display or sound should be handled by caller if appropriate.
//
//Params:
	int nCommand,        //(in)   Game command.
	CCueEvents &CueEvents,  //(out)  List of events that can be handled by caller.
							//    These are things that the UI wouldn't necessarily
							//    be aware of by looking at the modified game
							//    data on return.
	UINT wX, UINT wY)	//(in) [default=(-1,-1)]
{
	//Caller should not be trying to process new commands after the game is
	//inactive.  Before doing so, caller will need to reload the room in some way.
	ASSERT(this->bIsGameActive);

	const UINT dwStart = GetTicks();

	//Reset relative movement for the current turn.
	UpdatePrevCoords();
	this->pRoom->ClearDeadMonsters(true);
	this->pRoom->ClearPushStates();

	if (!this->dwCutScene)
		ResetCutSceneStartTurn();

	this->bWasRoomConqueredAtTurnStart = !this->pRoom->IsBeaconActive()
		&& (IsCurrentRoomPendingExit() || IsCurrentRoomConquered());

	this->swordsman.bHasTeleported = false;

	const bool bPlayerIsAnsweringQuestion = this->UnansweredQuestions.size() != 0;
	const UINT monsterKillComboAtStartOfTurn = this->wMonsterKillCombo;
	const bool bPlacingDoubleAtStartOfTurn = this->swordsman.wPlacingDoubleType != 0;

	//Note: Private data pointers found in CueEvents are guaranteed to be valid
	//until the CCurrentGame instance that originally returned the CueEvents goes out
	//of scope or the ProcessCommand() method is called again on the same instance.
	CueEvents.Clear();
	this->pRoom->ClearPlotHistory();

	this->pRoom->InitStateForThisTurn();

	//Add this command to list of commands for the room.
	if (!this->Commands.IsFrozen())
	{
		//Accept answering questions first, since they tend to take priority
		if (bPlayerIsAnsweringQuestion && bIsAnswerCommand(nCommand))
		{
			//If player is not being shown and it's not a cutscene, shouldn't be adding any commands.
			if (!this->dwCutScene && !this->swordsman.IsInRoom())
				return;

			this->Commands.Add(nCommand);
			if (nCommand == CMD_ANSWER) //store selected answer as a second command item
				this->Commands.AddData(wX, wY);
		}
		else if (IsCutScenePlaying())
		{
			//Only accept explicit time-advancement commands from the front end during cut scenes
			//Questions were already dealt with in the previous section
			if (nCommand != CMD_ADVANCE_CUTSCENE)
				return;

			nCommand = CMD_WAIT;
			this->Commands.Add(nCommand, (BYTE)(this->dwCutScene/10));	//10ms increments
		}
		else if (!this->swordsman.wPlacingDoubleType)
		{
			//If player is not being shown, shouldn't be adding any commands.
			if (!this->swordsman.IsInRoom())
				return;

			if (nCommand == CMD_CLONE)
			{
				//Determine what clone command does.
				if (this->temporalSplit.queuing()) {
					//can't switch to an explicit selected clone while preparing a temporal split
					if (wX != UINT(-1))
						return;
				} else {
					//If clone is placed, transfer to next one in sequence.
					if (wX == UINT(-1))
					{
						const CMonster *pMonster = this->pRoom->FindNextClone();
						if (!pMonster)
							return; //no clone to transfer to -- ignore this command

						wX = pMonster->wX;
						wY = pMonster->wY;
					}
				}
			}

			this->Commands.Add(nCommand);
			if (nCommand == CMD_CLONE ||  //store extra coord data as a second command item
					nCommand == CMD_ANSWER) //store selected answer as a second command item
				this->Commands.AddData(wX, wY);
		}	//else: handle command addition below in ProcessDoublePlacement
	}

	//Set a state variable and do nothing else.
	if (nCommand == CMD_SETVAR)
	{
		ProcessCommandSetVar(wX, wY);
		return;
	}

	//Increment the turn#.
	if (bPlayerIsAnsweringQuestion ||
			!this->swordsman.wPlacingDoubleType)
	{
		++this->wTurnNo;
		if (!this->Commands.IsFrozen())
		{
			++this->dwLevelMoves;
			ASSERT(this->dwLevelMoves > 0);
		}
	}

	//Switch to another clone.
	if (nCommand == CMD_CLONE)
	{
		if (CanSwitchToClone())
		{
			ASSERT(wX != UINT(-1)); //destination clone should have been resolved by now
			if (SwitchToCloneAt(wX,wY))
				CueEvents.Add(CID_CloneSwitch);
			//Don't apply process turns at the start of the room
			if (this->wPlayerTurn != 0)
				this->pRoom->ProcessTurn(CueEvents, false);

			//After switching to a clone, don't allow saving to a checkpoint
			//on the next turn if bumping into an obstacle.
			this->wLastCheckpointX = wX;
			this->wLastCheckpointY = wY;
		} else if (this->temporalSplit.queuing()) {
			ActivateTemporalSplit(CueEvents);
		}
		return;
	}

	const UINT wOriginalMonsterCount = this->pRoom->wMonsterCount;
	const UINT bConquerTokenNeedsActivating = this->pRoom->bHasConquerToken &&
			this->conquerTokenTurn==NO_CONQUER_TOKEN_TURN;
	const RoomCompletionData roomCompletionData(wOriginalMonsterCount, bConquerTokenNeedsActivating);
	this->bContinueCutScene = false;

	//If there are any unanswered questions, process them.
	if (bPlayerIsAnsweringQuestion)
	{
		//If player has questions to answer during a cut scene, wait until player responds.
		if (this->dwCutScene && nCommand == CMD_WAIT)
			return;

		//Decode command data.
		const int nArgument = nCommand == CMD_ANSWER ? wX*256 + wY : nCommand;
		ProcessUnansweredQuestions(nArgument, this->UnansweredQuestions, CueEvents);

		//After answering all questions, allow NPCs to execute non turn-expending commands.
		if (this->UnansweredQuestions.empty())
		{
			this->bExecuteNoMoveCommands = true;
			CMonster *pMonster = this->pRoom->pFirstMonster;
			while (pMonster)
			{
				if (pMonster->wType == M_CHARACTER)
					pMonster->Process(CMD_WAIT, CueEvents);
				pMonster = pMonster->pNext;
			}
			this->bExecuteNoMoveCommands = false;
			this->pRoom->ProcessTurn(CueEvents, false);
		}
	} else {
		bool bPlayerLeftRoom = false;
		if (this->swordsman.wPlacingDoubleType)
		{
			ProcessDoublePlacement(nCommand, CueEvents, wX, wY);
			this->bContinueCutScene = true;
		} else {
			//If an answer/double command is replayed in an invalid context, ignore it.
			if (bIsComplexCommand(nCommand))
				return;

			//If CMD Key was used, trigger the event
			if (nCommand == CMD_EXEC_COMMAND)
			{
				CueEvents.Add(CID_CommandKeyPressed);
				nCommand = CMD_WAIT; //For all other interactions, treat as a Wait
			}

			//Swordsman tired logic.
			unsigned char *pbMonstersKilled = this->monstersKilled + (this->wTurnNo % TIRED_TURN_COUNT);
			this->wMonstersKilledRecently -= *pbMonstersKilled;
			*pbMonstersKilled = 0;

			const bool bPlayerIsEnemyTarget = this->swordsman.IsTarget();

			//Player takes a turn when in the room.
			++this->wPlayerTurn;    //do first -- increment even when CIDA_PlayerLeftRoom and before demo is saved
			if (this->swordsman.IsInRoom())
				ProcessPlayer(nCommand, CueEvents);

			if (RemoveInvalidCommand(CueEvents))
				return;

			bPlayerLeftRoom = CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerLeftRoom), CIDA_PlayerLeftRoom);
			if (bPlayerLeftRoom)
			{
				//If play in room stopped, then room processing won't take place
				//and outstanding data must be cleaned up here.
				CPlatform::clearFallTiles();
				UpdatePrevCoords(); //monsters are no longer moving from previous position
			} else {
				//When player becomes an enemy target this turn, brain pathmap needs to be updated.
				if (!bPlayerIsEnemyTarget && this->swordsman.IsTarget())
					this->pRoom->SetPathMapsTarget(this->swordsman.wX, this->swordsman.wY);

				ProcessReactionToPlayerMove(nCommand, CueEvents);
			}
		}
	}

	ProcessRoomCompletion(roomCompletionData, CueEvents);

	//Check for new questions that were asked.  Put them in a list of questions
	//for which answers will be expected on subsequent calls.
	AddQuestionsToList(CueEvents, this->UnansweredQuestions);

	if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied))
	{
		this->swordsman.bIsDying = true;
		this->bIsGameActive = false;
		++this->dwLevelDeaths;
		//Manual and automatic recording of death demos handled in front end.
	}

	//Activate checkpoint if player moved onto it.
	if (bIsMovementCommand(nCommand) && (this->swordsman.wX != this->swordsman.wPrevX ||
				this->swordsman.wY != this->swordsman.wPrevY))
		QueryCheckpoint(CueEvents, this->swordsman.wX, this->swordsman.wY);
	if (CueEvents.HasOccurred(CID_CheckpointActivated))
	{
		ProcessCheckpointActivation(CueEvents);
	} else {
		//If checkpoint wasn't touched this turn, allow saving to it starting next turn.
		if (this->pRoom->IsValidColRow(this->wLastCheckpointX, this->wLastCheckpointY) &&
				(this->wLastCheckpointX != this->swordsman.wX ||
					this->wLastCheckpointY != this->swordsman.wY))
		{
			//Also, if no monster is activating a checkpoint.
			CMonster *pMonster = this->pRoom->GetMonsterAtSquare(this->wLastCheckpointX, this->wLastCheckpointY);
			if (!pMonster || !bMonsterCanActivateCheckpoint(pMonster->wType))
				this->wLastCheckpointX = this->wLastCheckpointY = NO_CHECKPOINT;
		}
	}

	//If the room was just conquered, toggle the green doors.
	if (CueEvents.HasOccurred(CID_RoomConquerPending) && !this->pRoom->bGreenDoorsOpened)
		if (ToggleGreenDoors(CueEvents))
			//Adding this event with true flag indicates green doors toggled?
			CueEvents.Add(CID_RoomConquerPending, new CAttachableWrapper<bool>(true), true);

	//Return cue event for plots if any plots were made.  This check needs
	//to go after any code that could call pRoom->Plot().
	if (this->pRoom->PlotsMade.size())
		CueEvents.Add(CID_Plots, &(this->pRoom->PlotsMade));

	//During active moves, check for swords colliding.
	if (!this->swordsman.wPlacingDoubleType) {
		const WeaponType wt = this->pRoom->SwordfightCheck();
		if (wt != WT_Off)
			CueEvents.Add(CID_Swordfight, new CAttachableWrapper<int>(int(wt)), true);
	}

	//Call once all cue events could have fired.
	if (this->bIsGameActive) //don't need to check if game is no longer in play (incl. transitioning to a new level)
		this->pRoom->CharactersCheckForCueEvents(CueEvents);

	//Cut scene updates.
	if (!this->bContinueCutScene)
		this->dwCutScene = 0;
	if (this->dwCutScene != 0 && this->cutSceneStartTurn == -1)
		this->cutSceneStartTurn = this->wTurnNo;

	//Player should always be visible while cut scene is not playing.
	if (!this->swordsman.IsInRoom() && !this->dwCutScene && !this->bSwordsmanOutsideRoom)
		SetPlayerRole(M_BEETHRO, CueEvents); //place player in room now as default (Beethro)
	//Update path maps to NPC swordsman.
	if (!bIsSmitemaster(this->swordsman.wAppearance))
	{
		UINT wSX, wSY;
		if (GetSwordsman(wSX, wSY))
			this->pRoom->SetPathMapsTarget(wSX, wSY);
	}

	if (this->wMonsterKillCombo && this->wMonsterKillCombo == monsterKillComboAtStartOfTurn &&
		!bPlacingDoubleAtStartOfTurn)
	{
		//Kill combo has ended.
		static const UINT MIN_KILL_COMBO = 10;
		if (this->wMonsterKillCombo >= MIN_KILL_COMBO)
			CueEvents.Add(CID_Combo, new CAttachableWrapper<UINT>(this->wMonsterKillCombo), true);
		this->wMonsterKillCombo = 0;
	}

	ResetTemporalSplitQueuingIfInvalid(CueEvents);

	//Updates sounds/speech, unless player has left room, in which case it was already
	//processed for entering the new room.
	if (!CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerLeftRoom), CIDA_PlayerLeftRoom))
		StashPersistingEvents(CueEvents);

	if (this->pRoom->IsRoomLightingChanged())
		CueEvents.Add(CID_LightToggled);

	if (CueEvents.HasOccurred(CID_ChallengeCompleted) && !this->Commands.IsFrozen() && !this->bNoSaves)
		FlagChallengesCompleted(CueEvents);

	//Should never have anything left unprocessed at end of turn.
	ASSERT(!CPlatform::fallTilesPending());

	//Make a queue of periodic game snapshots that can be retrieved to reduce
	//in-game rewind/replay time.
	const UINT dwElapsed = (GetTicks() - dwStart) + 1;
	this->dwComputationTime += dwElapsed;
	if (TakeSnapshotNow())
		SnapshotGameState();
}

//***************************************************************************************
void CCurrentGame::ProcessCheckpointActivation(CCueEvents& CueEvents)
{
	if (this->dwCutScene)
	{
		//Don't let checkpoints be activated during a cut scene.
		CueEvents.ClearEvent(CID_CheckpointActivated);
	} else {
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*,
				CueEvents.GetFirstPrivateData(CID_CheckpointActivated));
		ASSERT(pCoord);
		this->wLastCheckpointX = pCoord->wX;
		this->wLastCheckpointY = pCoord->wY;

		this->checkpointTurns.push_back(this->wTurnNo);
		//If player is still alive after checkpoint was stepped on:
		if (!this->Commands.IsFrozen() && this->bIsGameActive)
			SaveToCheckpoint();
	}
}

//***************************************************************************************
void CCurrentGame::SnapshotGameState()
{
	this->dwComputationTime = 0; //reset before saving snapshot
	CCurrentGame *pNewSnapshot = new CCurrentGame(*this);
	if (pNewSnapshot)
	{
		pNewSnapshot->pSnapshotGame = this->pSnapshotGame;
		this->pSnapshotGame = pNewSnapshot;
		++this->numSnapshots;
	}
}

//***************************************************************************************
void CCurrentGame::ActivateTemporalSplit(CCueEvents& CueEvents)
{
	//Rewind play to the point when queuing started.
	ASSERT(this->temporalSplit.queuingTurn >= 0);
	const int num_commands_to_undo = this->Commands.Count() - this->temporalSplit.queuingTurn;
	const vector<int> player_commands = this->temporalSplit.player_commands;
	if (num_commands_to_undo > 0)
	{
		//Prevent Commands alterations and multiple freezes while rewinding game state
		ASSERT(!this->activatingTemporalSplit || this->Commands.IsFrozen());
		++this->activatingTemporalSplit;

		//Retain current room turn counts.
		const UINT playerTurn_ = this->wPlayerTurn;
		const UINT turnNo_ = this->wTurnNo;
		const vector<UINT> checkpointTurns_ = this->checkpointTurns;

		UndoCommands(num_commands_to_undo, CueEvents);

		this->wPlayerTurn = playerTurn_;
		this->wTurnNo = turnNo_;
		this->checkpointTurns = checkpointTurns_;

		ASSERT(this->activatingTemporalSplit > 0);
		--this->activatingTemporalSplit;

		//Ensure player ended up back where temporal split sequence originated.
		ASSERT(this->swordsman.wX == (UINT)this->temporalSplit.x);
		ASSERT(this->swordsman.wY == (UINT)this->temporalSplit.y);
		ASSERT(this->pRoom->GetTSquare(this->swordsman.wX, this->swordsman.wY) == T_TOKEN);
		ASSERT(bTokenActive(this->pRoom->GetTParam(this->swordsman.wX, this->swordsman.wY)));
	}
	this->temporalSplit.clear();

	//Create temporal clone w/ inputted commands.
	CMonster *pMonster = this->pRoom->AddNewMonster(
			M_TEMPORALCLONE, this->swordsman.wX, this->swordsman.wY);
	pMonster->wO = this->swordsman.wO;
	CTemporalClone *pClone = DYN_CAST(CTemporalClone*, CMonster*, pMonster);
	pClone->bInvisible = this->swordsman.bIsInvisible;
	pClone->bIsTarget = this->swordsman.IsTarget();
	pClone->weaponType = this->swordsman.GetActiveWeapon();
	pClone->wIdentity = this->swordsman.wIdentity;
	pClone->wAppearance = this->swordsman.wAppearance;
	pClone->SetWeaponSheathed();
	pClone->SetMovementType();
	pClone->InputCommands(player_commands);

	CueEvents.Add(CID_ActivatedTemporalSplit);

	this->pRoom->SetTParam(this->swordsman.wX, this->swordsman.wY, TemporalSplitUsed);

	if (CueEvents.HasOccurred(CID_CheckpointActivated))
		ProcessCheckpointActivation(CueEvents);

	//Ensure future undoing beyond this turn does not require replaying this temporal split.
	//That would be slow, and recursive temporal activations probably don't work as implemented.
	//Pre-condition: nothing else happens to the game state in ProcessCommand after this point
	SnapshotGameState();
}

//***************************************************************************************
void CCurrentGame::AddTemporalSplitCommand(int nCommand, bool moved)
{
	if (this->temporalSplit.queuing()) {
		if (ContinueQueuingTemporalSplitMoves()) {
			this->temporalSplit.add_move(nCommand, moved);
		} else {
			this->temporalSplit.clear();
		}
	}
}

//***************************************************************************************
bool CCurrentGame::ContinueQueuingTemporalSplitMoves() const
{
	ASSERT(this->temporalSplit.queuing());

	if (!this->swordsman.IsInRoom())
		return false;

	if (this->pRoom->GetBottomTSquare(this->temporalSplit.x, this->temporalSplit.y) != T_TOKEN)
		return false;
	if (calcTokenType(this->pRoom->GetTParam(this->temporalSplit.x, this->temporalSplit.y)) != TemporalSplit)
		return false;

	return true;
}

//***************************************************************************************
void CCurrentGame::ResetTemporalSplitQueuingIfInvalid(CCueEvents& CueEvents)
{
	if (this->temporalSplit.queuing())
	{
		if (!ContinueQueuingTemporalSplitMoves()) {
			this->temporalSplit.clear();
		} else {
			//When activating a temporal split token, the player must end the turn on the token to activate it
			if (this->temporalSplit.queuingTurn == int(this->wTurnNo) && !IsPlayerAt(this->temporalSplit.x, this->temporalSplit.y))
				ResetPendingTemporalSplit(CueEvents);
		}
	}
}

//*****************************************************************************
void CCurrentGame::ResetPendingTemporalSplit(CCueEvents& CueEvents)
{
	if (this->temporalSplit.queuing()) {
		this->pRoom->DisableToken(CueEvents, this->temporalSplit.x, this->temporalSplit.y);
		CueEvents.ClearEvent(CID_TemporalSplitStart);
		this->temporalSplit.clear();
	}
}


//***************************************************************************************
void CCurrentGame::ProcessRoomCompletion(RoomCompletionData roomCompletionData, CCueEvents& CueEvents)
// Does all the necessary room completion checks and adds the event if it should be completed
{
	const bool bPlayerLeftRoom = CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerLeftRoom), CIDA_PlayerLeftRoom);
	const bool bIsLastMonsterKilled = roomCompletionData.wOriginalMonsterCount && !this->pRoom->wMonsterCount;

	const bool bIsKillMonstersSatisfied = (
		!roomCompletionData.bConquerTokenNeedsActivating
		&& roomCompletionData.wOriginalMonsterCount 
		&& !this->pRoom->wMonsterCount
	);
	const bool bIsConquerTokenSatisfied = (
		!this->pRoom->wMonsterCount
		&& roomCompletionData.bConquerTokenNeedsActivating
		&& this->conquerTokenTurn != NO_CONQUER_TOKEN_TURN
	);
	const bool bIsAnyCompletionSatisfied = (
		bIsKillMonstersSatisfied
		|| bIsConquerTokenSatisfied
	);

	//Was the room just conquered?
	//Criteria for conquering are:
	//1. Player didn't just leave the room.
	//2. No monsters in the room.
	//3. If Conquer tokens are in the room, one must be touched.
	if (!bPlayerLeftRoom && bIsAnyCompletionSatisfied)
		if (!IsCurrentRoomConquered())   // and was it not already cleared?
			CueEvents.Add(CID_RoomConquerPending);
}
//***************************************************************************************
void CCurrentGame::ProcessReactionToPlayerMove(int nCommand, CCueEvents& CueEvents)
//After the player's turn, and the room hasn't been exited, everything else in the room takes a turn.
{
	this->bHalfTurn = this->swordsman.bIsHasted && !this->bHalfTurn;

	// Explode any kegs that might've been stabbed by player push
	this->pRoom->ExplodeStabbedPowderKegs(CueEvents);

	ProcessMonsters(nCommand, CueEvents);

	this->pRoom->ClearPushStates();
	this->pRoom->ExplodeStabbedPowderKegs(CueEvents);

	//Check for stuff falling as a result of monster moves now.
	if (CPlatform::fallTilesPending())
		CPlatform::checkForFalling(this->pRoom, CueEvents);

	ResolveSimultaneousTarstuffStabs(CueEvents);

	this->pRoom->ProcessTurn(CueEvents, !this->bHalfTurn);

	//If room processing caused any more tar stabs (e.g. by floor spikes), handle again.
	ResolveSimultaneousTarstuffStabs(CueEvents);

	if (this->pRoom->bTarWasBuilt)
	{
		this->pRoom->bTarWasBuilt = false;
		this->pRoom->BreakUnstableTar(CueEvents);
	}

	//If oremites temporarily took away the player weapon, but then something covered it up
	//and the weapon came back, process a weapon hit now.
	//NOTE: It won't be synched with other weapon hits, but that's the way it goes.
	if (bEntityHasSword(this->swordsman.wAppearance) && !this->swordsman.HasWeapon())
	{
		SetPlayerWeaponSheathedState();
		if (this->swordsman.HasWeapon())
		{
			const int dx = this->swordsman.wX - this->swordsman.wPrevX,
				dy = this->swordsman.wY - this->swordsman.wPrevY;
			ProcessPlayerWeapon(dx, dy, CueEvents);
			ResolveSimultaneousTarstuffStabs(CueEvents);

			//In case sword hit changed something, must check room stuff again.
			this->pRoom->ProcessTurn(CueEvents, false);
		}
	}

	this->pRoom->PostProcessTurn(CueEvents, false);

	SetPlayerMood(CueEvents);
}

//***************************************************************************************
bool CCurrentGame::RemoveInvalidCommand(const CCueEvents& CueEvents)
{
	//If player tried to exit room when locked, then unwind the move in
	//progress as if it didn't happen, except for receiving the exit locked event.
	if (CueEvents.HasOccurred(CID_RoomExitLocked) ||
		CueEvents.HasOccurred(CID_BumpedMasterWall) ||
		CueEvents.HasOccurred(CID_BumpedHoldCompleteWall))
	{
		//Room exit should never be locked during move playback.
		ASSERT(!CueEvents.HasOccurred(CID_RoomExitLocked) || !this->Commands.IsFrozen());
		//Master wall could be bumped during a playback if the room was modified after this playback was recorded.

		//The above cue events can only be triggered by a player manually inputting movement commands.
		ASSERT(!this->dwCutScene);

		//Undo move counts.  Act as though this move was never made.
		ASSERT(this->wPlayerTurn);
		--this->wPlayerTurn;
		ASSERT(this->wTurnNo);
		--this->wTurnNo;
		ASSERT(this->dwLevelMoves);
		--this->dwLevelMoves;
		if (!this->Commands.IsFrozen())
			this->Commands.RemoveLast();

		return true;
	}
	return false;
}

//***************************************************************************************
void CCurrentGame::ProcessArmedMonsterWeapon(CArmedMonster* pArmedMonster, CCueEvents& CueEvents)
{
	if (!pArmedMonster->HasSword())
		return;

	const UINT weaponX = pArmedMonster->GetWeaponX(), weaponY = pArmedMonster->GetWeaponY();
	const int dx = nGetDX(pArmedMonster->wPrevX, pArmedMonster->wX);
	const int dy = nGetDX(pArmedMonster->wPrevY, pArmedMonster->wY);

	bool bPerformPush = true;
	if (pArmedMonster->CanAttackWithWeaponTowards(dx, dy)) {
		ProcessWeaponHit(weaponX, weaponY, CueEvents, pArmedMonster);

		const CMonster* pMonster = this->pRoom->GetMonsterAtSquare(weaponX, weaponY);
		bPerformPush = pMonster && pMonster->IsPushableByWeaponAttack();
	}

	if (bPerformPush) {
		const WeaponStab push(weaponX, weaponY,
				pArmedMonster->wSwordMovement, pArmedMonster->GetWeaponType());
		ProcessWeaponPush(push, dx, dy, OAT_Monster, CueEvents, pArmedMonster);
	}
}

//***************************************************************************************
void CCurrentGame::ProcessPlayerWeapon(int dx, int dy, CCueEvents& CueEvents)
{
	if (!this->swordsman.HasWeapon())
		return;

	bool bPerformPush = true;
	if (this->swordsman.CanAttackTowards(dx, dy)) {
		ProcessWeaponHit(this->swordsman.wSwordX, this->swordsman.wSwordY, CueEvents);

		const CMonster* pMonster = this->pRoom->GetMonsterAtSquare(this->swordsman.wSwordX, this->swordsman.wSwordY);
		bPerformPush = pMonster && pMonster->IsPushableByWeaponAttack();
	}

	if (bPerformPush) {
		WeaponStab push(this->swordsman.wSwordX, this->swordsman.wSwordY,
				this->swordsman.wSwordMovement, this->swordsman.GetActiveWeapon());
		push.pPlayer = &this->swordsman;
		ProcessWeaponPush(push, dx, dy, OAT_Player, CueEvents);
	}
}

//***************************************************************************************
void CCurrentGame::ProcessWeaponHit(
//Processes results of a weapon (player's or otherwise) entering a square.
//
//Params:
	const UINT wSX, const UINT wSY,        //(in)   Square sword is in.
	CCueEvents &CueEvents,     //(out)  List of events that can be handled by caller.
								//    These are things that the UI wouldn't necessarily
								//    be aware of by looking at the modified game
								//    data on return.
	CArmedMonster *pArmedMonster) //(in)   If NULL (default) this call is checking the
								//    player's sword.  Otherwise, this will be a
								//    pointer to a double and the call will be checking
								//    that double's sword.
{
	if (!this->pRoom->IsValidColRow(wSX, wSY))
		return;

	WeaponStab stab(wSX, wSY);

	if (pArmedMonster) {
		stab.direction = pArmedMonster->wSwordMovement;
		stab.weaponType = pArmedMonster->GetWeaponType();
		stab.pArmedMonster = pArmedMonster;
	} else {
		stab.direction = GetSwordMovement();
		stab.weaponType = this->swordsman.GetActiveWeapon();
		stab.pPlayer = &this->swordsman;
	}

	StabRoomTile(stab, CueEvents);
}

//***************************************************************************************
void CCurrentGame::ProcessWeaponPush(
	const WeaponStab& push,
	int dx, int dy,
	const OrbActivationType eActivationType,
	CCueEvents &CueEvents,
	CArmedMonster *pArmedMonster)
{
	// This is an ugly hack because Characters with imperative Pushable By Weapon
	// Can be pushed around regardless of the weapon used
	CMonster *pMonster = this->pRoom->GetMonsterAtSquare(push.wX, push.wY);

	//Blades cannot push but sword can push monsters which can be pushed by attacks (
	if (push.weaponType == WT_Dagger ||
			(push.weaponType == WT_Sword && (!pMonster || !pMonster->IsPushableByWeaponAttack())))
		return;

	UINT pushX, pushY;
	if (dx || dy) {
		pushX = dx;
		pushY = dy;
	} else {
		pushX = nGetOX(push.direction);
		pushY = nGetOY(push.direction);
	}
	WeaponPushback(push, pushX, pushY, eActivationType, CueEvents, pArmedMonster);
}

//***************************************************************************************
void CCurrentGame::ProcessScriptedPush(
	const WeaponStab& push,
	CCueEvents& CueEvents,
	CCharacter* pCharacter
)
{
	ProcessWeaponPush(push, 0, 0, OrbActivationType::OAT_Monster, CueEvents, pCharacter);
}

//*****************************************************************************
bool CCurrentGame::PushPlayerInDirection(int dx, int dy, CCueEvents &CueEvents)
// Returns true if the player has been moved
{
	const UINT wFromX = this->swordsman.wX;
	const UINT wFromY = this->swordsman.wY;
	const UINT wDestX = wFromX + dx;
	const UINT wDestY = wFromY + dy;
	const UINT wPushO = nGetO(dx, dy);
	const bool bEnteredTunnel = PlayerEnteredTunnel(this->pRoom->GetOSquare(wFromX, wFromY), wPushO);

	if (!this->pRoom->IsValidColRow(wDestX, wDestY))
		return false;

	//Player can be pushed only once during each entity's turn.
	if (this->pRoom->pushed_player)
		return false;

	if (bEnteredTunnel)
	{
		CueEvents.Add(CID_Tunnel);
		TunnelMove(dx,dy);
	} else {
		CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wDestX, wDestY);
		if (pMonster != NULL && pMonster->wType != M_FLUFFBABY)
			return false;
		if (!this->pRoom->CanPushOntoOTile(wDestX, wDestY))
			return false;
		if (!this->pRoom->CanPushOntoTTile(wDestX, wDestY))
			return false;
		if (!this->pRoom->CanPushOntoFTile(this->swordsman.wX, this->swordsman.wY, wDestX, wDestY))
			return false;
        if (this->pRoom->DoesGentryiiPreventDiagonal(this->swordsman.wX, this->swordsman.wY, wDestX, wDestY))
            return false;

		for (UINT nO = 0; nO < ORIENTATION_COUNT; ++nO) {
			if (nO != NO_ORIENTATION)
			{
				CMonster* pMonster = this->pRoom->GetMonsterAtSquare(wDestX - nGetOX(nO), wDestY - nGetOY(nO));
				if (pMonster && pMonster->HasSwordAt(wDestX, wDestY) && pMonster->GetWeaponType() == WT_Caber)
					return false;
			}
		}

		if (!this->swordsman.Move(wDestX, wDestY))
			return false;

		if (pMonster && pMonster->wType == M_FLUFFBABY) {
			this->pRoom->KillMonster(pMonster,CueEvents,false,&this->swordsman);
			this->pRoom->ProcessPuffAttack(CueEvents, this->swordsman.wX, this->swordsman.wY);
		}

		this->pRoom->CheckForFallingAt(wDestX, wDestY, CueEvents);

		if (this->swordsman.CanDropTrapdoor(this->pRoom->GetOSquare(wFromX,wFromY)))
			this->pRoom->DestroyTrapdoor(wFromX,wFromY,CueEvents);
	}

	this->pRoom->pushed_player = true;

	SetPlayerWeaponSheathedState();
	this->swordsman.wSwordMovement = nGetO(dx, dy);

	ProcessPlayerMoveInteraction(dx, dy, CueEvents, false, true);

	if (this->pDyingEntity == NULL)
	{
		if (this->swordsman.wAppearance == M_CONSTRUCT && pRoom->GetOSquare(this->swordsman.wX, this->swordsman.wY) == T_GOO && this->wTurnNo > 0){
			SetDyingEntity(&this->swordsman);
			CueEvents.Add(CID_PlayerEatenByOremites);
		} else {
			QueryCheckpoint(CueEvents, wDestX, wDestY);
		}
	}

	//Display non-functional temporary stun effect to highlight player was pushed
	CueEvents.Add(CID_Stun, new CStunTarget(NULL, 1, true), true);

	if (this->swordsman.IsTarget())
		this->pRoom->SetPathMapsTarget(wDestX, wDestY);

	return true;
}

//***************************************************************************************
void CCurrentGame::StabMonsterAt(
	const WeaponStab& stab,
	CCueEvents& CueEvents)
{
	const UINT wSX = stab.wX;
	const UINT wSY = stab.wY;

	const CEntity *pKillingEntity = stab.pPlayer;
	if (!pKillingEntity)
		pKillingEntity = stab.pArmedMonster;

	//Did stab hit a monster?
	CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wSX, wSY);
	if (!pMonster)
		return;

	//don't allow critical character kill on room entrance
	if (!this->wTurnNo && (
			this->pRoom->IsMonsterOfTypeAt(M_HALPH, wSX, wSY, true) ||
			this->pRoom->IsMonsterOfTypeAt(M_HALPH2, wSX, wSY, true) ||
			this->pRoom->IsMonsterOfTypeAt(M_CLONE, wSX, wSY, true) ||
			this->pRoom->IsMonsterOfTypeAt(M_TEMPORALCLONE, wSX, wSY, true) ||
			this->pRoom->IsMonsterOfTypeAt(M_NEATHER, wSX, wSY, true)
			))
		return;

	pMonster = pMonster->GetOwningMonster();

	if (stab.weaponType == WT_FloorSpikes && pMonster->IsFlying())
		return;

	//Each monster can be stabbed by spikes or firetraps once per turn.
	//Example: rattlesnake should not be stabbed multiple times based on tail-body orientation
	// The exception are rattlesnakes which can have their tail stabbed on a given turn and then have its head burned to its demise
	if ((stab.weaponType == WT_FloorSpikes || stab.weaponType == WT_Firetrap) && this->pRoom->monsters_stabbed_by_spikes_this_turn.count(pMonster)){
		if (!(pMonster->wType == M_SERPENTB && stab.weaponType == WT_Firetrap && stab.wX == pMonster->wX && stab.wY == pMonster->wY))
			return;
	}

	const UINT wType = pMonster->wType;
	const bool bFriendly = pMonster->IsFriendly();
	const UINT wOriginalMonsterCount = this->pRoom->wMonsterCount;
	const UINT origStabCount = CueEvents.GetOccurrenceCount(CID_MonsterPieceStabbed);
	if (bIsSerpent(pMonster->wType) && stab.weaponType == WT_Firetrap &&
			pMonster->wX == wSX && pMonster->wY == wSY)
	{
		//Firetraps on serpent head kills it, like a bomb.
		this->pRoom->KillMonster(pMonster, CueEvents);
		CueEvents.Add(CID_MonsterPieceStabbed, new CMoveCoord(wSX, wSY, NO_ORIENTATION), true);
		CheckTallyKill(pMonster);
	}
	else if (pMonster->OnStabbed(CueEvents, wSX, wSY, stab.weaponType))
	{
		if (CueEvents.HasOccurredWith(CID_MonsterDiedFromStab, pMonster))
		{
			//Store info about stab effect.
			pMonster->SetKillInfo(stab.direction);

			if (bIsRockGolemType(wType))
			{
				//Rock golems over shallow water are killed normally and create stepping stones
				if (this->pRoom->GetOSquare(wSX,wSY) == T_SHALLOW_WATER)
				{
					this->pRoom->KillMonster(pMonster, CueEvents, false, pKillingEntity);
					this->pRoom->Plot(wSX,wSY,T_STEP_STONE);
					CueEvents.Add(CID_Splash, new CCoord(wSX,wSY), true);
				}
				//Monster was disabled previously, but should not be removed from the room.
				TallyKill();
				if (stab.pPlayer)
					TallyMonsterKilledByPlayerThisTurn();
			} else {
				this->pRoom->KillMonster(pMonster, CueEvents, false, pKillingEntity);

				//If monster was killed, add to tally.
				if (this->pRoom->wMonsterCount < wOriginalMonsterCount)
				{
					CheckTallyKill(pMonster);
					if (stab.pPlayer)
						TallyMonsterKilledByPlayerThisTurn();
				}
				else if (wType == M_SLAYER || wType == M_SLAYER2)
					TallyKill();
				if (wType == M_ROCKGIANT)
				{

					ROOMCOORD attackCoords = ROOMCOORD(wSX, wSY);
					CRockGiant::Shatter(CueEvents, this, pMonster->wX, pMonster->wY, false, &attackCoords);
					//Make sure the Golem at (wSX,wSY) is killed
					CMonster *pNewGolem = this->pRoom->GetMonsterAtSquare(wSX, wSY);
					if (pNewGolem)
						pNewGolem->OnStabbed(CueEvents, (UINT)-1, (UINT)-1, stab.weaponType);
				}
			}
		}
		else if (CueEvents.HasOccurredWith(CID_SnakeDiedFromTruncation, pMonster))
		{
			ASSERT(pMonster->bAlive);
			ASSERT(pMonster->wType == M_SERPENTB || pMonster->wType == M_SERPENTG);
			this->pRoom->KillMonster(pMonster, CueEvents, false, pKillingEntity);
			TallyKill();
		}

		const UINT wTarType = this->pRoom->GetTSquare(wSX, wSY);
		if (bIsTar(wTarType)) {
			if (stab.weaponType == WT_Caber)
			{
				//Cabers can't destroy tarstuff normally,
				//so we should flag unstable tarstuff now
				if (!this->pRoom->IsTarStableAt(wSX,wSY,wTarType))
					CueEvents.Add(CID_TarstuffStabbed, new CMoveCoord(wSX,wSY,stab.direction), true);
			} else if (pMonster->wType == M_TARMOTHER || pMonster->wType == M_GELMOTHER) {
				//Mark tarstuff under mother for removal.
				CueEvents.Add(CID_TarstuffStabbed, new CMoveCoord(wSX,wSY,stab.direction), true);
			}
			//else: Mud is only removed here if it is vulnerable.  Handled below.
		}

		//When player attacks a non-friendly entity...
		if (!bFriendly)
		{
			if (stab.pPlayer)
				SetPlayerAsTarget(); //Player is acquired as a target
			else if (stab.pArmedMonster && stab.pArmedMonster->wType == M_TEMPORALCLONE)
			{
				CTemporalClone *pClone = const_cast<CTemporalClone*>(DYN_CAST(const CTemporalClone*, const CArmedMonster*, stab.pArmedMonster));
				pClone->bIsTarget = true;
			}
		}
	} else {
		//Monster (piece) was stabbed but monster is not killed.
		if (CueEvents.GetOccurrenceCount(CID_MonsterPieceStabbed) > origStabCount)
		{
			if (stab.weaponType == WT_FloorSpikes || stab.weaponType == WT_Firetrap)
				this->pRoom->monsters_stabbed_by_spikes_this_turn.insert(pMonster);

			if (!bFriendly)
			{
				if (stab.pPlayer)
					SetPlayerAsTarget(); //Player is acquired as a target
				else if (stab.pArmedMonster && stab.pArmedMonster->wType == M_TEMPORALCLONE)
				{
					CTemporalClone *pClone = const_cast<CTemporalClone*>(DYN_CAST(const CTemporalClone*, const CArmedMonster*, stab.pArmedMonster));
					pClone->bIsTarget = true;
				}
			}
		}

		//Pickaxe can break rock golem piles.
		if (stab.weaponType == WT_Pickaxe && bIsRockGolemType(wType)) {
			ASSERT(!pMonster->IsAlive());
			CueEvents.Add(CID_MonsterDiedFromStab, pMonster);
			this->pRoom->KillMonster(pMonster, CueEvents, false, pKillingEntity);

			//Rock golem is no longer an obstacle, so pathmaps might need updating.
			this->pRoom->UpdatePathMapAt(wSX, wSY);
			this->pRoom->RecalcStationPaths();

			TallyKill();
		} else if (wType == M_FLUFFBABY &&
				(stab.weaponType == WT_Firetrap || stab.weaponType == WT_Caber)) {
			this->pRoom->KillMonster(pMonster, CueEvents, false, pKillingEntity);
			this->pRoom->ProcessPuffAttack(CueEvents, wSX, wSY);
		}
	}
}

//Add kill to "swordsman tired" tally.
void CCurrentGame::TallyMonsterKilledByPlayerThisTurn()
{
	++this->wMonstersKilledRecently;
	++this->monstersKilled[this->wTurnNo % TIRED_TURN_COUNT];
}

//***************************************************************************************
void CCurrentGame::StabRoomTile(
	const WeaponStab& stab,
	CCueEvents& CueEvents)
{
	StabMonsterAt(stab, CueEvents);

	const UINT wSX = stab.wX;
	const UINT wSY = stab.wY;

	//Check for things in T-square for weapon to hit.
	UINT wTileNo = this->pRoom->GetTSquare(wSX, wSY);
	switch (wTileNo)
	{
		case T_ORB:
			this->pRoom->ActivateOrb(wSX, wSY, CueEvents,
					stab.pPlayer ? OAT_Player : (stab.pArmedMonster ? OAT_Monster : OAT_Item));
		break;
		case T_BEACON: case T_BEACON_OFF:
			this->pRoom->ActivateBeacon(wSX, wSY, CueEvents);
		break;

		case T_TAR:	case T_MUD: case T_GEL:
			if (stab.weaponType == WT_Caber)
				break;
			if (this->pRoom->StabTar(wSX, wSY, CueEvents, false) || //don't remove tar yet!
				stab.weaponType == WT_Spear || stab.weaponType == WT_Firetrap)
			{
				//Stab hits vulnerable tar -- mark for removal at end of turn
				CueEvents.Add(CID_TarstuffStabbed, new CMoveCoord(wSX,wSY,stab.direction), true);
			}
		break;

		case T_BOMB:
			//Explode bomb immediately (probably kills player).
			if (this->wTurnNo)   //don't kill player on room entrance
				this->pRoom->ExplodeBomb(CueEvents, wSX, wSY);
		break;
		case T_POWDER_KEG:
			//Explode immediately (probably kills player).
			if (this->wTurnNo)   //don't kill player on room entrance
				this->pRoom->ExplodePowderKeg(CueEvents, wSX, wSY);
		break;
		case T_FUSE:
			if (stab.weaponType == WT_Firetrap) {
				this->pRoom->LightFuse(CueEvents, wSX, wSY);
			}
		break;

		case T_MIRROR:
		{
			if (stab.weaponType == WT_Firetrap)
				break;

			//Swords smash mirror blocks when striking head on, otherwise they push them.
			UINT wSrcX, wSrcY;   //where sword wielder is standing
			if (stab.pArmedMonster)
			{
				wSrcX = stab.pArmedMonster->wX;
				wSrcY = stab.pArmedMonster->wY;
			} else if (stab.pPlayer) {
				wSrcX = stab.pPlayer->wX;
				wSrcY = stab.pPlayer->wY;
			} else {
				wSrcX = wSX;
				wSrcY = wSY;
			}
			const bool headonStrike = wSrcX + nGetOX(stab.direction) == wSX && wSrcY + nGetOY(stab.direction) == wSY;
			if (headonStrike || (stab.weaponType != WT_Sword))
			{
				//Strike shatters mirror.
				this->pRoom->Plot(wSX, wSY, T_EMPTY);
				CueEvents.Add(CID_MirrorShattered, new CMoveCoord(wSX, wSY, stab.direction), true);
			} else {
				//Push mirror, if possible.
				const UINT wDestX = wSX + nGetOX(stab.direction),
						wDestY = wSY + nGetOY(stab.direction);
				if (this->pRoom->CanPushTo(wSX, wSY, wDestX, wDestY))
					this->pRoom->PushTLayerObject(wSX, wSY, wDestX, wDestY, CueEvents);
			}
		}
		break;

		//Briars are destroyed by pickaxe.
		case T_BRIAR_SOURCE:
			if (stab.weaponType == WT_Pickaxe || stab.weaponType == WT_Firetrap) {
				this->pRoom->briars.removeSource(wSX,wSY);
				this->pRoom->Plot(wSX,wSY,T_EMPTY);
				CueEvents.Add(CID_CutBriar, new CMoveCoord(wSX, wSY, stab.direction), true);
			}
		break;
		case T_BRIAR_DEAD: case T_BRIAR_LIVE:
			if (stab.weaponType == WT_Pickaxe || stab.weaponType == WT_Firetrap) {
				this->pRoom->Plot(wSX,wSY,T_EMPTY);
				CueEvents.Add(CID_CutBriar, new CMoveCoord(wSX, wSY, stab.direction), true);
			}
		break;

		case T_FLUFF:
			if (stab.weaponType == WT_Firetrap || stab.weaponType == WT_Caber) {
				this->pRoom->RemoveStabbedTar(wSX, wSY, CueEvents, true);
				//Firetraps should leave Fluff conversion until after all firetraps have activated
				if (stab.weaponType == WT_Caber)
					this->pRoom->ConvertUnstableTar(CueEvents, true);
				CueEvents.Add(CID_FluffDestroyed, new CMoveCoordEx(wSX, wSY, NO_ORIENTATION, T_FLUFF), true);
			}
		break;
	}

	//Check for things in O-square for sword to hit.

	//Did sword hit a crumbly wall?
	wTileNo = this->pRoom->GetOSquare(wSX, wSY);
	switch (wTileNo)
	{
		case T_WALL_B:
		case T_WALL_H:
			if (stab.weaponType != WT_Caber)
				this->pRoom->DestroyCrumblyWall(wSX, wSY, CueEvents, stab.direction);
		break;
		default: break;
	}

	//Player wounded?
	if (!stab.pPlayer && IsPlayerAt(wSX, wSY))
	{
		if (this->swordsman.IsVulnerableToWeapon(stab.weaponType)
				&& this->wTurnNo) //don't kill player on room entrance
		{
			//If the character's weapon has been marked as safe to the player,
			//then its weapon hits won't harm the player.
			if (stab.pArmedMonster && stab.pArmedMonster->wType == M_CHARACTER)
			{
				const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CArmedMonster*, stab.pArmedMonster);
				if (pCharacter->IsSwordSafeToPlayer())
					return;
			}

			SetDyingEntity(&this->swordsman, stab.pArmedMonster);
			if (stab.pArmedMonster) {
				CueEvents.Add(CID_MonsterKilledPlayer, stab.pArmedMonster);
			} else {
				CueEvents.Add(CID_PlayerImpaled);
			}
		}
	}
}

//*****************************************************************************
bool CCurrentGame::StartTemporalSplit()
{
	return this->temporalSplit.start(this->wTurnNo, this->swordsman.wX, this->swordsman.wY);
}

//*****************************************************************************
void CCurrentGame::WeaponPushback(
	const WeaponStab& push,
	const int dx, const int dy,
	const OrbActivationType eActivationType,
	CCueEvents& CueEvents,
	CArmedMonster *pArmedMonster)
{
	if (!dx && !dy)
		return;

	const WeaponType wt = WeaponType(push.weaponType);
	const UINT wFromX = push.wX;
	const UINT wFromY = push.wY;

	if (!this->pRoom->IsValidColRow(wFromX, wFromY))
		return;

	const UINT wDestX = wFromX + dx, wDestY = wFromY + dy;
	const bool bValidDestinationTile = this->pRoom->IsValidColRow(wDestX, wDestY);

	if (IsPlayerAt(wFromX, wFromY)) {
		if (!PushPlayerInDirection(dx, dy, CueEvents) && wt == WT_Caber){
			StabRoomTile(push, CueEvents);
		}
	}

	//Can push single-tile entities
	CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wFromX, wFromY);
	if (pMonster)
	{
		//Monsters can only be pushed once per entity's turn.
		const bool bMonsterAlreadyPushed = this->pRoom->WasMonsterPushedThisTurn(pMonster);

		if (bValidDestinationTile && !bMonsterAlreadyPushed &&
				this->pRoom->CanPushMonster(pMonster, wFromX, wFromY, wDestX, wDestY)) {
			pMonster->PushInDirection(dx, dy, true, CueEvents);
		} else {
			if (pMonster->wType == M_FLUFFBABY)
			{
				// It is impossible to push puffs against force arrows and ortho squares
				if (bValidDestinationTile && this->pRoom->CanPushOntoFTile(wFromX, wFromY, wDestX, wDestY)){
					this->pRoom->KillMonster(pMonster, CueEvents);
					if (bValidDestinationTile) {
						const CMonster *pMonsterAtDest = this->pRoom->GetMonsterAtSquare(wDestX, wDestY);
						const bool bKillingTarstuffMother = pMonsterAtDest && bIsMother(pMonsterAtDest->wType);
						this->pRoom->ProcessPuffAttack(CueEvents, wDestX, wDestY);
						if (bKillingTarstuffMother)
							this->pRoom->FixUnstableTar(CueEvents);
					}
					else {
						CueEvents.Add(CID_FluffPuffDestroyed, new CCoord(wFromX, wFromY), true);
					}
				} else {
					pMonster->Stun(CueEvents, 1);
					pMonster->bPushedThisTurn = true;

					if (wt == WT_Caber){
						this->pRoom->KillMonster(pMonster, CueEvents);
						this->pRoom->ProcessPuffAttack(CueEvents, wFromX, wFromY);
						CueEvents.Add(CID_FluffPuffDestroyed, new CCoord(wFromX, wFromY), true);
					}
				}
			} else {
				const bool bStunnable = !(pMonster->IsPiece() || (pMonster->IsLongMonster() && pMonster->wType != M_GENTRYII));
				//Using a caber to smash a monster against an obstacle will damage it.
				//If the monster is invulnerable to sword stab, it gets stunned like usual.
				if (wt == WT_Caber) {
					WeaponStab stab(wFromX, wFromY, GetSwordMovement(), WT_Caber);
					if (pArmedMonster)
						stab.pArmedMonster = pArmedMonster;
					else
						stab.pPlayer = &this->swordsman;
				
					StabMonsterAt(stab, CueEvents);
				}

				if (bStunnable && pMonster->IsAlive())
					pMonster->Stun(CueEvents, 1);
			}
		}
	}

	UINT wTileNo = this->pRoom->GetTSquare(wFromX, wFromY);
	switch (wTileNo)
	{
		case T_MIRROR:
		case T_POWDER_KEG:
		{
			//Push item, if possible.

			//Objects can only be pushed once per entity's turn.
			const bool bItemAlreadyPushed = this->pRoom->WasObjectPushedThisTurn(wFromX, wFromY);

			if (bValidDestinationTile && !bItemAlreadyPushed &&
					this->pRoom->CanPushTo(wFromX, wFromY, wDestX, wDestY))
			{
				this->pRoom->PushTLayerObject(wFromX, wFromY, wDestX, wDestY, CueEvents);
			} else {
				//Smashing some items with the caber will break them.
				if (wt == WT_Caber) {
					switch (wTileNo) {
						case T_MIRROR:
							this->pRoom->Plot(wFromX, wFromY, T_EMPTY);
							CueEvents.Add(CID_MirrorShattered, new CMoveCoord(wFromX, wFromY, GetSwordMovement()), true);
						break;
						case T_POWDER_KEG:
							//Explode
							if (this->wTurnNo)   //don't kill player on room entrance
								this->pRoom->ExplodePowderKeg(CueEvents, wFromX, wFromY);
						break;
					}
				}
			}
		}
		break;
		case T_BOMB:
		{
			//Cabers detonate bombs
			if (wt == WT_Caber)
			{
				//Explode bomb immediately
				if (this->wTurnNo)   //don't destroy bombs on entrance
					this->pRoom->ExplodeBomb(CueEvents, wFromX, wFromY);
			}
		}
		break;
		case T_ORB:
		{
			//Orbs activate from pushing
			this->pRoom->ActivateOrb(wFromX, wFromY, CueEvents, eActivationType);
		}
		break;
		case T_BEACON: case T_BEACON_OFF:
		{
			//Beacons activate from pushing
			this->pRoom->ActivateBeacon(wFromX, wFromY, CueEvents);
		}
		break;
		case T_FLUFF:
		{
			//Turn Fluff into Puffs and push them.
			this->pRoom->RemoveStabbedTar(wFromX, wFromY, CueEvents, false);
			this->pRoom->ConvertUnstableTar(CueEvents, true);
			CueEvents.Add(CID_FluffDestroyed, new CMoveCoordEx(wFromX, wFromY, nGetO(dx,dy), T_FLUFF), true);
			CMonster *pNewFluff = this->pRoom->GetMonsterAtSquare(wFromX, wFromY);
			if (!pNewFluff)
			{
				pNewFluff = this->pRoom->AddNewMonster(M_FLUFFBABY,wFromX,wFromY);
				//FIXME-Fluff: Need CID_FluffBabyFormed
				pNewFluff->SetCurrentGame(this);
				pNewFluff->bIsFirstTurn = true;
				pNewFluff->SetOrientation(dx, dy);
			}
			//Now try to push the Puff in the intended direction
			if (bValidDestinationTile &&
					this->pRoom->CanPushMonster(pNewFluff, wFromX, wFromY, wDestX, wDestY)) {
					pNewFluff->PushInDirection(dx, dy, true, CueEvents);
			} else {
				this->pRoom->KillMonster(pNewFluff,CueEvents);
				if (bValidDestinationTile) {
					const CMonster* pMonsterAtDest = this->pRoom->GetMonsterAtSquare(wDestX, wDestY);
					const bool bKillingTarstuffMother = pMonsterAtDest && bIsMother(pMonsterAtDest->wType);
					this->pRoom->ProcessPuffAttack(CueEvents, wDestX, wDestY);
					if (bKillingTarstuffMother)
						this->pRoom->FixUnstableTar(CueEvents);
				}
				else
					CueEvents.Add(CID_FluffPuffDestroyed, new CCoord(wFromX, wFromY), true);
			}
		}
		break;
	}
}

//*****************************************************************************
void CCurrentGame::QueryCheckpoint(CCueEvents& CueEvents, const UINT wX, const UINT wY) const
//If a checkpoint exists at (wX, wY), cue checkpoint save.
{
	//Checkpoint was already saved this turn.
	if (CueEvents.HasOccurred(CID_CheckpointActivated))
		return;

	//Flag checkpoint, unless player settings have disabled them.
	if (this->bIsGameActive &&
		(this->dwAutoSaveOptions & ASO_CHECKPOINT)==ASO_CHECKPOINT &&
		this->pRoom->checkpoints.has(wX, wY))
	{
		//Only save checkpoint when stepping onto it, not while standing on it.
		if ((wX != this->wLastCheckpointX) ||
				(wY != this->wLastCheckpointY))
		{
			//Add CueEvent to handle effect.
			CueEvents.Add(CID_CheckpointActivated, new CCoord(wX, wY), true);
		}
	}
}

 //*****************************************************************************
void CCurrentGame::ReplaceDoubleCommands()
//Before 3.0, saved games and demos recorded double placement as a series of
//move commands followed by CMD_WAIT to place.
//This converts commands of pre-3.0 format to a single CMD_DOUBLE command.
{
	ASSERT(this->swordsman.wPlacingDoubleType);
	ASSERT(!this->Commands.IsFrozen());

	const UINT wCmdStart = this->wTurnNo;
	ASSERT(wCmdStart < this->Commands.GetSize());
	UINT wCmdInd = wCmdStart;
	for (CDbCommands::const_iterator cmdIter = this->Commands.Get(wCmdStart);
	     cmdIter != this->Commands.end(); // Will probably return sooner
	     cmdIter = this->Commands.GetNext(), ++wCmdInd)
	{
		//Complex commands (i.e. CMD_ANSWER in 2.0) are skipped.
		if (bIsComplexCommand(cmdIter->bytCommand))
		{
			cmdIter = this->Commands.GetNext();
			if (cmdIter == this->Commands.end()) //this indicates bad data and shouldn't happen...
				break; //...but we need to be robust to it
			++wCmdInd;
			continue;
		}

		if (cmdIter->bytCommand == CMD_WAIT &&
		    !this->pRoom->DoesSquareContainDoublePlacementObstacle(
					this->swordsman.wDoubleCursorX, this->swordsman.wDoubleCursorY))
		{
			//Found the end placement command.
			++wCmdInd; // the CMD_WAIT is among the replaced commands
			this->Commands.Replace(wCmdStart, wCmdInd, CMD_DOUBLE,
					       this->swordsman.wDoubleCursorX,
					       this->swordsman.wDoubleCursorY);
			return;
		}

		if (cmdIter->bytCommand == CMD_DOUBLE)
		{
			//There shouldn't be 3.0 commands in pre-3.0 turn sequences.
			ASSERT(!"Encountered CMD_DOUBLE in old double placement commands");
			this->Commands.Truncate(wCmdStart);
			return;
		}

		//ProcessDoublePlacement() will only change the cursor
		//position, no actual game state.
		CCueEvents ignore;
		ProcessDoublePlacement(cmdIter->bytCommand, ignore, 0, 0);
	}

	//Got to end of commands without placing the double.
	//Technically, a saved game or demo may end like that.
	//Just truncate move sequence to when double placement began,
	//since there is no CMD_DOUBLE.
	this->Commands.Truncate(wCmdStart);
}

//*****************************************************************************
void CCurrentGame::RestartRoom(
//Restarts the current room.
//
//Params:
	CCueEvents &CueEvents)     //(out)  Cue events generated by swordsman's
								//    first step into the room.
{
	//If in the middle of recording a demo, stop recording.
	UINT wBeginTurn = 0;
	const bool bRecordingDemo = IsDemoRecording();
	if (bRecordingDemo)
	{
		this->bIsDemoRecording = false;
		wBeginTurn = this->DemoRecInfo.wBeginTurnNo; //preserve when demo recording began
	}

	//If this is the first time this room has been entered, make sure it will
	//remain flagged this way after reloading the room below.
	bool bIgnored;
	SavePrep(bIgnored);

	ASSERT(this->pRoom);
	this->pRoom->Reload();

	//Move the player back to the beginning of the room.
	CueEvents.Clear();
	SetPlayerToRoomStart();
	SetMembersAfterRoomLoad(CueEvents, true, false);

	//If player was recording a demo from the beginning of the room,
	//then resume recording from this point.
	if (bRecordingDemo && !wBeginTurn)
		this->bIsDemoRecording = true;
}

//*****************************************************************************
void CCurrentGame::RestartRoomFromLastCheckpoint(
//Restart the current room from the previous checkpoint
//touched in this room.  If no checkpoints have been touched in this room, then
//the player will restart from the beginning of the room.
//
//Params:
	CCueEvents &CueEvents)  //(out)  Cue events generated by player's first step
							//    into the room.
{
	//Ignore any checkpoints that were activated on the current turn.
	while (!this->checkpointTurns.empty() &&
			this->checkpointTurns.back() >= this->wTurnNo)
		this->checkpointTurns.pop_back();

	//Don't need to find checkpoint if none were used before this turn.
	if (this->checkpointTurns.empty())
	{
		RestartRoom(CueEvents);
		return;
	}
	const UINT wLastCheckpointSave = this->checkpointTurns.back();
	if (!wLastCheckpointSave) //going back to turn 0
	{
		RestartRoom(CueEvents);
		return;
	}

	//If in the middle of recording a demo, stop recording.
	UINT wBeginTurn = 0;
	const bool bRecordingDemo = IsDemoRecording();
	if (bRecordingDemo)
	{
		this->bIsDemoRecording = false;
		wBeginTurn = this->DemoRecInfo.wBeginTurnNo; //preserve when demo recording began
	}

	//If this is the first time this room has been entered, make sure it will
	//remain flagged this way after reloading the room below.
	bool bIgnored;
	SavePrep(bIgnored);

	//Rewind game commands to turn of last checkpoint save.
	SetTurn(wLastCheckpointSave, CueEvents);
	this->Commands.Truncate(wLastCheckpointSave);
	ASSERT(this->pRoom->checkpoints.has(this->wLastCheckpointX, this->wLastCheckpointY));

	//If player was recording a demo, starting from before this turn,
	//then resume recording from this point.
	if (bRecordingDemo && wBeginTurn <= wLastCheckpointSave)
	{
		this->DemoRecInfo.wBeginTurnNo = wBeginTurn;
		this->bIsDemoRecording = true;
	}
}

//*****************************************************************************
void CCurrentGame::RestartRoomFromLastDifferentCheckpoint(
//Restart the current room from the previous checkpoint
//   touched in this room.
//If player is on a checkpoint, go to the more recent different one touched.
//If no checkpoints have been touched in this room, then
//   the player will restart from the beginning of the room.
//
//Params:
	CCueEvents &CueEvents)  //(out)  Cue events generated by swordsman's first step
							//    into the room.
{
	const UINT wCheckpointX = this->wLastCheckpointX, wCheckpointY = this->wLastCheckpointY;
	RestartRoomFromLastCheckpoint(CueEvents);
	//Rewind until a different checkpoint than this one was stepped on.
	while (this->wTurnNo && (wCheckpointX == this->wLastCheckpointX &&
			wCheckpointY == this->wLastCheckpointY))
		RestartRoomFromLastCheckpoint(CueEvents);
}

//*****************************************************************************
void CCurrentGame::PostSave(const bool bConqueredOnEntrance, const bool bExploredOnEntrance)
//Call this following a call to SavePrep and recording a save game record
//to restore game flags the way they were before saving.
{
	if (this->bIsNewRoom)
	{
		if (bExploredOnEntrance)
			SetCurrentRoomExplored();
		if (bConqueredOnEntrance)
			SetCurrentRoomConquered();
	}
}

//*****************************************************************************
bool CCurrentGame::SavePrep(bool& bExploredOnEntrance) //(out)
//If this is the first time the current room has been entered,
//unflag explored+conquered before saving to ensure the room will
//be re-flagged the way it is now when loading the saved game.
//
//Call PostSave after saving to undo these temporary changes.
//
//Returns: whether new room was conquered on entrance
{
	bool bConqueredOnEntrance = false;
	bExploredOnEntrance = false;
	if (this->bIsNewRoom)
	{
		if (IsCurrentRoomExplored())
		{
			bExploredOnEntrance = true;
			this->ExploredRooms -= this->pRoom->dwRoomID;
		}
		bConqueredOnEntrance = IsCurrentRoomConquered();
		if (bConqueredOnEntrance)
			this->ConqueredRooms -= this->pRoom->dwRoomID;
	}
	return bConqueredOnEntrance;
}

//*****************************************************************************
void CCurrentGame::SaveToCheckpoint()
//Saves the current game to the checkpoint slot for this room/checkpoint.
{
	//It is not valid to save the current game when it is inactive.
	ASSERT(this->bIsGameActive);

	//There should be a checkpoint at the indicated location.
	ASSERT(this->pRoom->checkpoints.has(this->wLastCheckpointX, this->wLastCheckpointY));

	if (this->bNoSaves)
		return;

	bool bExploredOnEntrance;
	const bool bConqueredOnEntrance = SavePrep(bExploredOnEntrance);
	CDbPackedVars _stats = this->stats; //must retain what state game was in on entrance
	this->stats = this->statsAtRoomStart;

	this->eType = ST_Checkpoint;
	this->wCheckpointX = this->wLastCheckpointX;
	this->wCheckpointY = this->wLastCheckpointY;
	this->wVersionNo = VERSION_NUMBER;
	this->bIsHidden = false;

	//Is there already a saved game for this room checkpoint?
	const UINT dwExistingSavedGameID = g_pTheDB->SavedGames.FindByCheckpoint(
			this->pRoom->dwRoomID, this->wLastCheckpointX, this->wLastCheckpointY);
	this->dwSavedGameID = dwExistingSavedGameID; //0 or existing ID, to be overwritten
	Update();

	this->stats = _stats;
	PostSave(bConqueredOnEntrance, bExploredOnEntrance);
}

//*****************************************************************************
void CCurrentGame::SaveToContinue()
//Save the current player's game to the continue slot for this hold.
{
	//It is not valid to save the current game when it is inactive.
	if (!this->bIsGameActive)
		return;

	if (this->bNoSaves)
		return;

	bool bExploredOnEntrance;
	const bool bConqueredOnEntrance = SavePrep(bExploredOnEntrance);
	CDbPackedVars _stats = this->stats; //must retain what state game was in on entrance
	this->stats = this->statsAtRoomStart;

	//Set saved game ID to current player's continue slot.
	UINT dwContinueID = g_pTheDB->SavedGames.FindByContinue();
	if (!dwContinueID)
		dwContinueID = g_pTheDB->SavedGames.SaveNewContinue(g_pTheDB->GetPlayerID());
	this->wVersionNo = VERSION_NUMBER;
	this->dwSavedGameID = dwContinueID;
	this->eType = ST_Continue;
	this->bIsHidden = true;
	Update();

	//Update player's time stamp.
	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	ASSERT(pPlayer);
	pPlayer->Update();
	delete pPlayer;

	this->stats = _stats;
	PostSave(bConqueredOnEntrance, bExploredOnEntrance);
}

//*****************************************************************************
void CCurrentGame::SaveToEndHold()
//Save the current player's game to the end hold slot for this hold.
//It is overwritten each time the hold is ended.
{
	//It is not valid to save the current game when it is inactive.
	ASSERT(this->bIsGameActive);

	SaveExitedLevelStats();

	if (this->bNoSaves)
		return;

	//Mark completed rooms.
	AddRoomsToPlayerTally();

	//Set saved game ID to current player's end hold slot.
	const UINT dwEndHoldID = g_pTheDB->SavedGames.FindByEndHold(this->pHold->dwHoldID);
	this->dwSavedGameID = dwEndHoldID;
	this->wVersionNo = VERSION_NUMBER;
	this->eType = ST_EndHold;
	this->bIsHidden = true;
	Update();
}

//*****************************************************************************
void CCurrentGame::SaveToHoldMastered()
//Save the current player's game to the "hold mastered" slot for this hold.
//It is overwritten each time this method is called.
{
	//It is not valid to save the current game when it is inactive.
	ASSERT(this->bIsGameActive);

	SaveExitedLevelStats();

	if (this->bNoSaves)
		return;

	//Mark completed rooms.
	AddRoomsToPlayerTally();

	//Set saved game ID to current player's "hold mastered" slot, if any.
	const UINT dwHoldMasteredID = g_pTheDB->SavedGames.FindByHoldMastered(this->pHold->dwHoldID);
	this->dwSavedGameID = dwHoldMasteredID;
	this->wVersionNo = VERSION_NUMBER;
	this->eType = ST_HoldMastered;
	this->bIsHidden = true;
	Update();
}

//*****************************************************************************
void CCurrentGame::SaveToLevelBegin()
//Saves the current game to the level-begin slot for this level.
{
	//It is not valid to save the current game when it is inactive.
	ASSERT(this->bIsGameActive);

	if (this->bNoSaves)
		return;

	//Swordsman should be at beginning of level entry room.
	RoomEntranceAsserts();

	bool bExploredOnEntrance;
	const bool bConqueredOnEntrance = SavePrep(bExploredOnEntrance);
	CDbPackedVars _stats = this->stats; //must retain what state game was in on entrance
	this->stats = this->statsAtRoomStart;

	this->eType = ST_LevelBegin;
	this->wVersionNo = VERSION_NUMBER;
	this->bIsHidden = false;

	//Is there already a saved game for this level?
	const UINT dwExistingSavedGameID = g_pTheDB->SavedGames.FindByLevelBegin(
			this->pRoom->dwLevelID);
	this->dwSavedGameID = dwExistingSavedGameID; //0 or existing ID, to be overwritten
	Update();

	this->stats = _stats;
	PostSave(bConqueredOnEntrance, bExploredOnEntrance);
}

//*****************************************************************************
void CCurrentGame::RoomEntranceAsserts()
{
	ASSERT(this->wTurnNo == 0);
	ASSERT(this->swordsman.wX == this->wStartRoomX);
	ASSERT(this->swordsman.wY == this->wStartRoomY);
	ASSERT(this->swordsman.wO == this->wStartRoomO);
	ASSERT(this->pRoom->dwRoomID == this->dwRoomID);
	ASSERT(this->swordsman.wIdentity == this->wStartRoomAppearance);
	ASSERT(this->swordsman.bWeaponOff == this->bStartRoomSwordOff);
	ASSERT(this->swordsman.wWaterTraversal == this->wStartRoomWaterTraversal);
	ASSERT((UINT)this->swordsman.weaponType == this->wStartRoomWeaponType);
}

//*****************************************************************************
void CCurrentGame::SaveToRoomBegin()
//Saves the current game to the begin-room slot for this room.
{
	//It is not valid to save the current game when it is inactive.
	ASSERT(this->bIsGameActive);

	if (this->bNoSaves)
		return;

	//Swordsman should be at beginning of room.
	RoomEntranceAsserts();

	bool bExploredOnEntrance;
	const bool bConqueredOnEntrance = SavePrep(bExploredOnEntrance);
	CDbPackedVars _stats = this->stats; //must retain what state game was in on entrance
	this->stats = this->statsAtRoomStart;

	this->eType = ST_RoomBegin;
	this->wVersionNo = VERSION_NUMBER;
	this->bIsHidden = false;

	//Is there already a saved game for this room?
	const UINT dwExistingSavedGameID = g_pTheDB->SavedGames.FindByRoomBegin(
			this->pRoom->dwRoomID);
	this->dwSavedGameID = dwExistingSavedGameID; //0 or existing ID, to be overwritten
	Update();

	this->stats = _stats;
	PostSave(bConqueredOnEntrance, bExploredOnEntrance);
}

//*****************************************************************************
void CCurrentGame::SaveToWorldMap()
{
	//It is not valid to save the current game when it is inactive.
	ASSERT(this->bIsGameActive);

	if (this->bNoSaves)
		return;

	this->eType = ST_WorldMap;
	this->wVersionNo = VERSION_NUMBER;
	this->bIsHidden = false;

	//Is there already a saved game for this room?
	const UINT dwExistingSavedGameID = g_pTheDB->SavedGames.FindByHoldWorldMap(this->pHold->dwHoldID, this->worldMapID);
	this->dwSavedGameID = dwExistingSavedGameID; //0 or existing ID, to be overwritten
	Update();
}

//*****************************************************************************
void CCurrentGame::SetAutoSaveOptions(const UINT dwSetAutoSaveOptions)
{
	this->dwAutoSaveOptions = dwSetAutoSaveOptions;

	if (dwSetAutoSaveOptions == ASO_NONE)
		this->bNoSaves = true;
}

//*****************************************************************************
void CCurrentGame::ScriptCompleted(const CCharacter* pCharacter)
//Mark script as pending completed.
//This is finalized when the room is exited.
{
	//Characters added via GenerateEntity don't need to have their ID added to
	//the save file, since they will no longer exist when the room is left.
	if (pCharacter->bNewEntity)
		return;

	if (pCharacter->bGlobal)
	{
		//Global scripts are removed directly from the list of running global scripts
		UINT dwScriptID = pCharacter->dwScriptID;
		UINT dwCharID;
		bool dwCharFound = false;
		ASSERT(dwScriptID);

		//Run through Custom Characters searching for unique Script ID
		for (vector<HoldCharacter*>::iterator character = this->pHold->characters.begin();
				character != this->pHold->characters.end(); ++character)
		{
			HoldCharacter& c = *(*character);
			if (c.dwScriptID == dwScriptID)
			{
				dwCharFound = true;
				dwCharID = c.dwCharID;
				break;
			}
		}
		ASSERT(dwCharFound);
		if (dwCharFound)
		{
			//Remove charID from list of running global scripts
			this->GlobalScriptsRunning -= dwCharID;
		}
	} else
		this->CompletedScriptsPending += pCharacter->dwScriptID;
}

//*****************************************************************************
void CCurrentGame::SetComputationTimePerSnapshot(const UINT dwTime)
//Set the amount of move calculation time to elapse between game state snapshots.
{
	//Practical lower bound on time between snapshots.
	static const UINT dwMinTime = 100; //ms

	this->dwComputationTimePerSnapshot = dwTime;
	if (this->dwComputationTimePerSnapshot < dwMinTime)
		this-> dwComputationTimePerSnapshot = dwMinTime;
}

//*****************************************************************************
void CCurrentGame::SetCurrentRoomConquered()
//Adds the current room to the list of conquered rooms.
{
	CDbSavedGame::ConqueredRooms += this->pRoom->dwRoomID;
}

//*****************************************************************************
void CCurrentGame::SetCurrentRoomExplored()
//Adds the current room to the list of explored rooms.
{
	CDbSavedGame::ExploredRooms += this->pRoom->dwRoomID;
}

//*****************************************************************************
bool CCurrentGame::SetDyingEntity(
//Sets which entities are involved in a game over so graphics routines know who to focus on.
//
//Returns: Whether this was the first death of the turn.
//
//Params:
	const CEntity* pDyingEntity,   //(in) Critical Entity whose death caused a game over
	const CEntity* pKillingEntity) //(in) Optional killing Entity who caused death [default=NULL]
{
	//Ignore "kills" on Turn 0
	//Anything that causes them will cause the CueEvents to be ignored anyhow
	if (!this->wTurnNo)
		return false;

	//Only the first death of the turn counts
	if (this->GetDyingEntity() != NULL)
		return false;

	this->pDyingEntity = pDyingEntity;
	this->pKillingEntity = pKillingEntity;
	return true;
}

//*****************************************************************************
void CCurrentGame::TeleportPlayer(
	//Attempt to teleport player to a new square and init all changes
	//
	//Params:
	const UINT wSetX, const UINT wSetY,  //(in)   Coords of new square.
	CCueEvents& CueEvents)
{
	if (!this->pRoom->IsValidColRow(wSetX, wSetY))
	{
		return;
	}

	bool bMonsterObstacle;
	this->pRoom->DoesSquareContainPlayerObstacle(wSetX, wSetY, NO_ORIENTATION, bMonsterObstacle);
	if (bMonsterObstacle || this->pRoom->DoesSquareContainTeleportationObstacle(wSetX, wSetY, this->swordsman.wIdentity)){
		return;
	}

	if (this->pRoom->IsMonsterSwordAt(wSetX, wSetY, true)){
		return;
	}

	// Teleporting to the tile the player is already standing at should do nothing
	if (this->swordsman.wX == wSetX && this->swordsman.wY == wSetY)
	{
		this->swordsman.wPrevX = wSetX;
		this->swordsman.wPrevY = wSetY;
		return;
	}

	// Dying player shouldn't be disturbed from their resting place
	if (this->IsPlayerDying() || this->pDyingEntity == &(this->swordsman))
	{
		return;
	}

	ResetPendingTemporalSplit(CueEvents);

	const UINT dX = wSetX - this->swordsman.wX;
	const UINT dY = wSetY - this->swordsman.wY;

	const UINT wOTileNo = this->pRoom->GetOSquare(this->swordsman.wX, this->swordsman.wY);
	const UINT wFTileNo = this->pRoom->GetFSquare(this->swordsman.wX, this->swordsman.wY);
	const UINT wTTileNo = this->pRoom->GetTSquare(this->swordsman.wX, this->swordsman.wY);
	
	this->swordsman.wSwordMovement = NO_ORIENTATION;
	
	const bool bWeaponWasSheathed = !this->swordsman.HasWeapon();
	const bool bWasOnSameScroll = wTTileNo==T_SCROLL;
	
	SetPlayer(wSetX, wSetY);
	
	pRoom->ExplodeStabbedPowderKegs(CueEvents);
	
	SetPlayerWeaponSheathedState();
	
	//Check for stepping on monster
	CMonster* pMonster = this->pRoom->GetMonsterAtSquare(this->swordsman.wX, this->swordsman.wY);
	if (pMonster)
	{
		if (pMonster->wType == M_FLUFFBABY)
		{
			this->pRoom->KillMonster(pMonster,CueEvents,false,&this->swordsman);
			this->pRoom->ProcessPuffAttack(CueEvents, this->swordsman.wX, this->swordsman.wY);
		}
		else if (pMonster->wType == M_FEGUNDOASHES ||  //fegundo ashes
			this->swordsman.CanStepOnMonsters() ||  //player in monster-role attacked another monster
			this->swordsman.CanDaggerStep(pMonster, true))  //player stabbed with a dagger
		{
			CueEvents.Add(CID_MonsterDiedFromStab, pMonster);
			this->pRoom->KillMonster(pMonster, CueEvents, false, &this->swordsman);
			CheckTallyKill(pMonster);
			pMonster->wO = this->swordsman.wO; //splatter in this direction
			SetPlayerAsTarget();  //monsters will attack the player now
		}
	}
	
	ProcessPlayerMoveInteraction(0, 0, CueEvents, bWasOnSameScroll, true, true);

	this->pRoom->CheckForFallingAt(wSetX, wSetY, CueEvents);

	if (this->pDyingEntity == NULL)
	{
		QueryCheckpoint(CueEvents, wSetX, wSetY);
	}

	this->swordsman.bHasTeleported = true;
}

//*****************************************************************************
void CCurrentGame::SetPlayer(
//Move player to new square.
//
//Params:
	const UINT wSetX, const UINT wSetY) //(in)   Coords of new square.
{
	ASSERT(this->pRoom->IsValidColRow(wSetX, wSetY));

	const bool moved = this->swordsman.Move(wSetX,wSetY);

	//Reset PathMaps' target.
	if (moved && this->swordsman.IsTarget())
		this->pRoom->SetPathMapsTarget(this->swordsman.wX, this->swordsman.wY);
}

//*****************************************************************************
bool CCurrentGame::IsSupportedPlayerRole(const UINT wType)
//Returns: whether this monster type is supported as a player role
{
	switch (wType)
	{
		case M_BRAIN:
		case M_SKIPPERNEST:
			return false; //non-moveable monster types are not supported
		case M_SERPENT:
		case M_SERPENTB:
		case M_SERPENTG:
		case M_ROCKGIANT:
		case M_GENTRYII:
			return false; //large monster types are not supported
		default:
			return true;
	}
}

//*****************************************************************************
void CCurrentGame::SetPlayerAsTarget()
{
	const bool bOldTarget = this->swordsman.IsTarget();
	this->swordsman.bIsTarget = true; //standard non-player roles become targets
	this->swordsman.wStealth = Stealth_Default; //script-based stealth is turned off for all roles
	if (bOldTarget != this->swordsman.IsTarget())
		this->pRoom->CreatePathMaps(); //update pathmaps
}

//*****************************************************************************
void CCurrentGame::SetPlayerRole(const UINT wType, CCueEvents& CueEvents)
//Sets the player's appearance to indicated monster/character type.
{
	if (!IsSupportedPlayerRole(wType))
		return;

	UINT wOldIdentity = this->swordsman.wIdentity;

	if (wType >= CUSTOM_CHARACTER_FIRST && wType != M_NONE)
	{
		//When logical role is a custom value, use its designated appearance
		//for behavioral semantics.
		ASSERT(this->pHold);
		HoldCharacter *pChar = this->pHold->GetCharacter(wType);
		if (pChar)
		{
			if (!IsSupportedPlayerRole(pChar->wType))
				return;
			this->swordsman.wIdentity = wType;
			this->swordsman.wAppearance = pChar->wType;
		}
		else
			//This is a dangling custom character reference.
			//Ignore it and use the default role (Beethro).
			this->swordsman.wAppearance = this->swordsman.wIdentity = M_BEETHRO;
	} else
		this->swordsman.wAppearance = this->swordsman.wIdentity = wType;

	//When changing the player role, then all clones in the room must be synched.
	SynchClonesWithPlayer(CueEvents);

	//When player's role changes, cancel temporal recordings.
	if (wOldIdentity != this->swordsman.wIdentity) {
		ResetPendingTemporalSplit(CueEvents);
	}

	//When player's role changes, brain pathmap needs to be updated.
	if (this->swordsman.IsTarget() && this->pRoom &&
			this->pRoom->pCurrentGame) //wait until the room and game have been connected
		this->pRoom->SetPathMapsTarget(this->swordsman.wX, this->swordsman.wY);
}

//*****************************************************************************
void CCurrentGame::SetCloneWeaponsSheathed()
//Set all clones in the room to have the same sword-wielding properties as the player.
{
	if (!this->pRoom)
		return;

	for (CMonster *pMonster = this->pRoom->pFirstMonster; pMonster != NULL;
			pMonster = pMonster->pNext)
	{
		switch (pMonster->wType)
		{
			case M_CLONE:
			case M_TEMPORALCLONE:
			{
				CPlayerDouble *pDouble = DYN_CAST(CPlayerDouble*, CMonster*, pMonster);
				pDouble->SetWeaponSheathed();
			}
			break;
			default: break;
		}
	}
}

//*****************************************************************************
bool CCurrentGame::SetPlayerWeaponSheathedState()
//Sets and returns whether player's weapon is sheathed.
//Currently, this is based on whether player is standing on oremites or in shallow water.
{
	ASSERT(this->pRoom);

	this->swordsman.bIsHiding = IsPlayerWading()
		? this->swordsman.GetWaterTraversalState() == WTrv_CanHide
		: false;

	const UINT wOSquare = this->pRoom->GetOSquare(this->swordsman.wX, this->swordsman.wY);
	this->swordsman.bWeaponSheathed = wOSquare == T_GOO &&
		this->swordsman.HasMetalWeapon();
	return this->swordsman.bIsHiding || this->swordsman.bWeaponSheathed;
}

bool CCurrentGame::IsPlayerWading() const
{
	ASSERT(this->pRoom);

	const UINT wOSquare = this->pRoom->GetOSquare(this->swordsman.wX, this->swordsman.wY);
	return wOSquare == T_SHALLOW_WATER;
}

//*****************************************************************************
bool CCurrentGame::SetPlayerToEastExit()
//Move player to an exit along east column.
{
	//Look for a clear square along the east.  Search outwards from the row
	//player is currently at.
	for (UINT wRowOffset = 0; ; ++wRowOffset)
	{
		bool bNorthInBounds = (this->swordsman.wY - wRowOffset < this->pRoom->wRoomRows);
		bool bSouthInBounds = (this->swordsman.wY + wRowOffset < this->pRoom->wRoomRows);
		if (!bNorthInBounds && !bSouthInBounds)
			return false; //No exit found.

		//Check for exit at north offset.
		if (bNorthInBounds)
		{
			if (this->pRoom->CanSetSwordsman(
				this->pRoom->wRoomCols - 1, this->swordsman.wY - wRowOffset))
			{
				SetPlayer(this->pRoom->wRoomCols - 1, this->swordsman.wY - wRowOffset);
				return true;
			}
		}

		//Check for exit at south offset.
		if (bSouthInBounds)
		{
			if (this->pRoom->CanSetSwordsman(
				this->pRoom->wRoomCols - 1, this->swordsman.wY + wRowOffset))
			{
				SetPlayer(this->pRoom->wRoomCols - 1, this->swordsman.wY + wRowOffset);
				return true;
			}
		}
	}
}

//*****************************************************************************
bool CCurrentGame::SetPlayerToNorthExit()
//Move player to an exit along north row.
{
	//Look for a clear square along the north.  Search outwards from the column
	//player is currently at.
	for (UINT wColOffset = 0; ; ++wColOffset)
	{
		bool bWestInBounds = (this->swordsman.wX - wColOffset < this->pRoom->wRoomCols);
		bool bEastInBounds = (this->swordsman.wX + wColOffset < this->pRoom->wRoomCols);
		if (!bWestInBounds && !bEastInBounds)
			return false; //No exit found.

		//Check for exit at west offset.
		if (bWestInBounds)
		{
			if (this->pRoom->CanSetSwordsman(
				this->swordsman.wX - wColOffset, 0))
			{
				SetPlayer(this->swordsman.wX - wColOffset, 0);
				return true;
			}
		}

		//Check for exit at east offset.
		if (bEastInBounds)
		{
			if (this->pRoom->CanSetSwordsman(
				this->swordsman.wX + wColOffset, 0))
			{
				SetPlayer(this->swordsman.wX + wColOffset, 0);
				return true;
			}
		}
	}
}

//*****************************************************************************
bool CCurrentGame::SetPlayerToSouthExit()
//Move player to an exit along south row.
{
	//Look for a clear square along the south.  Search outwards from the column
	//player is currently at.
	for (UINT wColOffset = 0; ; ++wColOffset)
	{
		bool bWestInBounds = (this->swordsman.wX - wColOffset < this->pRoom->wRoomCols);
		bool bEastInBounds = (this->swordsman.wX + wColOffset < this->pRoom->wRoomCols);
		if (!bWestInBounds && !bEastInBounds)
			return false; //No exit found.

		//Check for exit at west offset.
		if (bWestInBounds)
		{
			if (this->pRoom->CanSetSwordsman(
				this->swordsman.wX - wColOffset, this->pRoom->wRoomRows - 1))
			{
				SetPlayer(this->swordsman.wX - wColOffset, this->pRoom->wRoomRows - 1);
				return true;
			}
		}

		//Check for exit at east offset.
		if (bEastInBounds)
		{
			if (this->pRoom->CanSetSwordsman(
				this->swordsman.wX + wColOffset, this->pRoom->wRoomRows - 1))
			{
				SetPlayer(this->swordsman.wX + wColOffset, this->pRoom->wRoomRows - 1);
				return true;
			}
		}
	}
}

//*****************************************************************************
bool CCurrentGame::SetPlayerToWestExit()
//Move player to an exit along west column.
{
	//Look for a clear square along the west.  Search outwards from the row
	//player is currently at.
	for (UINT wRowOffset = 0; ; ++wRowOffset)
	{
		bool bNorthInBounds = (this->swordsman.wY - wRowOffset < this->pRoom->wRoomRows);
		bool bSouthInBounds = (this->swordsman.wY + wRowOffset < this->pRoom->wRoomRows);
		if (!bNorthInBounds && !bSouthInBounds)
			return false; //No exit found.

		//Check for exit at north offset.
		if (bNorthInBounds)
		{
			if (this->pRoom->CanSetSwordsman(0,
					this->swordsman.wY - wRowOffset))
			{
				SetPlayer(0, this->swordsman.wY - wRowOffset);
				return true;
			}
		}

		//Check for exit at south offset.
		if (bSouthInBounds)
		{
			if (this->pRoom->CanSetSwordsman(0,
					this->swordsman.wY + wRowOffset))
			{
				SetPlayer(0, this->swordsman.wY + wRowOffset);
				return true;
			}
		}
	}
}

//*****************************************************************************
void CCurrentGame::SetRoomStatusFromAllSavedGames()
//Set explored status of rooms to include those from all saved games for the
//current player in the current level.
{
	ASSERT(CDbBase::IsOpen());

	//Get a list of all the rooms in the current level.
	c4_View SavedGamesView;
	CIDSet savedGamesInLevel = CDb::getSavedGamesInLevel(this->pLevel->dwLevelID);

	const UINT dwCurrentPlayerID = g_pTheDB->GetPlayerID();

	//Each iteration looks at one saved game for the current level and adds
	//any unique explored or conquered rooms to object members.
	for (CIDSet::const_iterator savedGame = savedGamesInLevel.begin();
			savedGame != savedGamesInLevel.end(); ++savedGame)
	{
		const UINT savedGameRowI = g_pTheDB->LookupRowByPrimaryKey(*savedGame, V_SavedGames, SavedGamesView);
		c4_RowRef row = SavedGamesView[savedGameRowI];
		if (p_IsHidden(row) != 0)
			continue;
		if (static_cast<UINT>(p_PlayerID(row)) != dwCurrentPlayerID)
			continue;

		//Add any new explored rooms from this saved game.
		c4_View ExploredRoomsView = p_ExploredRooms(row);
		const UINT dwRoomCount = ExploredRoomsView.GetSize();
		for (UINT dwI = 0; dwI < dwRoomCount; ++dwI)
			this->ExploredRooms += (UINT) p_RoomID(ExploredRoomsView[dwI]);

		//Add the saved game's current room.
		this->ExploredRooms += p_RoomID(row);
	}
}

//*****************************************************************************
void CCurrentGame::SetTurn(
//Sets current game to a specific turn, without removing commands.
//
//Params:
	UINT wTurnNo,        //(in)   Turn to which game will be set.
	CCueEvents &CueEvents)  //(out)  Cue events generated by the last command.
{
	ASSERT(this->pRoom);

	// It's possible player made moves against room edge that couldn't lead anywhere - disable
	// room exit lock to avoid those moves from breaking playback
	const bool bOldIsRoomLocked = this->bRoomExitLocked;
	this->bRoomExitLocked = false;

	if (wTurnNo > this->Commands.Count())	//bounds check
		wTurnNo = this->Commands.Count();

	//If this is the first time a new room has been entered, make sure it will
	//also be marked this way when reloading this saved game.
	if (this->bIsNewRoom)
		this->ExploredRooms -= this->pRoom->dwRoomID;

	//If game state snapshots have been maintained during play,
	//find the latest one before the turn number being replayed to.
	//(We don't want to restore to the exact turn a snapshot was taken,
	//because without replaying the final turn, we won't have any cue events
	//to display the events of this turn in the front end.)
	CCurrentGame *pSnapshot;
	for (pSnapshot = this->pSnapshotGame; pSnapshot != NULL;
			pSnapshot = pSnapshot->pSnapshotGame)
	{
		if (pSnapshot->wTurnNo < wTurnNo)
			break; //this is the closest one before the turn to replay to
	}
	if (pSnapshot)
	{
		//Delete snapshots taken after this snapshot.
		CCurrentGame *pLaterSnapshot = this->pSnapshotGame;
		while (pLaterSnapshot != pSnapshot)
		{
			CCurrentGame *pDelete = pLaterSnapshot;
			pLaterSnapshot = pLaterSnapshot->pSnapshotGame;
			pDelete->pSnapshotGame = NULL; //don't destroy the earlier snapshots
			delete pDelete;
		}

		//Preserve member vars that shouldn't change on game state reversion.
		const CDbCommands commands(this->Commands);
		const int ats_ = activatingTemporalSplit;
		const UINT d_ = this->dwLevelDeaths, k_ = this->dwLevelKills,
				m_ = this->dwLevelMoves, t_ = this->dwLevelTime,
				st_ = this->dwStartTime;
		const bool rl_ = this->bRoomExitLocked;
		const UINT dwAutoSaveOptions_ = this->dwAutoSaveOptions;

		//Restore game to state of selected snapshot.
		CueEvents.Clear();
		SetMembers(*pSnapshot);

		//Hook in this and earlier snapshots.
		this->pSnapshotGame = pSnapshot;

		//Restore command list.
		this->Commands = commands; //Commands may or may not be truncated by caller.
		this->activatingTemporalSplit = ats_;

		//Replay moves from time of snapshot to target turn number.
		if (this->wTurnNo < wTurnNo)
			VERIFY(PlayCommandsToTurn(wTurnNo, CueEvents));

		//Restore stats that shouldn't change.
		this->dwLevelDeaths = d_;
		this->dwLevelKills = k_;
		this->dwLevelMoves = m_;
		this->dwLevelTime = t_;
		this->dwStartTime = st_;
		this->bRoomExitLocked = rl_;
		this->dwAutoSaveOptions = dwAutoSaveOptions_;

		return;
	}

	//Freeze commands as a precaution--nothing below should change commands.
	const bool freeze = ShouldFreezeCommandsDuringSetTurn();
	if (freeze)
		FreezeCommands();

	this->pRoom->Reload();

	//Move the player back to the beginning of the room.
	CueEvents.Clear();
	SetPlayerToRoomStart();
	SetMembersAfterRoomLoad(CueEvents, false, false);

	if (freeze)
		UnfreezeCommands();

	//Play the commands back.
	if (wTurnNo)
		PlayCommandsToTurn(wTurnNo, CueEvents);

	this->bRoomExitLocked = bOldIsRoomLocked;
}

//*****************************************************************************
bool CCurrentGame::ShouldFreezeCommandsDuringSetTurn() const
{
	if (!this->Commands.IsFrozen())
		return true;

	//Prevent double freezing of Commands by undoing moves as part of
	//a temporal split activation command, while in process of undoing a move
	return !IsActivatingTemporalSplit();
}

//*****************************************************************************
bool CCurrentGame::IsActivatingTemporalSplit() const
{
	return this->activatingTemporalSplit > 0;
}

//*****************************************************************************
void CCurrentGame::SynchClonesWithPlayer(CCueEvents& CueEvents)
//Sets clones in the room to have the same properties as the player.
//Currently, this includes sword-wielding and flying (based on player role) attributes.
{
	if (!this->pRoom)
		return;

	SetCloneWeaponsSheathed();

	//Check for clones depressing pressure plates according to player role.
	if (this->wTurnNo > 0) //...except on room entrance
	{
		CDbRoom& room = *this->pRoom;
		for (CMonster *pMonster = room.pFirstMonster; pMonster != NULL;
				pMonster = pMonster->pNext)
		{
			switch (pMonster->wType)
			{
				case M_CLONE:
				case M_TEMPORALCLONE:
					if (room.GetOSquare(pMonster->wX, pMonster->wY) == T_PRESSPLATE && !pMonster->CanPressPressurePlates())
						room.ActivateOrb(pMonster->wX, pMonster->wY, CueEvents, OAT_PressurePlate);
				break;
				default: break;
			}
		}
	}
}

//*****************************************************************************
bool CCurrentGame::TunnelGetExit(
//Find tunnel exit.
//
//Params:
	const UINT wStartX, const UINT wStartY,	//(in) tunnel entrance
	const int dx, const int dy,	//(in) tunnel direction
	UINT& wX, UINT& wY, //(out) location of tunnel exit
	const CMonster *pMonster) //(in) optional monster using the tunnel -- if NULL, assume the player
const
{
	wX = wStartX;
	wY = wStartY;

	while (true)
	{
		wX += dx;
		wY += dy;

		//If search goes off the room edge, wrap to other side.
		if (!this->pRoom->IsValidColRow(wX,wY))
		{
			if (wX >= this->pRoom->wRoomCols)
			{
				//Wrap horizontally.
				wX = wX == this->pRoom->wRoomCols ? 0 : this->pRoom->wRoomCols-1;
			} else {
				//Wrap vertically.
				wY = wY == this->pRoom->wRoomRows ? 0 : this->pRoom->wRoomRows-1;
			}
		}

		if (wStartX == wX && wStartY == wY)
			return false;   //Found no other tunnel -- exit from same one.

		if (bIsTunnel(this->pRoom->GetOSquare(wX,wY)))
		{
			//Found a different tunnel exit -- place player here if open.
			const UINT tTile = this->pRoom->GetTSquare(wX,wY);
			bool bSwordBlocked = this->pRoom->IsMonsterSwordAt(wX,wY,true,pMonster);
			if (pMonster)
				bSwordBlocked |= IsPlayerWeaponAt(wX,wY,true);
			if (bSwordBlocked || bIsTarOrFluff(tTile) || bIsBriar(tTile) ||
					this->pRoom->GetMonsterAtSquare(wX,wY) != NULL || (this->swordsman.wX == wX && this->swordsman.wY == wY))
			{
				//Tunnel exit is blocked -- can't use tunnel.
				wX = wStartX;
				wY = wStartY;
				return false;
			}
			return true;
		}
	}
}

//*****************************************************************************
void CCurrentGame::UndoCommand(
//Undoes one command.
//
//Params:
	CCueEvents &CueEvents)  //(out)  Cue events generated by the new last command.
{
	if (this->Commands.Empty()) return; //No commands to undo.

	UndoCommands(1, CueEvents);
}

//*****************************************************************************
void CCurrentGame::UndoCommands(
//Undos one or more commands by restarting current room and replaying recorded moves to
//reach the current turn minus a specified number of undone commands.
//
//Params:
	const UINT wUndoCount,  //(in)   Number of commands to undo.
	CCueEvents &CueEvents)  //(out)  Cue events generated by the new last command.
{
	ASSERT(this->pRoom);
	ASSERT(wUndoCount > 0 && wUndoCount <= this->Commands.Count());
	UINT wPlayCount = this->Commands.Count() - wUndoCount;

	//If an undo is performed while a demo is being recorded,
	//keep recording the demo and remove the previous command.
	const bool bDemoRecording = IsDemoRecording();
	UINT wBeginTurnNo = 0;
	if (bDemoRecording)
	{
		wBeginTurnNo = this->DemoRecInfo.wBeginTurnNo;	//room reload clears this value
		this->bIsDemoRecording = false;
	}

	//If this is the first time this room has been entered, make sure it will
	//remain flagged this way after reloading the room below.
	bool bIgnored;
	SavePrep(bIgnored);

	//Play the commands back, minus undo count.
	SetTurn(wPlayCount, CueEvents);
	if (!IsActivatingTemporalSplit())
		this->Commands.Truncate(wPlayCount);

	//If player was recording a demo, starting from before this turn,
	//then resume recording from this point.
	if (bDemoRecording && wBeginTurnNo <= wPlayCount)
	{
		this->bIsDemoRecording = true;
		this->DemoRecInfo.wBeginTurnNo = wBeginTurnNo;
	}
}

//*****************************************************************************
void CCurrentGame::UnfreezeCommands()
//Allow modification of command list after a call to FreezeCommands().
{
	//Commands should not have been frozen while recording.  Recording requires
	//commands to be added for each call to ProcessCommand().  It is possible to have
	//some carefully thought-out mixture of the two states, in which case this assertion
	//can be changed.
	ASSERT(!this->bIsDemoRecording);

	this->Commands.Unfreeze();
}

//*****************************************************************************
void CCurrentGame::UpdatePrevCoords()
//Update previous coords for player and monsters to current positions.
{
	this->swordsman.wPrevX = this->swordsman.wX;
	this->swordsman.wPrevY = this->swordsman.wY;
	this->swordsman.wPrevO = this->swordsman.wO;
	this->swordsman.wSwordMovement = NO_ORIENTATION;

	for (CMonster *pMonster = this->pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		CMonster& m = *pMonster;
		m.wPrevX = m.wX;
		m.wPrevY = m.wY;
		m.wPrevO = m.wO;

		if (m.wType == M_CHARACTER || bEntityHasSword(m.wType))
		{
			CArmedMonster *pArmedMonster = DYN_CAST(CArmedMonster*, CMonster*, pMonster);
			pArmedMonster->wSwordMovement = NO_ORIENTATION;
		}
	}

	UpdatePrevPlatformCoords();
	UpdatePrevTLayerCoords();
}

//*****************************************************************************
void CCurrentGame::UpdatePrevPlatformCoords()
{
	for (vector<CPlatform*>::iterator platformIter = this->pRoom->platforms.begin();
			platformIter != this->pRoom->platforms.end(); ++platformIter)
	{
		CPlatform& platform = *(*platformIter);
		platform.xDelta = platform.yDelta = 0;
	}
}

//*****************************************************************************
void CCurrentGame::UpdatePrevTLayerCoords()
{
	for (list<RoomObject*>::iterator it=this->pRoom->tLayerObjects.begin();
			it!=this->pRoom->tLayerObjects.end(); ++it)
	{
		(*it)->syncPrevPosition();
	}
}

//*****************************************************************************
UINT CCurrentGame::UpdateTime(const UINT dwTime)  //[default=0]
//Keeps track of real time played.  Increments total each time called.
//Accurate to the millisecond, for up to a year and a half total duration.
//
//Returns: how much time has elapsed since last query
{
	if (!dwTime)
	{
		//Stop timing.  Don't add anything to elapsed time.
		this->dwStartTime = 0;
		return 0;
	}

	if (!this->dwStartTime)
	{
		//Start timing.
		this->dwStartTime = dwTime;
		return 0;
	}

	//Add to time elapsed.
	const UINT inc = dwTime - this->dwStartTime;
	this->dwStartTime = dwTime;
	this->dwLevelTime += inc;
	return inc;
}

//*****************************************************************************
bool CCurrentGame::WalkDownStairs()
//Move swordsman one step down/up stairs.
//
//Returns: whether the end of the stairs have been reached.
{
	//Decide whether to walk up or down the stairs.
	const UINT wOSquare = pRoom->GetOSquare(this->swordsman.wX, this->swordsman.wY);
	if (!bIsStairs(wOSquare)) return false; //not on stairs at all
	const int yOffset = wOSquare == T_STAIRS ? 1 : -1;
	if (this->pRoom->IsValidColRow(this->swordsman.wX, this->swordsman.wY + yOffset))
	{
		if (pRoom->GetOSquare(this->swordsman.wX, this->swordsman.wY + yOffset) == wOSquare)
		{
			++this->wTurnNo;  //to animate swordsman walking down each step in front end
			SetPlayer(this->swordsman.wX, this->swordsman.wY + yOffset);
			if (!this->swordsman.HasWeapon()) //swordless, the player faces the direction of movement
				this->swordsman.SetOrientation(nGetO(0, yOffset));
			return true;
		}
	}

	return false;
}

//***************************************************************************************
UINT CCurrentGame::WriteCurrentRoomDieDemo()
//Writes a demo to show player dieing in this room.
//
//Returns:
//DemoID of new Demos record.
{
	DEMO_REC_INFO dri;
	dri.dwDescriptionMessageID = MID_DieDemoDescription;
	dri.SetFlag(CDbDemo::Death);

	return WriteCurrentRoomDemo(dri);
}

//
//Private methods.
//

//***************************************************************************************
void CCurrentGame::AddCompletedScripts()
//Record that scripts were completed.
{
	//Check for scripts that completed but were not marked yet.
	//This could happen for scripts that execute and complete on room entrance,
	//and then the player immediately leaves the room without making a move.
	CMonster *pMonster = this->pRoom->pFirstMonster;
	while (pMonster)
	{
		if (M_CHARACTER == pMonster->wType)
		{
			CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
			if (pCharacter->bScriptDone)
				ScriptCompleted(pCharacter);
		}
		pMonster = pMonster->pNext;
	}

	this->CompletedScripts += this->CompletedScriptsPending;
	this->CompletedScriptsPending.clear();
	//Update saved global scripts status
	this->GlobalScripts = this->GlobalScriptsRunning;
}

//***************************************************************************************
void CCurrentGame::AddRoomsToPlayerTally()
//Mark rooms that have been explored and conquered to player's total tally.
{
	if (this->bNoSaves)
		return;

	if (GetAutoSaveOptions() <= ASO_CHECKPOINT) //implies playtesting or no saving
		return; //don't save the following record

	const UINT dwPlayerID = g_pTheDB->GetPlayerID();
	ASSERT(dwPlayerID);
	g_pTheDB->SavedGames.AddRoomsToPlayerTally(dwPlayerID, this->ConqueredRooms, this->ExploredRooms);
}

//***************************************************************************************
void CCurrentGame::AddQuestionsToList(
//Adds questions from CID_MonsterSpoke event to a list of questions.
//
//Params:
	CCueEvents &CueEvents,              //(in)   May contain CID_MonsterSpoke and
											//    associated questions.
	list<CMonsterMessage> &QuestionList)   //(out)  Questions will be added to it.
const
{
	const CAttachableObject *pObj = CueEvents.GetFirstPrivateData(CID_MonsterSpoke);
	while (pObj)
	{
		const CMonsterMessage *pMessage = DYN_CAST(const CMonsterMessage*, const CAttachableObject*, pObj);
		if (pMessage->eType == MMT_YESNO || pMessage->eType == MMT_MENU)
			QuestionList.push_back(*pMessage);
		pObj = CueEvents.GetNextPrivateData();
	}
}

//*****************************************************************************
void CCurrentGame::AmbientSoundTracking(CCueEvents &CueEvents)
//Keep track of which persistent (looping) ambient sounds are playing on this turn.
//Also maintains a log of speech events.
{
	const CAttachableObject* pObj = CueEvents.GetFirstPrivateData(CID_AmbientSound);
	while (pObj)
	{
		const CMoveCoordEx *pCoord = DYN_CAST(const CMoveCoordEx*, const CAttachableObject*, pObj);
		//wX/Y: sound location, wO: DataID (0 = stop all), wValue: loop if set
		if (!pCoord->wO)
		{
			//Stop ambient sounds.
			if (!this->pRoom->IsValidColRow(pCoord->wX, pCoord->wY))
				this->ambientSounds.clear(); //stop all ambient sounds
			else {
				//Remove sounds at indicated coordinate.
				vector<CMoveCoordEx> continuing;
				for (vector<CMoveCoordEx>::const_iterator ac = this->ambientSounds.begin();
						ac != this->ambientSounds.end(); ++ac)
				{
					if (ac->wX != pCoord->wX || ac->wY != pCoord->wY)
						continuing.push_back(*ac); //this sound is still playing
				}
				this->ambientSounds = continuing; //keep only sounds still playing
			}
		}
		else if (pCoord->wValue)
			this->ambientSounds.push_back(*pCoord);
		pObj = CueEvents.GetNextPrivateData();
	}

	//Speech log.
	pObj = CueEvents.GetFirstPrivateData(CID_Speech);
	while (pObj)
	{
		static const UINT MAX_SPEECH_HISTORY = 100;
		const CFiredCharacterCommand *pCmd = DYN_CAST(const CFiredCharacterCommand*,
				const CAttachableObject*, pObj);
		if (!pCmd->bFlush) //Ignore flush commands.
		{
			if (this->roomSpeech.size() >= MAX_SPEECH_HISTORY)
			{
				//Pop first item from speech sequence.
				delete this->roomSpeech[0].pSpeechCommand;
				const UINT sizeMinusOne = this->roomSpeech.size()-1;
				for (UINT i=0; i<sizeMinusOne; ++i)
					this->roomSpeech[i] = this->roomSpeech[i+1];
				this->roomSpeech.pop_back();
			}

			CCharacterCommand *pCommand = new CCharacterCommand(*(pCmd->pCommand));
			ASSERT(pCommand->pSpeech);
			pCommand->pSpeech->MessageText = pCmd->text.c_str(); //get interpolated text
			UINT& characterType = pCommand->pSpeech->wCharacter;
			WSTRING customName = DefaultCustomCharacterName;

			if (characterType == Speaker_Self)
			{
				//Resolve now because there won't be any hook to the executing NPC later.
				CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pCmd->pExecutingNPC);
				characterType = pCharacter->wLogicalIdentity;

				//Convert to the speaker type.
				if (characterType < CUSTOM_CHARACTER_FIRST)
					characterType = getSpeakerType(MONSTERTYPE(characterType));

				customName = pCharacter->GetCustomName();
			}
			else if (characterType == Speaker_Player)
			{
				//Make sure the player's current role is attached to the speech log
				const UINT wPlayerIdentity = this->swordsman.wIdentity;
				if (wPlayerIdentity >= CUSTOM_CHARACTER_FIRST)
					characterType = this->swordsman.wIdentity;
				else
					characterType = getSpeakerType(MONSTERTYPE(wPlayerIdentity));
			}
			else if (characterType == Speaker_Custom)
			{
				ASSERT(this->pRoom->IsValidColRow(pCommand->x, pCommand->y));
				CMonster *pMonster = this->pRoom->GetMonsterAtSquare(
					pCommand->x, pCommand->y);
				if (pMonster)
				{
					if (pMonster->wType == M_CHARACTER) {
						const CCharacter* pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);

						customName = pCharacter->GetCustomName();
					}
					
					characterType = getSpeakerType(MONSTERTYPE(pMonster->GetResolvedIdentity()));
				}
			}

			this->roomSpeech.push_back(SpeechLog(customName, pCommand));
		}
		pObj = CueEvents.GetNextPrivateData();
	}
}

//*****************************************************************************
void CCurrentGame::BlowHorn(CCueEvents &CueEvents, const UINT wSummonType,
							const UINT wHornX, const UINT wHornY)
//Tooting ones horn.
//Currently expects only PlayerDoubles (Clones and Stalwart2) to be placed.
{
	UINT wX, wY;
	this->pRoom->Plot(wHornX, wHornY, T_EMPTY);

	ResetPendingTemporalSplit(CueEvents);

	if (!GetNearestEntranceForHorn(wHornX, wHornY, wSummonType, wX, wY))
	{
		CueEvents.Add(CID_HornFail);
	} else {
		CueEvents.Add(wSummonType == M_CLONE ? CID_Horn_Squad : CID_Horn_Soldier);

		CPlayerDouble *pDouble = DYN_CAST(CPlayerDouble*, CMonster*, this->pRoom->AddNewMonster(wSummonType, wX, wY));
		pDouble->wO = this->swordsman.wO;
		pDouble->bIsFirstTurn = true;

		UINT wNewOSquare = this->pRoom->GetOSquare(pDouble->wX, pDouble->wY);
		//Activate pressure plate at destination if not flying.
		if (wNewOSquare == T_PRESSPLATE &&
			(pDouble->wType != M_CLONE || bCanEntityPressPressurePlates(this->swordsman.wAppearance)))
			this->pRoom->ActivateOrb(pDouble->wX, pDouble->wY, CueEvents, OAT_PressurePlate);

		if (pDouble->wType == M_CLONE)
		{
			pDouble->SetWeaponSheathed(); //Need to set sheath status before using items
			const bool bSmitemaster = bIsSmitemaster(this->swordsman.wAppearance);
			const bool bCanGetItems = this->swordsman.CanLightFuses();
			UINT wNewTSquare = this->pRoom->GetTSquare(pDouble->wX, pDouble->wY);
			switch(wNewTSquare)
			{
				case T_POTION_K:  //Mimic potion.
					if (bSmitemaster)
						DrankPotion(CueEvents, M_MIMIC, pDouble->wX, pDouble->wY);
				break;

				case T_POTION_D:  //Decoy potion.
					if (bSmitemaster)
						DrankPotion(CueEvents, M_DECOY, pDouble->wX, pDouble->wY);
				break;

				case T_POTION_C:  //Clone potion.
					if (this->swordsman.CanLightFuses())
						DrankPotion(CueEvents, M_CLONE, pDouble->wX, pDouble->wY);
				break;

				case T_POTION_I:  //Invisibility potion.
					if (bCanGetItems)
					{
						this->swordsman.bIsInvisible = !this->swordsman.bIsInvisible;   //Toggle effect.
						this->pRoom->Plot(pDouble->wX, pDouble->wY, T_EMPTY);
						CueEvents.Add(CID_DrankPotion);
					}
				break;

				case T_POTION_SP:  //Speed potion.
					if (bCanGetItems)
					{
						this->swordsman.bIsHasted = !this->swordsman.bIsHasted;  //Toggle effect.
						this->pRoom->Plot(pDouble->wX, pDouble->wY, T_EMPTY);
						CueEvents.Add(CID_DrankPotion);
					}
				break;

				case T_HORN_SQUAD:    //Squad horn.
					if (this->swordsman.CanLightFuses())  //same condition as clone potion..
						BlowHorn(CueEvents, M_CLONE, pDouble->wX, pDouble->wY);
				break;

				case T_HORN_SOLDIER:  //Soldier horn.
					if (bIsMonsterTarget(this->swordsman.wAppearance) || this->swordsman.bCanGetItems)
						BlowHorn(CueEvents, M_STALWART2, pDouble->wX, pDouble->wY);
				break;

				case T_TOKEN:
					this->pRoom->ActivateToken(CueEvents, pDouble->wX, pDouble->wY, pDouble);
					break;

				case T_FUSE:
					//Light the fuse.
					if (this->swordsman.CanLightFuses())
						this->pRoom->LightFuseEnd(CueEvents, pDouble->wX, pDouble->wY);
					break;
			}
		}
		pDouble->SetWeaponSheathed();

		ProcessArmedMonsterWeapon(pDouble, CueEvents);
	}
}

//*****************************************************************************
bool CCurrentGame::CanSwitchToClone() const
{
	if (this->swordsman.wPlacingDoubleType != 0)
		return false;

	if (this->temporalSplit.queuing())
		return false;

	return true;
}

//*****************************************************************************
void CCurrentGame::DeleteLeakyCueEvents(CCueEvents &CueEvents)
//Deletes objects allocated on the heap and connected to cue events
//that must be deleted by a handler, but aren't handled in this instance.
{
	static const UINT eventsRequiringDeletion = 1;
	static const CUEEVENT_ID events[eventsRequiringDeletion] = {CID_Speech};

	const CAttachableObject *pObj;
	for (UINT i=0; i<eventsRequiringDeletion; ++i)
		for (pObj = CueEvents.GetFirstPrivateData(events[i]); pObj != NULL;
				pObj = CueEvents.GetNextPrivateData())
			delete pObj;
}

//*****************************************************************************
void CCurrentGame::DrankPotion(CCueEvents &CueEvents, const UINT wDoubleType,
								const UINT wPotionX, const UINT wPotionY)
//Drink potion, begin double placement and init double cursor position.
{
	this->swordsman.wPlacingDoubleType=wDoubleType;
	this->swordsman.wDoubleCursorX=this->swordsman.wX;
	this->swordsman.wDoubleCursorY=this->swordsman.wY;
	this->pRoom->Plot(wPotionX, wPotionY, T_EMPTY);
	CueEvents.Add(CID_DrankPotion);

	ResetPendingTemporalSplit(CueEvents);
}

//*****************************************************************************
void CCurrentGame::FegundoToAsh(CMonster *pMonster, CCueEvents &CueEvents)
//Fegundo has exploded.  Remove it and replace it with ashes.
{
	const UINT wX = pMonster->wX;
	const UINT wY = pMonster->wY;
	if (pMonster->bAlive) //if a resulting bomb explosion did not also kill the fegundo
	{
		this->pRoom->KillMonster(pMonster, CueEvents);

		//Add ashes if room square doesn't forbid it.
		const UINT wOTile = this->pRoom->GetOSquare(wX,wY);
		switch (wOTile)
		{
			case T_PIT: case T_PIT_IMAGE:
				CueEvents.Add(CID_ObjectFell, new CMoveCoordEx2(wX, wY,
					S, M_OFFSET + M_FEGUNDOASHES, 0), true);
			break;
			case T_WATER: case T_SHALLOW_WATER:
				CueEvents.Add(CID_Splash, new CCoord(wX,wY), true);
			break;
			default:
				CMonster *pAshes = this->pRoom->AddNewMonster(M_FEGUNDOASHES,wX,wY);
				pAshes->bIsFirstTurn = true;
				if (wOTile == T_PRESSPLATE)
					this->pRoom->ActivateOrb(wX, wY, CueEvents, OAT_PressurePlate);
			break;
		}
	}
}

//*****************************************************************************
void CCurrentGame::FlagChallengesCompleted(CCueEvents& CueEvents)
{
	set<WSTRING> challengesCompleted;
	Challenges::GetFrom(&CueEvents, challengesCompleted);
	ASSERT(!challengesCompleted.empty());

	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	ASSERT(pPlayer);
	if (pPlayer->challenges.add(this->pHold->dwHoldID, challengesCompleted))
		pPlayer->Update();
	delete pPlayer;

	//On turn 0, overwrite any preexisting challenge demo.
	WriteCompletedChallengeDemo(!this->wTurnNo ? challengesCompleted : set<WSTRING>());
}

//***************************************************************************************
bool CCurrentGame::IsSwordsmanTired()
//Returns: Whether swordsman has just finished a long job
//and is breathing a sigh of exhausted relief.
{
	static const float KILL_RATIO = 0.70f;
	static const UINT NO_MONSTER_RADIUS = 3;

	if (!bIsBeethroDouble(this->swordsman.wAppearance) &&
			this->swordsman.wAppearance != M_BEETHRO_IN_DISGUISE)
		return false;

	//Criteria:
	//1. At least KILL_RATIO % of TIRED_TURN_COUNT turns was spent
	//   killing monsters.
	if (!this->bLotsOfMonstersKilled)
	{
		if (this->wMonstersKilledRecently < TIRED_TURN_COUNT * KILL_RATIO)
			return false;
		this->bLotsOfMonstersKilled = true;
	}

	//2. At least some monsters have been killed recently.
	if (this->wMonstersKilledRecently == 0)
	{
		//Too much time has elapsed, so (1) doesn't count any more.
		this->bLotsOfMonstersKilled = false;
		return false;
	}

	//3. No monsters are now within NO_MONSTER_RADIUS squares of player.
	const bool bTired = !(this->pRoom->IsMonsterWithin(this->swordsman.wX,this->swordsman.wY,NO_MONSTER_RADIUS));

	if (bTired)
	{
		//Reset all "tired" vars to prevent sound repeating in consecutive turns.
		this->bLotsOfMonstersKilled = false;
		this->wMonstersKilledRecently = 0;
		memset(this->monstersKilled,0,TIRED_TURN_COUNT * sizeof(unsigned char));
		return true;
	}
	return false;
}

//*****************************************************************************
void CCurrentGame::LoadNewRoomForExit(
//Loads room adjacent to the current room in the direction of the swordsman's
//exit of the current room.  Swordsman coords are also updated so that he wraps to
//other side in new room.
//
//Params:
	const UINT dwSX, const UINT dwSY,
	CDbRoom* pNewRoom,
	CCueEvents &CueEvents,  //(out)  Cue events generated by swordsman's first step
	                        //  into the room.
	const bool bSaveGame)   //save game if set
{
	ASSERT(pNewRoom);
	delete this->pRoom;
	this->pRoom = pNewRoom;

	//Put swordsman at designated square.
	SetPlayer(dwSX, dwSY);

	//Set start room members.
	SetRoomStartToPlayer();
	SetPlayerToRoomStart(); //reset some vars so saving game works properly

	//Save game if requested.
	//ATTN: Do this before SetMembersAfterRoomLoad changes anything.
	if (bSaveGame)
	{
		SaveToRoomBegin();

		//Upload explored room data
		if (!IsCurrentRoomExplored())
			UploadExploredRoom();
	}


	SetMembersAfterRoomLoad(CueEvents);
}

//***************************************************************************************
bool CCurrentGame::LoadEastRoom()
//Loads room east of the current room in the context of the swordsman exiting
//from the east end of the room.
//
//Returns:
//True if room was successfully loaded, false if not.
{
	//Swordsman should be at east edge.
	ASSERT(this->swordsman.wX == this->pRoom->wRoomCols - 1);

	//Attempt to load room.
	if (!SetRoomAtCoords(this->pRoom->dwRoomX + 1, this->pRoom->dwRoomY))
		return false;

	//Put player at west edge of new room.
	SetPlayer(0, this->swordsman.wY);

	return true;
}

//***************************************************************************************
bool CCurrentGame::LoadNorthRoom()
//Loads room north of the current room in the context of the swordsman exiting
//from the north end of the room.
//
//Returns:
//True if room was successfully loaded, false if not.
{
	//Swordsman should be at north edge.
	ASSERT(this->swordsman.wY == 0);

	//Attempt to load room.
	if (!SetRoomAtCoords(this->pRoom->dwRoomX, this->pRoom->dwRoomY - 1))
		return false;

	//Put player at south edge of new room.
	SetPlayer(this->swordsman.wX, this->pRoom->wRoomRows - 1);

	return true;
}

//***************************************************************************************
bool CCurrentGame::LoadSouthRoom()
//Loads room south of the current room in the context of the swordsman exiting
//from the south end of the room.
//
//Returns:
//True if room was successfully loaded, false if not.
{
	//Swordsman should be at south edge.
	ASSERT(this->swordsman.wY == this->pRoom->wRoomRows - 1);

	//Attempt to load room.
	if (!SetRoomAtCoords(this->pRoom->dwRoomX, this->pRoom->dwRoomY + 1))
		return false;

	//Put player at north edge of new room.
	SetPlayer(this->swordsman.wX, 0);

	return true;
}

//***************************************************************************************
bool CCurrentGame::LoadWestRoom()
//Loads room west of the current room in the context of the swordsman exiting
//from the west end of the room.
//
//Returns:
//True if room was successfully loaded, false if not.
{
	//Swordsman should be at west edge.
	ASSERT(this->swordsman.wX == 0);

	//Attempt to load room.
	if (!SetRoomAtCoords(this->pRoom->dwRoomX - 1, this->pRoom->dwRoomY))
		return false;

	//Put player at east edge of new room.
	SetPlayer(this->pRoom->wRoomCols - 1, this->swordsman.wY);

	return true;
}

//***************************************************************************************
bool CCurrentGame::PlayerCanExitRoom(
//Returns: whether player can exit this room and enter the next room.
//
//OUT: Loads room adjacent to the current room in the direction of the player's
//exit of the current room.  Player coords are also updated so that he wraps
//to other side in new room.
//
//Params:
	const UINT wDirection,  //(in) direction player is leaving room from
	UINT &dwNewSX, UINT &dwNewSY,  //(out) destination room info
	CDbRoom* &pNewRoom)
{
	pNewRoom = NULL;

	//Player should be exiting from proper edge of room.
	//Get coordinates of room being entered.
	UINT dwNewRoomX, dwNewRoomY;
	switch (wDirection)
	{
		case N:
			ASSERT(this->swordsman.wY == 0);
			dwNewRoomX = this->pRoom->dwRoomX;
			dwNewRoomY = this->pRoom->dwRoomY - 1;
			break;
		case S:
		   ASSERT(this->swordsman.wY == this->pRoom->wRoomRows - 1);
			dwNewRoomX = this->pRoom->dwRoomX;
			dwNewRoomY = this->pRoom->dwRoomY + 1;
			break;
		case W:
			ASSERT(this->swordsman.wX == 0);
			dwNewRoomX = this->pRoom->dwRoomX - 1;
			dwNewRoomY = this->pRoom->dwRoomY;
			break;
		case E:
		   ASSERT(this->swordsman.wX == this->pRoom->wRoomCols - 1);
			dwNewRoomX = this->pRoom->dwRoomX + 1;
			dwNewRoomY = this->pRoom->dwRoomY;
			break;
		default:
			ASSERT(!"Bad direction value.");
		return false;
	}

	//Attempt to load room.
	pNewRoom = this->pLevel->GetRoomAtCoords(dwNewRoomX, dwNewRoomY);
	if (!pNewRoom)
		return false;

	//Put player at opposite edge of new room.
	switch (wDirection)
	{
		case N:
			dwNewSX = this->swordsman.wX;
			dwNewSY = pNewRoom->wRoomRows - 1;
			break;
		case S:
			dwNewSX = this->swordsman.wX;
			dwNewSY = 0;
			break;
		case W:
			dwNewSX = this->pRoom->wRoomCols - 1;
			dwNewSY = this->swordsman.wY;
			break;
		case E:
			dwNewSX = 0;
			dwNewSY = this->swordsman.wY;
			break;
	}

	//Determine whether player can enter room here.
	const UINT wTurnNo_ = this->wTurnNo; //need to reset the turn number when
	this->wTurnNo = 0; //calling SetCurrentGame to indicate initial default NPC state must be set
	pNewRoom->SetCurrentGame(this);
	this->wTurnNo = wTurnNo_;

	if (pNewRoom->bCheckForHoldCompletion && !this->bHoldCompleted) {
		this->bHoldCompleted = g_pTheDB->Holds.IsHoldCompleted(this->pHold->dwHoldID, g_pTheDB->GetPlayerID());
		pNewRoom->bCheckForHoldCompletion = false;
	}
	if (pNewRoom->bCheckForHoldMastery && !this->bHoldMastered) {
		this->bHoldMastered = g_pTheDB->Holds.IsHoldMastered(this->pHold->dwHoldID, g_pTheDB->GetPlayerID());
		pNewRoom->bCheckForHoldMastery = false;
	}

	return pNewRoom->CanSetSwordsman(dwNewSX, dwNewSY, IsRoomAtCoordsConquered(dwNewRoomX, dwNewRoomY));
}

//***************************************************************************************
void CCurrentGame::ProcessDoublePlacement(
//Processes double placement (period of the game between player stepping on
//a double potion and a double being placed in the room at player-specified
//position).
//
//Params:
	int nCommand,        //(in)   Game command.
	CCueEvents &CueEvents,  //(out)  List of events that can be handled by caller.
							//    These are things that the UI wouldn't necessarily
							//    be aware of by looking at the modified game
							//    data on return.
	const UINT wX, const UINT wY)	//(in) optional
{
	ASSERT(this->swordsman.wPlacingDoubleType);

   //Figure out how to change swordsman based on command.
   int dx = 0, dy = 0;
   switch (nCommand)
   {
      case CMD_N: dy = -1; break;
      case CMD_NW: dx = dy = -1; break;
      case CMD_NE: dx = 1; dy = -1; break;
      case CMD_E: dx = 1; break;
      case CMD_SE: dx = dy = 1; break;
      case CMD_S: dx = 0; dy = 1; break;
      case CMD_SW: dx = -1; dy = 1; break;
      case CMD_W: dx = -1; break;

		case CMD_CC: case CMD_C:
		case CMD_WAIT: case CMD_DOUBLE: break;
		default: return; //invalid command -- might be from an old version demo
   }

   if (nCommand==CMD_WAIT || nCommand==CMD_DOUBLE) // Create a new Double
   {
		//Place double at this spot.
		if (nCommand==CMD_DOUBLE)
		{
			ASSERT(this->pRoom->IsValidColRow(wX,wY));
			this->swordsman.wDoubleCursorX = wX;
			this->swordsman.wDoubleCursorY = wY;
		}

		//Check for obstacles in destination square.
      if (!this->pRoom->DoesSquareContainDoublePlacementObstacle(this->swordsman.wDoubleCursorX,
				this->swordsman.wDoubleCursorY, this->swordsman.wPlacingDoubleType))
      {
         CPlayerDouble *pDouble = DYN_CAST(CPlayerDouble*, CMonster*, pRoom->AddNewMonster(
					this->swordsman.wPlacingDoubleType,
					this->swordsman.wDoubleCursorX, this->swordsman.wDoubleCursorY));
         this->swordsman.wPlacingDoubleType=0;

         if (pDouble) //Yes, adding a Double worked.
         {
				//Count double placement as one command.
				if (!this->Commands.IsFrozen())
				{
					this->Commands.Add(CMD_DOUBLE);
					this->Commands.AddData(pDouble->wX, pDouble->wY);
				}
				++this->wTurnNo;
				++this->dwLevelMoves;
				ASSERT(this->dwLevelMoves > 0);

				CueEvents.Add(CID_DoublePlaced);
				pDouble->SetCurrentGame(this);
				pDouble->wPrevX=pDouble->wX;
				pDouble->wPrevY=pDouble->wY;
				pDouble->wPrevO=pDouble->wO=this->swordsman.wO;
				pDouble->weaponType = this->swordsman.GetActiveWeapon();
				pDouble->Process(CMD_WAIT, CueEvents);

				QueryCheckpoint(CueEvents, pDouble->wX, pDouble->wY);

				//Activate pressure plate at destination if not flying.
				if (this->pRoom->GetOSquare(pDouble->wX, pDouble->wY) == T_PRESSPLATE &&
						(pDouble->wType != M_CLONE || bCanEntityPressPressurePlates(this->swordsman.wAppearance)))
					this->pRoom->ActivateOrb(pDouble->wX, pDouble->wY, CueEvents, OAT_PressurePlate);

				this->pRoom->ActivateToken(CueEvents, pDouble->wX, pDouble->wY, pDouble);
				pDouble->SetWeaponSheathed();
				if (pDouble->HasSword())
				{
					ProcessArmedMonsterWeapon(pDouble, CueEvents);
					ResolveSimultaneousTarstuffStabs(CueEvents);
				}

				//Start immediately at clone's position, if the player didn't die by placement.
				if (pDouble->wType == M_CLONE &&
						!CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied))
					SwitchToCloneAt(pDouble->wX, pDouble->wY);

				//Handle events possible after double placement.
				this->pRoom->ProcessTurn(CueEvents, false);

				UpdatePrevCoords();
         }
      }
   }
	//Move double placement cursor.
   else if (dx != 0 || dy != 0)
   {
      //Don't allow moving cursor outside of room boundaries.
      if (!this->pRoom->IsValidColRow(this->swordsman.wDoubleCursorX + dx, this->swordsman.wDoubleCursorY + dy))
      {
         dx = dy = 0;
      } else {
         this->swordsman.wDoubleCursorX += dx;
         this->swordsman.wDoubleCursorY += dy;
      }
   }
}

//***************************************************************************************
void CCurrentGame::PreprocessMonsters(
//Perform any calculations required by the monsters *before* any monster makes a move.
//
//Params:
	CCueEvents &CueEvents)  //(in/out)  List of events that can be handled by caller.
{
	//Determine whether Halph/Slayer can enter room.
	if (this->wSpawnCycleCount % 5 != 0)
		return;

	CDbRoom &room = *this->pRoom;
	const bool bHalphEnters = !room.halphEnters.empty();
	const bool bHalph2Enters = !room.halph2Enters.empty();
	const bool bSlayerEnters = !room.slayerEnters.empty();
	const bool bSlayer2Enters = !room.slayer2Enters.empty();
	ASSERT(!bHalphEnters || !bHalph2Enters || !bSlayerEnters || !bSlayer2Enters); //this check could be tightened
	if (!bHalphEnters && !bHalph2Enters && !bSlayerEnters && !bSlayer2Enters)
		return;

	//Determine direction of entrance.
	UINT wX, wY;
	if (bHalphEnters)
		room.halphEnters.first(wX, wY);
	else if (bHalph2Enters)
		room.halph2Enters.first(wX, wY);
	else if (bSlayerEnters)
		room.slayerEnters.first(wX, wY);
	else
		room.slayer2Enters.first(wX, wY);
	const int nOX = wX == 0 ? 1 : wX == room.wRoomCols - 1 ? -1 : 0;
	const int nOY = wY == 0 ? 1 : wY == room.wRoomRows - 1 ? -1 : 0;
	const UINT wO = nGetO(nOX, nOY);
	//Can Halph/Slayer enter room at this time?
	const UINT wAppearance = this->swordsman.wAppearance;
	this->swordsman.wAppearance = M_BEETHRO; //this type of player
	bool bIgnored;
	const bool bClosed = room.DoesSquareContainPlayerObstacle(wX, wY, wO, bIgnored) ||
		room.DoesOrthoSquarePreventDiagonal(wX, wY, nOX, nOY) || room.GetOSquare(wX, wY) == T_FIRETRAP_ON ||
		room.IsMonsterOfTypeAt(M_FLUFFBABY, wX, wY) || IsPlayerAt(wX, wY) || IsPlayerWeaponAt(wX, wY);
	this->swordsman.wAppearance = wAppearance;
	if (bClosed)
		return;  //No.

	if (bHalphEnters && bIsShallowWater(room.GetOSquare(wX, wY)))
		return;  //Young Halph can't wade

	//Slayer won't enter if hostile player seems to be guarding the entrance, even w/o a sword.
	if ((bSlayerEnters || bSlayer2Enters) && this->swordsman.IsTarget())
		if (nDist(wX, wY, this->swordsman.wSwordX, this->swordsman.wSwordY) <= 3)
			return;

	//Add Halph/Slayer to room.
	CMonster *pMonster;
	if (bHalphEnters)
	{
		room.halphEnters.clear();
		pMonster = room.AddNewMonster(M_HALPH, wX, wY);
		CueEvents.Add(CID_HalphEntered, pMonster);
	} else if (bHalph2Enters) {
		room.halph2Enters.clear();
		pMonster = room.AddNewMonster(M_HALPH2, wX, wY);
		CueEvents.Add(CID_HalphEntered, pMonster);
	} else if (bSlayerEnters) {
		room.slayerEnters.clear();
		pMonster = room.AddNewMonster(M_SLAYER, wX, wY);
		CueEvents.Add(CID_SlayerEntered, pMonster);
	} else {
		room.slayer2Enters.clear();
		pMonster = room.AddNewMonster(M_SLAYER2, wX, wY);
		CueEvents.Add(CID_SlayerEntered, pMonster);
	}
	pMonster->wO = wO;
	pMonster->bIsFirstTurn = true;
	pMonster->SetCurrentGame(this);
	//Might need updated pathmaps
	room.CreatePathMaps();

	//Set state for entrance tile.
	if (pMonster->wType == M_SLAYER || pMonster->wType == M_SLAYER2)
	{
		CArmedMonster *pArmedMonster = DYN_CAST(CArmedMonster*, CMonster*, pMonster);
		pArmedMonster->SetWeaponSheathed();
	}
	//Check for stepping on pressure plate.
	if (room.GetOSquare(wX, wY) == T_PRESSPLATE)
		room.ActivateOrb(wX, wY, CueEvents, OAT_PressurePlate);
}

//***************************************************************************************
void CCurrentGame::ProcessMonsters(
//Processes all the monsters in the current room.
//
//Params:
	int nLastCommand,    //(in)      Last swordsman command.
	CCueEvents &CueEvents)  //(in/out)  List of events that can be handled by caller.
{
	if (!this->bHalfTurn)
	{
		//Increment the spawn cycle counter.
		++this->wSpawnCycleCount;
	}

	PreprocessMonsters(CueEvents);

	//Check whether pathmaps are needed now
	//We will be unable to delete them later when we've determined
	//that the room is brained
	this->bBrainSensesSwordsman = false;
	if (!this->pRoom->IsPathmapNeeded())
		this->pRoom->DeletePathMaps();

	//Since player and mimic movement is practically synched,
	//wait until all mimics have moved, possibly moving platforms, before
	//things can fall down.
	bool bPlatformCheck = this->pRoom->platforms.empty(); //for platform
	bool bBrainCheck = this->pRoom->wBrainCount == 0; //for brains

	//Each iteration processes one monster.
	for (CMonster *pMonster = this->pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		this->pRoom->ClearPushStates();

		if (pMonster->wType == M_CONSTRUCT && !this->bHalfTurn)
		{
			CConstruct *pConstruct = DYN_CAST(CConstruct*, CMonster*, pMonster);
			pConstruct->ResetOremiteDamage();
		}

		pMonster->bProcessing = true;

		//Platforms fall when doubles are done moving.
		if (!bPlatformCheck && pMonster->GetProcessSequence() > SPD_PDOUBLE)
		{
			CPlatform::checkForFalling(this->pRoom, CueEvents);
			bPlatformCheck = true;
		}
		//Brains detect players once doubles are done moving.
		if (!bBrainCheck && pMonster->GetProcessSequence() > SPD_PDOUBLE)
		{
			this->bBrainSensesSwordsman = this->pRoom->BrainSensesSwordsman();
			bBrainCheck = true;
		}

		ProcessMonster(pMonster, nLastCommand, CueEvents);

		// Kegs stabbed by pushes should explode after each monster
		this->pRoom->ExplodeStabbedPowderKegs(CueEvents);

		pMonster->bProcessing = false;

		//If we deferred an Unlink due to processing the monster, then unlink it now.
		if (pMonster->bUnlink)
			this->pRoom->UnlinkMonster(pMonster);
	}

	//Process all monsters again to remove Stun checks added this turn
	for (CMonster *pMonster = this->pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
	{
		if (pMonster->stunned)
		{
			//Monsters are stunned for this turn and next full turn
			if (pMonster->bNewStun || !this->bHalfTurn)
				--pMonster->stunned;
			pMonster->bNewStun = false;
			pMonster->bPushedThisTurn = false;
		}
	}
	
	//If we got this far without a brain check, then run it now
	if (!bBrainCheck)
		this->bBrainSensesSwordsman = this->pRoom->BrainSensesSwordsman();

	//Don't need to run a platform check since that
	//will be handled again after ProcessMonsters
}

//***************************************************************************************
void CCurrentGame::ProcessMonster(CMonster* pMonster, int nLastCommand, CCueEvents &CueEvents)
{
	if (!pMonster->bIsFirstTurn)
	{
		if (pMonster->TakesTurn())
		{
			//Skip monster's move when performing a speed move
			//(Doubles get to move as they're synchronized with the player.)
			const bool bIsDouble = bIsBeethroDouble(pMonster->wType);
			if (!this->bHalfTurn || bIsDouble)
			{
				//Monster makes a move.
				pMonster->Process(nLastCommand, CueEvents);

			} else {
				//Handle monsters that perform specific functions on half-turns.
				switch (pMonster->wType)
				{
					case M_EYE:
					{
						//Do evil eyes wake up?
						if (!pMonster->IsStunned())
						{
							CMoveCoord *pCoord;
							int nOX, nOY;
							CEvilEye *pEye = DYN_CAST(CEvilEye*, CMonster*, pMonster);
							pEye->WakeupCheck(CueEvents, pCoord, nOX, nOY);
						}
					}
					break;
					case M_CHARACTER:
					{
						//Character commands that don't expend a turn processed.
						this->bExecuteNoMoveCommands = true;
						pMonster->Process(nLastCommand, CueEvents);
						this->bExecuteNoMoveCommands = false;
					}
					break;
					default: break; //all other monster types do nothing
				}
			}

			//Check for damage inflicted from something in the room.
			if (pMonster->bAlive && pMonster->CheckForDamage(CueEvents))
			{
				//Monster died.
				this->pRoom->KillMonster(pMonster, CueEvents);
				CheckTallyKill(pMonster);
				if (pMonster->wType == M_ROCKGIANT)
					CRockGiant::Shatter(CueEvents, this, pMonster->wX, pMonster->wY);
				return;  //extra processing below gets skipped on death
			}

			UINT wX, wY, wO;

			//Check for events stemming from the monster's behavior
			//which require modification of the monster list.
			switch (pMonster->wType)
			{
				case M_MIMIC: case M_CLONE: case M_DECOY:
				case M_SLAYER: case M_SLAYER2: case M_GUARD:
				case M_STALWART: case M_STALWART2:
				case M_TEMPORALCLONE:
					if ((!this->bHalfTurn || bIsBeethroDouble(pMonster->wType))
						    && (!pMonster->bUnlink || pMonster->bForceWeaponAttack) //Removed PlayerDoubles shouldn't strike with swords unless died while moving
							&& !pMonster->bPushedThisTurn)  //Monsters pushed this turn already had their attack calculated
					{
						//Process double's sword hit on turns they take.
						CArmedMonster *pArmedMonster = DYN_CAST(CArmedMonster*, CMonster*, pMonster);
						pArmedMonster->SetWeaponSheathed();
						ProcessArmedMonsterWeapon(pArmedMonster, CueEvents);
					}
				break;
				case M_NEATHER:
					if (CueEvents.HasOccurredWith(CID_MonsterExitsRoom, pMonster))
						this->pRoom->KillMonster(pMonster, CueEvents);
				break;
				case M_CHARACTER:
				{
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
					if (pCharacter->IsVisible() &&
							!pCharacter->bNewStun && !pCharacter->bPreventMoveAfterPush)
					{
						pCharacter->SetWeaponSheathed();
						ProcessArmedMonsterWeapon(pCharacter, CueEvents);
					}
					PostProcessCharacter(pCharacter, CueEvents);
				}
				break;
				case M_SERPENT:
					if (CueEvents.HasOccurredWith(CID_SnakeDiedFromTruncation, pMonster))
					{
						// Red serpent died from shortening.
						if (pMonster->bAlive)
						{
							this->pRoom->KillMonster(pMonster, CueEvents);
							TallyKill();
						}
					}
				break;
				case M_REGG:
					if (CueEvents.HasOccurredWith(CID_EggHatched, pMonster))
					{
						// Spawn a roach AFTER egg has been removed.
						const UINT stunned = pMonster->stunned;
						const bool bNewStun = pMonster->bNewStun;
						const bool bPushedThisTurn = pMonster->bPushedThisTurn;
						wX = pMonster->wX;
						wY = pMonster->wY;
						this->pRoom->KillMonster(pMonster, CueEvents);
						CMonster *m = this->pRoom->AddNewMonster(M_ROACH,wX,wY);
						m->bIsFirstTurn = true;
						m->stunned = stunned;
						m->bNewStun = bNewStun;
						m->bPushedThisTurn = bPushedThisTurn;
					}
				break;
				case M_FEGUNDO:
					if (CueEvents.HasOccurredWith(CID_FegundoToAsh, pMonster))
						FegundoToAsh(pMonster, CueEvents);
				break;
				case M_FEGUNDOASHES:
					if (CueEvents.HasOccurredWith(CID_AshToFegundo, pMonster))
					{
						// Spawn a fegundo AFTER ash has been removed.
						const UINT stunned = pMonster->stunned;
						const bool bNewStun = pMonster->bNewStun;
						const bool bPushedThisTurn = pMonster->bPushedThisTurn;
						wX = pMonster->wX;
						wY = pMonster->wY;
						wO = this->swordsman.wO;
						this->pRoom->KillMonster(pMonster, CueEvents);
						CMonster *m = this->pRoom->AddNewMonster(M_FEGUNDO,wX,wY);
						m->SetOrientation(nGetOX(wO), nGetOY(wO));
						m->bIsFirstTurn = true;
						m->stunned = stunned;
						m->bNewStun = bNewStun;
						m->bPushedThisTurn = bPushedThisTurn;
					}
				break;
			}
		}
	}
}

//***************************************************************************************
void CCurrentGame::ResolveSimultaneousTarstuffStabs(
//Processes results of all weapon stabs entering tarstuff tiles that
//must be stabbed simultaneously.
//
//Params:
	CCueEvents &CueEvents)  //(out)  List of events that can be handled by caller.
							//    These are things that the UI wouldn't necessarily
							//    be aware of by looking at the modified game
							//    data on return.
{
	//NOTE: this is currently only relevant and in effect for tarstuff stabbings
	const CAttachableObject *pObj = CueEvents.GetFirstPrivateData(CID_TarstuffStabbed);
	vector<CMoveCoord> simulSwordHits;

	while (pObj)
	{
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		simulSwordHits.push_back(CMoveCoord(pCoord->wX,pCoord->wY,pCoord->wO));

		//Remove the stab, but don't reset the flag
		CueEvents.Remove(CID_TarstuffStabbed, pObj);

		pObj = CueEvents.GetFirstPrivateData(CID_TarstuffStabbed);
	}

    //Run through stabs in reverse order for backwards compatibility
	bool bTarStabbed = false;
	for (vector<CMoveCoord>::const_reverse_iterator rIter = simulSwordHits.rbegin();
			rIter != simulSwordHits.rend(); ++rIter)
	{
		//Remove tarstuff now.
		const CMoveCoord& moveCoord = *rIter;
		if (this->pRoom->StabTar(moveCoord.wX, moveCoord.wY, CueEvents, true, moveCoord.wO))
			bTarStabbed = true;
	}

	//Convert remaining tarstuff into babies after stabs
	if (bTarStabbed)
		this->pRoom->ConvertUnstableTar(CueEvents);
}

//***************************************************************************************
void CCurrentGame::ProcessPlayer(
//Processes a player command.
//
//Params:
	const int nCommand,        //(in)   Game command.
	CCueEvents &CueEvents)  //(out)  List of events that can be handled by caller.
							//    These are things that the UI wouldn't necessarily
							//    be aware of by looking at the modified game
							//    data on return.
{
	int dx = 0, dy = 0;
	//Figure out how to change player based on command.
	switch (nCommand)
	{
		//Rotate.
		case CMD_C:
			if (this->swordsman.HasWeapon())
				CueEvents.Add(CID_SwingSword,
					new CCoord(this->swordsman.wO, nCommand), true); //comes first
			this->swordsman.RotateClockwise();
			break;
		case CMD_CC:
			this->swordsman.RotateCounterClockwise();
			if (this->swordsman.HasWeapon())
				CueEvents.Add(CID_SwingSword,
					new CCoord(this->swordsman.wO, nCommand), true); //comes after
			break;
		//Move.
		case CMD_NW: dx = dy = -1; break;
		case CMD_N: dy = -1; break;
		case CMD_NE: dx = 1; dy = -1; break;
		case CMD_W: dx = -1; break;
		case CMD_E: dx = 1; break;
		case CMD_SW: dx = -1; dy = 1; break;
		case CMD_S: dx = 0; dy = 1; break;
		case CMD_SE: dx = dy = 1; break;

		case CMD_WAIT: break;

		default: return; //invalid command -- might be from an old version demo
	}

	const UINT nFirstO = nGetO(dx,dy);

	//Player can take steps if not frozen.
	if (this->swordsman.bFrozen)
		dx = dy = 0;

	//Calculate sword movement
	this->swordsman.wSwordMovement = CSwordsman::GetSwordMovement(nCommand, this->swordsman.wO);
	this->swordsman.nLastCommand = nCommand;

	const UINT wOTileNo = this->pRoom->GetOSquare(this->swordsman.wX, this->swordsman.wY);
	const UINT wFTileNo = this->pRoom->GetFSquare(this->swordsman.wX, this->swordsman.wY);
	const UINT wTTileNo = this->pRoom->GetTSquare(this->swordsman.wX, this->swordsman.wY);
	bool bEnteredTunnel = false;
	bool bMovingPlatform = false;

	//Look for obstacles and set dx/dy accordingly.
	const UINT wMoveO = nGetO(dx, dy);
	UINT destX = this->swordsman.wX + dx, destY = this->swordsman.wY + dy;

	if (!dx && !dy)
		goto MakeMove;

	//Check for arrows underfoot.
	if (bIsArrowObstacle(wFTileNo, wMoveO))
	{
		CueEvents.Add(CID_HitObstacle, new CMoveCoord(
				this->swordsman.wX, this->swordsman.wY, wMoveO), true);
		dx = dy = 0;
		destX = this->swordsman.wX; destY = this->swordsman.wY;
		goto MakeMove;
	}
	if (this->pRoom->DoesSquarePreventDiagonal(
			this->swordsman.wX, this->swordsman.wY, dx,dy))
	{
		CueEvents.Add(CID_HitObstacle, new CMoveCoord(
			this->swordsman.wX + (wFTileNo == T_NODIAGONAL ? 0 : dx),
			this->swordsman.wY + (wFTileNo == T_NODIAGONAL ? 0 : dy), wMoveO), true);
		dx = dy = 0;
		destX = this->swordsman.wX; destY = this->swordsman.wY;
		goto MakeMove;
	}

	{ // scope to prevent new variables from tripping up gotos to MakeMove

	//Check for tunnel entrance before checking for room exit.
	bEnteredTunnel = PlayerEnteredTunnel(wOTileNo, wMoveO);
	if (bEnteredTunnel)
		goto MakeMove;

	//Check for leaving room.
	if (!this->pRoom->IsValidColRow(destX, destY))
	{
		//Prevent player from leaving room when lock is enabled.
		if (this->bRoomExitLocked)
		{
			ASSERT(!IsCutScenePlaying());
			CueEvents.Add(CID_RoomExitLocked);
			this->pRoom->BurnFuseEvents(CueEvents); //need to regather these events
			return;
		}

		if (ProcessPlayer_HandleLeaveRoom(wMoveO, CueEvents))
			return;

		//Probably a room that had an exit but no room adjacent
		//in that direction, or cannot enter room on that square.
		//Just treat it as an obstacle.
		dx = dy = 0;
		destX = this->swordsman.wX; destY = this->swordsman.wY;
		goto MakeMove;
	}

	//Prevent player from leaving room via stairs when lock is enabled.
	if (this->bRoomExitLocked)
	{
		//Don't check whether move is legal -- just mention to player stepping
		//onto stairs is currently locked.
		if (bIsStairs(this->pRoom->GetOSquare(destX, destY)))
		{
			ASSERT(!IsCutScenePlaying());
			CueEvents.Add(CID_RoomExitLocked);
			return;
		}
	}

	//Check for obstacles in destination square.
	bool bMonsterObstacle; //If a monster obstacle was found
	if (this->pRoom->DoesSquareContainPlayerObstacle(
			destX, destY, wMoveO, bMonsterObstacle))
	{
		//There is an obstacle on the destination square,
		//but it might need special handling or queries.
		//Check each layer for an object that gets handled specially.
		const UINT wNewOTile = this->pRoom->GetOSquare(destX, destY);
		const UINT wNewFTile = this->pRoom->GetFSquare(destX, destY);
		const UINT wNewTTile = this->pRoom->GetTSquare(destX, destY);
		bool bNotAnObstacle = false, bPushingTLayerItem = false;

		//Check o-layer stuff.
		switch (wNewOTile)
		{
			case T_PIT: case T_PIT_IMAGE:
				//Flying roles do not consider pit an obstacle
				if (bIsEntityFlying(this->swordsman.wAppearance))
					goto CheckFLayer;
				//If standing on a platform, check whether it can move.
				if (wOTileNo == T_PLATFORM_P)
					if (this->pRoom->CanMovePlatform(this->swordsman.wX, this->swordsman.wY, nFirstO))
					{
						bMovingPlatform = bNotAnObstacle = true;
						goto CheckFLayer;
					}
				CueEvents.Add(CID_Scared);
			break;
			case T_SHALLOW_WATER:
				//Flying/Swimming/Wading roles do not consider shallow water an obstacle
				if (bIsEntityFlying(this->swordsman.wAppearance) ||
						bIsEntitySwimming(this->swordsman.wAppearance) ||
						this->swordsman.GetWaterTraversalState() >= WTrv_CanWade)
					goto CheckFLayer;
				CueEvents.Add(CID_Scared);
			break;
			case T_WATER:
				//Flying/Swimming roles do not consider water an obstacle
				if (bIsEntityFlying(this->swordsman.wAppearance) ||
						bIsEntitySwimming(this->swordsman.wAppearance))
					goto CheckFLayer;
				if (wOTileNo == T_PLATFORM_W)
					if (this->pRoom->CanMovePlatform(this->swordsman.wX, this->swordsman.wY, nFirstO))
					{
						bMovingPlatform = bNotAnObstacle = true;
						goto CheckFLayer;
					}
				CueEvents.Add(CID_Scared);
			break;
			case T_DOOR_Y:
				if (bIsBeethroDouble(this->swordsman.wAppearance))
				{
					//Beethro knocked on a yellow door.
					//If Halph is in the room, request he open it.
					CMonster *pMonster = this->pRoom->MonsterOfTypeExists(M_HALPH);
					if (!pMonster)
						pMonster = this->pRoom->MonsterOfTypeExists(M_HALPH2);
					if (pMonster)
					{
						CHalph *pHalph = DYN_CAST(CHalph*, CMonster*, pMonster);
						pHalph->RequestOpenDoor(destX, destY, CueEvents);
					} else
						CueEvents.Add(CID_HitObstacle,
								new CMoveCoord(destX, destY, wMoveO), true);
				} else
					CueEvents.Add(CID_HitObstacle, new CMoveCoord(destX, destY, wMoveO), true);
			break;
			case T_WALL_M:
				if (!this->bHoldMastered)
				{
					//Player hit "master wall" and couldn't go through.
					//Don't allow this move to be made.
					CueEvents.Add(CID_BumpedMasterWall);
					return;
				}
				goto CheckFLayer;
			break;
			case T_WALL_WIN:
				if (!this->bHoldCompleted)
				{
					//Player hit "hold completion wall" and couldn't go through.
					//Don't allow this move to be made.
					CueEvents.Add(CID_BumpedHoldCompleteWall);
					return;
				}
				goto CheckFLayer;
			break;
			default:
				goto CheckFLayer;
			break;
		}
		//If here, that means a special o-layer obstacle was encountered and handled.
		//Player still can't move, however.
		dx = dy = 0;
		destX = this->swordsman.wX; destY = this->swordsman.wY;
		goto MakeMove;

CheckFLayer:
		//Check for special f/t-layer handling after checking o-layer.
		if (bIsArrowObstacle(wNewFTile, wMoveO) ||
				(wNewFTile == T_NODIAGONAL && dx && dy)) //can't go against arrows, etc
		{
			CueEvents.Add(CID_HitObstacle,
					new CMoveCoord(destX, destY, wMoveO), true);
			dx = dy = 0;
			destX = this->swordsman.wX; destY = this->swordsman.wY;
			goto MakeMove;
		}

		switch (wNewTTile)
		{
			//If player is in humanoid, non-sworded role, allow hitting orbs directly.
			case T_ORB:
			case T_BEACON: case T_BEACON_OFF:
				if ((bIsHuman(this->swordsman.wAppearance) &&
						!bEntityHasSword(this->swordsman.wAppearance)) ||
						this->swordsman.bCanGetItems) //power token allows this too
					if (wNewTTile == T_ORB)
					{
						this->pRoom->ActivateOrb(destX, destY, CueEvents, OAT_Player);
					} else {
						this->pRoom->ActivateBeacon(destX, destY, CueEvents);
					}
				else //can't activate orb
					CueEvents.Add(CID_HitObstacle, new CMoveCoord(destX, destY, wMoveO), true);
			break;

			case T_MIRROR:
			case T_POWDER_KEG:
			{
				//Player ran into item.  Push if possible.
				if (this->pRoom->CanPlayerMoveOnThisElement(
							this->swordsman.wAppearance, this->pRoom->GetOSquare(destX, destY)) &&
						this->pRoom->CanPushTo(destX, destY, destX + dx, destY + dy))
				{
					//Push, if monster layer doesn't have an obstacle too.
					bPushingTLayerItem = bNotAnObstacle = true;
					goto CheckMonsterLayer;
				}
				//Can't push.
				CueEvents.Add(CID_HitObstacle,
						new CMoveCoord(destX, destY, wMoveO), true);
			}
			break;
			// Briar and fluff are obstacles to the player even when moving a platform.
			case T_BRIAR_SOURCE: case T_BRIAR_DEAD: case T_BRIAR_LIVE:
				CueEvents.Add(CID_Scared);
			break;
			case T_FLUFF:
				CueEvents.Add(CID_HitObstacle,
						new CMoveCoord(destX, destY, wMoveO), true);
			break;
			default:
				goto CheckMonsterLayer;	//no special obstacle handling on t-layer
			break;
		}
		//If here, that means a special t-layer obstacle was encountered and handled.
		//Player still can't move, however.
		dx = dy = 0;
		destX = this->swordsman.wX; destY = this->swordsman.wY;
		goto MakeMove;

CheckMonsterLayer:
		//Check for special monster handling after checking o- and t-layers.
		CMonster *pMonster = this->pRoom->GetMonsterAtSquare(destX, destY);
		bool bHittingObstacle = true;
		if (pMonster && bMonsterObstacle)
		{
			switch (pMonster->wType)
			{
				case M_HALPH: case M_HALPH2: if (bIsBeethroDouble(this->swordsman.wAppearance))
				{
					//Beethro taps Halph on the shoulder.
					CHalph *pHalph = DYN_CAST(CHalph*, CMonster*, pMonster);
					pHalph->SwordsmanBumps(CueEvents);
					bHittingObstacle = false;
					bNotAnObstacle = false;
				}
				break;
				case M_CHARACTER:
				{
					//Player bumps into an NPC, see if we can push him first
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);


					bool bPushedCharacter = false;

					if (pCharacter->IsPushableByBody()){
						const UINT wDestX = pCharacter->wX + dx;
						const UINT wDestY = pCharacter->wY + dy;

						//Prevent pushes that would allow an otherwise illegal move.
						bool bPlayerCanMoveTo = this->pRoom->CanPlayerMoveOnThisElement(
							this->swordsman.wAppearance, this->pRoom->GetOSquare(destX, destY)
						) && !bIsTLayerObstacle(wNewTTile);

						if (this->pRoom->IsValidColRow(wDestX, wDestY) && bPlayerCanMoveTo &&
								this->pRoom->CanPushMonster(pMonster, pMonster->wX, pMonster->wY, wDestX, wDestY)){
							pCharacter->PushInDirection(dx, dy, false, CueEvents);
							bPushedCharacter = true;
						}
					}

					if (!bPushedCharacter)
						pCharacter->bPlayerTouchedMe = true;

					bNotAnObstacle = bPushedCharacter;
				}
				break;
				default:
					bNotAnObstacle = false;
				break;
			}
		} 
		
		//Every special obstacle check should have been performed by now.

		//If a seeming obstacle could actually be moved on, do it now.
		//(e.g. walking on top of a platform or pushing a mirror)
		if (bNotAnObstacle &&
				//can't move onto a sword even if the obstacle in the way is moveable/avoidable
				!this->pRoom->IsMonsterSwordAt(destX, destY, true))
		{
			//Can move platform now.
			if (bMovingPlatform)
				this->pRoom->MovePlatform(this->swordsman.wX, this->swordsman.wY, nFirstO);

			//If item is being pushed, can do it now.
			if (bPushingTLayerItem) {
				this->pRoom->PushTLayerObject(destX, destY, destX + dx, destY + dy, CueEvents);
			}
			goto MakeMove;
		}

		//If here, that means obstacle is handled in normal way.
		if (bHittingObstacle)
			CueEvents.Add(CID_HitObstacle,
					new CMoveCoord(destX, destY, wMoveO), true);
		dx = dy = 0;
		destX = this->swordsman.wX; destY = this->swordsman.wY;
	}

	} // scope to prevent new variables from tripping up gotos to MakeMove

MakeMove:
	//Before he moves, remember important square contents and conditions.
	const bool bMoved = dx!=0 || dy!=0;
	const bool bCanDropTrapdoor = this->swordsman.CanDropTrapdoor(wOTileNo);
	//These exclude the wait move executed on entering a room.
	const bool bWasOnSameScroll = (wTTileNo==T_SCROLL) && !bMoved && this->wTurnNo;
	const UINT wStartingX = this->swordsman.wX;
	const UINT wStartingY = this->swordsman.wY;
	const bool bStayedOnHotFloor = (wOTileNo==T_HOT) && !bMoved && this->wTurnNo &&
			//flying types don't get burned by hot tiles
			this->swordsman.wAppearance != M_WWING && this->swordsman.wAppearance != M_FEGUNDO;

	AddTemporalSplitCommand(nCommand, dx || dy);

	//Set player to new location.
	if (!bEnteredTunnel && nCommand != CMD_C && nCommand != CMD_CC)
		SetPlayer(destX, destY);

	pRoom->ExplodeStabbedPowderKegs(CueEvents);

	const UINT wNewOSquare = pRoom->GetOSquare(this->swordsman.wX, this->swordsman.wY);

	//If obstacle was in the way, player's sword doesn't move.
	if (!dx && !dy && nCommand != CMD_C && nCommand != CMD_CC &&
			this->swordsman.wSwordMovement != this->swordsman.wO) //pushing forward is valid
		this->swordsman.wSwordMovement = NO_ORIENTATION;

	const bool bWeaponWasSheathed = !this->swordsman.HasWeapon();
	//Swordless entities automatically face the way they're trying to move.
	const bool face_movement_direction = bWeaponWasSheathed || this->swordsman.GetActiveWeapon() == WT_Dagger;
	if (face_movement_direction && nFirstO != NO_ORIENTATION)
	{
		this->swordsman.SetOrientation(nFirstO);
		this->swordsman.wSwordMovement = nFirstO;
	}

	//Sword must be put away when on oremites or when in shallow water.
	if (!bIsStairs(wNewOSquare)) //don't update sword status when moving on stairs
		SetPlayerWeaponSheathedState();

	if (!this->swordsman.HasWeapon())
		CueEvents.ClearEvent(CID_SwingSword); //don't fire the pending swing cue

	//Check for movement off of a trapdoor.
	if (bCanDropTrapdoor && bMoved)
		this->pRoom->DestroyTrapdoor(this->swordsman.wX - dx,
				this->swordsman.wY - dy, CueEvents);

	//Check for stepping on monster
	CMonster* pMonster = this->pRoom->GetMonsterAtSquare(this->swordsman.wX, this->swordsman.wY);
	if (pMonster)
	{
		if (pMonster->wType == M_FLUFFBABY)
		{
			this->pRoom->KillMonster(pMonster,CueEvents,false,&this->swordsman);
			this->pRoom->ProcessPuffAttack(CueEvents, this->swordsman.wX, this->swordsman.wY);
		}
		else if (pMonster->wType == M_FEGUNDOASHES ||  //fegundo ashes
			this->swordsman.CanStepOnMonsters() ||  //player in monster-role attacked another monster
			this->swordsman.CanDaggerStep(pMonster, true))  //player stabbed with a dagger
		{
			CueEvents.Add(CID_MonsterDiedFromStab, pMonster);
			this->pRoom->KillMonster(pMonster, CueEvents, false, &this->swordsman);
			CheckTallyKill(pMonster);
			pMonster->wO = this->swordsman.wO; //splatter in this direction
			if (!pMonster->IsFriendly())
				SetPlayerAsTarget();  //monsters will attack the player now
		}
	}

	//Teleportation tunnels: handle move before sword hit.
	if (bEnteredTunnel)
	{
		CueEvents.Add(CID_Tunnel);
		TunnelMove(dx,dy);
	}

	if (bIsMovementCommand(nCommand) && (this->swordsman.wX != this->swordsman.wPrevX || 
		this->swordsman.wY != this->swordsman.wPrevY))
	{
		QueryCheckpoint(CueEvents, this->swordsman.wX, this->swordsman.wY);

	}

	ProcessPlayerMoveInteraction(dx, dy, CueEvents, bWasOnSameScroll, true);


	//Check for o-square things swordsman can step onto.
	switch (wNewOSquare)
	{
		case T_STAIRS:       //Level exit.
		case T_STAIRS_UP:
			ProcessPlayer_HandleLeaveLevel(CueEvents);
		break;
		case T_HOT:
			if (bStayedOnHotFloor && this->swordsman.wX == wStartingX && this->swordsman.wY == wStartingY)
			{
				//Player dies if on same hot tile two (non-hasted) turns in a row.
				if ((!this->swordsman.bIsHasted || this->bWaitedOnHotFloorLastTurn) &&
						bIsEntityTypeVulnerableToHeat(this->swordsman.wAppearance))
				{
					SetDyingEntity(&this->swordsman);
					CueEvents.Add(CID_PlayerBurned);
				}
				this->bWaitedOnHotFloorLastTurn = true;
			} else {
				//Just moved onto this tile.
				this->bWaitedOnHotFloorLastTurn = false;
			}
		break;
		case T_GOO:
			if (this->wTurnNo > 0 && this->swordsman.wAppearance == M_CONSTRUCT)
			{
				SetDyingEntity(&this->swordsman);
				CueEvents.Add(CID_PlayerEatenByOremites);
			}
		break;
		default: break;
	}
}

//***************************************************************************************
void CCurrentGame::ProcessPlayerMoveInteraction(int dx, int dy, CCueEvents& CueEvents,
	const bool bWasOnSameScroll, const bool bPlayerMove, const bool bPlayerTeleported)
{
	const bool bMoved = dx!=0 || dy!=0 || bPlayerTeleported;
	const bool bSmitemaster = bIsSmitemaster(this->swordsman.wAppearance);
	const bool bCanGetItems = this->swordsman.CanLightFuses();
	const UINT wOSquare = this->pRoom->GetOSquare(this->swordsman.wX, this->swordsman.wY);
	const UINT wTSquare = this->pRoom->GetTSquare(this->swordsman.wX, this->swordsman.wY);

	if (wOSquare == T_PRESSPLATE && bMoved && bCanEntityPressPressurePlates(this->swordsman.wAppearance))
		this->pRoom->ActivateOrb(this->swordsman.wX, this->swordsman.wY, CueEvents, OAT_PressurePlate);

	//Check for reading a scroll.
	if (!bWasOnSameScroll && wTSquare == T_SCROLL)
	{
		CDbMessageText *pScrollText = new CDbMessageText();
		const WSTRING wstr = GetScrollTextAt(this->swordsman.wX, this->swordsman.wY);
		*pScrollText = wstr.c_str();
		ASSERT((const WCHAR *)(*pScrollText)); //On assertion, room data is probably stored incorrectly.
		if (CueEvents.HasOccurred(CID_StepOnScroll)) //Remove any data from a scroll the player had previously stepped on.
			CueEvents.ClearEvent(CID_StepOnScroll);
		CueEvents.Add(CID_StepOnScroll, pScrollText, true);
	}

	//Check for t-square things player can step onto.
	//Only Beethro or a power-tokened monster/NPC role can use some of these items.
	switch (wTSquare)
	{
		case T_POTION_K:  //Mimic potion.
			if (bPlayerMove && bSmitemaster && this->swordsman.wPlacingDoubleType == 0)
				DrankPotion(CueEvents, M_MIMIC, this->swordsman.wX, this->swordsman.wY);
		break;

		case T_POTION_D:  //Decoy potion.
			if (bPlayerMove && bSmitemaster && this->swordsman.wPlacingDoubleType == 0)
				DrankPotion(CueEvents, M_DECOY, this->swordsman.wX, this->swordsman.wY);
		break;

		case T_POTION_C:  //Clone potion.
			if (bPlayerMove && this->swordsman.CanLightFuses() && this->swordsman.wPlacingDoubleType == 0)
				DrankPotion(CueEvents, M_CLONE, this->swordsman.wX, this->swordsman.wY);
		break;

		case T_POTION_I:  //Invisibility potion.
			if (bPlayerMove && bCanGetItems)
			{
				this->swordsman.bIsInvisible = !this->swordsman.bIsInvisible;   //Toggle effect.
				this->pRoom->Plot(this->swordsman.wX, this->swordsman.wY, T_EMPTY);
				CueEvents.Add(CID_DrankPotion);
				ResetPendingTemporalSplit(CueEvents);
			}
		break;

		case T_POTION_SP:  //Speed potion.
			if (bPlayerMove && bCanGetItems)
			{
				this->swordsman.bIsHasted = !this->swordsman.bIsHasted;  //Toggle effect.
				this->pRoom->Plot(this->swordsman.wX, this->swordsman.wY, T_EMPTY);
				CueEvents.Add(CID_DrankPotion);
				ResetPendingTemporalSplit(CueEvents);
			}
		break;

		case T_HORN_SQUAD:    //Squad horn.
			if (bPlayerMove && this->swordsman.CanLightFuses())  //same condition as clone potion..
				BlowHorn(CueEvents, M_CLONE, this->swordsman.wX, this->swordsman.wY);
		break;

		case T_HORN_SOLDIER:  //Soldier horn.
			if (bPlayerMove && (bIsMonsterTarget(this->swordsman.wAppearance) || this->swordsman.bCanGetItems))
				BlowHorn(CueEvents, M_STALWART2, this->swordsman.wX, this->swordsman.wY);
		break;

		case T_TOKEN:
			//Room token.  Activated by any player type.
			if (bMoved || !this->wTurnNo)
				this->pRoom->ActivateToken(CueEvents, this->swordsman.wX, this->swordsman.wY);
		break;

		default:        //normal step (appropriate type of footfall)
			if (bMoved && bPlayerMove && !bIsEntityFlying(this->swordsman.wAppearance)) {
				if (bIsWater(wOSquare))
					CueEvents.Add(CID_Wade, new CCoord(this->swordsman.wX - dx, this->swordsman.wY - dy));
				else
					CueEvents.Add(CID_Step);
			}
		break;
	}

	ProcessPlayerWeapon(dx, dy, CueEvents);

	//T-layer item-grabbing/activation that happens after sword hits for 2.0 backwards compatibility.
	//EX: Player hits an orb, raising a door, breaking a fuse, allowing him to light it immediately.
	switch (wTSquare)
	{
		case T_FUSE:
			//Light the fuse.
			if (bCanGetItems)
				this->pRoom->LightFuseEnd(CueEvents, this->swordsman.wX, this->swordsman.wY);
		break;
	}
}

//***************************************************************************************
bool CCurrentGame::PlayerEnteredTunnel(const UINT wOTileNo, const UINT wMoveO, UINT wRole) const
{
	//Entity enters tunnel when moving off of a tunnel in its entrance direction.
	if (!bIsTunnel(wOTileNo))
		return false;
	
	//As default, use player's identity
	if (wRole == M_NONE)
		wRole = this->swordsman.wAppearance;

	const bool bCanEnterTunnel = bIsMonsterTarget(wRole) || this->swordsman.bCanGetItems;
	if (bCanEnterTunnel)
	{
		switch (wOTileNo)
		{
			case T_TUNNEL_N: return wMoveO == N;
			case T_TUNNEL_S: return wMoveO == S;
			case T_TUNNEL_E: return wMoveO == E;
			case T_TUNNEL_W: return wMoveO == W;
			default: ASSERT(!"Unrecognized tunnel type"); return false;
		}
	}
	return false;
}

//***************************************************************************************
void CCurrentGame::ProcessPlayer_HandleLeaveLevel(
//This is a hunk of code yanked out of ProcessPlayer() for readability.
//
//Ending state:      The game will be inactive.  A cue event of either CID_WinGame or
//             CID_ExitLevelPending will have been added.
//Side effects:      Demos may be saved.
//
//Params:
	CCueEvents &CueEvents,  //(out)  Events added to it.
	const LevelExit& exit,  //specifies destination [default=StairLookup]
	const bool bSkipEntranceDisplay) //[default=false]
{
	ASSERT(exit.type == LevelExit::SpecificID ||
		bIsStairs(this->pRoom->GetOSquare(this->swordsman.wX, this->swordsman.wY)));

	//If a critical character died on exit move, the exit doesn't count.
	if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied))
		return;

	if (this->wPlayerTurn == 0)
		return;

	this->bIsLeavingLevel = true;

	//Write a demo record if recording.
	if (this->bIsDemoRecording)
	{
		const UINT dwDemoID = WriteCurrentRoomDemo(this->DemoRecInfo, false, false);

		//Set demo recording info to begin in a new room.
		this->DemoRecInfo.wBeginTurnNo = 0;
		this->DemoRecInfo.dwPrevDemoID = dwDemoID;
	}

	//Do things for a conquered room.
	const bool bConquered = WasRoomConqueredOnThisVisit();
	if (bConquered)
	{
		//Save a conquer demo if requested.
		if ( (this->dwAutoSaveOptions & ASO_CONQUERDEMO)==ASO_CONQUERDEMO )
			WriteCurrentRoomConquerDemo();

		//If player exited the room after killing monsters in the room, then
		//add a cue event and set room's official status to conquered.  For rooms
		//that have no monsters, this would already have happened on the player's
		//first step into the room.
		if (!IsCurrentRoomConquered())
		{
			CueEvents.Add(CID_ConquerRoom);
			SetCurrentRoomConquered();
		}

		//Mark completed rooms.
		AddRoomsToPlayerTally();

		//If a secret room was just conquered,
		//re-check hold mastery in case the last secret room was just conquered.
		if (this->pRoom->bIsSecret)
			UpdateHoldMastery(CueEvents);
	} else {
		if (this->pRoom->IsBeaconActive() && !this->bWasRoomConqueredAtTurnStart)
		{
			CDbSavedGame::ConqueredRooms -= this->pRoom->dwRoomID;
		}
	}

	//Now record that scripts were completed.
	AddCompletedScripts();

	UINT dwEntranceID=exit.entranceID; //default: 0=end hold if no destination is defined for stairs

	if (exit.type == LevelExit::StairLookup)
	{
		//Check for exits.
		for (UINT i=0; i<this->pRoom->Exits.size(); ++i)
		{
			CExitData *pStairs = this->pRoom->Exits[i];
			if (IsInRect(this->swordsman.wX, this->swordsman.wY,
					pStairs->wLeft, pStairs->wTop, pStairs->wRight, pStairs->wBottom))
			{
				//Robustness measure: verify the destination actually exists.
				if (pStairs->dwEntranceID) {
					if (LevelExit::IsWorldMapID(pStairs->dwEntranceID)) {
						if (!this->pHold->DoesWorldMapExist(
								LevelExit::ConvertWorldMapID(pStairs->dwEntranceID))) {
							//Dangling world map ID -- ignore it.
							continue;
						}
					} else if (!this->pHold->GetEntrance(pStairs->dwEntranceID)) {
						//Exit has dangling entrance ID -- ignore it and keep looking.
						continue;
					}
				}
				dwEntranceID = pStairs->dwEntranceID;
				break;
			}
		}
	}

	bool bEndHold = false;
	if (LevelExit::IsWorldMapID(dwEntranceID)) {
		if (this->pHold->DoesWorldMapExist(LevelExit::ConvertWorldMapID(dwEntranceID))) {
			CueEvents.Add(CID_ExitLevelPending,
					new CCoord(dwEntranceID, bSkipEntranceDisplay), true);
		} else {
			bEndHold = true;
		}
	} else {
		//If no exits, this signifies the end of the hold, i.e.,
		//If there are no levels to go to next, then the game has been won.
		CEntranceData *pEntrance = this->pHold->GetEntrance(dwEntranceID);
		if (pEntrance) {
			//Send back level to go to next.  ProcessCommand() caller
			//is charged with loading the next one if that is appropriate.
			CueEvents.Add(CID_ExitLevelPending,
					new CCoord(dwEntranceID, bSkipEntranceDisplay), true);
		} else {
			bEndHold = true;
		}
	}

	if (bEndHold) {
		//Stop any multi-room demo sequence being recorded.
		this->bIsDemoRecording = false;

		CueEvents.Add(CID_WinGame);
		if (!this->Commands.IsFrozen())
		{
			++this->dwLevelMoves; //Add move before saving.  Another move will be tallied after saving, but it is ignored
			SaveToEndHold();
		}
	}

	//A load is needed before more commands can be processed.
	this->bIsGameActive = false;

	UpdatePrevCoords();
}

//***************************************************************************************
bool CCurrentGame::ProcessPlayer_HandleLeaveRoom(
//This is a hunk of code yanked out of ProcessPlayer() for readability.
//
//Beginning state:   Player is standing on the edge of a room and user has
//             issued a command to move out of the room.
//Ending state:      If successful, player will either be positioned in the new room,
//             or the game will be inactive and ready for a new room load.
//Side effects:      Demos may be saved.
//             Cue events may be added.
//             Current room may be set to conquered.
//             Game may become inactive if this call is during demo playback.
//
//Params:
	const UINT wMoveO,      //(in)   Direction of movement that leaves the room.
	CCueEvents &CueEvents)  //(out)  Events added to it.
//
//Returns:
//True if player left the room successfully, false if player's departure was
//unsuccessful (ProcessPlayer() should keep player in current room).
{
	const UINT wExitDirection = GetRoomExitDirection(wMoveO);
	UINT dwNewSX, dwNewSY;
	CDbRoom *pNewRoom = NULL;
	if (!PlayerCanExitRoom(wExitDirection, dwNewSX, dwNewSY, pNewRoom))
	{
		delete pNewRoom;
		return false;
	}

	//If player does not have a sword, turn to face the way the room is being exited.
	const bool bChangeOrientation = !this->swordsman.HasWeapon() || (this->swordsman.GetActiveWeapon() == WT_Dagger);
	if (bChangeOrientation && wExitDirection != NO_ORIENTATION)
		this->swordsman.SetOrientation(wExitDirection);

	//Write a demo record if recording.
	if (this->bIsDemoRecording)
	{
		const UINT dwDemoID = WriteCurrentRoomDemo(this->DemoRecInfo, false, false);

		//Set demo recording info to begin in a new room.
		this->DemoRecInfo.wBeginTurnNo = 0;
		this->DemoRecInfo.dwPrevDemoID = dwDemoID;
	}

	//Do things for a conquered room.
	const bool bConquered = WasRoomConqueredOnThisVisit();
	if (bConquered)
	{
		//Save a conquer demo if requested.
		if ( (this->dwAutoSaveOptions & ASO_CONQUERDEMO)==ASO_CONQUERDEMO )
			WriteCurrentRoomConquerDemo();

		//If player exited the room after killing monsters in the room, then
		//add a cue event and set room's official status to conquered.  For rooms
		//that have no monsters, this would already have happened on the player's
		//first step into the room.
		bool bWasLevelComplete = IsCurrentLevelComplete();
		if (!IsCurrentRoomConquered())
		{
			CueEvents.Add(CID_ConquerRoom);
			SetCurrentRoomConquered();
		}

		//If all required rooms have now been conquered, then level is completed.
		if (IsCurrentLevelComplete())
		{
			if (!bWasLevelComplete) CueEvents.Add(CID_CompleteLevel);
			//Don't need to toggle blue doors for the room being left.
//			this->pRoom->ToggleTiles(T_DOOR_C, T_DOOR_CO); //blue door
		}
	} else {
		if (this->pRoom->IsBeaconActive())
		{
			CDbSavedGame::ConqueredRooms -= this->pRoom->dwRoomID;
		}
	}

	//If commands are frozen (demo playback), then don't load a new room right now.
	//Instead, add exit pending event and return.
	if (this->Commands.IsFrozen())
	{
		CueEvents.Add(CID_ExitRoomPending, new CAttachableWrapper<UINT>(wExitDirection), true);
		this->bIsGameActive = false;  //A load is needed before more commands
		                              //can be processed.
		delete pNewRoom;
		UpdatePrevCoords();     //fix characters at their current location
		return true;
	}

	//Now record that scripts were completed.
	AddCompletedScripts();

	const bool bRoomWasSecret = this->pRoom->bIsSecret;	//save value

	//Decide whether to record a saved game in the new room.
	bool bSaveRoom = false;
	if ((this->dwAutoSaveOptions & ASO_ROOMBEGIN)==ASO_ROOMBEGIN)
	{
		ASSERT(!this->Commands.IsFrozen());     //shouldn't be saving when demo playback is occurring

		bSaveRoom = ShouldSaveRoomBegin(pNewRoom->dwRoomID);
	}

	//Leave the room.
	LoadNewRoomForExit(dwNewSX, dwNewSY, pNewRoom, CueEvents, bSaveRoom);  //retain pNewRoom

	//If a secret room was just conquered,
	//re-check hold mastery in case the last secret room was just conquered.
	if (bRoomWasSecret && bConquered)
		UpdateHoldMastery(CueEvents);

	CueEvents.Add(CID_ExitRoom, new CAttachableWrapper<UINT>(wExitDirection), true);
	return true;
}

//***************************************************************************************
void CCurrentGame::ProcessUnansweredQuestions(
//Processes unanswered questions.  Game is in a state where it is waiting for player
//to answer one of more questions posed by monsters.  If multiple questions have
//been asked, then game will take an answer for the first question asked, then the second,
//etc.
//
//Params:
	const int nCommand,              //(in)      CMD_YES or CMD_NO, or answer #.
	list<CMonsterMessage> &UnansweredQuestions,  //(in/out)  Receives list of unanswered
												//       questions.  If command is
												//       a valid answer to the first,
												//       the the first question will
												//       be removed from list.
	CCueEvents &CueEvents)                 //(out)     Adds cue events as appropriate.
{
	list<CMonsterMessage>::const_iterator iQuestion = UnansweredQuestions.begin();
	switch (iQuestion->eType)
	{
		case MMT_YESNO:
			if (nCommand != CMD_YES && nCommand != CMD_NO)
			{
				//This command should not have been sent.  Possible causes:
				//- the room has changed since the demo was recorded
				//- a demo has been corrupted or misrecorded.
				//- the front end has been implemented incorrectly and sent a bad command.
				return;
			}

			//Send answer to question's sender.
			if (iQuestion->pSender && iQuestion->pSender->OnAnswer(nCommand, CueEvents))
			{
				//There should be some kind of shared cue event handler between this
				//and ProcessMonsters() or ProcessWeaponHit().  I happen to know that
				//the only relevant cue event right now is CID_MonsterDiedFromStab for
				//the 'Neather, but that could change.
				const CAttachableObject *pObj = CueEvents.GetFirstPrivateData(CID_MonsterDiedFromStab);
				while (pObj)
				{
					const CMonster *pStabbedMonster = DYN_CAST(const CMonster*, const CAttachableObject*, pObj);
					this->pRoom->KillMonster(const_cast<CMonster*>(pStabbedMonster), CueEvents);
					pObj = CueEvents.GetNextPrivateData();
				}
			}

			//Remove the question that was just answered from the list.
		break;
		case MMT_MENU:
			if (iQuestion->pSender)
				iQuestion->pSender->OnAnswer(nCommand, CueEvents);
		break;
		default:
			ASSERT(!"Unexpected question type.");
		break;
	}
	UnansweredQuestions.pop_front();
}

//*****************************************************************************
void CCurrentGame::SaveExitedLevelStats()
//Saved stats for this level to packed vars.
{
	if (!this->pLevel)
		return;

	ASSERT(this->pLevel->dwLevelIndex);

	char levelIndex[10];
	char varName[20];
	_itoa(this->pLevel->dwLevelIndex, levelIndex, 10);

	strcpy(varName, levelIndex);
	strcat(varName, "d");
	this->stats.SetVar(varName, this->dwLevelDeaths);
	strcpy(varName, levelIndex);
	strcat(varName, "k");
	this->stats.SetVar(varName, this->dwLevelKills);
	strcpy(varName, levelIndex);
	strcat(varName, "m");
	this->stats.SetVar(varName, this->dwLevelMoves);
	strcpy(varName, levelIndex);
	strcat(varName, "t");
	this->stats.SetVar(varName, this->dwLevelTime);

	this->statsAtRoomStart = this->stats;
}

//***************************************************************************************
void CCurrentGame::SetMembers(const CCurrentGame &Src)
//Performs deep copy.
{
	CDbSavedGame::operator=(Src);

	ASSERT(Src.pHold);
	if (this->pHold)
		delete this->pHold;
	this->pHold = new CDbHold(*Src.pHold);
	ASSERT(Src.pLevel);
	if (this->pLevel)
		delete this->pLevel;
	this->pLevel = new CDbLevel(*Src.pLevel);
	ASSERT(Src.pRoom);
	if (this->pRoom)
		delete this->pRoom;
	this->pRoom = new CDbRoom(*Src.pRoom);

	this->wTurnNo = Src.wTurnNo; //set this before calling SetCurrentGame (for NPCs)
	this->pRoom->SetCurrentGame(this);

	ASSERT(Src.pEntrance);
	this->pEntrance = this->pHold->GetEntrance(Src.pEntrance->dwEntranceID);

	this->swordsman = Src.swordsman;

	this->bIsDemoRecording = Src.bIsDemoRecording;
	this->bIsLeavingLevel = Src.bIsLeavingLevel;
	this->bIsGameActive = Src.bIsGameActive;
	this->wPlayerTurn = Src.wPlayerTurn;
	this->wSpawnCycleCount = Src.wSpawnCycleCount;
	this->bHalfTurn = Src.bHalfTurn;
	this->bBrainSensesSwordsman = Src.bBrainSensesSwordsman;
	this->wLastCheckpointX = Src.wLastCheckpointX;
	this->wLastCheckpointY = Src.wLastCheckpointY;
	this->checkpointTurns = Src.checkpointTurns;
	this->dwStartTime = Src.dwStartTime;
	this->bHoldCompleted = Src.bHoldCompleted;
	this->bHoldMastered = Src.bHoldMastered;
	this->dwCutScene = Src.dwCutScene;
	this->bContinueCutScene = Src.bContinueCutScene;
	this->cutSceneStartTurn = Src.cutSceneStartTurn;
	this->music = Src.music;
	this->bMusicStyleFrozen = Src.bMusicStyleFrozen;
	this->bWaitedOnHotFloorLastTurn = Src.bWaitedOnHotFloorLastTurn;
	this->statsAtRoomStart = Src.statsAtRoomStart;
	this->ambientSounds = Src.ambientSounds;
	this->conquerTokenTurn = Src.conquerTokenTurn;
	this->bWasRoomConqueredAtTurnStart = Src.bWasRoomConqueredAtTurnStart;
	this->bIsLeavingLevel = Src.bIsLeavingLevel;

	//Speech log.
	vector<SpeechLog>::const_iterator iter;
	for (iter = this->roomSpeech.begin();	iter != this->roomSpeech.end(); ++iter)
		delete iter->pSpeechCommand;
	this->roomSpeech.clear();
	for (iter = Src.roomSpeech.begin();	iter != Src.roomSpeech.end(); ++iter)
	{
		CCharacterCommand& c = *iter->pSpeechCommand;
		this->roomSpeech.push_back(SpeechLog(iter->customName, new CCharacterCommand(c)));
	}

	//"swordsman exhausted/relieved" event logic
	memcpy(this->monstersKilled, Src.monstersKilled, TIRED_TURN_COUNT * sizeof(unsigned char));
	this->wMonstersKilledRecently = Src.wMonstersKilledRecently;
	this->bLotsOfMonstersKilled = Src.bLotsOfMonstersKilled;

	this->wMonsterKills = Src.wMonsterKills;
	this->wMonsterKillCombo = Src.wMonsterKillCombo;
	this->customRoomLocationText = Src.customRoomLocationText;
	this->displayFilter = Src.displayFilter;
	this->threatClockDisplay = Src.threatClockDisplay;
	this->playerLight = Src.playerLight;
	this->playerLightType = Src.playerLightType;
	this->persistingImageOverlays = Src.persistingImageOverlays;
	this->imageOverlayNextID = Src.imageOverlayNextID;
	this->scriptReturnX = Src.scriptReturnX;
	this->scriptReturnY = Src.scriptReturnY;
	this->scriptReturnF = Src.scriptReturnF;

	this->DemoRecInfo = Src.DemoRecInfo;
	this->UnansweredQuestions.clear();
	for (list<CMonsterMessage>::const_iterator uq = Src.UnansweredQuestions.begin();
			uq != Src.UnansweredQuestions.end(); ++uq)
	{
		CMonsterMessage mm(*uq);
		if (mm.pSender)
		{
			//Pointer to the source monster must be updated.
			CMonster *pMonster;
			if (mm.pSender->wType == M_CHARACTER)
			{
				//Find NPC with same ID.
				CCharacter *pSrcCharacter = DYN_CAST(CCharacter*, CMonster*, mm.pSender);
				const UINT scriptID = pSrcCharacter->dwScriptID;
				mm.pSender = NULL;
				for (pMonster = this->pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext)
				{
					if (pMonster->wType == M_CHARACTER)
					{
						CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
						if (pCharacter->dwScriptID == scriptID) //found a match
						{
							mm.pSender = pMonster;
							break;
						}
					}
				}
				ASSERT(mm.pSender);
			} else {
				//Otherwise, the only way to match them up is by position in the monster list.
				UINT wIndex=0;
				for (pMonster = Src.pRoom->pFirstMonster; pMonster != NULL;	pMonster = pMonster->pNext, ++wIndex)
					if (mm.pSender == pMonster)
						break;
				mm.pSender = NULL;
				if (pMonster)
				{
					for (pMonster = this->pRoom->pFirstMonster; pMonster != NULL; pMonster = pMonster->pNext, --wIndex)
						if (!wIndex)
						{
							mm.pSender = pMonster;
							break;
						}
					ASSERT(mm.pSender); //should have been assigned
				}
				//else if NULL, pSender was not found in the original monster list
			}
		}
		this->UnansweredQuestions.push_back(mm);
	}
	this->bIsNewRoom = Src.bIsNewRoom;
	this->bExecuteNoMoveCommands = Src.bExecuteNoMoveCommands;
	//this->wAddNewEntityDepth = Src.wAddNewEntityDepth; //not propagated across turns

	this->pDyingEntity = Src.pDyingEntity;
	this->pKillingEntity = Src.pKillingEntity;
	this->temporalSplit = Src.temporalSplit;
	this->activatingTemporalSplit = Src.activatingTemporalSplit;

	this->dwAutoSaveOptions = Src.dwAutoSaveOptions;
	this->CompletedScriptsPending = Src.CompletedScriptsPending;
	this->GlobalScriptsRunning = Src.GlobalScriptsRunning;
	this->bNoSaves = Src.bNoSaves;

	//skip copying the snapshot pointer
	this->dwComputationTime = Src.dwComputationTime;
	this->dwComputationTimePerSnapshot = Src.dwComputationTimePerSnapshot;
	this->numSnapshots = Src.numSnapshots;
}

//***************************************************************************************
void CCurrentGame::SetMembersAfterRoomLoad(
//Sets members of current game object that should be changed whenever
//a new room is loaded.
//
//Params:
	CCueEvents &CueEvents,  //(out)  Cue events generated by swordsman's first step
							//    into the room.
	const bool bResetCommands, //(in) [default = true]
	const bool bInitialEntrance) //(in) [default=true] certain operations should only be triggered on the room play's initial entrance, until exiting it
{
	ASSERT(this->pRoom);

	this->swordsman.ResetStats();

	this->bIsGameActive = true;
	this->wSpawnCycleCount = this->wPlayerTurn = this->wTurnNo = 0;
	this->bHalfTurn = false;
	this->bWaitedOnHotFloorLastTurn = false;
	this->conquerTokenTurn = NO_CONQUER_TOKEN_TURN;
	this->bWasRoomConqueredAtTurnStart = false;
	this->bIsLeavingLevel = false;

	this->wMonsterKills = this->wMonsterKillCombo = 0;

	memset(this->monstersKilled, 0, TIRED_TURN_COUNT * sizeof(unsigned char));
	this->wMonstersKilledRecently = 0;
	this->bLotsOfMonstersKilled = false;

	this->dwCutScene = 0;
	this->persistingImageOverlays.clear();
	this->imageOverlayNextID = 0;

	this->pRoom->SetCurrentGame(this);
	this->dwRoomID = this->pRoom->dwRoomID;

	this->dwPlayerID = g_pTheDB->GetPlayerID();

	//Update demo recording information.
	this->DemoRecInfo.wBeginTurnNo = 0;

	this->temporalSplit.clear();

	//Level is considered already complete (skipping CID_CompleteLevel) only
	//if the level isn't being entered for the first time.
	const bool bWasLevelComplete = IsCurrentLevelComplete() && !CDbSavedGame::ExploredRooms.empty();

	//If room has not been explored before, add it to explored list.
	this->bIsNewRoom = !IsCurrentRoomExplored();
	if (this->bIsNewRoom)
		SetCurrentRoomExplored();

	const bool bWasRoomConquered = IsCurrentRoomConquered();

	//Before removing anything on player entrance, init these stats.
	UINT wMonsterCountAtStart = this->pRoom->wMonsterCount;
	this->pRoom->bGreenDoorsOpened = false;

	if (this->swordsman.wAppearance != M_NONE)
	{
		//Remove a crumbly wall, bomb or monster underneath the swordsman if it exists.
		if (bIsCrumblyWall(this->pRoom->GetOSquare(this->swordsman.wX, this->swordsman.wY)) &&
			this->swordsman.wAppearance != M_SEEP) //seep can remain on the wall
			this->pRoom->DestroyCrumblyWall(this->swordsman.wX, this->swordsman.wY, CueEvents);
		const UINT tTile = this->pRoom->GetTSquare(this->swordsman.wX, this->swordsman.wY);
		if (tTile == T_BOMB || tTile == T_POWDER_KEG || bIsTLayerCoveringItem(tTile))
			this->pRoom->Plot(this->swordsman.wX, this->swordsman.wY, T_EMPTY);
		CCueEvents Ignored; //don't receive cues if this monster is not really there
		CMonster *pMonster = this->pRoom->GetMonsterAtSquare(this->swordsman.wX, this->swordsman.wY);
		if (pMonster)
		{
			switch (pMonster->wType)
			{
				//Remove critical types from monster list w/o recording their "kill".
			case M_CLONE: case M_BEETHRO: case M_GUNTHRO:
			case M_HALPH: case M_HALPH2:
			case M_TEMPORALCLONE:
				this->pRoom->KillMonsterAtSquare(this->swordsman.wX, this->swordsman.wY,
					Ignored, true);
				break;

			default:
				this->pRoom->KillMonsterAtSquare(this->swordsman.wX, this->swordsman.wY,
					bWasRoomConquered ? Ignored : CueEvents);
				break;
			}
		}
	}

	//Remove any monster messages left unprocessed.
	this->UnansweredQuestions.clear();

	this->customRoomLocationText.clear();
	this->displayFilter = ScriptFlag::D_Normal;
	this->threatClockDisplay = 0;
	this->playerLight = this->playerLightType = 0;
	this->scriptReturnX = this->scriptReturnY = this->scriptReturnF = 0;
	ResetCutSceneStartTurn();

	this->music.reset();
	this->bMusicStyleFrozen = false;

	//Flag if secret room entered.
	if (this->bIsNewRoom && this->pRoom->bIsSecret)
		CueEvents.Add(CID_SecretRoomFound);

	if (!bWasRoomConquered && !wMonsterCountAtStart &&
			!this->pRoom->bHasConquerToken && !this->pRoom->bHasActiveBeacon)
	{
		SetCurrentRoomConquered();

		//When restoring game to the the first-time entry of a clean room,
		//ensure this flag is set to allow resaving victory demo on room exit.
		this->bIsNewRoom = true;
	}

	//Setup starting Global Scripts
	this->GlobalScriptsRunning = this->GlobalScripts;

	this->pRoom->SetRoomEntryState(CueEvents, bWasLevelComplete, IsCurrentLevelComplete(),
			bWasRoomConquered, wMonsterCountAtStart);

	if (!this->swordsman.IsInRoom() && !this->bSwordsmanOutsideRoom)
		SetPlayerRole(M_BEETHRO, CueEvents); //place player in room now as default (Beethro)

	//Init NPCs after initial room state checks and modifications are performed.
	this->bExecuteNoMoveCommands = true;
	this->pRoom->PreprocessMonsters(CueEvents);
	this->bExecuteNoMoveCommands = false;

	//Process the swordsman's movement onto the first square.
	if (this->swordsman.IsInRoom() && !this->swordsman.bHasTeleported)
		ProcessPlayer(CMD_WAIT, CueEvents);

	//Any fuses lit on room entrance will burn on the first full turn.
	this->pRoom->LitFuses += this->pRoom->NewFuses;
	this->pRoom->NewFuses.clear();

	ResolveSimultaneousTarstuffStabs(CueEvents);
	if (this->pRoom->bTarWasBuilt)
	{
		this->pRoom->bTarWasBuilt = false;
		this->pRoom->BreakUnstableTar(CueEvents);
	}

	this->pRoom->KillMonstersOnHazard(CueEvents); //check again if doors have changed
	SetPlayerMood(CueEvents);
	this->pRoom->ResetMonsterFirstTurnFlags();
	//Check whether swordsman's first step left room in a conquer-pending state.
	if ((wMonsterCountAtStart || this->pRoom->bHasConquerToken || this->pRoom->bHasActiveBeacon) &&
			IsCurrentRoomPendingExit()) //room needs conquering
	{
		ToggleGreenDoors(CueEvents);
		//Don't mark room conquered until player exits the room.
	}

	if (bResetCommands && !this->Commands.IsFrozen())
		this->Commands.Clear();

	//Player should always be visible if no cut scene is playing.
	if (!this->swordsman.IsInRoom() && !this->dwCutScene)
		SetPlayerRole(M_BEETHRO, CueEvents); //place player in room now as default (Beethro)

	//Setup PathMap to player for monsters that require it.
	this->pRoom->CreatePathMaps();

	//Player should never die on entering room.
	ASSERT(!CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied),CIDA_PlayerDied));

	//Characters can catch events occurring on room entrance.
	this->pRoom->CharactersCheckForCueEvents(CueEvents);

	if (bInitialEntrance)
	{
		AddRoomsToPlayerTally();

		//Check for Hold Mastered on entering a cleared-on-entrance secret room
		if (this->pRoom->bIsSecret && IsCurrentRoomPendingConquer())
			UpdateHoldMastery(CueEvents);

		if (CueEvents.HasOccurred(CID_ChallengeCompleted) && !this->Commands.IsFrozen() && !this->bNoSaves)
			FlagChallengesCompleted(CueEvents);
	}

	//Reset ambient sounds and speech.
	this->ambientSounds.clear();
	for (vector<SpeechLog>::const_iterator iter = this->roomSpeech.begin();
			iter != this->roomSpeech.end(); ++iter)
		delete iter->pSpeechCommand;
	this->roomSpeech.clear();

	StashPersistingEvents(CueEvents);
}

//*****************************************************************************
//Call to set whether cue events fired on this turn will affect the music mood (front-end)
void CCurrentGame::SetMusicStyle(const MusicData& newMusic, CCueEvents& CueEvents)
{
	if (!newMusic.songlistKey.empty()) {
		this->bMusicStyleFrozen = true;
	} else {
		if (newMusic.trackID == UINT(MusicData::SONGID_DEFAULT)) {
			if (!this->bMusicStyleFrozen)
				return; //already default -- no change

			this->bMusicStyleFrozen = false;
		} else {
			this->bMusicStyleFrozen = true;
		}
	}
	CueEvents.Add(CID_SetMusic);

	this->music = newMusic;
}

//*****************************************************************************
void CCurrentGame::SetPlayerMood(
//Determine player's mood, according to relative position of monsters.
//Scared overrides aggressive, aggressive overrides tired/normal mood.
//
//Params:
	CCueEvents &CueEvents)  //(out)  List of events that can be handled by caller.
{
	CMonster *pMonster;
	UINT x, y;

	//Scared?
	//(whether aggressive monster is behind Beethro)
	if (bIsSmitemaster(this->swordsman.wAppearance))
	{
		const UINT wX = this->swordsman.wX, wY = this->swordsman.wY,   //shorthand
				wO = this->swordsman.wO;
		const int oX = nGetOX(wO);
		const int oY = nGetOY(wO);
		CCoordStack checkSquares;

		//Determine which squares to check.
		if (wX-oX < this->pRoom->wRoomCols) //avoid assertion
			checkSquares.Push(wX-oX,wY-oY);  //check directly behind swordsman
		//check behind on left/right sides
		if (oX && oY)  //swordsman is facing diagonally
		{
			checkSquares.Push(wX-oX,wY);
			checkSquares.Push(wX,wY-oY);
		} else { //swordsman is facing straight
			if (oX && wX-oX < this->pRoom->wRoomCols) //avoid assertion
			{
				checkSquares.Push(wX-oX,wY-1);
				checkSquares.Push(wX-oX,wY+1);
			}
			else if (wY-oY < this->pRoom->wRoomRows)  //avoid assertion
			{  //oY
				checkSquares.Push(wX-1,wY-oY);
				checkSquares.Push(wX+1,wY-oY);
			}
		}

		//Check squares for monster.
		while (checkSquares.Pop(x,y)) {
			if (this->pRoom->IsValidColRow(x,y))
			{
				pMonster = this->pRoom->GetMonsterAtSquare(x,y);
				if (this->pRoom->MonsterHeadIsAt(x,y) && pMonster->IsAggressive())
				{
					CueEvents.Add(CID_SwordsmanAfraid);
					while (checkSquares.GetSize())   //remove remaining squares
						checkSquares.Pop(x,y);
					return;  //don't check for other moods
				}
			}
		}
	}

	//Aggressive?
	//(whether weapon is adjacent to a non-friendly monster)
	if (this->swordsman.HasWeapon())
	{
		const UINT wSX = this->swordsman.wSwordX, wSY = this->swordsman.wSwordY;   //shorthand
		for (x=wSX-1; x<=wSX+1; ++x)
			for (y=wSY-1; y<=wSY+1; ++y)
			{
				if (x==wSX && y==wSY) continue;
				if (this->pRoom->IsValidColRow(x,y))
				{
					pMonster = this->pRoom->GetMonsterAtSquare(x,y);
					if (pMonster && !pMonster->IsFriendly())
					{
						CueEvents.Add(CID_SwordsmanAggressive);
						return;  //don't check for other moods
					}
				}
			}
	}

	//Tired?
	if (IsSwordsmanTired())
		CueEvents.Add(CID_SwordsmanTired);

	CueEvents.Add(CID_SwordsmanNormal);
}

//***************************************************************************************
void CCurrentGame::SetPlayerToRoomStart()
//Sets player-related members to start of room.
{
	CCueEvents Ignored;

	//First, prepare vars for recording saved games.
	this->bIsGameActive = true;
	this->wTurnNo = 0;
	this->dwRoomID = this->pRoom->dwRoomID;
	this->dwPlayerID = g_pTheDB->GetPlayerID();

	//Do this second -- it depends on the vars above being set first.
	SetPlayerRole(this->wStartRoomAppearance, Ignored);

	this->swordsman.bWeaponOff = this->bStartRoomSwordOff;
	this->swordsman.wWaterTraversal = this->wStartRoomWaterTraversal;
	this->swordsman.SetWeaponType(this->wStartRoomWeaponType);
	this->swordsman.SetOrientation(this->wStartRoomO);
	SetPlayer(this->wStartRoomX, this->wStartRoomY);
	this->wLastCheckpointX = this->wLastCheckpointY = NO_CHECKPOINT;
	this->checkpointTurns.clear();
	this->CompletedScriptsPending.clear();
	this->stats = this->statsAtRoomStart;

	//There should be no commands at the beginning of the room, unless
	//the command sequence is being replayed.
	if (!this->Commands.IsFrozen())
		this->Commands.Clear();

	//No death-related entities should exist when restarting room.
	this->pDyingEntity = NULL;
	this->pKillingEntity = NULL;

	//No room play snapshots should exist when restarting room.
	delete this->pSnapshotGame;
	this->pSnapshotGame = NULL;
	this->dwComputationTime = 0;
	this->numSnapshots = 0;
}

//*****************************************************************************
bool CCurrentGame::SetRoomAtCoords(
//Loads a room on this level and sets the current room to it.
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY) //(in)   Coords to specify room to load.
//
//Returns:
//True if successful, false if not.  If a failure occurs, the current room will
//stay loaded.
{
	//Load new room.
	CDbRoom *pNewRoom = this->pLevel->GetRoomAtCoords(dwRoomX, dwRoomY);
	if (!pNewRoom)
		return false;

	//Free the old room and return successful.
	delete this->pRoom;
	this->pRoom = pNewRoom;
	return true;
}

//***************************************************************************************
void CCurrentGame::SetRoomStartToPlayer()
//Opposite of SetPlayerToRoomStart().
{
	this->wStartRoomAppearance = this->swordsman.wIdentity;
	this->bStartRoomSwordOff = this->swordsman.bWeaponOff;
	this->wStartRoomWaterTraversal = this->swordsman.wWaterTraversal;
	this->wStartRoomWeaponType = this->swordsman.weaponType;

	this->wStartRoomO = this->swordsman.wO;
	this->wStartRoomX = this->swordsman.wX;
	this->wStartRoomY = this->swordsman.wY;

	this->statsAtRoomStart = this->stats;
	this->checkpointTurns.clear();
}

//*****************************************************************************
void CCurrentGame::StashPersistingEvents(CCueEvents& CueEvents)
//Used on move sequence replay to display effects that should persist in the room indefinitely
//Effects are added/cleared in the order they are cued.
{
	AmbientSoundTracking(CueEvents);

	const CAttachableObject *pObj = CueEvents.GetFirstPrivateData(CID_ImageOverlay);
	while (pObj)
	{
		const CImageOverlay *pImageOverlay = DYN_CAST(const CImageOverlay*, const CAttachableObject*, pObj);
		if (pImageOverlay->loopsForever())
			this->persistingImageOverlays.push_back(*pImageOverlay);
		const int clearLayer = pImageOverlay->clearsImageOverlays();
		RemoveClearedImageOverlays(clearLayer);

		pObj = CueEvents.GetNextPrivateData();
	}
}

//*****************************************************************************
void CCurrentGame::RemoveClearedImageOverlays(const int clearLayers)
{
	if (clearLayers == ImageOverlayCommand::NO_LAYERS)
		return;

	if (clearLayers == ImageOverlayCommand::ALL_LAYERS) {
		this->persistingImageOverlays.clear();
		return;
	}

	vector<CImageOverlay> keptImages;

	for (vector<CImageOverlay>::const_iterator imageIt=this->persistingImageOverlays.begin();
			imageIt!=this->persistingImageOverlays.end(); ++imageIt)
	{
		const CImageOverlay& image = *imageIt;
		if (clearLayers != image.getLayer())
			keptImages.push_back(image);
	}

	this->persistingImageOverlays = keptImages;
}

//*****************************************************************************
void CCurrentGame::CheckTallyKill(const CMonster* pMonster)
{
	switch (pMonster->wType) {
		case M_CHARACTER:
		{
			const CCharacter *pCharacter = DYN_CAST(const CCharacter*, const CMonster*, pMonster);
			if (pCharacter->IsRequiredToConquer())
				TallyKill();
		}
		break;
		case M_ROCKGOLEM: case M_CONSTRUCT:
			if (pMonster->IsAlive())
				TallyKill();
		break;
		default:
			CheckTallyKill(pMonster->wType);
		break;
	}
}

void CCurrentGame::CheckTallyKill(const UINT wType)
//Add one to kill count for current room and level.
{
	//These monster types count as kills.
	switch (wType)
	{
		case M_AUMTLICH:
		case M_BRAIN:
		case M_EYE:
		case M_GELBABY: case M_GELMOTHER:
		case M_GOBLIN:
		case M_GUARD:
		case M_MUDBABY: case M_MUDMOTHER:
		case M_NEATHER:
		case M_QROACH: case M_REGG: case M_ROACH:
		case M_ROCKGIANT: case M_ROCKGOLEM: case M_CONSTRUCT:
		case M_SEEP:
		case M_SKIPPERNEST:
		case M_SLAYER: case M_SLAYER2:
		case M_SERPENT: case M_SERPENTB: case M_SERPENTG:
		case M_SPIDER:
		case M_TARBABY: case M_TARMOTHER:
		case M_WATERSKIPPER:
		case M_WWING:
		case M_GENTRYII:
			TallyKill();
		break;

		default: break; //other types don't count as a kill
	}
}

void CCurrentGame::TallyKill()
{
	++this->wMonsterKills;
	++this->wMonsterKillCombo;

	if (!this->Commands.IsFrozen())
		++this->dwLevelKills;
}

//*****************************************************************************
bool cloneComparator::operator() (const CClone *a, const CClone *b) const
//Compare clones by wCreationIndex
{
    return a->wCreationIndex < b->wCreationIndex;
}

//*****************************************************************************
bool CCurrentGame::SwitchToCloneAt(const UINT wX, const UINT wY)
//Switch player and clone's positions.
{
	ASSERT(this->pRoom->IsValidColRow(wX,wY));
	CMonster *pMonster = this->pRoom->GetMonsterAtSquare(wX, wY);

	if (!pMonster) //robustness checks
		return false;
	if (pMonster->wType != M_CLONE)
		return false;

	if (this->pRoom->GetMonsterAtSquare(this->swordsman.wX, this->swordsman.wY))
		return false; //monster on player's location could be a temporal clone

	const UINT wO = pMonster->wO;
	pMonster->Move(this->swordsman.wX, this->swordsman.wY);
	pMonster->wO = this->swordsman.wO;

	CClone *pClone = DYN_CAST(CClone*, CMonster*, pMonster);
	const UINT wSwappedCreationIndex = pClone->wCreationIndex;
	pClone->wCreationIndex = this->pRoom->wLastCloneIndex;
	this->pRoom->wLastCloneIndex = wSwappedCreationIndex;
	const WeaponType playerWeapon = this->swordsman.GetActiveWeapon();
	this->swordsman.SetWeaponType(pClone->weaponType, false);
	pClone->weaponType = playerWeapon;
	pClone->SetWeaponSheathed();

	const bool bCloneWaitedOnHotFloorLastTurn = pClone->bWaitedOnHotFloorLastTurn;
	pClone->bWaitedOnHotFloorLastTurn = this->bWaitedOnHotFloorLastTurn;
	this->bWaitedOnHotFloorLastTurn = bCloneWaitedOnHotFloorLastTurn;
	this->swordsman.Move(wX,wY);
	this->swordsman.SetOrientation(wO);
	SetPlayerWeaponSheathedState();

	//Don't show swapping movement.
	UpdatePrevCoords();

	//Brain pathmap target is now at this location.
	if (this->swordsman.IsTarget())
		this->pRoom->SetPathMapsTarget(wX,wY);

	// Sort the clones by creationIndex and reposition them
	// We do the weird positiong swapping instead of unlink/relink to preserve movement order
	// of every other monster
	{
		std::vector<CClone*> clones;
		pMonster = this->pRoom->GetMonsterOfType(M_CLONE, true);
		while (pMonster && pMonster->wProcessSequence == SPD_PDOUBLE) {
			if (pMonster->wType == M_CLONE) {
				pClone = DYN_CAST(CClone*, CMonster*, pMonster);
				clones.push_back(pClone);
				this->pRoom->RemoveMonsterFromTileArray(pClone);
			}
			pMonster = pMonster->pNext;
		}

		std::vector<CClone*> sortedClones;
		for (UINT i = 0; i < clones.size(); ++i) {
			CClone *clone = clones[i];
			CClone *cloneCopy = DYN_CAST(CClone*, CMonster*, clone->Clone());
			sortedClones.push_back(cloneCopy);
		}

		std::sort(sortedClones.begin(), sortedClones.end(), cloneComparator());

		ASSERT(sortedClones.size() == clones.size());
		for (UINT i = 0; i < clones.size(); ++i) {
			CClone *existingClone = clones[i];
			CClone *sortedClone = sortedClones[i];

			// This unfortunately has to be updated each time a new property is added to
			// CClone or any of its parent classes, which state is important
			existingClone->wPrevX = existingClone->wX = sortedClone->wX;
			existingClone->wPrevY = existingClone->wY = sortedClone->wY;
			existingClone->wPrevO = existingClone->wO = sortedClone->wO;
			existingClone->wCreationIndex = sortedClone->wCreationIndex;
			existingClone->bPushedThisTurn = sortedClone->bPushedThisTurn;
			existingClone->bWaitedOnHotFloorLastTurn = sortedClone->bWaitedOnHotFloorLastTurn;
			existingClone->stunned = sortedClone->stunned;
			existingClone->bNewStun = sortedClone->bNewStun;
			existingClone->weaponType = sortedClone->weaponType;
			existingClone->bWeaponSheathed = sortedClone->bWeaponSheathed;
			existingClone->bNoWeapon = sortedClone->bNoWeapon;
			existingClone->wSwordMovement = sortedClone->wSwordMovement;
			existingClone->bFrozen = sortedClone->bFrozen;

			this->pRoom->SetMonsterSquare(existingClone);

			delete sortedClone;
		}
	}

	//Currently, this command can happen without anything else changing.
	//Q: How to handle invisible player swapping with clone?
	//A: Currently, player retains invisibility after transfer (no code required for this).

	return true;
}

//*****************************************************************************
bool CCurrentGame::TakeSnapshotNow() const
//Returns: whether a new game state snapshot should be taken and stored
{
	if (!this->bIsGameActive) //don't record snapshot if play has ended
		return false;

	//Take snapshot if enough CPU time to process turns has elapsed.
	if (this->dwComputationTime < this->dwComputationTimePerSnapshot)
		return false;

	//Don't take snapshots so often if turns appear resource intensive.
	static const UINT MIN_TURNS_PER_SNAPSHOT = 15;
	const UINT turnsSinceLastSnapshot = this->wTurnNo -
			(this->pSnapshotGame ? this->pSnapshotGame->wTurnNo : 0);
	if (turnsSinceLastSnapshot < MIN_TURNS_PER_SNAPSHOT)
		return false;

	//Don't store more than this many snapshots to avoid sucking up too much
	//memory in pathological room environments.
	static const UINT MAX_SNAPSHOTS = 60;
	if (this->numSnapshots > MAX_SNAPSHOTS)
		return false;

	return true;
}

//*****************************************************************************
bool CCurrentGame::ToggleGreenDoors(CCueEvents &CueEvents)
//Called when a room is cleared.
//Returns: whether any green doors were toggled
{
	ASSERT(this->pRoom);
	return this->pRoom->ToggleGreenDoors(CueEvents);
}

//*****************************************************************************
bool CCurrentGame::TunnelMove(
//Move player to exit of tunnel entered.
//Search for an exit is performed axially along direction of tunnel entrance.
//
//Returns: whether tunnel was entered and another exited
//
//Params:
	const int dx, const int dy)   //(in) direction entering tunnel
{
	//If exit is found, then place player at destination.
	UINT wX, wY;
	if (!TunnelGetExit(this->swordsman.wX, this->swordsman.wY, dx, dy, wX, wY))
	{
		this->swordsman.wPrevX = this->swordsman.wX;
		this->swordsman.wPrevY = this->swordsman.wY;
		return false;
	}

	SetPlayer(wX, wY);
	return true;
}

//***************************************************************************************
void CCurrentGame::UpdateHoldMastery(CCueEvents& CueEvents)
{
	if (this->bHoldMastered)
		return; //nothing to do

	const UINT holdID = this->pHold->dwHoldID;
	this->bHoldMastered = g_pTheDB->Holds.IsHoldMastered(holdID, this->dwPlayerID); //already known

	//New mastery?
	if (!this->bHoldMastered) {
		this->bHoldMastered = g_pTheDB->Holds.ScanForNewHoldMastery(holdID, this->dwPlayerID, true); //no need to recheck for mastery save
		if (this->bHoldMastered) {
			CueEvents.Add(CID_HoldMastered);
			SaveToHoldMastered();
		}
	}
}

//***************************************************************************************
bool CCurrentGame::WasRoomConqueredOnThisVisit()
//Was the current room conquered on this visit?  This should be called before the
//player has left the room.
//
//Returns:
//True if room was conquered on this visit, false if not.
const
{
	ASSERT(this->bIsGameActive);

	// Level exit can be triggered by a scripting command and it's not fun for the player to be told right at this
	// time that the room was actually not conquered, because something in the room caused that to be.
	// Therefore we just assume that Go to level entrance keeps the room solved if it was solved when the turn started
	if (!this->bWasRoomConqueredAtTurnStart || !this->bIsLeavingLevel) {
		if (this->pRoom->wMonsterCount)
			return false;     //Room is still in an unconquered state.
		if (this->pRoom->bHasConquerToken && this->conquerTokenTurn == NO_CONQUER_TOKEN_TURN)
			return false;  //none of the room's conquer tokens were touched
		if (this->pRoom->IsBeaconActive())
			return false;  //An active beacon reseeds the room
	}

	//No monsters left in the room.
	return !IsCurrentRoomConquered() ||
			//If the room isn't already conquered, that means there were monsters
			//in the room when the player got here, and the player has conquered
			//the room by killing them all and leaving the room.

			this->bIsNewRoom;
			//If the room is already conquered and this is a new room, that means
			//there were no monsters when the player entered the room, so it was
			//immediately conquered upon entrance.
}

//***************************************************************************************
void CCurrentGame::UploadExploredRoom()
//Uploads the currently loaded room to CaravelNet to mark as explored
//Only sends the minimum necessary XML, and doesn't care if the server receives it or not
{
	const CDbPackedVars settings = g_pTheDB->GetCurrentPlayerSettings();
	const bool bSubmitScores = settings.GetVar(Settings::ConnectToInternet, false);
	if (bSubmitScores)
	{
		//Only upload explored rooms for published holds.
		ASSERT(g_pTheNet);
		if (g_pTheNet->IsLocalHold(this->pHold->dwHoldID) && g_pTheNet->IsLoggedIn())
		{
			//Prepare room for upload.
			CNetRoom room;
			room.FillFromRoom(this->dwRoomID);
			g_pTheNet->UploadExploredRoom(room);
		}
	}
}

//***************************************************************************************
UINT CCurrentGame::WriteCompletedChallengeDemo(
	const set<WSTRING>& challengesCompleted) //if set, overwrite a preexisting challenge demo in this room with these same challenges earned, or a subset
{
	return WriteCurrentRoomScoringDemo(CDbDemo::CompletedChallenge, challengesCompleted);
}

UINT CCurrentGame::WriteCurrentRoomConquerDemo()
{
	return WriteCurrentRoomScoringDemo(CDbDemo::Victory);
}

//***************************************************************************************
UINT CCurrentGame::WriteCurrentRoomScoringDemo(
	CDbDemo::DemoFlag flag,
	const set<WSTRING>& challengesCompleted) //if set, overwrite a preexisting challenge demo in this room with these same challenges earned, or a subset
//Returns: DemoID of new/overwritten Demos record.
{
	DEMO_REC_INFO dri;
	dri.SetFlag(flag);

	bool bHidden = false;
	UINT overwriteDemoID = 0;
	switch (flag) {
		case CDbDemo::Victory: dri.dwDescriptionMessageID = MID_ConquerDemoDescription; break;
		case CDbDemo::CompletedChallenge:
			dri.dwDescriptionMessageID = MID_ChallengeCompletedDemoDescription;
			if (!this->wTurnNo) {
				bHidden = true;
				ASSERT(!challengesCompleted.empty());
			}
			if (!challengesCompleted.empty())
				overwriteDemoID = g_pTheDB->Demos.FindByChallenges(
						this->pRoom->dwRoomID, challengesCompleted, true);
			break;
		default: break;
	}

	const UINT dwDemoID = WriteCurrentRoomDemo(dri, bHidden, true, overwriteDemoID);

	SubmitCurrentGameScoringDemo(dwDemoID, flag);

	return dwDemoID;
}

//***************************************************************************************
UINT CCurrentGame::WriteCurrentRoomDemo(
//Update database with demo information for current room.
//
//Note: Logic for updating demo descriptions in this method depends on this method
//being the only updater of demo descriptions.
//
//Params:
	DEMO_REC_INFO &dri,  //(in/out)  Receives with members set with info needed to
						//       record the demo.  Returns with with values that
						//       can be used in a subsequent call for a multi-room
						//       demo.
	const bool bHidden,     //(in)      Make the demo hidden?  Default is false.
	const bool bAppendRoomLocation,  //(in) Add room location to description (default = true)
	const UINT overwriteDemoID) //(in) [default=0 (none)]
//
//Returns:
//DemoID of new Demos record.
{
	ASSERT(dri.dwDescriptionMessageID);

	if (this->wTurnNo == dri.wBeginTurnNo) //could happen if room was exited immediately on entrance, or challenge demo
	{
		if (!CDbDemos::IsFlagSet(dri.dwFlags, CDbDemo::CompletedChallenge))
			return 0;
	}
	if (this->bNoSaves)
		return 0;

	bool bExploredOnEntrance;
	const bool bConqueredOnEntrance = SavePrep(bExploredOnEntrance);
	CDbPackedVars _stats = this->stats; //must retain what state game was in on entrance
	this->stats = this->statsAtRoomStart;

	//Either save to a new save slot or overwrite a preexisting one.
	this->dwSavedGameID = overwriteDemoID ? g_pTheDB->Demos.GetSavedGameIDofDemo(overwriteDemoID) : 0;
	this->eType = ST_Demo;
	this->bIsHidden = true;
	Update();
	ASSERT(this->dwSavedGameID);

	//Get a new demo and set its properties.
	CDbDemo *pDemo = new CDbDemo;
	pDemo->dwDemoID = overwriteDemoID;
	pDemo->bIsHidden = bHidden;
	pDemo->dwSavedGameID = this->dwSavedGameID;
	pDemo->wBeginTurnNo = dri.wBeginTurnNo;
	pDemo->wEndTurnNo = this->wTurnNo ? this->wTurnNo - 1 : 0;
	pDemo->dwFlags = dri.dwFlags;

	//Prepend room position to non-empty (player viewable) demo descriptions.
	const WCHAR *pText = g_pTheDB->GetMessageText(dri.dwDescriptionMessageID);
	WSTRING descText;
	if (bAppendRoomLocation && !dri.dwPrevDemoID)
	{
		descText = AbbrevRoomLocation();
		descText += wszColon;
		descText += wszSpace;
	}
	descText += pText;
	pDemo->DescriptionText = descText.c_str();

	if (dri.dwPrevDemoID)
	{
		WSTRING wstrNewDescription = (const WCHAR *)pDemo->DescriptionText;
		if (dri.dwPrevDemoID == dri.dwFirstDemoID) //2nd demo.
		{
			WSTRING wTmp;
			AsciiToUnicode( " (2)", wTmp );
			wstrNewDescription += wTmp;
		}
		else //3rd or later demo.
		{
			//Parse number from description text.
			WSTRING wstrNum;
			WSTRING::iterator iNumStart = wstrNewDescription.end();
			WSTRING::iterator iNumStop = wstrNewDescription.end();
			WSTRING wTmp;
			AsciiToUnicode("()", wTmp);
			WSTRING::iterator iSeek;
			for (iSeek = wstrNewDescription.end() - 1; iSeek != wstrNewDescription.begin(); --iSeek)
			{
				if ((WCHAR)*iSeek == wTmp[1]) {iNumStop = iSeek; continue;}
				if ((WCHAR)*iSeek == wTmp[0]) {iNumStart = iSeek + 1; break;}
			}
			for (iSeek = iNumStart; iSeek != iNumStop; ++iSeek)
				wstrNum += *iSeek;
			int nNum = _Wtoi(wstrNum.c_str());

			//Parsing error or previous call didn't append "(n)".
			ASSERT(nNum != 0);
			ASSERT((iNumStart != wstrNewDescription.end()) &&
					   (iNumStop != wstrNewDescription.end()));

			//Concat new description with incremented number.
			WCHAR wczNewNum[7];
			_itoW(nNum + 1, wczNewNum, 10);
			wstrNewDescription.replace(iNumStart, iNumStop, wczNewNum);
		}
		pDemo->DescriptionText = wstrNewDescription.c_str();
		dri.dwDescriptionMessageID = pDemo->DescriptionText.UpdateText();
	}
	pDemo->dwChecksum = GetChecksum();

	//Update the demo.
	pDemo->Update();
	const UINT dwNewDemoID = pDemo->dwDemoID;
	delete pDemo;
	ASSERT(dwNewDemoID);

	this->stats = _stats;
	PostSave(bConqueredOnEntrance, bExploredOnEntrance);

	//If recording began in a previous room, update the demo record for that room
	//so that its NextDemoID field will point to this new demo record.
	if (dri.dwPrevDemoID)
	{
		CDbDemo *pPrevDemo = g_pTheDB->Demos.GetByID(dri.dwPrevDemoID);
		ASSERT(pPrevDemo); //If fires, then probably a bad value in dwPrevDemoID.
		ASSERT(pPrevDemo->dwNextDemoID == 0L);
		pPrevDemo->dwNextDemoID = dwNewDemoID;
		if (pPrevDemo->dwDemoID == dri.dwFirstDemoID) //First demo.
		{
			//Add "(1)" to description to indicate multi-room demo.
			WSTRING wstrNewDescription = (const WCHAR *)pPrevDemo->DescriptionText;
			WSTRING wTmp;
			AsciiToUnicode(" (1)", wTmp);
			wstrNewDescription += wTmp;
			pPrevDemo->DescriptionText = wstrNewDescription.c_str();
		}
		pPrevDemo->Update();
		delete pPrevDemo;
	}

	//If no first demo ID is stored already then this is it.
	if (!dri.dwFirstDemoID)
		dri.dwFirstDemoID = dwNewDemoID;

	return dwNewDemoID;
}

//***************************************************************************************
void CCurrentGame::SubmitCurrentGameScoringDemo(const UINT dwDemoID, CDbDemo::DemoFlag flag)
//Queues a demo to validate a hi-score/challenge submission for published holds.
//
//Pulls information from the current game to prep the submission record.
{
	if (!dwDemoID)
		return;

	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	ASSERT(pPlayer);
	bool bSubmitScores = pPlayer->Settings.GetVar(Settings::ConnectToInternet, false);
	if (flag == CDbDemo::Victory && !this->wPlayerTurn) //don't accept 0-move conquer demos
		bSubmitScores = false;

	if (bSubmitScores)
	{
		//Only upload demos for published holds.
		ASSERT(g_pTheNet);
		if (g_pTheNet->IsLocalHold(this->pHold->dwHoldID))
		{
			//Prepare demo for upload.
			string text;
			CIDSet ids(dwDemoID);
			if (CDbXML::ExportXML(V_Demos, ids, text, UINT(-1))) //no multi-room demos
			{
				//Uploads are to be handled by front end to avoid delay here so that
				//uploading demos in quick succession doesn't slow things down.
				UINT turn = this->wPlayerTurn;
				switch (flag) {
					case CDbDemo::Victory:
						if (this->conquerTokenTurn != NO_CONQUER_TOKEN_TURN)
							turn = this->conquerTokenTurn;
					break;
					case CDbDemo::CompletedChallenge:
						turn = 0; //not a conquer demo -- doesn't get a ranking
					break;
					default: break;
				}
				CNetRoom room;
				room.FillFromDemo(dwDemoID);
				const WCHAR *name = pPlayer->NameText;
				CCurrentGame::demosForUpload.push(new DEMO_UPLOAD(room, text,
						turn, this->Commands.GetTimeElapsed(), dwDemoID, UINT(time_t(this->Created)), flag, name));
			}
		}
	} else {
		//Otherwise flag that the player now has unsent score data.
		//It can be sent when the player connects to CaravelNet.
		if (this->pHold->bCaravelNetMedia)
		{
			pPlayer->Settings.SetVar(Settings::CNetProgressIsOld, (BYTE)1);
			pPlayer->Update();
		}
	}

	delete pPlayer;
}

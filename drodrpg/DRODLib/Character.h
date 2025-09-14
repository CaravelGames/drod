// $Id: Character.h 9409 2010-03-27 15:42:14Z mrimer $

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
 *
 * ***** END LICENSE BLOCK ***** */

//Character.h
//Declarations for CCharacter.
//Class for handling scriptable monster character game logic.

//A character may take the form of Beethro, an NPC or most monsters.
//It is used to deliver story elements to the player.
//
//Remarks about character scripting:
//
//1.  Each character script begins execution when a room is entered.
//    Commands that require expending a turn are delayed until the player moves.
//2.  A script can be made to run only once by executing the CC_EndScript or
//    CC_EndScriptOnExit command.
//    The character will be removed from play for the remainder of the current game.
//3.  A character can be turned into (i.e. replaced by) a regular monster with the
//    CC_TurnIntoMonster command.

#ifndef CHARACTER_H
#define CHARACTER_H

#include "CharacterCommand.h"
#include "MonsterFactory.h"
#include "CueEvents.h"
#include "GameConstants.h"
#include "DbSpeech.h"
#include "PlayerDouble.h"
#include "PlayerStats.h"

#include <set>
#include <vector>
using std::vector;

#define DefaultCustomCharacterName wszEmpty
#define ParamProcessSequenceStr "ProcessSequenceParam"
#define ColorStr "Color"
#define HueStr "Hue"
#define SaturationStr "Saturation"
#define ParamSpeechColorStr "SpeechColorParam"
#define GhostImageStr "GhostImage"
#define MinimapTreasureStr "MinimapTreasure"

class CSwordsman;
struct HoldCharacter;
class CDbHold;
typedef map<UINT, map<int, int>> ScriptArrayMap;
class CCharacter : public CPlayerDouble
{
public:
	CCharacter(CCurrentGame *pSetCurrentGame = NULL);
	IMPLEMENT_CLONE(CMonster, CCharacter);
	virtual CMonster* Replicate() const;

	bool           AttacksWhenAdjacent() const {return this->bAttackAdjacent;}
	bool           AttacksInFront() const {return this->bAttackInFront;}
	bool           AttacksInFrontWhenBackTurned() const {return this->bAttackInFrontWhenBackIsTurned;}

//	virtual bool   BrainAffects() const {return false;}
	virtual bool   CanAttackFirst() const {return this->bAttackFirst;}
	virtual bool   CanAttackLast() const {return this->bAttackLast;}
	virtual bool   CanCutBriar() const {return this->bBriar;}
	virtual bool   CanCutTarAnywhere() const { return this->bCutTarAnywhere; }
	virtual bool   CanSpawnEggs() const {return this->bSpawnEggs;}
	void           ChangeHold(const CDbHold* pSrcHold, CDbHold* pDestHold, CImportInfo& info, const bool bGetNewScriptID=true);
	static void    ChangeHoldForCommands(COMMAND_VECTOR& commands, const CDbHold* pOldHold, CDbHold* pNewHold, CImportInfo& info, bool bUpdateSpeech);
	void           CheckForCueEvent(CCueEvents &CueEvents);
	virtual bool   CheckForDamage(CCueEvents& CueEvents);
	int            CountArrayVarEntries(const CCharacterCommand& command, CCurrentGame* pGame);
	virtual bool   DamagedByFiretraps() const { return !bFiretrapImmune; }
	virtual bool   DamagedByHotTiles() const { return !bHotTileImmune; }
	void           Defeat();
	bool           DidPlayerMove(const CCharacterCommand& command, const CSwordsman& player, const int nLastCommand) const;
	virtual bool   DoesSquareContainObstacle(const UINT wCol, const UINT wRow) const;
	bool DoesVarSatisfy(const CCharacterCommand& command, CCurrentGame* pGame);
	bool DoesArrayVarSatisfy(const CCharacterCommand& command, CCurrentGame* pGame);

	bool EvaluateConditionalCommand(
		const CCharacterCommand& command, CCurrentGame* pGame, const int nLastCommand, CCueEvents& CueEvents);
	bool EvaluateLogicalAnd(
		UINT wCommandIndex, CCurrentGame* pGame, const int nLastCommand, CCueEvents& CueEvents);
	bool EvaluateLogicalOr(UINT wCommandIndex, CCurrentGame* pGame, const int nLastCommand, CCueEvents& CueEvents);
	bool EvaluateLogicalXOR(UINT wCommandIndex, CCurrentGame* pGame, const int nLastCommand, CCueEvents& CueEvents);

	void   ExportText(CDbRefs &dbRefs, CStretchyBuffer& str);
	static string ExportXMLSpeech(CDbRefs &dbRefs, const COMMAND_VECTOR& commands, const bool bRef=false);
	MESSAGE_ID ImportSpeech(CImportInfo &info);
	void   ImportText(const char** atts);

	void           FailedIfCondition();
	const CCharacterCommand* GetCommandWithLabel(const UINT label) const;
	vector<WSTRING> GetCustomDescriptions() const;
	std::set<WSTRING> GetCustomWeaknesses() const;
	virtual UINT   GetIdentity() const {return this->wIdentity;}
	virtual UINT   GetLogicalIdentity() const {return this->wLogicalIdentity;}
	UINT           GetNextSpeechID();
	virtual UINT   GetResolvedIdentity() const;
	virtual UINT   GetSpawnType(UINT defaultMonsterID) const;
	float          GetStatModifier(ScriptVars::StatModifiers statType) const;
	virtual bool   HasCustomDescription() const { return !this->customDescription.empty(); }
	virtual bool   HasCustomWeakness() const { return !this->customWeakness.empty(); }
	bool           HasSpecialDeath() const;
	virtual bool   HasGoblinWeakness() const {return this->bGoblinWeakness;}
	virtual bool   HasNoEnemyDefense() const {return this->bNoEnemyDEF;}
	virtual bool   HasSerpentWeakness() const {return this->bSerpentWeakness;}

	virtual UINT   getATK() const;   //allow "negative" values to be returned
	virtual UINT   getColor() const;
	virtual UINT   getHue() const;
	virtual UINT   getSaturation() const;
	virtual std::array<float, 3> getHSV() const;
	virtual UINT   getDEF() const;   //allow "negative" values to be returned
	virtual UINT   getSword() const;

	WSTRING        GetCustomName() const { return this->customName; }
	UINT           GetCustomSpeechColor() const { return this->customSpeechColor; }
	void           getCommandParams(const CCharacterCommand& command,
			UINT& x, UINT& y, UINT& w, UINT& h, UINT& f) const;
	void           getCommandRect(const CCharacterCommand& command,
			UINT& x, UINT& y, UINT& w, UINT& h) const;
	void           getCommandX(const CCharacterCommand& command,
			UINT& x) const;
	void           getCommandXY(const CCharacterCommand& command,
			UINT& x, UINT& y) const;

	WSTRING getPredefinedVar(const UINT varIndex) const;
	UINT getPredefinedVarInt(const UINT varIndex) const;
	WSTRING getPredefinedVarString(const UINT varIndex) const;

	static int getArrayValue(const ScriptArrayMap& scriptArrays, const UINT& varId, const int arrayIndex);

	virtual bool   IsAlive() const {return this->bAlive && !this->bReplaced;}
	virtual bool   IsAggressive() const {return false;}
	virtual bool   IsCombatable() const;
	virtual bool   IsDamageableAt(const UINT wX, const UINT wY) const;
	bool           IsDoorStateAt(const CCharacterCommand& command, const CDbRoom& room) const;
	virtual bool   IsEggSpawner() const { return this->bSpawnEggs; }
	bool           IsEntityAt(const CCharacterCommand& command, const CDbRoom& room, const CSwordsman& player) const;
	virtual bool   IsExplosiveSafe() const { return bExplosiveSafe; }
	virtual bool   IsFriendly() const;
	bool           IsGhostImage() const {return this->bGhostImage;}
	bool           IsInvisibleInspectable() const {return this->bInvisibleInspectable;}
	bool           IsLuckyGR() const {return this->bLuckyGR;}
	bool           IsLuckyXP() const {return this->bLuckyXP;}
	bool           IsMetal() const {return this->bMetal;}
	virtual bool   IsMissionCritical() const {return this->bMissionCritical;}
	bool           IsMistImmune() const { return this->bMistImmune; }
	virtual bool   IsOnMistTile() const;
	bool           IsPlayerFacing(const CCharacterCommand& command, const CSwordsman& player) const;
	bool           IsRestartScriptOnRoomEntrance() const {return this->bRestartScriptOnRoomEntrance;}
	bool           IsSafeToPlayer() const {return this->bSafeToPlayer;}
	bool           IsSwordSafeToPlayer() const {return this->bSwordSafeToPlayer;}
	virtual bool   IsTileObstacle(const UINT wTileNo) const;
	virtual bool   IsMinimapTreasure() const;
	bool           IsOpenTileAt(const CCharacterCommand& command, const CCurrentGame* pGame);
	bool IsValidEntityWait(const CCharacterCommand& command, const CDbRoom& room) const;
	virtual bool   IsWallAndMirrorSafe() const { return this->bWallMirrorSafe; }
	bool           RemovesSword() const {return this->bRemovesSword;}

	static bool    IsValidExpression(const WCHAR *pwStr, UINT& index, CDbHold *pHold, const char closingChar=0);
	static bool    IsValidTerm(const WCHAR *pwStr, UINT& index, CDbHold *pHold);
	static bool    IsValidFactor(const WCHAR* pwStr, UINT& index, CDbHold* pHold);
	static bool    IsValidPrimitiveParameters(ScriptVars::PrimitiveType ePrimitive, 
			const WCHAR* pwStr, UINT& index, CDbHold* pHold);

	virtual bool   IsVisible() const {return this->bVisible;}
	virtual bool   IsVulnerable() const {return this->bVulnerable;}
	virtual bool   IsWallDwelling() const { return this->bWallDwelling; }
	bool           IsWeaponAt(const CCharacterCommand& command, const CCurrentGame* pGame) const;
	static void    LoadCommands(const CDbPackedVars& ExtraVars, COMMAND_VECTOR& commands);
	static void    LoadCommands(const CDbPackedVars& ExtraVars, COMMANDPTR_VECTOR& commands);
	virtual bool   OnAnswer(int nCommand, CCueEvents &CueEvents);
	virtual bool   OnStabbed(CCueEvents &CueEvents, const UINT /*wX*/=-1, const UINT /*wY*/=-1);

	static int     parseExpression(const WCHAR *pwStr, UINT& index, CCurrentGame *pGame, CCharacter *pNPC=NULL, const char closingChar = 0);
	static int     parseNestedExpression(const WCHAR* pwStr, UINT& index, CCurrentGame* pGame, CCharacter* pNPC);
	static int     parseNumber(const WCHAR* pwStr, UINT& index);
	static int     parseTerm(const WCHAR *pwStr, UINT& index, CCurrentGame *pGame, CCharacter *pNPC);
	static int     parseFactor(const WCHAR *pwStr, UINT& index, CCurrentGame *pGame, CCharacter *pNPC);
	static int     parsePrimitive(ScriptVars::PrimitiveType ePrimitive,
			const WCHAR* pwStr, UINT& index, CCurrentGame* pGame, CCharacter* pNPC);

	virtual void   Process(const int nLastCommand, CCueEvents &CueEvents);
	virtual bool   ProcessAfterAttack(CCueEvents &CueEvents);
	void           ProcessAfterDefeat(CCueEvents &CueEvents);
	virtual bool   ProcessAfterDefend(CCueEvents &CueEvents);
	virtual bool   ProcessAfterUse(CCueEvents &CueEvents);
	void           ProcessAfterVictory(CCueEvents &CueEvents);

	virtual void   ReflectX(CDbRoom *pRoom);
	virtual void   ReflectY(CDbRoom *pRoom);
	bool           ResetLevelExits();
	void           ResolveLogicalIdentity(CDbHold *pHold);
	void           RestartScript();
	void           RotateClockwise(CDbRoom *pRoom);

	virtual void   Save(const c4_RowRef &MonsterRowRef, const bool bSaveScript=true);
	static void    SaveCommands(CDbPackedVars& ExtraVars, const COMMAND_VECTOR& commands);
	static void    SaveCommands(CDbPackedVars& ExtraVars, const COMMANDPTR_VECTOR& commands);
	static void    SaveSpeech(const COMMAND_VECTOR& commands);
	static void    SaveSpeech(const COMMANDPTR_VECTOR& commands);
	virtual void   SetColor(const UINT color) { this->color = color; }
	virtual void   SetHue(const UINT hue);
	virtual void   SetSaturation(const UINT saturation);
	virtual void   SetCurrentGame(const CCurrentGame *pSetCurrentGame);
	virtual void   SetCustomSpeechColor(const UINT color) { this->customSpeechColor = color; }
	virtual void   SetExtraVarsForExport() { PackExtraVars(true); } //include config params and script
	void           SetExtraVarsFromMembersWithoutScript(CDbPackedVars& vars) const;
	void           SetGhostImage(const bool ghostImage) { this->bGhostImage = ghostImage; }
	virtual void   SetMembers(const CDbPackedVars& vars);
	void           SetMinimapTreasure(const bool minimapTreasure) { this->bMinimapTreasure = minimapTreasure; }
	virtual void   Delete();

	//Behavior patterns.
	virtual bool HasRayGun() const {return this->bHasRayGun && IsVisible();}
	virtual bool HasRayBlocking() const {return this->bHasRayBlocking;}
	virtual bool TurnToFacePlayerWhenFighting() const {return this->bSurprisedFromBehind;}

	bool           IsTileAt(const CCharacterCommand& command, CCueEvents &CueEvents) const;
	bool           IsTileGroupAt(const CCharacterCommand& command) const;

	COMMAND_VECTOR commands;
	HoldCharacter *pCustomChar; //custom character type
	CIDSet answerOptions;   //optional answers supplied to a Question command
	UINT  dwScriptID;       //charater script ref
	UINT  wIdentity;        //monster type
	UINT  wInitialIdentity; //initial identity in case it changes e.g. Set Appearance command
	UINT  wLogicalIdentity; //logical ID (might be a hold custom character type)
	bool  bVisible;         //on screen in room, or not
	bool  bInvisibleInspectable; //appears in tooltip when invisible
	bool  bScriptDone;      //true when script has run to completion
	bool  bReplaced;        //true when script command replaces the character
									//with a normal monster
	bool  bGlobal;          //true when the character has been marked for the global script list
	bool  bYesNoQuestion;   //question type being asked
	bool  bPlayerTouchedMe; //player bumped into this NPC this turn
	bool  bAttacked;        //only one behavior-based attack per turn is allowed
	ScriptFlag::EquipmentType equipType;//what type of inventory I represent
	MovementIQ movementIQ;  //movement behavior
	bool  bMovementChanged; //movement type is changed
	bool  bParseIfElseAsCondition; //a multi-turn elseif sequence is in play
	UINT worldMapID;        //the world map that "world map *" script commands will operate on

private:
	bool BuildTiles(const CCharacterCommand& command, CCueEvents &CueEvents);
	void Disappear();
	int  GetIndexOfCommandWithLabel(const int label) const;
	int  GetIndexOfPreviousIf(const bool bIgnoreElseIf) const;
	int  GetIndexOfNextElse(const bool bIgnoreElseIf) const;
	int  GetIndexOfNextLogicEnd(const UINT wStartIndex) const;
	bool HasUnansweredQuestion(CCueEvents &CueEvents) const;
	bool IsCommandTypeIn(const int startIndex, const int endIndex, const CCharacterCommand::CharCommand command) const;
	bool IsExpressionSatisfied(const CCharacterCommand& command, CCurrentGame* pGame);
	void MoveCharacter(const int dx, const int dy, const bool bFaceDirection,
			CCueEvents& CueEvents);
	void TeleportCharacter(const UINT wDestX, const UINT wDestY, CCueEvents& CueEvents);
	void TurnIntoMonster(CCueEvents& CueEvents, const bool bSpecial=false);

	void PackExtraVars(const bool bSaveScript);

	void SetDefaultMovementType();
	bool setPredefinedVarInt(UINT varIndex, const UINT val, CCueEvents& CueEvents);
	void setPredefinedVarString(UINT varIndex, const WSTRING val, CCueEvents& CueEvents);
	void SetVariable(const CCharacterCommand& command, CCurrentGame* pGame, CCueEvents& CueEvents);
	void SetArrayVariable(const CCharacterCommand& command, CCurrentGame* pGame, CCueEvents& CueEvents);
	void SetMapIcon(const CCharacterCommand& command, CCurrentGame* pGame, CCueEvents& CueEvents);

	void SyncCustomCharacterData(const CDbHold* pSrcHold, CDbHold* pDestHold, CImportInfo& info);
	static void SyncCustomCharacterData(UINT& wLogicalIdentity, const CDbHold* pSrcHold, CDbHold* pDestHold, CImportInfo& info);

	//Internal command storage representation --> 3.1.0+
	static void  DeserializeCommand(BYTE* buffer, UINT& index, CCharacterCommand& command);
	static void  DeserializeCommands(BYTE* buffer, const UINT bufferSize, COMMAND_VECTOR& commands);
	static void  DeserializeCommands(BYTE* buffer, const UINT bufferSize, COMMANDPTR_VECTOR& commands);
	static UINT  readBpUINT(const BYTE* buffer, UINT& index);
	static void  SerializeCommand(string& buffer, const CCharacterCommand& command);
	static void  SerializeCommands(string& buffer, const COMMAND_VECTOR& commands);
	static void  SerializeCommands(string& buffer, const COMMANDPTR_VECTOR& commands);
	static void  writeBpUINT(string& buffer, UINT n);

	void  setBaseMembers(const CDbPackedVars& vars);

	UINT wCurrentCommandIndex; //command to play next
	UINT wTurnDelay;        //turns before next command
	UINT wLastSO, wSO;      //swordsman's orientation
	UINT wXRel, wYRel;      //destination of relative movement
	bool bMovingRelative;   //true when MoveRel destination is set
	UINT wExitingRoomO;     //when movement is slated to cause a room exit

	//Imperatives.
	bool bVulnerable;       //whether deadly events can kill the NPC
	bool bMissionCritical;  //when set, killing this NPC results in game over
	bool bSafeToPlayer;     //whether NPC can kill player by stepping
	bool bSwordSafeToPlayer;//if true, sword stabs won't damage player
	bool bDefeated;         //NPC-monster was defeated in combat
	bool bShowStatChanges;  //scripted var changes affecting player stats will be displayed onscreen
	bool bGhostImage;       //display on screen even if bVisible is false
	bool bRestartScriptOnRoomEntrance; //whether to restart script from first command whenever room is entered
	bool bExecuteScriptOnCombat; //whether to execute script commands on the turn when combat is initiated

	//Behavior patterns.
	bool bAttackAdjacent;      //attack adjacent target
	bool bAttackInFront;       //attack target directly in front
	bool bAttackInFrontWhenBackIsTurned; //attack target directly in front when back is turned
	bool bFaceAwayFromTarget;  //face away from target each turn
	bool bFaceTarget;          //face toward the target each turn
	bool bHasRayGun;           //shoot a beam attack each turn
	bool bHasRayBlocking;      //can block beam attack
	bool bSurprisedFromBehind; //an attack from behind causes me to lose my first combat turn
	bool bGoblinWeakness;      //goblin sword does strong hit
	bool bSerpentWeakness;     //serpent sword does strong hit
	bool bMetal;               //custom inventory properties -- metal equipment
	bool bLuckyGR;             //x2 GR from fights
	bool bLuckyXP;             //x2 XP from fights
	bool bBriar;               //weapon can break briar
	bool bNoEnemyDEF;          //opponent's DEF is nullified in combat
	bool bAttackFirst;         //I attack first in combat
	bool bAttackLast;          //I attack last in combat
	bool bDropTrapdoors;       //stepping off a trapdoor drops it
	bool bMoveIntoSwords;      //can move onto swords instead of being blocked by them
	bool bPushObjects;         //can push movable objects
	bool bSpawnEggs;           //will spawn eggs in reaction to combats
	bool bRemovesSword;        //prevents player having sword when equipped
	bool bExplosiveSafe;       //sword does not detonate powder kegs
	bool bMinimapTreasure;     //counts as collectable item for minimap when visible and not ended
	bool bCutTarAnywhere;      //can cut tarstuff on otherwise non-vulnerable areas
	bool bWallMirrorSafe;      //sword doesn't break walls or mirrors
	bool bHotTileImmune;       //not damaged by hotiles
	bool bFiretrapImmune;      //not damaged by firetraps
	bool bMistImmune;          //DEF not negated by mist
	bool bWallDwelling;        //dies outside of solid tile

	UINT wJumpLabel;			//if non-zero, jump to the label if this command is satisfied
	bool bWaitingForCueEvent;
	bool bIfBlock;
	int  eachAttackLabelIndex, eachDefendLabelIndex, eachUseLabelIndex;
	int  eachVictoryLabelIndex; //if set, jump script execution here on each combat victory

	bool bReplacedWithDefault; //If a local script has been replaced by the NPC's default script

	WSTRING customName; // Custom name for this character, used for any display purpose, empty means use the default character name

	UINT customSpeechColor; //Value to represent custom speech color. empty means use default color

	UINT wLastSpeechLineNumber; //used during language import

	vector<UINT> jumpStack; //maintains index of GoTo commands executed, for Return commands

	//Predefined vars.
	UINT color, sword, hue, saturation; //cosmetic details
	UINT paramX, paramY, paramW, paramH, paramF; //script-definable script command parameter overrides
	UINT monsterHPmult, monsterATKmult, monsterDEFmult, monsterGRmult, monsterXPmult; // monster stat modifiers
	UINT itemMult, itemHPmult, itemATKmult, itemDEFmult, itemGRmult, itemShovelMult; // item value modifiers
	int wSpawnType; // type of monster to spawm when spawning eggs
	WSTRING customWeakness; // matching weakness does strong hit, empty means no custom weakness
	WSTRING customDescription; // additional monster information
};

//*****************************************************************************
//CC_Speech uses to pass data to front end
class CFiredCharacterCommand : public CAttachableObject
{
public:
	CFiredCharacterCommand(CMonster *pMonster, CCharacterCommand *pCommand, const UINT turnNo,
			const UINT scriptID, const UINT commandIndex)
		: CAttachableObject()
		, pSpeakingEntity(pMonster), pExecutingNPC(pMonster)
		, pCommand(pCommand)
		, bPlaySound(true)
		, bFlush(false)
		, bPseudoMonster(false)
		, turnNo(turnNo)
		, scriptID(scriptID)
		, commandIndex(commandIndex)
	{}

	CMonster *pSpeakingEntity, *pExecutingNPC;
	CCharacterCommand *pCommand;
	WSTRING text;     //interpolated subtitle text
	bool bPlaySound;  //whether sound clip will be played
	bool bFlush;      //flag to flush queued speech commands
	bool bPseudoMonster; //attached monster is to be deleted

	UINT turnNo;   //when this command was executed
	UINT scriptID; //unique ID of the character executing this command
	UINT commandIndex; //index of speech command in character's script
};

//*****************************************************************************
//CC_VisualEffect uses to pass data to front end.
struct VisualEffectInfo : public CAttachableObject
{
	VisualEffectInfo(const UINT x, const UINT y, const UINT o, const UINT type, const UINT sound,
			const UINT srcX, const UINT srcY)
		: CAttachableObject()
		, effect(x, y, o, type, sound)
		, source(srcX, srcY)
	{	}
	CMoveCoordEx2 effect;
	CCoord source;
};

#endif //...#ifndef CHARACTER_H


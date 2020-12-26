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
//3.  Character monsters are not used to determine room completion.
//    That is, character monsters will remain in a room even after it has been
//    conquered.  To remove a character once a room has been beaten, insert a CC_GoTo
//    end of the script on "WaitForCleanRoom" event at the beginning.
//4.  A character can be turned into a regular NPC or monster with the
//    CC_TurnIntoMonster command.  In this case, (a) its script will count as
//    not having been completed and the character will always exist in the room,
//    even after it has been conquered (see 3), and (b) it might need to be killed
//    in order to conquer the room.

#ifndef CHARACTER_H
#define CHARACTER_H

#include "CharacterCommand.h"
#include "MonsterFactory.h"
#include "CueEvents.h"
#include "GameConstants.h"
#include "DbSpeech.h"
#include "DbHolds.h"
#include "PlayerDouble.h"
#include "PlayerStats.h"

//Literals used to query and store values for the NPC in the packed vars object.
#define commandStr_3_0_2_2 "SerializedCommands"
#define commandStr "Commands"
#define idStr "id"
#define numCommandsStr "NumCommands"
#define scriptIDstr "ScriptID"
#define visibleStr "visible"

#define ParamXStr "XParam"
#define ParamYStr "YParam"
#define ParamWStr "WParam"
#define ParamHStr "HParam"
#define ParamFStr "FParam"
#define ParamProcessSequenceStr "ProcessSequenceParam"

enum CharacterDisplayMode {
	CDM_Normal,
	CDM_GhostFloor,
	CDM_GhostOverhead
};

struct HoldCharacter;
class CCharacter : public CPlayerDouble
{
public:
	CCharacter(CCurrentGame *pSetCurrentGame = NULL);
	IMPLEMENT_CLONE(CMonster, CCharacter);
	virtual CMonster* Replicate() const;

	virtual bool   CanDropTrapdoor(const UINT oTile) const;
	virtual bool   CanPressPressurePlates() const { return !this->IsFlying() && this->GetIdentity() != M_SEEP; }
	virtual bool   CanPushObjects() const;
	void           ChangeHold(const CDbHold* pSrcHold, CDbHold* pDestHold, CImportInfo& info, const bool bGetNewScriptID=true);
	static void    ChangeHoldForCommands(COMMAND_VECTOR& commands, const CDbHold* pOldHold, CDbHold* pNewHold, CImportInfo& info, bool bUpdateSpeech);
	void           CheckForCueEvent(CCueEvents &CueEvents);
	virtual bool   CheckForDamage(CCueEvents& CueEvents);
	void           CriticalNPCDied(CCueEvents& CueEvents);
	virtual bool   DoesSquareContainObstacle(const UINT wCol, const UINT wRow) const;

	bool           Display() const { return IsVisible() || IsGhostImage(); }
	bool           DisplayGhostImage() const { return IsGhostImage() && !IsVisible(); }

	bool           EndsWhenKilled() const {return this->bEndWhenKilled;}
	
	void   ExportText(CDbRefs &dbRefs, CStretchyBuffer& str);
	static string ExportXMLSpeech(CDbRefs &dbRefs, const COMMAND_VECTOR& commands, const bool bRef=false);
	MESSAGE_ID ImportSpeech(CImportInfo &info, const bool bHoldChar=false);
	void   ImportText(const char** atts);

	void           FailedIfCondition();
	const CCharacterCommand* GetCommandWithLabel(const UINT label) const;
	virtual UINT   GetIdentity() const {return this->wIdentity;}
	virtual UINT   GetResolvedIdentity() const;
	UINT           GetNextSpeechID();
	bool           HasSpecialDeath() const;
	virtual bool   HasSword() const;

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

	int getLocalVarInt(const WSTRING& varName) const;
	WSTRING getLocalVarString(const WSTRING& varName) const;

	virtual bool   IsAlive() const {return this->bAlive && !this->bReplaced;}
	virtual bool   IsAttackableTarget() const;
	virtual bool   IsBrainPathmapObstacle() const;
	virtual bool   IsFlying() const;
	virtual bool   IsFriendly() const;
	bool           IsGhostImage() const {return this->eDisplayMode != CDM_Normal;}
	bool           IsGhostImageFloor() const {return this->eDisplayMode == CDM_GhostFloor;}
	bool           IsGhostImageOverhead() const {return this->eDisplayMode == CDM_GhostOverhead;}
	bool           IsInvulnerable() const {return GetImperative() == ScriptFlag::Invulnerable;}
	bool           IsMissionCritical() const {return GetImperative() == ScriptFlag::MissionCritical;}
	virtual bool   IsMonsterTarget() const;
	virtual bool   IsNPCPathmapObstacle() const;
	virtual bool   IsOpenMove(const int dx, const int dy) const;
	virtual bool   IsPushableByBody() const;
	virtual bool   IsPushableByWeaponAttack() const;
	bool           IsRequiredToConquer() const {return GetImperative() == ScriptFlag::RequiredToConquer;}
	bool           IsSafeToPlayer() const {return this->bSafeToPlayer;}
	virtual bool   IsSwimming() const;
	bool           IsSwordSafeToPlayer() const {return this->bSwordSafeToPlayer;}
	virtual bool   IsTarget() const { return this->IsVisible() && this->IsMonsterTarget(); }
	bool           IsTileAt(const CCharacterCommand& command) const;
	virtual bool   IsTileObstacle(const UINT wTileNo) const;
	static bool    IsValidExpression(const WCHAR *pwStr, UINT& index, CDbHold *pHold, const bool bExpectCloseParen=false);
	static bool    IsValidTerm(const WCHAR *pwStr, UINT& index, CDbHold *pHold);
	static bool    IsValidFactor(const WCHAR *pwStr, UINT& index, CDbHold *pHold);
	virtual bool   IsVisible() const {return this->bVisible;}
	bool           JumpToCommandWithLabel(const WCHAR *pText);
	bool           JumpToCommandWithLabel(const UINT num);
	static void    LoadCommands(CDbPackedVars& ExtraVars, COMMAND_VECTOR& commands);
	static void    LoadCommands(CDbPackedVars& ExtraVars, COMMANDPTR_VECTOR& commands);
	virtual bool   OnAnswer(int nCommand, CCueEvents &CueEvents);
	virtual bool   OnStabbed(CCueEvents &CueEvents, const UINT /*wX*/=-1, const UINT /*wY*/=-1, WeaponType weaponType=WT_Sword);
	static int     parseExpression(const WCHAR *pwStr, UINT& index, CCurrentGame *pGame, CCharacter *pNPC=NULL, const bool bExpectCloseParen=false);
	static int     parseTerm(const WCHAR *pwStr, UINT& index, CCurrentGame *pGame, CCharacter *pNPC);
	static int     parseFactor(const WCHAR *pwStr, UINT& index, CCurrentGame *pGame, CCharacter *pNPC);
	virtual void   Process(const int nLastCommand, CCueEvents &CueEvents);
	virtual void   PushInDirection(int dx, int dy, bool bStun, CCueEvents &CueEvents);

	virtual void   ReflectX(CDbRoom *pRoom);
	virtual void   ReflectY(CDbRoom *pRoom);
	bool           ResetLevelExits();

	void           ResolveLogicalIdentity(CDbHold *pHold);
	virtual void   SetCurrentGame(const CCurrentGame *pSetCurrentGame);
	void   SetImperative(const ScriptFlag::Imperative eVal) {this->eImperative = eVal;}
	virtual void   SetExtraVarsFromMembers(const bool bHoldChar=false);
	void           SetExtraVarsFromMembersWithoutScript(const bool bHoldChar=false);
	virtual void   SetMembersFromExtraVars();
	virtual void   Stun(CCueEvents &CueEvents, UINT val=1);

	virtual void   Save(const c4_RowRef &MonsterRowRef, const bool bSaveScript=true);
	static void    SaveCommands(CDbPackedVars& ExtraVars, const COMMAND_VECTOR& commands);
	static void    SaveCommands(CDbPackedVars& ExtraVars, const COMMANDPTR_VECTOR& commands);
	static void    SaveSpeech(const COMMAND_VECTOR& commands);
	static void    SaveSpeech(const COMMANDPTR_VECTOR& commands);
	virtual void   Delete();

	virtual bool SetWeaponSheathed();

	COMMAND_VECTOR commands;
	UINT dwScriptID;       //charater script ref
	UINT  wIdentity;        //monster type
	UINT  wLogicalIdentity; //logical ID (might be a hold custom character type)
	HoldCharacter *pCustomChar; //custom character type
	bool  bVisible;         //on screen in room, or not
	CharacterDisplayMode eDisplayMode; //alternate front-end display logic when bVisible is false
	bool  bScriptDone;      //true when script has run to completion
	bool  bReplaced;        //true when script command replaces the character
									//with a normal monster
	bool  bWasPushed;       //True if the character was pushed and no turn has passed since the stun has finished
	bool  bPreventMoveAfterPush;    //true if the character was just pushed by the player, used to prevent move actions from happening after push
	bool  bGlobal;          //true if this is a global script character
	bool  bNewEntity;       //true if this character was created via AddNewEntity
	bool  bYesNoQuestion;   //question type being asked
	CIDSet answerOptions;   //optional answers supplied to a Question command
	bool  bPlayerTouchedMe; //player bumped into this NPC this turn
	bool  bParseIfElseAsCondition; //a multi-turn elseif sequence is in play

private:
	void BuildMarker(const CCharacterCommand& command);
	void BuildTiles(const CCharacterCommand& command, CCueEvents& CueEvents);
	bool ConfirmPathWithNextMoveOpen();
	void Disappear();
	bool DoesVarSatisfy(const CCharacterCommand& command, CCurrentGame *pGame);
	ScriptFlag::Imperative GetImperative() const {return this->eImperative;}
	int  GetIndexOfCommandWithLabel(const UINT label) const;
	bool GetMovement(const UINT wDestX, const UINT wDestY, int& dx, int& dy,
			int& dxFirst, int& dyFirst, bool& bPathmapping, const bool bAllowTurning=true);
	void MoveCharacter(const int dx, const int dy, const bool bFaceDirection,
			CCueEvents& CueEvents);
	void TeleportCharacter(const UINT wDestX, const UINT wDestY, CCueEvents& CueEvents);
	void TurnIntoMonster(CCueEvents& CueEvents, const bool bSpecial=false);
	bool TurnsSlowly() const;

	void setPredefinedVar(UINT varIndex, const UINT val, CCueEvents& CueEvents);
	void SetVariable(const CCharacterCommand& command, CCurrentGame *pGame, CCueEvents& CueEvents);
	void SetLocalVar(const WSTRING& varName, const WSTRING& val);

	void GenerateEntity(const UINT identity, const UINT wX, const UINT wY, const UINT wO, CCueEvents& CueEvents);

	void SyncCustomCharacterData(const CDbHold* pSrcHold, CDbHold* pDestHold, CImportInfo& info);
	static void SyncCustomCharacterData(UINT& wLogicalIdentity, const CDbHold* pSrcHold, CDbHold* pDestHold, CImportInfo& info);

	//changing internal command storage representation --> 3.0.2 rev3
	static UINT  readBpUINT(const BYTE* buffer, UINT& index);
	static void  writeBpUINT(string& buffer, UINT n);
	static void  DeserializeCommand(BYTE* buffer, UINT& index, CCharacterCommand& command);
	static void  DeserializeCommands(BYTE* buffer, const UINT bufferSize, COMMAND_VECTOR& commands);
	static void  DeserializeCommands(BYTE* buffer, const UINT bufferSize, COMMANDPTR_VECTOR& commands);
	static void  SerializeCommand(string& buffer, const CCharacterCommand& command);
	static void  SerializeCommands(string& buffer, const COMMAND_VECTOR& commands);
	static void  SerializeCommands(string& buffer, const COMMANDPTR_VECTOR& commands);

	//changing internal command storage representation from pre-3.0.2 --> 3.0.2 rev2
	static UINT  readUINT(const BYTE* buffer, UINT& index);
	static void  writeUINT(string& buffer, UINT n);
	static void  DeserializeCommands_3_0_2_2(BYTE* buffer, const UINT bufferSize, COMMAND_VECTOR& commands);

	static void  Upgrade2_0CommandTo3_0(CCharacterCommand& command, COMMAND_VECTOR& commands);

	void         SetBaseMembersFromExtraVars();

	UINT wCurrentCommandIndex; //command to play next
	UINT wTurnDelay;        //turns before next command
	ScriptFlag::Imperative eImperative; //imperative status
	UINT wXRel, wYRel;      //destination of relative movement
	bool bMovingRelative;   //true when MoveRel destination is set
	bool bSafeToPlayer;     //whether NPC can kill player by stepping
	bool bSwordSafeToPlayer;//if true, sword stabs won't damage player
	bool bEndWhenKilled;    //script completes when killed
	bool bNotPushable;      // if true the character cannot be pushed at all
	bool bPushableByBody;		//if true, the character can be pushed with body
	bool bPushableByWeapon;	//if true, the character can be pushed with any weapon
	bool bStunnable;         //if false, the character will never be stunned
	bool bBrainPathmapObstacle, bNPCPathmapObstacle;
	bool bWeaponOverride;   //Whether P_MONSTER_WEAPON now overrides the NPCs default properties
	MovementIQ movementIQ;  //movement behavior
	UINT worldMapID;        //the world map that "world map *" script commands will operate on

	UINT wJumpLabel;			//if non-zero, jump to the label if this command is satisfied
	bool bWaitingForCueEvent;
	bool bIfBlock;
	vector<UINT> jumpStack; //maintains index of GoTo commands executed, for Return commands

	UINT wLastSpeechLineNumber; //used during language import

	//Predefined vars.
	UINT paramX, paramY, paramW, paramH, paramF; //script-definable script command parameter overrides

	typedef map<WSTRING, WSTRING> LocalScriptMap;
	LocalScriptMap localScriptVars;
};

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

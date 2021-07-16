#ifndef CHARACTERCOMMAND_H
#define CHARACTERCOMMAND_H

#include "MonsterTypes.h"

#include <BackEndLib/Coord.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>

#include <string>
#include <vector>
#include <cstring> // memcpy

//Speakers for whom some faces are implemented.
enum SPEAKER
{
	//Custom speakers available to everyone.
	Speaker_Beethro=0,	//don't change these constants
	Speaker_Citizen1=6,
	Speaker_Citizen2=7,
	Speaker_Citizen3=45,
	Speaker_Citizen4=46,
	Speaker_Custom=5,
	Speaker_Instructor=10, //deprecated
	Speaker_MudCoordinator=11, //deprecated
	Speaker_Negotiator=3, //deprecated
	Speaker_None=4,
	Speaker_TarTechnician=13, //deprecated
	Speaker_BeethroInDisguise=47,
	Speaker_Self=48,
	Speaker_Player=49,
	Speaker_Stalwart=50,

	//Monster speakers.
	Speaker_Halph=1,
	Speaker_Slayer=2,
	Speaker_Goblin=9,
	Speaker_RockGolem=12,
	Speaker_Guard=14,
	Speaker_Pirate=17,
	Speaker_Roach=18,
	Speaker_QRoach=19,
	Speaker_RoachEgg=20,
	Speaker_WWing=21,
	Speaker_Eye=22,
	Speaker_Serpent=23,
	Speaker_TarMother=24,
	Speaker_TarBaby=25,
	Speaker_Brain=26,
	Speaker_Mimic=27,
	Speaker_Spider=28,
	Speaker_SerpentG=29,
	Speaker_SerpentB=30,
	Speaker_WaterSkipper=31,
	Speaker_WaterSkipperNest=32,
	Speaker_Aumtlich=33,
	Speaker_Clone=34,
	Speaker_Decoy=35,
	Speaker_Wubba=36,
	Speaker_Seep=37,
	Speaker_Fegundo=38,
	Speaker_FegundoAshes=39,
	Speaker_MudMother=40,
	Speaker_MudBaby=41,
	Speaker_GelMother=42,
	Speaker_GelBaby=43,
	Speaker_Citizen=16,
	Speaker_RockGiant=44,
	Speaker_MadEye=15,
	Speaker_GoblinKing=8,

	Speaker_Count=51
};

//Flags bits.
namespace ScriptFlag
{
	static const UINT PLAYER  = 0x00000001;
	static const UINT HALPH   = 0x00000002;
	static const UINT MONSTER = 0x00000004;
	static const UINT NPC     = 0x00000008;
	static const UINT PDOUBLE = 0x00000010;
	static const UINT SELF    = 0x00000020;
	static const UINT SLAYER  = 0x00000040;
	static const UINT BEETHRO = 0x00000080;
	static const UINT STALWART= 0x00000100;

	//NPC logic flags.
	//(f.e.) = logic that only affects front end display events
	enum Imperative
	{
		Killable=0,           //can be killed (default)
		Unkillable=1,         //can't be killed, but combat may still be fought
		MissionCritical=2,    //must not be killed or game ends
		NotMissionCritical=3, //may be killed (default)
		Die=4,                //NPC dies
//		DieSpecial=5,         //NPC dies in way distinct to specific monster type -- not used in RPG
		Safe=6,               //can't kill player by bumping, and player can't fight me
		Deadly=7,             //can kill player by bumping and sword (default)
		SwordSafeToPlayer=8,  //sword doesn't damage player
		HideStatChanges=9,    //(f.e.) player stat changes will not be displayed as events on screen
		ShowStatChanges=10,   //(f.e.) player stat changes will be displayed on screen (default)
		NoGhostDisplay=11,    //(f.e.) NPC isn't displayed when not visible (default)
		GhostDisplay=12,      //(f.e.) NPC is displayed when not visible -- i.e. a ghost
		                      //image is displayed, but doesn't occupy a room tile
									 //(affects the front end only)
		NoRestartScriptOnRoomEntrance=13, //don't restart script on room entrance (default)
		RestartScriptOnRoomEntrance=14,   //restart script execution from beginning on room entrance
		MakeGlobal=15,        //move NPC-script to the global NPC-script list
		RunOnCombat=16,       //execute script when combat is initiated (default)
		PauseOnCombat=17      //do not execute any script commands when combat is initiated
	};

	//Behavior patterns for NPCs/monsters.
	enum Behavior
	{
		Standard=0,             //do nothing special
		FaceTarget=1,           //face toward the target each turn
		AttackInFrontWithBackTurned=2, //attack target if directly in front with its back turned
		AttackInFront=3,        //attack target if directly in front
		AttackAdjacent=4,       //attack target when adjacent
		BeamAttack=5,				//beam attack
		SurprisedFromBehind=6,  //an attack from behind causes me to lose my first combat turn
		FaceAwayFromTarget=7,   //face away from target each turn
		GoblinWeakness=8,       //goblin sword does strong hit against me
		SerpentWeakness=9,      //serpent sword does strong hit against me
		Metal=10,               //custom inventory properties -- metal equipment
		LuckyGR=11,             //x2 GR from fights
		Briar=12,               //weapon can break briar
		NoEnemyDefense=13,      //opponent's DEF is nullified in combat
		AttackFirst=14,         //I attack first in combat
		DirectMovement=15,      //use DirectOnly movement
		NormalMovement=16,      //use SmartDiagonalOnly movement (default)
		SmarterMovement=17,     //use SmarterDiagonalOnly movement
		OmniMovement=18,        //use SmartOmniDirection movement
		DropTrapdoors=19,       //stepping off a trapdoor drops it
		MoveIntoSwords=20,      //can move onto swords instead of being blocked by them
		PushObjects=21,         //can push movable objects
		LuckyXP=22,             //x2 XP from fights
		AttackLast=23,          //I always attack last in combat
		BeamBlock=24,           //blocks beam attacks
		SpawnEggs=25,           //will spawn eggs in reaction to combats
	};

	//Inventory and global script types.
	enum EquipmentType
	{
		NotEquipment=0,
		Weapon=1,
		Armor=2,
		Accessory=3,
		Command=4        //some architect-defined custom script
	};

	enum TransactionType
	{
		Trade=0,         //trade current equipment for new
		Destroy=1,       //remove current equipment
		Sell=2,          //sell current equipment for GR
		Disable=3,       //this equipment type is disabled (as if the player had an empty slot)
		Enable=4,        //this equipment type is (re)enabled
		QueryStatus=5    //query the status of this equipment type (not simply whether enabled or disabled, but whether it is currently wielded in play)
	};

	enum GotoSmartType {
		PreviousIf = -1,
		NextElseOrElseIfSkipCondition = -2
	};

	enum StatType {
		HP = 0,
		ATK = 1,
		DEF = 2,
		GOLD = 3,
		XP = 4
	};
};

class CDbSpeech;
class CCharacterCommand
{
public:
	CCharacterCommand();
	CCharacterCommand(const CCharacterCommand& that, const bool bReplicateData=false);
	~CCharacterCommand();
	CCharacterCommand& operator=(const CCharacterCommand& that);
	void swap(CCharacterCommand &that);

	//DO NOT rearrange (i.e., reassign values given to these commands).
	enum CharCommand
	{
		CC_Appear=0,            //Appear at current square.
		CC_AppearAt,            //Appear at square (x,y).
		CC_MoveTo,              //Move to square (x,y) or target set in flags.
		                        //If w is set, then forbid turning while moving.
		                        //If h is set, then take only a single step before advancing to next command.
		CC_Wait,                //Wait X turns.
		CC_WaitForCueEvent,     //Wait for cue event X to fire.
		CC_WaitForRect,         //Wait until an entity in flags is in rect (x,y,w,h).
		CC_Speech,              //Deliver speech referenced by SpeechID, possibly at (x,y).
		CC_Imperative,          //Sets imperative status to X (was CC_Invincibility in 2.0).
		CC_Disappear,           //Disappear. (Use in conjunction with CC_MoveTo command to leave the room.)
		CC_TurnIntoMonster,     //Replace with normal monster of specified type.
		CC_FaceDirection,       //Rotate to face orientation X.
		CC_WaitForNotRect,      //Wait until an entity in flags is not in rect (x,y,w,h).
		CC_WaitForDoorTo,       //Wait for door at (x,y) to (w=close/open).
		CC_Label,               //Destination (x) for a GoTo command.
		CC_GoTo,                //Continue executing script commands from Label (x).
		CC_GotoIf,             //(deprecated)
		CC_WaitForMonster,     //(deprecated)
		CC_WaitForNotMonster,  //(deprecated)
		CC_WaitForTurn,         //Wait until play reaches turn X.
		CC_WaitForCleanRoom,    //(not used in RPG)
		CC_WaitForPlayerToFace, //Wait until player faces orientation X.
		CC_ActivateItemAt,      //Activate item at (x,y).  Works only for some items.
		CC_EndScript,           //Removes the character for the rest of the current game.
		CC_WaitForHalph,       //(deprecated)
		CC_WaitForNotHalph,    //(deprecated)
		CC_WaitForCharacter,   //(deprecated)
		CC_WaitForNotCharacter,//(deprecated)
		CC_FlushSpeech,         //(Front end) Purge speech events in queue (x=display/erase)
		CC_Question,			   //Ask a yes/no or multiple choice question (speech text).
		CC_SetMusic,			   //Set music being played to X (custom dataID W/label).
		CC_EndScriptOnExit,     //Removes the character for the rest of the current game when room is exited.
		CC_If,                  //Begin a conditional block if next command is satisfied.
		CC_IfElse,              //Begin a conditional block when command following CC_If was not satisfied (also ends If block).
		CC_IfEnd,               //Ends a conditional If or IfElse block.
		CC_LevelEntrance,       //Takes player to level entrance X.  If Y is set, skip level entrance display.
		CC_VarSet,              //Sets var X (operation Y) W, e.g. X += 5
		CC_WaitForVar,          //Wait until var X (comparison Y) W, e.g. X >= 5
		CC_SetPlayerAppearance, //Sets player to look like entity X.
		CC_CutScene,            //Begin cut scene (if X is set), else ends cut scene.
		CC_MoveRel,             //Move (x,y) relative to current position. If w is set, then forbid turning while moving.
		                        //If h is set, then take only a single step before advancing to next command.
		CC_SetPlayerSword,      //If X is set, player is given a sword, else it is taken away. (deprecated by Equipment)
		CC_AnswerOption,        //Text answer option (speech text) for a Question command that jumps to Label (x).
		CC_BuildTile,           //Build game element (flags) in rect (x,y,w,h).
		CC_AmbientSound,        //Play sound with DataID=w (0 stops ambient sounds).  If h is set, loop indefinitely.
		CC_AmbientSoundAt,      //Play sound with DataID=w (0 stops ambient sounds) at (x,y).  If h is set, loop indefinitely.
		CC_WaitForNoBuilding,   //(not used in RPG)
		CC_PlayVideo,           //Play video at (x,y) with DataID=w.
		CC_WaitForPlayerToMove, //Wait until player moves in direction X.
		CC_WaitForPlayerToTouchMe, //Wait until player bumps into this NPC.
		CC_ScoreCheckpoint,     //Defines a scoring point with identifier 'label'.
		CC_WaitForDefeat,       //Wait until the NPC-monster is defeated in combat.
		CC_AddRoomToMap,        //Add room at (x,y) to player's mapped rooms.  If w is set, room becomes explored.
		CC_Autosave,            //Autosave with identifier 'label'.
		CC_SetNPCAppearance,    //Sets this NPC to look like entity X.
		CC_WaitForOpenMove,     //Wait until NPC may take a step or turn toward orientation X.
		CC_Behavior,            //Activate behavior pattern X.
		CC_EachAttack,          //Goto label X each time I attack (i.e. hit the player).
		CC_EachDefend,          //Goto label X each time I defend (i.e. get hit).
		CC_EachUse,             //Goto label X each time I use this item (for custom inventory items).
		CC_Equipment,           //Give/take player equipment type X, defined by hold custom charID Y.
		CC_WaitForItem,         //Wait for game element (flags) to exist in rect (x,y,w,h).
		CC_GenerateEntity,      //Generates a new entity of type h in the room at (x,y) with orientation w.
		CC_GameEffect,          //Cues the front end to generate a graphic+sound effect (w,h,flags) at (x,y).
		CC_IfElseIf,            //Else combined with if to reduce code nesting
		CC_Return,              //Return to just after the previous CC_GoSub command executed.
		CC_GoSub,               //Jumps script execution to the indicated label.
		CC_EachVictory,         //Goto label X each time an enemy is defeated.
		CC_RoomLocationText,    //Sets the room location text for the current room.
		CC_FlashingText,        //Flashes a large message onscreen.  Use hex code RRGGBB color (x,y,w) when h is set.
		CC_SetMonsterVar,       //Sets monster at (x,y) attribute(w) to value (h).
		CC_SetMovementType,     //Sets the NPC's movement type to X.
		CC_ReplaceWithDefault,  //Replaces the character's script with its default script (if possible)
		CC_VarSetAt,            //Remotely set local variable w (with operation h) of NPC at (x,y)
		CC_Count
	};

	CharCommand command;
	UINT x, y, w, h, flags;
	WSTRING label;    //goto identifier
	CDbSpeech *pSpeech;

	static bool IsEachEventCommand(CharCommand command);
};

class CDbMessageText;
class CColorText : public CAttachableObject
{
public:
	CColorText(CDbMessageText* pText, int r, int g, int b, int customColor)
		: CAttachableObject()
		, pText(pText)
		, r(r), g(g), b(b)
		, customColor(customColor)
	{ }
	~CColorText();

	CDbMessageText* pText;
	int r, g, b;
	int customColor;
};

typedef std::vector<CCharacterCommand> COMMAND_VECTOR;
typedef std::vector<CCharacterCommand*> COMMANDPTR_VECTOR;

extern const UINT NPC_DEFAULT_SWORD;

extern bool    addWithClamp(int& val, const int operand);
extern SPEAKER getSpeakerType(const MONSTERTYPE eType);
extern UINT    getSpeakerNameText(const UINT wSpeaker, std::string& color);

#endif
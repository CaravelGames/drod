#ifndef CHARACTERCOMMAND_H
#define CHARACTERCOMMAND_H

#include "MonsterType.h"

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
	Speaker_Gunthro=51,
	Speaker_Citizen1=6,
	Speaker_Citizen2=7,
	Speaker_Citizen3=45,
	Speaker_Citizen4=46,
	Speaker_Custom=5,
	Speaker_EyeActive=15,
	Speaker_GoblinKing=8, //deprecated
	Speaker_Instructor=10, //deprecated
	Speaker_MudCoordinator=11, //deprecated
	Speaker_Negotiator=3, //deprecated
	Speaker_None=4,
	Speaker_TarTechnician=13, //deprecated
	Speaker_BeethroInDisguise=47,
	Speaker_Self=50,
	Speaker_Player=52,

	//Monster speakers.
	Speaker_Halph=1,
	Speaker_Slayer=2,
	Speaker_Goblin=9,
	Speaker_RockGolem=12,
	Speaker_Guard=14,
	Speaker_Stalwart=17,
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
	Speaker_Slayer2=48,
	Speaker_Halph2=49,
	Speaker_Stalwart2=53,
	Speaker_Architect=54,
	Speaker_Construct=55,
	Speaker_Gentryii=56,
	Speaker_TemporalClone=57,
	Speaker_FluffBaby=58,

	Speaker_Count=59,

	Speaker_HoldCharacter = static_cast<UINT>(-1)
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

	//How killing NPC affects the game
	enum Imperative
	{
		Vulnerable=0,        //can be killed (default)
		Invulnerable=1,      //can't be killed
		MissionCritical=2,   //must not be killed, or play restarts
		RequiredToConquer=3, //must be killed to conquer room
		Die=4,               //NPC dies
		DieSpecial=5,        //NPC dies in way distinct to specific monster type
		Safe=6,              //can't kill player by stepping (default)
		Deadly=7,            //can kill player by stepping and sword
		SwordSafeToPlayer=8, //sword doesn't damage player
		EndWhenKilled=9,     //End is invoked when NPC is killed
		FlexibleBeelining=10,//use SmartOmniDirection movement (default)
		DirectBeelining=11,  //use DirectOnly movement
		NoGhostDisplay=12,   //(f.e.) NPC isn't displayed when not visible (default)
		GhostDisplay=13,     //(f.e.) NPC is displayed when not visible -- i.e. a ghost
		NotPushable=14,      // character can not be pushed
		PushableByBody=15,   // character can be pushed with player body
		PushableByWeapon=16, // character can be pushed by weapons
		PushableByBoth=17,   // character can be pushed by both weapons and body
		Stunnable=18,        // character can be stunned like any monster
		NotStunnable=19,     // character is completely resistan to stunning
		GhostDisplayOverhead=20, //(f.e.) NPC is displayed when not visible, on top of all other entities
		DefaultPushability=21, //default character push behavior
		Pathfinding=22,      //pathfinding movement with beelining when blocked
		BrainPathmapObstacle=23,
		NotBrainPathmapObstacle=24,
		NPCPathmapObstacle=25,
		NotNPCPathmapObstacle=26,
		NormalBeelining=27,  //use Roach-like movement
		SmartBeelining=28,   //use Roach-like movement but with better orthogonal choice when diagonal is blocked
		PathfindingOpenOnly=29, //pathfinding movement, no movement when blocked
		InvisibleInspectable=30,   // Causes disappeared characters to be included when right clicking in the room
		InvisibleNotInspectable=31,
		InvisibleCountMoveOrder = 32,   // Causes disappeared characters to be counted when displaying move order on right click
		InvisibleNotCountMoveOrder = 33,
		Friendly = 34, // the character becomes 'friendly'. Allies will avoid stabbing it, and killing it won't break stealth.
		Unfriendly = 35 // the character becomes 'unfriendly'. Allies will stab it, and killing it will break stealth.
	};

	enum Behavior
	{
		ActivateTokens = 0,
		DropTrapdoors = 1,
		DropTrapdoorsArmed = 2,
		PushObjects = 3,
		MovePlatforms = 4,
		MonsterAttackable = 5,
		MonsterTarget = 6,
		MonsterTargetWhenPlayerIsTarget = 7,
		AllyTarget = 8,
		PuffTarget = 9,
		SwordDamageImmune = 10,
		PickaxeDamageImmune = 11,
		SpearDamageImmune = 12,
		DaggerDamageImmune = 13,
		CaberDamageImmune = 14,
		FloorSpikeImmune = 15,
		FiretrapImmune = 16,
		HotTileImmune = 17,
		ExplosionImmune = 18,
		BriarImmune = 19,
		AdderImmune = 20,
		PuffImmune = 21,
		ActivatePlates = 22,
		PushMonsters = 23,
		FatalPushImmune = 24,
		CanBeNPCBeethro = 25,
		RestrictedMovement = 26
	};

	enum DisplayFilter
	{
		D_Nothing=-1, //used in front-end
		D_Normal=0,
		D_BandW=1,
		D_Sepia=2,
		D_Negative=3
	};

	enum AttackTileType
	{
		AT_Stab=0,
		AT_Explode=1,
		AT_Damage=2,
		AT_Kill=3
	};

	enum TargettingType
	{
		RegularMonster = 0,
		BrainedMonster = 1,
		Guard=2,
		Puff=3,
		Stalwart=4
	};

	enum GotoSmartType
	{
		PreviousIf = -1,
		NextElseOrElseIfSkipCondition = -2
	};

	//World map icons
	static const UINT WMI_OFF        = 0x00000000; //remove icon when no flags are set
	static const UINT WMI_ON         = 0x00000001; //basic display
	static const UINT WMI_LEVELSTATE = 0x00000002; //display supplemental level state on icon
	static const UINT WMI_DISABLED   = 0x00000004; //disabled area
	static const UINT WMI_LOCKED     = 0x00000008; //locked area
	static const UINT WMI_NOLABEL    = 0x00000010; //disabled area with no label shown

	//Weapons
	static const UINT WEAPON_SWORD = 0x00000001; // Sword
	static const UINT WEAPON_PICKAXE = 0x00000002; // Pickaxe
	static const UINT WEAPON_SPEAR = 0x00000004; // Spear
	static const UINT WEAPON_STAFF = 0x00000008; // Staff
	static const UINT WEAPON_DAGGER = 0x00000010; // Dagger
	static const UINT WEAPON_CABER = 0x00000020; // Caber
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
		CC_GotoIf,             //(deprecated) Goto command with (label) if the next command is satisfied.
		CC_WaitForMonster,     //(deprecated) Wait until monster is in rect (x,y,w,h).
		CC_WaitForNotMonster,  //(deprecated) Wait until monster is not in rect (x,y,w,h).
		CC_WaitForTurn,         //Wait until play reaches turn X.
		CC_WaitForCleanRoom,    //Wait until room has been cleaned of monsters.
		CC_WaitForPlayerToFace, //Wait until player faces orientation X.
		CC_ActivateItemAt,      //Activate item at (x,y).  Works only for some items.
		CC_EndScript,           //Removes the character for the rest of the current game.
		CC_WaitForHalph,       //(deprecated) Wait until Halph is in rect (x,y,w,h).
		CC_WaitForNotHalph,    //(deprecated) Wait until Halph is not in rect (x,y,w,h).
		CC_WaitForCharacter,   //(deprecated) Wait until a visible character is in rect (x,y,w,h).
		CC_WaitForNotCharacter,//(deprecated) Wait until no visible character is in rect (x,y,w,h).
		CC_FlushSpeech,         //(Front end) Purge speech events in queue (x=display/erase)
		CC_Question,			//Ask a yes/no or multiple choice question (speech text).
		CC_SetMusic,			//Set music being played to X (optionally, custom Y/label).
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
		CC_PlayerEquipsWeapon,  //(deprecated) If X is set, player is given a weapon, else it is taken away.
		CC_AnswerOption,        //Text answer option (speech text) for a Question command that jumps to Label (x).
		CC_BuildMarker,         //Mark rect (x,y,w,h) for building game element (flags).
		CC_AmbientSound,        //Play sound with DataID=w (0 stops ambient sounds).  If h is set, loop indefinitely.
		CC_AmbientSoundAt,      //Play sound with DataID=w (0 stops ambient sounds) at (x,y).  If h is set, loop indefinitely.
		CC_WaitForNoBuilding,   //Wait until no build markers are queued in rect (x,y,w,h).
		CC_PlayVideo,           //Play video at (x,y) with DataID=w.
		CC_WaitForPlayerToMove, //Wait until player moves in direction X.
		CC_WaitForPlayerToTouchMe, //Wait until player bumps into this NPC.
		CC_SetNPCAppearance,    //Sets this NPC to look like entity X.

		//4.0 commands
		CC_SetWaterTraversal,	//Sets Shallow Water traversal for players to X.  Default value is 'As player role'
		CC_StartGlobalScript,   //Starts the specified custom character's default script up as a global script
		CC_WaitForItem,         //Wait for game element (flags) to exist in rect (x,y,w,h).
		CC_GenerateEntity,      //Generates a new entity of type h in the room at (x,y) with orientation w.
		CC_GameEffect,          //Cues the front end to generate a graphic+sound effect (w,h,flags) at (x,y).

		//5.0 commands
		CC_RoomLocationText,    //Sets the room location text.
		CC_FlashingText,        //Flashes a large message onscreen.  Use hex code RRGGBB color (x,y,w) when h is set.
		CC_DisplayFilter,       //Displays the room with a certain visual ambiance.
		CC_Build,               //Fill rect (x,y,w,h) with building game element (flags).
		CC_WorldMapMusic,       //Sets the music to be played to X (optionally, custom Y/label).
		CC_WorldMapIcon,        //Places an icon on the world map at (x,y), to level entrance (w), displayed as character type (h), of display type (flags).
		CC_WorldMapSelect,      //Sets the world map that other world map commands operate on to X.
		CC_SetPlayerWeapon,     //Sets the player weapon type to X (incl. on/off -- replaces CC_PlayerEquipsWeapon).
		CC_WaitForSomeoneToPushMe, //Waits until someone pushes this character if it has imperative "pushable"
		CC_WaitForOpenMove,     //Waits for a move in (x=orientation) to be open
		CC_WaitForCleanLevel,   //Waits for the level to be cleaned (not for the clean level event!)
		CC_ChallengeCompleted,  //Indicates scripted requirements for an architect's challenge (text) have been satisfied.
		CC_AttackTile,          //Attempts to damage entities at (x,y) with attack type (flags).
		CC_SetPlayerStealth,    //Either temporarily grant stealth to the player or automatically set the player as a monster target
		CC_WaitForPlayerInput,  //Waits for player to input command X
		CC_Return,              //Return to just after the previous CC_GoSub command executed.
		CC_GoSub,               //Jumps script execution to the indicated label.
		CC_ImageOverlay,        //Display image (w) with display strategy (text).
		CC_WorldMapImage,       //Places an icon on the world map at (x,y), to level entrance (w), displayed as image (h), of display type (flags).
		CC_WaitForEntityType,   //Wait until an entity type in flags is in rect (x,y,w,h).
		CC_WaitForNotEntityType,//Wait until an entity type in flags is not in rect (x,y,w,h).
		CC_TeleportTo,          //Teleport the NPC to (x,y)
		CC_TeleportPlayerTo,    //Teleport the Player to (x,y)
		CC_DestroyTrapdoor,     //Destroy Trapdoors in rect (x,y,w,h)
		CC_IfElseIf,            //Else combined with if to reduce code nesting
		CC_FaceTowards,         //Makes the character face a specific other entity or position
		CC_GetNaturalTarget,   //Finds the location of closest natural target
		CC_GetEntityDirection, //Finds the direction an entity at a given tile is facing
		CC_WaitForWeapon,		//Wait until a weapon is at (x,y).
		CC_Behavior,			// Set behavior X on or off.
		CC_WaitForRemains,      //Wait until a dead monster type in flags is in rect (x,y,w,h).
		CC_PushTile,            //Push object or entity at (x,y) in direction (w)
		CC_SetMovementType,     //Sets the character's movement type to X.
		CC_ReplaceWithDefault,  //Replaces the character's script with its default script (if possible)
		CC_VarSetAt,            //Remotely set local variable w (with operation h) of NPC at (x,y)

		CC_Count
	};

	CharCommand command;
	UINT x, y, w, h, flags;
	WSTRING label;    //goto identifier
	CDbSpeech *pSpeech;

	bool IsMusicCommand() const {
		switch (command) {
			case CC_SetMusic:
			case CC_WorldMapMusic:
				return true;
			default:
				return false;
		}
	}
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

	CDbMessageText *pText;
	int r, g, b;
	int customColor;
};

struct ImageOverlayCommand
{
	static const int NO_LOOP_MAX;
	static const int DEFAULT_LAYER;
	static const int ALL_LAYERS;
	static const int NO_LAYERS;

	enum IOC {
		CancelAll,
		CancelLayer,
		Center,
		DisplayDuration,
		DisplayRect,
		DisplaySize,
		FadeToAlpha,
		Grow,
		Jitter,
		Layer,
		Loop,
		Move,
		MoveTo,
		ParallelFadeToAlpha,
		ParallelGrow,
		ParallelJitter,
		ParallelMove,
		ParallelMoveTo,
		ParallelRotate,
		Rotate,
		Scale,
		SetAlpha,
		SetAngle,
		SetX,
		SetY,
		SrcXY,
		TurnDuration,
		Invalid
	};

	ImageOverlayCommand(IOC type, int val[4])
		: type(type)
	{
		memcpy(this->val, val, 4 * sizeof(int));
	}

	IOC type;
	int val[4];
};
typedef std::vector<ImageOverlayCommand> ImageOverlayCommands;

class CImageOverlay : public CAttachableObject
{
public:
	CImageOverlay(const WSTRING& text, UINT imageID, UINT instanceID)
		: CAttachableObject()
		, imageID(imageID)
		, instanceID(instanceID)
	{
		parse(text, commands);
	}
	~CImageOverlay();

	static bool parse(const WSTRING& wtext, ImageOverlayCommands& commands);

	int getLayer() const;
	int clearsImageOverlays() const;
	bool loopsForever() const;

	ImageOverlayCommands commands;
	UINT imageID;
	UINT instanceID; //for merging effect sets during play on move undo/checkpoint restore
};

typedef std::vector<CCharacterCommand> COMMAND_VECTOR;
typedef std::vector<CCharacterCommand*> COMMANDPTR_VECTOR;

extern SPEAKER getSpeakerType(const MONSTERTYPE eType);
extern UINT getSpeakerNameText(const UINT wSpeaker, std::string& color);

#endif
// $Id: CueEvents.h 10193 2012-05-16 21:27:53Z skell $

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
 * Michael Welsh Duggan (md5i), JP Burford (jpburford), John Wm. Wicks (j_wicks),
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//CCueEvents class declaration, cue event IDs, and classification macros.

//GENERAL
//
//Cue events are messages returned to the caller of CCurrentGame::ProcessCommand().
//Typically, these messages would correspond to game events that need a sound
//or graphical effect that the caller should handle.  The ProcessCommand caller is
//not required to handle any of the cue events.  Since CCurrentGame does nothing
//UI-related it is possible to run games "silently" without any UI.
//
//Each cue event may have one or more private data pointers (void *pvPrivateData)
//associated with it.  Some cue events will use this to point to data associated with
//the cue event.  You will know by the cue event ID which class or data type is
//pointed to, and can cast to that associated type for access to the private data.
//The private data pointer should not be deleted.
//
//USAGE
//
//Call Add() to say that an event occurred and optionally associate private data that
//may be used later to react to that event.  For example when a monster is killed,
//a CMonster pointer is associated with the CID_MonsterDiedFromStab event.  If three
//monsters were killed on a turn, then three CMonster pointers will be available for
//retrieval.
//
//To simply check whether events occurred, use HasOccurred() or HasAnyOccurred().  The
//IDCOUNT() macro defined in IDList.h should be used for calls to HasAnyOccurred().
//
//To retrieve private data, call GetFirstPrivateData(), and follow up with calls to
//GetNextPrivateData() if appropriate.  GetFirstPrivateData() will cause
//GetNextPrivateData() to return the second private data associated with the specified
//cue event if available.  GetFirstPrivateData() should also be used in lieu of
//HasOccurred() to check for a cue event when private data may be retrieved.  If you
//just want to check an ID without retrieving private data, then call HasOccurred() or
//HasAnyOccurred().
//
//Never delete private data pointers retrieved from cue events.  Deletion will be
//handled in the CueEvents destructor or in CCurrentGame depending on the design
//of the cue event.
//
//DESIGN CONSIDERATIONS FOR CREATING NEW CUE EVENTS
//
//Cue events are mainly for description of UI-related events, but there are other
//tasks where they can be put to good use, for example:
//
//When a monster gets killed by sword stab, CMonster::OnStabbed() passes back a
//CID_MonsterDiedFromStab cue event, and CCurrentGame::ProcessPlayer() handles
//the cue event with a call that deletes the CMonster instance which had its
//OnStabbed() member called.  It would be possible to execute "delete this" within
//OnStabbed(), but to avoid problems with bad pointers, the deletion is handled
//outside of CMonster code using the cue event.
//
//The tar mother is another example.  She causes tar in a room to grow every 30 turns.
//If there are multiple tar mothers in the room, the tar should grow just once--not for
//each tar mother processed.  By checking for a CID_TarGrew cue event after all of the
//tar mothers have been processed, we can make the tar grow just once if there are any
//tar mothers in the room and 30 turns have passed.  If the tar growing was performed
//up in CTarMother::Process() there would need to be some kind of state-checking
//between the tar mothers to ensure the tar grew one time only.
//
//There may be other valid uses of cue events.  Here are some other guidelines for
//making cue events.
//
//1. Avoid creating cue events that don't correspond to actions which the
//ProcessCommand() caller will handle.  The two cases described above are
//valid exceptions.  Typically, you can handle a purely game-logic event in the code
//that determines the event should occur.  For example, in ProcessPlayer() you
//can detect that the swordsman has destroyed the last trapdoor and make a call to
//RemoveRedDoors() from that routine.  This is preferable to passing a
//CID_RemoveRedDoors (fictitious) cue event back to ProcessCommand() or to its
//caller for handling the removal of red doors.  A local handling of an event is
//usually clearer to other developers and requires less time to process during
//execution.
//
//2. Wait for a UI feature to be planned before adding a cue event that may or may
//not be handled by the ProcessCommand() caller in the future.  It's easy to add
//cue events later, and we don't want to have the clutter and inefficiency of unused
//cue events.
//
//PRIVATE DATA VALIDITY GUARANTEE
//
//Private data pointers returned by ProcessCommand() are guaranteed to be valid
//until the CCurrentGame instance that originally returned them goes out
//of scope or the ProcessCommand() method is called again on the same instance.
//
//Conversely, when creating new cue events that have private data, you should design
//so that the above guarantee is honored.  If your private data object is only used
//to send data to the ProcessCommand caller, then probably you should set the
//bIsAttached parameter to true in your call to CCueEvents::Add().  Otherwise, the
//bIsAttached parameter should be false, and you must make sure that the pointer
//will be good at least until the next call to ProcessCommand().  This should usually
//be accomplished by using a member variable of CCurrentGame to hold the data to
//which you will return a pointer.

#ifndef CUEEVENTS_H
#define CUEEVENTS_H

#include <BackEndLib/Assert.h>
#include <BackEndLib/Types.h>

#include <memory.h>
#include <vector>
using std::vector;

//
//Cue event IDs.
//

enum CUEEVENT_ID
{
	//A monster killed the swordsman.
	//
	//Private data: CMonster *pMonsterThatKilledPlayer (one)
	CID_MonsterKilledPlayer = 0,

	//A serpent died when it got too short to live.
	//
	//Private data: CSerpent *pSerpentThatDied (one or more)
	CID_SnakeDiedFromTruncation,

	//A monster died from a stab--could be from the swordsman or a mimic.
	//
	//Private data: CMonster *pMonsterThatDied (one or more)
	CID_MonsterDiedFromStab,

	//The swordsman exited the room by walking across one of its four borders.  The
	//new room is loaded now.  Contrast to CID_ExitRoomPending which indicates that
	//the room has not yet been loaded.
	//
	//Private data: UINT *pwExitOrientation (one)
	CID_ExitRoom,

	//The swordsman has exited the level, but the new level has not been loaded yet.
	//The destination level entrance ID + flags is attached for the ProcessCommand() caller to
	//initiate the level load in response.
	//
	//Private data: CCoord *exitInfo (one)
	CID_ExitLevelPending,

	//Player has won the game.  The current room/level remains loaded.  The
	//ProcessCommand() caller may choose to transition to a win screen.
	//
	//Private data: NONE
	CID_WinGame,

	//Player is swimming in water.
	//
	//Private data: NONE
	CID_Swim,

	//Enough rooms in level have been conquered for the level to be considered "complete".
	//In the level-complete state, blue doors will be open.
	//
	//Private data: NONE
	CID_CompleteLevel,

	//The player cut a tile of briar with his sword.
	//
	//Private data: CMoveCoord *pTileDestroyedAt (one or more)
	CID_CutBriar,

	//The player stepped onto and is currently in a square containing a scroll.
	//
	//Private data: CDbMessageText *pScrollText (one)
	CID_StepOnScroll,

	//A goblin attacks the player from behind.
	//
	//Private data: NONE
	CID_GoblinAttacks,

	//A score checkpoint is triggered.
	//
	//Private data: CDbMessageText *pScoreIDText (one)
	CID_ScoreCheckpoint,

	//If any calls to CDbRoom::Plot() were made in the current room, this event will
	//fire.  Useful for making efficient updates to display structures.
	//
	//Private data: CCoordSet *plots (one)
	CID_Plots,

	//The swordsman or a mimic stepped off a trapdoor square and that square now
	//contains a pit.
	//
	//Private data: CCoord *pSquareRemovedAt (one or more)
	CID_TrapDoorRemoved,

	//The swordsman or a mimic stabbed a square of tarstuff and destroyed it.
	//
	//Private data: CMoveCoord *pTileDestroyedAt (one or more)
	CID_TarstuffDestroyed,

	//One or more tar mothers are present in the room and caused the tar to grow at
	//the tar growth interval.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_TarGrew,

	//A monster sent a message to the player.  It could be a simple statement for
	//the ProcessCommand() caller to display.  Or it could be a question that the
	//player must answer.  In the second case, ProcessCommand(), on the next call, will
	//be expecting a an appropriate command constant that indicates the player's answer.
	//It is possible for multiple messages to be attached to this event.
	//
	//Private data: CMonsterMessage *pMonsterMessage (one or more)
	CID_MonsterSpoke,

	//When player jumps a hole.
	CID_Jump,

	//The swordsman exited the room by walking across one of its four borders.  The
	//new room is not loaded now.  The ProcessCommand() caller may load the new room
	//with a call to LoadNewRoomForExit().  Contrast to CID_ExitRoom which indicates that
	//the room has been loaded.
	//
	//Private data: UINT *pwExitOrientation (one)
	CID_ExitRoomPending,

	//The swordsman or a mimic stabbed a crumbly wall square and destroyed it.
	//
	//Private data: CCoord *pSquareDestroyedAt (one or more)
	CID_CrumblyWallDestroyed,

	//The swordsman hit an orb.
	//
	//Private data: COrbData *pOrbData (none, one or more)
	CID_OrbActivatedByPlayer,

	//A player double hit an orb.
	//
	//Private data: COrbData *pOrbData (none, one or more)
	CID_OrbActivatedByDouble,

	//Player bumped a closed door and couldn't open it.
	//
	//Private data: CCoord *pDoorLocation (one)
	CID_BumpedLockedDoor,

	//Player consumed a speed/invisibility potion.
	//
	//Private data: NONE
	CID_DrankPotion,

	//Monster burned (possibly killed) by hot tile.  Monster pointer(s) are attached.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_MonsterBurned,

	//Swordsman ran into obstacle.
	//
	//Private data: CMoveCoord *pObstacle (one)
	CID_HitObstacle,

	//Swordsman swung sword in new direction.
	//
	//Private data: UINT (one)
	CID_SwingSword,

	//Swordsman got scared (almost stepped into pit).
	//
	//Private data: NONE
	CID_Scared,

	//Swordsman took a step.
	//
	//Private data: NONE
	CID_Step,

	//Swordsman afraid (monster has upper hand).
	//
	//Private data: NONE
	CID_SwordsmanAfraid,

	//Swordsman agressive (has upper hand on monster).
	//
	//Private data: NONE
	CID_SwordsmanAggressive,

	//Swordsman normal (not close to monsters).
	//
	//Private data: NONE
	CID_SwordsmanNormal,

	//An evil eye attacked the player.
	//
	//Private data: NONE
	CID_EvilEyeWoke,

	//A tar baby was formed.
	//
	//Private data: CTarBaby *pTarBaby (one or more)
	CID_TarBabyFormed,

	//The game was autosaved.
	//
	//Private data: NONE
	CID_Autosave,

	//The player has used an item.
	//
	//Private data: CMoveCoord *pMoveCoord (one), indicating (x,y,itemType)
	CID_ItemUsed,

	//A money door has been opened.
	//
	//Private data: NONE
	CID_MoneyDoorOpened,

	//The player has used the current accessory.
	//
	//Private data: NONE
	CID_AccessoryUsed,

	//The player has just finished killing a horde of monsters.
	//
	//Private data: NONE
	CID_SwordsmanTired,

	//All trapdoors have been removed.
	//
	//Private data: true (one) if red doors were opened also
	CID_AllTrapdoorsRemoved,

	//All tarstuff has been removed.
	//
	//Private data: NONE.
	CID_AllTarRemoved,

	//Player tried to fight a monster via sword hit but couldn't damage the monster.
	//
	//Private data: NONE
	CID_InvalidAttackMove,

	//When large monster piece is stabbed and destroyed.
	//
	//Private data: CMoveCoord *pSquareDestroyedAt (one or more)
	CID_MonsterPieceStabbed,

	//Bomb fuse is burning.
	//Orientation means: NO_ORIENTATION = new fuse initiated, else = direction fuse is burning
	//
	//Private data: CMoveCoord *pSquare (one or more)
	CID_FuseBurning,

	//Bomb exploded.
	//
	//Private data: CMoveCoord *pSquare (one or more)
	CID_BombExploded,

	//Explosion affected this tile.
	//
	//Private data: CCoord *pSquare (one or more)
	CID_Explosion,

	//Explosion killed player.
	//
	//Private data: NONE
	CID_ExplosionKilledPlayer,

	//The activated accessory may not be used.
	//
	//Private data: NONE
	CID_CantUseAccessory,

	//Player (or stalwart) has been frozen by zombie.
	//
	//Private data: CMoveCoord* (one or more)
	CID_PlayerFrozen,

	//Zombie gaze emanating from here.
	//
	//Private data: CMonster *pZombie (one or more)
	CID_ZombieGaze,

	//Nothing can be locked at this tile.
	//
	//Private data: NONE
	CID_CantLockHere,

	//Black gates were toggled.
	//
	//Private data: NONE
	CID_BlackGatesToggled,

	//Player obtained a level map.
	//
	//Private data: UINT (map tile type)
	CID_LevelMap,

	//Red gates were toggled.
	//
	//Private data: NONE
	CID_RedGatesToggled,

	//Player bumps against the edge of deep water.
	//
	//Private data: NONE
	CID_PlayerOnWaterEdge,

	//Player was stabbed by an enemy's sword (not during combat).
	//
	//Private data: NONE
	CID_PlayerStabbed,

	//Cues the front end to generate a visual effect.  Attached object contains display info.
	//
	//Private data: VisualEffectInfo* (one or more)
	CID_VisualEffect,

	//An NPC was defeated (not killed) in combat.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_NPC_Defeated,

	//A door was opened by knocking it
	//
	//Private data: NONE
	CID_KnockOpenedDoor,

	//Player locks a doors
	//
	//Private data: NONE
	CID_DoorLocked,

	//This secret room was entered for the first time.
	//
	//Private data: NONE
	CID_SecretRoomFound,

	//A monster sent a speech message to the player.
	//
	//Private data: CFiredCharacterCommand*
	CID_Speech,

	//One or more mud mothers are present in the room and caused the mud to grow at
	//the mud growth interval.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_MudGrew,

	//A mud baby was formed.
	//
	//Private data: CMudBaby *pMudBaby (one or more)
	CID_MudBabyFormed,

	//Multiple swords hit together.
	//
	//Private data: NONE
	CID_Swordfight,

	//Player used portable orb accessory.
	//
	//Private data: NONE
	CID_PortableOrbActivated,

	//All brains in the room have been removed.
	//
	//Private data: NONE
	CID_AllBrainsRemoved,

	//Beethro walked through a tunnel.
	//
	//Private data: NONE
	CID_Tunnel,

	//Something other than a sword hit from a player or monster hit an orb.
	//
	//Private data: COrbData *pOrbData (none, one or more)
	CID_OrbActivated,

	//The player can't exit the room here, because the adjoining room
	//contains an obstacle.
	//
	//Private data:
	CID_ExitBlockedOnOtherSide,

	//The player can't exit the room here because there is no adjoining room
	//in the direction of exit.
	//
	//Private data:
	CID_NoAdjoiningRoom,

	//An ambient sound effect is played.  Attached object contains play info.
	//
	//Private data: CMoveCoordEx* (one or more)
	CID_AmbientSound,

	//A mirror was broken.
	//
	//Private data: CMoveCoord *pSquare (one or more)
	CID_MirrorShattered,

	//Object fell out of room.
	//
	//Private data: CMoveCoordEx2(x, y, orientation, monster type, weapon type) (one or more)
	CID_ObjectFell,

	//Player burned by hot tile.
	//
	//Private data: NONE
	CID_PlayerBurned,

	//Private data: NONE
	CID_PlayerFellIntoPit,

	//One or more gel mothers are present in the room and caused the gel to grow at
	//the gel growth interval.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_GelGrew,

	//A gel baby was formed.
	//
	//Private data: CGelBaby *pGelBaby (one or more)
	CID_GelBabyFormed,

	//Private data: NONE
	CID_PlayerDrownedInWater,

	//A pressure plate was activated (depressed).
	//
	//Private data: COrbData *pOrbData (none, one or more)
	CID_PressurePlate,

	//A pressure plate was reset.
	//
	//Private data: COrbData *pOrbData (none, one or more)
	CID_PressurePlateReleased,

	//Mission critical character died.
	//
	//Private data: NONE
	CID_CriticalNPCDied,

	//Briar killed player.
	//
	//Private data: NONE
	CID_BriarKilledPlayer,

	//A token was toggled.
	//
	//Private data: NONE
	CID_TokenToggled,

	//A light changed on/off state.
	//
	//Private data: NONE
	CID_LightToggled,

	//Splash.
	//
	//Private data: CCoord *pSquare (one or more)
	CID_Splash,

	//A character NPC was killed.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_NPCKilled,

	//An object was built.
	//
	//Private data: CMoveCoord* (one or more)
	CID_ObjectBuilt,

	//Signal to play a video: at (x,y) with dataID==wO.
	//
	//Private data: CMoveCoord* (one or more)
	CID_PlayVideo,

	//Briar expanded from a root.
	//
	//Private data: NONE
	CID_BriarExpanded,

	//A money door has been locked.
	//
	//Private data: NONE
	CID_MoneyDoorLocked,

	//An entity was affected somehow, usually during combat.
	//
	//Private data: CCombatEffect *pEffect (one or more)
	CID_EntityAffected,

	//Player engaged a monster for combat.
	//
	//Private data: CMonster *pMonster (one)
	CID_MonsterEngaged,

	//Player acquired a key.
	//
	//Private data: BYTE (one) -- type of key
	CID_ReceivedKey,

	//Player received sword or shield.
	//
	//Private data: NONE
	CID_ReceivedEquipment,

	//Player received ATK gem.
	//
	//Private data: NONE
	CID_ReceivedATK,

	//Player received DEF gem.
	//
	//Private data: NONE
	CID_ReceivedDEF,

	//Player received HP elixir.
	//
	//Private data: NONE
	CID_ReceivedHP,

	//Change to custom room location text.
	//
	//Private data: NONE
	CID_RoomLocationTextUpdate,

	//Change to custom room location text.
	//
	//Private data: CColorText* (one or more)
	CID_FlashingMessage,

	//An orb was damaged (and not activated).
	//
	//Private data: COrbData *pOrbData (none, one or more)
	CID_OrbDamaged,

	//Player received Shovel item.
	//
	//Private data: UINT (one) -- type of shovel
	CID_ReceivedShovel,

	//A diggable block was dug out.
	//
	//Private data: CMoveCoord* (one or more)
	CID_Dig,

	//A thin ice tile melts
	//
	//Private data: CCoord *pSquare (one or more)
	CID_ThinIceMelted,

	//Image overlay script command
	//
	//Private data: CImageOverlay* (one or more)
	CID_ImageOverlay,

	//A mist tile is destroyed
	//
	//Private data: CMoveCoord *pSquareDestroyedAt (one or more)
	CID_MistDestroyed,

	//Firetrap burning this turn.
	//
	//Private data: CCoord *pSquare (one or more)
	CID_Firetrap,

	//Firetrap has been activated.
	//
	//Private data: CCoord *pSquare (one or more)
	CID_FiretrapActivated,

	//A crate was destroyed
	//
	//Private data: CMoveCoord *pSquare
	CID_CrateDestroyed,

	//Fire trap hits a player or monster
	//
	//Private data: CCoord *pSquare
	CID_FiretrapHit,

	//A roach queen spawned a roach egg.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_EggSpawned,

	//End of enumeration typedef.
	CUEEVENT_COUNT
};

static inline bool IS_VALID_CID(const CUEEVENT_ID cid)   {return (UINT)(cid) < CUEEVENT_COUNT;}

//
//Cue event comparison ID arrays.
//

//Simplifies passing array count param to HasAnyOccurred().
#define IDCOUNT(dw) (sizeof(dw) / sizeof(UINT))

//Has player left room or will he shortly be leaving/restarting?  Another way
//of looking at: are any more commands going to be processed in the current room.
extern const CUEEVENT_ID CIDA_PlayerLeftRoom[10];

//Did something kill the player?  This array should be checked for death instead
//of these separate events so that future causes of death will be checked without
//project-wide changes to code.
extern const CUEEVENT_ID CIDA_PlayerDied[6];

//Did a monster die?
extern const CUEEVENT_ID CIDA_MonsterDied[2];

//Was a monster stabbed?
extern const CUEEVENT_ID CIDA_MonsterStabbed[3];

class CAttachableObject;

//
//Linked list node for storing private data.
struct CID_PRIVDATA_NODE
{
	CID_PRIVDATA_NODE(const bool bIsAttached, const CAttachableObject *pvPrivateData)
		: bIsAttached(bIsAttached), pvPrivateData(pvPrivateData) { }

	bool bIsAttached;
	const CAttachableObject *pvPrivateData;
};

//******************************************************************************************
class CCueEvents
{
public:
	CCueEvents() {Zero();}
	virtual ~CCueEvents() {Clear();}

	void     Add(const CUEEVENT_ID eCID, const CAttachableObject *pvPrivateData = NULL, const bool bIsAttached=false);
	void     Clear();
	void		ClearEvent(const CUEEVENT_ID eCID, const bool bDeleteAttached=true);
	const CAttachableObject *     GetFirstPrivateData(const CUEEVENT_ID eCID);
	const CAttachableObject *     GetNextPrivateData();
	inline UINT GetEventCount() const {return this->wEventCount;}
	UINT     GetOccurrenceCount(const CUEEVENT_ID eCID) const;
	inline bool HasOccurred(const CUEEVENT_ID eCID) const {ASSERT(IS_VALID_CID(eCID)); return this->barrIsCIDSet[eCID];}
	bool     HasOccurredWith(const CUEEVENT_ID eCID, const CAttachableObject *pvPrivateData) const;
	bool     HasAnyOccurred(const UINT wCIDArrayCount, const CUEEVENT_ID *peCIDArray) const;
	bool     Remove(const CUEEVENT_ID eCID, const CAttachableObject *pvPrivateData);

protected:
	UINT     wNextPrivateDataIndex, wNextCID;

	bool           barrIsCIDSet[CUEEVENT_COUNT];
	vector<CID_PRIVDATA_NODE>  CIDPrivateData[CUEEVENT_COUNT];

	UINT     wEventCount;

private:
	void     Zero();

	PREVENT_DEFAULT_COPY(CCueEvents);
};

#endif //...#ifndef CUEEVENTS_H


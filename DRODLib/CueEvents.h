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

	//The room has been conquered.  This will occur when the player has exited a
	//room cleared of monsters, or when the player enters an unvisited room that contains
	//no monsters.
	//
	//Private data: NONE
	CID_ConquerRoom,

	//Enough rooms in level have been conquered for the level to be considered "complete".
	//In the level-complete state, blue doors will be open.
	//
	//Private data: NONE
	CID_CompleteLevel,

	//The swordsman stepped onto and is currently in a square containing a scroll.
	//
	//Private data: WCHAR *pwczScrollText (one)
	CID_StepOnScroll,

	//A roach egg has hatched.  This event is used by ProcessMonsters() to delete
	//the CRoachEgg monster.  A new CRoach monster will be present in the same
	//square.
	//
	//Private data: CRoachEgg *pEggThatHatched (one or more)
	CID_EggHatched,

	//Halph can no longer reach an orb.
	//
	//Private data: NONE
	CID_HalphInterrupted,

	//Halph told to open same door again.
	//
	//Private data: NONE
	CID_HalphHurryUp,

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
	//Private data: CMoveCoord *pSquareDestroyedAt (one or more)
	CID_TarstuffDestroyed,

	//One or more tar mothers are present in the room and signalled tar to grow at
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

	//The 'Neather has exited the room.  Used by ProcessMonsters() to delete
	//CNeather and CCharacter monsters.
	//
	//Private data: CMonster* (one or more)
	CID_MonsterExitsRoom,

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

	//All the criteria for conquering the room were just satisfied.
	//The room will be conquered when the player exits the room.
	//
	//Private data: true (one) if any green doors were toggled also
	CID_RoomConquerPending,

	//Swordsman drank (mimic/invisibility) potion.
	//
	//Private data: NONE
	CID_DrankPotion,

	//Swordsman placed a double.
	//
	//Private data: NONE
	CID_DoublePlaced,

	//Swordsman ran into obstacle.
	//
	//Private data: CMoveCoord *pObstacle (one)
	CID_HitObstacle,

	//Swordsman swung sword in new direction.
	//
	//Private data: CCoord (one)
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

	//An evil eye woke up.
	//
	//Private data: CMoveCoord *pCoord (one or more -- where gaze originates)
	CID_EvilEyeWoke,

	//A tar baby was formed.
	//
	//Private data: CTarBaby *pTarBaby (one or more)
	CID_TarBabyFormed,

	//The swordsman stepped on a checkpoint.
	//
	//Private data: CCoord *pSquare (one)
	CID_CheckpointActivated,

	//The 'Neather has just pulled off a particularly nasty move,
	//like trapping the player or releasing a horde of goblins on him.
	//
	//Private data: CNeather *pNeather (one)
	CID_NeatherLaughing,

	//The 'Neather's best laid plans have failed.
	//
	//Private data: CNeather *pNeather (one)
	CID_NeatherFrustrated,

	//Beethro has gotten a little too close to the 'Neather.
	//
	//Private data: CNeather *pNeather (one)
	CID_NeatherScared,

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
	//Private data: NONE
	CID_AllTarRemoved,

	//Player tried to exit room when room lock is enabled.
	//
	//Private data: NONE
	CID_RoomExitLocked,

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
	//Private data: CCoord *pSquare (one or more)
	CID_BombExploded,

	//Explosion affected this tile.
	//
	//Private data: CCoord *pSquare (one or more)
	CID_Explosion,

	//Explosion killed player.
	//
	//Private data: NONE
	CID_ExplosionKilledPlayer,

	//NPC changed type.
	//
	//Private data: NONE
	CID_NPCTypeChange,

	//Player (or stalwart) has been frozen by aumtlich.
	//
	//Private data: CMoveCoord* (one or more)
	CID_PlayerFrozen,

	//Aumtlich gaze emanating from here.
	//
	//Private data: CMonster *pAumtlich (one or more)
	CID_AumtlichGaze,

	//Wubba stabbed by sword.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_WubbaStabbed,

	//Close combat with a Slayer has commenced.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_SlayerCombat,

	//Halph entered the room.
	//
	//Private data: NONE
	CID_HalphEntered,

	//Halph died.
	//
	//Private data: NONE
	CID_HalphDied,

	//Halph starts following the player.
	//
	//Private data: NONE
	CID_HalphFollowing,

	//Halph stops following the player.
	//
	//Private data: NONE
	CID_HalphWaiting,

	//Halph can open the requested yellow door.
	//
	//Private data: NONE
	CID_HalphStriking,

	//Halph can't open the requested yellow door.
	//
	//Private data: NONE
	CID_HalphCantOpen,

	//Slayer entered the room.
	//
	//Private data: NONE
	CID_SlayerEntered,

	//Slayer's wisp is connected with the player.
	//
	//Private data: NONE
	CID_WispOnPlayer,

	//This secret room was entered for the first time.
	//
	//Private data: NONE
	CID_SecretRoomFound,

	//A monster sent a speech message to the player.
	//
	//Private data: CFiredCharacterCommand*
	CID_Speech,

	//One or more mud mothers are present in the room and signalled the mud to grow at
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
	//Private data: int (weapon type)
	CID_Swordfight,

	//Music has changed to type (X,Y) or (text).
	//
	//Private data: CCoord or WSTRING (one or more)
	CID_SetMusic,

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

	//A fegundo has burned.  This event is used by ProcessMonsters() to delete
	//the CFegundo monster.  A new CFegundoAshes monster will be present in the same
	//square.
	//
	//Private data: CFegundo *pPhoenix (one or more)
	CID_FegundoToAsh,

	//A fegundo has been re-born.  This event is used by ProcessMonsters() to delete
	//the CFegundoAshes monster.  A new CFegundo monster will be present in the same
	//square.
	//
	//Private data: CFegundoAshes *pAshes (one or more)
	CID_AshToFegundo,

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
	//Private data: CMoveCoordEx2(x, y, orientation, monster type, weapon type)
	CID_ObjectFell,

	//Player burned (killed) by hot tile.
	//
	//Private data: NONE
	CID_PlayerBurned,

	//Monster burned (killed) by hot tile.  Monster pointer(s) are attached.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_MonsterBurned,

	//One or more gel mothers are present in the room and signalled the gel to grow at
	//the gel growth interval.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_GelGrew,

	//A gel baby was formed.
	//
	//Private data: CGelBaby *pGelBaby (one or more)
	CID_GelBabyFormed,

	//NPC Beethro was killed by a monster.
	//
	//Private data: CMonster *pMonsterThatKilledPlayer (one)
	CID_NPCBeethroDied,

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

	//Briar killed the player.
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

	//Water splash.
	//
	//Private data: CCoord *pSquare (one or more)
	CID_Splash,

	//A character NPC was killed.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_NPCKilled,

	//An object was built.
	//
	//Private data: UINT (one or more)
	CID_ObjectBuilt,

	//Signal to play a video: at (x,y) with dataID==wO.
	//
	//Private data: CMoveCoord* (one or more)
	CID_PlayVideo,

	//Briar expanded from a root.
	//
	//Private data: NONE
	CID_BriarExpanded,

	//An orb was damaged (and not activated).
	//
	//Private data: COrbData *pOrbData (none, one or more)
	CID_OrbDamaged,

	//The hold was mastered by conquering the final secret room after the
	//hold has already been completed.
	//
	//Private data: NONE
	CID_HoldMastered,

	//The player bumped into a closed master wall.
	//
	//Private data: NONE
	CID_BumpedMasterWall,

	//Black gates were toggled.
	//
	//Private data: NONE
	CID_BlackGatesToggled,

	//Red gates were toggled.
	//
	//Private data: NONE
	CID_RedGatesToggled,

	//Command Key was used.
	//
	//Private data: NONE
	CID_CommandKeyPressed,

	//Cues the front end to generate a visual effect.  Attached object contains display info.
	//
	//Private data: VisualEffectInfo* (one or more)
	CID_VisualEffect,

	//Squad horn.
	//
	//Private data: NONE
	CID_Horn_Squad,

	//Soldier horn.
	//
	//Private data: NONE
	CID_Horn_Soldier,

	//Ripples from wading in water.
	//
	//Private data: CCoord *pSquare (one or more)
	CID_Wade,

	//Combo of some type.
	//
	//Private data: UINT (one or more)
	CID_Combo,

	//Play when a horn summon has no effect.
	CID_HornFail,

	//Change to custom room location text.
	//
	//Private data: NONE
	CID_RoomLocationTextUpdate,

	//Change to custom room location text.
	//
	//Private data: CColorText* (one or more)
	CID_FlashingMessage,

	//Display filter is modified.
	//
	//Private data: NONE
	CID_SetDisplayFilter,

	//Stun effect.
	//
	//Private data: CStunTarget *pStunTarget (one or more)
	CID_Stun,

	//A tile of briar was cut with a weapon.
	//
	//Private data: CMoveCoord *pTileDestroyedAt (one or more)
	CID_CutBriar,

	//A construct monster reactivated.
	//
	//Private data: NONE
	CID_ConstructReanimated,

	//Spikes are ready to be released.
	//
	//Private data: NONE
	CID_SpikesPoised,

	//Spikes have been released.
	//
	//Private data: CMoveCoord *pSquare (one or more)
	CID_SpikesUp,

	//Player was stabbed by spikes.
	//
	//Private data: NONE
	CID_PlayerImpaled,

	//Requirements for a scripted challenge were satisfied by the player.
	//
	//Private data: WCHAR *pwczText (one or more)
	CID_ChallengeCompleted,

	//Player activated the temporal split.
	//
	//Private data: NONE
	CID_ActivatedTemporalSplit,

	//Player started temporal split programming.
	//
	//Private data: NONE
	CID_TemporalSplitStart,

	//Player started temporal split programming.
	//
	//Private data: CMoveCoord *pSquare (one or more)
	CID_TemporalSplitEnd,

	//A seeding beacon was activated
	//
	//Private data: NONE
	CID_SeedingBeaconActivated,

	//A seeding beacon was deactivated
	//
	//Private data: NONE
	CID_SeedingBeaconDeactivated,

	//A gentryii pulls its chain taut
	//
	//Private data: NONE
	CID_GentryiiPulledTaut,

	//A Fluff puff evaporates
	//
	//Private data: CCoord *pSquare (one or more)
	CID_FluffPuffDestroyed,

	//Tarstuff is stabbed, pending a simultaneous stab at end of the turn
	//
	//Private data: CMoveCoord *pSquare (one or more)
	CID_TarstuffStabbed,

	//Firetrap burning this turn.
	//
	//Private data: CCoord *pSquare (one or more)
	CID_Firetrap,

	//Player switched places with a clone.
	//
	//Private data: NONE
	CID_CloneSwitch,

	//Firetrap has been activated.
	//
	//Private data: CCoord *pSquare (one or more)
	CID_FiretrapActivated,

	//Image overlay script command
	//
	//Private data: CImageOverlay* (one or more)
	CID_ImageOverlay,

	//A thin ice tile melts
	//
	//Private data: CCoord *pSquare (one or more)
	CID_ThinIceMelted,

	//A puff turned into a fluff tile
	//
	//Private data: CMoveCoord *pTile (one or more)
	CID_PuffMergedIntoFluff,

	//A piece of fluff was destroyed
	//
	//Private data: CMoveCoord *pSquareDestroyedAt (one or more)
	CID_FluffDestroyed,

	//Private data: NONE
	CID_PlayerFellIntoPit,

	//Private data: NONE
	CID_PlayerDrownedInWater,

	//The player bumped into a closed "hold complete" wall.
	//
	//Private data: NONE
	CID_BumpedHoldCompleteWall,

	//Tarstuff actually grew somewhere or tarstuff babies were spawned.
	//
	//Private data: UINT *tarType (one or more)
	CID_TarstuffGrew,

	//A roach queen spawned a roach egg.
	//
	//Private data: CMonster *pMonster (one or more)
	CID_EggSpawned,

	//Player eaten (killed) by oremites as construct.
	//
	//Private data: NONE
	CID_PlayerEatenByOremites,

	//A general event for a monster ending its existence in one way or another.
	// It is added for monster being killed, removed permanently from the room. Puff destroyer, golem turning into rubble,
	// snake being shortened to death, gentryii being fully dropped by build command and anything else that effectively
	// ends monster's existence
	//
	//Private data: CMonster *pMonster
	CID_MonsterExistenceCeased,

	//A change has been made to the room's lighting tiles.
	//
	//Private data: NONE
	CID_LightTilesChanged,

	//Second Command Key was used.
	//
	//Private data: NONE
	CID_CommandKeyTwoPressed,

	//Third Command Key was used.
	//
	//Private data: NONE
	CID_CommandKeyThreePressed,

	//Platform or raft couldn't be moved by player
	//
	//Private data: CCoord *pSquare
	CID_PlatformBlocked,

	//Script added a room to the explored rooms list
	//
	//Private data: NONE
	CID_AddedRoomToMap,

#ifdef TEST_SPIDER
	//Custom CueEvent to cause failure in test spider
	//Make sure this is always the last CueEvent defined
	//Fire this CueEvent in order to flag rooms that use/rely on a particular interaction
	CID_TestSpiderFailure,
#endif

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
extern const CUEEVENT_ID CIDA_PlayerLeftRoom[15];

//Did something kill the player?  This array should be checked for death instead
//of these separate events so that future causes of death will be checked without
//project-wide changes to code.
extern const CUEEVENT_ID CIDA_PlayerDied[11];

//Did a monster die?
extern const CUEEVENT_ID CIDA_MonsterDied[4];

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

	void SetMembers(const CCueEvents& Src);

	void     Add(const CUEEVENT_ID eCID, const CAttachableObject *pvPrivateData = NULL, const bool bIsAttached=false);
	void     Clear();
	void		ClearEvent(const CUEEVENT_ID eCID, const bool bDeleteAttached=true);
	const CAttachableObject* GetFirstPrivateData(const CUEEVENT_ID eCID);
	const CAttachableObject* GetNextPrivateData();
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

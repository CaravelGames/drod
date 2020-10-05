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
 * JP Burford (jpburford), John Wm. Wicks (j_wicks), Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//DrodSound.cpp
//Implementation of CDrodSound class.

#include "DrodSound.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/SettingsKeys.h"
#include <BackEndLib/Files.h>

const char moodText[SONG_MOOD_COUNT][8] = {
	"Ambient", "Attack", "Puzzle", "Exit", "Editor"
};

//Channels required/allocated for playing songs and sound effects.
#ifdef USE_SDL_MIXER
const UINT MODULE_CHANNEL_COUNT = 0;     //Music doesn't have a channel in SDL_mixer
const UINT SAMPLE_CHANNEL_COUNT = 15;
#else
const UINT MODULE_CHANNEL_COUNT = 2;	//two channels for cross-fading
const UINT SAMPLE_CHANNEL_COUNT = 14;
#endif

//Default volume settings.
const UINT DEFAULT_SOUND_VOLUME = 128, DEFAULT_MUSIC_VOLUME = 64, DEFAULT_VOICE_VOLUME = 255;

const float fDefaultMinDist = 8.0f, fDefaultMaxDist = 1000.0f;

//*****************************************************************************
CDrodSound::CDrodSound(const bool bNoSound)
	: CSound(bNoSound, SEID_COUNT, ::SAMPLE_CHANNEL_COUNT, ::MODULE_CHANNEL_COUNT)
{
   this->SoundEffectArray = new CDrodSoundEffect[SEID_COUNT];
   this->SongListArray = new list<WSTRING>[SONGID_COUNT];

	//Load sound effects ahead of time so that playing a sample takes less time.
	if (this->bSoundEffectsAvailable)
		this->bSoundEffectsAvailable = LoadSoundEffects();

	LoadMusicFileLists();

	this->nDistanceNorm = 2; //Euclidean
	this->fMinDist = fDefaultMinDist;
	this->fMaxDist = fDefaultMaxDist;
}

//*****************************************************************************
bool CDrodSound::GetSoundFilenames(
//Gets one or more filenames for a sound effect.
//
//Params:
	const UINT eSEID,          //(in)   One of the SEID_* constants.
	list<WSTRING> &FilepathList) //(out)  List of one or more file names.
//
//Returns:
//True if successful, false if not.
const
{
	FilepathList.clear();

	//Get INI key name that corresponds to wave ID.
	string strKeyName;
	switch (eSEID)
	{
		case SEID_CLEAR:  strKeyName="BeethroClear"; break;
		case SEID_DIE:    strKeyName="BeethroDie"; break;
		case SEID_HI:     strKeyName="BeethroHi"; break;
		case SEID_OOF:    strKeyName="BeethroOof"; break;
		case SEID_SCARED: strKeyName="BeethroScared"; break;

		case SEID_GB_CLEAR:  strKeyName="GunthroClear"; break;
		case SEID_GB_DIE:    strKeyName="GunthroDie"; break;
		case SEID_GB_HI:     strKeyName="GunthroHi"; break;
		case SEID_GB_OOF:    strKeyName="GunthroOof"; break;
		case SEID_GB_SCARED: strKeyName="GunthroScared"; break;
		case SEID_GB_TIRED:  strKeyName="GunthroTired"; break;

		case SEID_CIT_CLEAR:  strKeyName="CitizenClear"; break;
		case SEID_CIT_DIE:    strKeyName="CitizenDie"; break;
		case SEID_CIT_OOF:    strKeyName="CitizenOof"; break;
		case SEID_CIT_HI:     strKeyName="CitizenHi"; break;
		case SEID_CIT_SCARED: strKeyName="CitizenScared"; break;

		case SEID_GOB_CLEAR:  strKeyName="GoblinClear"; break;
		case SEID_GOB_DIE:    strKeyName="GoblinDie"; break;
		case SEID_GOB_OOF:    strKeyName="GoblinOof"; break;
		case SEID_GOB_HI:     strKeyName="GoblinHi"; break;
		case SEID_GOB_SCARED: strKeyName="GoblinScared"; break;

		case SEID_HALPH2_CLEAR:  strKeyName="Halph2Clear"; break;
		case SEID_HALPH2_DIE:    strKeyName="Halph2Die"; break;
		case SEID_HALPH2_OOF:    strKeyName="Halph2Oof"; break;
		case SEID_HALPH2_HI:     strKeyName="Halph2Hi"; break;
		case SEID_HALPH2_SCARED: strKeyName="Halph2Scared"; break;
		case SEID_HALPH2CANTOPEN:    strKeyName = "Halph2CantOpen"; break;
		case SEID_HALPH2ENTERED:     strKeyName = "Halph2Entered"; break;
		case SEID_HALPH2FOLLOWING:   strKeyName = "Halph2Following"; break;
		case SEID_HALPH2HURRYUP:     strKeyName = "Halph2HurryUp"; break;
		case SEID_HALPH2INTERRUPTED: strKeyName = "Halph2Interrupted"; break;
		case SEID_HALPH2STRIKING:    strKeyName = "Halph2Striking"; break;
		case SEID_HALPH2WAITING:     strKeyName = "Halph2Waiting"; break;

		case SEID_MON_CLEAR:  strKeyName="MonsterClear"; break;
		case SEID_MON_OOF:    strKeyName="MonsterOof"; break;

		case SEID_NEGO_CLEAR:   strKeyName="NegoClear"; break;
		case SEID_NEGO_DIE:     strKeyName="NegoDie"; break;
		case SEID_NEGO_OOF:     strKeyName="NegoOof"; break;
		case SEID_NEGO_HI:      strKeyName="NegoHi"; break;
		case SEID_NEGO_SCARED:  strKeyName="NegoScared"; break;

		case SEID_ROCK_CLEAR:  strKeyName="RockClear"; break;
		case SEID_ROCK_DIE:    strKeyName="RockDie"; break;
		case SEID_ROCK_OOF:    strKeyName="RockOof"; break;
		case SEID_ROCK_HI:     strKeyName="RockHi"; break;
		case SEID_ROCK_SCARED: strKeyName="RockScared"; break;

		case SEID_SLAYER_CLEAR:  strKeyName="SlayerClear"; break;
		case SEID_SLAYER_DIE:    strKeyName="SlayerDie"; break;
		case SEID_SLAYER_OOF:    strKeyName="SlayerOof"; break;
		case SEID_SLAYER_HI:     strKeyName="SlayerHi"; break;
		case SEID_SLAYER_SCARED: strKeyName="SlayerScared"; break;
		case SEID_SLAYER2COMBAT:    strKeyName="Slayer2Combat"; break;
		case SEID_SLAYER2ENTERNEAR: strKeyName="Slayer2EnterNear"; break;
		case SEID_SLAYER2ENTERFAR:  strKeyName="Slayer2EnterFar"; break;
		case SEID_SLAYER2KILL:      strKeyName="Slayer2Kill"; break;

		case SEID_SOLDIER_CLEAR:  strKeyName="SoldierClear"; break;
		case SEID_SOLDIER_DIE:    strKeyName="SoldierDie"; break;
		case SEID_SOLDIER_OOF:    strKeyName="SoldierOof"; break;
		case SEID_SOLDIER_HI:     strKeyName="SoldierHi"; break;
		case SEID_SOLDIER_SCARED: strKeyName="SoldierScared"; break;

		case SEID_STALWART_CLEAR:  strKeyName="StalwartClear"; break;
		case SEID_STALWART_DIE:    strKeyName="StalwartDie"; break;
		case SEID_STALWART_OOF:    strKeyName="StalwartOof"; break;
		case SEID_STALWART_HI:     strKeyName="StalwartHi"; break;
		case SEID_STALWART_SCARED: strKeyName="StalwartScared"; break;

		case SEID_TAR_OOF:     strKeyName="TarOof"; break;
		case SEID_TAR_SCARED:  strKeyName="TarScared"; break;

		case SEID_WOM_CLEAR:   strKeyName="WomanClear"; break;
		case SEID_WOM_DIE:     strKeyName="WomanDie"; break;
		case SEID_WOM_OOF:     strKeyName="WomanOof"; break;
		case SEID_WOM_HI:      strKeyName="WomanHi"; break;
		case SEID_WOM_SCARED:  strKeyName="WomanScared"; break;

		case SEID_CONSTRUCT_CLEAR:  strKeyName="ConstructClear"; break;
		case SEID_CONSTRUCT_DIE:    strKeyName="ConstructDie"; break;
		case SEID_CONSTRUCT_OOF:    strKeyName="ConstructOof"; break;
		case SEID_CONSTRUCT_HI:     strKeyName="ConstructHi"; break;
		case SEID_CONSTRUCT_SCARED: strKeyName="ConstructScared"; break;

		case SEID_ENGINEER_CLEAR:  strKeyName="EngineerClear"; break;
		case SEID_ENGINEER_DIE:    strKeyName="EngineerDie"; break;
		case SEID_ENGINEER_OOF:    strKeyName="EngineerOof"; break;
		case SEID_ENGINEER_HI:     strKeyName="EngineerHi"; break;
		case SEID_ENGINEER_SCARED: strKeyName="EngineerScared"; break;

		case SEID_BREAKWALL:   strKeyName="BreakWall"; break;
		case SEID_BRIAR_BREAK: strKeyName="BriarBreak"; break;
		case SEID_BUTTON:      strKeyName="Button"; break;
		case SEID_CHAIN_PULL:  strKeyName="ChainPull"; break;
		case SEID_CHECKPOINT:  strKeyName="Checkpoint"; break;
		case SEID_CONSTRUCT_REVIVE: strKeyName="ConstructRevive"; break;
		case SEID_CONSTRUCT_SMASH:  strKeyName="ConstructSmash"; break;
		case SEID_DOOROPEN:    strKeyName="DoorOpen"; break;
		case SEID_EVILEYEWOKE: strKeyName="EvilEyeWoke"; break;
		case SEID_FALLING:     strKeyName="Falling"; break;
		case SEID_FIRETRAP:    strKeyName="Firetrap"; break;
		case SEID_FIRETRAP_START: strKeyName="FiretrapStart"; break;
		case SEID_FROZEN:      strKeyName="Frozen"; break;
		case SEID_GOLEM_DEATH: strKeyName="GolemDeath"; break;
		case SEID_GUARD_DEATH: strKeyName="GuardDeath"; break;

		case SEID_HALPHCANTOPEN:    strKeyName = "HalphCantOpen"; break;
		case SEID_HALPHDIE:         strKeyName = "HalphDie"; break;
		case SEID_HALPHENTERED:     strKeyName = "HalphEntered"; break;
		case SEID_HALPHFOLLOWING:   strKeyName="HalphFollowing"; break;
		case SEID_HALPHHURRYUP:     strKeyName = "HalphHurryUp"; break;
		case SEID_HALPHINTERRUPTED: strKeyName = "HalphInterrupted"; break;
		case SEID_HALPHSTRIKING:    strKeyName = "HalphStriking"; break;
		case SEID_HALPHWAITING:     strKeyName="HalphWaiting"; break;

		case SEID_HORN_SOLDIER:  strKeyName="HornSoldier"; break;
		case SEID_HORN_SQUAD:    strKeyName="HornSquad"; break;
		case SEID_HORN_FAIL:     strKeyName="HornFail"; break;
		case SEID_ICEMELT:       strKeyName="IceMelt"; break;
		case SEID_LASTBRAIN:     strKeyName="LastBrain"; break;
		case SEID_LEVELCOMPLETE: strKeyName = "LevelComplete"; break;
		case SEID_MIMIC:         strKeyName="Mimic"; break;

		case SEID_NFRUSTRATED:  strKeyName="NFrustrated"; break;
		case SEID_NLAUGHING:    strKeyName="NLaughing"; break;
		case SEID_NSCARED:      strKeyName="NScared"; break;

		case SEID_ORBBROKE:     strKeyName="OrbBroke"; break;
		case SEID_ORBHIT:       strKeyName="OrbHit"; break;
		case SEID_ORBHITQUIET:  strKeyName="OrbHitQuiet"; break;
		case SEID_POTION:       strKeyName="Potion"; break;
		case SEID_PRESSPLATE:   strKeyName="PressPlate"; break;
		case SEID_PRESSPLATEUP: strKeyName="PressPlateUp"; break;
		case SEID_PUFF_EXPLOSION: strKeyName="PuffExplosion"; break;
		case SEID_READ:         strKeyName="Read"; break;
		case SEID_ROACH_EGG_SPAWNED: strKeyName="RoachEggSpawned"; break;
		case SEID_RUN:          strKeyName="Run"; break;
		case SEID_SCREENSHOT:   strKeyName="Screenshot"; break;
		case SEID_SECRET:       strKeyName="Secret"; break;
		case SEID_SEEDING_BEACON_OFF: strKeyName="SeedingBeaconOff"; break;
		case SEID_SEEDING_BEACON_ON:  strKeyName="SeedingBeaconOn"; break;
		case SEID_SEEP_DEATH:   strKeyName="SeepDeath"; break;
		case SEID_SHATTER:      strKeyName="Shatter"; break;
		case SEID_SIZZLE:       strKeyName="Sizzle"; break;

		case SEID_SLAYERCOMBAT:    strKeyName="SlayerCombat"; break;
		case SEID_SLAYERENTERNEAR: strKeyName="SlayerEnterNear"; break;
		case SEID_SLAYERENTERFAR:  strKeyName="SlayerEnterFar"; break;
		case SEID_SLAYERKILL:      strKeyName="SlayerKill"; break;

		case SEID_SNORING:      strKeyName="Snoring"; break;
		case SEID_SOFT_SWING:   strKeyName="SoftSwing"; break;
		case SEID_SPIKES_POISED:strKeyName="SpikesPoised"; break;
		case SEID_SPIKES_UP:    strKeyName="SpikesUp"; break;
		case SEID_SPLASH:       strKeyName="Splash"; break;
		case SEID_SPLAT:        strKeyName="Splat"; break;
		case SEID_STABTAR:      strKeyName="StabTar"; break;
		case SEID_STAFF:        strKeyName="Staff"; break;
		case SEID_SWING:        strKeyName="Swing"; break;
		case SEID_SWING_LOW:    strKeyName="SwingLow"; break;
		case SEID_TAR_GROWTH:   strKeyName="TarGrowth"; break;
		case SEID_TARBABY_DEATH: strKeyName="TarBabyDeath"; break;
		case SEID_TARMOTHER_DEATH: strKeyName="TarMotherDeath"; break;
		case SEID_TEMPORAL_SPLIT_START:  strKeyName="TemporalSplitStart"; break;
		case SEID_TEMPORAL_SPLIT_REWIND: strKeyName="TemporalSplitRewind"; break;
		case SEID_THUNDER:      strKeyName="Thunder"; break;
		case SEID_TIRED:        strKeyName="Tired"; break;
		case SEID_TRAPDOOR:     strKeyName="Trapdoor"; break;
		case SEID_TUNNEL:       strKeyName="Tunnel"; break;
		case SEID_UNDO:         strKeyName="Undo"; break;
		case SEID_WADE:         strKeyName="Wade"; break;
		case SEID_WALK:         strKeyName="Walk"; break;
		case SEID_WALK_MONSTER: strKeyName="WalkMonster"; break;
		case SEID_STARTFUSE:    strKeyName="Fuse"; break;
		case SEID_BOMBEXPLODE:  strKeyName="Bomb"; break;
		case SEID_SWORDS:       strKeyName="Sword"; break;
		case SEID_WIFF:         strKeyName="Wiff"; break;
		case SEID_WIFF_LOW:     strKeyName="WiffLow"; break;
		case SEID_WISP:         strKeyName="Wisp"; break;
		case SEID_WORLDMAP_CLICK: strKeyName="WorldmapClick"; break;
		case SEID_WUBBA:        strKeyName="Wubba"; break;

		default: return false;
	}

	//Retrieve sound filename(s) from INI file.
	CFiles Files;
	return Files.GetGameProfileString("Waves", strKeyName.c_str(), FilepathList);
}

//***********************************************************************************
bool CDrodSound::GetSongFilepaths(
//Gets list of song file names.
//
//Params:
	const UINT eSongID,  //(in)   One of the SONGID_* constants.
	list<WSTRING> &FilepathList) //(out)  List of full paths to one or more music files.
//
//Returns:
//True if successful, false if not.
{
	FilepathList.clear();

	ASSERT(eSongID < SONGID_COUNT);

	//Get INI key name that corresponds to song ID.
	string strKeyName;
	switch (eSongID)
	{
		case SONGID_INTRO_GATEB:   strKeyName = "Intro_GATEB"; break;
		case SONGID_INTRO_JTRH:    strKeyName = "Intro_JTRH"; break;
		case SONGID_INTRO_KDD:     strKeyName = "Intro_KDD"; break;
		case SONGID_INTRO_TCB:     strKeyName = "Intro_TCB"; break;
		case SONGID_INTRO_TSS:     strKeyName = "Intro_TSS"; break;
		case SONGID_CREDITS_GATEB: strKeyName = "Credits_GATEB"; break;
		case SONGID_CREDITS_JTRH:  strKeyName = "Credits_JTRH"; break;
		case SONGID_CREDITS_KDD:   strKeyName = "Credits_KDD"; break;
		case SONGID_CREDITS_TCB:   strKeyName = "Credits_TCB"; break;
		case SONGID_CREDITS_TSS:   strKeyName = "Credits_TSS"; break;
		case SONGID_ENDOFTHEGAME_JTRH: strKeyName = "Finale_JTRH"; break;
		case SONGID_ENDOFTHEGAME_TCB:  strKeyName = "Finale_TCB"; break;
		case SONGID_WINGAME_GATEB: strKeyName = "WinGame_GATEB"; break;
		case SONGID_WINGAME_JTRH:  strKeyName = "WinGame_JTRH"; break;
		case SONGID_WINGAME_KDD:   strKeyName = "WinGame_KDD"; break;
		case SONGID_WINGAME_TCB:   strKeyName = "WinGame_TCB"; break;
		case SONGID_WINGAME_TSS:   strKeyName = "WinGame_TSS"; break;
		case SONGID_QUIT_GATEB:    strKeyName = "Exit_GATEB"; break;
		case SONGID_QUIT_JTRH:     strKeyName = "Exit_JTRH"; break;
		case SONGID_QUIT_KDD:      strKeyName = "Exit_KDD"; break;
		case SONGID_QUIT_TCB:      strKeyName = "Exit_TCB"; break;
		case SONGID_QUIT_TSS:      strKeyName = "Exit_TSS"; break;
		default: return false;
	}

	//Retrieve song filename(s) from INI file.
	CFiles Files;
	return Files.GetGameProfileString(INISection::Songs, strKeyName.c_str(), FilepathList);
}

//**********************************************************************************
void CDrodSound::LoadSoundEffect(
//Loads sound bites for the specified effect.
//
//Params:
	const UINT seID, const bool bRandom,
	const bool bLoadOnPlay,      //if true (default), don't load sound files until first time the sound effect is played
	const int channelAssignment) //if SEID_NONE (default), sound may play on any non-reserved channel
{
	list<WSTRING> FilepathArray;
	GetSoundFilenames(seID, FilepathArray);
	VERIFY(this->SoundEffectArray[seID].Load(FilepathArray,
		channelAssignment, this->b3DSound, bRandom, bLoadOnPlay));
}

//**********************************************************************************
bool CDrodSound::LoadSoundEffects()
//Load sound effects into an array.
//
//Returns:
//True if at least one sound effect loaded, false if not.
{
#ifdef WITHOUT_SOUND
	return false;
#else
	ASSERT(this->SoundEffectArray);

	bool bOneSoundEffectLoaded = false;
	CSound::PRIVATE_SOUND_CHANNEL_COUNT = 0;

	//Sounds that cannot play at the same time are designated to specific channels.

	//For readability--macro that loads one sound effect.
	//Note that sound effect channels come after module channels.
#  define PRIVATE_CHANNEL_SOUNDEFFECT(s, bRandom) \
		LoadSoundEffect((s), bRandom, true, MODULE_CHANNEL_COUNT + PRIVATE_SOUND_CHANNEL_COUNT)

	//Channel n+1--Beethro/Gunthro/player's voice.
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SCARED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SNORING, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_TIRED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GB_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GB_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GB_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GB_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GB_SCARED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GB_TIRED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CIT_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CIT_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CIT_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CIT_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CIT_SCARED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GOB_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GOB_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GOB_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GOB_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GOB_SCARED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH2_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH2_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH2_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH2_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH2_SCARED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_MON_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_MON_OOF, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_NEGO_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_NEGO_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_NEGO_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_NEGO_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_NEGO_SCARED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ROCK_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ROCK_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ROCK_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ROCK_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ROCK_SCARED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYER_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYER_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYER_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYER_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYER_SCARED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SOLDIER_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SOLDIER_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SOLDIER_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SOLDIER_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SOLDIER_SCARED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_STALWART_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_STALWART_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_STALWART_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_STALWART_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_STALWART_SCARED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_TAR_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_TAR_SCARED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_WOM_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_WOM_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_WOM_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_WOM_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_WOM_SCARED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CONSTRUCT_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CONSTRUCT_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CONSTRUCT_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CONSTRUCT_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CONSTRUCT_SCARED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ENGINEER_CLEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ENGINEER_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ENGINEER_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ENGINEER_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ENGINEER_SCARED, true );

	++PRIVATE_SOUND_CHANNEL_COUNT;

	//Channel n+2--'Neather's/Slayer's voice.
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_NLAUGHING, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_NFRUSTRATED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_NSCARED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYERCOMBAT, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYERENTERNEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYERENTERFAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYERKILL, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYER2COMBAT, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYER2ENTERNEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYER2ENTERFAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYER2KILL, true );
	++PRIVATE_SOUND_CHANNEL_COUNT;

    //Channel n+3--Halph's voice.
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHCANTOPEN, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHDIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHENTERED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHFOLLOWING, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHHURRYUP, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHINTERRUPTED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHSTRIKING, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHWAITING, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH2CANTOPEN, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH2ENTERED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH2FOLLOWING, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH2HURRYUP, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH2INTERRUPTED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH2STRIKING, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH2WAITING, true );
	++PRIVATE_SOUND_CHANNEL_COUNT;

#  undef PRIVATE_CHANNEL_SOUNDEFFECT

	//The following sounds can play at the same time.
	//They will be dynamically set to any available public channel when played.

#  define SHARED_CHANNEL_SOUNDEFFECT(s) \
	LoadSoundEffect((s), true, true, SEID_NONE)

	SHARED_CHANNEL_SOUNDEFFECT( SEID_EVILEYEWOKE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SPLAT );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_BUTTON );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_ORBHIT );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_ORBHITQUIET );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_MIMIC );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_WALK );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_WALK_MONSTER );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_POTION );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SWING );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SWING_LOW );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_READ );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_ROACH_EGG_SPAWNED );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SECRET );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_TRAPDOOR );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_CHECKPOINT );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_BREAKWALL );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_DOOROPEN );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_LASTBRAIN );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_STABTAR );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_LEVELCOMPLETE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_STARTFUSE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_BOMBEXPLODE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SWORDS );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_WISP );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_WUBBA );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_TUNNEL );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_PRESSPLATE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_PRESSPLATEUP );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_ORBBROKE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_FALLING );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SHATTER );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SIZZLE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SPLASH );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_FROZEN );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_HORN_SQUAD );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_HORN_SOLDIER );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_HORN_FAIL );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_THUNDER );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_WADE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SCREENSHOT );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SPIKES_POISED );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SPIKES_UP );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_WIFF );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_WIFF_LOW );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_TEMPORAL_SPLIT_START );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_TEMPORAL_SPLIT_REWIND );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SEEDING_BEACON_OFF );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SEEDING_BEACON_ON );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_CHAIN_PULL );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_CONSTRUCT_REVIVE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_CONSTRUCT_SMASH );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_BRIAR_BREAK );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_FIRETRAP );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_FIRETRAP_START );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_ICEMELT );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_WORLDMAP_CLICK );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_PUFF_EXPLOSION );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_STAFF );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_TAR_GROWTH );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_GOLEM_DEATH );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_GUARD_DEATH );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SEEP_DEATH );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_TARBABY_DEATH );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_TARMOTHER_DEATH );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SOFT_SWING );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_RUN );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_UNDO );

#  undef SHARED_CHANNEL_SOUNDEFFECT

	//If this assertion fires, too many private sound channels were designated.
	ASSERT(MODULE_CHANNEL_COUNT + PRIVATE_SOUND_CHANNEL_COUNT < CHANNEL_COUNT);

	//Check sound effects to see what got loaded.
	for (int nSEI = 0; nSEI < SEID_COUNT; ++nSEI)
	{
		if (this->SoundEffectArray[nSEI].IsLoaded())
			bOneSoundEffectLoaded = true;
		else
		{
			//If this fires, check 1. a load macro is present for each
			//SEID_* value. 2. filenames specified in DROD.INI are correct.
			CFiles f;
			char text[128];
			sprintf(text, "Sound effect #%i failed to load.  Check whether "
				"the filenames specified in DROD.INI are correct." NEWLINE, nSEI);
			f.AppendErrorLog(text);
			ASSERT(!"Sound effect failed to load.");
		}
	}

	//Set special sound effect parameters.
	SetMinMaxDistance(SEID_ALL, fDefaultMinDist, fDefaultMaxDist); //all sounds have a gradual volume fall-off

	return bOneSoundEffectLoaded;
#endif //WITHOUT_SOUND
}

//**********************************************************************************
bool CDrodSound::LoadMusicFileLists()
//Load song play lists into an array.
{
#ifdef WITHOUT_SOUND
	return false;
#else

	ASSERT(this->SongListArray);
	for (int eSongID = 0; eSongID < SONGID_COUNT; ++eSongID)
	{
		if (SONGID_2_0_IGNORE_BEGIN <= eSongID && eSongID <= SONGID_2_0_IGNORE_END)
			continue;

		//Get file names for songs in this song list.
		if (!GetSongFilepaths(eSongID, this->SongListArray[eSongID]))
		{
			CFiles f;
			f.AppendErrorLog("A song sequence wasn't listed. Check whether ");
			const string filepath = UnicodeToAscii(
					this->SongListArray[eSongID].empty() ? wszQuestionMark :
					this->SongListArray[eSongID].front());
			f.AppendErrorLog(filepath.c_str());
			f.AppendErrorLog(" is a valid filename." NEWLINE);
			ASSERT(!"Song sequence wasn't listed.");
			return false;
		}

	}

	return true;
#endif
}

//**********************************************************************************
SOUNDSTREAM* CDrodSound::LoadSong(
//Attempts to load a sampled song from the db into a new FSOUND_SAMPLE.
//
//Returns:
//Loaded song file, or NULL on failure.
//
//Params:
	const WCHAR* pwszWaveFilepath, //(in) Path+name of wave file to load
	const UINT mode,   //(in) song mode flags used by sound engine (optional)
	BYTE* &pRawData)        //(out) optional pointer to data buffer needing deallocation on song completion
//const //SDL_mixer version stores data
{
#ifdef WITHOUT_SOUND
	return NULL;
#else
	ASSERT(pwszWaveFilepath);
	ASSERT(WCSlen(pwszWaveFilepath) > 1);

	SOUNDSTREAM *pStream = NULL;

	//Search for sound with this name in the DB.
	if (!CDb::IsOpen()) return NULL;	//DB not available

	CDb db;
	CIDSet musicFormats(DATA_WAV);
	musicFormats += DATA_OGG;
	db.Data.FilterByFormat(musicFormats);
	const UINT dwDataID = db.Data.FindByName(pwszWaveFilepath);
	if (dwDataID)
	{
		CDbDatum *pData = g_pTheDB->Data.GetByID(dwDataID, true);
		pStream = LoadStream(pData, mode, pRawData);
		delete pData;
	}
	return pStream;
#endif //WITHOUT_SOUND
}

//**********************************************************************************
SOUNDSTREAM* CDrodSound::LoadStream(
//Loads a stream from given data object.
//
//Params:
	CDbDatum *pData,
	const UINT mode,   //(in) song mode flags used by sound engine (optional)
	BYTE* &pRawData)        //(out) optional pointer to data buffer needing deallocation on song completion
{
#ifdef WITHOUT_SOUND
	return NULL;
#else
	SOUNDSTREAM *pStream = NULL;

	ASSERT(pData);
	ASSERT(!pRawData);      //shouldn't be set yet

	switch (pData->wDataFormat)
	{
		case DATA_WAV:
		case DATA_OGG:
		{
			const UINT dwSize = g_pTheDB->Data.GetRawDataForID(pData->dwDataID, pRawData);
			if (dwSize)
			{
				ASSERT(pRawData);
#ifdef USE_SDL_MIXER
				if (this->pSongStream)
					Mix_FreeMusic(this->pSongStream);
				this->pSongStream = pStream = Mix_LoadMUS_RW(SDL_RWFromConstMem((BYTE*)pRawData, dwSize), 1);
#else
				const UINT wStreamMode = mode | FSOUND_LOADMEMORY;
				pStream = FSOUND_Stream_Open((char*)pRawData, wStreamMode, 0, dwSize);
#endif
				if (!pStream)
				{
					string str;
					GetLastSoundError(str);
					CFiles f;
					f.AppendErrorLog(str.c_str());
					ASSERT(!"CDrodSound::LoadSong(): Failed to open stream.");
				}
			}
		}
		break;
		default:
			ASSERT(!"Unsupported music format"); break;
	}
	return pStream;
#endif //WITHOUT_SOUND
}

//**********************************************************************************
SOUNDSAMPLE* CDrodSoundEffect::LoadWave(
//Loads a sound sample from the appropriate location into a new SOUNDSAMPLE.
//
//Returns:
//Loaded sound object.
//
//Params:
	const WCHAR *pwszFile,   //(in) Name of sound file to load
	const bool b3DSound)     //(in) Load as 3D sound (2D if false)
const
{
#ifdef WITHOUT_SOUND
	return NULL;
#else
	ASSERT(pwszFile);
	ASSERT(WCSlen(pwszFile) > 1);

	//Search for sound on disk.
	SOUNDSAMPLE *pSample = CSoundEffect::LoadWave(pwszFile, b3DSound);
	if (pSample)
		return pSample;

	//Search for sound with this name in the DB.
	if (!CDb::IsOpen())
		return NULL;	//DB not available

	CDb db;
	CIDSet soundFormats(DATA_WAV);
	soundFormats += DATA_OGG;
	db.Data.FilterByFormat(soundFormats);
	const UINT dwDataID = db.Data.FindByName(pwszFile);
	if (!dwDataID) return NULL;

	CDbDatum *pData = g_pTheDB->Data.GetByID(dwDataID);
	ASSERT(pData);
	switch (pData->wDataFormat)
	{
		case DATA_WAV:
		case DATA_OGG:
			pSample = LoadSample(pData->data, b3DSound);
			break;
		default:
			ASSERT(!"Unrecognized sound format"); break;
	}

	delete pData;
	return pSample;
#endif //WITHOUT_SOUND
}

//**********************************************************************************
bool CDrodSound::PlayData(const UINT dwDataID)
//Plays media object data as streamed music.
//
//Returns: whether data is valid and music began playing
{
	//Return successful without doing anything if music has been disabled.
	if (!this->bMusicOn || !this->bMusicAvailable)
		return true;

	CDbDatum *pDatum = g_pTheDB->Data.GetByID(dwDataID, true);
	if (!pDatum)
		return false;

	list<WSTRING> newSongList;
	newSongList.push_back(pDatum->DataNameText);

	if (IsSongListPlaying(&newSongList))
	{
		delete pDatum;
		return true; //this song title is already playing
	}

	//Stop any music playing.
	if (!StopSong())
	{
		delete pDatum;
		return false;
	}

	//Load media data into stream.
	UINT mode = 0;
#ifndef USE_SDL_MIXER //(handled below in StartSongStream for SDL_mixer)
	mode = FSOUND_NORMAL | FSOUND_2D | FSOUND_IGNORETAGS | FSOUND_LOOP_NORMAL;
#endif
	BYTE *pRawData = NULL;
	SOUNDSTREAM *pStream = LoadStream(pDatum, mode, pRawData);
	delete pDatum;
	if (!pStream)
		return false;

	//Set play parameters.
	this->SongList = newSongList;
	this->dwFadeDuration = this->dwFadeBegin = 0;
	this->nFadingSongChannel = SOUNDLIB::SONG_NONE;

	//Play stream.
	return StartSongStream(pStream, pRawData);
}

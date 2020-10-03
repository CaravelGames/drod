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

//DrodSound.h
//Declarations for CDrodSound.
//Class for playing waves and music for DROD.

#ifndef DRODSOUND_H
#define DRODSOUND_H

#include <FrontEndLib/Sound.h>

//Song IDs.
enum SONGID
{
	SONGID_DEFAULT = -3, //indicates to use the default music style for this area
	SONGID_CUSTOM = -2,  //accompanied by dataID to play embedded music

	//Must retain IDs where set here for scripting backwards compatibility
	SONGID_NONE = SOUNDLIB::SONG_NONE,
	SONGID_INTRO_TCB = 0,
	SONGID_WINGAME_TCB = 1,
	SONGID_CREDITS_TSS = 2,
	SONGID_ENDOFTHEGAME_JTRH = 3,
	SONGID_ENDOFTHEGAME_TCB = 4,

	//5-16 are now unused: these were used in 2.0 for hardcoded room style values,
	//and get upgraded when these numbers are loaded in CCharacter::Upgrade2_0CommandTo3_0()
	SONGID_2_0_IGNORE_BEGIN = 5,
	SONGID_2_0_IGNORE_END = 16,

	SONGID_CREDITS_GATEB = 17,
	SONGID_CREDITS_JTRH,
	SONGID_CREDITS_KDD,
	SONGID_CREDITS_TCB,
	SONGID_INTRO_GATEB,
	SONGID_INTRO_JTRH,
	SONGID_INTRO_KDD,
	SONGID_INTRO_TSS,
	SONGID_QUIT_GATEB,
	SONGID_QUIT_JTRH,
	SONGID_QUIT_KDD,
	SONGID_QUIT_TCB,
	SONGID_QUIT_TSS,
	SONGID_WINGAME_GATEB,
	SONGID_WINGAME_JTRH,
	SONGID_WINGAME_KDD,
	SONGID_WINGAME_TSS,

	SONGID_COUNT
};

enum SONGMOOD {
	SONG_AMBIENT = 0,
	SONG_ATTACK = 1,
	SONG_PUZZLE = 2,
	SONG_EXIT = 3,
	SONG_EDITOR = 4,
	SONG_MOOD_COUNT
};

extern const char moodText[SONG_MOOD_COUNT][8];

//Sound effect IDs.  The sequence and values of the constants does not affect
//anything.  I have them arranged by the channel they will play on.  The
//actual channel assignment occurs in LoadSoundEffects(), so if you
//rearrange or add to the SEIDs, LoadSoundEffects() should also be updated.
//
//If a sample is played on a channel and another sample is already playing on that
//channel, the currently playing sample will stop.  Samples are grouped together
//when:
//1. There is a thing in the game that couldn't make two sounds at once, i.e.
//   a person speaking, so it is natural to stop a current sample on the same
//   channel.
//2. There is no case where the two samples would play at the same time.
//
//Performance is better when less channels are used, but when in doubt, give a
//sample its own channel.
enum SEID
{
	//Special parameters to indicate certain groups of sound effects.
	SEID_ALL = SOUNDLIB::SEID_ALL,
	SEID_NONE = SOUNDLIB::SEID_NONE,

	//These sound effects can play at the same time on any public channel.
	SEID_BUTTON = SOUNDLIB::SEID_BUTTON,
	SEID_READ = SOUNDLIB::SEID_READ,
	SEID_EVILEYEWOKE,
	SEID_SPLAT,
	SEID_ORBHIT,
	SEID_ORBHITQUIET,
	SEID_MIMIC,
	SEID_WALK,
	SEID_POTION,
	SEID_SWING,
	SEID_SECRET,
	SEID_TRAPDOOR,
	SEID_CHECKPOINT,
	SEID_BREAKWALL,
	SEID_DOOROPEN,
	SEID_LASTBRAIN,
	SEID_STABTAR,
	SEID_LEVELCOMPLETE,
	SEID_STARTFUSE,
	SEID_BOMBEXPLODE,
	SEID_SWORDS,
	SEID_WISP,
	SEID_WUBBA,
	SEID_TUNNEL,
	SEID_PRESSPLATE,
	SEID_PRESSPLATEUP,
	SEID_ORBBROKE,
	SEID_FROZEN,
	SEID_FALLING,
	SEID_SHATTER,
	SEID_SIZZLE,
	SEID_SPLASH,
	SEID_WALK_MONSTER,
	SEID_HORN_SOLDIER,
	SEID_HORN_SQUAD,
	SEID_HORN_FAIL,
	SEID_THUNDER,
	SEID_WADE,
	SEID_SCREENSHOT,
	SEID_SPIKES_POISED,
	SEID_SPIKES_UP,
	SEID_WIFF,
	SEID_TEMPORAL_SPLIT_START,
	SEID_TEMPORAL_SPLIT_REWIND,
	SEID_SEEDING_BEACON_OFF,
	SEID_SEEDING_BEACON_ON,
	SEID_CHAIN_PULL,
	SEID_CONSTRUCT_REVIVE,
	SEID_CONSTRUCT_SMASH,
	SEID_BRIAR_BREAK,
	SEID_FIRETRAP,
	SEID_FIRETRAP_START,
	SEID_ICEMELT,
	SEID_WIFF_LOW,
	SEID_SWING_LOW,
	SEID_WORLDMAP_CLICK,
	SEID_PUFF_EXPLOSION,
	SEID_STAFF,
	SEID_TAR_GROWTH,
	SEID_ROACH_EGG_SPAWNED,
	SEID_GOLEM_DEATH,
	SEID_GUARD_DEATH,
	SEID_SEEP_DEATH,
	SEID_TARBABY_DEATH,
	SEID_TARMOTHER_DEATH,
	SEID_SOFT_SWING,
	SEID_RUN,
	SEID_UNDO,

	//Grouped sound effects -- may only play one at a time -- designated private channels.
	//Channel n+1--Beethro/Gunthro voice.
	SEID_OOF,
	SEID_SCARED,
	SEID_DIE,
	SEID_CLEAR,
	SEID_TIRED,
	SEID_SNORING,
	SEID_HI,

	SEID_GB_CLEAR,
	SEID_GB_DIE,
	SEID_GB_HI,
	SEID_GB_OOF,
	SEID_GB_SCARED,
	SEID_GB_TIRED,

	SEID_CIT_CLEAR,  //citizen (male)
	SEID_CIT_DIE,
	SEID_CIT_OOF,
	SEID_CIT_HI,
	SEID_CIT_SCARED,

	SEID_GOB_CLEAR,  //goblin
	SEID_GOB_DIE,
	SEID_GOB_OOF,
	SEID_GOB_HI,
	SEID_GOB_SCARED,

	SEID_MON_CLEAR,  //monster (generic)
	SEID_MON_OOF,

	SEID_NEGO_CLEAR,  //Negotiator
	SEID_NEGO_DIE,
	SEID_NEGO_OOF,
	SEID_NEGO_HI,
	SEID_NEGO_SCARED,

	SEID_ROCK_CLEAR, //rock golem
	SEID_ROCK_DIE,
	SEID_ROCK_OOF,
	SEID_ROCK_HI,
	SEID_ROCK_SCARED,

	SEID_SOLDIER_CLEAR, //soldier
	SEID_SOLDIER_DIE,
	SEID_SOLDIER_OOF,
	SEID_SOLDIER_HI,
	SEID_SOLDIER_SCARED,

	SEID_STALWART_CLEAR, //stalwart
	SEID_STALWART_DIE,
	SEID_STALWART_OOF,
	SEID_STALWART_HI,
	SEID_STALWART_SCARED,

	SEID_TAR_OOF,    //tarstuff
	SEID_TAR_SCARED,

	SEID_WOM_CLEAR,  //woman
	SEID_WOM_DIE,
	SEID_WOM_OOF,
	SEID_WOM_HI,
	SEID_WOM_SCARED,

	SEID_CONSTRUCT_CLEAR,  //construct
	SEID_CONSTRUCT_DIE,
	SEID_CONSTRUCT_OOF,
	SEID_CONSTRUCT_HI,
	SEID_CONSTRUCT_SCARED,

	SEID_ENGINEER_CLEAR,  //engineer
	SEID_ENGINEER_DIE,
	SEID_ENGINEER_OOF,
	SEID_ENGINEER_HI,
	SEID_ENGINEER_SCARED,

	//Channel n+2--'Neather's/Slayer's voice.
	SEID_NLAUGHING,
	SEID_NFRUSTRATED,
	SEID_NSCARED,
	SEID_SLAYERCOMBAT,
	SEID_SLAYERENTERNEAR,
	SEID_SLAYERENTERFAR,
	SEID_SLAYERKILL,
	SEID_SLAYER2COMBAT,
	SEID_SLAYER2ENTERNEAR,
	SEID_SLAYER2ENTERFAR,
	SEID_SLAYER2KILL,

	SEID_SLAYER_CLEAR,
	SEID_SLAYER_DIE,
	SEID_SLAYER_OOF,
	SEID_SLAYER_HI,
	SEID_SLAYER_SCARED,

	//Channel n+3--Halph's voice.
	SEID_HALPHFOLLOWING,
	SEID_HALPHWAITING,
	SEID_HALPHSTRIKING,
	SEID_HALPHCANTOPEN,
	SEID_HALPHDIE,
	SEID_HALPHENTERED,
	SEID_HALPHINTERRUPTED,
	SEID_HALPHHURRYUP,

	SEID_HALPH2CANTOPEN, //Halph (older)
	SEID_HALPH2ENTERED,
	SEID_HALPH2FOLLOWING,
	SEID_HALPH2HURRYUP,
	SEID_HALPH2INTERRUPTED,
	SEID_HALPH2STRIKING,
	SEID_HALPH2WAITING,
	SEID_HALPH2_CLEAR, //these sfx for original Halph aren't supported
	SEID_HALPH2_DIE,
	SEID_HALPH2_OOF,
	SEID_HALPH2_HI,
	SEID_HALPH2_SCARED,

	SEID_COUNT
};

//Number of channels required for playing sound effects.
extern const UINT SAMPLE_CHANNEL_COUNT;
//Channels required for playing songs.
extern const UINT MODULE_CHANNEL_COUNT;

//Default volume settings.
extern const UINT DEFAULT_SOUND_VOLUME, DEFAULT_MUSIC_VOLUME, DEFAULT_VOICE_VOLUME;

class CDbDatum;
class CDrodSound : public CSound
{
public:
	CDrodSound(const bool bNoSound);
	virtual ~CDrodSound() {}

	virtual bool GetSongFilepaths(const UINT nSongID, list<WSTRING> &FilepathList);
	virtual bool GetSoundFilenames(const UINT eSEID, list<WSTRING> &FilepathList) const;
	virtual bool PlayData(const UINT dwDataID);

protected:
	void LoadSoundEffect(const UINT seID, const bool bRandom,
			const bool bLoadOnPlay=true, const int channelAssignment=SEID_NONE);
	virtual bool LoadSoundEffects();
	bool LoadMusicFileLists();

	virtual SOUNDSTREAM* LoadSong(const WCHAR *pwszWaveFilepath,
			const UINT mode, BYTE* &pRawData);
	SOUNDSTREAM* LoadStream(CDbDatum *pData, const UINT mode, BYTE* &pRawData);
};

//Class for loading and playing samples for a sound effect.
class CDrodSoundEffect : public CSoundEffect
{
private:
#ifndef WITHOUT_SOUND
	virtual SOUNDSAMPLE* LoadWave(const WCHAR *pwszFile, const bool b3DSound) const;
#endif
};

#endif //...#ifndef DRODSOUND_H

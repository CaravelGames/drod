// $Id: DrodSound.h 9014 2008-05-10 19:06:54Z mrimer $

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
 * 1997, 2000, 2001, 2002, 2005, 2007 Caravel Software. All Rights Reserved.
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
	SONGID_NONE = SOUNDLIB::SONG_NONE,
	SONGID_INTRO = 0,
	SONGID_WINGAME = 1,
	SONGID_CREDITS = 2,
	SONGID_QUIT = 3,
	SONGID_ARCHITECTS = 4,
	SONGID_BATTLE = 5,
	SONGID_BENEATH = 6,
	SONGID_DREAMER = 7,
	SONGID_GOBLINS = 8,
	SONGID_REDGUARD = 9,
	SONGID_SEASIDE = 10,
	SONGID_SERPENTS = 11,
	SONGID_SLAYER = 12,
	SONGID_PIRATES = 13,
	SONGID_GOBLINKING = 14,
	SONGID_BIGSERPENT = 15,
	SONGID_TAR = 16,
	SONGID_PUZZLE = 17,
	SONGID_SECRETAREA = 18,
	SONGID_SMALLERPLANS = 19,
	SONGID_GEOMETRY = 20,
	SONGID_SLIPSTAIR = 21,
	SONGID_SYMPATHETIC = 22,
	SONGID_ASCENDANT = 23,
	SONGID_ROACHESRUN = 24,
	SONGID_NEWIDEA = 25,
	SONGID_WALTZ = 26,
	SONGID_SWINGHALLS = 27,
	SONGID_AMBWINDY = 28,
	SONGID_AMBBEACH = 29,
	SONGID_AMBFOREST = 30,
	SONGID_AMBDRIPS = 31,
	SONGID_AMBROACHES = 32,
	SONGID_TITLE_2 = 33,

	SONGID_COUNT
};

enum SONGMOOD {
/*
	SONG_AMBIENT = 0,
	SONG_ATTACK = 1,
	SONG_PUZZLE = 2,
	SONG_EXIT = 3,
*/
	SONG_PUZZLE = 0,
	SONG_EXIT = 1,
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
	SEID_TUNNEL,
	SEID_PRESSPLATE,
	SEID_PRESSPLATEUP,
	SEID_ORBBROKE,
	SEID_FROZEN,
	SEID_FALLING,
	SEID_SHATTER,
	SEID_SIZZLE,
	SEID_SPLASH,
	SEID_JUMP,
	SEID_WATERSTEP,
	SEID_KEY,
	SEID_MONSTERATTACK,
	SEID_HIT,
	SEID_PUNCH,
	SEID_SHIELDED,
	SEID_AREACLEAR,
	SEID_THUNDER,
	SEID_BRIAR_BREAK,
	SEID_AUTOSAVE,
	SEID_ATK_PICKUP,
	SEID_DEF_PICKUP,
	SEID_HP_PICKUP,
	SEID_SHOVEL_PICKUP,
	SEID_DIG,
	SEID_ICEMELT,
	SEID_PUFF_EXPLOSION,
	SEID_FIRETRAP,
	SEID_FIRETRAP_START,
	SEID_CONSTRUCT_SMASH,
	SEID_ROACH_EGG_SPAWNED,

	//Grouped sound effects -- may only play one at a time -- designated private channels.
	//Channel n+1--Tendry's voice.
	SEID_OOF,
	SEID_SCARED,
	SEID_DIE,
//	SEID_CLEAR,
	SEID_TIRED,
	SEID_SNORING,
//	SEID_HI,

	SEID_CIT_DIE,    //citizen (male)
	SEID_CIT_OOF,
	SEID_CIT_SCARED,

	SEID_CONSTRUCT_DIE,    // construct
	SEID_CONSTRUCT_SCARED,
	SEID_CONSTRUCT_OOF,

	SEID_ENGINEER_DIE,   // engineer
	SEID_ENGINEER_SCARED,
	SEID_ENGINEER_OOF,

	SEID_GOB_DIE,    //goblin
	SEID_GOB_OOF,
	SEID_GOB_SCARED,

	SEID_HALPH_DIE, //Halph
	SEID_HALPH_OOF,
	SEID_HALPH_SCARED,

	SEID_MON_OOF,    //monster (generic)

//	SEID_NEATHER_SCARED, //'Neather
//	SEID_NEATHER_OOF,

	SEID_ROCK_DIE,   //rock golem
	SEID_ROCK_OOF,
	SEID_ROCK_SCARED,

	SEID_SLAYERDIE,  //slayer
	SEID_SLAYER_OOF,
	SEID_SLAYER_SCARED,

	SEID_STALWART_DIE, //stalwart
	SEID_STALWART_OOF,
	SEID_STALWART_SCARED,

	SEID_TAR_OOF,    //tarstuff
	SEID_TAR_SCARED,

	SEID_WOM_DIE,    //citizen (female)
	SEID_WOM_OOF,
	SEID_WOM_SCARED,

	SEID_WUBBA,

	//Channel n+2--Slayer's voice.
	SEID_SLAYERKILL,

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

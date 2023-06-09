// $Id: DrodSound.cpp 9094 2008-07-09 05:43:41Z mrimer $

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
//	"Ambient", "Attack",
	"Puzzle", "Exit"
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
const UINT DEFAULT_SOUND_VOLUME = 128, DEFAULT_MUSIC_VOLUME = 85, DEFAULT_VOICE_VOLUME = 255;

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
	this->fMinDist = fDefaultMinDist / 2;
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
//		case SEID_CLEAR: strKeyName="BeethroClear"; break;
		case SEID_DIE: strKeyName="BeethroDie"; break;
//		case SEID_HI: strKeyName="BeethroHi"; break;
		case SEID_OOF: strKeyName="BeethroOof"; break;
		case SEID_SCARED: strKeyName="BeethroScared"; break;

		case SEID_CIT_DIE:   strKeyName="CitizenDie"; break;
		case SEID_CIT_OOF:   strKeyName="CitizenOof"; break;
		case SEID_CIT_SCARED:   strKeyName="CitizenScared"; break;
		case SEID_GOB_DIE:    strKeyName="GoblinDie"; break;
		case SEID_GOB_OOF:   strKeyName="GoblinOof"; break;
		case SEID_GOB_SCARED:   strKeyName="GoblinScared"; break;
		case SEID_HALPH_DIE:    strKeyName="HalphDie"; break;
		case SEID_HALPH_OOF:    strKeyName="HalphOof"; break;
		case SEID_HALPH_SCARED: strKeyName="HalphScared"; break;
		case SEID_MON_OOF: strKeyName="MonsterOof"; break;
//		case SEID_NEATHER_OOF:  strKeyName="NFrustrated"; break;
//		case SEID_NEATHER_SCARED: strKeyName="NScared"; break;
		case SEID_ROCK_DIE:   strKeyName="RockDie"; break;
		case SEID_ROCK_SCARED:   strKeyName="RockScared"; break;
		case SEID_SLAYERDIE:  strKeyName="SlayerDie"; break;
		case SEID_SLAYER_OOF:    strKeyName="SlayerOof"; break;
		case SEID_SLAYER_SCARED: strKeyName="SlayerScared"; break;
		case SEID_STALWART_DIE:    strKeyName="StalwartDie"; break;
		case SEID_STALWART_OOF:    strKeyName="StalwartOof"; break;
		case SEID_STALWART_SCARED: strKeyName="StalwartScared"; break;
		case SEID_TAR_OOF: strKeyName="TarOof"; break;
		case SEID_TAR_SCARED: strKeyName="TarScared"; break;
		case SEID_WOM_DIE:   strKeyName="WomanDie"; break;
		case SEID_WOM_OOF:   strKeyName="WomanOof"; break;
		case SEID_WOM_SCARED:   strKeyName="WomanScared"; break;

		case SEID_AREACLEAR: strKeyName="AreaClear"; break;
		case SEID_AUTOSAVE: strKeyName="Autosave"; break;
		case SEID_BREAKWALL: strKeyName="BreakWall"; break;
		case SEID_BRIAR_BREAK: strKeyName="BriarBreak"; break;
		case SEID_BUTTON:    strKeyName="Button"; break;
		case SEID_CHECKPOINT:   strKeyName="Checkpoint"; break;
		case SEID_DOOROPEN:     strKeyName="DoorOpen"; break;
		case SEID_EVILEYEWOKE:  strKeyName="EvilEyeWoke"; break;
		case SEID_FALLING:  strKeyName="Falling"; break;
		case SEID_FROZEN:  strKeyName="Frozen"; break;

		case SEID_SLAYERKILL:      strKeyName="SlayerKill"; break;

		case SEID_HIT:  strKeyName="Hit"; break;
		case SEID_JUMP:  strKeyName="Jump"; break;
		case SEID_KEY:  strKeyName="Key"; break;
		case SEID_LASTBRAIN: strKeyName="LastBrain"; break;
		case SEID_LEVELCOMPLETE:   strKeyName = "LevelComplete"; break;
		case SEID_MIMIC:     strKeyName="Mimic"; break;
		case SEID_MONSTERATTACK:  strKeyName="MonsterAttack"; break;

		case SEID_ATK_PICKUP: strKeyName = "ATKPickup"; break;
		case SEID_DEF_PICKUP: strKeyName = "DEFPickup"; break;
		case SEID_HP_PICKUP: strKeyName = "HPPickup"; break;
		case SEID_SHOVEL_PICKUP: strKeyName = "ShovelPickup"; break;
		case SEID_DIG: strKeyName = "Dig"; break;

		case SEID_ORBBROKE:  strKeyName="OrbBroke"; break;
		case SEID_ORBHIT:    strKeyName="OrbHit"; break;
		case SEID_ORBHITQUIET:     strKeyName="OrbHitQuiet"; break;
		case SEID_POTION:    strKeyName="Potion"; break;
		case SEID_PRESSPLATE:       strKeyName="PressPlate"; break;
		case SEID_PRESSPLATEUP:       strKeyName="PressPlateUp"; break;
		case SEID_PUNCH:        strKeyName="Punch"; break;
		case SEID_READ:         strKeyName="Read"; break;
		case SEID_SECRET:    strKeyName="Secret"; break;
		case SEID_SHATTER:     strKeyName="Shatter"; break;
		case SEID_SHIELDED:     strKeyName="Shielded"; break;
		case SEID_SIZZLE:     strKeyName="Sizzle"; break;
		case SEID_SNORING:      strKeyName="Snoring"; break;
		case SEID_SPLASH:     strKeyName="Splash"; break;
		case SEID_SPLAT:     strKeyName="Splat"; break;
		case SEID_STABTAR:      strKeyName="StabTar"; break;
		case SEID_SWING:     strKeyName="Swing"; break;
		case SEID_THUNDER:   strKeyName="Thunder"; break;
		case SEID_TIRED:     strKeyName="Tired"; break;
		case SEID_TRAPDOOR:     strKeyName="Trapdoor"; break;
		case SEID_TUNNEL:       strKeyName="Tunnel"; break;
		case SEID_WALK:         strKeyName="Walk"; break;
		case SEID_WATERSTEP:  strKeyName="WaterStep"; break;
		case SEID_STARTFUSE:    strKeyName="Fuse"; break;
		case SEID_BOMBEXPLODE:  strKeyName="Bomb"; break;
		case SEID_SWORDS:       strKeyName="Sword"; break;
		case SEID_WISP:         strKeyName="Wisp"; break;
		case SEID_WUBBA:			strKeyName="Wubba"; break;
		default: return false;
	}

	//Retrieve sound filename(s) from INI file.
	CFiles Files;
	return Files.GetGameProfileString(INISection::Waves, strKeyName.c_str(), FilepathList);
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
		case SONGID_INTRO:         strKeyName = "Intro"; break;
		case SONGID_WINGAME:			strKeyName = "WinGame"; break;
		case SONGID_CREDITS:			strKeyName = "Credits"; break;
		case SONGID_QUIT:          strKeyName = "Exit"; break;
		case SONGID_ARCHITECTS:    strKeyName = "Architects"; break;
		case SONGID_BATTLE:        strKeyName = "Battle"; break;
		case SONGID_BENEATH:       strKeyName = "Beneath"; break;
		case SONGID_DREAMER:       strKeyName = "Dreamer"; break;
		case SONGID_GOBLINS:       strKeyName = "Goblins"; break;
		case SONGID_REDGUARD:      strKeyName = "Redguard"; break;
		case SONGID_SEASIDE:       strKeyName = "Seaside"; break;
		case SONGID_SERPENTS:      strKeyName = "Serpents"; break;
		case SONGID_SLAYER:        strKeyName = "Slayer"; break;
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

	//Channel n+1--Beethro's/player's voice.
//	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CLEAR, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_DIE, true );
//	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HI, false );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SCARED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SNORING, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_TIRED, true );

	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CIT_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CIT_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_CIT_SCARED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GOB_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GOB_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_GOB_SCARED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPH_SCARED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_MON_OOF, true );
//	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_NEATHER_OOF, true );
//	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_NEATHER_SCARED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ROCK_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ROCK_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_ROCK_SCARED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYERDIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYER_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYER_SCARED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_STALWART_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_STALWART_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_STALWART_SCARED, true );         
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_TAR_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_TAR_SCARED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_WOM_DIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_WOM_OOF, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_WOM_SCARED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_WUBBA, true );

   PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYERKILL, false );

	++PRIVATE_SOUND_CHANNEL_COUNT;

	//Channel n+2--Slayer's voice.
/*
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_NLAUGHING, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYERENTERNEAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYERENTERFAR, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_SLAYERCOMBAT, true );
	++PRIVATE_SOUND_CHANNEL_COUNT;
*/

/*
   //Channel n+3--Halph's voice.
   PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHCANTOPEN, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHDIE, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHENTERED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHFOLLOWING, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHHURRYUP, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHINTERRUPTED, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHSTRIKING, true );
	PRIVATE_CHANNEL_SOUNDEFFECT( SEID_HALPHWAITING, true );
	++PRIVATE_SOUND_CHANNEL_COUNT;
*/

#  undef PRIVATE_CHANNEL_SOUNDEFFECT

	//The following sounds can play at the same time.
	//They will be dynamically set to any available public channel when played.

#  define SHARED_CHANNEL_SOUNDEFFECT(s) \
	LoadSoundEffect((s), true, true, SEID_NONE)

	SHARED_CHANNEL_SOUNDEFFECT( SEID_ATK_PICKUP );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_DEF_PICKUP );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_HP_PICKUP );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SHOVEL_PICKUP );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_DIG );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_AREACLEAR );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_AUTOSAVE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_BOMBEXPLODE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_BREAKWALL );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_BRIAR_BREAK );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_BUTTON );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_CHECKPOINT );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_DOOROPEN );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_EVILEYEWOKE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_FALLING );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_FROZEN );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_HIT );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_JUMP );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_KEY );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_LASTBRAIN );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_LEVELCOMPLETE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_MIMIC );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_MONSTERATTACK );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_ORBBROKE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_ORBHIT );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_ORBHITQUIET );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_POTION );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_PRESSPLATE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_PRESSPLATEUP );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_PUNCH );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_READ );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SECRET );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SHATTER );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SHIELDED );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SIZZLE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SPLASH );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SPLAT );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_STABTAR );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_STARTFUSE );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SWING );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_SWORDS );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_THUNDER );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_TRAPDOOR );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_TUNNEL );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_WALK );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_WATERSTEP );
	SHARED_CHANNEL_SOUNDEFFECT( SEID_WISP );
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
		//Get file names for songs in this song list.
		if (!GetSongFilepaths(eSongID, this->SongListArray[eSongID]))
		{
			CFiles f;
			f.AppendErrorLog("A song sequence wasn't listed. Check whether ");
			const string filepath = UnicodeToUTF8(this->SongListArray[eSongID].empty() ?
					wszQuestionMark : this->SongListArray[eSongID].front());
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

	//If a data object with this name is already playing as a song,
	//assume it's the same music and don't change it.
	list<WSTRING> newSongList;
	WSTRING dataName = g_pTheDB->Data.GetNameFor(dwDataID);
	newSongList.push_back(dataName);
	if (IsSongListPlaying(&newSongList))
		return true; //this song title is already playing

	//Stop any music playing.
	if (!StopSong())
		return false;

	//Load media data into stream.
	UINT mode = 0;
#ifndef USE_SDL_MIXER //(handled below in StartSongStream for SDL_mixer)
	mode = FSOUND_NORMAL | FSOUND_2D | FSOUND_IGNORETAGS | FSOUND_LOOP_NORMAL;
#endif
	BYTE *pRawData = NULL;
	CDbDatum *pDatum = g_pTheDB->Data.GetByID(dwDataID, true);
	if (!pDatum)
		return false;
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

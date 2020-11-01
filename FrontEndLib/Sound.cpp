// $Id: Sound.cpp 10235 2012-06-05 13:17:04Z mrimer $

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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//Sound.cpp
//Implementation of CSound class.

#ifdef WIN32
#pragma warning(disable:4786)
#include <windows.h>  //WideCharToMultiByte
#endif

#define INCLUDED_FROM_SOUND_CPP
#include "Sound.h"
#undef INCLUDED_FROM_SOUND_CPP

#include <BackEndLib/Files.h>
#include <BackEndLib/Ports.h>

#include <SDL.h>
#include <math.h>
#include <algorithm>

#define ABS(x)  ( ((x) < 0) ? -(x) : (x) )
#define MAX(a,b)  ( ((a) > (b)) ? (a) : (b) )

//Global instance of the one-and-only sound object.
CSound *g_pTheSound = NULL;

UINT CSound::SOUND_EFFECT_COUNT = 0;   //# of loaded sound effects
UINT CSound::PRIVATE_SOUND_CHANNEL_COUNT = 0;   //# of designated private sound channels

UINT CSound::SAMPLE_CHANNEL_COUNT = 0; //channels available for sound effects
UINT CSound::SONG_CHANNEL_COUNT = 0; //channels available for music
UINT CSound::CHANNEL_COUNT = SONG_CHANNEL_COUNT + SAMPLE_CHANNEL_COUNT;

bool CSound::bNoFocusPlaysMusic = false;

#ifdef USE_SDL_MIXER
volatile CSound::SongInfo_t CSound::SongInfo = { false };
#else
volatile CSound::SongInfo_t CSound::SongInfo = { NULL, 0, false };
#endif

const float fDefaultMinDist = 1.0f, fDefaultMaxDist = 1000000000.0f;

static const WCHAR wcszMusic[] = { We(SLASH),We('M'),We('u'),We('s'),We('i'),We('c'),We(SLASH),We(0) };
static const WCHAR wcszSounds[] = { We(SLASH),We('S'),We('o'),We('u'),We('n'),We('d'),We('s'),We(SLASH),We(0) };

//
//CSoundEffect public methods.
//

//***********************************************************************************
CSoundEffect::CSoundEffect()
{
#ifndef WITHOUT_SOUND
	this->nChannel = -1;
	this->bPlayRandomSample = this->bIsLoaded = false;
	this->iLastSamplePlayed = this->Samples.end();
	this->bLoadOnPlay = false;
#ifdef USE_SDL_MIXER
	// Set distance to FMOD defaults
	this->fMinDist = fDefaultMinDist;
	this->fMaxDist = fDefaultMaxDist;
#endif
#endif
}

//***********************************************************************************
CSoundEffect::~CSoundEffect()
//Destructor.
{
	if (this->bIsLoaded) Unload();
}

//**********************************************************************************
WSTRING CSoundEffect::GetPath()
//Gets path where sounds are found.
{
	CFiles Files;
	WSTRING wstrPath = Files.GetResPath();
	return wstrPath + wcszSounds;
}

//**********************************************************************************
WSTRING CSoundEffect::GetModPath()
//Gets user path where mod sounds can be found.
{
	CFiles Files;
	WSTRING wstrPath = Files.GetDatPath();
	return wstrPath + wcszSounds;
}

//***********************************************************************************
bool CSoundEffect::IsLoaded() const
//Returns: whether sound effect is loaded.
//If bLoadOnPlay is set, this assumes sound files can be loaded when Play is invoked.
//If that fails, however, then IsLoaded will return false thereafter.
{
	return this->bIsLoaded || this->bLoadOnPlay;
}

//***********************************************************************************
bool CSoundEffect::Load(
//Load a sound effect so that it is ready to play.
//
//Params:
	list<WSTRING> FilepathArray,     //(in)   One or more full paths to sound
	     // files that will be loaded as samples to play for this sound effect.
	const int nSetChannel,           //(in)   Channel on which samples will play.
	const bool b3DSound,             // (in)  if true, use 3D support, or 2D if false
	const bool bSetPlayRandomSample, //(in)   If true, samples will play randomly.
	     //If false, (default) samples will play in sequence.
	     //Does not affect a sound effect with only one loaded sample.
	const bool bLoadOnPlay)          //if true (default), don't load sound files
	     //until the first time the sound effect is played
//
//Returns:
//True if all files in argument list were loaded, false if not.
//If no samples load, the Play() method will do nothing when called.
{
#ifdef WITHOUT_SOUND
	return true;
#else
	ASSERT(!this->bIsLoaded);
	if (nSetChannel != SOUNDLIB::SEID_NONE)
	{
		ASSERT(static_cast<UINT>(nSetChannel) >= CSound::SONG_CHANNEL_COUNT);
		ASSERT(static_cast<UINT>(nSetChannel) < CSound::CHANNEL_COUNT);
	}

	this->nChannel = nSetChannel;
	this->b3DSound = b3DSound;
	this->bPlayRandomSample = bSetPlayRandomSample;

	if (!bLoadOnPlay)
		return LoadFiles(FilepathArray);

	this->bLoadOnPlay = bLoadOnPlay;
	this->FilesToLoad = FilepathArray;
	return true; //everything went well so far -- we'll attempt file load later
#endif
}

//***********************************************************************************
bool CSoundEffect::LoadFiles(list<WSTRING>& Filenames)
//Load specified sound files.
//
//Returns: whether all files loaded properly
{
	ASSERT(!this->bIsLoaded);

	//Load each sample from a sound file.
	bool bCompleteSuccess = true;
	for (list<WSTRING>::const_iterator iFilepath = Filenames.begin();
		iFilepath != Filenames.end(); ++iFilepath)
	{
		SOUNDSAMPLE *pSample = LoadWave(iFilepath->c_str(), this->b3DSound);
		if (pSample)
		{
#ifndef USE_SDL_MIXER
			VERIFY(FSOUND_Sample_SetMode(pSample, FSOUND_LOOP_OFF));
#endif
			this->Samples.push_back(pSample);
		} else {
			bCompleteSuccess = false;

			//Error logging.
			CFiles f;
			BYTE *u8str = NULL;
			to_utf8(iFilepath->c_str(), u8str);
			char text[1024];
			_snprintf(text, 1024, "Sound file \"%s\" failed to load.  Check whether "
				"the filenames are correct." NEWLINE, u8str);
			delete[] u8str;
			f.AppendErrorLog(text);
		}
	}

	//If at least one sample loaded, then I will call this sound effect "loaded".
	//It can be used.
	this->bIsLoaded = (this->Samples.size() > 0);

	//Once we've tried to loaded files, we're not waiting on anything more to play.
	this->bLoadOnPlay = false;

	SetMinMaxDistance(g_pTheSound->fMinDist, g_pTheSound->fMaxDist);

	return bCompleteSuccess;
}

//***********************************************************************************
bool CSoundEffect::Play(const int nUseChannel, const float frequencyMultiplier) //[default=1.0]
//Plays a sample for the sound effect on the specified channel.
{
#ifndef WITHOUT_MUSIC
	//Do nothing if sound effect is not loaded.
	if (this->bLoadOnPlay) //if load was delayed until first play...
		LoadFiles(this->FilesToLoad); //...then this is the time to load the files.
	if (!this->bIsLoaded)
		return false;

	//Stop any other samples playing on this channel.
	ASSERT(nUseChannel >= 0);
#ifdef USE_SDL_MIXER
	Mix_HaltChannel(nUseChannel);  //Always returns 0 =P
	ASSERTP(!Mix_Playing(nUseChannel), "Sound wasn't really stopped.");
#else
	if (!FSOUND_StopSound(nUseChannel))
	{
		string str;
		CSound::GetLastSoundError(str);
		CFiles f;
		f.AppendErrorLog(str.c_str());
		ASSERT(!"Failed to stop sound.");
	}
	ASSERTP(!FSOUND_IsPlaying(nUseChannel), "Sound wasn't really stopped.");
#endif

	//Figure out which sample to play next.
	ASSERT(this->Samples.size());
	list<SOUNDSAMPLE *>::iterator iPlaySample;
	if (this->Samples.size() == 1)
		iPlaySample = this->iLastSamplePlayed = this->Samples.begin();
	else if (this->bPlayRandomSample)
	{
		const UINT randSample = RAND(this->Samples.size());
		iPlaySample = this->Samples.begin();
		for (UINT sampleCount=0; sampleCount < randSample; ++sampleCount)
		{
			ASSERT(iPlaySample != this->Samples.end());
			++iPlaySample;
		}
		this->iLastSamplePlayed = iPlaySample;
	}
	else
	{
		if (this->iLastSamplePlayed == this->Samples.end())
			this->iLastSamplePlayed = this->Samples.begin();
		else
		{
			if (++(this->iLastSamplePlayed) == this->Samples.end())
				this->iLastSamplePlayed = this->Samples.begin();
		}
		iPlaySample = this->iLastSamplePlayed;
	}

	//Play the sample.
#ifdef USE_SDL_MIXER
	const int result = Mix_PlayChannel(nUseChannel, *iPlaySample, 0);
	if (result == -1)
	{
		string str;
		CSound::GetLastSoundError(str);
		CFiles f;
		f.AppendErrorLog(str.c_str());
		ASSERT(!"Failed to play sound.");
		return false;
	} 
	ASSERT(nUseChannel == result);
#else
	const int result = FSOUND_PlaySound(nUseChannel, *iPlaySample);
	if (result == -1)
	{
		string str;
		CSound::GetLastSoundError(str);
		CFiles f;
		f.AppendErrorLog(str.c_str());
		//If an error other than no free channels arose, assert it.
		ASSERTP(FSOUND_GetError() == FMOD_ERR_CHANNEL_ALLOC, "Failed to play sound.");
		return false;
	}
	ASSERT(nUseChannel == result % 4096);
	if (frequencyMultiplier != 1.0)
		FSOUND_SetFrequency(nUseChannel, int(FSOUND_GetFrequency(nUseChannel) * frequencyMultiplier));
#endif

#endif
	return true;
}

//********************************************************************************
void CSoundEffect::GetMinMaxDistance(float& fMin, float& fMax) const
//Gets the minimum and maximum audible distance for a sample.
{
#ifdef USE_SDL_MIXER
	fMin = this->fMinDist;
	fMax = this->fMaxDist;
#else
	ASSERT(this->Samples.size());
	FSOUND_Sample_GetMinMaxDistance(this->Samples.front(), &fMin, &fMax);
#endif
}

//********************************************************************************
bool CSoundEffect::SetMinMaxDistance(const float fMin, const float fMax)
//Sets the minimum and maximum audible distance for a sample.
{
#ifdef USE_SDL_MIXER
	this->fMinDist = fMin;
	this->fMaxDist = fMax;
	return true;
#else
	bool bRes = true;
	for (list<SOUNDSAMPLE *>::const_iterator iSeek = this->Samples.begin();
			iSeek != this->Samples.end(); ++iSeek)
		bRes &= (FSOUND_Sample_SetMinMaxDistance(*iSeek, fMin, fMax) != 0);
	return bRes;
#endif
}

//***********************************************************************************
void CSoundEffect::Unload()
{
#ifndef WITHOUT_SOUND
	ASSERT(this->bIsLoaded || this->bLoadOnPlay);

	//Free the samples.
	for (list<SOUNDSAMPLE *>::iterator iSeek = this->Samples.begin();
			iSeek != this->Samples.end(); ++iSeek)
	{
#ifdef USE_SDL_MIXER
		Mix_FreeChunk(*iSeek);
#else
		FSOUND_Sample_Free(*iSeek);
#endif
	}
	this->Samples.clear();

	this->nChannel = -1;
	this->iLastSamplePlayed = this->Samples.end();
	this->b3DSound = false;
	this->bPlayRandomSample = false;
	this->bIsLoaded = false;
	this->bLoadOnPlay = false;
	this->FilesToLoad.clear();
#endif
}

//
//CSoundEffect private methods.
//

#ifndef WITHOUT_SOUND
//**********************************************************************************
SOUNDSAMPLE* CSoundEffect::LoadWave(
//Load sound file into a sound object.
//
//Returns:
//Loaded sound object.
//
//Params:
	const WCHAR *pwszFile,   //(in) Name of sound file to load
	const bool b3DSound)     //(in) Load as 3D sound (2D if false)
const
{
	//Check for a sound in the user mod directory.
	WSTRING wstrFilepath = GetModPath();
	wstrFilepath += pwszFile;

	//Load file from disk.
	CStretchyBuffer buffer;
	CFiles::ReadFileIntoBuffer(wstrFilepath.c_str(), buffer, true);

	if (buffer.empty()) {
		//Check for a sound in the standard directory.
		wstrFilepath = GetPath();
		wstrFilepath += pwszFile;
		CFiles::ReadFileIntoBuffer(wstrFilepath.c_str(), buffer, true);
	}

	return LoadSample(buffer, b3DSound);
}

//**********************************************************************************
SOUNDSAMPLE* CSoundEffect::LoadSample(
	CStretchyBuffer& buffer,         //(in) raw sound data
	const bool b3DSound)             //(in) Load as 3D sound (2D if false)
const
{
	if (buffer.empty())
		return NULL; //no data

#ifdef USE_SDL_MIXER
	//Happily ignore the 3D flag
	SDL_RWops *pOp = SDL_RWFromConstMem((BYTE*)buffer, buffer.Size());
	ASSERT(pOp);
	SOUNDSAMPLE *pSample = Mix_LoadWAV_RW(pOp, 1);
#else //FMOD
	//Attempt to load as hardware-supported 3D sound, if requested.
	unsigned int mode = FSOUND_LOADMEMORY | (b3DSound ? FSOUND_HW3D : FSOUND_2D);
	SOUNDSAMPLE *pSample = FSOUND_Sample_Load(FSOUND_FREE, (char*)(BYTE*)buffer,
			mode, 0, buffer.Size());
	if (pSample)
		return pSample;

	//If sound didn't load successfully as requested, try loading as 2D in software.
	//(This seems happen for .ogg format sound files.)
	mode = FSOUND_LOADMEMORY | FSOUND_2D;
	pSample = FSOUND_Sample_Load(FSOUND_FREE, (char*)(BYTE*)buffer, mode, 0, buffer.Size());
#endif

	if (!pSample)
	{
		string str;
		CSound::GetLastSoundError(str);
		CFiles f;
		f.AppendErrorLog(str.c_str());
		ASSERT(!"CSoundEffect::LoadSample(): Failed to load sample.");
	}
	return pSample;
}

#endif

//
//CSound Public methods.
//

//**********************************************************************************
CSound::CSound(
//Constructor.  If sound initialization fails, this object will be disabled for
//the rest of its life, and calls to methods will generally not do anything.
//
//Params:
	const bool bNoSound,
	const UINT SOUND_EFFECT_COUNT,
	const UINT SAMPLE_CHANNEL_COUNT,
	const UINT SONG_CHANNEL_COUNT)
	: nDistanceNorm(0)
	, fMinDist(fDefaultMinDist), fMaxDist(fDefaultMaxDist)
	, bNoSound(bNoSound)
	, bMusicOn(!bNoSound), bMusicAvailable(false)
	, bSoundEffectsOn(!bNoSound), bSoundEffectsAvailable(false)
	, bVoicesOn(!bNoSound)
	, b3DSound(false)
	, nCurrentPlayingSongChannel(SOUNDLIB::SONG_NONE)
	, nFadingSongChannel(SOUNDLIB::SONG_NONE)
	, dwFadeBegin(0L), dwFadeDuration(0L)
	, nSoundVolume(128), nMusicVolume(128), nVoiceVolume(128)
#ifdef USE_SDL_MIXER
	, allocedMusic(NULL), pSongStream(NULL)
#endif
	, SoundEffectArray(NULL)
	, SongListArray(NULL), SongList()
	, ChannelSoundEffects(NULL)
	, bFadeRequest(false)
{
#ifndef WITHOUT_SOUND
	if (bNoSound) return;
#ifndef USE_SDL_MIXER
	ASSERT(SONG_CHANNEL_COUNT >= 2);  //need two channels for music cross-fading
#endif
	CSound::SOUND_EFFECT_COUNT = SOUND_EFFECT_COUNT;
	CSound::SAMPLE_CHANNEL_COUNT = SAMPLE_CHANNEL_COUNT;
	CSound::SONG_CHANNEL_COUNT = SONG_CHANNEL_COUNT;
	CSound::CHANNEL_COUNT = SAMPLE_CHANNEL_COUNT + SONG_CHANNEL_COUNT;

	this->f2DListenerPos[0] = this->f2DListenerPos[1] = 0.0;			//origin
	this->f2DListenerDir[0] = 0.0;  this->f2DListenerDir[1] = 1.0;	//north

	//Allocate this->SoundEffectArray and this->SongListArray in derived class.

	this->ChannelSoundEffects = new UINT[CHANNEL_COUNT];
	for (UINT nChannel=CHANNEL_COUNT; nChannel--; )
		this->ChannelSoundEffects[nChannel] = static_cast<UINT>(SOUNDLIB::SEID_NONE);
#ifndef USE_SDL_MIXER
	this->Streams.resize(SONG_CHANNEL_COUNT);
#endif

	//The call to InitSound() will set music and sound effects availability.
	InitSound();
#endif
}

//**********************************************************************************
CSound::~CSound()
//Destructor.
{
 	if (!bNoSound)
 	{
		FreeSoundDump();
		UnloadSoundEffects();
		DeinitSound();

		delete[] this->ChannelSoundEffects;
		delete[] this->SoundEffectArray;
		delete[] this->SongListArray;
	}
}

//********************************************************************************
void CSound::Enable3DSound(const bool bSet3DSound)
//Enables/disables 3D sound effects.
{
#ifndef USE_SDL_MIXER  //(always disabled with SDL_mixer)
	if (bSet3DSound == this->b3DSound) return;
	this->b3DSound = bSet3DSound;

	//Reload sound effects.
	if (this->bSoundEffectsAvailable)
	{
		UnloadSoundEffects();
		LoadSoundEffects();
	}
#endif
}

//********************************************************************************
bool CSound::EnableMusic(const bool bSetMusicOn)
//Turns music playback on or off.
{
	if (bSetMusicOn == this->bMusicOn) return this->bMusicOn;

	this->bMusicOn = !bNoSound && bSetMusicOn;
	if (!bSetMusicOn)
		StopSong();

	return this->bMusicOn;
}

//********************************************************************************
bool CSound::EnableSoundEffects(const bool bSetSoundEffectsOn)
//Turns sound effect playback on or off.
{
	return this->bSoundEffectsOn = !bNoSound && bSetSoundEffectsOn;
}

//********************************************************************************
bool CSound::EnableVoices(const bool bSet)
//Turns voice playback on or off.
{
	return this->bVoicesOn = !bNoSound && bSet;
}

//*****************************************************************************
void CSound::FreeSoundDump()
//Deallocates all floating dynamically allocated sound samples.
{
	for (UINT i=this->soundDump.size(); i--; )
	{
#ifdef USE_SDL_MIXER
		Mix_FreeChunk(this->soundDump[i]);
#else
		FSOUND_Sample_Free(this->soundDump[i]);
#endif
	}
	this->soundDump.clear();
}

//********************************************************************************
UINT CSound::GetMemoryUsage()
//Returns: amount of memory currently allocated by sound system
{
#if defined(WITHOUT_SOUND) || defined(USE_SDL_MIXER)  //!!! FIXME
	return 0;
#else
	UINT current, max;
	FSOUND_GetMemoryStats(&current,&max);
	return current;
#endif
}

//********************************************************************************
void CSound::GetMinMaxDistance(const UINT eSEID, float& fMin, float& fMax) const
//Gets the minimum and maximum audible distance for a sound effect.
{
	this->SoundEffectArray[eSEID].GetMinMaxDistance(fMin, fMax);
}

//********************************************************************************
bool CSound::IsSongListPlaying(list<WSTRING>* pSonglist) const
//Returns: whether inputted song list matches the song list currently playing
{
	ASSERT(pSonglist);
	if (this->SongList.size() != pSonglist->size()) return false; //different sizes

	//Compare titles in song lists of same size for matches.
	for (list<WSTRING>::const_iterator song = this->SongList.begin();
			song != this->SongList.end(); ++song)
	{
		if (std::find(pSonglist->begin(), pSonglist->end(), *song) == pSonglist->end())
			return false; //this song isn't in the other list
	}

	return true; //identical
}

//********************************************************************************
void CSound::PauseMusic(const bool val)
{
#ifdef USE_SDL_MIXER
	if (val)
		Mix_PauseMusic();
	else
		Mix_ResumeMusic();
#else
	if (this->nCurrentPlayingSongChannel >= 0)
		FSOUND_SetPaused(this->nCurrentPlayingSongChannel, val);
#endif
}

void CSound::UnpauseMusic() { PauseMusic(false); }

//********************************************************************************
void CSound::PauseSounds(const bool val)
{
#ifdef USE_SDL_MIXER
	if (val)
		Mix_Pause(-1);
	else
		Mix_Resume(-1);
#else
	for (UINT nChannelNo = SONG_CHANNEL_COUNT; nChannelNo < CHANNEL_COUNT; ++nChannelNo)
		FSOUND_SetPaused(nChannelNo, val);
#endif
}

void CSound::UnpauseSounds() { PauseSounds(false); }

//********************************************************************************
void CSound::Mute(const bool bMute)	//[default=true]
//Mutes all sound by turning all volumes to zero, or unmutes by restoring sound volumes.
//While muted, music is paused.
{
#ifndef WITHOUT_SOUND
	//For preserving old volume levels while sound is muted.
	static int musicVolume = -1, soundVolume = -1, voiceVolume = -1;

	if (bMute)
	{
		if (musicVolume == -1 && soundVolume == -1 && voiceVolume == -1)
		{
			//Preserve values for restoration later.
			musicVolume = g_pTheSound->GetMusicVolume();
			soundVolume = g_pTheSound->GetSoundVolume();
			voiceVolume = g_pTheSound->GetVoiceVolume();

			PauseMusic();
			PauseSounds();
		}
		g_pTheSound->SetMusicVolume(0);
		g_pTheSound->SetSoundEffectsVolume(0);
		g_pTheSound->SetVoicesVolume(0);
	} else {
		if (musicVolume != -1 && soundVolume != -1 && voiceVolume != -1)
		{
			//Restore sound/music.
			g_pTheSound->SetMusicVolume(musicVolume);
			g_pTheSound->SetSoundEffectsVolume(soundVolume);
			g_pTheSound->SetVoicesVolume(voiceVolume);

			UnpauseMusic();
			UnpauseSounds();

			musicVolume = soundVolume = voiceVolume = -1;
		}
	}
#endif
}

//********************************************************************************
void CSound::SetSoundEffectsVolume(
//Sets volume of sound effects.
	const int volume)  //(in) value between 0 (silent) and 255 (full)
{
	ASSERT(volume >= 0 && volume <= 255);
	this->nSoundVolume = volume;
}

//********************************************************************************
void CSound::SetVoicesVolume(
//Sets volume of voices.
	const int volume)  //(in) value between 0 (silent) and 255 (full)
{
	ASSERT(volume >= 0 && volume <= 255);
	this->nVoiceVolume = volume;
}

//********************************************************************************
bool CSound::SetMinMaxDistance(const UINT eSEID, const float fMin, const float fMax) const
//Sets the minimum and maximum audible distance for a sound effect.
//Use eSEID == SEID_ALL to set this value for all sound effects.
{
	ASSERT(this->SoundEffectArray);
	if (eSEID != (UINT)SOUNDLIB::SEID_ALL)
	{
		ASSERT(eSEID < CSound::SOUND_EFFECT_COUNT);
		return this->SoundEffectArray[eSEID].SetMinMaxDistance(fMin, fMax);
	} else {
		bool bRes = true;
		for (UINT wIndex=CSound::SOUND_EFFECT_COUNT; wIndex--; )
			bRes &= this->SoundEffectArray[wIndex].SetMinMaxDistance(fMin, fMax);
		return bRes;
	}
}

//********************************************************************************
void CSound::SetMusicVolume(
//Sets volume of music.
	const int volume)  //(in) value between 0 (silent) and 255 (full)
{
#ifndef WITHOUT_SOUND
	ASSERT(volume >= 0 && volume <= 255);
	this->nMusicVolume = volume;
#ifdef USE_SDL_MIXER
	Mix_VolumeMusic((volume+1)/2); //(Max is 128)
#else
	if (this->nCurrentPlayingSongChannel >= 0)
	{
		ASSERT((UINT)this->nCurrentPlayingSongChannel < SONG_CHANNEL_COUNT);
		if (this->Streams[this->nCurrentPlayingSongChannel])
		{
			FSOUND_SetVolume(this->nCurrentPlayingSongChannel,volume);
		}
	}
#endif
#endif //WITHOUT_SOUND
}

//********************************************************************************
void CSound::SetChannelVolume(
//Sets the volume of a channel.
	const int nChannel, //(in) channel to set the volume of
	const int volume)   //(in) value between 0 (silent) and 255 (full)
{
#ifndef WITHOUT_SOUND
		ASSERT(nChannel != SOUNDLIB::SONG_NONE);
		ASSERT(volume >= 0);
		ASSERT(volume <= 255);
#ifdef USE_SDL_MIXER
		Mix_Volume(nChannel, (volume+1)/2); //(0-128)
#else
		FSOUND_SetVolume(nChannel, volume);
#endif
#endif
}

//********************************************************************************
#ifndef WITHOUT_SOUND
#ifdef USE_SDL_MIXER
void CSound::songFinishedCallback()
{
	ASSERT(!SongInfo.bHasEnded);
	SongInfo.bHasEnded = true;
}
#else
signed char F_CALLBACKAPI CSound::endStreamCallback(FSOUND_STREAM* stream, void* buff, int len, void* param)
//Returns: ignored when created by FSOUND_Stream_SetEndCallback
{
	ASSERT(!buff);
	ASSERT(!len);
	ASSERT(!SongInfo.bHasEnded);
	if (!SongInfo.bHasEnded) {
		SongInfo.stream = stream;
		SongInfo.nChannel = int(param);
		ASSERT(g_pTheSound);
		ASSERT(g_pTheSound->Streams[SongInfo.nChannel] == SongInfo.stream);
		SongInfo.bHasEnded = true;
	}
	return 0;
}
#endif
#endif

//********************************************************************************
void CSound::UpdateMusic()
//Handle song switching and perform music cross-fading incrementally.
//Call this method through the working event loop.
{
#ifndef WITHOUT_SOUND
	//Song switching
#ifdef USE_SDL_MIXER
	if (SongInfo.bHasEnded) {
		//Song ended normally (or faded out), play next song in list
		PlayNextSong();
	}
#else //FMOD
	if (SongInfo.bHasEnded) {
		const int nChannel = SongInfo.nChannel;
		ASSERT(nChannel >= 0);
		ASSERT((UINT)nChannel < CSound::SONG_CHANNEL_COUNT);

		if (this->Streams[nChannel] == SongInfo.stream)
		{
			const bool bFadingChannelStopped = nChannel == this->nFadingSongChannel;
			VERIFY(StopSong(nChannel));
			ASSERT(!this->Streams[nChannel]);

			//Start a new song only if this song wasn't already fading out while a new song is fading in.
			if (!bFadingChannelStopped)
				PlayNextSong();
		} else {
			//Channel was already replaced with a new stream after the song ended.
			//In this case, we shouldn't stop the song on that channel or play a new song.
			//Just free the resources for the song that ended.
			FSOUND_Stream_Close((FSOUND_STREAM*)SongInfo.stream); //make non-volatile
			SongInfo.bHasEnded = false;
		}
	}

	//Cross-fading
	if (!this->dwFadeDuration)
		return;  //no fading occurring -- stop here
	ASSERT(this->dwFadeBegin);

	//Shouldn't be cross-fading if there are no song channels set, or only one channel set.
	ASSERT(this->nFadingSongChannel != SOUNDLIB::SONG_NONE ||
			this->nCurrentPlayingSongChannel != SOUNDLIB::SONG_NONE);
	ASSERT(this->nFadingSongChannel != this->nCurrentPlayingSongChannel);
	ASSERT(this->nFadingSongChannel < (int)CSound::SONG_CHANNEL_COUNT ||
			this->nFadingSongChannel == SOUNDLIB::SONG_NONE);
	ASSERT(this->nCurrentPlayingSongChannel < (int)CSound::SONG_CHANNEL_COUNT ||
			this->nCurrentPlayingSongChannel == SOUNDLIB::SONG_NONE);

	const Uint32 dwElapsedFade = SDL_GetTicks() - this->dwFadeBegin;
	if (dwElapsedFade >= this->dwFadeDuration)
	{
		//Fade completed.
		this->dwFadeBegin = this->dwFadeDuration = 0L;
		if (this->nFadingSongChannel != SOUNDLIB::SONG_NONE)
			StopSong(this->nFadingSongChannel);
		ASSERT(this->nFadingSongChannel == SOUNDLIB::SONG_NONE);
		SetChannelVolume(this->nCurrentPlayingSongChannel, this->nMusicVolume);
	} else {
		//Set current volume of songs fading in/out.
		ASSERT(this->dwFadeDuration);
		const int vol = (int)((this->nMusicVolume * dwElapsedFade) / (float)this->dwFadeDuration);
		SetChannelVolume(this->nCurrentPlayingSongChannel, vol);
		if (this->nFadingSongChannel != SOUNDLIB::SONG_NONE)
			SetChannelVolume(this->nFadingSongChannel, this->nMusicVolume-vol);
	}
#endif
#endif //WITHOUT_SOUND
}

//********************************************************************************
void CSound::CrossFadeSong(
	const UINT eSongID,           //(in)   Song to fade in.
	const UINT dwFadeDuration)   //(in)   How long fade lasts (ms).
{
	CrossFadeSong(this->SongListArray + eSongID, dwFadeDuration);
}

//********************************************************************************
void CSound::CrossFadeSong(
//Plays a song and fades it in.  If a song is already playing, it fades out.
//If a song is still fading out when this method is called again,
//it is stopped so the current one can fade out.
//
//Params:
	list<WSTRING>* pSonglist,   //(in)   Song to fade in.
	const UINT dwFadeDuration)   //(in)   How long fade lasts (ms).
{
#ifndef WITHOUT_SOUND
	//Return successful without doing anything if music has been disabled.
	if (!this->bMusicOn || !this->bMusicAvailable)
		return;
	if (IsSongListPlaying(pSonglist))
		return; //song is already playing
	ASSERT(dwFadeDuration);

#ifdef USE_SDL_MIXER
	//Let SDL_mixer do the fading for us
	this->bFadeRequest = (Mix_PlayingMusic() && Mix_FadeOutMusic(dwFadeDuration));
	PlaySong(pSonglist);
	this->bFadeRequest = false;
#else //FMOD
	const int nOldPlayChannel = this->nCurrentPlayingSongChannel;

	//If a previous cross-fade is still in progress, stop it now.
	if (this->dwFadeDuration)
	{
		ASSERT(this->nFadingSongChannel != SOUNDLIB::SONG_NONE);
		ASSERT(this->dwFadeBegin);
		StopSong(this->nFadingSongChannel);
	}

	this->bFadeRequest = true;
	if (PlaySong(pSonglist))
	{
		if (nOldPlayChannel >= 0)
		{
			this->nFadingSongChannel = nOldPlayChannel;
			this->dwFadeDuration = dwFadeDuration;
			this->dwFadeBegin = SDL_GetTicks();
		} else {
			//No previous song was playing, so don't cross-fade -- just start new song at full volume.
			//!!todo: perform fade-in of a single song, when no song is fading out
			FSOUND_SetVolume(this->nCurrentPlayingSongChannel, this->nMusicVolume);
			ASSERT(!IsSoundPlayingOnChannel(GetFreeMusicChannel()));
		}
	}
	this->bFadeRequest = false;
#endif
#endif //WITHOUT_SOUND
}

//********************************************************************************
bool CSound::PlayNextSong()
//Play next song in song list.
//'SongList' is modified, with the song moved to the end of the list.
{
	//No other song should be playing or starting up at this point.
	SongInfo.bHasEnded = false;

#ifndef WITHOUT_SOUND
	if (this->SongList.empty())
		return false;	//no song list specified
	UINT mode = 0;
#ifndef USE_SDL_MIXER //(handled below in StartSongStream for SDL_mixer)
	//If only one song has been specified for play, tell it to loop it forever.
	mode = FSOUND_NORMAL | FSOUND_2D | FSOUND_IGNORETAGS;
	if (this->SongList.size() == 1)
		mode |= FSOUND_LOOP_NORMAL;
#endif

	//Get next song from songlist.
	WSTRING wstrSongFilename = this->SongList.front();
	ASSERT(wstrSongFilename.size() > 0);

	//Attempt to load song mod from disk first.
	WSTRING wstrSongFilepath = GetSongModPath();
	wstrSongFilepath += wstrSongFilename;
	SOUNDSTREAM *pStream = LoadSongStream(wstrSongFilepath, mode);

	if (!pStream) {
		//Use standard song path.
		wstrSongFilepath = GetSongPath();
		wstrSongFilepath += wstrSongFilename;
		pStream = LoadSongStream(wstrSongFilepath, mode);
	}

	//If loading from disk fails, then
	//provide a hook for loading a way other than streaming from disk.
	BYTE *pRawData = NULL;
	if (!pStream)
		pStream = LoadSong(wstrSongFilename.c_str(), mode, pRawData);

	//Begin play.
	if (!StartSongStream(pStream, pRawData))
		return false;

	//Rotate song to end of song list.
	if (this->SongList.size() > 1)
	{
		this->SongList.pop_front();
		this->SongList.push_back(wstrSongFilename);
	}

#endif //WITHOUT_SOUND

	return true;
}

SOUNDSTREAM* CSound::LoadSongStream(const WSTRING& wstrSongFilepath, UINT mode)
{
	//Convert Unicode filename for use with FMOD.
#ifdef WIN32
	char sANSI[512];
	WideCharToMultiByte(CP_ACP, 0, wstrSongFilepath.c_str(), -1, sANSI, 512, NULL, NULL);
#else
	char sANSI[wstrSongFilepath.length() + 1];
	UnicodeToUTF8(wstrSongFilepath.c_str(), sANSI);
#endif

	SOUNDSTREAM *pStream = NULL;
#ifdef USE_SDL_MIXER
	this->pSongStream = pStream = Mix_LoadMUS(sANSI);
#else //FMOD
	pStream = FSOUND_Stream_Open(sANSI, mode, 0, 0);
	if (!pStream)
	{
		//Probably file for loading stream isn't available.
		const int nErrCode = FSOUND_GetError();
		if (nErrCode != FMOD_ERR_FILE_NOTFOUND)
		{
			//If error was something else, inform.
			string str;
			CSound::GetLastSoundError(str);
			CFiles f;
			f.AppendErrorLog(str.c_str());
		}
	}
#endif
	return pStream;
}

//********************************************************************************
bool CSound::PlaySong(const UINT eSongID)
{
	return PlaySong(this->SongListArray + eSongID);
}

//********************************************************************************
bool CSound::PlaySong(
//Plays a song.
//
//Returns: whether song is playing
//
//Params:
	list<WSTRING>* pSonglist)
{
#ifndef WITHOUT_SOUND
	//Return successful without doing anything if music has been disabled.
	if (!this->bMusicOn || !this->bMusicAvailable)
		return true;

	if (IsSongListPlaying(pSonglist))
	{
		//A fade request should never ask to fade to the songlist that is already playing.
		ASSERT(!bFadeRequest);
		return true;
	}

	//Stop any currently playing song.
	if (!this->bFadeRequest)
	{
		StopSong();

		//If new song should start playing without any cross-fade,
		//reset cross fading parameters now.
		this->dwFadeDuration = this->dwFadeBegin = 0;
		this->nFadingSongChannel = SOUNDLIB::SONG_NONE;
	}

	this->SongList = *pSonglist;

#ifdef USE_SDL_MIXER
	//Don't call PlayNextSong if a fade was requested, it will be called by
	//the music updater when the current song is finished instead.
	if (!(this->bFadeRequest || PlayNextSong()))
		return false;
#else
	//Play the song.
	if (!PlayNextSong())
		return false;
#endif

	//Update original song list with new play order.
	*pSonglist = this->SongList;

#endif
	return true;
}

//********************************************************************************
bool CSound::StartSongStream(
//Returns: true if operation succeeded, else false
	SOUNDSTREAM *pStream, //stream to play
	BYTE *pRawData)       //optional buffer to raw stream data
{
	if (!pStream) //nothing to play
		return false;

#ifdef USE_SDL_MIXER
	this->allocedMusic = pRawData;  // Keep track of allocated music buffer
	Mix_HookMusicFinished(songFinishedCallback);
	//Telling SDL_mixer to repeat the song seems to have issues with scratchiness on repeat
	//let's hope it's been fixed by now
	const int nResult = Mix_PlayMusic(pStream, (this->SongList.size() == 1) ? -1 : 0);
	//const int nResult = Mix_PlayMusic(pStream, 0);
#else //FMOD
	const int nUseChannel = GetFreeMusicChannel();

	//Play the song.
	ASSERT(nUseChannel >= 0);
	ASSERT((UINT)nUseChannel < SONG_CHANNEL_COUNT);
	StopSong(nUseChannel);	//ensure any song playing on given channel is stopped
	const int nResult = FSOUND_Stream_Play(nUseChannel, pStream);
#endif
	if (nResult == -1) //Failure
	{
		//Play failed.
		string str;
		CSound::GetLastSoundError(str);
		CFiles f;
		f.AppendErrorLog(str.c_str());
		ASSERT(!"Failed to play loaded stream.");
#ifdef USE_SDL_MIXER
		if (this->pSongStream)
		{
			Mix_FreeMusic(this->pSongStream);
			this->pSongStream = NULL;
		}
#else
		FSOUND_Stream_Close(pStream);
#endif
		return false;
	}

	//Success.
#ifndef USE_SDL_MIXER
	ASSERT(nUseChannel >= 0);
	ASSERT(nUseChannel == nResult % 4096);  //channel used is really the requested channel
	this->Streams[nUseChannel] = pStream;
	FSOUND_Stream_SetEndCallback(pStream, endStreamCallback, (void*)nUseChannel);
	FSOUND_SetVolume(nUseChannel, //Set volume.
			this->bFadeRequest ? 0 : this->nMusicVolume);   //song fading in starts quiet
	this->nCurrentPlayingSongChannel = nUseChannel;
	if (!this->bFadeRequest)
		StopSong(GetFreeMusicChannel()); //ensure no song is playing on another channel now
	ASSERT(this->bFadeRequest || this->nFadingSongChannel == -1);
	ASSERT(this->bFadeRequest || !IsSoundPlayingOnChannel(GetFreeMusicChannel()));
	if (pRawData)
	{
		//Keep track of channel-data pair so the raw data can be deallocated
		//when channel is done playing.
		ASSERT(this->allocedChannels.find(nUseChannel) == this->allocedChannels.end());
		this->allocedChannels[nUseChannel] = pRawData;
	}
#endif
	return true;
}

//********************************************************************************
bool CSound::StopSong(
//Stops a currently playing song.
//
//Returns:
//True if no song is playing when function returns, false if not.
//
//Params:
#ifdef USE_SDL_MIXER
	const int /*nOnChannel UNUSED*/)
#else
	const int nOnChannel)   //(in) channel to stop playing music on
	                        //(default = SOUNDLIB::SONG_NONE, means stop all songs)
#endif
{
#ifndef WITHOUT_SOUND
	if (!this->bMusicAvailable) return true;

#ifdef USE_SDL_MIXER
	//Always stopping all songs, since there can be only one in SDL_mixer ..
	Mix_HookMusicFinished(NULL); //reset before HaltMusic
	Mix_HaltMusic();
	if (this->allocedMusic) {
		delete[] this->allocedMusic;
		this->allocedMusic = NULL;
	}
	if (this->pSongStream) {
		Mix_FreeMusic(this->pSongStream);
		this->pSongStream = NULL;
	}
	this->SongList.clear();
#else
	if (nOnChannel == SOUNDLIB::SONG_NONE)
	{
		//Stop all songs.
		bool bSuccess = true;
		for (UINT wIndex=0; wIndex<CSound::SONG_CHANNEL_COUNT; ++wIndex)
			if (!StopSong(wIndex))	//recursive call
				bSuccess = false;

		this->SongList.clear();
		return bSuccess;
	}

	ASSERT(nOnChannel >= 0 && (UINT)nOnChannel < CSound::SONG_CHANNEL_COUNT);
	if (this->Streams[nOnChannel])
	{
		FSOUND_Stream_Close(this->Streams[nOnChannel]);
		this->Streams[nOnChannel] = NULL;

		//If our data buffer is being used to hold the song, then free it now.
		CHANNELBUFFERMAP::const_iterator iter = this->allocedChannels.find(nOnChannel);
		if (iter != this->allocedChannels.end())
		{
			delete[] (BYTE*)(iter->second);
			VERIFY(this->allocedChannels.erase(nOnChannel) == 1);
		}
	}
	else if (!FSOUND_StopSound(nOnChannel))
	{
		string str;
		CSound::GetLastSoundError(str);
		CFiles f;
		f.AppendErrorLog(str.c_str());
		ASSERT(!"Failed to stop song.");
		return false;
	}
	//No stream or data buffer should be left on this channel by this point.
	ASSERT(!this->Streams[nOnChannel]);
	ASSERT(this->allocedChannels.find(nOnChannel) == this->allocedChannels.end());

	if (nOnChannel == this->nCurrentPlayingSongChannel)
		this->nCurrentPlayingSongChannel = SOUNDLIB::SONG_NONE;
	else if (nOnChannel == this->nFadingSongChannel)
	{
		//Stopping music cross-fading.
		this->nFadingSongChannel = SOUNDLIB::SONG_NONE;
		this->dwFadeDuration = this->dwFadeBegin = 0;
	}
#endif

#endif //WITHOUT_SOUND
	return true;
}

//********************************************************************************
void CSound::PlaySoundEffect(
//Plays a sound effect.
//
//Params:
	const UINT eSEID, //(in) A SEID_* constant indicating sound effect to play.
	float* pos, float* vel, //(in) 3D sound info (default = NULL)
	const bool bUseVoiceVolume,	//(in) whether to use voice volume instead of
											//sound effect volume [default=false]
	const float frequencyMultiplier) //(in) for changing the playback frequency [default=1.0]
{
#ifndef WITHOUT_SOUND
	//Return successful without doing anything if sound effects have been disabled.
	if (!this->bSoundEffectsAvailable) return;
	if ((bUseVoiceVolume && !this->bVoicesOn) ||
			(!bUseVoiceVolume && !this->bSoundEffectsOn)) return;

	ASSERT(eSEID < CSound::SOUND_EFFECT_COUNT);

	//Assign sound effect to a sound channel.
	int nChannel = this->SoundEffectArray[eSEID].GetChannel();
	if (nChannel == SOUNDLIB::SEID_NONE)
	{
		//Sound hasn't been designated a channel.  Find free channel to play sound on.
		nChannel = GetFreeSharedChannel();
		if (nChannel == SOUNDLIB::SEID_NONE)
			return;  //no channels available
	}

	//Play it.
	ASSERT(nChannel < (int)CSound::CHANNEL_COUNT);
	if (this->SoundEffectArray[eSEID].Play(nChannel, frequencyMultiplier))
	{
		//This sound effect is playing on this channel.
		this->ChannelSoundEffects[nChannel] = eSEID;

		//Set sound's positional/volume info.
		Update(nChannel, pos, vel, eSEID, bUseVoiceVolume);
	}
#endif
}

//*****************************************************************************
int CSound::PlaySoundEffect(
//Plays sound effect stored as raw data in stretchy buffer.
//
//Returns: channel
//
//Params:
	const CStretchyBuffer& sound, //sound data
	const bool bLoop,       //(in) loop indefinitely if set [default=false]
	float* pos, float* vel, //(in) 3D sound info [default = NULL]
	const bool bUseVoiceVolume)	//(in) [default=false]
{
#ifdef WITHOUT_SOUND
	return SOUNDLIB::SEID_NONE;
#else
	//Return successful without doing anything if sound effects have been disabled.
	if (!this->bSoundEffectsOn || !this->bSoundEffectsAvailable) return SOUNDLIB::SEID_NONE;

	string str;
	CFiles f;

#ifdef USE_SDL_MIXER
	SDL_RWops *pOp = SDL_RWFromConstMem((BYTE*)sound, sound.Size());
	ASSERT(pOp);
	Mix_Chunk *pSample = Mix_LoadWAV_RW(pOp, 1);
#else
	unsigned int mode = FSOUND_LOADMEMORY;
	if (pos || vel) mode |= FSOUND_2D;
	FSOUND_SAMPLE *pSample = FSOUND_Sample_Load(FSOUND_FREE,
			(const char*)(const BYTE*)sound, mode, 0, sound.Size());
#endif
	if (!pSample)
	{
		CSound::GetLastSoundError(str);
		f.AppendErrorLog(str.c_str());
		ASSERT(!"Failed to load sound in CSound::PlaySoundEffect().");
		return SOUNDLIB::SEID_NONE;
	}

	//Get free sound channel.
	const int nUseChannel = GetFreeSharedChannel();
	if (nUseChannel == SOUNDLIB::SEID_NONE)
	{
#ifdef USE_SDL_MIXER
		Mix_FreeChunk(pSample);
#else
		FSOUND_Sample_Free(pSample);
#endif
		return SOUNDLIB::SEID_NONE;
	}

#ifdef USE_SDL_MIXER
	Mix_HaltChannel(nUseChannel);

	const int r = Mix_PlayChannel(nUseChannel, pSample, bLoop ? -1 : 0);
#else
	VERIFY(FSOUND_StopSound(nUseChannel));

	//Play the sample.
	FSOUND_Sample_SetMode(pSample, bLoop ? FSOUND_LOOP_NORMAL : FSOUND_LOOP_OFF);
	const int r = FSOUND_PlaySound(nUseChannel, pSample);
#endif
	if (r == -1)
	{
		//Couldn't play.
#ifdef USE_SDL_MIXER
		Mix_FreeChunk(pSample);
#else
		FSOUND_Sample_Free(pSample);
#endif
		CSound::GetLastSoundError(str);
		f.AppendErrorLog(str.c_str());
		ASSERT(!"Failed to play sound in CSound::PlaySoundEffect().");
		return SOUNDLIB::SEID_NONE;
	}
	this->soundDump.push_back(pSample);

	//Indicate that no preset SEID is playing on this channel.
	ASSERT((UINT)nUseChannel < CHANNEL_COUNT);
	this->ChannelSoundEffects[nUseChannel] = static_cast<UINT>(SOUNDLIB::SEID_NONE);

	Update(nUseChannel, pos, vel, SOUNDLIB::SEID_NONE, bUseVoiceVolume);   //Set sound's positional/volume info.
	return nUseChannel;
#endif //WITHOUT_SOUND
}

//*****************************************************************************
int CSound::PlayVoice(
//Plays voice stored as raw data in stretchy buffer.
//
//Returns: channel sound is playing on, or SOUNDLIB::SEID_NONE if no channels are available
//
//Params:
	const CStretchyBuffer& sound) //sound data
{
#ifdef WITHOUT_SOUND
	return SOUNDLIB::SEID_NONE;
#else
	//Return successful without doing anything if sound effects have been disabled.
	if (!this->bVoicesOn || !this->bSoundEffectsAvailable) return SOUNDLIB::SEID_NONE;

	string str;
	CFiles f;

#ifdef USE_SDL_MIXER
	SDL_RWops *pOp = SDL_RWFromConstMem((BYTE*)sound, sound.Size());
	Mix_Chunk *pSample = Mix_LoadWAV_RW(pOp, 1);
#else
	unsigned int mode = FSOUND_LOADMEMORY;
	FSOUND_SAMPLE *pSample = FSOUND_Sample_Load(FSOUND_FREE,
			(const char*)(const BYTE*)sound, mode, 0, sound.Size());
#endif
	if (!pSample)
	{
		CSound::GetLastSoundError(str);
		f.AppendErrorLog(str.c_str());
		ASSERT(!"Failed to load sound in CSound::PlayVoice().");
		return SOUNDLIB::SEID_NONE;
	}

	const int nUseChannel = GetFreeSharedChannel();
#ifdef USE_SDL_MIXER
	Mix_HaltChannel(nUseChannel);

	const int r = Mix_PlayChannel(nUseChannel, pSample, 0);
#else
	if (nUseChannel == SOUNDLIB::SEID_NONE) //no sound channel available
	{
#ifdef USE_SDL_MIXER
		Mix_FreeChunk(pSample);
#else
		FSOUND_Sample_Free(pSample);
#endif
		return SOUNDLIB::SEID_NONE;
	}
	VERIFY(FSOUND_StopSound(nUseChannel));

	//Play the sample.
	FSOUND_Sample_SetMode(pSample, FSOUND_LOOP_OFF);
	const int r = FSOUND_PlaySound(nUseChannel, pSample);
#endif
	if (r == -1)
	{
		//Couldn't play.
#ifdef USE_SDL_MIXER
		Mix_FreeChunk(pSample);
#else
		FSOUND_Sample_Free(pSample);
#endif
		CSound::GetLastSoundError(str);
		f.AppendErrorLog(str.c_str());
		ASSERT(!"Failed to play sound in CSound::PlayVoice().");
		return SOUNDLIB::SEID_NONE;
	}
	this->soundDump.push_back(pSample);

	//Indicate that no preset SEID is playing on this channel.
	ASSERT((UINT)nUseChannel < CHANNEL_COUNT);
	this->ChannelSoundEffects[nUseChannel] = static_cast<UINT>(SOUNDLIB::SEID_NONE);

#ifdef USE_SDL_MIXER
	Mix_Volume(nUseChannel, (this->nVoiceVolume+1)/2);
#else
	FSOUND_SetVolume(nUseChannel, this->nVoiceVolume);
#endif
	return nUseChannel;
#endif //WITHOUT_SOUND
}

//*****************************************************************************
void CSound::StopAllSoundEffects()
{
#ifndef WITHOUT_SOUND
#ifdef USE_SDL_MIXER
	Mix_HaltChannel(-1);
#else
	for (UINT nChannelNo = SONG_CHANNEL_COUNT;
			nChannelNo < CHANNEL_COUNT; ++nChannelNo)
		FSOUND_StopSound(nChannelNo);
#endif
#endif //WITHOUT_SOUND
}

//*****************************************************************************
bool CSound::StopSoundEffect(const UINT eSEID)
//Returns: whether sound effect is successfully stopped
{
#ifndef WITHOUT_SOUND
	//Return successful without doing anything if sound effects have been disabled.
	if (!this->bSoundEffectsOn || !this->bSoundEffectsAvailable) return true;

	ASSERT(eSEID < CSound::SOUND_EFFECT_COUNT);
	const int nChannel = this->SoundEffectArray[eSEID].GetChannel();
	if (nChannel != SOUNDLIB::SEID_NONE)
	{
		//Sound is designated to a specific channel -- check it.
		ASSERT(nChannel < (int)CSound::CHANNEL_COUNT);

		//Other sound effects may play on the same channel.  Check that my sound effect
		//is the last one that was played.
		if (this->ChannelSoundEffects[nChannel] != eSEID) return true;
		return StopSoundOnChannel(nChannel);
	}

	bool bStopped = true;
	for (UINT nChannelNo = SONG_CHANNEL_COUNT + PRIVATE_SOUND_CHANNEL_COUNT;
			nChannelNo < CHANNEL_COUNT; ++nChannelNo)
		if (this->ChannelSoundEffects[nChannelNo] == eSEID)
			//This channel was playing this sound.  Check whether it's still playing.
			if (IsSoundPlayingOnChannel(nChannelNo))
				bStopped &= StopSoundOnChannel(nChannelNo);
	return bStopped;
#endif
}

//*****************************************************************************
bool CSound::StopSoundOnChannel(const int nChannel)
//Stop sound on specified channel.
{
#ifndef WITHOUT_SOUND
	ASSERT(nChannel >= 0);
	ASSERT(static_cast<UINT>(nChannel) < CHANNEL_COUNT);
#ifdef USE_SDL_MIXER
	Mix_HaltChannel(nChannel);
	this->ChannelSoundEffects[nChannel] = static_cast<UINT>(SOUNDLIB::SEID_NONE);
	return true;
#else
	const bool bRes = FSOUND_StopSound(nChannel) != 0;
	if (bRes)
		//Indicate no sound is playing on this channel any more.
		this->ChannelSoundEffects[nChannel] = static_cast<UINT>(SOUNDLIB::SEID_NONE);
	return bRes;
#endif
#endif
}

//*****************************************************************************
UINT CSound::GetFreeSharedChannel() const
//Get first free shared channel for playing this sound.
{
#ifndef WITHOUT_SOUND
	for (UINT nChannelNo = SONG_CHANNEL_COUNT + PRIVATE_SOUND_CHANNEL_COUNT;
			nChannelNo < CHANNEL_COUNT; ++nChannelNo)
		if (!IsSoundPlayingOnChannel(nChannelNo))
		{
			//Found a channel with no sound playing.
			return nChannelNo;
		}
	//No channels are available for playing this sound.
#endif //WITHOUT_SOUND
	return (UINT)SOUNDLIB::SEID_NONE;
}

//**********************************************************************************
WSTRING CSound::GetSongPath()
//Gets path where songs are found.
{
	CFiles Files;
	WSTRING wstrPath = Files.GetResPath();
	return wstrPath + wcszMusic;
}

//**********************************************************************************
WSTRING CSound::GetSongModPath()
//Gets path where songs are found.
{
	CFiles Files;
	WSTRING wstrPath = Files.GetDatPath();
	return wstrPath + wcszMusic;
}

//*****************************************************************************
UINT CSound::GetSoundLength(const int nChannel) const
//Returns: the length of the sound playing on nChannel, in milliseconds
{
#ifdef USE_SDL_MIXER
	return GetSoundLength(Mix_Playing(nChannel) ? Mix_GetChunk(nChannel) : NULL);
#else
	return GetSoundLength(FSOUND_GetCurrentSample(nChannel));
#endif
}

UINT CSound::GetSoundLength(SOUNDSAMPLE *pSample) const
//Returns: the length of the sound sample, in milliseconds
{
#ifdef WITHOUT_SOUND
	return 0;
#else
	if (!pSample) return 0;
#ifdef USE_SDL_MIXER
	//!!! FIXME: patch SDL_mixer to add a function for getting this
	UINT wLength = ((Mix_Chunk*)pSample)->alen; 
	int nDefFreq, channels;
	Uint16 format;
	Mix_QuerySpec(&nDefFreq, &format, &channels);
	if (format != AUDIO_U8 && format != AUDIO_S8)
		wLength /= 2;
	wLength /= channels;
#else
	const UINT wLength = FSOUND_Sample_GetLength(pSample);
	int nDefFreq;
	FSOUND_Sample_GetDefaults(pSample, &nDefFreq, NULL, NULL, NULL);
#endif
	return wLength * 1000 / nDefFreq;   //convert samples to ms
#endif //WITHOUT_SOUND
}


//*****************************************************************************
UINT CSound::GetSoundLength(const CStretchyBuffer& sound) const
{
#ifdef WITHOUT_SOUND
	return 0;
#else
	int nDefFreq;
	UINT wLength;
#	ifdef USE_SDL_MIXER
	SDL_RWops *pOp = SDL_RWFromConstMem((char*)(BYTE*)sound, sound.Size());
	ASSERT(pOp);
	Mix_Chunk *pSample = Mix_LoadWAV_RW(pOp, 1);
	if (!pSample)
		return 0;

	//!!! FIXME: patch SDL_mixer to add a function for getting this
	wLength = ((Mix_Chunk*)pSample)->alen; 
	int channels;
	Uint16 format;
	Mix_QuerySpec(&nDefFreq, &format, &channels);
	if (format != AUDIO_U8 && format != AUDIO_S8)
		wLength /= 2;
	wLength /= channels;

	Mix_FreeChunk(pSample);
#	else
	unsigned int mode = FSOUND_LOADMEMORY;
	FSOUND_SAMPLE *pSample = FSOUND_Sample_Load(FSOUND_FREE,
			(const char*)(const BYTE*)sound, mode, 0, sound.Size());

	wLength = FSOUND_Sample_GetLength(pSample);

	FSOUND_Sample_GetDefaults(pSample, &nDefFreq, NULL, NULL, NULL);

	FSOUND_Sample_Free(pSample);
#	endif

	return wLength * 1000 / nDefFreq;
#endif
}

//*****************************************************************************
UINT CSound::GetNumInstancesPlaying(const UINT eSEID) const
{
#ifdef __sgi
	return 1;
#endif
	//Return no instances when sound effects have been disabled.
	if (!this->bSoundEffectsOn || !this->bSoundEffectsAvailable)
		return 0;

	ASSERT(eSEID < CSound::SOUND_EFFECT_COUNT);

	//Get channel sound effect is designated to play on.
	const int nChannel = this->SoundEffectArray[eSEID].GetChannel();
	if (nChannel != SOUNDLIB::SEID_NONE)
	{
		//Sound is designated to a specific channel -- check it.
		ASSERT(nChannel < (int)CSound::CHANNEL_COUNT);

		//Other sound effects may play on the same channel.  Check that my sound effect
		//is the last one that was played.
		if (this->ChannelSoundEffects[nChannel] != eSEID)
			return 0;

		//Check that a sample is currently playing on the channel.  If it is, then I
		//know that it is playing a sample for my sound effect.
		return IsSoundPlayingOnChannel(nChannel) ? 1 : 0;
	}

	//Sound could be playing on any public channel.  Must check them all.
	UINT count=0;
	for (UINT nChannelNo = SONG_CHANNEL_COUNT + PRIVATE_SOUND_CHANNEL_COUNT;
			nChannelNo < CHANNEL_COUNT; ++nChannelNo)
		if (this->ChannelSoundEffects[nChannelNo] == eSEID)
			//This channel was playing this sound.  Check whether it's still playing.
			if (IsSoundPlayingOnChannel(nChannelNo))
				++count;

	return count;
}

//*****************************************************************************
bool CSound::IsSoundEffectPlaying(
//Is a sound effect playing now?
//
//Params:
	const UINT eSEID) //(in)   Sound effect to check.
//
//Returns:
//True if it is, false if not.
const
{
	return GetNumInstancesPlaying(eSEID) > 0;
}

//***********************************************************************************
UINT CSound::GetSoundIDPlayingOnChannel(const int nChannel) const
//Returns the SEID playing on specified channel, or SEID_NONE if none.
{
	ASSERT(nChannel >= 0);
	ASSERT(static_cast<UINT>(nChannel) < CHANNEL_COUNT);
	if (!IsSoundPlayingOnChannel(nChannel))
		return static_cast<UINT>(SOUNDLIB::SEID_NONE);
	return this->ChannelSoundEffects[nChannel];
}

//***********************************************************************************
bool CSound::IsSoundPlayingOnChannel(const int nChannel) const
{
	ASSERT(nChannel >= 0);
#ifdef USE_SDL_MIXER
	return Mix_Playing(nChannel);
#else
	return FSOUND_IsPlaying(nChannel) != 0;
#endif
}

//***********************************************************************************
bool CSound::WaitForSoundEffectsToStop(
//Wait for all sound effects to stop playing.
//
//Params:
	const UINT dwMaxWaitTime) //(in)   Longest time in msecs to wait.  Default
								//    is 3000 (3 secs).
//
//Returns:
//True if all sound effects stopped, false if max wait time elapsed.
const
{
#ifndef WITHOUT_SOUND
	//Return successful without doing anything if sound effects have been disabled.
	if (!this->bSoundEffectsOn || !this->bSoundEffectsAvailable) return true;

	UINT dwStartTime = SDL_GetTicks();
	do
	{
#ifdef USE_SDL_MIXER
		if (!Mix_Playing(-1)) return true;
#else
		//Check for samples playing on channels reserved for samples.  Channels
		//after the module channels are sample channels.
		UINT nChannelNo;
		for (nChannelNo = SONG_CHANNEL_COUNT; nChannelNo < CHANNEL_COUNT;
				++nChannelNo)
		{
			if (FSOUND_IsPlaying(nChannelNo)) break; //At least one channel is playing.
		}
		if (nChannelNo == CHANNEL_COUNT) return true; //Everything has stopped.
#endif
		//Let other threads run.
		SDL_Delay(100);
	}
	while (SDL_GetTicks() - dwStartTime < dwMaxWaitTime);

	//Timed out waiting.
	return false;
#else
	return true;
#endif
}

//***********************************************************************************
void CSound::Update(
//Update "ears" for 2D/3D sound engine.
//
//Params:
	float *fPos,    //(in) position
	float *fDir,    //(in) orientation
	float *fVel)   //(in) velocity
{
#ifndef WITHOUT_SOUND
#ifndef USE_SDL_MIXER
	if (this->b3DSound)
	{
		ASSERT(fDir);

		//Orientation vectors must be of unit length.  Normalize.
		const float fDirMagnitude = static_cast<float>(sqrt(fDir[0]*fDir[0] + fDir[1]*fDir[1] + fDir[2]*fDir[2]));
		ASSERT(fDirMagnitude > 0.0);
		const float fDirOneOverMagnitude = 1.0f / fDirMagnitude;
		fDir[0] *= fDirOneOverMagnitude;
		fDir[1] *= fDirOneOverMagnitude;
		fDir[2] *= fDirOneOverMagnitude;

		FSOUND_3D_Listener_SetAttributes(fPos, fVel, fDir[0], fDir[1], fDir[2],
			0.0f, 0.0f, 1.0f);   //up vector
		FSOUND_Update();
	} else
#endif
	{
		//2D sound.
		if (!fPos)
			this->f2DListenerPos[0] = this->f2DListenerPos[1] = 0.0;			//origin
		else
		{
			this->f2DListenerPos[0] = fPos[0];
			this->f2DListenerPos[1] = fPos[1];
		}
		if (!fDir)
		{
			this->f2DListenerDir[0] = 0.0;  this->f2DListenerDir[1] = 1.0;	//north
		} else {
			//Orientation vector must be of unit length.  Normalize.
			const float fDirMagnitude = static_cast<float>(sqrt(fDir[0]*fDir[0] + fDir[1]*fDir[1]));
			ASSERT(fDirMagnitude > 0.0);
			const float fDirOneOverMagnitude = 1.0f / fDirMagnitude;

			this->f2DListenerDir[0] = fDir[0] * fDirOneOverMagnitude;
			this->f2DListenerDir[1] = fDir[1] * fDirOneOverMagnitude;
		}

		//Velocity is being ignored.
	}
#endif
}

//***********************************************************************************
void CSound::Update(
//Update position/velocity and volume of a sound channel.
//
//Params:
	const int nChannel,
	float *fPos, float *fVel, const UINT eSEID,	//[default=NULL,NULL,SEID_NONE]
	const bool bUseVoiceVolume)	//(in) whether to use voice volume instead of
											//sound effect volume [default=false]
{
#ifndef WITHOUT_SOUND
#ifndef USE_SDL_MIXER
	if (this->b3DSound)
	{
		VERIFY(FSOUND_3D_SetAttributes(nChannel, fPos, fVel) != 0);
	} else
#endif
	{
		if (!fPos)
		{
			//Place sound right on listener.
#ifdef USE_SDL_MIXER
			Mix_SetPanning(nChannel, 127, 127); //(don't unregister, messes up volume -- see below)
			Mix_Volume(nChannel, bUseVoiceVolume ?
				(this->nVoiceVolume+1)/2 : (this->nSoundVolume+1)/2);
#else
			FSOUND_SetPan(nChannel, 128);
			FSOUND_SetVolume(nChannel, bUseVoiceVolume ? this->nVoiceVolume : this->nSoundVolume);
#endif
		} else {
			//Determine amount of panning based on listener position+direction,
			//and sound position by calculating cosine of angle between
			//the vector facing to the listener's right, and the vector
			//of the sound direction (result will be within [-1,1], with 0 centered).
			//COS = (v1 dot v2)/( |v1| |v2| )
			//    = ((dir[y],-dir[x]) dot (sound-listener)) / ( |v1| |sound-listener| )
			//				to get the right-hand vector for listener's orientation
			//		= ((dir[y],-dir[x]) dot (sound-listener)) / |sound-listener|
			//				can remove v1 since listener orientation vector is already normalized
			ASSERT(ABS(1.0 - sqrt(this->f2DListenerDir[0]*this->f2DListenerDir[0] +
					this->f2DListenerDir[1]*this->f2DListenerDir[1])) < 0.001f);
			const float fDistX = ABS(fPos[0]-this->f2DListenerPos[0]);
			const float fDistY = ABS(fPos[1]-this->f2DListenerPos[1]);
			const float v2 = fDistX * fDistX + fDistY * fDistY;
			const float fCosine = v2 > 0.0 ?
				(this->f2DListenerDir[1] * (this->f2DListenerPos[0]-fPos[0]) +
				-this->f2DListenerDir[0] * (this->f2DListenerPos[1]-fPos[1]))
				/ static_cast<float>(sqrt(v2)) : 0;
			ASSERT(-1.0 <= fCosine && fCosine <= 1.0);

#ifdef USE_SDL_MIXER
			int rightvolume = static_cast<int>((fCosine+1.0) * 127.0); // /2*254 = *127
			//FIXME: Do true panning in stead of this linear panning (= technically incorrect but "works")
			Mix_SetPanning(nChannel, 254 - rightvolume, rightvolume);
#else
			FSOUND_SetPan(nChannel, static_cast<int>(((fCosine+1.0)/2.0) * 255));	//[0..1] * 255
#endif

			//Set sound volume based on distance to listener.
			float fDistToSound;
			switch (this->nDistanceNorm)
			{
				case 0: fDistToSound = MAX(fDistX, fDistY); break; //L-inf norm
				case 1: fDistToSound = fDistX + fDistY; break;     //Manhattan
				case 2: fDistToSound = static_cast<float>(sqrt(fDistX*fDistX + fDistY*fDistY)); break; //Euclidean
				default:
					fDistToSound = static_cast<float>(pow(
						pow(fDistX, this->nDistanceNorm) + pow(fDistY, this->nDistanceNorm),
						1.0f/(float)this->nDistanceNorm));
				break;
			}
			float fMin = this->fMinDist, fMax = this->fMaxDist;
			if (eSEID != (UINT)SOUNDLIB::SEID_NONE)
				GetMinMaxDistance(eSEID, fMin, fMax);
			if (fDistToSound < fMin)
				fDistToSound = fMin;
			else if (fDistToSound > fMax)
				fDistToSound = fMax;

			//Volume has linear fall-off based on existing in a roughly 2D world.
			ASSERT(fDistToSound > 0.0);
			const int nVol = static_cast<int>((bUseVoiceVolume ?
					this->nVoiceVolume : this->nSoundVolume) * fMin / fDistToSound);
			ASSERT(nVol >= 0);
			ASSERT(nVol <= 255);
#ifdef USE_SDL_MIXER
			Mix_Volume(nChannel, (nVol+1)/2);
#else
			FSOUND_SetVolume(nChannel, nVol);
#endif
		}

		//Velocity is being ignored.
	}
#endif
}

//***********************************************************************************
bool CSound::VerifySound(const CStretchyBuffer &buffer) const
//Returns: true if the buffer represents valid sound data, else false.
{
#ifdef USE_SDL_MIXER
	SDL_RWops *pOp = SDL_RWFromConstMem((char*)(BYTE*)buffer, buffer.Size());
	ASSERT(pOp);
	Mix_Chunk *pSample = Mix_LoadWAV_RW(pOp, 1);
	if (!pSample)
		return false;
	Mix_FreeChunk(pSample);
	return true;
#else
	FSOUND_SAMPLE *pSample = FSOUND_Sample_Load(FSOUND_FREE, (char*)(BYTE*)buffer,
			FSOUND_LOADMEMORY | FSOUND_2D, 0, buffer.Size());
	if (!pSample)
		return false;
	FSOUND_Sample_Free(pSample);
	return true;
#endif
}

//
//CSound Protected methods.
//

//***********************************************************************************
#ifndef USE_SDL_MIXER
int CSound::GetFreeMusicChannel() const
//Returns: which channel is free for playing a music sample.
{
	ASSERT(CSound::SONG_CHANNEL_COUNT == 2);  //modify this method if this changes
	if (this->nCurrentPlayingSongChannel == 0) return 1;
	ASSERT(this->nCurrentPlayingSongChannel == SOUNDLIB::SONG_NONE ||
			this->nCurrentPlayingSongChannel == 1);
	return 0;
}
#endif

//***********************************************************************************
void CSound::GetLastSoundError(
//Gets last sound error code along with test description.
//
//Params:
	string &strErrDesc)  //(in/out)  Corresponds to error code.  Appends to end of
						//       string which may or may not be empty.
{
#ifndef WITHOUT_SOUND
#ifdef USE_SDL_MIXER
	//Get SDL_mixer error
	strErrDesc = "    SDL_Mixer Error: ";
	strErrDesc += Mix_GetError();
	strErrDesc += "\n";
#else
	//Get FMOD error
	int nErrCode = FSOUND_GetError();
	char szTemp[30];
	sprintf(szTemp, "    FMOD Error #%d: ", nErrCode);
	strErrDesc += szTemp;

	//These are taken out of the FMOD 3.5 docs.  Update with new versions as
	//needed.
	switch (nErrCode)
	{
		case FMOD_ERR_NONE:
			strErrDesc +=  "No errors." NEWLINE;
		break;

		case FMOD_ERR_BUSY:
			strErrDesc +=  "Cannot call this command after FSOUND_Init. Call "
							"FSOUND_Close first." NEWLINE;
		break;

		case FMOD_ERR_UNINITIALIZED:
			strErrDesc +=  "This command failed because FSOUND_Init or "
							"FSOUND_SetOutput was not called." NEWLINE;
		break;

		case FMOD_ERR_INIT:
			strErrDesc +=  "Error initializing output device." NEWLINE;
		break;

		case FMOD_ERR_ALLOCATED:
			strErrDesc +=  "Error initializing output device, but more "
							"specifically, the output device is already in use and "
							"cannot be reused." NEWLINE;
      break;

		case FMOD_ERR_PLAY:
			strErrDesc +=  "Playing the sound failed." NEWLINE;
      break;

		case FMOD_ERR_OUTPUT_FORMAT:
			strErrDesc +=  "Soundcard does not support the features needed for "
							"this soundsystem (16bit stereo output)." NEWLINE;
      break;

		case FMOD_ERR_COOPERATIVELEVEL:
			strErrDesc +=  "Error setting cooperative level for hardware." NEWLINE;
      break;

		case FMOD_ERR_CREATEBUFFER:
			strErrDesc +=  "Error creating hardware sound buffer." NEWLINE;
      break;

		case FMOD_ERR_FILE_NOTFOUND:
			strErrDesc +=  "File not found." NEWLINE;
      break;

		case FMOD_ERR_FILE_FORMAT:
			strErrDesc +=  "Unknown file format." NEWLINE;
      break;

		case FMOD_ERR_FILE_BAD:
			strErrDesc +=  "Error loading file." NEWLINE;
      break;

		case FMOD_ERR_MEMORY:
			strErrDesc +=  "Not enough memory or resources." NEWLINE;
      break;

		case FMOD_ERR_VERSION:
			strErrDesc +=  "The version number of this file format is not "
							"supported." NEWLINE;
      break;

		case FMOD_ERR_INVALID_PARAM:
			strErrDesc +=  "An invalid parameter was passed to this "
							"function." NEWLINE;
      break;

		case FMOD_ERR_NO_EAX:
			strErrDesc +=  "Tried to use an EAX command on a non EAX enabled "
							"channel or output." NEWLINE;
      break;

		case FMOD_ERR_CHANNEL_ALLOC:
			strErrDesc +=  "Failed to allocate a new channel." NEWLINE;
      break;

		case FMOD_ERR_RECORD:
			strErrDesc +=  "Recording is not supported on this machine." NEWLINE;
      break;

		case FMOD_ERR_MEDIAPLAYER:
			strErrDesc +=  "Windows Media Player not installed so cannot play wma "
							"or use internet streaming." NEWLINE;
		break;

		default:
			strErrDesc +=  "Description unavailable." NEWLINE;
	}

	//return nErrCode;
#endif
#else
	//return 0;
#endif
}

//**********************************************************************************
bool CSound::InitSound()
//Initializes sound module.
//
//Returns:
//true if at least sound effects will be available, false otherwise.
{
#ifdef WITHOUT_SOUND
	return false;
#else
	this->bSoundEffectsAvailable = this->bMusicAvailable = true;

	//This log string will appear if an error occurs.
	string strInitLog = "This is what happened while initializing sound:" NEWLINE;

	while (true) //non-looping.
	{
#ifdef USE_SDL_MIXER
		if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
		{
			strInitLog += "  Unable to initialize SDL audio.\n";
			break;
		}

		SDL_version mix_compiled;
		MIX_VERSION(&mix_compiled);
		const SDL_version *mix_linked = Mix_Linked_Version();
		//Different major and minor version may be incompatible, patch shouldn't matter
		if (mix_compiled.major != mix_linked->major ||
		    mix_compiled.minor != mix_linked->minor ||
		    mix_compiled.patch >  mix_linked->patch)
		{
			char szVersion[100];
			sprintf(szVersion, "  Compiled against SDL_mixer %i.%i.%i but linked to %i.%i.%i!\n",
				mix_compiled.major, mix_compiled.minor, mix_compiled.patch,
				mix_linked->major,  mix_linked->minor,  mix_linked->patch);
			strInitLog += szVersion;
			//break;  It might work
		}

		//TODO: Add user settings for sound freq and chunk size ? (slow vs fast pc ..)
		//(chunk size of 2048 isn't the most horrible compromise imaginable, I think)
		if (!Mix_OpenAudio(44100, AUDIO_S16SYS, 2, 2048))
			strInitLog += "  SDL_mixer was initialized with best case parms.\n";
		else
		{
			strInitLog += "  SDL_mixer(44100, AUDIO_S16SYS, 2, 2048) failed.\n";
			
			//Try 22khz and 11khz
			if (!Mix_OpenAudio(22050, AUDIO_S16SYS, 2, 2048)) return true;
			strInitLog += "  SDL_mixer(22050, AUDIO_S16SYS, 2, 2048) failed.\n";
			if (!Mix_OpenAudio(11025, AUDIO_S16SYS, 2, 2048)) return true;
			strInitLog += "  SDL_mixer(11025, AUDIO_S16SYS, 2, 2048) failed.\n";
			
			//Give up.
			break;
		}

		//Set number of channels
		Mix_AllocateChannels(SAMPLE_CHANNEL_COUNT);

		return true;
		
#else
		//Compare loaded FMOD.DLL version to import library linked into .EXE.
		if (FSOUND_GetVersion() < FMOD_VERSION)
		{
			char szVersion[100];
			sprintf(szVersion, "  FMOD.DLL version is %g and import library is %g!" NEWLINE,
					FSOUND_GetVersion(), FMOD_VERSION);
			strInitLog += szVersion;
			break;
		}
		else
			strInitLog += "  FMOD.DLL version matches import library." NEWLINE;

		//Take advantage of hardware channels for 3D sound.
		const bool bRes = FSOUND_SetMinHardwareChannels(CHANNEL_COUNT) != 0;

		//Init FSOUND--try best case params first.
		if (FSOUND_Init(44100, CHANNEL_COUNT, 0))
			strInitLog += "  FSOUND was initialized with best case params." NEWLINE;
		else
		{
			strInitLog += "  FSOUND_Init(44100, CHANNEL_COUNT, 0) failed." NEWLINE;

			//Maybe the mixing rate is too high?  Try 22mhz and 11mhz.
			if (FSOUND_Init(22050, CHANNEL_COUNT, 0)) return true;
			strInitLog += "  FSOUND_Init(22050, CHANNEL_COUNT, 0) failed." NEWLINE;
			if (FSOUND_Init(11025, CHANNEL_COUNT, 0)) return true;
			strInitLog += "  FSOUND_Init(11025, CHANNEL_COUNT, 0) failed." NEWLINE;

			//Maybe I'm asking for too many channels?  Try just enough for
			//sound effects, but not music.
			if (!bRes)
				FSOUND_SetMinHardwareChannels(SAMPLE_CHANNEL_COUNT);
			if (FSOUND_Init(44100, SAMPLE_CHANNEL_COUNT, 0))
			{
				//Not enough channels for sound effects.
				CFiles Files;
				Files.AppendErrorLog("Not enough available channels for sound effects." NEWLINE);
				this->bMusicAvailable = false;
				return true;
			}
			strInitLog += "  FSOUND_Init(44100, SAMPLE_CHANNEL_COUNT, 0) failed." NEWLINE;

			//I give up.
			break;
		}

		//Ready to play sound effects and probably music.
		return true;
#endif
	}

	//Sadly, there will be no sound.
	this->bMusicAvailable = this->bSoundEffectsAvailable = false;

	//Log the last SDL_mixer/FSOUND error which probably caused the failure.
	{
		GetLastSoundError(strInitLog);
		CFiles Files;
		Files.AppendErrorLog(strInitLog.c_str());
	}

	//Cleanup.
	DeinitSound();
	return false;
#endif //WITHOUT_SOUND
}

//***********************************************************************************
void CSound::DeinitSound()
//Deinits sound module.
{
#ifndef WITHOUT_SOUND
	StopSong();
#ifdef USE_SDL_MIXER
	Mix_CloseAudio();
	SDL_QuitSubSystem(SDL_INIT_AUDIO);
#else
	FSOUND_Close();
#endif
#endif
}

//**********************************************************************************
void CSound::UnloadSoundEffects()
//Unloads sound effects from array.
{
	for (UINT nSEI = 0; nSEI < CSound::SOUND_EFFECT_COUNT; ++nSEI)
	{
		if (this->SoundEffectArray[nSEI].IsLoaded())
			this->SoundEffectArray[nSEI].Unload();
	}
	CSound::PRIVATE_SOUND_CHANNEL_COUNT = 0;
}

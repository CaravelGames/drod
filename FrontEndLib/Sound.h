// $Id: Sound.h 10235 2012-06-05 13:17:04Z mrimer $

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

//Sound.h
//Declarations for CSound.
//Class for playing sounds and music.

#ifndef SOUND_H
#define SOUND_H

#ifdef WIN32
#pragma warning(disable:4786)
#endif
#include <BackEndLib/Assert.h>
#include <BackEndLib/StretchyBuffer.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>

#ifdef __sgi
#define USE_SDL_MIXER
#endif

#ifndef WITHOUT_SOUND
#ifdef USE_SDL_MIXER
#define USE_RWOPS  //Required to declare Mix_LoadMUS_RW
#include "SDL_mixer.h"
typedef Mix_Chunk SOUNDSAMPLE;
typedef Mix_Music SOUNDSTREAM;
#else
#include <fmod.h>
typedef FSOUND_SAMPLE SOUNDSAMPLE;
typedef FSOUND_STREAM SOUNDSTREAM;
#endif
#endif

#include <list>
#include <map>
#include <string>
#include <vector>
using namespace std;

//Music and sound effect IDs.
namespace SOUNDLIB
{
	enum SONGID
	{
		SONG_NONE = -1
	};

	enum SEID
	{
		SEID_ALL = -2,	//indicates all sound effects in some methods
		SEID_NONE = -1,
		SEID_BUTTON = 0,
		SEID_READ
	};
};

//Used to keep track of allocated data buffers playing on specific channels
//so these buffers can be freed when channel stops playing
typedef std::map<int,void*> CHANNELBUFFERMAP;

//Class for loading and playing samples for a sound effect.
class CSoundEffect
{
public:
	CSoundEffect();
	virtual ~CSoundEffect();

	int     GetChannel() const {return this->nChannel;}
	void    GetMinMaxDistance(float& fMin, float& fMax) const;
	bool    IsLoaded() const;
	bool	  Load(list<WSTRING> FilepathArray, const int nSetChannel,
			const bool b3DSound, const bool bSetPlayRandomSample=false,
			const bool bLoadOnPlay=true);
	bool    LoadFiles(list<WSTRING>& Filenames);
	bool    Play(const int nUseChannel, const float frequencyMultiplier=1.0);
	bool    SetMinMaxDistance(const float fMin, const float fMax);
	void    Unload();

#ifndef WITHOUT_SOUND
	static WSTRING GetPath();
	static WSTRING GetModPath();
#endif

protected:
#ifndef WITHOUT_SOUND
	SOUNDSAMPLE*            LoadSample(CStretchyBuffer& buffer, const bool b3DSound) const;
	virtual SOUNDSAMPLE*    LoadWave(const WCHAR *pwszFile, const bool b3DSound) const;

	list<SOUNDSAMPLE *>     Samples;

#ifdef USE_SDL_MIXER
	float fMinDist, fMaxDist;
#endif
#endif

	int     nChannel;
	bool    bPlayRandomSample;
	bool    bIsLoaded; //sounds are ready to play

	bool    b3DSound;
	bool    bLoadOnPlay; //wait until effect is played to load sound files
	list<WSTRING> FilesToLoad;

#ifndef WITHOUT_SOUND
	list<SOUNDSAMPLE *>::iterator iLastSamplePlayed;
#endif

	PREVENT_DEFAULT_COPY(CSoundEffect);
};

//Class used to perform all sound operations.
class CSound
{
public:
	CSound(const bool bNoSound, const UINT SOUND_EFFECT_COUNT,
			const UINT SAMPLE_CHANNEL_COUNT, const UINT SONG_CHANNEL_COUNT=2);
	virtual ~CSound();

	void        CrossFadeSong(const UINT eSongID, const UINT dwFadeDuration=3000);
	void        CrossFadeSong(list<WSTRING>* pSonglist, const UINT dwFadeDuration=3000);
	bool        PlayNextSong();
	bool        PlaySong(const UINT nSongID);
	bool        PlaySong(list<WSTRING>* pSonglist);
	void        PlaySoundEffect(const UINT eSEID, float* pos=NULL, float* vel=NULL,
			const bool bUseVoiceVolume=false, const float frequencyMultiplier=1.0f,
			const float fVolumeMultiplier = 1.0f);
	int         PlaySoundEffect(const CStretchyBuffer& sound, const bool bLoop=false,
			float* pos=NULL, float* vel=NULL, const bool bUseVoiceVolume=false);
	int         PlayVoice(const CStretchyBuffer& sound);
	void        Enable3DSound(const bool bSet3DSound);
	bool        EnableMusic(const bool bSetMusic);
	bool        EnableSoundEffects(const bool bSetSoundEffects);
	bool        EnableVoices(const bool bSet);
	void        FreeSoundDump();
	void        GetMinMaxDistance(const UINT eSEID, float& fMin, float& fMax) const;
	static UINT GetMemoryUsage();
	list<WSTRING>* GetSongListArray() const {return this->SongListArray;}
	virtual bool GetSongFilepaths(const UINT nSongID, list<WSTRING> &FilepathList)=0;
	virtual bool GetSoundFilenames(const UINT eSEID, list<WSTRING> &FilepathList) const=0;
	static WSTRING GetSongPath();
	static WSTRING GetSongModPath();
	UINT        GetSoundIDPlayingOnChannel(const int nChannel) const;
	UINT        GetSoundLength(const int nChannel) const;
	UINT        GetSoundLength(SOUNDSAMPLE *pSample) const;
	UINT		GetSoundLength(const CStretchyBuffer& sound) const;
	int         GetMusicVolume() const {return this->nMusicVolume;}
	int         GetSoundVolume() const {return this->nSoundVolume;}
	UINT        GetNumInstancesPlaying(const UINT eSEID) const;
	int         GetVoiceVolume() const {return this->nVoiceVolume;}
	bool			 Is3DSound() const {return this->b3DSound;}
	bool        IsMusicOn() const {return this->bMusicOn;}
	bool        IsSoundEffectPlaying(const UINT eSEID) const;
	bool        IsSoundPlayingOnChannel(const int nChannel) const;
	bool        IsSoundEffectsOn() const {return this->bSoundEffectsOn;}
	virtual SOUNDSTREAM*     LoadSong(const WCHAR* /*pwszWaveFilepath*/,
			const UINT /*mode*/,	BYTE* &/*pRawData*/) {return NULL;}
	bool        IsSongListPlaying(list<WSTRING>* pSonglist) const;
	void        Mute(const bool bMute=true);
	virtual bool PlayData(const UINT /*dwDataID*/) {return false;}
	void        PauseMusic(const bool val=true);
	void        PauseSounds(const bool val=true);
	bool        SetMinMaxDistance(const UINT eSEID, const float fMin, const float fMax) const;
	void        SetChannelVolume(const int nChannel, const int volume);
	void        SetSoundEffectsVolume(const int volume);
	void        SetMusicVolume(const int volume);
	void        SetVoicesVolume(const int volume);
	bool        StartSongStream(SOUNDSTREAM *pStream, BYTE *pRawData);
	bool        StopSong(const int nChannelNo=SOUNDLIB::SONG_NONE);
	bool        StopSoundEffect(const UINT eSEID);
	bool        StopSoundOnChannel(const int nChannel);
	void        StopAllSoundEffects();
	void        Unmute() {Mute(false);}
	void        UnpauseMusic();
	void        UnpauseSounds();
	void        Update(float *fPos, float *fDir, float *fVel);
	void        Update(const int nChannel, float *fPos=NULL, float *fVel=NULL,
			const UINT eSEID=static_cast<UINT>(SOUNDLIB::SEID_NONE),
			const bool bUseVoiceVolume=false, const float fVolumeMultiplier=1.0f);
	void        UpdateMusic();
	bool			 VerifySound(const CStretchyBuffer &buffer) const;
	bool        WaitForSoundEffectsToStop(const UINT dwMaxWaitTime = 3000L) const;

	static UINT SOUND_EFFECT_COUNT, PRIVATE_SOUND_CHANNEL_COUNT,
			SAMPLE_CHANNEL_COUNT, SONG_CHANNEL_COUNT, CHANNEL_COUNT;
	static bool bNoFocusPlaysMusic;

	int         nDistanceNorm;
	float fMinDist, fMaxDist; //global default min/max

protected:
	friend class CSoundEffect;
#ifndef WITHOUT_SOUND
#ifdef USE_SDL_MIXER
	static void songFinishedCallback();
#else
	static signed char F_CALLBACKAPI endStreamCallback(FSOUND_STREAM* stream, void* buff, int len, void* param);
#endif
#endif
	void        DeinitSound();
#ifndef USE_SDL_MIXER
	int         GetFreeMusicChannel() const;
#endif
	UINT        GetFreeSharedChannel() const;
	static void          GetLastSoundError(string &strErrorDesc);
	virtual bool         LoadSoundEffects()=0;
	bool        InitSound();
	void        UnloadSoundEffects();

	bool        bNoSound;
	bool        bMusicOn;
	bool        bMusicAvailable;
	bool        bSoundEffectsOn;
	bool        bSoundEffectsAvailable;
	bool        bVoicesOn;
	bool        b3DSound;
	int         nCurrentPlayingSongChannel;   //current song playing on this channel
	int         nFadingSongChannel;           //previous song fading out on this channel
	UINT       dwFadeBegin, dwFadeDuration;  //time duration to fade out old song
	int            nSoundVolume, nMusicVolume, nVoiceVolume;
	float			f2DListenerPos[2], f2DListenerDir[2];	//2D listener (x,y) position/direction
#ifndef WITHOUT_SOUND
#ifdef USE_SDL_MIXER
	BYTE* allocedMusic;  //Allocated music buffer for SDL_mixer
	Mix_Music *pSongStream;
	vector<Mix_Chunk*> soundDump;  //buffers to be deleted whenever sound effects are halted
#else
	CHANNELBUFFERMAP allocedChannels;	//alloced music buffers to be freed when channel stops playing
	vector<FSOUND_SAMPLE*> soundDump;   //buffers to be deleted whenever sound effects are halted

	vector<FSOUND_STREAM*>   Streams;	//for streaming current song
#endif
	struct SongInfo_t {
#ifndef USE_SDL_MIXER
		volatile FSOUND_STREAM* stream;
		volatile int            nChannel;
#endif
		volatile bool           bHasEnded;
	};
	static volatile SongInfo_t SongInfo;
#endif

	CSoundEffect  *SoundEffectArray;
	list<WSTRING> *SongListArray;	//names of all song lists
	list<WSTRING> SongList;		//ptr to sequence of currently playing songlist

	UINT        *ChannelSoundEffects;

	PREVENT_DEFAULT_COPY(CSound);

private:
	SOUNDSTREAM* LoadSongStream(const WSTRING& wstrSongFilepath, UINT mode);

	bool        bFadeRequest;
};

//Define global pointer to the one and only CSound object.
#ifndef INCLUDED_FROM_SOUND_CPP
	extern CSound *g_pTheSound;
#endif

#endif //...#ifndef SOUND_H

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef FACEWIDGET_H
#define FACEWIDGET_H

#include "DrodWidget.h"
#include "DrodSound.h"
#include "../DRODLib/MonsterFactory.h"
#include "../DRODLib/Character.h"

static const UINT CX_FACE = 130;
static const UINT CY_FACE = 164;

//Moods the swordsman can be in that affect his facial expression.
enum MOOD
{
	Mood_Normal = 0,
	Mood_Aggressive,
	Mood_Nervous,
	Mood_Strike,
	Mood_Happy,
	Mood_Dying,
	Mood_Talking,

	Mood_Count
};

//Face frame enum--corresponds to order within bitmap.
enum FACE_FRAME
{
	FF_Normal = 0,
	FF_Normal_Blinking,
	FF_Striking,
	FF_HalphNormal,
	FF_HalphAggressive,
	FF_HalphHappy,
	FF_HalphNervous,
	FF_84thNegotiator,

	FF_Aggressive,
	FF_Aggressive_Blinking,
	FF_Dying_1,
	FF_Slayer,
	FF_Goblin,
	FF_RockGolem,
	FF_RockGiant,
	FF_TarTech,

	FF_Nervous,
	FF_Nervous_Blinking,
	FF_Dying_2,
	FF_Citizen1,
	FF_Citizen2,
	FF_Citizen3,
	FF_Citizen4,
	FF_MudCoord,

	FF_Happy,
	FF_Happy_Blinking,
	FF_Dying_3,
	FF_Guard,
	FF_Stalwart,
	FF_Aumtlich,
	FF_Citizen,
	FF_GoblinKing,

	FF_Talking,
	FF_Talking_Blinking,
	FF_dontuse_eyes,
	FF_Brain,
	FF_39thSlayer,
	FF_Soldier,
	FF_None,
	FF_dontuse_g_eyes,

	FF_GunthroNormal,
	FF_GNormal_Blinking,
	FF_GStriking,
	FF_GAggressive,
	FF_GAggressive_Blinking,
	FF_GDying_1,
	FF_GDying_2,
	FF_GDying_3,

	FF_GNervous,
	FF_GNervous_Blinking,
	FF_GHappy,
	FF_GHappy_Blinking,
	FF_GTalking,
	FF_GTalking_Blinking,
	FF_Engineer,
	FF_Construct,

	FF_Serpent,
	FF_SerpentG,
	FF_SerpentB,
	FF_Eye,
	FF_EyeActive,
	FF_RoachEgg,
	FF_TarMother,
	FF_TarBaby,

	FF_MudMother,
	FF_MudBaby,
	FF_GelMother,
	FF_GelBaby,
	FF_Seep,
	FF_Spider,
	FF_WaterSkipper,
	FF_SkipperNest,

	FF_Fegundo,
	FF_FegundoAshes,
	FF_Mimic,
	FF_Decoy,
	FF_Wubba,
	FF_QRoach,
	FF_Roach,
	FF_WWing,

	FF_Default
};

typedef std::map<UINT, SDL_Surface*> SurfaceMap;

//******************************************************************************
class CFaceWidget : public CWidget
{
public:
	CFaceWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
			const UINT wSetW, const UINT wSetH);
	~CFaceWidget();

	void           PaintFull(const bool bVal=true) {this->bAlwaysPaintFull = bVal;}
	inline SPEAKER GetCharacter() const {return this->eSpeaker;}
	inline UINT   GetImageID() const {return this->dwImageID;}
	inline bool    IsMoodLocked() const {return this->bMoodLocked;}
	bool           IsSpeakerAnimated() const;
	void           MovePupils(const UINT wLookAtX=(UINT)-1, const UINT wLookAtY=(UINT)-1);
	virtual void      Paint(bool bUpdateRect = true);
	virtual void      PaintClipped(const int nX, const int nY, const UINT wW,
			const UINT wH, const bool bUpdateRect = true);
	void           ResetForPaint() {this->bMoodDrawn = false;}
	FACE_FRAME     ResolveFace();
	void           SetCharacter(const SPEAKER eSpeaker, const bool bLockMood);
	void           SetImage(const UINT dwDataID);
	void           SetMood(const MOOD eSetMood, const Uint32 lDelay=0,
		const bool bOverrideLock=false);
	void           SetMoodToSoundEffect(MOOD eSetMood, SEID eUntilSEID);
	void           SetReading(const bool bReading);
	void           SetIsDeathAnimation(const bool bIsDeathAnimation);
	void           SetSleeping();

struct POINT
{
	 long  x;
	 long  y;
};

protected:
	virtual void   HandleAnimate();
	virtual void   HandleMouseUp(const SDL_MouseButtonEvent &Button);
	virtual bool   IsAnimated() const {return true;}

private:
	void           DrawPupils();
	inline void       DrawPupils_DrawOnePupil(
			SDL_Surface *pDestSurface, const int nDestX, const int nDestY,
			const int nMaskX, const int nMaskY) const;
	bool           bHasMoodDelayPassed() const {return SDL_GetTicks()-this->dwStartDelayMood > this->dwDelayMood;}
	void           SetMoodDelay(const Uint32 dwDelay);

	SDL_Rect*      getEyeMask() const;
	const POINT*   getEyeMaskOffset() const;
	UINT           getBetweenPupils() const;

	SPEAKER        eSpeaker;         //character face being displayed
	MOOD           eMood, ePrevMood; //face's mood
	Uint32         dwDelayMood, dwStartDelayMood;        //how long to show face (0 means indefinitely)
	bool           bMoodDrawn, bIsReading, bIsBlinking, bIsSleeping;  //whether face is reading, etc.
	bool           bDoBlink;      //when set, do a blink at next possible time
	SEID           eMoodSEID;
	bool           bMoodLocked;   //can't change mood w/o explicit flag
	bool           bIsDeathAnimation; // Whether death animation is playing

	//Eye movement.
	int               nPupilX, nPupilY;
	int               nPupilTargetX, nPupilTargetY;

	Uint32			dwLastFrame;
	bool           bAlwaysPaintFull;

	SDL_Surface *pImage;   //custom image currently being shown
	UINT dwImageID;       //ID of image being shown
	SurfaceMap faceImages; //loaded custom images
};

#endif //#ifndef FACEWIDGET_H

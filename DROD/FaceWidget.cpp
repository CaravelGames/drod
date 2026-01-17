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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "FaceWidget.h"
#include "DrodBitmapManager.h"
#include "DrodSound.h"

#include "../DRODLib/GameConstants.h"
#include <BackEndLib/Assert.h>

//Face coords and dimensions.
static const UINT CY_FACE_PAD = 1;
static const UINT CX_FACE_PAD = 1;
static const UINT FACES_IN_ROW = 8;
#define yFace(ff) ( CY_FACE_PAD + ((ff) / FACES_IN_ROW) * (CY_FACE + CY_FACE_PAD) )
#define xFace(ff) ( CX_FACE_PAD + ((ff) % FACES_IN_ROW) * (CX_FACE + CX_FACE_PAD) )

//Locations of eye masks in the faces bitmap.  An eye mask defines the area a
//pupil can move within and the parts of that area in which the pupil's pixel
//will be visible.
static SDL_Rect m_EyeMaskRectArray[Mood_Count] =
{
	{266, 671, 55, 19},  //Normal
	{266, 691, 55, 19},  //Aggressive
	{266, 711, 55, 19},  //Nervous
	{266, 771, 55, 19},  //Strike
	{266, 731, 55, 19},  //Happy
	{266, 791, 55, 16},  //Dying
	{266, 751, 55, 19}   //Talking
};

static SDL_Rect m_GunthroEyeMaskRectArray[Mood_Count] =
{
	{921, 671, 44, 14},  //Normal
	{921, 691, 44, 14},  //Aggressive
	{921, 711, 44, 14},  //Nervous
	{921, 771, 44, 14},  //Strike
	{921, 731, 44, 14},  //Happy
	{921, 791, 44, 14},  //Dying
	{921, 751, 44, 14}   //Talking
};

//Offset from the top-left of the face widget to point where the top-left of the eye
//mask corresponds.
static const CFaceWidget::POINT m_EyeMaskOffsetArray[Mood_Count] =
{
	{46, 35},           //Normal
	{46, 35},           //Aggressive
	{46, 35},           //Nervous
	{46, 35},           //Strike
	{46, 35},           //Happy
	{46, 35},           //Dying
	{46, 35}            //Talking
};

static const CFaceWidget::POINT m_GunthroEyeMaskOffsetArray[Mood_Count] =
{
	{63, 61},           //Normal
	{63, 61},           //Aggressive
	{63, 61},           //Nervous
	{61, 61},           //Strike
	{62, 61},           //Happy
	{63, 61},           //Dying
	{63, 61}            //Talking
};

//Offset from the top-left of the eye mask to the center of the left pupil.
//Right pupil offset can be found from this by adding CX_BETWEEN_PUPILS.
static const CFaceWidget::POINT m_LeftPupilOffsetArray[Mood_Count] =
{
	{9, 11}, //Normal
	{9, 11}, //Aggressive
	{9, 11}, //Nervous
	{9, 11}, //Strike
	{9, 11}, //Happy
	{9, 11}, //Dying
	{9, 11}  //Talking
};

//Pupil-related constants.
static const UINT CX_BETWEEN_PUPILS = 36;

static const UINT CX_BETWEEN_PUPILS_GUNTHRO = 24;
static const UINT CX_BETWEEN_PUPILS_GUNTHRO_AGG = 27;

static const int X_PUPIL = 266;
static const int Y_PUPIL = 662;

static const int X_PUPIL_GUNTHRO = 921;

static const UINT CX_PUPIL = 6;
static const UINT CY_PUPIL = 6;
static const UINT CX_PUPIL_HALF = (CX_PUPIL / 2);
static const UINT CY_PUPIL_HALF = (CY_PUPIL / 2);

static const UINT CY_GUNTHRO_PUPIL = 5;

//
//Public methods.
//

//******************************************************************************
CFaceWidget::CFaceWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget
	const int nSetX, const int nSetY,               //    constructor.
	const UINT wSetW, const UINT wSetH)             //
	: CWidget((WIDGETTYPE)WT_Face, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, nPupilX(0), nPupilY(0), nPupilTargetX(0), nPupilTargetY(0)
	, dwLastFrame(0), bAlwaysPaintFull(false)
	, pImage(NULL)
	, facePlayer(Face(PlayerRole))
	, faceSpeaker(Face(Speaker))
	, faceDying(Face(Death))
{
	this->imageFilenames.push_back(string("Faces"));

	this->facePlayer.bIsActive = true;
}

//****************************************************************************
CFaceWidget::~CFaceWidget()
{
	for (SurfaceMap::const_iterator surface = this->faceImages.begin();
			surface != this->faceImages.end(); ++surface)
		SDL_FreeSurface(surface->second);
}

//****************************************************************************
SDL_Rect* CFaceWidget::getEyeMask() {
	const Face* face = GetActiveFace();
	return face->eCharacter == Speaker_Gunthro ? m_GunthroEyeMaskRectArray + face->eMood : m_EyeMaskRectArray + face->eMood;
}

const CFaceWidget::POINT* CFaceWidget::getEyeMaskOffset() {
	const Face* face = GetActiveFace();
	return face->eCharacter == Speaker_Gunthro ? m_GunthroEyeMaskOffsetArray + face->eMood : m_EyeMaskOffsetArray + face->eMood;
}

UINT CFaceWidget::getBetweenPupils() {
	const Face* face = GetActiveFace();
	if (face->eCharacter != Speaker_Gunthro)
		return CX_BETWEEN_PUPILS;

	switch (face->eMood) {
		case Mood_Aggressive:
		case Mood_Strike:
			return CX_BETWEEN_PUPILS_GUNTHRO_AGG;
		default: return CX_BETWEEN_PUPILS_GUNTHRO;
	}
}

Face* CFaceWidget::GetFace(const FaceWidgetLayer layer) {
	switch (layer) {
		case Speaker: return &faceSpeaker;
		case Death: return &faceDying;
		case PlayerRole:
		default:
			return &facePlayer;
	}
}

const FaceWidgetLayer CFaceWidget::GetActiveLayer() const{
	if (faceSpeaker.bIsActive)
		return faceSpeaker.eLayer;

	if (faceDying.bIsActive)
		return faceDying.eLayer;
	
	return facePlayer.eLayer;
}

Face* CFaceWidget::GetActiveFace() {
	return GetFace(GetActiveLayer());
}

//****************************************************************************
bool CFaceWidget::IsSpeakerAnimated(const Face* face) const
//Returns: whether current character face is animated
{
	const SPEAKER eSpeaker = face->eCharacter;
	return (eSpeaker == Speaker_Beethro || eSpeaker == Speaker_Gunthro ||
			eSpeaker == Speaker_Clone || eSpeaker == Speaker_BeethroInDisguise) && !face->dwImageID;
}

//****************************************************************************
void CFaceWidget::SetCharacter(
//Sets the character face shown.
//
//Params:
    const FaceWidgetLayer eLayer, //(in) layer on which the character is set
	const SPEAKER eSpeaker,    //(in) speaker to display
	const HoldCharacter* pHoldCharacter) //(in) hold character if speaker is a custom character
{
	ASSERT(eSpeaker < Speaker_Count);
	Face* face = GetFace(eLayer);
	ASSERT(face);
	face->eCharacter = eSpeaker;
	face->pHoldCharacter = pHoldCharacter;
	face->dwImageID = pHoldCharacter
		? pHoldCharacter->dwDataID_Avatar
		: 0;
	face->bIsDrawn = false;

	if (face->dwImageID) {
		const UINT dwDataID = face->dwImageID;
		SDL_Surface* pSurface;
		SurfaceMap::iterator id = this->faceImages.find(dwDataID);
		if (id != this->faceImages.end())
			pSurface = id->second;
		else
		{
			//No -- load image into face widget.
			pSurface = g_pTheDBM->LoadImageSurface(dwDataID);
			if (pSurface)
				this->faceImages[dwDataID] = pSurface;
		}
	}
}

//****************************************************************************
void CFaceWidget::SetMood(
//Sets the mood state of the face.
//
//Params:
	const FaceWidgetLayer eLayer, //(in) Layer to modify
	const MOOD eSetMood,       //(in) Mood to show
	const Uint32 lDelay)       //(in) Amount of time in msecs to pass before mood reverts to previous. [default=0 ms]
{
	Face* face = GetFace(eLayer);

	const bool bHasTemporaryMood = face->eMoodSEID != SEID_NONE || face->dwMoodUntil > 0;

	// No temporary mood and the new mood is the same to the old? No need to do anything
	if (!bHasTemporaryMood && eSetMood == face->eMood)
		return;

	if (lDelay) {
		if (face->eMoodSEID != SEID_NONE) // Sound-based temporary mood takes priority, do nothing
			return;

		face->dwMoodUntil = SDL_GetTicks() + lDelay;
	}

	if (!bHasTemporaryMood)
		face->ePrevMood = face->eMood;

	face->eMood = eSetMood;
	face->bIsDrawn = false;
	face->bIsSleeping = false;
}

//*****************************************************************************
void CFaceWidget::SetMoodToSoundEffect(
//Sets mood state of the face.  Mood will last until a sound effect finishes playing.
//
//Params:
	const FaceWidgetLayer eLayer, //(in) Layer to modify
	MOOD eSetMood,    //(in)   Mood to show.
	SEID eUntilSEID)  //(in)   Sound effect to show mood during.
{
	ASSERT(eUntilSEID > SEID_NONE && eUntilSEID < SEID_COUNT);

	Face* face = GetFace(eLayer);

	const bool bHasTemporaryMood = face->eMoodSEID != SEID_NONE || face->dwMoodUntil > 0;
	face->eMoodSEID = eUntilSEID;

	// No temporary mood and the new mood is the same to the old? No need to do anything
	if (!bHasTemporaryMood && eSetMood == face->eMood)
		return;

	if (!bHasTemporaryMood)
		face->ePrevMood = face->eMood;

	face->eMood = eSetMood;
	face->bIsDrawn = false;
	face->bIsSleeping = false;
}

//****************************************************************************
void CFaceWidget::SetDying(const bool bIsDying, MOOD eDyingMood)
{
	// Faces with less priority must redraw when this face ever ends
	facePlayer.bIsDrawn = false;
	faceSpeaker.bIsDrawn = false;

	faceDying.bIsActive = bIsDying;
	faceDying.pHoldCharacter = NULL;

	if (bIsDying) {
		faceDying.eMood = eDyingMood;
		faceDying.eCharacter = facePlayer.eCharacter;
		faceDying.dwImageID = facePlayer.dwImageID;
		faceDying.bIsDrawn = false;
	}
}

//****************************************************************************
void CFaceWidget::SetSpeaker(
	const bool bIsSpeaking,
	const SPEAKER eSpeaker,
	const HoldCharacter* pHoldCharacter,
	MOOD eSpeakingMood)
{
	// Faces with less priority must redraw when this face ever ends
	facePlayer.bIsDrawn = false;

	faceSpeaker.bIsActive = bIsSpeaking;
	faceSpeaker.pHoldCharacter = NULL;

	if (bIsSpeaking) {
		SetCharacter(Speaker, eSpeaker, pHoldCharacter);
		SetMood(Speaker, eSpeakingMood);
	}
}

//****************************************************************************
void CFaceWidget::SetReading(const bool bReading)
{
	facePlayer.bIsReading = bReading;
}

//****************************************************************************
void CFaceWidget::SetSleeping(const bool bIsSleeping)
{
	facePlayer.bIsSleeping = bIsSleeping;
	facePlayer.bIsBlinking = bIsSleeping;
	facePlayer.bIsDrawn = false;
}

//****************************************************************************
void CFaceWidget::DrawPupils()
//Draws the pupils.
{
	SDL_Surface *pDestSurface = LockDestSurface();

	const Face* face = GetActiveFace();
	//Draw left pupil.
	const UINT CX_LEFT_PUPIL_OFFSET =
		m_LeftPupilOffsetArray[face->eMood].x - CX_PUPIL_HALF + this->nPupilX;
	const UINT CY_PUPIL_OFFSET =
		m_LeftPupilOffsetArray[face->eMood].y - CY_PUPIL_HALF + this->nPupilY;

	DrawPupils_DrawOnePupil(pDestSurface,

		//Destination coords at which to draw left pupil.
		this->x + getEyeMaskOffset()->x + CX_LEFT_PUPIL_OFFSET,
		this->y + getEyeMaskOffset()->y + CY_PUPIL_OFFSET,

		//Coords to area of eye mask that corresponds to dest coords.
		getEyeMask()->x + CX_LEFT_PUPIL_OFFSET,
		getEyeMask()->y + CY_PUPIL_OFFSET
	);

	//Draw right pupil.
	const UINT CX_RIGHT_PUPIL_OFFSET = CX_LEFT_PUPIL_OFFSET + getBetweenPupils();

	DrawPupils_DrawOnePupil(pDestSurface,

		//Destination coords at which to draw right pupil.
		this->x + getEyeMaskOffset()->x + CX_RIGHT_PUPIL_OFFSET,
		this->y + getEyeMaskOffset()->y + CY_PUPIL_OFFSET,

		//Coords to area of eye mask that corresponds to dest coords.
		getEyeMask()->x + CX_RIGHT_PUPIL_OFFSET,
		getEyeMask()->y + CY_PUPIL_OFFSET
	);

	UnlockDestSurface();
}

//****************************************************************************
void CFaceWidget::DrawPupils_DrawOnePupil(
//Draws one pupil to surface.  Should only be called by DrawPupils().
//
//Params:
	SDL_Surface *pDestSurface,       //(in)   Already-locked surface.
	const int nDestX, const int nDestY, //(in)   Dest coords.
	const int nMaskX, const int nMaskY) //(in)   Mask coords.
{
	//I am going to copy pixels from a source surface to a dest surface.
	//The source surface contains the pupil image.  A mask surface contains
	//pixels that are either black (0,0,0) or white (255,255,255).  If a
	//mask pixel is black, then I don't copy that pixel from source to dest.
	//Masked areas are basically the corners of the eye.

	//Dest coords should draw pupil entirely inside of the face widget area.
	ASSERT(nDestX >= this->x && nDestX + CX_PUPIL < this->x + this->w);
	ASSERT(nDestY >= this->y && nDestY + CY_PUPIL < this->y + this->h);

	//Mask coords should be inside of faces surface.
	SDL_Surface *pSrcSurface = this->images[0];
	ASSERT(pSrcSurface);
	ASSERT(nMaskX >= 0 && nMaskX + CX_PUPIL < static_cast<UINT>(pSrcSurface->w));
	ASSERT(nMaskY >= 0 && nMaskY + CY_PUPIL < static_cast<UINT>(pSrcSurface->h));

	//For speed, this routine is hardcoded to a 6 x 6 pupil and 3/4 BPP surfaces.
	ASSERT(CX_PUPIL == 6 && CY_PUPIL == 6);
	const UINT wSrcBPP = pSrcSurface->format->BytesPerPixel;
	const UINT wBPP = pDestSurface->format->BytesPerPixel;
	ASSERT(wSrcBPP >= 3);
	ASSERT(wBPP >= 3);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	ASSERT(pDestSurface->format->Rmask == 0xff0000);
	ASSERT(pDestSurface->format->Gmask == 0x00ff00);
	ASSERT(pDestSurface->format->Bmask == 0x0000ff);
#endif

	const Face* face = GetActiveFace();
	const bool bGunthro = face->eCharacter == Speaker_Gunthro;
	const int xPupil = bGunthro ? X_PUPIL_GUNTHRO : X_PUPIL;
	const UINT cyPupil = bGunthro ? CY_GUNTHRO_PUPIL : CY_PUPIL;

	//Set pixel pointers to starting locations.
	Uint8 *pMaskPixel = static_cast<Uint8 *>(pSrcSurface->pixels) +
			nMaskY * pSrcSurface->pitch + nMaskX * wSrcBPP + PIXEL_FUDGE_FACTOR;
	Uint8 *pSrcPixel = static_cast<Uint8 *>(pSrcSurface->pixels) +
			Y_PUPIL * pSrcSurface->pitch + xPupil * wSrcBPP + PIXEL_FUDGE_FACTOR;
	Uint8 *pDestPixel = static_cast<Uint8 *>(pDestSurface->pixels) +
			nDestY * pDestSurface->pitch + nDestX * wBPP + PIXEL_FUDGE_FACTOR;

	//These are used to advance pointers when they have reached end of rows.
	const UINT FACES_ROW_OFFSET = pSrcSurface->pitch - ((CX_PUPIL - 1) * wSrcBPP);
	const UINT DEST_ROW_OFFSET = pDestSurface->pitch - ((CX_PUPIL - 1) * wBPP);

	//Macro to copy one pixel if not masked and advance pointers to
	//next pixel in row.
#  define COPYPIXEL \
		if (*pMaskPixel) \
		{ \
			pDestPixel[0] = pSrcPixel[0]; \
			pDestPixel[1] = pSrcPixel[1]; \
			pDestPixel[2] = pSrcPixel[2]; \
		} \
		pSrcPixel += wSrcBPP; \
		pMaskPixel += wSrcBPP; \
		pDestPixel += wBPP;

	//Macro to copy last pixel in row if not masked and advance pointers to
	//first pixel in next row.
#  define COPYLASTPIXEL \
		if (*pMaskPixel) \
		{ \
			pDestPixel[0] = pSrcPixel[0]; \
			pDestPixel[1] = pSrcPixel[1]; \
			pDestPixel[2] = pSrcPixel[2]; \
		} \
		pSrcPixel += FACES_ROW_OFFSET; \
		pMaskPixel += FACES_ROW_OFFSET; \
		pDestPixel += DEST_ROW_OFFSET;

	//All the pixel copying happens here.  For performance, I'm using minimal loops.
	for (UINT i=cyPupil; i--; )
	{
		COPYPIXEL   COPYPIXEL   COPYPIXEL   COPYPIXEL   COPYPIXEL   COPYLASTPIXEL
	}

#  undef COPYPIXEL
#  undef COPYLASTPIXEL
}

//****************************************************************************
void CFaceWidget::MovePupils(
//Moves pupils as appropriate for the current state of the face.
//
//Params:
	const UINT wLookAtX, const UINT wLookAtY) //optional screen position to look at [default=(-1,-1)]
{
	const bool is_gunthro = GetActiveFace()->eCharacter == Speaker_Gunthro;

	Face* face = GetActiveFace();
	int xRightBound = CX_PUPIL - 1;
	int xLeftBound = -xRightBound;
	int yBottomBound = (getEyeMask()->h - CY_PUPIL - CY_PUPIL) / 2;
	int yTopBound = -yBottomBound;
	if (is_gunthro) {
		xRightBound -= 1;
		xLeftBound -= 1;
		yTopBound -= 3;
		if (face->eMood == Mood_Aggressive)
			yBottomBound -= 1;
	} else {
		if (face->eMood == Mood_Nervous)
			yTopBound += 2; //Nervous mood has big eye corners that pupils can disappear into.
		if (face->eMood == Mood_Happy)
			yTopBound += 1; //Happy mood--same thing, but corners are a little smaller.
	}

	ASSERT(xRightBound >= xLeftBound);
	ASSERT(yBottomBound >= yTopBound);

	//A specific spot to look at was specified.
	bool bTarget = false;
	if (wLookAtX != (UINT)-1 && wLookAtY != (UINT)-1)
	{
		this->nPupilX = (int)wLookAtX - (this->x + getEyeMaskOffset()->x + getBetweenPupils()/2);
		this->nPupilY = (int)wLookAtY - (this->y + getEyeMaskOffset()->y + CY_PUPIL/2);
		bTarget = true;
	}

	if (face->bIsReading)
	{
		//Scroll eyes across as if reading.
		if (--(this->nPupilX) < xLeftBound)
			this->nPupilX = xRightBound;
		this->nPupilY = yBottomBound;

		if (is_gunthro)
			this->nPupilY -= 2;
	}
	else
	{
		switch (face->eMood)
		{
			case Mood_Dying:
			case Mood_Strike:
				//Keep pupils fixed in center.
				this->nPupilX = this->nPupilY = 0;
			break;

			default:
			{
				//Figure out the relaxation level.  Higher value means
				//that the pupils are less likely to move around.
				int nRelaxationLevel;
				switch (face->eMood)
				{
					case Mood_Happy: case Mood_Talking: nRelaxationLevel = 6; break;
					case Mood_Aggressive: case Mood_Nervous: nRelaxationLevel = 0; break;
					default: nRelaxationLevel = 4; break;
				}
				if (bTarget)
					nRelaxationLevel = 999; //don't move for now

				//Bounds check the target.
				if (this->nPupilTargetX < xLeftBound)
					this->nPupilTargetX = xLeftBound;
				else if (this->nPupilTargetX > xRightBound)
					this->nPupilTargetX = xRightBound;
				if (this->nPupilTargetY < yTopBound)
					this->nPupilTargetY = yTopBound;
				else if (this->nPupilTargetY > yBottomBound)
					this->nPupilTargetY = yBottomBound;

				//Are pupils at their target?
				if (this->nPupilX == this->nPupilTargetX &&
						this->nPupilY == this->nPupilTargetY)  //Yes.
				{
					//After a random delay based on the relaxation level...
					if (!nRelaxationLevel || ((int) rand() % nRelaxationLevel) == 0)
					{
						//...pick a new target.
						this->nPupilTargetX = (((int)
								rand() % (xRightBound - xLeftBound + 1)) + xLeftBound);
						this->nPupilTargetY = (((int)
								rand() % (yBottomBound - yTopBound + 1)) + yTopBound);
					}
				}
				else                                //No.
				{  //Move pupils towards target.
					const int xDist = this->nPupilTargetX - this->nPupilX;
					//Pupils decelerate as they get closer.
					const int nXSpeed = 1 + (abs(xDist) > 2) + (abs(xDist) > 4);
					this->nPupilX += sgn(xDist) * nXSpeed;
					this->nPupilY += sgn(this->nPupilTargetY - this->nPupilY);
				}
			}
			break;
		}
	}

	//Bounds check the pupils.
	if (this->nPupilX < xLeftBound)
		this->nPupilX = xLeftBound;
	else if (this->nPupilX > xRightBound)
		this->nPupilX = xRightBound;
	if (this->nPupilY < yTopBound)
		this->nPupilY = yTopBound;
	else if (this->nPupilY > yBottomBound)
		this->nPupilY = yBottomBound;
}

//*****************************************************************************
void CFaceWidget::PaintClipped(
//
//Params:
	const int /*nX*/, const int /*nY*/, const UINT /*wW*/, const UINT /*wH*/,
	const bool /*bUpdateRect*/)
{
	//Paint() uses direct access to pixels, so it can't be clipped with
	//SDL_SetClipRect().  Either you need to write PaintClipped() or change
	//the situation which causes this widget to be clipped.
	ASSERT(!"Face widget should not be clipped.");
}

//*****************************************************************************
FACE_FRAME CFaceWidget::ResolveFaceFrame(Face *face)
{
	//Figure out what face frame to use.
	//Select src image based on this->eSpeaker.
	ASSERT(face);

	FACE_FRAME eFrame = FF_Normal;
	if (face->dwImageID)
		return eFrame;

	switch (face->eCharacter)
	{
		case Speaker_Clone:
		case Speaker_Beethro:
		case Speaker_BeethroInDisguise:
		switch (face->eMood)
		{
			case Mood_Normal:
				eFrame = face->bIsBlinking ? FF_Normal_Blinking : FF_Normal;
			break;
			case Mood_Aggressive:
				eFrame = face->bIsBlinking ? FF_Aggressive_Blinking : FF_Aggressive;
			break;
			case Mood_Nervous:
				eFrame = face->bIsBlinking ? FF_Nervous_Blinking : FF_Nervous;
			break;
			case Mood_Happy:
				eFrame = face->bIsBlinking ? FF_Happy_Blinking : FF_Happy;
			break;
			case Mood_Talking:
				eFrame = face->bIsBlinking ? FF_Talking_Blinking : FF_Talking;
			break;
			case Mood_Strike:
				eFrame = FF_Striking;
			break;
			case Mood_Dying:
			{
				//Update face in real-time.
				static UINT wDyingAnimFrame = 0;
				static const Uint32 dwDyingFrameLength = 50;	//ms
				const Uint32 dwNow = SDL_GetTicks();
				if (dwNow - this->dwLastFrame > dwDyingFrameLength)
				{
					++wDyingAnimFrame;
					this->dwLastFrame = dwNow;
				}
				switch (wDyingAnimFrame % 4)
				{
					case 0:         eFrame = FF_Dying_1; break;
					case 1: case 3: eFrame = FF_Dying_2; break;
					case 2:         eFrame = FF_Dying_3; break;
				}
			}
			break;
			default: ASSERT(!"Bad mood for Beethro's face."); break;
		}
		break;

		case Speaker_Gunthro:
		switch (face->eMood)
		{
			case Mood_Normal:
				eFrame = face->bIsBlinking ? FF_GNormal_Blinking : FF_GunthroNormal;
			break;
			case Mood_Aggressive:
				eFrame = face->bIsBlinking ? FF_GAggressive_Blinking : FF_GAggressive;
			break;
			case Mood_Nervous:
				eFrame = face->bIsBlinking ? FF_GNervous_Blinking : FF_GNervous;
			break;
			case Mood_Happy:
				eFrame = face->bIsBlinking ? FF_GHappy_Blinking : FF_GHappy;
			break;
			case Mood_Talking:
				eFrame = face->bIsBlinking ? FF_GTalking_Blinking : FF_GTalking;
			break;
			case Mood_Strike:
				eFrame = FF_GStriking;
			break;
			case Mood_Dying:
			{
				//Update face in real-time.
				static UINT wDyingAnimFrame = 0;
				static const Uint32 dwDyingFrameLength = 50;	//ms
				const Uint32 dwNow = SDL_GetTicks();
				if (dwNow - this->dwLastFrame > dwDyingFrameLength)
				{
					++wDyingAnimFrame;
					this->dwLastFrame = dwNow;
				}
				switch (wDyingAnimFrame % 4)
				{
					case 0:         eFrame = FF_GDying_1; break;
					case 1: case 3: eFrame = FF_GDying_2; break;
					case 2:         eFrame = FF_GDying_3; break;
				}
			}
			break;
			default: ASSERT(!"Bad mood for Gunthro's face."); break;
		}
		break;

		//Speakers with a few moods implemented.
		case Speaker_Halph:
		case Speaker_Halph2:
		switch (face->eMood)
		{
			case Mood_Aggressive: eFrame = FF_HalphAggressive; break;
			case Mood_Nervous: eFrame = FF_HalphNervous; break;
			case Mood_Happy: eFrame = FF_HalphHappy; break;
			default: eFrame = FF_HalphNormal; break;
		}
		break;

		//Speakers with a single mood implemented.
		case Speaker_MudCoordinator: eFrame = FF_MudCoord; break;
		case Speaker_Citizen1: eFrame = FF_Citizen1; break;
		case Speaker_TarTechnician: eFrame = FF_TarTech; break;
		case Speaker_Citizen2: eFrame = FF_Citizen2; break;
		case Speaker_Negotiator: eFrame = FF_84thNegotiator; break;
		case Speaker_Citizen3: eFrame = FF_Citizen3; break;
		case Speaker_Instructor:
		case Speaker_Citizen4: eFrame = FF_Citizen4; break;
		case Speaker_EyeActive: eFrame = FF_EyeActive; break;
		//Monster speakers.
		case Speaker_Architect: eFrame = FF_Engineer; break;
		case Speaker_Aumtlich: eFrame = FF_Aumtlich; break;
		case Speaker_Brain: eFrame = FF_Brain; break;
		case Speaker_Citizen: eFrame = FF_Citizen; break;
		case Speaker_Construct: eFrame = FF_Construct; break;
		case Speaker_Decoy: eFrame = FF_Decoy; break;
		case Speaker_Eye: eFrame = FF_Eye; break;
		case Speaker_Fegundo: eFrame = FF_Fegundo; break;
		case Speaker_FegundoAshes: eFrame = FF_FegundoAshes; break;
		case Speaker_GelBaby: eFrame = FF_GelBaby; break;
		case Speaker_GelMother: eFrame = FF_GelMother; break;
		case Speaker_GoblinKing: eFrame = FF_GoblinKing; break;
		case Speaker_Goblin: eFrame = FF_Goblin; break;
		case Speaker_Guard: eFrame = FF_Guard; break;
		case Speaker_Mimic: eFrame = FF_Mimic; break;
		case Speaker_MudBaby: eFrame = FF_MudBaby; break;
		case Speaker_MudMother: eFrame = FF_MudMother; break;
		case Speaker_QRoach: eFrame = FF_QRoach; break;
		case Speaker_Roach: eFrame = FF_Roach; break;
		case Speaker_RoachEgg: eFrame = FF_RoachEgg; break;
		case Speaker_RockGolem: eFrame = FF_RockGolem; break;
		case Speaker_RockGiant: eFrame = FF_RockGiant; break;
		case Speaker_Seep: eFrame = FF_Seep; break;
		case Speaker_Serpent: eFrame = FF_Serpent; break;
		case Speaker_Stalwart: eFrame = FF_Stalwart; break;
		case Speaker_Stalwart2: eFrame = FF_Soldier; break;
		case Speaker_TarBaby: eFrame = FF_TarBaby; break;
		case Speaker_TarMother: eFrame = FF_TarMother; break;
		case Speaker_SerpentG: eFrame = FF_SerpentG; break;
		case Speaker_SerpentB: eFrame = FF_SerpentB; break;
		case Speaker_Slayer: eFrame = FF_39thSlayer; break;
		case Speaker_Slayer2: eFrame = FF_Slayer; break;
		case Speaker_Spider: eFrame = FF_Spider; break;
		case Speaker_WaterSkipper: eFrame = FF_WaterSkipper; break;
		case Speaker_WaterSkipperNest: eFrame = FF_SkipperNest; break;
		case Speaker_Wubba: eFrame = FF_Wubba; break;
		case Speaker_WWing: eFrame = FF_WWing; break;

		case Speaker_None:
		default:
			eFrame = FF_None;
		break;
	}
	if (eFrame == FF_None)
		eFrame = FF_Default;

	//!!HACK for empty face images in the current data.
	//If portraits are added for these types, then remove this.
	if (eFrame >= FF_Roach)
		eFrame = FF_Default;

	return eFrame;
}

//******************************************************************************
void CFaceWidget::Paint(
//Paint the face.  Take into account the mood and the time that's passed
//since the last paint to animate the face.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
								  //       surface is the screen, the screen
								  //       will be immediately updated in
								  //       the widget's rect.
{
	//Drawing code below needs to be modified to accept offsets.  Until then,
	//this widget can't be offset.
	ASSERT(!IsScrollOffset());

	//Make things easy and insist on standard width and height.
	ASSERT(this->w == CX_FACE && this->h == CY_FACE);

	PaintFace(GetActiveFace());

	if (bUpdateRect) UpdateRect();
}

//******************************************************************************
void CFaceWidget::PaintFace(
	//Paint the provided face
	//Params:
	Face* face
) {
	SDL_Surface* pDestSurface = GetDestSurface();
	SDL_Rect Dest = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);

	//Figure out what face frame to use.
	//Select src image based on this->eSpeaker.
	FACE_FRAME eFrame = ResolveFaceFrame(face);

	//Blit entire face frame if needed.
	const bool bDrawPupils = IsSpeakerAnimated(face) && !face->bIsBlinking;
	if (
		this->bAlwaysPaintFull
		|| !face->bIsDrawn
		|| face->eMood == Mood_Dying) // For simplicity always animate death
	{
		//Draw special image, if set.
		if (face->dwImageID)
		{
			SDL_Surface* faceImage = this->faceImages[face->dwImageID];
			int srcX = 0;

			if (faceImage->w >= this->w * (face->eMood + 1))
				srcX = this->w * face->eMood;

			SDL_Rect Src = MAKE_SDL_RECT(srcX, 0, this->w, this->h);
			SDL_BlitSurface(faceImage, &Src, pDestSurface, &Dest);
		}
		else {
			//Bounds check -- just revert to default if this face frame is not loaded.
			if (eFrame == FF_Default || yFace(eFrame) >= (UINT)this->images[0]->h)
			{
				if (face->eCharacter != Speaker_Beethro)
				{
					face->eCharacter = Speaker_Beethro;
					PaintFace(face);
				}
				return;
			}

			SDL_Rect Src = MAKE_SDL_RECT(xFace(eFrame), yFace(eFrame), this->w, this->h);
			SDL_BlitSurface(this->images[0], &Src, pDestSurface, &Dest);
		}

		face->bIsDrawn = true;
	}
	else if (bDrawPupils)
	{
		//The pupils need to be drawn but I did not blit the face frame.
		//So I will first blit the eye mask area to the dest surface on which
		//the pupils will be drawn below.  This is how the current pupils
		//are "erased".
		SDL_Rect EyeMaskSrc = { static_cast<Sint16>(xFace(eFrame) + getEyeMaskOffset()->x),
			static_cast<Sint16>(yFace(eFrame) + getEyeMaskOffset()->y),
			getEyeMask()->w, getEyeMask()->h };
		SDL_Rect EyeMaskDest = { static_cast<Sint16>(this->x + getEyeMaskOffset()->x),
			static_cast<Sint16>(this->y + getEyeMaskOffset()->y),
			EyeMaskSrc.w, EyeMaskSrc.h };
		SDL_BlitSurface(this->images[0], &EyeMaskSrc, pDestSurface, &EyeMaskDest);
	}

	//Draw the pupils if needed.
	if (bDrawPupils)
	{
		DrawPupils();
	}
}
//
//Protected methods.
//

//******************************************************************************
void CFaceWidget::HandleAnimate()
//Handle animation of the widget.
{
	if (!CBitmapManager::bGameHasFocus)
		return; // Beethro's face animating while everything else is static is really creepy

	const Face* face = GetActiveFace();
	const Uint32 dwNow = SDL_GetTicks();

	static const Uint32 dwFrameLength = 200;	//ms
	const bool bFrameEnded = dwNow - this->dwLastFrame > dwFrameLength;
	const bool bForceDraw = !GetActiveFace()->bIsDrawn;

	//Animate widget 
	//Animation frame rate is slower (probably) than screen animation rate.
	if (bForceDraw || bFrameEnded)
	{
		// Blinking should always take one animation frame to ensure it looks decent
		if (bFrameEnded && (rand() % 20) == 0) {
			this->bDoBlink = true;
		}

		HandleAnimateFace(&facePlayer);
		HandleAnimateFace(&faceSpeaker);
		HandleAnimateFace(&faceDying);
		MovePupils();

		faceDying.bIsBlinking = false; // Never blink on death

		if (bFrameEnded) {
			this->bDoBlink = false;
			this->dwLastFrame = dwNow;
		}

		RequestPaint(true);
	}
}

//******************************************************************************
void CFaceWidget::HandleAnimateFace(Face* face)
//Handle animation of the widget.
{
	const Uint32 dwNow = SDL_GetTicks();

	const bool bHasMoodTimedOut = face->dwMoodUntil > 0 && dwNow > face->dwMoodUntil;
	const bool bHasMoodSoundFinished = face->eMoodSEID != SEID_NONE && !g_pTheSound->IsSoundEffectPlaying(face->eMoodSEID);

	//After timing of a temporary face is done (i.e., happy for a moment after
	//a monster kill) go back to previous mood
	if (bHasMoodTimedOut || bHasMoodSoundFinished)
	{
		// Only redraw mood if the old mood was different from the temporary one
		if (face->eMood != face->ePrevMood)
			face->bIsDrawn = false;

		//Cleanup all timing data
		face->eMood = face->ePrevMood;
		face->eMoodSEID = SEID_NONE;
		face->dwMoodUntil = 0;
	}

	const bool bWasBlinking = face->bIsBlinking;
	if (!face->bIsSleeping)
	{
		if (face->bIsBlinking)
			face->bIsBlinking = false;
		else if (face->eMood == Mood_Strike || face->eMood == Mood_Dying)
			face->bIsBlinking = false; //No blinking in these moods.
		else
			face->bIsBlinking = this->bDoBlink;
	}

	if (bWasBlinking != face->bIsBlinking)
		face->bIsDrawn = false;
}

//*****************************************************************************
void CFaceWidget::HandleMouseUp(const SDL_MouseButtonEvent &Button)
{
	const long nLeft = this->x + getEyeMaskOffset()->x -
			(getEyeMask()->w - getBetweenPupils())/2;
	const long nTop = this->y + getEyeMaskOffset()->y;
	if (Button.y >= nTop && Button.y <= nTop + getEyeMask()->h &&
		 Button.x >= nLeft && Button.x <= nLeft + getEyeMask()->w)
	{
		//Clicking on eyes makes face blink.
		this->bDoBlink = true;
	} else {
		Face* face = GetActiveFace();
		//Briefly show a random mood.
		if (!face->bIsSleeping)
		{
			MOOD eMood;
			do {
				eMood = (MOOD)(rand()%Mood_Count);
			} while (eMood == face->eMood);
			SetMood(face->eLayer, eMood, 300);
			RequestPaint();
		}
	}
}

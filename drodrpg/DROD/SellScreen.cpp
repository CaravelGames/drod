// $Id: SellScreen.cpp 10126 2012-04-24 05:40:08Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C)
 * 2003, 2005 Caravel Software. All Rights Reserved.
 *
 * ***** END LICENSE BLOCK ***** */

#include "SellScreen.h"
#include "DrodSound.h"
#include "DrodBitmapManager.h"
#include <FrontEndLib/BitmapManager.h>
#include <FrontEndLib/ButtonWidget.h>
#include "../DRODLib/Db.h"
#include <BackEndLib/Browser.h>
#include <BackEndLib/Files.h>

// Widget tags.
const UINT TAG_BUY = 2000;
const UINT TAG_BACK = 2001;
const UINT TAG_GOTOFORUM = 2002;
const UINT TAG_EXIT = 2003;

#define IMAGE_BG      (0)
#define IMAGE_SSHOT   (1)

#define NUM_SSHOTS_TO_DISPLAY (3)

#define SDL_TRANSPARENT_COLOR 0,255,255

#define ANG_MOMENTUM (0.6f) //initial speed of rotation
#define GRAVITY (0.02f) //change in speed of rotation
#define MAX_DEGREE_DELTA (4.0f)

//******************************************************************************
CSellScreen::CSellScreen()
	: CDrodScreen(SCR_Sell)
	, lastUpdate(0)
{
	//Load this background screen.
	this->imageFilenames.push_back(string("Sell1"));
	this->imageFilenames.push_back(string("SellSShots"));

	static const int X_BUY_BUTTON = 748;
	static const int Y_BUY_BUTTON = 534;
	static const int X_FORUM_BUTTON = X_BUY_BUTTON;
	static const int Y_FORUM_BUTTON = 607;
	static const int X_BACK_BUTTON = X_BUY_BUTTON;
	static const int Y_BACK_BUTTON = 680;
	static const int X_EXIT_BUTTON = 884;
	static const int Y_EXIT_BUTTON = Y_BACK_BUTTON;
	static const UINT CX_BUY_BUTTON = 268;
	static const UINT CY_BUY_BUTTON = 70;
	static const UINT CX_BACK_BUTTON = 132;
	static const UINT CY_BACK_BUTTON = CY_BUY_BUTTON;
	static const UINT CX_EXIT_BUTTON = CX_BACK_BUTTON;
	static const UINT CY_EXIT_BUTTON = CY_BACK_BUTTON;

	CButtonWidget *pButton;

	//Buy button.
	pButton = new CButtonWidget(TAG_BUY,
			X_BUY_BUTTON, Y_BUY_BUTTON, CX_BUY_BUTTON, CY_BUY_BUTTON,
			g_pTheDB->GetMessageText(MID_BuyNow));
	AddWidget(pButton);

	//Caravel forum button.
	pButton = new CButtonWidget(TAG_GOTOFORUM,
			X_FORUM_BUTTON, Y_FORUM_BUTTON, CX_BUY_BUTTON, CY_BUY_BUTTON,
			g_pTheDB->GetMessageText(MID_GoToForum));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_BACK,
			X_BACK_BUTTON, Y_BACK_BUTTON, CX_BACK_BUTTON, CY_BACK_BUTTON,
			g_pTheDB->GetMessageText(MID_Back));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_EXIT,
			X_EXIT_BUTTON, Y_EXIT_BUTTON, CX_EXIT_BUTTON, CY_EXIT_BUTTON,
			g_pTheDB->GetMessageText(MID_Exit));
	AddWidget(pButton);
}

//*****************************************************************************
void CSellScreen::OnBetweenEvents()
//Called between frames.
{
	//Animate screen shots.
	MoveScreenshots(GetDestSurface(), true);
}

//******************************************************************************
void CSellScreen::OnClick(
//Handles click events.
//
//Params:
	const UINT dwTagNo)                      //(in) Widget receiving event.
{
	switch (dwTagNo)
	{
		case TAG_BUY:
			GoToBuyNow();
		break;

		case TAG_BACK:
			GoToScreen(SCR_Return);
		break;

		case TAG_EXIT:
			GoToScreen(SCR_None);
		break;

		case TAG_GOTOFORUM:
			GoToForum();
		break;
	}
}

//*****************************************************************************
bool CSellScreen::OnQuit()
//Called when SDL_QUIT event is received.
//Since this is the exit confirmation screen, any further exit events at this
//point should always exit the app.
{
	GoToScreen(SCR_None);
	return true;
}

//******************************************************************************
void CSellScreen::Paint(
//Paint the screen.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	SDL_Surface *pDestSurface = GetDestSurface();

	//Blit the background graphic.
	if (this->images.size() > IMAGE_BG)
		SDL_BlitSurface(this->images[IMAGE_BG], NULL, pDestSurface, NULL);

	MoveScreenshots(pDestSurface, false);

	PaintChildren();
	if (bUpdateRect) UpdateRect();
}

//******************************************************************************
bool CSellScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	g_pTheSound->PlaySong(SONGID_QUIT);

	//Select indices of screen shots to display.
	this->sshotIndex.clear();

	//How many screen shots are available?
	SDL_Surface *pSrcSurface = this->images[IMAGE_SSHOT];
	const UINT width = pSrcSurface->w;
	const UINT SCREENSHOT_HEIGHT = width; //same w&h
	const UINT NUM_SCREENSHOTS = pSrcSurface->h / SCREENSHOT_HEIGHT;

	CIDSet indicesChosen;
	for (UINT i=0; i<NUM_SSHOTS_TO_DISPLAY; ++i)
	{
		UINT index;
		do {
			index = RAND(NUM_SCREENSHOTS);
		} while (indicesChosen.has(index));

		SSHOT_INFO info = {index, 0.0f, 0.0f};
		info.delta = (i%2)==0 ? ANG_MOMENTUM : -ANG_MOMENTUM;
		this->sshotIndex.push_back(info);

		//Ensure each screen shot is displayed only once, if possible.
		if (NUM_SCREENSHOTS >= NUM_SSHOTS_TO_DISPLAY)
			indicesChosen += index;
	}

	//Set transparent tile colorkey on screen shot source surface.
	const Uint32 TranspColor = SDL_MapRGB(pSrcSurface->format,
			SDL_TRANSPARENT_COLOR);
	SDL_SetColorKey(pSrcSurface, SDL_SRCCOLORKEY, TranspColor);

	//Set frame rate as high as needed for smooth animations.
	SetBetweenEventsInterval(15); //60+ fps

	return true;
}

//*****************************************************************************
void CSellScreen::MoveScreenshots(SDL_Surface *pDestSurface, const bool bUpdateRect)
//Slide screen shots around right side of screen.
{
	if (this->images.size() <= IMAGE_SSHOT)
		return;

	//Screenshot dimensions.
	SDL_Surface *pSrcSurface = this->images[IMAGE_SSHOT];
	const UINT width = pSrcSurface->w;
	const UINT SCREENSHOT_HEIGHT = width; //same w&h

	//Area where this stuff is occurring.
	static const int sshotX[NUM_SSHOTS_TO_DISPLAY] = {25+160, 200+160, 35+160};
	static const int sshotY[NUM_SSHOTS_TO_DISPLAY] = {10+160, 235+160, 440+160};

	//Redraw background.
	static const SDL_Rect updateRect[2] = {
		MAKE_SDL_RECT(0, 0, 423, CScreen::CY_SCREEN),
		MAKE_SDL_RECT(423, 173, 587-413, 612-173) //a second small piece to speed things up a bit
	};
	UINT i;
	for (i=0; i<2; ++i)
	{
		SDL_Rect updateRectSrc = updateRect[i], updateRectDest = updateRect[i];
		SDL_BlitSurface(this->images[IMAGE_BG], &updateRectSrc, pDestSurface, &updateRectDest);
	}

	//Movement rate.
	const Uint32 dwNow = SDL_GetTicks();
	const Uint32 deltaTime = this->lastUpdate ? dwNow - this->lastUpdate : 0;
	float fDeltaDegrees = deltaTime / 15.0f; //change in degrees
	if (fDeltaDegrees > MAX_DEGREE_DELTA)
		fDeltaDegrees = MAX_DEGREE_DELTA;
	this->lastUpdate = dwNow;

	//Draw each screen shot.
	for (i=0; i<NUM_SSHOTS_TO_DISPLAY; ++i)
	{
		SSHOT_INFO& sshot = this->sshotIndex[i];
		const int index = (int)sshot.index;
		const int yPixel = index*SCREENSHOT_HEIGHT;

		const bool bPositive = sshot.rotation > 0.0f;

		//Update rotation angle.
		sshot.rotation += fDeltaDegrees * sshot.delta;

		//Keep momentum stable.
		if (bPositive != (sshot.rotation > 0.0f))
		{
			if (bPositive) //was positive, going negative
				sshot.delta = -ANG_MOMENTUM; //reset to initial value
			else
				sshot.delta = ANG_MOMENTUM; //reset to initial value
		}

		//Pendulum.
		if (sshot.rotation > 0.0f)
			sshot.delta -= fDeltaDegrees * GRAVITY;
		else
			sshot.delta += fDeltaDegrees * GRAVITY;

		//Draw image rotated to an angle.
		Uint8 *pSrcPixel = (Uint8*)pSrcSurface->pixels + yPixel * pSrcSurface->pitch;

		//Rotate.
		SDL_Surface *pRotatedSurface = g_pTheDBM->RotateSurface(pSrcSurface, pSrcPixel,
				width, SCREENSHOT_HEIGHT, sshot.rotation);
		if (pRotatedSurface)
		{
			SDL_Rect sshotDest = MAKE_SDL_RECT(sshotX[i] - pRotatedSurface->w/2,
				sshotY[i] - pRotatedSurface->h/2, pRotatedSurface->w, pRotatedSurface->h);
			SDL_BlitSurface(pRotatedSurface, NULL, pDestSurface, &sshotDest);

			SDL_FreeSurface(pRotatedSurface);
		} else {
			//Show unrotated images if something doesn't work for whatever reason.
			SDL_Rect sshotSrc = MAKE_SDL_RECT(0, yPixel, width, SCREENSHOT_HEIGHT);
			SDL_Rect sshotDest = MAKE_SDL_RECT(sshotX[i] - width/2,
				sshotY[i] - SCREENSHOT_HEIGHT/2, width, SCREENSHOT_HEIGHT);
			SDL_BlitSurface(pSrcSurface, &sshotSrc, pDestSurface, &sshotDest);
		}
	}

	//Update affected screen area.
	if (bUpdateRect)
	{
		for (i=0; i<2; ++i)
		{
			const SDL_Rect& rect = updateRect[i];
			SDL_UpdateRect(pDestSurface, rect.x, rect.y, rect.w, rect.h);
		}
	}
}

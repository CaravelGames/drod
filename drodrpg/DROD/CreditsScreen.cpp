// $Id: CreditsScreen.cpp 9213 2008-09-08 00:34:55Z mrimer $

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
 * 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "CreditsScreen.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"
#include "DrodScreenManager.h"
#include "DrodSound.h"
#include "FaceWidget.h"
#include <FrontEndLib/ImageWidget.h>
#include <FrontEndLib/ScrollingTextWidget.h>
#include "../DRODLib/Db.h"
#include "../Texts/MIDs.h"
#include <BackEndLib/Wchar.h>

float          CCreditsScreen::fScrollRateMultiplier = 1.0f;
UINT           CCreditsScreen::wNormalScrollRate = 40;

#define SetRate(r) this->pScrollingText->SetScrollRate((UINT)\
	(r * this->wNormalScrollRate))

const UINT CY_CREDITS = CScreen::CY_SCREEN;
const int Y_CREDITS = 0;
const int X_CREDITS = 160;
const UINT CX_CREDITS = CScreen::CX_SCREEN - X_CREDITS*2;

//
//Protected methods.
//

//************************************************************************************
CCreditsScreen::CCreditsScreen()
	: CDrodScreen(SCR_Credits)
	, pScrollingText(NULL)
//Constructor.
{
	this->imageFilenames.push_back(string("Credits"));

	this->pScrollingText = new CScrollingTextWidget(0L, X_CREDITS, Y_CREDITS, 
			CX_CREDITS, CY_CREDITS);
	AddWidget(this->pScrollingText);
	SetRate(CCreditsScreen::fScrollRateMultiplier);
}

//*****************************************************************************
void CCreditsScreen::Paint(
//Overridable method to paint the screen.  
//
//Params:
	bool bUpdateRect)          //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//Blit the background graphic.
	SDL_BlitSurface(this->images[0], NULL, GetDestSurface(), NULL);

	PaintChildren();
	
	if (bUpdateRect) UpdateRect();
}

//************************************************************************************
bool CCreditsScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	HideCursor();

	//Return to title screen when done.
	g_pTheSM->ClearReturnScreens();
	g_pTheSM->InsertReturnScreen(SCR_Title);

	g_pTheSound->PlaySong(SONGID_CREDITS);

	//Fix up scrolling text widget state in case screen was activated before.
	this->pScrollingText->SetBackground(this->images[0]);
	this->pScrollingText->ClearText();
	this->pScrollingText->ScrollAbsolute(0,0);
	this->pScrollingText->Show();

	//For face image display preparation.
	CFaceWidget *pFaceWidget = new CFaceWidget(0, 0, 0, CX_FACE, CY_FACE);
	AddWidget(pFaceWidget, true);

//Add some text to the scrolling text widget.
#  define A_TEXT(mid) \
		this->pScrollingText->AddText(g_pTheDB->GetMessageText(mid), F_CreditsText);\
		this->pScrollingText->AddText(wszCRLF, F_CreditsText)

//Add a contributor header.
#	define A_CONTRIBUTOR(midName, midC) \
		this->pScrollingText->AddText(g_pTheDB->GetMessageText(midC), F_CreditsHeader, CLabelWidget::TA_CenterGroup); \
		this->pScrollingText->AddText(g_pTheDB->GetMessageText(midName), F_CreditsSubheader, CLabelWidget::TA_CenterGroup); \
		this->pScrollingText->AddText(wszCRLF, F_CreditsText)

//Add optional face image below name.
#	define A_FACEIMAGE(eSpeaker) { \
	SDL_Surface *pFaceSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, CX_FACE, CY_FACE, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0); \
	pFaceWidget->SetDestSurface(pFaceSurface); \
	pFaceWidget->SetCharacter(eSpeaker, false); \
	pFaceWidget->Paint(); \
	CImageWidget *pImage = new CImageWidget(0, (this->pScrollingText->GetW()-CX_FACE)/2, 0, pFaceSurface); \
	this->pScrollingText->Add(pImage); }


	//Credits start here.
	A_CONTRIBUTOR(MID_Cr_Producer, MID_Cr_Producer_C);
	A_FACEIMAGE(Speaker_Stalwart);

	A_CONTRIBUTOR(MID_Cr_LeadProgrammer, MID_Cr_LeadProgrammer_C);
	A_FACEIMAGE(Speaker_TarTechnician);

	A_CONTRIBUTOR(MID_Cr_Music, MID_Cr_Music_C);
	A_FACEIMAGE(Speaker_Slayer);

	A_CONTRIBUTOR(MID_Cr_Artwork, MID_Cr_Artwork_C);
	A_FACEIMAGE(Speaker_GoblinKing);

	A_CONTRIBUTOR(MID_Cr_Levels, MID_Cr_Levels_C);
	A_FACEIMAGE(Speaker_Aumtlich);

	A_CONTRIBUTOR(MID_Cr_Story, MID_Cr_Story_C);
	A_FACEIMAGE(Speaker_Pirate);

	A_CONTRIBUTOR(MID_Cr_CaravelNet, MID_Cr_CaravelNet_C);
	A_FACEIMAGE(Speaker_Goblin);

	A_CONTRIBUTOR(MID_Cr_Voice, MID_Cr_Voice_C);
	A_FACEIMAGE(Speaker_RockGiant);

	A_CONTRIBUTOR(MID_Cr_SoundEffects, MID_Cr_SoundEffects_C);
	A_FACEIMAGE(Speaker_Citizen1);

	A_CONTRIBUTOR(MID_Cr_VoiceCoord, MID_Cr_VoiceCoord_C);
	A_FACEIMAGE(Speaker_Halph);

	A_CONTRIBUTOR(MID_Cr_Linux, MID_Cr_Linux_C);
	A_FACEIMAGE(Speaker_Beethro);

	A_CONTRIBUTOR(MID_Cr_Mac, MID_Cr_Mac_C);
	A_FACEIMAGE(Speaker_Citizen3);

	A_CONTRIBUTOR(MID_Cr_Web, MID_Cr_Web_C);
	A_FACEIMAGE(Speaker_Citizen);

	A_CONTRIBUTOR(MID_Cr_TestCoord, MID_Cr_TestCoord_C);
	A_FACEIMAGE(Speaker_RockGolem);

	A_CONTRIBUTOR(MID_Cr_Testers, MID_Cr_Testers_C);
	A_FACEIMAGE(Speaker_Citizen2);

	this->pScrollingText->AddText(wszCRLF, F_CreditsText);

	A_TEXT(MID_CreditsLastWords);

	A_TEXT(MID_CreditsTheEnd);


#undef A_TEXT
#undef A_CONTRIBUTOR
#undef A_FACEIMAGE

	RemoveWidget(pFaceWidget);

	ClearEvents(); //don't let an extra keypress during transition cause quick exit

	return true;
}

//*****************************************************************************************
void CCreditsScreen::OnKeyDown(
//
//Params:
	const UINT dwTagNo, const SDL_KeyboardEvent &Key)
{ 
	HideCursor();

	CScreen::OnKeyDown(dwTagNo,Key); //For alt-F4, F10, etc.
	if (IsDeactivating()) {ShowCursor(); return;}

	 //Ignore control keys below if song is playing.
	 if (!this->pScrollingText->IsVisible()) return;

	//Check for other keys.
	switch (Key.keysym.sym)
	{
		case SDLK_PAUSE:
			//pause animation
			this->bPaused = true;
			SetRate(0);
			break;
		case SDLK_SPACE:
			//toggle pause animation unless song is playing.
			this->bPaused = !this->bPaused;
			SetRate(this->bPaused ? 0 : this->fScrollRateMultiplier);
			break;
		case SDLK_KP2: case SDLK_DOWN:
			//increase scroll rate
			this->bPaused = false;
			if (this->fScrollRateMultiplier < 5.0f) this->fScrollRateMultiplier += 0.10f;
			SetRate(this->fScrollRateMultiplier);
			break;
		case SDLK_KP8: case SDLK_UP:
			//slow scroll rate
			this->bPaused = false;
			if (this->fScrollRateMultiplier > 0.15f) this->fScrollRateMultiplier -= 0.10f;
			SetRate(this->fScrollRateMultiplier);
			break;
		default:
			//unpause
			this->bPaused = false;
			SetRate(this->fScrollRateMultiplier);
			break;
	}
}

//************************************************************************************
void CCreditsScreen::OnBetweenEvents()
//Handle events 
{  
	CScreen::OnBetweenEvents();

	//If the scrolling text widget is visible, then I am in the first stage.
	if (this->pScrollingText->IsVisible())
	{
		//Exit after the scrolling text is done.
		if (this->pScrollingText->empty())
		{
			GoToScreen(SCR_Return);
			return;
		}
	}
}

//*****************************************************************************
bool CCreditsScreen::OnQuit()
//Called when SDL_QUIT event is received.
//Returns whether Quit action was confirmed.
{
	//Pause action while the "Really quit?" dialog is activated.
	const bool bWasPaused = this->bPaused;
	if (!bWasPaused)
	{
		this->bPaused = true;
		SetRate(0);
	}

	const bool bQuit = CScreen::OnQuit(); //no sell screen

	if (!bWasPaused)
	{
		this->bPaused = false;
		SetRate(this->fScrollRateMultiplier);
	} else {
		//redraw screen parts
		PaintChildren();
		UpdateRect();
	}

	return bQuit;
}


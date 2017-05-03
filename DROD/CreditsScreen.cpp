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

#define CREDITS_BG (0)

//
//Protected methods.
//

//************************************************************************************
CCreditsScreen::CCreditsScreen(SCREENTYPE screentype)
	: CDrodScreen(screentype)
	, pFaceWidget(NULL)
	, pScrollingText(NULL)
//Constructor.
{
	this->pScrollingText = new CScrollingTextWidget(0L, X_CREDITS, Y_CREDITS, 
			CX_CREDITS, CY_CREDITS);
	AddWidget(this->pScrollingText);
	SetRate(CCreditsScreen::fScrollRateMultiplier);
}

//*****************************************************************************
CCreditsGatEBScreen::CCreditsGatEBScreen()
	: CCreditsScreen(SCR_CreditsGatEB)
{
	this->imageFilenames.push_back(string("Credits"));
}

CCreditsJtRHScreen::CCreditsJtRHScreen()
	: CCreditsScreen(SCR_CreditsJtRH)
{
	this->imageFilenames.push_back(string("CreditsJtRH"));
}

CCreditsKDDScreen::CCreditsKDDScreen()
	: CCreditsScreen(SCR_CreditsKDD)
{
	this->imageFilenames.push_back(string("CreditsJtRH"));
}

CCreditsTCBScreen::CCreditsTCBScreen()
	: CCreditsScreen(SCR_CreditsTCB)
{
	this->imageFilenames.push_back(string("Credits"));
}

CCreditsTSSScreen::CCreditsTSSScreen()
	: CCreditsScreen(SCR_CreditsTSS)
{
	this->imageFilenames.push_back(string("CreditsJtRH"));
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
	SDL_BlitSurface(this->images[CREDITS_BG], NULL, GetDestSurface(), NULL);

	PaintChildren();
	
	if (bUpdateRect) UpdateRect();
}

//************************************************************************************
void CCreditsScreen::SetForActivateStart()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	HideCursor();

	//Return to title screen when done.
	g_pTheSM->ClearReturnScreens();
	g_pTheSM->InsertReturnScreen(SCR_Title);

	//Fix up scrolling text widget state in case screen was activated before.
	this->pScrollingText->SetBackground(this->images[CREDITS_BG]);
	this->pScrollingText->ClearText();
	this->pScrollingText->ScrollAbsolute(0,0);
	this->pScrollingText->Show();

	//For face image display preparation.
	this->pFaceWidget = new CFaceWidget(0, 0, 0, CX_FACE, CY_FACE);
	AddWidget(this->pFaceWidget, true);

//Add some text to the scrolling text widget.
#  define A_TEXT(mid) \
		this->pScrollingText->AddText(g_pTheDB->GetMessageText(mid), F_CreditsText);\
		this->pScrollingText->AddText(wszCRLF, F_CreditsText)

//Add a contributor header.
#	define A_CONTRIBUTOR_NAMEONLY(midName) \
		this->pScrollingText->AddText(g_pTheDB->GetMessageText(midName), F_CreditsHeader, CLabelWidget::TA_CenterGroup) \

#	define A_CONTRIBUTOR(midName, midC) \
		this->pScrollingText->AddText(g_pTheDB->GetMessageText(midName), F_CreditsHeader, CLabelWidget::TA_CenterGroup); \
		this->pScrollingText->AddText(g_pTheDB->GetMessageText(midC), F_CreditsSubheader, CLabelWidget::TA_CenterGroup) \

//Add optional face image below name.
#	define A_FACEIMAGE(eSpeaker) { \
	SDL_Surface *pFaceSurface = SDL_CreateRGBSurface(SDL_SWSURFACE, CX_FACE, CY_FACE, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0); \
	this->pFaceWidget->SetDestSurface(pFaceSurface); \
	this->pFaceWidget->SetCharacter(eSpeaker, false); \
	this->pFaceWidget->Paint(); \
	CImageWidget *pImage = new CImageWidget(0, (this->pScrollingText->GetW()-CX_FACE)/2, 0, pFaceSurface); \
	this->pScrollingText->Add(pImage); }
}

//************************************************************************************
bool CCreditsGatEBScreen::SetForActivate()
{
	SetForActivateStart();

	g_pTheSound->PlaySong(SONGID_CREDITS_GATEB);

	A_TEXT(MID_CreditsIntro_GatEB);

	A_CONTRIBUTOR(MID_ErikH, MID_ErikH_C_GatEB);
	A_FACEIMAGE(Speaker_Beethro);
	A_TEXT(MID_ErikH_Text_GatEB);

	A_CONTRIBUTOR(MID_MikeR, MID_MikeR_C_GatEB);
	A_FACEIMAGE(Speaker_Halph);
	A_TEXT(MID_MikeR_Text_GatEB);

	A_CONTRIBUTOR(MID_MattS, MID_MattS_C_GatEB);
	A_FACEIMAGE(Speaker_Stalwart);
	A_TEXT(MID_MattS_Text_GatEB);

	A_CONTRIBUTOR(MID_JacobG, MID_JacobG_C_GatEB);
	A_FACEIMAGE(Speaker_RockGiant);
	A_TEXT(MID_JacobG_Text_GatEB);

	A_CONTRIBUTOR(MID_TerenceF, MID_TerenceF_C_GatEB);
	A_FACEIMAGE(Speaker_Brain);
	A_TEXT(MID_TerenceF_Text_GatEB);

	A_CONTRIBUTOR(MID_LarryM_HenriK, MID_LarryM_HenriK_C_GatEB);
	A_FACEIMAGE(Speaker_Aumtlich);
	A_TEXT(MID_LarryM_HenriK_Text_GatEB);

	A_CONTRIBUTOR(MID_JonS, MID_JonS_C);
	A_FACEIMAGE(Speaker_RockGolem);
	A_TEXT(MID_JonS_Text_GatEB);

	A_CONTRIBUTOR(MID_GerryJ, MID_GerryJ_C_GatEB);
	A_FACEIMAGE(Speaker_GoblinKing);
	A_TEXT(MID_GerryJ_Text_GatEB);

	A_CONTRIBUTOR(MID_LoganW, MID_LoganW_C_GatEB);
	A_FACEIMAGE(Speaker_Gunthro);
	A_TEXT(MID_LoganW_Text_GatEB);

	A_CONTRIBUTOR(MID_TomB_EytanZ, MID_TomB_EytanZ_C_GatEB);
	A_FACEIMAGE(Speaker_TarTechnician);
	A_TEXT(MID_TomB_EytanZ_Text_GatEB);

	A_CONTRIBUTOR(MID_NeilF, MID_NeilF_C_GatEB);
	A_FACEIMAGE(Speaker_Citizen2);
	A_TEXT(MID_NeilF_Text_GatEB);

	A_CONTRIBUTOR(MID_BrettB, MID_BrettB_C_GatEB);
	A_FACEIMAGE(Speaker_Citizen3);
	A_TEXT(MID_BrettB_Text_GatEB);

	A_CONTRIBUTOR(MID_WaiL, MID_WaiL_C_GatEB);
	A_FACEIMAGE(Speaker_Citizen1);
	A_TEXT(MID_WaiL_Text_GatEB);

	A_CONTRIBUTOR(MID_JenniferL, MID_JenniferL_C_GatEB);
	A_FACEIMAGE(Speaker_Citizen4);
	A_TEXT(MID_JenniferL_Text_GatEB);

	A_CONTRIBUTOR_NAMEONLY(MID_VoiceTalent);
	A_FACEIMAGE(Speaker_Negotiator);
	A_TEXT(MID_VoiceTalent_Text_GatEB);

	A_CONTRIBUTOR(MID_LevelDescriptionWriters_GatEB, MID_LevelDescriptionWriters_C_GatEB);
	A_FACEIMAGE(Speaker_Citizen);
	A_TEXT(MID_LevelDescriptionWriters_Text_GatEB);

	A_CONTRIBUTOR(MID_Testers_GatEB, MID_Testers_C);
	A_FACEIMAGE(Speaker_Slayer2);
	A_TEXT(MID_Testers_Text_GatEB);

	A_TEXT(MID_CreditsLastWords_GatEB);

	A_TEXT(MID_CreditsTheEnd_GatEB);

	SetForActivateComplete();

	return true;
}

//************************************************************************************
bool CCreditsJtRHScreen::SetForActivate()
{
	SetForActivateStart();

	g_pTheSound->PlaySong(SONGID_CREDITS_JTRH);

	A_TEXT(MID_CreditsIntro_JtRH);

	A_CONTRIBUTOR(MID_ErikH, MID_ErikH_C_JtRH);
	A_FACEIMAGE(Speaker_Slayer);
	A_TEXT(MID_ErikH_Text_JtRH);

	A_CONTRIBUTOR(MID_MikeR, MID_MikeR_C_JtRH);
	A_FACEIMAGE(Speaker_Beethro);
	A_TEXT(MID_MikeR_Text_JtRH);

	A_CONTRIBUTOR(MID_MattS, MID_MattS_C_JtRH);
	A_FACEIMAGE(Speaker_Goblin);
	A_TEXT(MID_MattS_Text_JtRH);

	A_CONTRIBUTOR(MID_GerryJ, MID_GerryJ_C_JtRH);
	A_FACEIMAGE(Speaker_Negotiator);
	A_TEXT(MID_GerryJ_Text_JtRH);

	A_CONTRIBUTOR(MID_EmmettP, MID_EmmettP_C_JtRH);
	A_FACEIMAGE(Speaker_GoblinKing);
	A_TEXT(MID_EmmettP_Text_JtRH);

	A_CONTRIBUTOR(MID_AdamP, MID_AdamP_C_JtRH);
	A_FACEIMAGE(Speaker_MudCoordinator);
	A_TEXT(MID_AdamP_Text_JtRH);

	A_CONTRIBUTOR(MID_EytanZ, MID_EytanZ_C_JtRH);
	A_FACEIMAGE(Speaker_TarTechnician);
	A_TEXT(MID_EytanZ_Text_JtRH);

	A_CONTRIBUTOR(MID_MattC, MID_MattC_C_JtRH);
	A_FACEIMAGE(Speaker_Halph);
	A_TEXT(MID_MattC_Text_JtRH);

	A_CONTRIBUTOR(MID_NeilF, MID_NeilF_C_JtRH);
	A_FACEIMAGE(Speaker_Citizen1);
	A_TEXT(MID_NeilF_Text_JtRH);

	A_CONTRIBUTOR(MID_ClaytonW, MID_ClaytonW_C_JtRH);
	A_FACEIMAGE(Speaker_RockGolem);
	A_TEXT(MID_ClaytonW_Text_JtRH);

	A_CONTRIBUTOR(MID_StenR, MID_StenR_C_JtRH);
	A_FACEIMAGE(Speaker_Beethro);
	A_TEXT(MID_StenR_Text_JtRH);

	A_CONTRIBUTOR(MID_ToddD, MID_ToddD_C_JtRH);
	A_FACEIMAGE(Speaker_Slayer);
	A_TEXT(MID_ToddD_Text_JtRH);

	A_CONTRIBUTOR(MID_BrittanyH, MID_BrittanyH_C_JtRH);
	A_FACEIMAGE(Speaker_Negotiator);
	A_TEXT(MID_BrittanyH_Text_JtRH);

	A_CONTRIBUTOR(MID_ChrisMarks, MID_ChrisMarks_C_JtRH);
	A_FACEIMAGE(Speaker_GoblinKing);
	A_TEXT(MID_ChrisMarks_Text_JtRH);

	A_CONTRIBUTOR(MID_TylerDowning, MID_TylerDowning_C_JtRH);
	A_FACEIMAGE(Speaker_Halph);
	A_TEXT(MID_TylerDowning_Text_JtRH);

	A_CONTRIBUTOR(MID_SteveHartley, MID_SteveHartley_C_JtRH);
	A_FACEIMAGE(Speaker_Citizen2);
	A_TEXT(MID_SteveHartley_Text_JtRH);

	A_CONTRIBUTOR(MID_MichaelAbbott, MID_MichaelAbbott_C_JtRH);
	A_FACEIMAGE(Speaker_TarTechnician);
	A_TEXT(MID_MichaelAbbott_Text_JtRH);

	A_CONTRIBUTOR(MID_Testers_JtRH, MID_Testers_C_JtRH);
	A_FACEIMAGE(Speaker_Citizen2);
	A_TEXT(MID_Testers_Text_JtRH);

	A_TEXT(MID_CreditsLastWords_JtRH);

	A_TEXT(MID_CreditsTheEnd_JtRH);

	SetForActivateComplete();

	return true;
}

//************************************************************************************
bool CCreditsKDDScreen::SetForActivate()
{
	SetForActivateStart();

	g_pTheSound->PlaySong(SONGID_CREDITS_KDD);

	A_TEXT(MID_CreditsIntro_KDD);

	A_CONTRIBUTOR(MID_ErikH, MID_ErikH_C_KDD);
	A_FACEIMAGE(Speaker_GoblinKing);
	A_TEXT(MID_ErikH_Text_KDD);

	A_CONTRIBUTOR(MID_NeilF, MID_NeilF_C_KDD);
	A_FACEIMAGE(Speaker_Slayer);
	A_TEXT(MID_NeilF_Text_KDD);

	A_CONTRIBUTOR(MID_MikeR, MID_MikeR_C_KDD);
	A_FACEIMAGE(Speaker_Beethro);
	A_TEXT(MID_MikeR_Text_KDD);

	A_CONTRIBUTOR(MID_OtherActors_KDD, MID_OtherActors_C_KDD);
	A_FACEIMAGE(Speaker_Goblin);
	A_TEXT(MID_OtherActors_Text_KDD);

	A_CONTRIBUTOR(MID_OtherDesigners_KDD, MID_OtherDesigners_C_KDD);
	A_FACEIMAGE(Speaker_TarTechnician);
	A_TEXT(MID_OtherDesigners_Text_KDD);

	A_TEXT(MID_CreditsLastWords_KDD);

	A_TEXT(MID_CreditsTheEnd_KDD);

	SetForActivateComplete();

	return true;
}

//************************************************************************************
bool CCreditsTCBScreen::SetForActivate()
{
	SetForActivateStart();

	g_pTheSound->PlaySong(SONGID_CREDITS_TCB);

	A_TEXT(MID_CreditsIntro_TCB);

	A_CONTRIBUTOR(MID_ErikH, MID_ErikH_C_TCB);
	A_FACEIMAGE(Speaker_Slayer);
	A_TEXT(MID_ErikH_Text_TCB);

	A_CONTRIBUTOR(MID_MikeR, MID_MikeR_C_TCB);
	A_FACEIMAGE(Speaker_Beethro);
	A_TEXT(MID_MikeR_Text_TCB);

	A_CONTRIBUTOR(MID_MattS, MID_MattS_C_TCB);
	A_FACEIMAGE(Speaker_Stalwart);
	A_TEXT(MID_MattS_Text_TCB);

	A_CONTRIBUTOR(MID_GerryJ, MID_GerryJ_C_TCB);
	A_FACEIMAGE(Speaker_GoblinKing);
	A_TEXT(MID_GerryJ_Text_TCB);

	A_CONTRIBUTOR(MID_EytanZ, MID_EytanZ_C_TCB);
	A_FACEIMAGE(Speaker_TarTechnician);
	A_TEXT(MID_EytanZ_Text_TCB);

	A_CONTRIBUTOR(MID_NeilF, MID_NeilF_C_TCB);
	A_FACEIMAGE(Speaker_Aumtlich);
	A_TEXT(MID_NeilF_Text_TCB);

	A_CONTRIBUTOR(MID_AdamP, MID_AdamP_C_TCB);
	A_FACEIMAGE(Speaker_MudCoordinator);
	A_TEXT(MID_AdamP_Text_TCB);

	A_CONTRIBUTOR(MID_JonS, MID_JonS_C_TCB);
	A_FACEIMAGE(Speaker_RockGolem);
	A_TEXT(MID_JonS_Text_TCB);

	A_CONTRIBUTOR(MID_ClaytonW, MID_ClaytonW_C_TCB);
	A_FACEIMAGE(Speaker_RockGiant);
	A_TEXT(MID_ClaytonW_Text_TCB);

	A_CONTRIBUTOR(MID_WilliamF, MID_WilliamF_C_TCB);
	A_FACEIMAGE(Speaker_Citizen2);
	A_TEXT(MID_WilliamF_Text_TCB);

	A_CONTRIBUTOR(MID_AshleyK, MID_AshleyK_C_TCB);
	A_FACEIMAGE(Speaker_Citizen3);
	A_TEXT(MID_AshleyK_Text_TCB);

	A_CONTRIBUTOR_NAMEONLY(MID_VoiceTalent_TCB);
	A_FACEIMAGE(Speaker_Negotiator);
	A_TEXT(MID_VoiceTalent_Text_TCB);

	A_CONTRIBUTOR(MID_Testers_TCB, MID_Testers_C);
	A_FACEIMAGE(Speaker_Slayer2);
	A_TEXT(MID_Testers_Text_TCB);

	A_TEXT(MID_CreditsLastWords_TCB);

	A_TEXT(MID_CreditsTheEnd_TCB);

	SetForActivateComplete();

	return true;
}

//************************************************************************************
bool CCreditsTSSScreen::SetForActivate()
{
	SetForActivateStart();

	g_pTheSound->PlaySong(SONGID_CREDITS_TSS);

	A_TEXT(MID_CreditsIntro_TSS);

	A_CONTRIBUTOR(MID_ErikH, MID_ErikH_C_TSS);
	A_FACEIMAGE(Speaker_Gunthro);
	A_TEXT(MID_ErikH_Text_TSS);

	A_CONTRIBUTOR(MID_MikeR, MID_MikeR_C_TSS);
	A_FACEIMAGE(Speaker_Beethro);
	A_TEXT(MID_MikeR_Text_TSS);

	A_CONTRIBUTOR(MID_MattS, MID_MattS_C_TSS);
	A_FACEIMAGE(Speaker_Construct);
	A_TEXT(MID_MattS_Text_TSS);

	A_CONTRIBUTOR(MID_JacobG, MID_JacobG_C_TSS);
	A_FACEIMAGE(Speaker_TarTechnician);
	A_TEXT(MID_JacobG_Text_TSS);

	A_CONTRIBUTOR(MID_TerenceF, MID_TerenceF_C_TSS);
	A_FACEIMAGE(Speaker_Stalwart);
	A_TEXT(MID_TerenceF_Text_TSS);

	A_CONTRIBUTOR(MID_LarryM, MID_LarryM_C_TSS);
	A_FACEIMAGE(Speaker_GoblinKing);
	A_TEXT(MID_LarryM_Text_TSS);

	A_CONTRIBUTOR(MID_LevelDesigners_TSS, MID_LevelDesigners_C_TSS);
	A_FACEIMAGE(Speaker_Halph);
	A_TEXT(MID_LevelDesigners_Text_TSS);

	A_CONTRIBUTOR(MID_MaurycyZ, MID_MaurycyZ_C_TSS);
	A_FACEIMAGE(Speaker_Brain);
	A_TEXT(MID_MaurycyZ_Text_TSS);

	A_CONTRIBUTOR(MID_AlexK, MID_AlexK_C_TSS);
	A_FACEIMAGE(Speaker_Aumtlich);
	A_TEXT(MID_AlexK_Text_TSS);

	A_CONTRIBUTOR(MID_GrahamB, MID_GrahamB_C_TSS);
	A_FACEIMAGE(Speaker_Citizen1);
	A_TEXT(MID_GrahamB_Text_TSS);

	A_CONTRIBUTOR(MID_JonS, MID_JonS_C_TSS);
	A_FACEIMAGE(Speaker_RockGolem);
	A_TEXT(MID_JonS_Text_TSS);

	A_CONTRIBUTOR(MID_EmmettP, MID_EmmettP_C_TSS);
	A_FACEIMAGE(Speaker_RockGiant);
	A_TEXT(MID_EmmettP_Text_TSS);

	A_CONTRIBUTOR(MID_GerryJ, MID_GerryJ_C_TSS);
	A_FACEIMAGE(Speaker_Negotiator);
	A_TEXT(MID_GerryJ_Text_TSS);

	A_CONTRIBUTOR_NAMEONLY(MID_VoiceTalent);
	A_FACEIMAGE(Speaker_Halph);
	A_TEXT(MID_VoiceTalent_Text_TSS);

	A_CONTRIBUTOR(MID_Testers_TSS, MID_Testers_C);
	A_FACEIMAGE(Speaker_Slayer);
	A_TEXT(MID_Testers_Text_TSS);

	A_TEXT(MID_CreditsLastWords_TSS);

	A_TEXT(MID_CreditsTheEnd_TSS);

	SetForActivateComplete();

	return true;
}

//*****************************************************************************************
void CCreditsScreen::SetForActivateComplete()
{
	RemoveWidget(this->pFaceWidget);

	ClearEvents(); //don't let an extra keypress during transition cause quick exit
}

#undef A_TEXT
#undef A_CONTRIBUTOR
#undef A_FACEIMAGE

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
		case SDLK_KP_2: case SDLK_DOWN:
			//increase scroll rate
			this->bPaused = false;
			if (this->fScrollRateMultiplier < 5.0f) this->fScrollRateMultiplier += 0.10f;
			SetRate(this->fScrollRateMultiplier);
			break;
		case SDLK_KP_8: case SDLK_UP:
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

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
 * 2003, 2005 Caravel Software. All Rights Reserved.
 *
 * ***** END LICENSE BLOCK ***** */

#include "SellScreen.h"
#include "DrodFontManager.h"
#include "DrodSound.h"

#include "../DRODLib/Db.h"
#include "../DRODLib/SettingsKeys.h"

#include <FrontEndLib/BitmapManager.h>
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/LabelWidget.h>

#include <BackEndLib/Browser.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Ports.h>

// Widget tags.
const UINT TAG_BUY = 2000;
const UINT TAG_BACK = 2001;
const UINT TAG_EXIT = 2002;
const UINT TAG_GOTOFORUM = 2003;

///////////////////////////
#define IMAGE_BG      (0)

//TCB+GatEB+TSS
#define IMAGE_SSHOT   (1)

//GatEB
#define IMAGE_TEXTS   (2)

//TSS
#define IMAGE_BAD_TEXTS (2)
#define IMAGE_GOOD_TEXTS (3)

/*
TOADD:
	JtRH+KDD show the sell screens when demo version is present, thanks screens when full version is present
	GatEB: sell in demo, thanks in full
*/

//******************************************************************************
void CSellScreen2::OnClick(
//Handles click events.
//
//Params:
	const UINT dwTagNo) //(in) Widget receiving event.
{
	switch (dwTagNo)
	{
		case TAG_BACK:
			GoToScreen(SCR_Return);
		break;

		case TAG_BUY:
			GoToBuyNow();
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
bool CSellScreen2::OnQuit()
//Called when SDL_QUIT event is received.
//Since this is the exit confirmation screen, any further exit events at this
//point should always exit the app.
{
	GoToScreen(SCR_None);
	return true;
}

//******************************************************************************
void CSellScreen2::PlaySong(SONGID songid)
{
	g_pTheSound->PlaySong(songid);
}

//******************************************************************************
bool CSellScreen2::SetForActivate()
//Called before screen is activated and first paint.
{
	PlaySellSong();

	return true;
}


void CSellScreen2::Version2ScreenSetup()
{
	//Screen naming conventions.
	static const WCHAR demo[] = {We('S'),We('e'),We('l'),We('l'),We(0)};
	static const WCHAR registered[] = {We('T'),We('h'),We('a'),We('n'),We('k'),We('s'),We(0)};

	const string imageSuffix = GetImageSuffix();
	WSTRING wstrImageSuffix;
	AsciiToUnicode(imageSuffix.c_str(), wstrImageSuffix);

	const string INI_KEY = "ExitScreenNum";
	const string ini_key = INI_KEY + imageSuffix;

	//Load a new screen for this session.
	CFiles f;
	WSTRING wExitScreenNum;
	WCHAR wcExitScreenNum[10];
	int nExitScreenNum = 1;	//default

	{
		string exitScreenNum;
		if (f.GetGameProfileString(INISection::Startup, ini_key.c_str(), exitScreenNum))
			nExitScreenNum = atoi(exitScreenNum.c_str());
		_itoW(nExitScreenNum, wcExitScreenNum, 10);
		wExitScreenNum = wcExitScreenNum;
	}

	//Check whether this screen exists.
	WSTRING wScrName;
	bool bCheckAgain;
	bool bRegistered = false;
	bool bFoundFile = true;
	do
	{
		bCheckAgain = false;
		WSTRING wstr;
#ifndef STEAMBUILD //Steam shouldn't show demo sell screens when playing KDD/JtRH DLC
		wScrName = demo;	//check for demo screen first
		wScrName += wExitScreenNum;
		wScrName += wstrImageSuffix;
		if (!CBitmapManager::GetImageFilepath(wScrName.c_str(), wstr) &&
				!g_pTheDB->Data.FindByName(wScrName.c_str()))
#endif
		{
			//Check for registered screen.
			wScrName = registered;
			wScrName += wExitScreenNum;
			wScrName += wstrImageSuffix;
			wstr = wszEmpty;
			if (CBitmapManager::GetImageFilepath(wScrName.c_str(), wstr) ||
					g_pTheDB->Data.FindByName(wScrName.c_str()))
				bRegistered = true;
			else
			{
				if (nExitScreenNum != 1)
				{
					//Reset counter to the first screen and look for an existing name.
					bCheckAgain = true;
					nExitScreenNum = 1;
					_itoW(nExitScreenNum, wcExitScreenNum, 10);
					wExitScreenNum = wcExitScreenNum;
				} else {
					bFoundFile = false;	//no BG file to load was found
				}
			}
		}
	} while (bCheckAgain);

	if (bFoundFile)
	{
		//Show the following screen next session.
		char cExitScreenNum[10];
		_itoa(nExitScreenNum+1, cExitScreenNum, 10);
		f.WriteGameProfileString(INISection::Startup, ini_key.c_str(), cExitScreenNum);

		//Load this background screen.
		this->imageFilenames.push_back(UnicodeToAscii(wScrName));
	} else {
		//Just show a default screen when images are missing.
		this->imageFilenames.push_back(string("Background"));
	}

	static const int X_BUY_BUTTON = 23;
	static const int Y_BUY_BUTTON = 623;
	static const int X_BACK_BUTTON = X_BUY_BUTTON;
	static const int Y_BACK_BUTTON = 688;
	static const int X_EXIT_BUTTON = 156;
	static const int Y_EXIT_BUTTON = Y_BACK_BUTTON;
	static const UINT CX_BUY_BUTTON = 264;
	static const UINT CY_BUY_BUTTON = 63;
	static const UINT CX_BACK_BUTTON = 131;
	static const UINT CY_BACK_BUTTON = CY_BUY_BUTTON;
	static const UINT CX_EXIT_BUTTON = CX_BACK_BUTTON;
	static const UINT CY_EXIT_BUTTON = CY_BACK_BUTTON;

	CButtonWidget *pButton;

	//Only need the buy button for non-registered versions.
	if (!bRegistered)	
	{
		pButton = new CButtonWidget(TAG_BUY,
				X_BUY_BUTTON, Y_BUY_BUTTON, CX_BUY_BUTTON, CY_BUY_BUTTON,
				g_pTheDB->GetMessageText(MID_BuyNow));
		AddWidget(pButton);
	}

	pButton = new CButtonWidget(TAG_BACK,
			X_BACK_BUTTON, Y_BACK_BUTTON, CX_BACK_BUTTON, CY_BACK_BUTTON,
			g_pTheDB->GetMessageText(MID_Back));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_EXIT,
			X_EXIT_BUTTON, Y_EXIT_BUTTON, CX_EXIT_BUTTON, CY_EXIT_BUTTON,
			g_pTheDB->GetMessageText(MID_Exit));
	AddWidget(pButton);
}

//******************************************************************************
void CSellScreen2::Paint(
//Paint the screen.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//Blit the background graphic.
	if (this->images.size())
		SDL_BlitSurface(this->images[IMAGE_BG], NULL, GetDestSurface(), NULL);

	PaintChildren();
	if (bUpdateRect) UpdateRect();
}



//******************************************************************************
CSellScreen3::CSellScreen3(SCREENTYPE screentype)
	: CSellScreen2(screentype)
	, nextSShotToDisplay(-1)
	, dwNextSShotMove(0)
{
}

//*****************************************************************************
void CSellScreen3::MoveScreenshots(SDL_Surface *pDestSurface, const bool bUpdateRect)
//Slide screenshots around right side of screen.
{
	//Time to show a new screenshot?
	static const UINT SSHOT_DISPLAY_TIME = 2500; //ms
	const Uint32 dwNow = SDL_GetTicks();
	if (dwNow >= this->dwNextSShotMove)
	{
		this->dwNextSShotMove = dwNow + SSHOT_DISPLAY_TIME;
		++this->nextSShotToDisplay;
	}

	//Screenshot dimensions.
	ASSERT(this->images.size() > IMAGE_SSHOT);
	const UINT width = this->images[IMAGE_SSHOT]->w;
	static const UINT SCREENSHOT_HEIGHT = 226;

	//Area where this stuff is occurring.
	static const int yLower = 243;
	const int yUpper = GetScreenshotY();
	static const SDL_Rect updateRect = MAKE_SDL_RECT(749, yUpper, CScreen::CX_SCREEN - 749, yLower+SCREENSHOT_HEIGHT - yUpper);

	//Redraw background.
	{
		SDL_Rect updateRectSrc = updateRect, updateRectDest = updateRect;
		SDL_BlitSurface(this->images[IMAGE_BG], &updateRectSrc, pDestSurface, &updateRectDest);
	}

	//Screenshots cycle on and off screen in order.
	static const UINT SLIDE_DURATION = 1000;
	float fSlideInterp = (dwNow - (this->dwNextSShotMove - SSHOT_DISPLAY_TIME)) / float(SLIDE_DURATION);
	if (fSlideInterp > 1.0)
		fSlideInterp = 1.0;
	const UINT NUM_SCREENSHOTS = this->images[IMAGE_SSHOT]->h / SCREENSHOT_HEIGHT;
	UINT index;

	//Show one before previous sliding off.
	if (this->nextSShotToDisplay > 1)
	{
		index = (this->nextSShotToDisplay-2) % NUM_SCREENSHOTS;
		SDL_Rect sshotSrc = MAKE_SDL_RECT(0, index*SCREENSHOT_HEIGHT, width, SCREENSHOT_HEIGHT);
		const int xDisplay = updateRect.x + int((CScreen::CX_SCREEN - updateRect.x) * fSlideInterp);
		SDL_Rect sshotDest = MAKE_SDL_RECT(xDisplay, yUpper, width, SCREENSHOT_HEIGHT);
		SDL_BlitSurface(this->images[IMAGE_SSHOT], &sshotSrc, pDestSurface, &sshotDest);
	}
	//Show previous one sliding up.
	if (this->nextSShotToDisplay > 0)
	{
		index = (this->nextSShotToDisplay-1) % NUM_SCREENSHOTS;
		SDL_Rect sshotSrc = MAKE_SDL_RECT(0, index*SCREENSHOT_HEIGHT, width, SCREENSHOT_HEIGHT);
		const int yDisplay = yLower - int((yLower - updateRect.y) * fSlideInterp);
		SDL_Rect sshotDest = MAKE_SDL_RECT(updateRect.x, yDisplay, width, SCREENSHOT_HEIGHT);
		SDL_BlitSurface(this->images[IMAGE_SSHOT], &sshotSrc, pDestSurface, &sshotDest);
	}
	//Show newest screenshot sliding on.
	if (this->nextSShotToDisplay >= 0)
	{
		index = this->nextSShotToDisplay % NUM_SCREENSHOTS;
		SDL_Rect sshotSrc = MAKE_SDL_RECT(0, index*SCREENSHOT_HEIGHT, width, SCREENSHOT_HEIGHT);
		const int xDisplay = CScreen::CX_SCREEN - int((CScreen::CX_SCREEN - updateRect.x) * fSlideInterp);
		SDL_Rect sshotDest = MAKE_SDL_RECT(xDisplay, yLower, width, SCREENSHOT_HEIGHT);
		SDL_BlitSurface(this->images[IMAGE_SSHOT], &sshotSrc, pDestSurface, &sshotDest);
	}

	//Draw cabinet over screenshots (TCB).
	PaintCabinetEdge(pDestSurface);

	if (bUpdateRect)
		PresentRect(pDestSurface, &updateRect);
}

//*****************************************************************************
void CSellScreen3::OnBetweenEvents()
{
	MoveScreenshots(GetDestSurface(), true);
}

//******************************************************************************
CSellScreenGatEB::CSellScreenGatEB()
	: CSellScreen3(SCR_SellGatEB)
	, sellTextIndex(0)
{
	const bool bDemo = !IsGameFullVersion();

	const string background = bDemo ? "Sell1GatEB" : "Thanks1GatEB";
	this->imageFilenames.push_back(background);

	this->imageFilenames.push_back(string("StroutOfficeSShotsGatEB"));
	this->imageFilenames.push_back(string("SellTexts"));

	static const int X_BUY_BUTTON = 746;
	static const int Y_BUY_BUTTON = 619;
	static const int X_BACK_BUTTON = X_BUY_BUTTON;
	static const int Y_BACK_BUTTON = 692;
	static const int X_EXIT_BUTTON = 882;
	static const int Y_EXIT_BUTTON = Y_BACK_BUTTON;
	static const UINT CX_BUY_BUTTON = 268;
	static const UINT CY_BUY_BUTTON = 70;
	static const UINT CX_BACK_BUTTON = 132;
	static const UINT CY_BACK_BUTTON = CY_BUY_BUTTON;
	static const UINT CX_EXIT_BUTTON = CX_BACK_BUTTON;
	static const UINT CY_EXIT_BUTTON = CY_BACK_BUTTON;

	CButtonWidget *pButton;

	//Buy button for non-registered versions, forum for registered.
	if (bDemo)	
	{
		pButton = new CButtonWidget(TAG_BUY,
				X_BUY_BUTTON, Y_BUY_BUTTON, CX_BUY_BUTTON, CY_BUY_BUTTON,
				g_pTheDB->GetMessageText(MID_BuyNow));
	} else {
		pButton = new CButtonWidget(TAG_GOTOFORUM,
				X_BUY_BUTTON, Y_BUY_BUTTON, CX_BUY_BUTTON, CY_BUY_BUTTON,
				g_pTheDB->GetMessageText(MID_GoToForum));
	}
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

//******************************************************************************
void CSellScreenGatEB::Paint(
//Paint the screen.
//
//Params:
	bool bUpdateRect) //(in)   If true (default) and destination
	                  //    surface is the screen, the screen
	                  //    will be immediately updated in
	                  //    the widget's rect.
{
	SDL_Surface *pDestSurface = GetDestSurface();

	//Blit the background graphic.
	ASSERT(this->images.size() > IMAGE_SSHOT);
	SDL_BlitSurface(this->images[IMAGE_BG], NULL, pDestSurface, NULL);

	//Blit sensational text.
	static const int TEXT_WIDTH = 341, TEXT_HEIGHT = 163;
	const int num_texts = this->images[IMAGE_TEXTS]->h / (TEXT_HEIGHT+1);
	const int text_index = this->sellTextIndex % num_texts;
	SDL_Rect src_rect = MAKE_SDL_RECT(0, text_index * (TEXT_HEIGHT+1), TEXT_WIDTH, TEXT_HEIGHT);
	SDL_Rect dest_rect = MAKE_SDL_RECT(380, 527, TEXT_WIDTH, TEXT_HEIGHT);
	SDL_BlitSurface(this->images[IMAGE_TEXTS], &src_rect, pDestSurface, &dest_rect);

	MoveScreenshots(pDestSurface, false);

	PaintChildren();
	if (bUpdateRect) UpdateRect();
}

//******************************************************************************
bool CSellScreenGatEB::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	PlaySellSong();

	SelectNextSellText();

	return true;
}

//******************************************************************************
void CSellScreenGatEB::SelectNextSellText()
{
	//Select text for this session.
	CFiles f;
	static const char EXIT_NUM_INI_STRING[] = "ExitTextNum";

	int nExitTextNum = 0;
	{
		string textNum;
		if (f.GetGameProfileString(INISection::Startup, EXIT_NUM_INI_STRING, textNum))
			nExitTextNum = atoi(textNum.c_str());
	}

	this->sellTextIndex = nExitTextNum;

	{
		//Show the following text next session.
		char cExitScreenNum[12];
		_itoa(nExitTextNum+1, cExitScreenNum, 10);
		f.WriteGameProfileString(INISection::Startup, EXIT_NUM_INI_STRING, cExitScreenNum);
	}
}


//******************************************************************************
CSellScreenTSS::CSellScreenTSS()
	: CSellScreen3(SCR_SellTSS)
	, badTextIndex(0), goodTextIndex(0)
{
	const bool bDemo = !IsGameFullVersion();

	this->imageFilenames.push_back("Sell1TSS");

	this->imageFilenames.push_back(string("StroutOfficeSShotsTSS"));
	this->imageFilenames.push_back(string("Sell_TSS_Left"));
	this->imageFilenames.push_back(string("Sell_TSS_Right"));

	static const UINT CY_BUY_BUTTON = 60;
	static const int X_BUY_BUTTON = 746;
	static const int Y_BUY_BUTTON = 639;
	static const int X_BACK_BUTTON = X_BUY_BUTTON;
	static const int Y_BACK_BUTTON = Y_BUY_BUTTON + CY_BUY_BUTTON + 3;
	static const int X_EXIT_BUTTON = 882;
	static const int Y_EXIT_BUTTON = Y_BACK_BUTTON;
	static const UINT CX_BUY_BUTTON = 268;
	static const UINT CX_BACK_BUTTON = 132;
	static const UINT CY_BACK_BUTTON = CY_BUY_BUTTON;
	static const UINT CX_EXIT_BUTTON = CX_BACK_BUTTON;
	static const UINT CY_EXIT_BUTTON = CY_BACK_BUTTON;

	CButtonWidget *pButton;

	//Buy button for non-registered versions, forum for registered.
	if (bDemo)	
	{
		pButton = new CButtonWidget(TAG_BUY,
				X_BUY_BUTTON, Y_BUY_BUTTON, CX_BUY_BUTTON, CY_BUY_BUTTON,
				g_pTheDB->GetMessageText(MID_BuyNow));
	} else {
		pButton = new CButtonWidget(TAG_GOTOFORUM,
				X_BUY_BUTTON, Y_BUY_BUTTON, CX_BUY_BUTTON, CY_BUY_BUTTON,
				g_pTheDB->GetMessageText(MID_GoToForum));
	}
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

//******************************************************************************
void CSellScreenTSS::Paint(
//Paint the screen.
//
//Params:
	bool bUpdateRect) //(in)   If true (default) and destination
	                  //    surface is the screen, the screen
	                  //    will be immediately updated in
	                  //    the widget's rect.
{
	SDL_Surface *pDestSurface = GetDestSurface();

	//Blit the background graphic.
	ASSERT(this->images.size() > IMAGE_SSHOT);
	SDL_BlitSurface(this->images[IMAGE_BG], NULL, pDestSurface, NULL);

	//Blit bad texts.
	static const int BAD_TEXT_WIDTH = 225, BAD_TEXT_HEIGHT = 205;
	const int num_bad_texts = this->images[IMAGE_BAD_TEXTS]->h / (BAD_TEXT_HEIGHT+1);
	const int bad_text_index = this->badTextIndex % num_bad_texts;
	SDL_Rect bad_src_rect = MAKE_SDL_RECT(0, bad_text_index * (BAD_TEXT_HEIGHT+1), BAD_TEXT_WIDTH, BAD_TEXT_HEIGHT);
	SDL_Rect bad_dest_rect = MAKE_SDL_RECT(28, 227, BAD_TEXT_WIDTH, BAD_TEXT_HEIGHT);
	SDL_BlitSurface(this->images[IMAGE_BAD_TEXTS], &bad_src_rect, pDestSurface, &bad_dest_rect);

	//Blit good texts.
	static const int GOOD_TEXT_WIDTH = 180, GOOD_TEXT_HEIGHT = 170;
	const int num_good_texts = this->images[IMAGE_GOOD_TEXTS]->h / (GOOD_TEXT_HEIGHT+1);
	const int good_text_index = this->goodTextIndex % num_good_texts;
	SDL_Rect good_src_rect = MAKE_SDL_RECT(0, good_text_index * (GOOD_TEXT_HEIGHT+1), GOOD_TEXT_WIDTH, GOOD_TEXT_HEIGHT);
	SDL_Rect good_dest_rect = MAKE_SDL_RECT(495, 253, GOOD_TEXT_WIDTH, GOOD_TEXT_HEIGHT);
	SDL_BlitSurface(this->images[IMAGE_GOOD_TEXTS], &good_src_rect, pDestSurface, &good_dest_rect);

	MoveScreenshots(pDestSurface, false);

	PaintChildren();
	if (bUpdateRect) UpdateRect();
}

//******************************************************************************
bool CSellScreenTSS::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	PlaySellSong();

	SelectNextSellText();

	return true;
}

//******************************************************************************
void CSellScreenTSS::SelectNextSellText()
{
	//Select text for this session.
	CFiles f;
	static const char TSS_BAD_EXIT_NUM_INI_STRING[] = "TSSExitBadTextNum";
	static const char TSS_GOOD_EXIT_NUM_INI_STRING[] = "TSSExitGoodTextNum";

	int nBadExitTextNum = 0, nGoodExitTextNum = 0;
	{
		string textNum;
		if (f.GetGameProfileString(INISection::Startup, TSS_BAD_EXIT_NUM_INI_STRING, textNum))
			nBadExitTextNum = atoi(textNum.c_str());
		if (f.GetGameProfileString(INISection::Startup, TSS_GOOD_EXIT_NUM_INI_STRING, textNum))
			nGoodExitTextNum = atoi(textNum.c_str());
	}

	this->badTextIndex = nBadExitTextNum;
	this->goodTextIndex = nGoodExitTextNum;

	{
		//Show the subsequent texts next session.
		char cExitScreenNum[12];
		_itoa(nBadExitTextNum+1, cExitScreenNum, 10);
		f.WriteGameProfileString(INISection::Startup, TSS_BAD_EXIT_NUM_INI_STRING, cExitScreenNum);
		_itoa(nGoodExitTextNum+1, cExitScreenNum, 10);
		f.WriteGameProfileString(INISection::Startup, TSS_GOOD_EXIT_NUM_INI_STRING, cExitScreenNum);
	}
}

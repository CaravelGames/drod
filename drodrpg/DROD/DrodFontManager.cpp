// $Id: DrodFontManager.cpp 9776 2011-11-10 16:32:50Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef WIN32
#pragma warning(disable:4786)
#endif

#define INCLUDED_FROM_DRODFONTMANAGER_CPP
#include "DrodFontManager.h"
#undef INCLUDED_FROM_DRODFONTMANAGER_CPP

#include <FrontEndLib/ListBoxWidget.h>
#include "../Texts/MIDs.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Files.h>

//Holds the only instance of CDrodFontManager for the app.
CDrodFontManager *g_pTheDFM = NULL;

//const SDL_Color InventoryBG = {180, 180, 180, 0};
const SDL_Color InvSlotBG = {178, 178, 178, 0}; //BG color of inventory slots

//*****************************************************************************
CDrodFontManager::CDrodFontManager()
	: CFontManager()
{
	this->LoadedFonts = new LOADEDFONT[FONT_COUNT];

	for (UINT wFontI = 0; wFontI < FONT_COUNT; ++wFontI)
		memset(&(this->LoadedFonts[wFontI]), 0, sizeof(this->LoadedFonts[wFontI]));
}

//*****************************************************************************
UINT CDrodFontManager::Init()
//Initializes the font manager.
//
//Returns:
//MID_Success or MID_* describing failure.
{
	LOGCONTEXT("CDrodFontManager::Init");

	if ( TTF_Init() < 0 ) return MID_TTFInitFailed;
	atexit(TTF_Quit);

	//Load all the fonts used by application.
	if (!LoadFonts())
	{
		return MID_TTFInitFailed;
	}

	return MID_Success;
}

//
//Private methods.
//

//*********************************************************************************
bool CDrodFontManager::LoadFonts()
{
	static const WCHAR pwzFonts[] = { We('F'),We('o'),We('n'),We('t'),We('s'),We(0) };
	static const WCHAR pwzTomnr[] = { We('t'),We('o'),We('m'),We('n'),We('r'),We('.'),We('t'),We('t'),We('f'),We(0) };
	static const WCHAR pwzEpilog[] = { We('e'),We('p'),We('i'),We('l'),We('o'),We('g'),We('.'),We('t'),We('t'),We('f'),We(0) };

	CFiles Files;
	WSTRING wstrFontFilepath = Files.GetResPath();
	wstrFontFilepath += wszSlash;
	wstrFontFilepath += pwzFonts;
	wstrFontFilepath += wszSlash;
	WSTRING wstrFont2Filepath = wstrFontFilepath;
	wstrFontFilepath += pwzTomnr;
	wstrFont2Filepath += pwzEpilog;

	//Note: The entire loaded font array has been zeroed, so it is not necessary
	//to set every member of LOADEDFONT.

	//Load message font.
	UINT wSpaceWidth;
	TTF_Font *pFont = GetFont(wstrFontFilepath, 24);
	if (!pFont) return false;
	this->LoadedFonts[F_Message].pTTFFont = pFont;
	this->LoadedFonts[F_Message].wLineSkipHeight = TTF_FontLineSkip(pFont);
	GetWordWidth(F_Message, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_Message].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_Message].BackColor = LightBrown;
	this->LoadedFonts[F_Message].bAntiAlias = true;

	//Text font (reduced lineskip)
	this->LoadedFonts[F_Text].pTTFFont = pFont;
	this->LoadedFonts[F_Text].wLineSkipHeight = TTF_FontLineSkip(pFont) * 3/4;
	this->LoadedFonts[F_Text].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_Text].BackColor = White;
	this->LoadedFonts[F_Text].bAntiAlias = true;

	//Load hyperlink font.
	pFont = GetFont(wstrFontFilepath, 24);
	if (!pFont) return false;
	this->LoadedFonts[F_Hyperlink].pTTFFont = pFont;
	this->LoadedFonts[F_Hyperlink].wLineSkipHeight = TTF_FontLineSkip(pFont);
	this->LoadedFonts[F_Hyperlink].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_Hyperlink].ForeColor = Blue;
	this->LoadedFonts[F_Hyperlink].BackColor = LightBrown;
	this->LoadedFonts[F_Hyperlink].bAntiAlias = true;

	this->LoadedFonts[F_TextHyperlink].pTTFFont = pFont;
	this->LoadedFonts[F_TextHyperlink].wLineSkipHeight = TTF_FontLineSkip(pFont) * 3/4;
	this->LoadedFonts[F_TextHyperlink].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TextHyperlink].ForeColor = Blue;
	this->LoadedFonts[F_TextHyperlink].BackColor = White;
	this->LoadedFonts[F_TextHyperlink].bAntiAlias = true;

	//Active hyperlink -- must be same size and shape as normal
	this->LoadedFonts[F_ActiveHyperlink].pTTFFont = pFont;
	this->LoadedFonts[F_ActiveHyperlink].wLineSkipHeight = TTF_FontLineSkip(pFont);
	this->LoadedFonts[F_ActiveHyperlink].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_ActiveHyperlink].ForeColor = DarkBlue;
	this->LoadedFonts[F_ActiveHyperlink].BackColor = LightBrown;
	this->LoadedFonts[F_ActiveHyperlink].bAntiAlias = true;

	this->LoadedFonts[F_TextActiveHyperlink].pTTFFont = pFont;
	this->LoadedFonts[F_TextActiveHyperlink].wLineSkipHeight = TTF_FontLineSkip(pFont) * 3/4;
	this->LoadedFonts[F_TextActiveHyperlink].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TextActiveHyperlink].ForeColor = DarkBlue;
	this->LoadedFonts[F_TextActiveHyperlink].BackColor = White;
	this->LoadedFonts[F_TextActiveHyperlink].bAntiAlias = true;

	//Load external hyperlink font.
	this->LoadedFonts[F_ExtHyperlink].pTTFFont = pFont;
	this->LoadedFonts[F_ExtHyperlink].wLineSkipHeight = TTF_FontLineSkip(pFont);
	this->LoadedFonts[F_ExtHyperlink].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_ExtHyperlink].ForeColor = FullRed;
	this->LoadedFonts[F_ExtHyperlink].BackColor = LightBrown;
	this->LoadedFonts[F_ExtHyperlink].bAntiAlias = true;

	this->LoadedFonts[F_TextExtHyperlink].pTTFFont = pFont;
	this->LoadedFonts[F_TextExtHyperlink].wLineSkipHeight = TTF_FontLineSkip(pFont) * 3/4;
	this->LoadedFonts[F_TextExtHyperlink].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TextExtHyperlink].ForeColor = FullRed;
	this->LoadedFonts[F_TextExtHyperlink].BackColor = White;
	this->LoadedFonts[F_TextExtHyperlink].bAntiAlias = true;

	//Active external hyperlink -- must be same size and shape as normal
	this->LoadedFonts[F_TextActiveExtHyperlink].pTTFFont = pFont;
	this->LoadedFonts[F_TextActiveExtHyperlink].wLineSkipHeight = TTF_FontLineSkip(pFont) * 3/4;
	this->LoadedFonts[F_TextActiveExtHyperlink].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TextActiveExtHyperlink].ForeColor = Maroon;
	this->LoadedFonts[F_TextActiveExtHyperlink].BackColor = White;
	this->LoadedFonts[F_TextActiveExtHyperlink].bAntiAlias = true;

	//Load bold font.
	pFont = GetFont(wstrFontFilepath, 24, TTF_STYLE_BOLD);
	if (!pFont) return false;
	this->LoadedFonts[F_TextBold].pTTFFont = pFont;
	this->LoadedFonts[F_TextBold].wLineSkipHeight = TTF_FontLineSkip(pFont) * 3/4;
	this->LoadedFonts[F_TextBold].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TextBold].BackColor = White;
	this->LoadedFonts[F_TextBold].bAntiAlias = true;

	//Load italic font.
	pFont = GetFont(wstrFontFilepath, 24, TTF_STYLE_ITALIC);
	if (!pFont) return false;
	this->LoadedFonts[F_TextItalic].pTTFFont = pFont;
	this->LoadedFonts[F_TextItalic].wLineSkipHeight = TTF_FontLineSkip(pFont) * 3/4;
	this->LoadedFonts[F_TextItalic].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TextItalic].BackColor = White;
	this->LoadedFonts[F_TextItalic].bAntiAlias = true;

	//Load underlined font.
	pFont = GetFont(wstrFontFilepath, 24, TTF_STYLE_UNDERLINE);
	if (!pFont) return false;
	this->LoadedFonts[F_TextUnderline].pTTFFont = pFont;
	this->LoadedFonts[F_TextUnderline].wLineSkipHeight = TTF_FontLineSkip(pFont) * 3/4;
	this->LoadedFonts[F_TextUnderline].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TextUnderline].BackColor = White;
	this->LoadedFonts[F_TextUnderline].bAntiAlias = true;

	//Load H3 font.
	pFont = GetFont(wstrFontFilepath, 29, TTF_STYLE_BOLD);
	if (!pFont) return false;
	this->LoadedFonts[F_TextHead3].pTTFFont = pFont;
	this->LoadedFonts[F_TextHead3].wLineSkipHeight = TTF_FontLineSkip(pFont) * 3/2;
	this->LoadedFonts[F_TextHead3].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TextHead3].BackColor = White;
	this->LoadedFonts[F_TextHead3].bAntiAlias = true;
	this->LoadedFonts[F_TextHead3].bUnderline = true;

	//Load item multiplier font.
	this->LoadedFonts[F_ItemMultiplier].pTTFFont = pFont;
	this->LoadedFonts[F_ItemMultiplier].wLineSkipHeight = TTF_FontLineSkip(pFont);
	this->LoadedFonts[F_ItemMultiplier].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_ItemMultiplier].ForeColor = Yellow;
	this->LoadedFonts[F_ItemMultiplier].BackColor = White;
	this->LoadedFonts[F_ItemMultiplier].bAntiAlias = true;
	this->LoadedFonts[F_ItemMultiplier].bOutline = true;
	this->LoadedFonts[F_ItemMultiplier].OutlineColor = Black;
	this->LoadedFonts[F_ItemMultiplier].wOutlineWidth = 1;

	this->LoadedFonts[F_ScoreTotal].pTTFFont = pFont;
	this->LoadedFonts[F_ScoreTotal].wLineSkipHeight = TTF_FontLineSkip(pFont);
	GetWordWidth(F_ScoreTotal, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_ScoreTotal].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_ScoreTotal].BackColor = LightBrown;
	this->LoadedFonts[F_ScoreTotal].bAntiAlias = true;

	//Load small font.
	pFont = GetFont(wstrFontFilepath, 19);
	if (!pFont) return false;
	this->LoadedFonts[F_Small].pTTFFont = pFont;
	this->LoadedFonts[F_Small].wLineSkipHeight = 21;
	this->LoadedFonts[F_Small].wSpaceWidth = 4;
	this->LoadedFonts[F_Small].BackColor = LightBrown;
	this->LoadedFonts[F_Small].bAntiAlias = true;

	//Load header font.
	pFont = GetFont(wstrFontFilepath, 22, TTF_STYLE_BOLD);
	if (!pFont) return false;
	this->LoadedFonts[F_Header].pTTFFont = pFont;
	this->LoadedFonts[F_Header].wLineSkipHeight = 24;
	this->LoadedFonts[F_Header].wSpaceWidth = 6;
	this->LoadedFonts[F_Header].BackColor = LightBrown;
	this->LoadedFonts[F_Header].bAntiAlias = true;

	pFont = GetFont(wstrFontFilepath, 22, TTF_STYLE_BOLD);
	if (!pFont) return false;
	this->LoadedFonts[F_HeaderWhite].pTTFFont = pFont;
	this->LoadedFonts[F_HeaderWhite].wLineSkipHeight = 24;
	this->LoadedFonts[F_HeaderWhite].wSpaceWidth = 6;
	this->LoadedFonts[F_HeaderWhite].ForeColor = White;
	this->LoadedFonts[F_HeaderWhite].BackColor = LightBrown;
	this->LoadedFonts[F_HeaderWhite].bAntiAlias = true;

	//Load button font.
	pFont = GetFont(wstrFontFilepath, 21);
	if (!pFont) return false;
	this->LoadedFonts[F_Button].pTTFFont = pFont;
	this->LoadedFonts[F_Button].wLineSkipHeight = TTF_FontLineSkip(pFont);
	GetWordWidth(F_Button, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_Button].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_Button].BackColor = LightBrown;
	this->LoadedFonts[F_Button].bAntiAlias = true;

	//Load button font (white text).
	pFont = GetFont(wstrFontFilepath, 21);
	if (!pFont) return false;
	this->LoadedFonts[F_ButtonWhite].pTTFFont = pFont;
	this->LoadedFonts[F_ButtonWhite].wLineSkipHeight = this->LoadedFonts[F_Button].wLineSkipHeight;
	GetWordWidth(F_ButtonWhite, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_ButtonWhite].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_ButtonWhite].ForeColor = White;
	this->LoadedFonts[F_ButtonWhite].bAntiAlias = true;

	//Load disabled button font.
	pFont = GetFont(wstrFontFilepath, 21);
	if (!pFont) return false;
	this->LoadedFonts[F_Button_Disabled].pTTFFont = pFont;
	this->LoadedFonts[F_Button_Disabled].wLineSkipHeight = TTF_FontLineSkip(pFont);
	GetWordWidth(F_Button_Disabled, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_Button_Disabled].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_Button_Disabled].ForeColor = DarkBrown;
	this->LoadedFonts[F_Button_Disabled].BackColor = LightBrown;
	this->LoadedFonts[F_Button_Disabled].bAntiAlias = true;

	//Load title font.
	pFont = GetFont(wstrFont2Filepath, 38);
	if (!pFont) return false;
	this->LoadedFonts[F_Title].pTTFFont = pFont;
	this->LoadedFonts[F_Title].wLineSkipHeight = TTF_FontLineSkip(pFont);
	this->LoadedFonts[F_Title].BackColor = MediumBrown;
	this->LoadedFonts[F_Title].bAntiAlias = true;
	GetWordWidth(F_Title, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_Title].wSpaceWidth = wSpaceWidth;

	//Load frame caption font.
	pFont = GetFont(wstrFontFilepath, 21);
	if (!pFont) return false;
	this->LoadedFonts[F_FrameCaption].pTTFFont = pFont;
	this->LoadedFonts[F_FrameCaption].wLineSkipHeight = TTF_FontLineSkip(pFont);
	GetWordWidth(F_FrameCaption, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_FrameCaption].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_FrameCaption].BackColor = MediumBrown;
	this->LoadedFonts[F_FrameCaption].bAntiAlias = true;

	//Load scroll font.
	pFont = GetFont(wstrFontFilepath, 22);
	if (!pFont) return false;
	this->LoadedFonts[F_Scroll].pTTFFont = pFont;
	this->LoadedFonts[F_Scroll].wLineSkipHeight = 20;
	this->LoadedFonts[F_Scroll].wSpaceWidth = 3;
	this->LoadedFonts[F_Scroll].BackColor = BlueishWhite;
	this->LoadedFonts[F_Scroll].bAntiAlias = true;

	//Load title marquee fonts
	this->LoadedFonts[F_TitleMarquee].pTTFFont = pFont;
	this->LoadedFonts[F_TitleMarquee].wLineSkipHeight = TTF_FontLineSkip(pFont) * 3/4;
	this->LoadedFonts[F_TitleMarquee].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TitleMarquee].ForeColor = AlmostWhite;
	this->LoadedFonts[F_TitleMarquee].BackColor = DarkGray;
	this->LoadedFonts[F_TitleMarquee].bAntiAlias = true;
	this->LoadedFonts[F_TitleMarquee].bOutline = true;
	this->LoadedFonts[F_TitleMarquee].OutlineColor = Black;
	this->LoadedFonts[F_TitleMarquee].wOutlineWidth = 2;
	
	this->LoadedFonts[F_TitleMarqueeHyperlink].pTTFFont = pFont;
	this->LoadedFonts[F_TitleMarqueeHyperlink].wLineSkipHeight = TTF_FontLineSkip(pFont) * 3/4;
	this->LoadedFonts[F_TitleMarqueeHyperlink].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TitleMarqueeHyperlink].ForeColor = LightBlue;
	this->LoadedFonts[F_TitleMarqueeHyperlink].BackColor = DarkGray;
	this->LoadedFonts[F_TitleMarqueeHyperlink].bAntiAlias = true;
	this->LoadedFonts[F_TitleMarqueeHyperlink].bOutline = true;
	this->LoadedFonts[F_TitleMarqueeHyperlink].OutlineColor = Black;
	this->LoadedFonts[F_TitleMarqueeHyperlink].wOutlineWidth = 2;

	this->LoadedFonts[F_TitleMarqueeActiveHyperlink].pTTFFont = pFont;
	this->LoadedFonts[F_TitleMarqueeActiveHyperlink].wLineSkipHeight = TTF_FontLineSkip(pFont) * 3/4;
	this->LoadedFonts[F_TitleMarqueeActiveHyperlink].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TitleMarqueeActiveHyperlink].ForeColor = LightYellow;
	this->LoadedFonts[F_TitleMarqueeActiveHyperlink].BackColor = DarkGray;
	this->LoadedFonts[F_TitleMarqueeActiveHyperlink].bAntiAlias = true;
	this->LoadedFonts[F_TitleMarqueeActiveHyperlink].bOutline = true;
	this->LoadedFonts[F_TitleMarqueeActiveHyperlink].OutlineColor = Black;
	this->LoadedFonts[F_TitleMarqueeActiveHyperlink].wOutlineWidth = 2;

	//Load sign font.
	pFont = GetFont(wstrFontFilepath, 26);
	if (!pFont) return false;
	this->LoadedFonts[F_Sign].pTTFFont = pFont;
	this->LoadedFonts[F_Sign].wLineSkipHeight = TTF_FontLineSkip(pFont);
	GetWordWidth(F_Sign, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_Sign].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_Sign].BackColor = BlueishWhite;
	this->LoadedFonts[F_Sign].bAntiAlias = true;

	//Load list box item font.
	pFont = GetFont(wstrFontFilepath, 19);
	if (!pFont) return false;
	this->LoadedFonts[F_ListBoxItem].pTTFFont = pFont;
	this->LoadedFonts[F_ListBoxItem].wLineSkipHeight = CY_LBOX_ITEM;
	this->LoadedFonts[F_ListBoxItem].wSpaceWidth = 8;
	this->LoadedFonts[F_ListBoxItem].BackColor = PinkishWhite;
	this->LoadedFonts[F_ListBoxItem].bAntiAlias = true;

	//Inventory slot item text.
	this->LoadedFonts[F_InvSlotText].pTTFFont = pFont;
	this->LoadedFonts[F_InvSlotText].wLineSkipHeight = CY_LBOX_ITEM;
	this->LoadedFonts[F_InvSlotText].wSpaceWidth = 8;
	this->LoadedFonts[F_InvSlotText].BackColor = InvSlotBG;
	this->LoadedFonts[F_InvSlotText].bAntiAlias = true;
	

	//Load selected list box item font.
	pFont = GetFont(wstrFontFilepath, 19);
	if (!pFont) return false;
	this->LoadedFonts[F_SelectedListBoxItem].pTTFFont = pFont;
	this->LoadedFonts[F_SelectedListBoxItem].wLineSkipHeight = CY_LBOX_ITEM;
	this->LoadedFonts[F_SelectedListBoxItem].wSpaceWidth = this->LoadedFonts[F_ListBoxItem].wSpaceWidth;
	this->LoadedFonts[F_SelectedListBoxItem].BackColor = MediumBrown;
	this->LoadedFonts[F_SelectedListBoxItem].bAntiAlias = true;

	//Load flashing message font.
	pFont = GetFont(wstrFont2Filepath, 64);
	if (!pFont) return false;
	this->LoadedFonts[F_FlashMessage].pTTFFont = pFont;
	this->LoadedFonts[F_FlashMessage].wLineSkipHeight = GetFontHeight(F_FlashMessage);
	GetWordWidth(F_FlashMessage, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_FlashMessage].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_FlashMessage].ForeColor = Yellow;
	this->LoadedFonts[F_FlashMessage].BackColor = PinkishWhite;
	this->LoadedFonts[F_FlashMessage].bOutline = true;
	this->LoadedFonts[F_FlashMessage].OutlineColor = Black;
	this->LoadedFonts[F_FlashMessage].wOutlineWidth = 2;
	this->LoadedFonts[F_FlashMessage].bAntiAlias = true;

	//Load level name font.
	pFont = GetFont(wstrFontFilepath, 51);
	if (!pFont) return false;
	this->LoadedFonts[F_LevelName].pTTFFont = pFont;
	this->LoadedFonts[F_LevelName].wLineSkipHeight = GetFontHeight(F_LevelName);
	GetWordWidth(F_LevelName, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_LevelName].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_LevelName].ForeColor = Yellow;
	this->LoadedFonts[F_LevelName].BackColor = MidYellow;
	this->LoadedFonts[F_LevelName].bAntiAlias = true;

	//Load level info font.
	pFont = GetFont(wstrFontFilepath, 26);
	if (!pFont) return false;
	this->LoadedFonts[F_LevelInfo].pTTFFont = pFont;
	this->LoadedFonts[F_LevelInfo].wLineSkipHeight = GetFontHeight(F_LevelInfo);
	GetWordWidth(F_LevelInfo, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_LevelInfo].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_LevelInfo].ForeColor = Gold;
	this->LoadedFonts[F_LevelInfo].BackColor = DarkYellow;
	this->LoadedFonts[F_LevelInfo].bAntiAlias = true;

	//Load level description font.
	pFont = GetFont(wstrFontFilepath, 22);
	if (!pFont) return false;
	this->LoadedFonts[F_LevelDescription].pTTFFont = pFont;
	this->LoadedFonts[F_LevelDescription].wLineSkipHeight = 24;
	this->LoadedFonts[F_LevelDescription].wSpaceWidth = 5;
	this->LoadedFonts[F_LevelDescription].ForeColor = White;
	this->LoadedFonts[F_LevelDescription].BackColor = Black;
	this->LoadedFonts[F_LevelDescription].bAntiAlias = true;

	//Load credits header font.
	pFont = GetFont(wstrFont2Filepath, 62);
	if (!pFont) return false;
	this->LoadedFonts[F_CreditsHeader].pTTFFont = pFont;
	this->LoadedFonts[F_CreditsHeader].wLineSkipHeight = GetFontHeight(F_CreditsHeader);
	GetWordWidth(F_CreditsHeader, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_CreditsHeader].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_CreditsHeader].ForeColor = Yellow;
	this->LoadedFonts[F_CreditsHeader].BackColor = Gray;
	this->LoadedFonts[F_CreditsHeader].bAntiAlias = true;

	//Load credits subheader font.
	pFont = GetFont(wstrFont2Filepath, 54);
	if (!pFont) return false;
	this->LoadedFonts[F_CreditsSubheader].pTTFFont = pFont;
	this->LoadedFonts[F_CreditsSubheader].wLineSkipHeight = GetFontHeight(F_CreditsSubheader);
	GetWordWidth(F_CreditsSubheader, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_CreditsSubheader].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_CreditsSubheader].ForeColor = Gold;
	this->LoadedFonts[F_CreditsSubheader].BackColor = Gray;
	this->LoadedFonts[F_CreditsSubheader].bAntiAlias = true;

	//Load credits text font.
	pFont = GetFont(wstrFont2Filepath, 42);
	if (!pFont) return false;
	this->LoadedFonts[F_CreditsText].pTTFFont = pFont;
	this->LoadedFonts[F_CreditsText].wLineSkipHeight = GetFontHeight(F_CreditsText);
	GetWordWidth(F_CreditsText, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_CreditsText].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_CreditsText].ForeColor = White;
	this->LoadedFonts[F_CreditsText].BackColor = Gray;
	this->LoadedFonts[F_CreditsText].bAntiAlias = true;

	//Load frame rate font.
	pFont = GetFont(wstrFont2Filepath, 42);
	if (!pFont) return false;
	this->LoadedFonts[F_FrameRate].pTTFFont = pFont;
	this->LoadedFonts[F_FrameRate].wLineSkipHeight = GetFontHeight(F_FrameRate);
	GetWordWidth(F_FrameRate, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_FrameRate].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_FrameRate].ForeColor = Yellow;
	this->LoadedFonts[F_FrameRate].BackColor = Black;
	this->LoadedFonts[F_FrameRate].bAntiAlias = true;
	this->LoadedFonts[F_FrameRate].bOutline = true;

	//Load stats font.
	pFont = GetFont(wstrFont2Filepath, 54);
	if (!pFont) return false;
	this->LoadedFonts[F_Stats].pTTFFont = pFont;
	this->LoadedFonts[F_Stats].wLineSkipHeight = GetFontHeight(F_Stats);
	this->LoadedFonts[F_Stats].ForeColor = Yellow;
	this->LoadedFonts[F_Stats].BackColor = Black;
	this->LoadedFonts[F_Stats].bAntiAlias = true;
	this->LoadedFonts[F_Stats].bOutline = true;
	this->LoadedFonts[F_Stats].OutlineColor = Black;
	this->LoadedFonts[F_Stats].wOutlineWidth = 1;
	GetWordWidth(F_Stats, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_Stats].wSpaceWidth = wSpaceWidth;

	//Load title menu font.
	pFont = GetFont(wstrFontFilepath, 38);
	if (!pFont) return false;
	this->LoadedFonts[F_TitleMenu].pTTFFont = pFont;
	this->LoadedFonts[F_TitleMenu].wLineSkipHeight = TTF_FontLineSkip(pFont) * 9/10;
	this->LoadedFonts[F_TitleMenu].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TitleMenu].ForeColor = Gold;
	this->LoadedFonts[F_TitleMenu].BackColor = DarkGray;
	this->LoadedFonts[F_TitleMenu].bAntiAlias = true;
	this->LoadedFonts[F_TitleMenu].bOutline = true;
	this->LoadedFonts[F_TitleMenu].OutlineColor = Black;
	this->LoadedFonts[F_TitleMenu].wOutlineWidth = 2;

	this->LoadedFonts[F_TitleMenuActive].pTTFFont = pFont;
	this->LoadedFonts[F_TitleMenuActive].wLineSkipHeight = TTF_FontLineSkip(pFont) * 9/10;
	this->LoadedFonts[F_TitleMenuActive].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TitleMenuActive].ForeColor = LightYellow;
	this->LoadedFonts[F_TitleMenuActive].BackColor = DarkGray;
	this->LoadedFonts[F_TitleMenuActive].bAntiAlias = true;
	this->LoadedFonts[F_TitleMenuActive].bOutline = true;
	this->LoadedFonts[F_TitleMenuActive].OutlineColor = Black;
	this->LoadedFonts[F_TitleMenuActive].wOutlineWidth = 2;

	this->LoadedFonts[F_TitleMenuSelected].pTTFFont = pFont;
	this->LoadedFonts[F_TitleMenuSelected].wLineSkipHeight = TTF_FontLineSkip(pFont) * 9/10;
	this->LoadedFonts[F_TitleMenuSelected].wSpaceWidth = wSpaceWidth;
	this->LoadedFonts[F_TitleMenuSelected].ForeColor = AlmostWhite;
	this->LoadedFonts[F_TitleMenuSelected].BackColor = DarkGray;
	this->LoadedFonts[F_TitleMenuSelected].bAntiAlias = true;
	this->LoadedFonts[F_TitleMenuSelected].bOutline = true;
	this->LoadedFonts[F_TitleMenuSelected].OutlineColor = Black;
	this->LoadedFonts[F_TitleMenuSelected].wOutlineWidth = 2;

	//Load damage preview fonts.
	pFont = GetFont(wstrFont2Filepath, 14);
	if (!pFont) return false;
	this->LoadedFonts[F_DamagePreviewSmall].pTTFFont = pFont;
	this->LoadedFonts[F_DamagePreviewSmall].wLineSkipHeight = GetFontHeight(F_DamagePreviewSmall);
	this->LoadedFonts[F_DamagePreviewSmall].ForeColor = Yellow;
	this->LoadedFonts[F_DamagePreviewSmall].BackColor = Black;
	this->LoadedFonts[F_DamagePreviewSmall].bAntiAlias = true;
	this->LoadedFonts[F_DamagePreviewSmall].bOutline = true;
	this->LoadedFonts[F_DamagePreviewSmall].OutlineColor = Black;
	this->LoadedFonts[F_DamagePreviewSmall].wOutlineWidth = 1;
	GetWordWidth(F_DamagePreviewSmall, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_DamagePreviewSmall].wSpaceWidth = wSpaceWidth;

	pFont = GetFont(wstrFont2Filepath, 17);
	if (!pFont) return false;
	this->LoadedFonts[F_DamagePreviewMedium].pTTFFont = pFont;
	this->LoadedFonts[F_DamagePreviewMedium].wLineSkipHeight = GetFontHeight(F_DamagePreviewMedium);
	this->LoadedFonts[F_DamagePreviewMedium].ForeColor = Yellow;
	this->LoadedFonts[F_DamagePreviewMedium].BackColor = Black;
	this->LoadedFonts[F_DamagePreviewMedium].bAntiAlias = true;
	this->LoadedFonts[F_DamagePreviewMedium].bOutline = true;
	this->LoadedFonts[F_DamagePreviewMedium].OutlineColor = Black;
	this->LoadedFonts[F_DamagePreviewMedium].wOutlineWidth = 1;
	GetWordWidth(F_DamagePreviewMedium, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_DamagePreviewMedium].wSpaceWidth = wSpaceWidth;

	pFont = GetFont(wstrFont2Filepath, 20);
	if (!pFont) return false;
	this->LoadedFonts[F_DamagePreviewLarge].pTTFFont = pFont;
	this->LoadedFonts[F_DamagePreviewLarge].wLineSkipHeight = GetFontHeight(F_DamagePreviewLarge);
	this->LoadedFonts[F_DamagePreviewLarge].ForeColor = Yellow;
	this->LoadedFonts[F_DamagePreviewLarge].BackColor = Black;
	this->LoadedFonts[F_DamagePreviewLarge].bAntiAlias = true;
	this->LoadedFonts[F_DamagePreviewLarge].bOutline = true;
	this->LoadedFonts[F_DamagePreviewLarge].OutlineColor = Black;
	this->LoadedFonts[F_DamagePreviewLarge].wOutlineWidth = 2;
	GetWordWidth(F_DamagePreviewLarge, wszSpace, wSpaceWidth);
	this->LoadedFonts[F_DamagePreviewLarge].wSpaceWidth = wSpaceWidth;

	//Make sure all fonts were loaded.
#ifdef _DEBUG
	for (UINT wFontI = 0; wFontI < F_Last; ++wFontI)
	{
		ASSERT(this->LoadedFonts[wFontI].pTTFFont);
	}
#endif

	return true;
}

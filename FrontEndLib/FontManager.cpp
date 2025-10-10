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
 *
 * ***** END LICENSE BLOCK ***** */

#define INCLUDED_FROM_FONTMANAGER_CPP
#include "FontManager.h"
#undef INCLUDED_FROM_FONTMANAGER_CPP
#include "Screen.h"
#include "ListBoxWidget.h"
#include "BitmapManager.h"
#include "Colors.h"
#include "Outline.h"

#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>

//Holds the only instance of CFontManager for the app.
CFontManager *g_pTheFM = NULL;

static const UINT MAXLEN_WORD = 1024;

//pre-defined colors
const SDL_Color Black = {0, 0, 0, 0};
const SDL_Color DarkGray = {64, 64, 64, 0};
const SDL_Color DarkBrown = {138, 126, 103, 0};
const SDL_Color MediumBrown = {190, 181, 165, 0};
const SDL_Color LightGray = {180, 180, 180, 0};
const SDL_Color Gray = {128, 128, 128, 0};
const SDL_Color LightBrown = {205, 196, 179, 0};
const SDL_Color BlueishWhite = {229, 228, 245, 0};
const SDL_Color PinkishWhite = {232, 215, 219, 0};
const SDL_Color DarkYellow = {64, 64, 0, 0};
const SDL_Color MidYellow = {128, 128, 0, 0};
const SDL_Color Yellow = {255, 255, 0, 0};
const SDL_Color LightYellow = {255, 255, 96, 0};
const SDL_Color White = {255, 255, 255, 0};
const SDL_Color Maroon = {192, 0, 0, 0};
const SDL_Color Gold = {227, 171, 4, 0};
const SDL_Color Blue = {0, 0, 255, 0};
const SDL_Color DarkBlue = {0, 0, 128, 0};
const SDL_Color LightBlue = {96, 96, 255, 0};
const SDL_Color LightCyan = {128, 255, 255, 0};
const SDL_Color FullRed = {255, 0, 0, 0};
const SDL_Color LightRed = {255, 128, 128, 0};
const SDL_Color LightBlue2 = {164, 164, 255, 0};
const SDL_Color DarkGreen = {0, 128, 0, 0};
const SDL_Color AlmostWhite = {255, 254, 255, 0}; //needed for black outlining to work


//
//Public methods.
//
//*********************************************************************************
CFontManager::CFontManager()
	: vFontCache(0)   //init empty font cache
	, LoadedFonts(NULL)
	, pColorMapSurface(NULL)
//Constructor.
{
}

//*********************************************************************************
CFontManager::~CFontManager()
//Destructor.
{
	for (UINT wFontI=this->vFontCache.size(); wFontI--; )
	{
		TTF_CloseFont(this->vFontCache[wFontI].pFont);
		delete this->vFontCache[wFontI].pBuffer;
	}

	if (this->pColorMapSurface) SDL_FreeSurface(this->pColorMapSurface);

	delete[] this->LoadedFonts;
}

//*********************************************************************************
UINT CFontManager::AdjustTextRectHeight(const UINT eFontType, const UINT wH) const
//Adjusts indicated text height by the difference in real font's actual height
//and the imposed line height.  (I think...)
{
	const int nHeightDelta =
		TTF_FontHeight(this->LoadedFonts[eFontType].pTTFFont)
			- (int)GetFontLineHeight(eFontType);
	return wH + (nHeightDelta > 0 ? nHeightDelta : 0);
}

//*********************************************************************************
void CFontManager::DrawTextXY(
//Draw a line of text to a surface.
//
//Params:
	const UINT eFontType,      //(in)   Indicates font and associated settings.
	const WCHAR *pwczText,     //(in)   Text to draw.
	SDL_Surface *pSurface,     //(in)   Dest surface.
	const int nX, const int nY,//(in)   Dest coords.
	const UINT wWidth, const UINT wHeight, //(in) optional size constraints.
	const Uint8 opacity)       //[default=255]
const
{
	if (pwczText[0] == L'\0') return; //Nothing to do.

	const LOADEDFONT *pFont = this->LoadedFonts+eFontType;
	ASSERT(pFont->pTTFFont);

	//Adjust drawing position for any spaces preceding the first word.
	const WCHAR *pwczSeek = pwczText;
	UINT wSpaceCount, wCRLFCount;
	pwczSeek = DrawText_SkipOverNonWord(pwczSeek, wSpaceCount, wCRLFCount);
	if (wCRLFCount)
		return; //Stop at CR, since only one line of text is being drawn.
	UINT xDraw = wSpaceCount * pFont->wSpaceWidth;

	//Each iteration draws one word to surface.
	SDL_Surface *pText = NULL;
	WCHAR wczWord[MAXLEN_WORD + 1];
	UINT wWordLen;
	while (*pwczSeek != '\0' && (!wWidth || xDraw < wWidth))
	{
		//Copy the next word into buffer.
		pwczSeek = DrawText_CopyNextWord(pwczSeek, wczWord, wWordLen);
		pwczSeek = DrawText_SkipOverNonWord(pwczSeek, wSpaceCount, wCRLFCount);

		//Render the word.
		pText = RenderWord(eFontType, wczWord);
		if (!pText) {ASSERT(!"Failed to render word."); return;}

		//Blit word to dest surface (clip if needed).
		const UINT width = wWidth ? wWidth - xDraw : pText->w;
		const UINT height = wHeight ? wHeight : pText->h;
		SDL_Rect src = MAKE_SDL_RECT(0, 0, width, height);
		SDL_Rect dest = MAKE_SDL_RECT(nX + xDraw, nY, width, height);
		if (opacity < 255)
			EnableSurfaceBlending(pText, opacity);
		SDL_BlitSurface(pText, &src, pSurface, &dest);

		xDraw += pText->w;
		SDL_FreeSurface(pText);

		if (wCRLFCount)
			return;  //Stop at CR, since only one line of text is being drawn.

		//Adjust drawing position for spaces found after word.
		xDraw += pFont->wSpaceWidth * wSpaceCount;
	}
}

//*********************************************************************************
WSTRING CFontManager::CalcPartialWord(
//Determine how much of a rendered word will fit within a certain width.
//
//Params:
	const UINT eFontType,      //(in)   Indicates font and associated settings.
	WCHAR *wczWord,            //(in)   Word to render.
	const UINT wWordLen,       //(in)   # chars in word.
	const int nMaxWidth,       //(in)   Allotted width.
	UINT &wCharsNotDrawn)      //(out)  The number of chars that still need to be drawn.
const
{
	WSTRING wstr;
	UINT wCounter = 0;
	UINT wWidth = 0;
	// Find out how many chars will fit in the allotted space
	if (nMaxWidth > 0)
	{
		while (wWidth < static_cast<UINT>(nMaxWidth) && wCounter < wWordLen)
		{
			wstr += wczWord[wCounter++];
			GetWordWidth(eFontType, wstr.c_str(), wWidth);
		}
		if (wCounter == wWordLen && wWidth <= static_cast<UINT>(nMaxWidth))
		{
			//Everything fit.
			wCharsNotDrawn = 0;
			return wstr;
		}
	}
	if (wstr.size() == 0)
	{
		//No chars fit.
		wCharsNotDrawn = wWordLen;
		return wstr;
	}
	// Get rid of the last three characters, then add a dash
	if (wstr.size() > 3) {
		wCounter -= 3;
		wstr.erase(wstr.end()-3, wstr.end());
		wstr += wszHyphen[0];
	}

	wCharsNotDrawn = wWordLen - wCounter;

	return wstr;
}

//*********************************************************************************
void CFontManager::DrawPartialWord(
//Blit as much of a word as will fit within a rectangle on a surface.
//
//Pre-Cond: there is sufficient height in the rectangle to render the word.
//
//Params:
	const UINT eFontType,      //(in)   Indicates font and associated settings.
	WCHAR *wczWord,            //(in)   Word to render.
	const UINT wWordLen,       //(in)   # chars in word.
	const int xDraw, const int yDraw,//(in)   Coord to start drawing word at.
	const UINT wXLimit,        //(in)   Dest rectangle x-pixel limit.
	SDL_Surface *pSurface,     //(in)   Dest surface.
	UINT &wCharsNotDrawn,      //(out)  The number of chars that still need to be drawn.
	const Uint8 opacity)       //(in)   Transparency [default=255]
const
{
	WSTRING wstr = CalcPartialWord(eFontType, wczWord, wWordLen,
			(int)wXLimit-xDraw, wCharsNotDrawn);
	if (wstr.size() == 0) return; //Nothing to render.

	//Render the word.
	SDL_Surface *pText = RenderWord(eFontType, wstr.c_str());
	if (!pText) {ASSERT(!"Failed to render word.(2)"); return;}

	//Blit word to dest surface (clip if needed).
	const UINT width = wXLimit ? wXLimit : pText->w;
	const UINT height = pText->h;
	SDL_Rect src = MAKE_SDL_RECT(0, 0, width, height);
	SDL_Rect dest = MAKE_SDL_RECT(xDraw, yDraw, width, height);
	if (opacity < 255)
		EnableSurfaceBlending(pText, opacity);
	SDL_BlitSurface(pText, &src, pSurface, &dest);

	SDL_FreeSurface(pText);
}

//*********************************************************************************
void CFontManager::DrawTextToRect(
//Draw text within a rectangle on a surface.
//
//Params:
	const UINT eFontType,    //(in) Indicates font and associated settings.
	const WCHAR *pwczText,   //(in) Text to draw.
	int nX, int nY,          //(in) Dest coords.
	UINT wW, UINT wH,        //(in) Dest width and height to draw within.
	SDL_Surface *pSurface,   //(in) Dest surface.
	const UINT wFirstIndent, //(in) First line pixel indent [default=0].
	const Uint8 opacity,     //(in) Transparency [default=255].
	const bool bPrintLeadingSpaces) //(in) On lines after the first one [default=false]
const
{
	const LOADEDFONT *pFont = this->LoadedFonts+eFontType;
	ASSERT(pFont->pTTFFont);
	if (pFont->wLineSkipHeight > wH) return; //No room to display any text.

	if (WCSlen(pwczText) == 0) return; //Nothing to do.

	//Adjust drawing position for any spaces and CRLFs preceding the first word.
	int xDraw = nX, yDraw = nY;
	if (wFirstIndent < wW)
		xDraw += wFirstIndent;
	else
		yDraw += pFont->wLineSkipHeight;
	const WCHAR *pwczSeek = pwczText;
	UINT wSpaceCount, wCRLFCount;
	pwczSeek = DrawText_SkipOverNonWord(pwczSeek, wSpaceCount, wCRLFCount);
	if (wCRLFCount) {
		yDraw += (wCRLFCount * pFont->wLineSkipHeight);
	} else {
		xDraw += (wSpaceCount * pFont->wSpaceWidth);
	}

	//Each iteration draws one word to surface.
	SDL_Surface *pText = NULL;
	WCHAR wczWord[MAXLEN_WORD + 1];
	UINT wWordLen;
	UINT wSpaceLength = 0;
	while (yDraw + (int)pFont->wLineSkipHeight <= (int)(nY + wH) && *pwczSeek != '\0')
	{
		//Copy the next word into buffer.
		pwczSeek = DrawText_CopyNextWord(pwczSeek, wczWord, wWordLen);

		//Render the text.
		if (pText) SDL_FreeSurface(pText);
		pText = RenderWord(eFontType, wczWord);
		if (!pText) {ASSERT(!"Failed to render word.(3)"); return;}

		//Does rendered text fit horizontally in rect after drawing point?
		if (xDraw + pText->w <= static_cast<int>(nX + wW))
		{
			//Rendered text fits.
			//Blit word to dest surface.
			SDL_Rect src = MAKE_SDL_RECT(0, 0, pText->w, pText->h);
			SDL_Rect dest = MAKE_SDL_RECT(xDraw, yDraw, pText->w, pText->h);
			if (opacity < 255)
				EnableSurfaceBlending(pText, opacity);
			SDL_BlitSurface(pText, &src, pSurface, &dest);

			//Draw underline for underlined fonts.
			if (pFont->bUnderline)
			{
				dest.y += pText->h * 6/7;  //raise into letter "drop" area
				dest.h = 1;
				dest.x -= wSpaceLength;    //underline any preceding whitespace
				dest.w += wSpaceLength;
				//!!doesn't work with alpha 'opacity' set
				SDL_FillRect(pSurface, &dest, SDL_MapRGB(pSurface->format,
						pFont->ForeColor.r, pFont->ForeColor.g, pFont->ForeColor.b));
			}

			xDraw += pText->w;
		} else {
			//Would the text fit horizontally at the beginning of a row?
			if (static_cast<UINT>(pText->w) > wW) //No.
			{
				//Moving down to a new row won't help draw this text.  So
				//draw it char-by-char until one char doesn't fit.
				UINT wNumCharsLeft;
				DrawPartialWord(eFontType, wczWord, wWordLen, xDraw, yDraw, nX + wW,
					pSurface, wNumCharsLeft);
				pwczSeek -= wNumCharsLeft;

				//Jump down to next row.
				xDraw = nX;
				yDraw += pFont->wLineSkipHeight;
				continue;   //don't need to check for whitespace
			} //...Text won't fit on a row by itself.
			else
			{
				//Blit word to dest surface on next row,
				//if another row will fit in rectangle.
				if (yDraw + (int)pFont->wLineSkipHeight * 2 <= (int)(nY + wH))
				{
					xDraw = nX;
					yDraw += pFont->wLineSkipHeight;
					SDL_Rect src = MAKE_SDL_RECT(0, 0, pText->w, pText->h);
					SDL_Rect dest = MAKE_SDL_RECT(xDraw, yDraw, pText->w, pText->h);
					if (opacity < 255)
						EnableSurfaceBlending(pText, opacity);
					SDL_BlitSurface(pText, &src, pSurface, &dest);

					xDraw += pText->w;
				} else {
					//Otherwise, just draw as much as will fit on this (last) row.
					UINT wNumCharsLeft;
					DrawPartialWord(eFontType, wczWord, wWordLen, xDraw, yDraw,
							nX + wW, pSurface, wNumCharsLeft, opacity);

					xDraw = nX;
					yDraw += pFont->wLineSkipHeight;
				}
			}
		}

		//Adjust drawing position for spaces and CRLFs found after word.
		pwczSeek = DrawText_SkipOverNonWord(pwczSeek, wSpaceCount, wCRLFCount, bPrintLeadingSpaces); //retain spaces after CRLF?
		if (wCRLFCount)
		{
			xDraw = nX;
			yDraw += (pFont->wLineSkipHeight * wCRLFCount);
			wSpaceLength = 0;

			if (bPrintLeadingSpaces) {
				do {
					pwczSeek = DrawText_SkipOverNonWord(pwczSeek, wSpaceCount, wCRLFCount, true);
					if (wCRLFCount) {
						yDraw += (pFont->wLineSkipHeight * wCRLFCount);
					}
				} while (wCRLFCount); //move over completely blank lines

				//leading spaces
				wSpaceLength = pFont->wSpaceWidth * wSpaceCount;
				xDraw += wSpaceLength;
			}
		} else {
			wSpaceLength = pFont->wSpaceWidth * wSpaceCount;
			xDraw += wSpaceLength;
		}
	} //...while yDraw is not past the rect.

	if (pText) SDL_FreeSurface(pText);
}

//*********************************************************************************
void CFontManager::DrawHotkeyTextToLine(
//Draw text with letter(s) highlighted, representing hotkeys.
//
//Params:
	const UINT eFontType,         //(in)   Indicates font and associated settings.
	const WCHAR *pwczText,  //(in)   Text to draw.
	int nX, int nY,               //(in)   Dest coords.
	UINT wW,                      //(in)   Dest width to draw within.
	SDL_Surface *pSurface,  //(in)   Dest surface.
	const Uint8 opacity, //[default=255]
	const int eHotkeyFont) //[default=-1]
const
{
	LOADEDFONT *pFont = this->LoadedFonts+eFontType;
	LOADEDFONT *pHotkeyFont = eHotkeyFont >= 0 ? this->LoadedFonts+eHotkeyFont : NULL;
	const SDL_Color foreColor = pFont->ForeColor;
	const SDL_Color hotkeyColor = pHotkeyFont ? pHotkeyFont->ForeColor : Maroon;
	ASSERT(pFont->pTTFFont);
	SDL_Surface *pText = NULL;
	const UINT wStrLen = WCSlen(pwczText);
	int xDraw = nX, yDraw = nY;
	bool bHotkey = false;   //whether next character is a hotkey and should be highlighted

	WCHAR *const wczChars = new WCHAR[wStrLen + 1];
	UINT wNumChars = 0;  //consecutive chars w/o hotkey

	//Render all chars between hotkeys at once.
	//Render hotkeys separately.
	for (UINT wCharI = 0; wCharI < wStrLen; ++wCharI)
	{
		if (pwczText[wCharI] == '&')
		{
			bHotkey = true;
			WCv(wczChars[wNumChars]) = '\0';
		} else {
			wczChars[wNumChars++] = pwczText[wCharI];

			//Set hotkey char to highlighted color.
			if (bHotkey)
			{
				pFont->ForeColor = hotkeyColor;
				WCv(wczChars[wNumChars]) = '\0';
			} else {
				if (wCharI < wStrLen-1)
					continue;   //don't draw anything yet
				WCv(wczChars[wNumChars]) = '\0'; //at end of text -- draw everything left
			}
		}

		if (wNumChars == 0)
			continue;   //nothing to render

		//Render chars.
		pText = RenderWord(eFontType, wczChars);
		if (!pText) {ASSERT(!"Failed to render word.(4)"); return;}

		//Is char past right bound?
		const UINT width = (int)(xDraw + pText->w) <= (int)(nX + wW) ?
				pText->w : ((int)(nX + wW) >= xDraw ? nX + wW - xDraw : 0);

		//Blit char to dest surface.
		if (width > 0)
		{
			SDL_Rect src = MAKE_SDL_RECT(0, 0, width, pText->h);
			SDL_Rect dest = MAKE_SDL_RECT(xDraw, yDraw, width, pText->h);
			if (opacity < 255)
				EnableSurfaceBlending(pText, opacity);
			SDL_BlitSurface(pText, &src, pSurface, &dest);
			xDraw += pText->w;
		}

		SDL_FreeSurface(pText);

		if (bHotkey && pwczText[wCharI-1] == '&')
		{
			bHotkey = false;
			pFont->ForeColor = foreColor; //back to original color
		}

		if (!width) //no more space to render
			break;

		wNumChars = 0; //ready to collect chars for next string snippet
	}

	delete[] wczChars;
}

//*********************************************************************************
int CFontManager::GetHotkeyTextLineWidth(
//Calculate width of a text with hotkey.
//
//Params:
	const UINT eFontType,  //(in)   Indicates font and associated settings.
	const WCHAR *pwczText) //(in)   Text to draw.
const
{
	int xDraw = 0;

	LOADEDFONT *pFont = this->LoadedFonts+eFontType;
	ASSERT(pFont->pTTFFont);
	SDL_Surface *pText = NULL;
	const UINT wStrLen = WCSlen(pwczText);
	bool bHotkey = false;   //whether next character is a hotkey and should be highlighted

	WCHAR *const wczChars = new WCHAR[wStrLen + 1];
	UINT wNumChars = 0;  //consecutive chars w/o hotkey

	//Apply same rendering algorithm as in DrawHotkeyTextToLine().
	for (UINT wCharI = 0; wCharI < wStrLen; ++wCharI)
	{
		if (pwczText[wCharI] == '&')
		{
			bHotkey = true;
			WCv(wczChars[wNumChars]) = '\0';
		} else {
			wczChars[wNumChars++] = pwczText[wCharI];

			//Set hotkey char to highlighted color.
			if (bHotkey)
			{
				WCv(wczChars[wNumChars]) = '\0';
			} else {
				if (wCharI < wStrLen-1)
					continue;   //don't draw anything yet
				WCv(wczChars[wNumChars]) = '\0'; //at end of text -- draw everything left
			}
		}

		if (wNumChars == 0)
			continue;   //nothing to render

		pText = RenderWord(eFontType, wczChars);
		if (!pText) {ASSERT(!"Failed to render word.(4)"); return 0;}
		xDraw += pText->w;
		SDL_FreeSurface(pText);

		if (bHotkey && pwczText[wCharI-1] == '&')
			bHotkey = false;

		wNumChars = 0; //ready to collect chars for next string snippet
	}

	delete[] wczChars;

	return xDraw;
}

//*********************************************************************************
UINT CFontManager::GetTextRectHeight(
//Get height needed to draw text within a width.  The width of the longest line
//within the rect is also calculated.
//
//Params:
	const UINT eFontType,      //(in)   Indicates font and associated settings.
	const WCHAR *pwczText,  //(in)   Text to draw.
	UINT wW,          //(in)   Dest width to draw within.
	UINT &wLongestLineW, //(out)  Width of longest line.
	UINT &wH,            //(out)  Height needed to draw text within a width
	const UINT wFirstIndent)   //(in)   First line pixel indent (optional).
//
//Returns:
//The width of the last line.
const
{
	const LOADEDFONT *pFont = this->LoadedFonts+eFontType;
	ASSERT(pFont->pTTFFont);

	//Adjust drawing position for any spaces and CRLFs preceding the first word.
	UINT xDraw = wFirstIndent, yDraw = 0;
	if (wFirstIndent >= wW)
	{
		xDraw = 0;
		yDraw = pFont->wLineSkipHeight;
	}
	const WCHAR *pwczSeek = pwczText;
	UINT wSpaceCount, wCRLFCount;
	pwczSeek = DrawText_SkipOverNonWord(pwczSeek, wSpaceCount, wCRLFCount);
	if (wCRLFCount)
		yDraw += (wCRLFCount * pFont->wLineSkipHeight);
	else
		xDraw += (wSpaceCount * pFont->wSpaceWidth);

	//Each iteration draws one word to surface.
	wLongestLineW = 0;
	SDL_Surface *pText = NULL;
	WCHAR wczWord[MAXLEN_WORD + 1];
	UINT wWordLen;
	while (*pwczSeek != '\0')
	{
		//Copy the next word into buffer.
		pwczSeek = DrawText_CopyNextWord(pwczSeek, wczWord, wWordLen);

		//Render the text.
		if (pText) SDL_FreeSurface(pText);
		pText = RenderWord(eFontType, wczWord, true);
		if (!pText) {ASSERT(!"Failed to render word.(5)"); return 0L;}

		//Does rendered text fit horizontally in rect after drawing point?
		if (xDraw + pText->w > wW) //No.
		{
			//Would the text fit horizontally at the beginning of a row?
			if (static_cast<UINT>(pText->w) > wW) //No.
			{
				//Moving down to a new row won't help draw this text.  So
				//draw it char-by-char until one char doesn't fit.
				UINT wCharI;
				for (wCharI = 0; wCharI < wWordLen; ++wCharI)
				{
					static WCHAR wczChar[2] = { wczWord[wCharI], We(0) };

					//Render the char.
					if (pText) SDL_FreeSurface(pText);
					pText = RenderWord(eFontType, wczChar, true);
					if (!pText) {ASSERTP(false, "Failed to render word.(6)"); return 0L;}

					//Is char past right bound?
					if (xDraw + pText->w > wW) //Yes.
						break;
					else
						xDraw += pText->w;
				}
				//Render the rest on the next line.
				pwczSeek -= wWordLen - wCharI + 1;

				//Jump down to next row.
				if (xDraw > wLongestLineW) wLongestLineW = xDraw;
				xDraw = 0;
				yDraw += pFont->wLineSkipHeight;
				continue;   //don't need to check for whitespace
			} //...Text won't fit on a row by itself.
			else
			{
				//Move down to next row.
				if (xDraw > wLongestLineW) wLongestLineW = xDraw;
				xDraw = pText->w;
				yDraw += pFont->wLineSkipHeight;
			}
		} //...Rendered text does not fit horizontally within rect.
		else
			//Rendered text fits.
			xDraw += pText->w;

		//Adjust drawing position for spaces and CRLFs found after word.
		pwczSeek = DrawText_SkipOverNonWord(pwczSeek, wSpaceCount, wCRLFCount);
		if (wCRLFCount)
		{
			if (xDraw > wLongestLineW) wLongestLineW = xDraw;
			xDraw = 0;
			yDraw += (pFont->wLineSkipHeight * wCRLFCount);
		}
		else
			xDraw += pFont->wSpaceWidth * wSpaceCount;
	} //...while yDraw is not past the rect.

	if (pText) SDL_FreeSurface(pText);

	if (xDraw > wLongestLineW) wLongestLineW = xDraw;
	if (wLongestLineW > wW) wLongestLineW = wW; //Sometimes it's a few pixels over.

	wH = yDraw + pFont->wLineSkipHeight;

	return xDraw;
}

//*****************************************************************************
void CFontManager::GetTextWidthHeight(
//Get width and height of a line of rendered text.
//
//Params:
	const UINT eFontType,      //(in)   Indicates font and associated settings.
	const WCHAR *pwczText,  //(in)   Text to draw.
	UINT &wW, UINT &wH)     //(out)  Width and height of the text.
const
{
	//Empty text has no dimensions.
	if (WCSlen(pwczText)==0) {wW = 0; wH = 0; return;}

	const LOADEDFONT *pFont = this->LoadedFonts+eFontType;
	ASSERT(pFont->pTTFFont);

	wH = pFont->wLineSkipHeight;

	//Adjust drawing position for any spaces preceding the first word.
	const WCHAR *pwczSeek = pwczText;
	UINT wSpaceCount, wCRLFCount;
	pwczSeek = DrawText_SkipOverNonWord(pwczSeek, wSpaceCount, wCRLFCount);
	ASSERT(!wCRLFCount); //shouldn't have any CRs in a single line of text
	if (wCRLFCount) return; //Stop at CR, since only one line of text is being drawn.
	wW = wSpaceCount * pFont->wSpaceWidth;

	//Each iteration draws one word to surface.
	SDL_Surface *pText = NULL;
	WCHAR wczWord[MAXLEN_WORD + 1];
	UINT wWordLen;
	while (*pwczSeek != '\0')
	{
		//Copy the next word into buffer.
		pwczSeek = DrawText_CopyNextWord(pwczSeek, wczWord, wWordLen);
		pwczSeek = DrawText_SkipOverNonWord(pwczSeek, wSpaceCount, wCRLFCount);

		//Render the word.
		pText = RenderWord(eFontType, wczWord, true);
		if (!pText) {ASSERT(!"Failed to render word.(7)"); return;}

		wW += pText->w;
		SDL_FreeSurface(pText);

		if (wCRLFCount)
			return;  //Stop at CR, since only one line of text is being drawn.

		//Adjust drawing position for spaces found after word.
		wW += pFont->wSpaceWidth * wSpaceCount;
	}
}

//*****************************************************************************
void CFontManager::GetTextWidth(
//Get width of a line of rendered text.
//
//Params:
	const UINT eFontType,      //(in)   Indicates font and associated settings.
	const WCHAR *wczText,   //(in)   Text to draw.
	UINT &wW)      //(out)  Width of the text.
const
{
	wW = 0;
	const WCHAR *pwczSeek = wczText;

	//Render spaces then words.
	const LOADEDFONT *pFont = this->LoadedFonts+eFontType;
	SDL_Surface *pText = NULL;
	WCHAR wczWord[MAXLEN_WORD + 1];
	UINT wWordLen;
	while (*pwczSeek != '\0')
	{
		//Get spaces before next word.
		UINT wSpaceCount, wCRLFCount;
		pwczSeek = DrawText_SkipOverNonWord(pwczSeek, wSpaceCount, wCRLFCount);
		wW += (wSpaceCount * pFont->wSpaceWidth);
		if (*pwczSeek == '\0')
			break;

		//Copy the next word into buffer.
		pwczSeek = DrawText_CopyNextWord(pwczSeek, wczWord, wWordLen);

		//Render the text.
		pText = RenderWord(eFontType, wczWord, true);
		if (!pText) {ASSERT(!"Failed to render word."); return;}
		wW += pText->w;
		SDL_FreeSurface(pText);
	}
}

//*****************************************************************************
UINT CFontManager::GetCharsThatFitWithin(
//Get width of a line of rendered text.
//
//Params:
	const UINT eFontType,   //(in)   Indicates font and associated settings.
	const WCHAR *wczText,   //(in)   Text to draw.
	const UINT wWidth)      //(out)  Width of the text.
const
{
	UINT wW = 0, wLastFittingIndex = 0, wNewIndex;
	const WCHAR *pwczSeek = wczText;
	const UINT length = WCSlen(wczText);

	//Render spaces then words.
	const LOADEDFONT *pFont = this->LoadedFonts+eFontType;
	WCHAR wczWord[MAXLEN_WORD + 1];
	UINT wWordLen;
	while (wLastFittingIndex < length)
	{
		//Get initial spaces.
		UINT wSpaceCount, wCRLFCount;
		pwczSeek = DrawText_SkipOverNonWord(pwczSeek, wSpaceCount, wCRLFCount);
		wW += (wSpaceCount * pFont->wSpaceWidth);

		//If whitespace fills up to the end of the area, then return to its end
		//because we don't need to show whitespace on the new line.
		wNewIndex = UINT(pwczSeek - wczText);
		if (wW > wWidth || wCRLFCount)
			return wNewIndex;
		wLastFittingIndex = wNewIndex;

		if (wLastFittingIndex == length)
			break;

		//Copy the next word into buffer.
		pwczSeek = DrawText_CopyNextWord(pwczSeek, wczWord, wWordLen);

		//Render the text.
		SDL_Surface *pText = RenderWord(eFontType, wczWord, true);
		if (!pText) {ASSERT(!"Failed to render word."); return wW;}
		wW += pText->w;
		SDL_FreeSurface(pText);

		wNewIndex = UINT(pwczSeek - wczText);
		if (wW > wWidth)
			return wLastFittingIndex > 0 ? wLastFittingIndex :
					wNewIndex; //!!fix: if entire word filled up entire space, this just says it fit, but we should probably break it up at the location where it overflowed
		wLastFittingIndex = wNewIndex;
	}
	return wLastFittingIndex;
}

//*****************************************************************************
void CFontManager::GetWordWidth(
//Get width of a word of rendered text.
//NOTE: Call this to initialize font spacing width.
//
//Params:
	const UINT eFontType,      //(in)   Indicates font and associated settings.
	const WCHAR *wczWord,   //(in)   Text to draw.
	UINT &wW)      //(out)  Width of the text.
const
{
   //Render the word.
	SDL_Surface *pText = RenderWord(eFontType, wczWord, true);
	if (!pText) {ASSERT(!"Failed to render word.(8)."); return;}

	wW = pText->w;
	SDL_FreeSurface(pText);
}

//*****************************************************************************
UINT CFontManager::GetFontHeight(
//Gets height of a font, not including line skip space.
//
//Params:
	const UINT eFontType)      //(in)   Indicates font and associated settings.
//
//Returns:
//The height.
const
{
	return TTF_FontHeight(this->LoadedFonts[eFontType].pTTFFont);
}

//*****************************************************************************
SDL_Surface * CFontManager::RenderWord(
//Renders text to a surface.  Uses rendering options associated with a font type.
//
//Params:
	const UINT eFontType,      //(in)   Font to use.
	const WCHAR *pwczText,  //(in)   Text to render.
	const bool bRenderFast) //(in)   Render fast, overriding anti-aliasing if
									//    needed (default = false)
//
//Returns:
//Surface with rendered text or NULL if an error occurred.
const
{
	SDL_Surface *pText = NULL;

	ASSERT(WCSlen(pwczText));

	const LOADEDFONT *pFont = this->LoadedFonts+eFontType;
	ASSERT(pFont->pTTFFont);

	//Draw the text.
	if (pFont->bAntiAlias && pFont->bOutline) {
		TTF_SetFontOutline(pFont->pTTFFont, pFont->wOutlineWidth);
		pText = TTF_RenderUNICODE_Blended(pFont->pTTFFont, reinterpret_cast<const Uint16*>(pwczText), pFont->OutlineColor);
		DisableSurfaceBlending(pText); // ?
		SDL_SetSurfaceRLE(pText, 1);
		TTF_SetFontOutline(pFont->pTTFFont, 0);
		SDL_Surface* p2 = TTF_RenderUNICODE_Blended(pFont->pTTFFont, reinterpret_cast<const Uint16*>(pwczText), pFont->ForeColor);
		SDL_Rect dest = MAKE_SDL_RECT(pFont->wOutlineWidth, pFont->wOutlineWidth - 1, pText->w, pText->h);
		g_pTheBM->BlitRGBAtoRGBA(p2, NULL, pText, &dest);
		SDL_FreeSurface(p2);
	} else if (pFont->bAntiAlias && !bRenderFast) {
		pText = TTF_RenderUNICODE_Shaded(pFont->pTTFFont, reinterpret_cast<const Uint16*>(pwczText),
				pFont->ForeColor, pFont->BackColor);
		if (!pText) return NULL;

		//Set background color used for anti-aliasing to be transparent.
		Uint32 TransparentColor = SDL_MapRGB(pText->format, pFont->BackColor.r,
					pFont->BackColor.g, pFont->BackColor.b);
		SetColorKey(pText, SDL_TRUE, TransparentColor);
	} else {
		//Note that next call will set a color key for non-text pixels in surface.
		//This color key may be used by AddOutline().
		pText = TTF_RenderUNICODE_Solid(pFont->pTTFFont, reinterpret_cast<const Uint16*>(pwczText), pFont->ForeColor);
		if (!pText) return NULL;
	}

	//Outline the text if specified for font.
	const bool draw_outline = pFont->bOutline && !bRenderFast && !pFont->bAntiAlias;
	if (draw_outline)
	{
		ASSERT(pFont->wOutlineWidth > 0);
		AddOutline(pText, pFont->OutlineColor, pFont->wOutlineWidth);
	}

	return pText;
}

//*********************************************************************************
TTF_Font* CFontManager::GetFont(
//Returns: a pointer either to a previously cached font or a newly loaded
//font from a call to TTF_OpenFont().
//
//Params:
	WSTRING const &wstrFilename, const UINT pointsize, const int style)
{
	FontCacheMember FCM;
	TTF_Font *pFont;

	//Search for font in font cache.
	for (UINT nIndex=this->vFontCache.size(); nIndex--; )
	{
		FCM = this->vFontCache[nIndex];
		if (FCM.wPointsize == pointsize && FCM.nStyle == style
				&& FCM.wsFilename == wstrFilename)
			return FCM.pFont; //found identical font -- return it
	}

	//Font not found in cache -- load it.
	CStretchyBuffer* pFontBuffer = new CStretchyBuffer;
	if (!CFiles::ReadFileIntoBuffer(wstrFilename.c_str(), *pFontBuffer,true))
	{
		delete pFontBuffer;
		return NULL;	//load failed
	}
	pFont = TTF_OpenFontRW(SDL_RWFromMem((BYTE*)*pFontBuffer, pFontBuffer->Size()), 1, pointsize);

	if (style != TTF_STYLE_NORMAL)
		TTF_SetFontStyle(pFont, style);

#if (TTF_MAJOR_VERSION >= 2) && ((TTF_MINOR_VERSION > 0) || (TTF_PATCHLEVEL >= 10))
	TTF_SetFontHinting(pFont, TTF_HINTING_LIGHT);
#endif

	//Add font and relevant characteristics to cache.
	FCM.wsFilename = wstrFilename;
	FCM.wPointsize = pointsize;
	FCM.nStyle = style;
	FCM.pFont = pFont;
	FCM.pBuffer = pFontBuffer;
	this->vFontCache.push_back(FCM);
	return pFont;
}

//*********************************************************************************
const WCHAR *CFontManager::DrawText_CopyNextWord(
//Copy one word into word buffer.
//
//Params:
	const WCHAR *pwczStart, //(in)   Place to begin copying word from.
	WCHAR *pwczWord,     //(out)  Word.
	UINT &wWordLen)         //(out)  Length of word.
//
//Returns:
//Read position after word.
const
{
	const WCHAR *pwczSeek = pwczStart;

	//Copy word into buffer.
	wWordLen = 0;
	WCHAR *pwczWrite = pwczWord;
	while (*pwczSeek != '\0' && *pwczSeek != ' ' && *pwczSeek != '\r'
#ifndef WIN32
		&& *pwczSeek != '\n'
#endif
		&& *pwczSeek != '-')
	{
		++wWordLen;
		if (wWordLen > MAXLEN_WORD)  //Stop copying after max length.
			*pwczSeek++;
		else
			*(pwczWrite++) = *(pwczSeek++);
	}
	if (*pwczSeek == '-') {
		*(pwczWrite++) = *(pwczSeek++);
	}
	ASSERT(pwczWrite != pwczWord); //Bad call.
	pWCv(pwczWrite) = '\0';

	return pwczSeek;
}

//*********************************************************************************
const WCHAR *CFontManager::DrawText_SkipOverNonWord(
//Skip over non-word chars.
//
//Params:
	const WCHAR *pwczStart, //(in)   Place to begin reading from.
	UINT &wSpaceCount,   //(out)  Number of space chars.
	UINT &wCRLFCount,    //(out)  Number of CRLF pairs.
	bool bStopAtCRLF)    //(in)   If a CRLF is encountered, tally it and return immediately [default=false]
//
//Returns:
//Read position after non-word chars.
const
{
	const WCHAR *pwczSeek = pwczStart;

	//Seek past non-word chars and count spaces and CRLFs.
	wSpaceCount = 0;
	wCRLFCount = 0;
	while (*pwczSeek == ' ' || *pwczSeek == '\r' || *pwczSeek == '\n')
	{
		if (*pwczSeek == ' ')
			++wSpaceCount;
#ifdef WIN32
		if (*pwczSeek == '\r')
			++wCRLFCount;
#else
		if (*pwczSeek == '\n' || (*pwczSeek == '\r' && *(pwczSeek+1) != '\n'))
			++wCRLFCount;
#endif
		++pwczSeek;

		if (wCRLFCount && bStopAtCRLF)
			break;
	}

	return pwczSeek;
}

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

#include "HTMLWidget.h"
#include "BitmapManager.h"
#include "LabelWidget.h"
#include "HyperLinkWidget.h"
#include "FontManager.h"
#include "ScrollableWidget.h"
#include "ImageWidget.h"
#include <BackEndLib/Files.h>
#include <BackEndLib/Ports.h>  // stricmp

//*****************************************************************************
static const WCHAR wszHREF[] = {We('h'),We('r'),We('e'),We('f'),We(0)};
static const UINT LISTINDENT = 42;

//*****************************************************************************
//
//Expat callback entrypoints (private)
//

void StartElement_cb (
	void *pHTMLWidget,   // (in) Pointer to HTML Widget
	const XML_Char *name, const XML_Char **atts)
{
	((CHTMLWidget*)pHTMLWidget)->StartElement(name, atts);
}

void InElement_cb (
	void *pHTMLWidget,   // (in) Pointer to HTML Widget
	const XML_Char* s, int len)
{
	((CHTMLWidget*)pHTMLWidget)->InElement(s, len);
}

void EndElement_cb (
	void *pHTMLWidget,   // (in) Pointer to HTML Widget
	const XML_Char* name)
{
	((CHTMLWidget*)pHTMLWidget)->EndElement(name);
}

//*****************************************************************************
//
//Public methods.
//

//*****************************************************************************
CHTMLWidget::CHTMLWidget(
	const UINT dwSetTagNo, const int nSetX, const int nSetY, const UINT wSetW,
	const UINT wSetH, const UINT wSetMargin, const WCHAR *pwczFileName,
	const UINT dwSetLinkTagStart)
	: CWidget(WT_HTML, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, nOffsetX(0), nOffsetY(0), bNeedRepaint(false)
	, pHTMLSurface(NULL), wMargin(wSetMargin), dwLinkTagStart(dwSetLinkTagStart)
{
	ASSERT(this->w - this->wMargin * 2 > 0);

	if (pwczFileName)
		this->wstrIndexPage = pwczFileName;
}

//*****************************************************************************
CHTMLWidget::~CHTMLWidget()
{
	if (this->pHTMLSurface)
		SDL_FreeSurface(this->pHTMLSurface);
}

//*****************************************************************************
void CHTMLWidget::GetChildClippingRect(
//Children draw themselves to a surface sized to hold the whole page.
//
//Params:
	SDL_Rect &ChildClipRect)   //(out) the area this widget wants its children clipped to
const
{
	//When drawing to the browser's internal surface, children are clipped
	//directly to it.  When the children's coordinates are being considered
	//externally, however, they must be relative to this widget itself.
	if (GetDestSurface() == this->pHTMLSurface)
	{
		//Clip to pHTMLSurface
		ChildClipRect.x = 0;
		ChildClipRect.y = 0;
		ChildClipRect.w = this->pHTMLSurface->w;
		ChildClipRect.h = this->pHTMLSurface->h;
	}
	else pParent->GetChildClippingRect(ChildClipRect);
}

//*****************************************************************************
bool CHTMLWidget::GoBack(
//Go back n pages.
//
//Params:
	UINT wCount)   //(in)   Number of pages to skip.
//
//Returns: true if the new page loaded/parsed successfully.
{
	if (this->swstrBackLink.empty()) return false;

	GetScrollOffset(this->nOffsetX, this->nOffsetY);
	SHTMLPos sNewPage;
	sNewPage.wstrLink = this->wstrCurrentPage;
	sNewPage.wY = -this->nOffsetY;

	while (wCount-- && !this->swstrBackLink.empty())
	{
		if (!sNewPage.wstrLink.empty())
			this->swstrForwardLink.push(sNewPage);
		sNewPage = this->swstrBackLink.top();
		this->swstrBackLink.pop();
	}

	this->wstrCurrentPage = wszEmpty;
	return LoadFile(sNewPage.wstrLink.c_str(), false, sNewPage.wY);
}

//*****************************************************************************
bool CHTMLWidget::GoForward(
//Go forward n pages.
//
//Params:
	UINT wCount)   //(in)   Number of pages to skip.
//
//Returns: true if the new page loaded/parsed successfully.
{
	if (this->swstrForwardLink.empty()) return false;

	GetScrollOffset(this->nOffsetX, this->nOffsetY);
	SHTMLPos sNewPage;
	sNewPage.wstrLink = this->wstrCurrentPage;
	sNewPage.wY = -this->nOffsetY;

	while (wCount-- && !this->swstrForwardLink.empty())
	{
		if (!sNewPage.wstrLink.empty())
			this->swstrBackLink.push(sNewPage);
		sNewPage = this->swstrForwardLink.top();
		this->swstrForwardLink.pop();
	}

	return LoadFile(sNewPage.wstrLink.c_str(), false, sNewPage.wY);
}

//*****************************************************************************
bool CHTMLWidget::GoHome()
//Go to specified index page.
{
	return LoadFile();
}

//*****************************************************************************
bool CHTMLWidget::LoadFile(
//Load a file and parse it.
//
//Params:
	const WCHAR *pwczFileName,    //(in)   File to load, or NULL to load index.
	bool bNewLink,                //(in)   Whether page is new (not from the
	                              //       forward/back lists).
	UINT wScrollY)                //(in)   Pixels to scroll down page initially
	                              //       (ignored if the filename contains a
	                              //        valid in-page anchor reference)
//
//Returns: whether page load was successful.
{
	bool bFullPath = false;

	if (!pwczFileName)
		pwczFileName = this->wstrIndexPage.c_str();  //default = index page
	if (!WCSlen(pwczFileName)) return false;  //empty name

	WSTRING wNewPage = pwczFileName;
	WSTRING wGoToId;
	UINT refpos = (UINT)-1;

	//Very simple path finder
	for (UINT i = WCSlen(pwczFileName); i--;)
	{
		if (pwczFileName[i] == SLASH)
		{
			bFullPath = true;
			this->wstrBasePath = wNewPage.substr(0, i);
			break;
		}
		else if (pwczFileName[i] == '#')
		{
			refpos = i;
			wGoToId = &pwczFileName[i+1];
		}
	}

	if (refpos != (UINT)-1)
		wNewPage = refpos ? wNewPage.substr(0, refpos) : this->wstrCurrentPage;
	if (refpos && !bFullPath)
		wNewPage = this->wstrBasePath + wszSlash + wNewPage;

	if (bNewLink)
	{
		//Add previous page to back list.  Clear forward history.
		GetScrollOffset(this->nOffsetX, this->nOffsetY);
		SHTMLPos prev;
		prev.wstrLink = this->wstrCurrentPage;
		prev.wY = -this->nOffsetY;
		if (!this->wstrCurrentPage.empty())
			this->swstrBackLink.push(prev);
		while (!swstrForwardLink.empty())
			this->swstrForwardLink.pop();
	}

	//Only load if page requested isn't already being shown.
	if (WCScmp(wNewPage.c_str(), this->wstrCurrentPage.c_str()))
	{
		CStretchyBuffer text;
		CFiles files;
		if (!files.ReadFileIntoBuffer(wNewPage.c_str(), text))
			return false;

		this->wstrCurrentPage = wNewPage;
		this->wstrCurrentId = wGoToId;

		VERIFY(Parse(text));  //parse should be successful, but return true in any case
	}

	//Scroll to id or specified Y-position, if possible
	if (this->pParent && this->pParent->GetType() == WT_Scrollable)
	{
		if (!wGoToId.empty())
		{
			HTMLIDMAP::const_iterator idit = mIdmap.find(wGoToId);
			if (idit != mIdmap.end())
				wScrollY = (*idit).second;
		}

		CScrollableWidget *pContainer = DYN_CAST(CScrollableWidget*, CWidget*, this->pParent);
		pContainer->ScrollAbsolute(0, 0);
		pContainer->ScrollDownPixels(wScrollY);
	}

	return true;
}

//*****************************************************************************
void CHTMLWidget::Paint(
//Paint text inside of the widget area.
//
//Params:
	bool bUpdateRect)       //(in)   If true (default) and destination
								//    surface is the screen, the screen
								//    will be immediately updated in
								//    the widget's rect.
{
	if (this->pHTMLSurface)
	{
		//Prepare for blitting the surface during idle animation.
		//Wait until entire page is painted before repainting it.
		if (this->wHeightDrawn >= (UINT)this->pHTMLSurface->h)
		{
			bNeedRepaint = false;
			this->wHeightDrawn = 0;
			GetScrollOffset(this->nOffsetX, this->nOffsetY);
		}
		else
			bNeedRepaint = true;
	} else {
		PaintChildren(); //No surface; slow render mode.
		if (bUpdateRect) UpdateRect();
	}
}

//*****************************************************************************
void CHTMLWidget::HandleAnimate()
//Draw as much of page surface as can be done this frame without slowing animation.
{
	ASSERT(IsVisible());
	if (!this->pHTMLSurface)
		return;
	if (this->wHeightDrawn >= (UINT)this->pHTMLSurface->h)
		return; //all done
	SDL_Surface *pDestSurface = GetDestSurface();

	//Blit horizontal strips of HTML surface to screen until animation duration is up.
	static const UINT dwMaxTime = 30;  //30ms
	const UINT dwStopTime = SDL_GetTicks() + dwMaxTime;

	ASSERT(this->pParent);
	SDL_Rect clipRect;
	this->pParent->GetChildClippingRect(clipRect);
	SDL_SetClipRect(pDestSurface, &clipRect);
	do {
		static const UINT wStripHeight = 64;
		SDL_Rect src = MAKE_SDL_RECT(-this->nOffsetX, -this->nOffsetY + this->wHeightDrawn,
				this->pHTMLSurface->w, wStripHeight);
		SDL_Rect dest = MAKE_SDL_RECT(this->x, this->y + this->wHeightDrawn, this->pHTMLSurface->w, wStripHeight);

		//Bounds check on last row.
		if (this->wHeightDrawn + wStripHeight > (UINT)this->pHTMLSurface->h)
			src.h = dest.h = this->pHTMLSurface->h - this->wHeightDrawn;
		if (dest.y + dest.h >= clipRect.y + clipRect.h)
		{
			//Now rendering past the visible rect -- stop drawing the page after this,
			//or repaint if dirty flag is set.
			src.h = dest.h = (clipRect.y + clipRect.h) - dest.y;
			if (bNeedRepaint)
			{
				bNeedRepaint = false;
				this->wHeightDrawn = -dest.h;
				GetScrollOffset(this->nOffsetX, this->nOffsetY);
			}
			else
				this->wHeightDrawn = this->pHTMLSurface->h;
		}

		SDL_BlitSurface(this->pHTMLSurface, &src, pDestSurface, &dest);
		UpdateRect(dest);

		this->wHeightDrawn += dest.h;
		if (this->wHeightDrawn >= (UINT)this->pHTMLSurface->h) break; //all done
		if (this->wHeightDrawn >= this->h) break; //drawn entire visible area
	} while (SDL_GetTicks() < dwStopTime);
	SDL_SetClipRect(pDestSurface, NULL);
}

//*****************************************************************************
void CHTMLWidget::ScrollUp(const UINT wLines)   //[default=3]
{
	if (!this->pParent || this->pParent->GetType() != WT_Scrollable) return;

	//Scroll container.
	CScrollableWidget *pContainer = DYN_CAST(CScrollableWidget*, CWidget*, this->pParent);
	pContainer->ScrollUpOneLine(wLines);
	pContainer->RequestPaint();
}

//*****************************************************************************
void CHTMLWidget::ScrollUpPage()
{
	if (!this->pParent || this->pParent->GetType() != WT_Scrollable) return;

	//Scroll container.
	CScrollableWidget *pContainer = DYN_CAST(CScrollableWidget*, CWidget*, this->pParent);
	pContainer->ScrollUpOnePage();
	pContainer->RequestPaint();
}

//*****************************************************************************
void CHTMLWidget::ScrollDown(const UINT wLines)   //[default=3]
{
	if (!this->pParent || this->pParent->GetType() != WT_Scrollable) return;

	//Scroll container.
	CScrollableWidget *pContainer = DYN_CAST(CScrollableWidget*, CWidget*, this->pParent);
	pContainer->ScrollDownOneLine(wLines);
	pContainer->RequestPaint();
}

//*****************************************************************************
void CHTMLWidget::ScrollDownPage()
{
	if (!this->pParent || this->pParent->GetType() != WT_Scrollable) return;

	//Scroll container.
	CScrollableWidget *pContainer = DYN_CAST(CScrollableWidget*, CWidget*, this->pParent);
	pContainer->ScrollDownOnePage();
	pContainer->RequestPaint();
}

//*****************************************************************************
//
// Private methods
//

//*****************************************************************************
void CHTMLWidget::Flush(bool bInMargin)
//Flush the output buffer.
{
	if ((aeTagLevel[UL_Tag] || aeTagLevel[OL_Tag]) && !aeTagLevel[LI_Tag])
	{
		wstrBuffer = wszEmpty;
		return;
	}

	if (aeTagLevel[TITLE_Tag])
	{
		wstrTitle = wstrBuffer;
	}
	else
	{
		CLabelWidget *pText;

		if (bInMargin)
			this->wX = 0;

		if (aeTagLevel[A_Tag] && !swstrLink.empty() && !swstrLink.top().empty())
		{
			UINT eFontType = FONTLIB::F_TextHyperlink;
			UINT eFontType2 = FONTLIB::F_TextActiveHyperlink;
			static const WCHAR wszWorldLink[] =  {We('h'),We('t'),We('t'),We('p'),We(':'),We(0)};

			if (!swstrLink.top().compare(0, 5, wszWorldLink))
			{
				eFontType = FONTLIB::F_TextExtHyperlink;
				eFontType2 = FONTLIB::F_TextActiveExtHyperlink;
			}

			pText = new CHyperLinkWidget(this->dwCurrentLinkTag++, this->wMargin,
					this->wY, this->w - this->wMargin * 2, this->h,
					eFontType, eFontType2, wstrBuffer.c_str(),
					swstrLink.top().c_str(), true, this->wX);
		}
		else
		{
			UINT eFontType = FONTLIB::F_Text;
			if (aeTagLevel[H3_Tag])
				eFontType = FONTLIB::F_TextHead3;
			else if (aeTagLevel[B_Tag])
				eFontType = FONTLIB::F_TextBold;
			else if (aeTagLevel[I_Tag])
				eFontType = FONTLIB::F_TextItalic;
			else if (aeTagLevel[U_Tag])
				eFontType = FONTLIB::F_TextUnderline;

			// Plain text.
			pText = new CLabelWidget(0, this->wMargin, this->wY,
					this->w - this->wMargin * 2, this->h,
					eFontType, wstrBuffer.c_str(), true, this->wX);
		}

		UINT wW, wH;
		const UINT wLastW = pText->GetTextWidthHeight(wW, wH, this->wX);
		const UINT wLineH = g_pTheFM->GetFontLineHeight(pText->GetFontType());

		if (bInMargin)
			pText->Move(pText->GetX() - (int)wW, pText->GetY());
		else
		{
			this->wX = wLastW;
			this->wY += wH - wLineH;
			if (this->wX >= this->w - this->wMargin * 2)
			{
				this->wX = 0;
				this->wY += wLineH;
				if (this->wX >= this->w - this->wMargin * 2)
					this->wX = 0;
			}
		}

		AddWidget(pText);
	}

	wstrBuffer = wszEmpty;
}

//*****************************************************************************
WSTRING CHTMLWidget::GetAttr(
//Extract an attribute from a list provided by expat.
//
//Params:
	const WCHAR *attr,      //(in)   Attribute to find
	const XML_Char **atts)  //(in)   List of attributes
//
//Returns: Value of attribute if found, else empty string.
{
	WSTRING wstrValue = wszEmpty;

	for (UINT i = 0; attr[i / 2] && atts[i]; i += 2)
	{
		UINT j;
		for (j = 0; (static_cast<XML_Char>(WCv(attr[j])) == atts[i][j])
				&& atts[i][j] != 0; ++j) ;
		if (atts[i][j])
			continue;
		for (const XML_Char *attval = atts[i + 1]; *attval; ++attval)
		{
			WCHAR wattval = W_t(*attval);
			wstrValue += wattval;
		}
		break;
	}

	return wstrValue;
}

//*****************************************************************************
void CHTMLWidget::NewLine(
//Jumps to a new line
//
//Params:
	bool bAlways)  //(in)  Jump to new line even if the current line is empty ? [default=false]
{
	if (this->wX || bAlways)
	{
		this->wX = 0;
		this->wY += g_pTheFM->GetFontLineHeight(FONTLIB::F_Text);
	}
	this->bSkipSpace = true;
}

#ifdef RUSSIAN_BUILD
//*****************************************************************************
//AKELLA: To translate HTML pages to an XML encoding format the game will parse
void ConvertCyrillicEncodedXMLToUTF8(const CStretchyBuffer& b)
{
	const BYTE *pBytes = (const BYTE*)b;
	const UINT size = b.Size() / sizeof(char);
	string strOut;

	char temp[16];
	for (UINT index=0; index<size; ++index)
	{
		if (pBytes[index] < 128)
			strOut.append(1, pBytes[index]);
		else
		{
			strOut += "&#";
			strOut += _itoa(pBytes[index] + 848, temp, 10);
			strOut += ";";
		}
	}

	CFiles f;
	f.AppendErrorLog(strOut.c_str());
}
#endif

//*****************************************************************************
bool CHTMLWidget::Parse(
//Parse text buffer and add child widgets for each part.
//
//Params:
	const CStretchyBuffer &text)  //(in)   Text to parse.
//
//Returns: True if parse was successful, false if there was a parse error.
{
	//Renderer init.
	ClearChildren();
	this->wCurrentColumn = this->wX = 0;
	this->wY = this->wMargin;
	this->bSkipSpace = true;
	this->dwCurrentLinkTag = this->dwLinkTagStart;
	for (int i = First_Tag; i <= Tag_Count; ++i)  // Including unknown
		aeTagLevel[i] = 0;
	mIdmap.clear();

	static const WCHAR defaultColor[] = {We('F'),We('F'),We('F'),We('F'),We('F'),We('F'),We(0)};
	this->wstrBGColor = defaultColor;

	//Parser init.
	XML_Parser parser = XML_ParserCreate(NULL);
	XML_SetUserData(parser, this);
	XML_SetElementHandler(parser, StartElement_cb, EndElement_cb);
	XML_SetCharacterDataHandler(parser, InElement_cb);
// XML_SetParamEntityParsing(parser, XML_PARAM_ENTITY_PARSING_NEVER);

	bool bResult = true;
	char *buf = (char*)(BYTE*)text;
	//size - 2 seems to fix "invalid token" errors at EOF (even at empty files).
	const UINT size = text.Size() - 2;

	//Parse the XML.
	static const char entities[] = "<?xml version=\"1.0\" encoding=\""
   "UTF-8"
   "\"?>"
   "<!DOCTYPE html ["
	"<!ENTITY nbsp \"\xc2\xa0\">" //utf-8 for 0xa0 = unicode nbsp
   "]>";

	if ((XML_Parse(parser, entities, strlen(entities), false) == XML_STATUS_ERROR) ||
			(XML_Parse(parser, buf, size, true) == XML_STATUS_ERROR))
	{
		//Some problem occurred.
		char errorStr[256];
		_snprintf(errorStr, 256,
				"HTML Parse Error: %s at line %u:%u" NEWLINE,
				XML_ErrorString(XML_GetErrorCode(parser)),
				(UINT)XML_GetCurrentLineNumber(parser),
				(UINT)XML_GetCurrentColumnNumber(parser));
		CFiles Files;
		Files.AppendErrorLog((char *)errorStr);

#ifdef RUSSIAN_BUILD
		//Fix unsupported KOI8 encoded Cyrillic chars.
//		ConvertCyrillicEncodedXMLToUTF8(text);
#endif

		UTF8ToUnicode(errorStr, this->wstrStatus);

		bResult = false;
	}

	//Parser clean-up.
	XML_ParserFree(parser);

	NewLine();	//once done, ensure final line of text is included in height
	NewLine(true);  //...and have a blank line at the bottom to look good

	//Render surface to display on screen.
	bool bCanResize = CanResize();
	if (bCanResize)
	{
		this->pParent->ScrollAbsolute(0, 0);   //scroll to top of page
		if (this->wY < this->pParent->GetH())  //stretch surface to fill container
			this->wY = this->pParent->GetH();
	}
	UpdateHTMLSurface();

	//Resize (if possible).
	if (bCanResize)
		Resize(this->pHTMLSurface->w, this->pHTMLSurface->h);

	return bResult;
}

//*****************************************************************************
HTMLTagType CHTMLWidget::ParseTag(const char *str)
//Returns: enumeration corresponding to tag name
const
{
	static const char * tagStr[Tag_Count] = {
		"body", "title", "a", "b", "i", "u", "br", "h1", "h2", "h3", "hr",
		"img", "p", "ul", "ol", "li", "td", "tr", "table"
	};

	for (int eTag=First_Tag; eTag<Tag_Count; ++eTag)
		if (!_stricmp(str, tagStr[eTag])) //not case sensitive
			return (HTMLTagType)eTag;

	return Unknown_Tag;
}

//*****************************************************************************
inline BYTE getXdigit(const WCHAR xc)
//Returns: hex char converted into byte value
{
	ASSERT(iswxdigit(xc));
	if (iswdigit(xc))
		return WCv(xc) - '0';
	return 10 + (WCv(towlower(xc)) - 'a');
}

Uint32 CHTMLWidget::TranslateColor(
//Returns: SDL numeric color value
//
//Params:
	SDL_Surface *pSurface,  //(in)
	const WSTRING& wstr)  //string of hex RGB values (i.e. "ffffff")
const
{
	//Skip '#', or any other non-hex chars at front of string
	UINT wIndex = 0;
	while (!iswxdigit(wstr[wIndex]))
		++wIndex;

	//Convert hex chars to byte values.
	ASSERT(wstr.size() - wIndex == 6);
	const BYTE nR = getXdigit(wstr[wIndex]) * 16 + getXdigit(wstr[wIndex+1]);
	const BYTE nG = getXdigit(wstr[wIndex+2]) * 16 + getXdigit(wstr[wIndex+3]);
	const BYTE nB = getXdigit(wstr[wIndex+4]) * 16 + getXdigit(wstr[wIndex+5]);

	return SDL_MapRGB(pSurface->format, nR, nG, nB);
}

//*****************************************************************************
void CHTMLWidget::UpdateHTMLSurface()
//Updates the HTML surface.
{
	if (this->pHTMLSurface)
		SDL_FreeSurface(this->pHTMLSurface);

	//Create surface to fit the parsed HTML page.
	this->pHTMLSurface = CBitmapManager::ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, this->w, this->wY, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	if (this->pHTMLSurface)
	{
		//Set surface BG color to designated page color.
		const Uint32 wBGColor = TranslateColor(this->pHTMLSurface, this->wstrBGColor);
		SDL_FillRect(this->pHTMLSurface, NULL, wBGColor);

		//Draw children on the correct spot on the HTML surface.
		SDL_Surface *pDestSurface = GetDestSurface();
		SetDestSurface(this->pHTMLSurface);
		Scroll(-this->x, -this->y);
		PaintChildren();
		Scroll(this->x, this->y);
		SetDestSurface(pDestSurface);

		//Prepare for paint
		this->wHeightDrawn = (UINT)this->pHTMLSurface->h;
		this->bNeedRepaint = false;
	}
	else
	{
		//Couldn't create surface; report error and use slow rendering mode
		char errorStr[256];
		_snprintf(errorStr, 256,
				"Error creating HTML surface: %s",
				SDL_GetError());
		CFiles Files;
		Files.AppendErrorLog((char *)errorStr);
	}
}

//*****************************************************************************
//
// Expat callback functions
//

//*****************************************************************************
void CHTMLWidget::StartElement(
//Expat callback function: Process XML start tag, and attributes.
//
//Params:
	const XML_Char *name, const XML_Char **atts)
{
	//Get tag type (assume no special chars).
	const HTMLTagType eTagType = ParseTag(name);

	if (wstrBuffer.length()) Flush();

	++aeTagLevel[eTagType];

	//Get id/name attribute
	{
		static const WCHAR wszID[] = {We('i'),We('d'),We(0)};
		static const WCHAR wszNAME[] = {We('n'),We('a'),We('m'),We('e'),We(0)};
		WSTRING tmp = GetAttr(wszID, atts);
		if (tmp.empty())
			tmp = GetAttr(wszNAME, atts);
		if (!tmp.empty())
			mIdmap.insert(std::make_pair(tmp, this->wY));
	}

	switch (eTagType)
	{
		case BODY_Tag:
		{
			static const WCHAR wszBgcolor[] = {We('b'),We('g'),We('c'),We('o'),We('l'),We('o'),We('r'),We(0)};
			WSTRING bgcolor = GetAttr(wszBgcolor, atts);
			if (bgcolor.size())
				this->wstrBGColor = bgcolor;
			break;
		}
		case UL_Tag:
		case OL_Tag:
			this->swOLstack.push(this->wOLcounter);
			this->wOLcounter = (eTagType == OL_Tag ? 1 : 0);
			this->wMargin += LISTINDENT;
			NewLine(this->swOLstack.size() == 1);
			break;
		case H1_Tag:
		case H2_Tag:
		case H3_Tag:
			NewLine();
			break;
		case TITLE_Tag:
			wstrTitle = wszEmpty;
			break;
		case A_Tag:
			swstrLink.push(GetAttr(wszHREF, atts));
			break;
		case B_Tag:
			break;
		case HR_Tag:
		case BR_Tag:
			NewLine(true);
			break;
		case IMG_Tag:
		{
			static const WCHAR wszSrc[] = {We('s'),We('r'),We('c'),We(0)};
			WSTRING imageURL = this->wstrBasePath;
			imageURL += wszSlash;
			imageURL += GetAttr(wszSrc, atts);
			CImageWidget *pImage = new CImageWidget(0, this->wX + this->wMargin, this->wY, imageURL.c_str());
			ASSERT(pImage);
			AddWidget(pImage);
			this->wY += pImage->GetH();
			this->wX = 0;  //next thing goes on new line
			break;
		}
		case P_Tag:
			this->wY += static_cast<UINT>(g_pTheFM->GetFontLineHeight(FONTLIB::F_Text) * 2/3);
			break;
		case LI_Tag:
		{
			//Use image as bullet in stead ?
			static const WCHAR wszItem[] = {We('*'),We(' '),We(0)};
			NewLine();
			if (this->wOLcounter)
			{
				WCHAR wszBuf[33];
				wstrBuffer = (WSTRING)_itow(this->wOLcounter, wszBuf, 10)
					+ wszPeriod + wszSpace;
				++this->wOLcounter;
			}
			else wstrBuffer = wszItem;
			Flush(true);
			this->bSkipSpace = true;
			break;
		}
		case TABLE_Tag:
			NewLine();
			vwColumns.clear();
			//Fall through
		case TR_Tag:
			this->wCurrentColumn = 0;
			break;
		case TD_Tag:
		{
			if (this->wCurrentColumn >= vwColumns.size())
			{
				static const WCHAR wszWidth[] = {We('w'),We('i'),We('d'),We('t'),We('h'),We(0)};
				WSTRING wstrWidthAttr = GetAttr(wszWidth, atts);
				this->wTDWidth = wstrWidthAttr.length() > 0 ?
					_Wtoi(wstrWidthAttr.c_str()) : 0;
				vwColumns.push_back(this->wX += 32);
			}
			else
			{
				this->wX = vwColumns[this->wCurrentColumn];
				this->wTDWidth = 0;
			}
			++this->wCurrentColumn;
			this->bSkipSpace = true;
			break;
		}
		default:
			break;
	}
}

//*****************************************************************************
void CHTMLWidget::InElement(
//Expat callback function: Process text between XML tags.
//
//Params:
	const XML_Char* s, int len)
{
	WCHAR *wszText = new WCHAR[len + 1];

	// Text handler:
	// * Convert whitespace to space
	// * Strip spaces
	// * UTF-8 to UCS-2 conversion (UCS-4 is just stripped)
	UINT i = 0;
	for (const XML_Char *s_end = s + len; s < s_end; ++i, ++s)
	{
		WCHAR value = {(WCHAR_t)UTF8ToUCS4Char(&s)};
		if (iswspace(value))
		{
			if (this->bSkipSpace)
			{
				--i;
				continue;
			}
			this->bSkipSpace = true;
			WCv(value) = ' ';
		}
		else
		{
			this->bSkipSpace = false;
			if (WCv(value) == 0xa0) //nbsp
				WCv(value) = ' ';
		}
		wszText[i] = value;
	}
	WCv(wszText[i]) = 0;

	wstrBuffer += wszText;
	delete[] wszText;
}

//*****************************************************************************
void CHTMLWidget::EndElement(
//Expat callback function: Process XML end tag.
//
//Params:
	const XML_Char* name)
{
	//Get tag type (assume no special chars).
	const HTMLTagType eTagType = ParseTag(name);

	if (wstrBuffer.length()) Flush();

	if (aeTagLevel[eTagType])
		--aeTagLevel[eTagType];

	switch (eTagType)
	{
		case H3_Tag:
			this->wX = 0;
			this->wY += static_cast<UINT>(g_pTheFM->GetFontLineHeight(FONTLIB::F_TextHead3));
			this->bSkipSpace = true;
			break;
		case UL_Tag:
		case OL_Tag:
			this->wOLcounter = swOLstack.top();
			this->swOLstack.pop();
			this->wMargin -= LISTINDENT;
			NewLine();
			break;
		case LI_Tag:
		case P_Tag:
		case TR_Tag:
			NewLine();
			break;
		case TD_Tag:
			if (this->wTDWidth)
				this->wX = vwColumns[this->wCurrentColumn - 1] + this->wTDWidth;
			break;
		default:
			break;
	}
}

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

#ifndef HTMLWIDGET_H
#define HTMLWIDGET_H

#include "Widget.h"

#include <BackEndLib/StretchyBuffer.h>

#include <expat.h>

#include <stack>
#include <map>
using std::stack;

//Enumeration of supported tags.
enum HTMLTagType
{
	First_Tag=0,
	BODY_Tag=First_Tag,
	TITLE_Tag,
	A_Tag,
	B_Tag,
	I_Tag,
	U_Tag,
	BR_Tag,
	H1_Tag,
	H2_Tag,
	H3_Tag,
	HR_Tag,
	IMG_Tag,
	P_Tag,
	UL_Tag,
	OL_Tag,
	LI_Tag,
	TD_Tag,
	TR_Tag,
	TABLE_Tag,
	Tag_Count,
	Unknown_Tag = Tag_Count
};

typedef std::map<WSTRING, UINT> HTMLIDMAP;

struct SHTMLPos
{
	WSTRING wstrLink;
	UINT    wY;
};

//******************************************************************************
class CHTMLWidget : public CWidget
{
public:
	CHTMLWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY, const UINT wSetW,
			const UINT wSetH, const UINT wSetMargin, const WCHAR *pwczFileName,
			const UINT dwSetLinkTagStart);

	UINT           GetBackCount() const { return this->swstrBackLink.size(); }
	virtual void   GetChildClippingRect(SDL_Rect &ChildClipRect) const;
	UINT           GetForwardCount() const { return this->swstrForwardLink.size(); }
	const WCHAR *  GetTitle() const { return this->wstrTitle.c_str(); }
	const WCHAR *  GetStatus() const { return this->wstrStatus.c_str(); }
	bool           GoBack(UINT wCount=1);
	bool           GoForward(UINT wCount=1);
	bool           GoHome();

	void           ScrollUp(const UINT wLines=7);
	void           ScrollUpPage();
	void           ScrollDown(const UINT wLines=7);
	void           ScrollDownPage();

	void           SetTitle(const WCHAR *s) { if (s) wstrTitle = s; }
	void           SetStatus(const WCHAR *s) { if (s) wstrStatus = s; }
	bool           LoadFile(const WCHAR *pwczFileName=NULL, bool bNewLink=true, UINT wScrollY=0);
	virtual void   Paint(bool bUpdateRect=true);
	virtual bool   ParentMustPaintChildren() const {return true;}

	//For XML parsing.
	void           StartElement(const XML_Char *name, const XML_Char **atts);
	void           InElement(const XML_Char *s, int len);
	void           EndElement(const char *name);

protected:
	virtual ~CHTMLWidget();

	virtual void   HandleAnimate();
	virtual bool   IsAnimated() const {return true;}
	UINT     wHeightDrawn;  //amount rendered so far during animation
	int nOffsetX, nOffsetY; //what part of page to render
	bool     bNeedRepaint;  //need to repaint ?

private:
	void           Flush(bool bInMargin=false);
	WSTRING        GetAttr(const WCHAR *attr, const XML_Char **atts);
	void           NewLine(bool bAlways=false);
	bool           Parse(const CStretchyBuffer &text);
	HTMLTagType    ParseTag(const char *str) const;
	Uint32         TranslateColor(SDL_Surface *pSurface, const WSTRING& wstr) const;
	void           UpdateHTMLSurface();

	SDL_Surface *      pHTMLSurface;

	WSTRING            wstrTitle;
	WSTRING            wstrStatus;
	WSTRING            wstrBuffer;
	WSTRING            wstrBasePath;
	WSTRING            wstrIndexPage;
	WSTRING            wstrCurrentPage;
	WSTRING            wstrCurrentId;
	stack<WSTRING>     swstrLink;
	stack<SHTMLPos>    swstrBackLink;
	stack<SHTMLPos>    swstrForwardLink;
	HTMLIDMAP          mIdmap;

	UINT               aeTagLevel[Tag_Count + 1];  // Including unknown

	UINT  wX;
	UINT  wY;
	UINT  wMargin;

	vector<UINT>       vwColumns;
	UINT               wCurrentColumn;
	UINT               wTDWidth;
	UINT               wOLcounter;
	stack<UINT>        swOLstack;

	UINT dwLinkTagStart;
	UINT dwCurrentLinkTag;

	bool     bSkipSpace;    //ignore next whitespace
	WSTRING  wstrBGColor;   //BG color of page
};

#endif //#ifndef HTMLWIDGET_H

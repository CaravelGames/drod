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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996, 
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef SCROLLINGTEXTWIDGET_H
#define SCROLLINGTEXTWIDGET_H

#include "FontManager.h"
#include "Widget.h"
#include "LabelWidget.h"

//******************************************************************************
class CScrollingTextWidget : public CWidget
{
public:
	CScrollingTextWidget(const UINT dwSetTagNo,
			const int nSetX, const int nSetY, const UINT wSetW, const UINT wSetH,
			const UINT wPixelsPerSecond=33);

	void				Add(CWidget *pWidget);
	void           AddText(const WCHAR *pwczAddText, 
			const UINT eFontType, CLabelWidget::TEXTALIGN eTextAlign = CLabelWidget::TA_Left);
	void           ClearText();
	bool           empty() const {return this->Children.empty();}
	CWidget *      GetLastShownWidget() const;
	bool           IsAllTextShown() const;
	virtual void   Paint(bool bUpdateRect = true);  
	void           ScrollText();
	void           SetBackground(SDL_Surface *pSetSurface, const int x = -1, 
			const int y = -1);
	void           SetScrollRate(const UINT wPixelsPerSecond);

	virtual void      HandleAnimate();
	virtual bool      IsAnimated() const {return true;}

private:
	SDL_Surface *     pBackgroundSurface;
	int               nBackgroundX, nBackgroundY;
	int               nNextLabelY;

	UINT              wMSPerPixel;
	UINT             dwLastScroll;
	bool              bPaused;
};

#endif //...#ifndef SCROLLINGTEXTWIDGET_H

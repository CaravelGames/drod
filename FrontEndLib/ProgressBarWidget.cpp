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
 * Portions created by the Initial Developer are Copyright (C) 2003, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "ProgressBarWidget.h"
#include "BitmapManager.h"
#include "Inset.h"
#include <BackEndLib/Assert.h>

//
//Public methods.
//

//*********************************************************************************************************
CProgressBarWidget::CProgressBarWidget(
//Constructor.
//
//Params:
	UINT dwSetTagNo,                //(in)   Required params for CWidget
	int nSetX, int nSetY,               //    constructor.
	UINT wSetW, UINT wSetH,             //
	float fSetValue)                //(in)   0 = left, 255 = right.
	: CWidget(WT_ProgressBar, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, fValue(fSetValue), fLastDrawnValue(fSetValue - 1)
{
	this->imageFilenames.push_back(string("Dialog"));
}

//*********************************************************************************************************
void CProgressBarWidget::DrawBar()
{
	SDL_Rect rectInset;
	GetRect(rectInset);

	static const UINT CX_BAR_SPACE = 1;
	static const UINT CY_BAR_SPACE = 1;
	static const UINT CX_MIN_INSET = (CX_BAR_SPACE + CX_INSET_BORDER) * 2 + 3; //3 = left/right bar border + red center.
	static const UINT CY_MIN_INSET = (CY_BAR_SPACE + CY_INSET_BORDER) * 2 + 3; //3 = top/bottom bar border + red center.

	SDL_Surface *pDestSurface = GetDestSurface();

	if (rectInset.w >= (int)CX_MIN_INSET && rectInset.h >= (int)CY_MIN_INSET)  //Inset must be big enough to display bar.
	{
		SURFACECOLOR Black;
		SURFACECOLOR Red;
		GetDestSurfaceColor(0, 0, 0, Black);
		ASSERT(this->fValue >= 0.0 && this->fValue <= 1.0);
		GetDestSurfaceColor(128 + (Uint8)(this->fValue*127), (Uint8)(this->fValue*255), 0, Red);   //get brighter

		//Calc bar's rect.
		SDL_Rect rectBar;
		rectBar.x = rectInset.x + CX_INSET_BORDER + CX_BAR_SPACE;
		rectBar.y = rectInset.y + CY_INSET_BORDER + CY_BAR_SPACE;
		rectBar.h = rectInset.h - (CY_INSET_BORDER + CY_BAR_SPACE) * 2;
		const UINT wAvailableWidth = rectInset.w - (CX_INSET_BORDER + CX_BAR_SPACE) * 2;
		rectBar.w = static_cast<Uint16>(wAvailableWidth * this->fValue);
		if (rectBar.w < 3) rectBar.w = 3;

		//Draw the bar.
		DrawFilledRect(rectBar, Red, pDestSurface);
		DrawRect(rectBar, Black, pDestSurface);
	}
}

//*********************************************************************************************************
void CProgressBarWidget::Paint(
//Paint widget area.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//Draw inset covering entire widget area.
	SDL_Rect rectInset;
	GetRect(rectInset);
	DrawInset(rectInset.x, rectInset.y, rectInset.w, rectInset.h, this->images[0], GetDestSurface());

	DrawBar();
	
	this->fLastDrawnValue = this->fValue;
	if (bUpdateRect) UpdateRect();
}

//*********************************************************************************************************
void CProgressBarWidget::HandleAnimate()
{
	 if (this->fValue > this->fLastDrawnValue)
		 DrawBar();	//grow bar
	 else if (this->fValue < this->fLastDrawnValue)
		 RequestPaint();	//shrink bar (requires background redraw)
}

//*********************************************************************************************************
void CProgressBarWidget::SetValue(
//Set value of progress bar, which affects its position.
//
//Params:
	const float fSetValue) //(in)   New value.
{
	this->fValue = fSetValue < 0.0f ? 0.0f : fSetValue > 1.0f ? 1.0f : fSetValue;
}

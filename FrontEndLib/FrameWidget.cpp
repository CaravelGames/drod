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

#include "FrameWidget.h"
#include "BitmapManager.h"
#include "FontManager.h"
#include <BackEndLib/Assert.h>

//
//Public methods.
//

//******************************************************************************
CFrameWidget::CFrameWidget(
//Constructor.
//
//Params:
	UINT dwSetTagNo,                //(in)   Required params for CWidget 
	int nSetX, int nSetY,               //    constructor.
	UINT wSetW, UINT wSetH,             //
	const WCHAR *pwczSetCaption)        //(in)   Caption text for frame. If 
											//    NULL, frame will not be 
											//    drawn with a caption.
	: CWidget(WT_Frame, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
{
	if (pwczSetCaption) SetCaption(pwczSetCaption);
}

//******************************************************************************
CFrameWidget::~CFrameWidget()
//Destructor.
//
{
}

//******************************************************************************
void CFrameWidget::SetCaption(
//Sets caption that appears at top of frame.
//
//Params:
	const WCHAR *pwczSetCaption) //(in) Text for caption.  NULL to remove caption.
{
	this->wstrCaption = pwczSetCaption ? pwczSetCaption : wszEmpty;
}

//******************************************************************************
void CFrameWidget::Paint(
//Paint widget area.
//
//Params:
	bool bUpdateRect)          //(in)   If true (default) and destination
									//    surface is the screen, the screen
									//    will be immediately updated in
									//    the widget's rect.
{
	//Drawing code below needs to be modified to accept offsets.  Until then,
	//this widget can't be offset.
	ASSERT(!IsScrollOffset());

	ASSERT(this->w >= 4);
	ASSERT(this->h >= 4);

	//Calc surface colors if needed.
	SDL_Surface *pDestSurface = GetDestSurface();
	const SURFACECOLOR Light = GetSurfaceColor(pDestSurface, 225, 214, 192);
	const SURFACECOLOR Dark = GetSurfaceColor(pDestSurface, 145, 132, 109);

	//Here's a picture to see what it looks like:
	//
	//       CCCCCCC
	//    ###CCCCCCC#######    # = dark
	//    #..CCCCCCC..... . . = light
	//    #. CCCCCCC     #. C = caption
	//    #.             #.
	//    #.             #.
	//    #.             #.
	//    #.             #.
	//    #.             #.
	//    #.             #.
	//    # ##############.
	//     ................

	static const UINT CX_INDENT = 2, CX_CORNER = 2, CX_CAPTION_SPACE = 2;
	static const UINT wCaptionBuffer = CX_CORNER*2 + CX_INDENT + CX_CAPTION_SPACE*2;

	UINT wCaptionW = 0, wCaptionH = 0;
	bool bDrawCaption = this->wstrCaption.size() > 0;
	if (bDrawCaption)
	{
		//Get dimensions of caption.
		g_pTheFM->GetTextWidthHeight(FONTLIB::F_FrameCaption,
				this->wstrCaption.c_str(), wCaptionW, wCaptionH);

		//Constrain width to available space.
		if (wCaptionW > this->w - wCaptionBuffer)
			wCaptionW = this->w - wCaptionBuffer;
	}

	if (bDrawCaption)
	{
		//Draw caption text.
		static const int Y_OFFSET = -4; //kludge -- font is drawn too low otherwise
		const int xCaption = this->x + CX_CORNER + CX_INDENT + CX_CAPTION_SPACE;
		const int yCaption = this->y - (wCaptionH / 2) + Y_OFFSET;
		g_pTheFM->DrawTextXY(FONTLIB::F_FrameCaption, this->wstrCaption.c_str(),
				pDestSurface, xCaption, yCaption, this->w - wCaptionBuffer);

		//Get direct access to pixels.
		LockDestSurface();

		//Draw dark outer top row left of the caption.
		DrawRow(this->x, this->y, CX_CORNER + CX_INDENT, Dark);

		//Draw dark outer top row right of the caption.
		const int xRightOfCaption = xCaption + wCaptionW + CX_CAPTION_SPACE;
		int width = this->w - (xRightOfCaption - this->x) - 1;
		if (width > 0)
			DrawRow(xRightOfCaption, this->y, width, Dark);

		//Draw light inner top row left of the caption.
		DrawRow(this->x + 1, this->y + 1, CX_CORNER + CX_INDENT + 1, Light);

		//Draw light inner top row right of the caption.
		width = this->w - (xRightOfCaption - this->x) - 3;
		if (width > 0)
			DrawRow(xRightOfCaption, this->y + 1, width, Light);
	}
	else  //Draw top rows without caption.
	{
		//Get direct access to pixels.
		LockDestSurface();

		//Draw dark outer top row.
		DrawRow(this->x, this->y, this->w - 1, Dark);

		//Draw light inner top row.
		DrawRow(this->x + 1, this->y + 1, this->w - 3, Light);
	}

	//Draw dark left column.
	DrawCol(this->x, this->y + 1, this->h - 2, Dark);

	//Draw light inner left column.
	DrawCol(this->x + 1, this->y + 2, this->h - 4, Light);

	//Draw light outer bottom row.
	DrawRow(this->x + 1, this->y + this->h - 1, this->w - 1, Light);

	//Draw light outer right column.
	DrawCol(this->x + this->w - 1, this->y + 1, this->h - 2, Light);

	//Draw dark inner bottom row.
	DrawRow(this->x + 2, this->y + this->h - 2, this->w - 3, Dark);

	//Draw dark inner right column.
	DrawCol(this-> x + this->w - 2, this->y + 2, this->h - 4, Dark);

	UnlockDestSurface();

	PaintChildren();
	if (bUpdateRect) UpdateRect();
}

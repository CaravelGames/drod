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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef SLIDERWIDGET_H
#define SLIDERWIDGET_H

#include "FocusWidget.h"

static const UINT CY_STANDARD_SLIDER = 30;

//******************************************************************************
class CSliderWidget : public CFocusWidget
{
	public:
		CSliderWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
				const UINT wSetW, const UINT wSetH, const BYTE bytSetValue, const BYTE bytNumTicks=0);
		virtual ~CSliderWidget();

		inline BYTE    GetNumTicks() const {return this->bytTickMarks;}
		inline BYTE    GetValue() const {return this->bytValue;}
		virtual void   Paint(bool bUpdateRect = true);
		void           SetValue(const BYTE bytSetValue);
		void           SetDrawTickMarks(const bool bDrawTickMarks);

		std::vector<UINT> pBiggerTicks; // Values of ticks that should be drawn bigger when bDrawTickMarks is true

	protected:
		virtual void   HandleDrag(const SDL_MouseMotionEvent &Motion);
		virtual void   HandleMouseDown(const SDL_MouseButtonEvent &Button);
		virtual void   HandleKeyDown(const SDL_KeyboardEvent &KeyboardEvent);

	private:
		void           DrawFocused();
		void           SetToX(const int nX);

		BYTE           bytValue, bytPrevValue;
		BYTE           bytTickMarks;	//# of tick increments (0 = off)
		bool           bDrawTickMarks; //Whether to draw small vertical lines for each tick value

		bool           bWasSliderDrawn;
		bool           bFocusRegionsSaved;
		SDL_Surface *     pEraseSurface;
		SDL_Surface *     pFocusSurface[2];
};

#endif //#ifndef SLIDERWIDGET_H

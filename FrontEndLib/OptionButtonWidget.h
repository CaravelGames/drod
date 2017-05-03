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

#ifndef OPTIONBUTTONWIDGET_H
#define OPTIONBUTTONWIDGET_H

#include "FocusWidget.h"

#include <BackEndLib/Wchar.h>

//Standard height of option buttons.
static const UINT CY_STANDARD_OPTIONBUTTON = 32;

//******************************************************************************
class COptionButtonWidget : public CFocusWidget
{
	public:
		COptionButtonWidget(const UINT dwSetTagNo, const int nSetX,
				const int nSetY, const UINT wSetW, const UINT wSetH,
				const WCHAR *pwczSetCaption, const bool bSetChecked=false,
				const bool bWhiteText=false);
		virtual ~COptionButtonWidget();

		bool           IsChecked() const {return this->bIsChecked;}
		virtual void   Paint(bool bUpdateRect = true);
		void           SetCaption(const WCHAR *pwczSetCaption);
		void           SetChecked(const bool bSetChecked);
		void           SetChecked(const int nSetChecked);
		void           ToggleCheck(const bool bShowEffects=false);

	protected:
		virtual void   HandleKeyDown(const SDL_KeyboardEvent &KeyboardEvent);
		virtual void   HandleMouseDown(const SDL_MouseButtonEvent &Button);
		virtual bool   IsDoubleClickable() const {return false;}

	private:
		void           DrawChecked();
		void           DrawUnchecked();
		void           DrawFocused();

		bool           bFocusRegionsSaved;
		bool           bIsChecked;
		bool           bWhiteText; //show text white instead of black
		SDL_Surface *  pFocusSurface;
		WSTRING        wstrCaption;
};

#endif //#ifndef OPTIONBUTTONWIDGET_H

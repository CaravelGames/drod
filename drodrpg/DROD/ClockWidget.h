// $Id: ClockWidget.h 8102 2007-08-15 14:55:40Z trick $

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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef CLOCKWIDGET_H
#define CLOCKWIDGET_H

#include "DrodWidget.h"

#include <list>
#include <string>

//Clock dimensions.
static const UINT CX_CLOCK = 152;
static const UINT CY_CLOCK = 152;

//******************************************************************************
class CClockWidget : public CWidget
{
public:
	CClockWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
		  const UINT wSetW, const UINT wSetH);
	virtual ~CClockWidget();

	virtual void      Paint(bool bUpdateRect = true);

	void  SetTime(const UINT wValue, const bool bHalfStep);
	UINT  GetTime() const {return this->wTime;}

	void ShowClock(const UINT wTime, const bool bHalfStep, const bool bState,
			const bool bReset=false);

   void NextOption();

	protected:
		virtual void   HandleAnimate();
		virtual bool   IsAnimated() const {return true;}
      virtual  void  HandleMouseDown(const SDL_MouseButtonEvent &Button);


private:
	SDL_Surface *pEraseSurface;
	bool bEraseSurfaceMade;

	UINT wTime; //time showing on clock
	bool bHalfStep;

	bool bClockDrawn, bEraseOldClock;
	bool bClockVisible;  //whether clock is visible
	UINT wPixelsShowing;
	Uint32 dwLastAnimate;
	bool bShowImmediate;	//don't animate clock movement, just show
	std::list<std::string> clockOptions;
   std::list<std::string>::iterator curOption;
};

#endif //#ifndef CLOCKWIDGET_H

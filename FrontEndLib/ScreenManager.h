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

//ScreenManager.h
//Declarations for CScreenManager.
//CScreenManager instances, loads and unloads (uninstances) screens,
//with knowledge of specific screen types.
//It directs execution to an input loop in an appropriate CScreen class.

#ifndef SCREENMANAGER_H
#define SCREENMANAGER_H

#include "Screen.h"
#include <BackEndLib/MessageIDs.h>

#include <string>
using std::string;

//Screen (surface) transition effects.
//Add other transition effects here as needed.
enum TRANSTYPE {
	Fade,    //Smooth color transition from one screen to another.
	Cut,     //New screen shown without transition.
	Pan      //Slide from one screen to another.
};

//Values needed here, but can't place them in a final enumeration yet (i.e.,
//there will be more screens used in the app).
//Give these values to the corresponding names in the final enumeration.
namespace SCREENLIB
{
	enum SCREENTYPE {
		SCR_None = 0,     //Not an actual screen--indicates no screen or application exit.
		SCR_Return = 1    //Not an actual screen--indicates the screen previously visited.
	};
};

//*****************************************************************************
class CScreen;
class CScreenManager
{
public:
	CScreenManager(SDL_Surface *pSetScreenSurface);
	virtual ~CScreenManager();

	virtual UINT Init() {return 0L;}

	SDL_Cursor *   GetCursor(const UINT cursorType) const
			{return this->pCursor ? this->pCursor[cursorType] : NULL;}

	//Specifies the transition to use on the next screen change.  After that
	//screen change, the transition type will go back to the default type.
	void        SetDestTransition(const TRANSTYPE eTransType)
			{this->eTransition = eTransType;}

	UINT     ActivateScreen(const UINT eScreen);
	CScreen *      GetScreen(const UINT eScreen);
	UINT     GetReturnScreenType() const;
	void        InsertReturnScreen(const UINT eScreen);
	bool        IsScreenInstanced(const UINT eScreen) const;
	void        ChangeReturnScreen(const UINT eScreen);
	void        ClearReturnScreens();
	void        RemoveReturnScreen();
	virtual void    GetScreenName(const UINT eScreen, string &strName) const = 0;

	bool     bTransitioning;   //forbids CWidget::RequestPaint() calls to paint
	UINT     crossfadeDuration; //screen transition time

protected:
	virtual void FreeCursors() {}
	CScreen *      GetInstancedScreen(const UINT eScreen) const;
	virtual CScreen * GetNewScreen(const UINT eScreen)=0;
	CScreen *   InstanceScreen(const UINT eScreen);
	bool     LoadScreen(CScreen* &pScreen);
	void     UnloadAllScreens();

	SDL_Cursor*    LoadSDLCursor(const char* cursorName);
	virtual bool      LoadCursors()=0;

	SDL_Cursor **     pCursor;
	list<CScreen *>      InstancedScreens;
	list<UINT>  ReturnScreenList;

	//Screen transition effects.
	TRANSTYPE eTransition;

	PREVENT_DEFAULT_COPY(CScreenManager);
};

//Define global pointer to the one and only CScreenManager object.
#ifndef INCLUDED_FROM_SCREENMANAGER_CPP
	extern CScreenManager *g_pTheSM;
#endif

#endif //...#ifndef SCREENMANAGER_H

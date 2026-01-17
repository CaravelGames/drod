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

//Screen.h
//Declarations for CScreen.
//CScreen handles all input and output for a given screen.

//SUMMARY
//
//CScreen is an abstract base class from which all screens derive.  A screen is just a big
//top-level widget that can contain child widgets.  So pretty much all the comments found at the
//top of Widget.h apply.  A screen always takes up the entire window space (CX_SCREEN pixels
//across and CY_SCREEN pixels down).  Only one screen can be visible at once.  Sizable screens are
//not supported.
//
//USAGE
//
//You can derive new classes from CScreen.  It's necessary to add a new SCREENTYPE enumeration
//at the top of ScreenManager.h and add a switch handler in CScreenManager::GetNewScreen() that will
//construct your new class.
//
//To write code that sends the user to your new screen, determine the event that should send
//the user there, i.e. user clicks a "Go To Scary Screen" button.  Code in the event handler, (i.e.
//OnClick()) should make a call to its own class's GoToScreen() method.  When the event handler
//exits, the screen manager will transition to the new screen.
//
//If you followed all the above instructions without doing anything else, you would have a black
//screen that doesn't do much.  The user can hit escape from the new screen to return to whatever
//screen they were at previously.
//
//You can show things on the screen in two different ways.  1. Add widgets to the screen that will
//be painted automatically without writing code in your CScreen-derived class.  2. Override the
//default Paint() method to draw something besides that big black rectangle.  More about writing
//widget-adding and painting code is found at the top of Widget.h.  Both methods may be used
//together.  Generally, you only want to use method 2 when you are painting something that only
//appears on one screen.  Otherwise, you'd use existing widgets or write new widgets to paint what
//you want.
//
//You can respond to events like the user pressing a key or pushing a button.  CScreen
//has default event-handling methods that you can override in your CScreen-derived class.  They
//are all prefixed with "On".  When overriding a CScreen event-handling method it is almost always
//a good idea to call CScreen's method at the top of your overriding method.  This ensures that
//you get the nice default behaviour that makes the screens consistent.  More about writing
//event-handling code is found at the top of EventHandlerWidget.h and all of the event-handlers
//are described in the CEventHandlerWidget class declaration.
//
//DIALOGS
//
//Dialogs are another type of widget that can be a child of a screen, but they are different in
//that they prevent the screen from receiving events while they are on the screen.  In other words,
//dialogs are modal.  More about writing dialogs and code to call them is found at the top of
//DialogWidget.h.
//
//There are some common dialogs that can be activated by calling CScreen methods.  These methods
//are named in the form Show*Message(), i.e. ShowYesNoMessage().  You pick the one that suits the
//type of information you want to display and input you want to collect from the user.  If you
//create a new dialog that is useful to show on more than one screen, and can be used in one
//call, then consider writing a new CScreen method like the others to activate the dialog and
//return user input.
//
//TWO SCREENS OR ONE SCREEN WITH TWO MODES?
//
//You may have a screen which has two different modes to it.  I.e. if you are in "Scary" mode
//everything is drawn with scary graphics and if you are in "happy" mode everything looks happy.
//Your first impulse may be to put an "if (scary) ... else" statement in your Paint() method and input
//handlers.  It's almost always better to use two separate screens and share a base class between
//them.  The base class can have static members for shared data between the two classes.  You can
//call g_pTheSM->SetDestTransition(Cut) before calling GoToScreen() and you will have an instant
//seamless transition between the screens.

#ifndef SCREEN_H
#define SCREEN_H

#ifdef WIN32
#pragma warning(disable:4786)
#endif

//Values needed here, but can't place them in a final enumeration yet (i.e.,
//there will be more screens used in the app).
//Give these values to the corresponding names in the final enumeration.
namespace SCREENLIB {
	enum CURSORTYPE {
		CUR_Select = 0,
		CUR_Wait = 1,
		CUR_Internet = 2
	};

	enum FULLSCREENMODE {
		REAL_BORDERLESS = 0,
		FAUX_BORDERLESS = 1,
		LEGACY = 2
	};
}

#include "EventHandlerWidget.h"
#include "DialogWidget.h"
#include "EffectList.h"
#include "ScreenManager.h"

#include <BackEndLib/InputKey.h>
#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/StretchyBuffer.h>
#include <BackEndLib/Wchar.h>

#include <string>

//Message dialog widget tags.
static const UINT TAG_YES = 9001;
static const UINT TAG_NO = 9002;
static const UINT TAG_TEXT = 9003;
static const UINT TAG_FRAME = 9004;
static const UINT TAG_CANCEL_ = 9005;
static const UINT TAG_TEXTBOX = 9006;
static const UINT TAG_TEXTBOX2D = 9007;

//*****************************************************************************
class CFileDialogWidget;
class CScreen : public CEventHandlerWidget
{
friend class CDialogWidget;

public:
	struct TextInputDialogOptions {
		TextInputDialogOptions() :
			pwczMessage(NULL),
			bMultiline(false),
			bRequireText(false),
			bDigitsOnly(false) {}

		TextInputDialogOptions(const WCHAR *pwczMessage,
			bool bMultiline,
			bool bRequireText,
			bool bDigitsOnly) :
			pwczMessage(pwczMessage),
			bMultiline(bMultiline),
			bRequireText(bRequireText),
			bDigitsOnly(bDigitsOnly) {}

		const WCHAR *pwczMessage;
		bool bMultiline;
		bool bRequireText;
		bool bDigitsOnly;
	};

	static int     GetDisplayForDesktopResOfAtLeast(UINT wX, UINT wY);
	UINT           GetScreenType() const {return this->eType;}
	static void    GetScreenSize(int &nW, int &nH);
	static void    GetWindowPos(int &nX, int &nY);
	static void    InitMIDs(const UINT wQUIT, const UINT wOverwrite);
	static bool    IsFullScreen();
	static void    SaveSnapshot(SDL_Surface* pSurface, const WSTRING &fileName, DataFormat format=DATA_JPG);
	static void    SetCursor(const UINT cursorType=SCREENLIB::CUR_Select);
	static void    SetFullScreenStatic(const bool bSetFull);
	void           SetUpdateRectAfterMessage(const bool bVal=true) {this->bUpdateRectAfterMessage = bVal;}
	static void    SetWindowCentered();
	static void    SetWindowPos(const int nSetX, const int nSetY);

	static int     CX_SCREEN, CY_SCREEN;   //logical screen dimensions
	static bool    bAllowFullScreen;
	static bool    bAllowWindowed;
	static bool    bAllowWindowResizing;

	static SDL_Rect WindowTargetRect;      //position of logical screen in physical window
	static double   WindowScaleFactor;

	static SCREENLIB::FULLSCREENMODE eFullScreenMode;

	// Whether to force-minimize when losing focus on fullscreen, to avoid it never minimizing on its own
	static bool   bMinimizeOnFullScreen;
	static Uint32 dwCurrentTicks; // SDL_GetTicks() value during the start of handling of the current frame
	static Uint32 dwLastRenderTicks; // SDL_GetTicks() value during the last present frame call
	static Uint32 dwPresentsCount; // Count of how many times SDL presented a frame to a window

	static InputKey inputKeyFullScreen;
	static InputKey inputKeyScreenshot;

protected:
	friend class CScreenManager;
	friend class CEffectList;
	CScreen(const UINT eSetType);
	virtual ~CScreen();

	UINT           GetDestScreenType() const {return this->eDestScreenType;}
	void           GoToScreen(const UINT eNextScreen);
	void           HideCursor();
	void           HideStatusMessage();
	bool           IsCursorVisible() const;
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);
	virtual void   OnMouseDown(const UINT dwTagNo,
			const SDL_MouseButtonEvent &Button);
	virtual void   OnMouseMotion(const UINT dwTagNo,
			const SDL_MouseMotionEvent &MotionEvent);
	virtual void   OnMouseUp(const UINT dwTagNo,
			const SDL_MouseButtonEvent &Button);
	virtual bool   OnQuit();
	virtual void   OnWindowEvent(const SDL_WindowEvent &wevent);
	virtual void   Paint(bool bUpdateRect=true);
	virtual bool   PlayVideo(const WCHAR *pFilename, const int x=0, const int y=0);
	bool           PlayVideoBuffer(CStretchyBuffer& buffer, SDL_Window *window, const int x=0, const int y=0);

	void           RemoveToolTip();
	void           RequestToolTip(const WCHAR *pwczText);
	UINT           SelectFile(WSTRING& filePath, WSTRING& fileName, const bool bWrite);
	UINT           SelectFiles(WSTRING& filePath, vector<WSTRING>& fileName);
	void           SetDestScreenType(const UINT eSet) {this->eDestScreenType = eSet;}
	void           SetFullScreen(const bool bSetFull);
	virtual bool   SetForActivate();
	void           SetResizableScreen(const bool bResizable);
	void           SetScreenType(const UINT eSetType) {this->eType = eSetType;}
	void           ShowCursor();
	UINT           ShowMessage(const WCHAR* pwczMessage);
	void           ShowStatusMessage(const WCHAR *pwczMessage);
	UINT           ShowTextInputMessage(const WCHAR* pwczMessage,
			WSTRING &wstrUserInput, const bool bMultiLineText=false,
			const bool bMustEnterText=true);
	UINT           ShowTextInputMessage(WSTRING &wstrUserInput, const TextInputDialogOptions options);
	void           ShowToolTip(const WCHAR* pwczText);
	virtual UINT   ShowYesNoMessage(const MESSAGE_ID /*dwMessageID*/,
			const MESSAGE_ID /*dwYesButtonText*/, const MESSAGE_ID /*dwNoButtonText*/) //don't use default zero parameter values
		{return TAG_NO;}
	virtual UINT  ShowYesNoMessage(const MESSAGE_ID dwMessageID) {return ShowYesNoMessage(dwMessageID, 0, 0);}
	virtual void   SaveSurface(SDL_Surface *pSurface=NULL);
	virtual void   ToggleScreenSize();
	virtual bool   UnloadOnDeactivate() const {return true;}
	virtual void   UpdateRect() const;
	void           UpdateRect(const SDL_Rect &rect) const;

	virtual void   OnBetweenEvents();

	Uint32         dwLastMouseMove;
	Uint16         wLastMouseX, wLastMouseY;

	//Effects drawn on top of screen
	CEffectList *  pEffects;
	bool           bShowTip, bShowingTip;  //time to show tool tip / tool tip displaying

	CFileDialogWidget * pFileBox;    //choose from files on disk
	CDialogWidget  *  pInputTextDialog;
	CDialogWidget  *  pMessageDialog;
	CDialogWidget  *  pStatusDialog;

	static UINT MIDReallyQuit, MIDOverwriteFilePrompt;

private:
	UINT              eType;
	UINT              eDestScreenType;
	bool              bUpdateRectAfterMessage; //whether to repaint the screen immediately after a message is closed
	static bool       bIsFauxFullscreenOn;
};

// XXX maybe not the best place to put this.. make a window class?
void SetMainWindow(SDL_Window *window, SDL_Surface *shadow, SDL_Texture *tex);
SDL_Window * GetMainWindow(int *getdisplayindex = NULL);
SDL_Surface * GetWindowShadowSurface(SDL_Window *window);
SDL_Texture * GetWindowTexture(SDL_Window *window);
Uint32 GetDisplayFormatEnum();
Uint32 GetDisplayFormatAlphaEnum();
void PresentRect(SDL_Surface *shadow = NULL, const SDL_Rect *rect = NULL);
void PresentRect(SDL_Surface *shadow, int x, int y, int w, int h);
void PresentFrame();

void UpdateWindowSize(int ww, int wh);
void ConvertPhysicalCoordsToLogical(int *x, int *y);
void ConvertLogicalCoordsToPhysical(int *x, int *y);

#endif //...#ifndef SCREEN_H

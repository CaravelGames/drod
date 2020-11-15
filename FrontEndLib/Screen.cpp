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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2004, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer), Matt Schikore (schik), Gerry Jo Jellestad (trick)
 *
 * ***** END LICENSE BLOCK ***** */

#include "Screen.h"

#include "ButtonWidget.h"
#include "LabelWidget.h"
#include "FileDialogWidget.h"
#include "FontManager.h"
#include "FrameWidget.h"
#include "OptionButtonWidget.h"
#include "TextBoxWidget.h"
#include "TextBox2DWidget.h"
#include "ToolTipEffect.h"
#include "BitmapManager.h"
#include "JpegHandler.h"
#include "PNGHandler.h"
#include "TheoraPlayer.h"

#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Dyn.h>

#include <SDL_syswm.h>

#include <math.h>

#ifdef STEAMBUILD
#  include <steam_api.h>
#endif

static SDL_Window *m_pWindow = NULL;

bool m_bIsCursorVisible = true;

//Message dialog coords and dimensions.
const UINT CX_MESSAGE = 700;
const UINT CY_MESSAGE = 200;
const UINT CY_TEXTINPUT = 230;
const UINT CY_TEXTINPUT2D = CY_STANDARD_TBOX2D + 75;
const UINT CX_SPACE = 12;
const UINT CY_SPACE = 12;
const UINT CX_MESSAGE_BUTTON = 80;
const UINT CY_MESSAGE_BUTTON = CY_STANDARD_BUTTON;
const UINT CX_TEXT = CX_MESSAGE - (CX_SPACE * 2);
const UINT CY_TEXT = CY_MESSAGE - (CY_SPACE * 3) - CY_MESSAGE_BUTTON;
const int X_TEXTBOX = CX_SPACE;
const UINT CX_TEXTBOX = CX_MESSAGE - X_TEXTBOX * 2;
const int Y_MESSAGE_BUTTON = CY_TEXT + (CY_SPACE * 2);
const int X_OK2 = CX_MESSAGE/2 - CX_SPACE - CX_MESSAGE_BUTTON;
const int X_CANCEL = X_OK2 + CX_MESSAGE_BUTTON + CX_SPACE;
const int X_TEXT = CX_SPACE;
const int Y_TEXT = CY_SPACE;
const int FRAME_BUFFER = 5;

int CScreen::CX_SCREEN = 1024;
int CScreen::CY_SCREEN = 768;
bool CScreen::bAllowFullScreen = true;  //allow full screen mode by default
bool CScreen::bAllowWindowed = true;    //allow windowed mode by default too
SDL_Rect CScreen::WindowTargetRect = { 0, 0, CScreen::CX_SCREEN, CScreen::CY_SCREEN };
double CScreen::WindowScaleFactor = 1;

Uint32 CScreen::dwCurrentTicks = 0;
Uint32 CScreen::dwLastRenderTicks = 0;
Uint32 CScreen::dwPresentsCount = 0;

InputKey CScreen::inputKeyFullScreen = SDLK_F10;
InputKey CScreen::inputKeyScreenshot = SDLK_F11;

UINT CScreen::MIDReallyQuit = 0;
UINT CScreen::MIDOverwriteFilePrompt = 0;

//***************************************************************************
void SetMainWindow(SDL_Window *window, SDL_Surface *shadow, SDL_Texture *tex) //(in)
{
	ASSERT(window);
	m_pWindow = window;
	SDL_SetWindowData(window, "shadow", shadow);
	SDL_SetWindowData(window, "texture", tex);
	
	int ww, wh;
	SDL_GetWindowSize(window, &ww, &wh);
	UpdateWindowSize(ww, wh);
}

//***************************************************************************
SDL_Window* GetMainWindow(int *getdisplayindex)
{
	if (getdisplayindex)
		*getdisplayindex = SDL_GetWindowDisplayIndex(m_pWindow);
	return m_pWindow;
}

SDL_Surface* GetWindowShadowSurface(SDL_Window *window)
{
	return (SDL_Surface*)SDL_GetWindowData(window, "shadow");
}

SDL_Texture* GetWindowTexture (SDL_Window *window)
{
	return (SDL_Texture*)SDL_GetWindowData(window, "texture");
}

void UpdateWindowSize (int ww, int wh)
{
	//NOTE: We pass in the size because SDL_GetWindowSize isn't reliable right after changing

	double scalex = (double)ww / (double)CScreen::CX_SCREEN;
	double scaley = (double)wh / (double)CScreen::CY_SCREEN;
	CScreen::WindowScaleFactor = min(scalex, scaley);

	CScreen::WindowTargetRect.w = (int)floor(CScreen::CX_SCREEN * CScreen::WindowScaleFactor + 0.5);
	CScreen::WindowTargetRect.h = (int)floor(CScreen::CY_SCREEN * CScreen::WindowScaleFactor + 0.5);
	CScreen::WindowTargetRect.x = (ww - CScreen::WindowTargetRect.w) / 2;
	CScreen::WindowTargetRect.y = (wh - CScreen::WindowTargetRect.h) / 2;
}

void ConvertPhysicalCoordsToLogical (int *x, int *y)
{
	if (x)
		*x = (int)floor((*x - CScreen::WindowTargetRect.x) / CScreen::WindowScaleFactor + 0.5);
	if (y)
		*y = (int)floor((*y - CScreen::WindowTargetRect.y) / CScreen::WindowScaleFactor + 0.5);
}

void ConvertLogicalCoordsToPhysical (int *x, int *y)
{
	if (x)
		*x = ((int)floor(*x * CScreen::WindowScaleFactor + 0.5)) + CScreen::WindowTargetRect.x;
	if (y)
		*y = ((int)floor(*y * CScreen::WindowScaleFactor + 0.5)) + CScreen::WindowTargetRect.y;
}

//***************************************************************************
void PresentFrame()
{
#ifdef STEAMBUILD
	SteamAPI_RunCallbacks();
#endif
	SDL_Renderer *renderer = SDL_GetRenderer(m_pWindow);
	SDL_RenderClear(renderer); // clear border, if any
	SDL_RenderCopy(renderer, GetWindowTexture(m_pWindow), NULL, &CScreen::WindowTargetRect);
	SDL_RenderPresent(renderer);
	CScreen::dwLastRenderTicks = SDL_GetTicks();
	++CScreen::dwPresentsCount;
}

void PresentRect(SDL_Surface *shadow, const SDL_Rect *rect_in)
{
	if (!shadow)
		shadow = GetWindowShadowSurface(m_pWindow);
	else if (shadow != GetWindowShadowSurface(m_pWindow))
		return;

	SDL_Texture* texture = GetWindowTexture(m_pWindow);
	SDL_Rect rect;
	void* tpixels;
	int tpitch;

	if (!rect_in)
	{
		rect.x = rect.y = 0;
		rect.w = shadow->w;
		rect.h = shadow->h;
		SDL_LockTexture(texture, NULL, &tpixels, &tpitch);
	}
	else
	{
		rect = *rect_in;

		if (rect.x >= shadow->w || rect.y >= shadow->h)
			return;

		if (!rect.w)
			rect.w = shadow->w - rect.x;
		if (!rect.h)
			rect.h = shadow->h - rect.y;

		if (rect.x < 0)
		{
			rect.w += rect.x;
			rect.x = 0;
		}
		if (rect.y < 0)
		{
			rect.h += rect.y;
			rect.y = 0;
		}

		if (rect.w < 1 || rect.h < 1)
			return;

		if (rect.w > shadow->w - rect.x)
			rect.w = shadow->w - rect.x;
		if (rect.h > shadow->h - rect.y)
			rect.h = shadow->h - rect.y;

		SDL_LockTexture(texture, (rect.x || rect.y || rect.w != shadow->w || rect.h != shadow->h)
				? &rect : NULL, &tpixels, &tpitch);
	}

	if (tpitch == shadow->pitch && !rect.x && rect.w == shadow->w)
	{
		memcpy(tpixels, (char*)shadow->pixels + rect.y * shadow->pitch, tpitch * (rect.h - rect.y));
	}
	else
	{
		char *dest = (char*)tpixels;
		char *src = (char*)shadow->pixels + rect.y * shadow->pitch +
					rect.x * shadow->format->BytesPerPixel;
		const int y_end = rect.y + rect.h;
		const int row_bytes = rect.w * shadow->format->BytesPerPixel;
		for (int yi = rect.y; yi < y_end; ++yi, src += shadow->pitch, dest += tpitch)
			memcpy(dest, src, row_bytes);
	}
	SDL_UnlockTexture(texture);

	//Update on-screen window
	PresentFrame();
}

void PresentRect(SDL_Surface *shadow, int x, int y, int w, int h)
{
	SDL_Rect r = { x, y, w, h };
	PresentRect(shadow, &r);
}

//
//Protected methods.
//

//*****************************************************************************
CScreen::CScreen(
//Constructor.
//
//Params:
	const UINT eSetType)       //(in)   Type of screen this is.
	: CEventHandlerWidget(WT_Screen, 0L, 0, 0, CX_SCREEN, CY_SCREEN)
	, dwLastMouseMove(0L)
	, wLastMouseX(CX_SCREEN/2)
	, wLastMouseY(CY_SCREEN/2)
	, bShowingTip(false)
	, eType(eSetType)
	, eDestScreenType(SCREENLIB::SCR_Return)
	, bUpdateRectAfterMessage(true)
{
	this->pEffects = new CEffectList(this);

	//Message dialog.
	this->pMessageDialog = new CDialogWidget(0L, 0, 0, CX_MESSAGE, CY_MESSAGE);
	this->pMessageDialog->Hide();

	CLabelWidget *pLabel = new CLabelWidget(TAG_TEXT, X_TEXT, Y_TEXT,
			CX_TEXT, CY_TEXT, FONTLIB::F_Message, wszEmpty);
	this->pMessageDialog->AddWidget(pLabel);

	CFrameWidget *pFrame = new CFrameWidget(TAG_FRAME, X_TEXT - FRAME_BUFFER, Y_TEXT - FRAME_BUFFER,
			CX_TEXT + FRAME_BUFFER*2, CY_TEXT + FRAME_BUFFER*2, NULL);
	this->pMessageDialog->AddWidget(pFrame);

	//Status dialog.
	this->pStatusDialog = new CDialogWidget(0L, 0, 0, CX_MESSAGE, CY_MESSAGE);
	this->pStatusDialog->Hide();

	pLabel = new CLabelWidget(TAG_TEXT, X_TEXT, Y_TEXT,
		CX_TEXT, CY_TEXT, FONTLIB::F_Message, wszEmpty);
	this->pStatusDialog->AddWidget(pLabel);

	//Text input dialog.
	this->pInputTextDialog =
			new CDialogWidget(0L, 0, 0, CX_MESSAGE, CY_TEXTINPUT);
	this->pInputTextDialog->Hide();

	CTextBoxWidget *pTextBox = new CTextBoxWidget(TAG_TEXTBOX,
			0, 0, CX_TEXTBOX, CY_STANDARD_TBOX);
	pTextBox->Hide();
	this->pInputTextDialog->AddWidget(pTextBox);

	CTextBox2DWidget *pTextBox2D = new CTextBox2DWidget(TAG_TEXTBOX2D,
			0, 0, CX_TEXTBOX, CY_STANDARD_TBOX2D, 1350);
	pTextBox2D->Hide();
	this->pInputTextDialog->AddWidget(pTextBox2D);

	pLabel = new CLabelWidget(TAG_TEXT, X_TEXT, Y_TEXT,
			CX_TEXT, CY_TEXT, FONTLIB::F_Message, wszEmpty);
	this->pInputTextDialog->AddWidget(pLabel);

	pFrame = new CFrameWidget(TAG_FRAME, X_TEXT - FRAME_BUFFER, Y_TEXT - FRAME_BUFFER,
			CX_TEXT + FRAME_BUFFER*2, CY_TEXT + FRAME_BUFFER*2, NULL);
	this->pInputTextDialog->AddWidget(pFrame);

	//Add them to screen.
	AddWidget(this->pMessageDialog);
	AddWidget(this->pStatusDialog);
	AddWidget(this->pInputTextDialog);
}

//*****************************************************************************
CScreen::~CScreen()
//Destructor.
{
	ASSERT(!this->bIsLoaded); //Unload() must be called.
	delete this->pEffects;
}

//*****************************************************************************
int CScreen::GetDisplayForDesktopResOfAtLeast(UINT wX, UINT wY)
//Outputs a display index that can support a resolution of at least wX, wY, or -1 on failure
{
	wX = wY = 0;
	int n = SDL_GetNumVideoDisplays();
	for (int i = 0; i < n; ++i)
	{
		SDL_DisplayMode dm;
		if (!SDL_GetDesktopDisplayMode(i, &dm) && dm.w >= (int)wX && dm.h >= (int)wY)
			return i;
	}
	return -1;
}

//*****************************************************************************
void CScreen::InitMIDs(const UINT wQUIT, const UINT wOverwrite)
{
	CScreen::MIDReallyQuit = wQUIT;
	CScreen::MIDOverwriteFilePrompt = wOverwrite;
}

//*****************************************************************************
void CScreen::GoToScreen(
//Deactivate this screen and go to another one.
//Expect the current screen to be pushed onto the return stack on deactivation.
//
//Params:
	const UINT eNextScreen) //(in)   Screen to be at next.
{
	SetDestScreenType(eNextScreen);
	Deactivate();
}

//*****************************************************************************
void CScreen::SetCursor(const UINT cursorType)
//Sets the image of the mouse cursor.
{
	if (g_pTheSM)
	{
		SDL_ShowCursor(SDL_ENABLE);
		m_bIsCursorVisible = true;
		SDL_SetCursor(g_pTheSM->GetCursor(cursorType));
	}
}

//*****************************************************************************
bool CScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	this->bShowTip = this->bShowingTip = false;
	this->dwLastMouseMove = SDL_GetTicks();
	return true;
}

//*****************************************************************************
void CScreen::OnWindowEvent(const SDL_WindowEvent &wevent)
//Always take care of this no matter what widget is receiving event commands.
{
	CEventHandlerWidget::OnWindowEvent(wevent);
}

//*****************************************************************************
void CScreen::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT /*dwTagNo*/,         //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Event.
{
	const InputKey inputKey = BuildInputKey(Key);

	if (inputKey == CScreen::inputKeyFullScreen)
		ToggleScreenSize();
	
	else if (inputKey == CScreen::inputKeyScreenshot)
		SaveSurface();

	switch (Key.keysym.sym)
	{
		case SDLK_RETURN:
			if (Key.keysym.mod & KMOD_ALT && !GetHotkeyTag(Key.keysym))
				ToggleScreenSize();
		break;

		case SDLK_F4:
#if defined(__linux__) || defined(__FreeBSD__)
		case SDLK_PAUSE:
#endif
			if (Key.keysym.mod & (KMOD_ALT | KMOD_CTRL))
				GoToScreen(SCREENLIB::SCR_None);	//boss key -- exit immediately
		break;

		case SDLK_ESCAPE:
			Deactivate();
		break;

		default: break;
	}
}

//*****************************************************************************
void CScreen::OnMouseDown(
//When clicking a mouse button, make mouse cursor appear.
//
//Params:
	const UINT /*dwTagNo*/, const SDL_MouseButtonEvent &Button)
{
	ShowCursor();
	this->pEffects->RemoveEffectsOfType(EFFECTLIB::ETOOLTIP);
	this->bShowingTip = false;

	this->dwLastMouseMove = SDL_GetTicks();
	this->wLastMouseX = Button.x;
	this->wLastMouseY = Button.y;
}

//*****************************************************************************
bool CScreen::OnQuit()
//Called when SDL_QUIT event is received.
//Returns whether Quit action was confirmed.
{
	static bool bQuitDialogActive = false;
	if (bQuitDialogActive) return true; //to stop a recursive call from CDialogWidget::OnQuit.

	//If app is minimized when quit request is received, just exit without
	//bringing up a confirmation dialog.
	if (!WindowIsVisible())
	{
		GoToScreen(SCREENLIB::SCR_None);
		return true;
	}

	bQuitDialogActive = true;
	const UINT dwTagNo = ShowYesNoMessage(CScreen::MIDReallyQuit);
	bQuitDialogActive = false;
	if (dwTagNo == TAG_YES || dwTagNo == TAG_QUIT)
	{
		GoToScreen(SCREENLIB::SCR_None);
		return true;
	}
	return false;
}

//*****************************************************************************
void CScreen::OnMouseMotion(
//When moving the mouse substantially, make mouse cursor appear and erase tooltips.
//
//Params:
	const UINT /*dwTagNo*/,
	const SDL_MouseMotionEvent &MotionEvent)
{
	static const UINT MIN_MOUSE_MOVE = 2;
	if ((UINT)abs(MotionEvent.xrel) >= MIN_MOUSE_MOVE ||
			(UINT)abs(MotionEvent.yrel) >= MIN_MOUSE_MOVE)  //protect against jiggles
	{
		ShowCursor();
		RemoveToolTip();
	}

	if (MotionEvent.xrel || MotionEvent.yrel)
		this->dwLastMouseMove = SDL_GetTicks();
	this->wLastMouseX = MotionEvent.x;
	this->wLastMouseY = MotionEvent.y;
}

//*****************************************************************************
void CScreen::OnMouseUp(
//When clicking a mouse button, make mouse cursor appear.
//
//Params:
	const UINT /*dwTagNo*/, const SDL_MouseButtonEvent &/*Button*/)
{
	this->dwLastMouseMove = SDL_GetTicks();
}

//*****************************************************************************
void CScreen::OnBetweenEvents()
//If mouse hasn't moved for a bit, flag screen as ready to display a tool tip.
{
	//Show tool tip after mouse inactivity, if applicable.
	const Uint32 dwNow = SDL_GetTicks();
	const bool bTimeToShowTip = dwNow - this->dwLastMouseMove > 500; //0.5 second
	this->bShowTip = (bTimeToShowTip && !this->bShowingTip && !this->MouseDraggingInWidget());

	//Draw effects onto screen.
	this->pEffects->UpdateAndDrawEffects(true);
}

//*****************************************************************************
void CScreen::HideCursor()
//Hide the mouse cursor.
{
	if (m_bIsCursorVisible)
	{
		SDL_ShowCursor(SDL_DISABLE);
		m_bIsCursorVisible = false;
	}
}

//*****************************************************************************
void CScreen::ShowCursor()
//Show the mouse cursor.
{
	if (!m_bIsCursorVisible)
	{
		SDL_ShowCursor(SDL_ENABLE);
		if (IsFullScreen())
		{
			//Keep mouse where it was when it was hidden.
			int mx = this->wLastMouseX;
			int my = this->wLastMouseY;
			ConvertLogicalCoordsToPhysical(&mx, &my);
			SDL_WarpMouseInWindow(NULL, mx, my);
		}
		m_bIsCursorVisible = true;
	}
}

//*****************************************************************************
bool CScreen::IsCursorVisible() const {return m_bIsCursorVisible;}

//*****************************************************************************
//Returns: whether display mode is full screen
bool CScreen::IsFullScreen() const
{
	return (SDL_GetWindowFlags(GetMainWindow()) & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0;
}

//*****************************************************************************
void CScreen::SetFullScreen(
//Sets this screen and all future screens to display fullscreen/windowed.
//
//Params:
	const bool bSetFull) //(in) true = Fullscreen; false = windowed
{
	if (!(bSetFull ? CScreen::bAllowFullScreen : CScreen::bAllowWindowed))
		return;

	if (!IsLocked() && (bSetFull != IsFullScreen()))
	{
		// XXX we need SDL_WINDOW_FULLSCREEN_DESKTOP on linux. SDL_WINDOW_FULLSCREEN may be better on windows?
		// (_DESKTOP should preserve aspect ratio better though)
		SDL_SetWindowFullscreen(GetMainWindow(), bSetFull ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);

		//On some systems, must refresh the screen.
		Paint();
		UpdateRect();

		if (!bSetFull)
		{
			//Ensure windowed screen is placed at visible location.
			int nX, nY, nW, nH;
			CScreen::GetWindowPos(nX, nY);
			CScreen::GetScreenSize(nW, nH);
			if (nX < -CScreen::CX_SCREEN || nY < -CScreen::CY_SCREEN ||
					nX >= nW || nY >= nH)
				SetWindowCentered();
		}
	}
}

//*****************************************************************************
void CScreen::GetScreenSize(int &nW, int &nH)
{
	// this actually gets the size of the monitor the window is currently on
	SDL_Rect bounds;
	int disp;
	GetMainWindow(&disp);
	SDL_GetDisplayBounds(disp, &bounds);
	nW = bounds.w;
	nH = bounds.h;
}

//*****************************************************************************
void CScreen::GetWindowPos(int &nX, int &nY)
//OUT: windowed app position on screen
{
	// get position on current monitor.
	SDL_Rect bounds;
	int disp;
	SDL_Window* window = GetMainWindow(&disp);
	SDL_GetDisplayBounds(disp, &bounds);
	SDL_GetWindowPosition(window, &nX, &nY);
	nX -= bounds.x;
	nY -= bounds.y;
}

//*****************************************************************************
void CScreen::SetWindowCentered()
//Centers the windowed app on screen.
{
	int disp;
	SDL_Window* window = GetMainWindow(&disp);
	SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED_DISPLAY(disp), SDL_WINDOWPOS_CENTERED_DISPLAY(disp));
}

//*****************************************************************************
void CScreen::SetWindowPos(const int nSetX, const int nSetY)
//Sets the windowed app position on screen.
{
	// add display origin to keep the window on the same monitor as it is currently
	SDL_Rect bounds;
	int disp;
	SDL_Window* window = GetMainWindow(&disp);
	SDL_GetDisplayBounds(disp, &bounds);
	SDL_SetWindowPosition(window, bounds.x + nSetX, bounds.y + nSetY);
}

//*****************************************************************************
UINT CScreen::SelectFile(
//Displays a dialog box with a file list box.
//Hitting OK will set fileName to the selected file.
//Hitting Cancel will leave it unchanged.
//
//Return: tag # of button clicked
//
//Params:
	WSTRING& filePath,   //(in/out) File path; new path on OK.
	WSTRING& fileName,   //(in/out) Default filename; File path + filename on OK.
	const bool bWrite)   //(in) whether the file being selected is being written to
{
	//Select current choice.
	this->pFileBox->SetWriting(bWrite);
	this->pFileBox->SelectMultipleFiles(false);
	this->pFileBox->SetDirectory(filePath.c_str());
	if (bWrite && fileName.length())
		this->pFileBox->SetFilename(fileName.c_str());
	else
		this->pFileBox->SelectFile();

	this->pFileBox->Show();
	bool bConfirmed;
	UINT dwTagNo;
	do {
		bConfirmed = true;
		dwTagNo = this->pFileBox->Display();
		if (dwTagNo == TAG_OK)
		{
			filePath = this->pFileBox->GetSelectedDirectory();
			fileName = this->pFileBox->GetSelectedFileName();
			if (bWrite && CFiles::DoesFileExist(fileName.c_str()))
				if (ShowYesNoMessage(CScreen::MIDOverwriteFilePrompt) != TAG_YES)
					bConfirmed = false;
		}
	} while (!bConfirmed);
	this->pFileBox->Hide();
	Paint();

	return dwTagNo;
}

//*****************************************************************************
UINT CScreen::SelectFiles(
//Displays a dialog box with a file list box.
//Hitting OK will set fileNames to the selected files.
//Hitting Cancel will leave it unchanged.
//
//Return: tag # of button clicked
//
//Params:
	WSTRING& filePath,   //(in/out) File path; new path on OK.
	vector<WSTRING>& fileNames)   //(in/out) Default filename; File path + filename on OK.
{
	this->pFileBox->SetWriting(false);
	this->pFileBox->SelectMultipleFiles(true);
	this->pFileBox->SetDirectory(filePath.c_str());
	this->pFileBox->SelectFile();

	this->pFileBox->Show();
	bool bConfirmed;
	UINT dwTagNo;
	do {
		bConfirmed = true;
		dwTagNo = this->pFileBox->Display();
		if (dwTagNo == TAG_OK)
		{
			filePath = this->pFileBox->GetSelectedDirectory();
			fileNames = this->pFileBox->GetSelectedFileNames();
		}
	} while (!bConfirmed);
	this->pFileBox->Hide();
	Paint();

	return dwTagNo;
}

//*****************************************************************************
void CScreen::Paint(
//Overridable method to paint the screen.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
//
//As a placeholder, it justs draws a black screen to show something upon
//arrival to the screen.
{
	SDL_Surface *pScreenSurface = GetDestSurface();
	SDL_Rect rect = MAKE_SDL_RECT(0, 0, pScreenSurface->w, pScreenSurface->h);
	static const SURFACECOLOR Black = {0, 0, 0};
	DrawFilledRect(rect, Black);

	PaintChildren();

	this->pEffects->UpdateAndDrawEffects(true, pScreenSurface);

	if (bUpdateRect) UpdateRect();
}

//*****************************************************************************
bool CScreen::PlayVideo(const WCHAR *pFilename, const int x, const int y)
//Plays a video file on the current screen.
{
	SetCursor(SCREENLIB::CUR_Wait);
	CFiles f;
	CStretchyBuffer buffer;
	const bool bRes = f.ReadFileIntoBuffer(pFilename, buffer, true);
	if (bRes) {
		HideCursor();
		PlayVideoBuffer(buffer, GetMainWindow(), x, y);
		ShowCursor();
	}
	SetCursor();
	return bRes;
}

//*****************************************************************************
void CScreen::RemoveToolTip()
{
	this->pEffects->RemoveEffectsOfType(EFFECTLIB::ETOOLTIP);
	this->bShowingTip = false;
}

//*****************************************************************************
void CScreen::RequestToolTip(const WCHAR *pwczText)
//If this->bShowTip indicates that the app is in a state ready to display
//a tooltip, then remove any existing tooltips and display this tooltip message.
{
	if (this->bShowTip)
	{
		RemoveToolTip();
		ShowToolTip(pwczText);
	}
}

//*****************************************************************************
void CScreen::ShowToolTip(const WCHAR* pwczText)
//Pops up text as a tool tip effect immediately at the mouse pointer location.
{
	if (!pwczText)
		return;

	CEffect *pEffect = new CToolTipEffect(this, CCoord(this->wLastMouseX,
			this->wLastMouseY), pwczText);
	ASSERT(pEffect);
	this->pEffects->AddEffect(pEffect);

	this->bShowingTip = true;
}

//*****************************************************************************
void CScreen::SaveSurface(SDL_Surface *pSurface)
//Outputs screen snapshot to default file name.
{
	static WCHAR defaultFileName[] = {We('s'),We('c'),We('r'),We('e'),We('e'),We('n'),We(0)};
	if (!pSurface) pSurface = GetWidgetScreenSurface();
	WSTRING wstr(defaultFileName);
	SaveSnapshot(pSurface, wstr);
}

//*****************************************************************************
void CScreen::SaveSnapshot(
//Outputs a snapshot image of the current screen surface to specified path+file.
//
//Params:
	SDL_Surface* pSurface,   //surface to output
	const WSTRING &fileName, //output filename
	DataFormat format)       //output image file format
{
	ASSERT(pSurface);
	ASSERT(!fileName.empty());

	WSTRING name = fileName;

	switch (format)
	{
		case DATA_BMP:
		{
			static const WCHAR bmpExt[] = {We('.'),We('b'),We('m'),We('p'),We(0)};
			name += bmpExt;
			const string str = UnicodeToUTF8(name);
			SDL_SaveBMP(pSurface, str.c_str());
		}
		break;
		case DATA_JPG:
		{
			static const WCHAR jpegExt[] = {We('.'),We('j'),We('p'),We('g'),We(0)};
			name += jpegExt;
			CJpegHandler::Compress(name.c_str(), pSurface);
		}
		break;
		case DATA_PNG:
		{
			static const WCHAR pngExt[] = {We('.'),We('p'),We('n'),We('g'),We(0)};
			name += pngExt;
			CPNGHandler::Write(name.c_str(), pSurface);
		}
		break;
		default: break;
	}
}

//*****************************************************************************
UINT CScreen::ShowTextInputMessage(
//Display a message in a dialog, prompting user for input.
//Dialog has an OK and Cancel button and execution
//waits for a button to be pushed before returning.
//
//Params:
	WSTRING &wstrUserInput,               //(in/out) Default text and text entered by user
	const TextInputDialogOptions options) //(in) Dialog options
//Returns:
//TAG_OK or TAG_CANCEL (button pressed).
//or TAG_QUIT (SDL_Quit was received).
{
	CLabelWidget *pText = DYN_CAST(CLabelWidget*, CWidget*,
			this->pInputTextDialog->GetWidget(TAG_TEXT));
	pText->SetText(options.pwczMessage);

	//Resize label text for text height.
	UINT wIgnored;
	SDL_Rect rect;
	pText->GetRect(rect);
	UINT wTextHeight;
	g_pTheFM->GetTextRectHeight(FONTLIB::F_Message,
			options.pwczMessage, rect.w, wIgnored, wTextHeight);
	pText->Resize(rect.w, wTextHeight);

	//Resize frame around text prompt to correct size.
	const int dy = wTextHeight - rect.h;
	CWidget *pWidget = this->pInputTextDialog->GetWidget(TAG_FRAME);
	pWidget->GetRect(rect);
	pWidget->Resize(rect.w, rect.h + dy);

	//Select single- or multi-line text entry.
	CTextBoxWidget *pTextBox;
	UINT dwTextBoxTag;
	UINT CY_TEXTBOX;
	if (options.bMultiline)
	{
		CTextBox2DWidget *pTextBox2D = DYN_CAST(CTextBox2DWidget*, CWidget*,
			   this->pInputTextDialog->GetWidget(TAG_TEXTBOX2D));
		pTextBox = static_cast<CTextBoxWidget*>(pTextBox2D);
		dwTextBoxTag = TAG_TEXTBOX2D;
		CY_TEXTBOX = CY_STANDARD_TBOX2D;
	} else {
		CTextBoxWidget *pTextBox1D = DYN_CAST(CTextBoxWidget*, CWidget*,
				this->pInputTextDialog->GetWidget(TAG_TEXTBOX));
		pTextBox = pTextBox1D;
		dwTextBoxTag = TAG_TEXTBOX;
		CY_TEXTBOX = CY_STANDARD_TBOX;
	}
	pTextBox->Move(X_TEXTBOX, wTextHeight + CY_SPACE * 2);
	pTextBox->SetText(wstrUserInput.c_str());
	pTextBox->SetDigitsOnly(options.bDigitsOnly);
	pTextBox->Show();

	this->pInputTextDialog->SetEnterText(options.bRequireText ? dwTextBoxTag : 0L);

	//Resize the rest of the dialog.
	const int yButtons = wTextHeight + CY_TEXTBOX + (CY_SPACE * 3);
	pWidget = this->pInputTextDialog->GetWidget(TAG_OK);
	pWidget->Move(X_OK2, yButtons);
	pWidget->Show();

	pWidget = this->pInputTextDialog->GetWidget(TAG_CANCEL_);
	pWidget->Move(X_CANCEL, yButtons);

	//Resize dialog to correct height.
	this->pInputTextDialog->GetRect(rect);
	this->pInputTextDialog->Resize(rect.w, yButtons + pWidget->GetH() + CY_SPACE);

	//Center the dialog on the screen.
	this->pInputTextDialog->Center();

	//Activate the dialog widget.
	bool bWasCursorVisible = IsCursorVisible();
	ShowCursor();
	const UINT dwRet = this->pInputTextDialog->Display();
	if (!bWasCursorVisible) HideCursor();
	pTextBox->Hide();

	//Repaint the screen to fix area underneath dialog.
	Paint(this->bUpdateRectAfterMessage);

	StopKeyRepeating();
	StopMouseRepeating();

	//Return text entered and tag.
	if (dwRet == TAG_OK)
		wstrUserInput = pTextBox->GetText();
	return dwRet;
}

//*****************************************************************************
UINT CScreen::ShowTextInputMessage(
//Display a message in a dialog, prompting user for input.
//Dialog has an OK and Cancel button and execution
//waits for a button to be pushed before returning.
//
//Params:
	const WCHAR* pwczMessage,  //(in)      Indicates message to display.
	WSTRING &wstrUserInput,    //(in/out)  Default text/text entered by user.
	const bool bMultiLineText, //(in)      if true, show 2D text box, else single-line text box
	const bool bMustEnterText) //(in)      if true, OK button is disabled
										//          when text box is empty
										//          (default = true)
//
//Returns:
//TAG_OK or TAG_CANCEL (button pressed).
//or TAG_QUIT (SDL_Quit was received).
{
	TextInputDialogOptions options(pwczMessage, bMultiLineText, bMustEnterText, false);
	
	return ShowTextInputMessage(wstrUserInput, options);
}

//*****************************************************************************
void CScreen::UpdateRect() const
//Updates entire dest (screen) surface.
{
	if (IsLocked()) return; //never call SDL_UpdateRect when surface is locked

	g_pTheBM->UpdateScreen(GetDestSurface());
}

//*****************************************************************************
void CScreen::UpdateRect(const SDL_Rect &rect) const
//Updates dest surface in this absolute screen rect.
{
	//Bounds checking.
	ASSERT(rect.x >= 0);
	ASSERT(rect.y >= 0);
	ASSERT(rect.x + rect.w <= CX_SCREEN);
	ASSERT(rect.y + rect.h <= CY_SCREEN);
	ASSERT(g_pTheBM);
	g_pTheBM->UpdateRect(rect);
}

//*****************************************************************************
void CScreen::ToggleScreenSize()
//Toggles between window and fullscreen mode.
{
	SetFullScreen(!IsFullScreen());
}

//
//Private methods.
//

//*****************************************************************************
UINT CScreen::ShowMessage(
//Display a message in a dialog.  Execution waits for a button to be pushed
//before returning.
//
//Params:
	const WCHAR* pwczMessage)  //(in)   Indicates message to display.
//
//Returns:
//TAG_YES (user pressed yes button), TAG_NO (user pressed no button),
//or TAG_QUIT (SDL_Quit was received).
{
	if (!pwczMessage)
	{
		WSTRING wstrErr;
		UTF8ToUnicode("Error: Could not retrieve message.", wstrErr);
		pwczMessage = wstrErr.c_str();
	}

	CLabelWidget *pText = DYN_CAST(CLabelWidget*, CWidget*,
			this->pMessageDialog->GetWidget(TAG_TEXT));
	pText->SetText(pwczMessage);

	//Resize label text for text height.
	SDL_Rect rect;
	pText->GetRect(rect);
	UINT wTextWidth, wTextHeight;
	g_pTheFM->GetTextRectHeight(FONTLIB::F_Message,
			pwczMessage, CX_TEXT, wTextWidth, wTextHeight);
	pText->SetHeight(wTextHeight);
	const int dy = (int) wTextHeight - (int) rect.h;

	//Get buttons width.  Frame must not be thinner than this.
	CWidget *pWidget = this->pMessageDialog->GetWidget(TAG_NO);
	UINT wButtonsWidth = pWidget->GetW();
	pWidget = this->pMessageDialog->GetWidget(TAG_YES);
	const UINT wFirstButtonW = pWidget->GetW() + CX_SPACE*2;
	wButtonsWidth += wFirstButtonW;
	if (wTextWidth < wButtonsWidth)
		wTextWidth = wButtonsWidth;

	//Resize frame to tightly fit buttons and text width.
	pWidget = this->pMessageDialog->GetWidget(TAG_FRAME);
	pWidget->Resize(wTextWidth + FRAME_BUFFER*2, pWidget->GetH() + dy);

	//Dialog is just wider than frame.
	this->pMessageDialog->GetRect(rect);
	const UINT wDialogWidth = pWidget->GetW() + FRAME_BUFFER*3;
	this->pMessageDialog->Resize(wDialogWidth, rect.h + dy);

	//Center buttons in new dialog width.
	int yButtons = wTextHeight + (CY_SPACE * 2);

	pWidget = this->pMessageDialog->GetWidget(TAG_YES);
	const UINT wFirstButtonX = (wDialogWidth-wButtonsWidth)/2;
	pWidget->Move(wFirstButtonX, yButtons);

	pWidget = this->pMessageDialog->GetWidget(TAG_NO);
	pWidget->Move(wFirstButtonX + wFirstButtonW, yButtons);

	//Center OK button.
	pWidget = this->pMessageDialog->GetWidget(TAG_OK);
	pWidget->Move((wDialogWidth - pWidget->GetW())/2, yButtons);

	//Center the dialog on the screen.
	this->pMessageDialog->Center();
	ASSERT(this->pMessageDialog->IsInsideOfParent()); //If this fires, probably the dialog text is too long.

	//Activate the dialog widget.
	bool bWasCursorVisible = IsCursorVisible();
	ShowCursor();
	UINT dwRet = this->pMessageDialog->Display();
	if (!bWasCursorVisible) HideCursor();

	//Repaint the screen to fix area underneath dialog.
	Paint(this->bUpdateRectAfterMessage);

	//Throw away anything causing possible side effects of having popped up this dialog.
	StopKeyRepeating();
	StopMouseRepeating();
	SDL_Event event;
	while (PollEvent(&event)) ;

	return dwRet;
}

//*****************************************************************************
void CScreen::ShowStatusMessage(
//Display a message in a dialog.  Execution continues.
//Call HideStatusMessage() to make dialog box disappear.
//
//Params:
	const WCHAR *pwczMessage)  //(in)   Indicates message to display.
//
//Returns:
//TAG_YES (user pressed yes button), TAG_NO (user pressed no button),
//or TAG_QUIT (SDL_Quit was received).
{
	if (!pwczMessage)
	{
		WSTRING wstrErr;
		UTF8ToUnicode("Error: Could not retrieve message.", wstrErr);
		pwczMessage = wstrErr.c_str();
	}
	CLabelWidget *pText = DYN_CAST(CLabelWidget*, CWidget*,
			this->pStatusDialog->GetWidget(TAG_TEXT));
	pText->SetText(pwczMessage);

	//Resize label text for text height.
	UINT wTextWidth, wTextHeight;
	g_pTheFM->GetTextRectHeight(FONTLIB::F_Message,
			pwczMessage, CX_TEXT, wTextWidth, wTextHeight);
	pText->SetHeight(wTextHeight);
	pText->Move((CX_MESSAGE - wTextWidth) / 2, Y_TEXT);

	//Resize the dialog.
	this->pStatusDialog->SetHeight(wTextHeight + CY_SPACE * 2);

	//Center the dialog on the screen.
	this->pStatusDialog->Center();

	//Activate the dialog widget.
	ShowCursor();
	this->pStatusDialog->Show();
	this->pStatusDialog->Paint();
	g_pTheBM->UpdateRects(GetDestSurface());
}

//*****************************************************************************
void CScreen::HideStatusMessage()
//Hide the status dialog.
{
	this->pStatusDialog->Hide();

	//Repaint the screen to fix area underneath dialog.
	Paint();
}

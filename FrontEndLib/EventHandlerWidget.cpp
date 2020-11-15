// $Id: EventHandlerWidget.cpp 10235 2012-06-05 13:17:04Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "EventHandlerWidget.h"
#include "OptionButtonWidget.h"
#include "BitmapManager.h"
#include "Sound.h"
#include "Screen.h"
#include <BackEndLib/Assert.h>

#ifdef STEAMBUILD
#	include <steam_api.h>
#endif

//A focus list iterator pointing to the end will mean that no widget is selected.
#define NO_SELECTION (this->FocusList.end())

//Key repeating is reset whenever a CEventHandlerWidget is activated.  For
//nested activations, I don't want the parents to have a repeating key when
//their call to activation occurs.  To accomplish this, one set of key repeat
//state vars is used for all event-handling widgets.
SDL_KeyboardEvent m_RepeatingKey; //Will be initialized in constructor.
UINT              m_dwLastKeyDown = 0;
UINT              m_dwLastKeyRepeat = 0;
UINT              m_dwKeyRepeatStart = 0;
UINT              m_dwLastUserInput = 0;

//************************************************************************************
Uint32 GetMouseState (int *x, int *y)
{
	Uint32 buttons = SDL_GetMouseState(x, y);
	ConvertPhysicalCoordsToLogical(x, y);
	return buttons;
}

int PollEvent (SDL_Event *event)
{
	if (!SDL_PollEvent(event))
		return 0;
	switch (event->type)
	{
		case SDL_MOUSEMOTION:
			ConvertPhysicalCoordsToLogical(&event->motion.x, &event->motion.y);
			//Better to leave relative coordinates unscaled?
			break;

		case SDL_MOUSEBUTTONUP:
		case SDL_MOUSEBUTTONDOWN:
			ConvertPhysicalCoordsToLogical(&event->button.x, &event->button.y);
			break;

		//(Don't scale the mouse wheel)

		case SDL_WINDOWEVENT:
			if (event->window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
				UpdateWindowSize(event->window.data1, event->window.data2);
			break;
	}
	return 1;
}

//************************************************************************************
CEventHandlerWidget::CEventHandlerWidget(
//Constructor.
//
	WIDGETTYPE eSetType,             //(in)   Required params for CWidget
	UINT dwSetTagNo,                   //    constructor.
	int nSetX, int nSetY,               //
	UINT wSetW, UINT wSetH)
	: CWidget(eSetType, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, bPaused(false)
	, bUpdateMotion(false)
	, bAllowRepeating(true)

	, pHeldDownWidget(NULL)
	, pHoveringWidget(NULL)
	, bIsFirstMouseDownRepeat(true)
	, dwLastMouseDownRepeat(0L)
	, wButtonIndex(0)

	, dwTimeOfLastClick(0L)
	, dwXOfLastClick(0L), dwYOfLastClick(0L)

	, bDeactivate(false)

	, dwBetweenEventsInterval(33L) //33ms = 30 frames per second.
	, dwLastOnBetweenEventsCall(SDL_GetTicks())

	, dwStartKeyRepeatDelay(300L)
	, dwContinueKeyRepeatDelay(33L)  //33ms = 30x per second.
	, dwWhenActivated(0)

	, pCallbackBetweenEventsObject(NULL)

	, bWindowIsVisible(true)
	, bWindowHasFocus(true)
{
	this->iSelectedWidget = NO_SELECTION;

	m_dwLastKeyDown = 0L;
	m_dwLastKeyRepeat = 0L;
	m_RepeatingKey.keysym.sym = SDLK_UNKNOWN;
}

//
// Public methods
//

//*****************************************************************************
UINT CEventHandlerWidget::GetTimeOfLastUserInput()
//Returns: m_dwLastUserInput
{
	if (!m_dwLastUserInput)
		m_dwLastUserInput = SDL_GetTicks();
	return m_dwLastUserInput;
}

//*****************************************************************************
void CEventHandlerWidget::StopKeyRepeating()
//Stop any current key from repeating.
{
	m_RepeatingKey.keysym.sym = SDLK_UNKNOWN;
}

//*****************************************************************************
void CEventHandlerWidget::StopMouseRepeating()
//Stop mouse button from repeating.
{
	this->pHeldDownWidget = NULL;
}

//******************************************************************************
CWidget * CEventHandlerWidget::GetSelectedWidget()
//Returns the selected widget or NULL if no selectable widgets available.
{
	if (this->FocusList.size()==0 || this->iSelectedWidget == NO_SELECTION) 
		return NULL;

	CWidget *pWidget = *(this->iSelectedWidget);
	if (!pWidget->IsSelectable(true))
		return NULL; //if selected widget was disabled
	return pWidget;
}

//******************************************************************************
void CEventHandlerWidget::SetBetweenEventsInterval(
//Set the interval between calls to OnBetweenEvents().
//
//Params:
	const UINT dwSetMSecs) //(in)   Interval expressed in milliseconds.
{
	this->dwBetweenEventsInterval = dwSetMSecs;
}

//******************************************************************************
void CEventHandlerWidget::SetKeyRepeat(
//Set the interval between key repeats.
//
//Params:
	const UINT dwContinueMSecs,  //(in) Interval between repeats
	const UINT dwStartMSecs)     //(in) Interval before starting repeat
{
	this->dwStartKeyRepeatDelay = dwStartMSecs;
	this->dwContinueKeyRepeatDelay = dwContinueMSecs;
}

//******************************************************************************
void CEventHandlerWidget::Activate()
//Handle the input loop while event-handling widget is active.  
//The method exits when Deactivate() is called.
{
	StopKeyRepeating();
	StopMouseRepeating();

	//Process events in loop below.
	//To avoid difficult-to-debug intermittent errors, no event-handling should
	//occur after deactivation.  I.e. don't handle a SDL_KEYUP when SDL_KEYDOWN
	//handling caused deactivation.  You are not guaranteed to receive a matching
	//SDL_KEYUP in every case, so it is best to consistently see an error caused
	//by not receiving the SDL_KEYUP.

	this->dwWhenActivated = SDL_GetTicks();

	SDL_Event event;
	this->bDeactivate = false; //A callee will call Deactivate() which sets this.
	while (!this->bDeactivate)
	{
		//Process one frame.
		CScreen::dwCurrentTicks = SDL_GetTicks();

		//Get any events waiting in the queue.
		while (!this->bDeactivate && PollEvent(&event))
			HandleEvent(event);

		if (!this->bDeactivate)
			Activate_HandleBetweenEvents();

		//Update music (switch song or continue music fade if one is in progress).
		g_pTheSound->UpdateMusic();

		//Draw all changes made during this frame on screen.
		g_pTheBM->UpdateRects(GetWidgetScreenSurface());

		//As the last step of the loop, the game sleeps a tiny bit,
		//freeing up the processor on Windows and maybe other OSes.
		if (bWindowIsVisible || bWindowHasFocus)
		{
			if (bWindowIsVisible && bWindowHasFocus)
			{
				//This reduces CPU usage during idle moments.
				SDL_Delay(1);
			} else {
				//Make app less aggressive when it doesn't have focus.
				SDL_Delay(50);
			}
		} else {
			//Slow it down even more when minimized.
			SDL_Delay(200);
		}
	}

	//Let derived class handle deactivation tasks it may have.
	OnDeactivate();
}

//**********************************************************************************
void CEventHandlerWidget::HandleEvent(const SDL_Event& event)
//Handle event.
{
	switch (event.type)
	{
		case SDL_KEYDOWN:
			Activate_HandleKeyDown(event.key);
		break;

		case SDL_KEYUP:
			Activate_HandleKeyUp(event.key);
		break;

		case SDL_MOUSEBUTTONDOWN:
			Activate_HandleMouseDown(event.button);
		break;

		case SDL_MOUSEBUTTONUP:
			Activate_HandleMouseUp(event.button);
		break;

		case SDL_MOUSEMOTION:
			Activate_HandleMouseMotion(event.motion);
		break;

		case SDL_MOUSEWHEEL:
			Activate_HandleMouseWheel(event.wheel);
		break;

		case SDL_TEXTINPUT:
			Activate_HandleTextInput(event.text);
		break;

		case SDL_QUIT:
			Activate_HandleQuit();
		break;

		case SDL_WINDOWEVENT:
			Activate_HandleWindowEvent(event.window);
		break;

		default:
		break;
	}
}

//**********************************************************************************
void CEventHandlerWidget::RemoveHoveringWidget(
//Remove hovering widget from the event handler.
//
//Params:
	CWidget *pWidget) //(in)
{
	if (!pWidget || pWidget == this->pHoveringWidget)
		this->pHoveringWidget = NULL;
}

//
// Protected methods
//

//**********************************************************************************
void CEventHandlerWidget::AddAnimatedWidget(
//Add widget to list of widgets that will be animated between events.
//
//Params:
	CWidget *pWidget) //(in)
{
	ASSERT(pWidget->IsAnimated());
	this->AnimatedList.push_back(pWidget);
}

//**********************************************************************************
void CEventHandlerWidget::RemoveAnimatedWidget(
//Remove widget from list of widgets that will be animated between events.
//
//Params:
	CWidget *pWidget) //(in)
{
	ASSERT(pWidget->IsAnimated());
	this->AnimatedList.remove(pWidget);
}

//**********************************************************************************
void CEventHandlerWidget::AddFocusWidget(
//Add widget to list of focusable widgets that may be selected.
//
//Params:
	CWidget *pWidget) //(in)
{
	ASSERT(pWidget->IsFocusable());

	this->FocusList.push_back(pWidget);

	//If I don't have anything selected, and this widget is selectable, then
	//select it.
	if (this->iSelectedWidget == NO_SELECTION && pWidget->IsSelectable(true))
	{
		WIDGET_ITERATOR iLast = this->FocusList.end();
		--iLast;
		ChangeSelection(iLast, false);
	}
}

//**********************************************************************************
void CEventHandlerWidget::RemoveFocusWidget(
//Remove widget from list of focusable widgets that may be selected.
//
//Params:
	CWidget *pWidget) //(in)
{
	ASSERT(pWidget->IsFocusable());

	//Is the currently selected widget about to be removed?
	if (this->iSelectedWidget != NO_SELECTION &&
			*(this->iSelectedWidget) == pWidget) //Yes.
	{
		//Move selection to next widget, since this one is going away.
		WIDGET_ITERATOR iBefore = this->iSelectedWidget;
		SelectNextWidget();
		if (this->iSelectedWidget == iBefore) //This is the last widget.
			ChangeSelection(NO_SELECTION, true); //Select nothing.
	}

	this->FocusList.remove(pWidget);
}

//**********************************************************************************
void CEventHandlerWidget::ClearEvents()
//Clears all pending input events, except for quit signals and window events.
{
	SDL_SetModState(KMOD_NONE);
	StopKeyRepeating();
	StopMouseRepeating();
	ClearEventBuffer();
}

//**********************************************************************************
void CEventHandlerWidget::ClearEventBuffer()
{
	SDL_Event event;
	SDL_PumpEvents();
	do {
		//Don't ignore any quit or window events received at this time.
		if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_QUIT, SDL_QUIT))
			break;
		if (SDL_PeepEvents(&event, 1, SDL_PEEKEVENT, SDL_WINDOWEVENT, SDL_WINDOWEVENT))
			break;
	} while (SDL_PeepEvents(&event, 1, SDL_GETEVENT, SDL_FIRSTEVENT, SDL_LASTEVENT) == 1);

	//If key is no longer repeating, clear it too.
	if (m_RepeatingKey.keysym.sym != SDLK_UNKNOWN) // XXX
	{
		const Uint8 *keyState = SDL_GetKeyboardState(NULL);
		if (!keyState[m_RepeatingKey.keysym.scancode])
			m_RepeatingKey.keysym.sym = SDLK_UNKNOWN;
	}
}

//**********************************************************************************
void CEventHandlerWidget::ClearWidgets()
//Removes all widgets from the animated/focus lists.
{
	ChangeSelection(NO_SELECTION, false); //Select nothing.
	this->AnimatedList.clear();
	this->FocusList.clear();
}

//**********************************************************************************
void CEventHandlerWidget::Activate_HandleWindowEvent(
//Handles SDL_WINDOWEVENT event.
//
//Params:
	const SDL_WindowEvent &wevent)   //(in) Event to handle.
{
	OnWindowEvent(wevent);
}

//*****************************************************************************
void CEventHandlerWidget::OnWindowEvent(const SDL_WindowEvent &wevent)
//Always take care of this no matter what widget is receiving event commands.
{
	SDL_Window *window = SDL_GetWindowFromID(wevent.windowID);
	Uint32 wflags = SDL_GetWindowFlags(window);
	bWindowHasFocus = !!(wflags & SDL_WINDOW_INPUT_FOCUS);
	bWindowIsVisible = !(wflags & (SDL_WINDOW_HIDDEN | SDL_WINDOW_MINIMIZED));

	switch (wevent.event)
	{
		case SDL_WINDOWEVENT_FOCUS_GAINED:
			OnWindowEvent_GetFocus();
			break;

		case SDL_WINDOWEVENT_FOCUS_LOST:
			OnWindowEvent_LoseFocus();
			break;

		case SDL_WINDOWEVENT_EXPOSED:
		{
			CEventHandlerWidget *pHandler = GetEventHandlerWidget();
			if (pHandler)
			{
				//Ensure whole screen is being repainted.
				if (pHandler->pParent)
					pHandler->pParent->Paint();
				else
					pHandler->Paint();
			}
			RequestPaint();
			break;
		}

		default: break;
	}
}

//*****************************************************************************
void CEventHandlerWidget::OnWindowEvent_GetFocus()
{
	CBitmapManager::bGameHasFocus = true;
	g_pTheSound->Unmute();
	g_pTheSound->UnpauseSounds();
	ClearEvents();  // !!! do we still need this?
}

//*****************************************************************************
void CEventHandlerWidget::OnWindowEvent_LoseFocus()
{
	CBitmapManager::bGameHasFocus = false;
	//Disable sound/music when app is inactive.
	if (!g_pTheSound->bNoFocusPlaysMusic)
		g_pTheSound->Mute();
	else
		g_pTheSound->PauseSounds();
	ClearEvents();  // !!! do we still need this?
}

//**********************************************************************************
void CEventHandlerWidget::Activate_HandleBetweenEvents()
//Handle things that do not occur in response to an SDL event.
{
	const UINT dwNow = SDL_GetTicks();
	if (bWindowIsVisible || bWindowHasFocus)
	{
		static const UINT MOUSEDOWN_REPEAT_INITIAL_DELAY = 500;
		static const UINT MOUSEDOWN_REPEAT_CONTINUE_DELAY = 100;
		
		//If user has held a widget down long enough, call its HandleMouseDownRepeat().
		const UINT dwNow = SDL_GetTicks();
		if (this->bAllowRepeating &&
				this->pHeldDownWidget &&
				dwNow - this->dwLastMouseDownRepeat > ((this->bIsFirstMouseDownRepeat) ?
				MOUSEDOWN_REPEAT_INITIAL_DELAY : MOUSEDOWN_REPEAT_CONTINUE_DELAY) )
		{
			int nX, nY;
			GetMouseState(&nX, &nY);
			if (this->pHeldDownWidget->ContainsCoords(nX, nY))
			{
				//Populate an event struct to pass to the handler.
				SDL_MouseButtonEvent Button;
				memset(&Button, 0, sizeof(Button));
				Button.type = SDL_MOUSEBUTTONDOWN;
				Button.button = SDL_BUTTON_LEFT;
				Button.state = SDL_PRESSED;
				Button.clicks = 1;
				Button.x = nX;
				Button.y = nY;

				this->dwLastMouseDownRepeat = dwNow;
				this->bIsFirstMouseDownRepeat = false;
				this->pHeldDownWidget->HandleMouseDownRepeat(Button);
			} else {
				//Repeat drag, in the event the list needs to be scrolled up or down.
				SDL_MouseMotionEvent Motion;
				memset(&Motion, 0, sizeof(Motion));
				Motion.type = SDL_MOUSEMOTION;
				Motion.state = SDL_BUTTON_LMASK;
				Motion.x = nX;
				Motion.y = nY;

				this->dwLastMouseDownRepeat = dwNow;
				this->bIsFirstMouseDownRepeat = false;
				this->pHeldDownWidget->HandleDrag(Motion);
			}
		}

		//Check for a repeating keypress.
		UINT dwRepeatTagNo;
		if (this->bAllowRepeating && IsKeyRepeating(dwRepeatTagNo))
		{
			if (!CheckForSelectionChange(m_RepeatingKey))
			{
				CWidget *pSelectedWidget = GetSelectedWidget();
				//Call keydown and keyup handlers.
				m_RepeatingKey.type = SDL_KEYDOWN;
				m_RepeatingKey.state = SDL_PRESSED;
				if (pSelectedWidget)
				{
					pSelectedWidget->HandleKeyDown(m_RepeatingKey);
					if (!IsDeactivating())
						OnKeyDown(pSelectedWidget->GetTagNo(), m_RepeatingKey);
				}
				else
					OnKeyDown(dwRepeatTagNo, m_RepeatingKey);
				m_RepeatingKey.type = SDL_KEYUP;
				m_RepeatingKey.state = SDL_RELEASED;
				if (pSelectedWidget)
					pSelectedWidget->HandleKeyUp(m_RepeatingKey);
				else
					OnKeyUp(dwRepeatTagNo, m_RepeatingKey);
			}
		}
	}

	//Animate widgets and call between events handler if interval has elapsed.
	if (!this->bPaused &&
		!IsDeactivating() &&
		dwNow - this->dwLastOnBetweenEventsCall > this->dwBetweenEventsInterval)
	{
		//Animate widgets.
		for (WIDGET_ITERATOR iSeek = this->AnimatedList.begin();
				iSeek != this->AnimatedList.end(); ++iSeek)
		{
			if ((*iSeek)->IsVisible())
				(*iSeek)->HandleAnimate();
		}

		OnBetweenEvents();
		this->dwLastOnBetweenEventsCall = dwNow;

		//Handle moving widgets
		if (this->bUpdateMotion)
		{
			int nX, nY;
			Uint8 mb = GetMouseState(&nX, &nY);
			SDL_MouseMotionEvent nomotion;
			memset(&nomotion, 0, sizeof(nomotion));
			nomotion.type = SDL_MOUSEMOTION;
			nomotion.state = mb;
			nomotion.x = nX;
			nomotion.y = nY;
			Activate_HandleMouseMotion(nomotion);
		}
	}
}

//******************************************************************************
void CEventHandlerWidget::Activate_HandleQuit()
//Handles SDL_QUIT event.
{
	OnQuit();
}

//******************************************************************************
void CEventHandlerWidget::Activate_HandleTextInput(
//Handles SDL_TEXTINPUT event.
//
//Params:
	const SDL_TextInputEvent &TextInputEvent)   //(in) Event to handle.
{
	CWidget *pSelectedWidget = GetSelectedWidget();

	if (pSelectedWidget)
	{
		//Handle key down in selected widget.
		pSelectedWidget->HandleTextInput(TextInputEvent);

		if (IsDeactivating())
			return;  //if widget became inactive after key, stop handling event

		//Selected widget gets event.
		OnTextInput(pSelectedWidget->GetTagNo(), TextInputEvent);
	}
	else
	{
		//Event-handler gets event.
		OnTextInput(this->dwTagNo, TextInputEvent);
	}
}

//******************************************************************************
void CEventHandlerWidget::Activate_HandleKeyDown(
//Handles SDL_KEYDOWN event.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
{
	//Remember information that will be used for repeating the key press later
	//if user holds the key down long enough.
	if (KeyboardEvent.repeat != 0)
		return; //we're implementing our own key repeat logic here
	m_dwKeyRepeatStart = m_dwLastUserInput = m_dwLastKeyDown = SDL_GetTicks();
	m_dwLastKeyRepeat = m_dwLastKeyDown;
	m_RepeatingKey = KeyboardEvent;
	
	CWidget *pSelectedWidget = GetSelectedWidget();
	
	//Alt-enter does not affect focus or selected widget.
	if (!(KeyboardEvent.keysym.mod & KMOD_ALT &&
		KeyboardEvent.keysym.sym == SDLK_RETURN))
	{
		//If widget focus changes, then don't pass event to selected widget.
		if (!CheckForSelectionChange(KeyboardEvent))
		{
			//Check for widget affected by an ALT + hot key combination.
			//Ignore CTRL + key combinations.
			const UINT dwHotkeyTag = GetHotkeyTag(KeyboardEvent.keysym);
			CWidget *pHotkeyWidget = (KeyboardEvent.keysym.mod & KMOD_CTRL) ?
					NULL : GetWidget(dwHotkeyTag,true);
			if (pHotkeyWidget && !pHotkeyWidget->IsSelectable())
				pHotkeyWidget = NULL;
			if (pHotkeyWidget && (KeyboardEvent.keysym.mod & KMOD_ALT))
			{
				SelectWidget(pHotkeyWidget);
				SimulateMouseClick(pHotkeyWidget);
				OnClick(dwHotkeyTag);
			} else {
				if (pSelectedWidget)
				{
					//Handle key down in selected widget.
					pSelectedWidget->HandleKeyDown(KeyboardEvent);
					
					if (IsDeactivating())
						return;  //if widget became inactive after key, stop handling event
					
					//If selected widget doesn't accept text entry then hotkey
					//without ALT can be used for any widget.
					//If selected widget does accept text entry, it can use the Enter
					//keys to act as hotkeys for clicking an OK button.
					if (pHotkeyWidget)
					{
						if (!pSelectedWidget->AcceptsTextEntry() ||
							((KeyboardEvent.keysym.sym == SDLK_RETURN ||
							KeyboardEvent.keysym.sym == SDLK_KP_ENTER) &&
							dwHotkeyTag == pSelectedWidget->GetHotkeyTag(KeyboardEvent.keysym)))
						{
							SelectWidget(pHotkeyWidget);
							SimulateMouseClick(pHotkeyWidget);
							OnClick(dwHotkeyTag);
						}
					}
				}
			}
		}
	}
	if (IsDeactivating())
		return;  //if widget became inactive after hotkey, stop handling event

	//Call notifier.
	if (pSelectedWidget)
		//Selected widget gets event.
		OnKeyDown(pSelectedWidget->GetTagNo(), KeyboardEvent);
	else
		//Event-handler gets event.
		OnKeyDown(this->dwTagNo, KeyboardEvent);
}

//******************************************************************************
void CEventHandlerWidget::Activate_HandleKeyUp(
//Handles SDL_KEYUP event.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
{
	//Sometimes a key up event for the previous repeating key will come
	//in after the key down for the current repeating key.  To prevent cancelling
	//the current key's repeating, only stop key repeating when the key up event 
	//is for the current key.
	if (KeyboardEvent.keysym.sym == m_RepeatingKey.keysym.sym)
		StopKeyRepeating();
	
	OnKeyUp(this->dwTagNo, KeyboardEvent);
}

//******************************************************************************
void CEventHandlerWidget::Activate_HandleMouseDown(
//Handles SDL_MOUSEBUTTONDOWN event.
//
//Params:
	const SDL_MouseButtonEvent &Button) //(in) Event to handle.
{
	static const UINT DOUBLE_CLICK_DELAY = 400; //in ms
	static const UINT DOUBLE_CLICK_PROXIMITY = 10; //in pixels

	m_dwLastUserInput = SDL_GetTicks();

	//Reset these members so that actions repeating in response to a widget
	//being held down will be correctly timed in OnBetweenEvents().
	this->dwLastMouseDownRepeat = m_dwLastUserInput;
	this->bIsFirstMouseDownRepeat = true;
	this->wButtonIndex = Button.button;

	//Check for mouse down inside any widget.
	CWidget *pWidget = GetWidgetContainingCoords(Button.x, Button.y, WT_Unspecified);
	if (pWidget)
	{
		pWidget->HandleMouseDown(Button);
		this->pHeldDownWidget = pWidget;

		if (pWidget->IsSelectable())
			SelectWidget(pWidget);
	}

	//Check for double-click event.
	//Notice it is observed on the second mouse-down, not mouse-up.
	const UINT dwTagNo = pWidget ? pWidget->GetTagNo() : this->dwTagNo;
   if (static_cast<UINT>(abs(Button.x - (int)this->dwXOfLastClick) +
         abs(Button.y - (int)this->dwYOfLastClick)) < DOUBLE_CLICK_PROXIMITY &&
			(SDL_GetTicks() - this->dwTimeOfLastClick) < DOUBLE_CLICK_DELAY)
	{
		OnDoubleClick(dwTagNo);
		this->dwTimeOfLastClick = 0;  //reset for next double-click

		//A double-click stops holding down immediately,
		//except for widgets where each click is always distinct.
		if (!pWidget || pWidget->IsDoubleClickable())
			this->pHeldDownWidget = NULL;
	} else {
		OnMouseDown(dwTagNo, Button);

      //Store the details so that we can process a possible double-click later.
      this->dwTimeOfLastClick = SDL_GetTicks();
      this->dwXOfLastClick = Button.x;
      this->dwYOfLastClick = Button.y;
	}
}

//******************************************************************************
void CEventHandlerWidget::Activate_HandleMouseUp(
//Handles SDL_MOUSEBUTTONDOWN event.
//
//Params:
	const SDL_MouseButtonEvent &Button) //(in) Event to handle.
{
	//Mouse wheel events don't actually cause a mouse up event.
	if (Button.button > 3) return;

	this->dwLastMouseDownRepeat = 0;

	//Get these pointers now.
	CWidget *pHeldWidget = this->pHeldDownWidget;
	CWidget *pWidget = GetWidgetContainingCoords(Button.x, Button.y, WT_Unspecified);

	//If already holding down a widget, that widget handles the mouse up
	//regardless of coords.
	if (pHeldWidget)
	{
		//Store value to avoid possible crash after HandleMouseUp().
		const UINT dwHeldWidgetTag = pHeldWidget->GetTagNo();

		pHeldWidget->HandleMouseUp(Button);
		//If a widget was being dragged, notify the dragging has stopped.
		OnDragUp(dwHeldWidgetTag, Button);
	}

	//Check for mouse up inside any other widgets.
	if (pWidget && pWidget->IsVisible())
	{
		const UINT dwEventTagNo = pWidget->GetTagNo();
		if (pWidget == pHeldWidget) 
			//A click occurs if the mouseup occurred within same widget as mousedown.
			//For some controls, like buttons, OnClick() means the user chose to 
			//perform that button's action, when OnMouseUp() would be an unreliable
			//indicator.
			OnClick(dwEventTagNo);
		else
			pWidget->HandleMouseUp(Button);
		OnMouseUp(dwEventTagNo, Button);
	}
	else
	{
		//If exec got here, then the mouse up event is for the event-handler widget.
		OnMouseUp(this->dwTagNo, Button);
	}
	this->pHeldDownWidget = NULL;
}

//******************************************************************************
void CEventHandlerWidget::Activate_HandleMouseMotion(
//Handles SDL_MOUSEMOTION event.
//
//Params:
	const SDL_MouseMotionEvent &Motion) //(in) Event to handle.
{
	//Note that zero-motion events might be generated by moving widgets to
	//get the event handler to update hovering status and such
	if (this->bUpdateMotion)
		//Updating motion now, so don't do it again unless we get another request
		this->bUpdateMotion = false;
	else
		//Not a code-synthesized event.
		m_dwLastUserInput = SDL_GetTicks();

	//Handle mouse motion for any dragging widget.
	if ((Motion.xrel || Motion.yrel) && this->pHeldDownWidget)
		this->pHeldDownWidget->HandleDrag(Motion);

	CWidget *pWidget = GetWidgetContainingCoords(Motion.x, Motion.y, WT_Unspecified);
	//Send mouse out event for a hoverable child widget that the mouse just left.
	if (this->pHoveringWidget && this->pHoveringWidget != pWidget)
	{
		this->pHoveringWidget->HandleMouseOut();
		OnMouseOut(this->pHoveringWidget->GetTagNo());
	}
	//Send mouse motion event for a child widget that mouse is over.
	if (pWidget)
	{
		this->pHoveringWidget = pWidget->IsHoverable() ? pWidget : NULL;
		pWidget->HandleMouseMotion(Motion);
		OnMouseMotion(pWidget->GetTagNo(), Motion);
	}
	else
	{
		this->pHoveringWidget = NULL;
		OnMouseMotion(this->dwTagNo, Motion);
	}
}

//*****************************************************************************
void CEventHandlerWidget::Activate_HandleMouseWheel(
//Handles SDL_MOUSEWHEEL event.
//
//Params:
	const SDL_MouseWheelEvent &Wheel) //(in) Event to handle.
{
	CWidget *pWidget = GetSelectedWidget();
	if (pWidget)
		pWidget->HandleMouseWheel(Wheel);
	else
		HandleMouseWheel(Wheel);
	OnMouseWheel(Wheel);
}

//*****************************************************************************
bool CEventHandlerWidget::IsKeyRepeating(
//Handles repeating keypresses.
//Repeat key after an initial delay, and then continue repeating after a smaller
//delay.
//
//Params:
	UINT &dwRepeatTagNo)   //(out)  Tag# of widget receiving key repeat.
							//    Not set if method returns false.
//
//Returns:
//True if a key is repeating, false if not.
{
	if (m_RepeatingKey.keysym.sym != SDLK_UNKNOWN) 
	{
		const Uint32 dwNow = SDL_GetTicks();
		if (dwNow - m_dwLastKeyDown > dwStartKeyRepeatDelay)
		{
#ifdef __APPLE__  // sometimes key-up events arrive noticeably late
			SDL_PumpEvents();
			const Uint8 *keyState = SDL_GetKeyboardState(NULL);
			if (!keyState[m_RepeatingKey.keysym.scancode])
				return false;
#endif
			//key has been held down long enough to repeat
			if (dwNow - m_dwLastKeyRepeat > dwContinueKeyRepeatDelay)
			{
				dwRepeatTagNo = this->dwTagNo;
				m_dwLastKeyRepeat = dwNow;
				return true;
			}
		}
	}

	return false;
}

//
//Private methods.
//

//*************************************************************************************
void CEventHandlerWidget::SelectFirstWidget(
//Select first selectable widget.
//
//Params:
	const bool bPaint)         //(in)   If true (default) then widget selection will
								//    be painted.
{
	//Exit early if focus list contains no widgets.
	if (this->FocusList.size()==0) 
	{
		this->iSelectedWidget = NO_SELECTION;
		return;
	}

	//Search for first selectable widget from beginning of focus list.
	for (WIDGET_ITERATOR iSeek = this->FocusList.begin(); iSeek != this->FocusList.end();
			++iSeek)
	{
		CWidget *pWidget = *iSeek;
		if (pWidget && pWidget->IsSelectable(!bPaint))
		{
			ChangeSelection(iSeek, bPaint);
			return;
		}
	}

	//There are no selectable widgets.
	ChangeSelection(NO_SELECTION, bPaint);
}

//*************************************************************************************
bool CEventHandlerWidget::SelectNextWidget(
//Select next selectable widget.
//Returns: whether previous widget was selected
//
//Params:
	const bool bPaint)         //(in)   If true (default) then widget selection will
								//    be painted.
{
	//Exit early if focus list contains no widgets.
	if (this->FocusList.size()==0) 
	{
		this->iSelectedWidget = NO_SELECTION;
		return false;
	}

	WIDGET_ITERATOR iSeek = this->iSelectedWidget;
	
	//Search for next selectable widget to end of focus list.
	CWidget *pWidget;
	if (iSeek != this->FocusList.end())
	{
		for (++iSeek; iSeek != this->FocusList.end(); ++iSeek)
		{
			pWidget = *iSeek;
			if (pWidget && pWidget->IsSelectable(!bPaint))
			{
				ChangeSelection(iSeek, bPaint);
				return true;
			}
		}
	}

	//Search for next selectable widget from beginning of focus list.
	iSeek = this->FocusList.begin();
	do
	{
		pWidget = *iSeek;
		if (pWidget && pWidget->IsSelectable(!bPaint))
		{
			ChangeSelection(iSeek, bPaint);
			return true;
		}
		++iSeek;
	} while (iSeek != this->iSelectedWidget && iSeek != this->FocusList.end());

	//There is only one focus widget.  Keep it selected.
	if (iSeek == this->iSelectedWidget)
		return false;

	//There are no selectable widgets.
	ChangeSelection(NO_SELECTION, bPaint);
	return false;
}

//*************************************************************************************
bool CEventHandlerWidget::SelectPrevWidget(
//Select previous selectable widget.
//Returns: whether previous widget was selected
//
//Params:
	const bool bPaint)         //(in)   If true (default) then widget selection will
								//    be painted.
{
	//Exit early if focus list contains no widgets.
	if (this->FocusList.size()==0) 
	{
		this->iSelectedWidget = NO_SELECTION;
		return false;
	}

	WIDGET_ITERATOR iSeek = this->iSelectedWidget;
	
	//Search for next selectable widget to beginning of focus list.
	CWidget *pWidget;
	if (iSeek != this->FocusList.begin())
	{
		--iSeek;
		do
		{
			pWidget = *iSeek;
			if (pWidget && pWidget->IsSelectable(!bPaint))
			{
				ChangeSelection(iSeek, bPaint);
				return true;
			}
			--iSeek;
		}
		while (iSeek != this->FocusList.begin());
	}

	//Search for next selectable widget from end of focus list.
	iSeek = this->FocusList.end();
	--iSeek;
	do
	{
		pWidget = *iSeek;
		if (pWidget && pWidget->IsSelectable(!bPaint))
		{
			ChangeSelection(iSeek, bPaint);
			return true;
		}
		--iSeek;
	}
	while (iSeek != this->iSelectedWidget);

	//There is only one focus widget.  Keep it selected.
	if (iSeek == this->iSelectedWidget)
		return false;

	//There are no selectable widgets.
	ChangeSelection(NO_SELECTION, bPaint);
	return false;
}

//*************************************************************************************
void CEventHandlerWidget::SelectWidget(
//Select a specific widget.
//
//Params:
	CWidget *pWidget,       //(in)   Widget to select.
	const bool bPaint)         //(in)   If true (default) then widget selection will
								//    be painted.
{
	if (pWidget && pWidget->IsSelectable(!bPaint)) //allow selecting a widget with
		//an invisible parent widget only if the selected widget is not to be painted
	{
		for (WIDGET_ITERATOR iSeek = this->FocusList.begin(); 
				iSeek != this->FocusList.end(); ++iSeek)
		{
			if (*iSeek == pWidget)
			{
				ChangeSelection(iSeek, bPaint);
				return;
			}
		}

		ASSERT(!"Caller is trying to select a widget that isn't in the focus list.");
	}
	else
		ASSERT(!"Caller is trying to select a nonexistent or unselectable widget.");
}
void CEventHandlerWidget::SelectWidget(
//Overload to select by widget tag#.
//
//Params:
	const UINT dwSelectTagNo, //(in)   Widget to select.
	const bool bPaint)         //(in)   If true (default) then widget selection will
								//    be painted.
{
	ASSERT(dwSelectTagNo);

	CWidget *pWidget = GetWidget(dwSelectTagNo);
	SelectWidget(pWidget, bPaint);
}

//*************************************************************************************
void CEventHandlerWidget::ChangeSelection(
//Change the selected widget to a new widget.
//
//Params:
	WIDGET_ITERATOR iSelect,      //(in)   Widget to select.  Passing NO_SELECTION  
									//    will cause no selection.
	const bool bPaint)            //(in)   If true then widget selection will
									//    be painted.
{
	if (this->iSelectedWidget == iSelect) return; //Nothing to do.

	if (this->iSelectedWidget != NO_SELECTION)
	{
		CFocusWidget *pFrom = DYN_CAST(CFocusWidget *, CWidget *, *(this->iSelectedWidget));
		pFrom->Unselect(bPaint);
	}

	if (iSelect != NO_SELECTION)
	{
		CFocusWidget *pTo = DYN_CAST(CFocusWidget *, CWidget *, *(iSelect));
		pTo->Select(bPaint);
	}
	this->iSelectedWidget = iSelect;
}

//**********************************************************************************
bool CEventHandlerWidget::CheckForSelectionChange(
//Returns true if widget selection is changed by KeyboardEvent.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
{
	 //One not-obvious thing below: alt-tab is ignored to prevent a weird effect with
	 //alt-tabbing in Windows.  Unfortunately SDL can't seem to detect when the alt-tab
	 //menu has appeared, otherwise I might have handled this by disabling all unput while
	 //alt-tab menu is visible.

	if (KeyboardEvent.keysym.sym == SDLK_TAB && !(KeyboardEvent.keysym.mod & KMOD_ALT))
	{
		if (KeyboardEvent.keysym.mod & KMOD_SHIFT)
			return SelectPrevWidget();
		return SelectNextWidget();
	}
	return false;
}

//**********************************************************************************
void CEventHandlerWidget::SimulateMouseClick(CWidget *pWidget)
//Simulate a mouse click on the widget.
{
	if (!pWidget) return;

	SDL_MouseButtonEvent Button;
	memset(&Button, 0, sizeof(Button));
	Button.type = SDL_MOUSEBUTTONDOWN;
	Button.button = SDL_BUTTON_LEFT;
	Button.clicks = 1;
	Button.state = SDL_PRESSED;
	pWidget->HandleMouseDown(Button);
	Button.state = SDL_RELEASED;
	pWidget->HandleMouseUp(Button);
}

//***************************************************************************
UINT CEventHandlerWidget::GetHotkeyTag(
	//Returns: tag attached to hotkey 'key' for this object, focused object or any of the children if it exists, else 0.
	//
	//Params:
	const SDL_Keysym& keysym) //(in)
{
	//Only check active widgets.
	if (!IsActive())
		return 0;

	UINT wTag = this->GetHotkeyTagInSelf(keysym);
	if (wTag == 0 && this->iSelectedWidget != NO_SELECTION){
		CWidget *pWidget = *(this->iSelectedWidget);
		wTag = pWidget->GetHotkeyTag(keysym);
	}
	if (wTag == 0){
		wTag = this->GetHotkeyTagInChildren(keysym);
	}

	//No hotkey mappings found for this key.
	return wTag;
}

//***************************************************************************
Uint32 CEventHandlerWidget::GetKeyRepeatDuration() const
{
	return m_dwKeyRepeatStart ? SDL_GetTicks() - m_dwKeyRepeatStart : 0;
}
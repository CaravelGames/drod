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

//SUMMARY
//
//A class derived from CEventHandlerWidget is responsible for handling events, sending
//event notifications to derived classes, and providing control methods for selecting
//focusable widgets.
//
//USAGE
//
//Derive your new widget class from CEventHandlerWidget or the two classes which
//are derived from CEventHandlerWidget--CDialogWidget and CScreenWidget.  Default UI 
//behaviour will be given to your class.  Overide the event notifier methods for
//events you are interested in.  For example, if you want to play a song when the user
//presses the "Play Song" button, override OnClick() in your derived class.
//
//Event notifiers that apply to a specific widget will receive a dwTagNo parameter
//indicating to which widget the event applied.  Event notifiers corresponding directly 
//to an SDL event will receive SDL event information in a parameter.  See descriptions 
//of event notifiers below to determine what criteria must be true before they are called.
//
//Event polling and notification begin when Activate() is called, and end when
//Deactivate() is called.  If you are deriving from CScreen, you should leave 
//these calls to the CScreenManager loop, and just use GoToScreen() to specify the
//next destination screen.  If you are deriving from CDialog, you should use the
//all-in-one CDialog::Display() method, which handles activation and deactivation.
//
//You can explicitly change the focus by using one of the widget selection methods.  
//Default focus-changing behaviour is provided without this.
//
//ADDING NEW EVENT NOTIFIERS
//
//It should be pretty straightforward if you look at the other notifiers and follow their
//example.  One design consideration is that you can call an event notifier from 
//widget event-handling code, as opposed to calling from this class.  For example, 
//OnSelectChange() is called by CListBoxWidget when it learns that its selection has 
//changed.  The information needed to determine CListBoxWidget's selection change is
//not available from CEventHandlerWidget, so the solution is to call the notifier
//from CListBoxWidget.
//
//GETTING NEW WIDGETS HOOKED IN
//
//If your widget doesn't respond to input, then there is nothing extra to do.  It will
//not be affected by events.
//
//Derive your new widget from CFocusWidget if it should be selectable.  Your painting
//code should call CFocusWidget::IsSelected() and paint for the two different states.
//
//If your widget responds to input, look at the CWidget::Handle*() methods.  
//You should write your widget to respond to the existing Handle*() methods that are
//already called from CEventHandlerWidget, and not add widget-type-specific code 
//into CEventHandlerWidget.  If the existing Handle*() methods do not provide the 
//appropriate stimuli for your widget to take correct behaviour, try to make 
//modifications that are not special-cased to your widget's event-handling, but 
//general to all widgets.
//
//The Handle*() methods are described in Widget.h.

#ifndef EVENTHANDLERWIDGET_H
#define EVENTHANDLERWIDGET_H

#include "Widget.h"
#include <BackEndLib/Assert.h>

#include <list>
using std::list;

class CScreenManager;
class CRoomScreen;
class CEventHandlerWidget : public CWidget
{
	friend class CRoomScreen;
public:
	void        ClearEventBuffer();
	void        ClearEvents();
	static UINT GetTimeOfLastUserInput();
	virtual UINT GetHotkeyTag(const SDL_Keysym& keysym);
	CWidget *   GetSelectedWidget();
	UINT        GetTimeActivated() const { return dwWhenActivated; }
	void        HandleEvent(const SDL_Event& event);
	inline bool IsDeactivating() const {return this->bDeactivate;}
	void        SelectFirstWidget(const bool bPaint = true);
	bool        SelectNextWidget(const bool bPaint = true);
	bool        SelectPrevWidget(const bool bPaint = true);
	void        SelectWidget(const UINT dwSelectTagNo, const bool bPaint = true);
	void        SelectWidget(CWidget *pSelectWidget, const bool bPaint = true);
	void        SetBetweenEventsHandler(CEventHandlerWidget *pHandler)
	{
		this->pCallbackBetweenEventsObject = pHandler;
	}
	void        StopKeyRepeating();
	void        StopMouseRepeating();

	CWidget *   MouseDraggingInWidget() const {return this->pHeldDownWidget;}
	bool        RightMouseButton() const {return this->wButtonIndex == SDL_BUTTON_RIGHT;}

	void        RemoveHoveringWidget(CWidget *pWidget);
	void        UpdateMotion() { this->bUpdateMotion = true; }

	inline bool WindowIsVisible() const { return bWindowIsVisible; }
	inline bool WindowHasFocus() const { return bWindowHasFocus; }

	//
	//Overridable event notifiers.
	//

	virtual void   OnBetweenEvents() {
		//A different object may be specified for handling this object's betweenEvents
		//occurrences, which is simpler than overriding a class to change behavior.
		if (this->pCallbackBetweenEventsObject)
			this->pCallbackBetweenEventsObject->OnBetweenEvents();
	}
	//Called periodically when no events are being processed.  The guaranteed minimum
	//interval can be set by SetBetweenEventsInterval() and defaults to 33ms (30 fps).

	virtual void   OnSelectChange(const UINT /*dwTagNo*/) { }
	//Called when a widget's selection changes.  Not every widget is used to select
	//information, and it is up to event-handling code within the widget to decide
	//when its selection has changed.

	virtual void   OnClick(const UINT /*dwTagNo*/) { }
	//Called when a widget which uses the button metaphor has been chosen by the user.
	//This choice could be made with a matching mousedown/mouseup pair, hotkey press, or
	//other criteria chosen by event-handling code within the widget.

	virtual void   OnDeactivate() { }
	//Called right after the event loop has exited.

	virtual void   OnDoubleClick(const UINT /*dwTagNo*/) { }
	//Called when two OnClick events are received within a short time of each other.

	virtual void   OnDragUp(const UINT /*dwTagNo*/, const SDL_MouseButtonEvent &/*Button*/) { }
	//Called when a widget previously received a mouse down within its area,
	//and the mouse button has been released.  The mouse pointer may or may not be within
	//the widget area.

	virtual void   OnKeyDown(const UINT /*dwTagNo*/, const SDL_KeyboardEvent &/*Key*/) { }
	//Called when a key has been pressed.  If a widget is selected that is affected by
	//keydown events, the dwTagNo param will be set to the selected widget's tag#.
	//Otherwise dwTagNo will be set to the event-handling widget's tag#.
	//Don't use this for text input; use OnTextInput instead.

	virtual void   OnKeyUp(const UINT /*dwTagNo*/, const SDL_KeyboardEvent &/*Key*/) { }
	//Called when a key has been released.  For now, dwTagNo will always be set to the
	//event-handling widget's tag#.  If there is a need to respond to key releases inside
	//of specific widgets, I recommend making changes to Activate_HandleKeyUp().

	virtual void   OnMouseDown(const UINT /*dwTagNo*/, const SDL_MouseButtonEvent &/*Button*/) { }
	//Called when a mouse button has been pressed.  If the coords of the click are within
	//a specific widget, dwTagNo will be set to that widget's tag#.  Otherwise dwTagNo
	//will be set to the event-handling widget's tag#.

	virtual void   OnMouseUp(const UINT /*dwTagNo*/, const SDL_MouseButtonEvent &/*Button*/) { }
	//Called when a mouse button has been released.  If the coords of the click are within
	//a specific widget, dwTagNo will be set to that widget's tag#.  Otherwise dwTagNo
	//will be set to the event-handling widget's tag#.

	virtual void   OnMouseMotion(const UINT /*dwTagNo*/, const SDL_MouseMotionEvent &/*Motion*/) { }
	//Called when the mouse is moving.  If the coords of the click are within
	//a specific widget, dwTagNo will be set to that widget's tag#.  Otherwise dwTagNo
	//will be set to the event-handling widget's tag#.

	virtual void   OnMouseWheel(const SDL_MouseWheelEvent &/*Wheel*/) { }
	//Called when the mouse wheel has been rolled.

	virtual void   OnMouseOut(const UINT /*dwTagNo*/) { }
	//Called when the mouse has left the specified hoverable widget.

	virtual bool   OnQuit() {return false;}
	//Called when user sends an application exit signal.  (Probably clicked on close window
	//button.)

	virtual void   OnRearranged(const UINT /*dwTagNo*/) { }
	//Called when a widget's items have been reordered, possibly indicating a
	//change in state that needs to be updated in the model.

	virtual void   OnTextInput(const UINT /*dwTagNo*/, const SDL_TextInputEvent &/*text*/) { }
	//Called when some text has been input.

	virtual void   OnWindowEvent(const SDL_WindowEvent &wevent);
	//Called when the window received an event, including losing/gaining
	//focus, minimizing/restoring, and mouse entering/leaving window.

	virtual void   OnWindowEvent_GetFocus();
	//Called when window receives focus

	virtual void   OnWindowEvent_LoseFocus();
	//Called when window loses focus

protected:
	friend class CScreenManager;
	friend class CWidget;

	CEventHandlerWidget(WIDGETTYPE eSetType, UINT dwSetTagNo,
		int nSetX, int nSetY, UINT wSetW, UINT wSetH);
	virtual ~CEventHandlerWidget() {}

	void        Activate();
	void        AddAnimatedWidget(CWidget *pWidget);
	void        AddFocusWidget(CWidget *pWidget);
	void        ClearWidgets();
	void        Deactivate() {ASSERT(!this->bDeactivate); this->bDeactivate=true;}
	void        RemoveAnimatedWidget(CWidget *pWidget);
	void        RemoveFocusWidget(CWidget *pWidget);
	virtual bool   SetForActivate() {return true;}
	void        SetBetweenEventsInterval(const UINT dwSetMSecs);
	void        SetKeyRepeat(const UINT dwContinueMSecs, const UINT dwStartMSecs=300L);

	bool bPaused;  //whether animation is paused
	bool bUpdateMotion; //whether widgets moved
	bool bAllowRepeating; //whether widget supports automatic key/mouse repeating

private:
	//Event handling methods called directly by Activate().
	void        Activate_HandleBetweenEvents();
	void        Activate_HandleKeyDown(const SDL_KeyboardEvent &Key);
	void        Activate_HandleKeyUp(const SDL_KeyboardEvent &Key);
	void        Activate_HandleMouseDown(const SDL_MouseButtonEvent &Button);
	void        Activate_HandleMouseUp(const SDL_MouseButtonEvent &Button);
	void        Activate_HandleMouseMotion(const SDL_MouseMotionEvent &Motion);
	void        Activate_HandleMouseWheel(const SDL_MouseWheelEvent &Wheel);
	void        Activate_HandleQuit();
	void        Activate_HandleTextInput(const SDL_TextInputEvent &text);
	void        Activate_HandleWindowEvent(const SDL_WindowEvent &wevent);

	void        ChangeSelection(WIDGET_ITERATOR iSelect, const bool bPaint);
	bool        CheckForSelectionChange(const SDL_KeyboardEvent &KeyboardEvent);
	bool        IsKeyRepeating(UINT &dwRepeatTagNo);
	void        SimulateMouseClick(CWidget *pWidget);

	CWidget *      pHeldDownWidget;
	CWidget *      pHoveringWidget;
	bool        bIsFirstMouseDownRepeat;
	UINT       dwLastMouseDownRepeat;
	UINT        wButtonIndex;

	//for double-click
	UINT       dwTimeOfLastClick;
   UINT       dwXOfLastClick, dwYOfLastClick;

	bool        bDeactivate;
	UINT       dwBetweenEventsInterval;
	UINT       dwLastOnBetweenEventsCall;
	UINT       dwStartKeyRepeatDelay,  dwContinueKeyRepeatDelay;
	UINT       dwWhenActivated;

	list<CWidget *>   AnimatedList;
	list<CWidget *>   FocusList;
	WIDGET_ITERATOR   iSelectedWidget;

	//Hook for alternate object to override default (no-op) event handling.
	CEventHandlerWidget* pCallbackBetweenEventsObject;

	bool bWindowIsVisible;
	bool bWindowHasFocus;
};

int PollEvent(SDL_Event *event);
Uint32 GetMouseState(int *x, int *y);

#endif //#ifndef EVENTHANDLERWIDGET_H

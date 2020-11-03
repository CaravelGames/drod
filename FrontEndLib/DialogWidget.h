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

#ifndef DIALOGWIDGET_H
#define DIALOGWIDGET_H

#include "EventHandlerWidget.h"

//SUMMARY
//
//A dialog appears on top of a screen and takes over event-handling from the
//screen.  When the user presses a button, the dialog exits.
//
//USAGE
//
//Do the following in Load() method of CScreen or CScreen-derived class, before
//the call to LoadChildren():
//  1. Instance a new CDialogWidget.
//  2. Add child widgets to it for labels, buttons, frames, etc.  Use tag#s 
//     for buttons and other widgets so that you can identify which buttons
//     have been pressed.  A button without a tag# will not close the dialog.
//  3. Call CDialogWidget::Hide() so that dialog will not be visible right away.
//  4. Call AddWidget() to add the dialog to the screen.
//
//To activate the dialog and accept user input.
//  1. Call CDialogWidget::Display(). (Note that event-handling has passed to
//     the dialog at this point.)
//  2. Respond to the returned tag#.  If it is TAG_QUIT, you are obliged to
//     close the app down, probably by calling CScreen::GoToScreen(SCR_None)
//     after confirming the exit with the user.
//  3. Repaint the area underneath the now-hidden dialog--probably with a call
//     to C*Screen::Paint().  If you want to respond to button presses without
//     closing the dialog, you could skip the underneath repainting, and 
//     call Activate() on the dialog again.
//
//Dialog and associated resources will be freed in C*Screen::Unload() when it 
//makes its call to UnloadChildren().  Don't try to delete the CDialogWidget
//pointer.
//
//If you want to handle the input differently, derive a new class
//from CDialogWidget and override the On*() event notification methods.  
//CKeypressDialogWidget is an example of this.

//******************************************************************************
class CDialogWidget : public CEventHandlerWidget
{
public:
	CDialogWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
			const UINT wSetW, const UINT wSetH, const bool bListBoxDoubleClickReturns=false);

	virtual void   Center();
	UINT          Display(const bool bSelectFirstWidget=true);
	virtual void      Paint(bool bUpdateRect = true);

	//Call this to control in what screen area the dialog should appear centered.
	void           SetCenteringRect(const SDL_Rect& rect) {this->centerRect = rect;}

	void           SetEnterText(const UINT dwTagNo) {this->dwReqTextField = dwTagNo;}

protected:
	void              CheckTextBox();

	virtual bool   SetForActivate();

	virtual void      OnClick(const UINT dwTagNo);
	virtual void      OnDoubleClick(const UINT dwTagNo);
	virtual void      OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);
	virtual void      OnTextInput(const UINT dwTagNo, const SDL_TextInputEvent &text);
	virtual void      OnSelectChange(const UINT dwTagNo);
	virtual bool      OnQuit();
	virtual void      OnWindowEvent_GetFocus();
	virtual void      OnWindowEvent_LoseFocus();

	UINT          dwDeactivateValue;

private:
	UINT          dwReqTextField;   //when a text box is present as a child
	bool           bListBoxDoubleClickReturns;
	SDL_Rect       centerRect;
};

#endif //#ifndef DIALOGWIDGET_H

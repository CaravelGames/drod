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
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef TEXTBOXWIDGET_H
#define TEXTBOXWIDGET_H

#include "ScrollableWidget.h"
#include "FontManager.h"
#include <BackEndLib/Types.h>
#include <BackEndLib/Ports.h>

//Height of standard single-line text box.
static const UINT CY_STANDARD_TBOX = 30;

struct TextUndoInfo
{
	TextUndoInfo(const WSTRING& wstr, UINT cursorIndex)
		: text(wstr), cursorIndex(cursorIndex)
	{}
	TextUndoInfo(const WCHAR* pwText, UINT cursorIndex)
		: cursorIndex(cursorIndex)
	{
		if (pwText)
			this->text = pwText;
	}
	WSTRING text;
	UINT cursorIndex;
};

//******************************************************************************
class CTextBoxWidget : public CScrollableWidget
{
public:
	CTextBoxWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
			const UINT wSetW, const UINT wSetH, const UINT wMaxTextLength=255,
			const UINT enterSendsHotkey=TAG_OK);
	virtual ~CTextBoxWidget() { }

	virtual bool   AcceptsTextEntry() {return true;}
	const WCHAR *  GetText() const;
	int            GetNumber() const;
	void           DeleteAtCursor();
	virtual void   HandleDrag(const SDL_MouseMotionEvent &Motion);
	virtual void   HandleKeyDown(const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   HandleMouseDown(const SDL_MouseButtonEvent &MouseButtonEvent);
	virtual void   HandleMouseUp(const SDL_MouseButtonEvent &MouseButtonEvent);
	virtual void   HandleTextInput(const SDL_TextInputEvent &TextInputEvent);
	bool           InsertAtCursor(const WCHAR c, const bool bSetCursorIndex=true);
	bool           IsEmpty() const;
	virtual void   Paint(bool bUpdateRect = true);
	void           SetDigitsOnly(const bool bAccept = true)
			{this->bDigitsOnly = bAccept;}
	void           SetFilenameSafe(const bool bFlag = true)
			{this->bFilenameSafe = bFlag;}
	void           SetAllowNegative(const bool bAccept = true)
			{this->bAllowNegative = bAccept;}
	void           SetViewAsPassword(const bool bFlag = true)
			{this->bIsPassword = bFlag;}
	virtual void   SetCursorByPos(int xPos, int yPos);
	void           SetText(const WCHAR* newText);

protected:
	virtual void   CalcAreas() {}
	virtual bool   IsAnimated() const {return true;}
	virtual void   HandleAnimate();

	bool           CharIsWordbreak(const WCHAR wc) const;
	bool           CursorOnWhitespace() const;

	virtual void   DrawText(const int nOffsetX, const int nOffsetY);
	virtual void   DrawCursor(const int nOffsetX, const int nOffsetY);
	void           DrawCursorNow();

	bool           MoveCursorLeft();
	bool           MoveCursorLeftWord();
	bool           MoveCursorRight();
	bool           MoveCursorRightWord();

	virtual void   ClearSelection();
	bool           DeleteSelected();
	bool           HasSelection() const;
	bool           GetSelection(UINT &wStart, UINT &wEnd) const;
	void           SetSelection(const UINT wStart);

	virtual void   SelectAllText();
	virtual void   SetCursorIndex(const UINT wIndex);
	void           SetText(const WCHAR* newText, const bool bClearUndos);
	void           TypeCharacter(const WCHAR wc) { TypeCharacters(&wc, 1); }
	void           TypeCharacters(const WSTRING& wstr) { TypeCharacters(wstr.c_str(), wstr.length()); }
	void           TypeCharacters(const WCHAR* wcs, size_t length);
	virtual void   UpdateWidgetStats() {}

	void           redo();
	void           undo();

	WSTRING        text;
	WSTRING        passwordText;    //if it's a password, actual text is stored here
	UINT           wTextDisplayIndex;   //index of first character in display
	UINT           wMaxTextLength;
	UINT           wCursorIndex;     //cursor position in text
	UINT           fontType;
	bool           bDrawCursor;

	UINT           wMarkStart, wMarkEnd; //for marking text to cut/copy
	vector<TextUndoInfo> undoList, redoList;

private:
	UINT          dwLastCursorDraw; //cursor animation
	bool           bDigitsOnly;   //only allow digits to be input
	bool           bAllowNegative;   //allow negative numbers to be input
	bool           bFilenameSafe; //only allow certain chars to be input
	bool           bIsPassword;   //shows *s for text
};

#endif //#ifndef TEXTBOXWIDGET_H

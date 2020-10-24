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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2004, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer), Jamieson Cobleigh, Matthew Schikore (schik), Gerry Jo Jellestad (trick)
 *
 * ***** END LICENSE BLOCK ***** */

#include "TextBoxWidget.h"
#include "BitmapManager.h"
#include "EventHandlerWidget.h"
#include "FontManager.h"
#include "Inset.h"

#include <BackEndLib/Clipboard.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Assert.h>
#include <BackEndLib/Wchar.h>

#define ABS(a) ((a) > 0 ? (a) : -(a))
#define EDGE_OFFSET  (3)

//
//Public methods.
//

//******************************************************************************
CTextBoxWidget::CTextBoxWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,             //(in)   Required params for CWidget
	const int nSetX, const int nSetY,   //    constructor.
	const UINT wSetW, const UINT wSetH, //
	const UINT wMaxTextLength,          //(in)
	const UINT enterSendsHotkey)        //(in) whether hitting Enter will return
													//    a hotkey event [default = TAG_OK]
	: CScrollableWidget(dwSetTagNo, nSetX, nSetY, wSetW, wSetH, WT_TextBox)
	, wTextDisplayIndex(0)
	, wMaxTextLength(wMaxTextLength)
	, wCursorIndex(0)
	, fontType(FONTLIB::F_Message)
	, bDrawCursor(true)
	, wMarkStart((UINT)-1)
	, wMarkEnd((UINT)-1)
	, dwLastCursorDraw(0L)
	, bDigitsOnly(false)
	, bAllowNegative(false)
	, bFilenameSafe(false)
	, bIsPassword(false)
{
	this->imageFilenames.push_back(string("Dialog"));

	if (enterSendsHotkey)
	{
		//Enter key acts as hotkey to perform OK command in parent widget.
		AddHotkey(SDLK_RETURN,enterSendsHotkey);
		AddHotkey(SDLK_KP_ENTER,enterSendsHotkey);
	}
	this->text.reserve(wMaxTextLength);
	this->passwordText.reserve(wMaxTextLength);
}

//******************************************************************************
void CTextBoxWidget::Paint(
//Paint widget area.
//
//Params:
	bool bUpdateRect) //(in)   If true (default) and destination
						//    surface is the screen, the screen
						//    will be immediately updated in
						//    the widget's rect.
{
	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);

	//Draw inset over entire widget area.
	SDL_Surface *pScreenSurface = GetDestSurface();
	DrawInset(this->x + nOffsetX, this->y + nOffsetY, this->w, this->h,
			this->images[0], pScreenSurface, true, false, !IsEnabled());

	//Draw scroll bar if needed.
	const bool bDrawScrollBar = this->UpRect.h != 0;
	if (bDrawScrollBar)
		DrawVertScrollBar(pScreenSurface);

	DrawText(nOffsetX, nOffsetY);

	//Draw cursor if widget has focus.
	if (IsSelected())
		DrawCursor(nOffsetX, nOffsetY);

	PaintChildren(false);

	if (bUpdateRect) UpdateRect();
}

//******************************************************************************
void CTextBoxWidget::HandleKeyDown(
//Processes a keyboard event within the scope of the text box.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
{
	const SDL_Keycode key = KeyboardEvent.keysym.sym;
	const bool bShift = (KeyboardEvent.keysym.mod & KMOD_SHIFT) != 0;
	const UINT wOldCursorPos = this->wCursorIndex;

	// Keep the cursor visible while keys are being pressed.
	DrawCursorNow();
	RequestPaint();

	switch (key) {
	case SDLK_BACKSPACE:
		if (HasSelection())
			DeleteSelected();
		else if (MoveCursorLeft())
			DeleteAtCursor();
		break;
	case SDLK_DELETE:
	case SDLK_KP_PERIOD:
		if (!HasSelection())
			DeleteAtCursor();
		else
			DeleteSelected();
		break;
	case SDLK_LEFT:
	case SDLK_KP_4:
		if (KeyboardEvent.keysym.mod & KMOD_CTRL)
			MoveCursorLeftWord();
		else
			MoveCursorLeft();
		if (bShift) SetSelection(wOldCursorPos); else ClearSelection();
		break;
	case SDLK_RIGHT:
	case SDLK_KP_6:
		if (KeyboardEvent.keysym.mod & KMOD_CTRL)
			MoveCursorRightWord();
		else
			MoveCursorRight();
		if (bShift) SetSelection(wOldCursorPos); else ClearSelection();
		break;
	case SDLK_HOME:
	case SDLK_PAGEUP:
		SetCursorIndex(0);
		if (bShift) SetSelection(wOldCursorPos); else ClearSelection();
		break;
	case SDLK_END:
	case SDLK_PAGEDOWN:
		SetCursorIndex(this->text.size());
		if (bShift) SetSelection(wOldCursorPos); else ClearSelection();
		break;

	default:
#ifndef __APPLE__
		if (KeyboardEvent.keysym.mod & KMOD_CTRL)
#else
		if ((KeyboardEvent.keysym.mod & KMOD_CTRL) || (KeyboardEvent.keysym.mod & KMOD_GUI))
#endif
		{
			switch (key)
			{
				case SDLK_a: //Ctrl-A: select all
					SelectAllText();
				break;

				//COPYING AND PASTING
				case SDLK_v: // Paste
				{
					WSTRING insert;
					if (CClipboard::GetString(insert))
					{
						SanitizeText(insert);

						if (HasSelection())
							DeleteSelected();
						WSTRING curText = GetText();
						const UINT wOrigCursorIndex = this->wCursorIndex;
						for (WSTRING::const_iterator i = insert.begin(); i != insert.end(); ++i)
							InsertAtCursor(*i, i+1 == insert.end()); //quick update until last position
						if (curText != (this->bIsPassword ? this->passwordText : this->text))
						{
							this->undoList.push_back(TextUndoInfo(curText, wOrigCursorIndex));
							this->redoList.clear();
						}
					}
				}
				break;

				case SDLK_c:  // Copy
				{
					if (this->bIsPassword) break;
					if (!HasSelection()) break;

					UINT wStart, wEnd;
					GetSelection(wStart, wEnd);
					ASSERT(wEnd > wStart && this->text.size() >= (wEnd-wStart));

					CClipboard::SetString(this->text.substr(wStart, wEnd-wStart));
				}
				break;

				case SDLK_x:  // Cut
				{
					if (this->bIsPassword) break;
					if (!HasSelection()) break;

					UINT wStart, wEnd;
					GetSelection(wStart, wEnd);
					ASSERT(wEnd > wStart && this->text.size() >= (wEnd-wStart));

					if (CClipboard::SetString(this->text.substr(wStart, wEnd-wStart)))
					{
						this->undoList.push_back(TextUndoInfo(GetText(), this->wCursorIndex));
						this->redoList.clear();
						VERIFY(DeleteSelected());
					}
				}
				break;

				case SDLK_z:   //Undo
#ifdef __APPLE__
					// option-shift-Z is redo on Mac:
					if ( (KeyboardEvent.keysym.mod & KMOD_GUI) && (KeyboardEvent.keysym.mod & KMOD_SHIFT) )
					{
						redo();
						break;
					}
#endif
					undo();
				break;

				case SDLK_y:   //Redo
					redo();
				break;

				default: break;
			}
			RequestPaint();
		} else {
			//without modifier keys being held
			//(handled by text input event)
		}
		break;
	}

	CalcAreas();
}

//******************************************************************************
void CTextBoxWidget::HandleTextInput(
//Processes a text input event within the scope of the text box.
//
//Params:
	const SDL_TextInputEvent &text)   //(in) Event to handle.
{
	// Keep the cursor visible while keys are being pressed.
	DrawCursorNow();
	RequestPaint();

	WSTRING wtext;
	if (UTF8ToUnicode(text.text, wtext))
		TypeCharacters(wtext);

	CalcAreas();
}

//******************************************************************************
void CTextBoxWidget::HandleMouseDown(
//Processes a mouse event within the scope of the text box.
//
//Params:
	const SDL_MouseButtonEvent &MouseButtonEvent)   //(in) Event to handle.
{
	SetCursorByPos(MouseButtonEvent.x - this->x, MouseButtonEvent.y - this->y);
	if (MouseButtonEvent.state == SDL_PRESSED)
		this->wMarkStart = this->wMarkEnd = this->wCursorIndex;
	RequestPaint();
}

//******************************************************************************
void CTextBoxWidget::HandleDrag(
	const SDL_MouseMotionEvent &MouseMotionEvent)   //(in) Event to handle.
{
	SetCursorByPos(MouseMotionEvent.x - this->x, MouseMotionEvent.y - this->y);
	this->wMarkEnd = this->wCursorIndex;
	RequestPaint();
}

//******************************************************************************
void CTextBoxWidget::HandleMouseUp(
//Processes a mouse event within the scope of the text box.
//
//Params:
	const SDL_MouseButtonEvent &MouseButtonEvent)   //(in) Event to handle.
{
	SetCursorByPos(MouseButtonEvent.x - this->x, MouseButtonEvent.y - this->y);
	if (MouseButtonEvent.state == SDL_RELEASED)
	{
		this->wMarkEnd = this->wCursorIndex;
		if (this->wMarkEnd == this->wMarkStart)   //nothing really selected
			ClearSelection();
	}
	RequestPaint();
}

//******************************************************************************
void CTextBoxWidget::SelectAllText()
{
	this->wMarkStart = 0;
	this->wMarkEnd = this->text.size();
}

//******************************************************************************
void CTextBoxWidget::SetCursorByPos(
//Sets cursor to index of character rendered at xPos
//
//Params:
	int xPos, int /*yPos*/) //(in) cursor's new relative position (in pixels)
{
	xPos -= EDGE_OFFSET;

	const UINT wLength = this->text.size();
	if (!wLength)
	{
		this->wTextDisplayIndex = 0;
		SetCursorIndex(0);
		return;
	}
	if (xPos <= 0)
	{
		//Place cursor at beginning of display.  Slide view left if possible.
		if (this->wTextDisplayIndex)
			--this->wTextDisplayIndex;
		SetCursorIndex(this->wTextDisplayIndex);
	} else {
		//Calculate where to place cursor.
		UINT wIndex = 0, wWidth = 0, wLastWidth = 0;
		UINT wTextW, wTextH;
		WCHAR *wStr = new WCHAR[wLength+2];

		//If placing cursor past right edge of view, slide view right if possible.
		const int nEndPos = this->w - EDGE_OFFSET;
		if (xPos > nEndPos)
		{
			WCScpy(wStr, this->text.c_str() + this->wTextDisplayIndex);
			g_pTheFM->GetTextWidthHeight(this->fontType, wStr, wTextW, wTextH);
			if (static_cast<int>(wTextW) > nEndPos)
			{
				++this->wTextDisplayIndex;
				WCv(wStr[1]) = '\0';
				g_pTheFM->GetTextWidthHeight(this->fontType, wStr, wTextW, wTextH);
				xPos = nEndPos;
			}
		}

		WCv(wStr[0]) = '\0';
		while (wIndex <= wLength && (wIndex+this->wTextDisplayIndex < wLength) &&
				static_cast<int>(wWidth) < xPos)
		{
			//Calculate width of string up to the nth character.
			wStr[wIndex] = this->text[wIndex+this->wTextDisplayIndex];
			WCv(wStr[wIndex+1]) = '\0';
			g_pTheFM->GetTextWidthHeight(this->fontType, wStr, wTextW, wTextH);
			wLastWidth = wWidth;
			wWidth = wTextW;

			++wIndex;
		}
		delete[] wStr;

		if (wIndex > wLength)
			SetCursorIndex(wLength);
		else
			//Decide which side of the character that cursor falls on to place cursor.
			SetCursorIndex(ABS(static_cast<int>(wLastWidth) - xPos) <= ABS(static_cast<int>(wWidth) - xPos) ?
				wIndex+this->wTextDisplayIndex-1 : wIndex+this->wTextDisplayIndex);
	}

	DrawCursorNow(); //quick feedback

	CalcAreas(); //in case widget state has changed
}

//******************************************************************************
void CTextBoxWidget::SetCursorIndex(
//Sets cursor to index.  Updates displayed text region.
//
//Params:
	const UINT wIndex)   //(in) cursor's new position
{
	this->wCursorIndex = wIndex;
	if (this->wCursorIndex > this->text.size())
		this->wCursorIndex = this->text.size();

	//Set which part of string to display in box.
	if (this->wCursorIndex < this->wTextDisplayIndex)
		this->wTextDisplayIndex = this->wCursorIndex;
	if (this->wCursorIndex > 0)        //don't need to calc when at beginning of text
	{
		//Calculate what part of string to show.
		UINT wTextW, wTextH;
		int nOffsetX, nOffsetY;
		WCHAR *wStr = new WCHAR[this->text.size()+1];
		GetScrollOffset(nOffsetX, nOffsetY);
		do
		{
			WCSncpy(wStr, this->text.c_str() + this->wTextDisplayIndex,
					this->wCursorIndex - this->wTextDisplayIndex);
			WCv(wStr[this->wCursorIndex - this->wTextDisplayIndex]) = '\0';
			g_pTheFM->GetTextWidthHeight(this->fontType, wStr, wTextW, wTextH);
			//Move right until the cursor is showing.
			if (wTextW > this->w - (nOffsetX + EDGE_OFFSET * 2))
				++this->wTextDisplayIndex;
			else
				break;
		} while (true);
		delete[] wStr;
	}
}

//*****************************************************************************
bool CTextBoxWidget::CharIsWordbreak(const WCHAR wc) const
//Returns: whether character to right of cursor is whitespace,
//a dash (word break), or NULL
{
	return iswspace(wc) || wc == '-' || wc == 0;
}

//*****************************************************************************
bool CTextBoxWidget::CursorOnWhitespace() const
//Returns: whether character to right of cursor is whitespace,
//a dash (word break), or NULL
{
	return CharIsWordbreak(this->text[this->wCursorIndex]);
}

//*****************************************************************************
bool CTextBoxWidget::MoveCursorLeft()
//Moves cursor left one character.
//Returns: whether operation was successful (possible)
{
	if (this->wCursorIndex == 0)
		return false;

	SetCursorIndex(this->wCursorIndex-1);
	return true;
}

//******************************************************************************
bool CTextBoxWidget::MoveCursorLeftWord()
//Moves cursor left to start of word.
//Returns: whether operation was successful (possible)
{
	if (this->wCursorIndex == 0)
		return false;

	//Go left at least one space.
	SetCursorIndex(this->wCursorIndex-1);

	//Pass whitespace.
	while (this->wCursorIndex > 0 && CursorOnWhitespace())
		SetCursorIndex(this->wCursorIndex-1);
	//Go to beginning of word.
	bool bFoundWord = false;
	while (this->wCursorIndex > 0 && !CursorOnWhitespace())
	{
		SetCursorIndex(this->wCursorIndex-1);
		bFoundWord = true;
	}

	//Passed beginning of word -- go back.
	if (bFoundWord && CursorOnWhitespace())
		SetCursorIndex(this->wCursorIndex+1);

	return true;
}

//******************************************************************************
bool CTextBoxWidget::MoveCursorRight()
//Moves cursor right one character.
//Returns: whether operation was successful (possible)
{
	ASSERT(this->wCursorIndex <= this->text.size());
	if (this->wCursorIndex == this->text.size())
		return false;

	SetCursorIndex(this->wCursorIndex+1);
	return true;
}

//******************************************************************************
bool CTextBoxWidget::MoveCursorRightWord()
//Moves cursor right to start of word.
//Returns: whether operation was successful (possible)
{
	const UINT wLength = this->text.size();
	ASSERT(this->wCursorIndex <= wLength);
	if (this->wCursorIndex == wLength)
		return false;

	//Pass word.
	while (this->wCursorIndex < wLength && !CursorOnWhitespace())
		SetCursorIndex(this->wCursorIndex+1);

	//Pass whitespace.
	while (this->wCursorIndex < wLength && CursorOnWhitespace())
		SetCursorIndex(this->wCursorIndex+1);

	return true;
}

//******************************************************************************
bool CTextBoxWidget::IsEmpty() const
//Returns: whether text box is empty
{
	return (this->bIsPassword ? this->passwordText.length() : this->text.length()) == 0;
}

//******************************************************************************
const WCHAR* CTextBoxWidget::GetText() const
//Returns text
{
	return this->bIsPassword ? this->passwordText.c_str() : this->text.c_str();
}

//******************************************************************************
int CTextBoxWidget::GetNumber() const
//Returns: integer value of text (in an expected integer format)
{
	return _Wtoi(GetText());
}

//******************************************************************************
void CTextBoxWidget::SetText(const WCHAR* newText) {SetText(newText, true);}

//******************************************************************************
void CTextBoxWidget::SetText(const WCHAR* newText, const bool bClearUndos)
//Sets text and puts cursor at end.
{
	const UINT wTextLen = (newText ? WCSlen(newText) : 0);
	if (wTextLen > this->wMaxTextLength)
	{
		//Truncate text to max limit.  (Sub-optimal behavior, but could avoid further bugs.)
		this->text = newText;
		this->text.resize(this->wMaxTextLength);
	}
	else
		this->text = (newText ? newText : wszEmpty);
	ASSERT(this->text.size() <= this->wMaxTextLength);

	if (this->bIsPassword) {
		UINT n, nOrig;
		this->passwordText.resize(wTextLen+1);
		for (n = 0, nOrig = 0; n < wTextLen; ++n)
		{
			//Only alphanumeric chars are accepted.
			if (iswalnum(this->text[n]))
			{
				this->passwordText[nOrig] = this->text[n];
				this->text[nOrig] = W_t('-');
				++nOrig;
			}
		}
		this->text.resize(nOrig);
		this->passwordText.resize(nOrig);
	}

	//Selection is no longer valid.
	ClearSelection();

	this->wTextDisplayIndex = 0;
	CalcAreas();

	//Undo/redo lists are invalidated.  Cursor is placed at end of text.
	if (bClearUndos)
	{
		this->undoList.clear();
		this->redoList.clear();
		SetCursorIndex(0);
		SetCursorIndex(this->text.size());
		CalcAreas();
	}
}

//******************************************************************************
bool CTextBoxWidget::InsertAtCursor(
//Inserts a character at the current cursor position.
//
//Returns: true if character was inserted, else false
//
//Params:
	const WCHAR c, //(in)
	const bool bSetCursorIndex) //[default=true]
{
	//When set, only accept digits.
	if (this->bDigitsOnly)
	{
		if (!iswdigit(c) && c != '-')
			return false;

		//Accept minus sign at beginning of negative number.
		if (c == '-' && (!this->bAllowNegative || this->wCursorIndex > 0))
			return false;
	}

	//When set, only accept filename-safe chars.
	if (this->bFilenameSafe && !charFilenameSafe(c))
		return false;

	//Restrict max string length.
	if (this->text.size() == this->wMaxTextLength)
		return false;

	//Insert char at (just before) cursor position.
	WCHAR addChar[2] = {c,We(0)};
	if (this->bIsPassword)
	{
		//Only alphanumeric chars are accepted.
		if (!iswalnum(c)) return false;

		this->passwordText.insert(this->wCursorIndex, addChar);
		this->text += wszHyphen;
	} else
		this->text.insert(this->wCursorIndex, addChar);

	if (bSetCursorIndex)
	{
		CalcAreas(); //widget's state has changed
		SetCursorIndex(this->wCursorIndex+1);

		//Call OnSelectChange() notifier.
		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
	} else {
		++this->wCursorIndex;
	}

	return true;
}

//******************************************************************************
void CTextBoxWidget::DeleteAtCursor()
//Deletes the character at the current cursor position.
{
	//Remove character at cursor's position.
	this->text.erase(this->wCursorIndex, 1);
	if (this->bIsPassword)
		this->passwordText.erase(this->wCursorIndex, 1);

	//In case graphical cursor position needs updating (due to word wrap, etc.)
	CalcAreas();
	SetCursorIndex(this->wCursorIndex);

	//Call OnSelectChange() notifier.
	CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
	if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
}

//
//Protected methods.
//

//******************************************************************************
void CTextBoxWidget::ClearSelection()
{
	this->wMarkStart = this->wMarkEnd = (UINT)-1;
}

//******************************************************************************
bool CTextBoxWidget::DeleteSelected()
//Deletes the selected characters.
//Returns: true if successful, false otherwise (no selection?)
{
	if (!HasSelection())
		return false;

	UINT wStart, wEnd;
	GetSelection(wStart, wEnd);

	this->text.erase(wStart,wEnd-wStart);
	if (this->bIsPassword)
		this->passwordText.erase(wStart,wEnd-wStart);

	this->wCursorIndex = wStart;
	SetCursorIndex(this->wCursorIndex);

	ClearSelection();

	//Call OnSelectChange() notifier.
	CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
	if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());

	return true;
}

//******************************************************************************
void CTextBoxWidget::DrawCursorNow()
//Set vars so cursor will draw on next call to HandleAnimate.
{
	this->dwLastCursorDraw = 0;
	this->bDrawCursor = false;
}

//******************************************************************************
bool CTextBoxWidget::HasSelection() const
//Determines if text is currently selected.
//Returns: true if there is a selection
{
	return (this->wMarkStart != (UINT)-1 || this->wMarkEnd != (UINT)-1) &&
		this->wMarkStart != this->wMarkEnd;
}

//******************************************************************************
bool CTextBoxWidget::GetSelection(UINT &wStart, UINT &wEnd) const
//Gets the start and end of the current selection.
//Returns: true if successful, false if there is no selection
{
	if (!HasSelection()) return false;
	wStart = (this->wMarkStart == (UINT)-1) ? 0 : this->wMarkStart;
	wEnd = (this->wMarkEnd == (UINT)-1) ? this->text.size() : this->wMarkEnd;
	if (wStart > wEnd)
		std::swap(wStart, wEnd);

	return true;
}

//******************************************************************************
void CTextBoxWidget::SetSelection(const UINT wStart)
{
	ASSERT(wStart <= this->text.size());
	if (wStart == this->wCursorIndex)
		return;   //selection is empty
	if (this->wMarkStart == (UINT)-1)
		this->wMarkStart = wStart;
	this->wMarkEnd = this->wCursorIndex;
}

//******************************************************************************
void CTextBoxWidget::HandleAnimate()
//Draw blinking cursor if widget has focus (once per second).
{
	if (IsSelected())
	{
		const Uint32 dwNow = SDL_GetTicks();
		if (dwNow - this->dwLastCursorDraw > 500)
		{
			this->dwLastCursorDraw = dwNow;
			this->bDrawCursor = !this->bDrawCursor;
			RequestPaint();
		}
	}
}

//
//Protected methods.
//

//******************************************************************************
void CTextBoxWidget::DrawText(
//Draw text, starting from wTextDisplayIndex.
//
//Params:
	const int nOffsetX, const int nOffsetY)   //(in) Drawing offsets.
{
#ifdef RUSSIAN_BUILD
	static const int Y_OFFSET = 1;   //fonts often draw a bit too low
#else
	static const int Y_OFFSET = 4;   //fonts often draw too low
#endif
	const int wX = this->x + nOffsetX + EDGE_OFFSET;
	const int wY = this->y + nOffsetY - Y_OFFSET;
	const UINT wW = this->w - 2*(nOffsetX + EDGE_OFFSET);
	const UINT wH = this->h - nOffsetY + Y_OFFSET;
	g_pTheFM->DrawTextXY(this->fontType, this->text.c_str() + this->wTextDisplayIndex,
			GetDestSurface(), wX, wY, wW, wH);

	if (!HasSelection() || !IsSelected()) //don't show selection when widget doesn't have focus
		return;

	//Highlight selected text.
	//Determine which part of selection is in viewable area.
	UINT wStart, wEnd;
	GetSelection(wStart, wEnd);
	if (wStart < this->wTextDisplayIndex)
		wStart = this->wTextDisplayIndex;
	if (wStart >= wEnd) return;   //nothing in viewable window

	//Calculate where to start highlighting.
	UINT wTextStart, wTextEnd, wIgnored;
	ASSERT(wStart >= this->wTextDisplayIndex);
	WCHAR *wStr = new WCHAR[wEnd+1];  //max size
	WCSncpy(wStr, this->text.c_str() + this->wTextDisplayIndex,
			wStart - this->wTextDisplayIndex);
	WCv(wStr[wStart - this->wTextDisplayIndex]) = '\0';
	g_pTheFM->GetTextWidthHeight(this->fontType, wStr, wTextStart, wIgnored);

	//Calculate where to end highlighting.
	WCSncpy(wStr, this->text.c_str() + this->wTextDisplayIndex,
			wEnd - this->wTextDisplayIndex);
	WCv(wStr[wEnd - this->wTextDisplayIndex]) = '\0';
	g_pTheFM->GetTextWidthHeight(this->fontType, wStr, wTextEnd, wIgnored);
	delete[] wStr;

	//Highlight.
	ASSERT(wTextEnd >= wTextStart);
	if (wTextEnd == wTextStart) return;
	UINT wSelectionWidth = wTextEnd - wTextStart;
	if (wTextStart + wSelectionWidth > wW) //bounds check
		wSelectionWidth = wW - wTextStart;
	g_pTheBM->Invert(GetDestSurface(), wX + wTextStart, this->y + nOffsetY + CY_INSET_BORDER,
			wSelectionWidth, this->h - nOffsetY - 2*CY_INSET_BORDER);
}

//******************************************************************************
void CTextBoxWidget::DrawCursor(
//Draw cursor.
//
//Params:
	const int nOffsetX, const int nOffsetY)   //(in) Drawing offsets.
{
	if (!this->bDrawCursor)
		return;

	//Calculate where to place cursor.
	ASSERT(this->wCursorIndex >= this->wTextDisplayIndex);
	WCHAR *wStr = new WCHAR[this->wCursorIndex+1];  //max size
	WCSncpy(wStr, this->text.c_str() + this->wTextDisplayIndex,
			this->wCursorIndex - this->wTextDisplayIndex);
	WCv(wStr[this->wCursorIndex - this->wTextDisplayIndex]) = '\0';
	UINT wTextW, wTextH;
	g_pTheFM->GetTextWidthHeight(this->fontType, wStr, wTextW, wTextH);
	delete[] wStr;

	//Draw cursor
	static const SURFACECOLOR Black = {0,0,0};
	static const int nBorder = 2; //1 + edge
	DrawCol(this->x + EDGE_OFFSET + wTextW + nOffsetX,
			this->y + nBorder + nOffsetY, this->h - 2 * nBorder,Black);
}

//******************************************************************************
void CTextBoxWidget::TypeCharacters(const WCHAR* wcs, size_t length)
{
	WSTRING inputText(wcs);

	SanitizeText(inputText);
	length = inputText.length();

	WSTRING curText = GetText();
	const UINT wCursorPos = this->wCursorIndex;
	if (HasSelection())
		DeleteSelected(); //replace selection with typed char
	bool ok = false;
	
	for (size_t i = 0; i < length && (ok = InsertAtCursor(inputText[i])); ++i) {}
	if (ok)
	{
		ClearSelection();
		this->undoList.push_back(TextUndoInfo(curText, wCursorPos));
		this->redoList.clear();
	} else {
		//No character was actually added -- undo deletion.
		SetText(curText.c_str(), false);
		SetCursorIndex(wCursorPos);
	}
}

//******************************************************************************
void CTextBoxWidget::redo()
//Redo last undone text edit.
{
	if (this->redoList.empty())
		return;

	const UINT wCursorPos = this->wCursorIndex;
	this->undoList.push_back(TextUndoInfo(GetText(), wCursorPos));
	TextUndoInfo& redoInfo = this->redoList.back();
	SetText(redoInfo.text.c_str(), false);
	SetCursorIndex(redoInfo.cursorIndex);
	this->redoList.pop_back();

	//Call OnSelectChange() notifier.
	CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
	if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
}

//******************************************************************************
void CTextBoxWidget::undo()
//Undo last text edit.
{
	if (this->undoList.empty())
		return;

	const UINT wCursorPos = this->wCursorIndex;
	this->redoList.push_back(TextUndoInfo(GetText(), wCursorPos));
	TextUndoInfo& undoInfo = this->undoList.back();
	SetText(undoInfo.text.c_str(), false);
	SetCursorIndex(undoInfo.cursorIndex);
	this->undoList.pop_back();

	//Call OnSelectChange() notifier.
	CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
	if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
}

//******************************************************************************
void CTextBoxWidget::SanitizeText(WSTRING &text)
{
	SanitizeSingleLineString(text);
}

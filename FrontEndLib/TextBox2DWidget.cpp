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
 * Mike Rimer (mrimer), Jamieson Cobleigh, Matthew Schikore (schik)
 *
 * ***** END LICENSE BLOCK ***** */

#include "TextBox2DWidget.h"
#include "BitmapManager.h"
#include "EventHandlerWidget.h"
#include "FontManager.h"
#include "Inset.h"

#include <BackEndLib/Types.h>
#include <BackEndLib/Assert.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Clipboard.h>

#define ABS(a) ((a) > 0 ? (a) : -(a))
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#define MAX(a,b) (((a) > (b)) ? (a) : (b))

#define EDGE_OFFSET  (CX_INSET_BORDER + 1)

#ifdef RUSSIAN_BUILD
const int Y_KLUDGE = 1;
#else
const int Y_KLUDGE = 2; //font is drawn too high otherwise
#endif

//
//Public methods.
//

//******************************************************************************
CTextBox2DWidget::CTextBox2DWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,             //(in)   Required params for CWidget
	const int nSetX, const int nSetY,   //    constructor.
	const UINT wSetW, const UINT wSetH, //
	const UINT nMaxTextLength,          //(in)
	const UINT enterSendsHotkey)        //(in) whether hitting Enter will return
													//    a hotkey event [default = TAG_OK]
	: CTextBoxWidget(dwSetTagNo, nSetX, nSetY, wSetW, wSetH, nMaxTextLength, enterSendsHotkey)
	, wCursorX(0), wCursorY(0)
	, nSelectStartX(-1), nSelectStartY(-1)
	, nSelectEndX(-1), nSelectEndY(-1)
	, wPosClickTopLine(0)
{
	CalcAreas();
}

//******************************************************************************
UINT CTextBox2DWidget::GetTextLineDisplayWidth(const int nOffsetX) const
{
	UINT wW = this->w - (nOffsetX + (EDGE_OFFSET * 2));
	if (this->UpRect.h != 0) //is a vertical scroll bar being drawn?
		wW -= CX_UP;
	return wW;
}

//******************************************************************************
void CTextBox2DWidget::HandleKeyDown(
//Processes a keyboard event within the scope of the text box.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
{
	const SDL_Keycode key = KeyboardEvent.keysym.sym;
	const bool bShift = (KeyboardEvent.keysym.mod & KMOD_SHIFT) != 0;
	const UINT wOldCursorPos = this->wCursorIndex;
	const UINT wOldCursorX = this->wCursorX;
	const UINT wOldCursorY = this->wCursorY;
	switch (key)
	{
		case SDLK_UP:
		case SDLK_KP_8:
			MoveCursorUp();
			if (bShift) SetSelection(wOldCursorPos, wOldCursorX, wOldCursorY);
			else ClearSelection();
			break;
		case SDLK_DOWN:
		case SDLK_KP_2:
			MoveCursorDown();
			if (bShift) SetSelection(wOldCursorPos, wOldCursorX, wOldCursorY);
			else ClearSelection();
			break;
		case SDLK_HOME:
		case SDLK_KP_7:
			MoveCursorToLineStart();
			if (bShift) SetSelection(wOldCursorPos, wOldCursorX, wOldCursorY);
			else ClearSelection();
			break;
		case SDLK_END:
		case SDLK_KP_1:
			MoveCursorToLineEnd();
			if (bShift) SetSelection(wOldCursorPos, wOldCursorX, wOldCursorY);
			else ClearSelection();
			break;
		case SDLK_PAGEDOWN:
		case SDLK_KP_3:
			MoveCursorDownPage();
			if (bShift) SetSelection(wOldCursorPos, wOldCursorX, wOldCursorY);
			else ClearSelection();
			break;
		case SDLK_PAGEUP:
		case SDLK_KP_9:
			MoveCursorUpPage();
			if (bShift) SetSelection(wOldCursorPos, wOldCursorX, wOldCursorY);
			else ClearSelection();
			break;

		case SDLK_LEFT:
		case SDLK_KP_4:
		case SDLK_RIGHT:
		case SDLK_KP_6:
			CTextBoxWidget::HandleKeyDown(KeyboardEvent);
			if (bShift) SetSelection(wOldCursorPos, wOldCursorX, wOldCursorY);
			else ClearSelection();
			break;

		case SDLK_RETURN:
		case SDLK_KP_ENTER:
		{
			if (!GetHotkeyTag(KeyboardEvent.keysym)) //don't type character if this is a hotkey
			{
				const WCHAR wc = TranslateUnicodeKeysym(KeyboardEvent.keysym);
				TypeCharacter(wc);
			}
			break;
		}
		default:
			CTextBoxWidget::HandleKeyDown(KeyboardEvent);
			return;  //don't Paint
	}

	CalcAreas(); //in case widget state changed
	RequestPaint();
}

//******************************************************************************
void CTextBox2DWidget::HandleMouseDown(
//Processes a mouse event within the scope of the text box.
//
//Params:
	const SDL_MouseButtonEvent &MouseButtonEvent)   //(in) Event to handle.
{
	CScrollableWidget::HandleMouseDown(MouseButtonEvent);

	if (IS_IN_RECT(MouseButtonEvent.x, MouseButtonEvent.y, this->ItemsRect))
	{
		//Copied from CTextBoxWidget::HandleMouseDown.
		SetCursorByPos(MouseButtonEvent.x - this->x, MouseButtonEvent.y - this->y);
		if (MouseButtonEvent.state == SDL_PRESSED)
			this->wMarkStart = this->wMarkEnd = this->wCursorIndex;

		//Retain where cursor is to avoid having to calculate it for selection highlighting.
		this->nSelectStartX = this->nSelectEndX = this->wCursorX;
		this->nSelectStartY = this->nSelectEndY = this->wCursorY;

		RequestPaint();	//call after setting the above vars
	}
}

//******************************************************************************
void CTextBox2DWidget::HandleDrag(
	const SDL_MouseMotionEvent &MouseMotionEvent)   //(in) Event to handle.
{
	CScrollableWidget::HandleDrag(MouseMotionEvent);

	if (this->eLastClickResult == SWCR_Selection)
	{
		CTextBoxWidget::HandleDrag(MouseMotionEvent);
		this->nSelectEndX = this->wCursorX;
		this->nSelectEndY = this->wCursorY;
	}
}

//******************************************************************************
void CTextBox2DWidget::HandleMouseUp(
//Processes a mouse event within the scope of the text box.
//
//Params:
	const SDL_MouseButtonEvent &MouseButtonEvent)   //(in) Event to handle.
{
	CScrollableWidget::HandleMouseUp(MouseButtonEvent);

	//Don't need to call SetCursorByPos here or set these vars.
	//this->nSelectEndX = this->wCursorX;
	//this->nSelectEndY = this->wCursorY;

	if (MouseButtonEvent.state == SDL_RELEASED)
	{
		if (this->eLastClickResult == SWCR_Selection)
		{
			this->wMarkEnd = this->wCursorIndex;
			if (this->wMarkEnd == this->wMarkStart)   //nothing really selected
				ClearSelection();
		}
	}
	RequestPaint();
}

//*****************************************************************************
bool CTextBox2DWidget::MoveCursorUp(UINT wNumLines) //[default=1]
//Moves cursor up one line.
//Returns: whether operation was successful (possible)
{
	for (UINT wLine = 0; wLine < wNumLines; ++wLine)
	{
		if (this->wCursorIndex == 0)
			return false;

		const UINT wLineOfTextH = g_pTheFM->GetFontLineHeight(this->fontType);
		if (wLineOfTextH > this->wCursorY)
		{
			//Already on top view line -- move view up, if possible.
			const UINT wCursorLine = getLineContainingIndex(this->wCursorIndex);
			if (!wCursorLine)
			{
				//Cursor is already on first line of text.  Move cursor to start of text.
				MoveCursorToLineStart();
			} else {
				//Move view up one line.
				const UINT wCursorX = this->wCursorX, wCursorY = this->wCursorY;
				MoveViewUp();
				SetCursorByPos(wCursorX, wCursorY);
			}
		} else
			SetCursorByPos(this->wCursorX, this->wCursorY - wLineOfTextH/2);
	}
	return true;
}

//******************************************************************************
bool CTextBox2DWidget::MoveCursorDown(UINT wNumLines) //[default=1]
//Moves cursor down one line.
//Returns: whether operation was successful (possible)
{
	for (UINT wLine = 0; wLine < wNumLines; ++wLine)
	{
		ASSERT(this->wCursorIndex <= this->text.size());
		if (this->wCursorIndex == this->text.size())
			return false;

		const UINT wLineOfTextH = g_pTheFM->GetFontLineHeight(this->fontType) + 1;
		const UINT wWindowHeight = this->h-CY_INSET_BORDER*2;
		if (this->wCursorY + wLineOfTextH >= wWindowHeight)
		{
			//Already on bottom view line -- move view down, if possible.
			UINT wNextLineStartIndex = this->wCursorIndex;
			if (!getLineStartIndexFollowingIndex(wNextLineStartIndex))
			{
				//Cursor is already on last line.  Move cursor to end of text.
				MoveCursorToLineEnd();
			} else {
				//Move view down one line.
				const UINT wCursorX = this->wCursorX, wCursorY = this->wCursorY;
				MoveViewDown();
				SetCursorByPos(wCursorX, wCursorY);
			}
		} else {
			SetCursorByPos(this->wCursorX, this->wCursorY + wLineOfTextH);
		}
	}
	return true;
}

//******************************************************************************
bool CTextBox2DWidget::MoveCursorToLineStart()
//Moves cursor to beginning of line.
//Returns: true
{
	SetCursorByPos(0, this->wCursorY);
	return true;
}

//******************************************************************************
bool CTextBox2DWidget::MoveCursorToLineEnd()
//Moves cursor to end of line.
//Returns: true
{
	SetCursorByPos(this->w-1, this->wCursorY);
	return true;
}

//******************************************************************************
bool CTextBox2DWidget::MoveCursorUpPage()
//Moves cursor down one page (widget height) and moves view with it.
//Returns: Whether or not the operation was successful (possible)
{
	ASSERT(this->wCursorIndex <= this->text.size());
	if (this->wCursorIndex == 0)
		return false;

	const UINT wPageLines = GetViewableTextLinesInWidget();
	const UINT topLine = getLineContainingIndex(this->wTextDisplayIndex);
	MoveViewUp(wPageLines);
	const UINT linesViewMoved = topLine - getLineContainingIndex(this->wTextDisplayIndex);
	MoveCursorUp(wPageLines - linesViewMoved);

	SetCursorByPos(this->wCursorX, this->wCursorY);
	return true;
}

//******************************************************************************
bool CTextBox2DWidget::MoveCursorDownPage()
//Moves cursor up one page (widget height) and moves view with it.
//Returns: Whether or not the operation was successful (possible)
{
	ASSERT(this->wCursorIndex <= this->text.size());
	if (this->wCursorIndex == this->text.size())
		return false;

	const UINT wPageLines = GetViewableTextLinesInWidget();
	const UINT topLine = getLineContainingIndex(this->wTextDisplayIndex);
	MoveViewDown(wPageLines);
	const UINT linesViewMoved = getLineContainingIndex(this->wTextDisplayIndex) - topLine;
	MoveCursorDown(wPageLines - linesViewMoved);

	SetCursorByPos(this->wCursorX, this->wCursorY);
	return true;
}

//******************************************************************************
void CTextBox2DWidget::SelectAllText()
{
	CTextBoxWidget::SelectAllText();
	CalculateHighlightedRegion();
}

void CTextBox2DWidget::CalculateHighlightedRegion()
{
	if (!HasSelection())
		return;

	const int nLineHeightPixels = g_pTheFM->GetFontLineHeight(this->fontType);

	//Calculate where to start highlighting.
	const UINT wTopViewLine = getLineContainingIndex(this->wTextDisplayIndex);
	if (this->wMarkStart <= this->wTextDisplayIndex) {
		this->nSelectStartX = this->nSelectStartY = 0;
	} else {
		const UINT wStartLineIndex = getLineContainingIndex(this->wMarkStart);
		ASSERT(wStartLineIndex >= wTopViewLine);
		this->nSelectStartY = (wStartLineIndex - wTopViewLine) * nLineHeightPixels;

		const UINT wIndexAtStartOfHighlightStartLine = getIndexAtStartOfLine(wStartLineIndex);
		UINT wW, wHIgnored;
		const WSTRING wStr = this->text.substr(wIndexAtStartOfHighlightStartLine, this->wMarkStart);
		g_pTheFM->GetTextWidthHeight(this->fontType, wStr.c_str(), wW, wHIgnored);
		this->nSelectStartX = int(wW);
	}

	//Calculate where to end highlighting.
	const UINT wEndLineIndex = getLineContainingIndex(this->wMarkEnd);
	const UINT wViewableTextLinesInWidget = GetViewableTextLinesInWidget();
	const UINT wHighlightedLines = wEndLineIndex - wTopViewLine;

	if (wHighlightedLines > wViewableTextLinesInWidget) {
		const int nOffsetX = 0; //!!TODO needed?
		this->nSelectEndX = GetTextLineDisplayWidth(nOffsetX);
		this->nSelectEndY = wViewableTextLinesInWidget * nLineHeightPixels - EDGE_OFFSET;
	} else {
		const UINT wIndexAtStartOfHighlightEndLine = getIndexAtStartOfLine(wEndLineIndex);
		UINT wW, wHIgnored;
		const WSTRING wStr = this->text.substr(wIndexAtStartOfHighlightEndLine, this->wMarkEnd);
		g_pTheFM->GetTextWidthHeight(this->fontType, wStr.c_str(), wW, wHIgnored);
		this->nSelectEndX = int(wW);

		this->nSelectEndY = wHighlightedLines * nLineHeightPixels;
	}
}

//******************************************************************************
void CTextBox2DWidget::SetCursorByPos(
//Sets cursor to index of character rendered at (xPos,yPos)
//
//Params:
	int xPos, int yPos)  //(in) cursor's new position (in pixels)
{
	if (yPos >= static_cast<int>(this->h) ||
			xPos >= static_cast<int>(this->w)) return;   //bounds checking

 	const UINT wLength = this->text.size();
	if ((xPos <= 0 && yPos <= 0) || wLength == 0)
	{
		//Place cursor at beginning of display.
		SetCursorIndex(this->wTextDisplayIndex);
	} else {
		//Calculate where to place cursor.
		if (xPos < 0) xPos = 0;
		if (yPos < 0) yPos = 0;
		const UINT wLineOfTextH = g_pTheFM->GetFontLineHeight(this->fontType);
		UINT wIndex = this->wTextDisplayIndex, wWidth = 0, wHeight = 0;
		//Move down to correct line.
		while (static_cast<int>(wHeight + wLineOfTextH) <= yPos && wIndex < wLength)
		{
			if (!getLineStartIndexFollowingIndex(wIndex))
			{
				//Clicked position is below last line of text.
				wIndex = wLength;
				break;
			}
			wHeight += wLineOfTextH;
		}

		//Move across line to correct character -- binary search to speed these expensive calls.
		UINT wFirstPos = wIndex, wLastPos = wIndex;
		if (!getLineStartIndexFollowingIndex(wLastPos))
			wLastPos = wLength;
		UINT originalLastPos = wLastPos;
		while (wFirstPos < wLastPos)
		{
			wIndex = wFirstPos + (wLastPos - wFirstPos + 1) / 2;
			GetPixelLocationAt(this->wTextDisplayIndex, wIndex, wWidth, wHeight);

			if (static_cast<int>(wWidth) > xPos || static_cast<int>(wHeight) > yPos)
			{
				//We are past the correct character position or line of text.
				wLastPos = wIndex - 1;
			} else {
				if (wFirstPos == wIndex)
					wLastPos = wIndex; //this is the correct index -- stop here
				else
					wFirstPos = wIndex;
			}
		}
		//Get final position.
		wIndex = wFirstPos;
		//LastPos is actually the character after this line, unless it's the actual last character.
		if (wIndex == originalLastPos && wIndex != this->text.size())
			wIndex -= 1;
		if (wIndex > wLength)
			SetCursorIndex(wLength);
		else
			SetCursorIndex(wIndex);

		GetPixelLocationAt(this->wTextDisplayIndex, this->wCursorIndex, wWidth, wHeight);
	}

	DrawCursorNow(); //quick feedback

	CalcAreas(); //in case widget state has changed
}

//******************************************************************************
void CTextBox2DWidget::SetCursorIndex(
//Sets cursor to index.  Updates displayed text region.
//
//Params:
	const UINT wIndex)   //(in) cursor's new position
{
	UINT wNewIndex = wIndex;
	if (wNewIndex > this->text.size())
		wNewIndex = this->text.size();
	const UINT line=getLineContainingIndex(wNewIndex);
	const UINT startIndex = getIndexAtStartOfLine(line);
	const UINT wLineOfTextH = g_pTheFM->GetFontLineHeight(this->fontType);
	UINT lineY = (line - getLineContainingIndex(this->wTextDisplayIndex)) * wLineOfTextH;

	//If cursor has moved outside viewing area, update it.
	if (wNewIndex == 0)
		this->wTextDisplayIndex = 0;  //view from beginning
	else if (wNewIndex < this->wTextDisplayIndex)
	{
		if (this->wTextDisplayIndex > this->text.size())
			this->wTextDisplayIndex = this->text.size();
		//Move view up until cursor comes into view.
		do {
			if (!MoveViewUp())
				break; //at top of view
		} while (wNewIndex < this->wTextDisplayIndex);
	} else {
/*
		int nOffsetX, nOffsetY;
		GetScrollOffset(nOffsetX, nOffsetY);
*/
		UINT wCursorX, wCursorY;
/*
		GetPixelLocationAt(startIndex, wNewIndex, wCursorX, wCursorY);
		ASSERT(!wCursorY || (wCursorY == wLineOfTextH)); //index should be on the first line following the index we provide
*/

		//How far below top of view window this line is.
		wCursorY = lineY;

		//Move viewing index down line by line until cursor is in viewing area.
		while (wCursorY + wLineOfTextH > this->h)
		{
			if (!MoveViewDown())
				break; //at bottom of view
			GetPixelLocationAt(startIndex, wNewIndex, wCursorX, wCursorY);
			wCursorY = lineY = (line - getLineContainingIndex(this->wTextDisplayIndex)) * wLineOfTextH;
		}
	}

	//Calculate where to place cursor.
	//This is processor intensive, so we'll store the value.
	GetPixelLocationAt(startIndex, wNewIndex, this->wCursorX, this->wCursorY);
	this->wCursorY = lineY;
	this->wCursorIndex = wNewIndex;
}

//******************************************************************************
void CTextBox2DWidget::ScrollDownOneLine(const UINT wLines)   //[default=1]
//Scroll view down the specified number of lines of text.
{
	const int linesAboveEnd = this->lineIndices.size()+1 -
			(getLineContainingIndex(this->wTextDisplayIndex) + GetViewableTextLinesInWidget());
	const UINT lineDelta = linesAboveEnd < int(wLines) ? linesAboveEnd : wLines;
	if (lineDelta)
	{
		MoveViewDown(lineDelta);
		CalcAreas();
		GetPixelLocationAt(this->wTextDisplayIndex, this->wCursorIndex, this->wCursorX, this->wCursorY);
		CalculateHighlightedRegion();
	}
}

//******************************************************************************
UINT CTextBox2DWidget::GetViewableTextLinesInWidget() const
{
	const UINT wDisplayH = this->h - CY_INSET_BORDER*2;
	const UINT lineH = g_pTheFM->GetFontLineHeight(this->fontType);
	return wDisplayH/lineH;
}

//******************************************************************************
void CTextBox2DWidget::ScrollDownOnePage()
//Scroll view down one page of text, if possible.
{
	ScrollDownOneLine(GetViewableTextLinesInWidget());
}

//******************************************************************************
void CTextBox2DWidget::ScrollUpOneLine(const UINT wLines)   //[default=1]
//Scroll view up the specified number of lines of text.
{
	const UINT linesBelowTop = getLineContainingIndex(this->wTextDisplayIndex);
	const UINT lineDelta = linesBelowTop < wLines ? linesBelowTop : wLines;
	if (lineDelta)
	{
		MoveViewUp(wLines);
		CalcAreas();
		GetPixelLocationAt(this->wTextDisplayIndex, this->wCursorIndex, this->wCursorX, this->wCursorY);
		CalculateHighlightedRegion();
	}
}

//******************************************************************************
void CTextBox2DWidget::ScrollUpOnePage()
//Scroll view up one page of text, if possible.
{
	ScrollUpOneLine(GetViewableTextLinesInWidget());
}

//
//Protected methods
//

//*****************************************************************************
void CTextBox2DWidget::CalcAreas()
//Calculate coords and dimensions of areas within the widget.
{
	UpdateWidgetStats();

	const UINT wDisplayH = this->h;

	//Total height of text content.
	const UINT lineH = g_pTheFM->GetFontLineHeight(this->fontType);
	const UINT wContentH = (this->lineIndices.size()+1) * lineH;

	//Calculate height of unseen text above current view position.
	ASSERT(this->wTextDisplayIndex ==
			getIndexAtStartOfLine(getLineContainingIndex(this->wTextDisplayIndex)));
	UINT wUnseenUpperContentH = getLineContainingIndex(this->wTextDisplayIndex) * lineH;
	CalcAreas_VerticalOnly(wContentH, wDisplayH, wUnseenUpperContentH);

	//If entire content fits in the view, then all text may be displayed.
	if (!wUnseenUpperContentH)
		this->wTextDisplayIndex = 0;
}

//******************************************************************************
CTextBox2DWidget::SW_CLICK_RESULT CTextBox2DWidget::ClickAtCoords(
//Updates widget in response to a mouse click at specified coords.
//
//Params:
	const int nX, const int nY)   //(in) Click coords.
//
//Returns:
//An SWCR_* or LBCR_* constant indicating what happened.
{
	//Check for click on scroll thumb button.
	if (IS_IN_RECT(nX, nY, this->VPosRect))
	{
		//retain position of text view at this thumb location
		this->wPosClickTopLine = getLineContainingIndex(this->wTextDisplayIndex);
		this->nPosClickY = nY;
		return SWCR_VPosButton;
	}

	return CScrollableWidget::ClickAtCoords(nX, nY);
}

//******************************************************************************
void CTextBox2DWidget::DragVertPosButton(
//Updates position button and list box position based on Y coordinate from mouse
//dragging after a click on the position button.
//
//Param:
	const int nY)  //(in)   Vertical mouse coord.
{
	const UINT wDisplayH = this->h;
	const UINT wLineOfTextH = g_pTheFM->GetFontLineHeight(this->fontType);
	const UINT numTextLines = this->lineIndices.size()+1;
	const UINT wContentH = numTextLines * wLineOfTextH;

	//Shouldn't be showing the vertical scroll bar if everything can fit in the view.
	ASSERT(wContentH > wDisplayH);

	//Figure the difference in lines of text from click coord to current coord.
	const int nMoveY = nY - this->nPosClickY;
	const double dblTextLinesToPixel = double(numTextLines) / double(this->VScrollRect.h);
	const int nMoveLines = int(double(nMoveY) * dblTextLinesToPixel);
	if (nMoveLines == 0)
		return;

	//Move the view's top to a new text line.
	UINT wOldTopLine = getLineContainingIndex(this->wTextDisplayIndex), wNewTopLine;
	const UINT wDisplayLines = wDisplayH / wLineOfTextH;
	if (int(this->wPosClickTopLine + nMoveLines + wDisplayLines) > int(numTextLines))
		wNewTopLine = numTextLines - wDisplayLines;
	else if (int(this->wPosClickTopLine) + nMoveLines < 0)
		wNewTopLine = 0;
	else
		wNewTopLine = this->wPosClickTopLine + nMoveLines;
	this->wTextDisplayIndex = getIndexAtStartOfLine(wNewTopLine);

	//Update where selection highlighting occurs.
	const int pixelsMoved = int(wNewTopLine - wOldTopLine) * wLineOfTextH;
	this->nSelectStartY -= pixelsMoved;
	this->nSelectEndY -= pixelsMoved;
	RequestPaint();

	//Recalc areas of widget since they may have changed.
	CalcAreas();
	GetPixelLocationAt(this->wTextDisplayIndex, this->wCursorIndex, this->wCursorX, this->wCursorY);
}

//******************************************************************************
void CTextBox2DWidget::ClearSelection()
{
	CTextBoxWidget::ClearSelection();
	this->nSelectStartX = this->nSelectStartY = -1;
	this->nSelectEndX = this->nSelectEndY = -1;
}

//******************************************************************************
void CTextBox2DWidget::SetSelection(const UINT wStart,
	const UINT wCursorStartX, const UINT wCursorStartY)
{
	CTextBoxWidget::SetSelection(wStart);
	if (this->nSelectStartX == -1)
	{
		this->nSelectStartX = wCursorStartX;
		this->nSelectStartY = wCursorStartY;
	}
	this->nSelectEndX = this->wCursorX;
	this->nSelectEndY = this->wCursorY;
}

//******************************************************************************
void CTextBox2DWidget::UpdateWidgetStats()
//Maintains bookkeeping vars to optimize internal handling of the widget.
{
	//Determine position where text was changed.
	UINT index = 0;
	const UINT textSize = this->text.size(), prevTextSize = this->prevText.size();
	while (index < textSize && index < prevTextSize)
	{
		if (this->text[index] != this->prevText[index])
			break;
		++index;
	}
	if (index == textSize && textSize == prevTextSize)
		return; //text hasn't changed -- no updating required

	//Recalculate what character index each line starts on,
	//starting from the beginning of the line before the one containing 'index'.
	UINT lineNo = getLineContainingIndex(index);
	if (lineNo > 0)
		--lineNo;
	this->lineIndices.resize(lineNo);
	index = lineNo ? this->lineIndices[lineNo-1] : 0; //start from previous line

	//Starting from 'index' on the current line,
	//find the index of the first character on the next line.
	const UINT lineH = g_pTheFM->GetFontLineHeight(this->fontType);
	UINT indexOfFirstCharOnLine = lineNo ? this->lineIndices[lineNo-1] : 0;
	while (index < textSize)
	{
		UINT wCursorX, wCursorY;

		//Binary search to speed these expensive calls.
		UINT wFirstPos = index, wLastPos = textSize;
		while (wFirstPos < wLastPos)
		{
			index = wFirstPos + (wLastPos - wFirstPos + 1) / 2;
			CalcCursorPosition(indexOfFirstCharOnLine, index, wCursorX, wCursorY);

			if ((wCursorY > 0 && wCursorX > 0) || wCursorY > lineH)
			{
				//We are past the first character on the next line.
				wLastPos = index - 1;
			} else {
				if (wFirstPos == index)
					wLastPos = index; //this is the correct index -- stop here
				else
					wFirstPos = index;
			}
		}
		ASSERT(wFirstPos == wLastPos);
		index = wFirstPos;

		//If a new line has been reached, then record what character it starts on.
		if (wCursorY)
		{
			//Don't break up words if they will fit on the next line.
			const UINT newLineIndex = index;
			while (index > indexOfFirstCharOnLine && !CharIsWordbreak(this->text[index]))
				--index;
			if (CharIsWordbreak(this->text[index]) && index < newLineIndex) //reached a break...
				++index;   //...go back forward to the character following it
			if (index == indexOfFirstCharOnLine) //couldn't find a word break across entire line...
				index = newLineIndex;             //...just break at the end of display area

			indexOfFirstCharOnLine = index;
			this->lineIndices.push_back(index);
		}
	}

	this->prevText = this->text;
}

//
//Private methods.
//

//******************************************************************************
void CTextBox2DWidget::DrawText(
//Draw text, starting from wTextDisplayIndex.
//
//Params:
	const int nOffsetX, const int nOffsetY)   //(in) Drawing offsets.
{
	const int wX = this->x + nOffsetX + EDGE_OFFSET;
	const int wY = this->y + nOffsetY - Y_KLUDGE;
	UINT wH = this->h - nOffsetY;

	//Render one line of text at a time.
	//This ensures it matches the line indexing calculated previously.
	const UINT lineH = g_pTheFM->GetFontLineHeight(this->fontType);
	UINT index = this->wTextDisplayIndex;
	SDL_Surface *pDestSurface = GetDestSurface();
	int lineNo = getLineContainingIndex(index);
	for (int line=0; (lineH*(line+1) <= wH) && (index < this->text.size()); ++line, ++lineNo)
	{
		ASSERT(getIndexAtStartOfLine(lineNo) == index);
		WSTRING lineText = this->text.c_str() + index;
		UINT nextIndex=index;
		if (!getLineStartIndexFollowingIndex(nextIndex))
			nextIndex = this->text.size();
		lineText.resize(nextIndex - index);
		if (lineText[lineText.size() - 1] == '\r') //strip CRs
			lineText.resize(lineText.size() - 1);
		g_pTheFM->DrawTextXY(this->fontType, lineText.c_str(), pDestSurface,
				wX, wY + lineH*line);
		index = nextIndex;
	}

	if (!HasSelection() || !IsSelected()) //don't show selection when widget doesn't have focus
		return;

	//Highlight selected text.
	const UINT wW = GetTextLineDisplayWidth(nOffsetX);

	//Determine where to start and end highlighting.
	int nStartX, nStartY, nEndX, nEndY;
	if (this->nSelectStartY <= this->nSelectEndY)
	{
		nStartX = this->nSelectStartX;  nStartY = this->nSelectStartY;
		nEndX = this->nSelectEndX;  nEndY = this->nSelectEndY;
	} else {
		nStartX = this->nSelectEndX;  nStartY = this->nSelectEndY;
		nEndX = this->nSelectStartX;  nEndY = this->nSelectStartY;
	}
	nEndY += Y_KLUDGE + EDGE_OFFSET;
	if (nEndY < 0) return;  //selected area out of view
	if (nStartY < 0)
	{	//Highlight begins above viewed area.  Start highlighting from beginning.
		nStartX = nStartY = 0;
	}
	nStartY += Y_KLUDGE + EDGE_OFFSET;

	//Highlight.
	const int nLineHeight = g_pTheFM->GetFontLineHeight(this->fontType);
	if (nStartY == nEndY)
	{
		if (nStartX == nEndX) return;
		if (nStartX > nEndX) std::swap(nStartX, nEndX);
		g_pTheBM->Invert(GetDestSurface(), wX + nStartX, wY + nStartY,
				nEndX - nStartX, nLineHeight-1);
	} else {
		//Highlight multi-line region in three strips.
		const int nMidRegionY = nEndY - (nStartY + nLineHeight);
		g_pTheBM->Invert(GetDestSurface(), wX + nStartX, wY + nStartY,
				wW - nStartX, nLineHeight);
		if (nMidRegionY >= nLineHeight) {
			g_pTheBM->Invert(GetDestSurface(), wX, wY + nStartY + nLineHeight,
					wW, nMidRegionY);
		} else if (nEndX>nStartX && nMidRegionY > 0) {
			g_pTheBM->Invert(GetDestSurface(), wX+nStartX, wY + nStartY + nLineHeight,
					nEndX-nStartX, nMidRegionY);
		}
		g_pTheBM->Invert(GetDestSurface(), wX, wY + nEndY, nEndX, nLineHeight-1);
	}
}

//******************************************************************************
void CTextBox2DWidget::DrawCursor(
//Draw cursor.
//
//Params:
	const int nOffsetX, const int nOffsetY)   //(in) Drawing offsets.
{
	if (!this->bDrawCursor)
		return;

	//Draw cursor.
	if (this->wCursorY != UINT(-1))
	{
		const int yPos = this->wCursorY + nOffsetY + Y_KLUDGE*2;
		const UINT cursorH = g_pTheFM->GetFontLineHeight(this->fontType) - 2*EDGE_OFFSET;
		if (yPos+cursorH < this->h)
		{
			static const SURFACECOLOR Black = {0,0,0};
			DrawCol(this->x + EDGE_OFFSET + this->wCursorX + nOffsetX, this->y + yPos,
					cursorH, Black);
		}
	}
}

//******************************************************************************
void CTextBox2DWidget::CalcCursorPosition(
//Get top of cursor's location (x,y).
//
//Params:
	const UINT viewIndex, const UINT wCursorIndex, //index of character at top of view; index of cursor
	UINT &wCursorX, UINT &wCursorY) //(out) Cursor location (in pixels).
const
{
	if (wCursorIndex < viewIndex)
	{
		wCursorX = wCursorY = UINT(-1); //cursor not visible
		return;
	}

	const UINT wLength = this->text.size() + 1;
	WCHAR *wStr = new WCHAR[wLength+1];
	UINT wTextW, wTextH;
	UINT wCursorI = wCursorIndex;
	bool bCursorOnWord = false;
	if (!CharIsWordbreak(this->text[wCursorIndex]))
	{
		//When cursor is on a word, we need to find out whether it will be pushed
		//to the next line:
		//1. Find beginning of word cursor is on.
		while (wCursorI > 0 && !CharIsWordbreak(this->text[wCursorI-1]))
			--wCursorI;
		bCursorOnWord = true;
	}
	if (wCursorI < viewIndex)  //stay within displayed text
		wCursorI = viewIndex;

	//Get dimensions of text up to this spot.
	ASSERT(viewIndex <= wCursorI);
	ASSERT(wCursorI - viewIndex <= wLength);
	WCSncpy(wStr, this->text.c_str() + viewIndex, wCursorI - viewIndex);
	WCv(wStr[wCursorI - viewIndex]) = '\0';
	const UINT wMaxWidth = this->w - (CX_INSET_BORDER * 2) - CX_UP;
	wCursorX = g_pTheFM->GetTextRectHeight(this->fontType, wStr,
			wMaxWidth, wTextW, wTextH);
	const UINT wLineOfTextH = g_pTheFM->GetFontLineHeight(this->fontType);
	wCursorY = wTextH - wLineOfTextH;

	if (bCursorOnWord)
	{
		//2. Get dimensions of next word.
		UINT wWordStartIndex = wCursorI;
		++wCursorI;
		while (wCursorI < wLength && !CharIsWordbreak(this->text[wCursorI]))
			++wCursorI;
		UINT wCharsNotDrawn;
		do {
			const UINT wWordLength = wCursorI - wWordStartIndex;
			WCSncpy(wStr, this->text.c_str() + wWordStartIndex, wWordLength);
			WCv(wStr[wWordLength]) = '\0';
			g_pTheFM->CalcPartialWord(this->fontType, wStr,
					wWordLength, wMaxWidth-wCursorX, wCharsNotDrawn);
			if (wCharsNotDrawn)
			{
				//The word can't fit on this line...
				UINT wCharsRendered = wWordLength - wCharsNotDrawn;
				g_pTheFM->CalcPartialWord(this->fontType, wStr,
						wWordLength, wMaxWidth, wCharsNotDrawn);
				if (wCharsNotDrawn)
				{
					if (wWordStartIndex + wCharsRendered > wCursorIndex)
					{
						//...but part of it is rendered on this line, and the cursor is
						//within this part.  Find out where in Step 3.
						break;
					}
				} else {
					//...but it can fit on the next line, so it will all be
					//rendered there.  Find out how far cursor is over in Step 3.
					wCharsRendered = 0;
				}
				//...else the cursor won't be rendered on this line.
				wCursorX = 0;
				wCursorY += g_pTheFM->GetFontLineHeight(this->fontType);
				wWordStartIndex += wCharsRendered;
			}
		} while (wCharsNotDrawn);
		//3. Find cursor's position relative to the start of the word it's on
		//(or the piece of the segmented word on a lower line).
		WCSncpy(wStr, this->text.c_str() + wWordStartIndex, wCursorIndex - wWordStartIndex);
		WCv(wStr[wCursorIndex - wWordStartIndex]) = '\0';
		UINT wWordW, wWordH;
		g_pTheFM->GetTextRectHeight(this->fontType, wStr, wMaxWidth, wWordW, wWordH);
		wCursorX += wWordW;
	}
	delete[] wStr;
}

//******************************************************************************
void CTextBox2DWidget::GetPixelLocationAt(
//Get top of cursor location (x,y) at the indicated text character position and view index.
//
//Pre-condition: lineIndices is populated and current
//
//Params:
	const UINT viewIndex, const UINT wIndex, //index of character at top of view; text index to report on
	UINT &wPixelX, UINT &wPixelY) //(out) location of position (in relative pixels)
const
{
	if (wIndex < viewIndex)
	{
		wPixelX = wPixelY = UINT(-1); //position not visible in view
		return;
	}

	const UINT wLineNo = getLineContainingIndex(wIndex);
	const UINT wThisLineStartIndex = getIndexAtStartOfLine(wLineNo);

	const UINT wLineOfTextH = g_pTheFM->GetFontLineHeight(this->fontType);
	wPixelY = wLineNo * wLineOfTextH;

	UINT wNextLineStartIndex = wIndex;
	if (!getLineStartIndexFollowingIndex(wNextLineStartIndex))
		wNextLineStartIndex = this->text.size();
	ASSERT(wNextLineStartIndex >= wThisLineStartIndex);
	ASSERT(wIndex >= wThisLineStartIndex);
	ASSERT(wIndex <= wNextLineStartIndex);

	//Get width of text up to this position in the line.
	WSTRING wStrToIndex = this->text.c_str() + wThisLineStartIndex;
	wStrToIndex.resize(wIndex - wThisLineStartIndex);
	g_pTheFM->GetTextWidth(this->fontType, wStrToIndex.c_str(), wPixelX);
}

//******************************************************************************
UINT CTextBox2DWidget::getIndexAtStartOfLine(const UINT lineNo) const
//Returns: index of the first character on specified line number,
// or of last line number if line number is out of range
{
	if (!lineNo)
		return 0; //first line starts at character 0
	if (lineNo > this->lineIndices.size())
		return this->lineIndices.back();
	return this->lineIndices[lineNo-1];
}

//******************************************************************************
UINT CTextBox2DWidget::getLineContainingIndex(const UINT index) const
//Returns: the line of text containing the 'index'th character in the text
{
	UINT lineNo = 0;
	const UINT numLines = this->lineIndices.size();
	while (lineNo < numLines)
	{
		if (this->lineIndices[lineNo] > index)
			break; //found the line containing 'index'
		++lineNo;
	}
	return lineNo;
}

//******************************************************************************
bool CTextBox2DWidget::getLineStartIndexFollowingIndex(
//Returns: whether there is a line of text after the one containing the 'index'th character
	UINT& index) //(in/out) a character index in the text --> index of the beginning of the following line of text
const
{
	UINT lineNo = getLineContainingIndex(index);
	if (lineNo < this->lineIndices.size())
	{
		index = this->lineIndices[lineNo];
		return true;
	}

	return false; //index is already on the last line
}

//******************************************************************************
UINT CTextBox2DWidget::MoveViewDown(const UINT wNumLines)   //[default=1]
//Move viewing area down a line.
//Returns: Number of lines actually moved down
{
	UINT numMoved = 0;
	UINT wIndex = this->wTextDisplayIndex;
	const UINT wLength = this->text.size();
	for (UINT line = 0; line < wNumLines; ++line)
	{
		if (wIndex >= wLength) break;
		if (!getLineStartIndexFollowingIndex(wIndex))
			break; //already at bottom line
		++numMoved;
	}
	this->wTextDisplayIndex = wIndex;
	const int pixelsMoved = numMoved * g_pTheFM->GetFontLineHeight(this->fontType);
	this->nSelectStartY -= pixelsMoved;
	this->nSelectEndY -= pixelsMoved;
	return numMoved;
}

//******************************************************************************
UINT CTextBox2DWidget::MoveViewUp(const UINT wNumLines)   //[default=1]
//Move viewing area up a line.
//Returns: Number of lines actually moved up
{
	const UINT wLineOfTextH = g_pTheFM->GetFontLineHeight(this->fontType);
	UINT wNumMoved = 0;
	UINT wIndex = this->wTextDisplayIndex;
	for (UINT wLine = 0; wLine < wNumLines; ++wLine)
	{
		if (wIndex == 0) break;
		const UINT topViewLine = getLineContainingIndex(wIndex);
		if (!topViewLine)
			break; //already at top line
		wIndex = getIndexAtStartOfLine(topViewLine - 1);
		++wNumMoved;
	}
	this->wTextDisplayIndex = wIndex;
	this->nSelectStartY += wNumMoved*wLineOfTextH;
	this->nSelectEndY += wNumMoved*wLineOfTextH;
	return wNumMoved;
}

//******************************************************************************
void CTextBox2DWidget::SanitizeText(WSTRING &text)
{
	SanitizeMultiLineString(text);
}
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

#ifndef TEXTBOX2DWIDGET_H
#define TEXTBOX2DWIDGET_H

#include "TextBoxWidget.h"

//Height of standard multi-line text box.
static const UINT CY_STANDARD_TBOX2D = 341; //31 * 11 lines

//******************************************************************************
class CTextBox2DWidget : public CTextBoxWidget
{
public:
	CTextBox2DWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
			const UINT wSetW, const UINT wSetH, const UINT nMaxTextLength=500,
			const UINT enterSendsHotkey=0);
	virtual ~CTextBox2DWidget() {}

	virtual void   HandleDrag(const SDL_MouseMotionEvent &Motion);
	virtual void   HandleKeyDown(const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   HandleMouseDown(const SDL_MouseButtonEvent &MouseButtonEvent);
	virtual void   HandleMouseUp(const SDL_MouseButtonEvent &MouseButtonEvent);
	virtual void   SanitizeText(WSTRING &text);
	virtual void   SetCursorByPos(int xPos, int yPos);
	virtual void   SetCursorIndex(const UINT wIndex);

	virtual void   ScrollDownOneLine(const UINT wLines=1);
	virtual void   ScrollDownOnePage();
	virtual void   ScrollUpOneLine(const UINT wLines=1);
	virtual void   ScrollUpOnePage();

protected:
	virtual void   CalcAreas();
	virtual SW_CLICK_RESULT ClickAtCoords(const int nX, const int nY);
	virtual void   DragVertPosButton(const int nY);

	virtual void   ClearSelection();

	virtual void   DrawText(const int nOffsetX, const int nOffsetY);
	virtual void   DrawCursor(const int nOffsetX, const int nOffsetY);

	bool           MoveCursorUp(UINT wNumLines = 1);
	bool           MoveCursorDown(UINT wNumLines = 1);
	bool           MoveCursorToLineStart();
	bool           MoveCursorToLineEnd();
	bool           MoveCursorUpPage();
	bool           MoveCursorDownPage();

	virtual void   SelectAllText();
	void           SetSelection(const UINT wStart,
			const UINT wCursorStartX, const UINT wCursorStartY);

	virtual void   UpdateWidgetStats();

private:
	void           CalcCursorPosition(const UINT viewIndex, const UINT wCursorIndex,
			UINT &wCursorX, UINT &wCursorY) const;
	void           CalculateHighlightedRegion();
	void           GetPixelLocationAt(const UINT viewIndex, const UINT wIndex,
			UINT &wPixelX, UINT &wPixelY) const;

	UINT           GetTextLineDisplayWidth(const int nOffsetX) const;
	UINT           GetViewableTextLinesInWidget() const;

	UINT           getIndexAtStartOfLine(const UINT lineNo) const;
	UINT           getLineContainingIndex(const UINT index) const;
	bool           getLineStartIndexFollowingIndex(UINT& index) const;

	UINT           MoveViewDown(const UINT wNumLines = 1);
	UINT           MoveViewUp(const UINT wNumLines = 1);

	UINT           wCursorX, wCursorY;   //cursor position
	int            nSelectStartX, nSelectStartY; //highlighted region marker
	int            nSelectEndX, nSelectEndY;

	vector<UINT>   lineIndices; //text index located at the start of each text display line (after the first)
	WSTRING        prevText;    //text used last time widget stats were calculated
	UINT           wPosClickTopLine; //retain original location when dragging vert scroll bar
};

#endif //#ifndef TEXTBOX2DWIDGET_H

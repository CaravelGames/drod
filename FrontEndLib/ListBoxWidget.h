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

#ifndef LISTBOXWIDGET_H
#define LISTBOXWIDGET_H

#include "ScrollableWidget.h"

#include <BackEndLib/IDSet.h>
#include <BackEndLib/Wchar.h>

//Height of one list box item.
static const UINT CY_LBOX_ITEM = 22;

struct LBOX_ITEM
{
	LBOX_ITEM()
		: bGrayed(false)
		, rearrangeable(true)
		, color({0, 0, 0, 0})
    {
    }

	union {
		UINT dwKey;
		void* pKey;
	};
	WSTRING text;
	bool bGrayed;  //whether this option is grayed out
	SDL_Color color; //color of text item if not grayed out
	bool rearrangeable;
};

//******************************************************************************
class CListBoxWidget : public CScrollableWidget
{
public:
	CListBoxWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
			const UINT wSetW, const UINT wSetH,
			const bool bSortAlphabetically=false, const bool bRearrangable=false,
			const bool bMultipleSelection=false);
	virtual ~CListBoxWidget() {Clear();}

	UINT           AddItem(const UINT dwSetKey, const WCHAR *pwczSetText,
			const bool bGrayed=false, const int nInsertAt = -1, const bool bGrayLast=false);
	UINT           AddItemPointer(void* pSetKey, const WCHAR *pwczSetText,
			const bool bGrayed=false, const int nInsertAt = -1, const bool bGrayLast=false);
	void           Clear();
	bool           ClickedSelection() const;
	void           DeselectAll();
	void           DeselectLine(const UINT wLineNo);
	bool           DisableItem(const UINT dwKey) {return EnableItem(dwKey, false);}
	bool           EnableItem(const UINT dwKey, const bool bVal=true);
	bool           EnableItemAtIndex(const UINT index, const bool bVal=true);
	UINT           GetCursorLine() const {return this->wCursorLine;}
	CIDSet         GetItemStatus(const bool bEnabledItems) const;
	UINT           GetItemCount() const {return this->Items.size();}
	UINT           GetKeyAtCursor() const;
	UINT           GetKeyAtLine(const UINT wLineNo) const;
	void*          GetKeyPointerAtLine(const UINT wLineNo) const;
	UINT           GetKeyWithText(const WCHAR* pText) const;
	int            GetLineWithKey(const UINT dwKey) const;
	int            GetLineWithKeyPointer(const void* pKey) const;
	int            GetLineWithText(const WCHAR *pText) const;
	UINT           GetSelectedItem() const;
	void*          GetSelectedItemPointer() const;
	CIDSet         GetSelectedItems() const;
	vector<void*>  GetSelectedItemsPointers() const;
	vector<UINT>   GetSelectedItemsVector() const;
	WSTRING        GetSelectedItemText() const;
	vector<WSTRING>GetSelectedItemTexts() const;
	UINT           GetSelectedLineCount() const;
	int            GetSelectedLineNumber() const;
	CIDSet         GetSelectedLineNumbers() const {return this->selectedLines;}
	const WCHAR*   GetTextAtCursor() const;
	const WCHAR*   GetTextAtLine(const UINT wLineNo) const;
	const WCHAR*   GetTextForKey(const UINT dwKey) const;
	UINT           GetTopLineNumber() const {return this->wTopLineNo;}
	UINT           HasAnyKey(const CIDSet& keys) const;
	bool           HasKeys(const CIDSet& keys) const;
	void           IgnoreLeadingArticlesInSort(bool val=true) { bIgnoreLeadingArticlesInSort = val; }
	bool           IsCursorLineSelected() const {return this->selectedLines.has(this->wCursorLine);}
	bool           IsEmpty() const;
	bool           IsItemEnabled(const UINT dwKey) const;
	bool           IsLineSelected(const UINT line) const {return this->selectedLines.has(line);}
	bool           ItemIsSelected() const {return !this->selectedLines.empty();}
	void           MoveViewToBottom();
	void           MoveViewToTop();
	virtual void   Paint(bool bUpdateRect = true);
	bool           RemoveItem(const UINT dwKey);
	bool           RemoveItem(const void* pKey);
	bool           RemoveItems(const CIDSet& keys);
	void           SelectAllLines();
	void           SelectItem(const UINT dwKey, const bool bRetainSelections=false);
	void           SelectItems(const CIDSet& keys, bool bRetainSelections=false);
	bool           SelectLine(const UINT wLineNo, const bool bRetainSelections=false);
	bool           SelectLineStartingWith(const WCHAR wc);
	bool           SelectLineWithText(const WCHAR* pText);
	void           SelectMultipleItems(const bool bVal);
	void           SetAllowFiltering(const bool bVal) { this->bAllowFiltering = bVal; }
	void           SetHotkeyItemSelection(const bool bVal=true) {this->bHotkeyItemSelection = bVal;}
	void           SetItemColor(const UINT dwKey, const SDL_Color& color);
	void           SetItemColorAtLine(const UINT index, const SDL_Color& color);
	void           SetItemText(const UINT dwKey, const WCHAR *pwczSetText);
	void           SetRearrangeable(bool val) { this->bRearrangable = val; }
	void           SetRearrangeable(const UINT dwKey, bool val);
	void           SetSelectedItemText(const WCHAR *pwczSetText);
	void           SetTopLineNumber(const UINT wSetTopLine);
	void           SortAlphabetically(const bool bVal) {this->bSortAlphabetically = bVal;}
	void           UnsetCursorLine() { this->wCursorLine = static_cast<UINT>(-1); }
	virtual void   Unselect(const bool bPaint = true);

	static WSTRING wstrFilterWord; // Text prefixing the filter-input preview

protected:
	virtual void   HandleDrag(const SDL_MouseMotionEvent &MouseMotionEvent);
	virtual void   HandleKeyDown(const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   HandleMouseDown(const SDL_MouseButtonEvent &MouseButtonEvent);
	virtual void   HandleMouseUp(const SDL_MouseButtonEvent &MouseButtonEvent);
	virtual void   HandleMouseWheel(const SDL_MouseWheelEvent &Wheel);
	virtual SW_CLICK_RESULT ClickAtCoords(const int nX, const int nY);

	virtual void   CalcAreas();
	virtual void   DragVertPosButton(const int nY);
	virtual void   ScrollDownOneLine(const UINT wLines=1);
	virtual void   ScrollDownOnePage();
	virtual void   ScrollUpOneLine(const UINT wLines=1);
	virtual void   ScrollUpOnePage();

	vector<LBOX_ITEM*> Items;
	UINT           wTopLineNo;  //of view area
	UINT           wDisplayLineCount; //lines in view area
	UINT           wCursorLine, wRangeStartLine;
	CIDSet         selectedLines;
	bool           bMultipleSelection; //can select multiple list items

	UINT           wPosClickTopLineNo;
	bool           bClickedSelection;
	bool           bSortAlphabetically;
	bool           bIgnoreLeadingArticlesInSort;
	bool           bHotkeyItemSelection; //Ctrl-char selects line with that letter

	Uint32         dwLastScroll;
	bool           bRearrangable;    //can rearrange choices in list
	UINT           wDraggingLineNo;  //this line # is being moved in the list
	bool           bRearranged;      //choices were rearranged on mouse drag
	bool           bAllowFiltering;  //Allow filtering by typing text
	WSTRING        wstrActiveFilter;

private:
	UINT           AddItem_Insert(LBOX_ITEM *pNew, const bool bGrayed,
			const int nInsertAt, const bool bGrayLast);
	bool           IsValidFilterCharacter(const WCHAR character);
	WSTRING        StripLeadingArticle(const WSTRING& text);
	void           UpdateFilter(WSTRING wstrFilter);

	vector<LBOX_ITEM *> filteredItems;
};

#endif //#ifndef LISTBOXWIDGET_H

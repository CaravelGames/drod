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

#include "ListBoxWidget.h"
#include "EventHandlerWidget.h"
#include "BitmapManager.h"
#include "FontManager.h"
#include "Inset.h"
#include <BackEndLib/Types.h>
#include <BackEndLib/Assert.h>
#include <BackEndLib/Ports.h>
#include <BackEndLib/Wchar.h>

using namespace std;

WSTRING CListBoxWidget::wstrFilterWord;


#ifdef RUSSIAN_BUILD
const int TEXT_DRAW_Y_OFFSET = -1; //Because this particular font is a tiny bit too far down.
#else
const int TEXT_DRAW_Y_OFFSET = -5; //Because this particular font is always too far down.
#endif

const UINT ITEM_LEFT_PADDING = 2;



//
//Public methods.
//

//******************************************************************************
CListBoxWidget::CListBoxWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,             //(in)   Required params for CWidget
	const int nSetX, const int nSetY,   //    constructor.
	const UINT wSetW, const UINT wSetH, //
	const bool bSortAlphabetically,     //[default=false]
	const bool bRearrangable,           //[default=false]
	const bool bMultipleSelection)      //[default=false]
	: CScrollableWidget(dwSetTagNo, nSetX, nSetY, wSetW, wSetH, WT_ListBox)
	, wTopLineNo(0)
	, wDisplayLineCount(0)
	, wCursorLine(0), wRangeStartLine(0)
	, bMultipleSelection(bMultipleSelection)
	, wPosClickTopLineNo(0)
	, bClickedSelection(false)
	, bSortAlphabetically(bSortAlphabetically)
	, bIgnoreLeadingArticlesInSort(false)
	, bHotkeyItemSelection(false)
	, dwLastScroll(0)
	, bRearrangable(bRearrangable)
	, wDraggingLineNo(UINT(-1))
	, bRearranged(false)
	, bAllowFiltering(false)
	, wstrActiveFilter(wszEmpty)
{
	if (CListBoxWidget::wstrFilterWord.size() == 0)
		AsciiToUnicode("Filter", CListBoxWidget::wstrFilterWord);

	CalcAreas();
}

//******************************************************************************
UINT CListBoxWidget::AddItem(
//Add an item to end of list box items.
//
//Params:
	const UINT dwSetKey, //(in)   Arbitrary key associated with
								//    text.  Should be unique to all other keys.
	const WCHAR *pwczSetText,  //(in)   Text to display on screen.
	const bool bGrayed,  //(in) Whether text should be showed as grayed out [default=false]
	const int nInsertAt, //(in) if >=0, where in sequence new element is placed [default=-1]
	const bool bGrayLast)	//(in) if true, grayed items always go after non-gray items [default=false]
//Returns:  The index of where this item was inserted
{
	ASSERT(pwczSetText);

	//Create new item.
	LBOX_ITEM *pNew = new LBOX_ITEM;
	pNew->dwKey = dwSetKey;
	pNew->text = pwczSetText;
	pNew->bGrayed = bGrayed;

	return AddItem_Insert(pNew, bGrayed, nInsertAt, bGrayLast);
}

UINT CListBoxWidget::AddItemPointer(
//Add an item to end of list box items.
//
//Params:
	void* pSetKey,
	const WCHAR *pwczSetText,
	const bool bGrayed,
	const int nInsertAt,
	const bool bGrayLast)
{
	ASSERT(pwczSetText);

	//Create new item.
	LBOX_ITEM *pNew = new LBOX_ITEM;
	pNew->pKey = pSetKey;
	pNew->text = pwczSetText;
	pNew->bGrayed = bGrayed;

	return AddItem_Insert(pNew, bGrayed, nInsertAt, bGrayLast);
}

//******************************************************************************
UINT CListBoxWidget::AddItem_Insert(
//Add newly created item to list.
//
//Params:
	LBOX_ITEM *pNew,
	const bool bGrayed,  //(in) Whether text should be showed as grayed out [default=false]
	const int nInsertAt, //(in) if >=0, where in sequence new element is placed [default=-1]
	const bool bGrayLast)	//(in) if true, grayed items always go after non-gray items [default=false]
{
	UINT wIndex = 0;
	if (this->bSortAlphabetically)
	{
		//Insert alphabetically.
		ASSERT(nInsertAt == -1);	//ignored here
		vector<LBOX_ITEM *>::iterator iter = this->Items.begin();
		WSTRING strippedArticleNewText;
		if (this->bIgnoreLeadingArticlesInSort)
			strippedArticleNewText = StripLeadingArticle(pNew->text);
		while (iter != this->Items.end())
		{
			LBOX_ITEM *pItem = *iter;
			if (bGrayLast && !bGrayed && pItem->bGrayed)
				break;	//place non-grayed item before any grayed ones

			if (bGrayLast && bGrayed && !pItem->bGrayed)
			{
				//place gray item after non-gray items
				++iter;  ++wIndex;  continue;
			}

			if (this->bIgnoreLeadingArticlesInSort) {
				const WSTRING strippedArticleItemText = StripLeadingArticle(pItem->text);
				if (WCSicmp(strippedArticleItemText.c_str(), strippedArticleNewText.c_str()) >= 0)
					break;
			} else {
				if (WCSicmp(pItem->text.c_str(),pNew->text.c_str()) >= 0)
					break;	//at correct alphabetical position
			}

			++iter;
			++wIndex;
		}
		this->Items.insert(iter, pNew);
	} else {
		if (nInsertAt == -1)
		{
			if (!bGrayLast || bGrayed)
			{
				//Append.
				this->Items.push_back(pNew);
				wIndex = this->Items.size() - 1;
			} else {
				//Append to non-gray items, before any grayed items.
				ASSERT(bGrayLast);
				ASSERT(!bGrayed);
				vector<LBOX_ITEM *>::iterator iter = this->Items.begin();
				while (iter != this->Items.end() && !(*iter)->bGrayed)
				{
					++iter;
					++wIndex;
				}
				this->Items.insert(iter, pNew);
			}
		} else {
			//Insert at specific index.
			ASSERT((int)this->Items.size() >= nInsertAt);
			this->Items.insert(this->Items.begin() + nInsertAt, pNew);
			wIndex = nInsertAt;
		}
	}

	//Recalc areas of widget since they may have changed.
	this->UpdateFilter(this->wstrActiveFilter);
	CalcAreas();
	return wIndex;
}

//******************************************************************************
WSTRING CListBoxWidget::StripLeadingArticle(const WSTRING& text)
{
	static const WCHAR a[] = {We('a'),We(' '),We(0)};
	static const WCHAR an[] = {We('a'),We('n'),We(' '),We(0)};
	static const WCHAR the[] = {We('t'),We('h'),We('e'),We(' '),We(0)};

	static const size_t NUM_ARTICLES = 3;
	static const WCHAR* articles[NUM_ARTICLES] = {the, an, a};

	for (size_t i=0; i<NUM_ARTICLES; ++i) {
		const size_t len = WCSlen(articles[i]);
		if (!WCSnicmp(text.c_str(), articles[i], len))
			return WSTRING(text.c_str() + len);
	}

	return text;
}

//******************************************************************************
void CListBoxWidget::Clear()
//Clear the list box so that it contains no items.
{
	for (vector<LBOX_ITEM *>::const_iterator iSeek = this->Items.begin();
			iSeek != this->Items.end(); ++iSeek)
		delete *iSeek;
	this->Items.clear();
	this->filteredItems.clear();
	this->selectedLines.clear();
	this->wstrActiveFilter.clear();
	UnsetCursorLine();

	//Recalc areas of widget since they may have changed.
	CalcAreas();
}

//*****************************************************************************
bool CListBoxWidget::ClickedSelection() const
//Returns: whether any list item was clicked on
{
	return this->bClickedSelection;
}

//******************************************************************************
void CListBoxWidget::DeselectAll()
//Deselects everything.
{
	this->selectedLines.clear();
}

//******************************************************************************
void CListBoxWidget::DeselectLine(const UINT wLineNo)
//Deselects line.
{
	this->selectedLines -= wLineNo;
}

//******************************************************************************
bool CListBoxWidget::EnableItem(const UINT dwKey, const bool bVal)	//[default=true]
//Enables/disables list item with specified key.
//
//Returns: true if item with key is found, else false.
{
	for(vector<LBOX_ITEM *>::const_iterator iSeek = this->filteredItems.begin();
		iSeek != this->filteredItems.end(); ++iSeek)
	{
		if ((*iSeek)->dwKey == dwKey) //Found item.
		{
			(*iSeek)->bGrayed = !bVal;
			return true;
		}
	}
	return false;
}

//******************************************************************************
bool CListBoxWidget::EnableItemAtIndex(const UINT index, const bool bVal) //[default=true]
//Enables/disables item at given index in list.
//
//Returns: true if index is valid, else false
{
	if (index >= this->filteredItems.size())
		return false;
	this->filteredItems[index]->bGrayed = !bVal;

	return true;
}

//******************************************************************************
CIDSet CListBoxWidget::GetItemStatus(const bool bEnabledItems) const
//Returns:  set of IDs for enabled/disabled items
{
	CIDSet ids;
	for (vector<LBOX_ITEM *>::const_iterator iSeek = this->Items.begin();
			iSeek != this->Items.end(); ++iSeek)
		if ((*iSeek)->bGrayed != bEnabledItems)
			ids += (*iSeek)->dwKey;
	return ids;
}

//******************************************************************************
UINT CListBoxWidget::GetKeyAtCursor() const
//Returns: the tag # of item where the cursor is, or 0 if list is empty
{
	return GetKeyAtLine(this->wCursorLine);
}

//******************************************************************************
UINT CListBoxWidget::GetKeyAtLine(const UINT wLineNo) const
//Returns: tag # of item on specified line in list, or 0 if line is not valid
{
	if (wLineNo >= this->filteredItems.size())
		return 0;

	return this->filteredItems[wLineNo]->dwKey;
}

//******************************************************************************
void* CListBoxWidget::GetKeyPointerAtLine(const UINT wLineNo) const
//Returns: tag pointer of item on specified line in list
{
	if (wLineNo >= this->filteredItems.size())
		return NULL;

	return this->filteredItems[wLineNo]->pKey;
}

//******************************************************************************
UINT CListBoxWidget::GetKeyWithText(const WCHAR* pText) const
//Returns: key of first line matching text, or -1 if none
{
	int nLine = 0;
	for (vector<LBOX_ITEM *>::const_iterator iSeek = this->filteredItems.begin();
		iSeek != this->filteredItems.end(); ++iSeek, ++nLine)
	{
		if (!WCScmp((*iSeek)->text.c_str(), pText)) //Found item.
			return (*iSeek)->dwKey;
	}
	return -1;
}

//******************************************************************************
int CListBoxWidget::GetLineWithKey(const UINT dwKey) const
//Returns: first line number with indicated key, or -1 if none
{
	int nLine = 0;
	for (vector<LBOX_ITEM *>::const_iterator iSeek = this->filteredItems.begin();
		iSeek != this->filteredItems.end(); ++iSeek, ++nLine)
	{
		if ((*iSeek)->dwKey == dwKey) //Found item.
			return nLine;
	}
	return -1;
}

//******************************************************************************
int CListBoxWidget::GetLineWithKeyPointer(const void* pKey) const
//Returns: first line number with indicated key pointer, or -1 if none
{
	int nLine = 0;
	for (vector<LBOX_ITEM *>::const_iterator iSeek = this->filteredItems.begin();
		iSeek != this->filteredItems.end(); ++iSeek, ++nLine)
	{
		if ((*iSeek)->pKey == pKey) //Found item.
			return nLine;
	}
	return -1;
}

//******************************************************************************
int CListBoxWidget::GetLineWithText(const WCHAR* pText) const
//Returns: first line number with text, or -1 if none
{
	int nLine = 0;
	for (vector<LBOX_ITEM *>::const_iterator iSeek = this->filteredItems.begin();
		iSeek != this->filteredItems.end(); ++iSeek, ++nLine)
	{
		if (!WCScmp((*iSeek)->text.c_str(), pText)) //Found item.
			return nLine;
	}
	return -1;
}

//******************************************************************************
UINT CListBoxWidget::GetSelectedItem() const
//Returns key corresponding to (first) selected item, or 0 if none.
{
	if (this->selectedLines.empty())
		return 0;
	const UINT wSelectedLine = this->selectedLines.getFirst();
	ASSERT(wSelectedLine < this->filteredItems.size());

	return this->filteredItems[wSelectedLine]->dwKey;
}

//******************************************************************************
void* CListBoxWidget::GetSelectedItemPointer() const
//Returns key-pointer corresponding to (first) selected item, or NULL if none.
{
	if (this->selectedLines.empty())
		return NULL;
	const UINT wSelectedLine = this->selectedLines.getFirst();
	ASSERT(wSelectedLine < this->filteredItems.size());

	return this->filteredItems[wSelectedLine]->pKey;
}

//******************************************************************************
CIDSet CListBoxWidget::GetSelectedItems() const
//Returns: a set of keys corresponding to all selected items, or empty set if none.
{
	CIDSet keys;
	for (CIDSet::const_iterator line = this->selectedLines.begin();
			line != this->selectedLines.end(); ++line)
	{
		ASSERT(*line < this->filteredItems.size());
		keys += this->filteredItems[*line]->dwKey;
	}
	return keys;
}

//******************************************************************************
vector<void*> CListBoxWidget::GetSelectedItemsPointers() const
//Returns: a vector of pointer keys corresponding to all selected items, or empty vector if none.
{
	vector<void*> keys;
	for (CIDSet::const_iterator line = this->selectedLines.begin();
			line != this->selectedLines.end(); ++line)
	{
		ASSERT(*line < this->filteredItems.size());
		keys.push_back(this->filteredItems[*line]->pKey);
	}
	return keys;
}

//******************************************************************************
vector<UINT> CListBoxWidget::GetSelectedItemsVector() const
//Returns: a vector of keys corresponding to all selected items, or empty vector if none.
{
	vector<UINT> keys;
	for (CIDSet::const_iterator line = this->selectedLines.begin();
			line != this->selectedLines.end(); ++line)
	{
		ASSERT(*line < this->filteredItems.size());
		keys.push_back(this->filteredItems[*line]->dwKey);
	}
	return keys;
}

//******************************************************************************
WSTRING CListBoxWidget::GetSelectedItemText() const
//Returns text corresponding to (first) selected item, or "" if none.
{
	if (this->selectedLines.empty())
		return wszEmpty;
	const UINT wSelectedLine = this->selectedLines.getFirst();
	ASSERT(wSelectedLine < this->filteredItems.size());

	return this->filteredItems[wSelectedLine]->text;
}

//******************************************************************************
vector<WSTRING> CListBoxWidget::GetSelectedItemTexts() const
//Returns text corresponding to (first) selected item, or "" if none.
{
	vector<WSTRING> texts;
	for (CIDSet::const_iterator line = this->selectedLines.begin();
			line != this->selectedLines.end(); ++line)
	{
		ASSERT(*line < this->filteredItems.size());
	   texts.push_back(this->filteredItems[*line]->text);
	}
	return texts;
}

//******************************************************************************
UINT CListBoxWidget::GetSelectedLineCount() const
//Return: number of list items currently selected
{
	return this->selectedLines.size();
}

//******************************************************************************
int CListBoxWidget::GetSelectedLineNumber() const
{
	return this->selectedLines.empty() ? -1 : (int)this->selectedLines.getFirst();
}

//******************************************************************************
const WCHAR* CListBoxWidget::GetTextAtCursor() const
//Returns: the text at the cursor's line, or NULL if none
{
	return GetTextAtLine(this->wCursorLine);
}

//******************************************************************************
const WCHAR* CListBoxWidget::GetTextAtLine(const UINT wLineNo) const
//Returns: text of item on specified line in list
{
	if (wLineNo >= this->filteredItems.size())
		return NULL;

	return this->filteredItems[wLineNo]->text.c_str();
}

//******************************************************************************
const WCHAR* CListBoxWidget::GetTextForKey(const UINT dwKey) const
//Returns: text corresponding to item with specified key, or "" if none.
{
	for (vector<LBOX_ITEM *>::const_iterator iSeek = this->filteredItems.begin();
		iSeek != this->filteredItems.end(); ++iSeek)
	{
		if ((*iSeek)->dwKey == dwKey) //Found item.
			return (*iSeek)->text.c_str();
	}
	return wszEmpty;
}

//******************************************************************************
UINT CListBoxWidget::HasAnyKey(const CIDSet& keys) const
//Returns: if the list contains any of these these keys, return its value
//         Otherwise 0.
{
	for (CIDSet::const_iterator key=keys.begin(); key != keys.end(); ++key)
		if (GetLineWithKey(*key) != -1)
			return *key;

	return 0;
}

//******************************************************************************
bool CListBoxWidget::HasKeys(const CIDSet& keys) const
//Returns: whether the list contains these keys or not
{
	for (CIDSet::const_iterator key=keys.begin(); key != keys.end(); ++key)
		if (GetLineWithKey(*key) == -1)
			return false;

	return true;
}

//******************************************************************************
bool CListBoxWidget::IsEmpty() const
//Returns: whether list is empty
{
	return GetItemCount() == 0;
}

//******************************************************************************
bool CListBoxWidget::IsItemEnabled(const UINT dwKey) const
//Returns: whether item with key is found and enabled, else false.
{
	for(vector<LBOX_ITEM *>::const_iterator iSeek = this->filteredItems.begin();
		iSeek != this->filteredItems.end(); ++iSeek)
	{
		if ((*iSeek)->dwKey == dwKey) //Found item.
			return !((*iSeek)->bGrayed);
	}
	return false;
}

//******************************************************************************
void CListBoxWidget::MoveViewToBottom()
//Moves view to bottom of list.
{
	const UINT topLine = this->wTopLineNo;
	if (GetItemCount() <= this->wDisplayLineCount)
		this->wTopLineNo = 0;
	else
		this->wTopLineNo = GetItemCount() - this->wDisplayLineCount;

	if (this->wTopLineNo != topLine)
		CalcAreas();
}

//******************************************************************************
void CListBoxWidget::MoveViewToTop()
//Moves view to top of list.
{
	if (this->wTopLineNo > 0)
	{
		this->wTopLineNo = 0;
		CalcAreas();
	}
}

//******************************************************************************
bool CListBoxWidget::RemoveItem(
//Removes first item with specified key from the list.
//Returns: whether item with this key was removed
//
//Params:
	const UINT dwKey)   //(in)   Unique key indicating which item to remove.
{
	//Find the line corresponding to the key.
	bool bRemoved = false;
	UINT wSeekLineNo = 0;
	for (vector<LBOX_ITEM *>::iterator iSeek = this->Items.begin();
		iSeek != this->Items.end(); ++iSeek, ++wSeekLineNo)
	{
		if ((*iSeek)->dwKey == dwKey) //Found item.
		{
			//Remove from list.
			this->selectedLines -= wSeekLineNo;

			//Update set of selected lines to their new list positions.
			CIDSet remainingSelectedKeys = GetSelectedItems();
			this->Items.erase(iSeek);
			SelectItems(remainingSelectedKeys);

			if (!this->Items.empty())
			{
				if ((wSeekLineNo < this->wCursorLine && int(this->wCursorLine) > 0) ||
						(wSeekLineNo == this->wCursorLine &&
								wSeekLineNo == this->Items.size()))
				{
					--this->wCursorLine;
					if (this->selectedLines.empty() && this->wCursorLine < this->Items.size())
						this->selectedLines += this->wCursorLine;
				}
			}
			bRemoved = true;
			break;
		}
	}

	UpdateFilter(this->wstrActiveFilter);
	//Recalc areas of widget since they may have changed.
	CalcAreas();

	return bRemoved;
}

bool CListBoxWidget::RemoveItem(const void* pKey)
{
	//Find the line corresponding to the key.
	bool bRemoved = false;
	UINT wSeekLineNo = 0;
	for (vector<LBOX_ITEM *>::iterator iSeek = this->Items.begin();
		iSeek != this->Items.end(); ++iSeek, ++wSeekLineNo)
	{
		if ((*iSeek)->pKey == pKey) //Found item.
		{
			//Remove from list.
			this->selectedLines -= wSeekLineNo;

			//Update set of selected lines to their new list positions.
			CIDSet remainingSelectedKeys = GetSelectedItems();
			this->Items.erase(iSeek);
			SelectItems(remainingSelectedKeys);

			if (!this->Items.empty())
			{
				if (wSeekLineNo < this->wCursorLine || (wSeekLineNo ==
						this->wCursorLine && wSeekLineNo == this->Items.size()))
				{
					--this->wCursorLine;
					if (this->selectedLines.empty())
						this->selectedLines += this->wCursorLine;
				}
			}
			bRemoved = true;
			break;
		}
	}

	UpdateFilter(this->wstrActiveFilter);

	//Recalc areas of widget since they may have changed.
	CalcAreas();

	return bRemoved;
}

bool CListBoxWidget::RemoveItems(const CIDSet& keys)
{
	//Find the line corresponding to the key.
	bool bRemoved = false;
	UINT wIndex = 0;

	CIDSet remainingSelectedKeys = GetSelectedItems();
	for (vector<LBOX_ITEM*>::iterator iSeek = this->Items.begin();
		iSeek != this->Items.end(); ++iSeek, ++wIndex)
	{
		wIndex++;

		if (keys.has((*iSeek)->dwKey)) {
			this->Items.erase(iSeek);
			bRemoved = true;
			if (this->wCursorLine > wIndex)
				--this->wCursorLine;

			wIndex--;
		}
	}

	this->selectedLines.clear();
	UpdateFilter(this->wstrActiveFilter);
	SelectItems(remainingSelectedKeys);

	//Recalc areas of widget since they may have changed.
	CalcAreas();

	return bRemoved;
}

//******************************************************************************
void CListBoxWidget::SelectAllLines()
//Selects all lines.
{
	this->selectedLines.clear();
	for (UINT line=this->filteredItems.size(); line--; )
		this->selectedLines += line;
}

//******************************************************************************
void CListBoxWidget::SelectItem(
//Selects an item in the list box.
//
//Params:
	const UINT dwKey,   //(in)   Indicates which item to select.
	const bool bRetainSelections)  //[default=false]
{
	//Find the line corresponding to the key.
	UINT wSeekLineNo = 0;
	for(vector<LBOX_ITEM *>::const_iterator iSeek = this->filteredItems.begin();
		iSeek != this->filteredItems.end(); ++iSeek)
	{
		if ((*iSeek)->dwKey == dwKey) //Found item.
		{
			SelectLine(wSeekLineNo, bRetainSelections);
			this->bClickedSelection = false;        //this selection wasn't just clicked
			return;
		}
		++wSeekLineNo;
	}
}

//******************************************************************************
void CListBoxWidget::SelectItems(const CIDSet& keys, bool bRetainSelections) //false
//Selects all items in the list box, if allowed.
//Otherwise, only the first item in the set is selected.
//If bRetainSelections is true, any previous selections will be retained.
{
	if (keys.empty())
		return; //nothing to select -- retain old selection

	if (!this->bMultipleSelection)
		SelectItem(keys.getFirst(), bRetainSelections); //select only the first key in the set
	else
		for (CIDSet::const_iterator key = keys.begin(); key != keys.end(); ++key)
		{
			SelectItem(*key, bRetainSelections);
			bRetainSelections = true; //remaining selections are added to this one
		}
}

//******************************************************************************
bool CListBoxWidget::SelectLine(
//Moves cursor and selects a line in the list box.
//
//Returns: true if selection set changed
//
//Params:
	const UINT wLineNo,  //(in)   Line to select.
	const bool bRetainSelections)  //[default=false]
{
	if (this->filteredItems.empty())
		return false; //nothing to select

	ASSERT(wLineNo == 0 || wLineNo < this->filteredItems.size());

	const CIDSet oldSelection = this->selectedLines;

	if (!bRetainSelections || !this->bMultipleSelection)
	{
		this->selectedLines.clear();
		this->wRangeStartLine = wLineNo;
	}
	this->selectedLines += wLineNo;
	this->wCursorLine = wLineNo;

	//If selected line is not in current page, scroll the list box to show it.
	if (wLineNo < this->wTopLineNo)
		this->wTopLineNo = wLineNo;
	else if (this->wTopLineNo + this->wDisplayLineCount <= wLineNo)
		this->wTopLineNo = wLineNo - this->wDisplayLineCount + 1;

	//Recalc areas of widget since they may have changed.
	CalcAreas();

	return oldSelection != this->selectedLines;
}

//*****************************************************************************
bool CListBoxWidget::SelectLineStartingWith(const WCHAR wc)
//Selects the first line in the list starting with the indicated character.
//Returns: Whether an item with this text exists.
{
	if (this->filteredItems.empty())
		return false; //nothing to select

	const CIDSet oldSelection = this->selectedLines;

	//Search for a line starting with this character.
	UINT wLineNo;
	for (wLineNo = 0; wLineNo < this->filteredItems.size(); ++wLineNo)
	{
		const WCHAR line_wc = this->bIgnoreLeadingArticlesInSort ? 
			towlower(StripLeadingArticle(this->filteredItems[wLineNo]->text)[0]) :
			towlower(this->filteredItems[wLineNo]->text[0]);
		
		if (line_wc == wc)
			break; //found one
		if (this->bSortAlphabetically &&
				(wc < line_wc || wLineNo == this->filteredItems.size()-1))
			break; //passed the spot where the letter would appear in the list
	}
	if (wLineNo == this->filteredItems.size())
		return false; //nothing found

	//Go to this line.
	this->selectedLines = wLineNo;
	this->wCursorLine = wLineNo;

	//If selected line is not in current page, scroll the list box to show it.
	if (wLineNo < this->wTopLineNo)
		this->wTopLineNo = wLineNo;
	else if (this->wTopLineNo + this->wDisplayLineCount <= wLineNo)
		this->wTopLineNo = wLineNo - this->wDisplayLineCount + 1;

	//Recalc areas of widget since they may have changed.
	CalcAreas();

	return oldSelection != this->selectedLines;
}

//*****************************************************************************
bool CListBoxWidget::SelectLineWithText(const WCHAR* pText)
//Selects the first line in the list matching this text.
//Returns: Whether an item with this text exists.
{
	if (this->filteredItems.empty())
		return false; //nothing to select

	const CIDSet oldSelection = this->selectedLines;

	//Search for a line with this text.
	UINT wLineNo;
	for (wLineNo = 0; wLineNo < this->filteredItems.size(); ++wLineNo)
		if (!WCScmp(this->filteredItems[wLineNo]->text.c_str(), pText))
			break; //found one
	if (wLineNo == this->filteredItems.size())
		return false; //nothing found

	//Go to this line.  Select it.
	this->selectedLines = wLineNo;
	this->wCursorLine = wLineNo;

	//If selected line is not in current page, scroll the list box to show it.
	if (wLineNo < this->wTopLineNo)
		this->wTopLineNo = wLineNo;
	else if (this->wTopLineNo + this->wDisplayLineCount <= wLineNo)
		this->wTopLineNo = wLineNo - this->wDisplayLineCount + 1;

	//Recalc areas of widget since they may have changed.
	CalcAreas();

	return oldSelection != this->selectedLines;
}

//*****************************************************************************
void CListBoxWidget::SelectMultipleItems(const bool bVal)
//Set whether multiple items can be selected simultaneously.
{
	this->bMultipleSelection = bVal;
}

//******************************************************************************
void CListBoxWidget::Paint(
//Paint widget area.
//
//Params:
	bool bUpdateRect)       //(in)   If true (default) and destination
								//    surface is the screen, the screen
								//    will be immediately updated in
								//    the widget's rect.
{
	ASSERT(IsLoaded());

	//Drawing code below needs to be modified to accept offsets.  Until then,
	//this widget can't be offset.
	ASSERT(!IsScrollOffset());

	//Draw inset area where text appears.
	SDL_Surface *pDestSurface = GetDestSurface();
	DrawInset(this->x, this->y, this->w, this->h, this->images[0],
			pDestSurface, true, false, !IsEnabled());

	const vector<LBOX_ITEM *> &items = this->filteredItems;

	const bool bDrawScrollBar = (items.size() > this->wDisplayLineCount);
	if (bDrawScrollBar)
		DrawVertScrollBar(pDestSurface);

	//Find top item.
	vector<LBOX_ITEM *>::const_iterator iListItem = items.begin() + this->wTopLineNo;

	//Draw item text.
	const UINT wStopLineNo = this->wTopLineNo + this->wDisplayLineCount;
	for (UINT wLineNo = this->wTopLineNo;
			wLineNo < wStopLineNo && iListItem != items.end();
			++wLineNo, ++iListItem)
	{
		Paint_Line(wLineNo, wLineNo - this->wTopLineNo, **iListItem);
	}

	this->ItemsRect.y = this->y + CY_INSET_BORDER;

	if (this->wstrActiveFilter.size() > 0)
	{
		int drawX, drawY;
		UINT drawWidth, drawHeight;
		GetLineDrawCoords(0, drawX, drawY, drawWidth, drawHeight);

		drawX = this->x + 10;
		drawY = this->y + TEXT_DRAW_Y_OFFSET + this->h - CY_LBOX_ITEM - 1;

		const SURFACECOLOR FilterSeparatorColor = GetSurfaceColor(GetDestSurface(), 102, 102, 102);
		
		WSTRING message = this->wstrFilterWord;
		message += wszColon;
		message += wszSpace;
		message += this->wstrActiveFilter;
		DrawRow(this->x, drawY + 3, drawWidth, FilterSeparatorColor, pDestSurface);
		g_pTheFM->DrawTextXY(FONTLIB::F_ListBoxItem, message.c_str(), pDestSurface,
			drawX, drawY,
			drawWidth - 20, (int)CY_LBOX_ITEM - TEXT_DRAW_Y_OFFSET);

	}

	//PaintChildren();
	ASSERT(this->Children.empty()); //list box doesn't have children
	if (bUpdateRect) UpdateRect();
}

//******************************************************************************
void CListBoxWidget::Paint_Line(
	const UINT wListItemNumber,
	const UINT wDrawLineNumber,
	const LBOX_ITEM &listItem)
{
	SDL_Surface *pDestSurface = GetDestSurface();

	bool bIsSelected = this->selectedLines.has(wListItemNumber);

	int drawX, drawY;
	UINT drawWidth, drawHeight;
	GetLineDrawCoords(wDrawLineNumber, drawX, drawY, drawWidth, drawHeight);

	ASSERT(drawY < static_cast<int>(this->y + this->h - CY_INSET_BORDER));

	//If this is selected item, draw solid rect underneath.
	if (bIsSelected)
	{
		SDL_Rect ItemRect = MAKE_SDL_RECT(
			drawX - ITEM_LEFT_PADDING, drawY,
			drawWidth + ITEM_LEFT_PADDING, drawHeight
		);

		const SURFACECOLOR BackColor = GetSurfaceColor(pDestSurface, 190, 181, 165);
		DrawFilledRect(ItemRect, BackColor, pDestSurface);
	}

	const UINT eDrawFont = bIsSelected
		? FONTLIB::F_SelectedListBoxItem
		: FONTLIB::F_ListBoxItem;
	SDL_Color origColor = g_pTheFM->GetFontColor(eDrawFont);

	//whether text is shown grayed out
	if (listItem.bGrayed)
		g_pTheFM->SetFontColor(eDrawFont, Gray);
	else
		g_pTheFM->SetFontColor(eDrawFont, listItem.color);

	//Draw selected line text.
	g_pTheFM->DrawTextXY(eDrawFont, listItem.text.c_str(), pDestSurface,
		drawX, drawY + TEXT_DRAW_Y_OFFSET, drawWidth, (int)drawHeight - TEXT_DRAW_Y_OFFSET);
	g_pTheFM->SetFontColor(eDrawFont, origColor);

	//Draw focus box (cursor).
	const bool bCursorOnLine = (wListItemNumber == this->wCursorLine);
	if (bCursorOnLine && IsSelected())
	{
		SDL_Rect ItemRect = MAKE_SDL_RECT(
			drawX - ITEM_LEFT_PADDING, drawY,
			drawWidth + ITEM_LEFT_PADDING, drawHeight
		);

		const SURFACECOLOR FocusColor =
			GetSurfaceColor(pDestSurface, RGB_FOCUS);
		DrawRect(ItemRect, FocusColor, pDestSurface);
	}
}

//******************************************************************************
void CListBoxWidget::SetItemColor(const UINT dwKey, const SDL_Color& color)
{
	//Find item with specified key.
	for (vector<LBOX_ITEM *>::const_iterator iSeek = this->Items.begin();
		iSeek != this->Items.end(); ++iSeek)
	{
		if ((*iSeek)->dwKey == dwKey) //Found item.
		{
			(*iSeek)->color = color;
			return;
		}
	}
}

//******************************************************************************
void CListBoxWidget::SetItemColorAtLine(const UINT index, const SDL_Color& color)
{
	if (index >= this->filteredItems.size())
		return;

	this->filteredItems[index]->color = color;
}

//******************************************************************************
void CListBoxWidget::SetItemText(const UINT dwKey, const WCHAR *pwczSetText)
//Updates the text description of the item with specified key.
{
	ASSERT(pwczSetText);

	//Find item with specified key.
	for (vector<LBOX_ITEM *>::const_iterator iSeek = this->Items.begin();
		iSeek != this->Items.end(); ++iSeek)
	{
		if ((*iSeek)->dwKey == dwKey) //Found item.
		{
			//Reallocate text.
			(*iSeek)->text = pwczSetText;
			return;
		}
	}
}

//******************************************************************************
void CListBoxWidget::SetItemTextAtLine(const UINT index, const WCHAR *pwczSetText)
//Updates the text description of the item at the specific line.
{
	ASSERT(pwczSetText);

	if (index >= this->filteredItems.size())
		return;

	this->filteredItems[index]->text = pwczSetText;
}

//******************************************************************************
void CListBoxWidget::SetRearrangeable(const UINT dwKey, bool val)
{
	for (vector<LBOX_ITEM *>::const_iterator iSeek = this->Items.begin();
		iSeek != this->Items.end(); ++iSeek)
	{
		if ((*iSeek)->dwKey == dwKey) //Found item.
		{
			(*iSeek)->rearrangeable = val;
			return;
		}
	}
}

//******************************************************************************
void CListBoxWidget::SetSelectedItemText(const WCHAR *pwczSetText)
//Updates the text description of the item at the cursor's location.
{
	if (this->wCursorLine >= this->filteredItems.size()) return;
	ASSERT(pwczSetText);

	vector<LBOX_ITEM *>::const_iterator iListItem = this->filteredItems.begin() + this->wCursorLine;
	(*iListItem)->text = pwczSetText;
}

//******************************************************************************
void CListBoxWidget::SetTopLineNumber(const UINT wSetTopLine)
//Sets the list view so the item line showing at the top of the list is the
//specified line number.
{
	if (wSetTopLine <= this->filteredItems.size() - this->wDisplayLineCount)
		this->wTopLineNo = wSetTopLine;
}

//
//Protected methods.
//

//******************************************************************************
void CListBoxWidget::HandleKeyDown(
//Processes a keyboard event within the scope of the widget.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
{
	UINT wPrevCursorLine = this->wCursorLine;
	UINT wNewCursorLine = wPrevCursorLine;

	switch (KeyboardEvent.keysym.sym)
	{
		case SDLK_UP: case SDLK_KP_8:
			if (wNewCursorLine > 0 && wNewCursorLine < this->filteredItems.size())
				--wNewCursorLine;
		break;

		case SDLK_DOWN: case SDLK_KP_2:
			if (wNewCursorLine < this->filteredItems.size() - 1)
				++wNewCursorLine;
		break;

		case SDLK_HOME: case SDLK_KP_7:
			wNewCursorLine = 0;
		break;

		case SDLK_END: case SDLK_KP_1:
			wNewCursorLine = this->filteredItems.size() - 1;
		break;

		case SDLK_PAGEUP: case SDLK_KP_9:
			if ((int)(wNewCursorLine) - (int)(this->wDisplayLineCount) > 0)
				wNewCursorLine -= this->wDisplayLineCount;
			else
				wNewCursorLine = 0;
		break;

		case SDLK_PAGEDOWN: case SDLK_KP_3:
			if ((int)(wNewCursorLine) + (int)(this->wDisplayLineCount) <
						(int)this->filteredItems.size() - 1)
				wNewCursorLine += this->wDisplayLineCount;
			else
				wNewCursorLine = this->filteredItems.size() - 1;
		break;

		case SDLK_SPACE:
			if (this->bAllowFiltering)
				UpdateFilter(this->wstrActiveFilter + wszSpace);

			//Toggle selection on current line.
			else if (this->bMultipleSelection && this->selectedLines.has(this->wCursorLine))
			{
				DeselectLine(this->wCursorLine);
				RequestPaint();
			}
			else
				wPrevCursorLine = (UINT)(-1); //select the current line below
		break;

		case SDLK_BACKSPACE:
			if (this->wstrActiveFilter.size() > 0) {
				UpdateFilter(this->wstrActiveFilter.substr(0, this->wstrActiveFilter.size() - 1));
				RequestPaint();
			}
			break;

		case SDLK_ESCAPE:
			if (this->wstrActiveFilter.size() > 0) {
				UpdateFilter(wszEmpty);
				RequestPaint();
				PreventEventBubbling();
			}
			break;

		default:
			if (this->bAllowFiltering && !(KeyboardEvent.keysym.mod & KMOD_SHIFT)) {
				const WCHAR character = KeyboardEvent.keysym.sym == SDLK_SPACE ? We(' ') : TranslateUnicodeKeysym(KeyboardEvent.keysym);

				if (IsValidFilterCharacter(character)) {
					UpdateFilter(this->wstrActiveFilter + character);
					PreventEventBubbling();
					RequestPaint();
				}
			}

			if (this->bHotkeyItemSelection)
			{
				//CTRL+char to select that line.
				if (KeyboardEvent.keysym.mod & KMOD_CTRL)
				{
					const WCHAR wc = TranslateUnicodeKeysym(KeyboardEvent.keysym);
					if (SelectLineStartingWith(wc))
						wNewCursorLine = this->wCursorLine;
				}
			}
		break;
	}

	if (wPrevCursorLine != wNewCursorLine)
	{
		//Determine how item selection changes.
		const SDL_Keymod mod = SDL_GetModState();
		const bool bMoveCursorOnly = (mod & KMOD_CTRL) != 0;
		const bool bRetainSelection = (mod & KMOD_SHIFT) != 0;
		if (bMoveCursorOnly)
			this->wCursorLine = wNewCursorLine;
		else
			SelectLine(wNewCursorLine, bRetainSelection);

		//Paint cursor on new line.
		RequestPaint();

		//Call OnSelectChange() notifier.
		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
	}
}

//******************************************************************************
void CListBoxWidget::UpdateFilter(WSTRING wstrFilter)
{
	if (!this->bAllowFiltering) {
		this->filteredItems = this->Items;
		return;
	}

	wstrFilter = WCSToLower(wstrFilter);
	const std::vector<WSTRING> filterTokens = WCSExplode(wstrFilter, wszSpace[0]);

	vector<LBOX_ITEM *> items;

	UINT wCursorKey = GetKeyAtCursor();

	if (wstrFilter.size() == 0)
		items = this->Items;

	else {
		for (vector<LBOX_ITEM *>::const_iterator iSeek = this->Items.begin();
			iSeek != this->Items.end(); ++iSeek)
		{
			LBOX_ITEM *item = *iSeek;

			if (WCSContainsAll(WCSToLower(item->text), filterTokens))
				items.push_back(item);
		}
	}

	this->filteredItems = items;
	this->selectedLines.clear();
	this->wstrActiveFilter = wstrFilter;

	CalcAreas();

	{ // Update the cursor to match the old highlighted item or select the item at the top of the list
		UINT wNewCursorLine = GetLineWithKey(wCursorKey);
		if (wNewCursorLine >= this->filteredItems.size())
			wNewCursorLine = 0;

		SelectLine(wNewCursorLine);
	}

	// If there are fewer items than can fit or we are scrolled past the last item somehow, make things fit
	if (this->filteredItems.size() <= this->wDisplayLineCount)
		this->wTopLineNo = 0;

	while (this->wTopLineNo > 0 && this->wTopLineNo + this->wDisplayLineCount < this->filteredItems.size())
		--this->wTopLineNo;

	// Scroll so that the selected item is visible
	if (this->selectedLines.size() > 0)
	{
		const UINT wSelectedLine = GetSelectedLineNumber();
		while (wSelectedLine < this->wTopLineNo)
			ScrollUpOnePage();
		while (wSelectedLine >= this->wTopLineNo + this->wDisplayLineCount)
			ScrollDownOnePage();

		// We don't want to update the selection when no filter is active
		// just to avoid messing up with parents during their setup
		if (GetKeyAtCursor() != wCursorKey && wstrFilter.size() > 0) {
			CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
			if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
		}
	}

}

//******************************************************************************
void CListBoxWidget::GetLineDrawCoords(const UINT wLineNumber, int &drawX, int &drawY, UINT &drawWidth, UINT &drawHeight)
{
	const bool bDrawScrollBar = (this->filteredItems.size() > this->wDisplayLineCount);

	drawX = this->x + CX_INSET_BORDER + ITEM_LEFT_PADDING;
	drawY = this->y + CY_INSET_BORDER + wLineNumber * CY_LBOX_ITEM;
	drawWidth = this->w - (CX_INSET_BORDER * 2) - ITEM_LEFT_PADDING - (bDrawScrollBar * CX_UP);
	drawHeight = CY_LBOX_ITEM;
}

//******************************************************************************
void CListBoxWidget::HandleMouseDown(
//Handles a mouse down event.
//
//Params:
	const SDL_MouseButtonEvent &MouseButtonEvent)   //(in) Event to handle.
{
	CScrollableWidget::HandleMouseDown(MouseButtonEvent);

	if (this->eLastClickResult == LBCR_NewSelection)
	{
		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
	}

	if (IS_IN_RECT(MouseButtonEvent.x, MouseButtonEvent.y, this->ItemsRect) &&
			this->bRearrangable && GetItemCount() > 0)
	{
		//Start dragging this line to a new position.
		this->wDraggingLineNo = GetSelectedLineNumber();
	} else {
		this->wDraggingLineNo = static_cast<UINT>(-1);
	}
}

//******************************************************************************
void CListBoxWidget::HandleDrag(
//Handles a mouse up event.
//
//Params:
	const SDL_MouseMotionEvent &MouseMotionEvent)   //(in) Event to handle.
{
	CScrollableWidget::HandleDrag(MouseMotionEvent);

	//If dragging a list item to a new line, transfer it here.
	if (this->wDraggingLineNo == static_cast<UINT>(-1)) return;
	if (this->selectedLines.size() != 1) return; //can't drag more than one line at a time

	ASSERT(this->Items.size());

	//When dragging past top/bottom of list, scroll view slowly.
	static const Uint32 dwScrollInterval = 100;
	if (MouseMotionEvent.y < this->ItemsRect.y)
	{
		const Uint32 dwNow = SDL_GetTicks();
		if (dwNow - this->dwLastScroll > dwScrollInterval)
		{
			this->dwLastScroll = dwNow;
			ScrollUpOneLine();
			RequestPaint();
		}
	}
	else if (MouseMotionEvent.y > this->ItemsRect.y + this->ItemsRect.h)
	{
		const Uint32 dwNow = SDL_GetTicks();
		if (dwNow - this->dwLastScroll > dwScrollInterval)
		{
			this->dwLastScroll = dwNow;
			ScrollDownOneLine();
			RequestPaint();
		}
	}

	//Determine whether cursor has been moved a line.
	if (MouseMotionEvent.y <= this->ItemsRect.y)
		this->wCursorLine = this->wTopLineNo;
	else if (MouseMotionEvent.y >= this->ItemsRect.y + this->ItemsRect.h)
		this->wCursorLine = this->wTopLineNo + this->wDisplayLineCount - 1;
	else
		this->wCursorLine = this->wTopLineNo +
				(MouseMotionEvent.y - this->ItemsRect.y) / CY_LBOX_ITEM;
	if (this->wCursorLine >= this->Items.size())
		this->wCursorLine = this->Items.size()-1;
	if (this->wDraggingLineNo == this->wCursorLine) return;

	//Move item one line at a time.
	if (this->wCursorLine < this->wDraggingLineNo)
		this->wCursorLine = this->wDraggingLineNo-1;
	else if (this->wCursorLine > this->wDraggingLineNo)
		this->wCursorLine = this->wDraggingLineNo+1;

	//Switch lines, if permitted.
	LBOX_ITEM *pItem = this->Items[this->wDraggingLineNo];
	if (pItem->rearrangeable && this->Items[this->wCursorLine]->rearrangeable) {
		this->Items[this->wDraggingLineNo] = this->Items[this->wCursorLine];
		this->Items[this->wCursorLine] = pItem;
		this->wDraggingLineNo = this->wCursorLine;
		this->filteredItems = this->Items;

		SelectLine(this->wCursorLine);

		this->bRearranged = true;
		RequestPaint();
	} else {
		this->wCursorLine = this->wDraggingLineNo;
	}

	UpdateFilter(WSTRING());
	RequestPaint();
}

//******************************************************************************
void CListBoxWidget::HandleMouseUp(
//Handles a mouse up event.
//
//Params:
	const SDL_MouseButtonEvent &MouseButtonEvent)   //(in) Event to handle.
{
	CScrollableWidget::HandleMouseUp(MouseButtonEvent);

	this->wDraggingLineNo = static_cast<UINT>(-1);

	if (this->bRearranged)
	{
		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) pEventHandler->OnRearranged(GetTagNo());
		this->bRearranged = false;
	}
}

//******************************************************************************
void CListBoxWidget::HandleMouseWheel(
//Handles a mouse wheel event.
//
//Params:
	const SDL_MouseWheelEvent &MouseWheelEvent)   //(in) Event to handle.
{
	CScrollableWidget::HandleMouseWheel(MouseWheelEvent);

	if (this->eLastClickResult == LBCR_NewSelection)
	{
		CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
		if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
	}
}

//******************************************************************************
CListBoxWidget::SW_CLICK_RESULT CListBoxWidget::ClickAtCoords(
//Updates list box in response to a mouse click at specified coords.
//
//Params:
	const int nX, const int nY)   //(in) Click coords.
//
//Returns:
//An SWCR_* or LBCR_* constant indicating what happened.
{
	this->bClickedSelection = false;

	//Check for click on position button.
	if (IS_IN_RECT(nX, nY, this->VPosRect))
	{
		this->wPosClickTopLineNo = this->wTopLineNo;
		this->nPosClickY = nY;
		return SWCR_VPosButton;
	}

	//Check for click inside scroll area, but not on position button since I just
	//checked that.
	if (IS_IN_RECT(nX, nY, this->VScrollRect))
	{
		if (nY < this->VPosRect.y)
		{
			ScrollUpOnePage();
			return SWCR_PageUp;
		}
		ScrollDownOnePage();
		return SWCR_PageDown;
	}

	//Check for click inside scroll bar up button.
	if (IS_IN_RECT(nX, nY, this->UpRect))
	{
		ScrollUpOneLine();
		return SWCR_UpButton;
	}

	//Check for click inside scroll bar down button.
	if (IS_IN_RECT(nX, nY, this->DownRect))
	{
		ScrollDownOneLine();
		return SWCR_DownButton;
	}

	//Check for click inside list items.
	if (IS_IN_RECT(nX, nY, this->ItemsRect))
	{
		//One of the list items was clicked.
		if (GetItemCount() > 0)
			this->bClickedSelection = true;

		this->wCursorLine = this->wTopLineNo +
				((nY - this->ItemsRect.y) / CY_LBOX_ITEM);
		if (this->wCursorLine >= this->filteredItems.size())
			this->wCursorLine = this->filteredItems.size() - 1;

		const SDL_Keymod mod = SDL_GetModState();
		const bool bRangeSelect = (mod & KMOD_SHIFT) != 0;
		const bool bMulti = bRangeSelect || (mod & KMOD_CTRL) != 0;
		if (this->bMultipleSelection && bRangeSelect)
		{
			//Select all list items in range.
			CIDSet oldSelection = this->selectedLines;
			if (oldSelection.empty())
				this->wRangeStartLine = this->wCursorLine;
			DeselectAll();
			const UINT wMin = this->wRangeStartLine < this->wCursorLine ? this->wRangeStartLine : this->wCursorLine;
			const UINT wMax = this->wRangeStartLine > this->wCursorLine ? this->wRangeStartLine : this->wCursorLine;
			for (UINT wI=wMin; wI<=wMax; ++wI)
				SelectLine(wI, true);
			if (this->selectedLines != oldSelection)
				return LBCR_NewSelection;
		}
		else if (this->bMultipleSelection && bMulti && this->selectedLines.has(this->wCursorLine))
		{
			//Shift-clicks toggle a selection.
			DeselectLine(this->wCursorLine);
			return LBCR_NewSelection;
		} else {
			if (SelectLine(this->wCursorLine, bMulti))
				return LBCR_NewSelection;
		}
	}

	//Click did not cause an update of the widget.
	return SWCR_Nothing;
}

//*****************************************************************************
void CListBoxWidget::CalcAreas()
//Calculate coords and dimensions of areas within list box.
{
	this->wDisplayLineCount = (this->h - (CY_INSET_BORDER * 2)) / CY_LBOX_ITEM;
	if (this->wstrActiveFilter.size() > 0)
		--this->wDisplayLineCount; // We need to keep the last row empty to display the filter

	const UINT wDisplayH = this->wDisplayLineCount * CY_LBOX_ITEM; //skip any remainder space that's less then one line
	const UINT wContentH = this->filteredItems.size() * CY_LBOX_ITEM;
	UINT wViewTopY = this->wTopLineNo * CY_LBOX_ITEM;
	CalcAreas_VerticalOnly(wContentH, wDisplayH, wViewTopY);
	this->wTopLineNo = wViewTopY / CY_LBOX_ITEM;
}

//******************************************************************************
void CListBoxWidget::DragVertPosButton(
//Updates position button and list box position based on Y coordinate from mouse
//dragging after a click on the position button.
//
//Param:
	const int nY)  //(in)   Vertical mouse coord.
{
	//Shouldn't be showing the vertical scroll bar if everything can fit in the view.
	ASSERT(this->Items.size() > this->wDisplayLineCount);

	//Figure difference in lines from click coord to current coord.
	const int nMoveY = nY - this->nPosClickY;
	const double dblLinesToPixel = (double) this->Items.size() / (double) this->VScrollRect.h;
	const int nMoveLines = (int)((double)(nMoveY) * dblLinesToPixel);
	if (nMoveLines == 0) return;

	//Move the top line to new location.
	if ((int) (this->wPosClickTopLineNo + nMoveLines + this->wDisplayLineCount) >
			(int)(this->Items.size()))
		this->wTopLineNo = this->Items.size() - this->wDisplayLineCount;
	else if ((int) this->wPosClickTopLineNo + nMoveLines < 0)
		this->wTopLineNo = 0;
	else
		this->wTopLineNo = this->wPosClickTopLineNo + nMoveLines;

	//Recalc areas of widget since they may have changed.
	CalcAreas();
}

//******************************************************************************
void CListBoxWidget::ScrollDownOneLine(const UINT wLines)   //[default=1]
//Scroll list box down X lines.
{
	for (UINT wI=wLines; wI--; )
	{
		if (this->wTopLineNo + this->wDisplayLineCount >= this->Items.size())
			break;
		++this->wTopLineNo;
	}

	//Recalc areas of widget since they may have changed.
	CalcAreas();
}

//******************************************************************************
void CListBoxWidget::ScrollDownOnePage()
//Scroll list box down one page.
{
	if (this->wTopLineNo + (this->wDisplayLineCount * 2) < this->Items.size())
		this->wTopLineNo += this->wDisplayLineCount;
	else
		this->wTopLineNo = this->Items.size() - this->wDisplayLineCount;
	CalcAreas();
}

//******************************************************************************
void CListBoxWidget::ScrollUpOneLine(const UINT wLines)   //[default=1]
//Scroll list box up X lines.
{
	for (UINT wI=wLines; wI--; )
	{
		if (this->wTopLineNo == 0)
			break;
		--this->wTopLineNo;
	}

	//Recalc areas of widget since they may have changed.
	CalcAreas();
}

//******************************************************************************
void CListBoxWidget::ScrollUpOnePage()
//Scroll list box up one page.
{
	if (this->wTopLineNo >= this->wDisplayLineCount)
		this->wTopLineNo -= this->wDisplayLineCount;
	else
		this->wTopLineNo = 0;
	CalcAreas();
}

//******************************************************************************
void CListBoxWidget::Unselect(const bool bPaint)
// Clear the filter when focus is lost
{
	UpdateFilter(WSTRING());
	CFocusWidget::Unselect(bPaint);
}

//******************************************************************************
bool CListBoxWidget::IsValidFilterCharacter(const WCHAR wc)
{
	return (wc >= 'a' && wc <= 'z')
		|| (wc >= 'A' && wc <= 'Z')
		|| (wc >= '0' && wc <= '9')
		|| (wc == ' ');
}
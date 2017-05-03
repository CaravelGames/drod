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

#include "ObjectMenuWidget.h"
#include "BitmapManager.h"
#include "EventHandlerWidget.h"
#include "Sound.h"

const UINT CX_FOCUS_BUFFER = 2 + 2;
const UINT CY_FOCUS_BUFFER = 2 + 2;

//
//Public methods.
//

//******************************************************************************
MENUOBJECT::MENUOBJECT(const UINT wObjectNo, const UINT wXSize, const UINT wYSize,
							  const UINT* pTiles, const float fR, const float fG, const float fB)
	: wObjectNo(wObjectNo), wXSize(wXSize), wYSize(wYSize), paTiles(NULL)
	, fR(fR), fG(fG), fB(fB)
{
	ASSERT(pTiles);
	this->paTiles = new UINT[wXSize * wYSize];
	memcpy(this->paTiles, pTiles, wXSize * wYSize * sizeof(UINT));
}

MENUOBJECT::~MENUOBJECT()
{
	delete[] this->paTiles;
}

//******************************************************************************
CObjectMenuWidget::CObjectMenuWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget 
	const int nSetX, const int nSetY,      //    constructor.
	const UINT wSetW, const UINT wSetH,    //
	const UINT wGapX, const UINT wGapY,    //(in)   Size of gaps between objects.
	const Uint32 BGColor)						//(in)  assumed uniform background color
	: CFocusWidget(WT_ObjectMenu, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, wGapX(wGapX)
	, wGapY(wGapY)
	, bSelectionDrawn(false)
	, BGColor(BGColor)
	, bDrawBackground(false)
{
	ASSERT(wSetW >= (UINT)CBitmapManager::CX_TILE);
	ASSERT(wSetH >= (UINT)CBitmapManager::CY_TILE);
	ASSERT(wGapX < wSetW/2);
	ASSERT(wGapY < wSetH/2);
	this->iterSelectedObject = this->lObjects.end();   //not a valid object
}

//******************************************************************************
CObjectMenuWidget::~CObjectMenuWidget()
//Destructor.
{
	ClearObjects();
}

//******************************************************************************
void CObjectMenuWidget::ClearObjects()
//Clear all menu objects.
{
	for (list<MENUOBJECT *>::iterator i = this->lObjects.begin(); i != this->lObjects.end(); ++i)
		delete *i;
	this->lObjects.clear();
	this->iterSelectedObject = this->lObjects.end();   //not a valid object
}

//******************************************************************************
void CObjectMenuWidget::AddObject(
//Adds a widget to menu corresponding to certain tag
//as well as to list of children.
//
//Params:
	const UINT wObjectNo,   //(in) object id
	const UINT wXSize, const UINT wYSize,  //(in) # tiles to show for display
	const UINT* paTiles,    //(in) literal tiles to draw
	const float fR, const float fG, const float fB) //(in) additive light
{
	ASSERT(wXSize > 0);
	ASSERT(wYSize > 0);
	ASSERT(wXSize*CBitmapManager::CX_TILE < this->w - CX_FOCUS_BUFFER*2);
	ASSERT(wYSize*CBitmapManager::CY_TILE < this->h - CY_FOCUS_BUFFER*2);
	ASSERT(paTiles);

	MENUOBJECT *pObject = new MENUOBJECT(wObjectNo, wXSize, wYSize, paTiles,
			1.0f + fR, 1.0f + fG, 1.0f + fB); //add one so math works correctly

	//Find next available spot to put object.
	int nX, nY;

	if (this->lObjects.empty())
	{
		//First object.  Place at top.
		pObject->nX = CX_FOCUS_BUFFER;
		pObject->nY = CY_FOCUS_BUFFER;
	} else {
		//Put after last object.
		MENUOBJECT *pLastObject = this->lObjects.back();
		nX = pLastObject->nX + pLastObject->wXSize*CBitmapManager::CX_TILE + this->wGapX;
		if (nX + wXSize*CBitmapManager::CX_TILE <= this->w - CX_FOCUS_BUFFER)
		{
			//Object will fit on current row.
			pObject->nX = nX;
			pObject->nY = pLastObject->nY;
		} else {
			//Object must be placed at beginning of next row.
			pObject->nX = CX_FOCUS_BUFFER;
			//Scan bottom row for lowest point and place below that.
			OBJECT_RITERATOR pTrav = this->lObjects.rbegin();
			//Calculate bottom of last object.
			int nMinX=nX, nMaxY = (*pTrav)->nY + (*pTrav)->wYSize*CBitmapManager::CY_TILE;
			while (pTrav != this->lObjects.rend())
			{
				//Have we finished the row we started on?
				if ((*pTrav)->nX >= nMinX)
					break;
				else
					nMinX = (*pTrav)->nX;
				//Calculate bottom of object.
				nY = (*pTrav)->nY + (*pTrav)->wYSize*CBitmapManager::CY_TILE; 
				if (nY > nMaxY)
					nMaxY = nY;
				++pTrav;
			}

			pObject->nY = nMaxY + this->wGapY;
			ASSERT(pObject->nY + wYSize*CBitmapManager::CY_TILE <= this->h);
		}
	}

	this->lObjects.push_back(pObject);

	//If this was the first object added, select it now.
	if (this->iterSelectedObject == this->lObjects.end())
	{
		SetSelectedObject(this->lObjects.begin());
		this->iterPrevSelectedObject = this->lObjects.begin();
	}
}

//******************************************************************************
void CObjectMenuWidget::ChangeObject(const UINT wOldObjectNo, const UINT wNewObjectNo)
//Change the menu object with the old type to the new specified type.
{
	OBJECT_ITERATOR iSeek;
	for (iSeek = this->lObjects.begin(); iSeek != this->lObjects.end(); ++iSeek)
	{
		MENUOBJECT *pObject = *iSeek;
		if (pObject->wObjectNo == wOldObjectNo)
			pObject->wObjectNo = wNewObjectNo;
	}
}

//******************************************************************************
UINT CObjectMenuWidget::GetObjectIDAt(const UINT x, const UINT y) const
//Returns: object number of menu object at (x,y) or NO_SELECTED_OBJECT if none.
{
	OBJECT_ITERATOR iSeek = GetObjectAt(x, y);
	if (iSeek != this->lObjects.end())
		return (*iSeek)->wObjectNo;
	return NO_SELECTED_OBJECT;
}

//******************************************************************************
OBJECT_ITERATOR CObjectMenuWidget::GetObjectAt(const UINT x, const UINT y) const
//Returns: iterator to menu object at (x,y), else this->lObjects.end() if none.
{
	MENUOBJECT *pObject;
	UINT wX, wY;
	OBJECT_ITERATOR iSeek;
	for (iSeek = this->lObjects.begin(); 
			iSeek != this->lObjects.end(); ++iSeek)
	{
		pObject = (*iSeek);
		wX = this->x + pObject->nX;
		wY = this->y + pObject->nY;
		//Check whether (x,y) is in bounds of this object.
		if (x >= wX && y >= wY &&
				x < wX + pObject->wXSize*CBitmapManager::CX_TILE &&
				y < wY + pObject->wYSize*CBitmapManager::CY_TILE)
			return iSeek;
	}
	return iSeek;  //end
}

//******************************************************************************
UINT CObjectMenuWidget::GetSelectedObject()
//Returns the ID of the currently selected object.
{
	return this->iterSelectedObject == this->lObjects.end() ?
		NO_SELECTED_OBJECT :
		(*this->iterSelectedObject)->wObjectNo;
}

//******************************************************************************
void CObjectMenuWidget::SetSelectedObject(
//Sets the selected object on the menu.  Does nothing if object is not on the menu.
//
//Params:
	const UINT wObject) //(in) an object
{
	OBJECT_ITERATOR iSeek;
	for (iSeek = this->lObjects.begin(); iSeek != this->lObjects.end(); ++iSeek)
	{
		MENUOBJECT *pObject = *iSeek;
		if (pObject->wObjectNo == wObject)
		{
			this->iterSelectedObject = iSeek;
			return;
		}
	}
}

//******************************************************************************
void CObjectMenuWidget::Paint(
//Paint widget area.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	//Drawing code below needs to be modified to accept offsets.  Until then,
	//this widget can't be offset.
	ASSERT(!IsScrollOffset());

	SDL_Surface *pSurface = GetDestSurface();
	MENUOBJECT *pObject;

	if (this->bDrawBackground)
	{
		//Fill menu surface and outline it.
		SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y, this->w, this->h);
		SDL_FillRect(pSurface,&rect,this->BGColor);

		SURFACECOLOR Black;
		GetDestSurfaceColor(0, 0, 0, Black);
		DrawRect(rect,Black,pSurface);
	}
	else if (this->bSelectionDrawn)
	{
		//Erase selection at previous location.
		pObject = (*this->iterPrevSelectedObject);
		const UINT DRAWX = this->x + pObject->nX - 2;
		const UINT DRAWY = this->y + pObject->nY - 2;
		const UINT CX_FOCUS = pObject->wXSize * CBitmapManager::CX_TILE + 4;
		const UINT CY_FOCUS = pObject->wYSize * CBitmapManager::CY_TILE + 4;
		SDL_Rect SelectionScreenRect = MAKE_SDL_RECT(DRAWX, DRAWY, CX_FOCUS, CY_FOCUS);
		//Erase the highlighting rectangle.
		SDL_FillRect(pSurface,&SelectionScreenRect,this->BGColor);
	}

	//Draw objects in menu.
	UINT wX, wY, wIndex, wTile;
	for (OBJECT_ITERATOR iSeek = this->lObjects.begin(); 
			iSeek != this->lObjects.end(); ++iSeek)
	{
		pObject = (*iSeek);
		wIndex = 0;
		for (wY=0; wY<pObject->wYSize; ++wY)
			for (wX=0; wX<pObject->wXSize; ++wX)
			{
				wTile = pObject->paTiles[wIndex++];
				if (wTile != TI_UNSPECIFIED)
				{
					const UINT wXPos = this->x + pObject->nX + wX*CBitmapManager::CX_TILE;
					const UINT wYPos = this->y + pObject->nY + wY*CBitmapManager::CY_TILE;
					g_pTheBM->BlitTileImage(wTile, wXPos, wYPos, pSurface, false, 255);
					g_pTheBM->LightenRectWithTileMask(pSurface, wXPos, wYPos,
							CBitmapManager::CX_TILE, CBitmapManager::CY_TILE,
							pObject->fR, pObject->fG, pObject->fB, wTile, 0, 0);
				}
			}
	}

	//Highlight selected object.
	if (this->iterSelectedObject != this->lObjects.end())
	{
		pObject = (*this->iterSelectedObject);
		const UINT DRAWX = this->x + pObject->nX - 2;
		const UINT DRAWY = this->y + pObject->nY - 2;
		const UINT CX_FOCUS = pObject->wXSize * CBitmapManager::CX_TILE + 4;
		const UINT CY_FOCUS = pObject->wYSize * CBitmapManager::CY_TILE + 4;
		SDL_Rect SelectionScreenRect = MAKE_SDL_RECT(DRAWX, DRAWY, CX_FOCUS, CY_FOCUS);

		//Save spot where selection box is drawn.
		this->bSelectionDrawn = true;
		this->iterPrevSelectedObject = this->iterSelectedObject;

		//Draw box on edge of selected object, with or without focus.
		//(Drawing outside of object would require saving a separate surface
		//to erase the box, and we don't really need to do that.)
		const SURFACECOLOR SelectedObjectHighlight = (IsSelected() ?
				GetSurfaceColor(pSurface, 0, 0, 0) :
				GetSurfaceColor(pSurface, 64, 64, 64));
		DrawRect(SelectionScreenRect,SelectedObjectHighlight,pSurface);
	}

	//Draw children in active menu.
	PaintChildren();
	
	if (bUpdateRect) UpdateRect();
}

//******************************************************************************
void CObjectMenuWidget::PopUp()
//All-in-one method to show the widget, paint it, activate it, and hide it 
//again.  Caller should probably repaint parent widget of dialog to 
//erase the dialog.
{
	CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
	if (!pEventHandler) return;

	Show();
	ASSERT(IsLoaded());  //dialog must be loaded before painted
	pEventHandler->Paint();

	SDL_Event event;
	bool bDeactivate = false; //A callee will call Deactivate() which sets this.
	while (!bDeactivate && !pEventHandler->IsDeactivating())
	{
		//Get any events waiting in the queue.
		while (!bDeactivate && !pEventHandler->IsDeactivating() && PollEvent(&event)) 
		{
			switch (event.type)
			{
				case SDL_MOUSEBUTTONDOWN:
				{
					const SDL_MouseButtonEvent& button = event.button;

					//Deactivate when clicking out of bounds.
					if ((button.x < this->x) || (this->x + this->w <= (UINT)button.x) ||
							(button.y < this->y) || (this->y + this->h <= (UINT)button.y))
					{
						bDeactivate = true;
						break;
					}

					//Select menu item.
					OBJECT_ITERATOR iSeek = GetObjectAt(button.x, button.y);
					if (iSeek != this->lObjects.end())
					{
						SetSelectedObject(iSeek);
						bDeactivate = true;
					}  //else no object was clicked on.  Do nothing.
				}
				break;

				//Ignore these events.
				case SDL_MOUSEBUTTONUP:
				break;

				//Close widget on these events.
				case SDL_KEYDOWN:
				case SDL_KEYUP:
					bDeactivate = true;
				break;

				//Pass these event types through.
				case SDL_MOUSEMOTION:
				case SDL_WINDOWEVENT:
					pEventHandler->HandleEvent(event);
				break;

				//Pass these events through and close the widget.
				default:
					pEventHandler->HandleEvent(event);
					bDeactivate = true;
				break;
			}
		}  //...while I have events.
	}

	Hide();
}

//******************************************************************************
void CObjectMenuWidget::SetObjectTiles(
//Sets the tiles for the existing menu widget corresponding to a certain tag.
//
//Params:
	const UINT wObjectNo,   //(in) object id
	const UINT wXSize, const UINT wYSize,  //(in) # tiles to show for display
	const UINT* pTiles)    //(in) literal tiles to draw
{
	OBJECT_ITERATOR iSeek;
	for (iSeek = this->lObjects.begin(); iSeek != this->lObjects.end(); ++iSeek)
	{
		MENUOBJECT *pObject = *iSeek;
		if (pObject->wObjectNo == wObjectNo)
		{
			//Found object.  Update its tiles.
			//!!this won't reposition all items in the menu if the object's size changes
			pObject->wXSize = wXSize;
			pObject->wYSize = wYSize;
			ASSERT(pTiles);
			delete[] pObject->paTiles;
			pObject->paTiles = new UINT[wXSize * wYSize];
			memcpy(pObject->paTiles, pTiles, wXSize * wYSize * sizeof(UINT));
			break;
		}
	}
}

//******************************************************************************
void CObjectMenuWidget::SetObjectLight(
//Sets the light added to the object.
//
//Params:
	const UINT wObjectNo,
	const float fR, const float fG, const float fB)
{
	OBJECT_ITERATOR iSeek;
	for (iSeek = this->lObjects.begin(); iSeek != this->lObjects.end(); ++iSeek)
	{
		MENUOBJECT *pObject = *iSeek;
		if (pObject->wObjectNo == wObjectNo)
		{
			//Found object.  Update its lighting.
			pObject->fR = 1.0f + fR; //add one so math works correctly
			pObject->fG = 1.0f + fG;
			pObject->fB = 1.0f + fB;
			break;
		}
	}
}

//
//Protected methods.
//

//******************************************************************************
void CObjectMenuWidget::HandleKeyDown(
//Processes a keyboard event within the scope of the widget.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
//
//Returns:
//True.
{
	const SDL_Keycode key = KeyboardEvent.keysym.sym;

	if (this->iterSelectedObject == this->lObjects.end()) return;

	if ((KeyboardEvent.keysym.mod & KMOD_CTRL)) return; //Ctrl used for hotkeys elsewhere

	switch (key) {
		//Move between objects.
		case SDLK_UP:
		case SDLK_KP_8:

		case SDLK_LEFT:
		case SDLK_KP_4:
			if (this->iterSelectedObject != this->lObjects.begin())
			{
				SetSelectedObject(--this->iterSelectedObject);
				RequestPaint();
			}
		break;

		case SDLK_DOWN:
		case SDLK_KP_2:

		case SDLK_RIGHT:
		case SDLK_KP_6:
			if (&(*this->iterSelectedObject) != &this->lObjects.back())
			{
				SetSelectedObject(++this->iterSelectedObject);
				RequestPaint();
			}
		break;

		default: break;
	}
}

//******************************************************************************
void CObjectMenuWidget::HandleMouseDown(
//Handles a mouse down event.
//All this should do is select a tag.
//
//Params:
	const SDL_MouseButtonEvent &Button) //(in)   Event to handle.
{
	OBJECT_ITERATOR iSeek = GetObjectAt(Button.x, Button.y);
	if (iSeek != this->lObjects.end())
	{
		SetSelectedObject(iSeek);
		RequestPaint();
	}  //else no object was clicked on.  Do nothing.
}

//
//Private methods.
//

//******************************************************************************
void CObjectMenuWidget::SetSelectedObject(
//Sets the selected object on the menu.  Updates the view.
//
//Params:
	OBJECT_ITERATOR const &newSelectedObject) //(in) an object in this->lObjects
{
	ASSERT(*newSelectedObject);
	this->iterSelectedObject = newSelectedObject;

	//Call OnSelectChange() notifier.
	CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
	if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());
}

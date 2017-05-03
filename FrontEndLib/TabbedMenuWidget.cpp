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

#include "TabbedMenuWidget.h"
#include "BitmapManager.h"
#include "EventHandlerWidget.h"
#include "FontManager.h"
#include "Sound.h"
#include <BackEndLib/Exception.h>

const SURFACECOLOR BlackLine = {0, 0, 0};

#define NO_TAB_TILE (UINT(-1))

#define CX_SPACE (10)

//
//Public methods.
//

//******************************************************************************
CTabbedMenuWidget::CTabbedMenuWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget 
	const int nSetX, const int nSetY,     //    constructor.
	const UINT wSetW, const UINT wSetH,   //
	const UINT wTabs,                     //(in) Number of tabs.
	const UINT wTabsHeight,               //(in) Height of tabs (in pixels)
	const BYTE R, const BYTE G, const BYTE B)	//(in)	Background color RGB
	: CFocusWidget(WT_TabbedMenu, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
	, wNumTabs(wTabs)
	, wSelectedTab(0)
	, tabHeight(wTabsHeight)
	, BGimageOpacity(255)
{
	ASSERT(this->h >= this->tabHeight);

	this->BGColor.byt1 = R;
	this->BGColor.byt2 = G;
	this->BGColor.byt3 = B;
	ASSERT(wTabs > 0);

	//Allocate menu tab info.
	this->tabWidths.resize(wTabs, wSetW / wTabs);
	this->tabTileNo.resize(wTabs, NO_TAB_TILE);
	this->tabText.resize(wTabs);
	this->plChildWidgetsForTab = new std::list<CWidget *>[wTabs];
	if (!this->plChildWidgetsForTab) throw CException();
}

//******************************************************************************
CTabbedMenuWidget::~CTabbedMenuWidget()
//Destructor.
{
	delete[] this->plChildWidgetsForTab;
}

//******************************************************************************
void CTabbedMenuWidget::SetBGImage(
//Sets a background image to display on top of the menu background color
//at the indicated opacity.  Use an empty filename to reset to a blank image.
//
//Params:
	const std::string& filename, const Uint8 opacity) //[default=255]
{
	const bool bAlreadyLoaded = IsLoaded();

	//Unload previous resources.
	if (bAlreadyLoaded)
	{
		//Unload(), except to not unload child widget resources.
		for (UINT wIndex=0; wIndex<this->imageFilenames.size(); ++wIndex)
		{
			if (this->images[wIndex])
				g_pTheBM->ReleaseBitmapSurface(this->imageFilenames[wIndex].c_str());
		}
		this->images.clear();

		this->bIsLoaded = false;
	}
	this->imageFilenames.clear();

	this->BGimageOpacity = opacity;

	//Load new resource.
	if (!filename.empty())
	{
		this->imageFilenames.push_back(filename);
		if (bAlreadyLoaded)
		{
			//Load(), except to not reload child resources.
			for (UINT wIndex=0; wIndex<this->imageFilenames.size(); ++wIndex)
			{
				//Load a graphics bitmap.
				ASSERT(this->images.size() <= wIndex || !this->images[wIndex]);
				SDL_Surface *pSurface = g_pTheBM->GetBitmapSurface(
						this->imageFilenames[wIndex].c_str());
				if (pSurface)
					this->images.push_back(pSurface);
			}
		}
	}
}

//******************************************************************************
void CTabbedMenuWidget::SetTabTile(
//Sets a tile to be displayed on the specified tab.
//
//Params:
	const UINT wTab, const UINT wTileNo)
{
	ASSERT(wTab < this->wNumTabs);
	this->tabTileNo[wTab] = wTileNo;
	this->tabText[wTab].resize(0);
}

//******************************************************************************
void CTabbedMenuWidget::SetTabText(
//Sets text to be displayed on the specified tab.
//
//Params:
	const UINT wTab, const WCHAR* pText)
{
	ASSERT(wTab < this->wNumTabs);
	this->tabText[wTab] = pText;
	this->tabTileNo[wTab] = NO_TAB_TILE;
}

//******************************************************************************
CWidget* CTabbedMenuWidget::AddWidgetToTab(
//Adds a widget to menu corresponding to certain tag
//as well as to list of children.
//
//Params:
	CWidget *pNewWidget,
	const UINT wTab,  //(in) to which tab
	bool bLoad) //(in) default = false
{
	ASSERT(wTab < this->wNumTabs);
	this->plChildWidgetsForTab[wTab].push_back(pNewWidget);
	pNewWidget->Show(wTab == this->wSelectedTab);

	return CWidget::AddWidget(pNewWidget,bLoad);
}

//******************************************************************************
void CTabbedMenuWidget::Paint(
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

	SDL_Surface *pDestSurface = GetDestSurface();

	//Draw menu area.
	//Fill menu surface.
	SDL_Rect rect = MAKE_SDL_RECT(this->x, this->y + this->tabHeight, this->w,
			this->h - this->tabHeight);
	const Uint32 BColor = GetBGColorValue();
	SDL_FillRect(pDestSurface,&rect,BColor);

	PaintBGImage(rect, pDestSurface);

	//Outline widget.
	DrawRect(rect,BlackLine,pDestSurface);

	//Draw tabs.
	DrawTabs();

	//Draw children in active menu.
	PaintChildren();
	
	if (bUpdateRect) UpdateRect();
}

Uint32 CTabbedMenuWidget::GetBGColorValue() const
{
#ifdef GAME_RENDERING_OFFSET
	return this->BGColor.byt1 << 8 | this->BGColor.byt2 << 16 | this->BGColor.byt3 << 24;
#else
	return this->BGColor.byt1 << 16 | this->BGColor.byt2 << 8 | this->BGColor.byt3;
#endif
}
//******************************************************************************
void CTabbedMenuWidget::PaintBGImage(SDL_Rect& rect, SDL_Surface* pDestSurface)
//Paint optional background image on the uniform color background.
{
	if (!this->images.empty() && this->BGimageOpacity)
	{
		SDL_Rect dest = rect;
		EnableSurfaceBlending(this->images[0], this->BGimageOpacity);
		SDL_BlitSurface(this->images[0], &rect, pDestSurface, &dest);
		DisableSurfaceBlending(this->images[0]);
	}
}

//******************************************************************************
void CTabbedMenuWidget::SelectTab(
//Select a tab and make widgets associated with it visible.
//
//Params:
	const UINT wTab)
{
	ASSERT(wTab < this->wNumTabs);

	if (this->wSelectedTab == wTab)
		return;

	g_pTheSound->PlaySoundEffect(SOUNDLIB::SEID_READ);

	//Make widgets associated with old tab invisible.
	WIDGET_ITERATOR iSeek;
	for (iSeek = this->plChildWidgetsForTab[this->wSelectedTab].begin(); 
			iSeek != this->plChildWidgetsForTab[this->wSelectedTab].end(); ++iSeek)
	{
		(*iSeek)->Hide();
	}

	this->wSelectedTab = wTab;

	//Make widgets associated with new tab visible.
	for (iSeek = this->plChildWidgetsForTab[this->wSelectedTab].begin();
			iSeek != this->plChildWidgetsForTab[this->wSelectedTab].end(); ++iSeek)
	{
		(*iSeek)->Show();
	}

	//Call OnSelectChange() notifier.
	CEventHandlerWidget *pEventHandler = GetEventHandlerWidget();
	if (pEventHandler) pEventHandler->OnSelectChange(GetTagNo());

	RequestPaint();
}

//******************************************************************************
void CTabbedMenuWidget::HandleKeyDown(
//Processes a keyboard event within the scope of the widget.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
//
//Returns:
//True.
{
	const SDL_Keycode key = KeyboardEvent.keysym.sym;

	switch (key) {
		//Move between tabs.
		case SDLK_LEFT:
		case SDLK_KP_4:
		{
			if (this->wSelectedTab > 0)
				SelectTab(this->wSelectedTab-1);
		}
		return;
		case SDLK_RIGHT:
		case SDLK_KP_6:
		{
			if (this->wSelectedTab < this->wNumTabs - 1)
				SelectTab(this->wSelectedTab+1);
		}
		return;

		default: break;
	}
}

//
//Protected methods.
//

//******************************************************************************
void CTabbedMenuWidget::HandleMouseDown(
//Handles a mouse down event.
//All this should do is select a tag.
//
//Params:
	const SDL_MouseButtonEvent &Button) //(in)   Event to handle.
{
	//Select a tab.
	if ((UINT)Button.y <= this->y + this->tabHeight)
	{
		const int xClickPos = Button.x - this->x;
		int xPos = 0;
		for (UINT wTab=0; wTab<this->wNumTabs; ++wTab)
		{
			const int width = this->tabWidths[wTab];
			if (xClickPos < xPos + width)
			{
				//Click occurred inside its region -- select tab.
				SelectTab(wTab);
				break;
			}
			xPos += width;
		}
	}
}

//
//Private methods.
//

//******************************************************************************
void CTabbedMenuWidget::DrawTabs()
//Draw tabs on top.
{
	static const UINT BORDER = 2;

	//Tile position on tab.
	const UINT wTabHeight = this->tabHeight;
	const UINT wYTile = (wTabHeight - CBitmapManager::CY_TILE) / 2;

	SDL_Surface *pDestSurface = GetDestSurface();
	const SURFACECOLOR SBGColor = GetSurfaceColor(pDestSurface,
			this->BGColor.byt1, this->BGColor.byt2, this->BGColor.byt3);
	const Uint32 BColor = GetBGColorValue();
	const SURFACECOLOR FocusColor = GetSurfaceColor(pDestSurface, RGB_FOCUS);

	//Draw with or without focus.
	const UINT wFocusedTab = this->IsSelected() ? this->wSelectedTab : UINT(-1);

	//Make BG anti-aliasing smooth.
	static const UINT eFontType = WT_Button;
	const SDL_Color TextBackColor = {this->BGColor.byt1, this->BGColor.byt2, this->BGColor.byt3, 0};
	const SDL_Color origTextBGColor = g_pTheFM->GetFontBackColor(eFontType);
	g_pTheFM->SetFontBackColor(eFontType, TextBackColor);

	int xPos = this->x;
	for (UINT wTab=0; wTab<this->wNumTabs; ++wTab)
	{
		//Fill tab surface.
		const UINT width = this->tabWidths[wTab];
		SDL_Rect rect = MAKE_SDL_RECT(xPos, this->y, width, wTabHeight);
		SDL_FillRect(pDestSurface,&rect,BColor);

		PaintBGImage(rect, pDestSurface);

		//Draw edge of tab.
		DrawRect(rect,BlackLine,pDestSurface);

		//Show selected tab.
		if (wTab == this->wSelectedTab)
		{
			//White out selected tab lines.
			DrawRow(xPos+1, this->y+wTabHeight, width-2, SBGColor, pDestSurface);
			DrawRow(xPos+1, this->y+wTabHeight-1, width-2, SBGColor, pDestSurface);

			SDL_Rect rect = MAKE_SDL_RECT(xPos+1, this->y+wTabHeight-1, width-2, 2);
			PaintBGImage(rect, pDestSurface);
		}

		//Draw tile or text.
		if (this->tabTileNo[wTab] != NO_TAB_TILE)
		{
			ASSERT(width >= CBitmapManager::CX_TILE);
			const UINT wXTile = (width - CBitmapManager::CX_TILE) / 2;
			g_pTheBM->BlitTileImage(this->tabTileNo[wTab],
					xPos + wXTile, this->y + wYTile, pDestSurface);

			//Draw focus border.
			if (wFocusedTab == wTab)
			{
				//Draw box around tile.
				const SDL_Rect rect = MAKE_SDL_RECT(xPos + wXTile - BORDER, this->y + wYTile - BORDER,
						CBitmapManager::CX_TILE + BORDER*2,
						CBitmapManager::CY_TILE + BORDER*2);
				DrawRect(rect,FocusColor, pDestSurface);
			}
		} else {
			g_pTheFM->DrawTextToRect(eFontType, this->tabText[wTab].c_str(),
				xPos + CX_SPACE, this->y, width, wTabHeight, pDestSurface);

			//Draw focus border.
			if (wFocusedTab == wTab)
			{
				//Draw box inside tab.
				const SDL_Rect rect = MAKE_SDL_RECT(xPos + BORDER, this->y + BORDER,
						width - BORDER * 2, wTabHeight - BORDER * 2);
				DrawRect(rect,FocusColor, pDestSurface);
			}
		}

		xPos += width;
	}

	g_pTheFM->SetFontBackColor(eFontType, origTextBGColor);
}

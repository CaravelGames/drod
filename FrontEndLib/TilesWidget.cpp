// $Id: TilesWidget.cpp 8019 2007-07-14 22:30:11Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2008
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//A widget for displaying a set of tiles in various locations.

#include "TilesWidget.h"

#include "BitmapManager.h"
#include <BackEndLib/Assert.h>

//
//Public methods.
//

//*****************************************************************************
CTilesWidget::CTilesWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget constructor.
	const int nSetX, const int nSetY, const UINT wSetW, const UINT wSetH)
	: CWidget(WT_Tiles, dwSetTagNo, nSetX, nSetY, wSetW, wSetH)
{
}

//*****************************************************************************
CTilesWidget::~CTilesWidget()
{
}

//*****************************************************************************
void CTilesWidget::AddTile(
//Add a new tile to the display.
//
//Params:
	const UINT tileNo, const int x, const int y,
	const std::array<float, 3> hsv,
	const float r, const float g, const float b) //[default=(1,1,1)]
{
	if (tileNo < g_pTheBM->GetNextCustomTileNo()) //if tile can be displayed
		this->tiles.push_back(TILEDATA(tileNo, x, y, hsv, r, g, b));
}

//*****************************************************************************
void CTilesWidget::ClearTiles()
//Removes all tiles from the widget display.
{
	this->tiles.clear();
}

//*****************************************************************************
void CTilesWidget::Paint(bool bUpdateRect)
{
	ASSERT(this->w > 0);
	ASSERT(this->h > 0);

	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);

	SDL_Rect dest;
	GetRect(dest);
	dest.x += nOffsetX;
	dest.y += nOffsetY;

	SDL_Surface *pDestSurface = GetDestSurface();

	for (UINT i=0; i<this->tiles.size(); ++i)
	{
		const TILEDATA& tile = this->tiles[i];

		if (dest.x >= this->x && dest.x < this->x + (int)this->w &&
				dest.y >= this->y && dest.y < this->y + (int)this->h)
		{
			const int x = dest.x + tile.x;
			const int y = dest.y + tile.y;
			g_pTheBM->BlitTileImage(tile.wTileNo, x, y, pDestSurface);
			g_pTheBM->HsvToRectWithTileMask(pDestSurface,
				x, y, g_pTheBM->CX_TILE, g_pTheBM->CY_TILE,
				tile.hsv[0], tile.hsv[1], tile.hsv[2],
				tile.wTileNo, 0, 0);
			g_pTheBM->LightenRectWithTileMask(pDestSurface, x, y,
				g_pTheBM->CX_TILE, g_pTheBM->CY_TILE,
				tile.r, tile.g, tile.b, tile.wTileNo, 0, 0);

		}
	}

	PaintChildren();

	if (bUpdateRect) UpdateRect();
}

// $Id: TilesWidget.h 8019 2007-07-14 22:30:11Z mrimer $

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

#ifndef TILESWIDGET_H
#define TILESWIDGET_H

#include "Widget.h"
#include <vector>

struct TILEDATA
{
	TILEDATA(UINT wTileNo, int x, int y, const float r, const float g, const float b)
		: wTileNo(wTileNo), x(x), y(y), r(r), g(g), b(b)
	{}
	UINT wTileNo;
	int x, y;
	float r, g, b;
};

//******************************************************************************
class CTilesWidget : public CWidget
{
public:
	CTilesWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
			const UINT wSetW, const UINT wSetH);
	~CTilesWidget();

	virtual void   Paint(bool bUpdateRect = true);

	void AddTile(const UINT tileNo, const int x, const int y,
			const float r=1.0f, const float g=1.0f, const float b=1.0f);
	void ClearTiles();

private:
	std::vector<TILEDATA> tiles;
};

#endif //#ifndef TILESWIDGET_H

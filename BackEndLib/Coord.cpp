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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996, 
 * 1997, 2000, 2001, 2002, 2012 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "Coord.h"
#include <math.h>

using std::vector;

#define ROUND(x)  (int)((x) + 0.5f)

//*****************************************************************************
vector<CCoord> CCoord::GetOrderedLineBetween(
	UINT x1, UINT y1,
	UINT x2, UINT y2)
{
	vector<CCoord> coords;
	bool reversed = false;

	//Plot in increasing x order.
	if (x2 < x1) {
		std::swap(x1, x2);
		std::swap(y1, y2);
		reversed = !reversed;
	}

	const int dx = int(x2) - int(x1);
	const int dy = int(y2) - int(y1);

	if (!dx) {
		if (y2 < y1) {
			std::swap(y1, y2);
			reversed = !reversed;
		}

		for (UINT y=y1; y<=y2; ++y)
			coords.push_back(CCoord(x1, y));
	} else if (!dy) {
		for (UINT x=x1; x<=x2; ++x)
			coords.push_back(CCoord(x, y1));
	} else {
		float m = dy / float(dx);

		if (fabs(m) <= 1.0f) {
			float y = float(y1);
			for (UINT x=x1; x<=x2; ++x) {
				coords.push_back(CCoord(x, ROUND(y)));
				y += m;
			}
		} else {
			m = dx / float(dy);

			if (y2 < y1) {
				std::swap(x1, x2);
				std::swap(y1, y2);
				reversed = !reversed;
			}

			float x = float(x1);
			for (UINT y=y1; y<=y2; ++y) {
				coords.push_back(CCoord(ROUND(x), y));
				x += m;
			}
		}
	}

	if (!reversed)
		return coords;

	vector<CCoord> unreversed_coords;
	for (vector<CCoord>::const_reverse_iterator it=coords.rbegin();
			it!=coords.rend(); ++it)
		unreversed_coords.push_back(CCoord(it->wX, it->wY));

	return unreversed_coords;
}

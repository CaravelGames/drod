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

#ifndef PENDINGPLOTEFFECT_H
#define PENDINGPLOTEFFECT_H

#include "DrodEffect.h"
#include "../DRODLib/GameConstants.h"

//*****************************************************************************
class CEditRoomWidget;
class CPendingPlotEffect : public CEffect
{
public:
	CPendingPlotEffect(CWidget *pSetWidget, const UINT wObjectNo,
			const UINT* wTileImageNo, const UINT wXSize=1, const UINT wYSize=1,
			const UINT wO=NO_ORIENTATION);

	virtual bool Draw(SDL_Surface* pDestSurface=NULL);

private:
	void PlotSwordsman(const UINT wStartX, const UINT wStartY, const UINT wEndX,
			const UINT wEndY, SDL_Surface* pDestSurface);
	void PlotTile(const UINT wX, const UINT wY, const UINT wObjectNo,
			const UINT wTileNo, SDL_Surface* pDestSurface);

	SDL_Rect OwnerRect;
	CEditRoomWidget *pRoomWidget;

	const UINT *   pwTileImageNo;
	UINT     wObjectNo, wXSize, wYSize, wO;
	static unsigned char nOpacity;
	static bool bRising;
};

#endif //...#ifndef PENDINGPLOTEFFECT_H

// $Id: SwordsmanSwirlEffect.h 8975 2008-05-01 22:47:03Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef SWORDSMANSWIRLEFFECT_H
#define SWORDSMANSWIRLEFFECT_H

#include "DrodEffect.h"
#include <BackEndLib/Coord.h>

class CCurrentGame;
class CSwordsmanSwirlEffect : public CEffect
{
public:
	CSwordsmanSwirlEffect(CWidget *pSetWidget, CCurrentGame *pCurrentGame);
	CSwordsmanSwirlEffect(CWidget *pSetWidget, CCoord coord);

	virtual bool Draw(SDL_Surface* pDestSurface=NULL);

private:
	CCurrentGame *pCurrentGame;
	UINT wOldX, wOldY;
	CCoord staticCenter;
};

#endif //...#ifndef SWORDSMANSWIRLEFFECT_H

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2004, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef EVILEYEGAZEEFFECT_H
#define EVILEYEGAZEEFFECT_H

#include <FrontEndLib/Effect.h>
#include <BackEndLib/CoordSet.h>
#include <BackEndLib/Types.h>

//****************************************************************************************
class CRoomWidget;
class CEvilEyeGazeEffect : public CEffect
{
public:
	CEvilEyeGazeEffect(CWidget *pSetWidget, const UINT wX, const UINT wY, const UINT wO,
			const Uint32 dwDuration);

	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);

	void DirtyOldBeamTiles();

	void UpdateOpacity(const Uint32& dwTimeElapsed);

	void DrawNewBeam();

protected:
	virtual void Draw(SDL_Surface& pDestSurface);

	UINT        wX, wY, wTileNo;
	int         dx, dy;
	Uint8       opacity;
	const Uint32 dwDuration;

	CRoomWidget *  pRoomWidget;
	CCoordSet   gazeTiles;
};

#endif //...#ifndef EVILEYEGAZEEFFECT_H

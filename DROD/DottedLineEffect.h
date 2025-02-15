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
 * Portions created by the Initial Developer are Copyright (C) 2025 Caravel Software.
 * All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef LINEEFFECT_H
#define LINEEFFECT_H

#include "DrodEffect.h"
#include <BackEndLib/CoordSet.h>

class CDottedLineEffect : public CEffect {
public:
	CDottedLineEffect(CWidget* pSetOwnerWidget, const UINT dwDuration, const CCoord& startTile, const CCoord& endTile);

protected:
	bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	void Draw(SDL_Surface& destSurface);

private:
	void CalculatePositions(const CCoord& startTile, const CCoord& endTile);

	CCoordSet positions;
	Uint8 nOpacity;
};

#endif
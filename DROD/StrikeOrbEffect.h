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
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef STRIKEORBEFFECT_H
#define STRIKEORBEFFECT_H

#include "DrodEffect.h"
#include "../DRODLib/DbRooms.h"
#include <BackEndLib/Types.h>
#include <FrontEndLib/Bolt.h>

#include <vector>
using std::vector;

struct BOLT
{
	int xBegin;
	int yBegin;
	int xEnd;
	int yEnd;
};

//****************************************************************************************
class CStrikeOrbEffect : public CEffect
{
public:
	CStrikeOrbEffect(CWidget *pSetWidget, const COrbData &SetOrbData, 
			SDL_Surface *pSetPartsSurface, const bool bSetDrawOrb);
	~CStrikeOrbEffect();

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);

private:
	UINT        wOrbX, wOrbY;
	OrbType     eOrbType;
	vector<BOLT> bolts;
	bool        bDrawOrb;

	Uint8 nOpacity;
	vector<BOLT_SEGMENTS*> drawBolts;

	SDL_Surface *  pPartsSurface;
};

#endif //...#ifndef STRIKEORBEFFECT_H

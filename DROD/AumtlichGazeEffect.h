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

#ifndef AUMTLICHGAZEEFFECT_H
#define AUMTLICHGAZEEFFECT_H

#include <FrontEndLib/Effect.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Coord.h>

#include <vector>

//****************************************************************************************
class CMonster;
class CRoomWidget;
class CAumtlichGazeEffect : public CEffect
{
public:
	CAumtlichGazeEffect(CWidget *pSetWidget, const CMonster *pAumtlich);

	virtual bool Draw(SDL_Surface* pDestSurface=NULL);

	CMoveCoord endCoord;

protected:
	void PrepareBeam(const CMonster *pAumtlich);

	CMoveCoord  origin;
	UINT        wValidTurn;   //game turn this gaze is valid for
	std::vector<CMoveCoord> coords;

	CRoomWidget *  pRoomWidget;

	static Uint32 dwLastDraw;  //to synchronize all aumtlich gazes
};

#endif //...#ifndef AUMTLICHGAZEEFFECT_H

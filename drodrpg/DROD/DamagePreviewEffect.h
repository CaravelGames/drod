// $Id: DamagePreviewEffect.h $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2004, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef DAMAGEPREVIEWEFFECT_H
#define DAMAGEPREVIEWEFFECT_H

#include <FrontEndLib/Effect.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Coord.h>

#include <vector>

//****************************************************************************************
class CMonster;
class CRoomWidget;
class CDamagePreviewEffect : public CEffect
{
public:
	CDamagePreviewEffect(CWidget *pSetWidget, const CMonster *pMonster);
	~CDamagePreviewEffect();

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);

	UINT        wValidTurn;   //game turn this display is valid for

	CRoomWidget *  pRoomWidget;

private:
	void PrepWidget();

	CMonster* pMonster;

	SDL_Surface* pTextSurface;
};

#endif //...#ifndef DAMAGEPREVIEWEFFECT_H
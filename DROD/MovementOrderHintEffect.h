// $Id: MovementOrderHintEffect.h $

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
 * Portions created by the Initial Developer are Copyright (C) 2025
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef MOVEMENTORDERHINTEFFECT_H
#define MOVEMENTORDERHINTEFFECT_H

#include <FrontEndLib/Effect.h>
#include <BackEndLib/Types.h>

 //****************************************************************************************
class CMonster;
class CRoomWidget;
class CMovementOrderHintEffect : public CEffect
{
public:
	CMovementOrderHintEffect(CWidget* pSetWidget, const CMonster* pMonster, int moveOrder);
	~CMovementOrderHintEffect();

	static void ClearSurfaceCache();

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
	virtual void Draw(SDL_Surface& destSurface);

	UINT        wValidTurn;   //game turn this display is valid for
	UINT        wMoveOrder;   //movement order value to display

	CRoomWidget* pRoomWidget;

private:
	void PrepWidget();

	SDL_Surface* GetSurfaceForOrder(int order);

	CMonster* pMonster;

	SDL_Surface* pTextSurface;
};

#endif //...#ifndef MOVEMENTORDERHINTEFFECT_H
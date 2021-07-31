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
 * 1997, 2000, 2001, 2002, 2005, 2020, 2021 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "TileSwirlEffect.h"
#include "DrodBitmapManager.h"

CTileSwirlEffect::CTileSwirlEffect(CWidget* pSetWidget, const CMoveCoord& SetCoord)
  : CSwirlEffect(pSetWidget, ETILESWIRL)
  , m_tilePosition(SetCoord)
{
}

bool CTileSwirlEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
  //Swirl gradually fades out.
  this->nOpacity = g_pTheBM->bAlpha ? 255 - ((dwTimeElapsed * 255) / dwDuration) : 255;

  //Center effect on tile's position.
  SDL_Rect OwnerRect;
  this->pOwnerWidget->GetRect(OwnerRect);

  const UINT wX = m_tilePosition.wX;
  const UINT wY = m_tilePosition.wY;

  const UINT wXCenter = OwnerRect.x + (wX * CBitmapManager::CX_TILE) +
    CBitmapManager::CX_TILE / 4;
  const UINT wYCenter = OwnerRect.y + (wY * CBitmapManager::CY_TILE) +
    CBitmapManager::CY_TILE / 4;

  CalculateSwirl(wXCenter, wYCenter);

  return true;
}

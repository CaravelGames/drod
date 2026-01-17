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

#include "DottedLineEffect.h"

#include "DrodBitmapManager.h"

#include <math.h>

#define SPRITE_SIZE (8) //Size in pixels of sprite used for dots

//********************************************************************************
CDottedLineEffect::CDottedLineEffect(
  CWidget* pSetOwnerWidget, const UINT dwDuration, const CCoord& startTile, const CCoord& endTile)
  : CEffect(pSetOwnerWidget, dwDuration, EffectType::EGENERIC)
{
  CalculatePositions(startTile, endTile);
}

//********************************************************************************
bool CDottedLineEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
  //Gradually fades out.
  this->nOpacity = g_pTheBM->bAlpha ? 255 - ((dwTimeElapsed * 255) / dwDuration) : 255;
  return true;
}

//********************************************************************************
void CDottedLineEffect::Draw(SDL_Surface& destSurface)
{
  for (CCoordSet::const_iterator coord = this->positions.begin(); coord != this->positions.end(); ++coord)
  {
    g_pTheBM->BlitTileImagePart(TI_SWIRLDOT_2, coord->wX, coord->wY, 0, 0, 9, 9, &destSurface, false, nOpacity);
  }
}

//********************************************************************************
void CDottedLineEffect::CalculatePositions(const CCoord& startTile, const CCoord& endTile)
{
  SDL_Rect OwnerRect;
  this->pOwnerWidget->GetRect(OwnerRect);
  UINT cxTile = CDrodBitmapManager::CX_TILE;
  UINT cyTile = CDrodBitmapManager::CY_TILE;

  CCoord startPosition(
    startTile.wX * cxTile + (cxTile / 2) - (SPRITE_SIZE / 2) + OwnerRect.x,
    startTile.wY * cyTile + (cyTile / 2) - (SPRITE_SIZE / 2) + OwnerRect.y);
  CCoord endPosition(
    endTile.wX * cxTile + (cxTile / 2) - (SPRITE_SIZE / 2) + OwnerRect.x,
    endTile.wY * cyTile + (cyTile / 2) - (SPRITE_SIZE / 2) + OwnerRect.y);

  this->positions.insert(startPosition);
  this->positions.insert(endPosition);

  //Interpolate addition dot positions
  int xDistance = endPosition.wX - startPosition.wX;
  int yDistance = endPosition.wY - startPosition.wY;
  int steps = abs(xDistance) > abs(yDistance) ? abs(xDistance) / cxTile : abs(yDistance) / cyTile;

  for (double step = 1; step < steps; ++step)
  {
    double ratio = (step / steps);
    CCoord position(
      startPosition.wX + (xDistance * ratio), startPosition.wY + (yDistance * ratio));
    this->positions.insert(position);
  }

  for (CCoordSet::const_iterator coord = this->positions.begin(); coord != this->positions.end(); ++coord)
  {
    SDL_Rect rect = { coord->wX, coord->wY, coord->wX + SPRITE_SIZE, coord->wY + SPRITE_SIZE };
    this->dirtyRects.push_back(rect);
  }
}

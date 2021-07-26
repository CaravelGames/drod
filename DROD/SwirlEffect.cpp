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

#include "SwirlEffect.h"
#include "DrodBitmapManager.h"
#include <BackEndLib/Assert.h>

#include <math.h>

#define NUM_SWIRL_OBJECTS     (10)        //Sprites in animation
#define SPRITE_SIZE           (6)         //largest image, in pixels
#define MAX_RADIUS            (4.0)       //radius of swirl at initial position
#define NUM_REVOLUTIONS       (2)         //# revolutions around player
#define MS_PER_REVOLUTION     (1000)      //Duration of one revolution (now 1 sec)
#define MS_PER_OBJECT         (60)        //delay before showing each next object in swirl

const double twoPi = 3.1415926536 * 2.0;

const UINT wSwirlImageNo[NUM_SWIRL_OBJECTS] = {
    TI_SWIRLDOT_1,
    TI_SWIRLDOT_1,
    TI_SWIRLDOT_2,
    TI_SWIRLDOT_2,
    TI_SWIRLDOT_3,
    TI_SWIRLDOT_3,
    TI_SWIRLDOT_4,
    TI_SWIRLDOT_4,
    TI_SWIRLDOT_5,
    TI_SWIRLDOT_5 };

const Uint32 EffectDuration = NUM_REVOLUTIONS * MS_PER_REVOLUTION + NUM_SWIRL_OBJECTS * MS_PER_OBJECT;

//********************************************************************************
CSwirlEffect::CSwirlEffect(
  //Constructor.
  //
  //Params:
  CWidget* pSetWidget, //(in) Should be a room widget.
  const UINT eType)
  : CEffect(pSetWidget, EffectDuration, eType)
  , nOpacity(0)
{
  ASSERT(pSetWidget->GetType() == WT_Room);
  SDL_Rect rect = { 0,0,0,0 };
  this->dirtyRects.push_back(rect);
}

//********************************************************************************
void CSwirlEffect::Draw(SDL_Surface& destSurface)
{
  for (UINT i = 0; i < this->drawSwirls.size(); ++i) {
    CMoveCoordEx swirl = this->drawSwirls.at(i);

    g_pTheBM->BlitTileImagePart(wSwirlImageNo[swirl.wValue], swirl.wX, swirl.wY,
      0, 0, swirl.wO, swirl.wO, &destSurface, false, this->nOpacity);
  }
}

//********************************************************************************
void CSwirlEffect::CalculateSwirl(UINT wXCenter, UINT wYCenter)
{
  //Reset area of effect.
  SDL_Rect OwnerRect;
  this->pOwnerWidget->GetRect(OwnerRect);
  ASSERT(this->dirtyRects.size() == 1);
  this->dirtyRects[0].x = OwnerRect.x + OwnerRect.w;
  this->dirtyRects[0].y = OwnerRect.y + OwnerRect.h;

  //Draw swirl.
  double radius, theta;
  UINT wX, wY, wSize;
  UINT xMax = 0, yMax = 0; //bottom edge of bounding box
  const UINT rMaxPosition = NUM_REVOLUTIONS * MS_PER_REVOLUTION;

  const double thetaFactor = twoPi / (MS_PER_REVOLUTION + 1.0);
  const double radiusFactor = MAX_RADIUS / (rMaxPosition + 1.0);
  for (UINT nIndex = 0; nIndex < NUM_SWIRL_OBJECTS; ++nIndex)
  {
    static const UINT SIZES[NUM_SWIRL_OBJECTS] = { 9, 9, 8, 8, 6, 6, 4, 4, 3, 3 };
    const UINT rPosition = dwTimeElapsed - nIndex * MS_PER_OBJECT;
    if (rPosition > rMaxPosition) continue;   //objects appear gradually (excludes "negative" values)
    wSize = SIZES[nIndex];  //dimensions of this dot
    radius = (rMaxPosition - rPosition) * radiusFactor;
    theta = rPosition * thetaFactor;
    wX = static_cast<UINT>(wXCenter + sin(theta) * CBitmapManager::CX_TILE * radius);
    wY = static_cast<UINT>(wYCenter + cos(theta) * CBitmapManager::CY_TILE * radius);

    if (static_cast<int>(wX) >= OwnerRect.x && static_cast<int>(wY) >= OwnerRect.y &&
      wX < OwnerRect.x + OwnerRect.w - wSize &&
      wY < OwnerRect.y + OwnerRect.h - wSize)
    {
      this->drawSwirls.push_back(CMoveCoordEx(wX, wY, wSize, nIndex));

      //Update bounding box of area of effect.
      if (static_cast<int>(wX) < this->dirtyRects[0].x)
        this->dirtyRects[0].x = wX;
      if (static_cast<int>(wY) < this->dirtyRects[0].y)
        this->dirtyRects[0].y = wY;
      if (wX + wSize > xMax)  //Technically should also have a -1,
        xMax = wX + wSize;   //but this simpler and consistent with
      if (wY + wSize > yMax)  //the calculation below.
        yMax = wY + wSize;
    }
  }
  this->dirtyRects[0].w = xMax - this->dirtyRects[0].x;
  this->dirtyRects[0].h = yMax - this->dirtyRects[0].y;
}

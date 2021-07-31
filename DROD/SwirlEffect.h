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

#ifndef SWIRLEFFECT_H
#define SWIRLEFFECT_H

#include "DrodEffect.h"
#include <BackEndLib/Coord.h>

class CSwirlEffect : public CEffect {
public:
  CSwirlEffect(CWidget* pSetWidget, const UINT eType);

protected:
  virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed) = 0;
  virtual void Draw(SDL_Surface& destSurface);

  virtual void CalculateSwirl(UINT wXCenter, UINT wYCenter);

  Uint8 nOpacity;
  std::vector<CMoveCoordEx> drawSwirls; //wO = width & height, wValue=frame
};

#endif
// $Id: FadeTileEffect.h 8102 2007-08-15 14:55:40Z trick $

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2007
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef FADETILEEFFECT_H
#define FADETILEEFFECT_H

#include "AnimatedTileEffect.h"

//*****************************************************************************
class CFadeTileEffect : public CAnimatedTileEffect
{
public:
	CFadeTileEffect(CWidget *pSetWidget, const CCoord &SetCoord,
			const UINT tile, const UINT duration=2000);

protected:
	virtual bool Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed);
};

#endif //...#ifndef FADETILEEFFECT_H

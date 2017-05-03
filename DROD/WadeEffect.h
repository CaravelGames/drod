// $Id: WadeEffect.h 8019 2007-07-14 22:30:11Z trick $

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
 * Portions created by the Initial Developer are Copyright (C) 2011
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef WADEEFFECT_H
#define WADEEFFECT_H

#include <FrontEndLib/AnimatedTileEffect.h>

//*****************************************************************************
class CWadeEffect : public CAnimatedTileEffect
{
public:
	CWadeEffect(CWidget *pSetWidget, const CCoord &SetCoord);

	virtual bool Draw(SDL_Surface* pDestSurface=NULL);
};

#endif   //...#ifndef WADEEFFECT_H

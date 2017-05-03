// $Id: ShrinkTileEffect.h 8102 2007-08-15 14:55:40Z mrimer $

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

#ifndef SCALETILEEFFECT_H
#define SCALETILEEFFECT_H

#include "Effect.h"
#include <BackEndLib/Coord.h>

//*****************************************************************************
class CScaleTileEffect : public CEffect
{
public:
	CScaleTileEffect(CWidget *pSetWidget,
			const int xCenter, const int yCenter,
			const UINT tile,
			const UINT duration, const float fScaleRate);
	~CScaleTileEffect();

	virtual bool Draw(SDL_Surface* pDestSurface=NULL);
	void SetOpacity(const BYTE opacity) {this->nOpacity = opacity;}
	void SetScaleSize(const int nWidth, const int nHeight);
	void SetTile(const UINT tileNo);

private:
	UINT  dwDuration;
	UINT  tileNo;
	SDL_Rect srcRect; //area of src tile to scale
	int   xCenter, yCenter;
	BYTE  nOpacity;
	float fScaleRate;

	SDL_Surface *pSrcSurface, *pScaledSurface;
	Uint8 *pSrcPixel; //origin pixel in source surface to rotate
};

#endif //...#ifndef SCALETILEEFFECT_H

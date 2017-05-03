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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): mrimer
 *
 * ***** END LICENSE BLOCK ***** */

//SUMMARY
//
//CParticleExplosionEffect is a base class for particle explosion effects drawn on top of widgets.
//Effects are temporary animations drawn by the owner widget.  The screen surface 
//update is performed by the owner widget.
//

#ifndef CPARTICLEEXPLOSIONEFFECT_H
#define CPARTICLEEXPLOSIONEFFECT_H

#include "DrodEffect.h"
#include "../DRODLib/CurrentGame.h"

#define PARTICLES_PER_EXPLOSION (25)

struct PARTICLE
{
	float x, y;    //position
	float mx, my;  //momentum
	float rotation; //angle (radians)
	float angularMomentum; //angular momentum (rad/s)
	int type;      //tile type index
	UINT tileNo, xDim, yDim; //tile sprite and size
	UINT wDurationLeft;
	bool bActive;
};


//******************************************************************************
class CRoomWidget;
class CParticleExplosionEffect : public CEffect
{
public:
	CParticleExplosionEffect(CWidget *pSetWidget, const CMoveCoord &MoveCoord,
			const UINT wMaxParticleSizeX, const UINT wMaxParticleSizeY, const UINT wParticleTypes,
			const UINT wMinParticles=PARTICLES_PER_EXPLOSION,
			const UINT wParticleMinDuration=7, const UINT wParticleSpeed=4,
			const bool bContinuous=false, const bool bFromEdge=false,
			const EffectType eType=EGENERIC);
	virtual ~CParticleExplosionEffect();

	virtual bool Draw(SDL_Surface* pDestSurface=NULL);

	void SetRotating(const bool bVal) {this->bRotatingParticles = bVal;}

protected:
	void InitParticles();
	bool MoveParticles();
	void ResetParticle(const UINT wIndex);

	CMoveCoord origin;
	const UINT wParticleTypes;
	std::vector<UINT> tileNums, xDims, yDims;

	std::vector<PARTICLE> parrParticles;

	UINT wParticleCount, wParticleMinDuration, wParticleSpeed;
	bool bContinuous; //effect continues indefinitely
	bool bRotatingParticles;
	bool bFromTileEdge; //particles originate from tile edge, not center

	int pixelOriginX, pixelOriginY;
	int nXOffset, nYOffset;
	float fXSpread, fYSpread;

private:
	bool OutOfBounds(const PARTICLE &particle) const;
	virtual bool HitsObstacle(const CDbRoom *pRoom, const PARTICLE &particle) const;
	void ReflectParticle(PARTICLE &particle) const;

	CRoomWidget *pRoomWidget;
	SDL_Rect screenRect;
};

#endif   //...#ifndef CPARTICLEEXPLOSIONEFFECT_H

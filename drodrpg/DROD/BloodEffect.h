// $Id: BloodEffect.h 9096 2008-07-09 05:45:56Z mrimer $

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
 * Contributor(s): mrimer
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef BLOODEFFECT_H
#define BLOODEFFECT_H

#include "ParticleExplosionEffect.h"

//****************************************************************************************
class CBloodEffect : public CParticleExplosionEffect
{
public:
	CBloodEffect(CWidget *pSetWidget, const CMoveCoord &MoveCoord,
			const UINT wParticles, const UINT wParticleMinDuration, const UINT baseSpeed);
};

class CBloodInWallEffect : public CParticleExplosionEffect
{
public:
	CBloodInWallEffect(CWidget *pSetWidget, const CMoveCoord &MoveCoord,
			const UINT wParticles=PARTICLES_PER_EXPLOSION);

	virtual void Draw(SDL_Surface& destSurface);
	virtual bool HitsObstacle(const CDbRoom *pRoom, const PARTICLE &particle) const;
};

#endif //...#ifndef BLOODEFFECT_H

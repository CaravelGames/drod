// $Id: ParticleExplosionEffect.cpp 10126 2012-04-24 05:40:08Z mrimer $

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

#include "ParticleExplosionEffect.h"
#include "RoomWidget.h"
#include "TileImageConstants.h"
#include "DrodBitmapManager.h"
#include "../DRODLib/GameConstants.h"
#include "../DRODLib/TileConstants.h"
#include "../DRODLib/DbRooms.h"

//*****************************************************************************
CParticleExplosionEffect::CParticleExplosionEffect(
//Constructor.
//
//Adds a bunch of particles to the display that will continue to move
//in a specified direction with animation updates.
//
//Params:
	CWidget *pSetWidget,          //(in) Should be a room widget.
	const CMoveCoord &MoveCoord,  //(in) Location of explosion and direction of its movement.
	const UINT wMaxParticleSizeX, //(in) Max dimensions of particle sprite
	const UINT wMaxParticleSizeY, //(in) " "
	const UINT wParticleTypes,    //(in) # styles to display
	const UINT wMinParticles,     //(in) Minimum number of particles to generate [default=25]
	const UINT wParticleMinDuration,  //(in) Minimum frames particle lives [default=7]
	const UINT wParticleSpeed,    //(in) Particle speed (in pixels/frame) [default=4]
	const bool bContinuous,       //(in) Whether to replace particles that die
											//so effect lasts indefinitely [default=false]
	const bool bFromEdge,         //(in) particles originate from tile's edge, not center
	const EffectType eType)       //[default = EGENERIC]
	: CEffect(pSetWidget, (UINT)-1, eType)
	, origin(MoveCoord)
	, wParticleTypes(wParticleTypes)
	, wParticleMinDuration(wParticleMinDuration)
	, bContinuous(bContinuous)
	, bFromTileEdge(bFromEdge)
	, pRoomWidget(NULL)
{
	if (pSetWidget->GetType() == WT_Room)
		this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);

	pSetWidget->GetRect(this->screenRect);

	ASSERT(IsValidOrientation(origin.wO));

	//Determine explosion point of origin (center of tile).
	this->pixelOriginX = origin.wX*CBitmapManager::CX_TILE;
	this->pixelOriginY = origin.wY*CBitmapManager::CY_TILE;
	if (bFromEdge)
	{
		switch (origin.wO)
		{
			case NW: case W: case SW: this->pixelOriginX += (CBitmapManager::CX_TILE - wMaxParticleSizeX); break;
			case N: case S: case NO_ORIENTATION: this->pixelOriginX += (CBitmapManager::CX_TILE - wMaxParticleSizeX)/2; break;
			default: break;
		}
		switch (origin.wO)
		{
			case NW: case N: case NE: this->pixelOriginY += (CBitmapManager::CY_TILE - wMaxParticleSizeY); break;
			case W: case E: case NO_ORIENTATION: this->pixelOriginY += (CBitmapManager::CY_TILE - wMaxParticleSizeY)/2; break;
			default: break;
		}
	} else {
		this->pixelOriginX += (CBitmapManager::CX_TILE - wMaxParticleSizeX)/2;
		this->pixelOriginY += (CBitmapManager::CY_TILE - wMaxParticleSizeY)/2;
	}

	//Define ranges for explosion directions.
	switch (origin.wO)
	{
		case NW: case W: case SW: this->nXOffset=-static_cast<int>(wParticleSpeed); break;
		case NE: case E: case SE: this->nXOffset=wParticleSpeed; break;
		case N: case S: case NO_ORIENTATION: this->nXOffset=0;  break;
		default: break;
	}
	switch (origin.wO)
	{
		case NW: case N: case NE: this->nYOffset=-static_cast<int>(wParticleSpeed); break;
		case SW: case S: case SE: this->nYOffset=wParticleSpeed; break;
		case W: case E: case NO_ORIENTATION: this->nYOffset=0;  break;
		default: break;
	}
	switch (origin.wO)
	{
		case N: case S:
			this->fXSpread=wParticleSpeed*1.414f;
			this->fYSpread=wParticleSpeed*1.414f;
			break;
		case E: case W:
			this->fXSpread=wParticleSpeed*1.414f;
			this->fYSpread=wParticleSpeed*1.414f;
			break;
		case NO_ORIENTATION:
		case NE: case NW: case SE: case SW:
			this->fXSpread=(float)wParticleSpeed;
			this->fYSpread=(float)wParticleSpeed;
			break;
		default: break;
	}

	this->tileNums.resize(wParticleTypes, TI_UNSPECIFIED);
	this->xDims.resize(wParticleTypes, 0);
	this->yDims.resize(wParticleTypes, 0);

	//Add random explosion particles.
	this->wParticleCount = wMinParticles + RAND(wMinParticles/2);
	this->parrParticles.resize(this->wParticleCount);

	this->dirtyRects.push_back(this->screenRect);
	this->dirtyRects[0].w = this->dirtyRects[0].h = 0;

	this->bRotatingParticles = g_pTheDBM->eyeCandy > 0;
}

//*****************************************************************************
CParticleExplosionEffect::~CParticleExplosionEffect()
//Destructor.
{
}

//********************************************************************************
bool CParticleExplosionEffect::Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed)
{
	static const Uint32 MAX_TIME_STEP = 100; //at least 10fps

	if (!MoveParticles(min(MAX_TIME_STEP, wDeltaTime)))
		return false;

	return true;
}
//********************************************************************************
void CParticleExplosionEffect::Draw(SDL_Surface& destSurface)
{
	SDL_Surface* pRotatedSurface = NULL;
	for (int nIndex = wParticleCount; nIndex--; )
	{
		PARTICLE& p = this->parrParticles[nIndex];

		//If particle is still active, plot to display.
		if (p.bActive)
		{
			const UINT wTileNo = p.tileNo;
			UINT dimX = p.xDim;
			UINT dimY = p.yDim;

			if (p.rotation != 0.0f)
			{
				//Draw tile rotated to its exact angle.
				SDL_Surface* pSrcSurface = g_pTheDBM->GetTileSurface(wTileNo);
				Uint8* pSrcPixel = g_pTheDBM->GetTileSurfacePixel(wTileNo);

				//Set to transparent tile colorkey.
				const UINT wSurfaceIndex = g_pTheDBM->GetTileSurfaceNumber(wTileNo);
				g_pTheBM->SetSurfaceColorKey(wTileNo, wSurfaceIndex, pSrcSurface);

				//Rotate.
				pRotatedSurface = g_pTheDBM->RotateSurface(pSrcSurface, pSrcPixel,
					dimX, dimY, p.rotation * RADS_TO_DEGREES);
			}

			if (!pRotatedSurface)
			{
				//Unrotated particle.
				g_pTheBM->BlitTileImagePart(wTileNo, ROUND(p.x),
					ROUND(p.y), 0, 0, dimX, dimY, &destSurface, true);
			}
			else {
				//Blit rotated particle.
				dimX = pRotatedSurface->w, dimY = pRotatedSurface->h;
				SDL_Rect src = MAKE_SDL_RECT(0, 0, dimX, dimY);
				SDL_Rect dest = MAKE_SDL_RECT(ROUND(p.x), ROUND(p.y), dimX, dimY);
				g_pTheDBM->BlitSurface(pRotatedSurface, &src, &destSurface, &dest);

				//Darken sprite.
				if (dest.w && dest.h)
				{
					ASSERT(src.w >= dest.w);
					ASSERT(src.h >= dest.h);
					src.w = dest.w; //update to blitted area
					src.h = dest.h;
					g_pTheDBM->DarkenWithMask(pRotatedSurface, src,
						&destSurface, dest, CBitmapManager::fLightLevel, true);
				}

				SDL_FreeSurface(pRotatedSurface);
				pRotatedSurface = NULL;
			}
		}
	}
}

//*****************************************************************************
void CParticleExplosionEffect::InitParticles()
//Initialize all particles.
{
	for (UINT wIndex=this->wParticleCount; wIndex--; )
		ResetParticle(wIndex);
}

//*****************************************************************************
bool CParticleExplosionEffect::MoveParticles(const UINT wDeltaTime)
//Updates positions of all the particles.
//Invalidates particles having left the valid display area.
//
//Returns: whether any particles are still active
{
	UINT xMax=0, yMax=0; //bottom edge of bounding box
	bool bActiveParticles = false;   //whether any particles are still active

	//Reset bounding box.
	ASSERT(this->dirtyRects.size() == 1);
	this->dirtyRects[0].x = this->screenRect.x + this->screenRect.w;
	this->dirtyRects[0].y = this->screenRect.y + this->screenRect.h;
	this->dirtyRects[0].w = this->dirtyRects[0].h = 0;

	static const Uint32 MAX_TIME_STEP = 100;        //at least 10fps
	const Uint32 dwNow = SDL_GetTicks();
	const Uint32 dwTimeElapsed = this->dwTimeOfLastMove >= dwNow ? 1 :
			dwNow - this->dwTimeOfLastMove >= MAX_TIME_STEP ? MAX_TIME_STEP :
			dwNow - this->dwTimeOfLastMove;
	const float fMultiplier = dwTimeElapsed / 33.0f;
	const int nDecay = ROUND(2.0f / fMultiplier);

	const CDbRoom *pRoom = this->pRoomWidget ? this->pRoomWidget->GetCurrentGame()->pRoom : NULL;
	
	bool bResetParticle = false;
	for (UINT wIndex=this->wParticleCount; wIndex--; )
	{
		PARTICLE& p = this->parrParticles[wIndex];

#define RESET_PARTICLE {if (this->bContinuous) bResetParticle = true; else {p.bActive=false; continue;}  }

		//See if particle should still be active and moved.
		if (!p.bActive)
			continue;

		//Update real position in real time.
		p.x += p.mx * fMultiplier;
		p.y += p.my * fMultiplier;

		//If particle is going to go out of bounds, kill it.
		if (OutOfBounds(p))
		{
			RESET_PARTICLE
		} else {
			//Exponential particle decay.
			if (RAND(nDecay) == 0)
			{
				if (--p.wDurationLeft == 0) //display time is over
				{
					RESET_PARTICLE
				}
			}

			//Does particle run into an obstacle?
			if (pRoom && HitsObstacle(pRoom, p))
			{
				ReflectParticle(p);
				if (OutOfBounds(p))
				{
					RESET_PARTICLE
				}
			}
		}

		if (bResetParticle)
		{
			ResetParticle(wIndex);
			bResetParticle = false;
		}

		if (this->bRotatingParticles)
			p.rotation += p.angularMomentum * fMultiplier;

		//By this point, any active particle should be in bounds.
#ifdef _DEBUG
		ASSERT(!OutOfBounds(p));
#endif

		//Update bounding box of area of effect for this particle.
		const int x = ROUND(p.x);
		const int y = ROUND(p.y);
		int x2, y2;
		if (this->bRotatingParticles)
		{
			x2 = x + int(p.xDim * 1.414f); //largest possible size increase is when rotated 45%
			y2 = y + int(p.yDim * 1.414f);
		} else {
			x2 = x + p.xDim;
			y2 = y + p.yDim;
		}
		if (x < this->dirtyRects[0].x)
			this->dirtyRects[0].x = x;
		if (y < this->dirtyRects[0].y)
			this->dirtyRects[0].y = y;
		if (x2 > static_cast<int>(xMax))
			xMax = x2;
		if (y2 > static_cast<int>(yMax))
			yMax = y2;

		bActiveParticles = true;
#undef RESET_PARTICLE
	}
	if (bActiveParticles)
	{
		this->dirtyRects[0].w = xMax - this->dirtyRects[0].x;
		this->dirtyRects[0].h = yMax - this->dirtyRects[0].y;
	}

	this->dwTimeOfLastMove = SDL_GetTicks();

	return bActiveParticles;
}

//*****************************************************************************
bool CParticleExplosionEffect::OutOfBounds(const PARTICLE &particle) const
{
	UINT x2, y2;
	if (particle.rotation == 0.0f)
	{
		x2 = static_cast<UINT>(particle.x + particle.xDim);
		y2 = static_cast<UINT>(particle.y + particle.yDim);
	} else {
		x2 = static_cast<UINT>(particle.x + int(particle.xDim * 1.414f)); //max size at 45 degree rotation
		y2 = static_cast<UINT>(particle.y + int(particle.yDim * 1.414f));
	}
	return (particle.x < this->screenRect.x || particle.y < this->screenRect.y ||
				x2 >= static_cast<UINT>(this->screenRect.x + this->screenRect.w) ||
				y2 >= static_cast<UINT>(this->screenRect.y + this->screenRect.h));
}

//*****************************************************************************
bool CParticleExplosionEffect::HitsObstacle(const CDbRoom *pRoom, const PARTICLE &particle) const
//O-square obstacle?
{
	ASSERT(pRoom);
	const UINT wOTileNo = pRoom->GetOSquare(
		((Sint16)particle.x - screenRect.x) / CBitmapManager::CX_TILE,
		((Sint16)particle.y - screenRect.y) / CBitmapManager::CY_TILE);
	switch (wOTileNo)
	{
		case T_BRIDGE: case T_BRIDGE_H: case T_BRIDGE_V:
		case T_FLOOR: case T_FLOOR_M:	case T_FLOOR_ROAD: case T_FLOOR_GRASS:
		case T_FLOOR_DIRT: case T_FLOOR_ALT: case T_FLOOR_IMAGE:
		case T_PLATFORM_W: case T_PLATFORM_P:
		case T_PIT: case T_PIT_IMAGE:
		case T_WATER: case T_GOO:
		case T_DOOR_YO: case T_DOOR_GO: case T_DOOR_CO:
		case T_DOOR_RO: case T_DOOR_BO: case T_DOOR_MONEYO:
		case T_TRAPDOOR: case T_TRAPDOOR2:
		case T_STAIRS: case T_STAIRS_UP:
		case T_HOT:
		case T_PRESSPLATE:
			return false;  //particle can go through these things
		default: return true;
	}
}

//*****************************************************************************
inline void CParticleExplosionEffect::ReflectParticle(PARTICLE &particle) const
{
	//Roughly reverse the position update just made.
	particle.x -= particle.mx; //NOTE: excludes time factor
	particle.y -= particle.my;

	//Randomly reflect particle trajectory.
	if (RAND(2) == 0)
	{
		particle.mx = -particle.mx;
		particle.x += particle.mx;
	} else {
		particle.my = -particle.my;
		particle.y += particle.my;
	}
}

//*****************************************************************************
void CParticleExplosionEffect::ResetParticle(const UINT wIndex)
//Start fresh particle near explosion origin.
{
	ASSERT(wIndex < this->wParticleCount);

	PARTICLE& p = this->parrParticles[wIndex];
	p.x = static_cast<float>(this->screenRect.x + this->pixelOriginX);
	p.y = static_cast<float>(this->screenRect.y + this->pixelOriginY);
	p.mx = this->nXOffset + fRAND_MID(this->fXSpread*0.5f);
	p.my = this->nYOffset + fRAND_MID(this->fYSpread*0.5f);
	if (this->bRotatingParticles)
	{
		p.rotation = fRAND_MID(PI);
		p.angularMomentum = fRAND_MID(0.5f);
	} else {
		p.rotation = p.angularMomentum = 0.0f;
	}
	p.wDurationLeft = this->wParticleMinDuration;

	p.type = RAND(this->wParticleTypes); //one of defined particle styles
	p.tileNo = this->tileNums[p.type];
	p.xDim = this->xDims[p.type];
	p.yDim = this->yDims[p.type];

	p.bActive = true;
}

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
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef DRODEFFECT_H
#define DRODEFFECT_H

#include <FrontEndLib/Effect.h>
#include <FrontEndLib/EffectList.h>

//Set of all DROD effects that need to be kept track of (for deleting early, for example).
//Add other effects here as needed.
enum EffectType
{
	EALL = DRAW_ALL_EFFECTS_TYPE, // Special flag used by EffectList to render every effect

	EGENERIC      = EFFECTLIB::EGENERIC,      //generic effect
	ESHADE        = EFFECTLIB::ESHADE,        //square highlighting
	ETRANSTILE    = EFFECTLIB::ETRANSTILE,    //transparent tiles
	EFRAMERATE    = EFFECTLIB::EFRAMERATE,    //frame rate effect
	ETOOLTIP      = EFFECTLIB::ETOOLTIP,      //tool tip
	ESUBTITLE     = EFFECTLIB::ESUBTITLE,     //scrolling text box
	EBUMPOBSTACLE = EFFECTLIB::EBUMPOBSTACLE, //player bumps into obstacle
	EFLASHSHADE   = EFFECTLIB::EFLASHSHADE,   //flashing square highlighting
	EFLOAT        = EFFECTLIB::EFLOAT,        //floating tile graphic
	EFLOATTEXT    = EFFECTLIB::EFLOATTEXT,    //floating text
	ETEXT         = EFFECTLIB::ETEXT,         //text
	EFLASHTEXT    = EFFECTLIB::EFLASHTEXT,    //Flashing text message (Secret room/exit level/mastery)
	EPENDINGPLOT,     //selecting room area for plot in editor
	EORBHIT,          //strike orb effect
	ESPARK,           //fuse spark effect
	EGAZE,            //Aumtlich gaze
	EVERMIN,          //vermin effect
	ESWORDSWING,      //sword swing effect
	ESWIRL,           //player swirl effect
	EEVILEYEGAZE,     //evil eye gaze (non-fading)
	EPENDINGBUILD,    //pending build in game
	ESNOWFLAKE,       //a falling snow flake
	EVARMONITOR,      //variable monitor
	ESTEAM,           //rising steam/smoke puff
	ESPLASH,          //water splash
	ECHATTEXT,        //CaravelNet chat text display
	ERAINDROP,        //a falling rain drop
	ESTUN,            //stunned entity
	EPUFFEXPLOSION,   //puff explosion
	ESPIKES,          //floor spikes
	EFIRETRAP,        //fire trap
	ETEMPORALMOVE,    //temporal move
	EIMAGEOVERLAY,    //image overlay
	EGRID,            //grid overlay
	ETEXTNOTICE,      //text notice
	ECNETNOTICE,      //caravelnet notice
	EROOMDRAWSTATS,
	ETILESWIRL,       //tile swirl effect
};

//*****************************************************************************
//Don't renumber these enumerations so scripted events always work properly.
enum VisualEffectType
{
	VET_BLOODSPLAT=0,
	VET_MUDSPLAT=1,
	VET_TARSPLAT=2,
	VET_GELSPLAT=3,
	VET_SEEPSPLAT=4,
	VET_GOLEMSPLAT=5,
	VET_SLAYERSPLAT=6,
	VET_DEBRIS=7,
	VET_SPARKS=8,
	VET_EXPLOSION=9,
	VET_SPLASH=10,
	VET_STEAM=11,
	VET_SWIRL=12,
	VET_VERMIN=13,
	VET_BOLT=14,
	VET_JITTER=15,
	VET_SPIKES=16,
	VET_PUFFEXPLOSION=17,
	VET_FIRETRAP=18,
	VET_ICEMELT=19,
	VET_PUFFSPLAT=20,
	VET_TILESWIRL=21
};

#endif //...#ifndef DRODEFFECT_H

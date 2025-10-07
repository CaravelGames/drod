// $Id: DrodEffect.h 9090 2008-07-09 05:41:49Z mrimer $

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
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef DRODEFFECT_H
#define DRODEFFECT_H

#include <FrontEndLib/Effect.h>

#define ROUND(x)  (int)((x) + 0.5)

//Set of all DROD effects that need to be kept track of (for deleting early, for example).
//Add other effects here as needed.
enum EffectType
{
	EGENERIC=EFFECTLIB::EGENERIC,           //generic effect
	ESHADE=EFFECTLIB::ESHADE,               //square highlighting
	ETRANSTILE=EFFECTLIB::ETRANSTILE,       //transparent tiles
	EFRAMERATE=EFFECTLIB::EFRAMERATE,       //frame rate effect
	ETOOLTIP=EFFECTLIB::ETOOLTIP,           //tool tip
	ESUBTITLE=EFFECTLIB::ESUBTITLE,         //scrolling text box
	EBUMPOBSTACLE=EFFECTLIB::EBUMPOBSTACLE, //player bumps into obstacle
	EFLASHSHADE=EFFECTLIB::EFLASHSHADE,     //flashing square highlighting
	EFLOAT=EFFECTLIB::EFLOAT,               //floating tile graphic
	EFLOATTEXT=EFFECTLIB::EFLOATTEXT,       //floating text
	ETEXT=EFFECTLIB::ETEXT,                 //text
	EFADETILE=EFFECTLIB::EFADETILE,         //fading tile
	EROTATETILE=EFFECTLIB::EROTATETILE,     //rotating tile
	ESCALETILE=EFFECTLIB::ESCALETILE,       //scaling tile
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
	EDAMAGEPREVIEW,   //hovering enemy damage display
	EIMAGEOVERLAY,    //image overlay
	EFIRETRAP,        //fire trap
	EPUFFEXPLOSION,   //puff explosion
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
	VET_MONSTERBITE=15,
	VET_PARRY=16,
	VET_JITTER=17,
	VET_STRONGHIT=18,
	VET_EQUIP=19,
	VET_ICEMELT=20,
	VET_FIRETRAP=21,
	VET_PUFFEXPLOSION=22,
	VET_CONSTRUCTSPLAT=23,
};

#endif //...#ifndef DRODEFFECT_H

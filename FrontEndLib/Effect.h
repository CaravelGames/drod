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

//SUMMARY
//
//CEffect is an abstract base class for graphical effects drawn on top of widgets.
//Effects are temporary animations drawn by the owner widget.  The screen surface 
//update is performed by the owner widget.
//
//USAGE
//
//The widget must have code to add, draw, and clear effects before an effect 
//can be used with it.  In CRoomWidget, a call to AddTLayerEffect() or 
//AddLastLayerEffect() is all that is needed for room widget to handle the effect.
//
//DESIGNING AN EFFECT
//
//1. Create a new class derived from CEffect.  
//2. Your constructor should take params for everything needed to draw the effect 
//   over its lifetime.  For speed, try to perform calculations here instead
//   of in Draw().  In other words, it is better to perform a calc one time in
//   the constructor, instead of every time Draw() is called from the i/o loop.
//   Avoid keeping pointers passed from cue event private data or current game--
//   they can become invalid during the lifetime of your effect.
//3. Override the Draw() pure virtual method in your class.  Draw() should do
//   the following:
//    a. Return false without drawing if the timespan that the effect can 
//       be drawn in has past.  For example if you have an explosion that lasts
//       1000ms and TimeElapsed() returns 1002, then the effect is no longer
//       active and you should return false.
//    b. Decide what to draw based on the time elapsed since construction
//       returned from TimeElapsed(), the class members set by the constructor,
//       and data from the owner widget.
//    c. Draw to the screen surface.
//    d. Specify the bounding box the effect covers (in absolute coordinates).
//    e. Return true to specify the effect is still continuing.
//       (If the effect has completed, return false before drawing anything.)

#ifndef EFFECT_H
#define EFFECT_H

#include "Widget.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Types.h>

//****************************************************************************************

#define ROUND(x)  (int)((x) + 0.5)

#define PI (3.1415926535f)
#define TWOPI (2.0f * PI)
#define RADS_TO_DEGREES (180.0f / PI)

//Generic effects needed, but can't place them in a final enumeration yet (i.e.,
//there will probably be more effects used in the app).
//Give these values to the corresponding names in the final enumeration.
namespace EFFECTLIB
{
	enum EFFECTTYPE
	{
		EGENERIC=0,       //generic effect
		ESHADE,           //square highlighting
		ETRANSTILE,       //transparent tiles
		EFRAMERATE,       //frame rate effect
		ETOOLTIP,         //tool tip
		ESUBTITLE,        //scrolling text box
		EBUMPOBSTACLE,    //something bumps into obstacle
		EFLASHSHADE,      //flashing square highlighting
		EFLOAT,           //floating tile graphic
		EFLOATTEXT,       //rising text
		ETEXT,            //text
		EFADETILE,        //fading tile
		EROTATETILE,      //rotating tile
		ESCALETILE,       //scaled tile
		EFLASHTEXT        //Flashing text message (Secret room/exit level/mastery)
	};
};

//****************************************************************************************
class CEffectList;
class CEffect
{
friend class CEffectList;  //to access dwTimeStarted

public:
	CEffect(CWidget *pSetOwnerWidget, const UINT dwDuration, const UINT eType);
	virtual ~CEffect() { }

	void           Draw(SDL_Surface* pDestSurface=NULL);
	virtual long   GetDrawSequence() const {return 0;}
	UINT           GetEffectType() const {return this->eEffectType;}
	float          GetElapsedFraction() const;
	float          GetRemainingFraction() const;
	void           RequestRetainOnClear(const bool bVal=true) {this->bRequestRetainOnClear = bVal;}
	bool           RequestsRetainOnClear() const {return this->bRequestRetainOnClear;}
	void           SetOpacity(float fOpacity) {this->fOpacity = fOpacity;}
	bool           Update(const UINT wDeltaTime); // Updates the state of the effect without drawing it

	vector<SDL_Rect>  dirtyRects; //bounding boxes covered by effect this frame

protected:
	virtual bool   Update(const UINT wDeltaTime, const Uint32 dwTimeElapsed) = 0;
	virtual void   Draw(SDL_Surface& destSurface) = 0;
	SDL_Surface *  GetDestSurface() {return this->pOwnerWidget->GetDestSurface();}

	CWidget *      pOwnerWidget;

	UINT           dwDuration;
	Uint32         dwTimeElapsed;
	Uint32         dwTimeOfLastMove; //last time effect was drawn

	UINT           eEffectType;   //can't finalize as enumeration yet

	//If set, when effects are cleared from a list, allow this one to stay unless forced.
	bool           bRequestRetainOnClear;

	float          fOpacity;  //opacity (not supported by all effects)
};

#endif //...#ifndef EFFECT_H

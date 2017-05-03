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
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef TITLESCREEN_H
#define TITLESCREEN_H

#include "DrodScreen.h"
#include "VerminEffect.h"

#include <BackEndLib/Types.h>
#include <BackEndLib/IDSet.h>
#include <FrontEndLib/MarqueeWidget.h>
#include <FrontEndLib/ImageWidget.h>

//Menu selection.
enum TitleSelection
{
	MNU_NEWGAME,
	MNU_CONTINUE,
	MNU_RESTORE,
	MNU_SETTINGS,
	MNU_HELP,
	MNU_DEMO,
	MNU_QUIT,
	MNU_BUILD,
	MNU_WHO,
	MNU_WHERE,
	MNU_TUTORIAL,
	MNU_PLAYMENU,
	MNU_MAINMENU,
	MNU_BUY,
	MNU_CHAT,

	MNU_COUNT,

	MNU_UNSPECIFIED = -1
};

class CMenuWidget;
class CTitleScreen : public CDrodScreen
{
protected:
	friend class CDrodScreenManager;

	CTitleScreen();
	~CTitleScreen();

	virtual bool   SetForActivate();

private:
	void     AddSpaceToMarquee();
	void     AddTextToMarquee(const WCHAR *pText);

	void     Animate();

	void     AnimateCaravelLogo(SDL_Surface *pDestSurface);
	void     AnimateWaves(SDL_Surface *pDestSurface, bool update);
	void     AnimateFlag(SDL_Surface *pDestSurface, bool update);

	void     RequestNews();
	UINT     GetNextDemoID();
	virtual void   Paint(bool bUpdateRect=true);
	SCREENTYPE  ProcessMenuSelection(TitleSelection wMenuPos);

	bool     IsShowingAlphaEffects() const;

	void     LoadDemos();

	virtual void   OnBetweenEvents();
	virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDeactivate();
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   OnMouseDown(const UINT dwTagNo, const SDL_MouseButtonEvent &Button);
	virtual void   OnMouseMotion(const UINT dwTagNo,
			const SDL_MouseMotionEvent &MotionEvent);
	virtual void   OnSelectChange(const UINT dwTagNo);
	void     PlayTitleSong();
	bool     PollForHoldList();
	bool     PollForNews();

	void     RedrawScreen(const bool bUpdate=true);
	void     RefreshBackground(SDL_Surface* pBackground, SDL_Surface* pDestSurface, SDL_Rect& dest,
		int x_offset=0, int y_offset=0);
	void     ResetCNetStatus();
	void     SetMenuOptionStatus();
	void     SetNewsText();
	void     SetTitleScreenSkin();
	void     SetVisibleWidgetsForHold();
	virtual bool   UnloadOnDeactivate() const;

	void DrawJtRHBorder(SDL_Surface *pDestSurface);
	void DrawLightMask(SDL_Surface *pDestSurface, int nMouseX, int nMouseY, float fFactor);
	bool UpdateMapViewPosition();
	void RedrawMapArea(const bool bAlwaysRedraw=true);

	//Demo display.
	CIDSet   ShowSequenceDemoIDs;
	CIDSet::const_iterator  currentDemo;
	bool     bReloadDemos; //demos are shown from the selected hold only
	UINT     dwNonTutorialHoldID;

	CMenuWidget *pMenu, *pPlayMenu;
	UINT     dwFirstPaint, dwDemoWaitTimer;
	bool     bSavedGameExists;
	CImageWidget *pInternetIcon;

	//Internet handles.
	UINT     wNewsHandle;
	WSTRING  wstrNewsText;
	CMarqueeWidget *pMarqueeWidget;
	bool bWaitingForHoldlist;

	//Graphics.
	CDbHold::HoldStatus hold_status;
	bool bPredarken;
	bool bReloadGraphics;
	bool bForcePaintOnNextAnimate;
	float fDarkFactor;	//how dark to make the background [0-1]

	//JtRH map view position.
	float fInitialMapPos;
	UINT  wMapX, wMapY;
	Uint32 dwMapCycleStart;
	int   multiplier;

	//TCB vermin.
	void addParticle();
	void clickParticles(const int x, const int y);
	void resetParticle(VERMIN& v);
	void updateDirection(VERMIN &v);
	void updateParticles(SDL_Surface *pDestSurface, const int nLightX, const int nLightY);
	vector<VERMIN> vermin;
	CEffectList verminEffects;
	bool bExtraCritters;
	UINT critterKills;
	bool bBackwards;

	//TSS debris.
	enum DebrisType {
		DT_SPRITE,
		DT_SPINNING,
		DT_NON_SPINNING,
		DEBRIS_TYPES
	};

	struct DEBRIS
	{
		DEBRIS() : damaged(SDL_Rect()), pScaledSurface(NULL) { }

		float fX, fY, fAngle;   //position (center), direction (in radians)
		float angularRotation;  //change in direction (radians per second)
		float distance_multiplier; //how far away object is
		UINT xSize, ySize;      //image size
		DebrisType type;   //image family displayed
		UINT type_num;
		bool bActive;
		SDL_Rect damaged;
		SDL_Surface *pScaledSurface;

		void clear_damaged() { damaged.w = damaged.h = 0; }
	};
	struct CompareDebrisPtr
	{
		//Farther away rendered first.
		bool operator()(DEBRIS* d1, DEBRIS* d2) {
			float d1_dist = d1->distance_multiplier, d2_dist = d2->distance_multiplier;

			//Tile graphics are tiny pictures of larger objects,
			//so normalize their distance relationship with custom images.
			if (d1->type == DT_SPRITE)
				d1_dist /= 10.0f;
			if (d2->type == DT_SPRITE)
				d2_dist /= 10.0f;

			return d1_dist < d2_dist;
		}
	};

	void addDebris();
	void clearDebris();
	void eraseDebrisFromScreen();
	UINT get_debris_image_index(DEBRIS& d) const;
	void resetDebris(DEBRIS& d);
	void select_unique_custom_debris(DEBRIS& d) const;
	void sort_debris_by_distance();
	void updateDebris(SDL_Surface *pDestSurface);

	typedef vector<DEBRIS*> DEBRISLIST;
	DEBRISLIST debris;

};

#endif //...#ifndef TITLESCREEN_H

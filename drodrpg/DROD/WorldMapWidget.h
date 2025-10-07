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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2012
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef WORLDMAPWIDGET_H
#define WORLDMAPWIDGET_H

#include "DrodWidget.h"
#include "../DRODLib/HoldRecords.h"

#include <vector>

#define NUM_WORLD_MAP_FRAMES (2)

//******************************************************************************
class CCurrentGame;
class CImageWidget;
class CImageVectorWidget;
class CLabelWidget;
class CTilesWidget;
class CWorldMapWidget : public CWidget
{
	struct AnimatedCharIcon {
		AnimatedCharIcon(const WorldMapIcon& icon, UINT dataID_Tiles, UINT animationSpeed)
			: icon(icon), dataID_Tiles(dataID_Tiles), animationSpeed(animationSpeed)
		{ }

		WorldMapIcon icon;
		UINT dataID_Tiles;
		UINT animationSpeed;
	};

	struct WorldMapIconUI : public WorldMapIcon
	{
		WorldMapIconUI()
			: WorldMapIcon()
			, pLabel(NULL)
		{ }
		WorldMapIconUI(const WorldMapIcon& rhs)
			: WorldMapIcon(rhs)
			, pLabel(NULL)
		{ }

		CLabelWidget* pLabel;
	};

	struct WorldMapImage
	{
		WorldMapImage(const WorldMapIconUI& icon)
			: icon(icon), pSurface(NULL), width(0), height(0)
		{ }

		WorldMapIconUI icon;
		SDL_Surface *pSurface;
		UINT width;
		UINT height;
	};


public:
	CWorldMapWidget(const UINT dwSetTagNo,
		const int nSetX, const int nSetY, const UINT wSetW, const UINT wSetH);

	virtual void Paint(bool bUpdateRect = true);

	const WorldMapIcon* GetClickedIcon() const { return this->pClickedIcon; }

	void SetCurrentGame(CCurrentGame *pCurrentGame);
	void UpdateAnimation() { HandleAnimate(); }

protected:
	virtual ~CWorldMapWidget();
	virtual void   HandleAnimate();
	virtual bool   IsAnimated() const {return true;}
	virtual void   HandleMouseMotion(const SDL_MouseMotionEvent &Motion);
	virtual void   HandleMouseUp(const SDL_MouseButtonEvent &Button);

private:
	void AddImageWidget(CImageWidget* &pImageWidget);
	void AddTilesWidget(CTilesWidget* &pTilesWidget);

	void AddIcons(CCurrentGame *pCurrentGame);
	CLabelWidget* AddLabel(const WorldMapIcon& icon, HoldWorldMap::DisplayType displayType,
			UINT wIconX, UINT wIconY, CCurrentGame *pCurrentGame);
	void AddTile(const WorldMapIcon& icon, UINT coveringIcon, CCurrentGame *pCurrentGame);
	void ClearIcons();
	void ClearLabels();
	void ClearMapImages();
	const WorldMapIconUI* GetIconAt(UINT x, UINT y);
	UINT GetTileFor(UINT charID, UINT frame, CCurrentGame *pCurrentGame) const;

	CImageWidget *pWorldMapImage;
	CImageVectorWidget *pLocationImages;
	CTilesWidget *pTilesWidget[NUM_WORLD_MAP_FRAMES];
	CTilesWidget *pOverlayTilesWidget, *pAnimatedCustomCharTilesWidget;

	Uint32 dwLastAnimate;
	std::vector<WorldMapIconUI> icons;
	std::vector<CLabelWidget*> labels;
	std::vector<AnimatedCharIcon> animatedChars;
	std::vector<WorldMapImage> mapImages;

	const WorldMapIconUI *pClickedIcon, *pHighlightedIcon;
};

#endif //#ifndef WORLDMAPWIDGET_H

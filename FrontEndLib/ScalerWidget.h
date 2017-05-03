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
 * mrimer
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef SCALERWIDGET_H
#define SCALERWIDGET_H

//SUMMARY
//
//CScalerWidget draws one or more other widgets, scaling them to fit
//inside of its area.  It draws in two passes.  The first pass draw, performed
//in Paint(), is done as quickly as possible and doesn't look great.  The second 
//pass draw, performed in one or more HandleAnimate() calls, generates an 
//anti-aliased image.
//
//USAGE
//
//Create CScalerWidget and add it to a parent like other widgets.  Its dimensions
//will be the destination area in which source widget(s) will be scaled to fit.
//
//Next, call CScalerWidget::AddScaledWidget() to specify one or more source widgets
//to draw scaled.  You use AddScaledWidget() just like AddWidget().  In fact you can
//have both scaled and normal children.  Normal children are added with
//AddWidget() in the usual way and are not drawn scaled.  Scaled children are drawn 
//before normal children, and all CWidget methods that affect children will not 
//affect them. (i.e. GetWidgetContainingCoords() called from the CScalerWidget 
//will not find any of the scaled widgets--they are sandboxed.)
//
//In calls to AddScaledWidget(), put one widget at x=0, y=0.  The algorithms depend
//on it, and assertians will fire if you don't do this.
//
//You can resize the CScalerWidget at any time.  However, extra calculations are
//made when you do this, so watch performance.  Generally, CScalerWidget is 
//optimized for repeated scaling with fixed dimensions.
//
//MEMORY
//
//CScalerWidget can use a lot of memory in its loaded state to keep instructions 
//for quick drawing and a hidden true-scale surface as a source image.  Use this 
//formula to estimate how much memory is used:
//
//  (DestWidth * DestHeight * 4 bytes) + (SourceWidth * SourceHeight * 3 bytes)
//
//FUTURE CONSIDERATIONS
//
//Anti-aliasing algorithms currently only work for source area that is smaller than
//the dest area.  Also, neither the source width or height may be less than half
//of the dest width or height.  An assertian will fire if an unsupported ratio is
//used.  This limitation can be removed by writing new anti-aliasing code to handle
//other ratios.

#include <BackEndLib/Types.h>
#include "FrameWidget.h"
#include "Widget.h"

//******************************************************************************
class CScalerWidget : public CWidget
{     
public:
	CScalerWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
			const UINT wSetW, const UINT wSetH, const bool bShowQuick=true);
	virtual  ~CScalerWidget();

	CWidget *         AddScaledWidget(CWidget *pNewWidget, bool bLoad = false);
	int               GetScaledW(const int nTrueW) const;
	int               GetScaledH(const int nTrueH) const;
	int               GetScaledX(const int nTrueX) const;
	int               GetScaledY(const int nTrueY) const;
	virtual bool   Load();
	virtual void   Paint(bool bUpdateRect = true);
	virtual void   PaintClipped(const int nX, const int nY, const UINT wW,
			const UINT wH, const bool bUpdateRect = true);
	void           RefreshSize();
	virtual void   Resize(const UINT wSetW, const UINT wSetH);
	void           ShowQuickPass(const bool bQuick) {this->bShowQuick = bQuick;}
	virtual void   Unload();

protected:
	virtual void   HandleAnimate();
	virtual bool   IsAnimated() const {return true;}

private:
	bool           CalcScaleInstructions();
	bool           CreateNewTrueScaleSurface();
	UINT           DrawAntiAliasedLines(const int nStartY, const UINT dwMaxTime=0);
	void           DrawScaledQuick();
	void           SetTrueScaleContainerSize();

	CFrameWidget *    pTrueScaleContainer;

	bool           bNewScaleDimensions;
	SDL_Surface *     pTrueScaleSurface;
	Uint8 * *         ppTrueToDestScaleMap;

	bool           bShowQuick; //whether to show quick first pass while rendering
	int            nAntiAliasY;
	bool           bAntiAliasInProgress;
};

#endif //#ifndef SCALERWIDGET_H

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
 * Portions created by the Initial Developer are Copyright (C) 2003, 2005
 * Caravel Software. All Rights Reserved.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef STACKWIDGET_H
#define STACKWIDGET_H

#include "FocusWidget.h"
#include <BackEndLib/Types.h>
#include <BackEndLib/Assert.h>

//*********************************************************************************************************
// This is just a container to glue together multiple widgets.
//
// It stacks them either vertically or horizontally.
//
// If, for instance, you stack vertically, then the horizontal size is completely based on the elements
//  added to it.  The vertical size is based on the elements added, but limited by nMaxSize.  If the elements
//  added surpass that maximum, a scrollbar will be added for just that direction.
//
// These can be nested to great effect, letting you easily lay out, for instance, a group of labels & form inputs
class CStackWidget : public CFocusWidget
{
public:
	CStackWidget(UINT dwSetTagNo, int nSetX, int nSetY, int nMaxSize, int nPadding, bool bVertical);

	virtual void   Paint(bool bUpdateRect = true);
	virtual void   AddWidget(CWidget* pWidget);

protected:
	virtual void   HandleAnimate();
	virtual bool   IsAnimated() const {return true;}

	void DrawVertScrollBar(SDL_Surface *pDestSurface);
	virtual void HandleMouseWheel(const SDL_MouseWheelEvent &MouseWheelEvent);

private:
	vector<CWidget*> widgets;
	bool bIsVertical;
	int nMaxSize;
	int nTotalSize;
	int nPadding;

	int nCurOffset;
};

#endif //#ifndef PROGRESSBARWIDGET_H

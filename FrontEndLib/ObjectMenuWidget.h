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

#ifndef OBJECTMENUWIDGET_H
#define OBJECTMENUWIDGET_H

#include "FocusWidget.h"
#include <list>

//To display one object on the ObjectMenuWidget.
//For example, an object might be a wall, depicted by a 2x2 wall tile section.
class MENUOBJECT
{
public:
	MENUOBJECT(const UINT wObjectNo, const UINT wXSize, const UINT wYSize, const UINT* pTiles,
			const float fR, const float fG, const float fB);
	~MENUOBJECT();

	UINT wObjectNo;      //object id
	UINT wXSize, wYSize; //# tiles to show for display
	UINT *paTiles;       //the literal tiles to show
	float fR, fG, fB;    //optional additive light
	int nX, nY;          //relative position in widget
};

typedef std::list<MENUOBJECT *>::const_iterator OBJECT_ITERATOR;
typedef std::list<MENUOBJECT *>::reverse_iterator OBJECT_RITERATOR;

#define  NO_SELECTED_OBJECT   ((UINT)-1)

//******************************************************************************
class CObjectMenuWidget : public CFocusWidget
{     
public:
	CObjectMenuWidget(const UINT dwSetTagNo, const int nSetX,
			const int nSetY, const UINT wSetW, const UINT wSetH,
			const UINT wGapX, const UINT wGapY, const Uint32 BGColor);
	virtual ~CObjectMenuWidget();

	void           AddObject(const UINT wObjectNo, const UINT wXSize,
			const UINT wYSize, const UINT* paTiles,
			const float fR=0.0f, const float fG=0.0f, const float fB=0.0f);
	void           ChangeObject(const UINT wOldObjectNo, const UINT wNewObjectNo);
	void           ClearObjects();
	inline void    DrawBackground(const bool bVal) {this->bDrawBackground = bVal;}
	UINT           GetObjectIDAt(const UINT x, const UINT y) const;
	UINT           GetSelectedObject() const;

	virtual void   Paint(bool bUpdateRect = true);

	void           PopUp();

	void           SetObjectLight(const UINT wObjectNo,
			const float fR, const float fG, const float fB);
	void           SetObjectTiles(const UINT wObjectNo,
			const UINT wXSize, const UINT wYSize, const UINT* pTiles);
	void           SetSelectedObject(const UINT wObject);

protected:
	virtual void   HandleKeyDown(const SDL_KeyboardEvent &KeyboardEvent);
	virtual void   HandleMouseDown(const SDL_MouseButtonEvent &Button);

private:
	OBJECT_ITERATOR   GetObjectAt(const UINT x, const UINT y) const;
	void           SetSelectedObject(OBJECT_ITERATOR const &pSelectedObject);

	UINT                 wGapX, wGapY;
	list<MENUOBJECT *>   lObjects;   //objects added to the menu
	OBJECT_ITERATOR      iterSelectedObject;

	//To erase previous selection graphic
	OBJECT_ITERATOR      iterPrevSelectedObject;
	bool                 bSelectionDrawn;
	Uint32					BGColor;
	bool                 bDrawBackground;
};

#endif //#ifndef OBJECTMENUWIDGET_H

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

#ifndef FOCUSWIDGET_H
#define FOCUSWIDGET_H

#include "Widget.h"
#include <BackEndLib/Types.h>

//******************************************************************************
class CFocusWidget : public CWidget
{
public:
	CFocusWidget(WIDGETTYPE eSetType, UINT dwSetTagNo, int nSetX, int nSetY, UINT wSetW, UINT wSetH);

	virtual void   Disable();
	virtual void   Hide(const bool bPaint = true);
	virtual bool   IsFocusable() const {return this->bFocusingAllowed;}
	bool  IsSelected() const {return this->bSelected;}
	void  Select(const bool bPaint = true);
	void  SetFocusAllowed(const bool bVal = true);
	void  Unselect(const bool bPaint = true);
	
private:
	bool  bSelected;
	bool  bFocusingAllowed;
};

#endif //#ifndef FOCUSWIDGET_H

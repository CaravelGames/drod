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

#ifndef PROGRESSBARWIDGET_H
#define PROGRESSBARWIDGET_H

#include "Widget.h"
#include <BackEndLib/Types.h>
#include <BackEndLib/Assert.h>

static const UINT CY_STANDARD_PROGRESSBAR = 30;

//*********************************************************************************************************
class CProgressBarWidget : public CWidget
{
public:
	CProgressBarWidget(UINT dwSetTagNo, int nSetX, int nSetY, UINT wSetW, UINT wSetH, float fSetValue);

	float          GetValue() const {return this->fValue;}
	virtual void   Paint(bool bUpdateRect = true);
	void           SetValue(const float fSetValue);

protected:
	void				DrawBar();
	virtual void   HandleAnimate();
	virtual bool   IsAnimated() const {return true;}

private:
	float           fValue, fLastDrawnValue;	//progress
};

#endif //#ifndef PROGRESSBARWIDGET_H

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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005, 2020
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Mike Rimer (mrimer)
 * Maurycy Zarzycki (skell)
 *
 * ***** END LICENSE BLOCK ***** */

 //This is a CDialog that has been augmented to show a list of level entrances.

#ifndef PUZZLEMODEOPTIONSDIALOGWIDGET_H
#define PUZZLEMODEOPTIONSDIALOGWIDGET_H

#include <FrontEndLib/DialogWidget.h>
#include <set>

class CDbHold;
class CEntranceData;
class CLabelWidget;
class CListBoxWidget;
class CCurrentGame;
class CPuzzleModeOptionsDialogWidget : public CDialogWidget
{
public:
	CPuzzleModeOptionsDialogWidget(const UINT dwSetTagNo, const int nSetX = 0,
		const int nSetY = 0);

	virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDeactivate();
	virtual bool   SetForActivate();

private:
	CLabelWidget* pHeaderLabel;
};

#endif   //PUZZLEMODEOPTIONSDIALOGWIDGET_H

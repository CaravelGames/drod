// $Id: WeatherDialogWidget.h 8102 2007-08-15 14:55:40Z trick $

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

//This is a CDialog that has been augmented to customize weather conditions.

#ifndef WEATHERDIALOGWIDGET_H
#define WEATHERDIALOGWIDGET_H

#include <FrontEndLib/DialogWidget.h>

class CDbRoom;
class CSliderWidget;
class COptionButtonWidget;
class CWeatherDialogWidget : public CDialogWidget
{
public:
	CWeatherDialogWidget(const UINT dwSetTagNo, const int nSetX=0, const int nSetY=0);

	void SetRoom(CDbRoom*const pRoom);
	void UpdateRoom(CDbRoom*const pRoom);

private:
   virtual void OnClick(const UINT dwTagNo);

	void PopulateSkyList();

	void SetWidgetStates();
	void UpdateRoom();

	CDbRoom *pRoom;       //room being edited
};

#endif   //WEATHERDIALOGWIDGET_H

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
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "WeatherDialogWidget.h"

#include "DrodFontManager.h"
#include "DrodScreen.h"
#include "RoomWidget.h"
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/OptionButtonWidget.h>
#include <FrontEndLib/SliderWidget.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/DbRooms.h"
#include "../DRODLib/SettingsKeys.h"
#include <BackEndLib/Files.h>

//Widget tag constants.  These must be different than tags on the parent widget.
const UINT TAG_OUTSIDE = 1101;
const UINT TAG_LIGHTNING = 1102;
const UINT TAG_CLOUDS = 1103;
const UINT TAG_SUNSHINE = 1104;
const UINT TAG_LIGHTFADE = 1105;

const UINT TAG_FOG = 1110;
const UINT TAG_LIGHT = 1111;
const UINT TAG_SNOW = 1112;
const UINT TAG_RAIN = 1113;

const UINT TAG_SKYIMAGE = 1120;

const UINT TAG_CANCEL = 1191;

#ifdef RUSSIAN_BUILD
const UINT CX_DIALOG = 650;
#else
const UINT CX_DIALOG = 600;
#endif
const UINT CY_DIALOG = 400;

const UINT CX_SPACE = 20;
const UINT CY_SPACE = 10;

//*****************************************************************************
CWeatherDialogWidget::CWeatherDialogWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget
	const int nSetX, const int nSetY)         //    constructor.
	: CDialogWidget(dwSetTagNo, nSetX, nSetY, CX_DIALOG, CY_DIALOG)
	, pRoom(NULL)
{
	static const UINT CX_TITLE = 240;
	static const UINT CY_TITLE = 30;
	static const int X_TITLE = (CX_DIALOG - CX_TITLE) / 2;
	static const int Y_TITLE = CY_SPACE;

#ifdef RUSSIAN_BUILD
	static const UINT CX_OKAY_BUTTON = 200;
	static const UINT CX_OKAYLEVEL_BUTTON = 260;
	static const int X_OKAY_BUTTON = 20;
#else
	static const UINT CX_OKAY_BUTTON = 140;
	static const UINT CX_OKAYLEVEL_BUTTON = 190;
	static const int X_OKAY_BUTTON = 60;
#endif

	static const UINT CY_OKAY_BUTTON = CY_STANDARD_BUTTON;
	static const UINT CY_OKAYLEVEL_BUTTON = CY_OKAY_BUTTON;
	static const UINT CX_CANCEL_BUTTON = 110;
	static const UINT CY_CANCEL_BUTTON = CY_OKAY_BUTTON;
	static const int X_OKAYLEVEL_BUTTON = X_OKAY_BUTTON + CX_OKAY_BUTTON + CX_SPACE;
	static const int X_CANCEL_BUTTON = X_OKAYLEVEL_BUTTON + CX_OKAYLEVEL_BUTTON + CX_SPACE;
	static const int Y_OKAY_BUTTON = CY_DIALOG - CY_OKAY_BUTTON - 2*CY_SPACE;
	static const int Y_OKAYLEVEL_BUTTON = Y_OKAY_BUTTON;
	static const int Y_CANCEL_BUTTON = Y_OKAY_BUTTON;

	//Two-column format.
	static const int Y_BODY = Y_TITLE + CY_TITLE + CY_SPACE;
	static const int X_LEFT = CX_SPACE;
	static const UINT CX_LEFT = (CX_DIALOG - X_LEFT) / 2;
	static const int X_RIGHTLABEL = X_LEFT + CX_LEFT;
#ifdef RUSSIAN_BUILD
	static const UINT CX_RIGHTLABEL = 140;
#else
	static const UINT CX_RIGHTLABEL = 120;
#endif
	static const int X_RIGHT = X_RIGHTLABEL + CX_RIGHTLABEL;
	static const UINT CX_RIGHT = CX_DIALOG - X_RIGHT - CX_SPACE;

	static const UINT CX_SKYLIST = CX_DIALOG - X_RIGHTLABEL - CX_SPACE;
	static const UINT CY_SKYLIST = 114; //5 items

#define SLIDER_Y(index) (Y_BODY+(index)*(CY_STANDARD_SLIDER+CY_SPACE))
#define OPTIONBUTTON_Y(index) (Y_BODY+(index)*(CY_STANDARD_OPTIONBUTTON+CY_SPACE))

	//
	//Add widgets to screen.
	//

	//Title.
	AddWidget(new CLabelWidget(0L, X_TITLE, Y_TITLE, CX_TITLE, CY_TITLE,
			F_Header, g_pTheDB->GetMessageText(MID_WeatherTitle)));

	//Weather conditions.
	UINT wIndex = 0;
	AddWidget(new COptionButtonWidget(TAG_OUTSIDE, X_LEFT, OPTIONBUTTON_Y(wIndex++),
			CX_LEFT, CY_STANDARD_OPTIONBUTTON, g_pTheDB->GetMessageText(MID_Outside)));
	AddWidget(new COptionButtonWidget(TAG_LIGHTNING, X_LEFT, OPTIONBUTTON_Y(wIndex++),
			CX_LEFT, CY_STANDARD_OPTIONBUTTON, g_pTheDB->GetMessageText(MID_Lightning)));
	AddWidget(new COptionButtonWidget(TAG_CLOUDS, X_LEFT, OPTIONBUTTON_Y(wIndex++),
			CX_LEFT, CY_STANDARD_OPTIONBUTTON, g_pTheDB->GetMessageText(MID_Clouds)));
	AddWidget(new COptionButtonWidget(TAG_SUNSHINE, X_LEFT, OPTIONBUTTON_Y(wIndex++),
			CX_LEFT, CY_STANDARD_OPTIONBUTTON, g_pTheDB->GetMessageText(MID_Sunshine)));
	AddWidget(new COptionButtonWidget(TAG_LIGHTFADE, X_LEFT, OPTIONBUTTON_Y(wIndex++),
			CX_LEFT, CY_STANDARD_OPTIONBUTTON, g_pTheDB->GetMessageText(MID_LightCrossfade)));

	//Sliders.
	wIndex=0;
	AddWidget(new CLabelWidget(0L, X_RIGHTLABEL, SLIDER_Y(wIndex), CX_RIGHTLABEL,
			CY_STANDARD_SLIDER, F_Button, g_pTheDB->GetMessageText(MID_AmbientLight)));
	AddWidget(new CSliderWidget(TAG_LIGHT, X_RIGHT, SLIDER_Y(wIndex++),
			CX_RIGHT, CY_STANDARD_SLIDER, 0, LIGHT_LEVELS));
	AddWidget(new CLabelWidget(0L, X_RIGHTLABEL, SLIDER_Y(wIndex), CX_RIGHTLABEL,
			CY_STANDARD_SLIDER, F_Button, g_pTheDB->GetMessageText(MID_Fog)));
	AddWidget(new CSliderWidget(TAG_FOG, X_RIGHT, SLIDER_Y(wIndex++),
			CX_RIGHT, CY_STANDARD_SLIDER, 0, FOG_INCREMENTS));
	AddWidget(new CLabelWidget(0L, X_RIGHTLABEL, SLIDER_Y(wIndex), CX_RIGHTLABEL,
			CY_STANDARD_SLIDER, F_Button, g_pTheDB->GetMessageText(MID_Snow)));
	AddWidget(new CSliderWidget(TAG_SNOW, X_RIGHT, SLIDER_Y(wIndex++),
			CX_RIGHT, CY_STANDARD_SLIDER, 0, SNOW_INCREMENTS));
	AddWidget(new CLabelWidget(0L, X_RIGHTLABEL, SLIDER_Y(wIndex), CX_RIGHTLABEL,
			CY_STANDARD_SLIDER, F_Button, g_pTheDB->GetMessageText(MID_Rain)));
	AddWidget(new CSliderWidget(TAG_RAIN, X_RIGHT, SLIDER_Y(wIndex++),
			CX_RIGHT, CY_STANDARD_SLIDER, 0, RAIN_INCREMENTS));

	//Sky list.
	CListBoxWidget *pSkyList = new CListBoxWidget(TAG_SKYIMAGE,
			X_RIGHTLABEL, SLIDER_Y(wIndex), CX_SKYLIST, CY_SKYLIST, true);
	AddWidget(pSkyList);
	PopulateSkyList();

	//Okay and cancel buttons.
	CButtonWidget *pButton;
	pButton = new CButtonWidget(TAG_OK, X_OKAY_BUTTON, Y_OKAY_BUTTON,
				CX_OKAY_BUTTON, CY_OKAY_BUTTON, g_pTheDB->GetMessageText(MID_ApplyToRoom));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_OK_WHOLELEVEL, X_OKAYLEVEL_BUTTON, Y_OKAYLEVEL_BUTTON,
				CX_OKAYLEVEL_BUTTON, CY_OKAYLEVEL_BUTTON, g_pTheDB->GetMessageText(MID_ApplyToLevel));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_CANCEL, X_CANCEL_BUTTON, Y_CANCEL_BUTTON,
				CX_CANCEL_BUTTON, CY_CANCEL_BUTTON, g_pTheDB->GetMessageText(MID_Cancel));
	AddWidget(pButton);
	AddHotkey(SDLK_ESCAPE,TAG_CANCEL); 
}

//*****************************************************************************
void CWeatherDialogWidget::PopulateSkyList()
//Fills list with all sky images.
{
	CListBoxWidget *pList = DYN_CAST(CListBoxWidget*, CWidget*, GetWidget(TAG_SKYIMAGE));
	pList->Clear();

	//Default option.
	WSTRING wstrDefault = wszLeftParen;
	wstrDefault += g_pTheDB->GetMessageText(MID_Default);
	wstrDefault += wszRightParen;
	pList->AddItem(0, wstrDefault.c_str());

	//Get names of all room styles.
	list<WSTRING> styles;
	if (!CFiles::GetGameProfileString(INISection::Graphics, INIKey::Style, styles))
		return;	//styles are missing

	set<WSTRING> parsedImages;
	UINT count=0;
	while (styles.size())
	{
		//Parse a room style.
		WSTRING styleName = styles.front();
		styles.pop_front();

		//Get this style's base filename from the INI.
		//ATTN: Gets the first name only.
		list<WSTRING> styleBasenames;
		if (!CFiles::GetGameProfileString(INISection::Graphics, styleName.c_str(), styleBasenames))
			continue;	//this style is missing
		if (styleBasenames.empty())
			continue;   //style name not specified
		WSTRING wstrBasename = styleBasenames.front();

		//Get style's sky images.
		WSTRING wstrSkyname = wstrBasename;
		wstrSkyname += wszSpace;
		wstrSkyname += wszSKIES;
		list<WSTRING> skies;
		if (CFiles::GetGameProfileString(INISection::Graphics, wstrSkyname.c_str(), skies))
			for (list<WSTRING>::const_iterator sky = skies.begin(); sky != skies.end(); ++sky)
			{
				const WSTRING& skyName = *sky;
				if (parsedImages.count(skyName) != 0)
					continue; //this image was already processed -- don't add it again
				parsedImages.insert(skyName);
				pList->AddItem(++count, skyName.c_str());
			}
	}
}

//*****************************************************************************
void CWeatherDialogWidget::SetRoom(CDbRoom*const pRoom)
{
	ASSERT(pRoom);
	ASSERT(!this->pRoom);
	this->pRoom = pRoom;

	SetWidgetStates();
}

//*****************************************************************************
void CWeatherDialogWidget::OnClick(
//Handles click event.
//
//Params:
	const UINT dwTagNo)          //(in)   Widget that received event.
{
	switch (dwTagNo)
	{
		case TAG_OK:
		case TAG_OK_WHOLELEVEL:
			UpdateRoom();
		break;
	}

	CDialogWidget::OnClick(dwTagNo); //catch deactivate

	//Release room on exit.
	if (IsDeactivating())
		this->pRoom = NULL;
}

//*****************************************************************************
void CWeatherDialogWidget::SetWidgetStates()
//Set widget states on dialog from room state.
{
	ASSERT(this->pRoom);

	// Draw before setting widgets state, as some of them may cache what's drawn underneath them
	this->RequestPaint(true);

	COptionButtonWidget *pOptionButton;
	CSliderWidget *pSliderWidget;

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_OUTSIDE));
	pOptionButton->SetChecked(this->pRoom->weather.bOutside);

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_LIGHTNING));
	pOptionButton->SetChecked(this->pRoom->weather.bLightning);

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_CLOUDS));
	pOptionButton->SetChecked(this->pRoom->weather.bClouds);

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_SUNSHINE));
	pOptionButton->SetChecked(this->pRoom->weather.bSunshine);

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_LIGHTFADE));
	pOptionButton->SetChecked(this->pRoom->weather.bSkipLightfade);

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_FOG));
	pSliderWidget->SetValue(this->pRoom->weather.wFog);

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_LIGHT));
	pSliderWidget->SetValue(this->pRoom->weather.wLight);

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_SNOW));
	pSliderWidget->SetValue(this->pRoom->weather.wSnow);

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_RAIN));
	pSliderWidget->SetValue(this->pRoom->weather.rain);

	CListBoxWidget *pList = DYN_CAST(CListBoxWidget*, CWidget*, GetWidget(TAG_SKYIMAGE));
	if (this->pRoom->weather.sky.empty())
		pList->SelectItem(0); //select default
	else
		pList->SelectLineWithText(this->pRoom->weather.sky.c_str());
}

//*****************************************************************************
void CWeatherDialogWidget::UpdateRoom(CDbRoom*const pRoom)
//Set a room's weather state from widget states on dialog.
{
	ASSERT(pRoom);

	COptionButtonWidget *pOptionButton;
	CSliderWidget *pSliderWidget;

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_OUTSIDE));
	pRoom->weather.bOutside = pOptionButton->IsChecked();

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_LIGHTNING));
	pRoom->weather.bLightning = pOptionButton->IsChecked();

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_CLOUDS));
	pRoom->weather.bClouds = pOptionButton->IsChecked();

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_SUNSHINE));
	pRoom->weather.bSunshine = pOptionButton->IsChecked();

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_LIGHTFADE));
	pRoom->weather.bSkipLightfade = pOptionButton->IsChecked();

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_FOG));
	pRoom->weather.wFog = pSliderWidget->GetValue();

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_LIGHT));
	pRoom->weather.wLight = pSliderWidget->GetValue();

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_SNOW));
	pRoom->weather.wSnow = pSliderWidget->GetValue();

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_RAIN));
	pRoom->weather.rain = pSliderWidget->GetValue();

	CListBoxWidget *pList = DYN_CAST(CListBoxWidget*, CWidget*, GetWidget(TAG_SKYIMAGE));
	if (pList->GetSelectedItem() == 0)
		pRoom->weather.sky.resize(0); //default
	else
		pRoom->weather.sky = pList->GetSelectedItemText();
}

//*****************************************************************************
void CWeatherDialogWidget::UpdateRoom()
//Set the active room's weather state from widget states on dialog.
{
	ASSERT(this->pRoom);
	UpdateRoom(this->pRoom);
}

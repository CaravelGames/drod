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

#include "DrodFontManager.h"
#include "PuzzleModeOptionsDialogWidget.h"

#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/OptionButtonWidget.h>
#include <FrontEndLib/SliderWidget.h>

#include <DRODLib/Db.h>
#include <DRODLib/SettingsKeys.h>

 //NOTE: tag #'s should not conflict with other widgets on screen
const UINT TAG_GRID_STYLE = 2100;
const UINT TAG_GRID_OPACITY = 2101;

const UINT TAG_VISIBILITY_SPIDERS = 2102;
const UINT TAG_VISIBILITY_EYE_BEAMS = 2103;
const UINT TAG_VISIBILITY_REVERSE_EYE_BEAMS = 2104;
const UINT TAG_VISIBILITY_SECRET_WALLS = 2105;
const UINT TAG_VISIBILITY_HIDE_WEATHER = 2106;
const UINT TAG_VISIBILITY_HIDE_LIGHTING = 2107;
const UINT TAG_VISIBILITY_HIDE_ANIMATIONS = 2108;
const UINT TAG_VISIBILITY_HIDE_BUILD_MARKERS = 2109;
const UINT TAG_VISIBILITY_BROKEN_WALLS = 2110;

static const UINT CX_SPACE = 15;
static const UINT CY_SPACE = 10;

const UINT CX_DIALOG = 800;
const UINT CY_DIALOG = (
	CY_SPACE // Top padding
	+ CY_LABEL_FONT_HEADER // Header height
	+ 2 * CY_SPACE // Header bottom-marging
	+ CY_STANDARD_OPTIONBUTTON * 9 + CY_SPACE * 10 // Visibility frame height
	+ CY_SPACE // Visibility frame bottom margin
	+ CY_STANDARD_BUTTON // OKay button height
	+ CY_SPACE // Bottom padding
	);

//*****************************************************************************
CPuzzleModeOptionsDialogWidget::CPuzzleModeOptionsDialogWidget(
	//Constructor.
	//
	//Params:
	const UINT dwSetTagNo, const int nSetX, const int nSetY) //(in)   Required params for CWidget constructor.
	: CDialogWidget(dwSetTagNo, nSetX, nSetY, CX_DIALOG, CY_DIALOG, false)   //double-click on list box disables
	, pHeaderLabel(NULL)
{
	
	static const UINT HALF_W = (CX_DIALOG - CX_SPACE * 3) / 2;
	static const UINT HALF_X = CX_SPACE * 2 + HALF_W;
	static const UINT FRAME_INNER_W = HALF_W - CX_SPACE * 2;
	static const UINT FRAME_HALF_W = (FRAME_INNER_W - CX_SPACE) / 2;

	static const int HEADER_X = 0;
	static const int HEADER_Y = CY_SPACE;
	static const UINT HEADER_W = CX_DIALOG;
	static const UINT HEADER_H = CY_LABEL_FONT_HEADER;
	static const UINT HEADER_BOTTOM = HEADER_Y + HEADER_H + CY_SPACE * 2;

	static const UINT BUTTON_W = 70;
	static const int OK_BUTTON_X = (CX_DIALOG - BUTTON_W) / 2;
	static const int OK_BUTTON_Y = CY_DIALOG - CY_STANDARD_BUTTON - CY_SPACE;

	static const int GRID_FRAME_X = CX_SPACE;
	static const int GRID_FRAME_Y = HEADER_BOTTOM;
	static const int GRID_FRAME_W = HALF_W;
	static const int GRID_FRAME_H = CY_STANDARD_SLIDER * 2 + CY_SPACE * 3;

	static const int GRID_STYLE_LABEL_X = CX_SPACE;
	static const int GRID_STYLE_LABEL_Y = CY_SPACE;
	static const int GRID_STYLE_LABEL_W = FRAME_HALF_W;
	static const int GRID_STYLE_LABEL_H = CY_STANDARD_SLIDER;
	static const int GRID_STYLE_LABEL_BOTTOM = GRID_STYLE_LABEL_Y + GRID_STYLE_LABEL_H + CY_SPACE;
	static const int GRID_STYLE_SLIDER_X = CX_SPACE * 2 + FRAME_HALF_W;
	static const int GRID_STYLE_SLIDER_Y = CY_SPACE;
	static const int GRID_STYLE_SLIDER_W = FRAME_HALF_W;
	static const int GRID_STYLE_SLIDER_H = CY_STANDARD_SLIDER;

	static const int GRID_OPACITY_LABEL_X = CX_SPACE;
	static const int GRID_OPACITY_LABEL_Y = GRID_STYLE_LABEL_BOTTOM;
	static const int GRID_OPACITY_LABEL_W = FRAME_HALF_W;
	static const int GRID_OPACITY_LABEL_H = CY_STANDARD_SLIDER;
	static const int GRID_OPACITY_SLIDER_X = CX_SPACE * 2 + FRAME_HALF_W;
	static const int GRID_OPACITY_SLIDER_Y = GRID_STYLE_LABEL_BOTTOM;
	static const int GRID_OPACITY_SLIDER_W = FRAME_HALF_W;
	static const int GRID_OPACITY_SLIDER_H = CY_STANDARD_SLIDER;


	static const int VISIBILITY_FRAME_X = HALF_X;
	static const int VISIBILITY_FRAME_Y = HEADER_BOTTOM;
	static const int VISIBILITY_FRAME_W = HALF_W;
	static const int VISIBILITY_FRAME_H = CY_STANDARD_OPTIONBUTTON * 9 + CY_SPACE * 10;

	static const int VIS_HIDE_ANIMATIONS_X = CX_SPACE;
	static const int VIS_HIDE_ANIMATIONS_Y = CY_SPACE;
	static const int VIS_HIDE_ANIMATIONS_W = FRAME_INNER_W;
	static const int VIS_HIDE_ANIMATIONS_H = CY_STANDARD_OPTIONBUTTON;
	static const int VIS_HIDE_ANIMATIONS_BOTTOM = VIS_HIDE_ANIMATIONS_Y + VIS_HIDE_ANIMATIONS_H + CY_SPACE;

	static const int VIS_HIDE_BUILD_MARKERS_X = CX_SPACE;
	static const int VIS_HIDE_BUILD_MARKERS_Y = VIS_HIDE_ANIMATIONS_BOTTOM;
	static const int VIS_HIDE_BUILD_MARKERS_W = FRAME_INNER_W;
	static const int VIS_HIDE_BUILD_MARKERS_H = CY_STANDARD_OPTIONBUTTON;
	static const int VIS_HIDE_BUILD_MARKERS_BOTTOM = VIS_HIDE_BUILD_MARKERS_Y + VIS_HIDE_BUILD_MARKERS_H + CY_SPACE;

	static const int VIS_HIDE_LIGHTING_X = CX_SPACE;
	static const int VIS_HIDE_LIGHTING_Y = VIS_HIDE_BUILD_MARKERS_BOTTOM;
	static const int VIS_HIDE_LIGHTING_W = FRAME_INNER_W;
	static const int VIS_HIDE_LIGHTING_H = CY_STANDARD_OPTIONBUTTON;
	static const int VIS_HIDE_LIGHTING_BOTTOM = VIS_HIDE_LIGHTING_Y + VIS_HIDE_LIGHTING_H + CY_SPACE;

	static const int VIS_HIDE_WEATHER_X = CX_SPACE;
	static const int VIS_HIDE_WEATHER_Y = VIS_HIDE_LIGHTING_BOTTOM;
	static const int VIS_HIDE_WEATHER_W = FRAME_INNER_W;
	static const int VIS_HIDE_WEATHER_H = CY_STANDARD_OPTIONBUTTON;
	static const int VIS_HIDE_WEATHER_BOTTOM = VIS_HIDE_WEATHER_Y + VIS_HIDE_WEATHER_H + CY_SPACE;

	static const int VIS_BROKEN_WALLS_X = CX_SPACE;
	static const int VIS_BROKEN_WALLS_Y = VIS_HIDE_WEATHER_BOTTOM;
	static const int VIS_BROKEN_WALLS_W = FRAME_INNER_W;
	static const int VIS_BROKEN_WALLS_H = CY_STANDARD_OPTIONBUTTON;
	static const int VIS_BROKEN_WALLS_BOTTOM = VIS_BROKEN_WALLS_Y + VIS_BROKEN_WALLS_H + CY_SPACE;

	static const int VIS_SECRET_WALLS_X = CX_SPACE;
	static const int VIS_SECRET_WALLS_Y = VIS_BROKEN_WALLS_BOTTOM;
	static const int VIS_SECRET_WALLS_W = FRAME_INNER_W;
	static const int VIS_SECRET_WALLS_H = CY_STANDARD_OPTIONBUTTON;
	static const int VIS_SECRET_WALLS_BOTTOM = VIS_SECRET_WALLS_Y + VIS_SECRET_WALLS_H + CY_SPACE;

	static const int VIS_EYE_BEAMS_X = CX_SPACE;
	static const int VIS_EYE_BEAMS_Y = VIS_SECRET_WALLS_BOTTOM;
	static const int VIS_EYE_BEAMS_W = FRAME_INNER_W;
	static const int VIS_EYE_BEAMS_H = CY_STANDARD_OPTIONBUTTON;
	static const int VIS_EYE_BEAMS_BOTTOM = VIS_EYE_BEAMS_Y + VIS_EYE_BEAMS_H + CY_SPACE;

	static const int VIS_EYE_BEAMS_REVERSE_X = CX_SPACE;
	static const int VIS_EYE_BEAMS_REVERSE_Y = VIS_EYE_BEAMS_BOTTOM;
	static const int VIS_EYE_BEAMS_REVERSE_W = FRAME_INNER_W;
	static const int VIS_EYE_BEAMS_REVERSE_H = CY_STANDARD_OPTIONBUTTON;
	static const int VIS_EYE_BEAMS_REVERSE_BOTTOM = VIS_EYE_BEAMS_REVERSE_Y + VIS_EYE_BEAMS_REVERSE_H + CY_SPACE;

	static const int VIS_SPIDERS_X = CX_SPACE;
	static const int VIS_SPIDERS_Y = VIS_EYE_BEAMS_REVERSE_BOTTOM;
	static const int VIS_SPIDERS_W = FRAME_INNER_W;
	static const int VIS_SPIDERS_H = CY_STANDARD_OPTIONBUTTON;
	static const int VIS_SPIDERS_BOTTOM = VIS_SPIDERS_Y + VIS_SPIDERS_H + CY_SPACE;

	CLabelWidget* pLabelWidget = NULL;
	CSliderWidget* pSliderWidget = NULL;
	CFrameWidget* pFrameWidget = NULL;
	COptionButtonWidget* pOptionWidget = NULL;

	this->pHeaderLabel = new CLabelWidget(0L, HEADER_X, HEADER_Y, HEADER_W, HEADER_H,
		F_Header, g_pTheDB->GetMessageText(MID_PuzzleModeOption_Header));
	this->pHeaderLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(this->pHeaderLabel);

	
	{ // GRID STYLE
		pFrameWidget = new CFrameWidget(0L,
			GRID_FRAME_X, GRID_FRAME_Y, GRID_FRAME_W, GRID_FRAME_H,
			g_pTheDB->GetMessageText(MID_PuzzleModeOption_Frame_GridOptions));
		AddWidget(pFrameWidget);

		pLabelWidget = new CLabelWidget(0L,
			GRID_STYLE_LABEL_X, GRID_STYLE_LABEL_Y, GRID_STYLE_LABEL_W, GRID_STYLE_LABEL_H,
			F_Small, g_pTheDB->GetMessageText(MID_PuzzleModeOption_GridStyle));
		pFrameWidget->AddWidget(pLabelWidget);
		pSliderWidget = new CSliderWidget(TAG_GRID_STYLE,
			GRID_STYLE_SLIDER_X, GRID_STYLE_SLIDER_Y, GRID_STYLE_SLIDER_W, GRID_STYLE_SLIDER_H,
			0, 5);
		pSliderWidget->SetDrawTickMarks(true);
		pFrameWidget->AddWidget(pSliderWidget);

		pLabelWidget = new CLabelWidget(0L,
			GRID_OPACITY_LABEL_X, GRID_OPACITY_LABEL_Y, GRID_OPACITY_LABEL_W, GRID_OPACITY_LABEL_H,
			F_Small, g_pTheDB->GetMessageText(MID_PuzzleModeOption_GridOpacity));
		pFrameWidget->AddWidget(pLabelWidget);
		pSliderWidget = new CSliderWidget(TAG_GRID_OPACITY,
			GRID_OPACITY_SLIDER_X, GRID_OPACITY_SLIDER_Y, GRID_OPACITY_SLIDER_W, GRID_OPACITY_SLIDER_H,
			0);
		pFrameWidget->AddWidget(pSliderWidget);
	}

	{ // VISIBILITY
		pFrameWidget = new CFrameWidget(0L,
			VISIBILITY_FRAME_X, VISIBILITY_FRAME_Y, VISIBILITY_FRAME_W, VISIBILITY_FRAME_H,
			g_pTheDB->GetMessageText(MID_PuzzleModeOption_Frame_Visibility));
		AddWidget(pFrameWidget);

		pOptionWidget = new COptionButtonWidget(TAG_VISIBILITY_HIDE_ANIMATIONS,
			VIS_HIDE_ANIMATIONS_X, VIS_HIDE_ANIMATIONS_Y, VIS_HIDE_ANIMATIONS_W, VIS_HIDE_ANIMATIONS_H,
			g_pTheDB->GetMessageText(MID_PuzzleModeOption_HideAnimations));
		pFrameWidget->AddWidget(pOptionWidget);

		pOptionWidget = new COptionButtonWidget(TAG_VISIBILITY_HIDE_BUILD_MARKERS,
			VIS_HIDE_BUILD_MARKERS_X, VIS_HIDE_BUILD_MARKERS_Y, VIS_HIDE_BUILD_MARKERS_W, VIS_HIDE_BUILD_MARKERS_H,
			g_pTheDB->GetMessageText(MID_PuzzleModeOption_HideBuildMarkers));
		pFrameWidget->AddWidget(pOptionWidget);

		pOptionWidget = new COptionButtonWidget(TAG_VISIBILITY_HIDE_LIGHTING,
			VIS_HIDE_LIGHTING_X, VIS_HIDE_LIGHTING_Y, VIS_HIDE_LIGHTING_W, VIS_HIDE_LIGHTING_H,
			g_pTheDB->GetMessageText(MID_PuzzleModeOption_HideLighting));
		pFrameWidget->AddWidget(pOptionWidget);

		pOptionWidget = new COptionButtonWidget(TAG_VISIBILITY_HIDE_WEATHER,
			VIS_HIDE_WEATHER_X, VIS_HIDE_WEATHER_Y, VIS_HIDE_WEATHER_W, VIS_HIDE_WEATHER_H,
			g_pTheDB->GetMessageText(MID_PuzzleModeOption_HideWeather));
		pFrameWidget->AddWidget(pOptionWidget);

		pOptionWidget = new COptionButtonWidget(TAG_VISIBILITY_BROKEN_WALLS,
			VIS_BROKEN_WALLS_X, VIS_BROKEN_WALLS_Y, VIS_BROKEN_WALLS_W, VIS_BROKEN_WALLS_H,
			g_pTheDB->GetMessageText(MID_PuzzleModeOption_ShowBrokenWalls));
		pFrameWidget->AddWidget(pOptionWidget);

		pOptionWidget = new COptionButtonWidget(TAG_VISIBILITY_SECRET_WALLS,
			VIS_SECRET_WALLS_X, VIS_SECRET_WALLS_Y, VIS_SECRET_WALLS_W, VIS_SECRET_WALLS_H,
			g_pTheDB->GetMessageText(MID_PuzzleModeOption_ShowSecretWalls));
		pFrameWidget->AddWidget(pOptionWidget);

		pOptionWidget = new COptionButtonWidget(TAG_VISIBILITY_EYE_BEAMS,
			VIS_EYE_BEAMS_X, VIS_EYE_BEAMS_Y, VIS_EYE_BEAMS_W, VIS_EYE_BEAMS_H,
			g_pTheDB->GetMessageText(MID_PuzzleModeOption_ShowEyeBeams));
		pFrameWidget->AddWidget(pOptionWidget);

		pOptionWidget = new COptionButtonWidget(TAG_VISIBILITY_REVERSE_EYE_BEAMS,
			VIS_EYE_BEAMS_REVERSE_X, VIS_EYE_BEAMS_REVERSE_Y, VIS_EYE_BEAMS_REVERSE_W, VIS_EYE_BEAMS_REVERSE_H,
			g_pTheDB->GetMessageText(MID_PuzzleModeOption_ShowEyeBeamsReverse));
		pFrameWidget->AddWidget(pOptionWidget);

		pOptionWidget = new COptionButtonWidget(TAG_VISIBILITY_SPIDERS,
			VIS_SPIDERS_X, VIS_SPIDERS_Y, VIS_SPIDERS_W, VIS_SPIDERS_H,
			g_pTheDB->GetMessageText(MID_PuzzleModeOption_ShowSpiders));
		pFrameWidget->AddWidget(pOptionWidget);

	}

	//OK button gets default focus
	CButtonWidget* pButton = new CButtonWidget(
		TAG_OK, OK_BUTTON_X, OK_BUTTON_Y, BUTTON_W, CY_STANDARD_BUTTON,
		g_pTheDB->GetMessageText(MID_Okay));
	AddWidget(pButton);
}

//*****************************************************************************
void CPuzzleModeOptionsDialogWidget::OnClick(
	const UINT dwTagNo)          //(in)   Widget that received event.
{
	if (!dwTagNo) return;

	CWidget* pWidget = GetWidget(dwTagNo);
	if (pWidget->GetType() == WT_Button)   //only buttons will return from dialog
	{
		switch (dwTagNo)
		{
		case TAG_OK: 
			Deactivate();
			break;
		}
	}
}

//*****************************************************************************
bool CPuzzleModeOptionsDialogWidget::SetForActivate()
{
	// Draw before setting widgets state, as some of them may cache what's drawn underneath them
	RequestPaint(true);

	CDbPackedVars settings = g_pTheDB->GetCurrentPlayerSettings();

	CSliderWidget* pSliderWidget = NULL;
	COptionButtonWidget* pOptionWidget = NULL;

	{ // GRID STYLE
		pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_GRID_STYLE));
		pSliderWidget->SetValue(settings.GetVar(Settings::PuzzleMode_GridStyle, 1U));

		pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_GRID_OPACITY));
		pSliderWidget->SetValue(settings.GetVar(Settings::PuzzleMode_GridOpacity, (Uint8)128));
	}

	{ // VISIBILITY
		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_HIDE_ANIMATIONS));
		pOptionWidget->SetChecked(settings.GetVar(Settings::PuzzleMode_VisibilityHideAnimations, false));

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_HIDE_BUILD_MARKERS));
		pOptionWidget->SetChecked(settings.GetVar(Settings::PuzzleMode_VisibilityHideBuildMarkers, false));

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_HIDE_LIGHTING));
		pOptionWidget->SetChecked(settings.GetVar(Settings::PuzzleMode_VisibilityHideLighting, false));

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_HIDE_WEATHER));
		pOptionWidget->SetChecked(settings.GetVar(Settings::PuzzleMode_VisibilityHideWeather, false));

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_EYE_BEAMS));
		pOptionWidget->SetChecked(settings.GetVar(Settings::PuzzleMode_VisibilityEyeBeams, true));

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_REVERSE_EYE_BEAMS));
		pOptionWidget->SetChecked(settings.GetVar(Settings::PuzzleMode_VisibilityReverseEyeBeams, false));

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_BROKEN_WALLS));
		pOptionWidget->SetChecked(settings.GetVar(Settings::PuzzleMode_VisibilityBrokenWalls, false));

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_SECRET_WALLS));
		pOptionWidget->SetChecked(settings.GetVar(Settings::PuzzleMode_VisibilitySecretWalls, false));

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_SPIDERS));
		pOptionWidget->SetChecked(settings.GetVar(Settings::PuzzleMode_VisibilitySpiders, true));

	}

	return true;
}

//*****************************************************************************
void CPuzzleModeOptionsDialogWidget::OnDeactivate()
{
	CDbPlayer* pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	if (!pCurrentPlayer) {
		ASSERT(!"CPuzzleModeOptionsDialogWidget::OnDeactivate(): Couldn't retrieve current player.");
		return; //Corrupt db.
	}

	CDbPackedVars& settings = pCurrentPlayer->Settings;

	CSliderWidget* pSliderWidget = NULL;
	COptionButtonWidget* pOptionWidget = NULL;

	{ // GRID STYLE
		pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_GRID_STYLE));
		settings.SetVar(Settings::PuzzleMode_GridStyle, (UINT)pSliderWidget->GetValue());
		
		pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_GRID_OPACITY));
		settings.SetVar(Settings::PuzzleMode_GridOpacity, (Uint8)pSliderWidget->GetValue());
	}

	{ // VISIBILITY
		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_HIDE_ANIMATIONS));
		settings.SetVar(Settings::PuzzleMode_VisibilityHideAnimations, pOptionWidget->IsChecked());

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_HIDE_BUILD_MARKERS));
		settings.SetVar(Settings::PuzzleMode_VisibilityHideBuildMarkers, pOptionWidget->IsChecked());

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_HIDE_LIGHTING));
		settings.SetVar(Settings::PuzzleMode_VisibilityHideLighting, pOptionWidget->IsChecked());

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_HIDE_WEATHER));
		settings.SetVar(Settings::PuzzleMode_VisibilityHideWeather, pOptionWidget->IsChecked());

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_EYE_BEAMS));
		settings.SetVar(Settings::PuzzleMode_VisibilityEyeBeams, pOptionWidget->IsChecked());

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_REVERSE_EYE_BEAMS));
		settings.SetVar(Settings::PuzzleMode_VisibilityReverseEyeBeams, pOptionWidget->IsChecked());

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_BROKEN_WALLS));
		settings.SetVar(Settings::PuzzleMode_VisibilityBrokenWalls, pOptionWidget->IsChecked());

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_SECRET_WALLS));
		settings.SetVar(Settings::PuzzleMode_VisibilitySecretWalls, pOptionWidget->IsChecked());

		pOptionWidget = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_VISIBILITY_SPIDERS));
		settings.SetVar(Settings::PuzzleMode_VisibilitySpiders, pOptionWidget->IsChecked());

	}

	pCurrentPlayer->Update();
	delete pCurrentPlayer;
}
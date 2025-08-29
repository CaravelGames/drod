// $Id: DrodFontManager.h 9776 2011-11-10 16:32:50Z mrimer $

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

#ifndef DRODFONTMANAGER_H
#define DRODFONTMANAGER_H

#include <FrontEndLib/FontManager.h>

//Set of all fonts.
enum FONTTYPE
{
	F_Unspecified = FONTLIB::F_Unspecified,
	F_Message = FONTLIB::F_Message,
	F_Text = FONTLIB::F_Text,
	F_Small = FONTLIB::F_Small,
	F_FrameCaption = FONTLIB::F_FrameCaption,
	F_ListBoxItem = FONTLIB::F_ListBoxItem,
	F_SelectedListBoxItem = FONTLIB::F_SelectedListBoxItem,
	F_Button = FONTLIB::F_Button,
	F_ButtonWhite = FONTLIB::F_ButtonWhite,
	F_Button_Disabled = FONTLIB::F_Button_Disabled,
	F_FrameRate = FONTLIB::F_FrameRate,
	F_FlashMessage = FONTLIB::F_FlashMessage,
	F_TextHead3 = FONTLIB::F_TextHead3,
	F_Hyperlink = FONTLIB::F_Hyperlink,
	F_TextHyperlink = FONTLIB::F_TextHyperlink,
	F_ExtHyperlink = FONTLIB::F_ExtHyperlink,
	F_TextExtHyperlink = FONTLIB::F_TextExtHyperlink,
	F_ActiveHyperlink = FONTLIB::F_ActiveHyperlink,
	F_TextActiveHyperlink = FONTLIB::F_TextActiveHyperlink,
	F_TextActiveExtHyperlink = FONTLIB::F_TextActiveExtHyperlink,
	F_TextBold = FONTLIB::F_TextBold,
	F_TextItalic = FONTLIB::F_TextItalic,
	F_TextUnderline = FONTLIB::F_TextUnderline,
	F_Header,
	F_HeaderWhite,
	F_Title,
	F_Scroll,
	F_Sign,
	F_LevelName,
	F_LevelInfo,
	F_LevelDescription,
	F_CreditsText,
	F_CreditsHeader,
	F_CreditsSubheader,
	F_Stats,
	F_TitleMenu,
	F_TitleMenuActive,
	F_TitleMenuSelected,
	F_TitleMarquee,
	F_TitleMarqueeHyperlink,
	F_TitleMarqueeActiveHyperlink,
	F_ItemMultiplier,
	F_ScoreTotal,
	F_InvSlotText,
	F_StatPanelText,
	F_DamagePreviewSmall,
	F_DamagePreviewMedium,
	F_DamagePreviewLarge,
	F_SettingsKeymaps,
	F_WorldMapLevel,

	F_Last
};

static const UINT FONT_COUNT = F_Last;

//****************************************************************************
class CDrodFontManager : public CFontManager
{
public:
	CDrodFontManager();

	virtual UINT      Init();

private:
	virtual bool   LoadFonts();
};

//Define global pointer to the one and only CDrodFontManager object.
#ifndef INCLUDED_FROM_DRODFONTMANAGER_CPP
	extern CDrodFontManager *g_pTheDFM;
#endif

#endif //...#ifndef DRODFONTMANAGER_H

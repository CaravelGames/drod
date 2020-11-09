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

#include "SettingsScreen.h"
#include "DrodBitmapManager.h"
#include "BrowserScreen.h"
#include "DrodScreenManager.h"
#include "DrodFontManager.h"
#include "DrodSound.h"
#include "GameScreen.h"

#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/OptionButtonWidget.h>
#include <FrontEndLib/SliderWidget.h>
#include <FrontEndLib/TabbedMenuWidget.h>
#include <FrontEndLib/TextBoxWidget.h>
#include <FrontEndLib/ProgressBarWidget.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/DbXML.h"
#include "../DRODLib/GameConstants.h"
#include "../DRODLib/SettingsKeys.h"

#include "../Texts/MIDs.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Metadata.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Base64.h>

using namespace InputCommands;

//Tabbed menu info.
#define PERSONAL_TAB (0)
#define GAS_TAB (1)
#define COMMANDS_TAB (2)
#define BG_COLOR 220,210,190

#define MIN_TARSTUFF_ALPHA (64)

//Default command key mappings.
const SDL_Keycode COMMANDKEY_ARRAY[2][DCMD_Count] = {	//desktop or laptop keyboard
{	//numpad default
	SDLK_KP_7, SDLK_KP_8, SDLK_KP_9, SDLK_KP_4, SDLK_KP_5, SDLK_KP_6, SDLK_KP_1, SDLK_KP_2, SDLK_KP_3,
	SDLK_w, SDLK_q, SDLK_r, SDLK_BACKSPACE, SDLK_KP_PLUS, SDLK_KP_PERIOD, SDLK_TAB
},{	//laptop default
	SDLK_7, SDLK_8, SDLK_9, SDLK_u, SDLK_i, SDLK_o, SDLK_j, SDLK_k, SDLK_l,
	SDLK_w, SDLK_q, SDLK_r, SDLK_BACKSPACE, SDLK_0, SDLK_PERIOD, SDLK_TAB
}};

//Widget tag constants.
const UINT TAG_NW_BUTTON = 1001;
const UINT TAG_N_BUTTON = 1002;
const UINT TAG_NE_BUTTON = 1003;
const UINT TAG_W_BUTTON = 1004;
const UINT TAG_WAIT_BUTTON = 1005;
const UINT TAG_E_BUTTON = 1006;
const UINT TAG_SW_BUTTON = 1007;
const UINT TAG_S_BUTTON = 1008;
const UINT TAG_SE_BUTTON = 1009;
const UINT TAG_C_BUTTON = 1010;
const UINT TAG_CC_BUTTON = 1011;
const UINT TAG_RESTART_BUTTON = 1012;
const UINT TAG_UNDO_BUTTON = 1013;
const UINT TAG_BATTLE_BUTTON = 1014;
const UINT TAG_COMMAND_BUTTON = 1015;
const UINT TAG_CLONESWITCH_BUTTON = 1016;

const UINT TAG_DEFAULT_DESKTOP = 1020;
const UINT TAG_DEFAULT_LAPTOP = 1021;

const UINT TAG_USE_FULLSCREEN = 1040;
const UINT TAG_ENABLE_SOUNDEFF = 1041;
const UINT TAG_ENABLE_MUSIC = 1042;
const UINT TAG_SOUNDEFF_VOLUME = 1043;
const UINT TAG_MUSIC_VOLUME = 1044;
const UINT TAG_ALPHA = 1045;
const UINT TAG_GAMMA = 1046;
const UINT TAG_ENVIRONMENT = 1047;
const UINT TAG_ENABLE_VOICES = 1048;
const UINT TAG_VOICES_VOLUME = 1049;
const UINT TAG_SHOW_SUBTITLES = 1050;
const UINT TAG_TARSTUFF_TRANSPARENCY = 1051;
const UINT TAG_DISPLAY_COMBOS = 1052;
const UINT TAG_NO_FOCUS_PLAYS_MUSIC = 1053;

const UINT TAG_SHOWCHECKPOINTS = 1055;
const UINT TAG_SAVEONCONQUER = 1056;
const UINT TAG_SAVEONDIE = 1057;
const UINT TAG_REPEATRATE = 1058;
const UINT TAG_UNDO_LEVEL = 1059;

const UINT TAG_NAME = 1060;
//const UINT TAG_CNETNAME = 1061; //defined in DrodScreen.h
//const UINT TAG_CNETPASSWORD = 1062;
const UINT TAG_REQUESTNEWKEY = 1063;
const UINT TAG_LOGIN_TO_CARAVELNET = 1064;
const UINT TAG_UPLOADSCORES = 1065;
const UINT TAG_CLOUD_ACTIVATE = 1066;

const UINT TAG_AUTOSAVE = 1070;
const UINT TAG_ITEMTIPS = 1071;
const UINT TAG_GROUPEDITORMENUITEMS = 1072;

const UINT TAG_UNDOLEVEL_NUM_LABEL = 1073;
const UINT TAG_AUTOUNDOONDEATH = 1074;

const UINT TAG_CANCEL = 1091;
const UINT TAG_HELP = 1092;

const UINT TAG_MENU = 1093;

const UINT CX_MESSAGE = 370;
const UINT CY_MESSAGE = 120;

//
//Protected methods.
//

//************************************************************************************
CSettingsScreen::CSettingsScreen()
//Constructor.
	: CDrodScreen(SCR_Settings)
	, pDialogBox(NULL), pCommandLabel(NULL)
	, pCurrentPlayer(NULL)
	, pNameWidget(NULL)
	, pCaravelNetNameWidget(NULL)
	, pCaravelNetPasswordWidget(NULL)
{
	SetKeyRepeat(66);

	this->imageFilenames.push_back(string("Background"));

	static const UINT CX_SPACE = 10;
	static const UINT CY_SPACE = 10;

	static const UINT CY_TITLE = CY_LABEL_FONT_TITLE;
	static const int Y_TITLE = Y_TITLE_LABEL_CENTER_DARK_BAR;

	static const UINT CX_OKAY_BUTTON = 110;
	static const UINT CY_OKAY_BUTTON = CY_STANDARD_BUTTON;
	static const UINT CX_CANCEL_BUTTON = CX_OKAY_BUTTON;
	static const UINT CY_CANCEL_BUTTON = CY_OKAY_BUTTON;
	static const UINT CX_HELP_BUTTON = CX_OKAY_BUTTON;
	static const UINT CY_HELP_BUTTON = CY_OKAY_BUTTON;
	const int X_CANCEL_BUTTON = (this->w - CX_CANCEL_BUTTON) / 2;
	static const int X_OKAY_BUTTON = X_CANCEL_BUTTON - CX_OKAY_BUTTON - CX_SPACE;
	const int X_HELP_BUTTON = X_CANCEL_BUTTON + CX_CANCEL_BUTTON + CX_SPACE;
	const int Y_OKAY_BUTTON = this->h - CY_SPACE - CY_OKAY_BUTTON;
	static const int Y_CANCEL_BUTTON = Y_OKAY_BUTTON;
	const int Y_HELP_BUTTON = Y_OKAY_BUTTON;

	//Menu
	const int X_TABBEDMENU = CX_SPACE * 2;
	const int Y_TABBEDMENU = Y_TITLE + CY_TITLE + Y_TITLE;
	const UINT CX_TABBEDMENU = this->w - X_TABBEDMENU * 2;
	const UINT CY_TABBEDMENU = Y_OKAY_BUTTON - Y_TABBEDMENU - CY_SPACE * 2;

	const UINT CY_MENU_TAB = 37;
	const int Y_INNERMENU = CY_MENU_TAB + CY_SPACE*3;

	//Personal frame and children.
	static const int X_PERSONAL_FRAME = CX_SPACE * 2;
	static const int Y_PERSONAL_FRAME = Y_INNERMENU;
	const UINT CX_PERSONAL_FRAME = 520;
	static const int X_NAME_LABEL = CX_SPACE;
	static const int Y_NAME_LABEL = CY_SPACE;
	static const UINT CX_NAME_LABEL = 60;
	static const UINT CY_NAME_LABEL = 40;
	static const int X_NAME = X_NAME_LABEL + CX_NAME_LABEL + CX_SPACE;
	static const int Y_NAME = Y_NAME_LABEL;
	const UINT CX_NAME = CX_PERSONAL_FRAME - X_NAME - CX_SPACE;
	static const UINT CY_NAME = CY_STANDARD_TBOX;

	static const int X_CNETCONNECT = CX_SPACE;
	static const int Y_CNETCONNECT = Y_NAME_LABEL + CY_NAME + CY_SPACE;
	static const UINT CX_CNETCONNECT = CX_PERSONAL_FRAME - X_CNETCONNECT - CX_SPACE;
	static const UINT CY_CNETCONNECT = CY_STANDARD_OPTIONBUTTON;

	static const int X_CNETNAME_LABEL = X_CNETCONNECT;
	static const int Y_CNETNAME_LABEL = Y_CNETCONNECT + CY_CNETCONNECT + CY_SPACE;
	static const UINT CX_CNETNAME_LABEL = 170;
	static const UINT CY_CNETNAME_LABEL = 40;
	static const int X_CNETNAME = X_CNETNAME_LABEL + CX_CNETNAME_LABEL + CX_SPACE;
	static const int Y_CNETNAME = Y_CNETNAME_LABEL;
	const UINT CX_CNETNAME = CX_PERSONAL_FRAME - X_CNETNAME - CX_SPACE;
	static const UINT CY_CNETNAME = CY_STANDARD_TBOX;

	static const int X_CNETPASSWORD_LABEL = X_CNETCONNECT;
	static const int Y_CNETPASSWORD_LABEL = Y_CNETNAME_LABEL + CY_CNETNAME + CY_SPACE;
	static const UINT CX_CNETPASSWORD_LABEL = CX_CNETNAME_LABEL;
	static const UINT CY_CNETPASSWORD_LABEL = CY_CNETNAME_LABEL;
	static const int X_CNETPASSWORD = X_CNETPASSWORD_LABEL + CX_CNETPASSWORD_LABEL + CX_SPACE;
	static const int Y_CNETPASSWORD = Y_CNETPASSWORD_LABEL;
	const UINT CX_CNETPASSWORD = CX_PERSONAL_FRAME - X_CNETPASSWORD - CX_SPACE;
	static const UINT CY_CNETPASSWORD = CY_STANDARD_TBOX;

	static const UINT CX_REQUESTKEY = 230;
	static const int X_REQUESTKEY = CX_PERSONAL_FRAME - CX_REQUESTKEY - CX_SPACE;
	static const int Y_REQUESTKEY = Y_CNETPASSWORD_LABEL + CY_CNETPASSWORD_LABEL;
	static const UINT CY_REQUESTKEY = CY_STANDARD_BUTTON;

	static const UINT CX_UPLOADSCORES = 240;
	static const int X_UPLOADSCORES = X_REQUESTKEY - CX_UPLOADSCORES - CX_SPACE * 2;
	static const int Y_UPLOADSCORES = Y_REQUESTKEY;
	static const UINT CY_UPLOADSCORES = CY_STANDARD_BUTTON;

#ifdef STEAMBUILD
	static const UINT CY_PERSONAL_FRAME = CY_NAME_LABEL + CY_CNETNAME +
			CY_CNETPASSWORD + CY_CNETCONNECT + CY_REQUESTKEY + CY_SPACE * 6;
#else
	static const UINT CX_CLOUD_ACTIVATE = 240;
	static const int X_CLOUD_ACTIVATE = X_UPLOADSCORES;
	static const int Y_CLOUD_ACTIVATE = Y_REQUESTKEY + CY_REQUESTKEY + CY_SPACE;
	static const UINT CY_CLOUD_ACTIVATE = CY_STANDARD_BUTTON;

	static const UINT CY_PERSONAL_FRAME = CY_NAME_LABEL + CY_CNETNAME +
			CY_CNETPASSWORD + CY_CNETCONNECT + CY_REQUESTKEY + CY_CLOUD_ACTIVATE + CY_SPACE * 7;
#endif

	//Editor frame and children.
	static const int X_EDITOR_FRAME = X_PERSONAL_FRAME + CX_PERSONAL_FRAME + CX_SPACE * 2;
	static const int Y_EDITOR_FRAME = Y_PERSONAL_FRAME;
	static const UINT CX_EDITOR_FRAME = CX_TABBEDMENU - X_EDITOR_FRAME - X_PERSONAL_FRAME;
	static const int X_AUTOSAVE = CX_SPACE;
	static const int Y_AUTOSAVE = CY_SPACE;
	static const UINT CX_AUTOSAVE = CX_EDITOR_FRAME - X_AUTOSAVE - CX_SPACE;
	static const UINT CY_AUTOSAVE = CY_STANDARD_OPTIONBUTTON;
	static const int X_ITEMTIPS = CX_SPACE;
	static const int Y_ITEMTIPS = Y_AUTOSAVE + CY_AUTOSAVE;
	static const UINT CX_ITEMTIPS = CX_AUTOSAVE / 2;
	static const UINT CY_ITEMTIPS = CY_STANDARD_OPTIONBUTTON;
	static const int X_GEMI = X_ITEMTIPS + CX_ITEMTIPS;
	static const int Y_GEMI = Y_ITEMTIPS;
	static const UINT CX_GEMI = CX_EDITOR_FRAME - X_GEMI;
	static const UINT CY_GEMI = CY_STANDARD_OPTIONBUTTON;
	static const UINT CY_EDITOR_FRAME = Y_ITEMTIPS + CY_ITEMTIPS + CY_SPACE;

	//Special frame and children.
	static const int X_SPECIAL_FRAME = X_PERSONAL_FRAME;
	static const int Y_SPECIAL_FRAME = Y_PERSONAL_FRAME + CY_PERSONAL_FRAME + 2*CY_SPACE;
	static const UINT CX_SPECIAL_FRAME = CX_PERSONAL_FRAME;

	static const int X_SHOWCHECKPOINTS = CX_SPACE;
	static const int Y_SHOWCHECKPOINTS = CY_SPACE;
	static const UINT CX_SHOWCHECKPOINTS = CX_SPECIAL_FRAME - X_SHOWCHECKPOINTS;
	static const UINT CY_SHOWCHECKPOINTS = CY_STANDARD_OPTIONBUTTON;
	static const int X_SAVEONCONQUER = CX_SPACE;
	static const int Y_SAVEONCONQUER = Y_SHOWCHECKPOINTS + CY_SHOWCHECKPOINTS;
	static const UINT CX_SAVEONCONQUER = CX_SHOWCHECKPOINTS;
	static const UINT CY_SAVEONCONQUER = CY_STANDARD_OPTIONBUTTON;
	static const int X_SAVEONDIE = CX_SPACE;
	static const int Y_SAVEONDIE = Y_SAVEONCONQUER + CY_SAVEONCONQUER;
	static const UINT CX_SAVEONDIE = CX_SHOWCHECKPOINTS;
	static const UINT CY_SAVEONDIE = CY_STANDARD_OPTIONBUTTON;
	static const int X_AUTOUNDOONDEATH = CX_SPACE;
	static const int Y_AUTOUNDOONDEATH = Y_SAVEONDIE + CY_SAVEONDIE;
	static const UINT CX_AUTOUNDOONDEATH = CX_SHOWCHECKPOINTS;
	static const UINT CY_AUTOUNDOONDEATH = CY_STANDARD_OPTIONBUTTON;

	static const UINT CY_SPECIAL_FRAME = Y_AUTOUNDOONDEATH + CY_AUTOUNDOONDEATH + CY_SPACE;

	//Graphics frame and children.
	const UINT CX_GRAPHICS_FRAME = CX_PERSONAL_FRAME;
	static const int X_GRAPHICS_FRAME = X_PERSONAL_FRAME;
	static const int Y_GRAPHICS_FRAME = Y_PERSONAL_FRAME;

	static const UINT CX_USEFULLSCREEN = 200;
	static const UINT CY_USEFULLSCREEN = CY_STANDARD_OPTIONBUTTON;
	static const int X_USEFULLSCREEN = CX_SPACE;
	static const int Y_USEFULLSCREEN = CY_SPACE;
	static const int X_ALPHA = X_USEFULLSCREEN;
	static const int Y_ALPHA = Y_USEFULLSCREEN + CY_STANDARD_OPTIONBUTTON + (4 * CY_SPACE);
	static const UINT CX_ALPHA = CX_USEFULLSCREEN;
	static const UINT CY_ALPHA = CY_STANDARD_OPTIONBUTTON;

	static const int Y_GAMMA_LABEL = Y_USEFULLSCREEN;
	static const UINT X_GAMMA_LABEL = X_USEFULLSCREEN + CX_USEFULLSCREEN - 10;
	static const UINT CX_GAMMA_LABEL = 60;
	static const UINT CY_GAMMA_LABEL = 28;
	static const UINT CX_GAMMA_MIN_LABEL = 20;
	static const UINT CX_GAMMA_ONE_LABEL = 17;
	static const UINT CX_GAMMA_MAX_LABEL = 20;
	static const int X_GAMMA_MIN = X_GAMMA_LABEL + CX_GAMMA_LABEL;
	static const int X_GAMMA_MAX = CX_GRAPHICS_FRAME - CX_GAMMA_MAX_LABEL - CX_SPACE;
	static const int X_GAMMA_ONE = X_GAMMA_MIN +
			(X_GAMMA_MAX + CX_GAMMA_MAX_LABEL - X_GAMMA_MIN)/3 - CX_GAMMA_ONE_LABEL/2 + 2;

	static const int X_GAMMA = X_GAMMA_MIN;
	static const UINT CX_GAMMA = X_GAMMA_MAX + CX_GAMMA_MAX_LABEL - X_GAMMA;
	static const UINT CY_GAMMA = CY_STANDARD_SLIDER;
	static const int Y_GAMMA = Y_GAMMA_LABEL + CY_GAMMA_LABEL;

	static const int X_ENVIRONMENTAL_LABEL = X_ALPHA;
	static const int Y_ENVIRONMENTAL_LABEL = Y_ALPHA + CY_ALPHA + CY_SPACE;
	static const UINT CX_ENVIRONMENTAL_LABEL = CX_USEFULLSCREEN;
	static const UINT CY_ENVIRONMENTAL_LABEL = CY_GAMMA_LABEL;

	static const int X_ENVIRONMENTAL = X_GAMMA;
	static const int Y_ENVIRONMENTAL = Y_ENVIRONMENTAL_LABEL;
	static const UINT CX_ENVIRONMENTAL = CX_GAMMA;
	static const UINT CY_ENVIRONMENTAL = CY_STANDARD_SLIDER;

	static const int X_TARSTUFFALPHA_LABEL = X_ALPHA;
	static const int Y_TARSTUFFALPHA_LABEL = Y_ENVIRONMENTAL_LABEL + CY_ENVIRONMENTAL_LABEL + CY_SPACE;
	static const UINT CX_TARSTUFFALPHA_LABEL = CX_USEFULLSCREEN;
	static const UINT CY_TARSTUFFALPHA_LABEL = CY_GAMMA_LABEL;

	static const int X_TARSTUFFALPHA = X_GAMMA;
	static const int Y_TARSTUFFALPHA = Y_TARSTUFFALPHA_LABEL;
	static const UINT CX_TARSTUFFALPHA = CX_GAMMA;
	static const UINT CY_TARSTUFFALPHA = CY_STANDARD_SLIDER;

	static const int X_DISPLAY_COMBOS = X_USEFULLSCREEN;
	static const int Y_DISPLAY_COMBOS = Y_TARSTUFFALPHA + CY_STANDARD_OPTIONBUTTON + (4 * CY_SPACE);
	static const UINT CX_DISPLAY_COMBOS = CX_USEFULLSCREEN;
	static const UINT CY_DISPLAY_COMBOS = CY_STANDARD_OPTIONBUTTON;

	static const UINT CY_GRAPHICS_FRAME = Y_DISPLAY_COMBOS + CY_DISPLAY_COMBOS + CY_SPACE;

	//Sound
	const UINT CX_SOUND_FRAME = CX_GRAPHICS_FRAME;
	static const int X_SOUND_FRAME = X_GRAPHICS_FRAME;
	static const int Y_SOUND_FRAME = Y_GRAPHICS_FRAME + CY_GRAPHICS_FRAME + CY_SPACE*2;

	static const int Y_QUIET_LABEL = CY_SPACE;
	static const UINT CX_QUIET_LABEL = 50;
	static const UINT CY_QUIET_LABEL = 28;
	static const int Y_LOUD_LABEL = Y_QUIET_LABEL;
	static const UINT CX_LOUD_LABEL = 40;
	static const UINT CY_LOUD_LABEL = CY_QUIET_LABEL;
	static const int X_LOUD_LABEL = CX_SOUND_FRAME - CX_SPACE - CX_LOUD_LABEL;
	static const int X_ENABLE_SOUNDEFF = CX_SPACE;
	static const int Y_ENABLE_SOUNDEFF = Y_QUIET_LABEL + CY_QUIET_LABEL;
	static const UINT CX_ENABLE_SOUNDEFF = 250;
	static const UINT CY_ENABLE_SOUNDEFF = CY_STANDARD_OPTIONBUTTON;
	static const int X_ENABLE_VOICES = X_ENABLE_SOUNDEFF;
	static const int Y_ENABLE_VOICES = Y_ENABLE_SOUNDEFF + CY_STANDARD_OPTIONBUTTON;
	static const UINT CX_ENABLE_VOICES = CX_ENABLE_SOUNDEFF;
	static const UINT CY_ENABLE_VOICES = CY_ENABLE_SOUNDEFF;
	static const int X_ENABLE_MUSIC = X_ENABLE_SOUNDEFF;
	static const int Y_ENABLE_MUSIC = Y_ENABLE_VOICES + CY_STANDARD_OPTIONBUTTON;
	static const UINT CX_ENABLE_MUSIC = CX_ENABLE_SOUNDEFF;
	static const UINT CY_ENABLE_MUSIC = CY_ENABLE_VOICES;
	static const int X_SOUNDEFF_VOLUME = X_ENABLE_VOICES + CX_ENABLE_SOUNDEFF;
	static const UINT CX_SOUNDEFF_VOLUME = CX_SOUND_FRAME - X_SOUNDEFF_VOLUME - CX_SPACE;
	static const UINT CY_SOUNDEFF_VOLUME = CY_STANDARD_SLIDER;
	static const int Y_SOUNDEFF_VOLUME = Y_ENABLE_SOUNDEFF +
			(CY_ENABLE_SOUNDEFF - CY_SOUNDEFF_VOLUME) / 2;
	static const int X_QUIET_LABEL = X_SOUNDEFF_VOLUME;
	static const int X_VOICES_VOLUME = X_ENABLE_VOICES + CX_ENABLE_VOICES;
	static const UINT CX_VOICES_VOLUME = CX_SOUND_FRAME - X_VOICES_VOLUME - CX_SPACE;
	static const UINT CY_VOICES_VOLUME = CY_STANDARD_SLIDER;
	static const int Y_VOICES_VOLUME = Y_ENABLE_VOICES +
			(CY_ENABLE_VOICES - CY_VOICES_VOLUME) / 2;
	static const UINT CX_MUSIC_VOLUME = CX_SOUNDEFF_VOLUME;
	static const UINT CY_MUSIC_VOLUME = CY_STANDARD_SLIDER;
	static const int X_MUSIC_VOLUME = X_SOUNDEFF_VOLUME;
	static const int Y_MUSIC_VOLUME = Y_ENABLE_MUSIC + (CY_ENABLE_MUSIC - CY_MUSIC_VOLUME) / 2;

	static const int X_SHOWSUBTITLES = CX_SPACE;
	static const int Y_SHOWSUBTITLES = Y_MUSIC_VOLUME + CY_MUSIC_VOLUME;
	static const UINT CX_SHOWSUBTITLES = CX_SOUND_FRAME - X_SHOWSUBTITLES;
	static const UINT CY_SHOWSUBTITLES = CY_STANDARD_OPTIONBUTTON;

	static const int X_NO_FOCUS_PLAYS_MUSIC = X_SHOWSUBTITLES;
	static const int Y_NO_FOCUS_PLAYS_MUSIC = Y_SHOWSUBTITLES + CY_STANDARD_OPTIONBUTTON + (4 * CY_SPACE);
	static const UINT CX_NO_FOCUS_PLAYS_MUSIC = CX_SHOWSUBTITLES;
	static const UINT CY_NO_FOCUS_PLAYS_MUSIC = CY_STANDARD_OPTIONBUTTON;

	const UINT CY_SOUND_FRAME = Y_NO_FOCUS_PLAYS_MUSIC + CY_NO_FOCUS_PLAYS_MUSIC + CY_SPACE;

	//Commands tab.
	const UINT CX_CMD_BUTTON = 210;
	static const UINT CY_CMD_BUTTON = CY_STANDARD_BUTTON;
	static const int X_CMD_BUTTON = CX_SPACE;
	static const int Y_CMD_BUTTON = Y_INNERMENU + CY_SPACE;
	const int X_CMD_LABEL = X_CMD_BUTTON + CX_CMD_BUTTON + CX_SPACE;
	const UINT CX_CMD_LABEL = 120;
	static const UINT CY_CMD_LABEL = CY_CMD_BUTTON;
	static const UINT BUTTON_COLUMNS = 3;

	//Repeat rate.
	static const int Y_SLOW_LABEL = Y_CMD_BUTTON + CY_CMD_BUTTON * 5;
	static const UINT CX_SLOW_LABEL = 100;
	static const UINT CY_SLOW_LABEL = 30;
	static const int Y_FAST_LABEL = Y_SLOW_LABEL;
#ifdef RUSSIAN_BUILD
	static const UINT CX_FAST_LABEL = 65;
	static const UINT CY_REPEATRATE_LABEL = 60;
	static const int Y_REPEATRATE_LABEL = Y_SLOW_LABEL + CY_SPACE;
#else
	static const UINT CX_FAST_LABEL = 40;
	static const UINT CY_REPEATRATE_LABEL = 30;
	static const int Y_REPEATRATE_LABEL = Y_SLOW_LABEL + CY_SLOW_LABEL;
#endif
	static const UINT CY_FAST_LABEL = CY_SLOW_LABEL;
	const int X_FAST_LABEL = CX_TABBEDMENU - CX_SPACE * 2 - CX_FAST_LABEL;
	static const UINT CX_REPEATRATE_LABEL = 100;
	static const int X_REPEATRATE_LABEL = CX_TABBEDMENU / 2;
	static const int X_REPEATRATE = X_REPEATRATE_LABEL + CX_REPEATRATE_LABEL + CX_SPACE;
	const UINT CX_REPEATRATE = CX_SPECIAL_FRAME - (5*CX_SPACE + CX_REPEATRATE_LABEL);
	static const UINT CY_REPEATRATE = CY_STANDARD_SLIDER;
	static const int Y_REPEATRATE = Y_REPEATRATE_LABEL +
			(CY_REPEATRATE_LABEL - CY_REPEATRATE) / 2;
	static const int X_SLOW_LABEL = X_REPEATRATE;

	//Undo level.
	static const int X_UNDOLEVEL_LABEL = X_REPEATRATE_LABEL;
	static const int Y_UNDOLEVEL_LABEL = Y_REPEATRATE + CY_REPEATRATE + CY_SPACE;
	static const UINT CX_UNDOLEVEL_LABEL = 100;
	static const UINT CY_UNDOLEVEL_LABEL = CY_REPEATRATE_LABEL;

	static const UINT CX_UNDOLEVEL_NUM_LABEL = 80;
	static const UINT CY_UNDOLEVEL_NUM_LABEL = CY_UNDOLEVEL_LABEL;

	static const UINT CY_UNDOLEVEL = CY_STANDARD_SLIDER;
	static const int X_UNDOLEVEL = X_UNDOLEVEL_LABEL + CX_UNDOLEVEL_LABEL + CX_SPACE;
	static const int Y_UNDOLEVEL = Y_UNDOLEVEL_LABEL +
			(CY_UNDOLEVEL_LABEL - CY_UNDOLEVEL) / 2;
	const UINT CX_UNDOLEVEL = CX_SPECIAL_FRAME - (6*CX_SPACE + CX_UNDOLEVEL_LABEL + CX_UNDOLEVEL_NUM_LABEL);

	static const int X_UNDOLEVEL_NUM_LABEL = X_UNDOLEVEL + CX_UNDOLEVEL + CX_SPACE;
	static const int Y_UNDOLEVEL_NUM_LABEL = Y_UNDOLEVEL;

	//
	//Add widgets to screen.
	//

	CSliderWidget *pSliderWidget;
	COptionButtonWidget *pOptionButton;
	CButtonWidget *pButton;

	//Title.
	CLabelWidget *pTitle = new CLabelWidget(0L, 0, Y_TITLE,
			this->w, CY_TITLE, F_Title, g_pTheDB->GetMessageText(MID_Settings));
	pTitle->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(pTitle);

	//Object menu.
	CTabbedMenuWidget *pTabbedMenu = new CTabbedMenuWidget(TAG_MENU, X_TABBEDMENU,
			Y_TABBEDMENU, CX_TABBEDMENU, CY_TABBEDMENU, 3, CY_MENU_TAB, BG_COLOR);
	pTabbedMenu->SetTabText(PERSONAL_TAB, g_pTheDB->GetMessageText(MID_Settings));
	pTabbedMenu->SetTabText(GAS_TAB, g_pTheDB->GetMessageText(MID_GraphicsAndSound));
	pTabbedMenu->SetTabText(COMMANDS_TAB, g_pTheDB->GetMessageText(MID_Commands));
	pTabbedMenu->SetBGImage("Background", 128);
	AddWidget(pTabbedMenu);


	//Settings tab.

	//Personal frame.
	CFrameWidget *pPersonalFrame = new CFrameWidget(0L, X_PERSONAL_FRAME, Y_PERSONAL_FRAME,
			CX_PERSONAL_FRAME, CY_PERSONAL_FRAME, g_pTheDB->GetMessageText(MID_Personal));
	pTabbedMenu->AddWidgetToTab(pPersonalFrame, PERSONAL_TAB);

	//Personal frame children.
	pPersonalFrame->AddWidget(
		new CLabelWidget(0L, X_NAME_LABEL, Y_NAME_LABEL,
		CX_NAME_LABEL, CY_NAME_LABEL, F_FrameCaption, g_pTheDB->GetMessageText(MID_Name)) );
	this->pNameWidget = new CTextBoxWidget(TAG_NAME, X_NAME, Y_NAME, CX_NAME, CY_NAME);
	pPersonalFrame->AddWidget(this->pNameWidget);

	pPersonalFrame->AddWidget(new COptionButtonWidget(TAG_LOGIN_TO_CARAVELNET,
			X_CNETCONNECT, Y_CNETCONNECT, CX_CNETCONNECT,
			CY_CNETCONNECT, g_pTheDB->GetMessageText(MID_ConnectToInternet), true));

	pPersonalFrame->AddWidget(
		new CLabelWidget(0L, X_CNETNAME_LABEL, Y_CNETNAME_LABEL,
		CX_CNETNAME_LABEL, CY_CNETNAME_LABEL, F_FrameCaption, g_pTheDB->GetMessageText(MID_CaravelNetName)) );
	this->pCaravelNetNameWidget = new CTextBoxWidget(TAG_CNETNAME,
			X_CNETNAME, Y_CNETNAME, CX_CNETNAME, CY_CNETNAME, 30);
	pPersonalFrame->AddWidget(this->pCaravelNetNameWidget);

	pPersonalFrame->AddWidget(
		new CLabelWidget(0L, X_CNETPASSWORD_LABEL, Y_CNETPASSWORD_LABEL,
		CX_CNETPASSWORD_LABEL, CY_CNETPASSWORD_LABEL, F_FrameCaption, g_pTheDB->GetMessageText(MID_CaravelNetKey)) );
	this->pCaravelNetPasswordWidget = new CTextBoxWidget(TAG_CNETPASSWORD,
				X_CNETPASSWORD, Y_CNETPASSWORD, CX_CNETPASSWORD, CY_CNETPASSWORD, 33);
#ifndef ENABLE_CHEATS
	this->pCaravelNetPasswordWidget->SetViewAsPassword(true);
#endif
	pPersonalFrame->AddWidget(this->pCaravelNetPasswordWidget);

	pPersonalFrame->AddWidget(
		new CButtonWidget(TAG_REQUESTNEWKEY, X_REQUESTKEY, Y_REQUESTKEY,
		CX_REQUESTKEY, CY_REQUESTKEY, g_pTheDB->GetMessageText(MID_RequestNewKey)) );

	pPersonalFrame->AddWidget(
		new CButtonWidget(TAG_UPLOADSCORES, X_UPLOADSCORES, Y_UPLOADSCORES,
		CX_UPLOADSCORES, CY_UPLOADSCORES, g_pTheDB->GetMessageText(MID_UploadScores)) );

#ifndef STEAMBUILD
	pPersonalFrame->AddWidget(
		new CButtonWidget(TAG_CLOUD_ACTIVATE, X_CLOUD_ACTIVATE, Y_CLOUD_ACTIVATE,
		CX_CLOUD_ACTIVATE, CY_CLOUD_ACTIVATE, g_pTheDB->GetMessageText(MID_ActivateCloudSync)) );
#endif
	
	//Special frame.
	CFrameWidget *pSpecialFrame = new CFrameWidget(0L, X_SPECIAL_FRAME, Y_SPECIAL_FRAME,
			CX_SPECIAL_FRAME, CY_SPECIAL_FRAME, g_pTheDB->GetMessageText(MID_Special));
	pTabbedMenu->AddWidgetToTab(pSpecialFrame, PERSONAL_TAB);

	pOptionButton = new COptionButtonWidget(TAG_SHOWCHECKPOINTS, X_SHOWCHECKPOINTS,
					Y_SHOWCHECKPOINTS, CX_SHOWCHECKPOINTS, CY_SHOWCHECKPOINTS,
					g_pTheDB->GetMessageText(MID_ShowCheckpoints), false);
	pSpecialFrame->AddWidget(pOptionButton);

	pOptionButton = new COptionButtonWidget(TAG_SAVEONCONQUER, X_SAVEONCONQUER,
					Y_SAVEONCONQUER, CX_SAVEONCONQUER, CY_SAVEONCONQUER,
					g_pTheDB->GetMessageText(MID_AutoSaveDemoOnConquer), 
					false);
	pSpecialFrame->AddWidget(pOptionButton);

	pOptionButton = new COptionButtonWidget(TAG_SAVEONDIE, X_SAVEONDIE,
					Y_SAVEONDIE, CX_SAVEONDIE, CY_SAVEONDIE,
					g_pTheDB->GetMessageText(MID_AutoSaveDemoOnDie), 
					false);
	pSpecialFrame->AddWidget(pOptionButton);

	pOptionButton = new COptionButtonWidget(TAG_AUTOUNDOONDEATH, X_AUTOUNDOONDEATH,
					Y_AUTOUNDOONDEATH, CX_AUTOUNDOONDEATH, CY_AUTOUNDOONDEATH,
					g_pTheDB->GetMessageText(MID_AutoUndoOnDeath),
					false);
	pSpecialFrame->AddWidget(pOptionButton);

	//Editor frame.
	CFrameWidget *pEditorFrame = new CFrameWidget(0L, X_EDITOR_FRAME, Y_EDITOR_FRAME,
			CX_EDITOR_FRAME, CY_EDITOR_FRAME, g_pTheDB->GetMessageText(MID_Editor));
	pTabbedMenu->AddWidgetToTab(pEditorFrame, PERSONAL_TAB);

	pOptionButton = new COptionButtonWidget(TAG_AUTOSAVE, X_AUTOSAVE,
					Y_AUTOSAVE, CX_AUTOSAVE, CY_AUTOSAVE,
					g_pTheDB->GetMessageText(MID_AutoSave), true);
	pEditorFrame->AddWidget(pOptionButton);

	pOptionButton = new COptionButtonWidget(TAG_ITEMTIPS, X_ITEMTIPS,
					Y_ITEMTIPS, CX_ITEMTIPS, CY_ITEMTIPS,
					g_pTheDB->GetMessageText(MID_ItemTips), true);
	pEditorFrame->AddWidget(pOptionButton);

	pOptionButton = new COptionButtonWidget(TAG_GROUPEDITORMENUITEMS, X_GEMI,
					Y_GEMI, CX_GEMI, CY_GEMI,
					g_pTheDB->GetMessageText(MID_GroupEditorMenuItems), false);
	pEditorFrame->AddWidget(pOptionButton);

	//Graphics and sound tab.

	//Graphics frame.
	CFrameWidget *pGraphicsFrame = new CFrameWidget(0, X_GRAPHICS_FRAME, Y_GRAPHICS_FRAME,
			CX_GRAPHICS_FRAME, CY_GRAPHICS_FRAME, g_pTheDB->GetMessageText(MID_Graphics));
	pTabbedMenu->AddWidgetToTab(pGraphicsFrame, GAS_TAB);

	//Graphics children.
	pOptionButton = new COptionButtonWidget(TAG_USE_FULLSCREEN, X_USEFULLSCREEN,
			Y_USEFULLSCREEN, CX_USEFULLSCREEN, CY_USEFULLSCREEN,
			g_pTheDB->GetMessageText(MID_UseFullScreen), false);
	pGraphicsFrame->AddWidget(pOptionButton);
	if (!CScreen::bAllowFullScreen || !CScreen::bAllowWindowed)
		pOptionButton->Disable();

	pOptionButton = new COptionButtonWidget(TAG_ALPHA, X_ALPHA, Y_ALPHA,
			CX_ALPHA, CY_ALPHA, g_pTheDB->GetMessageText(MID_AlphaBlending), true);
	pGraphicsFrame->AddWidget(pOptionButton);

	pGraphicsFrame->AddWidget(new CLabelWidget(0L, X_ENVIRONMENTAL_LABEL, Y_ENVIRONMENTAL_LABEL,
			CX_ENVIRONMENTAL_LABEL, CY_ENVIRONMENTAL_LABEL, F_Small, g_pTheDB->GetMessageText(MID_EnvironmentalEffects)) );
	pSliderWidget = new CSliderWidget(TAG_ENVIRONMENT, X_ENVIRONMENTAL, Y_ENVIRONMENTAL,
			CX_ENVIRONMENTAL, CY_ENVIRONMENTAL, 1, 1 + Metadata::GetInt(MetaKey::MAX_EYE_CANDY));
	pSliderWidget->SetDrawTickMarks(true);
	pSliderWidget->pBiggerTicks.push_back(4);
	pSliderWidget->pBiggerTicks.push_back(8);
	pSliderWidget->pBiggerTicks.push_back(12);
	pGraphicsFrame->AddWidget(pSliderWidget);

	pGraphicsFrame->AddWidget(new CLabelWidget(0L, X_TARSTUFFALPHA_LABEL, Y_TARSTUFFALPHA_LABEL,
			CX_TARSTUFFALPHA_LABEL, CY_TARSTUFFALPHA_LABEL, F_Small, g_pTheDB->GetMessageText(MID_TarstuffAlpha)) );
	pSliderWidget = new CSliderWidget(TAG_TARSTUFF_TRANSPARENCY, X_TARSTUFFALPHA, Y_TARSTUFFALPHA,
			CX_TARSTUFFALPHA, CY_TARSTUFFALPHA, 255, BYTE(255 - MIN_TARSTUFF_ALPHA + 1));
	pGraphicsFrame->AddWidget(pSliderWidget);

	pOptionButton = new COptionButtonWidget(TAG_DISPLAY_COMBOS, X_DISPLAY_COMBOS, Y_DISPLAY_COMBOS,
			CX_DISPLAY_COMBOS, CY_DISPLAY_COMBOS, g_pTheDB->GetMessageText(MID_ComboDisplay), true);
	pGraphicsFrame->AddWidget(pOptionButton);

	//Gamma.
	pGraphicsFrame->AddWidget(new CLabelWidget(0L, X_GAMMA_LABEL, Y_GAMMA_LABEL,
			CX_GAMMA_LABEL, CY_GAMMA_LABEL, F_Small, g_pTheDB->GetMessageText(MID_Gamma)) );
	char numtext[10];
	WSTRING wstr;
	sprintf(numtext, "%0.1f", CDrodBitmapManager::fMinGamma);
	AsciiToUnicode(numtext, wstr);
	pGraphicsFrame->AddWidget(new CLabelWidget(0L, X_GAMMA_MIN, Y_GAMMA_LABEL,
			CX_GAMMA_MIN_LABEL, CY_GAMMA_LABEL, F_Small, wstr.c_str()) );
	sprintf(numtext, "%0.1f", 1.0f);
	AsciiToUnicode(numtext, wstr);
	pGraphicsFrame->AddWidget(new CLabelWidget(0L, X_GAMMA_ONE, Y_GAMMA_LABEL,
			CX_GAMMA_ONE_LABEL, CY_GAMMA_LABEL, F_Small, wstr.c_str()) );
	sprintf(numtext, "%0.1f", CDrodBitmapManager::fMaxGamma);
	AsciiToUnicode(numtext, wstr);
	pGraphicsFrame->AddWidget(new CLabelWidget(0L, X_GAMMA_MAX, Y_GAMMA_LABEL,
			CX_GAMMA_MAX_LABEL, CY_GAMMA_LABEL, F_Small, wstr.c_str()) );

	pSliderWidget = new CSliderWidget(TAG_GAMMA, X_GAMMA, Y_GAMMA,
			CX_GAMMA, CY_GAMMA, CDrodBitmapManager::GetGammaOne());
	pGraphicsFrame->AddWidget(pSliderWidget);

	//Sound frame.
	CFrameWidget *pSoundFrame = new CFrameWidget(0, X_SOUND_FRAME, Y_SOUND_FRAME,
			CX_SOUND_FRAME, CY_SOUND_FRAME, g_pTheDB->GetMessageText(MID_Sound));
	pTabbedMenu->AddWidgetToTab(pSoundFrame, GAS_TAB);

	//Sound children.
	pSoundFrame->AddWidget(new CLabelWidget(0L, X_QUIET_LABEL, Y_QUIET_LABEL,
			CX_QUIET_LABEL, CY_QUIET_LABEL, F_Small, g_pTheDB->GetMessageText(MID_Quiet)) );
	pSoundFrame->AddWidget(new CLabelWidget(0L, X_LOUD_LABEL, Y_LOUD_LABEL,
			CX_LOUD_LABEL, CY_LOUD_LABEL, F_Small, g_pTheDB->GetMessageText(MID_Loud)) );

	pOptionButton = new COptionButtonWidget(TAG_ENABLE_SOUNDEFF,
			X_ENABLE_SOUNDEFF, Y_ENABLE_SOUNDEFF, CX_ENABLE_SOUNDEFF,
			CY_ENABLE_SOUNDEFF, g_pTheDB->GetMessageText(MID_PlaySoundEffects), false);
	pSoundFrame->AddWidget(pOptionButton);

	pSliderWidget = new CSliderWidget(TAG_SOUNDEFF_VOLUME, X_SOUNDEFF_VOLUME,
			Y_SOUNDEFF_VOLUME, CX_SOUNDEFF_VOLUME, CY_SOUNDEFF_VOLUME, DEFAULT_SOUND_VOLUME);
	pSoundFrame->AddWidget(pSliderWidget);

	pOptionButton = new COptionButtonWidget(TAG_ENABLE_VOICES, X_ENABLE_VOICES,
			Y_ENABLE_VOICES, CX_ENABLE_VOICES, CY_ENABLE_VOICES,
			g_pTheDB->GetMessageText(MID_PlayVoices), false);
	pSoundFrame->AddWidget(pOptionButton);

	pSliderWidget = new CSliderWidget(TAG_VOICES_VOLUME, X_VOICES_VOLUME,
			Y_VOICES_VOLUME, CX_VOICES_VOLUME, CY_VOICES_VOLUME, DEFAULT_VOICE_VOLUME);
	pSoundFrame->AddWidget(pSliderWidget);

	pOptionButton = new COptionButtonWidget(TAG_ENABLE_MUSIC, X_ENABLE_MUSIC,
			Y_ENABLE_MUSIC, CX_ENABLE_MUSIC, CY_ENABLE_MUSIC,
			g_pTheDB->GetMessageText(MID_PlayMusic), false);
	pSoundFrame->AddWidget(pOptionButton);

	pSliderWidget = new CSliderWidget(TAG_MUSIC_VOLUME, X_MUSIC_VOLUME,
			Y_MUSIC_VOLUME, CX_MUSIC_VOLUME, CY_MUSIC_VOLUME, DEFAULT_MUSIC_VOLUME);
	pSoundFrame->AddWidget(pSliderWidget);

	pOptionButton = new COptionButtonWidget(TAG_SHOW_SUBTITLES, X_SHOWSUBTITLES,
			Y_SHOWSUBTITLES, CX_SHOWSUBTITLES, CY_SHOWSUBTITLES,
			g_pTheDB->GetMessageText(MID_ShowSubtitlesWithVoices), false);
	pSoundFrame->AddWidget(pOptionButton);

	pOptionButton = new COptionButtonWidget(TAG_NO_FOCUS_PLAYS_MUSIC, X_NO_FOCUS_PLAYS_MUSIC, Y_NO_FOCUS_PLAYS_MUSIC,
			CX_NO_FOCUS_PLAYS_MUSIC, CY_NO_FOCUS_PLAYS_MUSIC,
			g_pTheDB->GetMessageText(MID_NoFocusPlaysMusic), false);
	pSoundFrame->AddWidget(pOptionButton);

	//Commands tab.

	//Command buttons.
	static const UINT BUTTON_COMMAND_COUNT = DCMD_Count;
	static const UINT ButtonTags[BUTTON_COMMAND_COUNT] = {
		TAG_NW_BUTTON, TAG_N_BUTTON, TAG_NE_BUTTON, 
		TAG_W_BUTTON, TAG_WAIT_BUTTON, TAG_E_BUTTON,
		TAG_SW_BUTTON, TAG_S_BUTTON, TAG_SE_BUTTON,
		TAG_C_BUTTON, TAG_CC_BUTTON, TAG_RESTART_BUTTON,
		TAG_UNDO_BUTTON, TAG_BATTLE_BUTTON, TAG_COMMAND_BUTTON, TAG_CLONESWITCH_BUTTON
	};

	//Show directional buttons in three columns.
	static const UINT DIRECTIONAL_BUTTON_COUNT = 11;
	UINT wButtonTagI;
	for (wButtonTagI = 0; wButtonTagI < DIRECTIONAL_BUTTON_COUNT; ++wButtonTagI)
	{
		const UINT xCol = wButtonTagI % BUTTON_COLUMNS;
		const UINT xRow = wButtonTagI / BUTTON_COLUMNS;

		//Command buttons.
		const MESSAGE_ID eMsgID = static_cast<MESSAGE_ID>(COMMAND_MIDS[wButtonTagI]);
		const UINT yPixel = Y_CMD_BUTTON + (CY_CMD_BUTTON + 1) * xRow;
		pButton = new CButtonWidget(ButtonTags[wButtonTagI],
			X_CMD_BUTTON + (CX_CMD_BUTTON + CX_CMD_LABEL) * xCol, yPixel,
			CX_CMD_BUTTON, CY_CMD_BUTTON, g_pTheDB->GetMessageText(eMsgID));
		pTabbedMenu->AddWidgetToTab(pButton, COMMANDS_TAB);

		//Command labels.
		this->pCommandLabelWidgets[wButtonTagI] = new CLabelWidget(0,
				X_CMD_LABEL + (CX_CMD_BUTTON + CX_CMD_LABEL) * xCol, yPixel,
				CX_CMD_LABEL, CY_CMD_LABEL, F_Small, wszEmpty);
		pTabbedMenu->AddWidgetToTab(this->pCommandLabelWidgets[wButtonTagI], COMMANDS_TAB);
	}

	//Rest of command buttons.
	int Y_BUTTON_OFFSET = Y_CMD_BUTTON + CY_CMD_BUTTON * 6;
	for ( ; wButtonTagI < BUTTON_COMMAND_COUNT; ++wButtonTagI)
	{
		//Command buttons.
		const MESSAGE_ID eMsgID = static_cast<MESSAGE_ID>(COMMAND_MIDS[wButtonTagI]);
		pButton = new CButtonWidget(ButtonTags[wButtonTagI],
			X_CMD_BUTTON, Y_BUTTON_OFFSET,
			CX_CMD_BUTTON, CY_CMD_BUTTON, g_pTheDB->GetMessageText(eMsgID));
		pTabbedMenu->AddWidgetToTab(pButton, COMMANDS_TAB);

		//Command labels.
		this->pCommandLabelWidgets[wButtonTagI] = new CLabelWidget(0,
				X_CMD_LABEL, Y_BUTTON_OFFSET,
				CX_CMD_LABEL, CY_CMD_LABEL, F_Small, wszEmpty);
		pTabbedMenu->AddWidgetToTab(this->pCommandLabelWidgets[wButtonTagI], COMMANDS_TAB);

		Y_BUTTON_OFFSET += CY_CMD_BUTTON + 1;
	}

	//Repeat rate.
	pTabbedMenu->AddWidgetToTab(
			new CLabelWidget(0, X_REPEATRATE_LABEL, Y_REPEATRATE_LABEL,
					CX_REPEATRATE_LABEL, CY_REPEATRATE_LABEL, F_Small,
					g_pTheDB->GetMessageText(MID_RepeatRate)), COMMANDS_TAB);
	pSliderWidget = new CSliderWidget(TAG_REPEATRATE, X_REPEATRATE,
			Y_REPEATRATE, CX_REPEATRATE, CY_REPEATRATE, 128);
	pTabbedMenu->AddWidgetToTab(pSliderWidget, COMMANDS_TAB);
	pTabbedMenu->AddWidgetToTab(
			new CLabelWidget(0, X_SLOW_LABEL, Y_SLOW_LABEL,
					CX_SLOW_LABEL, CY_SLOW_LABEL, F_Small, g_pTheDB->GetMessageText(MID_Slow)), COMMANDS_TAB);
	pTabbedMenu->AddWidgetToTab(
			new CLabelWidget(0, X_FAST_LABEL, Y_FAST_LABEL,
					CX_FAST_LABEL, CY_FAST_LABEL, F_Small, g_pTheDB->GetMessageText(MID_Fast)), COMMANDS_TAB);

	//Undo level.
	pTabbedMenu->AddWidgetToTab(
			new CLabelWidget(0, X_UNDOLEVEL_LABEL, Y_UNDOLEVEL_LABEL,
					CX_UNDOLEVEL_LABEL, CY_UNDOLEVEL_LABEL, F_Small,
					g_pTheDB->GetMessageText(MID_UndoLevel)), COMMANDS_TAB);
	pSliderWidget = new CSliderWidget(TAG_UNDO_LEVEL, X_UNDOLEVEL,
			Y_UNDOLEVEL, CX_UNDOLEVEL, CY_UNDOLEVEL, 1, BYTE(UNDO_LEVEL_INCREMENTS));
	pTabbedMenu->AddWidgetToTab(pSliderWidget, COMMANDS_TAB);

	pTabbedMenu->AddWidgetToTab(
			new CLabelWidget(TAG_UNDOLEVEL_NUM_LABEL, X_UNDOLEVEL_NUM_LABEL, Y_UNDOLEVEL_NUM_LABEL,
					CX_UNDOLEVEL_NUM_LABEL, CY_UNDOLEVEL_NUM_LABEL, F_Small, wszEmpty),
					COMMANDS_TAB);

	//Command default buttons.
	Y_BUTTON_OFFSET = CY_TABBEDMENU - CY_CMD_BUTTON - CY_SPACE;
	pButton = new CButtonWidget(TAG_DEFAULT_DESKTOP,
			X_CMD_BUTTON, Y_BUTTON_OFFSET,
			CX_CMD_BUTTON, CY_CMD_BUTTON, g_pTheDB->GetMessageText(MID_DefaultDesktop));
	pTabbedMenu->AddWidgetToTab(pButton, COMMANDS_TAB);

	pButton = new CButtonWidget(TAG_DEFAULT_LAPTOP,
			X_CMD_BUTTON + CX_CMD_BUTTON + CX_SPACE, Y_BUTTON_OFFSET,
			CX_CMD_BUTTON, CY_CMD_BUTTON, g_pTheDB->GetMessageText(MID_DefaultLaptop));
	pTabbedMenu->AddWidgetToTab(pButton, COMMANDS_TAB);

	//Okay, cancel and help buttons.
	pButton = new CButtonWidget(TAG_OK, X_OKAY_BUTTON, Y_OKAY_BUTTON,
				CX_OKAY_BUTTON, CY_OKAY_BUTTON, g_pTheDB->GetMessageText(MID_Okay));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_CANCEL, X_CANCEL_BUTTON, Y_CANCEL_BUTTON,
			CX_CANCEL_BUTTON, CY_CANCEL_BUTTON, g_pTheDB->GetMessageText(MID_Cancel));
	AddWidget(pButton);
	AddHotkey(SDLK_ESCAPE,TAG_CANCEL); 
	pButton = new CButtonWidget(TAG_HELP, X_HELP_BUTTON, Y_HELP_BUTTON, 
			CX_HELP_BUTTON, CY_HELP_BUTTON, g_pTheDB->GetMessageText(MID_Help)); 
	AddWidget(pButton); 
	AddHotkey(SDLK_F1,TAG_HELP);

	//Dialog box for reassigning a command key.
	static const int Y_COMMANDPROMPTLABEL = CY_SPACE;
	static const UINT CX_MESSAGE_BUTTON = 80;
	static const UINT CX_LABEL = CX_MESSAGE - CX_SPACE*2;
	static const UINT CY_LABEL = 25;
	static const int Y_COMMANDLABEL = Y_COMMANDPROMPTLABEL + CY_LABEL + CY_SPACE;
	this->pDialogBox = new CKeypressDialogWidget(0L,
			0, 0, CX_MESSAGE, CY_MESSAGE);
	AddWidget(this->pDialogBox);

	pTitle = new CLabelWidget(0L, 0, Y_COMMANDPROMPTLABEL,
			CX_LABEL, CY_LABEL, F_Small, g_pTheDB->GetMessageText(MID_GetKeyCommand));
	pTitle->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pDialogBox->AddWidget(pTitle);
	this->pCommandLabel = new CLabelWidget(0L, CX_SPACE, Y_COMMANDLABEL,
			CX_LABEL, CY_LABEL, F_Small, wszEmpty);
	this->pCommandLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pDialogBox->AddWidget(this->pCommandLabel);
	pButton = new CButtonWidget(TAG_CANCEL,
			(CX_MESSAGE-CX_MESSAGE_BUTTON) / 2,
			CY_MESSAGE-CY_STANDARD_BUTTON - CY_SPACE,
			CX_MESSAGE_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_CancelNoHotkey));
	this->pDialogBox->AddWidget(pButton);
	this->pDialogBox->Center();
	this->pDialogBox->Hide();
}

//************************************************************************************
CSettingsScreen::~CSettingsScreen()
{
	delete this->pCurrentPlayer;
}

//******************************************************************************
bool CSettingsScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	delete this->pCurrentPlayer;

	//Ensure all transactions are complete before tweaking profile settings.
	g_pTheNet->ClearActiveAction();

	//Load player data from database.
	this->pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	if (!this->pCurrentPlayer) return false;
	SetUnspecifiedPlayerSettings(this->pCurrentPlayer->Settings);
	UpdateWidgetsFromPlayerData(this->pCurrentPlayer);

	SelectFirstWidget(false);

	return true;
}

//
//Private methods.
//

//*****************************************************************************
void CSettingsScreen::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Event.
{
	CScreen::OnKeyDown(dwTagNo,Key);

	switch (Key.keysym.sym)
	{
		case SDLK_F10:
			//Keep screen size setting synchronized.
			SynchScreenSizeWidget();
		break;

		default: break;
	}

	switch (dwTagNo)
	{
		case TAG_MUSIC_VOLUME:
		case TAG_VOICES_VOLUME:
		case TAG_SOUNDEFF_VOLUME:
		{
			SDL_MouseButtonEvent fakeButton;
			OnDragUp(dwTagNo, fakeButton);
		}
		break;
	}

	SetWidgetStates();
}

//************************************************************************************
void CSettingsScreen::OnClick(const UINT dwTagNo)
//Called when widget receives click.
{
	//Conversion macros.
#  define BUTTONTAG_TO_DCMD(t)      (static_cast<DCMD>((t) - TAG_NW_BUTTON))

	switch (dwTagNo) {
					
		case TAG_ESCAPE:
		case TAG_CANCEL:
			RestorePlayerSettings();
			GoToScreen(SCR_Return);
		return;

		case TAG_OK:
		{
			//Verify all settings are valid.
			CTextBoxWidget *pTextBox = DYN_CAST(CTextBoxWidget*, CWidget*,
					GetWidget(TAG_NAME));
			if (WCSlen(pTextBox->GetText()) == 0)
			{
				ShowOkMessage(MID_EmptyNameError);
				break;
			}
			if (!AllCommandsAreAssignedToKeys(this->pCurrentPlayer->Settings))
			{
				ShowOkMessage(MID_DefineAllKeys);
				break;
			}
			bool bCNetDetailsChanged = AreCNetDetailsChanged(this->pCurrentPlayer);
			UpdatePlayerDataFromWidgets(this->pCurrentPlayer);
			delete this->pCurrentPlayer;
			this->pCurrentPlayer = NULL;
			
			if (bCNetDetailsChanged){
				//Use new player settings to get latest hold list.
				g_pTheNet->DownloadHoldList();	//must be done after UpdatePlayerData...
			}

			GoToScreen(SCR_Return);
			return;
		}

		case TAG_HELP:
			CBrowserScreen::SetPageToLoad("settings.html");
			GoToScreen(SCR_Browser);
		break;

		case TAG_DEFAULT_DESKTOP:
		case TAG_DEFAULT_LAPTOP:
			if (ShowYesNoMessage(MID_DefaultKeyCommandsPrompt) == TAG_YES)
			{
				//Revert key command settings to default.
				CFiles f;
				f.WriteGameProfileString(INISection::Localization, INIKey::Keyboard, dwTagNo == TAG_DEFAULT_LAPTOP ? "1" : "0");
				const UINT wKeyboard = dwTagNo == TAG_DEFAULT_LAPTOP ? 1 : 0;

				for (UINT wIndex = 0; wIndex<DCMD_Count; ++wIndex)
				{
					const SDL_Keycode nKey = COMMANDKEY_ARRAY[wKeyboard][wIndex];
					this->pCurrentPlayer->Settings.SetVar(COMMANDNAME_ARRAY[wIndex], nKey);
					DYN_CAST(CLabelWidget*, CWidget*, pCommandLabelWidgets[wIndex])->
							SetText(g_pTheDB->GetMessageText(KeyToMID(nKey)));
				}

				Paint();
			}
		break;

		case TAG_NW_BUTTON: case TAG_N_BUTTON: case TAG_NE_BUTTON:
		case TAG_W_BUTTON: case TAG_WAIT_BUTTON: case TAG_E_BUTTON:
		case TAG_SW_BUTTON: case TAG_S_BUTTON: case TAG_SE_BUTTON:
		case TAG_C_BUTTON: case TAG_CC_BUTTON: case TAG_RESTART_BUTTON:
		case TAG_UNDO_BUTTON: case TAG_BATTLE_BUTTON: case TAG_COMMAND_BUTTON:
		case TAG_CLONESWITCH_BUTTON:
		{
			DCMD eCommand = BUTTONTAG_TO_DCMD(dwTagNo);

			SDL_Keycode newKey, currentKey;
			currentKey = static_cast<SDL_Keycode>(this->pCurrentPlayer->Settings.GetVar(
				COMMANDNAME_ARRAY[eCommand], SDLK_UNKNOWN));
			if (GetCommandKeyRedefinition(eCommand, currentKey, newKey))
			{
				if (newKey != currentKey)
				{
					//Overwritten key commands set to undefined.
					for (int nCommand = DCMD_NW; nCommand < DCMD_Count; ++nCommand)
					{
						if (this->pCurrentPlayer->Settings.GetVar(
							COMMANDNAME_ARRAY[nCommand], 0)==newKey)
						{
							this->pCurrentPlayer->Settings.SetVar(
								COMMANDNAME_ARRAY[nCommand], SDLK_UNKNOWN);
							DYN_CAST(CLabelWidget*, CWidget*,
								this->pCommandLabelWidgets[nCommand])->
								SetText(g_pTheDB->GetMessageText(KeyToMID(SDLK_UNKNOWN)));
						}
					}

					//Update current player settings for this command to newKey.
					this->pCurrentPlayer->Settings.SetVar(
							COMMANDNAME_ARRAY[eCommand], newKey);

					//Update label of command that was changed.
					DYN_CAST(CLabelWidget*, CWidget*,
							this->pCommandLabelWidgets[eCommand])->
								SetText(g_pTheDB->GetMessageText(KeyToMID(newKey)));

					Paint();
				}
			} else {
				OnQuit();
				return;
			}
		}
		break;

		case TAG_SCREENSIZE:
		case TAG_ENABLE_SOUNDEFF:
		case TAG_ENABLE_VOICES:
		case TAG_ENABLE_MUSIC:
			SynchOption(dwTagNo);
		break;

		case TAG_REQUESTNEWKEY:
		{
			const string str = UnicodeToUTF8(pCaravelNetNameWidget->GetText());
			UINT wCaravelNetRequest = g_pTheNet->RequestNewKey(str);
			if (!wCaravelNetRequest) {
				ShowOkMessage(MID_CaravelNetUnreachable);
				break;
			}

			GenericNetTransactionWait(wCaravelNetRequest, MID_RequestingKey);
			Paint();

			CNetResult* pResult = g_pTheNet->GetResults(wCaravelNetRequest);
			// Buffer possibilities:
			//   '1' : Okay.  Email sent.
			//   '2' : Not registered.
			//   '3' : Registration Expired
			if (!pResult)
				break;
			switch (pResult->pJson->get("status", 1).asInt()) {
				case 1:
					ShowOkMessage(MID_KeySent);
					break;
				case 2:
					ShowOkMessage(MID_NotRegistered);
					break;
				case 3:
					ShowOkMessage(MID_RegistrationExpired);
					break;
				default:
					break;
			}
			delete pResult;
		}
		break;

		case TAG_UPLOADSCORES:
			UploadScores();
		break;
		case TAG_CLOUD_ACTIVATE:
			CloudActivate();
		break;
	}  //switch dwTagNo

#  undef BUTTONTAG_TO_DCMD
}

//************************************************************************************
void CSettingsScreen::OnDragUp(const UINT dwTagNo, const SDL_MouseButtonEvent &/*Button*/)
//Called when widget being dragged is released.
{
	COptionButtonWidget *pOptionButton;
	CSliderWidget *pSliderWidget;

	switch (dwTagNo) {
		case TAG_SOUNDEFF_VOLUME:
			pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
				GetWidget(TAG_ENABLE_SOUNDEFF));
			if (!pOptionButton->IsChecked())
			{
				pOptionButton->SetChecked(true);
				pOptionButton->Paint();
				g_pTheSound->EnableSoundEffects(true);
			}
			pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,
				GetWidget(dwTagNo));
			g_pTheSound->SetSoundEffectsVolume(pSliderWidget->GetValue());
			g_pTheSound->StopSoundEffect(SEID_ORBHIT);
			g_pTheSound->PlaySoundEffect(SEID_ORBHIT); //play sample sound
		break;

		case TAG_VOICES_VOLUME:
		{
			pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
				GetWidget(TAG_ENABLE_VOICES));
			if (!pOptionButton->IsChecked())
			{
				pOptionButton->SetChecked(true);
				pOptionButton->Paint();
				g_pTheSound->EnableVoices(true);
			}
			pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,
				GetWidget(dwTagNo));
			g_pTheSound->SetVoicesVolume(pSliderWidget->GetValue());
			const int nSoundVolume = g_pTheSound->GetSoundVolume();
			g_pTheSound->SetSoundEffectsVolume(g_pTheSound->GetVoiceVolume());
			g_pTheSound->StopSoundEffect(SEID_HALPHFOLLOWING);
			g_pTheSound->PlaySoundEffect(SEID_HALPHFOLLOWING); //play sample sound
			g_pTheSound->SetSoundEffectsVolume(nSoundVolume);
		}
		break;

		case TAG_MUSIC_VOLUME:
			pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
				GetWidget(TAG_ENABLE_MUSIC));
			if (!pOptionButton->IsChecked())
			{
				pOptionButton->SetChecked(true);
				pOptionButton->Paint();
				g_pTheSound->EnableMusic(true);
				g_pTheSound->PlaySong(SONGID_INTRO_TSS);
			}
			pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,
				GetWidget(dwTagNo));
			g_pTheSound->SetMusicVolume(pSliderWidget->GetValue());
		break;
	}
}

//*****************************************************************************
void CSettingsScreen::OnSelectChange(
//Handles a selection change.
//
//Params:
	const UINT dwTagNo)       //(in)   Widget that event applied to.
{
	switch (dwTagNo)
	{
		case TAG_GAMMA:
		{
			CSliderWidget *pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,
				GetWidget(dwTagNo));
			g_pTheDBM->SetGamma(pSliderWidget->GetValue());
		}
		break;
		case TAG_NO_FOCUS_PLAYS_MUSIC:
		{
			COptionButtonWidget *pOptionButton = DYN_CAST(COptionButtonWidget*,
				CWidget*, GetWidget(dwTagNo));
			g_pTheSound->bNoFocusPlaysMusic = pOptionButton->IsChecked();
		}
		break;
		case TAG_UNDO_LEVEL:
			SetUndoLevelNumLabel();
			Paint();
		break;
		default: break;
	}
	SynchOption(dwTagNo);
}

//************************************************************************************
void CSettingsScreen::RestorePlayerSettings()
//Upon cancelling, restore settings to the way they were.
{
	//Reload player data from database.  This instance of CDbPlayer won't have
	//changes made on this screen.
	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	UpdateWidgetsFromPlayerData(pPlayer);
	delete pPlayer;

	COptionButtonWidget *pOptionButton;
	CSliderWidget *pSliderWidget;

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_GAMMA));
	g_pTheDBM->SetGamma(pSliderWidget->GetValue());

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
		GetWidget(TAG_ENABLE_SOUNDEFF));
	g_pTheSound->EnableSoundEffects(pOptionButton->IsChecked());
	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,	GetWidget(TAG_SOUNDEFF_VOLUME));
	g_pTheSound->SetSoundEffectsVolume(pSliderWidget->GetValue());

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
		GetWidget(TAG_ENABLE_VOICES));
	g_pTheSound->EnableVoices(pOptionButton->IsChecked());
	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,	GetWidget(TAG_VOICES_VOLUME));
	g_pTheSound->SetVoicesVolume(pSliderWidget->GetValue());

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_ENABLE_MUSIC));
	g_pTheSound->EnableMusic(pOptionButton->IsChecked());
	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_MUSIC_VOLUME));
	g_pTheSound->SetMusicVolume(pSliderWidget->GetValue());
}

//************************************************************************************
void CSettingsScreen::SetUndoLevelNumLabel()
{
	CSliderWidget *pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,
		GetWidget(TAG_UNDO_LEVEL));

	CLabelWidget *pLabel = DYN_CAST(CLabelWidget*,
		CWidget*, GetWidget(TAG_UNDOLEVEL_NUM_LABEL));
	
	const UINT val = pSliderWidget->GetValue();
	WSTRING wstr;
	if (val >= UNDO_LEVEL_INCREMENTS-1) {
		wstr = g_pTheDB->GetMessageText(MID_UnlimitedUndo);
	} else {
		WCHAR temp[10];
		wstr = _itoW(UNDO_SETTING_LEVELS[val], temp, 10);
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_Turns);
	}
	pLabel->SetText(wstr.c_str());
}

//************************************************************************************
void CSettingsScreen::SetUnspecifiedPlayerSettings(
//Any player setting that is unspecified will get a default value.  This is where the
//default settings for a player are defined.
//
//Params:
	CDbPackedVars  &Settings)  //(in/out)  When received, param may contain any number
								//       of vars that match or don't match expected
								//       vars.  Returned with all expected vars set.
{
	//Set-if-missing macros.
#  define SETMISSING(name, value) if (!Settings.DoesVarExist(name)) Settings.SetVar(name, value);

	SETMISSING(Settings::Language, Language::English);
	
	SETMISSING(Settings::Fullscreen, IsFullScreen());
	SETMISSING(Settings::Alpha, true);
	SETMISSING(Settings::Gamma, (BYTE)CDrodBitmapManager::GetGammaOne());
	SETMISSING(Settings::EyeCandy, BYTE(Metadata::GetInt(MetaKey::MAX_EYE_CANDY)));
	SETMISSING(Settings::TarstuffAlpha, BYTE(255));

	SETMISSING(Settings::Music, true);
	SETMISSING(Settings::SoundEffects, true);
	SETMISSING(Settings::Voices, true);
	SETMISSING(Settings::MusicVolume, (BYTE)DEFAULT_MUSIC_VOLUME);
	SETMISSING(Settings::SoundEffectsVolume, (BYTE)DEFAULT_SOUND_VOLUME);
	SETMISSING(Settings::VoicesVolume, (BYTE)DEFAULT_VOICE_VOLUME);
	SETMISSING(Settings::ShowSubtitlesWithVoices, true);

	SETMISSING(Settings::ShowCheckpoints, true);
	SETMISSING(Settings::AutoSaveOptions, ASO_DEFAULT | ASO_CONQUERDEMO);

	SETMISSING(Settings::AutoSave, true);
	SETMISSING(Settings::ItemTips, true);
	SETMISSING(Settings::GEMI, false);

	SETMISSING(Settings::RepeatRate, (BYTE)128);

	SETMISSING(Settings::ConnectToInternet, false);

	//Check whether default is for keyboard or laptop.
	string strKeyboard;
	UINT wKeyboard = 0;	//default to numpad
	if (CFiles::GetGameProfileString(INISection::Localization, INIKey::Keyboard, strKeyboard))
	{
		wKeyboard = atoi(strKeyboard.c_str());
		if (wKeyboard > 1) wKeyboard = 0;	//invalid setting
	}

	for (UINT wIndex = 0; wIndex<DCMD_Count; ++wIndex)
	{
		SETMISSING(COMMANDNAME_ARRAY[wIndex], COMMANDKEY_ARRAY[wKeyboard][wIndex]);

		//SDL1 key mapping migration
		const int nKey = Settings.GetVar(COMMANDNAME_ARRAY[wIndex], COMMANDKEY_ARRAY[wKeyboard][wIndex]);
		const bool bInvalidSDL1mapping = nKey >= 128 && nKey <= 323;
		if (bInvalidSDL1mapping)
			Settings.SetVar(COMMANDNAME_ARRAY[wIndex], COMMANDKEY_ARRAY[wKeyboard][wIndex]);
	}

#  undef SETMISSING
}

//************************************************************************************
void CSettingsScreen::SetWidgetStates()
//Enable/disable widgets depending on state.
{
	CTabbedMenuWidget *pMenu = DYN_CAST(CTabbedMenuWidget*, CWidget*, GetWidget(TAG_MENU));
	const bool bButtonsVisible = pMenu->GetSelectedTab() == PERSONAL_TAB;

	CButtonWidget *pButton = DYN_CAST(CButtonWidget*, CWidget*,	GetWidget(TAG_REQUESTNEWKEY));
	ASSERT(pButton);
	pButton->Enable(WCSlen(this->pCaravelNetNameWidget->GetText()) > 0);
	if (bButtonsVisible)
		pButton->RequestPaint();

	const CIDSet& holdIds = g_pTheNet->GetLocalHolds();
	pButton = DYN_CAST(CButtonWidget*, CWidget*,	GetWidget(TAG_UPLOADSCORES));
	ASSERT(pButton);
	pButton->Enable(!holdIds.empty());
	if (bButtonsVisible)
		pButton->RequestPaint();
}

//************************************************************************************
void CSettingsScreen::SynchOption(const UINT dwTagNo)
//When an event regarding these widgets are received, provide feedback to user
//and update engine state to match.
{
	COptionButtonWidget *pOptionButton;
	switch (dwTagNo)
	{
		case TAG_SCREENSIZE:
			//Keep screen size button synchronized.
			SynchScreenSizeWidget();
		break;

		case TAG_ENABLE_SOUNDEFF:
			pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(dwTagNo));
			g_pTheSound->EnableSoundEffects(pOptionButton->IsChecked());
			g_pTheSound->PlaySoundEffect(SEID_ORBHIT); //play sample sound
		break;

		case TAG_ENABLE_VOICES:
			pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(dwTagNo));
			if (g_pTheSound->EnableVoices(pOptionButton->IsChecked()))
			{
				const int nSoundVolume = g_pTheSound->GetSoundVolume();
				g_pTheSound->SetSoundEffectsVolume(g_pTheSound->GetVoiceVolume());
				g_pTheSound->PlaySoundEffect(SEID_HALPHFOLLOWING, NULL, NULL, true); //play sample sound
				g_pTheSound->SetSoundEffectsVolume(nSoundVolume);
			}
		break;

		case TAG_ENABLE_MUSIC:
			pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(dwTagNo));
			g_pTheSound->EnableMusic(pOptionButton->IsChecked());
			if (pOptionButton->IsChecked())
				g_pTheSound->PlaySong(SONGID_INTRO_TSS);
		break;
	}
}

//************************************************************************************
void CSettingsScreen::SynchScreenSizeWidget()
//Should be called when screen size changes.
{
	COptionButtonWidget *pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
		GetWidget(TAG_USE_FULLSCREEN));

	if (IsFullScreen() != pOptionButton->IsChecked())
	{
		pOptionButton->SetChecked(IsFullScreen());
		pOptionButton->Paint();
	}
}

//************************************************************************************
void CSettingsScreen::UpdateWidgetsFromPlayerData(
//Synchronizes all the widgets on the screen with player data.
//
//Params:
	CDbPlayer *pPlayer)  //(in)   Player data.
{
	ASSERT(pPlayer);
	CDbPackedVars& settings = pPlayer->Settings;

	//Personal settings.
	this->pNameWidget->SetText(pPlayer->NameText);
	this->pCaravelNetNameWidget->SetText(pPlayer->CNetNameText);
	this->pCaravelNetPasswordWidget->SetText(pPlayer->CNetPasswordText);

	//Video settings.
	CSliderWidget *pSliderWidget;
	COptionButtonWidget *pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_USE_FULLSCREEN));
	pOptionButton->SetChecked((CScreen::bAllowFullScreen && CScreen::bAllowWindowed) ?
			settings.GetVar(Settings::Fullscreen, IsFullScreen()) :
			IsFullScreen());

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_ALPHA));
	pOptionButton->SetChecked(settings.GetVar(Settings::Alpha, g_pTheBM->bAlpha));

	BYTE bytValue = settings.GetVar(Settings::Gamma, (BYTE)CDrodBitmapManager::GetGammaOne());
	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,	GetWidget(TAG_GAMMA));
	pSliderWidget->SetValue(bytValue);

	bytValue = settings.GetVar(Settings::EyeCandy, g_pTheBM->eyeCandy);
	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_ENVIRONMENT));
	pSliderWidget->SetValue(bytValue);

	bytValue = settings.GetVar(Settings::TarstuffAlpha, BYTE(g_pTheDBM->tarstuffAlpha));
	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_TARSTUFF_TRANSPARENCY));
	pSliderWidget->SetValue(bytValue - MIN_TARSTUFF_ALPHA);

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_DISPLAY_COMBOS));
	pOptionButton->SetChecked(settings.GetVar(Settings::DisplayCombos, true));

	//Sound settings.
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_ENABLE_SOUNDEFF));
	pOptionButton->SetChecked(settings.GetVar(Settings::SoundEffects, g_pTheSound->IsSoundEffectsOn()));
	
	BYTE bytVolume = settings.GetVar(Settings::SoundEffectsVolume, (BYTE)DEFAULT_SOUND_VOLUME);
	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_SOUNDEFF_VOLUME));
	pSliderWidget->SetValue(bytVolume);

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_ENABLE_VOICES));
	pOptionButton->SetChecked(settings.GetVar(Settings::Voices, g_pTheSound->IsSoundEffectsOn()));
	
	bytVolume = settings.GetVar(Settings::VoicesVolume, (BYTE)DEFAULT_VOICE_VOLUME);
	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,
			GetWidget(TAG_VOICES_VOLUME));
	pSliderWidget->SetValue(bytVolume);

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_ENABLE_MUSIC));
	pOptionButton->SetChecked(settings.GetVar(Settings::Music, g_pTheSound->IsMusicOn()));

	bytVolume = settings.GetVar(Settings::MusicVolume, (BYTE)DEFAULT_MUSIC_VOLUME);
	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,
			GetWidget(TAG_MUSIC_VOLUME));
	pSliderWidget->SetValue(bytVolume);

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_SHOW_SUBTITLES));
	pOptionButton->SetChecked(settings.GetVar(Settings::ShowSubtitlesWithVoices, true));

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_NO_FOCUS_PLAYS_MUSIC));
	pOptionButton->SetChecked(settings.GetVar(Settings::NoFocusPlaysMusic, false));

	//Special settings.
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_SHOWCHECKPOINTS));
	pOptionButton->SetChecked(settings.GetVar(Settings::ShowCheckpoints, true));
	
	const UINT dwAutoSaveOptions = settings.GetVar(Settings::AutoSaveOptions, 
			ASO_DEFAULT | ASO_CONQUERDEMO);
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_SAVEONCONQUER));
	pOptionButton->SetChecked((dwAutoSaveOptions & ASO_CONQUERDEMO) == ASO_CONQUERDEMO);
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_SAVEONDIE));
	pOptionButton->SetChecked((dwAutoSaveOptions & ASO_DIEDEMO) == ASO_DIEDEMO);

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_AUTOUNDOONDEATH));
	pOptionButton->SetChecked(settings.GetVar(Settings::AutoUndoOnDeath, false));

	//Editor settings.
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_AUTOSAVE));
	pOptionButton->SetChecked(settings.GetVar(Settings::AutoSave, true));
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_ITEMTIPS));
	pOptionButton->SetChecked(settings.GetVar(Settings::ItemTips, true));
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_GROUPEDITORMENUITEMS));
	pOptionButton->SetChecked(settings.GetVar(Settings::GEMI, false));
	//Check above line

	//Command settings.
	for (int nCommand = DCMD_NW; nCommand < DCMD_Count; ++nCommand)
	{
		const int nKey = settings.GetVar(COMMANDNAME_ARRAY[nCommand], SDLK_UNKNOWN);
		DYN_CAST(CLabelWidget*, CWidget*, pCommandLabelWidgets[nCommand])->SetText(
			g_pTheDB->GetMessageText(KeyToMID(SDL_Keycode(nKey))));
	}

	bytVolume = settings.GetVar(Settings::RepeatRate, (BYTE)128);
	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,
			GetWidget(TAG_REPEATRATE));
	pSliderWidget->SetValue(bytVolume);

	bytVolume = settings.GetVar(Settings::UndoLevel, (BYTE)1);
	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,
			GetWidget(TAG_UNDO_LEVEL));
	pSliderWidget->SetValue(bytVolume);

	SetUndoLevelNumLabel();

	//Internet settings.
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_LOGIN_TO_CARAVELNET));
	pOptionButton->SetChecked(settings.GetVar(Settings::ConnectToInternet, false));

	SetWidgetStates();
}

//************************************************************************************
void CSettingsScreen::UpdatePlayerDataFromWidgets(
//Synchronizes player data with widgets on screen.
//
//Params:
	CDbPlayer *pPlayer)  //(in/out)  Accepts loaded player, returns with members updated.
{
	ASSERT(pPlayer);

	CDbPackedVars& settings = pPlayer->Settings;

	//Personal settings.
	CTextBoxWidget *pTextBox = DYN_CAST(CTextBoxWidget*, CWidget*,
			GetWidget(TAG_NAME));
	pPlayer->NameText = pTextBox->GetText();

	pTextBox = DYN_CAST(CTextBoxWidget*, CWidget*, GetWidget(TAG_CNETNAME));
	pPlayer->CNetNameText = pTextBox->GetText();

	pTextBox = DYN_CAST(CTextBoxWidget*, CWidget*, GetWidget(TAG_CNETPASSWORD));
	pPlayer->CNetPasswordText = pTextBox->GetText();

	//Video settings.
	CSliderWidget *pSliderWidget;
	COptionButtonWidget *pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_USE_FULLSCREEN));
	settings.SetVar(Settings::Fullscreen, pOptionButton->IsChecked());
	SetFullScreen(pOptionButton->IsChecked());

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_ALPHA));
	settings.SetVar(Settings::Alpha, pOptionButton->IsChecked());
	g_pTheBM->bAlpha = pOptionButton->IsChecked();

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,	GetWidget(TAG_GAMMA));
	settings.SetVar(Settings::Gamma, pSliderWidget->GetValue());

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_ENVIRONMENT));
	settings.SetVar(Settings::EyeCandy, pSliderWidget->GetValue());
	g_pTheBM->eyeCandy = pSliderWidget->GetValue();

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_TARSTUFF_TRANSPARENCY));
	settings.SetVar(Settings::TarstuffAlpha, BYTE(pSliderWidget->GetValue() + MIN_TARSTUFF_ALPHA));
	g_pTheDBM->tarstuffAlpha = pSliderWidget->GetValue() + MIN_TARSTUFF_ALPHA;

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*, GetWidget(TAG_DISPLAY_COMBOS));
	settings.SetVar(Settings::DisplayCombos, pOptionButton->IsChecked());
	g_pTheDBM->bDisplayCombos = pOptionButton->IsChecked();

	//Sound settings.
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_ENABLE_SOUNDEFF));
	settings.SetVar(Settings::SoundEffects, pOptionButton->IsChecked());

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,
			GetWidget(TAG_SOUNDEFF_VOLUME));
	settings.SetVar(Settings::SoundEffectsVolume, pSliderWidget->GetValue());

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_ENABLE_VOICES));
	settings.SetVar(Settings::Voices, pOptionButton->IsChecked());

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,
			GetWidget(TAG_VOICES_VOLUME));
	settings.SetVar(Settings::VoicesVolume, pSliderWidget->GetValue());

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_ENABLE_MUSIC));
	settings.SetVar(Settings::Music, pOptionButton->IsChecked());

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*,
			GetWidget(TAG_MUSIC_VOLUME));
	settings.SetVar(Settings::MusicVolume, pSliderWidget->GetValue());

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_SHOW_SUBTITLES));
	settings.SetVar(Settings::ShowSubtitlesWithVoices, pOptionButton->IsChecked());

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_NO_FOCUS_PLAYS_MUSIC));
	settings.SetVar(Settings::NoFocusPlaysMusic, pOptionButton->IsChecked());
	g_pTheSound->bNoFocusPlaysMusic = pOptionButton->IsChecked();

	//Special settings.
	UINT dwAutoSaveOptions = ASO_ROOMBEGIN | ASO_LEVELBEGIN;
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_SHOWCHECKPOINTS));
	settings.SetVar(Settings::ShowCheckpoints, pOptionButton->IsChecked());
	if (pOptionButton->IsChecked()) dwAutoSaveOptions |= ASO_CHECKPOINT;

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_SAVEONCONQUER));
	if (pOptionButton->IsChecked()) dwAutoSaveOptions |= ASO_CONQUERDEMO;
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_SAVEONDIE));
	if (pOptionButton->IsChecked()) dwAutoSaveOptions |= ASO_DIEDEMO;
	settings.SetVar(Settings::AutoSaveOptions, dwAutoSaveOptions);

	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_AUTOUNDOONDEATH));
	settings.SetVar(Settings::AutoUndoOnDeath, pOptionButton->IsChecked());

	//Editor settings.
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_AUTOSAVE));
	settings.SetVar(Settings::AutoSave, pOptionButton->IsChecked());
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_ITEMTIPS));
	settings.SetVar(Settings::ItemTips, pOptionButton->IsChecked());
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_GROUPEDITORMENUITEMS));
	settings.SetVar(Settings::GEMI, pOptionButton->IsChecked());

	//Command settings--these were updated in response to previous UI events, 
	//so nothing to do here.
	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_REPEATRATE));
	settings.SetVar(Settings::RepeatRate, pSliderWidget->GetValue());

	pSliderWidget = DYN_CAST(CSliderWidget*, CWidget*, GetWidget(TAG_UNDO_LEVEL));
	settings.SetVar(Settings::UndoLevel, pSliderWidget->GetValue());

	//Internet settings.
	pOptionButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_LOGIN_TO_CARAVELNET));
	settings.SetVar(Settings::ConnectToInternet, pOptionButton->IsChecked());

	pPlayer->Update();
}

bool CSettingsScreen::AreCNetDetailsChanged(
	//Checks if any of the CNet details have been changed
	//
	//Params:
	CDbPlayer *pPlayer)  //(in)  Loaded player which is checked against the settings
{

	CTextBoxWidget *pTextBox;
	
	pTextBox = DYN_CAST(CTextBoxWidget*, CWidget*, GetWidget(TAG_CNETNAME));
	if (pPlayer->CNetNameText != pTextBox->GetText()){
		return true;
	}

	pTextBox = DYN_CAST(CTextBoxWidget*, CWidget*, GetWidget(TAG_CNETPASSWORD));
	if (pPlayer->CNetPasswordText != pTextBox->GetText()){
		return true;
	}

	return false;
}
//************************************************************************************
bool CSettingsScreen::PollForInterrupt()
//Returns: whether the user has prompted to halt the operation
{
	//Get any events waiting in the queue.
	//Prompt to halt the process on a key press.
	SDL_Event event;
	while (PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_WINDOWEVENT:
				OnWindowEvent(event.window);
				this->pStatusDialog->Paint();	//make sure this stays on top
			break;
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
					case SDLK_ESCAPE:
					case SDLK_SPACE:
					case SDLK_RETURN:	case SDLK_KP_ENTER:
					case SDLK_F4:
#if defined(__linux__) || defined(__FreeBSD__)
					case SDLK_PAUSE:
#endif
						if (ShowYesNoMessage(MID_HaltDemoUploadPrompt) != TAG_NO)
							return true;
						this->pStatusDialog->Paint();	//make sure this stays on top
					break;
					default: break;
				}
			break;
			default: break;
		}
	}
	return false;
}

//************************************************************************************
void CSettingsScreen::UploadScores()
//For each local hold that's on CaravelNet, upload the player's general
//progress and best demo for each room.
{
	//If player can't log in, this set will be empty.
	ASSERT(g_pTheNet);
	const CIDSet& holdIds = g_pTheNet->GetLocalHolds();
	if (holdIds.empty())
		return;
	const UINT dwPlayerID = this->pCurrentPlayer->dwPlayerID;
	ASSERT(dwPlayerID);

	const UINT dwCurrentHoldID = g_pTheDB->GetHoldID();

	SetCursor(CUR_Internet);
	g_pTheDB->Commit();  //commit any outstanding changes now
	ShowStatusMessage(MID_Exporting);
	CDrodScreen::Callbackf(0.0f);

	bool bContinue = true;

	//Part I.  Upload explored+conquered rooms and holds.
	string exploredRoomsTexts;
	CIDSet::const_iterator id;
	for (id = holdIds.begin(); id != holdIds.end(); ++id)
	{
		if (!bContinue)
			break;

		ExportHoldProgressForUpload(*id, dwPlayerID, exploredRoomsTexts);

		if (PollForInterrupt())
			bContinue = false;
	}
	//Upload explored+conquered stats for all rooms and holds in one sequential bundle.
	if (!exploredRoomsTexts.empty())
		g_pTheNet->UploadExploredRooms(exploredRoomsTexts);
	CDrodScreen::Callbackf(0.1f);
	g_pTheDB->Commit(); //keep saved key current

	if (!bContinue) goto Done;

	//Part II.  Upload demos.
	UploadUnsubmittedDemos(dwPlayerID);

	CDrodScreen::Callbackf(1.0f);

	g_pTheDB->Commit();

Done:
	g_pTheDB->SetHoldID(dwCurrentHoldID);
	ExportCleanup();
	Paint();
}

//************************************************************************************
void CSettingsScreen::UploadUnsubmittedDemos(const UINT dwPlayerID)
//Scanning demos can take a very long time.
//Mark which demos are processed so only new demo records are uploaded next time.
{
	//Get all player's demos.
	CDb db;
	db.Demos.FilterByPlayer(dwPlayerID);
	db.SavedGames.FindHiddens(true);
	CIDSet demoIDs, allDemoIDs = db.Demos.GetIDs();

	//Upload all demos if requested.
	bool bFullScoreUpload = false;
	CFiles f;
	string str;
	if (f.GetGameProfileString(INISection::Customizing, "FullScoreUpload", str))
		bFullScoreUpload = atoi(str.c_str()) != 0;

	UINT wCount = 0;
	CIDSet::const_iterator demo;
	std::map<UINT, CIDSet> demoIDhandles;
	for (demo = allDemoIDs.begin(); demo != allDemoIDs.end(); ++demo)
	{
		CDbDemo* pDemo = db.Demos.GetByID(*demo);
		ASSERT(pDemo);
		if (!pDemo) continue;

		//We are searching for victory demos, so don't upload death demos.
		const bool bSkip = pDemo->IsFlagSet(CDbDemo::Death) ||
			(pDemo->IsFlagSet(CDbDemo::TestedForUpload) && !bFullScoreUpload);
		delete pDemo;
		if (bSkip)
			continue;

		demoIDs += *demo;

		if ((++wCount % 100) == 0) //avoid large uploads
		{
			string text;
			if (CDbXML::ExportXML(V_Demos, demoIDs, text, (UINT)-1)) //no multi-room demos
			{
				const UINT handle = g_pTheNet->UploadDemos(text);
				if (handle) {
					demoIDhandles[handle] = demoIDs;
				}
				demoIDs.clear();
				CDrodScreen::Callbackf((float)wCount / (float)(allDemoIDs.size() * 2)); //[0-50%]
				g_pTheDB->Commit(); //keep saved key current
			}
		}

		if (PollForInterrupt())
			break;
	}
	if (!demoIDs.empty())
	{
		string text;
		if (CDbXML::ExportXML(V_Demos, demoIDs, text, (UINT)-1)) //no multi-room demos
		{
			const UINT handle = g_pTheNet->UploadDemos(text);
			if (handle) {
				demoIDhandles[handle] = demoIDs;
			}
			CDrodScreen::Callbackf((float)wCount / (float)(allDemoIDs.size() * 2));
		}
	}

	//Confirm server has responded to receiving the score submissions.
	bool bAllConfirmed = true;
	for (std::map<UINT, CIDSet>::const_iterator it = demoIDhandles.begin(); it != demoIDhandles.end(); it++)
	{
		const UINT handle = it->first;
		const CIDSet demoIDs = it->second;

		const UINT MAX_TRIES = 200; //200 * 50ms = 10s total wait time for each response
		UINT tries;
		for (tries = 0; tries < MAX_TRIES; ++tries)
		{
			const int status = g_pTheNet->GetStatus(handle);
			if (status < 0)
			{
				SDL_Delay(50);
				continue;
			}

			CNetResult* pBuffer = g_pTheNet->GetResults(handle);
			if (pBuffer) {
				delete pBuffer;

				//Now flag all uploaded demos.
				for (demo = demoIDs.begin(); demo != demoIDs.end(); ++demo)
				{
					CDbDemo* pDemo = db.Demos.GetByID(*demo);
					ASSERT(pDemo);
					pDemo->SetFlag(CDbDemo::TestedForUpload); //don't submit this demo next time
					pDemo->Update();
					delete pDemo;
				}
			} else {
				//server did not respond with a confirmation
				bAllConfirmed = false;
			}
			break;
		}
		if (tries == MAX_TRIES)
			bAllConfirmed = false;

		wCount += demoIDs.size();
		CDrodScreen::Callbackf((float)wCount / (float)(allDemoIDs.size() * 2)); //[50-100%]
	}

	if (bAllConfirmed) {
		//Player scores on CaravelNet are now current.
		this->pCurrentPlayer->Settings.SetVar(Settings::CNetProgressIsOld, (BYTE)0);

		if (bFullScoreUpload) //reset after completing operation
			f.WriteGameProfileString(INISection::Customizing, "FullScoreUpload", "0");
	}
}

//************************************************************************************
void CSettingsScreen::Paint(
//Paint the screen.
//
//Params:
	bool bUpdateRect)             //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	SDL_BlitSurface(this->images[0], NULL, GetDestSurface(), NULL);

	PaintChildren();
	if (bUpdateRect) UpdateRect();
}

//************************************************************************************
bool CSettingsScreen::GetCommandKeyRedefinition(
//Returns false if SDL_Quit/ALT-F4 occurred, true otherwise. 
//
//Params:
	const DCMD eCommand,    //(in) Command being redefined
	const SDL_Keycode CurrentKey,   //(in)
	SDL_Keycode &NewKey)            //(out)
{
	const MESSAGE_ID eButtonMID = COMMAND_MIDS[eCommand];
	this->pCommandLabel->SetText(g_pTheDB->GetMessageText(eButtonMID));

	UINT dwRetTagNo;
	SDL_Keycode DialogKey = SDLK_UNKNOWN;
	bool bInvalidKey; 
	do 
	{
		dwRetTagNo = this->pDialogBox->Display();
		if (dwRetTagNo == TAG_QUIT || dwRetTagNo == TAG_CANCEL ||
				dwRetTagNo == TAG_ESCAPE) break;
		DialogKey = this->pDialogBox->GetKey();
		bInvalidKey = (DialogKey >= SDLK_F1 && DialogKey <= SDLK_F12) || (DialogKey >= SDLK_F13);
		if (bInvalidKey)
			ShowOkMessage(MID_InvalidCommandKey);
	} while (bInvalidKey);
	Paint();

	if (dwRetTagNo == TAG_QUIT)
		return false;
	if (dwRetTagNo == TAG_CANCEL || dwRetTagNo == TAG_ESCAPE)
	{
		NewKey = CurrentKey;
		return true;
	}

	NewKey = DialogKey;
	return true;
}

//*****************************************************************************
bool CSettingsScreen::AllCommandsAreAssignedToKeys(
//Returns whether all commands are assigned to keys.
//(Assuming the policy that each command be assigned to a unique key is enforced.)
//
//Params:
	CDbPackedVars &Settings) //(in)  Player settings.
const
{
	for (int nCommand = DCMD_NW; nCommand < DCMD_Count; ++nCommand)
	{
		if (Settings.GetVar(COMMANDNAME_ARRAY[nCommand], SDLK_UNKNOWN) == SDLK_UNKNOWN)
			return false;
	}
	return true;
}

//*****************************************************************************
void CSettingsScreen::CloudActivate()
{
	ShowOkMessage(MID_CaravelNetCloudExplanation);
	if (ShowYesNoMessage(MID_PerformCloudSynchPrompt) != TAG_YES)
		return;

	//Ensure current inputted credentials are saved and ready to use for authentication
	ASSERT(this->pCurrentPlayer);
	if (!this->pCurrentPlayer)
		return;
	UpdatePlayerDataFromWidgets(this->pCurrentPlayer);

	UINT handle = g_pTheNet->CloudGetPlayer();
	if (!handle) {
		ShowOkMessage(MID_CaravelNetAuthError);
		return;
	}
	GenericNetTransactionWait(handle, MID_CloudSynchInProgress, 0.0f, 0.01f);

	CNetResult* pResult = g_pTheNet->GetResults(handle);
	if (!pResult) {
		Paint();
		ShowOkMessage(MID_CloudSynchError);
		return;
	}
	if (pResult->pJson) {
		const string cloudPlayerXML = pResult->pJson->get("player", "").asString();
		if (!cloudPlayerXML.empty()) {
			// A player profile has already been uploaded to CaravelNet.
			// Abort uploading this one.
			Paint();
			ShowOkMessage(MID_CloudPlayerAlreadyUploaded);
			return;
		}
		delete pResult;
	}

	// If we get here, there was no player file on the server.  Just do an Initialize, which will
	// push the current player to the cloud.  
	handle = g_pTheNet->CloudInitialize();
	GenericNetTransactionWait(handle, MID_CloudSynchInProgress, 0.01f, 0.1f);

	pResult = g_pTheNet->GetResults(handle);
	if (pResult && pResult->pJson) {
		if (pResult->pJson->isMember("holdsNeeded")) {
			const Json::Value& holds = (*(pResult->pJson))["holdsNeeded"];
			//Upload one hold at a time.
			for (UINT x = 0; x < holds.size(); ++x) {
				CDbMessageText author, holdName;
				const CDate date(holds[x].get("created", 0).asInt());
				WSTRING wstr;
				Base64::decode(holds[x].get("architect", "").asString(), wstr);
				author = wstr.c_str();

				Base64::decode(holds[x].get("name", "").asString(), wstr);
				holdName = wstr.c_str();

				const UINT dwHoldId = g_pTheDB->Holds.GetHoldID(date, holdName, author);
				if (dwHoldId > 0) {
					handle = g_pTheNet->CloudUploadGeneralHold(dwHoldId);
				}
			}
		}
	}
	delete pResult;

	// Uploading holds is a background operation.  Wait for the queue to empty.
	CloudTransactionWait(MID_CloudSynchInProgress, 0.0, 100.0);

	// And after the initialize, push all the player progress.
	bool bSuccess = true;

	//Upload player's saved games and demos for each hold.
	const UINT dwHoldCount = g_pTheDB->Holds.GetViewSize();
	for (UINT dwHoldI = 0; dwHoldI < dwHoldCount; ++dwHoldI) {
		c4_RowRef row = g_pTheDB->Holds.GetRowRef(V_Holds, dwHoldI);
		const UINT dwHoldID = UINT(p_HoldID(row));
		handle = g_pTheNet->CloudUploadProgress(dwHoldID);
	}
	CloudTransactionWait(MID_CloudSynchInProgress);

	if (bSuccess) {
		CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
		pPlayer->Settings.SetVar(Settings::CloudActivated, true);
		pPlayer->Update();
		delete pPlayer;
	}

	Paint();
	ShowOkMessage(bSuccess ? MID_CloudSynchSuccessful : MID_CloudSynchError);
}

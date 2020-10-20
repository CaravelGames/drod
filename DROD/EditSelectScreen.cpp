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
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//#define ENABLE_CHEATS

#include "EditSelectScreen.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"
#include "DrodScreenManager.h"
#include "DrodSound.h"
#include "BrowserScreen.h"
#include "EditRoomScreen.h"
#include "GameScreen.h"

#include "MapWidget.h"
#include "EditRoomWidget.h"
#include "EntranceSelectDialogWidget.h"
#include "WeatherDialogWidget.h"

#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/ImageWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/OptionButtonWidget.h>
#include <FrontEndLib/ScalerWidget.h>
#include <FrontEndLib/TextBoxWidget.h>
#include <FrontEndLib/TabbedMenuWidget.h>

#include "../DRODLib/CueEvents.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbHolds.h"
#include "../DRODLib/DbLevels.h"
#include "../DRODLib/DbProps.h"
#include "../DRODLib/DbRooms.h"
#include "../DRODLib/DbXML.h"
#include "../DRODLib/SettingsKeys.h"

#include "../Texts/MIDs.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Clipboard.h>
#include <BackEndLib/Coord.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>

//Widget tags.
const UINT TAG_NEW_HOLD = 1014;
const UINT TAG_RENAME_LEVEL = 1015;
const UINT TAG_REDESC_LEVEL = 1016;
const UINT TAG_SHOWEXITLEVEL = 1017;

const UINT TAG_HOLD_AUTHOR_LABEL = 1020;
const UINT TAG_LEVEL_AUTHOR_LABEL = 1021;
const UINT TAG_LEVEL_DATE_LABEL = 1022;
const UINT TAG_POSITION_LABEL = 1023;
const UINT TAG_ROOM_IS_REQUIRED = 1024;
const UINT TAG_WHO_CAN_EDIT_LBOX = 1025;
const UINT TAG_ROOM_IS_SECRET = 1026;

const UINT TAG_HOLD_LBOX = 1030;
const UINT TAG_LEVEL_LBOX = 1031;
const UINT TAG_STYLE_LBOX = 1032;

const UINT TAG_COPY_HOLD = 1033;
const UINT TAG_DELETE_LEVEL = 1034;
const UINT TAG_MANAGE = 1035;
const UINT TAG_MANAGE_MODS = 1036;

const UINT TAG_MINIROOM = 1040;

const UINT TAG_WEATHER = 1050;
const UINT TAG_WEATHER_DIALOG = 1051;

const UINT TAG_HOLD_SETTINGS = 1060;
const UINT TAG_HOLD_SETTINGS_DIALOG = 1061;
const UINT TAG_RENAME_HOLD = 1062;
const UINT TAG_REDESC_HOLD = 1063;
const UINT TAG_ENDING_MESSAGE = 1064;
const UINT TAG_HOLD_SETTINGS_OK = 1065;

const UINT TAG_LEVELMENU = 1070;
const UINT TAG_WORLDMAP_LBOX = 1071;
const UINT TAG_RENAME_WORLDMAP = 1072;
const UINT TAG_SETIMAGE_WORLDMAP = 1073;
const UINT TAG_DELETE_WORLDMAP = 1074;
const UINT TAG_WORLDMAP_IMAGE = 1075;
const UINT TAG_MINIWORLDMAP = 1076;

const UINT TAG_WORLDMAPDISPLAYLIST = 1077;
const UINT TAG_WORLDMAPSETTINGSFRAME = 1078;
const UINT TAG_LEVELSETTINGSFRAME = 1079;

const UINT TAG_EDIT = 1091;
const UINT TAG_CANCEL = 1092;
const UINT TAG_HELP = 1093;

const SDL_Color DarkRed = {92, 0, 0, 0}; //authored hold tint

#ifdef RUSSIAN_BUILD
//Style names.
static const WCHAR wstrAboveground[] = {1053, 1072, 32, 1047, 1077, 1084, 1083, 1077, 0};
static const WCHAR wstrCity[] = {1043, 1086, 1088, 1086, 1076, 0};
static const WCHAR wstrDeepSpaces[] = {1043, 1083, 1091, 1073, 1086, 1082, 1080, 1077, 32, 1052, 1077, 1089, 1090, 1072, 0};
static const WCHAR wstrFortress[] = {1050, 1088, 1077, 1087, 1086, 1089, 1090, 1100, 0};
static const WCHAR wstrFoundation[] = {1060, 1091, 1085, 1076, 1072, 1084, 1077, 1085, 1090, 0};
static const WCHAR wstrIceworks[] = {1051, 1077, 1076, 1087, 1088, 1086, 1074, 0};
#endif

//Level tabbed menu
#define LEVEL_TAB (0)
#define WORLDMAP_TAB (1)
#define BG_COLOR 220,210,190

const UINT ADD_LEVEL_ID = static_cast<UINT>(-1);
const UINT ADD_WORLDMAP_ID = static_cast<UINT>(-1);

//
//Public methods.
//

//*****************************************************************************
UINT CEditSelectScreen::GetPrevHoldID() const
//Returns the selected hold ID, or 0 if none.
{
	return this->dwPrevHoldID;
}

//*****************************************************************************
void CEditSelectScreen::SetToCopiedHold(
//Updates widgets to point to new copy of hold made in the Edit Room screen.
//
//Params:
	CDbHold *pHold, CDbLevel *pLevel)
{
	delete this->pSelectedHold;
	this->pSelectedHold = g_pTheDB->Holds.GetByID(pHold->dwHoldID);
	delete this->pSelectedLevel;
	this->pSelectedLevel = g_pTheDB->Levels.GetByID(pLevel->dwLevelID);

	PopulateHoldListBox();
	PopulateLevelListBox();
	this->pHoldListBoxWidget->SelectItem(pHold->dwHoldID);

	//Update map.
	UINT dwX, dwY;
	this->pSelectedLevel->GetStartingRoomCoords(dwX,dwY);
	this->pMapWidget->LoadFromLevel(this->pSelectedLevel);
}

//
//Protected methods.
//

//*****************************************************************************
CEditSelectScreen::CEditSelectScreen()
	: CDrodScreen(SCR_EditSelect)
	, pLevelListBoxWidget(NULL), pHoldListBoxWidget(NULL)
	, pStyleListBoxWidget(NULL)
	, pWorldMapListBoxWidget(NULL)
	, pEntranceBox(NULL)
	, pSelectedHold(NULL), pSelectedLevel(NULL), pSelectedRoom(NULL)
	, dwPrevHoldID(0)
	, selectedWorldMapID(0)
	, pLevelCopy(NULL)
	, addLevelAfterEvents(false), addWorldMapAfterEvents(false)
	, pMapWidget(NULL)
	, pRoomWidget(NULL)
	, pScaledRoomWidget(NULL), pScaledWorldMapWidget(NULL)
	, pWorldMapImage(NULL)
//Constructor.
{
	SetKeyRepeat(66);

	this->imageFilenames.push_back(string("Background"));

	//Title bar
	static const UINT CX_SPACE = 10;
	static const UINT CY_SPACE = 10;
	static const UINT CY_TITLE = 50;
	static const UINT CY_TITLE_SPACE = 15;
	static const int Y_TITLE = CY_TITLE_SPACE;

	//Buttons
#ifdef RUSSIAN_BUILD
	static const UINT CX_EDIT_BUTTON = 130;
	static const UINT CX_HELP_BUTTON = 110;
#else
	static const UINT CX_EDIT_BUTTON = 100;
	static const UINT CX_HELP_BUTTON = CX_EDIT_BUTTON;
#endif
	static const UINT CY_EDIT_BUTTON = CY_STANDARD_BUTTON;
	static const UINT CX_CANCEL_BUTTON = 170;
	static const UINT CY_CANCEL_BUTTON = CY_STANDARD_BUTTON;
	static const UINT CY_HELP_BUTTON = CY_STANDARD_BUTTON;
	const int X_HELP_BUTTON = this->w - CX_HELP_BUTTON - CX_SPACE;
	const int X_CANCEL_BUTTON = X_HELP_BUTTON - CX_CANCEL_BUTTON - CX_SPACE;
	const int X_EDIT_BUTTON = X_CANCEL_BUTTON - CX_EDIT_BUTTON - CX_SPACE;
	const int Y_EDIT_BUTTON = this->h - CY_SPACE - CY_EDIT_BUTTON;
	const int Y_CANCEL_BUTTON = Y_EDIT_BUTTON;
	const int Y_HELP_BUTTON = Y_EDIT_BUTTON;

	//Mini-room widget has strict proportions and its dimensions will define 
	//placement of most everything else.
	static const UINT CY_MINIROOM = CDrodBitmapManager::CY_ROOM / 2;
	//Width of mini-room must be proportional to regular room display.
	static const UINT CX_MINIROOM = CY_MINIROOM * CDrodBitmapManager::CX_ROOM /
			CDrodBitmapManager::CY_ROOM;
	const int Y_MINIROOM = Y_EDIT_BUTTON - CY_SPACE - CY_MINIROOM;
	static const int X_MINIROOM = this->w - CX_SPACE - CX_MINIROOM;
	static const UINT CX_ROOM_REQUIRED_BUTTON = 150;
	static const UINT CX_POSITION_LABEL = CX_MINIROOM - CX_ROOM_REQUIRED_BUTTON;
	static const UINT CY_ROOM_REQUIRED_BUTTON = CY_STANDARD_OPTIONBUTTON;
	static const UINT CY_POSITION_LABEL = CY_ROOM_REQUIRED_BUTTON;
	static const int X_POSITION_LABEL = X_MINIROOM;
	const int Y_POSITION_LABEL = Y_MINIROOM - CY_POSITION_LABEL;
	static const int X_ROOM_REQUIRED_BUTTON = X_MINIROOM + CX_MINIROOM - CX_ROOM_REQUIRED_BUTTON;
	const int Y_ROOM_REQUIRED_BUTTON = Y_MINIROOM - CY_ROOM_REQUIRED_BUTTON;
	static const UINT CX_ROOM_SECRET_BUTTON = 150;
	static const UINT CY_ROOM_SECRET_BUTTON = CY_ROOM_REQUIRED_BUTTON;
	static const int X_ROOM_SECRET_BUTTON = X_MINIROOM + CX_MINIROOM - CX_ROOM_SECRET_BUTTON;
	const int Y_ROOM_SECRET_BUTTON = Y_ROOM_REQUIRED_BUTTON - CY_ROOM_SECRET_BUTTON;

	//Map
	const UINT CX_MAP = this->w - CX_SPACE - CX_MINIROOM - CX_SPACE * 3;
	static const UINT CY_MAP = CY_MINIROOM;
	static const int X_MAP = CX_SPACE;
	const int Y_MAP = this->h - CY_MAP - CY_SPACE;

	//Hold selection.
	static const int X_CHOOSE_HOLD_LABEL = CX_SPACE;
	static const int Y_CHOOSE_HOLD_LABEL = Y_TITLE + CY_TITLE;
	static const UINT CX_CHOOSE_HOLD_LABEL = 135;
	static const UINT CY_CHOOSE_HOLD_LABEL = CY_STANDARD_BUTTON;
	static const int X_NEW_HOLD = X_CHOOSE_HOLD_LABEL + CX_CHOOSE_HOLD_LABEL + CX_SPACE;
	static const int Y_NEW_HOLD = Y_CHOOSE_HOLD_LABEL + 3;
	static const UINT CX_NEW_HOLD = 120;
	static const UINT CY_NEW_HOLD = CY_STANDARD_BUTTON;
	static const int X_HOLD_LBOX = X_CHOOSE_HOLD_LABEL;
	static const int Y_HOLD_LBOX = Y_CHOOSE_HOLD_LABEL + CY_CHOOSE_HOLD_LABEL + 7;
	static const UINT CX_HOLD_LBOX = X_NEW_HOLD + CX_NEW_HOLD - X_HOLD_LBOX + 1;
	static const UINT CY_HOLD_LBOX = 246; //11*22 + 4

	//Level menu
	const int X_TABBEDMENU = X_NEW_HOLD + CX_NEW_HOLD + CX_SPACE;
	const int Y_TABBEDMENU = Y_CHOOSE_HOLD_LABEL;
	const UINT CX_TABBEDMENU = 290;
	const UINT CY_TABBEDMENU = CY_CHOOSE_HOLD_LABEL + CY_SPACE + CY_HOLD_LBOX + 45;

	const UINT CY_MENU_TAB = 35;
	const int Y_INNERMENU = CY_MENU_TAB + CY_SPACE/2;

	//Level selection.
	static const int X_LEVEL_LBOX = CX_SPACE;
	static const int Y_LEVEL_LBOX = Y_INNERMENU;
	static const UINT CX_LEVEL_LBOX = CX_TABBEDMENU - 2*X_LEVEL_LBOX;
	static const UINT CY_LEVEL_LBOX = CY_HOLD_LBOX;

	//Style selection.
	static const UINT CX_CHOOSE_STYLE_LABEL = 160;
	const int X_CHOOSE_STYLE_LABEL = this->w - CX_CHOOSE_STYLE_LABEL;
	static const int Y_CHOOSE_STYLE_LABEL = Y_CHOOSE_HOLD_LABEL - 3;
	static const UINT CY_CHOOSE_STYLE_LABEL = CY_STANDARD_BUTTON;
	const int X_STYLE_LBOX = X_CHOOSE_STYLE_LABEL;
	static const int Y_STYLE_LBOX = Y_CHOOSE_STYLE_LABEL + CY_CHOOSE_STYLE_LABEL;
	const UINT CX_STYLE_LBOX = CX_CHOOSE_STYLE_LABEL - CX_SPACE;;
	static const UINT CY_STYLE_LBOX = 136;

	const int X_MANAGEMODS = X_STYLE_LBOX;
	static const int Y_MANAGEMODS = Y_STYLE_LBOX + CY_STYLE_LBOX;
	static const UINT CX_MANAGEMODS = CX_STYLE_LBOX;
	static const UINT CY_MANAGEMODS = CY_STANDARD_BUTTON;

	static const UINT CX_WEATHER = 100;
	const int X_WEATHER = X_STYLE_LBOX;
	static const int Y_WEATHER = Y_MANAGEMODS + CY_MANAGEMODS + 4;

	//Hold operation buttons.
	static const int X_COPY_HOLD = X_CHOOSE_HOLD_LABEL;
	static const int Y_COPY_HOLD = Y_HOLD_LBOX + CY_HOLD_LBOX + CY_SPACE;
	static const UINT CX_COPY_HOLD = 80;
	static const UINT CY_COPY_HOLD = CY_STANDARD_BUTTON;
#ifdef RUSSIAN_BUILD
	static const int X_HOLD_SETTINGS = X_COPY_HOLD + CX_COPY_HOLD + 5;
	static const UINT CX_HOLD_SETTINGS = CX_COPY_HOLD + 20;
	static const int X_MANAGE = X_HOLD_SETTINGS + CX_HOLD_SETTINGS + 5;
#else
	static const int X_HOLD_SETTINGS = X_COPY_HOLD + CX_COPY_HOLD + CX_SPACE;
	static const UINT CX_HOLD_SETTINGS = CX_COPY_HOLD;
	static const int X_MANAGE = X_HOLD_SETTINGS + CX_HOLD_SETTINGS + CX_SPACE;
#endif
	static const int Y_HOLD_SETTINGS = Y_COPY_HOLD;
	static const UINT CY_HOLD_SETTINGS = CY_COPY_HOLD;
	static const int Y_MANAGE = Y_COPY_HOLD;
	static const UINT CX_MANAGE = CX_COPY_HOLD;
	static const UINT CY_MANAGE = CY_COPY_HOLD;

	//Hold settings (Author and editing privileges).
	static const int X_HOLD_FRAME = X_MINIROOM;
	static const int Y_HOLD_FRAME = Y_CHOOSE_HOLD_LABEL + CY_SPACE;
	const int CX_HOLD_FRAME = X_CHOOSE_STYLE_LABEL - X_HOLD_FRAME - CX_SPACE;

	static const int X_HOLD_AUTHOR_LABEL = CX_SPACE;
	static const int Y_HOLD_AUTHOR_LABEL = CY_SPACE/2;
	static const UINT CX_HOLD_AUTHOR_LABEL = 64;
	static const UINT CY_HOLD_AUTHOR_LABEL = 22;
	static const int X_HOLD_AUTHOR = X_HOLD_AUTHOR_LABEL + CX_HOLD_AUTHOR_LABEL + CX_SPACE;
	static const int Y_HOLD_AUTHOR = Y_HOLD_AUTHOR_LABEL;
	const UINT CX_HOLD_AUTHOR = X_CHOOSE_STYLE_LABEL - X_HOLD_AUTHOR - CX_SPACE;
	static const UINT CY_HOLD_AUTHOR = CY_HOLD_AUTHOR_LABEL + 2;

	static const int X_WHO_CAN_EDIT = CX_SPACE;
	const UINT CX_WHO_CAN_EDIT = CX_HOLD_FRAME - 2 * X_WHO_CAN_EDIT;
	static const UINT CY_WHO_CAN_EDIT = 32;
	static const int Y_WHO_CAN_EDIT = Y_HOLD_AUTHOR_LABEL + CY_HOLD_AUTHOR;
	static const int X_WHO_CAN_EDIT_LBOX = X_WHO_CAN_EDIT;
	static const int Y_WHO_CAN_EDIT_LBOX = Y_WHO_CAN_EDIT + CY_WHO_CAN_EDIT;
	const UINT CX_WHO_CAN_EDIT_LBOX = X_WHO_CAN_EDIT + CX_WHO_CAN_EDIT - X_WHO_CAN_EDIT_LBOX + 1;
	static const UINT CY_WHO_CAN_EDIT_LBOX = 92;

	static const int CY_HOLD_FRAME = Y_WHO_CAN_EDIT_LBOX + CY_WHO_CAN_EDIT_LBOX + CY_SPACE;

	//Level settings.
	static const UINT CX_RENAME_LEVEL = 80;
	static const int X_RENAME_LEVEL = X_LEVEL_LBOX + 5;
	static const int Y_RENAME_LEVEL = Y_LEVEL_LBOX + CY_LEVEL_LBOX + CY_SPACE-1;
	static const UINT CY_RENAME_LEVEL = CY_STANDARD_BUTTON;
	static const UINT CX_REDESC_LEVEL = CX_RENAME_LEVEL;
	static const int X_REDESC_LEVEL = X_RENAME_LEVEL + CX_RENAME_LEVEL + CX_SPACE;
	static const int Y_REDESC_LEVEL = Y_RENAME_LEVEL;
	static const UINT CY_REDESC_LEVEL = CY_STANDARD_BUTTON;
	static const int X_DELETE_LEVEL = X_REDESC_LEVEL + CX_REDESC_LEVEL + CX_SPACE;
	static const int Y_DELETE_LEVEL = Y_RENAME_LEVEL;
	static const UINT CX_DELETE_LEVEL = CX_RENAME_LEVEL;
	static const UINT CY_DELETE_LEVEL = CY_STANDARD_BUTTON;

	static const int X_LEVEL_FRAME = X_MINIROOM;
	static const int Y_LEVEL_FRAME = Y_HOLD_FRAME + CY_HOLD_FRAME + CY_SPACE;
	const int CX_LEVEL_FRAME = X_CHOOSE_STYLE_LABEL - X_LEVEL_FRAME - CX_SPACE;
	const int CY_LEVEL_FRAME = Y_POSITION_LABEL - Y_LEVEL_FRAME;
	static const int X_LEVEL_AUTHOR_LABEL = CX_SPACE;
	static const int Y_LEVEL_AUTHOR_LABEL = CY_SPACE/2;
#ifdef RUSSIAN_BUILD
	static const UINT CX_LEVEL_AUTHOR_LABEL = 70;
#else
	static const UINT CX_LEVEL_AUTHOR_LABEL = 60;
#endif
	static const UINT CY_LEVEL_AUTHOR_LABEL = CY_HOLD_AUTHOR_LABEL;
	static const int X_LEVEL_DATE_LABEL = X_LEVEL_AUTHOR_LABEL;
	static const int Y_LEVEL_DATE_LABEL = Y_LEVEL_AUTHOR_LABEL + CY_LEVEL_AUTHOR_LABEL;
	static const UINT CX_LEVEL_DATE_LABEL = CX_LEVEL_AUTHOR_LABEL;
	static const UINT CY_LEVEL_DATE_LABEL = CY_LEVEL_AUTHOR_LABEL;
	static const int X_SHOWEXITLEVEL = X_LEVEL_AUTHOR_LABEL;
	static const int Y_SHOWEXITLEVEL = Y_LEVEL_DATE_LABEL + CY_LEVEL_DATE_LABEL;
	static const UINT CX_SHOWEXITLEVEL = CX_LEVEL_FRAME - CX_SPACE;
	static const UINT CY_SHOWEXITLEVEL = CY_STANDARD_OPTIONBUTTON;

	static const int X_LEVEL_AUTHOR = X_LEVEL_AUTHOR_LABEL + CX_LEVEL_AUTHOR_LABEL + CX_SPACE;
	static const int Y_LEVEL_AUTHOR = Y_LEVEL_AUTHOR_LABEL;
	static const UINT CX_LEVEL_AUTHOR = CX_HOLD_AUTHOR;
	static const UINT CY_LEVEL_AUTHOR = CY_LEVEL_AUTHOR_LABEL + 3;
	static const int X_LEVEL_DATE = X_LEVEL_AUTHOR;
	static const int Y_LEVEL_DATE = Y_LEVEL_DATE_LABEL;
	static const UINT CX_LEVEL_DATE = CX_LEVEL_AUTHOR;
	static const UINT CY_LEVEL_DATE = CY_LEVEL_DATE_LABEL + 3;

	//Hold settings dialog.
#ifdef RUSSIAN_BUILD
	static const UINT CX_HOLD_SETTINGS_DIALOG = 420;
	static const UINT CX_HOLD_SETTINGS_LABEL = 250;
	static const UINT CX_RENAME_HOLD = 120;
#else
	static const UINT CX_HOLD_SETTINGS_DIALOG = 300;
	static const UINT CX_HOLD_SETTINGS_LABEL = 130;
	static const UINT CX_RENAME_HOLD = 80;
#endif
	static const UINT CY_HOLD_SETTINGS_DIALOG = 150;

	static const int Y_HOLD_SETTINGS_LABEL = CY_SPACE;
	static const UINT CY_HOLD_SETTINGS_LABEL = 40;
	static const int X_HOLD_SETTINGS_LABEL = (CX_HOLD_SETTINGS_DIALOG - CX_HOLD_SETTINGS_LABEL) / 2;

	static const UINT CY_HOLD_SETTINGS_OKAY = CY_STANDARD_BUTTON;
	static const UINT CX_HOLD_SETTINGS_OKAY = 80;
	static const int Y_HOLD_SETTINGS_OKAY = CY_HOLD_SETTINGS_DIALOG - CY_HOLD_SETTINGS_LABEL - CY_SPACE;
	static const int X_HOLD_SETTINGS_OKAY = (CX_HOLD_SETTINGS_DIALOG - CX_HOLD_SETTINGS_OKAY) / 2;
	
	static const int X_RENAME_HOLD = 2*CX_SPACE;
	static const int Y_RENAME_HOLD = Y_HOLD_SETTINGS_LABEL + CY_HOLD_SETTINGS_LABEL;
	static const UINT CY_RENAME_HOLD = CY_STANDARD_BUTTON;
	static const int X_REDESC_HOLD = X_RENAME_HOLD + CX_RENAME_HOLD + CX_SPACE;
	static const int Y_REDESC_HOLD = Y_RENAME_HOLD;
	static const UINT CX_REDESC_HOLD = CX_RENAME_HOLD;
	static const UINT CY_REDESC_HOLD = CY_STANDARD_BUTTON;
	static const int X_ENDING_MESSAGE = X_REDESC_HOLD + CX_REDESC_HOLD + CX_SPACE;
	static const int Y_ENDING_MESSAGE = Y_REDESC_HOLD;
	static const UINT CX_ENDING_MESSAGE = CX_RENAME_HOLD;
	static const UINT CY_ENDING_MESSAGE = CY_STANDARD_BUTTON;

	CButtonWidget *pButton;

	//Title.
	CLabelWidget *pTitle = new CLabelWidget(0L, 0, Y_TITLE,
			this->w, CY_TITLE, F_Title, g_pTheDB->GetMessageText(MID_Editor));
	pTitle->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(pTitle);

	//Edit, cancel and help buttons.
	pButton = new CButtonWidget(TAG_EDIT, X_EDIT_BUTTON, Y_EDIT_BUTTON,
				CX_EDIT_BUTTON, CY_EDIT_BUTTON, g_pTheDB->GetMessageText(MID_EditRoom));
	pButton->Disable();
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_CANCEL, X_CANCEL_BUTTON, Y_CANCEL_BUTTON, 
				CX_CANCEL_BUTTON, CY_CANCEL_BUTTON, g_pTheDB->GetMessageText(MID_ReturnToGame));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_HELP, X_HELP_BUTTON, Y_HELP_BUTTON, 
				CX_HELP_BUTTON, CY_HELP_BUTTON, g_pTheDB->GetMessageText(MID_Help));
	AddWidget(pButton);
	AddHotkey(SDLK_F1,TAG_HELP);

	//Hold selection area.
	AddWidget(new CLabelWidget(0L, X_CHOOSE_HOLD_LABEL, Y_CHOOSE_HOLD_LABEL,
				CX_CHOOSE_HOLD_LABEL, CY_CHOOSE_HOLD_LABEL, F_Header,
				g_pTheDB->GetMessageText(MID_ChooseHold)));
	pButton = new CButtonWidget(TAG_NEW_HOLD, X_NEW_HOLD, Y_NEW_HOLD,
				CX_NEW_HOLD, CY_NEW_HOLD, g_pTheDB->GetMessageText(MID_NewHold));
	AddWidget(pButton);
	this->pHoldListBoxWidget = new CListBoxWidget(TAG_HOLD_LBOX,
			X_HOLD_LBOX, Y_HOLD_LBOX, CX_HOLD_LBOX, CY_HOLD_LBOX, true);
	AddWidget(this->pHoldListBoxWidget);
	this->pHoldListBoxWidget->IgnoreLeadingArticlesInSort();

	//Hold buttons.
	pButton = new CButtonWidget(TAG_COPY_HOLD, X_COPY_HOLD, Y_COPY_HOLD,
				CX_COPY_HOLD, CY_COPY_HOLD, g_pTheDB->GetMessageText(MID_Copy));
	pButton->Disable();
	AddWidget(pButton);
	pButton = new CButtonWidget(TAG_HOLD_SETTINGS, X_HOLD_SETTINGS, Y_HOLD_SETTINGS,
				CX_HOLD_SETTINGS, CY_HOLD_SETTINGS, g_pTheDB->GetMessageText(MID_Settings));
	pButton->Disable();
	AddWidget(pButton);
	pButton = new CButtonWidget(TAG_MANAGE, X_MANAGE, Y_MANAGE,
				CX_MANAGE, CY_MANAGE, g_pTheDB->GetMessageText(MID_Manage));
	AddWidget(pButton);

	//Level selection tabbed menu.
	CTabbedMenuWidget *pTabbedMenu = new CTabbedMenuWidget(TAG_LEVELMENU, X_TABBEDMENU,
			Y_TABBEDMENU, CX_TABBEDMENU, CY_TABBEDMENU, 2, CY_MENU_TAB, BG_COLOR);
	pTabbedMenu->SetTabText(LEVEL_TAB, g_pTheDB->GetMessageText(MID_LevelSelectTab));
	pTabbedMenu->SetTabText(WORLDMAP_TAB, g_pTheDB->GetMessageText(MID_WorldMapSelectTab));
	pTabbedMenu->SetBGImage("Background", 128);
	AddWidget(pTabbedMenu);

	//Level selection area.
	this->pLevelListBoxWidget = new CListBoxWidget(TAG_LEVEL_LBOX,
			X_LEVEL_LBOX, Y_LEVEL_LBOX, CX_LEVEL_LBOX, CY_LEVEL_LBOX, false, true);
	pTabbedMenu->AddWidgetToTab(this->pLevelListBoxWidget, LEVEL_TAB);

	pButton = new CButtonWidget(TAG_RENAME_LEVEL, X_RENAME_LEVEL, Y_RENAME_LEVEL, 
				CX_RENAME_LEVEL, CY_RENAME_LEVEL, g_pTheDB->GetMessageText(MID_Rename));
	pButton->Disable();
	pTabbedMenu->AddWidgetToTab(pButton, LEVEL_TAB);
	pButton = new CButtonWidget(TAG_REDESC_LEVEL, X_REDESC_LEVEL, Y_REDESC_LEVEL,
				CX_REDESC_LEVEL, CY_REDESC_LEVEL, g_pTheDB->GetMessageText(MID_Describe));
	pButton->Disable();
	pTabbedMenu->AddWidgetToTab(pButton, LEVEL_TAB);
	pButton = new CButtonWidget(TAG_DELETE_LEVEL, X_DELETE_LEVEL, Y_DELETE_LEVEL,
				CX_DELETE_LEVEL, CY_DELETE_LEVEL, g_pTheDB->GetMessageText(MID_DeleteNoHotkey));
	pButton->Disable();
	pTabbedMenu->AddWidgetToTab(pButton, LEVEL_TAB);

	//World map selection area.
	this->pWorldMapListBoxWidget = new CListBoxWidget(TAG_WORLDMAP_LBOX,
			X_LEVEL_LBOX, Y_LEVEL_LBOX, CX_LEVEL_LBOX, CY_LEVEL_LBOX, false, true);
	pTabbedMenu->AddWidgetToTab(this->pWorldMapListBoxWidget, WORLDMAP_TAB);
	pButton = new CButtonWidget(TAG_RENAME_WORLDMAP, X_RENAME_LEVEL, Y_RENAME_LEVEL, 
				CX_RENAME_LEVEL, CY_RENAME_LEVEL, g_pTheDB->GetMessageText(MID_Rename));
	pButton->Disable();
	pTabbedMenu->AddWidgetToTab(pButton, WORLDMAP_TAB);
	pButton = new CButtonWidget(TAG_SETIMAGE_WORLDMAP, X_REDESC_LEVEL, Y_REDESC_LEVEL,
				CX_REDESC_LEVEL, CY_REDESC_LEVEL, g_pTheDB->GetMessageText(MID_WorldMapSetImage));
	pButton->Disable();
	pTabbedMenu->AddWidgetToTab(pButton, WORLDMAP_TAB);
	pButton = new CButtonWidget(TAG_DELETE_WORLDMAP, X_DELETE_LEVEL, Y_DELETE_LEVEL,
				CX_DELETE_LEVEL, CY_DELETE_LEVEL, g_pTheDB->GetMessageText(MID_DeleteNoHotkey));
	pButton->Disable();
	pTabbedMenu->AddWidgetToTab(pButton, WORLDMAP_TAB);

	this->pScaledWorldMapWidget = new CScalerWidget(TAG_MINIWORLDMAP, X_MINIROOM, Y_MINIROOM, 
			CX_MINIROOM, CY_MINIROOM, false);
	AddWidget(this->pScaledWorldMapWidget);
	this->pWorldMapImage = new CImageWidget(TAG_WORLDMAP_IMAGE, 0, 0, (SDL_Surface*)NULL);
	this->pScaledWorldMapWidget->AddScaledWidget(this->pWorldMapImage);

	//Hold settings area.
	CFrameWidget *pHoldSettingsFrame = new CFrameWidget(0L, X_HOLD_FRAME,
			Y_HOLD_FRAME, CX_HOLD_FRAME, CY_HOLD_FRAME,
			g_pTheDB->GetMessageText(MID_HoldSettings));
	AddWidget(pHoldSettingsFrame);

	pHoldSettingsFrame->AddWidget(new CLabelWidget(0L, X_HOLD_AUTHOR_LABEL,
			Y_HOLD_AUTHOR_LABEL, CX_HOLD_AUTHOR_LABEL, CY_HOLD_AUTHOR_LABEL,
			F_Small, g_pTheDB->GetMessageText(MID_LevelBy)));
	pHoldSettingsFrame->AddWidget(new CLabelWidget(TAG_HOLD_AUTHOR_LABEL,
			X_HOLD_AUTHOR, Y_HOLD_AUTHOR, CX_HOLD_AUTHOR, CY_HOLD_AUTHOR,
			F_Small, wszEmpty));

	pHoldSettingsFrame->AddWidget(new CLabelWidget(0L,
			X_WHO_CAN_EDIT, Y_WHO_CAN_EDIT, CX_WHO_CAN_EDIT, CY_WHO_CAN_EDIT,
			F_Small, g_pTheDB->GetMessageText(MID_WhoCanEdit)));
	CListBoxWidget *pWhoCanEditListBox = new CListBoxWidget(TAG_WHO_CAN_EDIT_LBOX,
			X_WHO_CAN_EDIT_LBOX, Y_WHO_CAN_EDIT_LBOX, CX_WHO_CAN_EDIT_LBOX,
			CY_WHO_CAN_EDIT_LBOX);
	pWhoCanEditListBox->AddItem(CDbHold::Anyone, g_pTheDB->GetMessageText(MID_Anyone));
	pWhoCanEditListBox->AddItem(CDbHold::YouAndConquerors, g_pTheDB->GetMessageText(MID_YouAndConquerors));
	pWhoCanEditListBox->AddItem(CDbHold::YouAndMasters, g_pTheDB->GetMessageText(MID_YouAndMasters));
	pWhoCanEditListBox->AddItem(CDbHold::OnlyYou, g_pTheDB->GetMessageText(MID_OnlyYou));
	pHoldSettingsFrame->AddWidget(pWhoCanEditListBox);

	//Level settings area.
	CFrameWidget *pLevelSettingsFrame = new CFrameWidget(TAG_LEVELSETTINGSFRAME,
			X_LEVEL_FRAME, Y_LEVEL_FRAME, CX_LEVEL_FRAME, CY_LEVEL_FRAME,
			g_pTheDB->GetMessageText(MID_LevelSettings));
	AddWidget(pLevelSettingsFrame);

	pLevelSettingsFrame->AddWidget(new CLabelWidget(0L, X_LEVEL_AUTHOR_LABEL,
			Y_LEVEL_AUTHOR_LABEL, CX_LEVEL_AUTHOR_LABEL, CY_LEVEL_AUTHOR_LABEL,
			F_Small, g_pTheDB->GetMessageText(MID_LevelBy)));
	pLevelSettingsFrame->AddWidget(new CLabelWidget(0L, X_LEVEL_DATE_LABEL,
			Y_LEVEL_DATE_LABEL, CX_LEVEL_DATE_LABEL, CY_LEVEL_DATE_LABEL, F_Small,
			g_pTheDB->GetMessageText(MID_LevelCreated)));
	pLevelSettingsFrame->AddWidget(new CLabelWidget(TAG_LEVEL_AUTHOR_LABEL,
			X_LEVEL_AUTHOR, Y_LEVEL_AUTHOR, CX_LEVEL_AUTHOR, CY_LEVEL_AUTHOR,
			F_Small, wszEmpty));
	pLevelSettingsFrame->AddWidget(new CLabelWidget(TAG_LEVEL_DATE_LABEL,
			X_LEVEL_DATE, Y_LEVEL_DATE, CX_LEVEL_DATE, CY_LEVEL_DATE, F_Small,
			wszEmpty));
	COptionButtonWidget *pOpButton = new COptionButtonWidget(TAG_SHOWEXITLEVEL,
			X_SHOWEXITLEVEL, Y_SHOWEXITLEVEL, CX_SHOWEXITLEVEL, CY_SHOWEXITLEVEL,
			g_pTheDB->GetMessageText(MID_ShowLevelExit));
	pLevelSettingsFrame->AddWidget(pOpButton);

	const UINT CY_WORLDMAP_SETTINGS_FRAME = CY_LEVEL_FRAME + CY_POSITION_LABEL - CY_SPACE;
	CFrameWidget *pWorldMapSettingsFrame = new CFrameWidget(TAG_WORLDMAPSETTINGSFRAME,
			X_LEVEL_FRAME, Y_LEVEL_FRAME, CX_LEVEL_FRAME, CY_WORLDMAP_SETTINGS_FRAME,
			g_pTheDB->GetMessageText(MID_WorldMapSettings));
	AddWidget(pWorldMapSettingsFrame);
	pWorldMapSettingsFrame->Hide();

	const int X_WORLDMAP_DISPLAY_LISTBOX = CX_SPACE;
	const int Y_WORLDMAP_DISPLAY_LISTBOX = 2*CY_SPACE;
	const UINT CX_WORLDMAP_DISPLAY_LISTBOX = CX_LEVEL_FRAME - 2*X_WORLDMAP_DISPLAY_LISTBOX;
	const UINT CY_WORLDMAP_DISPLAY_LISTBOX = 3*22 + 4;

	CListBoxWidget *pWorldMapDisplayTypeListBox = new CListBoxWidget(TAG_WORLDMAPDISPLAYLIST,
			X_WORLDMAP_DISPLAY_LISTBOX, Y_WORLDMAP_DISPLAY_LISTBOX,
			CX_WORLDMAP_DISPLAY_LISTBOX, CY_WORLDMAP_DISPLAY_LISTBOX);
	pWorldMapDisplayTypeListBox->AddItem(HoldWorldMap::NoLabels, g_pTheDB->GetMessageText(MID_WorldMapNoLabels));
	pWorldMapDisplayTypeListBox->AddItem(HoldWorldMap::Labels, g_pTheDB->GetMessageText(MID_WorldMapShowLabels));
	pWorldMapDisplayTypeListBox->AddItem(HoldWorldMap::LabelsWhenExplored, g_pTheDB->GetMessageText(MID_WorldMapDisplayLabelsWhenExplored));
	pWorldMapSettingsFrame->AddWidget(pWorldMapDisplayTypeListBox);

	//Room style selection area.
	AddWidget(new CLabelWidget(0L, X_CHOOSE_STYLE_LABEL, Y_CHOOSE_STYLE_LABEL,
				CX_CHOOSE_STYLE_LABEL, CY_CHOOSE_STYLE_LABEL, F_Header,
				g_pTheDB->GetMessageText(MID_RoomStyle)));
	this->pStyleListBoxWidget = new CListBoxWidget(TAG_STYLE_LBOX,
			X_STYLE_LBOX, Y_STYLE_LBOX, CX_STYLE_LBOX, CY_STYLE_LBOX, true);
	AddWidget(this->pStyleListBoxWidget);

	pButton = new CButtonWidget(TAG_MANAGE_MODS, X_MANAGEMODS, Y_MANAGEMODS,
				CX_MANAGEMODS, CY_MANAGEMODS, g_pTheDB->GetMessageText(MID_Manage));
	AddWidget(pButton);

	//Weather customization control.
	pButton = new CButtonWidget(TAG_WEATHER, X_WEATHER, Y_WEATHER,
				CX_WEATHER, CY_STANDARD_BUTTON, g_pTheDB->GetMessageText(MID_WeatherButton));
	pButton->Disable();
	AddWidget(pButton);

	CWeatherDialogWidget *pWeatherDialog = new CWeatherDialogWidget(TAG_WEATHER_DIALOG);
	AddWidget(pWeatherDialog);
	pWeatherDialog->Center();
	pWeatherDialog->Hide();

	//Room view.
	this->pScaledRoomWidget = new CScalerWidget(TAG_MINIROOM, X_MINIROOM, Y_MINIROOM, 
			CX_MINIROOM, CY_MINIROOM, false);
	AddWidget(this->pScaledRoomWidget);
	this->pRoomWidget = new CEditRoomWidget(0L, 0, 0, CDrodBitmapManager::CX_ROOM,
			CDrodBitmapManager::CY_ROOM);
	this->pRoomWidget->SetAnimateMoves(false);
	this->pScaledRoomWidget->AddScaledWidget(this->pRoomWidget);
	AddWidget(new CLabelWidget(TAG_POSITION_LABEL, X_POSITION_LABEL, Y_POSITION_LABEL, 
				CX_POSITION_LABEL, CY_POSITION_LABEL, F_Small, wszEmpty));
	pOpButton = new COptionButtonWidget(TAG_ROOM_IS_SECRET,
			X_ROOM_SECRET_BUTTON, Y_ROOM_SECRET_BUTTON, CX_ROOM_SECRET_BUTTON,
			CY_ROOM_SECRET_BUTTON, g_pTheDB->GetMessageText(MID_RoomIsSecret), false);
	pOpButton->Disable();
	AddWidget(pOpButton);
	pOpButton = new COptionButtonWidget(TAG_ROOM_IS_REQUIRED,
			X_ROOM_REQUIRED_BUTTON, Y_ROOM_REQUIRED_BUTTON, CX_ROOM_REQUIRED_BUTTON,
			CY_ROOM_REQUIRED_BUTTON, g_pTheDB->GetMessageText(MID_RoomIsRequired), false);
	pOpButton->Disable();
	AddWidget(pOpButton);

	//Room selection area.
	CScrollableWidget *pScrollingMap = new CScrollableWidget(0, X_MAP, Y_MAP,
			CX_MAP, CY_MAP);
	AddWidget(pScrollingMap);
	this->pMapWidget = new CMapWidget(TAG_MAP, 0, 0,
			CDrodBitmapManager::DISPLAY_COLS, CDrodBitmapManager::DISPLAY_ROWS, NULL);
	pScrollingMap->AddWidget(this->pMapWidget);

	//Level list dialog box.
	this->pEntranceBox = new CEntranceSelectDialogWidget(0L);
	AddWidget(this->pEntranceBox);
	this->pEntranceBox->Center();
	this->pEntranceBox->Hide();

	//Hold settings dialog box.
	CDialogWidget *pDialog = new CDialogWidget(TAG_HOLD_SETTINGS_DIALOG, 0, 0,
			CX_HOLD_SETTINGS_DIALOG, CY_HOLD_SETTINGS_DIALOG);
	AddWidget(pDialog);
	pDialog->Center();
	pDialog->Hide();
	pDialog->AddWidget(new CLabelWidget(0L, X_HOLD_SETTINGS_LABEL,
			Y_HOLD_SETTINGS_LABEL, CX_HOLD_SETTINGS_LABEL, CY_HOLD_SETTINGS_LABEL,
			F_Message, g_pTheDB->GetMessageText(MID_HoldSettings)));
	pButton = new CButtonWidget(TAG_RENAME_HOLD, X_RENAME_HOLD, Y_RENAME_HOLD,
			CX_RENAME_HOLD, CY_RENAME_HOLD, g_pTheDB->GetMessageText(MID_Rename));
	pDialog->AddWidget(pButton);
	pButton = new CButtonWidget(TAG_REDESC_HOLD, X_REDESC_HOLD, Y_REDESC_HOLD,
			CX_REDESC_HOLD, CY_REDESC_HOLD, g_pTheDB->GetMessageText(MID_Describe));
	pDialog->AddWidget(pButton);
	pButton = new CButtonWidget(TAG_ENDING_MESSAGE, X_ENDING_MESSAGE, Y_ENDING_MESSAGE,
			CX_ENDING_MESSAGE, CY_ENDING_MESSAGE, g_pTheDB->GetMessageText(MID_EndHoldMessage));
	pDialog->AddWidget(pButton);
	pButton = new CButtonWidget(TAG_HOLD_SETTINGS_OK, X_HOLD_SETTINGS_OKAY, Y_HOLD_SETTINGS_OKAY,
			CX_HOLD_SETTINGS_OKAY, CY_HOLD_SETTINGS_OKAY, g_pTheDB->GetMessageText(MID_Okay));
	pDialog->AddWidget(pButton);

	//Prepare style selection list box once.
	PopulateStyleListBox();
}

//*****************************************************************************
CEditSelectScreen::~CEditSelectScreen()
{
	FreeMembers();
}

//*****************************************************************************
void CEditSelectScreen::FreeMembers()
//Release DB members.
{
	ResetSelectedHold();

	delete this->pLevelCopy;
	this->pLevelCopy = NULL;

	//Clear other widgets.
	this->pHoldListBoxWidget->Clear();
	CListBoxWidget *pListBox = DYN_CAST(CListBoxWidget*, CWidget*,
			GetWidget(TAG_WHO_CAN_EDIT_LBOX));
	pListBox->SelectLine(0);
}

//*****************************************************************************
void CEditSelectScreen::ResetSelectedHold()
//Resets editor state to displaying no hold information.
{
	//Reset data objects.
	delete this->pSelectedHold;
	this->pSelectedHold = NULL;
	delete this->pSelectedLevel;
	this->pSelectedLevel = NULL;
	delete this->pSelectedRoom;
	this->pSelectedRoom = NULL;

	this->selectedWorldMapID = 0;

	GetLevelEntrancesInRoom(); //will clear list

	//Clear widgets.
	this->pLevelListBoxWidget->Clear();
	this->pRoomWidget->ResetRoom();
	this->pMapWidget->ClearMap();

	this->pWorldMapListBoxWidget->Clear();
	this->pWorldMapImage->SetImage((SDL_Surface*)NULL);
	this->pScaledWorldMapWidget->RefreshSize();

	CLabelWidget *pLabel = DYN_CAST(CLabelWidget*, CWidget*,
			GetWidget(TAG_HOLD_AUTHOR_LABEL));
	pLabel->SetText(wszEmpty);
	pLabel = DYN_CAST(CLabelWidget*, CWidget*,
			GetWidget(TAG_LEVEL_AUTHOR_LABEL));
	pLabel->SetText(wszEmpty);
	pLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAG_LEVEL_DATE_LABEL));
	pLabel->SetText(wszEmpty);

	COptionButtonWidget *pOpButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_ROOM_IS_REQUIRED));
	pOpButton->SetChecked(false);
	pOpButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_ROOM_IS_SECRET));
	pOpButton->SetChecked(false);
	pOpButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_SHOWEXITLEVEL));
	pOpButton->SetChecked(false);

	this->pStyleListBoxWidget->SelectLine(0);
}

//*****************************************************************************
WSTRING CEditSelectScreen::GetSelectedStyle() const
//Returns: the style of the selected room, or empty string if no room is selected
{
	if (this->pSelectedRoom)
		return this->pSelectedRoom->style;
	return wszEmpty;
}

//*****************************************************************************
bool CEditSelectScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	SetCursor(CUR_Wait);

	//Load EditRoomScreen now to be able to access its level/room pointers.
	if (!g_pTheSM->GetScreen(SCR_EditRoom))
	{
		SetCursor();
		ShowOkMessage(MID_CouldNotLoadResources);
		return false;
	}

   PopulateStyleListBox();

	//Get widgets and current hold/level/room views ready.
	if (!SetWidgets()) {
		SetCursor();
		return false;
	}

	this->dwPrevHoldID = 0;
	SelectFirstWidget(false);

	SetCursor();

	return true;
}

//
//Private methods.
//

//*****************************************************************************
void CEditSelectScreen::OnBetweenEvents()
{
	if (this->addLevelAfterEvents) {
		this->addLevelAfterEvents = false;
		if (!AddLevel()) {
			if (this->pSelectedLevel) {
				this->pLevelListBoxWidget->SelectItem(this->pSelectedLevel->dwLevelID); //revert
			} else {
				this->pLevelListBoxWidget->DeselectAll();
				this->pLevelListBoxWidget->UnsetCursorLine();
			}
		}
		Paint();
	}

	if (this->addWorldMapAfterEvents) {
		this->addWorldMapAfterEvents = false;
		if (!AddWorldMap()) {
			if (this->selectedWorldMapID) {
				this->pWorldMapListBoxWidget->SelectItem(this->selectedWorldMapID); //revert
			} else {
				this->pWorldMapListBoxWidget->DeselectAll();
				this->pWorldMapListBoxWidget->UnsetCursorLine();
			}
		}
		Paint();
	}
}

//*****************************************************************************
void CEditSelectScreen::OnClick(
//Called when widget receives a click event.
//
//Params:
	const UINT dwTagNo) //(in)   Widget that event applied to.
{
	UINT dwAnswerTagNo;
	WSTRING wstr;

	switch (dwTagNo)
	{
		case TAG_ESCAPE:
		case TAG_CANCEL:
		{
			LOGCONTEXT("CEditSelectScreen::OnClick--TAG_CANCEL");
			g_pTheDB->Commit();
			FreeMembers();

			GoToScreen(SCR_Return);
		}
		break;

		case TAG_HELP:
			CBrowserScreen::SetPageToLoad("editselect.html");
			GoToScreen(SCR_Browser);
		break;

		case TAG_EDIT:
		case TAG_MINIROOM:
			GoToRoomEditor();
		break;

		case TAG_HOLD_SETTINGS:
			EditHoldSettings();
		break;

		case TAG_NEW_HOLD:
			AddHold();
			Paint();
		break;

		case TAG_RENAME_LEVEL:
			RenameLevel();
		break;

		case TAG_REDESC_LEVEL:
		{
			//Update description for level's main entrance here
			if (!this->pSelectedLevel) break;
			ASSERT(this->pSelectedHold);
			CEntranceData *pEntrance = this->pSelectedHold->GetMainEntranceForLevel(
					this->pSelectedLevel->dwLevelID);
			ASSERT(pEntrance);
			wstr = pEntrance->DescriptionText;
			dwAnswerTagNo = ShowTextInputMessage(MID_DescribeLevel, wstr, true);
			if (dwAnswerTagNo == TAG_OK)
			{
				if (!ModifyHold())
					break;
				pEntrance->DescriptionText = wstr.c_str();
				this->pSelectedHold->Update();
			}
		}
		break;

		case TAG_COPY_HOLD:
			CopyHold();
		break;

		case TAG_DELETE_LEVEL:
			DeleteLevel();
		break;

		case TAG_RENAME_WORLDMAP:
			RenameWorldMap();
		break;

		case TAG_SETIMAGE_WORLDMAP:
			if (ModifyHold()) {
				SetImageWorldMap();
				this->pSelectedHold->Update();
				DrawScaledWorldMapImage();
				Paint();
			}
		break;

		case TAG_DELETE_WORLDMAP:
			DeleteWorldMap();
		break;

		case TAG_MANAGE:
			g_pTheDB->Commit();
			if (this->pSelectedHold)
				this->dwPrevHoldID = this->pSelectedHold->dwHoldID; //preserve ID for hold selection
			FreeMembers();
			GoToScreen(SCR_HoldSelect);
		break;

		case TAG_MANAGE_MODS:
			g_pTheDB->Commit();
			FreeMembers();
			GoToScreen(SCR_Mod);
		break;

		case TAG_WEATHER:
		{
			if (!this->pSelectedRoom) break;

			CWeatherDialogWidget *pWeatherDialog = DYN_CAST(CWeatherDialogWidget*,
					CWidget*, GetWidget(TAG_WEATHER_DIALOG));
			pWeatherDialog->SetRoom(this->pSelectedRoom);
			const UINT tag = pWeatherDialog->Display();
			if (tag == TAG_OK || tag == TAG_OK_WHOLELEVEL)
			{
				if (ModifyHold())
				{
					this->pSelectedRoom->Update();
					if (tag == TAG_OK_WHOLELEVEL)
					{
						//Synch all other rooms in room's level to the selected weather configuration.
						CDb db;
						db.Rooms.FilterBy(this->pSelectedRoom->dwLevelID);
						CIDSet roomIDs = db.Rooms.GetIDs();
						roomIDs -= this->pSelectedRoom->dwRoomID;
						for (CIDSet::const_iterator id=roomIDs.begin(); id!=roomIDs.end(); ++id)
						{
							CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(*id);
							pWeatherDialog->UpdateRoom(pRoom);
							pRoom->Update();
							delete pRoom;
						}
					}

					//Update the room widget with the new weather.
					this->pRoomWidget->LoadFromRoom(this->pSelectedRoom, this->pSelectedHold, &this->LevelEntrances);
				}
			}
			Paint();
		}
		break;

		default: return;	//don't update anything
	}

	SetWidgetStates();
}

//*****************************************************************************
void CEditSelectScreen::OnDoubleClick(const UINT dwTagNo)
{
	switch (dwTagNo)
	{
		case TAG_HOLD_LBOX:
			if (this->pHoldListBoxWidget->ClickedSelection() &&
					this->pHoldListBoxWidget->IsItemEnabled(this->pHoldListBoxWidget->GetSelectedItem()))
				RenameHold();
		break;

		case TAG_LEVEL_LBOX:
			if (this->pLevelListBoxWidget->ClickedSelection())
				RenameLevel();
		break;

		case TAG_WORLDMAP_LBOX:
			if (this->pWorldMapListBoxWidget->ClickedSelection())
				RenameWorldMap();
		break;

		case TAG_MAP:
			GoToRoomEditor();
		break;
	}
}

//*****************************************************************************
void CEditSelectScreen::OnKeyDown(
//Handles a keydown event.
//
//Params:
	const UINT dwTagNo, const SDL_KeyboardEvent &Key)
{
	CScreen::OnKeyDown(dwTagNo, Key);

	switch (Key.keysym.sym)
	{
		case SDLK_F1:
			CBrowserScreen::SetPageToLoad("editselect.html");
			GoToScreen(SCR_Browser);
		break;

		case SDLK_F2:
			//Output all scripts where each hold var is referenced.
			if (this->pSelectedHold)
			{
				g_pTheSound->PlaySoundEffect(SEID_MIMIC);
				SetCursor(CUR_Wait);
				g_pTheDB->Holds.LogScriptVarRefs(this->pSelectedHold->dwHoldID, (Key.keysym.mod & KMOD_ALT) != 0);
				SetCursor();
			}
		break;

		case SDLK_F3:
			//Hold stats.
			if (this->pSelectedHold)
			{
				SetCursor(CUR_Wait);
				RoomStats stats;
				this->pSelectedHold->getStats(stats);
				WSTRING wstr = getStatsText(stats);
				CClipboard::SetString(wstr);
				SetCursor();
				ShowOkMessage(wstr.c_str());
			}
		break;

		case SDLK_F4:
			//Level stats.
			if (this->pSelectedLevel)
			{
				RoomStats stats;
				this->pSelectedLevel->getStats(stats);
				WSTRING wstr = getStatsText(stats);
				CClipboard::SetString(wstr);
				ShowOkMessage(wstr.c_str());
			}
		break;

		case SDLK_F5:
			//User probably wants to playtest, so go to room editor for this.
			GoToRoomEditor();
		break;

		case SDLK_DELETE:
		{
			CWidget *pWidget = GetSelectedWidget();
			switch (pWidget->GetTagNo())
			{
			case TAG_MAP:
				{
					if (!this->pSelectedRoom) break;
					if (!ModifyHold()) break;
					//Not allowed to delete level entrance room.
					UINT dSX, dSY;
					this->pSelectedLevel->GetStartingRoomCoords(dSX, dSY);
					CDbRoom *pRoom = g_pTheDB->Rooms.GetByCoords(
							this->pSelectedLevel->dwLevelID, dSX, dSY);
					if (pRoom->dwRoomID == this->pSelectedRoom->dwRoomID)
						ShowOkMessage(MID_CantDeleteEntranceRoom);
					else
						if (ShowYesNoMessage(MID_DeleteRoomPrompt) == TAG_YES)
						{
							const UINT dwHoldID = this->pSelectedHold->dwHoldID;
							g_pTheDB->Rooms.Delete(this->pSelectedRoom->dwRoomID);

							//Reload hold and level.
							delete this->pSelectedHold;
							this->pSelectedHold = g_pTheDB->Holds.GetByID(dwHoldID);
							SelectLevel(this->pSelectedLevel->dwLevelID);
							Paint();
						}
					delete pRoom;
				}
				break;
			}
			SetWidgetStates();
		}
		break;

		//Cutting, copying and pasting selected level
		case SDLK_x:
		case SDLK_c:
			if (Key.keysym.mod & KMOD_CTRL)
			{
				CWidget *pWidget = GetSelectedWidget();
				ASSERT(pWidget);
				switch (pWidget->GetTagNo())
				{
					case TAG_LEVEL_LBOX:
					{
						if (!this->pSelectedLevel) break;
						//Get an instance of the level being copied.
						delete this->pLevelCopy;
						this->pLevelCopy = g_pTheDB->Levels.GetByID(
								this->pSelectedLevel->dwLevelID);

						g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR);
					}
					break;

					case TAG_MAP:
						this->pMapWidget->CopyRoom(Key.keysym.sym == SDLK_c); //Ctrl-C copies
					break;
				}
			}
			return;

		case SDLK_v:
		{
			if ((Key.keysym.mod & KMOD_CTRL) == 0) break;

			StopKeyRepeating();  //don't repeat this operation
			CWidget *pWidget = GetSelectedWidget();
			ASSERT(pWidget);
			switch (pWidget->GetTagNo())
			{
				case TAG_HOLD_LBOX: break;

				case TAG_LEVEL_LBOX:
				{
					//Paste a level.
					g_pTheSound->PlaySoundEffect(SEID_DOOROPEN);
					const bool bUpdate = PasteLevel();
					if (bUpdate)
						Paint();
				}
				break;

				case TAG_MAP:
				{
					//Paste a room.
					if (!ModifyHold()) break;
					if (this->pMapWidget->IsDeletingRoom())
					{
						//Not allowed to delete level entrance room.
						UINT dwSX, dwSY, dwMapX, dwMapY;
						ASSERT(this->pSelectedLevel);
						this->pSelectedLevel->GetStartingRoomCoords(dwSX, dwSY);
						this->pMapWidget->GetSelectedRoomXY(dwMapX, dwMapY);
						if (dwMapX == dwSX && dwMapY == dwSY)
						{
							ShowOkMessage(MID_CantDeleteEntranceRoom);
							break;
						}
						else
							if (ShowYesNoMessage(MID_DeleteRoomPrompt) != TAG_YES)
								break;
					}
					const bool bUpdate = this->pMapWidget->PasteRoom(this->pSelectedHold);
					if (bUpdate)
					{
						//Refresh level instance to resynch data.
						const UINT dwLevelID = this->pSelectedLevel->dwLevelID;
						delete this->pSelectedLevel;
						this->pSelectedLevel = g_pTheDB->Levels.GetByID(dwLevelID);

						//Update hold's and level's timestamp.
						//this->pSelectedHold->Update(); //hold was updated in PasteRoom().
						this->pSelectedLevel->Update();

						UINT dwRoomX, dwRoomY;
						this->pMapWidget->GetSelectedRoomXY(dwRoomX, dwRoomY);
						this->pMapWidget->LoadFromLevel(this->pSelectedLevel);
						SelectRoom(dwRoomX, dwRoomY);
						Paint();
					}
				}
				break;
			}
		}
		break;

		case SDLK_F7:
			ReflectLevelX();
		break;
		case SDLK_F8:
			ReflectLevelY();
		break;

		default:
			if (IsDeactivating())
				FreeMembers();
		break;
	}
}

//*****************************************************************************
void CEditSelectScreen::OnRearranged(const UINT dwTagNo)
//Called when the level list has been reordered.
{
	switch (dwTagNo)
	{
		case TAG_LEVEL_LBOX:
		{
			if (!ModifyHold())
			{
				PopulateLevelListBox(); //revert reordering of level list
				ASSERT(this->pSelectedLevel);
				this->pLevelListBoxWidget->SelectItem(this->pSelectedLevel->dwLevelID);
				this->pLevelListBoxWidget->RequestPaint();
				return;
			}

			g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);

			//Update order of levels in hold.
			for (UINT dwIndex=0; dwIndex<this->pLevelListBoxWidget->GetItemCount(); ++dwIndex)
			{
				const UINT dwLevelID = this->pLevelListBoxWidget->GetKeyAtLine(dwIndex);
				ASSERT(dwLevelID);
				if (dwLevelID != ADD_LEVEL_ID) {
					CDbLevel *pLevel = g_pTheDB->Levels.GetByID(dwLevelID);
					ASSERT(pLevel);
					pLevel->dwOrderIndex = dwIndex + 1;
					pLevel->Update();
					delete pLevel;
				}
			}
			//Reload selected level so local state is synched.
			const UINT dwLevelID = this->pSelectedLevel->dwLevelID;
			delete this->pSelectedLevel;
			this->pSelectedLevel = g_pTheDB->Levels.GetByID(dwLevelID);

			//Update hold entrance if first level was moved.
			const UINT dwFirstLevel = this->pLevelListBoxWidget->GetKeyAtLine(0);
			if (dwFirstLevel != this->pSelectedHold->dwLevelID)
				this->pSelectedHold->dwLevelID = dwFirstLevel;

			//Resort level entrances.
			ENTRANCE_VECTOR entrances;
			CEntranceSelectDialogWidget::SortEntrances(this->pSelectedHold, entrances);
			for (UINT wIndex=0; wIndex<entrances.size(); ++wIndex)
				this->pSelectedHold->Entrances[wIndex] = entrances[wIndex];
			this->pSelectedHold->Update();
		}
		break;

		case TAG_WORLDMAP_LBOX:
		{
			if (!ModifyHold())
			{
				PopulateWorldMapListBox(); //revert reordering
				this->pWorldMapListBoxWidget->SelectItem(this->selectedWorldMapID);
				this->pWorldMapListBoxWidget->RequestPaint();
				return;
			}

			g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);

			//Update order of world maps in hold.
			for (UINT dwIndex=0; dwIndex<this->pWorldMapListBoxWidget->GetItemCount(); ++dwIndex)
			{
				const UINT worldMapID = this->pWorldMapListBoxWidget->GetKeyAtLine(dwIndex);
				ASSERT(worldMapID);
				if (worldMapID != ADD_WORLDMAP_ID) {
					this->pSelectedHold->SetOrderIndexForWorldMap(worldMapID, dwIndex);
				}
			}

			this->pSelectedHold->Update();
		}
		break;

		default: break;
	}
}

//*****************************************************************************
void CEditSelectScreen::OnSelectChange(
//Handles a selection change.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	switch (dwTagNo)
	{
		case TAG_HOLD_LBOX:
		{
			const UINT newHoldID = this->pHoldListBoxWidget->GetSelectedItem();
			if (this->pHoldListBoxWidget->IsItemEnabled(newHoldID))
				SelectHold(newHoldID);
			else {
				//Don't have permissions to edit this hold -- reset widgets.
				ResetSelectedHold();
				SetWidgetStates();

				//Show the author + editing permissions on the restricted hold.
				CDbHold *pHold = g_pTheDB->Holds.GetByID(newHoldID, true);
				ASSERT(pHold);
				CLabelWidget *pLabel = DYN_CAST(CLabelWidget*, CWidget*,
						GetWidget(TAG_HOLD_AUTHOR_LABEL));
				pLabel->SetText(pHold->GetAuthorText());
				CListBoxWidget *pListBox = DYN_CAST(CListBoxWidget*, CWidget*,
						GetWidget(TAG_WHO_CAN_EDIT_LBOX));
				pListBox->SelectItem(pHold->editingPrivileges);
				delete pHold;
			}
			Paint();
		}
		break;

		case TAG_LEVEL_LBOX:
		{
			const UINT levelID = this->pLevelListBoxWidget->GetSelectedItem();
			if (levelID == ADD_LEVEL_ID) {
				//need to wait for keypress/click/etc to be released before popping up another dialog
				this->addLevelAfterEvents = true;
			} else {
				SelectLevel(levelID);
			}
			Paint();
		}
		break;

		case TAG_WORLDMAP_LBOX:
		{
			const UINT worldMapID = this->pWorldMapListBoxWidget->GetSelectedItem();
			if (worldMapID == ADD_WORLDMAP_ID) {
				//need to wait for keypress/click/etc to be released before popping up another dialog
				this->addWorldMapAfterEvents = true;
			} else {
				SelectWorldMap(worldMapID);
			}
			Paint();
		}
		break;

		case TAG_LEVELMENU:
			SetWidgetStates();
			Paint();
		break;

		case TAG_STYLE_LBOX:
			if (!ModifyHold())
			{
				//Revert to original style.
				if (this->pSelectedRoom)
					SelectListStyle(this->pSelectedRoom->style);
			} else {
				WSTRING newStyle = this->pStyleListBoxWidget->GetSelectedItemText();
#ifdef RUSSIAN_BUILD
				//Ugly hack -- before SetRoomStyle is called, hard-coded IDs 0-5 are
				//converted back to the English style names for internal use.
				const UINT styleID = this->pStyleListBoxWidget->GetSelectedItem();
				switch (styleID)
				{
					case 1: newStyle = L"Aboveground"; break;
					case 2: newStyle = L"City"; break;
					case 3: newStyle = L"Deep Spaces"; break;
					case 4: newStyle = L"Fortress"; break;
					case 5: newStyle = L"Foundation"; break;
					case 6: newStyle = L"Iceworks"; break;
				}
#endif
				SetRoomStyle(newStyle.c_str());
				if (this->pSelectedRoom)
				{
					this->pSelectedRoom->Update();
					this->pSelectedLevel->Update();
					this->pSelectedHold->Update();

					//When Shift key is held while selecting new style,
					//set all other rooms in this room's level to the style.
					if ((SDL_GetModState() & KMOD_CTRL) != 0)
					{
						g_pTheSound->PlaySoundEffect(SEID_POTION);
						CDb db;
						db.Rooms.FilterBy(this->pSelectedRoom->dwLevelID);
						CIDSet roomIDs = db.Rooms.GetIDs();
						roomIDs -= this->pSelectedRoom->dwRoomID;
						for (CIDSet::const_iterator id=roomIDs.begin(); id!=roomIDs.end(); ++id)
						{
							CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(*id);
							ASSERT(pRoom);
							pRoom->style = newStyle.c_str();
							pRoom->Update();
							delete pRoom;
						}
					}
				}
			}
			Paint();
		break;

		case TAG_WHO_CAN_EDIT_LBOX:
		{
			CListBoxWidget *pWhoCanEditListBox = DYN_CAST(CListBoxWidget *, CWidget *,
					GetWidget(TAG_WHO_CAN_EDIT_LBOX));

			//Only the hold author can change editing privileges.
			if (!this->pSelectedHold)
				break;
			if (this->pSelectedHold->dwPlayerID == g_pTheDB->GetPlayerID())
			{
				this->pSelectedHold->editingPrivileges = (CDbHold::EditAccess)
						pWhoCanEditListBox->GetSelectedItem();
				this->pSelectedHold->Update();
			} else {
				pWhoCanEditListBox->SelectItem(
						(UINT)this->pSelectedHold->editingPrivileges);
				pWhoCanEditListBox->Paint();
			}
		}
		break;

		case TAG_MAP:
		{
			UINT dwRoomX, dwRoomY;
			this->pMapWidget->RequestPaint();
			this->pMapWidget->GetSelectedRoomXY(dwRoomX, dwRoomY);
			if (this->pMapWidget->bVacantRoom)
			{
				if (ModifyHold() && ShowYesNoMessage(MID_AddRoomPrompt) == TAG_YES)
				{
					if (AddRoom(dwRoomX,dwRoomY))
					{
						//Update hold's timestamp.
						ASSERT(this->pSelectedHold);
						this->pSelectedHold->Update();
					}
				} else if (!this->pMapWidget->ReadyToPasteRoom()) {
					//Put the map selection back on the current room.
					if (this->pSelectedRoom)
						this->pMapWidget->SelectRoom(this->pSelectedRoom->dwRoomX,
								this->pSelectedRoom->dwRoomY);
				}
			}
			else
				SelectRoom(dwRoomX, dwRoomY);
			Paint();
		}
		break;

		case TAG_SHOWEXITLEVEL:
		{
			COptionButtonWidget *pButton = DYN_CAST(COptionButtonWidget*, CWidget*,
					GetWidget(TAG_SHOWEXITLEVEL));
			if (!ModifyHold())
			{
				pButton->SetChecked(this->pSelectedLevel->bIsRequired);
				pButton->Paint();
			} else {
				ASSERT(this->pSelectedLevel);
				this->pSelectedLevel->bIsRequired = pButton->IsChecked();
				this->pSelectedLevel->Update();
				this->pSelectedHold->Update();
			}
		}
		break;

		case TAG_ROOM_IS_REQUIRED:
		{
			COptionButtonWidget *pOpButton = DYN_CAST(COptionButtonWidget*,
					CWidget*, GetWidget(TAG_ROOM_IS_REQUIRED));
			if (!ModifyHold())
			{
				pOpButton->SetChecked(this->pSelectedRoom->bIsRequired);
				pOpButton->Paint();
			} else {
				this->pSelectedRoom->bIsRequired = pOpButton->IsChecked();
				this->pSelectedRoom->Update();
				this->pSelectedLevel->Update();
				this->pSelectedHold->Update();
				this->pMapWidget->DrawMapSurfaceFromRoom(this->pSelectedRoom);
				this->pMapWidget->RequestPaint();
			}
		}
		break;
		case TAG_ROOM_IS_SECRET:
		{
			COptionButtonWidget *pOpButton = DYN_CAST(COptionButtonWidget*,
					CWidget*, GetWidget(TAG_ROOM_IS_SECRET));
			if (!ModifyHold())
			{
				pOpButton->SetChecked(this->pSelectedRoom->bIsSecret);
				pOpButton->Paint();
			} else {
				this->pSelectedRoom->bIsSecret = pOpButton->IsChecked();
				//When marking a room secret, make it unrequired by default.
				if (this->pSelectedRoom->bIsSecret)
				{
					this->pSelectedRoom->bIsRequired = false;

					COptionButtonWidget *pOpButton = DYN_CAST(COptionButtonWidget*,
							CWidget*, GetWidget(TAG_ROOM_IS_REQUIRED));
					pOpButton->SetChecked(false);
					pOpButton->Paint();
				}
				this->pSelectedRoom->Update();
				this->pSelectedLevel->Update();
				this->pSelectedHold->Update();
				this->pMapWidget->DrawMapSurfaceFromRoom(this->pSelectedRoom);
				this->pMapWidget->RequestPaint();
			}
		}
		break;

		case TAG_WORLDMAPDISPLAYLIST:
		{
			CListBoxWidget* pWorldMapDisplayListBox = DYN_CAST(CListBoxWidget*, CWidget*,
				GetWidget(TAG_WORLDMAPDISPLAYLIST));
			if (ModifyHold()) {
				this->pSelectedHold->SetDisplayTypeForWorldMap(
					this->pWorldMapListBoxWidget->GetSelectedItem(),
					HoldWorldMap::DisplayType(pWorldMapDisplayListBox->GetSelectedItem()));
				this->pSelectedHold->Update();
			}
			else {
				pWorldMapDisplayListBox->SelectItem(
					this->pSelectedHold->GetWorldMapDisplayType(pWorldMapDisplayListBox->GetSelectedItem())
				);
				pWorldMapDisplayListBox->Paint();
			}
		}
		break;
	}
}

//*****************************************************************************
void CEditSelectScreen::SelectListStyle(const WSTRING& style)
//Select the style list box item with this text.
{
	for (UINT wIndex=this->pStyleListBoxWidget->GetItemCount(); wIndex--; )
		if (!style.compare(this->pStyleListBoxWidget->GetTextAtLine(wIndex)))
		{
			this->pStyleListBoxWidget->SelectLine(wIndex);
			break;
		}
}

//*****************************************************************************
bool CEditSelectScreen::SetWidgets()
//Set up widgets and data used by them when user enters edit screen.
//Should only be called by SetForActivate().
//
//Returns:
//True if successful, false if not.
{
	if (!this->pSelectedHold)
	{
		//Select entrance room for first level of current hold, if possible.
		//(Level selection box is populated here.)
		if (!SelectFirstHold())
		{
			FreeMembers(); //clear any partially added hold
			return false;  //no holds are accessible to player
								//and they don't want to create their own hold
		}
	} else {
		if (!this->pSelectedLevel)
		{
			Paint(); //display screen first
			if (AddLevel() == 0)
			{
				FreeMembers(); //clear any partially added hold
				return false;
			}
		}

		CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen *, CScreen *,
				g_pTheSM->GetScreen(SCR_EditRoom));
		ASSERT(pEditRoomScreen);
		const UINT dwRoomID = pEditRoomScreen->GetRoomID();

		//Ensure hold entrances are current by reloading.
		ASSERT(this->pSelectedHold);
		SelectHold(this->pSelectedHold->dwHoldID,
				dwRoomID != 0); //don't waste time loading level+room if roomID will be (re)loaded below

		//Ensure map, level and room are synched to those of edit room screen if
		//returning from there by reloading.
		if (dwRoomID)
		{
			const UINT dwLevelID = pEditRoomScreen->GetLevelID();
			ASSERT(dwLevelID);
			SelectLevel(dwLevelID, true);
			CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(dwRoomID);
			if (pRoom)
				SetSelectedRoom(pRoom);
			pEditRoomScreen->SetRoom(0); //reset, since this room can be invalidated now
		}

		SelectFirstWorldMap();
	}

	SetWidgetStates();

	return true;
}

//*****************************************************************************
void CEditSelectScreen::SetWidgetStates()
//Set button states depending on what data are available.
{
	CWidget *pWidget, *pButtons[4];
	COptionButtonWidget *pOpButton;
	UINT wIndex;

	const bool bHold = this->pSelectedHold != NULL;
	const bool bLevel = this->pSelectedLevel != NULL;
	const bool bRoom = this->pSelectedRoom != NULL;
	const bool bWorldMap = this->selectedWorldMapID != 0;

	//Hold
	static const UINT wHoldWidgets = 2;
	static const UINT holdWidgetTag[wHoldWidgets] = {
		TAG_COPY_HOLD, TAG_HOLD_SETTINGS
	};
	for (wIndex=0; wIndex<wHoldWidgets; ++wIndex)
	{
		VERIFY(pWidget = GetWidget(holdWidgetTag[wIndex]));
		pWidget->Enable(bHold);
	}
	pWidget = GetWidget(TAG_WHO_CAN_EDIT_LBOX);
	pWidget->Enable(bHold);

	//Level
	static const UINT wLevelWidgets = 4;
	static const UINT levelWidgetTag[wLevelWidgets] = {
		TAG_RENAME_LEVEL, TAG_REDESC_LEVEL, TAG_DELETE_LEVEL, TAG_SHOWEXITLEVEL
	};
	for (wIndex=0; wIndex<wLevelWidgets; ++wIndex)
	{
		VERIFY(pWidget = GetWidget(levelWidgetTag[wIndex]));
		pWidget->Enable(bLevel);
	}

	//World map.
	static const UINT wWorldMapWidgets = 4;
	static const UINT worldmapWidgetTag[wWorldMapWidgets] = {
		TAG_RENAME_WORLDMAP, TAG_SETIMAGE_WORLDMAP, TAG_DELETE_WORLDMAP,
		TAG_WORLDMAPDISPLAYLIST
	};
	for (wIndex=0; wIndex<wWorldMapWidgets; ++wIndex)
	{
		VERIFY(pWidget = GetWidget(worldmapWidgetTag[wIndex]));
		pWidget->Enable(bWorldMap);
	}

	//Room
	static const UINT wRoomWidgets = 4;
	static const UINT roomWidgetTag[wRoomWidgets] = {
		TAG_EDIT, TAG_ROOM_IS_REQUIRED, TAG_ROOM_IS_SECRET, TAG_WEATHER
	};
	for (wIndex=0; wIndex<wRoomWidgets; ++wIndex)
	{
		VERIFY(pButtons[wIndex] = GetWidget(roomWidgetTag[wIndex]));
		pButtons[wIndex]->Enable(bRoom);
	}
	pWidget = GetWidget(TAG_STYLE_LBOX);
	pWidget->Enable(bRoom);

	pOpButton = DYN_CAST(COptionButtonWidget*, CWidget*, pButtons[1]);
	pOpButton->SetChecked(this->pSelectedRoom && this->pSelectedRoom->bIsRequired);
	pOpButton = DYN_CAST(COptionButtonWidget*, CWidget*, pButtons[2]);
	pOpButton->SetChecked(this->pSelectedRoom && this->pSelectedRoom->bIsSecret);

	//Selective room/world map display.
	CTabbedMenuWidget *pTabbedMenu = DYN_CAST(CTabbedMenuWidget*, CWidget*, GetWidget(TAG_LEVELMENU));
	const bool bWorldMapTab = pTabbedMenu->GetSelectedTab() == WORLDMAP_TAB;
	this->pScaledWorldMapWidget->Show(bWorldMapTab);
	this->pScaledRoomWidget->Show(!bWorldMapTab);

	pWidget = GetWidget(TAG_WORLDMAPSETTINGSFRAME);
	pWidget->Show(bWorldMapTab);
	pWidget = GetWidget(TAG_LEVELSETTINGSFRAME);
	pWidget->Show(!bWorldMapTab);
	pWidget = GetWidget(TAG_POSITION_LABEL);
	pWidget->Show(!bWorldMapTab);
}

//*****************************************************************************
UINT CEditSelectScreen::AddHold()
//Inserts a new hold into the DB.
//
//Returns: new hold's ID#
{
	SetCursor();

	WSTRING wstrName;
	UINT dwTagNo = ShowTextInputMessage(MID_NameHold, wstrName);
	if (dwTagNo != TAG_OK)
		return 0L;

	//Provide default hold description.
	WSTRING wstrDescription = g_pTheDB->GetMessageText(MID_LevelBy);
	CDbPlayer *pHoldAuthor = g_pTheDB->GetCurrentPlayer();
	if (pHoldAuthor)
	{
		wstrDescription += wszSpace;
		wstrDescription += pHoldAuthor->NameText;
		delete pHoldAuthor;
	}

	//Get new hold.
	CDbHold *pHold = g_pTheDB->Holds.GetNew();

	//Set members that correspond to database fields.
	pHold->NameText = wstrName.c_str();
	pHold->DescriptionText = wstrDescription.c_str();
	pHold->dwPlayerID = g_pTheDB->GetPlayerID();

	//Save the new hold.
	if (!pHold->Update())
	{
		delete pHold;
		ShowOkMessage(MID_HoldNotSaved);
		return 0L;
	}

	//Add to hold list.
	const UINT dwHoldID = pHold->dwHoldID;
	this->pHoldListBoxWidget->AddItem(dwHoldID, pHold->NameText);
	this->pHoldListBoxWidget->SelectItem(dwHoldID);
	Paint();

	//Add first level to hold.
	delete this->pSelectedHold;
	this->pSelectedHold = pHold;
	AddLevel();

	delete pHold;
	this->pSelectedHold = NULL;

	SelectHold(dwHoldID);

	return dwHoldID;
}

//*****************************************************************************
bool CEditSelectScreen::SelectFirstHold()
//Populate hold list box with list of all holds.
//Select the current hold, if possible.  If its editing privileges
//are restricted, select the first accessible hold in the hold list box.
//If there aren't any accessible holds, have the player create one.
//
//Returns: true either if player can select a hold to edit,
//          or creates their own hold, else false.
{
	PopulateHoldListBox();

	//Select the most recently edited hold of those that are available.
	UINT dwSelectingHoldID = g_pTheDB->GetHoldID(); //selected one is default
	if (!this->pHoldListBoxWidget->IsItemEnabled(dwSelectingHoldID))
		dwSelectingHoldID = 0; //...unless this player doesn't have access to it
	time_t mostRecentTime = 0;
	UINT dwHoldID;
	for (UINT wIndex=this->pHoldListBoxWidget->GetItemCount(); wIndex--; )
	{
		dwHoldID = this->pHoldListBoxWidget->GetKeyAtLine(wIndex);
		if (!this->pHoldListBoxWidget->IsItemEnabled(dwHoldID)) continue;
		CDbHold *pHold = g_pTheDB->Holds.GetByID(dwHoldID, true);
		ASSERT(pHold);
		if ((time_t)pHold->LastUpdated > mostRecentTime)
		{
			mostRecentTime = (time_t)pHold->LastUpdated;
			dwSelectingHoldID = dwHoldID;
		}
		delete pHold;
	}

	if (dwSelectingHoldID)
		return SelectHold(dwSelectingHoldID);

	//Force player to create their own hold.
	//(Otherwise the screen is useless.)
	this->pLevelListBoxWidget->Clear();
	Paint(); //display screen first
	return AddHold() != 0;
}

//*****************************************************************************
bool CEditSelectScreen::SelectHold(
//Select hold by ID.
//Update level selection list.
//
//Returns: true if hold was selected, else false
//
//Params:
	const UINT dwHoldID,   //(in) Chosen hold ID.
	const bool bLoadHoldRecordOnly) //[default=false]
{
	if (!dwHoldID)
		return false;  //can happen if no holds in DB

	if (!this->pHoldListBoxWidget->IsItemEnabled(dwHoldID))
		return false;

	this->pHoldListBoxWidget->SelectItem(dwHoldID);

	if (!bLoadHoldRecordOnly)
		ResetSelectedHold();

	delete this->pSelectedHold;
	this->pSelectedHold = g_pTheDB->Holds.GetByID(dwHoldID);
	ASSERT(this->pSelectedHold);

	//Set hold author label.
	CLabelWidget *pLabel = DYN_CAST(CLabelWidget *, CWidget *,
			GetWidget(TAG_HOLD_AUTHOR_LABEL) );
	pLabel->SetText(this->pSelectedHold->GetAuthorText());

	//Set hold editing privileges.
	CListBoxWidget *pWhoCanEditListBox = DYN_CAST(CListBoxWidget *, CWidget *,
			GetWidget(TAG_WHO_CAN_EDIT_LBOX));
	pWhoCanEditListBox->SelectItem((UINT)this->pSelectedHold->editingPrivileges);

	PopulateLevelListBox();
	PopulateWorldMapListBox();

	if (!bLoadHoldRecordOnly)
		SelectFirstLevel();

	SelectFirstWorldMap();

	SetWidgetStates();

	return true;
}

//*****************************************************************************
void CEditSelectScreen::AddNewLevelLineToLevelList()
{
	this->pLevelListBoxWidget->AddItem(ADD_LEVEL_ID, g_pTheDB->GetMessageText(MID_NewLevel));
	this->pLevelListBoxWidget->SetRearrangeable(ADD_LEVEL_ID, false);
}

//*****************************************************************************
void CEditSelectScreen::AddNewMapLineToMapList()
{
	this->pWorldMapListBoxWidget->AddItem(ADD_WORLDMAP_ID, g_pTheDB->GetMessageText(MID_NewWorldMapPrompt));
	this->pWorldMapListBoxWidget->SetRearrangeable(ADD_WORLDMAP_ID, false);
}

//*****************************************************************************
UINT CEditSelectScreen::AddLevel()
//Inserts a new level into the current hold in the DB.
//
//Returns: new level's ID#
{
	if (!this->pSelectedHold)
		return 0L;

	if (!ModifyHold())
		return 0L;

	WSTRING wstrName;
	UINT dwTagNo = ShowTextInputMessage(MID_NameLevel, wstrName);
	if (dwTagNo != TAG_OK)
		return 0L;

	//Provide default level entrance description.
	WSTRING wstrDescription = g_pTheDB->GetMessageText(MID_Entrance);
	WCHAR temp[16];
	_itoW(this->pSelectedHold->Entrances.size() + 1, temp, 10);
	wstrDescription += wszSpace;
	wstrDescription += temp;

	//Get new level.
	CDbLevel *pLevel = g_pTheDB->Levels.GetNew();

	//Set members that correspond to database fields.
	//Note: pLevel->dwHoldID was already set to match its containing hold
	//in the call to CDbLevels::GetNew().
	pLevel->dwPlayerID = g_pTheDB->GetPlayerID();

	pLevel->NameText = wstrName.c_str();

	//Save the new level.
	pLevel->dwHoldID = this->pSelectedHold->dwHoldID;
	if (!pLevel->Update())
	{
		delete pLevel;
		ShowOkMessage(MID_LevelNotSaved);
		return 0L;
	}
	//Insert level into hold.
	this->pSelectedHold->InsertLevel(pLevel);

	//Add to level list box.
	const UINT dwLevelID = pLevel->dwLevelID;
	this->pLevelListBoxWidget->RemoveItem(ADD_LEVEL_ID);
	this->pLevelListBoxWidget->AddItem(dwLevelID, pLevel->NameText);
	AddNewLevelLineToLevelList(); //keep this line at the end of the list
	this->pLevelListBoxWidget->SelectItem(dwLevelID);

	//Add entrance room to level.
	delete this->pSelectedLevel;
	this->pSelectedLevel = pLevel;
	pLevel = NULL;
	SelectLevelEntranceRoom();

	//And new level entrance to hold's entrance list.
	CEntranceData *pEntrance = new CEntranceData(0, 0, this->pSelectedRoom->dwRoomID,
			CDrodBitmapManager::DISPLAY_COLS/2, CDrodBitmapManager::DISPLAY_ROWS/2,
			SE, true, CEntranceData::DD_Always, 0);
	pEntrance->DescriptionText = wstrDescription.c_str();
	this->pSelectedHold->AddEntrance(pEntrance);
	this->pSelectedHold->Update();

	SelectLevel(dwLevelID);

	return dwLevelID;
}

//*****************************************************************************
bool CEditSelectScreen::SelectFirstLevel()
//Select first level in hold, if any.
//
//Returns: whether there is a level in the hold to select
{
	ASSERT(this->pSelectedHold);
	delete this->pSelectedLevel;
	this->pSelectedLevel = NULL;
	UINT dwSelectLevelID = this->pSelectedHold->dwLevelID;

	if (!dwSelectLevelID)
		return false;

	//Select the most recently-edited level.
	time_t mostRecentTime = 0;
	UINT dwLevelID;
	for (UINT wIndex=this->pLevelListBoxWidget->GetItemCount(); wIndex--; )
	{
		dwLevelID = this->pLevelListBoxWidget->GetKeyAtLine(wIndex);
		if (dwLevelID != ADD_LEVEL_ID) {
			CDbLevel *pLevel = g_pTheDB->Levels.GetByID(dwLevelID, true);
			ASSERT(pLevel);
			if ((time_t)pLevel->LastUpdated > mostRecentTime)
			{
				mostRecentTime = (time_t)pLevel->LastUpdated;
				dwSelectLevelID = dwLevelID;
			}
			delete pLevel;
		}
	}

	return SelectLevel(dwSelectLevelID);
}

//*****************************************************************************
bool CEditSelectScreen::SelectLevel(
//Select level by ID.
//
//Params:
	const UINT dwLevelID,  //(in) Chosen level ID.
	const bool bLoadLevelRecordOnly) //[default=false]
{
	ASSERT(this->pSelectedHold);
	delete this->pSelectedLevel;
	ASSERT(dwLevelID);
	this->pSelectedLevel = g_pTheDB->Levels.GetByID(dwLevelID);
	ASSERT(this->pSelectedLevel);

	this->pLevelListBoxWidget->SelectItem(dwLevelID);

	//Update author and date labels.
	CLabelWidget *pLabel = DYN_CAST(CLabelWidget *, CWidget *,
			GetWidget(TAG_LEVEL_AUTHOR_LABEL) );
	pLabel->SetText(this->pSelectedLevel->GetAuthorText());
	pLabel = DYN_CAST(CLabelWidget *, CWidget *,
			GetWidget(TAG_LEVEL_DATE_LABEL) );
	WSTRING wstrCreated;
	this->pSelectedLevel->Created.GetLocalFormattedText(DF_LONG_DATE, wstrCreated);
	pLabel->SetText(wstrCreated.c_str());

	//Update exit level message flag.
	COptionButtonWidget *pButton = DYN_CAST(COptionButtonWidget*, CWidget*,
			GetWidget(TAG_SHOWEXITLEVEL));
	pButton->SetChecked(this->pSelectedLevel->bIsRequired);

	//Update map.
	UINT dwX, dwY;
	this->pSelectedLevel->GetStartingRoomCoords(dwX,dwY);
	this->pMapWidget->LoadFromLevel(this->pSelectedLevel);

	if (!bLoadLevelRecordOnly)
		SelectLevelEntranceRoom();

	return true;
}

//*****************************************************************************
UINT CEditSelectScreen::AddRoom(
//Inserts a new room into the current level at given position in the DB.
//
//Returns: new room's ID#, or 0 if failed
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY) //(in) Coords of chosen room.
{
	if (!this->pSelectedHold || !this->pSelectedLevel)
		return 0L;
	if (!ModifyHold())
		return 0L;
	const UINT dwRoomAtCoords = this->pSelectedLevel->GetRoomIDAtCoords(dwRoomX,dwRoomY);
	if (dwRoomAtCoords)
	{
		//Room already exists here.  Just return it.
		//Update widgets.
		this->pMapWidget->LoadFromLevel(this->pSelectedLevel);
		SelectRoom(dwRoomX,dwRoomY);
		return dwRoomAtCoords;  
	}

	//Get new room.
	CDbRoom *pRoom = g_pTheDB->Rooms.GetNew();

	//Set members that correspond to database fields.
	pRoom->dwLevelID = this->pSelectedLevel->dwLevelID;
	pRoom->dwRoomX = dwRoomX;
	pRoom->dwRoomY = dwRoomY;
	pRoom->wRoomCols = CDrodBitmapManager::DISPLAY_COLS;
	pRoom->wRoomRows = CDrodBitmapManager::DISPLAY_ROWS;
	pRoom->style = this->pStyleListBoxWidget->GetSelectedItemText();
	pRoom->bIsRequired = true;
	pRoom->bIsSecret = false;
	if (this->pSelectedRoom)
		pRoom->weather = this->pSelectedRoom->weather;   //maintain weather

	if (!pRoom->AllocTileLayers()) {
		delete pRoom;
		return 0;
	}

	//Make room empty.
	const UINT dwSquareCount = pRoom->CalcRoomArea();
	memset(pRoom->pszOSquares, T_FLOOR, dwSquareCount * sizeof(char));
	memset(pRoom->pszFSquares, T_EMPTY, dwSquareCount * sizeof(char));
	pRoom->ClearTLayer();

	pRoom->coveredOSquares.Init(pRoom->wRoomCols, pRoom->wRoomRows);
	pRoom->tileLights.Init(pRoom->wRoomCols, pRoom->wRoomRows);

	//Add proper edges to the room.
	CEditRoomScreen::FillInRoomEdges(pRoom);

	//Save the new room.
	if (!pRoom->Update())
	{
		ShowOkMessage(MID_RoomNotSaved);
		delete pRoom;
		return 0L;
	}

	const UINT dwRoomID = pRoom->dwRoomID;
	delete pRoom;

	//Update map.
	this->pMapWidget->LoadFromLevel(this->pSelectedLevel);
	SelectRoom(dwRoomX,dwRoomY);

	return dwRoomID;
}

//*****************************************************************************
UINT CEditSelectScreen::AddWorldMap()
//Inserts a new world into the current hold.
//
//Returns: new map's ID
{
	if (!this->pSelectedHold)
		return 0L;

	if (!ModifyHold())
		return 0L;

	WSTRING wstrName;
	UINT dwTagNo = ShowTextInputMessage(MID_NameWorldMap, wstrName);
	if (dwTagNo != TAG_OK)
		return 0L;

	const UINT newWorldMapID = this->pSelectedHold->AddWorldMap(wstrName.c_str());
	if (newWorldMapID) {
		this->pSelectedHold->Update();

		//Add to world map list box.
		this->pWorldMapListBoxWidget->RemoveItem(ADD_WORLDMAP_ID);
		this->pWorldMapListBoxWidget->AddItem(newWorldMapID, wstrName.c_str());
		AddNewMapLineToMapList(); //keep this line at the end of the list

		SelectWorldMap(newWorldMapID);

		SetWidgetStates();
	}

	return newWorldMapID;
}

//*****************************************************************************
bool CEditSelectScreen::SelectFirstWorldMap()
{
	return SelectWorldMap(this->pWorldMapListBoxWidget->GetKeyAtLine(0));
}

//*****************************************************************************
bool CEditSelectScreen::SelectWorldMap(const UINT dwWorldMapID)
{
	this->pWorldMapImage->SetImage((SDL_Surface*)NULL);

	ASSERT(this->pSelectedHold);
	if (!dwWorldMapID || dwWorldMapID == ADD_WORLDMAP_ID) {
		this->selectedWorldMapID = 0;
		this->pWorldMapListBoxWidget->DeselectAll();
		this->pWorldMapListBoxWidget->UnsetCursorLine();
		return false;
	}

	this->selectedWorldMapID = dwWorldMapID;

	this->pWorldMapListBoxWidget->SelectItem(dwWorldMapID);

	CListBoxWidget *pWorldMapDisplayListBox = DYN_CAST(CListBoxWidget*, CWidget*,
			GetWidget(TAG_WORLDMAPDISPLAYLIST));
	pWorldMapDisplayListBox->SelectItem(
			this->pSelectedHold->GetWorldMapDisplayType(dwWorldMapID));

	DrawScaledWorldMapImage();
	
	return true;
}

//*****************************************************************************
bool CEditSelectScreen::SetImageWorldMap()
{
	ASSERT(this->pSelectedHold);

	ASSERT(this->selectedWorldMapID);
	ASSERT(this->selectedWorldMapID != ADD_WORLDMAP_ID);

SelectImage:
	UINT dwDataID;
	CEntranceSelectDialogWidget::BUTTONTYPE eButton;
	do {
		dwDataID = this->pSelectedHold->GetWorldMapDataID(this->selectedWorldMapID);
		eButton = SelectListID(
				this->pEntranceBox, this->pSelectedHold, dwDataID,
				MID_ImageSelectPrompt, CEntranceSelectDialogWidget::Images);
		if (eButton != CEntranceSelectDialogWidget::OK &&
				eButton != CEntranceSelectDialogWidget::Delete)
			return false;

		if (eButton == CEntranceSelectDialogWidget::Delete)
		{
			//Remove this image from the database and make another selection.
			if (dwDataID) {
				g_pTheDB->Data.Delete(dwDataID);
				this->pSelectedHold->SetDataIDForWorldMap(this->selectedWorldMapID, 0);
			}
		}
	} while (eButton != CEntranceSelectDialogWidget::OK);

	if (dwDataID) {
		this->pSelectedHold->SetDataIDForWorldMap(this->selectedWorldMapID, dwDataID);
	} else {
		//Load new image from disk.
		const UINT dwID = ImportHoldImage(this->pSelectedHold->dwHoldID, EXT_JPEG | EXT_PNG);
		if (dwID)
			this->pSelectedHold->SetDataIDForWorldMap(this->selectedWorldMapID, dwID);
		goto SelectImage;	//return to image select menu
	}

	return true;
}

//*****************************************************************************
void CEditSelectScreen::DrawScaledWorldMapImage()
{
	this->pWorldMapImage->SetImage((SDL_Surface*)NULL);

	const UINT dataID = this->pSelectedHold->GetWorldMapDataID(this->selectedWorldMapID);
	if (dataID) {
		SDL_Surface *pImage = g_pTheDBM->LoadImageSurface(dataID);
		if (pImage) {
			this->pWorldMapImage->SetImage(pImage);
		}
	}

	this->pScaledWorldMapWidget->RefreshSize();

	if (!g_pTheSM->bTransitioning)
		this->pScaledWorldMapWidget->Paint();
}

//*****************************************************************************
void CEditSelectScreen::CopyHold()
//Make duplicate copy of selected hold.
{
	if (!this->pSelectedHold)
		return;
	if (ShowYesNoMessage(MID_CopyHoldPrompt) != TAG_YES)
		return;

	g_pTheSound->PlaySoundEffect(SEID_MIMIC);
	SetCursor(CUR_Wait);

	CDbHold *pNewHold = this->pSelectedHold->MakeCopy();
	const UINT dwCurrentPlayerID = g_pTheDB->GetPlayerID();
	ASSERT(dwCurrentPlayerID);
	if (this->pSelectedHold->dwPlayerID != dwCurrentPlayerID)
		pNewHold->ChangeAuthor(dwCurrentPlayerID);
	else
	{
		//Alter hold name to indicate this one's a copy.
		WSTRING holdName = (WSTRING)pNewHold->NameText;
		holdName += wszSpace;
		holdName += wszLeftParen;
		holdName += g_pTheDB->GetMessageText(MID_Copy);
		holdName += wszRightParen;
		pNewHold->NameText = holdName.c_str();
		pNewHold->Update();
	}

	PopulateHoldListBox();
	SelectHold(pNewHold->dwHoldID);
	delete pNewHold;
	Paint();
	SetCursor();
}

//*****************************************************************************
void CEditSelectScreen::DeleteLevel()
//Deletes the currently selected level from its hold, upon user confirmation.
//User specifies where to reroute all stairs that previously went to this level.
{
	if (!this->pSelectedLevel)
		return;
	ASSERT(this->pSelectedHold);
	if (!ModifyHold())
		return;
	WSTRING prompt = (const WCHAR*)this->pSelectedLevel->NameText;
	prompt += wszColon;
	prompt += wszCRLF;
	prompt += g_pTheDB->GetMessageText(MID_DeleteLevelPrompt);
	if (ShowYesNoMessage(prompt.c_str()) != TAG_YES)
		return;

	SetCursor(CUR_Wait);
	g_pTheDB->Levels.Delete(this->pSelectedLevel->dwLevelID);
	const UINT dwHoldID = this->pSelectedHold->dwHoldID;
	//Re-synch hold object.
	delete this->pSelectedHold;
	this->pSelectedHold = g_pTheDB->Holds.GetByID(dwHoldID);
	SetCursor();

	//Reset level exits that were pointing to entrances in the deleted level.
	this->pSelectedHold->RemoveLevel(this->pSelectedLevel->dwLevelID, 0);
	this->pSelectedHold->Update();

	SelectHold(this->pSelectedHold->dwHoldID);
	Paint();
}

//*****************************************************************************
void CEditSelectScreen::DeleteWorldMap()
//Deletes the currently selected world map from its hold, upon user confirmation.
{
	if (!this->selectedWorldMapID)
		return;
	ASSERT(this->pSelectedHold);
	if (!ModifyHold())
		return;

	WSTRING prompt = this->pWorldMapListBoxWidget->GetSelectedItemText();
	prompt += wszColon;
	prompt += wszCRLF;
	prompt += g_pTheDB->GetMessageText(MID_DeleteWorldMapPrompt);
	if (ShowYesNoMessage(prompt.c_str()) != TAG_YES)
		return;

	this->pSelectedHold->DeleteWorldMap(this->selectedWorldMapID);
	this->pSelectedHold->Update();

	this->pWorldMapListBoxWidget->RemoveItem(this->selectedWorldMapID);

	SelectFirstWorldMap();

	SetWidgetStates();

	Paint();
}

//*****************************************************************************
void CEditSelectScreen::EditHoldSettings()
//Pop-up dialog box to edit hold settings.
{
	if (!this->pSelectedHold)
		return;

	CDialogWidget *pDialog = DYN_CAST(CDialogWidget*, CWidget*,
			GetWidget(TAG_HOLD_SETTINGS_DIALOG));

	UINT dwTag;
	do {
		dwTag = pDialog->Display();
		switch (dwTag)
		{
			case TAG_RENAME_HOLD:
				RenameHold();
			break;
			case TAG_REDESC_HOLD:
			{
				WSTRING wstr = (const WCHAR*) this->pSelectedHold->DescriptionText;
				const UINT dwAnswerTagNo = ShowTextInputMessage(MID_DescribeHold, wstr, true);
				if (dwAnswerTagNo == TAG_OK)
				{
					if (!ModifyHold())
						break;
					this->pSelectedHold->DescriptionText = wstr.c_str();
					this->pSelectedHold->Update();
				}
			}
			break;
			case TAG_ENDING_MESSAGE:
			{
				WSTRING wstr = (const WCHAR*) this->pSelectedHold->EndHoldText;
				const UINT dwAnswerTagNo = ShowTextInputMessage(MID_EndHoldPrompt, wstr, true);
				if (dwAnswerTagNo == TAG_OK)
				{
					if (!ModifyHold())
						break;
					this->pSelectedHold->EndHoldText = wstr.c_str();
					this->pSelectedHold->Update();
				}
			}
			break;
			default: break;
		}
	} while (dwTag != TAG_HOLD_SETTINGS_OK && dwTag != TAG_ESCAPE && dwTag != TAG_QUIT);

	Paint();
}

//*****************************************************************************
bool CEditSelectScreen::ModifyHold()
//If hold being modified is owned by the current player, modifications are
//allowed.  Otherwise, the user is prompted to make a modified copy of the hold.
//If they agree, then the copy is created and selected, and the current player
//becomes the author (allowing future modifications to be made).
//
//Returns: whether modifying current hold is allowed
{
	if (!this->pSelectedHold)
		return false;
	const UINT dwCurrentPlayerID = g_pTheDB->GetPlayerID();
	ASSERT(dwCurrentPlayerID);
	if (this->pSelectedHold->dwPlayerID == dwCurrentPlayerID)
		return true;   //the hold author is making changes -- always allowed

	if (ShowYesNoMessage(MID_CopyHoldPrompt) != TAG_YES)
		return false;

	SetCursor(CUR_Wait);

	CDbHold *pNewHold = this->pSelectedHold->MakeCopy();
	pNewHold->ChangeAuthor(dwCurrentPlayerID);
	PopulateHoldListBox();
	SelectHold(pNewHold->dwHoldID);
	delete pNewHold;

	Paint();
	SetCursor();

	return false;
}

//*****************************************************************************
void CEditSelectScreen::SelectLevelEntranceRoom()
//Selects the entrance room for the level.
//If there is no entrance room, add one.
{
	if (!this->pSelectedLevel)
		return;
	ASSERT(this->pSelectedHold);

	UINT dSX, dSY;
	this->pSelectedLevel->GetStartingRoomCoords(dSX, dSY);

	const UINT dwRoomID = g_pTheDB->Rooms.FindIDAtCoords(this->pSelectedLevel->dwLevelID, dSX, dSY);
	if (dwRoomID)
		SelectRoom(dSX, dSY);
	else
	{
		//Add an entrance room to this level.
		const UINT dwLevelID = this->pSelectedLevel->dwLevelID;
		const UINT dwRoomX = 50, dwRoomY = dwLevelID * 100 + 50;
		const UINT dwEntranceRoomID = AddRoom(dwRoomX, dwRoomY);
		if (dwEntranceRoomID)
		{
			this->pSelectedLevel->SetStartingRoomID(dwEntranceRoomID);
		} else {
			//Room couldn't be created (probably corrupted DB).
			ASSERT(!"Failed to create room.");
			this->pRoomWidget->ResetRoom();
		}
	}
}

//*****************************************************************************
void CEditSelectScreen::SelectRoom(
//Selects room at coords and updates display.
//
//Params:
	const UINT dwRoomX, const UINT dwRoomY) //(in) Coords of chosen room.
{
	ASSERT(this->pSelectedLevel);

	//Reset room pointer so room widget isn't left with dangling pointer to old room.
	this->pRoomWidget->ResetRoom();

	//Find the room.
	CDbRoom *pRoom = g_pTheDB->Rooms.GetByCoords(
			this->pSelectedLevel->dwLevelID, dwRoomX, dwRoomY);
	if (pRoom)
		SetSelectedRoom(pRoom);

	SetWidgetStates();
}

//*****************************************************************************
void CEditSelectScreen::SetSelectedRoom(
//Selects room and updates display.
//
//Params:
	CDbRoom *pRoom)   //(in)
{
	ASSERT(pRoom);
	delete this->pSelectedRoom;
	this->pSelectedRoom = pRoom;

	ASSERT(this->pSelectedHold);
	pRoom->PlaceCharacters(this->pSelectedHold);

	//Update map widget.
	this->pMapWidget->SelectRoom(pRoom->dwRoomX, pRoom->dwRoomY);

	//Update the room widget with new room.
	GetLevelEntrancesInRoom();
	this->pRoomWidget->LoadFromRoom(pRoom, this->pSelectedHold, &this->LevelEntrances);
	this->pRoomWidget->Paint();

	//Select room style from the list box.
	SelectListStyle(this->pSelectedRoom->style);
	CRoomScreen::SetMusicStyle(this->pSelectedRoom->style, SONG_EDITOR);

	//Update room label.
	WSTRING wstrDesc;
	pRoom->GetLevelPositionDescription(wstrDesc);
	CLabelWidget *pRoomLabel = DYN_CAST(CLabelWidget*, CWidget*,
			GetWidget(TAG_POSITION_LABEL));
	pRoomLabel->SetText(wstrDesc.c_str());
}

//*****************************************************************************
void CEditSelectScreen::SetRoomStyle(
//Sets the style for the selected room and updates display and music.
//
//Params:
	const WCHAR* pwStyle) //(in) Chosen style name
{
	if (!this->pSelectedRoom) return;

	this->pSelectedRoom->style = pwStyle;
	//Update the room widget with new style.
	this->pRoomWidget->LoadFromRoom(this->pSelectedRoom, this->pSelectedHold, &this->LevelEntrances);
	this->pRoomWidget->Paint();

	CRoomScreen::SetMusicStyle(this->pSelectedRoom->style, SONG_EDITOR);
}

//*****************************************************************************
void CEditSelectScreen::GetLevelEntrancesInRoom()
//Compile list of all level entrances in current room
//(NOTE: Copied from CEditRoomScreen.)
{
	//Clear entrance list.
	UINT wIndex;
	for (wIndex=this->LevelEntrances.size(); wIndex--; )
		delete this->LevelEntrances[wIndex];
	this->LevelEntrances.clear();

	if (!this->pSelectedHold || !this->pSelectedRoom) return;

	for (wIndex=0; wIndex<this->pSelectedHold->Entrances.size(); ++wIndex)
	{
		CEntranceData *pEntrance = this->pSelectedHold->Entrances[wIndex];
		if (pEntrance->dwRoomID == this->pSelectedRoom->dwRoomID)
		{
			this->LevelEntrances.push_back(new CMoveCoord(pEntrance->wX,
					pEntrance->wY, pEntrance->wO));
		}
	}
}

//*****************************************************************************
void CEditSelectScreen::GoToRoomEditor()
{
	if (!this->pSelectedRoom) return;

	LOGCONTEXT("CEditSelectScreen::GoToRoomEditor");
	SetCursor(CUR_Wait);
	g_pTheDB->Commit();

	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen *, CScreen *,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen);
	if (pEditRoomScreen->SetRoom(this->pSelectedRoom->dwRoomID,
			true)) //don't load room data fully, since it will be reloaded on activation of roomEditScreen
	{
		delete this->pSelectedRoom;
		this->pSelectedRoom = NULL;   //will be set upon return from EditRoomScreen.
		GoToScreen(SCR_EditRoom);
	}
	SetCursor();
}

//*****************************************************************************
void CEditSelectScreen::Paint(
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

//*****************************************************************************
void CEditSelectScreen::PopulateHoldListBox()
//Puts all holds player has completed or authored into list box.
{
	BEGIN_DBREFCOUNT_CHECK;
	this->pHoldListBoxWidget->Clear();
	const UINT playerID = g_pTheDB->GetPlayerID();

	//Get holds in DB.
	CDbHold *pHold = g_pTheDB->Holds.GetFirst(true);
	while (pHold)
	{
#ifndef ENABLE_CHEATS
		if (pHold->status != CDbHold::Tutorial) //never show this type
#endif
		{
			this->pHoldListBoxWidget->AddItem(pHold->dwHoldID, pHold->NameText,
#ifdef ENABLE_CHEATS
				false
#else
				false//!g_pTheDB->Holds.PlayerCanEditHold(pHold->dwHoldID)
#endif
			);

			//Tint holds authored by other players.
			if (playerID != pHold->dwPlayerID)
				this->pHoldListBoxWidget->SetItemColor(pHold->dwHoldID, DarkRed);
		}
		delete pHold;
		pHold = g_pTheDB->Holds.GetNext();
	}
	END_DBREFCOUNT_CHECK;
}

//*****************************************************************************
void CEditSelectScreen::PopulateLevelListBox()
//Puts levels of current hold into list box.  Sort by level index.
{
	BEGIN_DBREFCOUNT_CHECK;
	{
	this->pLevelListBoxWidget->Clear();

	if (!this->pSelectedHold)
		return;

	//Get levels.  Sort by local index in hold.
	SORTED_LEVELS levels;
	CIDSet levelsInHold = CDb::getLevelsInHold(this->pSelectedHold->dwHoldID);
	for (CIDSet::const_iterator levelID = levelsInHold.begin(); levelID != levelsInHold.end(); ++levelID)
	{
		CDbLevel *pLevel = g_pTheDB->Levels.GetByID(*levelID);
		ASSERT(pLevel);
		levels.insert(pLevel);
	}

	for (SORTED_LEVELS::const_iterator level = levels.begin(); level != levels.end(); ++level)
	{
		this->pLevelListBoxWidget->AddItem((*level)->dwLevelID, (const WCHAR*)((*level)->NameText));
		delete *level;
	}

	AddNewLevelLineToLevelList();
	}
	END_DBREFCOUNT_CHECK;
}

//*****************************************************************************
void CEditSelectScreen::PopulateStyleListBox()
//Puts available styles into list box.
{
	this->pStyleListBoxWidget->Clear();

	CDb db;
	set<WSTRING> loadedStyles = db.Data.GetModNames();

	{
		CFiles f;
		list<WSTRING> ini_styles;
		if (f.GetGameProfileString(INISection::Graphics, INIKey::Style, ini_styles)) {
			for (list<WSTRING>::const_iterator ini_style = ini_styles.begin(); ini_style != ini_styles.end(); ++ini_style)
				loadedStyles.insert(*ini_style);
		}
	}

	UINT wCount = 0;
	for (set<WSTRING>::const_iterator style=loadedStyles.begin(); style!=loadedStyles.end(); ++style)
	{
		const WCHAR *pName = style->c_str();
#ifdef RUSSIAN_BUILD
		//Ugly hack -- Translate texts not found in the MIDs:
		//Pre-defined styles are hard-coded to IDs 1-6.
		//Before SetRoomStyle is called, these IDs are converted back to the English style names internally.
		switch (wCount+1)
		{
			case 1: pName = wstrAboveground; break;
			case 2: pName = wstrCity; break;
			case 3: pName = wstrDeepSpaces; break;
			case 4: pName = wstrFortress; break;
			case 5: pName = wstrFoundation; break;
			case 6: pName = wstrIceworks; break;
			//!!TODO: update for 4.0+
		}
#endif
		this->pStyleListBoxWidget->AddItem(++wCount, pName);
	}

	this->pStyleListBoxWidget->SelectLine(0);
}

//*****************************************************************************
void CEditSelectScreen::PopulateWorldMapListBox()
//Puts world maps of current hold into list box.
{
	BEGIN_DBREFCOUNT_CHECK;
	{
	this->pWorldMapListBoxWidget->Clear();

	if (!this->pSelectedHold)
		return;

	//Get maps.  Sort by order index in hold.
	SORTED_WORLD_MAPS maps;
	for (vector<HoldWorldMap>::const_iterator map=this->pSelectedHold->worldMaps.begin();
		map != this->pSelectedHold->worldMaps.end(); ++map)
	{
		maps.insert(&(*map));
	}

	for (SORTED_WORLD_MAPS::const_iterator sorted=maps.begin();
			sorted != maps.end(); ++sorted)
	{
		const HoldWorldMap& map = *(*sorted);
		this->pWorldMapListBoxWidget->AddItem(map.worldMapID, map.nameText.c_str());
	}

	this->selectedWorldMapID = this->pWorldMapListBoxWidget->GetKeyAtLine(0);

	AddNewMapLineToMapList();
	}
	END_DBREFCOUNT_CHECK;
}

//*****************************************************************************
bool CEditSelectScreen::PasteLevel()
//Pastes this->pLevelCopy to the selected spot in the current hold's level list.
//If no hold is selected, do nothing.
//If entrance level was cut-and-pasted, update hold's first level ID.
//If level is being placed as the entrance level, update new hold's first level ID.
//
//Returns: whether a level was pasted
{
	if (!this->pLevelCopy || !this->pSelectedHold) return false;
	if (!ModifyHold()) return false;

	ShowCursor();
	SetCursor(CUR_Wait);

	CImportInfo info;
	CDbData::SkipIDsFromHoldForCopy(info, this->pSelectedHold->dwHoldID);
	CDbLevel *pNewLevel = this->pLevelCopy->CopyIntoHold(this->pSelectedHold, info);	//performs Update on hold and returned level
	if (!pNewLevel)
		return false;

	//This is not needed in the new hold, it'll be reassigned anyway
	pNewLevel->dwLevelIndex = 0;
	pNewLevel->dwOrderIndex = 0;

	const UINT newLevelID = pNewLevel->dwLevelID;
	ASSERT(newLevelID != 0);

	const CDbHold *pSrcHold = this->pLevelCopy->GetHold();
	pNewLevel->RekeyExitIDs(pSrcHold, this->pSelectedHold, info);
	delete pSrcHold;

	this->pSelectedHold->InsertLevel(pNewLevel);	//performs Update on hold and level
	delete pNewLevel;

	//Must reload the hold, level and its rooms (synchronize the data).
	SelectHold(this->pSelectedHold->dwHoldID, true);
	this->pLevelListBoxWidget->SelectItem(newLevelID);
	SelectLevel(newLevelID);

	SetCursor();

	return true;
}

//*****************************************************************************
void CEditSelectScreen::ReflectLevelX()
//Reflects all rooms and room positions in level horizontally.
{
	if (!this->pSelectedHold || !this->pSelectedLevel) return;
	if (!ModifyHold()) return;

	SetCursor(CUR_Wait);

	CIDSet roomIDs = CDb::getRoomsInLevel(this->pSelectedLevel->dwLevelID);
	for (CIDSet::const_iterator room=roomIDs.begin(); room!=roomIDs.end(); ++room)
	{
		CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(*room);
		ASSERT(pRoom);
		pRoom->ReflectX();
		for (UINT wIndex=this->pSelectedHold->Entrances.size(); wIndex--; )
		{
			CEntranceData *pEntrance = this->pSelectedHold->Entrances[wIndex];
			if (pEntrance->dwRoomID == pRoom->dwRoomID)
				pEntrance->ReflectX(pRoom->wRoomCols);
		}
		pRoom->dwRoomX = 100 - pRoom->dwRoomX;
		pRoom->Update();
		delete pRoom;
	}
	this->pSelectedHold->Update();

	SelectLevel(this->pSelectedLevel->dwLevelID);
	Paint();
	SetCursor();
}

//*****************************************************************************
void CEditSelectScreen::ReflectLevelY()
//Reflects all rooms and room positions in level vertically.
{
	if (!this->pSelectedHold || !this->pSelectedLevel) return;
	if (!ModifyHold()) return;

	SetCursor(CUR_Wait);

	CIDSet roomIDs = CDb::getRoomsInLevel(this->pSelectedLevel->dwLevelID);
	for (CIDSet::const_iterator room=roomIDs.begin(); room!=roomIDs.end(); ++room)
	{
		CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(*room);
		ASSERT(pRoom);
		pRoom->ReflectY();
		for (UINT wIndex=this->pSelectedHold->Entrances.size(); wIndex--; )
		{
			CEntranceData *pEntrance = this->pSelectedHold->Entrances[wIndex];
			if (pEntrance->dwRoomID == pRoom->dwRoomID)
				pEntrance->ReflectY(pRoom->wRoomRows);
		}
		const UINT wRoomRefY = 100 - (pRoom->dwRoomY % 100);
		pRoom->dwRoomY = (pRoom->dwRoomY/100)*100 + wRoomRefY;
		pRoom->Update();
		delete pRoom;
	}
	this->pSelectedHold->Update();

	SelectLevel(this->pSelectedLevel->dwLevelID);
	Paint();
	SetCursor();
}

//*****************************************************************************
void CEditSelectScreen::RenameHold()
//User renames currently selected hold.
{
	if (!this->pSelectedHold) return;
	WSTRING wstr = static_cast<const WCHAR *>(this->pSelectedHold->NameText);
	const UINT dwAnswerTagNo = ShowTextInputMessage(MID_NameHold, wstr);
	if (dwAnswerTagNo == TAG_OK)
	{
		if (!ModifyHold())
			return;

		this->pSelectedHold->NameText = wstr.c_str();
		this->pSelectedHold->Update();
		this->pHoldListBoxWidget->SetSelectedItemText(wstr.c_str());
		this->pHoldListBoxWidget->Paint();
	}
}

//*****************************************************************************
void CEditSelectScreen::RenameLevel()
//User renames currently selected level.
{
	if (!this->pSelectedLevel) return;
	ASSERT(this->pSelectedHold);

	if (!ModifyHold())
		return;

	WSTRING wstr = static_cast<const WCHAR *>(this->pSelectedLevel->NameText);
	const UINT dwAnswerTagNo = ShowTextInputMessage(MID_NameLevel, wstr);
	if (dwAnswerTagNo == TAG_OK)
	{
		this->pSelectedLevel->NameText = wstr.c_str();
		this->pSelectedLevel->Update();
		this->pLevelListBoxWidget->SetSelectedItemText(wstr.c_str());
		this->pLevelListBoxWidget->Paint();
	}
}

//*****************************************************************************
void CEditSelectScreen::RenameWorldMap()
//User renames currently selected world map.
{
	if (!this->selectedWorldMapID)
		return;

	ASSERT(this->pSelectedHold);
	if (!ModifyHold())
		return;

	WSTRING wstr = this->pWorldMapListBoxWidget->GetSelectedItemText();
	const UINT dwAnswerTagNo = ShowTextInputMessage(MID_NameWorldMap, wstr);
	if (dwAnswerTagNo == TAG_OK)
	{
		this->pSelectedHold->RenameWorldMap(this->selectedWorldMapID, wstr);
		this->pSelectedHold->Update();
		this->pWorldMapListBoxWidget->SetSelectedItemText(wstr.c_str());
		this->pWorldMapListBoxWidget->Paint();
	}
}

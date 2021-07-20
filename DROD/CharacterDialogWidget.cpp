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

#include "CharacterDialogWidget.h"
#include "CommandListBoxWidget.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"
#include "EditRoomScreen.h"
#include "FaceWidget.h"

#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/ImageWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/TextBoxWidget.h>
#include <FrontEndLib/TextBox2DWidget.h>
#include <FrontEndLib/OptionButtonWidget.h>

#include "../DRODLib/Db.h"
#include "../DRODLib/MonsterFactory.h"
#include "../DRODLib/SettingsKeys.h"
#include "../DRODLib/CurrentGameRecords.h"

#include "../Texts/MIDs.h"
#include <BackEndLib/Files.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Clipboard.h>

#include <wctype.h>
#include <map>

//NOTE: tag #'s should not conflict with other widgets on screen
const UINT TAG_CANCEL = 999;
const UINT TAG_GRAPHICLISTBOX = 998;
const UINT TAG_COMMANDSLISTBOX = 997;
const UINT TAG_ADDCOMMAND = 996;
const UINT TAG_DELETECOMMAND = 995;
const UINT TAG_ISVISIBLE = 994;
const UINT TAG_CHARACTERS = 993;

const UINT TAG_ACTIONLISTBOX = 989;
const UINT TAG_EVENTLISTBOX = 988;
const UINT TAG_DELAY = 987;
const UINT TAG_WAIT = 986;
const UINT TAG_SPEAKERLISTBOX = 985;
const UINT TAG_MOODLISTBOX = 984;
const UINT TAG_SPEECHTEXT = 983;
const UINT TAG_ADDSOUND = 982;
const UINT TAG_SOUNDNAME_LABEL = 981;
const UINT TAG_DIRECTIONLISTBOX = 980;
const UINT TAG_ONOFFLISTBOX = 979;
const UINT TAG_ONOFFLISTBOX2 = 978;
const UINT TAG_OPENCLOSELISTBOX = 977;
const UINT TAG_GOTOLABELTEXT = 976;
const UINT TAG_GOTOLABELLISTBOX = 975;
const UINT TAG_MUSICLISTBOX = 974;
const UINT TAG_WAITFLAGSLISTBOX = 973;
const UINT TAG_MOVERELX = 972;
const UINT TAG_MOVERELY = 971;
const UINT TAG_IMPERATIVELISTBOX = 970;

const UINT TAG_EVENTLABEL = 969;
const UINT TAG_WAITLABEL = 968;
const UINT TAG_DELAYLABEL = 967;
const UINT TAG_SPEAKERLABEL = 966;
const UINT TAG_MOODLABEL = 965;
const UINT TAG_TEXTLABEL = 964;
const UINT TAG_DIRECTIONLABEL = 963;
const UINT TAG_GOTOLABEL = 962;
const UINT TAG_DISPLAYSPEECHLABEL = 961;
const UINT TAG_MUSICLABEL = 960;
const UINT TAG_NOTURNING = 959;
const UINT TAG_SINGLESTEP = 958;
const UINT TAG_SINGLESTEP2 = 888;
const UINT TAG_CUTSCENELABEL = 957;
const UINT TAG_MOVERELXLABEL = 956;
const UINT TAG_MOVERELYLABEL = 955;
const UINT TAG_LOOPSOUND = 954;
const UINT TAG_WAITABSLABEL = 953;
const UINT TAG_SKIPENTRANCELABEL = 952;
const UINT TAG_TESTSOUND = 951;

const UINT TAG_VARADD = 949;
const UINT TAG_VARREMOVE = 948;
const UINT TAG_VARLIST = 947;
const UINT TAG_VAROPLIST = 946;
const UINT TAG_VARCOMPLIST = 945;
const UINT TAG_VARNAMETEXTINPUT = 889;
const UINT TAG_VARNAMETEXTLABEL = 944;
const UINT TAG_VARVALUELABEL = 943;
const UINT TAG_VARVALUE = 942;

const UINT TAG_GRAPHICLISTBOX2 = 939;
const UINT TAG_GRAPHICLISTBOX3 = 887;
const UINT TAG_BUILDITEMLISTBOX = 938;
const UINT TAG_BUILDMARKERITEMLISTBOX = 885;
const UINT TAG_WAITFORITEMLISTBOX = 886;
const UINT TAG_DISPLAYFILTER = 937;
const UINT TAG_X_COORD_LABEL = 936;
const UINT TAG_Y_COORD_LABEL = 935;
const UINT TAG_X_COORD = 934;
const UINT TAG_Y_COORD = 933;
const UINT TAG_COLOR_LABEL = 932;
const UINT TAG_TEXT2 = 931;
const UINT TAG_NATURAL_TARGET_TYPES = 884;

const UINT TAG_CHARACTERNAME = 929;
const UINT TAG_ADDCHARACTER = 928;
const UINT TAG_CHARACTERLISTBOX = 927;
const UINT TAG_CHARGRAPHICLISTBOX = 926;
const UINT TAG_AVATARFACE = 925;
const UINT TAG_CUSTOMAVATAR = 924;
const UINT TAG_DELETECHARACTER = 923;
const UINT TAG_DEFAULTAVATAR = 922;
const UINT TAG_TILESIMAGE = 921;
const UINT TAG_CUSTOMTILES = 920;
const UINT TAG_DEFAULTTILES = 919;

const UINT TAG_STEALTHLISTBOX = 918;
const UINT TAG_WATERTRAVERSALLISTBOX = 917;
const UINT TAG_EDITDEFAULTSCRIPT = 916;
const UINT TAG_CUSTOM_NPCS = 915;
const UINT TAG_GLOBALSCRIPTLISTBOX = 914;
const UINT TAG_CUSTOM_NPC_ID = 913;
const UINT TAG_DIRECTIONLISTBOX2 = 910;
const UINT TAG_VISUALEFFECTS_LISTBOX = 909;
const UINT TAG_DIRECTIONLISTBOX3 = 908;
const UINT TAG_ONOFFLISTBOX3 = 907;
const UINT TAG_DIRECTIONLABEL2 = 906;
const UINT TAG_SOUNDEFFECTLABEL = 905;
const UINT TAG_ANIMATESPEED = 904;
const UINT TAG_ICONDISPLAY = 903;
const UINT TAG_IMAGEDISPLAY = 902;

const UINT TAG_WEAPON_LISTBOX = 901;
const UINT TAG_ATTACKTILE = 900;

const UINT TAG_ADDCOMMAND2 = 899;
const UINT TAG_DELETECOMMAND2 = 898;
const UINT TAG_DEFAULTCOMMANDSLISTBOX = 897;
const UINT TAG_OK2 = 896;

const UINT TAG_CHAROPTIONS = 895;
const UINT TAG_CHAROPTIONS2 = 894;
const UINT TAG_INPUTLABEL = 893;
const UINT TAG_INPUTLISTBOX = 892;
const UINT TAG_IMAGEOVERLAY_LABEL = 891;
const UINT TAG_IMAGEOVERLAYTEXT = 890;

const UINT TAG_WEAPON_LISTBOX2 = 883;
const UINT TAG_BEHAVIOR_LISTBOX = 882;
const UINT TAG_REMAINS_LISTBOX = 881;
const UINT TAG_ONOFFLISTBOX4 = 880;
const UINT TAG_KEEPBEHAVIOR_LABEL = 879;
const UINT TAG_MOVETYPELISTBOX = 878;

const UINT MAX_TEXT_LABEL_SIZE = 100;

const UINT CX_DIALOG = 820;
const UINT CY_DIALOG = 688;

const UINT CX_SPACE = 15;
const UINT CY_SPACE = 10;

const UINT LIST_LINE_HEIGHT = 22;

const SURFACECOLOR PaleRed = {255, 192, 192};

std::map<UINT, UINT> CCharacterDialogWidget::speechLengthCache;
const UINT CCharacterDialogWidget::INDENT_PREFIX_SIZE = 8;
const UINT CCharacterDialogWidget::INDENT_TAB_SIZE = 3;
const UINT CCharacterDialogWidget::INDENT_IF_CONDITION_SIZE = 5;

#define NOT_FOUND (UINT(-1))

void stripTrailingWhitespace(WSTRING& text)
{
	UINT textLength = text.length();
	while (textLength && iswspace(text[textLength-1]))
		text.resize(--textLength);
}

//******************************************************************************
bool TranslateColorText(const WSTRING& colorText, CCharacterCommand *pCommand)
{
	pCommand->h = 0;

	ASSERT(pCommand);

	static const size_t LENGTH=6;
	if (colorText.length() != LENGTH)
		return false;

	const string color = UnicodeToUTF8(colorText);
	for (size_t i=0; i<LENGTH; ++i) {
		const char c = tolower(color[i]);
		if (!isalnum(c) || c > 'f')
			return false;
	}

	string part = color.substr(0, 2);
	pCommand->x = (UINT)strtol(part.c_str(), NULL, 16);
	part = color.substr(2, 2);
	pCommand->y = (UINT)strtol(part.c_str(), NULL, 16);
	part = color.substr(4, 2);
	pCommand->w = (UINT)strtol(part.c_str(), NULL, 16);

	pCommand->h = 1; //valid color

	return true;
}

//******************************************************************************
WSTRING SetColorText(UINT r, UINT g, UINT b)
{
	char temp[7];
	sprintf(temp, "%02X%02X%02X", int(r) & 0xff, int(g) & 0xff, int(b) & 0xff);

	WSTRING colorText;
	AsciiToUnicode(temp, colorText);
	return colorText;
}

//******************************************************************************
void AddOperatorSymbol(WSTRING& wstr, const UINT op)
{
	switch (op)
	{
		case ScriptVars::Equals:
		case ScriptVars::EqualsText: wstr += wszEqual; break;
		case ScriptVars::Greater: wstr += wszCloseAngle; break;
		case ScriptVars::GreaterThanOrEqual: wstr += wszCloseAngle; wstr += wszEqual; break;
		case ScriptVars::Less: wstr += wszOpenAngle; break;
		case ScriptVars::LessThanOrEqual: wstr += wszOpenAngle; wstr += wszEqual; break;
		case ScriptVars::Inequal: wstr += wszExclamation; wstr += wszEqual; break;
		default: wstr += wszQuestionMark; break;
	}
}

//******************************************************************************
CRenameDialogWidget::CRenameDialogWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CDialogWidget
	const int nSetX, const int nSetY,      //    constructor.
	const UINT wSetW, const UINT wSetH,    //
	const bool bListBoxDoubleClickReturns) //[default = false]
	: CDialogWidget(dwSetTagNo, nSetX, nSetY, wSetW, wSetH, bListBoxDoubleClickReturns)
{
}

//*****************************************************************************
void CRenameDialogWidget::OnBetweenEvents()
{
	//When reentering the dialog, after having prompted the user for parameter
	//info for a custom NPC default script command, reenter the default script dialog.
	if (GetTagNo() == TAG_CUSTOM_NPCS)
	{
		CCharacterDialogWidget *pParent = DYN_CAST(CCharacterDialogWidget*,
				CWidget*, this->pParent);
		if (pParent->IsEditingDefaultScript())
		{
			pParent->EditDefaultScriptForCustomNPC();

			//If the parent has been flagged to deactivate while editing the
			//default script, this dialog should be deactivated now.
			if (pParent->IsDeactivating() && !IsDeactivating())
				Deactivate();
		}
	}
}

//*****************************************************************************
void CRenameDialogWidget::OnDoubleClick(
//Handles double click event.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	CDialogWidget::OnDoubleClick(dwTagNo);
	if (IsDeactivating())
		return;

	//ClickedSelection is queried in the methods called below.

	switch (dwTagNo)
	{
		case TAG_VARLIST:
		{
			CCharacterDialogWidget *pParent = DYN_CAST(CCharacterDialogWidget*,
						CWidget*, this->pParent);
			pParent->RenameVar();
		}
		break;

		case TAG_CHARACTERLISTBOX:
		{
			CCharacterDialogWidget *pParent = DYN_CAST(CCharacterDialogWidget*,
						CWidget*, this->pParent);
			if (pParent->RenameCharacter()) {
				const UINT charID = pParent->pCharListBox->GetSelectedItem();
				pParent->PopulateMainGraphicList(); //Refresh list.
				pParent->pCharListBox->SelectItem(charID);
			}
		}
		break;

		case TAG_DEFAULTCOMMANDSLISTBOX:
		{
			CCharacterDialogWidget *pParent = DYN_CAST(CCharacterDialogWidget*,
						CWidget*, this->pParent);
			pParent->EditClickedCommand();

			if (pParent->IsDeactivating())
			{
				if (!IsDeactivating())
					Deactivate(); //exit to prompt user
			}
		}
		break;

		default: return; //don't need to redraw anything
	}

	if (this->pParent)
		this->pParent->Paint();   //refresh screen
	Paint();
}

//*****************************************************************************
void CRenameDialogWidget::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Event.
{
	//Trap ESC so it doesn't close the parent dialog.
	if (Key.keysym.sym == SDLK_ESCAPE)
	{
		CWidget *pWidget = GetWidget(TAG_CANCEL);
		if (!pWidget)
			pWidget = GetWidget(TAG_OK2);
		if (!pWidget)
			pWidget = GetWidget(TAG_OK);
		if (pWidget)
			OnClick(pWidget->GetTagNo()); //deactivate
		return;
	}

	CCharacterDialogWidget *pParent = DYN_CAST(CCharacterDialogWidget*,
			CWidget*, this->pParent);
	if (pParent->IsEditingDefaultScript())
		pParent->OnKeyDown(dwTagNo, Key);
}

//*****************************************************************************
void CRenameDialogWidget::OnRearranged(const UINT dwTagNo)
//Called when the default commands list has been reordered.
{
	CCharacterDialogWidget *pParent = DYN_CAST(CCharacterDialogWidget*,
			CWidget*, this->pParent);
	if (pParent->IsEditingDefaultScript())
		pParent->OnRearranged(dwTagNo);
}

//*****************************************************************************
CCharacterDialogWidget::CCharacterDialogWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget
	const int nSetX, const int nSetY)         //    constructor.
	: CDialogWidget(dwSetTagNo, nSetX, nSetY, CX_DIALOG, CY_DIALOG)
	, pGraphicListBox(NULL), pCommandsListBox(NULL)
	, pAddCommandDialog(NULL), pAddCharacterDialog(NULL)
	, pDefaultScriptCommandsListBox(NULL)
	, pScriptDialog(NULL)
	, pDirectionListBox2(NULL), pDirectionListBox3(NULL)
	, pOnOffListBox3(NULL), pOnOffListBox4(NULL)
	, pVisualEffectsListBox(NULL)
	, pActionListBox(NULL), pEventListBox(NULL)
	, pSpeakerListBox(NULL), pMoodListBox(NULL)
	, pDirectionListBox(NULL), pInputListBox(NULL)
	, pStealthListBox(NULL), pWaterTraversalListBox(NULL), pGlobalScriptListBox(NULL)
	, pOnOffListBox(NULL), pOnOffListBox2(NULL), pOpenCloseListBox(NULL)
	, pGotoLabelListBox(NULL), pMusicListBox(NULL)
	, pVarListBox(NULL), pVarOpListBox(NULL), pVarCompListBox(NULL)
	, pWaitFlagsListBox(NULL), pImperativeListBox(NULL), pBuildItemsListBox(NULL)
	, pBuildMarkerListBox(NULL), pWaitForItemsListBox(NULL)
	, pCharNameText(NULL), pCharListBox(NULL)
	, pDisplayFilterListBox(NULL)
	, pWorldMapIconFlagListBox(NULL)
	, pWorldMapImageFlagListBox(NULL)
	, pWeaponListBox(NULL)
	, pWeaponFlagsListBox(NULL)
	, pAttackTileListBox(NULL)
	, pMovementTypeListBox(NULL)

	, pCharacter(NULL)
	, pCommand(NULL)
	, pSound(NULL)
	, wIncrementedLabel(0)
	, bEditingCommand(false), bRetainFields(false)
	, bEditingDefaultScript(false)
	, defaultScriptCustomCharID(0)
{
	static const UINT CX_TITLE = 240;
	static const UINT CY_TITLE = CY_LABEL_FONT_HEADER;
	static const int X_TITLE = (CX_DIALOG - CX_TITLE) / 2;
	static const int Y_TITLE = CY_SPACE;

	static const int X_COMMANDSLABEL = CX_SPACE;
	static const int Y_COMMANDSLABEL = Y_TITLE + CY_TITLE + CY_SPACE;
	static const UINT CX_COMMANDSLABEL = 110;
	static const UINT CY_COMMANDSLABEL = 30;
	static const int X_COMMANDS = X_COMMANDSLABEL;
	static const int Y_COMMANDS = Y_COMMANDSLABEL + CY_COMMANDSLABEL;
	static const UINT CX_COMMANDS = 610;
	static const UINT CY_COMMANDS = 25*LIST_LINE_HEIGHT + 4;
#ifdef RUSSIAN_BUILD
	static const UINT CX_ADDCOMMAND = 180;
	static const UINT CX_DELETECOMMAND = 180;
#else
	static const UINT CX_CHAROPTIONS = 100;
	static const UINT CX_ADDCOMMAND = 130;
	static const UINT CX_DELETECOMMAND = 150;
#endif

	static const int X_ADDCOMMAND = X_COMMANDS + CX_COMMANDS - CX_ADDCOMMAND - CX_SPACE;
	static const int Y_ADDCOMMAND = Y_COMMANDSLABEL - 4;
	static const UINT CY_ADDCOMMAND = CY_STANDARD_BUTTON;
	static const int X_DELETECOMMAND = X_ADDCOMMAND - CX_DELETECOMMAND - CX_SPACE;
	static const int Y_DELETECOMMAND = Y_ADDCOMMAND;
	static const UINT CY_DELETECOMMAND = CY_STANDARD_BUTTON;
	static const int X_CHAROPTIONS = X_DELETECOMMAND - CX_CHAROPTIONS - CX_SPACE;
	static const int Y_CHAROPTIONS = Y_ADDCOMMAND;
	static const UINT CY_CHAROPTIONS = CY_STANDARD_BUTTON;

	static const UINT CX_GRAPHICLISTBOX = 170;
	static const int X_GRAPHICLABEL = CX_DIALOG - CX_GRAPHICLISTBOX - CX_SPACE;
	static const int Y_GRAPHICLABEL = Y_ADDCOMMAND;
	static const UINT CX_GRAPHICLABEL = 80;
	static const UINT CY_GRAPHICLABEL = 30;

	static const int X_CHARACTERS = X_GRAPHICLABEL;
	static const int Y_CHARACTERS = Y_COMMANDS;
	static const UINT CX_CHARACTERS = CX_GRAPHICLISTBOX - CX_SPACE*2;
	static const UINT CY_CHARACTERS = CY_STANDARD_BUTTON;

	static const int X_GRAPHICLISTBOX = X_GRAPHICLABEL;
	static const int Y_GRAPHICLISTBOX = Y_CHARACTERS + CY_CHARACTERS + CY_SPACE;
	static const UINT CY_GRAPHICLISTBOX = 23*LIST_LINE_HEIGHT + 4;

	static const int X_ISVISIBLE = X_GRAPHICLABEL;
	static const int Y_ISVISIBLE = Y_GRAPHICLISTBOX + CY_GRAPHICLISTBOX;
	static const UINT CX_ISVISIBLE = CX_GRAPHICLISTBOX;
	static const UINT CY_ISVISIBLE = CY_STANDARD_OPTIONBUTTON;

	static const UINT CX_BUTTON = 70;
	static const int X_OKBUTTON = (CX_DIALOG - (CX_BUTTON + CX_SPACE)) / 2;
	static const int Y_OKBUTTON = CY_DIALOG - CY_STANDARD_BUTTON - CY_SPACE;

	AddWidget(new CLabelWidget(0L, X_TITLE, Y_TITLE, CX_TITLE, CY_TITLE,
			F_Header, g_pTheDB->GetMessageText(MID_CustomizeCharacter)));

	//Commands.
	AddWidget(new CButtonWidget(TAG_ADDCOMMAND, X_ADDCOMMAND, Y_ADDCOMMAND,
			CX_ADDCOMMAND, CY_ADDCOMMAND, g_pTheDB->GetMessageText(MID_AddCommand)));
	AddWidget(new CButtonWidget(TAG_DELETECOMMAND, X_DELETECOMMAND, Y_DELETECOMMAND,
			CX_DELETECOMMAND, CY_DELETECOMMAND, g_pTheDB->GetMessageText(MID_DeleteCommand)));
	AddWidget(new CButtonWidget(TAG_CHAROPTIONS, X_CHAROPTIONS, Y_CHAROPTIONS,
			CX_CHAROPTIONS, CY_CHAROPTIONS, g_pTheDB->GetMessageText(MID_CharOptions)));

	AddWidget(new CLabelWidget(0L, X_COMMANDSLABEL, Y_COMMANDSLABEL,
			CX_COMMANDSLABEL, CY_COMMANDSLABEL, F_Small, g_pTheDB->GetMessageText(MID_Commands)));
	this->pCommandsListBox = new CCommandListBoxWidget(TAG_COMMANDSLISTBOX, X_COMMANDS, Y_COMMANDS,
			CX_COMMANDS, CY_COMMANDS);
	AddWidget(this->pCommandsListBox);

	//Appearance (character/tile graphic).
	CButtonWidget *pButton = new CButtonWidget(TAG_CHARACTERS,
			X_CHARACTERS, Y_CHARACTERS, CX_CHARACTERS, CY_CHARACTERS,
			g_pTheDB->GetMessageText(MID_Characters));
	AddWidget(pButton);

	AddWidget(new CLabelWidget(0L, X_GRAPHICLABEL, Y_GRAPHICLABEL,
			CX_GRAPHICLABEL, CY_GRAPHICLABEL, F_Small, g_pTheDB->GetMessageText(MID_Graphic)));
	this->pGraphicListBox = new CListBoxWidget(TAG_GRAPHICLISTBOX,
			X_GRAPHICLISTBOX, Y_GRAPHICLISTBOX, CX_GRAPHICLISTBOX, CY_GRAPHICLISTBOX, true);
	this->pGraphicListBox->SetHotkeyItemSelection(true);
	this->pGraphicListBox->SortAlphabetically(true);
	AddWidget(this->pGraphicListBox);

	this->pIsVisibleButton = new COptionButtonWidget(TAG_ISVISIBLE,
			X_ISVISIBLE, Y_ISVISIBLE, CX_ISVISIBLE, CY_ISVISIBLE,
			g_pTheDB->GetMessageText(MID_IsVisible), false);
	AddWidget(this->pIsVisibleButton);

	//OK/cancel buttons.
	pButton = new CButtonWidget(
			TAG_OK, X_OKBUTTON, Y_OKBUTTON, CX_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Okay));
	AddWidget(pButton);

	this->pCharOptionsDialog = new CCharacterOptionsDialog();

	AddWidget(this->pCharOptionsDialog);
	this->pCharOptionsDialog->Center();
	this->pCharOptionsDialog->Hide();

	AddCommandDialog();
	AddCharacterDialog();
	AddScriptDialog();
}

//*****************************************************************************
CCharacterDialogWidget::~CCharacterDialogWidget()
{
	ASSERT(this->commandBuffer.empty());
}

//*****************************************************************************
void CCharacterDialogWidget::OnBetweenEvents()
{
	//When reentering the dialog, after having prompted the user for parameter
	//info for a custom NPC default script command, reenter the custom character
	//dialog in order to pop up the default script dialog again.
	if (this->bEditingDefaultScript)
	{
		//Select the custom NPC whose script is being edited.
		ASSERT(this->defaultScriptCustomCharID);
		this->pCharListBox->SelectItem(this->defaultScriptCustomCharID);

		EditCustomCharacters();
	}
}

//*****************************************************************************
void CCharacterDialogWidget::FinishCommand(
//Finish filling in the parameters to the command being added.
//
//Params:
	const UINT wX, const UINT wY, //(in)
	const UINT wW, const UINT wH) //(in) [default=-1]
{
	ASSERT(this->pCommand);
	this->pCommand->x = wX;
	this->pCommand->y = wY;
	if (wW != static_cast<UINT>(-1))
		this->pCommand->w = wW;
	if (wH != static_cast<UINT>(-1))
		this->pCommand->h = wH;
	AddCommand();
	SetWidgetStates();
	ASSERT(!this->pCommand);
}

//*****************************************************************************
void CCharacterDialogWidget::FinishCommandAndExit()
//If a command being defined can not be completed, exit the dialog gracefully,
//leaving it in a proper state.
{
	FinishCommand(0,0);

	//Save the NPC and script data in its current state to the room/hold.
	UpdateCharacter();

	if (this->bEditingDefaultScript)
		FinishEditingDefaultScript();
}

//*****************************************************************************
bool CCharacterDialogWidget::RenameCharacter()
//Prompt the user to rename the selected custom hold character.
//
//Returns: whether a character was renamed
{
	if (!this->pCharListBox->ClickedSelection())
		return false;

	const UINT charID = this->pCharListBox->GetSelectedItem();
	if (!charID)
		return false;

	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen);
	ASSERT(pEditRoomScreen->pHold);

	WSTRING wstr = this->pCharListBox->GetSelectedItemText();
	const UINT answerTagNo = pEditRoomScreen->ShowTextInputMessage(
			MID_RenameCharacterPrompt, wstr, false, true);
	if (answerTagNo != TAG_OK)
		return false;
	ASSERT(!wstr.empty());

	if (!pEditRoomScreen->pHold->RenameCharacter(charID, wstr))
	{
		pEditRoomScreen->ShowOkMessage(MID_CharNameDuplicationError);
		return false;
	}

	this->pCharListBox->SetSelectedItemText(wstr.c_str());
	this->pCharListBox->Paint();
	return true;
}

//*****************************************************************************
bool CCharacterDialogWidget::RenameVar()
//Prompt the user to rename the selected hold variable.
//
//Returns: whether a var was renamed
{
	if (!this->pVarListBox->ClickedSelection())
		return false;

	const UINT varID = this->pVarListBox->GetSelectedItem();
	if (!varID)
		return false;

	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen);
	ASSERT(pEditRoomScreen->pHold);

	//Not allowed to rename predefined variables.
	if (varID >= (UINT)ScriptVars::FirstPredefinedVar)
	{
		pEditRoomScreen->ShowOkMessage(MID_VarRenameNotAllowed);
		return false;
	}

	bool bGoodSyntax;
	WSTRING wstr;
	do {
		wstr = this->pVarListBox->GetSelectedItemText();
		const UINT answerTagNo = pEditRoomScreen->ShowTextInputMessage(
				MID_RenameVariablePrompt, wstr, false, true);
		if (answerTagNo != TAG_OK)
			return false;
		ASSERT(!wstr.empty());

		bGoodSyntax = CDbHold::IsVarNameGoodSyntax(wstr.c_str());
		if (!bGoodSyntax)
			pEditRoomScreen->ShowOkMessage(MID_VarNameSyntaxError);
	} while (!bGoodSyntax);

	if (!pEditRoomScreen->pHold->RenameVar(varID, wstr))
	{
		pEditRoomScreen->ShowOkMessage(MID_VarNameDuplicationError);
		return false;
	}

	this->pVarListBox->SetSelectedItemText(wstr.c_str());
	this->pVarListBox->Paint();
	return true;
}

//*****************************************************************************
void CCharacterDialogWidget::AddCharacterDialog()
//Create another dialog for managing custom characters.
//Separated from constructor for readability.
{
	static const UINT CX_CHAR_DIALOG = 820;
	static const UINT CY_CHAR_DIALOG = 650;

	static const UINT CY_TITLE = CY_LABEL_FONT_HEADER;
	static const int Y_TITLE = CY_SPACE;

	static const int X_TEXTLABEL = CX_SPACE;
	static const int Y_TEXTLABEL = Y_TITLE + CY_TITLE;
	static const UINT CX_TEXTLABEL = 220;
	static const UINT CY_TEXTLABEL = 30;
	static const int X_TEXT = X_TEXTLABEL;
	static const int Y_TEXT = Y_TEXTLABEL + CY_TEXTLABEL;
	static const UINT CX_TEXT = CX_TEXTLABEL;
	static const UINT CY_TEXT = 30;

	static const int X_ADDCHAR = X_TEXT + CX_TEXT + CX_SPACE;
	static const int Y_ADDCHAR = Y_TEXT;
#ifdef RUSSIAN_BUILD
	static const int CX_ADDCHAR = 200;
#else
	static const int CX_ADDCHAR = 130;
#endif
	static const int CY_ADDCHAR = CY_STANDARD_BUTTON;

	static const int X_CHARLABEL = X_TEXTLABEL;
	static const int Y_CHARLABEL = Y_TEXT + CY_TEXT + CY_SPACE*2;
	static const UINT CX_CHARLABEL = 140;
	static const UINT CY_CHARLABEL = CY_TEXTLABEL;
	static const int X_CHARLISTBOX = X_CHARLABEL;
	static const int Y_CHARLISTBOX = Y_CHARLABEL + CY_CHARLABEL;
	static const UINT CX_CHARLISTBOX = CX_TEXT;
	static const UINT CY_CHARLISTBOX = 16*LIST_LINE_HEIGHT + 4; //16 slots

	static const int X_CHARIDLABEL = X_CHARLABEL + CX_CHARLABEL;

	static const int X_EDITDEFAULTSCRIPT = X_CHARLISTBOX;
	static const int Y_EDITDEFAULTSCRIPT = Y_CHARLISTBOX + CY_CHARLISTBOX + CY_SPACE/2;
	static const int CX_EDITDEFAULTSCRIPT = 170;
	static const int CY_EDITDEFAULTSCRIPT = CY_STANDARD_BUTTON;
	static const int Y_DELETE = Y_EDITDEFAULTSCRIPT + CY_EDITDEFAULTSCRIPT + CY_SPACE;

	static const int X_DELETE = X_CHARLISTBOX;
	static const int CX_DELETE = 100;
	static const int CY_DELETE = CY_STANDARD_BUTTON;

	static const int X_GRAPHICLABEL = X_CHARLABEL + CX_TEXTLABEL + CX_SPACE;
	static const int Y_GRAPHICLABEL = Y_CHARLABEL;
	static const UINT CX_GRAPHICLABEL = 190;
	static const UINT CY_GRAPHICLABEL = CY_CHARLABEL;
	static const int X_GRAPHICLISTBOX3 = X_GRAPHICLABEL;
	static const int Y_GRAPHICLISTBOX3 = Y_CHARLISTBOX;
	static const UINT CX_GRAPHICLISTBOX3 = CX_GRAPHICLABEL;
	static const UINT CY_GRAPHICLISTBOX3 = CY_CHARLISTBOX;

	static const int X_AVATARLABEL = X_GRAPHICLABEL + CX_GRAPHICLABEL + CX_SPACE;
	static const int Y_AVATARLABEL = Y_GRAPHICLABEL;
	static const UINT CY_AVATARLABEL = CY_TEXTLABEL;

	static const int X_AVATAR = X_AVATARLABEL;
	static const int Y_AVATAR = Y_AVATARLABEL + CY_AVATARLABEL;
	static const UINT CX_AVATAR = CX_FACE;
	static const UINT CY_AVATAR = CY_FACE;
	static const UINT CX_AVATARLABEL = CX_AVATAR;

	static const int X_SETAVATAR = X_AVATAR;
	static const int Y_SETAVATAR = Y_AVATAR + CY_AVATAR + CY_SPACE;
#ifdef RUSSIAN_BUILD
	static const int CX_SETAVATAR = 230;
	static const int CX_DEFAULTAVATAR = 160;
#else
	static const int CX_SETAVATAR = 130;
	static const int CX_DEFAULTAVATAR = CX_SETAVATAR;
#endif
	static const int CY_SETAVATAR = CY_STANDARD_BUTTON;

	static const int X_DEFAULTAVATAR = X_AVATAR;
	static const int Y_DEFAULTAVATAR = Y_SETAVATAR + CY_SETAVATAR + CY_SPACE/2;
	static const int CY_DEFAULTAVATAR = CY_SETAVATAR;

	//Custom tiles.
	static const int X_TILESLABEL = X_DEFAULTAVATAR + CX_DEFAULTAVATAR + CX_SPACE;
	static const int Y_TILESLABEL = Y_GRAPHICLABEL;
	static const UINT CX_TILESLABEL = 100;
	static const UINT CY_TILESLABEL = CY_TEXTLABEL;

	static const int X_TILES = X_TILESLABEL;
	static const int Y_TILES = Y_TILESLABEL + CY_TILESLABEL;
//	static const UINT CX_TILES = 9 * CDrodBitmapManager::CX_TILE;
	static const UINT CY_TILES = 6 * CDrodBitmapManager::CY_TILE;

	static const int X_SETTILES = X_TILES;
	static const int Y_SETTILES = Y_TILES + CY_TILES + CY_SPACE;
#ifdef RUSSIAN_BUILD
	static const int CX_SETTILES = 170;
	static const int CX_DEFAULTTILES = 160;
#else
	static const int CX_SETTILES = 130;
	static const int CX_DEFAULTTILES = CX_SETTILES;
#endif
	static const int CY_SETTILES = CY_STANDARD_BUTTON;

	static const int X_ANIMATESPEEDLABEL = X_TILES;
	static const int Y_ANIMATESPEEDLABEL = Y_SETTILES + CY_SETTILES + CY_SPACE/2;
	static const UINT CX_ANIMATESPEEDLABEL = 200;
	static const UINT CY_ANIMATESPEEDLABEL = CY_TEXTLABEL;

	static const int X_ANIMATESPEED = X_ANIMATESPEEDLABEL;
	static const int Y_ANIMATESPEED = Y_ANIMATESPEEDLABEL + CY_ANIMATESPEEDLABEL;
	static const UINT CX_ANIMATESPEED = 130;
	static const UINT CY_ANIMATESPEED = CY_STANDARD_TBOX;

	static const int X_DEFAULTTILES = X_TILES;
	static const int Y_DEFAULTTILES = Y_ANIMATESPEED + CY_ANIMATESPEED + CY_SPACE;
	static const int CY_DEFAULTTILES = CY_SETTILES;

	static const UINT CX_OKBUTTON = 70;
	static const int X_OKBUTTON = (CX_CHAR_DIALOG - CX_OKBUTTON) / 2;
	static const int Y_OKBUTTON = CY_CHAR_DIALOG - CY_STANDARD_BUTTON - CY_SPACE;

	ASSERT(!this->pAddCharacterDialog);
	this->pAddCharacterDialog = new CRenameDialogWidget(TAG_CUSTOM_NPCS,
			-175, GetY() + (GetH()-CY_CHAR_DIALOG)/2,
			CX_CHAR_DIALOG, CY_CHAR_DIALOG);
	CLabelWidget *pTitle = new CLabelWidget(0L, 0, Y_TITLE,
			CX_CHAR_DIALOG, CY_TITLE, F_Header, g_pTheDB->GetMessageText(MID_CharacterManagement));
	pTitle->SetAlign(CLabelWidget::TA_CenterGroup);
	this->pAddCharacterDialog->AddWidget(pTitle);

	//New character.
	this->pAddCharacterDialog->AddWidget(new CLabelWidget(0L, X_TEXTLABEL,
			Y_TEXTLABEL, CX_TEXTLABEL, CY_TEXTLABEL, F_Small, g_pTheDB->GetMessageText(MID_NewCharacterName)));
	this->pCharNameText = new CTextBoxWidget(TAG_CHARACTERNAME, X_TEXT, Y_TEXT,
			CX_TEXT, CY_TEXT);
	this->pCharNameText->AddHotkey(SDLK_RETURN, TAG_ADDCHARACTER);
	this->pAddCharacterDialog->AddWidget(this->pCharNameText);
	CButtonWidget *pButton = new CButtonWidget(TAG_ADDCHARACTER, X_ADDCHAR,
			Y_ADDCHAR, CX_ADDCHAR, CY_ADDCHAR, g_pTheDB->GetMessageText(MID_AddCharacter));
	this->pAddCharacterDialog->AddWidget(pButton);

	this->pAddCharacterDialog->AddWidget(new CLabelWidget(TAG_CUSTOM_NPC_ID, X_CHARIDLABEL, Y_CHARLABEL,
			CX_CHARLABEL, CY_CHARLABEL, F_Small, wszEmpty));

	//Character list.
	this->pAddCharacterDialog->AddWidget(new CLabelWidget(0L, X_CHARLABEL, Y_CHARLABEL,
			CX_CHARLABEL, CY_CHARLABEL, F_Small, g_pTheDB->GetMessageText(MID_Characters)));
	this->pCharListBox = new CListBoxWidget(TAG_CHARACTERLISTBOX,
			X_CHARLISTBOX, Y_CHARLISTBOX, CX_CHARLISTBOX, CY_CHARLISTBOX, true);
	this->pAddCharacterDialog->AddWidget(this->pCharListBox);

	//Edit NPC default script button.
	pButton = new CButtonWidget(TAG_EDITDEFAULTSCRIPT, X_EDITDEFAULTSCRIPT, Y_EDITDEFAULTSCRIPT,
			CX_EDITDEFAULTSCRIPT, CY_EDITDEFAULTSCRIPT, g_pTheDB->GetMessageText(MID_EditDefaultScript));
	this->pAddCharacterDialog->AddWidget(pButton);

	//Delete button.
	pButton = new CButtonWidget(TAG_DELETECHARACTER, X_DELETE, Y_DELETE,
			CX_DELETE, CY_DELETE, g_pTheDB->GetMessageText(MID_Delete));
	this->pAddCharacterDialog->AddWidget(pButton);

	//Graphic list.
	this->pAddCharacterDialog->AddWidget(new CLabelWidget(0L, X_GRAPHICLABEL, Y_GRAPHICLABEL,
			CX_GRAPHICLABEL, CY_GRAPHICLABEL, F_Small, g_pTheDB->GetMessageText(MID_Graphic)));
	CListBoxWidget *pGraphicListBox = new CListBoxWidget(TAG_CHARGRAPHICLISTBOX,
			X_GRAPHICLISTBOX3, Y_GRAPHICLISTBOX3, CX_GRAPHICLISTBOX3, CY_GRAPHICLISTBOX3,
			true);
	PopulateGraphicListBox(pGraphicListBox);
	pGraphicListBox->AddItem(M_BEETHRO_IN_DISGUISE, g_pTheDB->GetMessageText(MID_BeethroInDisguise));
	pGraphicListBox->SelectItem(M_BEETHRO);
	this->pAddCharacterDialog->AddWidget(pGraphicListBox);

	//Avatar.
	this->pAddCharacterDialog->AddWidget(new CLabelWidget(0L, X_AVATARLABEL, Y_AVATARLABEL,
			CX_AVATARLABEL, CY_AVATARLABEL, F_Small, g_pTheDB->GetMessageText(MID_Avatar)));
	CFaceWidget *pFace = new CFaceWidget(TAG_AVATARFACE,
			X_AVATAR, Y_AVATAR, CX_AVATAR, CY_AVATAR);
	this->pAddCharacterDialog->AddWidget(pFace);
	pFace->PaintFull();
	pButton = new CButtonWidget(TAG_CUSTOMAVATAR, X_SETAVATAR, Y_SETAVATAR,
			CX_SETAVATAR, CY_SETAVATAR, g_pTheDB->GetMessageText(MID_CustomAvatar));
	this->pAddCharacterDialog->AddWidget(pButton);
	pButton = new CButtonWidget(TAG_DEFAULTAVATAR, X_DEFAULTAVATAR, Y_DEFAULTAVATAR,
			CX_DEFAULTAVATAR, CY_DEFAULTAVATAR, g_pTheDB->GetMessageText(MID_Default));
	this->pAddCharacterDialog->AddWidget(pButton);

	//Tiles.
	this->pAddCharacterDialog->AddWidget(new CLabelWidget(0L, X_TILESLABEL, Y_TILESLABEL,
			CX_TILESLABEL, CY_TILESLABEL, F_Small, g_pTheDB->GetMessageText(MID_Tiles)));
	this->pAddCharacterDialog->AddWidget(new CImageWidget(TAG_TILESIMAGE, X_TILES,
			Y_TILES, wszEmpty));
	pButton = new CButtonWidget(TAG_CUSTOMTILES, X_SETTILES, Y_SETTILES,
			CX_SETTILES, CY_SETTILES, g_pTheDB->GetMessageText(MID_CustomTiles));
	this->pAddCharacterDialog->AddWidget(pButton);

	this->pAddCharacterDialog->AddWidget(new CLabelWidget(0L, X_ANIMATESPEEDLABEL,
			Y_ANIMATESPEEDLABEL, CX_ANIMATESPEEDLABEL, CY_ANIMATESPEEDLABEL,
			F_Small, g_pTheDB->GetMessageText(MID_AnimationSpeed)));
	CTextBoxWidget *pAnimateSpeedText = new CTextBoxWidget(TAG_ANIMATESPEED,
			X_ANIMATESPEED, Y_ANIMATESPEED,
			CX_ANIMATESPEED, CY_ANIMATESPEED, 7);
	pAnimateSpeedText->SetDigitsOnly();
	this->pAddCharacterDialog->AddWidget(pAnimateSpeedText);

	pButton = new CButtonWidget(TAG_DEFAULTTILES, X_DEFAULTTILES, Y_DEFAULTTILES,
			CX_DEFAULTTILES, CY_DEFAULTTILES, g_pTheDB->GetMessageText(MID_Default));
	this->pAddCharacterDialog->AddWidget(pButton);

	//OK.
	pButton = new CButtonWidget(TAG_OK, X_OKBUTTON, Y_OKBUTTON, CX_OKBUTTON,
			CY_STANDARD_BUTTON, g_pTheDB->GetMessageText(MID_Okay));
	this->pAddCharacterDialog->AddWidget(pButton);

	//Add to main dialog.
	AddWidget(this->pAddCharacterDialog);
	this->pAddCharacterDialog->Center();
	this->pAddCharacterDialog->Hide();
}

//*****************************************************************************
void CCharacterDialogWidget::AddCommandDialog()
//Create another dialog for entering a script command.
//Separated from constructor for readability.
{
	static const UINT CY_COMMAND_DIALOG = 412;
	static const UINT CY_ACTIONLISTBOX = 13*LIST_LINE_HEIGHT+4;

#ifdef RUSSIAN_BUILD
	static const UINT CX_COMMAND_DIALOG = 795;
	static const UINT CX_TITLE = 350;
	static const UINT CX_ACTIONLISTBOX = 265;
#else
	
	static const UINT CX_COMMAND_DIALOG = 775;
	static const UINT CX_TITLE = 250;
	static const UINT CX_ACTIONLISTBOX = 245;
#endif
	static const UINT CY_TITLE = CY_LABEL_FONT_HEADER;
	static const int X_TITLE = (CX_COMMAND_DIALOG - CX_TITLE) / 2;
	static const int Y_TITLE = CY_SPACE;

	static const int X_ACTIONLABEL = CX_SPACE;
	static const int Y_ACTIONLABEL = Y_TITLE + CY_TITLE;
	static const UINT CX_ACTIONLABEL = 90;
	static const UINT CY_ACTIONLABEL = 30;
	static const int X_ACTIONLISTBOX = X_ACTIONLABEL;
	static const int Y_ACTIONLISTBOX = Y_ACTIONLABEL + CY_ACTIONLABEL;

	static const UINT CX_EVENTLISTBOX = 350;
	static const UINT CY_EVENTLISTBOX = CY_ACTIONLISTBOX;
	static const int X_EVENTLABEL = X_ACTIONLISTBOX + CX_ACTIONLISTBOX + CX_SPACE;
	static const int Y_EVENTLABEL = Y_ACTIONLABEL;
	static const UINT CX_EVENTLABEL = CX_EVENTLISTBOX;
	static const UINT CY_EVENTLABEL = 30;
	static const int X_EVENTLISTBOX = X_EVENTLABEL;
	static const int Y_EVENTLISTBOX = Y_EVENTLABEL + CY_EVENTLABEL;

	static const int X_WAITLABEL = X_EVENTLISTBOX;
	static const int Y_WAITLABEL = Y_ACTIONLABEL;
#ifdef RUSSIAN_BUILD
	static const UINT CX_WAITLABEL = 165;
#else
	static const UINT CX_WAITLABEL = 130;
#endif
	static const UINT CY_LABEL = 30;
	static const UINT CY_TEXTBOX = 30;

	static const UINT CY_WAITLABEL = CY_LABEL;
	static const int X_WAIT = X_WAITLABEL;
	static const int Y_WAIT = Y_WAITLABEL + CY_WAITLABEL;
	static const UINT CX_WAIT = 60;
	static const UINT CY_WAIT = CY_TEXTBOX;

	static const int X_TEXTLABEL = X_WAITLABEL;
	static const int Y_TEXTLABEL = Y_ACTIONLABEL;
	static const UINT CX_TEXTLABEL = 130;
	static const UINT CY_TEXTLABEL = CY_LABEL;
	static const int X_TEXT = X_TEXTLABEL;
	static const int Y_TEXT = Y_TEXTLABEL + CY_TEXTLABEL;
	static const UINT CX_TEXT = CX_COMMAND_DIALOG - X_TEXT - CX_SPACE*2;
	static const UINT CY_TEXT = CY_WAIT;

	static const int X_COLOR_LABEL = X_TEXTLABEL;
	static const int Y_COLOR_LABEL = Y_TEXT + CY_WAIT + CY_SPACE;
	static const UINT CX_COLOR_LABEL = 150;
	static const UINT CY_COLOR_LABEL = CY_LABEL;
	static const int X_TEXT2 = X_COLOR_LABEL;
	static const int Y_TEXT2 = Y_COLOR_LABEL + CY_COLOR_LABEL;
	static const UINT CX_TEXT2 = 100;
	static const UINT CY_TEXT2 = CY_WAIT;

	static const int X_ADDSOUND = X_TEXTLABEL;
	static const int Y_ADDSOUND = Y_TEXT + CY_TEXT + CY_SPACE;
	static const int CX_ADDSOUND = 130;
	static const int CY_ADDSOUND = CY_STANDARD_BUTTON;
	static const int X_TESTSOUND = X_ADDSOUND + CX_ADDSOUND + CX_SPACE;
	static const int Y_TESTSOUND = Y_ADDSOUND;
	static const int CX_TESTSOUND = 80;
	static const UINT CY_TESTSOUND = CY_ADDSOUND;
	static const int X_SOUNDNAMELABEL = X_TESTSOUND + CX_TESTSOUND + CX_SPACE/2;
	static const int Y_SOUNDNAMELABEL = Y_TESTSOUND;
	static const int CX_SOUNDNAMELABEL = CX_COMMAND_DIALOG - X_SOUNDNAMELABEL - CX_SPACE;
	static const UINT CY_SOUNDNAMELABEL = CY_TEXT;

	static const int X_DELAYLABEL = X_WAITLABEL;
	static const int Y_DELAYLABEL = Y_ADDSOUND + CY_ADDSOUND + CY_SPACE;
#ifdef RUSSIAN_BUILD
	static const UINT CX_DELAYLABEL = 160;
	static const UINT CX_DELAY = 120;
#else
	static const UINT CX_DELAYLABEL = 130;
	static const UINT CX_DELAY = 90;
#endif
	static const UINT CY_DELAYLABEL = CY_WAITLABEL;
	static const int X_DELAY = X_DELAYLABEL;
	static const int Y_DELAY = Y_DELAYLABEL + CY_DELAYLABEL;
	static const UINT CY_DELAY = CY_WAIT;

#ifdef RUSSIAN_BUILD
	static const UINT CX_SPEAKERLISTBOX = 190;
#else
	static const UINT CX_SPEAKERLISTBOX = 200;
#endif
	static const int X_SPEAKERLABEL = X_DELAYLABEL + CX_DELAYLABEL + CX_SPACE;
	static const int Y_SPEAKERLABEL = Y_DELAYLABEL;
	static const UINT CX_SPEAKERLABEL = CX_SPEAKERLISTBOX;
	static const UINT CY_SPEAKERLABEL = CY_WAITLABEL;
	static const int X_SPEAKERLISTBOX = X_SPEAKERLABEL;
	static const int Y_SPEAKERLISTBOX = Y_SPEAKERLABEL + CY_SPEAKERLABEL;
	static const UINT CY_SPEAKERLISTBOX = 7*LIST_LINE_HEIGHT+4;

	static const UINT CX_MOODLISTBOX = 100;
	static const int X_MOODLABEL = X_SPEAKERLABEL + CX_SPEAKERLABEL + CX_SPACE;
	static const int Y_MOODLABEL = Y_DELAYLABEL;
	static const UINT CX_MOODLABEL = CX_MOODLISTBOX;
	static const UINT CY_MOODLABEL = CY_WAITLABEL;
	static const int X_MOODLISTBOX = X_MOODLABEL;
	static const int Y_MOODLISTBOX = Y_MOODLABEL + CY_MOODLABEL;
	static const UINT CY_MOODLISTBOX = 5*LIST_LINE_HEIGHT+4;

	static const UINT CX_BUTTON = 80;
	static const int X_OKBUTTON = (CX_COMMAND_DIALOG - (CX_BUTTON + CX_SPACE) * 2) / 2;
	static const int Y_OKBUTTON = CY_COMMAND_DIALOG - CY_STANDARD_BUTTON - CY_SPACE;
	static const int X_CANCELBUTTON = X_OKBUTTON + CX_BUTTON + CX_SPACE;
	static const int Y_CANCELBUTTON = Y_OKBUTTON;

	static const UINT CX_DIRECTIONLISTBOX = 210;
	static const UINT CY_DIRECTIONLISTBOX = 10*LIST_LINE_HEIGHT+4;
	static const int X_DIRECTIONLABEL = X_EVENTLISTBOX;
	static const int Y_DIRECTIONLABEL = Y_ACTIONLABEL;
	static const UINT CX_DIRECTIONLABEL = CX_EVENTLISTBOX;
	static const UINT CY_DIRECTIONLABEL = 30;
	static const int X_DIRECTIONLISTBOX = X_EVENTLABEL;
	static const int Y_DIRECTIONLISTBOX = Y_EVENTLABEL + CY_EVENTLABEL;

	static const int X_INPUTLABEL = X_DIRECTIONLABEL;
	static const int Y_INPUTLABEL = Y_DIRECTIONLABEL;
	static const UINT CX_INPUTLABEL = CX_DIRECTIONLABEL;
	static const UINT CY_INPUTLABEL = CY_DIRECTIONLABEL;
	static const int X_INPUTLISTBOX = X_DIRECTIONLISTBOX;
	static const int Y_INPUTLISTBOX = Y_DIRECTIONLISTBOX;
	static const UINT CX_INPUTLISTBOX = CX_DIRECTIONLISTBOX;
	static const UINT CY_INPUTLISTBOX = 12*LIST_LINE_HEIGHT+4;

	static const UINT CX_ONOFFLISTBOX = 100;
	static const UINT CY_ONOFFLISTBOX = 53;
	static const int X_ONOFFLISTBOX = X_EVENTLISTBOX;
	static const int Y_ONOFFLISTBOX = Y_ACTIONLISTBOX;

	static const int X_ONOFFLISTBOX2 = X_ONOFFLISTBOX + CX_ONOFFLISTBOX + CX_SPACE;

	static const UINT CX_STEALTHLISTBOX = 250;
	static const UINT CY_STEALTHLISTBOX = 3*LIST_LINE_HEIGHT + 4;
	static const int X_STEALTHLISTBOX = X_ONOFFLISTBOX;
	static const int Y_STEALTHLISTBOX = Y_ONOFFLISTBOX;

	static const UINT CX_WATERTRAVERSALLISTBOX = 250;
	static const UINT CY_WATERTRAVERSALLISTBOX = 4*LIST_LINE_HEIGHT + 4;
	static const int X_WATERTRAVERSALLISTBOX = X_ONOFFLISTBOX;
	static const int Y_WATERTRAVERSALLISTBOX = Y_ONOFFLISTBOX;

	static const UINT CX_IMPERATIVELISTBOX = 420;
	static const UINT CY_IMPERATIVELISTBOX = 12*LIST_LINE_HEIGHT + 4;
	static const int X_IMPERATIVELISTBOX = X_ONOFFLISTBOX;
	static const int Y_IMPERATIVELISTBOX = Y_ONOFFLISTBOX;

	static const UINT CY_BEHAVIORLISTBOX = 10 * LIST_LINE_HEIGHT + 4;

	static const UINT CX_OPENCLOSELISTBOX = 100;
	static const UINT CY_OPENCLOSELISTBOX = 53;
	static const int X_OPENCLOSELISTBOX = X_EVENTLISTBOX;
	static const int Y_OPENCLOSELISTBOX = Y_ACTIONLISTBOX;

	static const int X_GOTOLABEL = X_WAITLABEL;
	static const int Y_GOTOLABEL = Y_WAITLABEL;
	static const UINT CX_GOTOLABEL = CX_TEXTLABEL;
	static const UINT CY_GOTOLABEL = CY_WAITLABEL;
	static const int X_GOTOLABELTEXT = X_DELAYLABEL;
	static const int Y_GOTOLABELTEXT = Y_GOTOLABEL + CY_GOTOLABEL;
	static const UINT CX_GOTOLABELTEXT = CX_TEXT;
	static const UINT CY_GOTOLABELTEXT = CY_WAIT;

	static const UINT CX_GOTOLABELLISTBOX = CX_GOTOLABELTEXT;
	static const UINT CY_GOTOLABELLISTBOX = 10*LIST_LINE_HEIGHT+4;
	static const int X_GOTOLABELLISTBOX = X_EVENTLISTBOX;
	static const int Y_GOTOLABELLISTBOX = Y_GOTOLABELTEXT + CY_GOTOLABELTEXT + CY_SPACE;

	static const int X_DISPLAYSPEECHLABEL = X_WAITLABEL;
	static const int Y_DISPLAYSPEECHLABEL = Y_WAITLABEL;
	static const UINT CX_DISPLAYSPEECHLABEL = CX_WAITLABEL;
	static const UINT CY_DISPLAYSPEECHLABEL = CY_WAITLABEL;

	static const int X_NOTURNINGLABEL = X_ONOFFLISTBOX;
	static const int Y_NOTURNINGLABEL = Y_WAITLABEL;
	static const UINT CX_NOTURNINGLABEL = CX_TEXTLABEL;
	static const UINT CY_NOTURNINGLABEL = CY_WAITLABEL;

	static const int X_SINGLESTEPLABEL = X_ONOFFLISTBOX2;
	static const int Y_SINGLESTEPLABEL = Y_WAITLABEL;
	static const UINT CX_SINGLESTEPLABEL = CX_TEXTLABEL;
	static const UINT CY_SINGLESTEPLABEL = CY_WAITLABEL;

	static const int X_SKIPENTRANCE = X_ONOFFLISTBOX;
	static const int Y_SKIPENTRANCE = Y_WAITLABEL;
	static const UINT CX_SKIPENTRANCE = 200;
	static const UINT CY_SKIPENTRANCE = CY_WAITLABEL;

	static const UINT CX_MUSICLISTBOX = 450;
	static const int X_MUSICLABEL = X_DELAYLABEL;
	static const int Y_MUSICLABEL = Y_DELAYLABEL;
	static const UINT CX_MUSICLABEL = CX_MUSICLISTBOX;
	static const UINT CY_MUSICLABEL = CY_MUSICLABEL;
	static const UINT CY_MUSICLISTBOX = CY_ACTIONLISTBOX;
	static const int X_MUSICLISTBOX = X_EVENTLISTBOX;
	static const int Y_MUSICLISTBOX = Y_EVENTLISTBOX;

	static const int X_WAITFLAGSLISTBOX = X_MUSICLISTBOX;
	static const int Y_WAITFLAGSLISTBOX = Y_ONOFFLISTBOX + CY_ONOFFLISTBOX + CY_SPACE;
	static const UINT CX_WAITFLAGSLISTBOX = 100;
	static const UINT CY_WAITFLAGSLISTBOX = 9*LIST_LINE_HEIGHT+4;

	//Widgets and for variable handling commands.
	static const int X_VARTEXTLABEL = X_WAITLABEL;
	static const int Y_VARTEXTLABEL = Y_WAITLABEL;
	static const UINT CX_VARTEXTLABEL = 190;
	static const UINT CY_VARTEXTLABEL = CY_WAITLABEL;

	static const UINT CX_VARLISTBOX = 280;
	static const UINT CY_VARLISTBOX = 10*LIST_LINE_HEIGHT+4;
	static const int X_VARLISTBOX = X_EVENTLISTBOX;
	static const int Y_VARLISTBOX = Y_TEXT + CY_TEXT + CY_SPACE;

	static const int X_VARADD = X_VARLISTBOX + CX_VARLISTBOX + CX_SPACE/2;
	static const int Y_VARADD = Y_VARLISTBOX;
	static const int CX_VARADD = 90;
	static const int CY_VARADD = CY_STANDARD_BUTTON;

	static const int X_VARREMOVE = X_VARADD + CX_VARADD + CX_SPACE;
	static const int Y_VARREMOVE = Y_VARADD;
	static const int CX_VARREMOVE = 85;
	static const int CY_VARREMOVE = CY_VARADD;

	static const int X_VAROPLIST = X_VARADD;
	static const int Y_VAROPLIST = Y_VARREMOVE + CY_VARREMOVE + CY_SPACE;
	static const int CX_VAROPLIST = 100;
	static const int CY_VAROPLIST = 8*LIST_LINE_HEIGHT + 4;

	static const int X_VARCOMPLIST = X_VARADD;
	static const int Y_VARCOMPLIST = Y_VAROPLIST;
	static const int CX_VARCOMPLIST = CX_VAROPLIST;
	static const int CY_VARCOMPLIST = 6*LIST_LINE_HEIGHT + 4;

	static const int X_VARVALUELABEL = X_VAROPLIST + CX_VAROPLIST + CX_SPACE/2;
	static const UINT CY_VARVALUELABEL = CY_WAITLABEL;
	static const int Y_VARVALUELABEL = Y_VAROPLIST;
	static const UINT CX_VARVALUELABEL = 100;

	static const int X_VARVALUE = X_VARVALUELABEL;
	static const int Y_VARVALUE = Y_VARVALUELABEL + CY_VARVALUELABEL;
	static const UINT CX_VARVALUE = 90;
	static const UINT CY_VARVALUE = 30;

	//Others
	static const int X_GRAPHICLISTBOX2 = X_EVENTLISTBOX;
	static const int Y_GRAPHICLISTBOX2 = Y_ACTIONLISTBOX;
	static const UINT CX_GRAPHICLISTBOX2 = 250;
	static const UINT CY_GRAPHICLISTBOX2 = CY_ACTIONLISTBOX;

	static const int ONOFFLISTBOX4_X = X_GRAPHICLISTBOX2 + CX_GRAPHICLISTBOX2 + CX_SPACE;

	static const int X_GLOBALSCRIPTLISTBOX = X_GRAPHICLISTBOX2;
	static const int Y_GLOBALSCRIPTLISTBOX = Y_GRAPHICLISTBOX2;
	static const UINT CX_GLOBALSCRIPTLISTBOX = CX_GRAPHICLISTBOX2;
	static const UINT CY_GLOBALSCRIPTLISTBOX = CY_GRAPHICLISTBOX2;

	static const UINT CX_DIRECTIONLISTBOX2 = CX_DIRECTIONLISTBOX;
	static const UINT CY_DIRECTIONLISTBOX2 = 8*LIST_LINE_HEIGHT+4;
	static const int X_DIRECTIONLABEL2 = X_GRAPHICLISTBOX2 + CX_GRAPHICLISTBOX2 + CX_SPACE;
	static const int Y_DIRECTIONLABEL2 = Y_DIRECTIONLABEL;
	static const UINT CX_DIRECTIONLABEL2 = CX_DIRECTIONLABEL;
	static const UINT CY_DIRECTIONLABEL2 = 30;
	static const int X_DIRECTIONLISTBOX2 = X_DIRECTIONLABEL2;
	static const int Y_DIRECTIONLISTBOX2 = Y_DIRECTIONLABEL2 + CY_DIRECTIONLABEL2;

	static const int X_DIRECTIONLISTBOX3 = X_DIRECTIONLISTBOX;
	static const int Y_DIRECTIONLISTBOX3 = Y_DIRECTIONLISTBOX;
	static const UINT CX_DIRECTIONLISTBOX3 = CX_DIRECTIONLISTBOX2;
	static const UINT CY_DIRECTIONLISTBOX3 = 9*LIST_LINE_HEIGHT + 4;

	static const int X_EFFECTLISTBOX = X_DIRECTIONLISTBOX3 + CX_DIRECTIONLISTBOX3 + CX_SPACE;
	static const int Y_EFFECTLISTBOX = Y_GRAPHICLISTBOX2;
	static const UINT CX_EFFECTLISTBOX = 200;
	static const UINT CY_EFFECTLISTBOX = CY_ACTIONLISTBOX;

	static const int X_SOUNDEFFECTLABEL = X_DIRECTIONLISTBOX3;
	static const int Y_SOUNDEFFECTLABEL = Y_DIRECTIONLISTBOX3 + CY_DIRECTIONLISTBOX3 + CY_SPACE/2;
	static const UINT CX_SOUNDEFFECTLABEL = CX_DIRECTIONLISTBOX3;
	static const UINT CY_SOUNDEFFECTLABEL = CY_WAITLABEL;

	static const int X_ONOFFLISTBOX3 = X_ONOFFLISTBOX;
	static const int Y_ONOFFLISTBOX3 = Y_DIRECTIONLISTBOX3 + CY_DIRECTIONLISTBOX3 + CY_SPACE/2 + CY_WAITLABEL;

	static const int X_ITEMLISTBOX = X_GRAPHICLISTBOX2;
	static const int Y_ITEMLISTBOX = Y_GRAPHICLISTBOX2;
	static const UINT CX_ITEMLISTBOX = 420;
	static const UINT CY_ITEMLISTBOX = CY_GRAPHICLISTBOX2;

	static const int X_CUTSCENELABEL = X_WAITLABEL;
	static const int Y_CUTSCENELABEL = Y_WAITLABEL;
	static const UINT CX_CUTSCENELABEL = 200;
	static const UINT CY_CUTSCENELABEL = CY_WAITLABEL;

	static const int X_MOVERELXLABEL = X_SINGLESTEPLABEL + CX_SINGLESTEPLABEL;
	static const int Y_MOVERELXLABEL = Y_WAITLABEL;
	static const UINT CX_MOVERELXLABEL = CX_TEXTLABEL;
	static const UINT CY_MOVERELXLABEL = CY_WAITLABEL;

	static const int X_MOVERELX = X_MOVERELXLABEL;
	static const int Y_MOVERELX = Y_MOVERELXLABEL + CY_MOVERELXLABEL;
	static const UINT CX_MOVERELX = 60;
	static const UINT CY_MOVERELX = CY_WAIT;

	static const int X_MOVERELYLABEL = X_MOVERELXLABEL + CX_MOVERELXLABEL;
	static const int Y_MOVERELYLABEL = Y_WAITLABEL;
	static const UINT CX_MOVERELYLABEL = CX_TEXTLABEL;
	static const UINT CY_MOVERELYLABEL = CY_WAITLABEL;

	static const int X_MOVERELY = X_MOVERELYLABEL;
	static const int Y_MOVERELY = Y_MOVERELYLABEL + CY_MOVERELYLABEL;
	static const UINT CX_MOVERELY = CX_MOVERELX;
	static const UINT CY_MOVERELY = CY_MOVERELX;

	static const int X_LOOPSOUND = X_WAITLABEL;
	static const int Y_LOOPSOUND = Y_WAITLABEL;
	static const UINT CX_LOOPSOUND = 200;
	static const UINT CY_LOOPSOUND = CY_WAITLABEL;

	static const int X_DISPLAYFILTER_LISTBOX = X_EVENTLISTBOX;
	static const int Y_DISPLAYFILTER_LISTBOX = Y_ACTIONLISTBOX;
	static const UINT CX_DISPLAYFILTER_LISTBOX = 120;
	static const UINT CY_DISPLAYFILTER_LISTBOX = 4*LIST_LINE_HEIGHT + 4;

	static const UINT CX_WEAPON_LISTBOX = 120;
	static const UINT CY_WEAPON_LISTBOX = 8*LIST_LINE_HEIGHT + 4;
	static const int X_WEAPON_LISTBOX = X_EVENTLISTBOX;
	static const int Y_WEAPON_LISTBOX = Y_ACTIONLISTBOX;

	static const int X_ATTACKTILE_LISTBOX = X_EVENTLISTBOX;
	static const int Y_ATTACKTILE_LISTBOX = Y_ACTIONLISTBOX;
	static const UINT CX_ATTACKTILE_LISTBOX = 120;
	static const UINT CY_ATTACKTILE_LISTBOX = 4*LIST_LINE_HEIGHT + 4;

	static const UINT MOVETYPELISTBOX_X = X_IMPERATIVELISTBOX;
	static const UINT MOVETYPELISTBOX_Y = Y_IMPERATIVELISTBOX;
	static const UINT MOVETYPELISTBOX_CX = 220;
	static const UINT MOVETYPELISTBOX_CY = 5 * LIST_LINE_HEIGHT + 4;

	//World map input.
	static const int X_XCOORDLABEL = X_GRAPHICLISTBOX2 + CX_GRAPHICLISTBOX2 + CX_SPACE;
	static const int Y_XCOORDLABEL = Y_WAITLABEL;
	static const UINT CX_XCOORDLABEL = 100;
	static const UINT CY_XCOORDLABEL = CY_WAITLABEL;

	static const int X_XCOORD = X_XCOORDLABEL;
	static const int Y_XCOORD = Y_XCOORDLABEL + CY_XCOORDLABEL;
	static const UINT CX_XCOORD = 60;
	static const UINT CY_XCOORD = CY_WAIT;

	static const int X_YCOORDLABEL = X_XCOORDLABEL + CX_XCOORDLABEL;
	static const int Y_YCOORDLABEL = Y_XCOORDLABEL;
	static const UINT CX_YCOORDLABEL = CX_XCOORDLABEL;
	static const UINT CY_YCOORDLABEL = CY_XCOORDLABEL;

	static const int X_YCOORD = X_YCOORDLABEL;
	static const int Y_YCOORD = Y_YCOORDLABEL + CY_YCOORDLABEL;
	static const UINT CX_YCOORD = CX_XCOORD;
	static const UINT CY_YCOORD = CY_XCOORD;

	static const int X_ICONDISPLAY_LISTBOX = X_XCOORD;
	static const int Y_ICONDISPLAY_LISTBOX = Y_XCOORD + CY_XCOORD + CY_SPACE;
	static const UINT CX_ICONDISPLAY_LISTBOX = 120;
	static const UINT CY_ICONDISPLAY_LISTBOX = 6*LIST_LINE_HEIGHT + 4;
	static const UINT CY_IMAGEDISPLAY_LISTBOX = 5*LIST_LINE_HEIGHT + 4;

	//Image overlay
	static const int X_IMAGELABEL = X_WAITLABEL;
	static const int Y_IMAGELABEL = Y_WAITLABEL;
	static const UINT CX_IMAGELABEL = 200;
	static const UINT CY_IMAGELABEL = CY_LABEL;

	static const int X_IMAGEOVERLAYTEXT = X_IMAGELABEL;
	static const int Y_IMAGEOVERLAYTEXT = Y_IMAGELABEL + CY_IMAGELABEL;
	static const UINT CX_IMAGEOVERLAYTEXT = CX_TEXT;
	static const UINT CY_IMAGEOVERLAYTEXT = 284;

	ASSERT(!this->pAddCommandDialog);

	this->pAddCommandDialog = new CRenameDialogWidget(0L, 0, 0,
			CX_COMMAND_DIALOG, CY_COMMAND_DIALOG);
	this->pAddCommandDialog->AddWidget(new CLabelWidget(0L, X_TITLE, Y_TITLE,
			CX_TITLE, CY_TITLE, F_Header, g_pTheDB->GetMessageText(MID_AddScriptCommand)));

	//Action list.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(0L, X_ACTIONLABEL, Y_ACTIONLABEL,
			CX_ACTIONLABEL, CY_ACTIONLABEL, F_Small, g_pTheDB->GetMessageText(MID_Action)));
	this->pActionListBox = new CListBoxWidget(TAG_ACTIONLISTBOX,
			X_ACTIONLISTBOX, Y_ACTIONLISTBOX, CX_ACTIONLISTBOX, CY_ACTIONLISTBOX);
	this->pActionListBox->SetHotkeyItemSelection(true);
	this->pActionListBox->SortAlphabetically(true);
	this->pAddCommandDialog->AddWidget(this->pActionListBox);
	PopulateCommandListBox();

	//Event checking.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_EVENTLABEL, X_EVENTLABEL, Y_EVENTLABEL,
			CX_EVENTLABEL, CY_EVENTLABEL, F_Small, g_pTheDB->GetMessageText(MID_Event)));
	this->pEventListBox = new CListBoxWidget(TAG_EVENTLISTBOX,
			X_EVENTLISTBOX, Y_EVENTLISTBOX, CX_EVENTLISTBOX, CY_EVENTLISTBOX, true);
	this->pAddCommandDialog->AddWidget(this->pEventListBox);
	this->pEventListBox->SetHotkeyItemSelection(true);
	this->pEventListBox->SortAlphabetically(true);
	PopulateEventListBox();

	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_NOTURNING, X_NOTURNINGLABEL,
			Y_NOTURNINGLABEL, CX_NOTURNINGLABEL, CY_NOTURNINGLABEL, F_Small,
			g_pTheDB->GetMessageText(MID_NoTurning)));

	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_SINGLESTEP, X_SINGLESTEPLABEL,
		Y_SINGLESTEPLABEL, CX_SINGLESTEPLABEL, CY_SINGLESTEPLABEL, F_Small,
		g_pTheDB->GetMessageText(MID_SingleStep)));

	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_SINGLESTEP2, X_NOTURNINGLABEL,
		Y_NOTURNINGLABEL, CX_NOTURNINGLABEL, CY_NOTURNINGLABEL, F_Small,
		g_pTheDB->GetMessageText(MID_SingleStep)));

	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_SKIPENTRANCELABEL, X_SKIPENTRANCE,
			Y_SKIPENTRANCE, CX_SKIPENTRANCE, CY_SKIPENTRANCE, F_Small,
			g_pTheDB->GetMessageText(MID_SkipEntranceDisplay)));

	//Wait # of turns.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_WAITLABEL, X_WAITLABEL,
			Y_WAITLABEL, CX_WAITLABEL, CY_WAITLABEL, F_Small, g_pTheDB->GetMessageText(MID_TurnsToWait)));
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_WAITABSLABEL, X_WAITLABEL,
			Y_WAITLABEL, CX_WAITLABEL, CY_WAITLABEL, F_Small, g_pTheDB->GetMessageText(MID_WaitUntilTurn)));
	CTextBoxWidget *pWait = new CTextBoxWidget(TAG_WAIT, X_WAIT, Y_WAIT,
			CX_WAIT, CY_WAIT, 4);
	pWait->SetDigitsOnly(true);
	pWait->SetText(wszZero);
	this->pAddCommandDialog->AddWidget(pWait);

	//Speech dialog.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_TEXTLABEL, X_TEXTLABEL,
			Y_TEXTLABEL, CX_TEXTLABEL, CY_TEXTLABEL, F_Small, g_pTheDB->GetMessageText(MID_DialogText)));
	CTextBoxWidget *pDialogue = new CTextBoxWidget(TAG_SPEECHTEXT, X_TEXT, Y_TEXT,
			CX_TEXT, CY_TEXT, 1024);
	this->pAddCommandDialog->AddWidget(pDialogue);

	//Sound clip.
	CButtonWidget *pAddSoundButton = new CButtonWidget(TAG_ADDSOUND, X_ADDSOUND,
			Y_ADDSOUND, CX_ADDSOUND, CY_ADDSOUND, g_pTheDB->GetMessageText(MID_AddSound));
	this->pAddCommandDialog->AddWidget(pAddSoundButton);
	CButtonWidget *pTestSoundButton = new CButtonWidget(TAG_TESTSOUND, X_TESTSOUND,
			Y_TESTSOUND, CX_TESTSOUND, CY_TESTSOUND, g_pTheDB->GetMessageText(MID_TestSound));
	this->pAddCommandDialog->AddWidget(pTestSoundButton);
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_SOUNDNAME_LABEL, X_SOUNDNAMELABEL,
			Y_SOUNDNAMELABEL, CX_SOUNDNAMELABEL, CY_SOUNDNAMELABEL, F_Small, wszEmpty));

	//Speech delay.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_DELAYLABEL, X_DELAYLABEL,
			Y_DELAYLABEL, CX_DELAYLABEL, CY_DELAYLABEL, F_Small, g_pTheDB->GetMessageText(MID_SpeechDelay)));
	CTextBoxWidget *pDelay = new CTextBoxWidget(TAG_DELAY, X_DELAY, Y_DELAY,
			CX_DELAY, CY_DELAY, 7);
	pDelay->SetDigitsOnly(true);
	pDelay->SetText(wszZero);
	this->pAddCommandDialog->AddWidget(pDelay);

	//Speaker.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_SPEAKERLABEL, X_SPEAKERLABEL, Y_SPEAKERLABEL,
			CX_SPEAKERLABEL, CY_SPEAKERLABEL, F_Small, g_pTheDB->GetMessageText(MID_Speaker)));
	this->pSpeakerListBox = new CListBoxWidget(TAG_SPEAKERLISTBOX,
			X_SPEAKERLISTBOX, Y_SPEAKERLISTBOX, CX_SPEAKERLISTBOX, CY_SPEAKERLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pSpeakerListBox);

	//Music dialog.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_MUSICLABEL, X_MUSICLABEL,
			Y_MUSICLABEL, CX_MUSICLABEL, CY_MUSICLABEL, F_Small, g_pTheDB->GetMessageText(MID_MusicType)));
	this->pMusicListBox = new CListBoxWidget(TAG_MUSICLISTBOX,
			X_MUSICLISTBOX, Y_MUSICLISTBOX, CX_MUSICLISTBOX, CY_MUSICLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pMusicListBox);
	this->pMusicListBox->AddItem(static_cast<UINT>(SONGID_DEFAULT), g_pTheDB->GetMessageText(MID_MusicDefault));
	this->pMusicListBox->AddItem(static_cast<UINT>(SONGID_CUSTOM), g_pTheDB->GetMessageText(MID_MusicCustom));
	this->pMusicListBox->AddItem(SONGID_NONE, g_pTheDB->GetMessageText(MID_MusicQuiet));
	this->pMusicListBox->AddItem(SONGID_INTRO_KDD, g_pTheDB->GetMessageText(MID_MusicTitle_KDD));
	this->pMusicListBox->AddItem(SONGID_INTRO_JTRH, g_pTheDB->GetMessageText(MID_MusicTitle_JTRH));
	this->pMusicListBox->AddItem(SONGID_INTRO_TCB, g_pTheDB->GetMessageText(MID_MusicTitle_TCB));
	this->pMusicListBox->AddItem(SONGID_INTRO_GATEB, g_pTheDB->GetMessageText(MID_MusicTitle_GATEB));
	this->pMusicListBox->AddItem(SONGID_INTRO_TSS, g_pTheDB->GetMessageText(MID_MusicTitle_TSS));
	this->pMusicListBox->AddItem(SONGID_WINGAME_TCB, g_pTheDB->GetMessageText(MID_MusicWinGame));
	this->pMusicListBox->AddItem(SONGID_ENDOFTHEGAME_JTRH, g_pTheDB->GetMessageText(MID_MusicFinale_JTRH));
	this->pMusicListBox->AddItem(SONGID_ENDOFTHEGAME_TCB, g_pTheDB->GetMessageText(MID_MusicFinale_TCB));
	this->pMusicListBox->AddItem(SONGID_CREDITS_KDD, g_pTheDB->GetMessageText(MID_MusicCredits_KDD));
	this->pMusicListBox->AddItem(SONGID_CREDITS_JTRH, g_pTheDB->GetMessageText(MID_MusicCredits_JTRH));
	this->pMusicListBox->AddItem(SONGID_CREDITS_TCB, g_pTheDB->GetMessageText(MID_MusicCredits_TCB));
	this->pMusicListBox->AddItem(SONGID_CREDITS_GATEB, g_pTheDB->GetMessageText(MID_MusicCredits_GATEB));
	this->pMusicListBox->AddItem(SONGID_CREDITS_TSS, g_pTheDB->GetMessageText(MID_MusicCredits_TSS));
	this->pMusicListBox->AddItem(SONGID_QUIT_KDD, g_pTheDB->GetMessageText(MID_MusicQuit_KDD));
	this->pMusicListBox->AddItem(SONGID_QUIT_JTRH, g_pTheDB->GetMessageText(MID_MusicQuit_JTRH));
	this->pMusicListBox->AddItem(SONGID_QUIT_TCB, g_pTheDB->GetMessageText(MID_MusicQuit_TCB));
	this->pMusicListBox->AddItem(SONGID_QUIT_GATEB, g_pTheDB->GetMessageText(MID_MusicQuit_GATEB));
	this->pMusicListBox->AddItem(SONGID_QUIT_TSS, g_pTheDB->GetMessageText(MID_MusicQuit_TSS));

	CFiles f;
	list<WSTRING> styles;
	if (f.GetGameProfileString(INISection::Graphics, INIKey::Style, styles))
	{
		UINT wCount = SONGID_COUNT;
		for (list<WSTRING>::iterator style = styles.begin(); style != styles.end(); ++style)
		{
			for (UINT mood=0; mood<SONG_MOOD_COUNT; ++mood)
			{
				WSTRING wstrMoodText;
				UTF8ToUnicode(moodText[mood], wstrMoodText);
				WSTRING wstr = *style + wstrMoodText;
				this->pMusicListBox->AddItem(wCount++, wstr.c_str());
			}
		}
	}
	this->pMusicListBox->SelectLine(0);

	//Moods.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_MOODLABEL, X_MOODLABEL, Y_MOODLABEL,
			CX_MOODLABEL, CY_MOODLABEL, F_Small, g_pTheDB->GetMessageText(MID_Mood)));
	this->pMoodListBox = new CListBoxWidget(TAG_MOODLISTBOX,
			X_MOODLISTBOX, Y_MOODLISTBOX, CX_MOODLISTBOX, CY_MOODLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pMoodListBox);
	this->pMoodListBox->AddItem(Mood_Normal, g_pTheDB->GetMessageText(MID_Normal));
	this->pMoodListBox->AddItem(Mood_Happy, g_pTheDB->GetMessageText(MID_Happy));
	this->pMoodListBox->AddItem(Mood_Aggressive, g_pTheDB->GetMessageText(MID_Aggressive));
	this->pMoodListBox->AddItem(Mood_Nervous, g_pTheDB->GetMessageText(MID_Nervous));
	this->pMoodListBox->AddItem(Mood_Strike, g_pTheDB->GetMessageText(MID_Striking));
	this->pMoodListBox->SelectLine(0);

	//Direction list box.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_DIRECTIONLABEL, X_DIRECTIONLABEL, Y_DIRECTIONLABEL,
			CX_DIRECTIONLABEL, CY_DIRECTIONLABEL, F_Small, g_pTheDB->GetMessageText(MID_ChooseDirection)));
	this->pDirectionListBox = new CListBoxWidget(TAG_DIRECTIONLISTBOX,
			X_DIRECTIONLISTBOX, Y_DIRECTIONLISTBOX, CX_DIRECTIONLISTBOX, CY_DIRECTIONLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pDirectionListBox);
	this->pDirectionListBox->AddItem(NW, g_pTheDB->GetMessageText(MID_NorthWest));
	this->pDirectionListBox->AddItem(N, g_pTheDB->GetMessageText(MID_North));
	this->pDirectionListBox->AddItem(NE, g_pTheDB->GetMessageText(MID_NorthEast));
	this->pDirectionListBox->AddItem(W, g_pTheDB->GetMessageText(MID_West));
	this->pDirectionListBox->AddItem(E, g_pTheDB->GetMessageText(MID_East));
	this->pDirectionListBox->AddItem(SW, g_pTheDB->GetMessageText(MID_SouthWest));
	this->pDirectionListBox->AddItem(S, g_pTheDB->GetMessageText(MID_South));
	this->pDirectionListBox->AddItem(SE, g_pTheDB->GetMessageText(MID_SouthEast));
	this->pDirectionListBox->AddItem(CMD_C, g_pTheDB->GetMessageText(MID_Clockwise));
	this->pDirectionListBox->AddItem(CMD_CC, g_pTheDB->GetMessageText(MID_CounterClockwise));
	this->pDirectionListBox->SelectLine(0);

	//Input list box.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_INPUTLABEL, X_INPUTLABEL, Y_INPUTLABEL,
			CX_INPUTLABEL, CY_INPUTLABEL, F_Small, g_pTheDB->GetMessageText(MID_ChooseInput)));
	this->pInputListBox = new CListBoxWidget(TAG_INPUTLISTBOX,
			X_INPUTLISTBOX, Y_INPUTLISTBOX, CX_INPUTLISTBOX, CY_INPUTLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pInputListBox);
	this->pInputListBox->AddItem(CMD_NW, g_pTheDB->GetMessageText(MID_NorthWest));
	this->pInputListBox->AddItem(CMD_N, g_pTheDB->GetMessageText(MID_North));
	this->pInputListBox->AddItem(CMD_NE, g_pTheDB->GetMessageText(MID_NorthEast));
	this->pInputListBox->AddItem(CMD_W, g_pTheDB->GetMessageText(MID_West));
	this->pInputListBox->AddItem(CMD_WAIT, g_pTheDB->GetMessageText(MID_Wait));
	this->pInputListBox->AddItem(CMD_E, g_pTheDB->GetMessageText(MID_East));
	this->pInputListBox->AddItem(CMD_SW, g_pTheDB->GetMessageText(MID_SouthWest));
	this->pInputListBox->AddItem(CMD_S, g_pTheDB->GetMessageText(MID_South));
	this->pInputListBox->AddItem(CMD_SE, g_pTheDB->GetMessageText(MID_SouthEast));
	this->pInputListBox->AddItem(CMD_C, g_pTheDB->GetMessageText(MID_Clockwise));
	this->pInputListBox->AddItem(CMD_CC, g_pTheDB->GetMessageText(MID_CounterClockwise));
	this->pInputListBox->AddItem(CMD_EXEC_COMMAND, g_pTheDB->GetMessageText(MID_UseCommandKey));
	this->pInputListBox->SelectLine(0);

	//Direction list box #2.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_DIRECTIONLABEL2, X_DIRECTIONLABEL2, Y_DIRECTIONLABEL2,
			CX_DIRECTIONLABEL2, CY_DIRECTIONLABEL2, F_Small, g_pTheDB->GetMessageText(MID_ChooseDirection)));
	this->pDirectionListBox2 = new CListBoxWidget(TAG_DIRECTIONLISTBOX2,
			X_DIRECTIONLISTBOX2, Y_DIRECTIONLISTBOX2, CX_DIRECTIONLISTBOX2, CY_DIRECTIONLISTBOX2);
	this->pAddCommandDialog->AddWidget(this->pDirectionListBox2);
	this->pDirectionListBox2->AddItem(NW, g_pTheDB->GetMessageText(MID_NorthWest));
	this->pDirectionListBox2->AddItem(N, g_pTheDB->GetMessageText(MID_North));
	this->pDirectionListBox2->AddItem(NE, g_pTheDB->GetMessageText(MID_NorthEast));
	this->pDirectionListBox2->AddItem(W, g_pTheDB->GetMessageText(MID_West));
	this->pDirectionListBox2->AddItem(E, g_pTheDB->GetMessageText(MID_East));
	this->pDirectionListBox2->AddItem(SW, g_pTheDB->GetMessageText(MID_SouthWest));
	this->pDirectionListBox2->AddItem(S, g_pTheDB->GetMessageText(MID_South));
	this->pDirectionListBox2->AddItem(SE, g_pTheDB->GetMessageText(MID_SouthEast));
	this->pDirectionListBox2->SelectLine(0);

	//Direction list box #3.
	this->pDirectionListBox3 = new CListBoxWidget(TAG_DIRECTIONLISTBOX3,
			X_DIRECTIONLISTBOX3, Y_DIRECTIONLISTBOX3, CX_DIRECTIONLISTBOX3, CY_DIRECTIONLISTBOX3);
	this->pAddCommandDialog->AddWidget(this->pDirectionListBox3);
	this->pDirectionListBox3->AddItem(NW, g_pTheDB->GetMessageText(MID_NorthWest));
	this->pDirectionListBox3->AddItem(N, g_pTheDB->GetMessageText(MID_North));
	this->pDirectionListBox3->AddItem(NE, g_pTheDB->GetMessageText(MID_NorthEast));
	this->pDirectionListBox3->AddItem(W, g_pTheDB->GetMessageText(MID_West));
	this->pDirectionListBox3->AddItem(NO_ORIENTATION, g_pTheDB->GetMessageText(MID_Center));
	this->pDirectionListBox3->AddItem(E, g_pTheDB->GetMessageText(MID_East));
	this->pDirectionListBox3->AddItem(SW, g_pTheDB->GetMessageText(MID_SouthWest));
	this->pDirectionListBox3->AddItem(S, g_pTheDB->GetMessageText(MID_South));
	this->pDirectionListBox3->AddItem(SE, g_pTheDB->GetMessageText(MID_SouthEast));
	this->pDirectionListBox3->SelectItem(NO_ORIENTATION);

	this->pOnOffListBox3 = new CListBoxWidget(TAG_ONOFFLISTBOX3,
			X_ONOFFLISTBOX3, Y_ONOFFLISTBOX3, CX_ONOFFLISTBOX, CY_ONOFFLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pOnOffListBox3);
	this->pOnOffListBox3->AddItem(0, g_pTheDB->GetMessageText(MID_Off));
	this->pOnOffListBox3->AddItem(1, g_pTheDB->GetMessageText(MID_On));
	this->pOnOffListBox3->SelectLine(0);

	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_KEEPBEHAVIOR_LABEL, ONOFFLISTBOX4_X, Y_DIRECTIONLABEL,
		CX_DIRECTIONLABEL, CY_DIRECTIONLABEL, F_Small, g_pTheDB->GetMessageText(MID_KeepBehaviors)));
	this->pOnOffListBox4 = new CListBoxWidget(TAG_ONOFFLISTBOX4,
		ONOFFLISTBOX4_X, Y_ONOFFLISTBOX, CX_ONOFFLISTBOX, CY_ONOFFLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pOnOffListBox4);
	this->pOnOffListBox4->AddItem(0, g_pTheDB->GetMessageText(MID_Off));
	this->pOnOffListBox4->AddItem(1, g_pTheDB->GetMessageText(MID_On));
	this->pOnOffListBox4->SelectLine(0);

	//Stealth list box.
	this->pStealthListBox = new CListBoxWidget(TAG_STEALTHLISTBOX,
			X_STEALTHLISTBOX, Y_STEALTHLISTBOX, CX_STEALTHLISTBOX, CY_STEALTHLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pStealthListBox);
	this->pStealthListBox->AddItem(Stealth_Default, g_pTheDB->GetMessageText(MID_Default));
	this->pStealthListBox->AddItem(Stealth_On, g_pTheDB->GetMessageText(MID_On));
	this->pStealthListBox->AddItem(Stealth_Off, g_pTheDB->GetMessageText(MID_Off));
	this->pStealthListBox->SelectLine(0);

	//Shallow Water traversal list box.
	this->pWaterTraversalListBox = new CListBoxWidget(TAG_WATERTRAVERSALLISTBOX,
			X_WATERTRAVERSALLISTBOX, Y_WATERTRAVERSALLISTBOX, CX_WATERTRAVERSALLISTBOX, CY_WATERTRAVERSALLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pWaterTraversalListBox);
	this->pWaterTraversalListBox->AddItem(WTrv_AsPlayerRole, g_pTheDB->GetMessageText(MID_WT_PlayerRole));
	this->pWaterTraversalListBox->AddItem(WTrv_NoEntry, g_pTheDB->GetMessageText(MID_WT_NoEntry));
	this->pWaterTraversalListBox->AddItem(WTrv_CanWade, g_pTheDB->GetMessageText(MID_WT_CanWade));
	this->pWaterTraversalListBox->AddItem(WTrv_CanHide, g_pTheDB->GetMessageText(MID_WT_CanHide));
	this->pWaterTraversalListBox->SelectLine(0);

	//Visual effects.
	this->pVisualEffectsListBox = new CListBoxWidget(TAG_VISUALEFFECTS_LISTBOX,
			X_EFFECTLISTBOX, Y_EFFECTLISTBOX, CX_EFFECTLISTBOX, CY_EFFECTLISTBOX, true);
	this->pAddCommandDialog->AddWidget(this->pVisualEffectsListBox);
	this->pVisualEffectsListBox->SetHotkeyItemSelection(true);
	this->pVisualEffectsListBox->SortAlphabetically(true);
	this->pVisualEffectsListBox->AddItem(VET_BLOODSPLAT, g_pTheDB->GetMessageText(MID_BloodSplatterEffect));
	this->pVisualEffectsListBox->AddItem(VET_MUDSPLAT, g_pTheDB->GetMessageText(MID_MudSplatterEffect));
	this->pVisualEffectsListBox->AddItem(VET_TARSPLAT, g_pTheDB->GetMessageText(MID_TarSplatterEffect));
	this->pVisualEffectsListBox->AddItem(VET_GELSPLAT, g_pTheDB->GetMessageText(MID_GelSplatterEffect));
	this->pVisualEffectsListBox->AddItem(VET_SEEPSPLAT, g_pTheDB->GetMessageText(MID_SeepSplatterEffect));
	this->pVisualEffectsListBox->AddItem(VET_GOLEMSPLAT, g_pTheDB->GetMessageText(MID_GolemSplatterEffect));
	this->pVisualEffectsListBox->AddItem(VET_SLAYERSPLAT, g_pTheDB->GetMessageText(MID_SlayerSplatterEffect));
	this->pVisualEffectsListBox->AddItem(VET_DEBRIS, g_pTheDB->GetMessageText(MID_DebrisEffect));
	this->pVisualEffectsListBox->AddItem(VET_SPARKS, g_pTheDB->GetMessageText(MID_SparkEffect));
	this->pVisualEffectsListBox->AddItem(VET_EXPLOSION, g_pTheDB->GetMessageText(MID_ExplosionEffect));
	this->pVisualEffectsListBox->AddItem(VET_SPLASH, g_pTheDB->GetMessageText(MID_SplashEffect));
	this->pVisualEffectsListBox->AddItem(VET_STEAM, g_pTheDB->GetMessageText(MID_SteamEffect));
	this->pVisualEffectsListBox->AddItem(VET_SWIRL, g_pTheDB->GetMessageText(MID_SwirlEffect));
	this->pVisualEffectsListBox->AddItem(VET_VERMIN, g_pTheDB->GetMessageText(MID_VerminEffect));
	this->pVisualEffectsListBox->AddItem(VET_BOLT, g_pTheDB->GetMessageText(MID_BoltEffect));
	this->pVisualEffectsListBox->AddItem(VET_JITTER, g_pTheDB->GetMessageText(MID_JitterEffect));
	this->pVisualEffectsListBox->AddItem(VET_SPIKES, g_pTheDB->GetMessageText(MID_FloorSpikes));
	this->pVisualEffectsListBox->AddItem(VET_FIRETRAP, g_pTheDB->GetMessageText(MID_Firetrap));
	this->pVisualEffectsListBox->AddItem(VET_PUFFEXPLOSION, g_pTheDB->GetMessageText(MID_PuffExplosionEffect));
	this->pVisualEffectsListBox->AddItem(VET_PUFFSPLAT, g_pTheDB->GetMessageText(MID_PuffMergedIntoFluff));
	this->pVisualEffectsListBox->AddItem(VET_ICEMELT, g_pTheDB->GetMessageText(MID_IceMeltEffect));
	this->pVisualEffectsListBox->SelectLine(0);

	this->pWaitFlagsListBox = new CListBoxWidget(TAG_WAITFLAGSLISTBOX,
			X_WAITFLAGSLISTBOX, Y_WAITFLAGSLISTBOX, CX_WAITFLAGSLISTBOX, CY_WAITFLAGSLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pWaitFlagsListBox);
	this->pWaitFlagsListBox->SelectMultipleItems(true);
	this->pWaitFlagsListBox->AddItem(ScriptFlag::PLAYER, g_pTheDB->GetMessageText(MID_Player));
	this->pWaitFlagsListBox->AddItem(ScriptFlag::HALPH, g_pTheDB->GetMessageText(MID_Halph2));
	this->pWaitFlagsListBox->AddItem(ScriptFlag::MONSTER, g_pTheDB->GetMessageText(MID_Monster));
	this->pWaitFlagsListBox->AddItem(ScriptFlag::NPC, g_pTheDB->GetMessageText(MID_NPC));
	this->pWaitFlagsListBox->AddItem(ScriptFlag::PDOUBLE, g_pTheDB->GetMessageText(MID_PlayerDouble));
	this->pWaitFlagsListBox->AddItem(ScriptFlag::SELF, g_pTheDB->GetMessageText(MID_Self));
	this->pWaitFlagsListBox->AddItem(ScriptFlag::SLAYER, g_pTheDB->GetMessageText(MID_Slayer2));
	this->pWaitFlagsListBox->AddItem(ScriptFlag::BEETHRO, g_pTheDB->GetMessageText(MID_Beethro));
	this->pWaitFlagsListBox->AddItem(ScriptFlag::STALWART, g_pTheDB->GetMessageText(MID_Stalwart));
	this->pWaitFlagsListBox->SelectLine(0);

	this->pBuildItemsListBox = new CListBoxWidget(TAG_BUILDITEMLISTBOX,
		X_ITEMLISTBOX, Y_ITEMLISTBOX, CX_ITEMLISTBOX, CY_ITEMLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pBuildItemsListBox);
	this->pBuildItemsListBox->SortAlphabetically(true);
	this->pBuildItemsListBox->SetHotkeyItemSelection(true);
	this->PopulateItemListBox(this->pBuildItemsListBox, true, false, false);
	this->pBuildItemsListBox->SelectLine(0);

	this->pBuildMarkerListBox = new CListBoxWidget(TAG_BUILDMARKERITEMLISTBOX,
		X_ITEMLISTBOX, Y_ITEMLISTBOX, CX_ITEMLISTBOX, CY_ITEMLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pBuildMarkerListBox);
	this->pBuildMarkerListBox->SortAlphabetically(true);
	this->pBuildMarkerListBox->SetHotkeyItemSelection(true);
	this->PopulateItemListBox(this->pBuildMarkerListBox, false, true, false);
	this->pBuildMarkerListBox->SelectLine(0);

	this->pNaturalTargetTypesListBox = new CListBoxWidget(TAG_NATURAL_TARGET_TYPES,
		X_ITEMLISTBOX, Y_ITEMLISTBOX, CX_ITEMLISTBOX, CY_ITEMLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pNaturalTargetTypesListBox);
	this->pNaturalTargetTypesListBox->SortAlphabetically(true);
	this->pNaturalTargetTypesListBox->SetHotkeyItemSelection(true);
	this->pNaturalTargetTypesListBox->AddItem(ScriptFlag::RegularMonster, g_pTheDB->GetMessageText(MID_TargetRegularMonster));
	// @TODO: These four are commented out because they are not yet implemented because that would require heavy refactoring
	//this->pNaturalTargetTypesListBox->AddItem(ScriptFlag::BrainedMonster, g_pTheDB->GetMessageText(MID_TargetBrainedMonster));
	//this->pNaturalTargetTypesListBox->AddItem(ScriptFlag::Guard, g_pTheDB->GetMessageText(MID_Guard));
	//this->pNaturalTargetTypesListBox->AddItem(ScriptFlag::Puff, g_pTheDB->GetMessageText(MID_FluffBaby));
	//this->pNaturalTargetTypesListBox->AddItem(ScriptFlag::Stalwart, g_pTheDB->GetMessageText(MID_Stalwart));
	this->pNaturalTargetTypesListBox->SelectLine(0);

	this->pRemainsListBox = new CListBoxWidget(TAG_REMAINS_LISTBOX,
		X_GRAPHICLISTBOX2, Y_GRAPHICLISTBOX2, CX_GRAPHICLISTBOX2, 3 * LIST_LINE_HEIGHT + 4);
	this->pAddCommandDialog->AddWidget(this->pRemainsListBox);
	this->pRemainsListBox->SortAlphabetically(true);
	this->pRemainsListBox->SetHotkeyItemSelection(true);
	this->pRemainsListBox->AddItem(M_ROCKGOLEM, g_pTheDB->GetMessageText(MID_RockGolem));
	this->pRemainsListBox->AddItem(M_FEGUNDO, g_pTheDB->GetMessageText(MID_Fegundo));
	this->pRemainsListBox->AddItem(M_CONSTRUCT, g_pTheDB->GetMessageText(MID_Construct));
	this->pRemainsListBox->SelectLine(0);

	this->pWaitForItemsListBox = new CListBoxWidget(TAG_WAITFORITEMLISTBOX,
		X_ITEMLISTBOX, Y_ITEMLISTBOX, CX_ITEMLISTBOX, CY_ITEMLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pWaitForItemsListBox);
	this->pWaitForItemsListBox->SortAlphabetically(true);
	this->pWaitForItemsListBox->SetHotkeyItemSelection(true);
	this->PopulateItemListBox(this->pWaitForItemsListBox, false, false, true);
	this->pWaitForItemsListBox->SelectLine(0);

	//Yes/No selection.
	this->pOnOffListBox = new CListBoxWidget(TAG_ONOFFLISTBOX,
			X_ONOFFLISTBOX, Y_ONOFFLISTBOX, CX_ONOFFLISTBOX, CY_ONOFFLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pOnOffListBox);
	this->pOnOffListBox->AddItem(0, g_pTheDB->GetMessageText(MID_Off));
	this->pOnOffListBox->AddItem(1, g_pTheDB->GetMessageText(MID_On));
	this->pOnOffListBox->SelectLine(0);

	this->pOnOffListBox2 = new CListBoxWidget(TAG_ONOFFLISTBOX2,
			X_ONOFFLISTBOX2, Y_ONOFFLISTBOX, CX_ONOFFLISTBOX, CY_ONOFFLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pOnOffListBox2);
	this->pOnOffListBox2->AddItem(0, g_pTheDB->GetMessageText(MID_Off));
	this->pOnOffListBox2->AddItem(1, g_pTheDB->GetMessageText(MID_On));
	this->pOnOffListBox2->SelectLine(0);

	this->pImperativeListBox = new CListBoxWidget(TAG_IMPERATIVELISTBOX,
			X_IMPERATIVELISTBOX, Y_IMPERATIVELISTBOX, CX_IMPERATIVELISTBOX, CY_IMPERATIVELISTBOX);
	this->pAddCommandDialog->AddWidget(this->pImperativeListBox);
	PopulateImperativeListBox();

	this->pBehaviorListBox = new CListBoxWidget(TAG_BEHAVIOR_LISTBOX,
			X_IMPERATIVELISTBOX, Y_IMPERATIVELISTBOX, CX_IMPERATIVELISTBOX, CY_BEHAVIORLISTBOX);
	this->pAddCommandDialog->AddWidget(this->pBehaviorListBox);
	PopulateBehaviorListBox();

	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_DISPLAYSPEECHLABEL,
			X_DISPLAYSPEECHLABEL, Y_DISPLAYSPEECHLABEL,
			CX_DISPLAYSPEECHLABEL, CY_DISPLAYSPEECHLABEL,
			F_Small, g_pTheDB->GetMessageText(MID_DisplaySpeech)));

	//Movement types
	this->pMovementTypeListBox = new CListBoxWidget(TAG_MOVETYPELISTBOX,
		MOVETYPELISTBOX_X, MOVETYPELISTBOX_Y, MOVETYPELISTBOX_CX, MOVETYPELISTBOX_CY);
	this->pAddCommandDialog->AddWidget(pMovementTypeListBox);
	this->pMovementTypeListBox->AddItem(MovementType::GROUND, g_pTheDB->GetMessageText(MID_Ground));
	this->pMovementTypeListBox->AddItem(MovementType::GROUND_AND_SHALLOW_WATER, g_pTheDB->GetMessageText(MID_GroundAndShallow));
	this->pMovementTypeListBox->AddItem(MovementType::WATER, g_pTheDB->GetMessageText(MID_Water));
	this->pMovementTypeListBox->AddItem(MovementType::WALL, g_pTheDB->GetMessageText(MID_Wall));
	this->pMovementTypeListBox->AddItem(MovementType::AIR, g_pTheDB->GetMessageText(MID_Air));

	//Open/close selection.
	this->pOpenCloseListBox = new CListBoxWidget(TAG_OPENCLOSELISTBOX,
			X_OPENCLOSELISTBOX, Y_OPENCLOSELISTBOX, CX_OPENCLOSELISTBOX, CY_OPENCLOSELISTBOX);
	this->pAddCommandDialog->AddWidget(this->pOpenCloseListBox);
	this->pOpenCloseListBox->AddItem(OA_CLOSE, g_pTheDB->GetMessageText(MID_Close));
	this->pOpenCloseListBox->AddItem(OA_OPEN, g_pTheDB->GetMessageText(MID_Open));
	this->pOpenCloseListBox->SelectLine(0);

	this->pWeaponListBox = new CListBoxWidget(TAG_WEAPON_LISTBOX,
			X_WEAPON_LISTBOX, Y_WEAPON_LISTBOX, CX_WEAPON_LISTBOX, CY_WEAPON_LISTBOX);
	this->pAddCommandDialog->AddWidget(this->pWeaponListBox);
	this->pWeaponListBox->AddItem(WT_Sword, g_pTheDB->GetMessageText(MID_WeaponSword));
	this->pWeaponListBox->AddItem(WT_Pickaxe, g_pTheDB->GetMessageText(MID_WeaponPickaxe));
	this->pWeaponListBox->AddItem(WT_Spear, g_pTheDB->GetMessageText(MID_WeaponSpear));
	this->pWeaponListBox->AddItem(WT_Staff, g_pTheDB->GetMessageText(MID_WeaponStaff));
	this->pWeaponListBox->AddItem(WT_Dagger, g_pTheDB->GetMessageText(MID_WeaponDagger));
	this->pWeaponListBox->AddItem(WT_Caber, g_pTheDB->GetMessageText(MID_WeaponCaber));
	this->pWeaponListBox->AddItem(WT_Off, g_pTheDB->GetMessageText(MID_Off));
	this->pWeaponListBox->AddItem(WT_On, g_pTheDB->GetMessageText(MID_On));
	this->pWeaponListBox->SelectLine(0);

	// Weapon Flags
	this->pWeaponFlagsListBox = new CListBoxWidget(TAG_WEAPON_LISTBOX2,
		X_WEAPON_LISTBOX, Y_WEAPON_LISTBOX, CX_WEAPON_LISTBOX, CY_WEAPON_LISTBOX);
	this->pAddCommandDialog->AddWidget(this->pWeaponFlagsListBox);
	this->pWeaponFlagsListBox->SelectMultipleItems(true);
	this->pWeaponFlagsListBox->AddItem(ScriptFlag::WEAPON_SWORD, g_pTheDB->GetMessageText(MID_WeaponSword));
	this->pWeaponFlagsListBox->AddItem(ScriptFlag::WEAPON_PICKAXE, g_pTheDB->GetMessageText(MID_WeaponPickaxe));
	this->pWeaponFlagsListBox->AddItem(ScriptFlag::WEAPON_SPEAR, g_pTheDB->GetMessageText(MID_WeaponSpear));
	this->pWeaponFlagsListBox->AddItem(ScriptFlag::WEAPON_STAFF, g_pTheDB->GetMessageText(MID_WeaponStaff));
	this->pWeaponFlagsListBox->AddItem(ScriptFlag::WEAPON_DAGGER, g_pTheDB->GetMessageText(MID_WeaponDagger));
	this->pWeaponFlagsListBox->AddItem(ScriptFlag::WEAPON_CABER, g_pTheDB->GetMessageText(MID_WeaponCaber));
	this->pWeaponFlagsListBox->SelectLine(0);

	//Goto label.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_GOTOLABEL, X_GOTOLABEL,
			Y_GOTOLABEL, CX_GOTOLABEL, CY_GOTOLABEL, F_Small, g_pTheDB->GetMessageText(MID_Label)));
	CTextBoxWidget *pGotoLabelText = new CTextBoxWidget(TAG_GOTOLABELTEXT, X_GOTOLABELTEXT, Y_GOTOLABELTEXT,
			CX_GOTOLABELTEXT, CY_GOTOLABELTEXT, MAX_TEXT_LABEL_SIZE);
	this->pAddCommandDialog->AddWidget(pGotoLabelText);

	//Goto label list.
	this->pGotoLabelListBox = new CListBoxWidget(TAG_GOTOLABELLISTBOX,
			X_GOTOLABELLISTBOX, Y_GOTOLABELLISTBOX, CX_GOTOLABELLISTBOX, CY_GOTOLABELLISTBOX, true);
	this->pAddCommandDialog->AddWidget(this->pGotoLabelListBox);

	//Variable handling widgets.

	CTextBoxWidget *pVarNameTextBox = new CTextBoxWidget(TAG_VARNAMETEXTINPUT, X_GOTOLABELTEXT, Y_GOTOLABELTEXT,
		CX_GOTOLABELTEXT, CY_GOTOLABELTEXT, MAX_TEXT_LABEL_SIZE);
	pVarNameTextBox->AddHotkey(SDLK_RETURN, TAG_VARADD);
	this->pAddCommandDialog->AddWidget(pVarNameTextBox);


	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_VARNAMETEXTLABEL,
			X_VARTEXTLABEL, Y_VARTEXTLABEL, CX_VARTEXTLABEL, CY_VARTEXTLABEL,
			F_Small, g_pTheDB->GetMessageText(MID_VarNameText)));

	this->pVarListBox = new CListBoxWidget(TAG_VARLIST,
			X_VARLISTBOX, Y_VARLISTBOX, CX_VARLISTBOX, CY_VARLISTBOX, true);
	this->pVarListBox->AddHotkey(SDLK_RETURN, TAG_OK);
	this->pAddCommandDialog->AddWidget(this->pVarListBox);

	CButtonWidget *pVarAddButton = new CButtonWidget(TAG_VARADD, X_VARADD,
			Y_VARADD, CX_VARADD, CY_VARADD, g_pTheDB->GetMessageText(MID_VarAdd));
	this->pAddCommandDialog->AddWidget(pVarAddButton);

	CButtonWidget *pVarRemoveButton = new CButtonWidget(TAG_VARREMOVE, X_VARREMOVE,
			Y_VARREMOVE, CX_VARREMOVE, CY_VARREMOVE, g_pTheDB->GetMessageText(MID_VarRemove));
	this->pAddCommandDialog->AddWidget(pVarRemoveButton);

	this->pVarOpListBox = new CListBoxWidget(TAG_VAROPLIST,
			X_VAROPLIST, Y_VAROPLIST, CX_VAROPLIST, CY_VAROPLIST);
	this->pAddCommandDialog->AddWidget(this->pVarOpListBox);
	this->pVarOpListBox->AddHotkey(SDLK_RETURN, TAG_OK);
	this->pVarOpListBox->AddItem(ScriptVars::Assign, g_pTheDB->GetMessageText(MID_VarAssign));
	this->pVarOpListBox->AddItem(ScriptVars::AssignText, g_pTheDB->GetMessageText(MID_VarAssignText));
	this->pVarOpListBox->AddItem(ScriptVars::AppendText, g_pTheDB->GetMessageText(MID_VarAppendText));
	this->pVarOpListBox->AddItem(ScriptVars::Inc, g_pTheDB->GetMessageText(MID_VarInc));
	this->pVarOpListBox->AddItem(ScriptVars::Dec, g_pTheDB->GetMessageText(MID_VarDec));
	this->pVarOpListBox->AddItem(ScriptVars::MultiplyBy, g_pTheDB->GetMessageText(MID_VarMultiplyBy));
	this->pVarOpListBox->AddItem(ScriptVars::DivideBy, g_pTheDB->GetMessageText(MID_VarDivideBy));
	this->pVarOpListBox->AddItem(ScriptVars::Mod, g_pTheDB->GetMessageText(MID_VarMod));
	this->pVarOpListBox->SelectLine(0);

	this->pVarCompListBox = new CListBoxWidget(TAG_VARCOMPLIST,
			X_VARCOMPLIST, Y_VARCOMPLIST, CX_VARCOMPLIST, CY_VARCOMPLIST);
	this->pAddCommandDialog->AddWidget(this->pVarCompListBox);
	this->pVarCompListBox->AddHotkey(SDLK_RETURN, TAG_OK);
	this->pVarCompListBox->AddItem(ScriptVars::Equals, g_pTheDB->GetMessageText(MID_VarEquals));
	this->pVarCompListBox->AddItem(ScriptVars::EqualsText, g_pTheDB->GetMessageText(MID_VarEqualsText));
	this->pVarCompListBox->AddItem(ScriptVars::Greater, g_pTheDB->GetMessageText(MID_VarGreater));
	this->pVarCompListBox->AddItem(ScriptVars::GreaterThanOrEqual, g_pTheDB->GetMessageText(MID_VarGreaterThanOrEqual));
	this->pVarCompListBox->AddItem(ScriptVars::Less, g_pTheDB->GetMessageText(MID_VarLess));
	this->pVarCompListBox->AddItem(ScriptVars::LessThanOrEqual, g_pTheDB->GetMessageText(MID_VarLessThanOrEqual));
	this->pVarCompListBox->AddItem(ScriptVars::Inequal, g_pTheDB->GetMessageText(MID_VarInequal));
	this->pVarCompListBox->SelectLine(0);

	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_VARVALUELABEL,
			X_VARVALUELABEL, Y_VARVALUELABEL, CX_VARVALUELABEL, CY_VARVALUELABEL,
			F_Small, g_pTheDB->GetMessageText(MID_VarOperand)));

	CTextBoxWidget *pVarOperand = new CTextBoxWidget(TAG_VARVALUE, X_VARVALUE, Y_VARVALUE,
			CX_VARVALUE, CY_VARVALUE);
	pVarOperand->SetText(wszZero);
	pVarOperand->AddHotkey(SDLK_RETURN, TAG_OK);
	this->pAddCommandDialog->AddWidget(pVarOperand);
	
	this->pPlayerGraphicListBox = new CListBoxWidget(TAG_GRAPHICLISTBOX2,
			X_GRAPHICLISTBOX2, Y_GRAPHICLISTBOX2, CX_GRAPHICLISTBOX2, CY_GRAPHICLISTBOX2, true);
	this->pPlayerGraphicListBox->SetHotkeyItemSelection(true);
	this->pPlayerGraphicListBox->SortAlphabetically(true);
	this->pAddCommandDialog->AddWidget(this->pPlayerGraphicListBox);

	this->pAddCommandGraphicListBox = new CListBoxWidget(TAG_GRAPHICLISTBOX3,
			X_GRAPHICLISTBOX2, Y_GRAPHICLISTBOX2, CX_GRAPHICLISTBOX2, CY_GRAPHICLISTBOX2, true);
	this->pAddCommandGraphicListBox->SetHotkeyItemSelection(true);
	this->pAddCommandGraphicListBox->SortAlphabetically(true);
	this->pAddCommandDialog->AddWidget(this->pAddCommandGraphicListBox);

	this->pGlobalScriptListBox = new CListBoxWidget(TAG_GLOBALSCRIPTLISTBOX,
			X_GLOBALSCRIPTLISTBOX, Y_GLOBALSCRIPTLISTBOX, CX_GLOBALSCRIPTLISTBOX, CY_GLOBALSCRIPTLISTBOX, true);
	this->pAddCommandDialog->AddWidget(this->pGlobalScriptListBox);

	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_CUTSCENELABEL, X_CUTSCENELABEL,
			Y_CUTSCENELABEL, CX_CUTSCENELABEL, CY_CUTSCENELABEL, F_Small,
			g_pTheDB->GetMessageText(MID_CutSceneIncrement)));

	//Relative movement.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_MOVERELXLABEL, X_MOVERELXLABEL,
			Y_MOVERELXLABEL, CX_MOVERELXLABEL, CY_MOVERELXLABEL, F_Small,
			g_pTheDB->GetMessageText(MID_MoveRelX)));
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_MOVERELYLABEL, X_MOVERELYLABEL,
			Y_MOVERELYLABEL, CX_MOVERELYLABEL, CY_MOVERELYLABEL, F_Small,
			g_pTheDB->GetMessageText(MID_MoveRelY)));
	CTextBoxWidget *pRelX = new CTextBoxWidget(TAG_MOVERELX, X_MOVERELX, Y_MOVERELX,
			CX_MOVERELX, CY_MOVERELX, 4);
	pRelX->SetDigitsOnly(true);
	pRelX->SetAllowNegative(true);
	pRelX->SetText(wszZero);
	this->pAddCommandDialog->AddWidget(pRelX);
	CTextBoxWidget *pRelY = new CTextBoxWidget(TAG_MOVERELY, X_MOVERELY, Y_MOVERELY,
			CX_MOVERELY, CY_MOVERELY, 4);
	pRelY->SetDigitsOnly(true);
	pRelY->SetAllowNegative(true);
	pRelY->SetText(wszZero);
	this->pAddCommandDialog->AddWidget(pRelY);

	//(x,y) coords
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_X_COORD_LABEL, X_XCOORDLABEL,
			Y_XCOORDLABEL, CX_XCOORDLABEL, CY_XCOORDLABEL, F_Small,
			g_pTheDB->GetMessageText(MID_X_Coord)));
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_Y_COORD_LABEL, X_YCOORDLABEL,
			Y_YCOORDLABEL, CX_YCOORDLABEL, CY_YCOORDLABEL, F_Small,
			g_pTheDB->GetMessageText(MID_Y_Coord)));
	pRelX = new CTextBoxWidget(TAG_X_COORD, X_XCOORD, Y_XCOORD,
			CX_XCOORD, CY_XCOORD, 4);
	pRelX->SetDigitsOnly(true);
	pRelX->SetText(wszZero);
	this->pAddCommandDialog->AddWidget(pRelX);
	pRelY = new CTextBoxWidget(TAG_Y_COORD, X_YCOORD, Y_YCOORD,
			CX_YCOORD, CY_YCOORD, 3);
	pRelY->SetDigitsOnly(true);
	pRelY->SetText(wszZero);
	this->pAddCommandDialog->AddWidget(pRelY);

	//Sounds.
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_LOOPSOUND, X_LOOPSOUND,
			Y_LOOPSOUND, CX_LOOPSOUND, CY_LOOPSOUND, F_Small,
			g_pTheDB->GetMessageText(MID_LoopSound)));

	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_SOUNDEFFECTLABEL,
			X_SOUNDEFFECTLABEL, Y_SOUNDEFFECTLABEL, CX_SOUNDEFFECTLABEL, CY_SOUNDEFFECTLABEL,
			F_Small, g_pTheDB->GetMessageText(MID_SoundEffect)));

	//Display filter list box.
	this->pDisplayFilterListBox = new CListBoxWidget(TAG_DISPLAYFILTER,
			X_DISPLAYFILTER_LISTBOX, Y_DISPLAYFILTER_LISTBOX, CX_DISPLAYFILTER_LISTBOX, CY_DISPLAYFILTER_LISTBOX);
	this->pAddCommandDialog->AddWidget(this->pDisplayFilterListBox);
	this->pDisplayFilterListBox->AddItem(ScriptFlag::D_Normal, g_pTheDB->GetMessageText(MID_DisplayNormal));
	this->pDisplayFilterListBox->AddItem(ScriptFlag::D_BandW, g_pTheDB->GetMessageText(MID_DisplayBandW));
	this->pDisplayFilterListBox->AddItem(ScriptFlag::D_Sepia, g_pTheDB->GetMessageText(MID_DisplaySepia));
	this->pDisplayFilterListBox->AddItem(ScriptFlag::D_Negative, g_pTheDB->GetMessageText(MID_DisplayNegative));
	this->pDisplayFilterListBox->SelectLine(0);

	//World map icon display flags list box.
	this->pWorldMapIconFlagListBox = new CListBoxWidget(TAG_ICONDISPLAY,
			X_ICONDISPLAY_LISTBOX, Y_ICONDISPLAY_LISTBOX, CX_ICONDISPLAY_LISTBOX, CY_ICONDISPLAY_LISTBOX);
	this->pAddCommandDialog->AddWidget(this->pWorldMapIconFlagListBox);
	this->pWorldMapIconFlagListBox->AddItem(ScriptFlag::WMI_OFF, g_pTheDB->GetMessageText(MID_WMI_Off));
	this->pWorldMapIconFlagListBox->AddItem(ScriptFlag::WMI_ON, g_pTheDB->GetMessageText(MID_WMI_On));
	this->pWorldMapIconFlagListBox->AddItem(ScriptFlag::WMI_LEVELSTATE, g_pTheDB->GetMessageText(MID_WMI_LevelState));
	this->pWorldMapIconFlagListBox->AddItem(ScriptFlag::WMI_LOCKED, g_pTheDB->GetMessageText(MID_WMI_Locked));
	this->pWorldMapIconFlagListBox->AddItem(ScriptFlag::WMI_DISABLED, g_pTheDB->GetMessageText(MID_WMI_Disabled));
	this->pWorldMapIconFlagListBox->AddItem(ScriptFlag::WMI_NOLABEL, g_pTheDB->GetMessageText(MID_WMI_NoLabel));
	this->pWorldMapIconFlagListBox->SelectLine(0);

	//World map image display flags list box.
	this->pWorldMapImageFlagListBox = new CListBoxWidget(TAG_IMAGEDISPLAY,
			X_ICONDISPLAY_LISTBOX, Y_ICONDISPLAY_LISTBOX, CX_ICONDISPLAY_LISTBOX, CY_IMAGEDISPLAY_LISTBOX);
	this->pAddCommandDialog->AddWidget(this->pWorldMapImageFlagListBox);
	this->pWorldMapImageFlagListBox->AddItem(ScriptFlag::WMI_OFF, g_pTheDB->GetMessageText(MID_WMI_Off));
	this->pWorldMapImageFlagListBox->AddItem(ScriptFlag::WMI_ON, g_pTheDB->GetMessageText(MID_WMI_On));
	this->pWorldMapImageFlagListBox->AddItem(ScriptFlag::WMI_LOCKED, g_pTheDB->GetMessageText(MID_WMI_Locked));
	this->pWorldMapImageFlagListBox->AddItem(ScriptFlag::WMI_DISABLED, g_pTheDB->GetMessageText(MID_WMI_Disabled));
	this->pWorldMapImageFlagListBox->AddItem(ScriptFlag::WMI_NOLABEL, g_pTheDB->GetMessageText(MID_WMI_NoLabel));
	this->pWorldMapImageFlagListBox->SelectLine(0);

	//Attack Tile list box.
	this->pAttackTileListBox = new CListBoxWidget(TAG_ATTACKTILE,
			X_ATTACKTILE_LISTBOX, Y_ATTACKTILE_LISTBOX, CX_ATTACKTILE_LISTBOX, CY_ATTACKTILE_LISTBOX);
	this->pAddCommandDialog->AddWidget(this->pAttackTileListBox);
	this->pAttackTileListBox->AddItem(ScriptFlag::AT_Stab, g_pTheDB->GetMessageText(MID_AttackTileStab));
	this->pAttackTileListBox->AddItem(ScriptFlag::AT_Explode, g_pTheDB->GetMessageText(MID_AttackTileExplosion));
	this->pAttackTileListBox->AddItem(ScriptFlag::AT_Damage, g_pTheDB->GetMessageText(MID_AttackTileDamage));
	this->pAttackTileListBox->AddItem(ScriptFlag::AT_Kill, g_pTheDB->GetMessageText(MID_AttackTileKill));
	this->pAttackTileListBox->SelectLine(0);

	//Color text
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_COLOR_LABEL, X_COLOR_LABEL,
			Y_COLOR_LABEL, CX_COLOR_LABEL, CY_COLOR_LABEL, F_Small,
			g_pTheDB->GetMessageText(MID_ScriptColor)));
	CTextBoxWidget *pText2 = new CTextBoxWidget(TAG_TEXT2, X_TEXT2, Y_TEXT2,
			CX_TEXT2, CY_TEXT2, 6);
	this->pAddCommandDialog->AddWidget(pText2);

	//Input overlay
	CTextBox2DWidget *pLabelText = new CTextBox2DWidget(TAG_IMAGEOVERLAYTEXT, X_IMAGEOVERLAYTEXT, Y_IMAGEOVERLAYTEXT,
			CX_IMAGEOVERLAYTEXT, CY_IMAGEOVERLAYTEXT, 4*1024);
	this->pAddCommandDialog->AddWidget(pLabelText);
	this->pAddCommandDialog->AddWidget(new CLabelWidget(TAG_IMAGEOVERLAY_LABEL, X_IMAGELABEL, Y_IMAGELABEL,
			CX_IMAGELABEL, CY_IMAGELABEL, F_Small, g_pTheDB->GetMessageText(MID_ImageOverlayStrategy)));

	//OK/cancel buttons.
	CButtonWidget *pOKButton = new CButtonWidget(
			TAG_OK, X_OKBUTTON, Y_OKBUTTON, CX_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Okay));
	pOKButton->AddHotkey(SDLK_RETURN, TAG_OK);
	this->pAddCommandDialog->AddWidget(pOKButton);
	CButtonWidget *pCancelButton = new CButtonWidget(
			TAG_CANCEL, X_CANCELBUTTON, Y_CANCELBUTTON, CX_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Cancel));
	this->pAddCommandDialog->AddWidget(pCancelButton);

	//Add to main dialog.
	AddWidget(this->pAddCommandDialog);
	
	this->pAddCommandDialog->Center();
	this->pAddCommandDialog->Hide();
}

//*****************************************************************************
void CCharacterDialogWidget::AddScriptDialog()
//A pop-up dialog for editing a script only.
{
	static const UINT CX_SCRIPT_DIALOG = 660;
	static const UINT CY_SCRIPT_DIALOG = 630;

	static const UINT CX_TITLE = 240;
	static const UINT CY_TITLE = CY_LABEL_FONT_HEADER;
	static const int X_TITLE = (CX_SCRIPT_DIALOG - CX_TITLE) / 2;
	static const int Y_TITLE = CY_SPACE;

	static const int X_COMMANDSLABEL = CX_SPACE;
	static const int Y_COMMANDSLABEL = Y_TITLE + CY_TITLE + CY_SPACE;
	static const UINT CX_COMMANDSLABEL = 110;
	static const UINT CY_COMMANDSLABEL = 33;
	static const int X_COMMANDS = X_COMMANDSLABEL;
	static const int Y_COMMANDS = Y_COMMANDSLABEL + CY_COMMANDSLABEL;
	static const UINT CX_COMMANDS = CX_SCRIPT_DIALOG - X_COMMANDS - CX_SPACE;
	static const UINT CY_COMMANDS = 22*LIST_LINE_HEIGHT + 4; //22 items
	static const UINT CX_ADDCOMMAND = 130;

	static const int X_ADDCOMMAND = X_COMMANDS + CX_COMMANDS - CX_ADDCOMMAND;
	static const int Y_ADDCOMMAND = Y_COMMANDSLABEL - 4;
	static const UINT CY_ADDCOMMAND = CY_STANDARD_BUTTON;
	static const UINT CX_DELETECOMMAND = 150;
	static const int X_DELETECOMMAND = X_ADDCOMMAND - CX_DELETECOMMAND - CX_SPACE;
	static const int Y_DELETECOMMAND = Y_ADDCOMMAND;
	static const UINT CY_DELETECOMMAND = CY_STANDARD_BUTTON;

	static const UINT CX_CHAROPTIONS = 100;
	static const int X_CHAROPTIONS = X_DELETECOMMAND - CX_CHAROPTIONS - CX_SPACE;
	static const int Y_CHAROPTIONS = Y_ADDCOMMAND;
	static const UINT CY_CHAROPTIONS = CY_STANDARD_BUTTON;

	static const UINT CX_BUTTON = 70;
	static const int X_OKBUTTON = (CX_SCRIPT_DIALOG - (CX_BUTTON + CX_SPACE)) / 2;
	static const int Y_OKBUTTON = CY_SCRIPT_DIALOG - CY_STANDARD_BUTTON - 15;

	ASSERT(!this->pScriptDialog);
	this->pScriptDialog = new CRenameDialogWidget(0, CX_SPACE, GetY() + (GetH()-CY_SCRIPT_DIALOG)/2,
			CX_SCRIPT_DIALOG, CY_SCRIPT_DIALOG);

	this->pScriptDialog->AddWidget(new CLabelWidget(0, X_TITLE, Y_TITLE, CX_TITLE, CY_TITLE,
			F_Header, g_pTheDB->GetMessageText(MID_EditDefaultScript)));

	//Commands.
	this->pScriptDialog->AddWidget(new CButtonWidget(TAG_ADDCOMMAND2, X_ADDCOMMAND, Y_ADDCOMMAND,
			CX_ADDCOMMAND, CY_ADDCOMMAND, g_pTheDB->GetMessageText(MID_AddCommand)));
	this->pScriptDialog->AddWidget(new CButtonWidget(TAG_DELETECOMMAND2, X_DELETECOMMAND, Y_DELETECOMMAND,
			CX_DELETECOMMAND, CY_DELETECOMMAND, g_pTheDB->GetMessageText(MID_DeleteCommand)));
	this->pScriptDialog->AddWidget(new CButtonWidget(TAG_CHAROPTIONS2, X_CHAROPTIONS, Y_CHAROPTIONS,
		CX_CHAROPTIONS, CY_CHAROPTIONS, g_pTheDB->GetMessageText(MID_CharOptions)));

	this->pScriptDialog->AddWidget(new CLabelWidget(0, X_COMMANDSLABEL, Y_COMMANDSLABEL,
			CX_COMMANDSLABEL, CY_COMMANDSLABEL, F_Small, g_pTheDB->GetMessageText(MID_Commands)));
	this->pDefaultScriptCommandsListBox = new CCommandListBoxWidget(TAG_DEFAULTCOMMANDSLISTBOX, X_COMMANDS, Y_COMMANDS,
			CX_COMMANDS, CY_COMMANDS);
	this->pScriptDialog->AddWidget(this->pDefaultScriptCommandsListBox);

	//OK button.
	CButtonWidget *pButton = new CButtonWidget(
			TAG_OK2, X_OKBUTTON, Y_OKBUTTON, CX_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Okay));
	this->pScriptDialog->AddWidget(pButton);

	//Add to main dialog.
	AddWidget(this->pScriptDialog);
	this->pScriptDialog->Center();
	this->pScriptDialog->Hide();
}

//*****************************************************************************
void CCharacterDialogWidget::AddCustomCharacter()
//Adds a custom character with specified name and image.
{
	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen->pHold);
	const WCHAR *pCharName = this->pCharNameText->GetText();
	ASSERT(WCSlen(pCharName) > 0);

	const UINT dwNewCharID = pEditRoomScreen->pHold->AddCharacter(pCharName);
	if (!dwNewCharID)
		pEditRoomScreen->ShowOkMessage(MID_CharNameDuplicationError);
	else {
		HoldCharacter *pChar = pEditRoomScreen->pHold->GetCharacter(dwNewCharID);
		ASSERT(pChar);
		CListBoxWidget *pCharGraphicList = DYN_CAST(CListBoxWidget*, CWidget*,
				GetWidget(TAG_CHARGRAPHICLISTBOX));
		pChar->wType = pCharGraphicList->GetSelectedItem();
		this->pCharNameText->SetText(wszEmpty); //reset for next use

		//Refresh list.  Select new character.
		PopulateMainGraphicList();
		this->pCharListBox->SelectItem(dwNewCharID);
		SelectCharacter();
	}
}

//*****************************************************************************
void CCharacterDialogWidget::DeleteCustomCharacter()
//Deletes selected character (pending room+hold save).
{
	const UINT dwCharID = this->pCharListBox->GetSelectedItem();
	if (!dwCharID)
		return;

	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));

	if (pEditRoomScreen->ShowYesNoMessage(MID_DeleteCharacterPrompt) != TAG_YES)
		return;

	ASSERT(pEditRoomScreen->pHold);
	VERIFY(pEditRoomScreen->pHold->DeleteCharacter(dwCharID));

	//Refresh list.
	PopulateMainGraphicList();

	SetCharacterWidgetStates();
	this->pAddCharacterDialog->Paint();
}

//*****************************************************************************
void CCharacterDialogWidget::EditCharacter(CCharacter* pCharacter)
//Prepares widgets for viewing and editing given character.
{
	ASSERT(pCharacter);
	ASSERT(!this->pCommand);
	ASSERT(!this->pCharacter);
	this->pCharacter = pCharacter;

	PopulateVarList();

	PopulateMainGraphicList();

	CWidget *pButton = this->pAddCommandDialog->GetWidget(TAG_VARREMOVE);
	ASSERT(pButton);
	const bool bEnable = this->pVarListBox->GetItemCount() > 0;
	if (!bEnable && GetSelectedWidget() == pButton)
		SelectNextWidget();
	pButton->Enable(bEnable);

	//Compile temporary commands list from character.
	for (UINT wIndex=0; wIndex<pCharacter->commands.size(); ++wIndex)
	{
		CCharacterCommand *pCommand = new CCharacterCommand(pCharacter->commands[wIndex]);
		this->commands.push_back(pCommand);
	}

	//Generate labels once command list has been compiled.
	PopulateGotoLabelList(this->commands);

	PopulateCommandDescriptions(this->pCommandsListBox, this->commands);

	this->pIsVisibleButton->SetChecked(pCharacter->IsVisible());

	SetWidgetStates();
}

//*****************************************************************************
COMMANDPTR_VECTOR* CCharacterDialogWidget::GetActiveCommands()
//Returns: a pointer to the command list being edited
{
	if (this->bEditingDefaultScript)
	{
		//Select the custom NPC whose script is being edited.
		ASSERT(this->defaultScriptCustomCharID);
		this->pCharListBox->SelectItem(this->defaultScriptCustomCharID);

		HoldCharacter *pChar = GetCustomCharacter();
		ASSERT(pChar);
		ASSERT(pChar->pCommands);
		return pChar->pCommands;
	}

	return &this->commands;
}

//*****************************************************************************
CListBoxWidget* CCharacterDialogWidget::GetActiveCommandListBox()
//Returns: pointer to the active command list box widget
{
	if (this->bEditingDefaultScript)
		return this->pDefaultScriptCommandsListBox;

	return this->pCommandsListBox;
}

//*****************************************************************************
void CCharacterDialogWidget::OnClick(
//Handles click event.
//
//Params:
	const UINT dwTagNo)          //(in)   Widget that received event.
{
	switch (dwTagNo)
	{
		case TAG_OK:
		case TAG_CANCEL:
		{
			//Reflect command changes in character object now.
			UpdateCharacter();

			CDialogWidget::OnClick(dwTagNo); //deactivate
		}
		break;

		case TAG_CHARACTERS:
		{
			//Select this character's custom type, if one is set.
			this->pCharListBox->SelectItem(this->pCharacter->wLogicalIdentity);

			EditCustomCharacters();
		}
		break;

		case TAG_CHAROPTIONS:
		{
			this->pCharOptionsDialog->SetCharacter(this->pCharacter);
			UINT dwTagNo = this->pCharOptionsDialog->Display();
			this->pCharOptionsDialog->Hide();

			if (dwTagNo == CCharacterOptionsDialog::TAG_SAVE){
				this->pCharacter->wProcessSequence = this->pCharOptionsDialog->GetProcessSequence();
			}
			
			Paint();
		}
		break;

		case TAG_DELETECOMMAND:
		{
			ASSERT(this->pCommandsListBox->GetItemCount() > 0);
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_EditRoom));
			ASSERT(pEditRoomScreen);
			if (pEditRoomScreen->ShowYesNoMessage(MID_DeleteCommandPrompt) == TAG_YES)
			{
				DeleteCommands(this->pCommandsListBox, this->commands);
				SetWidgetStates();
			}
			pEditRoomScreen->SetUpdateRectAfterMessage(true);
			Paint(false);
			pEditRoomScreen->UpdateRect();
		}
		break;

		case TAG_ADDCOMMAND:
		{
			ASSERT(this->pAddCommandDialog);
			ASSERT(!this->pCommand);
			ASSERT(!this->pSound);
			SetActionWidgetStates();
			this->bRetainFields = true;
			UINT dwTagNo;
			bool bLoop=true;
			do {
				dwTagNo = this->pAddCommandDialog->Display();
				switch (dwTagNo)
				{
					case TAG_ADDSOUND: AddSound(); break;
					case TAG_TESTSOUND: TestSound(); break;
					case TAG_VARADD: AddVar(); break;
					case TAG_VARREMOVE: DeleteVar();	break;
					default:
						bLoop = false;
					break;
				}
			} while (bLoop && !IsDeactivating());
			this->bRetainFields = false;
			if (dwTagNo == TAG_OK)
			{
				//Begin adding this command.
				const CCharacterCommand::CharCommand command =
					(CCharacterCommand::CharCommand)this->pActionListBox->GetSelectedItem();
				this->pCommand = new CCharacterCommand();
				this->pCommand->command = command;
				SetCommandParametersFromWidgets(this->pCommandsListBox, this->commands);
			} else {
				//Command addition was canceled.
				//Clear any data generated for the command.
				ASSERT(!this->pSound || !this->pSound->dwDataID);	//should be fresh (not added to DB yet)
				delete this->pSound;
				this->pSound = NULL;
			}
			SetWidgetStates();

			if (this->pParent) this->pParent->Paint();   //refresh screen
			Paint();
		}
		break;
	}
}

//*****************************************************************************
void CCharacterDialogWidget::OnDoubleClick(
//Handles double click event.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	switch (dwTagNo)
	{
		case TAG_COMMANDSLISTBOX:
			EditClickedCommand();
		break;

		case TAG_CHARACTERLISTBOX:
		{
			//Edit an existing command.
			if (!this->pCharListBox->ClickedSelection()) break;

			HoldCharacter *pChar = GetCustomCharacter();
			if (!pChar) break;

			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_EditRoom));
			ASSERT(pEditRoomScreen);
			WSTRING wstr = pChar->charNameText;
			const UINT dwAnswerTagNo = pEditRoomScreen->ShowTextInputMessage(
					MID_NameCharacterPrompt, wstr);
			if (dwAnswerTagNo == TAG_OK)
				pChar->charNameText = wstr;
		}
		break;
	}
}

//*****************************************************************************
void CCharacterDialogWidget::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Event.
{
	CDialogWidget::OnKeyDown(dwTagNo, Key);
	if (IsDeactivating())
	{
		UpdateCharacter();
		return;
	}

	if (this->pAddCommandDialog->IsVisible())
		return; //can't alter the command list while inputting a command

	switch (Key.keysym.sym)
	{
		case SDLK_DELETE:
		{
			CListBoxWidget *pActiveCommandList = GetActiveCommandListBox();
			if (pActiveCommandList->GetItemCount())
			{
				CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
						g_pTheSM->GetScreen(SCR_EditRoom));
				ASSERT(pEditRoomScreen);
				if (pEditRoomScreen->ShowYesNoMessage(MID_DeleteCommandPrompt) == TAG_YES)
				{
					DeleteCommands(pActiveCommandList, *GetActiveCommands());
					SetWidgetStates();
				}
				pEditRoomScreen->SetUpdateRectAfterMessage(true);
				Paint(false);
				pEditRoomScreen->UpdateRect();
			}
		}
		break;

		//Cut/copy/paste from command list.
		case SDLK_x:
		{
			if ((Key.keysym.mod & KMOD_CTRL) == 0)
				break;

			CListBoxWidget *pActiveCommandList = GetActiveCommandListBox();
			vector<void*> keys(pActiveCommandList->GetSelectedItemsPointers());
			if (keys.empty())
				break;

			g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR);
			ClearPasteBuffer();

			UINT wSelectedLine = pActiveCommandList->GetSelectedLineNumber();

			//Remove selected commands from script/command list.
			//Put into paste and clipboard buffers.
			COMMANDPTR_VECTOR *pCommands = GetActiveCommands();

			for (vector<void*>::const_iterator key = keys.begin(); key != keys.end(); ++key)
			{
				void *pKey = *key;
				CCharacterCommand *pCommand = (CCharacterCommand*)(pKey);
				ASSERT(pCommand);

				const UINT wLine = pActiveCommandList->GetLineWithKeyPointer(pKey);
				COMMANDPTR_VECTOR::iterator iter = pCommands->begin() + wLine;
				ASSERT(iter != pCommands->end());
				pCommands->erase(iter);
				pActiveCommandList->RemoveItem(pKey);

				this->commandBuffer.push_back(pCommand);
			}

			//Before removing label ID references, prepare commands so goto-style
			//commands may be pasted correctly.
			prepareForwardReferences(this->commandBuffer);

			//Now remove labels from list.
			for (COMMANDPTR_VECTOR::const_iterator command = this->commandBuffer.begin();
				command != this->commandBuffer.end(); ++command)
			{
				CCharacterCommand *pCommand = *command;
				if (pCommand->command == CCharacterCommand::CC_Label)
					this->pGotoLabelListBox->RemoveItem(pCommand->x);
			}

			//Display remaining commands.
			if (wSelectedLine >= pActiveCommandList->GetItemCount())
				wSelectedLine = pActiveCommandList->GetItemCount()-1;
			pActiveCommandList->SelectLine(wSelectedLine);
			if (IsEditingDefaultScript())
				SetDefaultScriptWidgetStates();
			else
				SetWidgetStates();
			Paint();
		}
		break;
		case SDLK_c:
		{
			if ((Key.keysym.mod & KMOD_CTRL) == 0)
				break;

			CListBoxWidget *pActiveCommandList = GetActiveCommandListBox();
			vector<void*> keys(pActiveCommandList->GetSelectedItemsPointers());
			if (keys.empty())
				break;

			g_pTheSound->PlaySoundEffect(SEID_POTION);

			//Replace paste and clipboard buffers with a copy of the current selection.
			ClearPasteBuffer();

			for (vector<void*>::const_iterator key = keys.begin(); key != keys.end(); ++key)
			{
				//Make a distinct copy of this command.
				CCharacterCommand *pCommand = (CCharacterCommand*)(*key);
				ASSERT(pCommand);
				CCharacterCommand *pCommandCopy = new CCharacterCommand(*pCommand, true);
				this->commandBuffer.push_back(pCommandCopy);
			}

			//Prepare label ID references for pasting.
			prepareForwardReferences(this->commandBuffer);
		}
		break;
		case SDLK_v:
		{
			if ((Key.keysym.mod & KMOD_CTRL) == 0)
				break;

			if (this->commandBuffer.empty())
				break;

			g_pTheSound->PlaySoundEffect(SEID_MIMIC);

			CListBoxWidget *pActiveCommandList = GetActiveCommandListBox();
			ASSERT(pActiveCommandList);
			const UINT wSelectedLine = pActiveCommandList->GetSelectedLineNumber();

			//Insert commands in paste buffer into command list after cursor position.
			COMMANDPTR_VECTOR *pCommands = GetActiveCommands();
			ASSERT(pCommands);
			pCommands->insert(pCommands->begin() + (wSelectedLine + 1),
					this->commandBuffer.begin(), this->commandBuffer.end());

			//Replicate these commands for repeat use in paste buffer.
			COMMANDPTR_VECTOR tempCommandBuffer;
			for (COMMANDPTR_VECTOR::const_iterator command = this->commandBuffer.begin();
					command != this->commandBuffer.end(); ++command)
			{
				CCharacterCommand *pCommand = *command;
				CCharacterCommand *pCommandCopy = new CCharacterCommand(*pCommand, true);
				tempCommandBuffer.push_back(pCommandCopy);

				//Add any labels to the label list.
				if (pCommand->command == CCharacterCommand::CC_Label)
				{
					//Generate unique label info.
					GenerateUniqueLabelName(pCommand->label);
					pCommand->x = ++this->wIncrementedLabel;
					AddLabel(pCommand);
				}
			}

			resolveForwardReferences(this->commandBuffer);

			this->commandBuffer = tempCommandBuffer;

			const UINT deletedCommands = FilterUnsupportedCommands();

			//Show inserted commands.
			const UINT wTopLine = pActiveCommandList->GetTopLineNumber();
			PopulateCommandDescriptions(pActiveCommandList, *pCommands);
			pActiveCommandList->SetTopLineNumber(wTopLine);

			const UINT selectedLine = wSelectedLine + this->commandBuffer.size() - deletedCommands;
			pActiveCommandList->SelectLine(selectedLine);
			pActiveCommandList->RequestPaint();
			if (IsEditingDefaultScript())
				SetDefaultScriptWidgetStates();
			else
				SetWidgetStates();
		}
		break;

		//Select all script commands.
		case SDLK_a:
		{
			if ((Key.keysym.mod & KMOD_CTRL) == 0) //Ctrl-A
				break;

			CListBoxWidget *pActiveCommandList = GetActiveCommandListBox();
			pActiveCommandList->SelectAllLines();
			pActiveCommandList->RequestPaint();
		}
		break;

		//Copy/paste script commands to/from clipboard text.
		case SDLK_b:
		{
			if (this->bEditingCommand)
				break;
			ASSERT(!this->pCommand);

			if ((Key.keysym.mod & KMOD_CTRL) == 0) //Ctrl-b
				break;

			CListBoxWidget *pActiveCommandList = GetActiveCommandListBox();
			COMMANDPTR_VECTOR *pCommands = GetActiveCommands();

			WSTRING wstrCommandsText;
			if ((Key.keysym.mod & KMOD_SHIFT) == 0)
			{
				//Commands -> clipboard
				const CIDSet selectedLines = pActiveCommandList->GetSelectedLineNumbers();
				for (CIDSet::const_iterator line=selectedLines.begin();
						line!=selectedLines.end(); ++line)
				{
					if (*line < pCommands->size())
						wstrCommandsText += toText(*pCommands, (*pCommands)[*line], pActiveCommandList, *line);
				}
				if (!wstrCommandsText.empty())
					g_pTheSound->PlaySoundEffect(SEID_POTION);
				CClipboard::SetString(wstrCommandsText);
			} else {  //Ctrl-shift-b
				//Clipboard -> commands
				CClipboard::GetString(wstrCommandsText);

				static const WCHAR delimiters[] = {We('\r'),We('\n'),We(0)};
				COMMANDPTR_VECTOR newCommands;
				WCHAR *pwLine, *pwText = (WCHAR*)wstrCommandsText.c_str();

				//Extract one line at a time and parse.
				pwLine = WCStok(pwText, delimiters);
				while (pwLine)
				{
					WSTRING wstrCommand = pwLine; //make copy
					CCharacterCommand *pCommand = fromText(wstrCommand);
					if (pCommand)
						newCommands.push_back(pCommand);
					pwLine = WCStok(NULL, delimiters);
				}

				if (!newCommands.empty())
				{
					g_pTheSound->PlaySoundEffect(SEID_MIMIC);

					resolveForwardReferences(newCommands);

					const UINT wSelectedLine = pActiveCommandList->GetSelectedLineNumber();
					if (wSelectedLine < pCommands->size())
						pCommands->insert(pCommands->begin() + (wSelectedLine + 1),
							newCommands.begin(), newCommands.end());
					else
						pCommands->insert(pCommands->begin(), newCommands.begin(), newCommands.end());

					const UINT deletedCommands = FilterUnsupportedCommands();

					//Show inserted commands.
					const UINT wTopLine = pActiveCommandList->GetTopLineNumber();
					PopulateCommandDescriptions(pActiveCommandList, *pCommands);
					pActiveCommandList->SetTopLineNumber(wTopLine);

					const UINT selectedLine = wSelectedLine + newCommands.size() - deletedCommands;
					pActiveCommandList->SelectLine(selectedLine);
				} else {
					//Sound for empty or invalid clipboard data.
					g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);
				}
				if (IsEditingDefaultScript())
					SetDefaultScriptWidgetStates();
				else
					SetWidgetStates();
				Paint();
			}
		}
		break;

		default: break;
	}
}

//*****************************************************************************
void CCharacterDialogWidget::OnRearranged(const UINT dwTagNo)
//Called when the commands list has been reordered.
{
	switch (dwTagNo)
	{
		case TAG_COMMANDSLISTBOX:
		case TAG_DEFAULTCOMMANDSLISTBOX:
		{
			//Update pointers to match the order in list box.
			CListBoxWidget *pActiveCommandList = GetActiveCommandListBox();
			COMMANDPTR_VECTOR *pCommands = GetActiveCommands();

			for (UINT wIndex=0; wIndex<pActiveCommandList->GetItemCount(); ++wIndex)
				(*pCommands)[wIndex] = (CCharacterCommand*)(pActiveCommandList->GetKeyPointerAtLine(wIndex));
			const int nSelectedLine = pActiveCommandList->GetSelectedLineNumber();
			ASSERT(nSelectedLine >= 0);
			const UINT wTopLine = pActiveCommandList->GetTopLineNumber();
			PopulateCommandDescriptions(pActiveCommandList, *pCommands);
			pActiveCommandList->SetTopLineNumber(wTopLine);
			pActiveCommandList->SelectLine(nSelectedLine);
			pActiveCommandList->RequestPaint();
		}
		break;

		default: break;
	}
}

//*****************************************************************************
void CCharacterDialogWidget::OnSelectChange(const UINT dwTagNo)
//Called when a list box selection has changed.
{
	switch (dwTagNo)
	{
		case TAG_ACTIONLISTBOX:
			SetActionWidgetStates();
			this->pAddCommandDialog->Paint();
		break;

		case TAG_GRAPHICLISTBOX:
			ASSERT(this->pCharacter);
			this->pCharacter->wLogicalIdentity = this->pGraphicListBox->GetSelectedItem();
			if (this->pCharacter->wLogicalIdentity == M_NONE)
			{
				//Don't allow visible to be checked when "None" graphic setting is selected.
				this->pIsVisibleButton->SetChecked(false);
				this->pIsVisibleButton->RequestPaint();
				this->pCharacter->bVisible = false;
			}
		break;

		case TAG_ISVISIBLE:
			ASSERT(this->pCharacter);
			if ((this->pCharacter->bVisible = this->pIsVisibleButton->IsChecked()))
			{
				//Don't allow "None" graphic setting when visible is checked.
				if (this->pGraphicListBox->GetSelectedItem() == M_NONE)
				{
					this->pCharacter->wLogicalIdentity = M_CITIZEN;
					this->pGraphicListBox->SelectItem(this->pCharacter->wLogicalIdentity);
					this->pGraphicListBox->RequestPaint();
				}
			}
		break;

		case TAG_CHARACTERNAME:
		{
			CButtonWidget *pButton = DYN_CAST(CButtonWidget*, CWidget*,
					GetWidget(TAG_ADDCHARACTER));
			pButton->Enable(!this->pCharNameText->IsEmpty());
			pButton->RequestPaint();
		}
		break;

		case TAG_CHARACTERLISTBOX:
			SelectCharacter();
		break;

		case TAG_CHARGRAPHICLISTBOX:
			SetCustomGraphic();
		break;

		case TAG_ANIMATESPEED:
			SetAnimationSpeed();
		break;

		default: break;
	}
}

//*****************************************************************************
void CCharacterDialogWidget::AddCommand()
//Append the newly created/modified command object to the list of commands.
//Update the command list box.
{
	ASSERT(this->pCommand);
	CCharacterCommand *pCommand = this->pCommand;
	this->pCommand = NULL;

	//Remove any loaded sound that was not used in this command.
	delete this->pSound;
	this->pSound = NULL;

	CListBoxWidget *pActiveCommandListBox = GetActiveCommandListBox();
	COMMANDPTR_VECTOR *pCommands = GetActiveCommands();

	int nSelectedLine = pActiveCommandListBox->GetSelectedLineNumber();
	if (this->bEditingCommand)
	{
		//Command parameters updated.
		this->bEditingCommand = false;
	} else {
		//New command added to list.
		ASSERT((int)pCommands->size() >= nSelectedLine+1);
		COMMANDPTR_VECTOR::iterator insertPoint = pCommands->begin();
		for (int n = 0; n < nSelectedLine+1; ++n)
			++insertPoint;
		pCommands->insert(insertPoint, pCommand);
		nSelectedLine = pActiveCommandListBox->AddItemPointer(pCommand,
				wszEmpty,	//real text added below
				false, nSelectedLine+1);
		SetCommandColor(pActiveCommandListBox, nSelectedLine, pCommand->command);
	}

	AddLabel(pCommand);

	//Refresh script
	const UINT wTopLine = pActiveCommandListBox->GetTopLineNumber();
	PopulateCommandDescriptions(pActiveCommandListBox, *pCommands);
	pActiveCommandListBox->SetTopLineNumber(wTopLine);
	pActiveCommandListBox->SelectLine(nSelectedLine);
}

//*****************************************************************************
void CCharacterDialogWidget::AddLabel(CCharacterCommand* pCommand)
//If command is a label, then adds the label ID+text to the label list.
{
	ASSERT(pCommand);
	if (pCommand->command == CCharacterCommand::CC_Label)
	{
		this->pGotoLabelListBox->AddItem(pCommand->x, pCommand->label.c_str());
		if (!this->pGotoLabelListBox->ItemIsSelected())
			this->pGotoLabelListBox->SelectLine(0);
	}
}

//*****************************************************************************
void CCharacterDialogWidget::AddSound()
//If a sound is already loaded, it will be deleted.
//Otherwise, calling this brings up file dialog to load a new sound effect.
{
	//If a sound is already loaded, mark it for deletion.
	bool bDeletingSound = this->pSound != NULL;

	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen *, CScreen *,
		g_pTheSM->GetScreen(SCR_EditRoom));

	ASSERT(pEditRoomScreen);
	ASSERT(pEditRoomScreen->pRoom);

	if (this->pCommand)
	{
		CDbSpeech *pSpeech = this->pCommand->pSpeech;
		if (this->bEditingCommand && pSpeech && pSpeech->GetSound())
		{
			bDeletingSound = true;
			ASSERT(pSpeech->GetSound() == this->pSound);
			pSpeech->SetSound(NULL);   //deletes this->pSound
			this->pSound = NULL;
		} else {
			//Delete any newly-loaded sound.
		}
	}

	delete this->pSound;
	this->pSound = NULL;
	if (bDeletingSound) { 
		SetActionWidgetStates(); 
		Paint(); 
		return; 
	}

	const UINT dwSoundId = pEditRoomScreen->SelectMediaID(0, CSelectMediaDialogWidget::Sounds);

	if (dwSoundId)
	{
		this->pSound = g_pTheDB->Data.GetByID(dwSoundId);
		g_pTheSound->PlayVoice(this->pSound->data);
	}

	SetActionWidgetStates();
	if (this->pParent) this->pParent->Paint();   //refresh screen
	Paint();
}

//*****************************************************************************
void CCharacterDialogWidget::AddVar()
//Adds a new hold variable by the name specified in the text box widget.
{
	CTextBoxWidget *pDialogue = DYN_CAST(CTextBoxWidget*, CWidget*,
			this->pAddCommandDialog->GetWidget(TAG_VARNAMETEXTINPUT));
	ASSERT(pDialogue);
	const WCHAR *pVarName = pDialogue->GetText();
	AddVar(pVarName);
}

//*****************************************************************************
UINT CCharacterDialogWidget::AddVar(const WCHAR* pVarName)
//Adds a new hold variable by the name specified in the text box widget.
{
	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen);
	ASSERT(pEditRoomScreen->pHold);

	if (!WCSlen(pVarName))
	{
		pEditRoomScreen->ShowOkMessage(MID_VarNameEmptyError);
		return 0;
	}

	if (!CDbHold::IsVarNameGoodSyntax(pVarName))
	{
		pEditRoomScreen->ShowOkMessage(MID_VarNameSyntaxError);
		return 0;
	}

	const UINT dwNewVarID = pEditRoomScreen->pHold->AddVar(pVarName);
	if (!dwNewVarID)
	{
		pEditRoomScreen->ShowOkMessage(MID_VarNameDuplicationError);
		return 0;
	}
	const UINT line = this->pVarListBox->AddItem(dwNewVarID, pVarName);
	this->pVarListBox->SelectLine(line);

	CWidget *pButton = this->pAddCommandDialog->GetWidget(TAG_VARREMOVE);
	ASSERT(pButton);
	pButton->Enable();
	if (pButton->IsVisible() && this->pAddCommandDialog->IsVisible())
		pButton->RequestPaint();

	return dwNewVarID;
}

//*****************************************************************************
void CCharacterDialogWidget::ClearPasteBuffer()
//Deletes all commands in the command line paste buffer.
{
	COMMANDPTR_VECTOR::iterator command;
	for (command = this->commandBuffer.begin(); command != this->commandBuffer.end(); ++command)
	{
		CCharacterCommand *pCommand = *command;

		//Mark DB members for deletion.
		if (pCommand->pSpeech)
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_EditRoom));
			ASSERT(pEditRoomScreen);
			if (this->bEditingDefaultScript)
			{
				ASSERT(pEditRoomScreen->pHold);
				pEditRoomScreen->pHold->MarkSpeechForDeletion(pCommand->pSpeech);
			} else {
				ASSERT(pEditRoomScreen->pRoom);
				pEditRoomScreen->pRoom->MarkSpeechForDeletion(pCommand->pSpeech);
			}
		}

		delete pCommand;
	}
	this->commandBuffer.clear();
}

//*****************************************************************************
void CCharacterDialogWidget::DeleteCommands(
//Delete selected commands from the given list box and commands vector.
//
//Params:
	CListBoxWidget *pActiveCommandList, COMMANDPTR_VECTOR& commands)
{
	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen);
	ASSERT(pEditRoomScreen->pRoom);

	int nSelectedLine = pActiveCommandList->GetSelectedLineNumber();
	ASSERT(nSelectedLine >= 0);
	UINT wTopLine = pActiveCommandList->GetTopLineNumber();

	CIDSet lines = pActiveCommandList->GetSelectedLineNumbers();
	for (CIDSet::const_reverse_iterator line = lines.rbegin(); line != lines.rend(); ++line)
	{
		const UINT wLine = *line;
		CCharacterCommand *pCommand = (CCharacterCommand*)(pActiveCommandList->GetKeyPointerAtLine(wLine));
		ASSERT(pCommand);
		COMMANDPTR_VECTOR::iterator iter = commands.begin() + wLine;
		ASSERT(iter != commands.end());
		commands.erase(iter);
		if (pCommand->command == CCharacterCommand::CC_Label)
			this->pGotoLabelListBox->RemoveItem(pCommand->x);
		if (pCommand->pSpeech)
		{
			if (this->bEditingDefaultScript)
			{
				ASSERT(pEditRoomScreen->pHold);
				pEditRoomScreen->pHold->MarkSpeechForDeletion(pCommand->pSpeech);
			} else

				pEditRoomScreen->pRoom->MarkSpeechForDeletion(pCommand->pSpeech);
		}
		delete pCommand;
	}
	pActiveCommandList->RemoveItems(lines);

	PopulateCommandDescriptions(pActiveCommandList, commands);  //refresh script
	const UINT wLines = pActiveCommandList->GetItemCount();
	if (wTopLine >= wLines)
		wTopLine = wLines - 1;
	if (nSelectedLine >= static_cast<int>(wLines))
		nSelectedLine = wLines - 1;
	pActiveCommandList->SetTopLineNumber(wTopLine);
	if (nSelectedLine >= 0)
		pActiveCommandList->SelectLine(nSelectedLine);
}

//*****************************************************************************
void CCharacterDialogWidget::DeleteVar()
//Delete selected hold var.
{
	ASSERT(this->pVarListBox->GetItemCount() > 0);
	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen);
	ASSERT(pEditRoomScreen->pHold);
	if (pEditRoomScreen->ShowYesNoMessage(MID_DeleteVarPrompt) != TAG_YES)
		return;

	const UINT dwVarID = this->pVarListBox->GetSelectedItem();
	const UINT line = this->pVarListBox->GetSelectedLineNumber();
	if (pEditRoomScreen->pHold->DeleteVar(dwVarID))
	{
		PopulateVarList();

		//Select closest var in list.
		const UINT numLines = this->pVarListBox->GetItemCount();
		if (line < numLines)
			this->pVarListBox->SelectLine(line);
		else if (numLines > 0)
			this->pVarListBox->SelectLine(numLines-1);

		//Set button state.
		CWidget *pButton = GetWidget(TAG_VARREMOVE);
		ASSERT(pButton);
		const bool bEnable = this->pVarListBox->GetItemCount() > 0;
		if (!bEnable && GetSelectedWidget() == pButton)
			SelectNextWidget();
		pButton->Enable(bEnable);
		pButton->RequestPaint();
	}
}

//*****************************************************************************
void CCharacterDialogWidget::EditClickedCommand()
//Edit an existing command, which was clicked on.
{
	CListBoxWidget *pActiveCommandList = GetActiveCommandListBox();

	if (!pActiveCommandList->ClickedSelection())
		return;

	void *pCommandPointer = pActiveCommandList->GetSelectedItemPointer();
	if (!pCommandPointer)
		return;
	this->pCommand = (CCharacterCommand*)(pCommandPointer);
	ASSERT(this->pCommand);

	this->bEditingCommand = true;
	this->pActionListBox->SelectItem(this->pCommand->command);
	SetWidgetsFromCommandParameters();
	this->bRetainFields = true; //so SetActionWidgetStates doesn't reset the fields
	SetActionWidgetStates();
	bool bSoundAttached = this->pSound != NULL;

	UINT dwTagNo;
	bool bLoop=true;
	do {
		dwTagNo = this->pAddCommandDialog->Display();
		switch (dwTagNo)
		{
			case TAG_ADDSOUND:
				AddSound();
				if (!this->pSound)
					bSoundAttached = false;
			break;
			case TAG_TESTSOUND: TestSound(); break;
			case TAG_VARADD: AddVar(); break;
			case TAG_VARREMOVE: DeleteVar();	break;
			default:
				bLoop = false;
			break;
		}
	} while (bLoop && !IsDeactivating());

	this->bRetainFields = false;
	if (dwTagNo == TAG_OK)
	{
		COMMANDPTR_VECTOR *pCommands = GetActiveCommands();
		SetCommandParametersFromWidgets(pActiveCommandList, *pCommands);
	}
	else
	{
		//Action was canceled -- reset vars.
		if (!this->bEditingCommand || (!bSoundAttached && this->pSound && !this->pSound->dwDataID))
			delete this->pSound; //delete sound only if it's not yet attached to a record
		this->pCommand = NULL;
		this->pSound = NULL;
		this->bEditingCommand = false;
	}

	if (this->pParent)
		this->pParent->Paint();   //refresh screen
	else Paint();
}

//*****************************************************************************
void CCharacterDialogWidget::EditCustomCharacters()
{
	ASSERT(this->pAddCharacterDialog);

	SetCharacterWidgetStates();

	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen);

	UINT dwTagNo;
	bool bLoop=true;
	do {
		pEditRoomScreen->Paint(false);

		dwTagNo = this->pAddCharacterDialog->Display();
		this->pAddCharacterDialog->Show(); //keep visible while performing operations below
		switch (dwTagNo)
		{
			case TAG_ADDCHARACTER: AddCustomCharacter(); break;
			case TAG_DELETECHARACTER: DeleteCustomCharacter(); break;
			case TAG_CUSTOMAVATAR: SetCustomImage(); break;
			case TAG_DEFAULTAVATAR: SetDefaultAvatar(); break;
			case TAG_CUSTOMTILES: SetCustomTiles(); break;
			case TAG_DEFAULTTILES: SetDefaultTiles(); break;
			case TAG_EDITDEFAULTSCRIPT: EditDefaultScriptForCustomNPC(); break;
			default:
				bLoop=false;
			break;
		}
	} while (bLoop && !IsDeactivating());

	//Now hide dialog.
	this->pAddCharacterDialog->Hide();

	//Refresh main graphic list.
	PopulateMainGraphicList();

	if (this->pParent)
		this->pParent->Paint();   //refresh screen
	Paint();
}

//*****************************************************************************
void CCharacterDialogWidget::EditDefaultScriptForCustomNPC()
//Edit the default script for the selected custom NPC.
{
	HoldCharacter *pChar = GetCustomCharacter();
	if (!pChar)
		return;

	//Retrieve the custom NPC's default script.
	if (!this->bEditingDefaultScript)
	{
		if (!pChar->pCommands)
		{
			//Allocate a script object, which will serve as a working copy for the script.
			//This working copy will be repacked and updated to the DB when the hold
			//object is saved.
			pChar->pCommands = new COMMANDPTR_VECTOR;
			CCharacter::LoadCommands(pChar->ExtraVars, *pChar->pCommands);
		}
		this->bEditingDefaultScript = true;
		this->defaultScriptCustomCharID = pChar->dwCharID;
	}

	//Different commands/fields are available in default scripts.
	PopulateImperativeListBox(true);

	PopulateGotoLabelList(*pChar->pCommands);

	PopulateCommandDescriptions(this->pDefaultScriptCommandsListBox, *pChar->pCommands);

	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen);

	//Edit script.
	UINT dwTagNo;
	bool bLoop=true;
	do {
		SetDefaultScriptWidgetStates();

		pEditRoomScreen->Paint(false);
		pEditRoomScreen->SetUpdateRectAfterMessage(false);
		this->pAddCharacterDialog->Paint();

		dwTagNo = this->pScriptDialog->Display();
		this->pScriptDialog->Show(); //keep visible while performing operations below
		switch (dwTagNo)
		{
			case TAG_DELETECOMMAND2:
			{
				ASSERT(this->pDefaultScriptCommandsListBox->GetItemCount() > 0);
				if (pEditRoomScreen->ShowYesNoMessage(MID_DeleteCommandPrompt) == TAG_YES)
					DeleteCommands(this->pDefaultScriptCommandsListBox, *pChar->pCommands);

				//Repaint dialogs after asking question.
				this->pAddCharacterDialog->Paint(false);
				this->pScriptDialog->Paint(false);
				pEditRoomScreen->UpdateRect();
			}
			break;

			case TAG_ADDCOMMAND2:
			{
				ASSERT(this->pAddCommandDialog);
				ASSERT(!this->pCommand);
				ASSERT(!this->pSound);

				SetActionWidgetStates();
				this->bRetainFields = true;
				UINT dwTagNo;
				bool bLoop=true;
				do {
					dwTagNo = this->pAddCommandDialog->Display();
					switch (dwTagNo)
					{
						case TAG_ADDSOUND: AddSound(); break;
						case TAG_TESTSOUND: TestSound(); break;
						case TAG_VARADD: AddVar(); break;
						case TAG_VARREMOVE: DeleteVar();	break;
						default:
							bLoop = false;
						break;
					}
				} while (bLoop);
				this->bRetainFields = false;
				if (dwTagNo == TAG_OK)
				{
					//Begin adding this command.
					const CCharacterCommand::CharCommand command =
						(CCharacterCommand::CharCommand)this->pActionListBox->GetSelectedItem();
					this->pCommand = new CCharacterCommand();
					this->pCommand->command = command;
					SetCommandParametersFromWidgets(this->pDefaultScriptCommandsListBox, *pChar->pCommands);
				} else {
					//Command addition was canceled.
					//Clear any data generated for the command.
					ASSERT(!this->pSound || !this->pSound->dwDataID);	//should be fresh (not added to DB yet)
					delete this->pSound;
					this->pSound = NULL;
				}
			}
			break;

			case TAG_CHAROPTIONS2:
			{
				this->pCharOptionsDialog->SetCharacter(pChar);
				UINT dwTagNo = this->pCharOptionsDialog->Display();
				this->pCharOptionsDialog->Hide();

				if (dwTagNo == CCharacterOptionsDialog::TAG_SAVE){
					pChar->ExtraVars.SetVar(ParamProcessSequenceStr, this->pCharOptionsDialog->GetProcessSequence());
				}
				
				Paint();
			}
			break;

			default:
				bLoop = false;
			break;
		}
	} while (bLoop && !IsDeactivating());

	//Now hide dialog.
	this->pScriptDialog->Hide();
	pEditRoomScreen->SetUpdateRectAfterMessage(true);

	//Undo default script-specific command options.
	PopulateImperativeListBox();

	if (IsDeactivating())
		return; //indicates the user is being prompted for command parameters -- don't do anything below yet

	//Save edited script back to custom NPC.
	FinishEditingDefaultScript();

	//Reload the gotos for the NPC being edited.
	PopulateGotoLabelList(this->commands);

	this->pAddCharacterDialog->Paint();
}

//*****************************************************************************
UINT CCharacterDialogWidget::FilterUnsupportedCommands()
//Some command options are not supported outside of global script definitions,
//i.e., custom NPC default scripts.
//When commands are pasted into a script,
//ensure only supported commands are retained.
//
//Returns: number of commands deleted
{
	return 0; //nothing needs filtering yet

/*
	if (this->bEditingDefaultScript)
		return 0; //no filtering needed

	COMMANDPTR_VECTOR *pCommands = GetActiveCommands();
	UINT index=0, deletedCount=0;
	while (index<pCommands->size())
	{
		COMMANDPTR_VECTOR::iterator commandIter = pCommands->begin() + index;
		CCharacterCommand *pCommand = *commandIter;
		bool bDeleted = false;
		switch (pCommand->command)
		{
			case CCharacterCommand::CC_Imperative:
				//"Make Global" imperative only available in default scripts.
				if (pCommand->x == ScriptFlag::MakeGlobal)
				{
					pCommands->erase(commandIter);
					delete pCommand;
					bDeleted = true;
					++deletedCount;
				}
			break;

			default: break;
		}

		if (!bDeleted)
			++index;
	}

	return deletedCount;
*/
}

//*****************************************************************************
void CCharacterDialogWidget::FinishEditingDefaultScript()
//A custom NPC's default script is now no longer being edited.
{
	ASSERT(this->bEditingDefaultScript);

	this->bEditingDefaultScript = false;
	this->defaultScriptCustomCharID = 0;
}

//*****************************************************************************
void CCharacterDialogWidget::GenerateUniqueLabelName(WSTRING& label) const
//Alters 'label', if needed, to ensure it is not a duplicate of any
//label name text.
{
	UINT labelID, num=0;
	WSTRING alteredName = label;
	bool bFound;
	do {
		UINT tempIndex = 0;
		labelID = findTextMatch(this->pGotoLabelListBox, alteredName.c_str(), tempIndex, bFound);
		//If a label matching this label's name is found in the label list,
		//alter this one's name until a unique label name is generated.
		if (bFound)
		{
			WCHAR temp[12];
			alteredName = label;
			alteredName += _itoW(num++, temp, 10);
		}
	} while (bFound);
	label = alteredName;
}

//*****************************************************************************
WSTRING CCharacterDialogWidget::GetCommandDesc(
//Returns: a string describing the command in normal language.
//
//Params:
	const COMMANDPTR_VECTOR& commands,
	CCharacterCommand* pCommand)
const
{
	WSTRING wstr;

	//Call language-specific version of method.
	switch (Language::GetLanguage())
	{
		case Language::English:
		case Language::French:
		case Language::Russian:
			wstr += GetCommandDesc_English(commands, *pCommand);
		break;
		default:
			//Language not supported -- just use English grammar.
			wstr += GetCommandDesc_English(commands, *pCommand);
		break;
	}

	return wstr;
}

//*****************************************************************************
WSTRING CCharacterDialogWidget::GetCommandDesc_English(
//Print command name.
//
//Params:
	const COMMANDPTR_VECTOR& commands,
	const CCharacterCommand& command)
const
{
	//Print command name.
	WSTRING wstr = this->pActionListBox->GetTextForKey(command.command);
	switch (command.command) {
		default:
			wstr += wszSpace;
			break;
		case CCharacterCommand::CC_Label:
			wstr.clear();
			break;
		//deprecated commands still display
		case CCharacterCommand::CC_PlayerEquipsWeapon:
			wstr = g_pTheDB->GetMessageText(MID_SetPlayerSword);
			wstr += wszSpace;
			break;
	}

	WCHAR temp[16];
	switch (command.command)
	{
		case CCharacterCommand::CC_AppearAt:
		case CCharacterCommand::CC_ActivateItemAt:
		case CCharacterCommand::CC_TeleportTo:
		case CCharacterCommand::CC_TeleportPlayerTo:
		case CCharacterCommand::CC_GetEntityDirection:
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
		break;
		case CCharacterCommand::CC_AttackTile:
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
			wstr += wszSpace;
			wstr += this->pAttackTileListBox->GetTextForKey(command.flags);
		break;
		case CCharacterCommand::CC_PushTile:
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
			wstr += wszSpace;
			wstr += this->pDirectionListBox2->GetTextForKey(command.w);
		break;
		case CCharacterCommand::CC_MoveRel:
			wstr += wszLeftParen;
			wstr += _itoW((int)command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW((int)command.y, temp, 10);
			wstr += wszRightParen;
			if (command.w)
			{
				wstr += wszSpace;
				wstr += g_pTheDB->GetMessageText(MID_NoTurning);
			}
			if (command.h)
			{
				wstr += wszSpace;
				wstr += g_pTheDB->GetMessageText(MID_SingleStep);
			}
		break;
		case CCharacterCommand::CC_MoveTo:
			if (command.flags)
			{
				UINT wBitfield = 1;
				for (UINT wBits = 0; wBits<32; ++wBits, wBitfield *= 2)
				{
					if ((command.flags & wBitfield) == wBitfield)
					{
						wstr += this->pWaitFlagsListBox->GetTextForKey(wBitfield);
						wstr += wszSpace;
					}
				}
			} else {
				wstr += wszLeftParen;
				wstr += _itoW((int)command.x, temp, 10);
				wstr += wszComma;
				wstr += _itoW((int)command.y, temp, 10);
				wstr += wszRightParen;
			}
			if (command.w)
			{
				wstr += wszSpace;
				wstr += g_pTheDB->GetMessageText(MID_NoTurning);
			}
			if (command.h)
			{
				wstr += wszSpace;
				wstr += g_pTheDB->GetMessageText(MID_SingleStep);
			}
			break;
		case CCharacterCommand::CC_FaceTowards:
			if (command.flags)
			{
				UINT wBitfield = 1;
				for (UINT wBits = 0; wBits<32; ++wBits, wBitfield *= 2)
				{
					if ((command.flags & wBitfield) == wBitfield)
					{
						wstr += this->pWaitFlagsListBox->GetTextForKey(wBitfield);
						wstr += wszSpace;
					}
				}
			}
			else {
				wstr += wszLeftParen;
				wstr += _itoW((int)command.x, temp, 10);
				wstr += wszComma;
				wstr += _itoW((int)command.y, temp, 10);
				wstr += wszRightParen;
			}
			if (command.w)
			{
				wstr += wszSpace;
				wstr += g_pTheDB->GetMessageText(MID_SingleStep);
			}
			break;
		case CCharacterCommand::CC_FaceDirection:
		case CCharacterCommand::CC_WaitForPlayerToFace:
		case CCharacterCommand::CC_WaitForPlayerToMove:
		case CCharacterCommand::CC_WaitForOpenMove:
			wstr += this->pDirectionListBox->GetTextForKey(command.x);
		break;

		case CCharacterCommand::CC_WaitForPlayerInput:
			wstr += this->pInputListBox->GetTextForKey(command.x);
		break;

		case CCharacterCommand::CC_DisplayFilter:
			wstr += this->pDisplayFilterListBox->GetTextForKey(command.x);
		break;

		case CCharacterCommand::CC_WaitForTurn:
			wstr += _itoW(command.x, temp, 10);
		break;
		case CCharacterCommand::CC_Wait:
			wstr += _itoW(command.x, temp, 10);
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_Turns);
		break;
		case CCharacterCommand::CC_WaitForCueEvent:
			wstr += this->pEventListBox->GetTextForKey(command.x);
		break;
		case CCharacterCommand::CC_WaitForRect:
		case CCharacterCommand::CC_WaitForNotRect:
		{
			UINT wBitfield = 1;
			for (UINT wBits = 0; wBits<32; ++wBits, wBitfield *= 2)
			{
				if ((command.flags & wBitfield) == wBitfield)
				{
					wstr += this->pWaitFlagsListBox->GetTextForKey(wBitfield);
					wstr += wszSpace;
				}
			}
		}
		//no break
		case CCharacterCommand::CC_WaitForHalph:
		case CCharacterCommand::CC_WaitForNotHalph:
		case CCharacterCommand::CC_WaitForMonster:
		case CCharacterCommand::CC_WaitForNotMonster:
		case CCharacterCommand::CC_WaitForCharacter:
		case CCharacterCommand::CC_WaitForNotCharacter:
		case CCharacterCommand::CC_WaitForNoBuilding:
		case CCharacterCommand::CC_DestroyTrapdoor:
			wstr += g_pTheDB->GetMessageText(MID_At);
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
			wstr += wszHyphen;
			wstr += wszLeftParen;
			wstr += _itoW(command.x + command.w, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y + command.h, temp, 10);
			wstr += wszRightParen;
		break;
		case CCharacterCommand::CC_WaitForEntityType:
		case CCharacterCommand::CC_WaitForNotEntityType:
		{
			WSTRING charName = this->pAddCommandGraphicListBox->GetTextForKey(command.flags);
			wstr += charName.length() ? charName : wszQuestionMark;
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_At);
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
			wstr += wszHyphen;
			wstr += wszLeftParen;
			wstr += _itoW(command.x + command.w, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y + command.h, temp, 10);
			wstr += wszRightParen;
		}
		break;
		case CCharacterCommand::CC_WaitForRemains:
		{
			WSTRING charName = this->pRemainsListBox->GetTextForKey(command.flags);
			wstr += charName.length() ? charName : wszQuestionMark;
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_At);
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
			wstr += wszHyphen;
			wstr += wszLeftParen;
			wstr += _itoW(command.x + command.w, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y + command.h, temp, 10);
			wstr += wszRightParen;
		}
		break;
		case CCharacterCommand::CC_WaitForDoorTo:
			wstr += this->pOpenCloseListBox->GetTextForKey(command.w);
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_At);
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
		break;
		case CCharacterCommand::CC_WaitForWeapon:
		{
			UINT wBitfield = 1;
			for (UINT wBits = 0; wBits<32; ++wBits, wBitfield *= 2)
			{
				if ((command.flags & wBitfield) == wBitfield)
				{
					wstr += this->pWeaponFlagsListBox->GetTextForKey(wBitfield);
					wstr += wszSpace;
				}
			}
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
		}
		break;

		case CCharacterCommand::CC_BuildMarker:
			wstr += this->pBuildMarkerListBox->GetTextForKey(command.flags);
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_At);
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
			wstr += wszHyphen;
			wstr += wszLeftParen;
			wstr += _itoW(command.x + command.w, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y + command.h, temp, 10);
			wstr += wszRightParen;
			break;

		case CCharacterCommand::CC_Build:
			wstr += this->pBuildItemsListBox->GetTextForKey(command.flags);
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_At);
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
			wstr += wszHyphen;
			wstr += wszLeftParen;
			wstr += _itoW(command.x + command.w, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y + command.h, temp, 10);
			wstr += wszRightParen;
		break;

		case CCharacterCommand::CC_Speech:
		{
			CDbSpeech *pSpeech = command.pSpeech;
			ASSERT(pSpeech);
			if (!pSpeech)	//robustness
			{
				wstr += wszQuote;
				wstr += wszQuestionMark;
				wstr += wszQuote;
				break;
			}
			if (pSpeech->wMood != Mood_Normal)
			{
				wstr += this->pMoodListBox->GetTextForKey(pSpeech->wMood);
				wstr += wszSpace;
			}
			if (pSpeech->wCharacter != Speaker_None)
			{
				if (pSpeech->wCharacter != Speaker_Custom)
				{
					WSTRING charName = this->pSpeakerListBox->GetTextForKey(pSpeech->wCharacter);
					wstr += charName.length() ? charName : wszQuestionMark;
				} else {
					wstr += g_pTheDB->GetMessageText(MID_At);
					wstr += wszSpace;
					wstr += wszLeftParen;
					wstr += _itoW(command.x, temp, 10);
					wstr += wszComma;
					wstr += _itoW(command.y, temp, 10);
					wstr += wszRightParen;
				}
				wstr += wszComma;
				wstr += wszSpace;
			}
			if (pSpeech->dwDelay)
			{
				wstr += _itoW(pSpeech->dwDelay, temp, 10);
				wstr += wszComma;
				wstr += wszSpace;
			} else {
				// Display calculated duration of the speech/sound file, if any
				UINT soundLength=0;
				if (pSpeech->dwDataID) {
					const UINT soundID = pSpeech->dwDataID;
					//Optimization: Cache duration of saved data.
					if (CCharacterDialogWidget::speechLengthCache[soundID]) {
						soundLength = CCharacterDialogWidget::speechLengthCache[soundID];
					} else {
						const CDbDatum *pData = pSpeech->GetSound();
						if (pData) {
							soundLength = g_pTheSound->GetSoundLength(pData->data);
							CCharacterDialogWidget::speechLengthCache[soundID] = soundLength;
						}
					}
				} else if (pSpeech->GetSound()) {
					soundLength = g_pTheSound->GetSoundLength(pSpeech->GetSound()->data);
				}
				if (!soundLength)
					soundLength = 1000 + pSpeech->MessageText.GetSize() * 50;

				if (soundLength) {
					wstr += wszLeftParen;
					wstr += _itoW(soundLength, temp, 10);
					wstr += wszRightParen;
					wstr += wszSpace;
				}
			}
			if (pSpeech->dwDataID)
			{
				//Load sound clip name only from DB.
				wstr += GetDataName(pSpeech->dwDataID);
				wstr += wszComma;
				wstr += wszSpace;
			}
			else if (pSpeech->GetSound() &&
					!((CDbDatum*)pSpeech->GetSound())->DataNameText.empty())
			{
				//Sound exists in object, but not yet in DB.  Just display its name.
				wstr += pSpeech->GetSound()->DataNameText;
				wstr += wszComma;
				wstr += wszSpace;
			}
			wstr += wszQuote;
			wstr += (const WCHAR*)(pSpeech->MessageText);
			wstr += wszQuote;
		}
		break;
		case CCharacterCommand::CC_PlayVideo:
		{
			wstr += GetDataName(command.w);

			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_At);
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += _itoW((int)command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW((int)command.y, temp, 10);
			wstr += wszRightParen;
		}
		break;
		case CCharacterCommand::CC_ImageOverlay:
		{
			wstr += GetDataName(command.w);

			if (!command.label.empty()) {
				wstr += wszColon;
				wstr += wszSpace;
				wstr += command.label;
			}
		}
		break;

		case CCharacterCommand::CC_FlushSpeech:
		case CCharacterCommand::CC_PlayerEquipsWeapon:
			wstr += this->pOnOffListBox->GetTextForKey(command.x);
		break;

		case CCharacterCommand::CC_SetPlayerWeapon:
			wstr += this->pWeaponListBox->GetTextForKey(command.x);
		break;

		case CCharacterCommand::CC_GetNaturalTarget:
			wstr += this->pNaturalTargetTypesListBox->GetTextForKey(command.x);
			break;
		case CCharacterCommand::CC_SetPlayerStealth:
			wstr += this->pStealthListBox->GetTextForKey(command.x);
		break;
		case CCharacterCommand::CC_SetWaterTraversal:
			wstr += this->pWaterTraversalListBox->GetTextForKey(command.x);
		break;

		case CCharacterCommand::CC_StartGlobalScript:
		{
			WSTRING charName = this->pGlobalScriptListBox->GetTextForKey(command.x);
			wstr += charName.length() ? charName : wszQuestionMark;
		}
		break;

		case CCharacterCommand::CC_WaitForItem:
			wstr += this->pWaitForItemsListBox->GetTextForKey(command.flags);
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_At);
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
			wstr += wszHyphen;
			wstr += wszLeftParen;
			wstr += _itoW(command.x + command.w, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y + command.h, temp, 10);
			wstr += wszRightParen;
		break;

		case CCharacterCommand::CC_GenerateEntity:
		{
			WSTRING charName = this->pAddCommandGraphicListBox->GetTextForKey(command.h);
			wstr += charName.length() ? charName : wszQuestionMark;
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszComma;
			wstr += this->pDirectionListBox2->GetTextForKey(command.w);
			wstr += wszRightParen;
		}
		break;

		case CCharacterCommand::CC_GameEffect:
			wstr += this->pDirectionListBox3->GetTextForKey(command.w);
			wstr += wszSpace;
			wstr += this->pVisualEffectsListBox->GetTextForKey(command.h);
			wstr += wszSpace;
			wstr += g_pTheDB->GetMessageText(MID_At);
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
			wstr += wszSpace;
			wstr += this->pOnOffListBox3->GetTextForKey(command.flags);
		break;

		case CCharacterCommand::CC_Imperative:
			wstr += this->pImperativeListBox->GetTextForKey(command.x);
		break;

		case CCharacterCommand::CC_Behavior:
			wstr += this->pBehaviorListBox->GetTextForKey(command.x);
			wstr += wszSpace;
			wstr += this->pOnOffListBox3->GetTextForKey(command.y);
		break;

		case CCharacterCommand::CC_SetMovementType:
			wstr += this->pMovementTypeListBox->GetTextForKey(command.x);
		break;

		case CCharacterCommand::CC_SetMusic:
			if (command.label.size())
				wstr += command.label;
			else if (int(command.x) == SONGID_CUSTOM)
				wstr += GetDataName(command.y);
			else
				wstr += this->pMusicListBox->GetTextForKey(command.x);
		break;

		case CCharacterCommand::CC_Label:
			wstr += command.label;
			wstr += wszColon;
		break;
		case CCharacterCommand::CC_ChallengeCompleted:
			wstr += wszQuote;
			wstr += command.label;
			wstr += wszQuote;
		break;
		case CCharacterCommand::CC_FlashingText:
			if (command.h) {
				wstr += _itoW((int)command.x, temp, 16); //hex
				wstr += wszComma;
				wstr += _itoW((int)command.y, temp, 16);
				wstr += wszComma;
				wstr += _itoW((int)command.w, temp, 16);
				wstr += wszSpace;
			}
		//no break
		case CCharacterCommand::CC_Question:
		case CCharacterCommand::CC_RoomLocationText:
		{
			CDbSpeech *pSpeech = command.pSpeech;
			ASSERT(pSpeech);
			wstr += wszQuote;
			if (!pSpeech)	//robustness
				wstr += wszQuestionMark;
			else
				wstr += (const WCHAR*)(pSpeech->MessageText);
			wstr += wszQuote;
		}
		break;
		case CCharacterCommand::CC_AnswerOption:
		{
			WSTRING wstrGoto = this->pActionListBox->GetTextForKey(CCharacterCommand::CC_GoTo);
			CDbSpeech *pSpeech = command.pSpeech;
			ASSERT(pSpeech);
			wstr += wszQuote;
			if (!pSpeech)	//robustness
				wstr += wszQuestionMark;
			else
				wstr += (const WCHAR*)(pSpeech->MessageText);
			wstr += wszQuote;
			wstr += wszSpace;
			wstr += wszColon;
			wstr += wszSpace;
			wstr += wstrGoto;
			wstr += wszSpace;
			AppendGotoDestination(wstr, commands, command);
		}
		break;
		case CCharacterCommand::CC_GoSub:
		case CCharacterCommand::CC_GoTo:
		case CCharacterCommand::CC_GotoIf:
		{
			if ((int)command.x < 0) {
				AppendGotoDestination(wstr, commands, command);
			} else {
				wstr += wszQuote;
				AppendGotoDestination(wstr, commands, command);
				wstr += wszQuote;
			}
		}
		break;

		case CCharacterCommand::CC_LevelEntrance:
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_EditRoom));
			ASSERT(pEditRoomScreen->pHold);

			if (command.y)
			{
				wstr += wszLeftParen;
				wstr += g_pTheDB->GetMessageText(MID_SkipEntranceDisplay);
				wstr += wszRightParen;
				wstr += wszSpace;
			}

			wstr += GetEntranceName(pEditRoomScreen, command.x);
		}
		break;
		case CCharacterCommand::CC_WorldMapSelect:
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_EditRoom));

			wstr += GetWorldMapNameText(pEditRoomScreen, command.x);
		}
		break;
		case CCharacterCommand::CC_WorldMapMusic:
			if (command.label.size())
				wstr += command.label;
			else if (int(command.x) == SONGID_CUSTOM)
				wstr += GetDataName(command.y);
			else
				wstr += this->pMusicListBox->GetTextForKey(command.x);
		break;

		case CCharacterCommand::CC_WorldMapIcon:
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_EditRoom));

			const bool bEntranceOff = command.flags == ScriptFlag::WMI_OFF;
			if (!bEntranceOff) {
				const WSTRING charName = this->pAddCommandGraphicListBox->GetTextForKey(command.h);
				wstr += charName.length() ? charName : wszQuestionMark;
				wstr += wszSpace;
			}

			const WSTRING displayFlag = this->pWorldMapIconFlagListBox->GetTextForKey(command.flags);
			wstr += displayFlag.length() ? displayFlag : wszQuestionMark;
			wstr += wszSpace;

			if (!bEntranceOff) {
				wstr += g_pTheDB->GetMessageText(MID_At);
				wstr += wszSpace;
				wstr += wszLeftParen;
				wstr += _itoW((int)command.x, temp, 10);
				wstr += wszComma;
				wstr += _itoW((int)command.y, temp, 10);
				wstr += wszRightParen;
				wstr += wszSpace;
			}

			wstr += g_pTheDB->GetMessageText(MID_To);
			wstr += wszSpace;
			wstr += GetEntranceName(pEditRoomScreen, command.w);
		}
		break;
		case CCharacterCommand::CC_WorldMapImage:
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_EditRoom));

			const bool bEntranceOff = command.flags == ScriptFlag::WMI_OFF;
			if (!bEntranceOff) {
				wstr += GetDataName(command.h);
				wstr += wszSpace;
			}

			const WSTRING displayFlag = this->pWorldMapImageFlagListBox->GetTextForKey(command.flags);
			wstr += displayFlag.length() ? displayFlag : wszQuestionMark;
			wstr += wszSpace;

			if (!bEntranceOff) {
				wstr += g_pTheDB->GetMessageText(MID_At);
				wstr += wszSpace;
				wstr += wszLeftParen;
				wstr += _itoW((int)command.x, temp, 10);
				wstr += wszComma;
				wstr += _itoW((int)command.y, temp, 10);
				wstr += wszRightParen;
				wstr += wszSpace;
			}

			wstr += g_pTheDB->GetMessageText(MID_To);
			wstr += wszSpace;
			wstr += GetEntranceName(pEditRoomScreen, command.w);
		}
		break;

		case CCharacterCommand::CC_VarSet:
		{
			const WCHAR *wszVarName = this->pVarListBox->GetTextForKey(command.x);
			wstr += WCSlen(wszVarName) ? wszVarName : wszQuestionMark;
			wstr += wszSpace;
			switch (command.y)
			{
				case ScriptVars::AppendText: wstr += wszPlus; //no break
				case ScriptVars::Assign:
				case ScriptVars::AssignText: wstr += wszEqual; break;
				case ScriptVars::Inc: wstr += wszPlus; break;
				case ScriptVars::Dec: wstr += wszHyphen; break;
				case ScriptVars::MultiplyBy: wstr += wszAsterisk; break;
				case ScriptVars::DivideBy: wstr += wszForwardSlash; break;
				case ScriptVars::Mod: wstr += wszPercent; break;
				default: wstr += wszQuestionMark; break;
			}
			wstr += wszSpace;
			switch (command.y)
			{
				case ScriptVars::AppendText:
				case ScriptVars::AssignText:
					wstr += wszQuote;
					wstr += command.label;
					wstr += wszQuote;
				break;
				default:
					if (!command.label.empty())
						wstr += command.label;
					else
						wstr += _itoW(command.w, temp, 10);
				break;
			}
		}
		break;

		case CCharacterCommand::CC_WaitForVar:
		{
			const WCHAR *wszVarName = this->pVarListBox->GetTextForKey(command.x);
			wstr += WCSlen(wszVarName) ? wszVarName : wszQuestionMark;
			wstr += wszSpace;
			AddOperatorSymbol(wstr, command.y);
			wstr += wszSpace;
			switch (command.y)
			{
				case ScriptVars::EqualsText:
					wstr += wszQuote;
					wstr += command.label;
					wstr += wszQuote;
				break;
				default:
					if (!command.label.empty())
						wstr += command.label;
					else
						wstr += _itoW(command.w, temp, 10);
				break;
			}
		}
		break;

		case CCharacterCommand::CC_SetPlayerAppearance:
		{
			WSTRING charName = this->pPlayerGraphicListBox->GetTextForKey(command.x);
			wstr += charName.length() ? charName : wszQuestionMark;
		}
		break;
		case CCharacterCommand::CC_SetNPCAppearance:
		{
			WSTRING charName = this->pAddCommandGraphicListBox->GetTextForKey(command.x);
			wstr += charName.length() ? charName : wszQuestionMark;
			if (command.y)
			{
				wstr += wszSpace;
				wstr += g_pTheDB->GetMessageText(MID_KeepBehaviors);
			}
		}
		break;

		case CCharacterCommand::CC_CutScene:
			wstr += _itoW(command.x, temp, 10);
		break;

		case CCharacterCommand::CC_AmbientSoundAt:
			wstr += wszLeftParen;
			wstr += _itoW(command.x, temp, 10);
			wstr += wszComma;
			wstr += _itoW(command.y, temp, 10);
			wstr += wszRightParen;
			wstr += wszSpace;
			//NO BREAK

		case CCharacterCommand::CC_AmbientSound:
			if (command.h && command.w)
			{
				wstr += g_pTheDB->GetMessageText(MID_LoopSound);
				wstr += wszSpace;
			}
			if (command.w)
				wstr += GetDataName(command.w);
			else
				wstr += g_pTheDB->GetMessageText(MID_Off);
		break;

		case CCharacterCommand::CC_Appear:
		case CCharacterCommand::CC_Disappear:
		case CCharacterCommand::CC_EndScript:
		case CCharacterCommand::CC_TurnIntoMonster:
		case CCharacterCommand::CC_ReplaceWithDefault:
		case CCharacterCommand::CC_WaitForCleanRoom:
		case CCharacterCommand::CC_WaitForCleanLevel:
		case CCharacterCommand::CC_WaitForPlayerToTouchMe:
		case CCharacterCommand::CC_WaitForSomeoneToPushMe:
		case CCharacterCommand::CC_EndScriptOnExit:
		case CCharacterCommand::CC_If:
		case CCharacterCommand::CC_IfElse:
		case CCharacterCommand::CC_IfElseIf:
		case CCharacterCommand::CC_IfEnd:
		case CCharacterCommand::CC_Return:
		break;

		default: break;
	}

	return wstr;
}

//*****************************************************************************
HoldCharacter* CCharacterDialogWidget::GetCustomCharacter()
//Return: pointer to selected custom character record
{
	const UINT dwSelectedCharID = this->pCharListBox->GetSelectedItem();
	if (!dwSelectedCharID)
		return NULL; //no character selected

	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen->pHold);
	return pEditRoomScreen->pHold->GetCharacter(dwSelectedCharID);
}

//*****************************************************************************
WSTRING CCharacterDialogWidget::GetDataName(const UINT dwID) const
//Load name of data object from DB.
{
	WSTRING wstr = g_pTheDB->Data.GetNameFor(dwID);
	if (wstr.empty())
		wstr = wszQuestionMark;
	return wstr;
}

//*****************************************************************************
UINT CCharacterDialogWidget::ExtractCommandIndent(
	const CListBoxWidget* pCommandList,
	const UINT wCommandIndex) const
//Extracts command's indent size from the command listbox text
{
	WSTRING wstr = pCommandList->GetTextAtLine(wCommandIndex);

	UINT i = 0;
	for (; i < wstr.size(); ++i)
	{
		if (!iswspace(wstr.at(i)))
			break;
	}

	return i;
}

//*****************************************************************************
void CCharacterDialogWidget::PrettyPrintCommands(CListBoxWidget* pCommandList, const COMMANDPTR_VECTOR &commands)
{
	WSTRING wstr;

	UINT wNestDepth = 0, wIndent = INDENT_PREFIX_SIZE;  //insert past labels

	UINT index = 0;
	bool bLastWasIfCondition = false;
	for (COMMANDPTR_VECTOR::const_iterator command = commands.begin();
		command != commands.end(); ++command, ++index)
	{
		wstr.clear();
		CCharacterCommand *pCommand = *command;
		bool bIsIfCondition = false;
		bool bUndoOneDepth = false;
		bool bIsLabel = false;
		switch (pCommand->command)
		{
			case CCharacterCommand::CC_If:
				bIsIfCondition = true;
				++wNestDepth; //indent inside of if block
				bUndoOneDepth = true;
				break;
			case CCharacterCommand::CC_IfElseIf:
				bIsIfCondition = true;
				break;
			case CCharacterCommand::CC_IfEnd:
				if (wNestDepth)
					--wNestDepth;
				else
					wstr += wszExclamation;	//superfluous IfEnd
				break;
			default: break;
		}

		//Unnest If block markers.
		switch (pCommand->command)
		{
			case CCharacterCommand::CC_IfElse:
			case CCharacterCommand::CC_IfElseIf:
				if (wNestDepth)
					bUndoOneDepth = true;
				else
					wstr += wszExclamation;	//superfluous IfEnd
			//no break
			case CCharacterCommand::CC_IfEnd:
			case CCharacterCommand::CC_Disappear:
			case CCharacterCommand::CC_MoveTo:
			case CCharacterCommand::CC_MoveRel:
			case CCharacterCommand::CC_EndScript:
			case CCharacterCommand::CC_EndScriptOnExit:
			case CCharacterCommand::CC_FlushSpeech:
			case CCharacterCommand::CC_GoSub:
			case CCharacterCommand::CC_GoTo:
			case CCharacterCommand::CC_If:
			case CCharacterCommand::CC_Imperative:
			case CCharacterCommand::CC_Behavior:
			case CCharacterCommand::CC_LevelEntrance:
			case CCharacterCommand::CC_SetMusic:
			case CCharacterCommand::CC_Speech:
			case CCharacterCommand::CC_TurnIntoMonster:
			case CCharacterCommand::CC_ReplaceWithDefault:
			case CCharacterCommand::CC_PlayerEquipsWeapon:
			case CCharacterCommand::CC_SetPlayerStealth:
			case CCharacterCommand::CC_SetWaterTraversal:
			case CCharacterCommand::CC_StartGlobalScript:
			case CCharacterCommand::CC_AnswerOption:
			case CCharacterCommand::CC_AmbientSound:
			case CCharacterCommand::CC_AmbientSoundAt:
			case CCharacterCommand::CC_PlayVideo:
			case CCharacterCommand::CC_WorldMapSelect:
			case CCharacterCommand::CC_WorldMapMusic:
			case CCharacterCommand::CC_WorldMapIcon:
			case CCharacterCommand::CC_WorldMapImage:
			case CCharacterCommand::CC_ChallengeCompleted:
			case CCharacterCommand::CC_Return:
				if (bLastWasIfCondition)
					wstr += wszQuestionMark;	//questionable If condition
				break;
			case CCharacterCommand::CC_Label:
				bIsLabel = true;
				if (bLastWasIfCondition)
					wstr += wszQuestionMark;	//questionable If condition
				break;
			case CCharacterCommand::CC_ImageOverlay:
				if (bLastWasIfCondition)
					wstr += wszQuestionMark;	//questionable If condition
				if (pCommand->label.empty()) {
					wstr += wszExclamation;
				}
				else {
					vector<ImageOverlayCommand> temp;
					if (!CImageOverlay::parse(pCommand->label, temp))
						wstr += wszExclamation;
				}
				break;

			case CCharacterCommand::CC_VarSet:
				if (bLastWasIfCondition)
					wstr += wszQuestionMark;	//questionable If condition
			//no break
			case CCharacterCommand::CC_WaitForVar:
			{
				//Verify integrity of hold var refs.
				switch (pCommand->y)
				{
				case ScriptVars::AppendText:
				case ScriptVars::AssignText:
					break;
				default:
					if (!pCommand->label.empty()) //an expression is used as an operand
					{
						CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen *, CScreen *,
							g_pTheSM->GetScreen(SCR_EditRoom));
						ASSERT(pEditRoomScreen);
						ASSERT(pEditRoomScreen->pHold);
						UINT index = 0;
						if (!CCharacter::IsValidExpression(pCommand->label.c_str(), index, pEditRoomScreen->pHold))
							wstr += wszAsterisk; //expression is not valid
					}
					break;
				}
			}
			break;

			//Deprecated commands.
			case CCharacterCommand::CC_GotoIf:
			case CCharacterCommand::CC_WaitForHalph:
			case CCharacterCommand::CC_WaitForNotHalph:
			case CCharacterCommand::CC_WaitForMonster:
			case CCharacterCommand::CC_WaitForNotMonster:
			case CCharacterCommand::CC_WaitForCharacter:
			case CCharacterCommand::CC_WaitForNotCharacter:
				wstr += wszAsterisk;
				break;
			default: break;
		}

		UINT wFinalIndent = wNestDepth * INDENT_TAB_SIZE;
		if (bIsLabel)
			wFinalIndent = 0;

		else if (bLastWasIfCondition)
			wFinalIndent += INDENT_IF_CONDITION_SIZE;

		else if (bUndoOneDepth)
			wFinalIndent -= INDENT_TAB_SIZE;

		wstr.insert(wstr.begin(), bIsLabel ? wIndent - 2 : wIndent, W_t(' '));
		wstr.insert(wstr.end(), wFinalIndent, W_t(' '));
		wstr += pCommandList->GetTextAtLine(index);
		pCommandList->SetItemTextAtLine(index, wstr.c_str());

		bLastWasIfCondition = bIsIfCondition;
	}
}

//*****************************************************************************
WSTRING CCharacterDialogWidget::GetEntranceName(
	CEditRoomScreen *pEditRoomScreen,
	UINT entranceID)
const
{
	WSTRING wstr;
	if (LevelExit::IsWorldMapID(entranceID)) {
		wstr += GetWorldMapNameText(pEditRoomScreen, LevelExit::ConvertWorldMapID(entranceID));
	} else {
		CEntranceData *pEntrance = pEditRoomScreen->pHold->GetEntrance(entranceID);
		if (!pEntrance) {
			wstr += g_pTheDB->GetMessageText(MID_DefaultExit);
		} else {
			wstr += wszQuote;
			wstr += pEntrance->GetPositionDescription();
			wstr += wszQuote;
		}
	}
	return wstr;
}

void CCharacterDialogWidget::AppendGotoDestination(WSTRING& wstr,
	const COMMANDPTR_VECTOR& commands, const CCharacterCommand& pCommand
) const
{
	int label = pCommand.x;
	if (label < 0) {
		switch (label) {
		case ScriptFlag::GotoSmartType::PreviousIf:
			wstr += g_pTheDB->GetMessageText(MID_PreviousIf);
		break;
		case ScriptFlag::GotoSmartType::NextElseOrElseIfSkipCondition:
			wstr += g_pTheDB->GetMessageText(MID_NextElseOrElseIfSkip);
		break;
		default:
			wstr += wszQuestionMark;
		}

		return;
	}

	const CCharacterCommand* pGotoCommand = GetCommandWithLabel(commands, label);
	wstr += pGotoCommand ? pGotoCommand->label : wszQuestionMark;
}

WSTRING CCharacterDialogWidget::GetWorldMapNameText(CEditRoomScreen *pEditRoomScreen, UINT worldMapID) const
{
	WSTRING wstr;

	ASSERT(pEditRoomScreen->pHold);

	const WSTRING name = pEditRoomScreen->pHold->GetWorldMapName(worldMapID);
	if (name.empty()) {
		wstr += wszQuestionMark;
	} else {
		wstr += wszQuote;
		wstr += name;
		wstr += wszQuote;
	}
	return wstr;
}

//*****************************************************************************
void CCharacterDialogWidget::PopulateImperativeListBox(const bool /*bDefaultScript*/) //[default=false]
//Some imperatives are only supported in default NPC scripts.
{
	this->pImperativeListBox->Clear();

	this->pImperativeListBox->AddItem(ScriptFlag::Vulnerable, g_pTheDB->GetMessageText(MID_Vulnerable));
	this->pImperativeListBox->AddItem(ScriptFlag::Invulnerable, g_pTheDB->GetMessageText(MID_Invulnerable));
	this->pImperativeListBox->AddItem(ScriptFlag::MissionCritical, g_pTheDB->GetMessageText(MID_MissionCritical));
	this->pImperativeListBox->AddItem(ScriptFlag::RequiredToConquer, g_pTheDB->GetMessageText(MID_RequiredToConquer));
	this->pImperativeListBox->AddItem(ScriptFlag::Safe, g_pTheDB->GetMessageText(MID_Safe));
	this->pImperativeListBox->AddItem(ScriptFlag::SwordSafeToPlayer, g_pTheDB->GetMessageText(MID_SwordSafeToPlayer));
	this->pImperativeListBox->AddItem(ScriptFlag::Deadly, g_pTheDB->GetMessageText(MID_Deadly));
	this->pImperativeListBox->AddItem(ScriptFlag::Friendly, g_pTheDB->GetMessageText(MID_Friendly));
	this->pImperativeListBox->AddItem(ScriptFlag::Unfriendly, g_pTheDB->GetMessageText(MID_Unfriendly));
	this->pImperativeListBox->AddItem(ScriptFlag::Die, g_pTheDB->GetMessageText(MID_Die));
	this->pImperativeListBox->AddItem(ScriptFlag::DieSpecial, g_pTheDB->GetMessageText(MID_DieSpecial));
	this->pImperativeListBox->AddItem(ScriptFlag::EndWhenKilled, g_pTheDB->GetMessageText(MID_EndWhenKilled));
	this->pImperativeListBox->AddItem(ScriptFlag::DirectBeelining, g_pTheDB->GetMessageText(MID_DirectBeelining));
	this->pImperativeListBox->AddItem(ScriptFlag::NormalBeelining, g_pTheDB->GetMessageText(MID_NormalBeelining));
	this->pImperativeListBox->AddItem(ScriptFlag::SmartBeelining, g_pTheDB->GetMessageText(MID_SmartBeelining));
	this->pImperativeListBox->AddItem(ScriptFlag::FlexibleBeelining, g_pTheDB->GetMessageText(MID_FlexibleBeelining));
	this->pImperativeListBox->AddItem(ScriptFlag::PathfindingOpenOnly, g_pTheDB->GetMessageText(MID_PathfindingOpenOnlyMovement));
	this->pImperativeListBox->AddItem(ScriptFlag::Pathfinding, g_pTheDB->GetMessageText(MID_PathfindingMovement));
	this->pImperativeListBox->AddItem(ScriptFlag::BrainPathmapObstacle, g_pTheDB->GetMessageText(MID_BrainPathmapObstacle));
	this->pImperativeListBox->AddItem(ScriptFlag::NotBrainPathmapObstacle, g_pTheDB->GetMessageText(MID_BrainPathmapNotObstacle));
	this->pImperativeListBox->AddItem(ScriptFlag::NPCPathmapObstacle, g_pTheDB->GetMessageText(MID_NPCPathmapObstacle));
	this->pImperativeListBox->AddItem(ScriptFlag::NotNPCPathmapObstacle, g_pTheDB->GetMessageText(MID_NPCPathmapNotObstacle));
	this->pImperativeListBox->AddItem(ScriptFlag::GhostDisplay, g_pTheDB->GetMessageText(MID_NPCGhostDisplay));
	this->pImperativeListBox->AddItem(ScriptFlag::GhostDisplayOverhead, g_pTheDB->GetMessageText(MID_NPCGhostDisplayOverhead));
	this->pImperativeListBox->AddItem(ScriptFlag::NoGhostDisplay, g_pTheDB->GetMessageText(MID_NPCNoGhostDisplay));
	this->pImperativeListBox->AddItem(ScriptFlag::InvisibleInspectable, g_pTheDB->GetMessageText(MID_NPCInvisibleInspectable));
	this->pImperativeListBox->AddItem(ScriptFlag::InvisibleNotInspectable, g_pTheDB->GetMessageText(MID_NPCInvisibleNotInspectable));
	this->pImperativeListBox->AddItem(ScriptFlag::InvisibleCountMoveOrder, g_pTheDB->GetMessageText(MID_NPCInvisibleIncludeMoveOrder));
	this->pImperativeListBox->AddItem(ScriptFlag::InvisibleNotCountMoveOrder, g_pTheDB->GetMessageText(MID_NPCInvisibleNotIncludeMoveOrder));
	this->pImperativeListBox->AddItem(ScriptFlag::NotPushable, g_pTheDB->GetMessageText(MID_NPCNotPushable));
	this->pImperativeListBox->AddItem(ScriptFlag::DefaultPushability, g_pTheDB->GetMessageText(MID_NPCDefaultPushableBehavior));
	this->pImperativeListBox->AddItem(ScriptFlag::PushableByBody, g_pTheDB->GetMessageText(MID_NPCPushableByBody));
	this->pImperativeListBox->AddItem(ScriptFlag::PushableByWeapon, g_pTheDB->GetMessageText(MID_NPCPushableByWeapon));
	this->pImperativeListBox->AddItem(ScriptFlag::PushableByBoth, g_pTheDB->GetMessageText(MID_NPCPushableByBoth));
	this->pImperativeListBox->AddItem(ScriptFlag::Stunnable, g_pTheDB->GetMessageText(MID_NPCStunnable));
	this->pImperativeListBox->AddItem(ScriptFlag::NotStunnable, g_pTheDB->GetMessageText(MID_NPCNotStunnable));
	this->pImperativeListBox->SelectLine(0);
	this->pImperativeListBox->SetAllowFiltering(true);
}

//*****************************************************************************
void CCharacterDialogWidget::PopulateBehaviorListBox()
{
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::ActivateTokens, g_pTheDB->GetMessageText(MID_ActivateToken));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::ActivatePlates, g_pTheDB->GetMessageText(MID_ActivatePlates));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::DropTrapdoors, g_pTheDB->GetMessageText(MID_DropTrapdoors));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::DropTrapdoorsArmed, g_pTheDB->GetMessageText(MID_DropTrapdoorsArmed));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::PushObjects, g_pTheDB->GetMessageText(MID_PushObjects));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::PushMonsters, g_pTheDB->GetMessageText(MID_PushMonsters));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::MovePlatforms, g_pTheDB->GetMessageText(MID_MovePlatforms));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::RestrictedMovement, g_pTheDB->GetMessageText(MID_RestrictedMovement));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::MonsterAttackable, g_pTheDB->GetMessageText(MID_CanBeMonsterAttacked));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::MonsterTarget, g_pTheDB->GetMessageText(MID_MonsterTarget));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::MonsterTargetWhenPlayerIsTarget, g_pTheDB->GetMessageText(MID_MonsterTargetWhenPlayerIsTarget));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::AllyTarget, g_pTheDB->GetMessageText(MID_AllyTarget));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::PuffTarget, g_pTheDB->GetMessageText(MID_PuffTarget));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::SwordDamageImmune, g_pTheDB->GetMessageText(MID_SwordDamageImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::PickaxeDamageImmune, g_pTheDB->GetMessageText(MID_PickaxeDamageImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::SpearDamageImmune, g_pTheDB->GetMessageText(MID_SpearDamageImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::DaggerDamageImmune, g_pTheDB->GetMessageText(MID_DaggerDamageImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::CaberDamageImmune, g_pTheDB->GetMessageText(MID_CaberDamageImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::FloorSpikeImmune, g_pTheDB->GetMessageText(MID_FloorSpikeImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::FiretrapImmune, g_pTheDB->GetMessageText(MID_FiretrapImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::HotTileImmune, g_pTheDB->GetMessageText(MID_HotTileImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::ExplosionImmune, g_pTheDB->GetMessageText(MID_ExplosionImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::BriarImmune, g_pTheDB->GetMessageText(MID_BriarImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::AdderImmune, g_pTheDB->GetMessageText(MID_AdderImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::PuffImmune, g_pTheDB->GetMessageText(MID_PuffImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::FatalPushImmune, g_pTheDB->GetMessageText(MID_FatalPushImmune));
	this->pBehaviorListBox->AddItem(ScriptFlag::Behavior::CanBeNPCBeethro, g_pTheDB->GetMessageText(MID_CanBeNPCBeethro));
	this->pBehaviorListBox->SelectLine(0);
	this->pBehaviorListBox->SetAllowFiltering(true);
}

//*****************************************************************************
void CCharacterDialogWidget::PopulateCommandListBox()
//Add to list all usable script commands.
{

	this->pActionListBox->AddItem(CCharacterCommand::CC_ActivateItemAt, g_pTheDB->GetMessageText(MID_StrikeOrbAt));
	this->pActionListBox->AddItem(CCharacterCommand::CC_AmbientSound, g_pTheDB->GetMessageText(MID_AmbientSound));
	this->pActionListBox->AddItem(CCharacterCommand::CC_AmbientSoundAt, g_pTheDB->GetMessageText(MID_AmbientSoundAt));
	this->pActionListBox->AddItem(CCharacterCommand::CC_AnswerOption, g_pTheDB->GetMessageText(MID_AnswerOption));
	this->pActionListBox->AddItem(CCharacterCommand::CC_Appear, g_pTheDB->GetMessageText(MID_Appear));
	this->pActionListBox->AddItem(CCharacterCommand::CC_AppearAt, g_pTheDB->GetMessageText(MID_AppearAt));
	this->pActionListBox->AddItem(CCharacterCommand::CC_AttackTile, g_pTheDB->GetMessageText(MID_AttackTile));
	this->pActionListBox->AddItem(CCharacterCommand::CC_Build, g_pTheDB->GetMessageText(MID_BuildCommand));
	this->pActionListBox->AddItem(CCharacterCommand::CC_BuildMarker, g_pTheDB->GetMessageText(MID_BuildMarker));
	this->pActionListBox->AddItem(CCharacterCommand::CC_ChallengeCompleted, g_pTheDB->GetMessageText(MID_ChallengeCompleted));
	this->pActionListBox->AddItem(CCharacterCommand::CC_CutScene, g_pTheDB->GetMessageText(MID_CutScene));
	this->pActionListBox->AddItem(CCharacterCommand::CC_DestroyTrapdoor, g_pTheDB->GetMessageText(MID_DestroyTrapdoor));
	this->pActionListBox->AddItem(CCharacterCommand::CC_Disappear, g_pTheDB->GetMessageText(MID_Disappear));
	this->pActionListBox->AddItem(CCharacterCommand::CC_DisplayFilter, g_pTheDB->GetMessageText(MID_DisplayFilter));
	this->pActionListBox->AddItem(CCharacterCommand::CC_EndScript, g_pTheDB->GetMessageText(MID_EndScript));
	this->pActionListBox->AddItem(CCharacterCommand::CC_EndScriptOnExit, g_pTheDB->GetMessageText(MID_EndScriptOnExit));
	this->pActionListBox->AddItem(CCharacterCommand::CC_FaceDirection, g_pTheDB->GetMessageText(MID_FaceDirection));
	this->pActionListBox->AddItem(CCharacterCommand::CC_FaceTowards, g_pTheDB->GetMessageText(MID_CharacterFaceTowards));
	this->pActionListBox->AddItem(CCharacterCommand::CC_GetNaturalTarget, g_pTheDB->GetMessageText(MID_GetNaturalTarget));
	this->pActionListBox->AddItem(CCharacterCommand::CC_GetEntityDirection, g_pTheDB->GetMessageText(MID_GetEntityDirection));
	this->pActionListBox->AddItem(CCharacterCommand::CC_FlashingText, g_pTheDB->GetMessageText(MID_FlashingMessage));
	this->pActionListBox->AddItem(CCharacterCommand::CC_FlushSpeech, g_pTheDB->GetMessageText(MID_FlushSpeech));
	this->pActionListBox->AddItem(CCharacterCommand::CC_GameEffect, g_pTheDB->GetMessageText(MID_VisualEffect));
	this->pActionListBox->AddItem(CCharacterCommand::CC_GenerateEntity, g_pTheDB->GetMessageText(MID_GenerateEntity));
	this->pActionListBox->AddItem(CCharacterCommand::CC_GoSub, g_pTheDB->GetMessageText(MID_GoSub));
	this->pActionListBox->AddItem(CCharacterCommand::CC_GoTo, g_pTheDB->GetMessageText(MID_GoTo));
	this->pActionListBox->AddItem(CCharacterCommand::CC_LevelEntrance, g_pTheDB->GetMessageText(MID_GotoLevelEntrance));
	this->pActionListBox->AddItem(CCharacterCommand::CC_If, g_pTheDB->GetMessageText(MID_If));
	this->pActionListBox->AddItem(CCharacterCommand::CC_IfElse, g_pTheDB->GetMessageText(MID_IfElse));
	this->pActionListBox->AddItem(CCharacterCommand::CC_IfElseIf, g_pTheDB->GetMessageText(MID_IfElseIf));
	this->pActionListBox->AddItem(CCharacterCommand::CC_IfEnd, g_pTheDB->GetMessageText(MID_IfEnd));
	this->pActionListBox->AddItem(CCharacterCommand::CC_ImageOverlay, g_pTheDB->GetMessageText(MID_ImageOverlay));
	this->pActionListBox->AddItem(CCharacterCommand::CC_Imperative, g_pTheDB->GetMessageText(MID_Imperative));
	this->pActionListBox->AddItem(CCharacterCommand::CC_Behavior, g_pTheDB->GetMessageText(MID_Behavior));
	this->pActionListBox->AddItem(CCharacterCommand::CC_Label, g_pTheDB->GetMessageText(MID_Label));
	this->pActionListBox->AddItem(CCharacterCommand::CC_MoveRel, g_pTheDB->GetMessageText(MID_MoveRel));
	this->pActionListBox->AddItem(CCharacterCommand::CC_MoveTo, g_pTheDB->GetMessageText(MID_MoveTo));
	this->pActionListBox->AddItem(CCharacterCommand::CC_PlayVideo, g_pTheDB->GetMessageText(MID_PlayVideo));
	this->pActionListBox->AddItem(CCharacterCommand::CC_PushTile, g_pTheDB->GetMessageText(MID_PushTile));
	this->pActionListBox->AddItem(CCharacterCommand::CC_Question, g_pTheDB->GetMessageText(MID_Question));
	this->pActionListBox->AddItem(CCharacterCommand::CC_Return, g_pTheDB->GetMessageText(MID_ReturnCommand));
	this->pActionListBox->AddItem(CCharacterCommand::CC_RoomLocationText, g_pTheDB->GetMessageText(MID_RoomLocationText));
	this->pActionListBox->AddItem(CCharacterCommand::CC_SetNPCAppearance, g_pTheDB->GetMessageText(MID_SetNPCAppearance));
	this->pActionListBox->AddItem(CCharacterCommand::CC_SetMovementType, g_pTheDB->GetMessageText(MID_SetMovementType));
	this->pActionListBox->AddItem(CCharacterCommand::CC_SetMusic, g_pTheDB->GetMessageText(MID_SetMusic));
	this->pActionListBox->AddItem(CCharacterCommand::CC_SetPlayerAppearance, g_pTheDB->GetMessageText(MID_SetPlayerAppearance));
	this->pActionListBox->AddItem(CCharacterCommand::CC_SetPlayerStealth, g_pTheDB->GetMessageText(MID_SetPlayerStealth));
	this->pActionListBox->AddItem(CCharacterCommand::CC_SetPlayerWeapon, g_pTheDB->GetMessageText(MID_SetPlayerWeapon));
	this->pActionListBox->AddItem(CCharacterCommand::CC_SetWaterTraversal, g_pTheDB->GetMessageText(MID_SetWaterTraversal));
	this->pActionListBox->AddItem(CCharacterCommand::CC_VarSet, g_pTheDB->GetMessageText(MID_VarSet));
	this->pActionListBox->AddItem(CCharacterCommand::CC_Speech, g_pTheDB->GetMessageText(MID_Speech));
	this->pActionListBox->AddItem(CCharacterCommand::CC_StartGlobalScript, g_pTheDB->GetMessageText(MID_StartGlobalScript));
	this->pActionListBox->AddItem(CCharacterCommand::CC_TeleportTo, g_pTheDB->GetMessageText(MID_TeleportTo));
	this->pActionListBox->AddItem(CCharacterCommand::CC_TeleportPlayerTo, g_pTheDB->GetMessageText(MID_TeleportPlayerTo));
	this->pActionListBox->AddItem(CCharacterCommand::CC_TurnIntoMonster, g_pTheDB->GetMessageText(MID_TurnIntoMonster));
	this->pActionListBox->AddItem(CCharacterCommand::CC_ReplaceWithDefault, g_pTheDB->GetMessageText(MID_ReplaceWithDefault));

	this->pActionListBox->AddItem(CCharacterCommand::CC_Wait, g_pTheDB->GetMessageText(MID_Wait));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForCleanLevel, g_pTheDB->GetMessageText(MID_WaitForCleanLevel));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForCleanRoom, g_pTheDB->GetMessageText(MID_WaitForCleanRoom));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForDoorTo, g_pTheDB->GetMessageText(MID_WaitForDoorTo));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForRect, g_pTheDB->GetMessageText(MID_WaitForEntity));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForEntityType, g_pTheDB->GetMessageText(MID_WaitForEntityType));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForCueEvent, g_pTheDB->GetMessageText(MID_WaitForEvent));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForItem, g_pTheDB->GetMessageText(MID_WaitForItem));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForNoBuilding, g_pTheDB->GetMessageText(MID_WaitForNoBuilding));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForOpenMove, g_pTheDB->GetMessageText(MID_WaitForOpenMove));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForPlayerToFace, g_pTheDB->GetMessageText(MID_WaitForPlayerToFace));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForPlayerInput, g_pTheDB->GetMessageText(MID_WaitForPlayerToInput));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForPlayerToMove, g_pTheDB->GetMessageText(MID_WaitForPlayerToMove));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForPlayerToTouchMe, g_pTheDB->GetMessageText(MID_WaitForPlayerToTouchMe));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForSomeoneToPushMe, g_pTheDB->GetMessageText(MID_WaitForSomeoneToPushMe));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForTurn, g_pTheDB->GetMessageText(MID_WaitForTurn));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForVar, g_pTheDB->GetMessageText(MID_WaitForVar));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForNotRect, g_pTheDB->GetMessageText(MID_WaitWhileEntity));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForNotEntityType, g_pTheDB->GetMessageText(MID_WaitWhileEntityType));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForRemains, g_pTheDB->GetMessageText(MID_WaitForRemains));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WaitForWeapon, g_pTheDB->GetMessageText(MID_WaitForWeapon));

	this->pActionListBox->AddItem(CCharacterCommand::CC_WorldMapIcon, g_pTheDB->GetMessageText(MID_WorldMapIcon));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WorldMapImage, g_pTheDB->GetMessageText(MID_WorldMapImage));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WorldMapMusic, g_pTheDB->GetMessageText(MID_WorldMapMusic));
	this->pActionListBox->AddItem(CCharacterCommand::CC_WorldMapSelect, g_pTheDB->GetMessageText(MID_WorldMapSelect));

	this->pActionListBox->SelectLine(0);
	this->pActionListBox->SetAllowFiltering(true);
}

//*****************************************************************************
void CCharacterDialogWidget::PopulateEventListBox()
//Add all cue events that character can check for to event list.
{
	this->pEventListBox->AddItem(CID_AllBrainsRemoved, g_pTheDB->GetMessageText(MID_AllBrainsRemoved));
	this->pEventListBox->AddItem(CID_AllTarRemoved, g_pTheDB->GetMessageText(MID_AllTarRemoved));
	this->pEventListBox->AddItem(CID_AllTrapdoorsRemoved, g_pTheDB->GetMessageText(MID_AllTrapdoorsRemoved));
	this->pEventListBox->AddItem(CID_AshToFegundo, g_pTheDB->GetMessageText(MID_AshToFegundo));
	this->pEventListBox->AddItem(CID_BombExploded, g_pTheDB->GetMessageText(MID_BombExploded));
	this->pEventListBox->AddItem(CID_BriarExpanded, g_pTheDB->GetMessageText(MID_FlowExpanded));
	this->pEventListBox->AddItem(CID_CommandKeyPressed, g_pTheDB->GetMessageText(MID_CommandKeyPressed));
	this->pEventListBox->AddItem(CID_ConstructReanimated, g_pTheDB->GetMessageText(MID_ConstructReanimated));
	this->pEventListBox->AddItem(CID_CrumblyWallDestroyed, g_pTheDB->GetMessageText(MID_CrumblyWallDestroyed));
	this->pEventListBox->AddItem(CID_CutBriar, g_pTheDB->GetMessageText(MID_CutBriar));
	this->pEventListBox->AddItem(CID_DoublePlaced, g_pTheDB->GetMessageText(MID_DoublePlaced));
	this->pEventListBox->AddItem(CID_EggHatched, g_pTheDB->GetMessageText(MID_EggHatched));
	this->pEventListBox->AddItem(CID_EggSpawned, g_pTheDB->GetMessageText(MID_EggSpawned));
	this->pEventListBox->AddItem(CID_EvilEyeWoke, g_pTheDB->GetMessageText(MID_EvilEyeWoke));
	this->pEventListBox->AddItem(CID_FegundoToAsh, g_pTheDB->GetMessageText(MID_FegundoToAsh));
	this->pEventListBox->AddItem(CID_FiretrapActivated, g_pTheDB->GetMessageText(MID_FiretrapActivated));
	this->pEventListBox->AddItem(CID_Firetrap, g_pTheDB->GetMessageText(MID_FiretrapBurning));
	this->pEventListBox->AddItem(CID_FluffDestroyed, g_pTheDB->GetMessageText(MID_FluffDestroyed));
	this->pEventListBox->AddItem(CID_FluffPuffDestroyed, g_pTheDB->GetMessageText(MID_FluffPuffDestroyed));
	this->pEventListBox->AddItem(CID_FuseBurning, g_pTheDB->GetMessageText(MID_FuseBurning));
	this->pEventListBox->AddItem(CID_GelBabyFormed, g_pTheDB->GetMessageText(MID_GelBabyFormed));
	this->pEventListBox->AddItem(CID_GelGrew, g_pTheDB->GetMessageText(MID_GelGrew));
	this->pEventListBox->AddItem(CID_HalphEntered, g_pTheDB->GetMessageText(MID_HalphEntered));
	this->pEventListBox->AddItem(CID_HitObstacle, g_pTheDB->GetMessageText(MID_HitObstacle));
	this->pEventListBox->AddItem(CID_LightToggled, g_pTheDB->GetMessageText(MID_LightToggled));
	this->pEventListBox->AddItem(CID_MirrorShattered, g_pTheDB->GetMessageText(MID_MirrorShattered));
	this->pEventListBox->AddItem(CID_MonsterBurned, g_pTheDB->GetMessageText(MID_MonsterBurned));
	this->pEventListBox->AddItem(CID_MonsterDiedFromStab, g_pTheDB->GetMessageText(MID_MonsterStabbed));
	this->pEventListBox->AddItem(CID_MonsterPieceStabbed, g_pTheDB->GetMessageText(MID_MonsterPieceStabbed));
	this->pEventListBox->AddItem(CID_MudBabyFormed, g_pTheDB->GetMessageText(MID_MudBabyFormed));
	this->pEventListBox->AddItem(CID_MudGrew, g_pTheDB->GetMessageText(MID_MudGrew));
	this->pEventListBox->AddItem(CID_NPCKilled, g_pTheDB->GetMessageText(MID_NPCKilled));
	this->pEventListBox->AddItem(CID_ObjectBuilt, g_pTheDB->GetMessageText(MID_ObjectBuilt));
	this->pEventListBox->AddItem(CID_ObjectFell, g_pTheDB->GetMessageText(MID_ObjectFell));
	this->pEventListBox->AddItem(CID_OrbActivatedByPlayer, g_pTheDB->GetMessageText(MID_PlayerHitsOrb));
	this->pEventListBox->AddItem(CID_OrbActivated, g_pTheDB->GetMessageText(MID_ItemHitsOrb));
	this->pEventListBox->AddItem(CID_OrbActivatedByDouble, g_pTheDB->GetMessageText(MID_MonsterHitsOrb));
	this->pEventListBox->AddItem(CID_OrbDamaged, g_pTheDB->GetMessageText(MID_OrbDamaged));
	this->pEventListBox->AddItem(CID_PlayerFrozen, g_pTheDB->GetMessageText(MID_PlayerFrozen));
	this->pEventListBox->AddItem(CID_PressurePlate, g_pTheDB->GetMessageText(MID_PressurePlateActivated));
	this->pEventListBox->AddItem(CID_PressurePlateReleased, g_pTheDB->GetMessageText(MID_PressurePlateReleased));
	this->pEventListBox->AddItem(CID_PuffMergedIntoFluff, g_pTheDB->GetMessageText(MID_PuffMergedIntoFluff));
	this->pEventListBox->AddItem(CID_RoomConquerPending, g_pTheDB->GetMessageText(MID_ConquerRoomPending));
	this->pEventListBox->AddItem(CID_Scared, g_pTheDB->GetMessageText(MID_Scared));
	this->pEventListBox->AddItem(CID_SeedingBeaconActivated, g_pTheDB->GetMessageText(MID_SeedingBeaconActivated));
	this->pEventListBox->AddItem(CID_SeedingBeaconDeactivated, g_pTheDB->GetMessageText(MID_SeedingBeaconDeactivated));
	this->pEventListBox->AddItem(CID_SlayerEntered, g_pTheDB->GetMessageText(MID_SlayerEntered));
	this->pEventListBox->AddItem(CID_SnakeDiedFromTruncation, g_pTheDB->GetMessageText(MID_SnakeDiedFromTruncation));
	this->pEventListBox->AddItem(CID_SpikesPoised, g_pTheDB->GetMessageText(MID_FloorSpikesPoised));
	this->pEventListBox->AddItem(CID_SpikesUp, g_pTheDB->GetMessageText(MID_FloorSpikesUp));
	this->pEventListBox->AddItem(CID_Splash, g_pTheDB->GetMessageText(MID_Splash));
	this->pEventListBox->AddItem(CID_SwingSword, g_pTheDB->GetMessageText(MID_PlayerSwingsSword));
	this->pEventListBox->AddItem(CID_TarBabyFormed, g_pTheDB->GetMessageText(MID_TarBabyFormed));
	this->pEventListBox->AddItem(CID_TarstuffDestroyed, g_pTheDB->GetMessageText(MID_TarDestroyed));
	this->pEventListBox->AddItem(CID_TarstuffStabbed, g_pTheDB->GetMessageText(MID_TarstuffStabbed));
	this->pEventListBox->AddItem(CID_TarGrew, g_pTheDB->GetMessageText(MID_TarGrew));
	this->pEventListBox->AddItem(CID_ThinIceMelted, g_pTheDB->GetMessageText(MID_ThinIceMelted));
	this->pEventListBox->AddItem(CID_TokenToggled, g_pTheDB->GetMessageText(MID_TokenToggled));
	this->pEventListBox->AddItem(CID_TrapDoorRemoved, g_pTheDB->GetMessageText(MID_TrapDoorRemoved));
	this->pEventListBox->AddItem(CID_Tunnel, g_pTheDB->GetMessageText(MID_Tunnel));
	this->pEventListBox->AddItem(CID_WispOnPlayer, g_pTheDB->GetMessageText(MID_WispOnPlayer));
	this->pEventListBox->AddItem(CID_WubbaStabbed, g_pTheDB->GetMessageText(MID_WubbaStabbed));
	
	this->pEventListBox->SelectLine(0);
	this->pEventListBox->SetAllowFiltering(true);
}

//*****************************************************************************
void CCharacterDialogWidget::PopulateItemListBox(CListBoxWidget *pListBox, 
	const bool bIsBuild, const bool bIsBuildMarker, const bool bIsWaitForItem)
//Add all common items for build and wait for item lists
{
	ASSERT(pListBox);

	if (bIsBuild || bIsBuildMarker){
		pListBox->AddItem(T_EMPTY, g_pTheDB->GetMessageText(MID_RemoveItem));
		pListBox->AddItem(T_REMOVE_FLOOR_ITEM, g_pTheDB->GetMessageText(MID_RemoveFloorLayer));
	}

	if (bIsBuild){
		pListBox->AddItem(T_REMOVE_OVERHEAD_IMAGE, g_pTheDB->GetMessageText(MID_RemoveOverheadImage));
	}

	if (bIsBuildMarker){
		pListBox->AddItem(T_REMOVE_BUILD_MARKER, g_pTheDB->GetMessageText(MID_None));
	}

	if (bIsWaitForItem){
		pListBox->AddItem(T_DARK_CEILING, g_pTheDB->GetMessageText(MID_DarkCeiling));
		pListBox->AddItem(T_LIGHT, g_pTheDB->GetMessageText(MID_Light));
		pListBox->AddItem(T_OBSTACLE, g_pTheDB->GetMessageText(MID_Obstacle));
		pListBox->AddItem(T_ORB, g_pTheDB->GetMessageText(MID_OrbWaitAny));
		pListBox->AddItem(T_PLATE_MULTI, g_pTheDB->GetMessageText(MID_PressurePlate));
		pListBox->AddItem(T_PLATE_ON_OFF, g_pTheDB->GetMessageText(MID_PressurePlateToggle));
		pListBox->AddItem(T_PLATE_ONEUSE, g_pTheDB->GetMessageText(MID_PressurePlateOneUse));
		pListBox->AddItem(T_PLATFORM_P, g_pTheDB->GetMessageText(MID_PlatformPit));
		pListBox->AddItem(T_PLATFORM_W, g_pTheDB->GetMessageText(MID_PlatformWater));
		pListBox->AddItem(T_LIGHT_CEILING, g_pTheDB->GetMessageText(MID_LightCeiling));
		pListBox->AddItem(T_SCROLL, g_pTheDB->GetMessageText(MID_Scroll));
		pListBox->AddItem(T_STAIRS, g_pTheDB->GetMessageText(MID_Stairs));
		pListBox->AddItem(T_STAIRS_UP, g_pTheDB->GetMessageText(MID_StairsUp));
		pListBox->AddItem(T_TOKEN_TSPLIT_USED, g_pTheDB->GetMessageText(MID_TemporalSplitUsed));
		pListBox->AddItem(T_WALLLIGHT, g_pTheDB->GetMessageText(MID_WallLight));
	}

	pListBox->AddItem(T_ARROW_E, g_pTheDB->GetMessageText(MID_ForceArrowE));
	pListBox->AddItem(T_ARROW_N, g_pTheDB->GetMessageText(MID_ForceArrowN));
	pListBox->AddItem(T_ARROW_NE, g_pTheDB->GetMessageText(MID_ForceArrowNE));
	pListBox->AddItem(T_ARROW_NW, g_pTheDB->GetMessageText(MID_ForceArrowNW));
	pListBox->AddItem(T_ARROW_S, g_pTheDB->GetMessageText(MID_ForceArrowS));
	pListBox->AddItem(T_ARROW_SE, g_pTheDB->GetMessageText(MID_ForceArrowSE));
	pListBox->AddItem(T_ARROW_SW, g_pTheDB->GetMessageText(MID_ForceArrowSW));
	pListBox->AddItem(T_ARROW_W, g_pTheDB->GetMessageText(MID_ForceArrowW));
	pListBox->AddItem(T_ARROW_OFF_E, g_pTheDB->GetMessageText(MID_ForceArrowDisabledE));
	pListBox->AddItem(T_ARROW_OFF_N, g_pTheDB->GetMessageText(MID_ForceArrowDisabledN));
	pListBox->AddItem(T_ARROW_OFF_NE, g_pTheDB->GetMessageText(MID_ForceArrowDisabledNE));
	pListBox->AddItem(T_ARROW_OFF_NW, g_pTheDB->GetMessageText(MID_ForceArrowDisabledNW));
	pListBox->AddItem(T_ARROW_OFF_S, g_pTheDB->GetMessageText(MID_ForceArrowDisabledS));
	pListBox->AddItem(T_ARROW_OFF_SE, g_pTheDB->GetMessageText(MID_ForceArrowDisabledSE));
	pListBox->AddItem(T_ARROW_OFF_SW, g_pTheDB->GetMessageText(MID_ForceArrowDisabledSW));
	pListBox->AddItem(T_ARROW_OFF_W, g_pTheDB->GetMessageText(MID_ForceArrowDisabledW));
	pListBox->AddItem(T_BEACON, g_pTheDB->GetMessageText(MID_Beacon));
	pListBox->AddItem(T_BEACON_OFF, g_pTheDB->GetMessageText(MID_BeaconOff));
	pListBox->AddItem(T_BOMB, g_pTheDB->GetMessageText(MID_Bomb));
	pListBox->AddItem(T_BRIAR_SOURCE, g_pTheDB->GetMessageText(MID_FlowSource));
	pListBox->AddItem(T_BRIAR_LIVE, g_pTheDB->GetMessageText(MID_FlowInner));
	pListBox->AddItem(T_BRIAR_DEAD, g_pTheDB->GetMessageText(MID_BriarWithered));
	pListBox->AddItem(T_BRIDGE, g_pTheDB->GetMessageText(MID_Bridge));
	pListBox->AddItem(T_BRIDGE_H, g_pTheDB->GetMessageText(MID_Bridge_H));
	pListBox->AddItem(T_BRIDGE_V, g_pTheDB->GetMessageText(MID_Bridge_V));
	pListBox->AddItem(T_DOOR_B, g_pTheDB->GetMessageText(TILE_MID[T_DOOR_B]));
	pListBox->AddItem(T_DOOR_BO, g_pTheDB->GetMessageText(TILE_MID[T_DOOR_BO]));
	pListBox->AddItem(T_DOOR_C, g_pTheDB->GetMessageText(TILE_MID[T_DOOR_C]));
	pListBox->AddItem(T_DOOR_CO, g_pTheDB->GetMessageText(TILE_MID[T_DOOR_CO]));
	pListBox->AddItem(T_DOOR_GO, g_pTheDB->GetMessageText(TILE_MID[T_DOOR_GO]));
	pListBox->AddItem(T_DOOR_M, g_pTheDB->GetMessageText(TILE_MID[T_DOOR_M]));
	pListBox->AddItem(T_DOOR_R, g_pTheDB->GetMessageText(TILE_MID[T_DOOR_R]));
	pListBox->AddItem(T_DOOR_RO, g_pTheDB->GetMessageText(TILE_MID[T_DOOR_RO]));
	pListBox->AddItem(T_DOOR_Y, g_pTheDB->GetMessageText(TILE_MID[T_DOOR_Y]));
	pListBox->AddItem(T_DOOR_YO, g_pTheDB->GetMessageText(TILE_MID[T_DOOR_YO]));
	pListBox->AddItem(T_FIRETRAP, g_pTheDB->GetMessageText(MID_Firetrap));
	pListBox->AddItem(T_FIRETRAP_ON, g_pTheDB->GetMessageText(MID_FiretrapOn));
	pListBox->AddItem(T_FLOOR, g_pTheDB->GetMessageText(MID_Floor));
	pListBox->AddItem(T_FLOOR_ALT, g_pTheDB->GetMessageText(MID_FloorAlt));
	pListBox->AddItem(T_FLOOR_DIRT, g_pTheDB->GetMessageText(MID_FloorDirt));
	pListBox->AddItem(T_FLOOR_GRASS, g_pTheDB->GetMessageText(MID_FloorGrass));
	pListBox->AddItem(T_FLOOR_IMAGE, g_pTheDB->GetMessageText(MID_FloorImage));
	pListBox->AddItem(T_FLOOR_M, g_pTheDB->GetMessageText(MID_FloorMosaic));
	pListBox->AddItem(T_FLOOR_ROAD, g_pTheDB->GetMessageText(MID_FloorRoad));
	pListBox->AddItem(T_FLOOR_SPIKES, g_pTheDB->GetMessageText(MID_FloorSpikes));
	pListBox->AddItem(T_FLUFF, g_pTheDB->GetMessageText(MID_Fluff));
	pListBox->AddItem(T_FLUFFVENT, g_pTheDB->GetMessageText(MID_FluffVent));
	pListBox->AddItem(T_FUSE, g_pTheDB->GetMessageText(MID_Fuse));
	pListBox->AddItem(T_GEL, g_pTheDB->GetMessageText(MID_Gel));
	pListBox->AddItem(T_GOO, g_pTheDB->GetMessageText(MID_Goo));
	pListBox->AddItem(T_HORN_SOLDIER, g_pTheDB->GetMessageText(MID_SoldierHorn));
	pListBox->AddItem(T_HORN_SQUAD, g_pTheDB->GetMessageText(MID_SquadHorn));
	pListBox->AddItem(T_HOT, g_pTheDB->GetMessageText(MID_Hot));
	pListBox->AddItem(T_MIRROR, g_pTheDB->GetMessageText(MID_Mirror));
	pListBox->AddItem(T_MUD, g_pTheDB->GetMessageText(MID_Mud));
	pListBox->AddItem(T_NODIAGONAL, g_pTheDB->GetMessageText(MID_Ortho));
	pListBox->AddItem(T_ORB_BROKEN, g_pTheDB->GetMessageText(MID_OrbWaitBroken));
	pListBox->AddItem(T_ORB_CRACKED, g_pTheDB->GetMessageText(MID_OrbWaitCracked));
	pListBox->AddItem(T_ORB_NORMAL, g_pTheDB->GetMessageText(MID_OrbWaitNormal));
	pListBox->AddItem(T_OVERHEAD_IMAGE, g_pTheDB->GetMessageText(MID_OverheadImage));
	pListBox->AddItem(T_PIT, g_pTheDB->GetMessageText(MID_Pit));
	pListBox->AddItem(T_PIT_IMAGE, g_pTheDB->GetMessageText(MID_PitImage));
	pListBox->AddItem(T_POTION_C, g_pTheDB->GetMessageText(MID_ClonePotion));
	pListBox->AddItem(T_POTION_D, g_pTheDB->GetMessageText(MID_DecoyPotion));
	pListBox->AddItem(T_POTION_I, g_pTheDB->GetMessageText(MID_InvisPotion));
	pListBox->AddItem(T_POTION_K, g_pTheDB->GetMessageText(MID_MimicPotion));
	pListBox->AddItem(T_POTION_SP, g_pTheDB->GetMessageText(MID_SpeedPotion));
	pListBox->AddItem(T_POWDER_KEG, g_pTheDB->GetMessageText(MID_PowderKeg));
	pListBox->AddItem(T_SHALLOW_WATER, g_pTheDB->GetMessageText(MID_ShallowWater));
	pListBox->AddItem(T_STATION, g_pTheDB->GetMessageText(MID_Station));
	pListBox->AddItem(T_STEP_STONE, g_pTheDB->GetMessageText(MID_StepStone));
	pListBox->AddItem(T_TAR, g_pTheDB->GetMessageText(MID_Tar));
	pListBox->AddItem(T_THINICE, g_pTheDB->GetMessageText(MID_ThinIce));
	pListBox->AddItem(T_THINICE_SH, g_pTheDB->GetMessageText(MID_ThinIce2));
	pListBox->AddItem(T_TOKEN_CONQUER, g_pTheDB->GetMessageText(MID_TokenConquer));
	pListBox->AddItem(T_TOKEN_DISARM, g_pTheDB->GetMessageText(MID_TokenSwordDisarm));
	pListBox->AddItem(T_TOKEN_PERSISTENTMOVE, g_pTheDB->GetMessageText(MID_TokenCitizen));
	pListBox->AddItem(T_TOKEN_POWER, g_pTheDB->GetMessageText(MID_TokenPowerTarget));
	pListBox->AddItem(T_TOKEN_ROTATECCW, g_pTheDB->GetMessageText(MID_TokenRotateCCW));
	pListBox->AddItem(T_TOKEN_ROTATECW, g_pTheDB->GetMessageText(MID_Token));
	pListBox->AddItem(T_TOKEN_SWITCH_GELMUD, g_pTheDB->GetMessageText(MID_TokenGelMud));
	pListBox->AddItem(T_TOKEN_SWITCH_TARGEL, g_pTheDB->GetMessageText(MID_TokenTarGel));
	pListBox->AddItem(T_TOKEN_SWITCH_TARMUD, g_pTheDB->GetMessageText(MID_TokenTarMud));
	pListBox->AddItem(T_TOKEN_TSPLIT, g_pTheDB->GetMessageText(MID_TemporalSplitToken));
	pListBox->AddItem(T_TOKEN_VISION, g_pTheDB->GetMessageText(MID_TokenTranslucentTar));
	pListBox->AddItem(T_TOKEN_WPCABER, g_pTheDB->GetMessageText(MID_TokenCaber));
	pListBox->AddItem(T_TOKEN_WPDAGGER, g_pTheDB->GetMessageText(MID_TokenDagger));
	pListBox->AddItem(T_TOKEN_WPPICKAXE, g_pTheDB->GetMessageText(MID_TokenPickaxe));
	pListBox->AddItem(T_TOKEN_WPSPEAR, g_pTheDB->GetMessageText(MID_TokenSpear));
	pListBox->AddItem(T_TOKEN_WPSTAFF, g_pTheDB->GetMessageText(MID_TokenStaff));
	pListBox->AddItem(T_TOKEN_WPSWORD, g_pTheDB->GetMessageText(MID_TokenSword));
	pListBox->AddItem(T_TRAPDOOR, g_pTheDB->GetMessageText(MID_Trapdoor));
	pListBox->AddItem(T_TRAPDOOR2, g_pTheDB->GetMessageText(MID_Trapdoor2));
	pListBox->AddItem(T_TUNNEL_E, g_pTheDB->GetMessageText(MID_Tunnel_E));
	pListBox->AddItem(T_TUNNEL_N, g_pTheDB->GetMessageText(MID_Tunnel_N));
	pListBox->AddItem(T_TUNNEL_S, g_pTheDB->GetMessageText(MID_Tunnel_S));
	pListBox->AddItem(T_TUNNEL_W, g_pTheDB->GetMessageText(MID_Tunnel_W));
	pListBox->AddItem(T_WALL, g_pTheDB->GetMessageText(MID_Wall));
	pListBox->AddItem(T_WALL2, g_pTheDB->GetMessageText(MID_Wall2));
	pListBox->AddItem(T_WALL_B, g_pTheDB->GetMessageText(MID_BrokenWall));
	pListBox->AddItem(T_WALL_H, g_pTheDB->GetMessageText(MID_SecretWall));
	pListBox->AddItem(T_WATER, g_pTheDB->GetMessageText(MID_Water));
	pListBox->AddItem(T_WALL_IMAGE, g_pTheDB->GetMessageText(MID_WallImage));

	pListBox->SetAllowFiltering(true);
}

//*****************************************************************************
void CCharacterDialogWidget::PopulateGraphicListBox(CListBoxWidget *pListBox)
//Add all monsters that character can appear as to graphics list.
{
	ASSERT(pListBox);

	static const UINT graphics[] = {
		M_ROACH, M_QROACH, M_GOBLIN, M_WWING, M_EYE,
		M_TARBABY, M_BRAIN, M_MIMIC, M_SPIDER,

		M_ROCKGOLEM, M_CONSTRUCT, M_WUBBA, M_SEEP,
		M_HALPH, M_HALPH2, M_SLAYER, M_SLAYER2,
		M_GUARD, M_MUDBABY, M_CLONE, M_TEMPORALCLONE,
		M_DECOY, M_STALWART, M_STALWART2,

		M_CITIZEN, M_ARCHITECT, M_GELBABY, M_WATERSKIPPER,
		M_SKIPPERNEST, M_FEGUNDO, M_AUMTLICH, M_FLUFFBABY,

		//Character pseudo monster types.
		M_CITIZEN1, M_CITIZEN2, M_CITIZEN3, M_CITIZEN4,
		M_GOBLINKING, M_MUDCOORDINATOR, M_TARTECHNICIAN,
		M_NEGOTIATOR, M_EYE_ACTIVE,
		M_BEETHRO, M_GUNTHRO,
		UINT(M_NONE),

		MONSTER_TYPES //sentinel
	};

	for (UINT i = 0; graphics[i] != MONSTER_TYPES; ++i) {
		const UINT graphic = graphics[i];
		pListBox->AddItem(graphic, g_pTheDB->GetMessageText(getMIDForMonster(graphic)));
	}

	pListBox->SetAllowFiltering(true);
}

//*****************************************************************************
void CCharacterDialogWidget::PopulatePlayerGraphicListBox(CListBoxWidget *pListBox)
//Add all monsters that character can appear as to graphics list.
{
	ASSERT(pListBox);

	this->PopulateGraphicListBox(pListBox);
	pListBox->RemoveItem(M_SKIPPERNEST);
	pListBox->RemoveItem(M_BRAIN);
}

//*****************************************************************************
void CCharacterDialogWidget::PopulateCommandDescriptions(
//Fills the given command list box with human-understandable descriptions of
//the specified command script.
//
//Params:
	CListBoxWidget *pCommandList, COMMANDPTR_VECTOR& commands)
{
	pCommandList->Clear();
	for (UINT wIndex=0; wIndex<commands.size(); ++wIndex)
	{
		CCharacterCommand *pCommand = commands[wIndex];
		const UINT insertedIndex = pCommandList->AddItemPointer(pCommand,
				GetCommandDesc(commands, pCommand).c_str());
		SetCommandColor(pCommandList, insertedIndex, pCommand->command);
	}
	
	PrettyPrintCommands(pCommandList, commands);

	if (commands.size())
		pCommandList->SelectLine(0);
}

//*****************************************************************************
void CCharacterDialogWidget::PopulateCharacterList(CListBoxWidget *pListBox)
//Add all custom hold characters to list.
{
	ASSERT(pListBox);

	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen);
	ASSERT(pEditRoomScreen->pHold);
	const vector<HoldCharacter*>& characters = pEditRoomScreen->pHold->characters;
	for (vector<HoldCharacter*>::const_iterator ch = characters.begin();
		ch != characters.end(); ++ch)
	{
		HoldCharacter *pChar = *ch;
		pListBox->AddItem(pChar->dwCharID, pChar->charNameText.c_str());
	}

	pListBox->SetAllowFiltering(true);
}

//*****************************************************************************
void CCharacterDialogWidget::PopulateGotoLabelList(const COMMANDPTR_VECTOR& commands)
//Compile goto label texts.  Determine next label ID.
{
	this->pGotoLabelListBox->Clear();
	this->wIncrementedLabel = 0;

	this->pGotoLabelListBox->AddItem(ScriptFlag::GotoSmartType::PreviousIf, g_pTheDB->GetMessageText(MID_PreviousIf));
	this->pGotoLabelListBox->AddItem(ScriptFlag::GotoSmartType::NextElseOrElseIfSkipCondition, g_pTheDB->GetMessageText(MID_NextElseOrElseIfSkip));

	for (UINT wIndex=0; wIndex<commands.size(); ++wIndex)
	{
		CCharacterCommand *pCommand = commands[wIndex];
		if (pCommand->command == CCharacterCommand::CC_Label)
		{
			this->pGotoLabelListBox->AddItem(pCommand->x, pCommand->label.c_str());
			if (!this->pGotoLabelListBox->ItemIsSelected())
				this->pGotoLabelListBox->SelectLine(0);
			if (pCommand->x > this->wIncrementedLabel)
				this->wIncrementedLabel = pCommand->x;
		}
	}

	this->pGotoLabelListBox->SetAllowFiltering(true);
}

//*****************************************************************************
void CCharacterDialogWidget::PopulateMainGraphicList()
//Refresh the list of available characters.
{
	ASSERT(this->pGraphicListBox);

	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen->pHold);

	this->pGraphicListBox->Clear();
	PopulateGraphicListBox(this->pGraphicListBox);
	PopulateCharacterList(this->pGraphicListBox);
	//Select Citizen 1 if a custom char in use was deleted
	this->pGraphicListBox->SelectItem(this->pCharacter ?
			this->pCharacter->wLogicalIdentity < CUSTOM_CHARACTER_FIRST ||
			this->pCharacter->wLogicalIdentity == M_NONE ||
			pEditRoomScreen->pHold->GetCharacter(this->pCharacter->wLogicalIdentity) != NULL ?
				this->pCharacter->wLogicalIdentity : M_CITIZEN1 : M_NONE);

	ASSERT(this->pAddCommandGraphicListBox);
	this->pAddCommandGraphicListBox->Clear();
	PopulateGraphicListBox(this->pAddCommandGraphicListBox);
	PopulateCharacterList(this->pAddCommandGraphicListBox);
	this->pAddCommandGraphicListBox->SelectItem(M_BEETHRO);

	ASSERT(this->pPlayerGraphicListBox);
	this->pPlayerGraphicListBox->Clear();
	PopulatePlayerGraphicListBox(this->pPlayerGraphicListBox);
	PopulateCharacterList(this->pPlayerGraphicListBox);
	this->pPlayerGraphicListBox->SelectItem(M_BEETHRO);

	RefreshCustomCharacterList(this->pGlobalScriptListBox);
	RefreshCustomCharacterList(this->pCharListBox);

	ASSERT(this->pSpeakerListBox);
	PopulateSpeakerList(this->pSpeakerListBox);

	this->pGraphicListBox->SetAllowFiltering(true);
	this->pPlayerGraphicListBox->SetAllowFiltering(true);
	this->pSpeakerListBox->SetAllowFiltering(true);
}

//*****************************************************************************
void CCharacterDialogWidget::RefreshCustomCharacterList(CListBoxWidget *pListBox)
{
	ASSERT(pListBox);
	pListBox->Clear();
	PopulateCharacterList(pListBox);
	pListBox->SelectLine(0);
}

//*****************************************************************************
void CCharacterDialogWidget::PopulateSpeakerList(CListBoxWidget *pListBox)
//Refresh the list of available speakers.
{
	pListBox->Clear();
	pListBox->SortAlphabetically(true);

	pListBox->AddItem(Speaker_Beethro, g_pTheDB->GetMessageText(MID_Beethro));
	pListBox->AddItem(Speaker_Gunthro, g_pTheDB->GetMessageText(MID_Gunthro));
	pListBox->AddItem(Speaker_SerpentG, g_pTheDB->GetMessageText(MID_GreenSerpent));
	pListBox->AddItem(Speaker_Architect, g_pTheDB->GetMessageText(MID_Architect));
	pListBox->AddItem(Speaker_Aumtlich, g_pTheDB->GetMessageText(MID_Aumtlich));
	pListBox->AddItem(Speaker_Brain, g_pTheDB->GetMessageText(MID_Brain));
	pListBox->AddItem(Speaker_Citizen, g_pTheDB->GetMessageText(MID_Citizen));
	pListBox->AddItem(Speaker_Citizen1, g_pTheDB->GetMessageText(MID_Citizen1));
	pListBox->AddItem(Speaker_Citizen2, g_pTheDB->GetMessageText(MID_Citizen2));
	pListBox->AddItem(Speaker_Citizen3, g_pTheDB->GetMessageText(MID_Citizen3));
	pListBox->AddItem(Speaker_Citizen4, g_pTheDB->GetMessageText(MID_Citizen4));
	pListBox->AddItem(Speaker_Clone, g_pTheDB->GetMessageText(MID_Clone));
	pListBox->AddItem(Speaker_Construct, g_pTheDB->GetMessageText(MID_Construct));
	pListBox->AddItem(Speaker_Decoy, g_pTheDB->GetMessageText(MID_Decoy));
	pListBox->AddItem(Speaker_Eye, g_pTheDB->GetMessageText(MID_EvilEye));
	pListBox->AddItem(Speaker_EyeActive, g_pTheDB->GetMessageText(MID_EvilEyeActive));
	pListBox->AddItem(Speaker_Fegundo, g_pTheDB->GetMessageText(MID_Fegundo));
	pListBox->AddItem(Speaker_FegundoAshes, g_pTheDB->GetMessageText(MID_FegundoAshes));
	pListBox->AddItem(Speaker_FluffBaby, g_pTheDB->GetMessageText(MID_FluffBaby));
	pListBox->AddItem(Speaker_GelBaby, g_pTheDB->GetMessageText(MID_GelBaby));
	pListBox->AddItem(Speaker_GelMother, g_pTheDB->GetMessageText(MID_GelMother));
	pListBox->AddItem(Speaker_Gentryii, g_pTheDB->GetMessageText(MID_Gentryii));
	pListBox->AddItem(Speaker_Goblin, g_pTheDB->GetMessageText(MID_Goblin));
	pListBox->AddItem(Speaker_GoblinKing, g_pTheDB->GetMessageText(MID_GoblinKing));
	pListBox->AddItem(Speaker_Guard, g_pTheDB->GetMessageText(MID_Guard));
	pListBox->AddItem(Speaker_Halph, g_pTheDB->GetMessageText(MID_Halph));
	pListBox->AddItem(Speaker_Halph2, g_pTheDB->GetMessageText(MID_Halph2));
	pListBox->AddItem(Speaker_Mimic, g_pTheDB->GetMessageText(MID_Mimic));
	pListBox->AddItem(Speaker_MudBaby, g_pTheDB->GetMessageText(MID_MudBaby));
	pListBox->AddItem(Speaker_MudCoordinator, g_pTheDB->GetMessageText(MID_MudCoordinator));
	pListBox->AddItem(Speaker_MudMother, g_pTheDB->GetMessageText(MID_MudMother));
	pListBox->AddItem(Speaker_Negotiator, g_pTheDB->GetMessageText(MID_Negotiator));
	pListBox->AddItem(Speaker_Roach, g_pTheDB->GetMessageText(MID_Roach));
	pListBox->AddItem(Speaker_RoachEgg, g_pTheDB->GetMessageText(MID_RoachEgg));
	pListBox->AddItem(Speaker_QRoach, g_pTheDB->GetMessageText(MID_RoachQueen));
	pListBox->AddItem(Speaker_SerpentB, g_pTheDB->GetMessageText(MID_BlueSerpent));
	pListBox->AddItem(Speaker_RockGolem, g_pTheDB->GetMessageText(MID_RockGolem));
	pListBox->AddItem(Speaker_RockGiant, g_pTheDB->GetMessageText(MID_RockGiant));
	pListBox->AddItem(Speaker_Seep, g_pTheDB->GetMessageText(MID_Seep));
	pListBox->AddItem(Speaker_Serpent, g_pTheDB->GetMessageText(MID_Serpent));
	pListBox->AddItem(Speaker_Slayer, g_pTheDB->GetMessageText(MID_Slayer));
	pListBox->AddItem(Speaker_Slayer2, g_pTheDB->GetMessageText(MID_Slayer2));
	pListBox->AddItem(Speaker_Spider, g_pTheDB->GetMessageText(MID_Spider));
	pListBox->AddItem(Speaker_Stalwart, g_pTheDB->GetMessageText(MID_Stalwart));
	pListBox->AddItem(Speaker_Stalwart2, g_pTheDB->GetMessageText(MID_Stalwart2));
	pListBox->AddItem(Speaker_TarBaby, g_pTheDB->GetMessageText(MID_TarBaby));
	pListBox->AddItem(Speaker_TarMother, g_pTheDB->GetMessageText(MID_TarMother));
	pListBox->AddItem(Speaker_TarTechnician, g_pTheDB->GetMessageText(MID_TarTechnician));
	pListBox->AddItem(Speaker_TemporalClone, g_pTheDB->GetMessageText(MID_TemporalClone));
	pListBox->AddItem(Speaker_WaterSkipper, g_pTheDB->GetMessageText(MID_Waterskipper));
	pListBox->AddItem(Speaker_WaterSkipperNest, g_pTheDB->GetMessageText(MID_WaterskipperNest));
	pListBox->AddItem(Speaker_WWing, g_pTheDB->GetMessageText(MID_Wraithwing));
	pListBox->AddItem(Speaker_Wubba, g_pTheDB->GetMessageText(MID_Wubba));

	PopulateCharacterList(pListBox);

	pListBox->SortAlphabetically(false);

	pListBox->AddItem(Speaker_None, g_pTheDB->GetMessageText(MID_None));
	pListBox->AddItem(Speaker_Custom, g_pTheDB->GetMessageText(MID_Custom));
	pListBox->AddItem(Speaker_Player, g_pTheDB->GetMessageText(MID_Player));
	pListBox->AddItem(Speaker_Self, g_pTheDB->GetMessageText(MID_Self));

	pListBox->SelectItem(Speaker_Self);
}

//*****************************************************************************
void CCharacterDialogWidget::PopulateVarList()
//Compile active hold's current var list.
{
	this->pVarListBox->Clear();
	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen);
	ASSERT(pEditRoomScreen->pHold);
	for (vector<HoldVar>::const_iterator var = pEditRoomScreen->pHold->vars.begin();
			var != pEditRoomScreen->pHold->vars.end(); ++var)
		this->pVarListBox->AddItem(var->dwVarID, var->varNameText.c_str());

	//Add hard-coded global vars to the end of the list.
	this->pVarListBox->SortAlphabetically(false);

	this->pVarListBox->AddItem(ScriptVars::P_MONSTER_COLOR, g_pTheDB->GetMessageText(MID_VarMonsterColor));
	this->pVarListBox->AddItem(ScriptVars::P_MONSTER_WEAPON, g_pTheDB->GetMessageText(MID_VarMonsterWeapon));
	this->pVarListBox->AddItem(ScriptVars::P_MONSTER_X, g_pTheDB->GetMessageText(MID_VarMonsterX));
	this->pVarListBox->AddItem(ScriptVars::P_MONSTER_Y, g_pTheDB->GetMessageText(MID_VarMonsterY));
	this->pVarListBox->AddItem(ScriptVars::P_MONSTER_O, g_pTheDB->GetMessageText(MID_VarMonsterO));
	this->pVarListBox->AddItem(ScriptVars::P_MONSTER_NAME, g_pTheDB->GetMessageText(MID_VarMonsterName));
	this->pVarListBox->AddItem(ScriptVars::P_SCRIPT_X, g_pTheDB->GetMessageText(MID_VarMonsterParamX));
	this->pVarListBox->AddItem(ScriptVars::P_SCRIPT_Y, g_pTheDB->GetMessageText(MID_VarMonsterParamY));
	this->pVarListBox->AddItem(ScriptVars::P_SCRIPT_W, g_pTheDB->GetMessageText(MID_VarMonsterParamW));
	this->pVarListBox->AddItem(ScriptVars::P_SCRIPT_H, g_pTheDB->GetMessageText(MID_VarMonsterParamH));
	this->pVarListBox->AddItem(ScriptVars::P_SCRIPT_F, g_pTheDB->GetMessageText(MID_VarMonsterParamF));

	this->pVarListBox->AddItem(ScriptVars::P_PLAYER_X, g_pTheDB->GetMessageText(MID_VarX));
	this->pVarListBox->AddItem(ScriptVars::P_PLAYER_Y, g_pTheDB->GetMessageText(MID_VarY));
	this->pVarListBox->AddItem(ScriptVars::P_PLAYER_O, g_pTheDB->GetMessageText(MID_VarO));

	this->pVarListBox->AddItem(ScriptVars::P_ROOMIMAGE_X, g_pTheDB->GetMessageText(MID_VarRoomImageX));
	this->pVarListBox->AddItem(ScriptVars::P_ROOMIMAGE_Y, g_pTheDB->GetMessageText(MID_VarRoomImageY));
	this->pVarListBox->AddItem(ScriptVars::P_OVERHEADIMAGE_X, g_pTheDB->GetMessageText(MID_VarOverheadImageX));
	this->pVarListBox->AddItem(ScriptVars::P_OVERHEADIMAGE_Y, g_pTheDB->GetMessageText(MID_VarOverheadImageY));

	this->pVarListBox->AddItem(ScriptVars::P_THREATCLOCK, g_pTheDB->GetMessageText(MID_VarThreatClock));
	this->pVarListBox->AddItem(ScriptVars::P_LEVELNAME, g_pTheDB->GetMessageText(MID_VarLevelName));
	this->pVarListBox->AddItem(ScriptVars::P_ROOM_X, g_pTheDB->GetMessageText(MID_VarRoomX));
	this->pVarListBox->AddItem(ScriptVars::P_ROOM_Y, g_pTheDB->GetMessageText(MID_VarRoomY));

	this->pVarListBox->AddItem(ScriptVars::P_PLAYERLIGHT, g_pTheDB->GetMessageText(MID_VarPlayerLight));
	this->pVarListBox->AddItem(ScriptVars::P_PLAYERLIGHTTYPE, g_pTheDB->GetMessageText(MID_VarPlayerLightType));

	this->pVarListBox->AddItem(ScriptVars::P_RETURN_X, g_pTheDB->GetMessageText(MID_VarReturnX));
	this->pVarListBox->AddItem(ScriptVars::P_RETURN_Y, g_pTheDB->GetMessageText(MID_VarReturnY));

	this->pVarListBox->SortAlphabetically(true);
}

//*****************************************************************************
void CCharacterDialogWidget::prepareForwardReferences(const COMMANDPTR_VECTOR& newCommands)
//When cutting commands from the command script, replace label IDs with text
//that can be resolved when the command is pasted back into a script.
{
	for (COMMANDPTR_VECTOR::const_iterator cIter = newCommands.begin();
			cIter != newCommands.end(); ++cIter)
	{
		CCharacterCommand& c = *(*cIter);
		switch (c.command)
		{
			case CCharacterCommand::CC_GoSub:
			case CCharacterCommand::CC_GoTo:
			case CCharacterCommand::CC_AnswerOption:
				c.label = this->pGotoLabelListBox->GetTextForKey(c.x);
				if (!c.label.empty()) //if label ID is valid, replace ID with this text
					c.x = 0;
			break;
			default: break;
		}
	}
}

//*****************************************************************************
void CCharacterDialogWidget::resolveForwardReferences(const COMMANDPTR_VECTOR& newCommands)
//Resolve forward references for commands just pasted into the current command script.
//For instance, the label text in a "goto <label>" command will be hooked up to
//the label ID of current "Label" command with this text.
{
	for (COMMANDPTR_VECTOR::const_iterator cIter = newCommands.begin();
			cIter != newCommands.end(); ++cIter)
	{
		CCharacterCommand& c = *(*cIter);
		switch (c.command)
		{
			case CCharacterCommand::CC_GoSub:
			case CCharacterCommand::CC_GoTo:
			case CCharacterCommand::CC_AnswerOption:
			{
				UINT tempIndex = 0;
				bool bFound;
				const UINT labelID = findTextMatch(this->pGotoLabelListBox, c.label.c_str(), tempIndex, bFound);
				c.x = (bFound ? labelID : 0);
				c.label.resize(0);
			}
			break;
			default: break;
		}
	}
}

//*****************************************************************************
void CCharacterDialogWidget::QueryRect()
//Get rectangular area info from user.
{
	//Get location information through CEditRoomScreen.
	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen);
	pEditRoomScreen->SetState(ES_GETRECT, true);
	for (UINT wY = this->pCommand->y; wY <= this->pCommand->y + this->pCommand->h; ++wY)
		for (UINT wX = this->pCommand->x; wX <= this->pCommand->x + this->pCommand->w; ++wX)
			pEditRoomScreen->pRoomWidget->AddShadeEffect(wX, wY, PaleRed);
	Deactivate();
}

//*****************************************************************************
void CCharacterDialogWidget::QueryXY()
//Get (x,y) info from user.
{
	//Get location information through CEditRoomScreen.
	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen);
	pEditRoomScreen->SetState(ES_GETSQUARE, true);
	pEditRoomScreen->pRoomWidget->AddShadeEffect(
			this->pCommand->x, this->pCommand->y, PaleRed);
	Deactivate();
}

//*****************************************************************************
void CCharacterDialogWidget::SetCommandColor(
	CListBoxWidget* pListBox, int line, CCharacterCommand::CharCommand command)
{
	switch (command) {
		case CCharacterCommand::CC_Label:
			pListBox->SetItemColorAtLine(line, DarkGreen);
		break;
		case CCharacterCommand::CC_GoTo:
		case CCharacterCommand::CC_GoSub:
		case CCharacterCommand::CC_Return:
		case CCharacterCommand::CC_AnswerOption:
		case CCharacterCommand::CC_EndScript:
		case CCharacterCommand::CC_EndScriptOnExit:
		case CCharacterCommand::CC_ReplaceWithDefault:
			pListBox->SetItemColorAtLine(line, Maroon);
		break;
		case CCharacterCommand::CC_If:
		case CCharacterCommand::CC_IfElse:
		case CCharacterCommand::CC_IfElseIf:
		case CCharacterCommand::CC_IfEnd:
			pListBox->SetItemColorAtLine(line, DarkBlue);
		break;
		case CCharacterCommand::CC_VarSet:
			pListBox->SetItemColorAtLine(line, FullRed);
		break;
		case CCharacterCommand::CC_Wait: 
			pListBox->SetItemColorAtLine(line, DarkGray);
		break;
		default: break;
	}
}

//*****************************************************************************
void CCharacterDialogWidget::SetDefaultScriptWidgetStates()
//Update widget states on main dialog.
{
	CWidget *pButton = GetWidget(TAG_DELETECOMMAND2);
	ASSERT(pButton);
	const bool bEnable = this->pDefaultScriptCommandsListBox->GetItemCount() > 0;
	if (!bEnable && GetSelectedWidget() == pButton)
		SelectNextWidget();
	pButton->Enable(bEnable);
	pButton->RequestPaint();
}

//*****************************************************************************
void CCharacterDialogWidget::SetWidgetStates()
//Update widget states on main dialog.
{
	CWidget *pButton = GetWidget(TAG_DELETECOMMAND);
	ASSERT(pButton);
	const bool bEnable = this->pCommandsListBox->GetItemCount() > 0;
	if (!bEnable && GetSelectedWidget() == pButton)
		SelectNextWidget();
	pButton->Enable(bEnable);
	pButton->RequestPaint();
}

//*****************************************************************************
void CCharacterDialogWidget::SetActionWidgetStates()
//Enable those widgets that are applicable to currently selected command.
{
	//Code is structured in this way to facilitate quick addition of
	//additional action parameters.
	static const UINT NUM_WIDGETS = 54;
	static const UINT widgetTag[NUM_WIDGETS] = {
		TAG_WAIT, TAG_EVENTLISTBOX, TAG_DELAY, TAG_SPEECHTEXT,
		TAG_SPEAKERLISTBOX, TAG_MOODLISTBOX, TAG_ADDSOUND, TAG_TESTSOUND, TAG_DIRECTIONLISTBOX,
		TAG_ONOFFLISTBOX, TAG_OPENCLOSELISTBOX, TAG_GOTOLABELTEXT,
		TAG_GOTOLABELLISTBOX, TAG_MUSICLISTBOX, TAG_ONOFFLISTBOX2,
		TAG_WAITFLAGSLISTBOX, TAG_VARADD, TAG_VARREMOVE,
		TAG_VARLIST, TAG_VAROPLIST, TAG_VARCOMPLIST, TAG_VARVALUE,
		TAG_GRAPHICLISTBOX2, TAG_MOVERELX, TAG_MOVERELY, TAG_IMPERATIVELISTBOX,
		TAG_BUILDITEMLISTBOX, TAG_STEALTHLISTBOX, TAG_WATERTRAVERSALLISTBOX, TAG_GLOBALSCRIPTLISTBOX,
		TAG_DIRECTIONLISTBOX2,
		TAG_VISUALEFFECTS_LISTBOX, TAG_DIRECTIONLISTBOX3, TAG_ONOFFLISTBOX3,
		TAG_DISPLAYFILTER, TAG_ICONDISPLAY, TAG_IMAGEDISPLAY,
		TAG_X_COORD, TAG_Y_COORD,
		TAG_WEAPON_LISTBOX, TAG_ATTACKTILE,
		TAG_TEXT2, TAG_INPUTLISTBOX, TAG_IMAGEOVERLAYTEXT,
		TAG_VARNAMETEXTINPUT, TAG_GRAPHICLISTBOX3, TAG_WAITFORITEMLISTBOX, TAG_BUILDMARKERITEMLISTBOX,
		TAG_NATURAL_TARGET_TYPES, TAG_WEAPON_LISTBOX2, TAG_BEHAVIOR_LISTBOX, TAG_REMAINS_LISTBOX,
		TAG_ONOFFLISTBOX4, TAG_MOVETYPELISTBOX
	};

	static const UINT NO_WIDGETS[] =    {0};
	static const UINT WAIT[] =          {TAG_WAIT, 0};
	static const UINT CUEEVENT[] =      {TAG_EVENTLISTBOX, 0};
	static const UINT SPEECH[] =        {TAG_DELAY, TAG_SPEECHTEXT, TAG_SPEAKERLISTBOX, TAG_MOODLISTBOX, TAG_ADDSOUND, TAG_TESTSOUND, 0};
	static const UINT ORIENTATION[] =   {TAG_DIRECTIONLISTBOX, 0};
	static const UINT ONOFF[] =         {TAG_ONOFFLISTBOX, 0};
	static const UINT OPENCLOSE[] =     {TAG_OPENCLOSELISTBOX, 0};
	static const UINT TEXTBOX[] =       {TAG_GOTOLABELTEXT, 0};
	static const UINT TEXT_AND_COLOR[] ={TAG_GOTOLABELTEXT, TAG_TEXT2, 0};
	static const UINT GOTOLIST[] =      {TAG_GOTOLABELLISTBOX, 0};
	static const UINT MUSIC[] =         {TAG_MUSICLISTBOX, 0};
	static const UINT MOVE[] =          {TAG_ONOFFLISTBOX, TAG_ONOFFLISTBOX2, TAG_WAITFLAGSLISTBOX, 0};
	static const UINT WAITFLAGS[] =     {TAG_WAITFLAGSLISTBOX, 0};
	static const UINT VARSET[] =        {TAG_VARNAMETEXTINPUT, TAG_VARADD, TAG_VARREMOVE, TAG_VARLIST, TAG_VAROPLIST, TAG_VARVALUE, 0};
	static const UINT VARGET[] =        {TAG_VARNAMETEXTINPUT, TAG_VARLIST, TAG_VARCOMPLIST, TAG_VARVALUE, 0};
	static const UINT GRAPHIC[] =       {TAG_GRAPHICLISTBOX3, 0};
	static const UINT NPC_GRAPHIC[] =   {TAG_GRAPHICLISTBOX3, TAG_ONOFFLISTBOX4, 0};
	static const UINT PLAYER_GRAPHIC[] ={TAG_GRAPHICLISTBOX2, 0};
	static const UINT MOVEREL[] =       {TAG_ONOFFLISTBOX, TAG_ONOFFLISTBOX2, TAG_MOVERELX, TAG_MOVERELY, 0};
	static const UINT IMPERATIVE[] =    {TAG_IMPERATIVELISTBOX, 0};
	static const UINT BEHAVIOR[] =      {TAG_ONOFFLISTBOX3, TAG_BEHAVIOR_LISTBOX, 0};
	static const UINT ANSWER[] =        {TAG_GOTOLABELTEXT, TAG_GOTOLABELLISTBOX, 0};
	static const UINT BUILD_ITEMS[] =   { TAG_BUILDITEMLISTBOX, 0 };
	static const UINT BUILD_MARKER_ITEMS[] = { TAG_BUILDMARKERITEMLISTBOX, 0 };
	static const UINT WAIT_FOR_ITEMS[] = {TAG_WAITFORITEMLISTBOX, 0};
	static const UINT XY[] =            {TAG_X_COORD, TAG_Y_COORD, 0};
	static const UINT STEALTH[] =       {TAG_STEALTHLISTBOX, 0};
	static const UINT WATERTRAVEL[] =   {TAG_WATERTRAVERSALLISTBOX, 0};
	static const UINT GLOBALSCRIPT[] =  {TAG_GLOBALSCRIPTLISTBOX, 0};
	static const UINT NEWENTITY[] =     {TAG_GRAPHICLISTBOX3, TAG_DIRECTIONLISTBOX2, 0};
	static const UINT EFFECT[] =        {TAG_VISUALEFFECTS_LISTBOX, TAG_DIRECTIONLISTBOX3, TAG_ONOFFLISTBOX3, 0};
	static const UINT DISPLAY_FILTER[] ={TAG_DISPLAYFILTER, 0};
	static const UINT WORLD_MAP_ICON[] ={TAG_GRAPHICLISTBOX3, TAG_ICONDISPLAY, TAG_X_COORD, TAG_Y_COORD, 0};
	static const UINT WORLD_MAP_IMAGE[]={TAG_IMAGEDISPLAY, TAG_X_COORD, TAG_Y_COORD, 0};
	static const UINT WEAPONS[] =       {TAG_WEAPON_LISTBOX, 0};
	static const UINT WEAPONS2[] =      { TAG_WEAPON_LISTBOX2, 0 };
	static const UINT ATTACK_TYPES[] =  {TAG_ATTACKTILE, 0};
	static const UINT INPUT[] =         { TAG_INPUTLISTBOX, 0 };
	static const UINT IMAGEOVERLAY[] =  { TAG_IMAGEOVERLAYTEXT, 0 };
	static const UINT FACE_TOWARDS[] = { TAG_ONOFFLISTBOX, TAG_WAITFLAGSLISTBOX, 0 };
	static const UINT NATURAL_TARGET[] = { TAG_NATURAL_TARGET_TYPES, 0 };
	static const UINT MONSTER_REMAINS[] = { TAG_REMAINS_LISTBOX, 0 };
	static const UINT PUSH_TILE[] =     { TAG_DIRECTIONLISTBOX2, 0 };
	static const UINT MOVETYPE[] = { TAG_MOVETYPELISTBOX, 0 };

	static const UINT* activeWidgets[CCharacterCommand::CC_Count] = {
		NO_WIDGETS,         //CC_Appear
		NO_WIDGETS,         //CC_AppearAt
		MOVE,               //CC_MoveTo
		WAIT,               //CC_Wait
		CUEEVENT,           //CC_WaitForCueEvent
		WAITFLAGS,          //CC_WaitForRect
		SPEECH,             //CC_Speech
		IMPERATIVE,         //CC_Imperative
		NO_WIDGETS,         //CC_Disappear
		NO_WIDGETS,         //CC_TurnIntoMonster
		ORIENTATION,        //CC_FaceDirection
		WAITFLAGS,          //CC_WaitForNotRect
		OPENCLOSE,          //CC_WaitForDoorTo
		TEXTBOX,            //CC_Label
		GOTOLIST,           //CC_GoTo
		0,
		0,
		0,
		WAIT,               //CC_WaitForTurn
		NO_WIDGETS,         //CC_WaitForCleanRoom
		ORIENTATION,        //CC_WaitForPlayerToFace
		NO_WIDGETS,         //CC_ActivateItemAt
		NO_WIDGETS,         //CC_EndScript
		0,
		0,
		0,
		0,
		ONOFF,              //CC_FlushSpeech
		TEXTBOX,            //CC_Question
		MUSIC,              //CC_SetMusic
		NO_WIDGETS,         //CC_EndScriptOnExit
		NO_WIDGETS,         //CC_If
		NO_WIDGETS,         //CC_IfElse
		NO_WIDGETS,         //CC_IfEnd
		ONOFF,              //CC_LevelEntrance
		VARSET,             //CC_VarSet
		VARGET,             //CC_WaitForVar
		PLAYER_GRAPHIC,     //CC_SetPlayerApperance
		WAIT,               //CC_CutScene
		MOVEREL,            //CC_MoveRel
		ONOFF,              //CC_PlayerEquipsWeapon
		ANSWER,             //CC_AnswerOption
		BUILD_MARKER_ITEMS, //CC_BuildMarker
		ONOFF,              //CC_AmbientSound
		ONOFF,              //CC_AmbientSoundAt
		NO_WIDGETS,         //CC_WaitForNoBuilding
		XY,                 //CC_PlayVideo
		ORIENTATION,        //CC_WaitForPlayerToMove
		NO_WIDGETS,         //CC_WaitForPlayerToTouchMe
		NPC_GRAPHIC,        //CC_SetNPCAppearance
		WATERTRAVEL,        //CC_SetWaterTraversal
		GLOBALSCRIPT,       //CC_StartGlobalScript
		WAIT_FOR_ITEMS,     //CC_WaitForItem
		NEWENTITY,          //CC_GenerateEntity
		EFFECT,             //CC_GameEffect
		TEXTBOX,            //CC_RoomLocationText
		TEXT_AND_COLOR,     //CC_FlashingText
		DISPLAY_FILTER,     //CC_DisplayFilter
		BUILD_ITEMS,        //CC_Build
		MUSIC,              //CC_WorldMapMusic
		WORLD_MAP_ICON,     //CC_WorldMapIcon
		NO_WIDGETS,         //CC_WorldMapSelect
		WEAPONS,            //CC_SetPlayerWeapon
		NO_WIDGETS,         //CC_WaitForSomeoneToPushMe
		ORIENTATION,        //CC_WaitForOpenMove
		NO_WIDGETS,         //CC_WaitForCleanLevel
		TEXTBOX,            //CC_ChallengeCompleted
		ATTACK_TYPES,       //CC_AttackTile
		STEALTH,            //CC_SetPlayerStealth
		INPUT,              //CC_WaitForPlayerInput
		NO_WIDGETS,         //CC_Return
		GOTOLIST,           //CC_GoSub
		IMAGEOVERLAY,       //CC_ImageOverlay
		WORLD_MAP_IMAGE,    //CC_WorldMapImage
		GRAPHIC,            //CC_WaitForEntityType
		GRAPHIC,            //CC_WaitForNotEntityType
		NO_WIDGETS,         //CC_TeleportTo
		NO_WIDGETS,         //CC_TeleportPlayerTo
		NO_WIDGETS,         //CC_DestroyTrapdoor
		NO_WIDGETS,         //CC_IfElseIf
		FACE_TOWARDS,       //CC_FaceTowards,
		NATURAL_TARGET,     //CC_GetNaturalTarget
		NO_WIDGETS,          //CC_GetEntityDirection
		WEAPONS2,          //CC_WaitForWeapon
		BEHAVIOR,            //CC_BEHAVIOR
		MONSTER_REMAINS,    //CC_WaitForRemains
		PUSH_TILE,          //CC_PushTile
		MOVETYPE,           //CC_SetMovementType
		NO_WIDGETS          //CC_ReplaceWithDefault
	};

	static const UINT NUM_LABELS = 30;
	static const UINT labelTag[NUM_LABELS] = {
		TAG_EVENTLABEL, TAG_WAITLABEL, TAG_DELAYLABEL, TAG_SPEAKERLABEL,
		TAG_MOODLABEL, TAG_TEXTLABEL, TAG_DIRECTIONLABEL, TAG_SOUNDNAME_LABEL,
		TAG_GOTOLABEL, TAG_DISPLAYSPEECHLABEL, TAG_MUSICLABEL, TAG_NOTURNING,
		TAG_SINGLESTEP, TAG_VARNAMETEXTLABEL, TAG_VARVALUELABEL, TAG_CUTSCENELABEL,
		TAG_MOVERELXLABEL, TAG_MOVERELYLABEL, TAG_LOOPSOUND, TAG_WAITABSLABEL,
		TAG_SKIPENTRANCELABEL, TAG_DIRECTIONLABEL2, TAG_SOUNDEFFECTLABEL,
		TAG_X_COORD_LABEL, TAG_Y_COORD_LABEL, TAG_COLOR_LABEL, TAG_INPUTLABEL,
		TAG_IMAGEOVERLAY_LABEL, TAG_SINGLESTEP2, TAG_KEEPBEHAVIOR_LABEL
	};

	static const UINT NO_LABELS[] =      {0};
	static const UINT CUEEVENT_L[] =     {TAG_EVENTLABEL, 0};
	static const UINT WAIT_L[] =         {TAG_WAITLABEL, 0};
	static const UINT WAITABS_L[] =      {TAG_WAITABSLABEL, 0};
	static const UINT SPEECH_L[] =       {TAG_DELAYLABEL, TAG_SPEAKERLABEL, TAG_MOODLABEL, TAG_TEXTLABEL, TAG_SOUNDNAME_LABEL, 0};
	static const UINT TEXT_L[] =         {TAG_TEXTLABEL, 0};
	static const UINT TEXT_AND_COLOR_L[]={TAG_TEXTLABEL, TAG_COLOR_LABEL, 0};
	static const UINT ORIENTATION_L[] =  {TAG_DIRECTIONLABEL, 0};
	static const UINT GOTO_L[] =         {TAG_GOTOLABEL, 0};
	static const UINT DISPSPEECH_L[] =   {TAG_DISPLAYSPEECHLABEL, 0};
	static const UINT MUSIC_L[] =        {TAG_MUSICLABEL, 0};
	static const UINT MOVE_L[] =         {TAG_NOTURNING, TAG_SINGLESTEP, 0};
	static const UINT VARSET_L[] =       {TAG_VARNAMETEXTLABEL, TAG_VARVALUELABEL, 0};
	static const UINT VARGET_L[] =       {TAG_VARNAMETEXTLABEL, TAG_VARVALUELABEL, 0};
	static const UINT CUTSCENE_L[] =     {TAG_CUTSCENELABEL, 0};
	static const UINT MOVEREL_L[] =      {TAG_NOTURNING, TAG_SINGLESTEP, TAG_MOVERELXLABEL, TAG_MOVERELYLABEL, 0};
	static const UINT LOOPSOUND_L[] =    {TAG_LOOPSOUND, 0};
	static const UINT XY_L[] =           {TAG_X_COORD_LABEL, TAG_Y_COORD_LABEL, 0};
	static const UINT SKIPENTRANCE_L[] = {TAG_SKIPENTRANCELABEL, 0};
	static const UINT NEWENTITY_L[] =    {TAG_DIRECTIONLABEL2, 0};
	static const UINT EFFECT_L[] =       {TAG_DIRECTIONLABEL, TAG_SOUNDEFFECTLABEL, 0};
	static const UINT INPUT_L[] =        {TAG_INPUTLABEL, 0};
	static const UINT IMAGE_OVERLAY_L[] = { TAG_IMAGEOVERLAY_LABEL, 0 };
	static const UINT FACE_TOWARDS_L[] = { TAG_SINGLESTEP2, 0 };
	static const UINT PUSH_TILE_L[] =    { TAG_DIRECTIONLABEL2, 0 };
	static const UINT NPC_GRAPHIC_L[] =    { TAG_KEEPBEHAVIOR_LABEL, 0 };


	static const UINT* activeLabels[CCharacterCommand::CC_Count] = {
		NO_LABELS,          //CC_Appear
		NO_LABELS,          //CC_AppearAt
		MOVE_L,             //CC_MoveTo
		WAIT_L,             //CC_Wait
		CUEEVENT_L,         //CC_WaitForCueEvent
		NO_LABELS,          //CC_WaitForRect
		SPEECH_L,           //CC_Speech
		NO_LABELS,          //CC_Imperative
		NO_LABELS,          //CC_Disappear
		NO_LABELS,          //CC_TurnIntoMonster
		ORIENTATION_L,      //CC_FaceDirection
		NO_LABELS,          //CC_WaitForNotRect
		NO_LABELS,          //CC_WaitForDoorTo
		GOTO_L,             //CC_Label
		NO_LABELS,          //CC_GoTo
		0,
		0,
		0,
		WAITABS_L,          //CC_WaitForTurn
		NO_LABELS,          //CC_WaitForCleanRoom
		ORIENTATION_L,      //CC_WaitForPlayerToFace
		NO_LABELS,          //CC_ActivateItemAt
		NO_LABELS,          //CC_EndScript
		0,
		0,
		0,
		0,
		DISPSPEECH_L,       //CC_FlushSpeech
		TEXT_L,             //CC_Question
		MUSIC_L,            //CC_SetMusic
		NO_LABELS,          //CC_EndScriptOnExit
		NO_LABELS,          //CC_If
		NO_LABELS,          //CC_IfElse
		NO_LABELS,          //CC_IfEnd
		SKIPENTRANCE_L,     //CC_LevelEntrance
		VARSET_L,           //CC_VarSet
		VARGET_L,           //CC_WaitForVar
		NO_LABELS,          //CC_SetPlayerAppearance
		CUTSCENE_L,         //CC_CutScene
		MOVEREL_L,          //CC_MoveRel
		NO_LABELS,          //CC_PlayerEquipsWeapon
		TEXT_L,             //CC_AnswerOption
		NO_LABELS,          //CC_BuildMarker
		LOOPSOUND_L,        //CC_AmbientSound
		LOOPSOUND_L,        //CC_AmbientSoundAt
		NO_LABELS,          //CC_WaitForNoBuilding
		XY_L,               //CC_PlayVideo
		ORIENTATION_L,      //CC_WaitForPlayerToMove
		NO_LABELS,          //CC_WaitForPlayerToTouchMe
		NPC_GRAPHIC_L,      //CC_SetNPCAppearance
		NO_LABELS,          //CC_SetWaterTraversal
		NO_LABELS,          //CC_StartGlobalScript
		NO_LABELS,          //CC_WaitForItem
		NEWENTITY_L,        //CC_GenerateEntity
		EFFECT_L,           //CC_GameEffect
		TEXT_L,             //CC_RoomLocationText
		TEXT_AND_COLOR_L,   //CC_FlashingText
		NO_LABELS,          //CC_DisplayFilter
		NO_LABELS,          //CC_Build
		MUSIC_L,            //CC_WorldMapMusic
		XY_L,               //CC_WorldMapIcon
		NO_LABELS,          //CC_WorldMapSelect
		NO_LABELS,          //CC_SetPlayerWeapon
		NO_LABELS,          //CC_WaitForSomeoneToPushMe
		ORIENTATION_L,      //CC_WaitForOpenMove
		NO_LABELS,          //CC_WaitForCleanLevel
		NO_LABELS,          //CC_ChallengeCompleted
		NO_LABELS,          //CC_AttackTile
		NO_LABELS,          //CC_SetPlayerStealth
		INPUT_L,            //CC_WaitForPlayerInput
		NO_LABELS,          //CC_Return
		NO_LABELS,          //CC_GoSub
		IMAGE_OVERLAY_L,    //CC_ImageOverlay
		XY_L,               //CC_WorldMapImage
		NO_LABELS,          //CC_WaitForEntityType
		NO_LABELS,          //CC_WaitForNotEntityType
		NO_LABELS,          //CC_TeleportTo
		NO_LABELS,          //CC_TeleportPlayerTo
		NO_LABELS,          //CC_DestroyTrapdoor
		NO_LABELS,          //CC_IfElseIf
		FACE_TOWARDS_L,     //CC_FaceTowards,
		NO_LABELS,          //CC_GetNaturalTarget
		NO_LABELS,           //CC_GetEntityDirection
		NO_LABELS,			//CC_WaitForWeapon
		NO_LABELS,          //CC_Behavior
		NO_LABELS,          //CC_WaitForRemains
		PUSH_TILE_L,        //CC_PushTile
		NO_LABELS,          //CC_SetMovementType
		NO_LABELS           //CC_ReplaceWithDefault
	};
	ASSERT(this->pActionListBox->GetSelectedItem() < CCharacterCommand::CC_Count);

	//Set important default settings.
	if (!this->bRetainFields)
	{
		CListBoxWidget *pFlags = DYN_CAST(CListBoxWidget*, CWidget*,
				this->pAddCommandDialog->GetWidget(TAG_WAITFLAGSLISTBOX));
		pFlags->DeselectAll();
		CTextBoxWidget *pDialogue = DYN_CAST(CTextBoxWidget*, CWidget*,
				this->pAddCommandDialog->GetWidget(TAG_SPEECHTEXT));
		pDialogue->SetText(wszEmpty);
		pDialogue = DYN_CAST(CTextBoxWidget*, CWidget*,
				this->pAddCommandDialog->GetWidget(TAG_GOTOLABELTEXT));
		pDialogue->SetText(wszEmpty);
		CListBoxWidget *pMood = DYN_CAST(CListBoxWidget*, CWidget*,
				this->pAddCommandDialog->GetWidget(TAG_MOODLISTBOX));
		pMood->SelectItem(Mood_Normal);
		CTextBoxWidget *pDuration = DYN_CAST(CTextBoxWidget*, CWidget*,
				this->pAddCommandDialog->GetWidget(TAG_DELAY));
		pDuration->SetText(wszZero);
	}

	const UINT dwSelectedItem = this->pActionListBox->GetSelectedItem();
	UINT wIndex;
	for (wIndex=0; wIndex<NUM_WIDGETS; ++wIndex)
	{
		CWidget *pWidget = this->pAddCommandDialog->GetWidget(widgetTag[wIndex]);
		ASSERT(pWidget);
		pWidget->Hide();
	}
	for (const UINT *widgets = activeWidgets[dwSelectedItem]; *widgets; ++widgets) {
		CWidget *pWidget = this->pAddCommandDialog->GetWidget(*widgets);
		ASSERT(pWidget);
		pWidget->Show();
	}

	for (wIndex=0; wIndex<NUM_LABELS; ++wIndex)
	{
		CWidget *pWidget = this->pAddCommandDialog->GetWidget(labelTag[wIndex]);
		ASSERT(pWidget);
		pWidget->Hide();
	}
	for (const UINT *labels = activeLabels[dwSelectedItem]; *labels; ++labels) {
		CWidget *pWidget = this->pAddCommandDialog->GetWidget(*labels);
		ASSERT(pWidget);
		pWidget->Show();
	}

	//Set name of loaded sound file, if any.
	CButtonWidget *pButton = DYN_CAST(CButtonWidget*, CWidget*,
			this->pAddCommandDialog->GetWidget(TAG_ADDSOUND));
	pButton->SetCaption(g_pTheDB->GetMessageText(this->pSound ? MID_RemoveSound : MID_AddSound));

	pButton = DYN_CAST(CButtonWidget*, CWidget*,
			this->pAddCommandDialog->GetWidget(TAG_TESTSOUND));
	pButton->Enable(this->pSound != NULL);

	CLabelWidget *pSoundName = DYN_CAST(CLabelWidget*, CWidget*,
			this->pAddCommandDialog->GetWidget(TAG_SOUNDNAME_LABEL));
	ASSERT(pSoundName);
	pSoundName->SetText(this->pSound ? this->pSound->DataNameText.c_str() : wszEmpty);
}

//*****************************************************************************
void CCharacterDialogWidget::SelectCharacter()
//Sets custom character dialog widgets to show the selected character with the specified ID.
{
	SetCharacterWidgetStates();
	this->pAddCharacterDialog->Paint();
}

//*****************************************************************************
void CCharacterDialogWidget::SetAnimationSpeed()
//Sets the selected custom character's animation speed to the value
//specified in the text box
{
	HoldCharacter *pChar = GetCustomCharacter();
	if (pChar)
	{
		CTextBoxWidget *pAnimationSpeed = DYN_CAST(CTextBoxWidget*, CWidget*,
				GetWidget(TAG_ANIMATESPEED));
		pChar->animationSpeed = (UINT)pAnimationSpeed->GetNumber();
	}
}

//*****************************************************************************
void CCharacterDialogWidget::SetCustomImage()
//UI for setting the current custom character's avatar or tile set.
{
	HoldCharacter *pChar = GetCustomCharacter();
	if (!pChar)
		return;

	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen->pHold);

	//Image management.
SelectImage:
	UINT dwDataID;
	CEntranceSelectDialogWidget::BUTTONTYPE eButton;
	do {
		dwDataID = pChar->dwDataID_Avatar;
		eButton = pEditRoomScreen->SelectListID(pEditRoomScreen->pEntranceBox,
				pEditRoomScreen->pHold, dwDataID,
				MID_ImageSelectPrompt, CEntranceSelectDialogWidget::Images);
		if (eButton != CEntranceSelectDialogWidget::OK &&
				eButton != CEntranceSelectDialogWidget::Delete)
			return;

		if (eButton == CEntranceSelectDialogWidget::Delete)
		{
			//Remove this image from the database and make another selection.
			pEditRoomScreen->pHold->MarkDataForDeletion(dwDataID);
			pChar->dwDataID_Avatar = 0;
		}
	} while (eButton != CEntranceSelectDialogWidget::OK);

	if (dwDataID)
	{
		//Set to selected image from DB.
		pChar->dwDataID_Avatar = dwDataID;
	} else {
		const UINT dwID = pEditRoomScreen->ImportHoldImage(pEditRoomScreen->pHold->dwHoldID, EXT_PNG | EXT_JPEG);
		if (dwID)
			pChar->dwDataID_Avatar = dwID;
		goto SelectImage;	//return to image management
	}

	SetCharacterWidgetStates();
	this->pAddCharacterDialog->Paint();
}

//*****************************************************************************
void CCharacterDialogWidget::SetCustomTiles()
//UI for setting the current custom character's avatar or tile set.
{
	HoldCharacter *pChar = GetCustomCharacter();
	if (!pChar)
		return;

	CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
			g_pTheSM->GetScreen(SCR_EditRoom));
	ASSERT(pEditRoomScreen->pHold);

	//Image management.
SelectImage:
	UINT dwDataID;
	CEntranceSelectDialogWidget::BUTTONTYPE eButton;
	do {
		dwDataID = pChar->dwDataID_Tiles;
		eButton = pEditRoomScreen->SelectListID(pEditRoomScreen->pEntranceBox,
				pEditRoomScreen->pHold, dwDataID,
				MID_ImageSelectPrompt, CEntranceSelectDialogWidget::Images);
		if (eButton != CEntranceSelectDialogWidget::OK &&
				eButton != CEntranceSelectDialogWidget::Delete)
			return;

		if (eButton == CEntranceSelectDialogWidget::Delete)
		{
			//Remove this image from the database and make another selection.
			pEditRoomScreen->pHold->MarkDataForDeletion(dwDataID);
			pChar->dwDataID_Tiles = 0;
		}
	} while (eButton != CEntranceSelectDialogWidget::OK);

	if (dwDataID)
	{
		//Set to selected image from DB.
		pChar->dwDataID_Tiles = dwDataID;

		//If custom tiles are not the right size, warn the user.
		SDL_Surface *pSurface = g_pTheDBM->LoadImageSurface(dwDataID);
		if (pSurface) {
			if ((pSurface->w % CDrodBitmapManager::CX_TILE) != 0  ||
				(pSurface->h % CDrodBitmapManager::CY_TILE) != 0) {
				pEditRoomScreen->ShowOkMessage(MID_NPCCustomTileSizeWarning);
			}
			SDL_FreeSurface(pSurface);
		}
	} else {
		const UINT dwID = pEditRoomScreen->ImportHoldImage(pEditRoomScreen->pHold->dwHoldID, EXT_PNG);
		if (dwID)
			pChar->dwDataID_Tiles = dwID;
		goto SelectImage;	//return to image management
	}

	SetCharacterWidgetStates();
	this->pAddCharacterDialog->Paint();
}

//*****************************************************************************
void CCharacterDialogWidget::SetDefaultAvatar()
//Reverts the current custom character's avatar to the default.
{
	HoldCharacter *pChar = GetCustomCharacter();
	if (!pChar)
		return;

	if (!pChar->dwDataID_Avatar)
		return;

	pChar->dwDataID_Avatar = 0;
	SetCharacterWidgetStates();
	this->pAddCharacterDialog->Paint();
}

//*****************************************************************************
void CCharacterDialogWidget::SetDefaultTiles()
//Reverts the current custom character's tileset to the default.
{
	HoldCharacter *pChar = GetCustomCharacter();
	if (!pChar)
		return;

	if (!pChar->dwDataID_Tiles)
		return;

	pChar->dwDataID_Tiles = 0;
	SetCharacterWidgetStates();
	this->pAddCharacterDialog->Paint();
}

//*****************************************************************************
void CCharacterDialogWidget::SetCustomGraphic()
//Sets the selected custom character's graphic to indicated choice.
{
	HoldCharacter *pChar = GetCustomCharacter();
	if (pChar)
	{
		CListBoxWidget *pCharGraphicList = DYN_CAST(CListBoxWidget*, CWidget*,
				GetWidget(TAG_CHARGRAPHICLISTBOX));
		pChar->wType = pCharGraphicList->GetSelectedItem();
	}

	SetCharacterWidgetStates();
	this->pAddCharacterDialog->Paint();
}

//*****************************************************************************
void CCharacterDialogWidget::SetCharacterWidgetStates()
//Set widgets for custom character dialog.
{
	const bool bCharExists = !this->pCharListBox->IsEmpty();

	CWidget *pWidget = this->pAddCharacterDialog->GetWidget(TAG_ADDCHARACTER);
	pWidget->Enable(!this->pCharNameText->IsEmpty());

	pWidget = this->pAddCharacterDialog->GetWidget(TAG_DELETECHARACTER);
	pWidget->Enable(bCharExists);

	pWidget = this->pAddCharacterDialog->GetWidget(TAG_CUSTOMAVATAR);
	pWidget->Enable(bCharExists);

	pWidget = this->pAddCharacterDialog->GetWidget(TAG_CUSTOMTILES);
	pWidget->Enable(bCharExists);

	pWidget = this->pAddCharacterDialog->GetWidget(TAG_EDITDEFAULTSCRIPT);
	pWidget->Enable(bCharExists);

	CListBoxWidget *pCharGraphicList = DYN_CAST(CListBoxWidget*, CWidget*,
			GetWidget(TAG_CHARGRAPHICLISTBOX));
	CFaceWidget *pFace = DYN_CAST(CFaceWidget*, CWidget*, GetWidget(TAG_AVATARFACE));
	CImageWidget *pTiles = DYN_CAST(CImageWidget*, CWidget*, GetWidget(TAG_TILESIMAGE));
	CTextBoxWidget *pAnimateSpeed = DYN_CAST(CTextBoxWidget*, CWidget*, GetWidget(TAG_ANIMATESPEED));

	CWidget *pDefaultAvatar = GetWidget(TAG_DEFAULTAVATAR);
	CWidget *pDefaultTiles = GetWidget(TAG_DEFAULTTILES);

	CLabelWidget *pIDLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAG_CUSTOM_NPC_ID));

	HoldCharacter *pChar = GetCustomCharacter();
	if (pChar)
	{
		//Wait to initialize these values until this method is called, when we
		//are sure CX_TILE/CY_TILE have been set.
		static const UINT CX_TILES = 9 * CDrodBitmapManager::CX_TILE;
		static const UINT CY_TILES = 6 * CDrodBitmapManager::CY_TILE;

		pCharGraphicList->SelectItem(pChar->wType);
		pFace->SetCharacter(PlayerRole, getSpeakerType(MONSTERTYPE(pChar->wType)), pChar);
		pDefaultAvatar->Enable(pChar->dwDataID_Avatar != 0);

		const bool bHasTiles = pChar->dwDataID_Tiles != 0;
		pDefaultTiles->Enable(bHasTiles);
		if (bHasTiles)
		{
			SDL_Surface *pSurface = g_pTheDBM->LoadImageSurface(pChar->dwDataID_Tiles);
			pTiles->SetImage(pSurface);
			if (pTiles->GetW() > CX_TILES) //bounds checking
				pTiles->SetWidth(CX_TILES);
			if (pTiles->GetH() > CY_TILES)
				pTiles->SetHeight(CY_TILES);
		} else {
			pTiles->SetImage((SDL_Surface*)NULL);
		}

		WCHAR temp[16];
		pAnimateSpeed->Enable(bHasTiles);
		pAnimateSpeed->SetText(_itoW((int)pChar->animationSpeed, temp, 10));

		WSTRING IDtext = g_pTheDB->GetMessageText(MID_CustomNPCID);
		IDtext += wszColon;
		IDtext += wszSpace;
		IDtext += _itoW(pChar->dwCharID, temp, 10);
		pIDLabel->SetText(IDtext.c_str());
	} else {
		pFace->SetCharacter(PlayerRole, getSpeakerType(
				MONSTERTYPE(pCharGraphicList->GetSelectedItem())), NULL);
		pDefaultAvatar->Disable();
		pDefaultTiles->Disable();
		pAnimateSpeed->SetText(wszEmpty);
		pAnimateSpeed->Disable();
		pIDLabel->SetText(wszEmpty);
	}
}

//*****************************************************************************
void CCharacterDialogWidget::SetCommandParametersFromWidgets(
//Set command parameters according to widget values.
//
//Params:
	CListBoxWidget *pActiveCommandList, COMMANDPTR_VECTOR& commands)
{
	ASSERT(this->pCommand);

	//When editing a label, or changing a label to a different command type.
	bool bRemovedLabel = false;
	CCharacterCommand::CharCommand oldCommandType = CCharacterCommand::CC_Count;
	if (this->bEditingCommand)
	{
		oldCommandType = this->pCommand->command;
		if (oldCommandType == CCharacterCommand::CC_Label)
		{
			this->pGotoLabelListBox->RemoveItem(this->pCommand->x);
			bRemovedLabel = true;
		}
	}

	this->pCommand->command =
		(CCharacterCommand::CharCommand)this->pActionListBox->GetSelectedItem();
	if (bRemovedLabel && this->pCommand->command == CCharacterCommand::CC_Label)
		bRemovedLabel = false;

	//When the command is changed, reset fields to avoid bad values, possibly leading to crashes.
	if (oldCommandType != this->pCommand->command)
	{
		this->pCommand->x = this->pCommand->y = 0;
		this->pCommand->w = this->pCommand->h = 0;
		this->pCommand->flags = 0;
		this->pCommand->label.resize(0);

		if (this->pCommand->pSpeech && this->pSound == this->pCommand->pSpeech->GetSound())
			this->pSound = NULL;

		delete this->pCommand->pSpeech;
		this->pCommand->pSpeech = NULL;
	}

	CCharacterCommand::CharCommand c = this->pCommand->command;
	switch (c)
	{
		case CCharacterCommand::CC_MoveTo:
		{
			this->pCommand->w = this->pOnOffListBox->GetSelectedItem();
			this->pCommand->h = this->pOnOffListBox2->GetSelectedItem();

			//Add set bit-fields.
			this->pCommand->flags = 0;
			CIDSet flagSet = this->pWaitFlagsListBox->GetSelectedItems();
			for (CIDSet::const_iterator flag = flagSet.begin();
					flag != flagSet.end(); ++flag)
				this->pCommand->flags += *flag;

			if (this->pCommand->flags)
				AddCommand();
			else
				QueryXY();
		}
		break;
		
		case CCharacterCommand::CC_MoveRel:
		{
			CTextBoxWidget *pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_MOVERELX));
			ASSERT(pRel);
			this->pCommand->x = (UINT)(_Wtoi(pRel->GetText()));
			pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_MOVERELY));
			ASSERT(pRel);
			this->pCommand->y = (UINT)(_Wtoi(pRel->GetText()));

			this->pCommand->w = this->pOnOffListBox->GetSelectedItem();
			this->pCommand->h = this->pOnOffListBox2->GetSelectedItem();
			AddCommand();
		}
		break;

		case CCharacterCommand::CC_WaitForDoorTo:
			this->pCommand->w = this->pOpenCloseListBox->GetSelectedItem();
			//NO BREAK
		case CCharacterCommand::CC_AppearAt:
		case CCharacterCommand::CC_ActivateItemAt:
		case CCharacterCommand::CC_TeleportTo:
		case CCharacterCommand::CC_TeleportPlayerTo:
			QueryXY();
		break;
		case CCharacterCommand::CC_AttackTile:
			this->pCommand->flags = this->pAttackTileListBox->GetSelectedItem();
			QueryXY();
		break;
		case CCharacterCommand::CC_PushTile:
			this->pCommand->w = this->pDirectionListBox2->GetSelectedItem();
			QueryXY();
		break;

		case CCharacterCommand::CC_Wait:
		case CCharacterCommand::CC_WaitForTurn:
		{
			CTextBoxWidget *pDelay = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_WAIT));
			ASSERT(pDelay);
			this->pCommand->x = _Wtoi(pDelay->GetText());
			AddCommand();
		}
		break;
		case CCharacterCommand::CC_WaitForCueEvent:
		{
			this->pCommand->x = this->pEventListBox->GetSelectedItem();
			AddCommand();
		}
		break;
		case CCharacterCommand::CC_WaitForRect:
		case CCharacterCommand::CC_WaitForNotRect:
		{
			//Add set bit-fields.
			this->pCommand->flags = 0;
			CIDSet flagSet = this->pWaitFlagsListBox->GetSelectedItems();
			for (CIDSet::const_iterator flag = flagSet.begin();
					flag != flagSet.end(); ++flag)
				this->pCommand->flags += *flag;

			QueryRect();
		}
		break;
		case CCharacterCommand::CC_WaitForEntityType:
		case CCharacterCommand::CC_WaitForNotEntityType:
			this->pCommand->flags = this->pAddCommandGraphicListBox->GetSelectedItem();
			QueryRect();
		break;

		case CCharacterCommand::CC_WaitForRemains:
			this->pCommand->flags = this->pRemainsListBox->GetSelectedItem();
			QueryRect();
		break;

		case CCharacterCommand::CC_WaitForWeapon:
		{
			//Add set bit-fields.
			this->pCommand->flags = 0;
			CIDSet flagSet = this->pWeaponFlagsListBox->GetSelectedItems();
			for (CIDSet::const_iterator flag = flagSet.begin();
				flag != flagSet.end(); ++flag)
				this->pCommand->flags += *flag;

			QueryXY();
		}
		break;

		case CCharacterCommand::CC_BuildMarker:
			this->pCommand->flags = this->pBuildMarkerListBox->GetSelectedItem();
			QueryRect();
			break;

		case CCharacterCommand::CC_Build:
			this->pCommand->flags = this->pBuildItemsListBox->GetSelectedItem();
			QueryRect();
		break;

		case CCharacterCommand::CC_GetEntityDirection:
			QueryXY();
			break;

		case CCharacterCommand::CC_WaitForItem:
			this->pCommand->flags = this->pWaitForItemsListBox->GetSelectedItem();
			QueryRect();
		break;

		case CCharacterCommand::CC_Speech:
		{
			//Speech dialog.
			CTextBoxWidget *pDelay = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_DELAY));
			ASSERT(pDelay);
			CListBoxWidget *pMood = DYN_CAST(CListBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_MOODLISTBOX));
			ASSERT(pMood);
			CTextBoxWidget *pDialogue = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_SPEECHTEXT));
			ASSERT(pDialogue);

			if (!this->pCommand->pSpeech)
				this->pCommand->pSpeech = g_pTheDB->Speech.GetNew();
			this->pCommand->pSpeech->dwDelay = _Wtoi(pDelay->GetText());
			this->pCommand->pSpeech->wCharacter = this->pSpeakerListBox->GetSelectedItem();
			this->pCommand->pSpeech->wMood = pMood->GetSelectedItem();
			const bool bTextWasModified = this->bEditingCommand && WCScmp(
					(const WCHAR*)this->pCommand->pSpeech->MessageText, pDialogue->GetText()) != 0;
			this->pCommand->pSpeech->MessageText = pDialogue->GetText();

			//Import sound clip, if desired.
			if (this->pSound)
			{
				this->pCommand->pSpeech->SetSound(this->pSound);
				this->pSound = NULL;
			}
			if (this->pCommand->pSpeech->wCharacter != Speaker_Custom ||
					bTextWasModified) //don't force repositioning custom text if the user just wanted to modify the text
				AddCommand();
			else {
				//User decides where to place this text message in room.
				QueryXY();
			}
		}
		break;

		case CCharacterCommand::CC_Imperative:
			this->pCommand->x = this->pImperativeListBox->GetSelectedItem();
			AddCommand();
		break;

		case CCharacterCommand::CC_Behavior:
			this->pCommand->x = this->pBehaviorListBox->GetSelectedItem();
			this->pCommand->y = this->pOnOffListBox3->GetSelectedItem();
			AddCommand();
		break;

		case CCharacterCommand::CC_SetMovementType:
			this->pCommand->x = this->pMovementTypeListBox->GetSelectedItem();
			AddCommand();
		break;

		case CCharacterCommand::CC_SetPlayerStealth:
			this->pCommand->x = this->pStealthListBox->GetSelectedItem();
			AddCommand();
		break;
		case CCharacterCommand::CC_SetWaterTraversal:
			this->pCommand->x = this->pWaterTraversalListBox->GetSelectedItem();
			AddCommand();
		break;

		case CCharacterCommand::CC_GenerateEntity:
			this->pCommand->w = this->pDirectionListBox2->GetSelectedItem();
			this->pCommand->h = this->pAddCommandGraphicListBox->GetSelectedItem();
			QueryXY();
		break;

		case CCharacterCommand::CC_GameEffect:
			this->pCommand->w = this->pDirectionListBox3->GetSelectedItem();
			this->pCommand->h = this->pVisualEffectsListBox->GetSelectedItem();
			this->pCommand->flags = this->pOnOffListBox3->GetSelectedItem();
			QueryXY();
		break;

		case CCharacterCommand::CC_StartGlobalScript:
			this->pCommand->x = this->pGlobalScriptListBox->GetSelectedItem();
			if (this->pCommand->x) {
				AddCommand();
			} else {
				//No custom character specified.  Don't add the command.
				RollbackCommand();
			}
		break;

		case CCharacterCommand::CC_FlushSpeech:
		case CCharacterCommand::CC_PlayerEquipsWeapon:
			this->pCommand->x = this->pOnOffListBox->GetSelectedItem();
			AddCommand();
		break;

		case CCharacterCommand::CC_FaceDirection:
		case CCharacterCommand::CC_WaitForPlayerToFace:
		case CCharacterCommand::CC_WaitForPlayerToMove:
		case CCharacterCommand::CC_WaitForOpenMove:
			this->pCommand->x = this->pDirectionListBox->GetSelectedItem();
			AddCommand();
		break;
		
		case CCharacterCommand::CC_FaceTowards:
		{
			this->pCommand->w = this->pOnOffListBox->GetSelectedItem();

			//Add set bit-fields.
			this->pCommand->flags = 0;
			CIDSet flagSet = this->pWaitFlagsListBox->GetSelectedItems();
			for (CIDSet::const_iterator flag = flagSet.begin();
					flag != flagSet.end(); ++flag)
				this->pCommand->flags += *flag;

			if (this->pCommand->flags)
				AddCommand();
			else
				QueryXY();
		}
		break;
		case CCharacterCommand::CC_WaitForPlayerInput:
			this->pCommand->x = this->pInputListBox->GetSelectedItem();
			AddCommand();
		break;

		case CCharacterCommand::CC_DisplayFilter:
			this->pCommand->x = this->pDisplayFilterListBox->GetSelectedItem();
			AddCommand();
		break;

		case CCharacterCommand::CC_WorldMapSelect:
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
				g_pTheSM->GetScreen(SCR_EditRoom));
			ASSERT(pEditRoomScreen->pHold);
			UINT dwVal = this->pCommand->x;
			if (pEditRoomScreen->SelectListID(pEditRoomScreen->pEntranceBox, pEditRoomScreen->pHold,
					dwVal, MID_SelectWorldMapPrompt, CEntranceSelectDialogWidget::WorldMaps) != CEntranceSelectDialogWidget::OK)
			{
				RollbackCommand();
				break;
			}
			this->pCommand->x = LevelExit::ConvertWorldMapID(dwVal);
			if (this->pCommand->x) {
				AddCommand();
			} else {
				//No map specified.  Don't add the command.
				RollbackCommand();
			}
		}
		break;

		case CCharacterCommand::CC_WorldMapMusic:
		case CCharacterCommand::CC_SetMusic:
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
				g_pTheSM->GetScreen(SCR_EditRoom));

			//Either music ID or name is set.
			this->pCommand->x = this->pMusicListBox->GetSelectedItem();
			this->pCommand->label = wszEmpty;
			if ((int)this->pCommand->x == SONGID_CUSTOM)
			{
				const UINT dwVal = pEditRoomScreen->SelectMediaID(this->pCommand->y, CSelectMediaDialogWidget::Sounds);
				if (dwVal) {
					this->pCommand->y = dwVal;
				} else {
					//No dataID was specified.  Don't add the command.
					RollbackCommand();
					break;
				}
			} else {
				this->pCommand->y = 0;
				if ((int)this->pCommand->x >= SONGID_COUNT)
				{
					this->pCommand->label = this->pMusicListBox->GetSelectedItemText();
					if (!this->pCommand->label.empty())
						this->pCommand->x = 0;
					this->pCommand->y = 0;
				}
			}
			AddCommand();
		}
		break;

		case CCharacterCommand::CC_Label:
		{
			CTextBoxWidget *pLabelText = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_GOTOLABELTEXT));
			ASSERT(pLabelText);
			//Provide an ID for this label.
			if (oldCommandType != CCharacterCommand::CC_Label)
				this->pCommand->x = ++this->wIncrementedLabel;
			ASSERT(this->pCommand->x);

			//Ensure this label is unique.
			WSTRING text = pLabelText->GetText();

			stripTrailingWhitespace(text);

			if (this->bEditingCommand)
				this->pCommand->label = wszEmpty; //don't match against the old label
			while (GetCommandWithLabelText(commands, text.c_str()) != NULL)
				text += wszHyphen;
			this->pCommand->label = text;

			//Default empty label to label ID.
			if (this->pCommand->label.empty())
			{
				WCHAR text[MAX_TEXT_LABEL_SIZE];
				this->pCommand->label = _itoW(this->pCommand->x, text, 10);
			}
			AddCommand();
		}
		break;

		case CCharacterCommand::CC_AnswerOption:
		{
			CTextBoxWidget *pLabelText = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_GOTOLABELTEXT));
			ASSERT(pLabelText);
			this->pCommand->x = this->pGotoLabelListBox->GetSelectedItem();
			if (!this->pCommand->pSpeech)
				this->pCommand->pSpeech = g_pTheDB->Speech.GetNew();
			this->pCommand->pSpeech->MessageText = pLabelText->GetText();
			if (this->pCommand->x && WCSlen(pLabelText->GetText()))
			{
				AddCommand();
			} else {
				//No answer text or goto destination label was specified.
				//Don't add the command.
				RollbackCommand();
			}
		}
		break;

		case CCharacterCommand::CC_FlashingText:
		{
			CTextBoxWidget *pLabelText = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_TEXT2));

			const WCHAR *pColorText = pLabelText->GetText();
			TranslateColorText(WSTRING(pColorText), this->pCommand);
		}
		//no break
		case CCharacterCommand::CC_Question:
		case CCharacterCommand::CC_RoomLocationText:
			{
				CTextBoxWidget *pLabelText = DYN_CAST(CTextBoxWidget*, CWidget*,
						this->pAddCommandDialog->GetWidget(TAG_GOTOLABELTEXT));
				ASSERT(pLabelText);
				if (!this->pCommand->pSpeech)
					this->pCommand->pSpeech = g_pTheDB->Speech.GetNew();
				this->pCommand->pSpeech->MessageText = pLabelText->GetText();
			}
			AddCommand();
		break;

		case CCharacterCommand::CC_GoSub:
		case CCharacterCommand::CC_GoTo:
			this->pCommand->x = this->pGotoLabelListBox->GetSelectedItem();
			if (this->pCommand->x) {
				AddCommand();
			} else {
				//No goto destination label was specified -- don't add the command.
				RollbackCommand();
			}
		break;

		case CCharacterCommand::CC_LevelEntrance:
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_EditRoom));
			ASSERT(pEditRoomScreen->pHold);
			UINT dwVal = this->pCommand->x;
			if (pEditRoomScreen->SelectListID(pEditRoomScreen->pEntranceBox, pEditRoomScreen->pHold,
					dwVal, MID_ExitLevelPrompt) == CEntranceSelectDialogWidget::OK)
			{
				this->pCommand->x = dwVal;
				this->pCommand->y = this->pOnOffListBox->GetSelectedItem();
				AddCommand();
			} else {
				RollbackCommand();
			}
		}
		break;

		case CCharacterCommand::CC_WorldMapIcon:
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_EditRoom));
			ASSERT(pEditRoomScreen->pHold);

			UINT dwVal = this->pCommand->w;
			if (pEditRoomScreen->SelectListID(pEditRoomScreen->pEntranceBox, pEditRoomScreen->pHold,
					dwVal, MID_ExitLevelPrompt) == CEntranceSelectDialogWidget::OK)
			{
				CTextBoxWidget *pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
						this->pAddCommandDialog->GetWidget(TAG_X_COORD));
				ASSERT(pRel);
				this->pCommand->x = (UINT)(_Wtoi(pRel->GetText()));

				pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
						this->pAddCommandDialog->GetWidget(TAG_Y_COORD));
				ASSERT(pRel);
				this->pCommand->y = (UINT)(_Wtoi(pRel->GetText()));

				this->pCommand->w = dwVal;
				this->pCommand->h = this->pAddCommandGraphicListBox->GetSelectedItem();
				this->pCommand->flags = this->pWorldMapIconFlagListBox->GetSelectedItem();
				AddCommand();
			}
		}
		break;
		case CCharacterCommand::CC_WorldMapImage:
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
					g_pTheSM->GetScreen(SCR_EditRoom));
			ASSERT(pEditRoomScreen->pHold);

			const UINT dwFlag = this->pWorldMapImageFlagListBox->GetSelectedItem();
			UINT dwMedia = 0;
			if (dwFlag != ScriptFlag::WMI_OFF)
			{
				dwMedia = pEditRoomScreen->SelectMediaID(this->pCommand->h, CSelectMediaDialogWidget::Images);
				if (!dwMedia) {
					//Don't add the command.
					RollbackCommand();
					break;
				}
			}

			UINT dwEntrance = this->pCommand->w;
			if (pEditRoomScreen->SelectListID(pEditRoomScreen->pEntranceBox, pEditRoomScreen->pHold,
					dwEntrance, MID_ExitLevelPrompt) == CEntranceSelectDialogWidget::OK)
			{
				CTextBoxWidget *pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
						this->pAddCommandDialog->GetWidget(TAG_X_COORD));
				ASSERT(pRel);
				this->pCommand->x = (UINT)(_Wtoi(pRel->GetText()));

				pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
						this->pAddCommandDialog->GetWidget(TAG_Y_COORD));
				ASSERT(pRel);
				this->pCommand->y = (UINT)(_Wtoi(pRel->GetText()));

				this->pCommand->w = dwEntrance;
				this->pCommand->h = dwMedia;
				this->pCommand->flags = dwFlag;
				AddCommand();
			}
		}
		break;

		case CCharacterCommand::CC_VarSet:
		case CCharacterCommand::CC_WaitForVar:
		{
			this->pCommand->x = this->pVarListBox->GetSelectedItem();
			if (!this->pCommand->x)
			{
				//No variable specified. Don't add the command.
				RollbackCommand();
				break;
			}
			this->pCommand->y =
				c == CCharacterCommand::CC_VarSet ?
					this->pVarOpListBox->GetSelectedItem() :
					this->pVarCompListBox->GetSelectedItem();
			this->pCommand->w = 0; //default

			const bool bTextVar =
				(c == CCharacterCommand::CC_VarSet &&
					(this->pCommand->y == ScriptVars::AssignText ||
					this->pCommand->y == ScriptVars::AppendText)) ||
				(c == CCharacterCommand::CC_WaitForVar &&
					this->pCommand->y == ScriptVars::EqualsText);
			if (bTextVar)
			{
				CTextBoxWidget *pVarText = DYN_CAST(CTextBoxWidget*, CWidget*,
						this->pAddCommandDialog->GetWidget(TAG_VARNAMETEXTINPUT));
				ASSERT(pVarText);
				this->pCommand->label = pVarText->GetText();
			} else {
				CTextBoxWidget *pVarOperand = DYN_CAST(CTextBoxWidget*, CWidget*,
						this->pAddCommandDialog->GetWidget(TAG_VARVALUE));
				ASSERT(pVarOperand);
				const WCHAR *pOperandText = pVarOperand->GetText();
				ASSERT(pOperandText);

				//Is operand just a number or is it a more complex expression?
				if (isWInteger(pOperandText))
				{
					this->pCommand->w = _Wtoi(pOperandText);
					this->pCommand->label.resize(0);
				} else {
					this->pCommand->label = pOperandText;
				}
			}

			AddCommand();
		}
		break;

		case CCharacterCommand::CC_SetPlayerAppearance:
			this->pCommand->x = this->pPlayerGraphicListBox->GetSelectedItem();
			AddCommand();
		break;
		case CCharacterCommand::CC_SetNPCAppearance:
			this->pCommand->x = this->pAddCommandGraphicListBox->GetSelectedItem();
			this->pCommand->y = this->pOnOffListBox4->GetSelectedItem();
			AddCommand();
		break;

		case CCharacterCommand::CC_SetPlayerWeapon:
			this->pCommand->x = this->pWeaponListBox->GetSelectedItem();
			AddCommand();
		break;

		case CCharacterCommand::CC_CutScene:
		{
			CTextBoxWidget *pDelay = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_WAIT));
			ASSERT(pDelay);
			this->pCommand->x = _Wtoi(pDelay->GetText());

			//Force cut scenes to be correctly consistent in demo playback.
			static const UINT wMaxMoveDelay = 255*10; //ms
			if (this->pCommand->x > wMaxMoveDelay)
				this->pCommand->x = wMaxMoveDelay;

			AddCommand();
		}
		break;

		case CCharacterCommand::CC_AmbientSound:
		case CCharacterCommand::CC_AmbientSoundAt:
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
				g_pTheSM->GetScreen(SCR_EditRoom));

			const UINT dwVal = pEditRoomScreen->SelectMediaID(this->pCommand->w, CSelectMediaDialogWidget::Sounds);
			this->pCommand->w = dwVal;
			this->pCommand->h = this->pOnOffListBox->GetSelectedItem();
			if (c == CCharacterCommand::CC_AmbientSound)
				AddCommand();
			else
				QueryXY();
		}
		break;

		case CCharacterCommand::CC_PlayVideo:
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
				g_pTheSM->GetScreen(SCR_EditRoom));

			const UINT dwVal = pEditRoomScreen->SelectMediaID(this->pCommand->w, CSelectMediaDialogWidget::Videos);
			if (dwVal)
			{
				CTextBoxWidget *pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
						this->pAddCommandDialog->GetWidget(TAG_X_COORD));
				ASSERT(pRel);
				this->pCommand->x = (UINT)(_Wtoi(pRel->GetText()));
				pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
						this->pAddCommandDialog->GetWidget(TAG_Y_COORD));
				ASSERT(pRel);
				this->pCommand->y = (UINT)(_Wtoi(pRel->GetText()));

				this->pCommand->w = dwVal;
				AddCommand();
			} else {
				//Don't add the command.
				RollbackCommand();
			}
		}
		break;

		case CCharacterCommand::CC_ImageOverlay:
		{
			CEditRoomScreen *pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
				g_pTheSM->GetScreen(SCR_EditRoom));

			CTextBox2DWidget *pText = DYN_CAST(CTextBox2DWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_IMAGEOVERLAYTEXT));
			ASSERT(pText);
			this->pCommand->label = pText->GetText();

			CImageOverlay tempImage(this->pCommand->label, 0, 0);
			if (tempImage.clearsImageOverlays() != ImageOverlayCommand::NO_LAYERS ||
					tempImage.clearsImageOverlayGroup() != ImageOverlayCommand::NO_GROUP) {
				//No image required.
				this->pCommand->w = 0;
			} else {
				const UINT dwVal = pEditRoomScreen->SelectMediaID(this->pCommand->w, CSelectMediaDialogWidget::Images);
				if (!dwVal) {
					//Don't add the command.
					RollbackCommand();
					break;
				}
				this->pCommand->w = dwVal;
			}

			AddCommand();
		}
		break;

		case CCharacterCommand::CC_WaitForNoBuilding:
		case CCharacterCommand::CC_DestroyTrapdoor:
			QueryRect();
		break;

		case CCharacterCommand::CC_ChallengeCompleted:
		{
			CTextBoxWidget *pTextWidget = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_GOTOLABELTEXT));
			ASSERT(pTextWidget);
			const WCHAR *pText = pTextWidget->GetText();
			ASSERT(pText);
			this->pCommand->label = pText;

			AddCommand();
		}
		break;

		case CCharacterCommand::CC_GetNaturalTarget:
		{
			this->pCommand->x = this->pNaturalTargetTypesListBox->GetSelectedItem();
			AddCommand();
		}
		break;

		case CCharacterCommand::CC_Appear:
		case CCharacterCommand::CC_Disappear:
		case CCharacterCommand::CC_EndScript:
		case CCharacterCommand::CC_TurnIntoMonster:
		case CCharacterCommand::CC_ReplaceWithDefault:
		case CCharacterCommand::CC_WaitForCleanRoom:
		case CCharacterCommand::CC_WaitForCleanLevel:
		case CCharacterCommand::CC_WaitForPlayerToTouchMe:
		case CCharacterCommand::CC_WaitForSomeoneToPushMe:
		case CCharacterCommand::CC_EndScriptOnExit:
		case CCharacterCommand::CC_If:
		case CCharacterCommand::CC_IfElse:
		case CCharacterCommand::CC_IfElseIf:
		case CCharacterCommand::CC_IfEnd:
		case CCharacterCommand::CC_Return:
			AddCommand();
		break;

		//Deprecated commands.
		case CCharacterCommand::CC_GotoIf:
		case CCharacterCommand::CC_WaitForHalph:
		case CCharacterCommand::CC_WaitForNotHalph:
		case CCharacterCommand::CC_WaitForMonster:
		case CCharacterCommand::CC_WaitForNotMonster:
		case CCharacterCommand::CC_WaitForCharacter:
		case CCharacterCommand::CC_WaitForNotCharacter:
		default: ASSERT(!"Invalid character command"); break;
	}

	//Gotos might have become invalid.  Display that immediately.
	if (bRemovedLabel)
		PopulateCommandDescriptions(pActiveCommandList, commands);
}

//*****************************************************************************
void CCharacterDialogWidget::RollbackCommand()
{
	if (!this->bEditingCommand)
		delete this->pCommand;

	this->pCommand = NULL;
}

//*****************************************************************************
void CCharacterDialogWidget::SetWidgetsFromCommandParameters()
//Set state of command settings widgets from command state.
{
	ASSERT(this->pCommand);
	ASSERT(!this->pSound);
	WCHAR temp[500];
	CCharacterCommand::CharCommand c = this->pCommand->command;
	switch (c)
	{
		case CCharacterCommand::CC_MoveRel:
		{
			CTextBoxWidget *pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_MOVERELX));
			ASSERT(pRel);
			pRel->SetText(_itoW((int)this->pCommand->x, temp, 10));
			pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_MOVERELY));
			ASSERT(pRel);
			pRel->SetText(_itoW((int)this->pCommand->y, temp, 10));

			this->pOnOffListBox->SelectItem(this->pCommand->w);
			this->pOnOffListBox2->SelectItem(this->pCommand->h);
		}
		break;
		
		case CCharacterCommand::CC_MoveTo:
			this->pOnOffListBox->SelectItem(this->pCommand->w);
			this->pOnOffListBox2->SelectItem(this->pCommand->h);
			SetBitFlags();
		break;

		case CCharacterCommand::CC_FaceTowards:
			this->pOnOffListBox->SelectItem(this->pCommand->w);
			SetBitFlags();
		break;

		case CCharacterCommand::CC_Wait:
		case CCharacterCommand::CC_WaitForTurn:
		{

			CTextBoxWidget *pDelay = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_WAIT));
			ASSERT(pDelay);
			pDelay->SetText(_itoW(this->pCommand->x, temp, 10));
		}
		break;

		case CCharacterCommand::CC_WaitForCueEvent:
			this->pEventListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_WaitForDoorTo:
			this->pOpenCloseListBox->SelectItem(this->pCommand->w);
		break;

		case CCharacterCommand::CC_AttackTile:
			this->pAttackTileListBox->SelectItem(this->pCommand->flags);
		break;

		case CCharacterCommand::CC_PushTile:
			this->pDirectionListBox2->SelectItem(this->pCommand->w);
			break;

		case CCharacterCommand::CC_LevelEntrance:
			this->pOnOffListBox->SelectItem(this->pCommand->y);
		break;

		case CCharacterCommand::CC_FaceDirection:
		case CCharacterCommand::CC_WaitForPlayerToFace:
		case CCharacterCommand::CC_WaitForPlayerToMove:
		case CCharacterCommand::CC_WaitForOpenMove:
			this->pDirectionListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_WaitForPlayerInput:
			this->pInputListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_DisplayFilter:
			this->pDisplayFilterListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_BuildMarker:
			this->pBuildMarkerListBox->SelectItem(this->pCommand->flags);
			break;

		case CCharacterCommand::CC_Build:
			this->pBuildItemsListBox->SelectItem(this->pCommand->flags);
			break;

		case CCharacterCommand::CC_WaitForItem:
			this->pWaitForItemsListBox->SelectItem(this->pCommand->flags);
		break;

		case CCharacterCommand::CC_WorldMapSelect:
		break;

		case CCharacterCommand::CC_WorldMapMusic:
		case CCharacterCommand::CC_SetMusic:
			if ((int)this->pCommand->x < SONGID_COUNT)
				this->pMusicListBox->SelectItem(this->pCommand->x);
			else if (this->pCommand->x == static_cast<UINT>(SONGID_CUSTOM))
			{
				this->pMusicListBox->SelectItem(this->pCommand->x);
			} else {
				for (UINT wIndex=this->pMusicListBox->GetItemCount(); wIndex--; )
					if (!this->pCommand->label.compare(this->pMusicListBox->GetTextAtLine(wIndex)))
					{
						this->pMusicListBox->SelectLine(wIndex);
						break;
					}
			}
		break;

		case CCharacterCommand::CC_WorldMapIcon:
		{
			CTextBoxWidget *pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_X_COORD));
			ASSERT(pRel);
			pRel->SetText(_itoW((int)this->pCommand->x, temp, 10));

			pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_Y_COORD));
			ASSERT(pRel);
			pRel->SetText(_itoW((int)this->pCommand->y, temp, 10));

			this->pAddCommandGraphicListBox->SelectItem(this->pCommand->h);
			this->pWorldMapIconFlagListBox->SelectItem(this->pCommand->flags);
		}
		break;
		case CCharacterCommand::CC_WorldMapImage:
		{
			CTextBoxWidget *pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_X_COORD));
			ASSERT(pRel);
			pRel->SetText(_itoW((int)this->pCommand->x, temp, 10));

			pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_Y_COORD));
			ASSERT(pRel);
			pRel->SetText(_itoW((int)this->pCommand->y, temp, 10));

			this->pWorldMapImageFlagListBox->SelectItem(this->pCommand->flags);
		}
		break;

		case CCharacterCommand::CC_Speech:
		{
			CTextBoxWidget *pDelay = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_DELAY));
			ASSERT(pDelay);
			CTextBoxWidget *pDialogue = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_SPEECHTEXT));
			ASSERT(pDialogue);

			CDbSpeech *pSpeech = this->pCommand->pSpeech;
			ASSERT(pSpeech);
			if (pSpeech)	//robustness
			{
				pDelay->SetText(_itoW(pSpeech->dwDelay, temp, 10));
				this->pSpeakerListBox->SelectItem(pSpeech->wCharacter);
				this->pMoodListBox->SelectItem(pSpeech->wMood);
				pDialogue->SetText((const WCHAR*)pSpeech->MessageText);

				this->pSound = (CDbDatum*)pSpeech->GetSound(); //loads sound clip from DB
			}
		}
		break;

		case CCharacterCommand::CC_Imperative:
			this->pImperativeListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_Behavior:
			this->pBehaviorListBox->SelectItem(this->pCommand->x);
			this->pOnOffListBox3->SelectItem(this->pCommand->y);
		break;

		case CCharacterCommand::CC_SetMovementType:
			this->pMovementTypeListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_SetPlayerStealth:
			this->pStealthListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_SetWaterTraversal:
			this->pWaterTraversalListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_GenerateEntity:
			this->pDirectionListBox2->SelectItem(this->pCommand->w);
			this->pAddCommandGraphicListBox->SelectItem(this->pCommand->h);
		break;

		case CCharacterCommand::CC_GameEffect:
			this->pDirectionListBox3->SelectItem(this->pCommand->w);
			this->pVisualEffectsListBox->SelectItem(this->pCommand->h);
			this->pOnOffListBox3->SelectItem(this->pCommand->flags);
		break;

		case CCharacterCommand::CC_StartGlobalScript:
			this->pGlobalScriptListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_FlushSpeech:
		case CCharacterCommand::CC_PlayerEquipsWeapon:
			this->pOnOffListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_Label:
		{
			CTextBoxWidget *pGotoLabel = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_GOTOLABELTEXT));
			pGotoLabel->SetText(this->pCommand->label.c_str());
		}
		break;
		case CCharacterCommand::CC_FlashingText:
		{
			CTextBoxWidget *pLabelText = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_TEXT2));
			if (!this->pCommand->h) {
				pLabelText->SetText(wszEmpty);
			} else {
				WSTRING colorText = SetColorText(this->pCommand->x, this->pCommand->y, this->pCommand->w);
				pLabelText->SetText(colorText.c_str());
			}
		}
		//no break
		case CCharacterCommand::CC_Question:
		case CCharacterCommand::CC_RoomLocationText:
		{
			CTextBoxWidget *pQuestionText = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_GOTOLABELTEXT));
			CDbSpeech *pSpeech = this->pCommand->pSpeech;
			ASSERT(pSpeech);
			if (pSpeech)	//robustness
				pQuestionText->SetText((const WCHAR*)pSpeech->MessageText);
		}
		break;
		case CCharacterCommand::CC_AnswerOption:
		{
			CTextBoxWidget *pAnswerText = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_GOTOLABELTEXT));
			CDbSpeech *pSpeech = this->pCommand->pSpeech;
			ASSERT(pSpeech);
			if (pSpeech)	//robustness
				pAnswerText->SetText((const WCHAR*)pSpeech->MessageText);
			this->pGotoLabelListBox->SelectItem(this->pCommand->x);
		}
		break;
		case CCharacterCommand::CC_GoSub:
		case CCharacterCommand::CC_GoTo:
			this->pGotoLabelListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_WaitForRect:
		case CCharacterCommand::CC_WaitForNotRect:
		case CCharacterCommand::CC_WaitForWeapon:
			SetBitFlags();
		break;
		case CCharacterCommand::CC_WaitForEntityType:
		case CCharacterCommand::CC_WaitForNotEntityType:
			this->pAddCommandGraphicListBox->SelectItem(this->pCommand->flags);
		break;

		case CCharacterCommand::CC_WaitForRemains:
			this->pRemainsListBox->SelectItem(this->pCommand->flags);
		break;

		case CCharacterCommand::CC_VarSet:
		case CCharacterCommand::CC_WaitForVar:
		{
			this->pVarListBox->SelectItem(this->pCommand->x);
			if (c == CCharacterCommand::CC_VarSet)
				this->pVarOpListBox->SelectItem(this->pCommand->y);
			else
				this->pVarCompListBox->SelectItem(this->pCommand->y);

			CTextBoxWidget *pVarOperand = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_VARVALUE));
			ASSERT(pVarOperand);
			CTextBoxWidget *pVarText = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_VARNAMETEXTINPUT));
			ASSERT(pVarText);

			const bool bTextVar =
				(c == CCharacterCommand::CC_VarSet &&
					(this->pCommand->y == ScriptVars::AssignText ||
					this->pCommand->y == ScriptVars::AppendText)) ||
				(c == CCharacterCommand::CC_WaitForVar &&
					this->pCommand->y == ScriptVars::EqualsText);
			if (bTextVar)
			{
				pVarOperand->SetText(wszEmpty);
				pVarText->SetText(this->pCommand->label.c_str());
			} else {
				if (!this->pCommand->label.empty())
					pVarOperand->SetText(this->pCommand->label.c_str());
				else
					pVarOperand->SetText(_itoW(this->pCommand->w, temp, 10));
				pVarText->SetText(wszEmpty);
			}
		}
		break;
		case CCharacterCommand::CC_ChallengeCompleted:
		{
			CTextBoxWidget *pTextWidget = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_GOTOLABELTEXT));
			ASSERT(pTextWidget);
			pTextWidget->SetText(this->pCommand->label.c_str());
		}
		break;

		case CCharacterCommand::CC_SetPlayerAppearance:
			this->pPlayerGraphicListBox->SelectItem(this->pCommand->x);
		break;
		case CCharacterCommand::CC_SetNPCAppearance:
			this->pAddCommandGraphicListBox->SelectItem(this->pCommand->x);
			this->pOnOffListBox4->SelectItem(this->pCommand->y);
		break;

		case CCharacterCommand::CC_SetPlayerWeapon:
			this->pWeaponListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_GetNaturalTarget:
			this->pNaturalTargetTypesListBox->SelectItem(this->pCommand->x);
		break;

		case CCharacterCommand::CC_CutScene:
		{
			CTextBoxWidget *pDelay = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_WAIT));
			ASSERT(pDelay);
			pDelay->SetText(_itoW(this->pCommand->x, temp, 10));
		}
		break;

		case CCharacterCommand::CC_AmbientSound:
		case CCharacterCommand::CC_AmbientSoundAt:
			this->pOnOffListBox->SelectItem(this->pCommand->h);
		break;

		case CCharacterCommand::CC_PlayVideo:
		{
			CTextBoxWidget *pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_X_COORD));
			ASSERT(pRel);
			pRel->SetText(_itoW((int)this->pCommand->x, temp, 10));
			pRel = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_Y_COORD));
			ASSERT(pRel);
			pRel->SetText(_itoW((int)this->pCommand->y, temp, 10));
		}
		break;

		case CCharacterCommand::CC_ImageOverlay:
		{
			CTextBox2DWidget *pText = DYN_CAST(CTextBox2DWidget*, CWidget*,
					this->pAddCommandDialog->GetWidget(TAG_IMAGEOVERLAYTEXT));
			pText->SetText(this->pCommand->label.c_str());
		}
		break;

		case CCharacterCommand::CC_Appear:
		case CCharacterCommand::CC_AppearAt:
		case CCharacterCommand::CC_Disappear:
		case CCharacterCommand::CC_TeleportTo:
		case CCharacterCommand::CC_TeleportPlayerTo:
		case CCharacterCommand::CC_EndScript:
		case CCharacterCommand::CC_WaitForCleanRoom:
		case CCharacterCommand::CC_WaitForCleanLevel:
		case CCharacterCommand::CC_WaitForPlayerToTouchMe:
		case CCharacterCommand::CC_WaitForSomeoneToPushMe:
		case CCharacterCommand::CC_ActivateItemAt:
		case CCharacterCommand::CC_DestroyTrapdoor:
		case CCharacterCommand::CC_TurnIntoMonster:
		case CCharacterCommand::CC_ReplaceWithDefault:
		case CCharacterCommand::CC_EndScriptOnExit:
		case CCharacterCommand::CC_If:
		case CCharacterCommand::CC_IfElse:
		case CCharacterCommand::CC_IfElseIf:
		case CCharacterCommand::CC_IfEnd:
		case CCharacterCommand::CC_WaitForNoBuilding:
		case CCharacterCommand::CC_Return:
		case CCharacterCommand::CC_GetEntityDirection:
			break;

		//Deprecated commands.
		case CCharacterCommand::CC_GotoIf:
		case CCharacterCommand::CC_WaitForHalph:
		case CCharacterCommand::CC_WaitForNotHalph:
		case CCharacterCommand::CC_WaitForMonster:
		case CCharacterCommand::CC_WaitForNotMonster:
		case CCharacterCommand::CC_WaitForCharacter:
		case CCharacterCommand::CC_WaitForNotCharacter:
		default: ASSERT(!"Invalid character command (2)"); break;
	}
	this->pAddCommandDialog->RequestPaint();
}

//*****************************************************************************
const CCharacterCommand* CCharacterDialogWidget::GetCommandWithLabel(
//Returns: pointer to command with specified label, or NULL if none
//
//Params:
	const COMMANDPTR_VECTOR& commands,
	const UINT label)
const
{
	if (!label) return NULL;

	for (UINT wIndex=commands.size(); wIndex--; )
	{
		const CCharacterCommand *pCommand = commands[wIndex];
		if (pCommand->command == CCharacterCommand::CC_Label &&
				label == pCommand->x)
			return pCommand;
	}
	return NULL;
}

//*****************************************************************************
const CCharacterCommand* CCharacterDialogWidget::GetCommandWithLabelText(
//Returns: pointer to command with specified label text, or NULL if none
//
//Params:
	const COMMANDPTR_VECTOR& commands,
	const WCHAR* pText)
const
{
	ASSERT(pText);
	for (UINT wIndex=commands.size(); wIndex--; )
	{
		const CCharacterCommand *pCommand = commands[wIndex];
		if (pCommand->command == CCharacterCommand::CC_Label &&
				!WCSicmp(pText, pCommand->label.c_str()))
			return pCommand;
	}
	return NULL;
}

//*****************************************************************************
void CCharacterDialogWidget::SetBitFlags()
//Set list items by bit-field.
{
	CCharacterCommand::CharCommand c = this->pCommand->command;
	this->pWaitFlagsListBox->DeselectAll();
	this->pWeaponFlagsListBox->DeselectAll();

	UINT wBitfield = 1;
	for (UINT wBits = 0; wBits<32; ++wBits, wBitfield *= 2)
	{
		if ((this->pCommand->flags & wBitfield) == wBitfield) {
			switch (c) 
			{
				case CCharacterCommand::CC_WaitForEntityType:
				case CCharacterCommand::CC_WaitForNotEntityType:
				{
					this->pWaitFlagsListBox->SelectItem(wBitfield, true);
				}
				break;
				case CCharacterCommand::CC_WaitForWeapon: 
				{
					this->pWeaponFlagsListBox->SelectItem(wBitfield, true);
				}
				break;
				default: 
				{
					ASSERT(!"Bad CCharacter command"); break;
				}
			}
		}
	}
}

//*****************************************************************************
void CCharacterDialogWidget::TestSound()
//Plays the loaded sound effect, if any.
{
	if (this->pSound)
		g_pTheSound->PlayVoice(this->pSound->data);
}

//*****************************************************************************
void CCharacterDialogWidget::UpdateCharacter()
//Transfers all script information to character assigned for editing
//and clears local state.
{
	//CDbDatum *pSound = NULL;
	ASSERT(this->pCharacter);
	this->pCharacter->commands.clear();
	COMMANDPTR_VECTOR::iterator command;
	for (command = this->commands.begin(); command != this->commands.end(); ++command)
	{
		this->pCharacter->commands.push_back(**command);
		delete *command;
	}
	this->commands.clear();
	ClearPasteBuffer();
	this->pCharacter = NULL;
	this->pCommand = NULL;
}

//
//Script Parsing/Assembling functions
//

//*****************************************************************************
UINT CCharacterDialogWidget::findStartTextMatch(
//Returns: key for text found in list box matching start of pText+index
	CListBoxWidget* pListBoxWidget, const WCHAR* pText, UINT& index,
	bool& bFound) //(out) whether a valid value was found
const
{
	ASSERT(pListBoxWidget);

	UINT maxMatchedStringLength=0, matchIndex=NOT_FOUND;
	for (UINT line=pListBoxWidget->GetItemCount(); line--; )
	{
		//Find longest match to make sure correct command is found.
		const WCHAR *pwCommandText = pListBoxWidget->GetTextAtLine(line);
		const UINT len = WCSlen(pwCommandText);
		if (len <= maxMatchedStringLength)
			continue;
		if (!WCSncmp(pText+index, pwCommandText, len))
		{
			maxMatchedStringLength = len;
			matchIndex = line;
		}
	}

	//Match found?
	if (matchIndex < pListBoxWidget->GetItemCount())
	{
		bFound = true;
		index += maxMatchedStringLength; //advance past this text
		return pListBoxWidget->GetKeyAtLine(matchIndex);
	}
	bFound = false;
	return NOT_FOUND;
}

//*****************************************************************************
UINT CCharacterDialogWidget::findTextMatch(
//Returns: key for text found in list box exactly matching from pText+index
	CListBoxWidget* pListBoxWidget, const WCHAR* pText, const UINT index,
	bool& bFound) //(out) whether a valid value was found
const
{
	ASSERT(pListBoxWidget);

	UINT matchIndex=NOT_FOUND;
	for (UINT line=pListBoxWidget->GetItemCount(); line--; )
	{
		//Find longest match to make sure correct command is found.
		const WCHAR *pwCommandText = pListBoxWidget->GetTextAtLine(line);
		if (!WCScmp(pText+index, pwCommandText))
			matchIndex = line;
	}

	//Match found?
	if (matchIndex < pListBoxWidget->GetItemCount())
	{
		bFound = true;
		return pListBoxWidget->GetKeyAtLine(matchIndex);
	}
	bFound = false;
	return NOT_FOUND;
}

//*****************************************************************************
bool getTextToLastQuote(const WCHAR* pText, UINT& pos, WSTRING& foundText)
//Get all text up to last quotation mark.
//Returns: whether ending quotation mark was found
{
	//Find last quotation character.
	const UINT textLength = WCSlen(pText);
	ASSERT(textLength > 0);
	UINT lastPos = textLength - 1;
	while (lastPos > pos && pText[lastPos] != W_t('"'))
		--lastPos;
	if (lastPos <= pos)
		return false;

	foundText.assign(pText + pos, lastPos - pos); //output text
	pos = lastPos + 1; //update cursor index to position after quotation char
	return true;
}

//*****************************************************************************
bool getTextUpTo(const WCHAR* pText, UINT& pos, WCHAR c)
{
	while (pText[pos] && pText[pos] != c)
		++pos;
	return pText[pos] == c;
}

//*****************************************************************************
CCharacterCommand* CCharacterDialogWidget::fromText(
//Parses a line of text into a command.
//
//Returns: pointer to a new character command if text parsed correctly, else NULL
//
//Params:
	WSTRING text)  //Text to parse
{
#define skipWhitespace while (pos < textLength && iswspace(pText[pos])) ++pos

#define skipComma skipWhitespace; if (pText[pos] == W_t(',')) {++pos; skipWhitespace;}
#define skipLeftParen skipWhitespace; if (pText[pos] == W_t('(')) {++pos; skipWhitespace;}
#define skipRightParen skipWhitespace; if (pText[pos] == W_t(')')) {++pos; skipWhitespace;}

#define parseChar(c) skipWhitespace; if (pText[pos++] != W_t(c)) {delete pCommand; return NULL;}

#define parseNumber(n) {UINT oldPos = pos; \
	if (pos < textLength && pText[pos] == W_t('-')) ++pos; \
	while (pos < textLength && iswdigit(pText[pos])) ++pos; \
	if (pos > oldPos)	n = _Wtoi(pText+oldPos); \
	else {delete pCommand; return NULL;}}

#define parseOptionalNumber(n) {UINT oldPos = pos; \
	if (pos < textLength && pText[pos] == W_t('-')) ++pos; \
	while (pos < textLength && iswdigit(pText[pos])) ++pos; \
	n = pos > oldPos ? _Wtoi(pText+oldPos) : 0;}

#define parseOption(x,pListBox,bFound) x = findStartTextMatch(pListBox, pText, pos, bFound)

#define parseMandatoryOption(x,pListBox,bFound) parseOption(x,pListBox,bFound); \
	if (!bFound) {delete pCommand; return NULL;}

	stripTrailingWhitespace(text);

	WCHAR *pText = (WCHAR*)text.c_str();

	//Ignore comment lines.
	if (pText[0] == W_t('#'))
		return NULL;

	const UINT textLength = text.length();
	UINT pos=0;
	bool bFound;

	//Strip pretty printing.
	while (pos < textLength && iswpunct(WCv(pText[pos]))) //skip leading error chars
		++pos;
	skipWhitespace;
	if (pos >= textLength)
		return NULL; //no relevant text on this line

	//Parse command.
	UINT eCommand = findStartTextMatch(this->pActionListBox, pText, pos, bFound);
	if (eCommand >= CCharacterCommand::CC_Count) {
		//deprecated alternate script command names, for backwards compatibility with text scripts
		if (!WCScmp(pText, g_pTheDB->GetMessageText(MID_SetPlayerSword))) {
			eCommand = CCharacterCommand::CC_PlayerEquipsWeapon;
		} else {
			return NULL; //text doesn't match commands
		}
	}

	CCharacterCommand *pCommand = new CCharacterCommand();
	pCommand->command = CCharacterCommand::CharCommand(eCommand);

	skipWhitespace;

	switch (eCommand)
	{
	case CCharacterCommand::CC_Appear:                                   //No arguments
	case CCharacterCommand::CC_Disappear:
	case CCharacterCommand::CC_EndScript:
	case CCharacterCommand::CC_EndScriptOnExit:
	case CCharacterCommand::CC_If:
	case CCharacterCommand::CC_IfElse:
	case CCharacterCommand::CC_IfElseIf:
	case CCharacterCommand::CC_IfEnd:
	case CCharacterCommand::CC_TurnIntoMonster:
	case CCharacterCommand::CC_ReplaceWithDefault:
	case CCharacterCommand::CC_WaitForCleanRoom:
	case CCharacterCommand::CC_WaitForCleanLevel:
	case CCharacterCommand::CC_WaitForPlayerToTouchMe:
	case CCharacterCommand::CC_WaitForSomeoneToPushMe:
	case CCharacterCommand::CC_Return:
	break;

	case CCharacterCommand::CC_CutScene:
	case CCharacterCommand::CC_Imperative:
	case CCharacterCommand::CC_Wait:
	case CCharacterCommand::CC_WaitForTurn:
		skipLeftParen;
		parseNumber(pCommand->x);
	break;

	case CCharacterCommand::CC_ActivateItemAt:
	case CCharacterCommand::CC_AppearAt:
	case CCharacterCommand::CC_TeleportTo:
	case CCharacterCommand::CC_TeleportPlayerTo:
	case CCharacterCommand::CC_LevelEntrance:
	case CCharacterCommand::CC_GetEntityDirection:
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
	break;

	case CCharacterCommand::CC_Behavior:
		skipLeftParen;
		parseNumber(pCommand->x);
		skipComma;
		parseMandatoryOption(pCommand->y, this->pDirectionListBox3, bFound);
	break;

	case CCharacterCommand::CC_SetMovementType:
		parseMandatoryOption(pCommand->x, this->pMovementTypeListBox, bFound);
	break;

	case CCharacterCommand::CC_AttackTile:
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		parseMandatoryOption(pCommand->flags,this->pAttackTileListBox,bFound);
	break;

	case CCharacterCommand::CC_PushTile:
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		parseMandatoryOption(pCommand->w, this->pDirectionListBox2, bFound);
	break;

	case CCharacterCommand::CC_WaitForDoorTo:
		parseMandatoryOption(pCommand->w,this->pOpenCloseListBox,bFound);
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
	break;

	case CCharacterCommand::CC_MoveTo:
	{
		UINT flag;
		do {
			parseOption(flag,this->pWaitFlagsListBox,bFound);
			if (bFound)
				pCommand->flags |= flag;
			skipWhitespace;
		} while (bFound);
	}
	//no break
	case CCharacterCommand::CC_MoveRel:
		if (!pCommand->flags) //MoveTo
		{
			skipLeftParen;
			parseNumber(pCommand->x); skipComma;
			parseNumber(pCommand->y);
			skipRightParen;
			skipComma;
		}
		skipLeftParen;
		parseOptionalNumber(pCommand->w); skipComma;
		parseOptionalNumber(pCommand->h);
	break;
	
	case CCharacterCommand::CC_FaceTowards:
		UINT flag;
		do {
			parseOption(flag,this->pWaitFlagsListBox,bFound);
			if (bFound)
				pCommand->flags |= flag;
			skipWhitespace;
		} while (bFound);

		if (!pCommand->flags) //MoveTo
		{
			skipLeftParen;
			parseNumber(pCommand->x); skipComma;
			parseNumber(pCommand->y);
			skipRightParen;
			skipComma;
		}
		skipLeftParen;
		parseOptionalNumber(pCommand->w);
	break;
	//no break
	case CCharacterCommand::CC_WaitForRect:
	case CCharacterCommand::CC_WaitForNotRect:
	{
		UINT flag;
		do {
			parseOption(flag,this->pWaitFlagsListBox,bFound);
			if (bFound)
				pCommand->flags |= flag;
			skipWhitespace;
		} while (bFound);
	}
	//no break
	case CCharacterCommand::CC_WaitForNoBuilding:
	case CCharacterCommand::CC_DestroyTrapdoor:
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->w); pCommand->w -= pCommand->x; skipComma;
		parseNumber(pCommand->h); pCommand->h -= pCommand->y;
	break;

	case CCharacterCommand::CC_WaitForEntityType:
	case CCharacterCommand::CC_WaitForNotEntityType:
		parseMandatoryOption(pCommand->flags, this->pAddCommandGraphicListBox, bFound);
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->w); pCommand->w -= pCommand->x; skipComma;
		parseNumber(pCommand->h); pCommand->h -= pCommand->y;
	break;

	case CCharacterCommand::CC_WaitForRemains:
		parseMandatoryOption(pCommand->flags, this->pRemainsListBox, bFound);
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->w); pCommand->w -= pCommand->x; skipComma;
		parseNumber(pCommand->h); pCommand->h -= pCommand->y;
	break;

	case CCharacterCommand::CC_WaitForWeapon:
	{
		UINT flag;
		do {
			parseOption(flag, this->pWeaponFlagsListBox, bFound);
			if (bFound)
				pCommand->flags |= flag;
			skipWhitespace;
		} while (bFound);
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
	}
	break;

	case CCharacterCommand::CC_GetNaturalTarget:
		parseMandatoryOption(pCommand->x, this->pNaturalTargetTypesListBox, bFound);
		break;

	case CCharacterCommand::CC_AmbientSoundAt:
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		//no break
	case CCharacterCommand::CC_AmbientSound:
		skipLeftParen;
		parseNumber(pCommand->w); skipComma;
		parseNumber(pCommand->h);
	break;

	case CCharacterCommand::CC_ImageOverlay:
		parseNumber(pCommand->w);
		skipComma;
		pCommand->label = pText+pos;
	break;
	case CCharacterCommand::CC_PlayVideo:
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		parseNumber(pCommand->w);
	break;

	case CCharacterCommand::CC_BuildMarker:
		parseMandatoryOption(pCommand->flags, this->pBuildMarkerListBox, bFound);
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->w); pCommand->w -= pCommand->x; skipComma;
		parseNumber(pCommand->h); pCommand->h -= pCommand->y;
		break;

	case CCharacterCommand::CC_Build:
		parseMandatoryOption(pCommand->flags, this->pBuildItemsListBox, bFound);
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->w); pCommand->w -= pCommand->x; skipComma;
		parseNumber(pCommand->h); pCommand->h -= pCommand->y;
		break;

	case CCharacterCommand::CC_WaitForItem:
		parseMandatoryOption(pCommand->flags,this->pWaitForItemsListBox,bFound);
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->w); pCommand->w -= pCommand->x; skipComma;
		parseNumber(pCommand->h); pCommand->h -= pCommand->y;
	break;

	case CCharacterCommand::CC_Label:
	{
		pCommand->label = pText+pos;
		//Generate unique label info.
		GenerateUniqueLabelName(pCommand->label);
		pCommand->x = ++this->wIncrementedLabel;
		AddLabel(pCommand);
	}
	break;

	case CCharacterCommand::CC_GoSub:
	case CCharacterCommand::CC_GoTo:
		pCommand->label = pText+pos;
		//Caller must look up label ID.
	break;

	case CCharacterCommand::CC_AnswerOption:
	{
		//Answer is all text between outermost quotes.
		parseChar('"');
		WSTRING question;
		const bool bRes = getTextToLastQuote(pText, pos, question);
		if (!bRes)
		{
			delete pCommand;
			return NULL;
		}

		ASSERT(!pCommand->pSpeech);
		pCommand->pSpeech = g_pTheDB->Speech.GetNew();
		pCommand->pSpeech->MessageText = question.c_str();

		skipComma;
		pCommand->label = pText+pos;
		//Caller must look up label ID.
	}
	break;

	case CCharacterCommand::CC_FlashingText:
		skipWhitespace;
		//Optional text color in parentheses.
		if (pText[pos] != W_t('(')) {
			pCommand->h = 0;
		} else {
			++pos; skipWhitespace;

			const UINT start=pos;
			if (!getTextUpTo(pText, pos, W_t(')'))) {
				delete pCommand;
				return NULL;
			}
			const UINT end=pos;

			WSTRING color = pText+start;
			color.resize(end-start);
			if (!TranslateColorText(color, pCommand)) {
				delete pCommand;
				return NULL;
			}
			pos=end;
			skipRightParen;
			skipComma;
		}
	//no break
	case CCharacterCommand::CC_Question:
	case CCharacterCommand::CC_RoomLocationText:
	{
		ASSERT(!pCommand->pSpeech);
		pCommand->pSpeech = g_pTheDB->Speech.GetNew();
		pCommand->pSpeech->MessageText = pText+pos;
	}
	break;

	case CCharacterCommand::CC_FlushSpeech:
	case CCharacterCommand::CC_PlayerEquipsWeapon:
		parseMandatoryOption(pCommand->x,this->pOnOffListBox,bFound);
	break;

	case CCharacterCommand::CC_SetPlayerStealth:
		parseMandatoryOption(pCommand->x,this->pStealthListBox,bFound);
	break;
	case CCharacterCommand::CC_SetWaterTraversal:
		parseMandatoryOption(pCommand->x,this->pWaterTraversalListBox,bFound);
	break;

	case CCharacterCommand::CC_GenerateEntity:
		parseMandatoryOption(pCommand->h, this->pAddCommandGraphicListBox, bFound);
		skipComma;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y); skipComma;
		parseMandatoryOption(pCommand->w,this->pDirectionListBox2,bFound);
	break;

	case CCharacterCommand::CC_GameEffect:
		parseMandatoryOption(pCommand->w,this->pDirectionListBox3,bFound);
		skipComma;
		parseMandatoryOption(pCommand->h, this->pVisualEffectsListBox,bFound);
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		parseMandatoryOption(pCommand->flags,this->pOnOffListBox3,bFound);
	break;

	case CCharacterCommand::CC_StartGlobalScript:
		parseMandatoryOption(pCommand->x,this->pGlobalScriptListBox,bFound);
	break;

	case CCharacterCommand::CC_WaitForCueEvent:
		parseMandatoryOption(pCommand->x,this->pEventListBox,bFound);
	break;

	case CCharacterCommand::CC_FaceDirection:
	case CCharacterCommand::CC_WaitForPlayerToFace:
	case CCharacterCommand::CC_WaitForPlayerToMove:
	case CCharacterCommand::CC_WaitForOpenMove:
		parseMandatoryOption(pCommand->x,this->pDirectionListBox,bFound);
	break;

	case CCharacterCommand::CC_WaitForPlayerInput:
		parseMandatoryOption(pCommand->x,this->pInputListBox,bFound);
	break;

	case CCharacterCommand::CC_DisplayFilter:
		parseMandatoryOption(pCommand->x,this->pDisplayFilterListBox,bFound);
	break;

	case CCharacterCommand::CC_SetPlayerAppearance:
		parseMandatoryOption(pCommand->x, this->pPlayerGraphicListBox, bFound);
	break;
	case CCharacterCommand::CC_SetNPCAppearance:
		parseMandatoryOption(pCommand->x, this->pAddCommandGraphicListBox, bFound);
		parseNumber(pCommand->y);
	break;

	case CCharacterCommand::CC_SetPlayerWeapon:
		parseMandatoryOption(pCommand->x,this->pWeaponListBox,bFound);
	break;

	case CCharacterCommand::CC_WorldMapSelect:
		parseNumber(pCommand->x);
	break;
	case CCharacterCommand::CC_WorldMapMusic:
	case CCharacterCommand::CC_SetMusic:
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y); skipComma;
		pCommand->label += pText + pos;
		if (!pCommand->label.empty())
			pCommand->x = 0; //ensure custom music when specified
	break;

	case CCharacterCommand::CC_WorldMapIcon:
		parseMandatoryOption(pCommand->h, this->pAddCommandGraphicListBox, bFound);
		skipComma;
		parseMandatoryOption(pCommand->flags,this->pWorldMapIconFlagListBox,bFound);
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		parseNumber(pCommand->w); skipComma;
	break;
	case CCharacterCommand::CC_WorldMapImage:
		parseNumber(pCommand->h); skipComma;
		parseMandatoryOption(pCommand->flags,this->pWorldMapImageFlagListBox,bFound);
		skipComma;
		skipLeftParen;
		parseNumber(pCommand->x); skipComma;
		parseNumber(pCommand->y);
		skipRightParen;
		skipComma;
		parseNumber(pCommand->w); skipComma;
	break;

	case CCharacterCommand::CC_Speech:
	{
		//Speech is all text between outermost quotes.
		parseChar('"');
		WSTRING speech;
		const bool bRes = getTextToLastQuote(pText, pos, speech);
		if (!bRes)
		{
			delete pCommand;
			return NULL;
		}

		ASSERT(!pCommand->pSpeech);
		pCommand->pSpeech = g_pTheDB->Speech.GetNew();
		pCommand->pSpeech->MessageText = speech.c_str();
		skipComma;

		parseMandatoryOption(pCommand->pSpeech->wMood,this->pMoodListBox,bFound);
		skipComma;

		parseMandatoryOption(pCommand->pSpeech->wCharacter,this->pSpeakerListBox,bFound);
		skipComma;

		if (pCommand->pSpeech->wCharacter == Speaker_Custom)
		{
			skipLeftParen;
			parseNumber(pCommand->x); skipComma;
			parseNumber(pCommand->y); skipComma;
		}

		parseNumber(pCommand->pSpeech->dwDelay); skipComma;
		//Ignore name of sound data file, if any.
	}
	break;

	case CCharacterCommand::CC_VarSet:
	{
		//Var name is all text between outermost quotes.
		parseChar('"');
		WSTRING varName;
		const bool bRes = getTextToLastQuote(pText, pos, varName);
		if (!bRes)
		{
			delete pCommand;
			return NULL;
		}

		UINT tempIndex = 0;
		pCommand->x = findTextMatch(this->pVarListBox, varName.c_str(), tempIndex, bFound);
		if (!bFound)
		{
			pCommand->x = AddVar(varName.c_str());
			if (!pCommand->x)
			{
				delete pCommand;
				return NULL;
			}
		}

		skipWhitespace;
		if (pos >= textLength)
		{
			delete pCommand;
			return NULL;
		}
		const char varOperator = char(WCv(pText[pos]));
		++pos;
		switch (varOperator)
		{
			default: //robust default for bad operator char
			case '=': pCommand->y = ScriptVars::Assign; break;
			case '+': pCommand->y = ScriptVars::Inc; break;
			case '-': pCommand->y = ScriptVars::Dec; break;
			case '*': pCommand->y = ScriptVars::MultiplyBy; break;
			case '/': pCommand->y = ScriptVars::DivideBy; break;
			case '%': pCommand->y = ScriptVars::Mod; break;
			case ':': pCommand->y = ScriptVars::AssignText; break;
			case ';': pCommand->y = ScriptVars::AppendText; break;
		}

		skipWhitespace;
		switch (pCommand->y)
		{
			case ScriptVars::AppendText:
			case ScriptVars::AssignText:
				pCommand->label = pText + pos;
			break;
			default:
			{
				if (isWInteger(pText + pos))
					pCommand->w = _Wtoi(pText + pos); //get number
				else
					pCommand->label = pText + pos; //get text expression
			}
			break;
		}
	}
	break;

	case CCharacterCommand::CC_WaitForVar:
	{
		//Var name is all text between outermost quotes.
		//(Copied from case VarSet above.)
		parseChar('"');
		WSTRING varName;
		const bool bRes = getTextToLastQuote(pText, pos, varName);
		if (!bRes)
		{
			delete pCommand;
			return NULL;
		}

		UINT tempIndex = 0;
		pCommand->x = findTextMatch(this->pVarListBox, varName.c_str(), tempIndex, bFound);
		if (!bFound)
		{
			pCommand->x = AddVar(varName.c_str());
			if (!pCommand->x)
			{
				delete pCommand;
				return NULL;
			}
		}

		skipWhitespace;
		if (pos >= textLength)
		{
			delete pCommand;
			return NULL;
		}
		const char varOperator = char(WCv(pText[pos]));
		++pos;
		const char varOperator2 = pos < textLength ? char(WCv(pText[pos])) : 0;
		switch (varOperator)
		{
			default: //robust default for bad operator char
			case '=': pCommand->y = ScriptVars::Equals; break;
			case '>':
				if (varOperator2 == '=') {
					pCommand->y = ScriptVars::GreaterThanOrEqual;
					++pos;
				} else {
					pCommand->y = ScriptVars::Greater;
				}
				break;
			case '<':
				if (varOperator2 == '=') {
					pCommand->y = ScriptVars::LessThanOrEqual;
					++pos;
				} else {
					pCommand->y = ScriptVars::Less;
				}
				break;
			case '!':
				if (varOperator2 == '=') {
					pCommand->y = ScriptVars::Inequal;
					++pos;
				}
				break;
			case ':': pCommand->y = ScriptVars::EqualsText; break;
		}

		skipWhitespace;
		switch (pCommand->y)
		{
			case ScriptVars::EqualsText:
				pCommand->label = pText + pos;
			break;
			default:
			{
				if (isWInteger(pText + pos))
					pCommand->w = _Wtoi(pText + pos); //get number
				else
					pCommand->label = pText + pos; //get text expression
			}
			break;
		}
	}
	break;

	case CCharacterCommand::CC_ChallengeCompleted:
		pCommand->label = pText+pos;
	break;

	default: ASSERT(!"Unrecognized script command"); break;
	}

	return pCommand;
#undef skipComma
#undef skipWhitespace
#undef skipLeftParen
#undef skipRightParen
#undef parseChar
#undef parseNumber
#undef parseOptionalNumber
#undef parseMandatoryOption
#undef parseOption
}

//*****************************************************************************
WSTRING RemoveNewlines(WSTRING wstr)
{
	for (size_t i = 0; i < wstr.length(); ++i) {
		if (wstr[i] == '\n' || wstr[i] == '\r')
			wstr[i] = W_t(' ');
	}
	return wstr;
}

//*****************************************************************************
WSTRING CCharacterDialogWidget::toText(
//Converts a character command into a simple text format which can be parsed by ::fromText.
//
//Returns: a non-empty string if the operation succeeded, otherwise an empty string
//
//Params:
	const COMMANDPTR_VECTOR& commands,
	CCharacterCommand* pCommand,       //Command to parse
	const CListBoxWidget* pCommandList, //Command list to get string from
	const UINT wCommandIndex)          //Index of the command
{
#define concatNum(n) wstr += _itoW(n,temp,10)
#define concatNumWithComma(n) concatNum(n); wstr += wszComma;
	WCHAR temp[1024];

	//Get command name.
	ASSERT(pCommand);
	CCharacterCommand& c = *pCommand;

	WSTRING wstr, wstrCommandName = this->pActionListBox->GetTextForKey(c.command);
	if (wstrCommandName.empty())
		return wstr;

	UINT indent = ExtractCommandIndent(pCommandList, wCommandIndex);
	wstr.insert(wstr.end(), indent - INDENT_PREFIX_SIZE + 2, W_t(' '));
	wstr += wstrCommandName;
	wstr += wszSpace;

	switch (c.command)
	{
	case CCharacterCommand::CC_Appear:
	case CCharacterCommand::CC_Disappear:
	case CCharacterCommand::CC_EndScript:
	case CCharacterCommand::CC_EndScriptOnExit:
	case CCharacterCommand::CC_If:
	case CCharacterCommand::CC_IfElse:
	case CCharacterCommand::CC_IfElseIf:
	case CCharacterCommand::CC_IfEnd:
	case CCharacterCommand::CC_TurnIntoMonster:
	case CCharacterCommand::CC_ReplaceWithDefault:
	case CCharacterCommand::CC_WaitForCleanRoom:
	case CCharacterCommand::CC_WaitForCleanLevel:
	case CCharacterCommand::CC_WaitForPlayerToTouchMe:
	case CCharacterCommand::CC_WaitForSomeoneToPushMe:
	case CCharacterCommand::CC_Return:
	break;

	case CCharacterCommand::CC_CutScene:
	case CCharacterCommand::CC_Imperative:
	case CCharacterCommand::CC_Wait:
	case CCharacterCommand::CC_WaitForTurn:
		concatNum(c.x);
	break;

	case CCharacterCommand::CC_Behavior:
		concatNumWithComma(c.x);
		wstr += this->pDirectionListBox3->GetTextForKey(c.y);
	break;

	case CCharacterCommand::CC_SetMovementType:
		wstr += this->pMovementTypeListBox->GetTextForKey(c.x);
	break;

	case CCharacterCommand::CC_ActivateItemAt:
	case CCharacterCommand::CC_AppearAt:
	case CCharacterCommand::CC_TeleportTo:
	case CCharacterCommand::CC_TeleportPlayerTo:
	case CCharacterCommand::CC_LevelEntrance:
	case CCharacterCommand::CC_GetEntityDirection:
		concatNumWithComma(c.x);
		concatNum(c.y);
	break;

	case CCharacterCommand::CC_AttackTile:
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
		wstr += this->pAttackTileListBox->GetTextForKey(c.flags);
	break;

	case CCharacterCommand::CC_PushTile:
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
		wstr += this->pDirectionListBox2->GetTextForKey(c.w);
	break;

	case CCharacterCommand::CC_WaitForDoorTo:
		wstr += this->pOpenCloseListBox->GetTextForKey(c.w);
		wstr += wszComma;
		concatNumWithComma(c.x);
		concatNum(c.y);
	break;

	case CCharacterCommand::CC_MoveTo:
	{
		UINT wBitfield = 1;
		for (UINT wBits = 0; wBits<32; ++wBits, wBitfield *= 2)
		{
			if ((c.flags & wBitfield) == wBitfield)
			{
				wstr += this->pWaitFlagsListBox->GetTextForKey(wBitfield);
				wstr += wszSpace;
			}
		}
	}
	//no break
	case CCharacterCommand::CC_MoveRel:
		if (!c.flags) //MoveTo
		{
			concatNumWithComma(c.x);
			concatNumWithComma(c.y);
		}
		concatNumWithComma(c.w);
		concatNum(c.h);
	break;
	
	case CCharacterCommand::CC_FaceTowards:
	{
		UINT wBitfield = 1;
		for (UINT wBits = 0; wBits < 32; ++wBits, wBitfield *= 2)
		{
			if ((c.flags & wBitfield) == wBitfield)
			{
				wstr += this->pWaitFlagsListBox->GetTextForKey(wBitfield);
				wstr += wszSpace;
			}
		}

		if (!c.flags) //MoveTo
		{
			concatNumWithComma(c.x);
			concatNumWithComma(c.y);
		}
		concatNum(c.w);
	}
	break;

	case CCharacterCommand::CC_WaitForRect:
	case CCharacterCommand::CC_WaitForNotRect:
	{
		UINT wBitfield = 1;
		for (UINT wBits = 0; wBits<32; ++wBits, wBitfield *= 2)
		{
			if ((c.flags & wBitfield) == wBitfield)
			{
				wstr += this->pWaitFlagsListBox->GetTextForKey(wBitfield);
				wstr += wszSpace;
			}
		}
	}
	//no break
	case CCharacterCommand::CC_WaitForNoBuilding:
	case CCharacterCommand::CC_DestroyTrapdoor:
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
		concatNumWithComma(c.x + c.w);
		concatNum(c.y + c.h);
	break;

	case CCharacterCommand::CC_WaitForEntityType:
	case CCharacterCommand::CC_WaitForNotEntityType:
	{
		WSTRING charName = this->pAddCommandGraphicListBox->GetTextForKey(c.flags);
		wstr += charName.length() ? charName : wszQuestionMark;
		wstr += wszComma;
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
		concatNumWithComma(c.x + c.w);
		concatNum(c.y + c.h);
	}
	break;

	case CCharacterCommand::CC_WaitForRemains:
	{
		WSTRING charName = this->pRemainsListBox->GetTextForKey(c.flags);
		wstr += charName.length() ? charName : wszQuestionMark;
		wstr += wszComma;
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
		concatNumWithComma(c.x + c.w);
		concatNum(c.y + c.h);
	}
	break;

	case CCharacterCommand::CC_WaitForWeapon:
	{
		UINT wBitfield = 1;
		for (UINT wBits = 0; wBits<32; ++wBits, wBitfield *= 2)
		{
			if ((c.flags & wBitfield) == wBitfield)
			{
				wstr += this->pWeaponFlagsListBox->GetTextForKey(wBitfield);
				wstr += wszSpace;
			}
		}
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
	}
	break;

	case CCharacterCommand::CC_AmbientSoundAt:
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
		//no break
	case CCharacterCommand::CC_AmbientSound:
		concatNumWithComma(c.w);
		concatNum(c.h);
	break;

	case CCharacterCommand::CC_ImageOverlay:
		concatNumWithComma(c.w);
		wstr += RemoveNewlines(c.label);
	break;
	case CCharacterCommand::CC_PlayVideo:
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
		concatNum(c.w);
	break;

	case CCharacterCommand::CC_BuildMarker:
		wstr += this->pBuildMarkerListBox->GetTextForKey(c.flags);
		wstr += wszComma;
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
		concatNumWithComma(c.x + c.w);
		concatNum(c.y + c.h);
		break;

	case CCharacterCommand::CC_Build:
		wstr += this->pBuildItemsListBox->GetTextForKey(c.flags);
		wstr += wszComma;
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
		concatNumWithComma(c.x + c.w);
		concatNum(c.y + c.h);
		break;

	case CCharacterCommand::CC_WaitForItem:
		wstr += this->pWaitForItemsListBox->GetTextForKey(c.flags);
		wstr += wszComma;
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
		concatNumWithComma(c.x + c.w);
		concatNum(c.y + c.h);
	break;

	case CCharacterCommand::CC_Label:
	case CCharacterCommand::CC_ChallengeCompleted:
		wstr += c.label;
	break;

	case CCharacterCommand::CC_GoSub:
	case CCharacterCommand::CC_GoTo:
	{
		AppendGotoDestination(wstr, commands, c);
	}
	break;

	case CCharacterCommand::CC_AnswerOption:
	{
		CDbSpeech *pSpeech = c.pSpeech;
		wstr += wszQuote;
		wstr += pSpeech ? (const WCHAR*)(pSpeech->MessageText) : wszQuestionMark;
		wstr += wszQuote;
		wstr += wszComma;
		AppendGotoDestination(wstr, commands, c);
	}
	break;

	case CCharacterCommand::CC_FlashingText:
		if (c.h) {
			wstr += wszLeftParen;
			wstr += SetColorText(c.x, c.y, c.w);
			wstr += wszRightParen;
			wstr += wszComma;
		}
	//no break
	case CCharacterCommand::CC_Question:
	case CCharacterCommand::CC_RoomLocationText:
	{
		CDbSpeech *pSpeech = c.pSpeech;
		wstr += pSpeech ? (const WCHAR*)pSpeech->MessageText : wszQuestionMark;
	}
	break;

	case CCharacterCommand::CC_FlushSpeech:
	case CCharacterCommand::CC_PlayerEquipsWeapon:
		wstr += this->pOnOffListBox->GetTextForKey(c.x);
	break;

	case CCharacterCommand::CC_SetPlayerStealth:
		wstr += this->pStealthListBox->GetTextForKey(c.x);
	break;
	case CCharacterCommand::CC_SetWaterTraversal:
		wstr += this->pWaterTraversalListBox->GetTextForKey(c.x);
	break;

	case CCharacterCommand::CC_GenerateEntity:
	{
		WSTRING charName = this->pAddCommandGraphicListBox->GetTextForKey(c.h);
		wstr += charName.length() ? charName : wszQuestionMark;
		wstr += wszComma;
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
		wstr += this->pDirectionListBox2->GetTextForKey(c.w);
	}
	break;

	case CCharacterCommand::CC_GameEffect:
		wstr += this->pDirectionListBox3->GetTextForKey(c.w);
		wstr += wszComma;
		wstr += this->pVisualEffectsListBox->GetTextForKey(c.h);
		wstr += wszComma;
		concatNumWithComma(c.x);
		concatNumWithComma(c.y);
		wstr += this->pOnOffListBox3->GetTextForKey(c.flags);
	break;

	case CCharacterCommand::CC_StartGlobalScript:
	{
		const WSTRING charName = this->pGlobalScriptListBox->GetTextForKey(c.x);
		wstr += charName.length() ? charName : wszQuestionMark;
	}
	break;

	case CCharacterCommand::CC_WaitForCueEvent:
		wstr += this->pEventListBox->GetTextForKey(c.x);
	break;

	case CCharacterCommand::CC_FaceDirection:
	case CCharacterCommand::CC_WaitForPlayerToFace:
	case CCharacterCommand::CC_WaitForPlayerToMove:
	case CCharacterCommand::CC_WaitForOpenMove:
		wstr += this->pDirectionListBox->GetTextForKey(c.x);
	break;

	case CCharacterCommand::CC_WaitForPlayerInput:
		wstr += this->pInputListBox->GetTextForKey(c.x);
	break;

	case CCharacterCommand::CC_DisplayFilter:
		wstr += this->pDisplayFilterListBox->GetTextForKey(c.x);
	break;
	
	case CCharacterCommand::CC_SetPlayerAppearance:
	{
		const WSTRING charName = this->pPlayerGraphicListBox->GetTextForKey(c.x);
		wstr += charName.length() ? charName : wszQuestionMark;
	}
	break;

	case CCharacterCommand::CC_GetNaturalTarget:
	{
		const WSTRING charName = this->pNaturalTargetTypesListBox->GetTextForKey(c.x);
		wstr += charName.length() ? charName : wszQuestionMark;
	}
	break;
	case CCharacterCommand::CC_SetNPCAppearance:
	{
		const WSTRING charName = this->pAddCommandGraphicListBox->GetTextForKey(c.x);
		wstr += charName.length() ? charName : wszQuestionMark;
		wstr += wszSpace;
		concatNum(c.y);
	}
	break;

	case CCharacterCommand::CC_SetPlayerWeapon:
	{
		const WSTRING charName = this->pWeaponListBox->GetTextForKey(c.x);
		wstr += charName.length() ? charName : wszQuestionMark;
	}
	break;

	case CCharacterCommand::CC_WorldMapSelect:
		concatNum(c.x);
	break;
	case CCharacterCommand::CC_WorldMapMusic:
	case CCharacterCommand::CC_SetMusic:
		if (c.label.size())
		{
			concatNumWithComma(0);
			concatNumWithComma(c.y);
			wstr += c.label;
		} else {
			concatNumWithComma(c.x);
			concatNum(c.y);
		}
	break;

	case CCharacterCommand::CC_WorldMapIcon:
	{
		const WSTRING charName = this->pAddCommandGraphicListBox->GetTextForKey(c.h);
		wstr += charName.length() ? charName : wszQuestionMark;
		wstr += wszComma;

		const WSTRING flag = this->pWorldMapIconFlagListBox->GetTextForKey(c.flags);
		wstr += flag.length() ? flag : wszQuestionMark;
		wstr += wszComma;

		concatNumWithComma(c.x);
		concatNumWithComma(c.y);

		concatNum(c.w);
	}
	break;
	case CCharacterCommand::CC_WorldMapImage:
	{
		concatNumWithComma(c.h);

		const WSTRING flag = this->pWorldMapImageFlagListBox->GetTextForKey(c.flags);
		wstr += flag.length() ? flag : wszQuestionMark;
		wstr += wszComma;

		concatNumWithComma(c.x);
		concatNumWithComma(c.y);

		concatNum(c.w);
	}
	break;

	case CCharacterCommand::CC_Speech:
	{
		CDbSpeech *pSpeech = c.pSpeech;
		ASSERT(pSpeech);
		if (!pSpeech)	//robustness
		{
			wstr += wszQuestionMark;
			break;
		}

		wstr += wszQuote;
		wstr += (const WCHAR*)(pSpeech->MessageText);
		wstr += wszQuote;
		wstr += wszComma;

		wstr += this->pMoodListBox->GetTextForKey(pSpeech->wMood);
		wstr += wszComma;

		WSTRING charName = this->pSpeakerListBox->GetTextForKey(pSpeech->wCharacter);
		wstr += charName.length() ? charName : wszQuestionMark;
		wstr += wszComma;
		if (pSpeech->wCharacter == Speaker_Custom)
		{
			concatNumWithComma(c.x);
			concatNumWithComma(c.y);
		}

		concatNumWithComma(pSpeech->dwDelay);

		if (pSpeech->dwDataID)
		{
			//Load sound clip name only from DB.
			wstr += GetDataName(pSpeech->dwDataID);
		}
		else if (pSpeech->GetSound() &&
				!((CDbDatum*)pSpeech->GetSound())->DataNameText.empty())
		{
			//Sound exists in object, but not yet in DB.  Just display its name.
			wstr += pSpeech->GetSound()->DataNameText;
		} else {
			//No attached sound.
			wstr += wszPeriod;
		}
	}
	break;

	case CCharacterCommand::CC_VarSet:
	{
		const WCHAR *wszVarName = this->pVarListBox->GetTextForKey(c.x);
		wstr += wszQuote;
		wstr += WCSlen(wszVarName) ? wszVarName : wszQuestionMark;
		wstr += wszQuote;
		wstr += wszSpace;
		switch (c.y)
		{
			case ScriptVars::Assign: wstr += wszEqual; break;
			case ScriptVars::Inc: wstr += wszPlus; break;
			case ScriptVars::Dec: wstr += wszHyphen; break;
			case ScriptVars::MultiplyBy: wstr += wszAsterisk; break;
			case ScriptVars::DivideBy: wstr += wszForwardSlash; break;
			case ScriptVars::Mod: wstr += wszPercent; break;
			case ScriptVars::AssignText: wstr += wszColon; break;
			case ScriptVars::AppendText: wstr += wszSemicolon; break;
			default: wstr += wszQuestionMark; break;
		}
		wstr += wszSpace;
		switch (c.y)
		{
			case ScriptVars::AppendText:
			case ScriptVars::AssignText:
				wstr += c.label;
			break;
			default:
				if (!c.label.empty())
					wstr += c.label;
				else
					concatNum(c.w);
			break;
		}
	}
	break;

	case CCharacterCommand::CC_WaitForVar:
	{
		const WCHAR *wszVarName = this->pVarListBox->GetTextForKey(c.x);
		wstr += wszQuote;
		wstr += WCSlen(wszVarName) ? wszVarName : wszQuestionMark;
		wstr += wszQuote;
		wstr += wszSpace;
		if (c.y == ScriptVars::EqualsText) {
			wstr += wszColon;
		} else {
			AddOperatorSymbol(wstr, c.y);
		}
		wstr += wszSpace;
		switch (c.y)
		{
			case ScriptVars::EqualsText:
				wstr += c.label;
			break;
			default:
				if (!c.label.empty())
					wstr += c.label;
				else
					concatNum(c.w);
			break;
		}
	}
	break;

	default: ASSERT(!"Bad command"); break;
	}

	wstr += wszCRLF;
	return wstr;

#undef concatNum
#undef concatNumWithComma
}

// $Id: EntranceSelectDialogWidget.cpp 8929 2008-04-24 03:23:38Z mrimer $

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

#include "EntranceSelectDialogWidget.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"
#include "DrodScreen.h"
#include "DrodSound.h"
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/OptionButtonWidget.h>
#include <BackEndLib/Clipboard.h>
#include <BackEndLib/Ports.h>

#include "../DRODLib/Character.h"
#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/Swordsman.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbLevels.h"
#include "../DRODLib/DbHolds.h"
#include "../Texts/MIDs.h"

//NOTE: tag #'s should not conflict with other widgets on screen
const UINT TAG_ENTRANCES_LISTBOX = 898;
const UINT TAG_DELETE = 897;
const UINT TAG_DONE = 896;

const UINT CX_DIALOG = 700;
const UINT CY_DIALOG = 610;

//*****************************************************************************
CEntranceSelectDialogWidget::CEntranceSelectDialogWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget 
	const int nSetX, const int nSetY)         //    constructor.
	: CDialogWidget(dwSetTagNo, nSetX, nSetY, CX_DIALOG, CY_DIALOG, true)   //double-click on list box disables
	, pListBoxWidget(NULL)
	, pSourceHold(NULL), pCurrentGame(NULL)
	, pPromptLabel(NULL)
{
	static const UINT CX_SPACE = 15;
	static const UINT CY_SPACE = 10;

	static const int Y_LABEL = CY_SPACE;
	static const UINT CY_LABEL = 60;

	static const UINT CX_BUTTON = 70;
	static const int X_OKBUTTON = (CX_DIALOG - 2*CX_BUTTON) / 2;
	static const int Y_OKBUTTON = CY_DIALOG - CY_STANDARD_BUTTON - CY_SPACE;
	static const int X_DELETEBUTTON = X_OKBUTTON + CX_BUTTON + 2*CX_SPACE;
	static const int Y_DELETEBUTTON = Y_OKBUTTON;

	static const int X_LISTBOX = CX_SPACE;
	static const int Y_LISTBOX = Y_LABEL + CY_LABEL;
	static const UINT CX_LISTBOX = CX_DIALOG - X_LISTBOX - CX_SPACE;
	static const UINT CY_LISTBOX = Y_OKBUTTON - Y_LISTBOX - CY_SPACE;

	this->pPromptLabel = new CLabelWidget(0L, 0, Y_LABEL, this->w, CY_LABEL,
			F_Small, wszEmpty);
	this->pPromptLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	AddWidget(this->pPromptLabel);

	//OK button gets default focus
	CButtonWidget *pButton = new CButtonWidget(
			TAG_OK, X_OKBUTTON, Y_OKBUTTON, CX_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Okay));
	AddWidget(pButton);

	pButton = new CButtonWidget(
			TAG_DONE, X_OKBUTTON, Y_OKBUTTON, CX_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Done));
	AddWidget(pButton);
	pButton->Hide();

	pButton = new CButtonWidget(
			TAG_DELETE, X_DELETEBUTTON, Y_DELETEBUTTON, CX_BUTTON, CY_STANDARD_BUTTON,
			g_pTheDB->GetMessageText(MID_Delete));
	pButton->Hide();
	AddWidget(pButton);

	this->pListBoxWidget = new CListBoxWidget(TAG_ENTRANCES_LISTBOX,
			X_LISTBOX, Y_LISTBOX, CX_LISTBOX, CY_LISTBOX);
	AddWidget(this->pListBoxWidget);
}

//*****************************************************************************
void CEntranceSelectDialogWidget::AddEntranceToList(CEntranceData *pEntrance)
{
	const WSTRING descText = pEntrance->GetPositionDescription();
	this->pListBoxWidget->AddItem(pEntrance->dwEntranceID, descText.c_str());
}

//*****************************************************************************
void CEntranceSelectDialogWidget::OnClick(
//Handles click event.  Overrides CDialogWidget::OnClick().
//
//Params:
	const UINT dwTagNo)          //(in)   Widget that received event.
{
	if (!dwTagNo) return;

	CWidget *pWidget = GetWidget(dwTagNo);
	if (pWidget->GetType() == WT_Button)   //only buttons will return from dialog
	{
		//Return a special enumerated value describing what semantic button type
		//was pressed.
		switch (dwTagNo)
		{
			case TAG_OK: this->dwDeactivateValue = OK; break;
			case TAG_DELETE: this->dwDeactivateValue = Delete; break;
			case TAG_DONE:
			default: this->dwDeactivateValue = Other; break;
		}
		Deactivate();
	}
}

//*****************************************************************************
void CEntranceSelectDialogWidget::OnDoubleClick(
//Handles double click event.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	CDialogWidget::OnDoubleClick(dwTagNo);

	//transcribe TAG_OK to OK button enumeration
	if (this->dwDeactivateValue == TAG_OK)
		this->dwDeactivateValue = OK;
}

//*****************************************************************************
void CEntranceSelectDialogWidget::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Key event.
{
	CDialogWidget::OnKeyDown(dwTagNo, Key);

	switch (Key.keysym.sym)
	{
		//Ctrl-c: copy text content of list to clipboard
		case SDLK_c:
			if (Key.keysym.mod & KMOD_CTRL)
			{
				WSTRING wstr;
				for (UINT line=0; line<this->pListBoxWidget->GetItemCount(); ++line)
				{
					wstr += this->pListBoxWidget->GetTextAtLine(line);
					wstr += wszCRLF;
				}
				CClipboard::SetString(wstr);
				g_pTheSound->PlaySoundEffect(SEID_MIMIC);
			}
		break;

		default: break;
	}
}

//*****************************************************************************
void CEntranceSelectDialogWidget::OnSelectChange(
//Handles selection change event.
//Pass event on to parent, if exists.
//
//Params:
	const UINT dwTagNo)
{
	//Disable delete button when non-deletable selection is chosen.
	CButtonWidget *pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_DELETE));
	if (pButton->IsVisible())
	{
		pButton->Enable(GetSelectedItem() != 0L);
		pButton->RequestPaint();
	}

	CDialogWidget::OnSelectChange(dwTagNo);
}

//*****************************************************************************
void CEntranceSelectDialogWidget::SortEntrances(
//Sort according to level order.  Main entrance in each level shown first.
//
//Params:
	CDbHold *pHold, ENTRANCE_VECTOR &entrances)
{
	if (!pHold) return;
	ASSERT(!entrances.size());

	//Get levels.  Sort by local index in hold.
	SORTED_LEVELS levels;
	CIDSet levelsInHold = CDb::getLevelsInHold(pHold->dwHoldID);
	for (CIDSet::const_iterator levelID = levelsInHold.begin(); levelID != levelsInHold.end(); ++levelID)
	{
		CDbLevel *pLevel = g_pTheDB->Levels.GetByID(*levelID);
		ASSERT(pLevel);
		levels.insert(pLevel);
	}

	CEntranceData *pEntrance;
	for (SORTED_LEVELS::const_iterator level = levels.begin(); level != levels.end(); ++level)
	{
		CEntranceData *pMainEntrance = pHold->GetMainEntranceForLevel((*level)->dwLevelID);
		entrances.push_back(pMainEntrance);
		for (UINT wIndex=0; wIndex<pHold->Entrances.size(); ++wIndex)
		{
			//Get the level entrances in this level.
			pEntrance = pHold->Entrances[wIndex];
			ASSERT(pEntrance->dwEntranceID);
			ASSERT(pEntrance->dwRoomID);
			if (!pEntrance->dwRoomID) continue; //bad data
			if (pEntrance == pMainEntrance) continue; //main entrance was already added
			if ((*level)->dwLevelID != g_pTheDB->Rooms.GetLevelIDForRoom(pEntrance->dwRoomID))
				continue;   //ignore entrances for other levels

			entrances.push_back(pEntrance);
		}
		delete *level;
	}
}

//*****************************************************************************
void CEntranceSelectDialogWidget::PrepareToPopulateList(const DATATYPE datatype) //[default=Entrances]
//Set up widgets in preparation to populate the list and display the dialog.
{
	this->pListBoxWidget->Clear();

	//Which buttons to show for each view option.
	bool bShowButtons[3] = {true, false, false}; //OK, DONE, DELETE
	switch (datatype)
	{
		default:
		case Entrances:
			if (!this->pSourceHold)
				return;
		break;
		case Images: case Sounds: case Videos:
			if (!this->pSourceHold)
				return;
			bShowButtons[2] = true;
		break;
		case Speech:
			if (!this->pCurrentGame)
				return;
			bShowButtons[0] = false;
			bShowButtons[1] = true;
		break;
		case ChatHistory:
		case GlobalVars:
			bShowButtons[0] = false;
			bShowButtons[1] = true;
		break;
	}
	CButtonWidget *pButton;
	pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_OK));
	pButton->Show(bShowButtons[0]);
	pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_DONE));
	pButton->Show(bShowButtons[1]);
	pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_DELETE));
	pButton->Show(bShowButtons[2]);
}

//*****************************************************************************
void CEntranceSelectDialogWidget::PopulateList(const DATATYPE datatype) //[default=Entrances]
//Populate list box with all of a certain data type in hold.
{
	PrepareToPopulateList(datatype);
	CButtonWidget *pButton;

	BEGIN_DBREFCOUNT_CHECK;
	switch (datatype)
	{
		case Entrances:
		{
			//Default choice (end hold).
			this->pListBoxWidget->SortAlphabetically(false);
			this->pListBoxWidget->AddItem(EXIT_ENDHOLD, g_pTheDB->GetMessageText(MID_DefaultExit));
			this->pListBoxWidget->AddItem(EXIT_PRIOR_LOCATION, g_pTheDB->GetMessageText(MID_ReturnToPriorLocation));

			ENTRANCE_VECTOR entrances;
			SortEntrances(this->pSourceHold, entrances);
			for (UINT wIndex=0; wIndex<entrances.size(); ++wIndex)
				AddEntranceToList(entrances[wIndex]);
		}
		break;

		case Images:
		{
			//Default choice (import image).
			this->pListBoxWidget->SortAlphabetically(false);
			this->pListBoxWidget->AddItem(0L, g_pTheDB->GetMessageText(MID_ImportImage));

			this->pListBoxWidget->SortAlphabetically(true);
			CDb db;
			CIDSet imageFormats = CBitmapManager::GetSupportedImageFormats();
			db.Data.FilterByFormat(imageFormats);
			db.Data.FilterByHold(this->pSourceHold->dwHoldID);
			CIDSet dataIDs = db.Data.GetIDs();
			dataIDs -= this->pSourceHold->GetDeletedDataIDs(); //these are pending deletion and no longer available
			for (CIDSet::const_iterator iter = dataIDs.begin(); iter != dataIDs.end(); ++iter)
			{
				const WSTRING name = db.Data.GetNameFor(*iter);
				this->pListBoxWidget->AddItem(*iter, name.c_str());
			}

			pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_DELETE));
			pButton->Disable();
		}
		break;

		case Sounds:
		{
			//Default choice (import sound).
			this->pListBoxWidget->SortAlphabetically(false);
			this->pListBoxWidget->AddItem(0L, g_pTheDB->GetMessageText(MID_ImportSoundOption));

			this->pListBoxWidget->SortAlphabetically(true);
			CDb db;
			CIDSet soundFormats(DATA_OGG); //all supported sound formats
			soundFormats += DATA_WAV;
			db.Data.FilterByFormat(soundFormats);
			db.Data.FilterByHold(this->pSourceHold->dwHoldID);
			CIDSet dataIDs = db.Data.GetIDs();
			dataIDs -= this->pSourceHold->GetDeletedDataIDs(); //these are pending deletion and no longer available
			for (CIDSet::const_iterator iter = dataIDs.begin(); iter != dataIDs.end(); ++iter)
			{
				const WSTRING name = db.Data.GetNameFor(*iter);
				this->pListBoxWidget->AddItem(*iter, name.c_str());
			}

			pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_DELETE));
			pButton->Disable();
		}
		break;

		case Videos:
		{
			//Default choice (import video).
			this->pListBoxWidget->SortAlphabetically(false);
			this->pListBoxWidget->AddItem(0L, g_pTheDB->GetMessageText(MID_ImportVideoOption));

			this->pListBoxWidget->SortAlphabetically(true);
			CIDSet videoFormats(DATA_THEORA); //all supported video formats
			CDb db;
			db.Data.FilterByFormat(videoFormats);
			db.Data.FilterByHold(this->pSourceHold->dwHoldID);
			CIDSet dataIDs = db.Data.GetIDs();
			dataIDs -= this->pSourceHold->GetDeletedDataIDs(); //these are pending deletion and no longer available
			for (CIDSet::const_iterator iter = dataIDs.begin(); iter != dataIDs.end(); ++iter)
			{
				const WSTRING name = db.Data.GetNameFor(*iter);
				this->pListBoxWidget->AddItem(*iter, name.c_str());
			}

			pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_DELETE));
			pButton->Disable();
		}
		break;

		case Speech:
		{
			//Speech history.
			this->pListBoxWidget->SortAlphabetically(false);
			ASSERT(this->pCurrentGame);
			vector<CCharacterCommand*>& speech = this->pCurrentGame->roomSpeech;
			for (UINT wIndex=0; wIndex<speech.size(); ++wIndex)
			{
				CDbSpeech *pSpeech = speech[wIndex]->pSpeech;
				ASSERT(pSpeech);

				//Get speaker name.
				UINT characterType = pSpeech->wCharacter;
				HoldCharacter *pCustomChar = NULL;
				if (characterType >= CUSTOM_CHARACTER_FIRST && characterType != M_NONE)
				{
					pCustomChar = this->pCurrentGame->pHold->GetCharacter(characterType);
					if (pCustomChar)
						characterType = pCustomChar->wType;
				}
				string colorUnused;
				const UINT dwSpeakerTextID = getSpeakerNameText(characterType, colorUnused);
				WSTRING charText;
				if (pCustomChar)
					charText = pCustomChar->charNameText.c_str();
				else if (dwSpeakerTextID == MID_Custom)
				{
					//Monster at (x,y) is speaking.
					CMonster *pMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(
							speech[wIndex]->x, speech[wIndex]->y);
					if (pMonster)
					{
						UINT customIdentity = pMonster->GetIdentity();
						if (customIdentity >= CUSTOM_CHARACTER_FIRST && customIdentity != M_NONE)
						{
							pCustomChar = this->pCurrentGame->pHold->GetCharacter(customIdentity);
							if (pCustomChar)
								customIdentity = pCustomChar->wType;
						} else {
							customIdentity = getSpeakerType(MONSTERTYPE(customIdentity));
						}
						const UINT speakerTextID = getSpeakerNameText(customIdentity, colorUnused);
						if (speakerTextID != MID_None)
							charText = g_pTheDB->GetMessageText(speakerTextID);
						else
							pMonster = NULL; //custom identity isn't a real entity type -- just show its coords below
					}
					if (!pMonster)
					{
						//No valid entity here.  Just display (x,y) coordinates.
						WCHAR temp[16];
						charText += wszLeftParen;
						charText += _itow(speech[wIndex]->x, temp, 10);
						charText += wszComma;
						charText += _itow(speech[wIndex]->y, temp, 10);
						charText += wszRightParen;
					}
				}
				else if (dwSpeakerTextID != MID_None)
				{
					charText = g_pTheDB->GetMessageText(dwSpeakerTextID);
				}

				//Append dialogue.
				charText += wszColon;
				charText += wszSpace;
				charText += (const WCHAR*)pSpeech->MessageText;
				
				//Wrap speech that is too long.
				const UINT width = this->pListBoxWidget->GetW() - this->pListBoxWidget->GetVScrollBarWidth() - 5;
				UINT newChars = 0, index = 0;
				do {
					newChars = g_pTheFM->GetCharsThatFitWithin(FONTLIB::F_ListBoxItem,
							charText.c_str() + index, width);
					ASSERT(newChars > 0);
					WSTRING line = charText.c_str() + index;
					line.resize(newChars);
					index += newChars;
					this->pListBoxWidget->AddItem(1 + wIndex, line.c_str());
				} while (index < charText.size());
			}
		}
		break;

		case ChatHistory:
		{
			this->pListBoxWidget->SortAlphabetically(false);

			CDrodScreen::chat.PopulateListBoxFromHistory(this->pListBoxWidget);
			this->pListBoxWidget->MoveViewToBottom();
		}
		break;

		case GlobalVars:
		{
			ASSERT(this->pCurrentGame);
			PopulateListBoxFromGlobalVars(this->pCurrentGame->pPlayer->st);
		}
		break;

		default: break;
	}
	END_DBREFCOUNT_CHECK;
}

//*****************************************************************************
UINT CEntranceSelectDialogWidget::GetSelectedItem() const
{
	return this->pListBoxWidget->GetSelectedItem();
}

//*****************************************************************************
void CEntranceSelectDialogWidget::PopulateListBoxFromGlobalVars(const PlayerStats& st)
//Populate the list with the set of global game vars and display their current values.
{
	this->pListBoxWidget->SortAlphabetically(false);

	for (UINT i=0; i<ScriptVars::numGlobals; ++i)
	{
		WCHAR temp[16];
		WSTRING wstr = g_pTheDB->GetMessageText(ScriptVars::globalVarMIDs[i]);
		wstr += wszSpace;
		wstr += wszEqual;
		wstr += wszSpace;
		wstr += _itoW((int)(st.getVar(ScriptVars::globals[i])), temp, 10);
		this->pListBoxWidget->AddItem(ScriptVars::globals[i], wstr.c_str());
	}
}

//*****************************************************************************
void CEntranceSelectDialogWidget::PopulateListBoxFromHoldVars(CCurrentGame* pGame)
{
	CDbPackedVars& stats = pGame->stats;

	for (vector<HoldVar>::const_iterator var = pGame->pHold->vars.begin();
		var != pGame->pHold->vars.end(); ++var) {
		char varID[10], varName[11] = "v";
		//Get local hold var.
		_itoa(var->dwVarID, varID, 10);
		strcat(varName, varID);

		const WCHAR* varValue = stats.GetVar(varName, L"0");

		WSTRING wstr = var->varNameText.c_str();
		wstr += wszSpace;
		wstr += wszEqual;
		wstr += wszSpace;
		wstr += varValue;
		this->pListBoxWidget->AddItem(var->dwVarID, wstr.c_str());
	}
}

//*****************************************************************************
void CEntranceSelectDialogWidget::SelectItem(
	const UINT dwTagNo) //(in)
{
	this->pListBoxWidget->SelectItem(dwTagNo);

	//Disable delete button when non-deletable selection is chosen.
	CButtonWidget *pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_DELETE));
	pButton->Enable(dwTagNo != 0);
}

//*****************************************************************************
void CEntranceSelectDialogWidget::SetCurrentGame(CCurrentGame *pGame)
{
	this->pCurrentGame = pGame;
	this->pSourceHold = NULL;
}

//*****************************************************************************
void CEntranceSelectDialogWidget::SetPrompt(
	const MESSAGE_ID messageID)
{
	this->pPromptLabel->SetText(g_pTheDB->GetMessageText(messageID));
}

//*****************************************************************************
void CEntranceSelectDialogWidget::SetSourceHold(CDbHold *pHold)   //(in)
{
	this->pSourceHold = pHold;
	this->pCurrentGame = NULL;
}

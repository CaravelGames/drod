// $Id: CharacterDialogWidget.h 9659 2011-08-25 16:17:55Z mrimer $

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

//This is a CDialog that has been augmented to customize a character monster.

#ifndef CHARACTERDIALOGWIDGET_H
#define CHARACTERDIALOGWIDGET_H

#include "EntranceSelectDialogWidget.h"
#include "../DRODLib/Character.h"
#include "../DRODLib/DbData.h"
#include <FrontEndLib/DialogWidget.h>
#include <BackEndLib/MessageIDs.h>

class CRenameDialogWidget : public CDialogWidget
{
public:
	CRenameDialogWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
		const UINT wSetW, const UINT wSetH, const bool bListBoxDoubleClickReturns=false);

	virtual void   OnBetweenEvents();

protected:
	virtual void OnDoubleClick(const UINT dwTagNo);
	virtual void OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);
	virtual void OnRearranged(const UINT dwTagNo);
};

class CCharacter;
class CCharacterCommand;
class CListBoxWidget;
class COptionButtonWidget;
class CTextBoxWidget;
struct HoldCharacter;
class CCharacterDialogWidget : public CDialogWidget
{
friend class CRenameDialogWidget;
public:
	CCharacterDialogWidget(const UINT dwSetTagNo, const int nSetX=0,
			const int nSetY=0);
	~CCharacterDialogWidget();

	void EditCharacter(CCharacter* pCharacter);
	void EditClickedCommand();
	void EditDefaultScriptForCustomNPC();
	void FinishCommand(const UINT wX, const UINT wY, const UINT wW=(UINT)-1, const UINT wH=(UINT)-1);
	void FinishCommandAndExit();
	bool IsCommandFinished() const {return this->pCommand == NULL;}
	bool IsEditingDefaultScript() const {return this->bEditingDefaultScript;}

	virtual void   OnBetweenEvents();

	bool RenameCharacter();
	bool RenameVar();

	UINT queryX, queryY, queryW, queryH; //for querying the user for coord info

private:
	void  AddCharacterDialog();
	void  AddCommandDialog();
	void  AddCommand();
	void  AddCustomCharacter();
	void  AddLabel(CCharacterCommand* pCommand);
	void  AddScriptDialog();
	void  AddSound();
	void  AddVar();
	UINT  AddVar(const WCHAR* pVarName);
	void  ClearPasteBuffer();
	void  DeleteCommands(CListBoxWidget *pActiveCommandList, COMMANDPTR_VECTOR& commands);
	void  DeleteCustomCharacter();
	void  DeleteVar();
	void  EditCustomCharacters();
	void  FinishEditingDefaultScript();
	UINT  FilterUnsupportedCommands();
	UINT  findStartTextMatch(CListBoxWidget* pListBoxWidget, const WCHAR* pText, UINT& index, bool& bFound) const;
	UINT  findTextMatch(CListBoxWidget* pListBoxWidget, const WCHAR* pText, const UINT index, bool& bFound) const;
	void  GenerateUniqueLabelName(WSTRING& label) const;
	COMMANDPTR_VECTOR* GetActiveCommands();
	CListBoxWidget* GetActiveCommandListBox();
	WSTRING GetCommandDesc(const COMMANDPTR_VECTOR& commands,
			CCharacterCommand* pCommand) const;
	WSTRING GetCommandDesc_English(const COMMANDPTR_VECTOR& commands,
			const CCharacterCommand& command) const;
	const CCharacterCommand* GetCommandWithLabel(const COMMANDPTR_VECTOR& commands,
			const UINT label) const;
	const CCharacterCommand* GetCommandWithLabelText(const COMMANDPTR_VECTOR& commands,
			const WCHAR* pText) const;
	HoldCharacter* GetCustomCharacter();
	WSTRING GetDataName(const UINT dwID) const;
	WSTRING GetPrettyPrinting(const COMMANDPTR_VECTOR& commands,
			CCharacterCommand* pCommand,
			const UINT ifIndent, const UINT tabSize) const;
	void AppendGotoDestination(WSTRING& wstr, const COMMANDPTR_VECTOR& commands,
		const CCharacterCommand& pCommand) const;
   virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDoubleClick(const UINT dwTagNo);
	virtual void   OnKeyDown(const UINT dwTagNo, const SDL_KeyboardEvent &Key);
	virtual void   OnRearranged(const UINT dwTagNo);
	virtual void   OnSelectChange(const UINT dwTagNo);

	void  PopulateCharacterList(CListBoxWidget *pListBox);
	void  PopulateCommandDescriptions(CListBoxWidget *pCommandList, COMMANDPTR_VECTOR& commands);
	void  PopulateCommandListBox();
	void  PopulateEventListBox();
	void  PopulateGotoLabelList(const COMMANDPTR_VECTOR& commands);
	void  PopulateGraphicListBox(CListBoxWidget *pListBox);
	void  PopulateImperativeListBox(const bool bDefaultScript=false);
	void  PopulateItemListBox(CListBoxWidget* pListBox);
	void  PopulateMainGraphicList();
	void  PopulateSpeakerList(CListBoxWidget *pListBox);
	void  PopulateVarList();

	void  prepareForwardReferences(const COMMANDPTR_VECTOR& newCommands);

	void  RefreshCustomCharacterList(CListBoxWidget *pListBox);

	void  QueryMapRoom();
	void  QueryRect();
	void  QueryXY();

	void  resolveForwardReferences(const COMMANDPTR_VECTOR& newCommands);
	void  SelectCharacter();
	UINT  SelectMediaID(const UINT dwDefault, const CEntranceSelectDialogWidget::DATATYPE eType);
	void  SetBitFlags();
	void  SetWidgetStates();
	void  SetActionWidgetStates();
	void  SetAnimationSpeed();
	void  SetCharacterWidgetStates();
	void  SetCommandColor(CListBoxWidget* pListBox, int line, CCharacterCommand::CharCommand command);
	void  SetCommandParametersFromWidgets(CListBoxWidget *pActiveCommandList, COMMANDPTR_VECTOR& commands);
	void  SetCustomImage();
	void  SetCustomGraphic();
	void  SetCustomTiles();
	void  SetDefaultAvatar();
	void  SetDefaultScriptWidgetStates();
	void  SetDefaultTiles();
	void  SetWidgetsFromCommandParameters();
	void  TestSound();
	void  UpdateCharacter();
	void  UpdateSmartGotoList(bool bActivate);

	//For text editing of script commands.
	CCharacterCommand* fromText(WSTRING text);
	WSTRING toText(const COMMANDPTR_VECTOR& commands, CCharacterCommand* pCommand);

	CListBoxWidget *pGraphicListBox, *pPlayerGraphicListBox;
	COptionButtonWidget *pIsVisibleButton;
	CListBoxWidget *pCommandsListBox, *pDefaultScriptCommandsListBox;
	CRenameDialogWidget *pAddCommandDialog, *pAddCharacterDialog, *pScriptDialog;
	CListBoxWidget *pActionListBox;
	CListBoxWidget *pEventListBox;
	CListBoxWidget *pSpeakerListBox;
	CListBoxWidget *pMoodListBox;
	CListBoxWidget *pVisualEffectsListBox;
	CListBoxWidget *pDirectionListBox, *pDirectionListBox2, *pDirectionListBox3;
	CListBoxWidget *pOnOffListBox, *pOnOffListBox2, *pOnOffListBox3, *pOpenCloseListBox;
	CListBoxWidget *pGotoLabelListBox;
	CListBoxWidget *pMusicListBox;
	CListBoxWidget *pVarListBox, *pVarOpListBox, *pVarCompListBox, *pWaitFlagsListBox,
			*pImperativeListBox, *pBuildItemsListBox, *pBehaviorListBox, *pVarCompListBox2;
	CListBoxWidget *pEquipmentTypesListBox, *pCustomNPCListBox, *pEquipTransListBox;
	CTextBoxWidget *pCharNameText;
	CListBoxWidget *pCharListBox;
	CTextBoxWidget* pSpeechText;
	CListBoxWidget *pStatListBox;
	CListBoxWidget *pMovementTypeListBox;

	CCharacter *pCharacter;       //character being edited
	COMMANDPTR_VECTOR commands,  //copy of commands for character being edited
			commandBuffer; //cut/copied commands for pasting

	CCharacterCommand *pCommand;  //new command being added to script
	CDbDatum *pSound;             //loaded sound clip
	UINT wIncrementedLabel;       //next label ID to give
	bool bEditingCommand;         //true if command is being edited, not added
	bool bRetainFields;           //set when field values should be retained

	bool bEditingDefaultScript;   //set when a custom NPC's default script is being edited
	UINT defaultScriptCustomCharID; //ID of custom NPC whose default script is being edited

	static std::map<UINT, UINT> speechLengthCache;
};

#endif   //CHARACTERDIALOGWIDGET_H

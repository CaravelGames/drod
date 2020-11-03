// $Id: SellScreenTCB.cpp 10159 2012-05-02 06:23:33Z mrimer $

#include "SellScreenTCB.h"
#include "DrodFontManager.h"
#include "DrodEffect.h"
#include "DrodSound.h"

#include <FrontEndLib/BitmapManager.h>
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/MenuWidget.h>

#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/Character.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/SettingsKeys.h"

#include <BackEndLib/Browser.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Ports.h>

// Widget tags.
const UINT TAG_BUY = 2000;
const UINT TAG_BACK = 2001;
const UINT TAG_EXIT = 2002;
const UINT TAG_GOTOFORUM = 2003;

const UINT TAG_MENUPROMPT = 2011;
const UINT TAG_MENUFRAME = 2012;
const UINT TAG_MENU = 2013;

///////////////////////////
#define IMAGE_BG      (0)
#define IMAGE_SSHOT   (1)
#define IMAGE_POSTER  (2)
#define IMAGE_CABINET (3)

//Var names.
const char sellDemoPlayStyle[] = "TCBDemoPlayStyle";
const char sellLastConversation[] = "TCBLastSellConversation";
const char sellPlayerSkill[] = "TCBPlayerSkill";
const char sellCompletedDemoSession[] = "TCBCompletedDemo";
const char sell504Asked[] = "TCB504Asked";
const char sellGuide[] = "TCBGuide";
const char sellLane[] = "TCBLane";
const char sellUncturage[] = "TCBUncturage";
const char sellLibrary[] = "TCBLibrary";
const char sellStarted[] = "TCBStarted";

//Apparent player style.
enum PlayerStyle
{
	PS_NotSpecified = 0,
	PS_Veteran = 1,
	PS_NotForMe = 2,
	PS_Clever = 3,
	PS_Frustrated = 4
};

const UINT CX_SPACE = 10;
const UINT CY_SPACE = 10;
const UINT CX_EDGE_BUFFER = 290;

const UINT posterHeight = 202; //pixels

//******************************************************************************
CSellScreenTCB::CSellScreenTCB()
	: CSellScreen3(SCR_SellTCB)
	, pGame(NULL)
	, bShowingSubtitlesWithVoice(true)
	, dwNextSpeech(0)
	, pMenuDialog(NULL)
	, posterIndex(-1)
	, bPromptingUser(false)
{
	//Load this background screen.
	this->imageFilenames.push_back(string("StroutOfficeBG"));
	this->imageFilenames.push_back(string("StroutOfficeSShotsTCB"));
	this->imageFilenames.push_back(string("StroutOfficePosters"));
	this->imageFilenames.push_back(string("StroutOfficeCabinet"));

	static const int X_BUY_BUTTON = 748;
	static const int Y_BUY_BUTTON = 607;
	static const int X_BACK_BUTTON = X_BUY_BUTTON;
	static const int Y_BACK_BUTTON = 680;
	static const int X_EXIT_BUTTON = 884;
	static const int Y_EXIT_BUTTON = Y_BACK_BUTTON;
	static const UINT CX_BUY_BUTTON = 268;
	static const UINT CY_BUY_BUTTON = 70;
	static const UINT CX_BACK_BUTTON = 132;
	static const UINT CY_BACK_BUTTON = CY_BUY_BUTTON;
	static const UINT CX_EXIT_BUTTON = CX_BACK_BUTTON;
	static const UINT CY_EXIT_BUTTON = CY_BACK_BUTTON;

	static const UINT CX_MENUBOX = 700;
	static const UINT CY_MENUBOX = 400;
	static const int X_TEXT = CX_SPACE;
	static const int Y_TEXT = CY_SPACE;
	static const UINT CX_TEXT = CX_MENUBOX - (X_TEXT * 2);
	static const UINT CY_TEXT = CY_MENUBOX - (Y_TEXT * 2) - CY_SPACE;
	static const int X_MENU = 0;
	static const int Y_MENU = 0;
	static const int CX_MENU = CX_TEXT;
	static const int CY_MENU = CY_MENUBOX;

	CButtonWidget *pButton;

	//Whether this is the demo version.
	const bool bDemo = !IsGameFullVersion();

	//Buy button for non-registered versions, forum for registered.
	if (bDemo)	
	{
		pButton = new CButtonWidget(TAG_BUY,
				X_BUY_BUTTON, Y_BUY_BUTTON, CX_BUY_BUTTON, CY_BUY_BUTTON,
				g_pTheDB->GetMessageText(MID_BuyNow));
		AddWidget(pButton);
	} else {
		pButton = new CButtonWidget(TAG_GOTOFORUM,
				X_BUY_BUTTON, Y_BUY_BUTTON, CX_BUY_BUTTON, CY_BUY_BUTTON,
				g_pTheDB->GetMessageText(MID_GoToForum));
		AddWidget(pButton);
	}

	pButton = new CButtonWidget(TAG_BACK,
			X_BACK_BUTTON, Y_BACK_BUTTON, CX_BACK_BUTTON, CY_BACK_BUTTON,
			g_pTheDB->GetMessageText(MID_Back));
	AddWidget(pButton);

	pButton = new CButtonWidget(TAG_EXIT,
			X_EXIT_BUTTON, Y_EXIT_BUTTON, CX_EXIT_BUTTON, CY_EXIT_BUTTON,
			g_pTheDB->GetMessageText(MID_Exit));
	AddWidget(pButton);

	//Menu dialog.
	this->pMenuDialog = new CDialogWidget(0L, 0, 0, CX_MENUBOX, CY_MENUBOX);
	this->pMenuDialog->Hide();
	AddWidget(this->pMenuDialog);

	CLabelWidget *pLabel = new CLabelWidget(TAG_MENUPROMPT, X_TEXT, Y_TEXT,
			CX_TEXT, CY_TEXT, FONTLIB::F_Message, wszEmpty);
	this->pMenuDialog->AddWidget(pLabel);

	CFrameWidget *pFrame = new CFrameWidget(TAG_MENUFRAME, X_TEXT - 3, Y_TEXT - 3,
			CX_TEXT + 6, CY_TEXT + 6, NULL);
	pFrame->Disable();
	this->pMenuDialog->AddWidget(pFrame);

	CMenuWidget *pMenu = new CMenuWidget(TAG_MENU, X_MENU, Y_MENU, CX_MENU, CY_MENU,
			F_Hyperlink, F_ActiveHyperlink, F_ExtHyperlink);
	pMenu->SetFontYOffset(-5);
	this->pMenuDialog->AddWidget(pMenu);

	//Center prompts on left side of screen.
	SDL_Rect rect = MAKE_SDL_RECT(0, 0, CX_SCREEN - CX_EDGE_BUFFER, CY_SCREEN);
	this->pMessageDialog->SetCenteringRect(rect);
}

//*****************************************************************************
void CSellScreenTCB::OnBetweenEvents()
{
	MoveScreenshots(GetDestSurface(), true);

	if (!this->bPromptingUser)
	{
		//When ready, advance the script dialogue.
		const Uint32 dwNow = SDL_GetTicks();
		if (dwNow < this->dwNextSpeech)
			return;
		if (!this->pGame)
			return;

		if (!this->cueEvents.HasOccurred(CID_MonsterSpoke))
			ProcessCommand(this->cueEvents, CMD_WAIT);
		else
			ProcessEvents(this->cueEvents);

		//Draw effects onto screen.
		this->pEffects->UpdateAndDrawEffects(true);
	}
}

//*****************************************************************************
void CSellScreenTCB::OnDeactivate()
{
	ClearSpeech();

	SavePlayerSettings();

	this->cueEvents.Clear();
	delete this->pGame;
	this->pGame = NULL;
}

//******************************************************************************
void CSellScreenTCB::Paint(
//Paint the screen.
//
//Params:
	bool bUpdateRect) //(in)   If true (default) and destination
	                  //    surface is the screen, the screen
	                  //    will be immediately updated in
	                  //    the widget's rect.
{
	SDL_Surface *pDestSurface = GetDestSurface();

	//Blit the background graphic.
	ASSERT(this->images.size() > IMAGE_CABINET);
	SDL_BlitSurface(this->images[IMAGE_BG], NULL, pDestSurface, NULL);

	//Set picture on back wall.
	if (this->posterIndex == -1)
	{
		//Select which poster to show this session.
		this->posterIndex = rand() % (this->images[IMAGE_POSTER]->h / posterHeight);
	}
	SDL_Rect posterSrc = MAKE_SDL_RECT(0, this->posterIndex*posterHeight, 148, posterHeight);
	SDL_Rect posterDest = MAKE_SDL_RECT(328, 37, 148, posterHeight);
	SDL_BlitSurface(this->images[IMAGE_POSTER], &posterSrc, pDestSurface, &posterDest);

	MoveScreenshots(pDestSurface, false);

	PaintChildren();
	if (bUpdateRect) UpdateRect();
}

//*****************************************************************************
void CSellScreenTCB::PaintCabinetEdge(SDL_Surface* pDestSurface)
{
	ASSERT(this->images.size() > IMAGE_CABINET);
	SDL_Rect cabinetDest = MAKE_SDL_RECT(736,268,this->images[IMAGE_CABINET]->w,this->images[IMAGE_CABINET]->h);
	SDL_BlitSurface(this->images[IMAGE_CABINET], NULL, pDestSurface, &cabinetDest);
}

//*****************************************************************************
void CSellScreenTCB::ResetSellParameters(CDbPlayer* pPlayer)
//Resets all saved player preferences relating to TCB sell screen history.
{
	if (!pPlayer)
		return;

	CDbPackedVars& ps = pPlayer->Settings; //from

	ps.SetVar(sell504Asked, UINT(0));
	ps.SetVar(sellCompletedDemoSession, UINT(0));
	ps.SetVar(sellDemoPlayStyle, UINT(0));
	ps.SetVar(sellLastConversation, UINT(0));
	ps.SetVar(sellPlayerSkill, UINT(0));

	ps.SetVar(sellGuide, UINT(0));
	ps.SetVar(sellLane, UINT(0));
	ps.SetVar(sellLibrary, UINT(0));
	ps.SetVar(sellStarted, UINT(0));
	ps.SetVar(sellUncturage, UINT(0));
}

//*****************************************************************************
void CSellScreenTCB::AddSubtitle(CFiredCharacterCommand *pFiredCommand, const Uint32 dwDuration)
//Adds a line of subtitle text for the given speech command.
{
	CMoveCoord *pCoord = this->pGame->getSpeakingEntity(pFiredCommand);
	ASSERT(pCoord);

	//Get text.
	ASSERT(pFiredCommand);
	ASSERT(pFiredCommand->pCommand);
	CDbSpeech *pSpeech = pFiredCommand->pCommand->pSpeech;
	ASSERT(pSpeech);
	const WSTRING wStr = pFiredCommand->text;

	Paint(); //erase old effects when new one is drawn

	//Search for subtitle with this unique pCoord (i.e. object identity, not location).
	SUBTITLES::iterator found = this->subtitles.find(pCoord);
	if (found != this->subtitles.end())
	{
		//Add another line of text to existing subtitle effect.
		found->second->AddTextLine(wStr.c_str(), dwDuration);
	} else {
		//Speaker text effect.
		static const SURFACECOLOR color = {224, 186, 163}; //light orange
		CSubtitleEffect *pSubtitle = new CSubtitleEffect(this, pCoord,
				wStr.c_str(), Black, color, dwDuration, 3, F_Small, 710);
		this->pEffects->AddEffect(pSubtitle);
		pSubtitle->AddToSubtitles(this->subtitles);
	}
}

//*****************************************************************************
void CSellScreenTCB::ClearSpeech()
//Clear all active speech related data and events.
{
	//Clear subtitles.
	this->pEffects->Clear();

	//Stop currently playing voice.
	for (vector<ChannelInfo>::const_iterator channel=this->speechChannels.begin();
			channel!=this->speechChannels.end(); ++channel)
	{
		//Stop this channel from playing if it's a speech sound clip.
		if (g_pTheSound->IsSoundPlayingOnChannel(channel->wChannel) &&
				g_pTheSound->GetSoundIDPlayingOnChannel(channel->wChannel) ==
						static_cast<UINT>(SOUNDLIB::SEID_NONE))
			VERIFY(g_pTheSound->StopSoundOnChannel(channel->wChannel));
	}
	this->speechChannels.clear();
	this->dwNextSpeech = 0; //can start next speech immediately

	//Clear queued speech.
	deque<CFiredCharacterCommand*>::iterator iter;
	for (iter=this->speech.begin(); iter!=this->speech.end(); ++iter)
		delete *iter;
	this->speech.clear();
}

//******************************************************************************
void CSellScreenTCB::DisableWidgetsForPrompt(const bool bDisable) //false to enable buttons
//Disables widgets and script execution during an interactive user prompt.
{
	this->bPromptingUser = bDisable;

	static const UINT numWidgets = 4;
	static const UINT widgetTags[numWidgets] = {
		TAG_BUY, TAG_GOTOFORUM, TAG_BACK, TAG_EXIT
	};
	for (UINT i=numWidgets; i--; )
	{
		CWidget *pWidget = GetWidget(widgetTags[i]);
		if (pWidget)
		{
			pWidget->Enable(!bDisable);
			pWidget->Paint();
		}
	}

	this->pMenuDialog->SetBetweenEventsHandler(bDisable ? this : NULL); //keep updating screen display during user prompt
}

//*****************************************************************************
UINT CSellScreenTCB::GetMessageAnswer(const CMonsterMessage *pMsg)
//Display a dialog box with a question and a menu of answer options.
//
//Returns: value of answer selected by user
{
	//Set question.
	CMenuWidget *pMenu = DYN_CAST(CMenuWidget*, CWidget*, GetWidget(TAG_MENU));
	CLabelWidget *pPrompt = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(TAG_MENUPROMPT));
	const WCHAR *pTextStr = pMsg->message.c_str();
	pPrompt->SetText(pTextStr);

	//Set answer options.
	CMonster *pMonster = pMsg->pSender;
	CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
	ASSERT(!pCharacter->answerOptions.empty());
	UINT count=0; //Up to 9 options + undo are allowed.  Any more choices are ignored.
	WCHAR temp[4];
	for (CIDSet::const_iterator option = pCharacter->answerOptions.begin();
			option != pCharacter->answerOptions.end() && count<9; ++option, ++count)
	{
		ASSERT(*option < pCharacter->commands.size());
		CCharacterCommand& command = pCharacter->commands[*option];
		CDbSpeech *pSpeech = command.pSpeech;
		ASSERT(pSpeech);
		const WCHAR *pText = (const WCHAR*)pSpeech->MessageText;
		WSTRING wstr = wszAmpersand; //add # hotkey
		wstr += _itoW(count+1, temp, 10);
		wstr += wszPeriod;
		wstr += wszSpace;
		wstr += this->pGame->ExpandText(pText); //resolve var refs
		pMenu->AddText(wstr.c_str(), command.x);
	}

	//Resize label for prompt text height.
	SDL_Rect rect;
	pPrompt->GetRect(rect);
	UINT wTextHeight, wIgnored;
	g_pTheFM->GetTextRectHeight(FONTLIB::F_Message, pTextStr, rect.w, wIgnored, wTextHeight);
	pPrompt->SetHeight(wTextHeight);

	static const UINT FRAME_BUFFER = 3;
	CWidget *pFrame = this->pMenuDialog->GetWidget(TAG_MENUFRAME);

	//Resize rest of dialog widgets.
	const int yMenu = wTextHeight + (CY_SPACE * 2);
	const UINT wMenuHeight = pMenu->DispHeight();
	const UINT wTotalHeight = yMenu + wMenuHeight + FRAME_BUFFER*2 + CY_SPACE;

	this->pMenuDialog->SetHeight(wTotalHeight);

	//Center the dialog over the room.
	this->pMenuDialog->Center();
	const int xBuffer = (int(this->w) - int(CX_EDGE_BUFFER) - int(this->pMenuDialog->GetW())) / 2;
	this->pMenuDialog->Move(xBuffer > 0 ? xBuffer : 0, this->pMenuDialog->GetY());
	ASSERT(this->pMenuDialog->IsInsideOfParent()); //If this fires, the dialog probably has too many options

	pMenu->Move(CX_SPACE, yMenu);

	pFrame->Move(CX_SPACE - FRAME_BUFFER, yMenu - FRAME_BUFFER);
	pFrame->SetHeight(wMenuHeight + FRAME_BUFFER*2);

	//Get answer.
	Paint();
	ShowCursor();
	const UINT dwAnswer = this->pMenuDialog->Display();

	//Cleanup.
	pMenu->clear();
	Paint();
	return dwAnswer;
}

//******************************************************************************
void CSellScreenTCB::ProcessCommand(CCueEvents& /*CueEvents*/, const UINT command, const UINT wX, const UINT wY) //[default=(-1,-1)]
{
	this->cueEvents.Clear();
	pGame->ProcessCommand(command, this->cueEvents, wX, wY);
	ProcessEvents(this->cueEvents);
}

//******************************************************************************
void CSellScreenTCB::ProcessEvents(CCueEvents& CueEvents)
//Process script cues.
{
	SCREENTYPE eNextScreen = SCR_SellTCB;

	//Determine whether it's time to start playing next speech.
	const Uint32 dwNow = SDL_GetTicks();
	if (dwNow < this->dwNextSpeech)
		return;   //Wait some more.

	//Queue has been emptied and completed.  Return everything to normal.
	if (this->speech.empty() && this->dwNextSpeech)
	{
		//Can start a new speech command at any time.
		this->dwNextSpeech = 0;
	}

	//Part 1. Process cues.
	const CAttachableObject *pObj = NULL;
	for (pObj = CueEvents.GetFirstPrivateData(CID_Speech); pObj != NULL;
			pObj = CueEvents.GetNextPrivateData())
	{
		const CFiredCharacterCommand *pCmd = DYN_CAST(const CFiredCharacterCommand*, const CAttachableObject*, pObj);
		CCharacterCommand *pCommand = pCmd->pCommand;
		ASSERT(pCommand);
		CDbSpeech *pSpeech = pCommand->pSpeech;
		ASSERT(pSpeech);
		if (pSpeech) //robustness
			this->speech.push_back(const_cast<CFiredCharacterCommand*>(pCmd));
	}
	//Clear refs to data members now to avoid exceptions later.
	CueEvents.ClearEvent(CID_Speech, false);

	//Start next queued speech.
	if (!this->speech.empty())
	{
		CFiredCharacterCommand *pCommand = this->speech.front();
		ASSERT(pCommand->pCommand);
		CDbSpeech *pSpeech = pCommand->pCommand->pSpeech;
		ASSERT(pSpeech);

		//Play sound clip if allowed.
		int nChannel = -1;
		Uint32 dwDelay = pSpeech->dwDelay;
		if (pCommand->bPlaySound)
		{
			const CDbDatum *pSound = pSpeech->GetSound();
			if (pSound)
			{
				nChannel = g_pTheSound->PlayVoice(pSound->data);
				if (nChannel >= 0)
				{
					//Keep track of which channels are playing speech sound clips.
					ChannelInfo c = {(UINT)nChannel, false, {0.0, 0.0, 0.0}};
					this->speechChannels.push_back(c);
					//If delay is default, set it to length of the sound sample.
					if (!dwDelay)
						dwDelay = g_pTheSound->GetSoundLength(nChannel);
				}
				pSpeech->UnloadSound();
			}

			//Determine reading/speaking time for speech text.
			static const Uint32 dwBaseDuration = 1000;  //ms
			if (!dwDelay && pSpeech->MessageText.GetSize())
			{
				//Determine a "smart" default duration for this (non-empty) speech.
				dwDelay = dwBaseDuration + pSpeech->MessageText.GetSize() * 50;
			}

			//Add subtitle effect for character giving speech.
			if (this->bShowingSubtitlesWithVoice ||  //always show text
					(nChannel < 0 && dwDelay != 1)) //else, when text is disabled, skip instant subtitles 
						//i.e. when the text for a sound bite is too long to show on one line,
						//and must be broken into multiple lines that are displayed together.
						//A delay of 1ms is used by convention to effect this.
			{
				static const Uint32 dwMinDisplayDuration = 2000;  //ms
				AddSubtitle(pCommand, max(dwMinDisplayDuration, dwDelay));
			}
		}

		//Mark time next speech will begin.
		this->dwNextSpeech = dwNow + dwDelay;

		//Done with this speech command.
		delete pCommand;
		this->speech.pop_front();
	}

	//When no speech is playing, then process user prompts.
	if (!this->dwNextSpeech)
	{
		for (pObj = CueEvents.GetFirstPrivateData(CID_MonsterSpoke); pObj != NULL;
				pObj = CueEvents.GetNextPrivateData())
		{
			//Remove recent speech subtitles now so that they won't clutter up the space when
			//the response to the user's answer is presented.
			this->pEffects->RemoveEffectsOfType(ESUBTITLE);

			const CMonsterMessage *pMsg = DYN_CAST(const CMonsterMessage*, const CAttachableObject*, pObj);
			switch (pMsg->eType)
			{
			case MMT_YESNO:
			{
				UINT dwRet;
				DisableWidgetsForPrompt(true);
				if (pMsg->eMessageID)
					dwRet = ShowYesNoMessage(pMsg->eMessageID);
				else
					dwRet = ShowYesNoMessage(pMsg->message.c_str());
				DisableWidgetsForPrompt(false);
				switch (dwRet)
				{
					case TAG_QUIT: case TAG_ESCAPE:
						eNextScreen = SCR_None;
					break;
					case TAG_YES:
						ProcessCommand(CueEvents, CMD_YES); //recursive call
					break;
					default:
						ProcessCommand(CueEvents, CMD_NO); //recursive call
					break;
				}
			}
			break;

			case MMT_MENU:
			{
				DisableWidgetsForPrompt(true);
				const UINT dwAnswer = GetMessageAnswer(pMsg);
				DisableWidgetsForPrompt(false);
				switch (dwAnswer)
				{
					case TAG_QUIT: case TAG_ESCAPE:
						eNextScreen = SCR_None;
					break;
					default:
						ProcessCommand(CueEvents, CMD_ANSWER, dwAnswer/256, dwAnswer%256); //recursive call
					break;
				}
			}
			break;
			default: break;
			}
		}

		//Once all user prompts have been processed, then clear event to allow script to progress further.
		CueEvents.ClearEvent(CID_MonsterSpoke);
	}

	if ((UINT)eNextScreen != GetScreenType() && !IsDeactivating())
		GoToScreen(eNextScreen);
}

//******************************************************************************
bool CSellScreenTCB::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	ASSERT(!this->pGame);

	PlaySellSong();

	this->dwNextSpeech = 0;

	//Get official hold.
	CDb db;
	const UINT officialHoldID = db.Holds.GetHoldIDWithStatus(CDbHold::TCB);
	if (!officialHoldID)
		return true;

	//Get second level.
	const UINT entranceLevelID = db.Holds.GetLevelIDAtOrderIndex(2, officialHoldID);
	CDbLevel *pLevel = db.Levels.GetByID(entranceLevelID);
	if (!pLevel)
		return true;

	//Get room with sell screen script (entrance).
	const UINT sellScriptRoomID = pLevel->GetStartingRoomID();
	delete pLevel;
	this->pGame = db.GetNewTestGame(sellScriptRoomID, this->cueEvents, 0, 0, N, true);
	if (this->pGame)
	{
		//Specify settings.
		this->bShowingSubtitlesWithVoice = true; //defaults

		CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
		if (pPlayer)
		{
			this->bShowingSubtitlesWithVoice =
					pPlayer->Settings.GetVar(Settings::ShowSubtitlesWithVoices, true);
			LoadPlayerSettings(pPlayer);
			delete pPlayer;
		}

		//Start sell script playing at this point.
		CCharacter *pNPC = DYN_CAST(CCharacter*, CMonster*, this->pGame->pRoom->GetMonsterOfType(M_CHARACTER));
		ASSERT(pNPC);
		if (pNPC)
			VERIFY(pNPC->JumpToCommandWithLabel(
					this->pGame->stats.GetVar(this->pGame->pHold->getVarAccessToken("CONVERSATION_NO"), UINT(0))));
	}

	return true;
}

//******************************************************************************
void CSellScreenTCB::LoadPlayerSettings(CDbPlayer *pPlayer)
//Set in-game hold vars to provide contextual state for sell script operation.
{
	ASSERT(pPlayer);
	ASSERT(this->pGame);
	CDbPackedVars& ps = pPlayer->Settings; //from
	CDbPackedVars& gs = this->pGame->stats; //to
	CDbHold *pHold = this->pGame->pHold;
	CDb db;

	//Calculate first time values.
	UINT conversation = ps.GetVar(sellLastConversation, UINT(0));
	UINT playerType = ps.GetVar(sellDemoPlayStyle, UINT(PS_NotSpecified));
	if (!conversation && pPlayer->bIsImported) //imported players are automatically marked veterans on first run
	{
		playerType = PS_Veteran;
		conversation = 100;
	}
	const UINT completedTutorial = ps.GetVar(Settings::TutorialFinished, UINT(0));
	const UINT TCBTotalMinutes = ps.GetVar(Settings::TCBTotalPlayTime, UINT(0)) / 60; //sec -> min
	CIDSet roomIDs;
	db.Holds.GetRoomsExplored(pHold->dwHoldID, pPlayer->dwPlayerID, roomIDs, true);
	UINT roomsConquered = roomIDs.size();
	if (completedTutorial != 0)
		roomsConquered += 5;
	const UINT playerSkill = roomsConquered * 100 / (TCBTotalMinutes ? TCBTotalMinutes : 1);

	gs.SetVar(pHold->getVarAccessToken("CONVERSATION_NO"), conversation);
	gs.SetVar(pHold->getVarAccessToken("PLAYER_TYPE"), playerType);
	gs.SetVar(pHold->getVarAccessToken("COMPLETED_TUTORIAL"), completedTutorial);
	gs.SetVar(pHold->getVarAccessToken("TOTAL_TCB_SESSION_MINUTES"), TCBTotalMinutes);
	gs.SetVar(pHold->getVarAccessToken("504_ASKED_ONCE"), ps.GetVar(sell504Asked, UINT(0)));
	gs.SetVar(pHold->getVarAccessToken("COMPLETED_DEMO_SESSION_NO"), ps.GetVar(sellCompletedDemoSession, UINT(0)));
	gs.SetVar(pHold->getVarAccessToken("PLAYER_SKILL"), playerSkill);

	//Progress that might be located in any one of the player's saved games.
	const UINT
		bGuide = ps.GetVar(sellGuide, UINT(0)),
		bLane = ps.GetVar(sellLane, UINT(0)),
		bUncturage = ps.GetVar(sellUncturage, UINT(0)),
		bLibrary = ps.GetVar(sellLibrary, UINT(0)),
		bStarted = ps.GetVar(sellStarted, UINT(0));
	const UINT
		bNewTutorial = completedTutorial || pHold->VarIsSetInAnySavedGame("COMPLETED_TUTORIAL", pPlayer->dwPlayerID),
		bNewGuide = bGuide || pHold->VarIsSetInAnySavedGame("Beethro's Guide", pPlayer->dwPlayerID),
		bNewLane = bLane || pHold->VarIsSetInAnySavedGame("GoldenLaneFee", pPlayer->dwPlayerID),
		bNewUncturage = bUncturage || pHold->VarIsSetInAnySavedGame("COMPLETED_UNCTURAGE", pPlayer->dwPlayerID),
		bNewLibrary = bLibrary || pHold->VarIsSetInAnySavedGame("COMPLETED_LIBRARY", pPlayer->dwPlayerID),
		bNewStarted = bStarted || pHold->VarIsSetInAnySavedGame("STARTED_TCB", pPlayer->dwPlayerID) ||
				TCBTotalMinutes > 0;

	gs.SetVar(pHold->getVarAccessToken("COMPLETED_TUTORIAL"), bNewTutorial);
	gs.SetVar(pHold->getVarAccessToken("Beethro's Guide"), bNewGuide);
	gs.SetVar(pHold->getVarAccessToken("GoldenLaneFee"), bNewLane);
	gs.SetVar(pHold->getVarAccessToken("COMPLETED_UNCTURAGE"), bNewUncturage);
	gs.SetVar(pHold->getVarAccessToken("COMPLETED_LIBRARY"), bNewLibrary);
	gs.SetVar(pHold->getVarAccessToken("STARTED_TCB"), bNewStarted);
	gs.SetVar(pHold->getVarAccessToken("NEW_PROGRESS"),
			(!bGuide && bNewGuide) || (!bLane && bNewLane) ||
			(!bUncturage && bNewUncturage) || (!bLibrary && bNewLibrary) ||
			(!completedTutorial && bNewTutorial) || (!bStarted && bNewStarted));

	//How long app has been running.
	const UINT sessionMinutes = SDL_GetTicks() / (1000*60);
	gs.SetVar(pHold->getVarAccessToken("CURRENT_SESSION_TIME"), sessionMinutes);

	//Whether any homemade holds are installed.
	const UINT holdID = db.Holds.GetHoldIDWithStatus(CDbHold::Homemade);
	gs.SetVar(pHold->getVarAccessToken("NON_TCB_CONQUERED_ROOM_COUNT"), holdID ? 1 : 0);
}

//******************************************************************************
void CSellScreenTCB::SavePlayerSettings()
//Save state of sell dialogue in player settings.
{
	if (!this->pGame)
		return;
	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	if (!pPlayer)
		return;
	CDbPackedVars& ps = pPlayer->Settings; //to
	CDbPackedVars& gs = this->pGame->stats; //from
	CDbHold *pHold = this->pGame->pHold;

	//Save these vars to return to sell dialogue at specified point next session.
	ps.SetVar(sellDemoPlayStyle, gs.GetVar(pHold->getVarAccessToken("PLAYER_TYPE"), UINT(0)));
	ps.SetVar(sellLastConversation, gs.GetVar(pHold->getVarAccessToken("CONVERSATION_NO"), UINT(0)));
	ps.SetVar(sell504Asked, gs.GetVar(pHold->getVarAccessToken("504_ASKED_ONCE"), UINT(0)));
	ps.SetVar(sellCompletedDemoSession, gs.GetVar(pHold->getVarAccessToken("COMPLETED_DEMO_SESSION_NO"), UINT(0)));
	ps.SetVar(Settings::TutorialFinished, gs.GetVar(pHold->getVarAccessToken("COMPLETED_TUTORIAL"), UINT(0)));
	ps.SetVar(sellGuide, gs.GetVar(pHold->getVarAccessToken("Beethro's Guide"), UINT(0)));
	ps.SetVar(sellLane, gs.GetVar(pHold->getVarAccessToken("GoldenLaneFee"), UINT(0)));
	ps.SetVar(sellUncturage, gs.GetVar(pHold->getVarAccessToken("COMPLETED_UNCTURAGE"), UINT(0)));
	ps.SetVar(sellLibrary, gs.GetVar(pHold->getVarAccessToken("COMPLETED_LIBRARY"), UINT(0)));
	ps.SetVar(sellStarted, gs.GetVar(pHold->getVarAccessToken("STARTED_TCB"), UINT(0)));

	//The rest of the vars were not modified during script execution and don't need updating.

	pPlayer->Update();
	delete pPlayer;
}

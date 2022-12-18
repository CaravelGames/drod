// $Id: GameScreen.cpp 10138 2012-04-25 21:48:22Z mrimer $

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
 * Richard Cookney (timeracer), JP Burford (jpburford), John Wm. Wicks (j_wicks),
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "GameScreen.h"
#include "BrowserScreen.h"
#include "DemosScreen.h"
#include "EditRoomScreen.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"
#include "DrodScreenManager.h"
#include "DrodSound.h"
#include "TileImageCalcs.h"
#include "TileImageConstants.h"

#include "BloodEffect.h"
#include "DebrisEffect.h"
#include "ExplosionEffect.h"
#include "SparkEffect.h"
#include "SplashEffect.h"
#include "SteamEffect.h"
#include "StrikeOrbEffect.h"
#include "SwordsmanSwirlEffect.h"
#include "SwordSwingEffect.h"
#include "TarStabEffect.h"
#include "TrapdoorFallEffect.h"
#include "VerminEffect.h"

//#include "ClockWidget.h"
#include "FaceWidget.h"
#include "MapWidget.h"
#include "RoomWidget.h"
#include "RoomEffectList.h"
#include <FrontEndLib/Fade.h>
#include <FrontEndLib/BumpObstacleEffect.h>
#include <FrontEndLib/FadeTileEffect.h>
#include <FrontEndLib/FlashMessageEffect.h>
#include <FrontEndLib/FloatTextEffect.h>
#include <FrontEndLib/MovingTileEffect.h>
#include <FrontEndLib/ScaleTileEffect.h>
#include <FrontEndLib/TextBoxWidget.h>
#include <FrontEndLib/TextEffect.h>
#include <FrontEndLib/TransTileEffect.h>
#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/DialogWidget.h>
#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/MenuWidget.h>
#include <FrontEndLib/TilesWidget.h>
#include "../Texts/MIDs.h"

#include "../DRODLib/Character.h"
#include "../DRODLib/Combat.h"
#include "../DRODLib/CueEvents.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbRooms.h"
#include "../DRODLib/DbPlayers.h"
#include "../DRODLib/DbXML.h"
#include "../DRODLib/GameConstants.h"
#include "../DRODLib/Mimic.h"
#include "../DRODLib/Monster.h"
#include "../DRODLib/MonsterFactory.h"
#include "../DRODLib/MonsterPiece.h"
#include "../DRODLib/SettingsKeys.h"
#include "../DRODLib/TileConstants.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Clipboard.h>
#include <BackEndLib/CoordIndex.h>
#include <BackEndLib/CoordStack.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/IDList.h>
#include <BackEndLib/Wchar.h>
#include <BackEndLib/Ports.h>

#ifdef STEAMBUILD
#	include <steam_api.h>
#endif

#define DEFAULT_COMBAT_TICK (75) //default speed one round of combat plays out (ms)

#define MOVES_TO_UNDO_BY_BUTTON (10) //how many moves to undo per Undo button click

const UINT GO_TO_STAIRS = ORIENTATION_COUNT; //meta-direction for quick room exit pathing

const UINT MAX_REPEAT_RATE = 16; //traverse one room per second

const UINT TAG_CLOCK = 1020;
const UINT TAG_MENUPROMPT = 1021;
const UINT TAG_MENUFRAME = 1022;
const UINT TAG_MENU = 1023;
const UINT TAG_FACE = 1024;
const UINT TAG_BIGMAPCONTAINER = 1025;
const UINT TAG_BIGMAP = 1026;

const UINT TAG_ESC = 1030;
const UINT TAG_HELP = 1031;
const UINT TAG_SAVE = 1032;
const UINT TAG_LOAD = 1033;

const UINT TAG_CHATBOX = 1034;
const UINT TAG_CHATENABLE = 1035;
const UINT TAG_CHATWHISPERSONLY = 1036;
const UINT TAG_CHATINPUT = 1037;
const UINT TAG_CHATUSERS = 1038;
const UINT TAG_SIGN_AREA = 1039;

const UINT TAG_UNDO = 1040;
const UINT TAG_ROT_CW = 1041;
const UINT TAG_ROT_CCW = 1042;
const UINT TAG_LOCK = 1043;
const UINT TAG_WAIT = 1044;
const UINT TAG_USECOMMAND = 1045;

const UINT TAG_BATTLEDIALOG = 1050;
const UINT TAG_BATTLETILES = 1051;
const UINT TAG_BATTLETEXT = 1052;
const UINT TAG_BATTLEFRAME = 1053;
const UINT TAG_BATTLEOK = 1054;

const UINT TAG_SCOREDIALOG = 1060;
const UINT TAG_SCORENAME = 1061;
const UINT TAG_SCOREFRAME = 1062;
const UINT TAG_SCORETEXT = 1063;
const UINT TAG_SCORETOTAL = 1064;
const UINT TAG_SCORE_OK = 1065;

const UINT TAG_UNDO_FROM_QUESTION = UINT(-9); //unused value

const UINT CX_SIDEBAR = 260; //width of active sidebar widget area

const UINT CX_SPACE = 12;
const UINT CY_SPACE = 12;

//UINT wMood = SONG_PUZZLE;
int nDangerLevel = 0;	//for setting room mood

typedef map<ROOMCOORD, vector<UINT> > TilesMap;


struct MonsterStats {
	MonsterStats(UINT monsterType, UINT ATK, UINT DEF, UINT HP, UINT GR, UINT color)
		: monsterType(monsterType), ATK(ATK), DEF(DEF), HP(HP), GR(GR), color(color)
	{}
	bool operator==(const MonsterStats& that) const
	{
		return
			this->monsterType == that.monsterType &&
			this->ATK == that.ATK &&
			this->DEF == that.DEF &&
			this->HP == that.HP &&
			this->GR == that.GR &&
			this->color == that.color;
	}
	UINT monsterType, ATK, DEF, HP, GR, color;
};

//const SURFACECOLOR lockColor = {255, 255, 128};
const SDL_Color Red = {196, 0, 0, 0}; //cutscene sign text color
//const SDL_Color Blue = {0, 0, 196, 0}; //displaying other room sign text color

CCombat *g_pPredictedCombat = NULL;

const UINT numMenuPics = 4+3; //monster tiles (4), sword, shield, accessory
const int X_PIC[numMenuPics] = {160, 160+44, 160,    160+44,  135, 135, 135};
const int Y_PIC[numMenuPics] = {427, 427,    427+44, 427+44,  205, 255, 305};

const int rightEndOfEquipmentSlot = 259; //same as value in CX_SWORD in RoomScreen.cpp

#ifdef STEAMBUILD
CSteamLeaderboards::CSteamLeaderboards() { }

void CSteamLeaderboards::FindLeaderboardAndUploadScore(const char *pchLeaderboardName, int score)
{
	if (!pchLeaderboardName)
		return;

	const string leaderboard(pchLeaderboardName);
	m_scoresToUpload.insert(make_pair(leaderboard, score)); //persist

	if (!FindLeaderboard(pchLeaderboardName))
		m_scoresToUpload.erase(leaderboard);
}

bool CSteamLeaderboards::FindLeaderboard(const char *pchLeaderboardName)
{
	if (SteamUserStats()) {
		SteamAPICall_t hSteamAPICall = SteamUserStats()->FindLeaderboard(pchLeaderboardName);
		m_steamCallResultFindLeaderboard.Set(hSteamAPICall, this, &CSteamLeaderboards::OnFindLeaderboard);
		return true;
	}

	return false;
}

bool CSteamLeaderboards::UploadScore(SteamLeaderboard_t hLeaderboard)
{
	if (!hLeaderboard)
		return false;

	const char *pchName = SteamUserStats()->GetLeaderboardName(hLeaderboard);
	if (!pchName)
		return false;
	map<string, int>::iterator it = m_scoresToUpload.find(string(pchName));
	if (it == m_scoresToUpload.end())
		return false;
	const int score = it->second;

	SteamAPICall_t hSteamAPICall = SteamUserStats()->UploadLeaderboardScore(
			hLeaderboard, k_ELeaderboardUploadScoreMethodKeepBest, score, NULL, 0);
	m_SteamCallResultUploadScore.Set(hSteamAPICall, this, &CSteamLeaderboards::OnUploadScore);

	// Load the top entry of the specified leaderboard.
	hSteamAPICall = SteamUserStats()->DownloadLeaderboardEntries(
			hLeaderboard, k_ELeaderboardDataRequestGlobal, 0, 1);
	m_steamCallResultDownloadScore.Set(hSteamAPICall, this, &CSteamLeaderboards::OnDownloadTopScore);

	return true;
}

void CSteamLeaderboards::OnFindLeaderboard(LeaderboardFindResult_t *pResult, bool bIOFailure) {
	// see if we encountered an error during the call
	if (!pResult->m_bLeaderboardFound || bIOFailure)
	{
		//OutputDebugString("Leaderboard could not be found\n");
		return;
	}

	UploadScore(pResult->m_hSteamLeaderboard);
}

void CSteamLeaderboards::OnUploadScore(LeaderboardScoreUploaded_t *pResult, bool bIOFailure) {
	if (!pResult->m_bSuccess || bIOFailure)
	{
		//OutputDebugString( "Score could not be uploaded to Steam\n" );
	}
}

void CSteamLeaderboards::OnDownloadTopScore(LeaderboardScoresDownloaded_t *pResult, bool bIOFailure)
{
	if (bIOFailure)
		return;

	const char *pchName = SteamUserStats()->GetLeaderboardName(pResult->m_hSteamLeaderboard);
	if (!pchName)
		return;

	if (pResult->m_cEntryCount) {
		LeaderboardEntry_t m_leaderboardEntry;
		SteamUserStats()->GetDownloadedLeaderboardEntry(
				pResult->m_hSteamLeaderboardEntries, 0, &m_leaderboardEntry, NULL, 0);

		ShowSteamScoreOnGameScreen(pchName, m_leaderboardEntry.m_nScore);
	}
}

void CSteamLeaderboards::ShowSteamScoreOnGameScreen(const char *pchLeaderboardName, int topScore)
{
	if (!pchLeaderboardName)
		return;

	map<string, int>::iterator it = m_scoresToUpload.find(string(pchLeaderboardName));
	if (it != m_scoresToUpload.end()) {
		CGameScreen *pGameScreen = DYN_CAST(CGameScreen*, CScreen*, g_pTheSM->GetScreen(SCR_Game));
		if (pGameScreen) {
			const int playerScore = it->second;
			pGameScreen->ShowScorepointRating(playerScore, topScore);
		}
	}
}

CSteamLeaderboards g_steamLeaderboards;

#endif

//
//CGameScreen public methods.
//


#define BG_SURFACE      (0) //same as in RoomScreen.cpp

#define BLANK_ITEM      (UINT(-1))
#define NO_DESTINATION  (UINT(-1))

//*****************************************************************************
UINT CGameScreen::CalcTileImageForEquipment(const UINT type) const
//Returns: the tile image to represent this item type in the player's inventory
{
	//Look up custom inventory image.
	CCharacter *pCharacter = this->pCurrentGame->getCustomEquipment(type);
	if (pCharacter)
	{
		UINT tile = this->pRoomWidget->GetCustomEntityTile(
				pCharacter->wLogicalIdentity, NO_ORIENTATION, 0);
		if (tile == TI_UNSPECIFIED)
		{
			//Show a predefined sword if no custom images for custom equipment exist.
			if (type == ScriptFlag::Weapon)
				tile = CalcTileImageForSword(pCharacter->getSword());

			if (tile == TI_UNSPECIFIED)
				tile = TI_GENERIC_EQUIPMENT;
		}
		return tile;
	}

	//Get predefined equipment image.
	switch (type)
	{
		case ScriptFlag::Weapon:
			return CalcTileImageForSword(this->pCurrentGame->pPlayer->st.sword);
		case ScriptFlag::Armor:
			return CalcTileImageForShield(this->pCurrentGame->pPlayer->st.shield);
		case ScriptFlag::Accessory:
			return CalcTileImageForAccessory(this->pCurrentGame->pPlayer->st.accessory);
		default: return TI_UNSPECIFIED;
	}
}

//*****************************************************************************
UINT CGameScreen::GetMonsterDisplayTile(CMonster *pMonster, const UINT x, const UINT y)
//Return: a tile to be displayed for the (x,y) of the monster
{
	UINT val = TI_UNSPECIFIED;
	ASSERT(pMonster);

	switch (y)
	{
		case 0:
		switch (x)
		{
			case 0:
			{
				static const UINT tile[MONSTER_TYPES] = {
					TI_ROACH_S, TI_QROACH_S, TI_REGG_4, TI_GOBLIN_S, TI_HALPH_S,
					TI_WW_S, TI_EYE_S, TI_SNKT_W, TI_TAREYE_WO, TI_TARBABY_S,
					TI_BRAIN, TI_MIMIC_S, TI_SPIDER_S, TI_SNKT_G_W, TI_SNKT_B_W,
					TI_ROCK_S, TI_WATERSKIPPER_S, TI_SKIPPERNEST, TI_AUMTLICH_S,
					TI_SMAN_YS, TI_SMAN_IYS, TI_WUBBA, TI_SEEP_S, TI_PIRATE_S,
					TI_HALPH_S, TI_SLAYER_S, TI_FEGUNDO_S, TI_UNSPECIFIED,
					TI_GUARD_S, TI_UNSPECIFIED, TI_MUDEYE_WO, TI_MUDBABY_S,
					TI_GELEYE_WO, TI_GELBABY_S, TI_CITIZEN_S, TI_ROCKGIANT_S,
					TI_EYE_WS, TI_GOBLINKING_S
				};
				val = tile[pMonster->wType];
				//NPC special appearance.
				if (pMonster->wType == M_CHARACTER)
				{
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
					val = this->pRoomWidget->GetEntityTile(pCharacter->wIdentity,
							pCharacter->wLogicalIdentity,
							bMonsterHasDirection(pCharacter->wIdentity) ? S : NO_ORIENTATION, 0);
				}
			}
			break;
			case 1:
				switch (pMonster->wType)
				{
					case M_MUDMOTHER: val = TI_MUDEYE_EO; break;
					case M_TARMOTHER: val = TI_TAREYE_EO; break;
					case M_GELMOTHER: val = TI_GELEYE_EO; break;
					case M_SERPENT: val = TI_SNK_E; break;
					case M_SERPENTB: val = TI_SNK_B_E; break;
					case M_SERPENTG: val = TI_SNK_G_E; break;
					case M_ROCKGIANT: val = TI_ROCKGIANT_S1; break;
					default: break; //no additional tiles needed
				}
			break;
			default: ASSERT(!"Unsupported (x,0) display location"); break;
		}
		break;

		case 1:
		switch (x)
		{
			case 0:
				switch (pMonster->wType)
				{
					case M_ROCKGIANT: val = TI_ROCKGIANT_S2; break;
					case M_SLAYER: val = TI_SLAYER_SWORD_S; break;
					case M_BEETHRO: case M_BEETHRO_IN_DISGUISE:
					case M_CLONE: val = TI_SWORD_YS; break;
					case M_DECOY: val = TI_SWORD_IYS; break;
					case M_MIMIC: val = TI_MIMIC_SWORD_S; break;
					case M_GUARD: case M_PIRATE: case M_STALWART: val = TI_GUARD_SWORD_S; break;
					case M_CHARACTER:
					{
						//NPC sword tile.
						UINT wSX, wSY;
						const bool bHasSword = pMonster->GetSwordCoords(wSX, wSY);
						if (bHasSword && this->pCurrentGame)
						{
							CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pMonster);
							
							const UINT sword = pCharacter->getSword();
							if (sword == NPC_DEFAULT_SWORD)
							{
								ASSERT(this->pCurrentGame->pHold);
								HoldCharacter *pCustomChar = this->pCurrentGame->pHold->GetCharacter(
										pCharacter->wLogicalIdentity);
								if (pCustomChar)
									val = g_pTheBM->GetCustomTileNo(pCustomChar->dwDataID_Tiles,
											this->pRoomWidget->GetCustomTileIndex(S), SWORD_FRAME);
							} else {
								//Show custom weapon type.
								val = this->pRoomWidget->GetSwordTile(pCharacter->GetIdentity(), S, sword);
							}
							//Calculate monster's default sword tile if no custom tile is provided.
							if (val == TI_UNSPECIFIED)
								val = this->pRoomWidget->GetSwordTile(pCharacter->GetIdentity(), S);
						}
					}
					break;
					default: break;
				}
			break;
			case 1:
				switch (pMonster->wType)
				{
					case M_ROCKGIANT: val = TI_ROCKGIANT_S3; break;
					default: break;
				}
			break;
			default: ASSERT(!"Unsupported (x,1) display location"); break;
		}
		break;
		default: ASSERT(!"Unsupported y display location"); break;
	}

	return val;
}

//*****************************************************************************
void CGameScreen::RedrawStats(
//Render the player stats.
//If a combat is specified, list the enemy's stats too.
//
//Params:
	CCombat *pCombat,        //if not NULL, indicates a real battle just happened in-game.
	                         //Otherwise, display any user-requested battle prediction
	const bool bUpdateRect)  //whether to paint to screen
{
	if (!this->pCurrentGame)
		return;

	SDL_Surface *pDestSurface = GetDestSurface();
	const CSwordsman& player = *this->pCurrentGame->pPlayer;
	const PlayerStats& ps = player.st;
	WCHAR temp[12];
	SDL_Rect src, dest;
	UINT i;

	//If combat is occurring, reset any previous battle prediction.
	if (pCombat)
	{
		delete g_pPredictedCombat;
		g_pPredictedCombat = NULL;
	} else {
		//Otherwise, display last battle prediction requested by user.
		if (g_pPredictedCombat)
		{
			//Ensure a new game hasn't been loaded since prediction was requested.
			if (g_pPredictedCombat->pGame == this->pCurrentGame)
			{
				pCombat = g_pPredictedCombat;
			} else {
				//Prediction no longer applies.
				delete g_pPredictedCombat;
				g_pPredictedCombat = NULL;
			}
		}
	}

	//If scroll is no longer visible, refresh this part of the status display.
	if (!this->bIsScrollVisible)
		PaintScroll();

	ASSERT(this->images[BG_SURFACE]);
	const bool bCombat = pCombat != NULL;

	//Refresh texts.
	static const UINT numStats = 17;
	static const UINT statTag[numStats] = {
		TAG_HP, TAG_ATK, TAG_DEF, TAG_GOLD,
		TAG_YKEY, TAG_GKEY, TAG_BKEY,
		TAG_MONNAME, TAG_MONHP, TAG_MONATK, TAG_MONDEF,
		TAG_SWORD, TAG_SHIELD, TAG_ACCESSORY,
		TAG_SKEY, TAG_XP, TAG_ITEMMULT
	};

	//Refresh sprites.
	static const UINT MAX_KEY_DISPLAY = 99;

	const bool bSwordDisabled = this->pCurrentGame->IsPlayerSwordDisabled();
	const bool bShieldDisabled = this->pCurrentGame->IsPlayerShieldDisabled();
	const bool bAccessoryDisabled = this->pCurrentGame->IsPlayerAccessoryDisabled();

	for (i=0; i<numMenuPics; ++i)
	{
		//Erase old pic.
		src.x = dest.x = X_PIC[i];
		src.y = dest.y = Y_PIC[i];
		src.w = dest.w = g_pTheDBM->CX_TILE;
		src.h = dest.h = g_pTheDBM->CY_TILE;
		SDL_BlitSurface(this->images[BG_SURFACE], &src, pDestSurface, &dest);
		if (bUpdateRect)
			UpdateRect(dest);
	}

	WSTRING wstr;
	for (i=0; i<numStats; ++i)
	{
		int val = 0;
		switch (i)
		{
			case 0: val = ps.HP; if (val < 0) val = 0; break;
			case 1: val = this->pCurrentGame->getPlayerATK(); break;
			case 2: val = this->pCurrentGame->getPlayerDEF(); break;
			case 3: val = ps.GOLD; break;
			case 4: val = ps.yellowKeys >= MAX_KEY_DISPLAY ? MAX_KEY_DISPLAY : ps.yellowKeys; break;
			case 5: val = ps.greenKeys >= MAX_KEY_DISPLAY ? MAX_KEY_DISPLAY : ps.greenKeys ; break;
			case 6: val = ps.blueKeys >= MAX_KEY_DISPLAY ? MAX_KEY_DISPLAY : ps.blueKeys; break;
			case 7: wstr = (bCombat ? this->pRoomWidget->GetMonsterName(pCombat->pMonster) : wszEmpty); break;
			case 8: val = bCombat ? pCombat->pMonster->getHP() : BLANK_ITEM; break;
			case 9: val = bCombat ? pCombat->monATK : BLANK_ITEM; break;
			case 10: val = bCombat ? pCombat->monDEF : BLANK_ITEM; break;
			case 11: val = ps.sword != NoSword ? CRoomWidget::GetSwordMID(ps.sword) : (UINT)MID_NoText; break;
			case 12: val = ps.shield != NoShield ? CRoomWidget::GetShieldMID(ps.shield) : (UINT)MID_NoText; break;
			case 13: val = ps.accessory != NoAccessory ? CRoomWidget::GetAccessoryMID(ps.accessory) : (UINT)MID_NoText; break;
			case 14: val = ps.skeletonKeys >= MAX_KEY_DISPLAY ? MAX_KEY_DISPLAY : ps.skeletonKeys; break;
			case 15: val = ps.XP; break;
			case 16:
				val = int(this->pCurrentGame->pLevel->dwMultiplier) * int(ps.itemMult) / 100;
				if (val > 99999)
					val = 99999; //don't overwrite area
				else if (val < -99999)
					val = -99999;
			break;
			default: break;
		}

		CLabelWidget *pLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(statTag[i]));

		//Repaint background.
		src.x = dest.x = pLabel->GetX();
		src.y = dest.y = pLabel->GetY();
		src.w = dest.w = pLabel->GetW();
		src.h = dest.h = pLabel->GetH();
		SDL_BlitSurface(this->images[BG_SURFACE], &src, pDestSurface, &dest);

		switch (i)
		{
			case 7:
				pLabel->SetText(wstr.c_str());
			break;
			case 11: case 12: case 13:
			{
				UINT item = i==11?ps.sword : i==12?ps.shield : ps.accessory;
				if (bIsCustomEquipment(item))
				{
					ASSERT(this->pCurrentGame);
					if (this->pCurrentGame->pHold)
					{
						HoldCharacter *pChar = this->pCurrentGame->pHold->GetCharacter(item);
						if (pChar)
						{
							pLabel->SetText(pChar->charNameText.c_str());
							break;
						}
					}
				}
				pLabel->SetText(g_pTheDB->GetMessageText(val));
			}
			break;
			case 0: case 1: case 2:
			case 3: case 4: case 5: case 6: case 15:
				pLabel->SetText(_itoW(val, temp, 10));
			break;
			case 16:
			{
				static const WCHAR multiplier[] = {We('x'),We(0)};
				WSTRING text = multiplier;
				text += _itoW(val, temp, 10);
				pLabel->SetText(text.c_str());
			}
			break;
			default:
				//allow values to be blank
				pLabel->SetText(val == (int)BLANK_ITEM ? wszEmpty : _itoW(val, temp, 10));
			break;
		}
		pLabel->RequestPaint();
	}

	//Draw tile image.
	for (i=0; i<numMenuPics; ++i)
	{
		UINT val = TI_UNSPECIFIED;
		switch (i)
		{
			//Draw image of monster being fought.
			case 0: case 1:
			case 2: case 3:
			if (bCombat)
				val = GetMonsterDisplayTile(pCombat->pMonster, i % 2, i / 2);
			break;

			//Draw sword.
			case 4:
			{
				val = CalcTileImageForEquipment(ScriptFlag::Weapon);
				if (val != TI_UNSPECIFIED && val != TI_TEMPTY)
					g_pTheDBM->BlitTileImage(val, X_PIC[i], Y_PIC[i], pDestSurface);

				//Draw crossed-out item when it can't be used.
				val = bSwordDisabled ? TI_CHECKPOINT : TI_TEMPTY;
			}
			break;

			//Draw shield
			case 5:
			{
				val = CalcTileImageForEquipment(ScriptFlag::Armor);
				if (val != TI_UNSPECIFIED && val != TI_TEMPTY)
					g_pTheDBM->BlitTileImage(val, X_PIC[i], Y_PIC[i], pDestSurface);

				//Draw crossed-out item when it can't be used.
				val = bShieldDisabled ? TI_CHECKPOINT : TI_TEMPTY;
			}
			break;

			//Draw accessory
			case 6:
			{
				val = CalcTileImageForEquipment(ScriptFlag::Accessory);
				if (val != TI_UNSPECIFIED && val != TI_TEMPTY)
					g_pTheDBM->BlitTileImage(val, X_PIC[i], Y_PIC[i], pDestSurface);

				//Draw crossed-out item when it can't be used.
				val = bAccessoryDisabled ? TI_CHECKPOINT : TI_TEMPTY;
			}
			break;
			default: break;
		}

		if (val != TI_TEMPTY && val != TI_UNSPECIFIED)
		{
			g_pTheDBM->BlitTileImage(val, X_PIC[i], Y_PIC[i], pDestSurface);

			//Add monster color to display image.
			if (i==0)
			{
				ASSERT(pCombat->pMonster);
				this->pRoomWidget->AddColorToTile(pDestSurface, pCombat->pMonster->getColor(), val, X_PIC[i], Y_PIC[i],
						CBitmapManager::CX_TILE, CBitmapManager::CY_TILE);
			}
		}
	}

	//Draw scroll, if visible, over the top of the stat display.
	if (this->bIsScrollVisible)
		PaintScroll(bUpdateRect);
}

//*****************************************************************************
SEID CGameScreen::GetMonsterAttackSoundEffect(CCombat* pCombat) const
{
	bool bMonsterHitsPlayerWithSword = false;
	if (pCombat)
	{
		UINT wX,wY;
		if (pCombat->pMonster->GetSwordCoords(wX,wY))
			bMonsterHitsPlayerWithSword = true;
	}
	return bMonsterHitsPlayerWithSword ?
		SEID_SPLAT : SEID_MONSTERATTACK;
}


//*****************************************************************************
bool CGameScreen::LoadContinueGame()
//Loads current game from current player's continue saved game slot.
//
//Returns:
//True if successful, false if not.
{
	//Load the game.
	const UINT dwContinueID = g_pTheDB->SavedGames.FindByContinue();
	if (!dwContinueID)
		return false;
	return LoadSavedGame(dwContinueID);
}

//*****************************************************************************
bool CGameScreen::LoadQuicksave()
//Loads current game from current player's quicksave saved game slot for this hold.
//
//Returns:
//True if successful, false if not.
{
	//Load the game.
	const UINT quicksaveID = g_pTheDB->SavedGames.FindByContinue(ST_Quicksave);
	if (!quicksaveID)
		return false;
	return LoadSavedGame(quicksaveID);
}

//*****************************************************************************
bool CGameScreen::LoadSavedGame(
//Loads current game from a saved game.
//
//Params:
	const UINT dwSavedGameID, //(in)   Saved game to load.
	bool bRestoreFromStart, //(in)   If true, game will be restored from start
							//    without playing back commands.  Default is
							//    false.
	const bool bNoSaves) //[default=false]
//
//Returns:
//True if successful, false if not.
{
	//Get rid of current game if needed.
	DeleteCurrentGame();

	//Load the game.
	this->bIsSavedGameStale = false;
	ClearCueEvents();
	this->pCurrentGame = g_pTheDB->GetSavedCurrentGame(dwSavedGameID,
			this->sCueEvents, bRestoreFromStart, bNoSaves);
	if (!this->pCurrentGame)
		return false;

	this->bPlayTesting = false;
	this->wUndoToTurn = this->pCurrentGame->wTurnNo;
//	this->bRoomClearedOnce = this->pCurrentGame->IsCurrentRoomPendingExit();

	SetSignTextToCurrentRoom();
	SetQuickCombat();
	return true;
}

//*****************************************************************************
bool CGameScreen::LoadNewGame(
//Loads current game from beginning of specified hold.
//
//Returns:
//True if successful, false if not.
//
//Params:
	const UINT dwHoldID)   //(in)
{
	//Get rid of current game if needed.
	DeleteCurrentGame();

	//Load the game.
	this->bIsSavedGameStale = false;
	ClearCueEvents();
	StopAmbientSounds();
	ClearSpeech(true);
	this->pCurrentGame = g_pTheDB->GetNewCurrentGame(dwHoldID, this->sCueEvents);
	if (!this->pCurrentGame) return false;

	//Set current game for widgets.
	if (!this->pMapWidget->LoadFromCurrentGame(this->pCurrentGame) ||
			!this->pRoomWidget->LoadFromCurrentGame(this->pCurrentGame))
		return false;

	this->bPlayTesting = false;
	this->wUndoToTurn = 0;
//	this->bRoomClearedOnce = this->pCurrentGame->IsCurrentRoomPendingExit();

	SetSignTextToCurrentRoom();
	SetQuickCombat();
	return true;
}

//*****************************************************************************
void CGameScreen::MarkCurrentEntranceExplored()
//Hook to mark the current hold entrance explored.
//Currently only used at the start of a new game.
{
	//At a level entrance position that should be shown?
	CEntranceData *pEntrance = this->pCurrentGame->pHold->GetEntranceAt(
			this->pCurrentGame->pRoom->dwRoomID,
			this->pCurrentGame->pPlayer->wX, this->pCurrentGame->pPlayer->wY);
	if (pEntrance)
		this->pCurrentGame->entrancesExplored += pEntrance->dwEntranceID;
}

//*****************************************************************************
bool CGameScreen::ShouldShowLevelStart()
//Used by screen-changing code outside of CGameScreen to determine if the
//level start screen should be shown before the game screen.
//
//Returns:
//True if it should, false if not.
{
	if (this->bIsSavedGameStale)
		return false;

	const bool bShowLevelStart = this->pCurrentGame->ShowLevelStart();
	if (bShowLevelStart)
		//I only want the caller to go to the level start screen once for each
		//time a saved game is loaded.  The save game is now "stale" until
		//one of the saved game loading methods of this class is called.  Then
		//it will set the flag to false again.
		this->bIsSavedGameStale = true;

	return bShowLevelStart;
}

//*****************************************************************************
void CGameScreen::SetGameAmbience(const bool bRecalc) //[default=false]
//Sets various ambient effects based on game state.
{
	//Beethro can go to sleep if room is peaceful/static and no dialog is occurring.
	if (this->pCurrentGame)
	{
		if (bRecalc)
			nDangerLevel = this->pCurrentGame->pRoom->DangerLevel();

		ASSERT(this->pRoomWidget);
		UINT wX, wY;
		this->pCurrentGame->GetSwordsman(wX, wY);
		const bool bValidTile = this->pCurrentGame->pRoom->IsValidColRow(wX, wY) &&
				this->pCurrentGame->pRoom->GetOSquare(wX, wY) != T_HOT;
		this->pRoomWidget->AllowSleep(this->speech.empty() && !nDangerLevel &&
//				this->pCurrentGame->pPlayer->wAppearance == M_BEETHRO &&
				!this->pCurrentGame->dwCutScene &&
//				this->pCurrentGame->pRoom->briars.empty() &&
				!this->pCurrentGame->InCombat() &&
				this->pCurrentGame->pRoom->LitFuses.empty() &&
//				!this->pCurrentGame->pPlayer->wPlacingDoubleType &&
				bValidTile &&
				GetScreenType() != SCR_Demo);
	}
}

//*****************************************************************************
void CGameScreen::SetMusicStyle()
//Changes the music to match room style, according to game mood.
//If music style already matches the current style, nothing will happen.
{
	const MusicData& music = this->pCurrentGame->music;
	if (!music.songMood.empty())
	{
		//Fade to next song in the specified style-mood's track list and update play order.
		CFiles f;
		list<WSTRING> songlist;
		//Ensure mood selection is valid.
		if (f.GetGameProfileString(INISection::Songs, music.songMood.c_str(), songlist))
		{
			g_pTheSound->CrossFadeSong(&songlist);
			f.WriteGameProfileString(INISection::Songs, music.songMood.c_str(), songlist);
			return;
		}
	}

	if (music.bPlayMusicID)
	{
		const UINT musicEnum = music.musicEnum;
		if (musicEnum != (UINT)SONGID_DEFAULT)
		{
			if (musicEnum == (UINT)SONGID_NONE)	//no music
				g_pTheSound->StopSong();
			else if (musicEnum < (UINT)SONGID_COUNT)
				g_pTheSound->CrossFadeSong(musicEnum);
			else if (musicEnum == UINT(SONGID_CUSTOM)) //custom music
				g_pTheSound->PlayData(music.musicID);
			return;
		}
	}

	//Default music mood -- based on room style and game environment.
	ASSERT(this->pCurrentGame);
	ASSERT(this->pCurrentGame->pRoom);
	nDangerLevel = this->pCurrentGame->pRoom->DangerLevel();
	UINT wMood = SONG_PUZZLE; //!nDangerLevel ? SONG_AMBIENT : nDangerLevel < 30 ? SONG_PUZZLE : SONG_ATTACK;

	CRoomScreen::SetMusicStyle(this->pCurrentGame->pRoom->style, wMood);
}

//*****************************************************************************
void CGameScreen::SetQuickCombat()
//Player settings option: immediately combat resolution on encounter.
{
	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	CDbPackedVars &settings = pCurrentPlayer->Settings;
	const BYTE val = settings.GetVar(Settings::CombatRate, BYTE(0));
	switch (val)
	{
		case 0: this->wCombatTickSpeed = DEFAULT_COMBAT_TICK; break;
		case 1: this->wCombatTickSpeed = DEFAULT_COMBAT_TICK/2; break;
		case 2: this->wCombatTickSpeed = DEFAULT_COMBAT_TICK/4; break;
		case 3: this->wCombatTickSpeed = DEFAULT_COMBAT_TICK/8; break;
		case 4: this->wCombatTickSpeed = 0; break;
	}
	delete pCurrentPlayer;

	this->pCurrentGame->bQuickCombat = (val == COMBAT_SPEED_NOTCHES-1);
	this->wThisCombatTickSpeed = this->wCombatTickSpeed;
}

//*****************************************************************************
bool CGameScreen::TestRoom(
//Returns: whether room is successfully loaded for testing
//
//Params:
	const UINT dwRoomID,   //(in) room to start in
	const UINT wX, const UINT wY, const UINT wO, //(in) Starting position
	const PlayerStats& st) //(in) starting stats
{
	//Get rid of current game if needed.
	DeleteCurrentGame();

	//Load the game.
	this->bIsSavedGameStale = false;
	ClearCueEvents();
	this->pCurrentGame = g_pTheDB->GetNewTestGame(dwRoomID, this->sCueEvents, wX, wY, wO,
			st, true); //don't save to DB while testing
	if (!this->pCurrentGame) return false;

	//Set current game for widgets.
	if (!this->pMapWidget->LoadFromCurrentGame(this->pCurrentGame) ||
			!this->pRoomWidget->LoadFromCurrentGame(this->pCurrentGame))
		return false;

	this->bPlayTesting = true;
	this->wUndoToTurn = 0;
//	this->bRoomClearedOnce = this->pCurrentGame->IsCurrentRoomPendingExit();

	SetSignTextToCurrentRoom();
	SetQuickCombat();
	return true;
}

//*****************************************************************************
bool CGameScreen::UnloadGame()
//Deletes the current game.
//
//Returns true if successful.
{
	LOGCONTEXT("CGameScreen::UnloadGame");
	if (this->pCurrentGame)
	{
		//Save current game to continue slot, unless this is CGameScreen-derived
		//CDemoScreen, or after play-testing.
		if (GetScreenType() == SCR_Game && !this->bPlayTesting &&
				g_pTheDB->GetHoldID() != g_pTheDB->Holds.GetHoldIDWithStatus(CDbHold::Tutorial))
		{
			this->pCurrentGame->SaveToContinue();
			g_pTheDB->Commit();
		}

		//Free current game.
		DeleteCurrentGame();
	}
	this->pRoomWidget->ResetRoom();
	ClearCueEvents(); //so no cues play on reentering screen

	return true;
}

//
//CGameScreen protected methods.
//

//*****************************************************************************
CGameScreen::CGameScreen(const SCREENTYPE eScreen) : CRoomScreen(eScreen)
	, pCurrentGame(NULL)
	, pRoomWidget(NULL)

	, bShowLevelStartBeforeActivate(false)
	, bPersistentEventsDrawn(false)
	, bNeedToProcessDelayedQuestions(false)
	, bShowingBigMap(false), bShowingCutScene(false), bShowingTempRoom(false)
	, bIsDialogDisplayed(false)
	, bDisableMouseMovement(false), bNoMoveByCurrentMouseClick(false)
	, wCombatTickSpeed(DEFAULT_COMBAT_TICK), wThisCombatTickSpeed(DEFAULT_COMBAT_TICK)
	, wMoveDestX(NO_DESTINATION), wMoveDestY(NO_DESTINATION)
	, wRoomQuickExitDirection(NO_ORIENTATION)

	, pFaceWidget(NULL)
	, pClockWidget(NULL)
	, pMenuDialog(NULL), pScoreDialog(NULL)
	, pSpeechBox(NULL)
	, pBigMapWidget(NULL)
	, pTempRoomWidget(NULL)

	, dwNextSpeech(0)
	, bShowingSubtitlesWithVoice(true)
	, dwTimeMinimized(0), dwLastCutSceneMove(0), dwSavedMoveDuration(0)
	, dwLastCombatTick(0)

	, bIsSavedGameStale(false)
	, bPlayTesting(false)
//	, bRoomClearedOnce(false)
	, wUndoToTurn(0)

	, fPos(NULL)

	, wUploadingScoreHandle(0)
	, dwUploadingDemo(0), dwUploadingSavedGame(0)
//Constructor.
{
	this->fPos = new float[3];
	this->fPos[0] = this->fPos[1] = this->fPos[2] = 0.0;

	//Room widgets.
	static const int X_ROOM = 163 + 44*3;
	static const int Y_ROOM = 40;
	static const int X_FACE = 15;
	static const int Y_FACE = 15;
//	static const int X_CLOCK = 4;
//	static const int Y_CLOCK = 413;

	//Command buttons.
	static const int X_ESC = 10;
	static const int Y_ESC = 726;
	static const UINT CX_ESC = 100;
	static const UINT CY_ESC = 32;

	static const int X_HELP = X_ESC + CX_ESC + 10;
	static const int Y_HELP = Y_ESC;
	static const UINT CX_HELP = 80;
	static const UINT CY_HELP = CY_ESC;

	static const int X_SAVE = 157;
	static const int Y_SAVE = 13;
	static const UINT CX_SAVE = 50;
	static const UINT CY_SAVE = 32;

	static const int X_LOAD = X_SAVE + CX_SAVE + 6;
	static const int Y_LOAD = Y_SAVE;
	static const UINT CX_LOAD = 47;
	static const UINT CY_LOAD = CY_SAVE;

	static const int X_WAIT = X_SAVE;
	static const int Y_WAIT = Y_SAVE + CY_SAVE + 13;
	static const UINT CX_WAIT = CX_SAVE;
	static const UINT CY_WAIT = CY_SAVE;

	static const int X_LOCK = X_LOAD;
	static const int Y_LOCK = Y_WAIT;
	static const UINT CX_LOCK = CX_LOAD;
	static const UINT CY_LOCK = CY_SAVE;

	static const int X_CCW = X_SAVE;
	static const int Y_CCW = Y_WAIT + CY_WAIT + 13;
	static const UINT CX_CCW = CX_SAVE;
	static const UINT CY_CCW = CY_SAVE;

	static const int X_CW = X_LOAD;
	static const int Y_CW = Y_CCW;
	static const UINT CX_CW = CX_LOAD;
	static const UINT CY_CW = CY_SAVE;

	static const int X_UNDO = X_SAVE;
	static const int Y_UNDO = Y_CCW + CY_CCW + 13;
	static const UINT CX_UNDO = CX_SAVE;
	static const UINT CY_UNDO = CY_WAIT;

	static const int X_COMMAND = X_LOAD;
	static const int Y_COMMAND = Y_UNDO;
	static const UINT CX_COMMAND = CX_LOAD;
	static const UINT CY_COMMAND = CY_UNDO;

	//Pop-up menu.
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

	//Pop-up map
	static const UINT BIGMAP_MARGIN = 100;
	const UINT CX_BIGMAP = CDrodBitmapManager::CX_ROOM - 2*BIGMAP_MARGIN;
	const UINT CY_BIGMAP = CDrodBitmapManager::CY_ROOM - 2*BIGMAP_MARGIN;

	//Pop up battle stats dialog.
	static const UINT CX_MESSAGE = 690;
	static const UINT CY_MESSAGE = 200;
	static const UINT FRAME_BUFFER = 3;

	static const UINT CX_BATTLEFRAME = CX_MESSAGE - (CX_SPACE - FRAME_BUFFER)*2;
	static const UINT CY_BATTLEFRAME = CY_MESSAGE - (CY_SPACE - FRAME_BUFFER)*2;
	static const int X_BATTLETILES = CX_SPACE/2;
	static const int Y_BATTLETILES = CY_SPACE/2;
	const UINT CX_BATTLETILES = CDrodBitmapManager::CX_TILE*2;
	const int X_BATTLETEXT = X_BATTLETILES + CX_BATTLETILES;
	const UINT CX_BATTLETEXT = CX_BATTLEFRAME - X_BATTLETEXT;

	static const UINT CX_MESSAGE_BUTTON = 80;
	static const UINT CY_MESSAGE_BUTTON = CY_STANDARD_BUTTON;
	static const int Y_MESSAGE_BUTTON = CY_TEXT + (CY_SPACE * 2);
	static const int X_OK1 = (CX_MESSAGE - CX_MESSAGE_BUTTON) / 2;

	//Score dialog.
	static const UINT CX_SCOREDIALOG = 600;
	static const UINT CY_SCOREDIALOG = 320;

	static const int Y_SCORENAME = 15;
	static const UINT CY_SCORENAME = 40;

	static const int Y_SCOREFRAME = Y_SCORENAME + CY_SCORENAME;
	static const UINT CX_SCOREFRAME = CX_SCOREDIALOG - CX_SPACE * 2;

	static const int X_SCORETEXT = 15;
	static const int Y_SCORETEXT = 15;
	static const UINT CX_SCORETEXT = 500;
	static const UINT CY_SCORETEXT = 140;

//	static const int X_SCORETOTAL = X_SCORETEXT;
	static const int Y_SCORETOTAL = Y_SCORETEXT + CY_SCORETEXT;
//	static const UINT CX_SCORETOTAL = CX_SCOREFRAME - X_SCORETOTAL;
	static const UINT CY_SCORETOTAL = 40;

	static const UINT CX_OK_BUTTON = 100;

	static const UINT CY_SCOREFRAME = CY_SCOREDIALOG - Y_SCOREFRAME - CY_SPACE;

	//Add widgets.
	this->pRoomWidget = new CRoomWidget(TAG_ROOM, X_ROOM, Y_ROOM,
			CDrodBitmapManager::CX_ROOM, CDrodBitmapManager::CY_ROOM);
	AddWidget(this->pRoomWidget);
	this->pTempRoomWidget = new CRoomWidget(0,
			this->pRoomWidget->GetX(), this->pRoomWidget->GetY(),
			this->pRoomWidget->GetW(),this->pRoomWidget->GetH());
	this->pTempRoomWidget->Hide();
	AddWidget(this->pTempRoomWidget);

	this->pFaceWidget = new CFaceWidget(TAG_FACE, X_FACE, Y_FACE, CX_FACE, CY_FACE);
	AddWidget(this->pFaceWidget);

//No clock in the RPG.
/*
	this->pClockWidget = new CClockWidget(TAG_CLOCK, X_CLOCK, Y_CLOCK, CX_CLOCK, CY_CLOCK);
	AddWidget(this->pClockWidget);
*/

	if (eScreen != SCR_Demo)
	{
		this->pMapWidget->Enable();
		this->pMapWidget->bUserMoveable = false;

/*
		CLabelWidget *pLabel = new CLabelWidget(TAG_HELP, X_HELP, Y_HELP, CX_HELP, CY_HELP, F_ButtonWhite,
				g_pTheDB->GetMessageText(MID_F1Help));
		pLabel->SetClickable(true);
		AddWidget(pLabel);
*/
		const SDL_Rect& EntireSign = GetEntireSignRect();
		CLabelWidget *pLabel = new CLabelWidget(TAG_SIGN_AREA,
			EntireSign.x, EntireSign.y, EntireSign.w, EntireSign.h, F_Button, wszEmpty);
		pLabel->SetClickable(true);
		AddWidget(pLabel);

		CButtonWidget *pButton;
		pButton = new CButtonWidget(TAG_ESC, X_ESC, Y_ESC, CX_ESC, CY_ESC,
				g_pTheDB->GetMessageText(MID_EscMenu));
		pButton->SetFocusAllowed(false); //don't tab through the command buttons
		AddWidget(pButton);
		pButton = new CButtonWidget(TAG_HELP, X_HELP, Y_HELP, CX_HELP, CY_HELP,
				g_pTheDB->GetMessageText(MID_F1Help));
		pButton->SetFocusAllowed(false);
		AddWidget(pButton);

		pButton = new CButtonWidget(TAG_SAVE, X_SAVE, Y_SAVE, CX_SAVE, CY_SAVE,
				g_pTheDB->GetMessageText(MID_Save));
		pButton->SetFocusAllowed(false);
		pButton->SetSilent();
		AddWidget(pButton);
		pButton = new CButtonWidget(TAG_LOAD, X_LOAD, Y_LOAD, CX_LOAD, CY_LOAD,
				g_pTheDB->GetMessageText(MID_Load));
		pButton->SetFocusAllowed(false);
		pButton->SetSilent();
		AddWidget(pButton);

		pButton = new CButtonWidget(TAG_ROT_CCW, X_CCW, Y_CCW, CX_CCW, CY_CCW,
				g_pTheDB->GetMessageText(MID_CCW));
		pButton->SetFocusAllowed(false);
		pButton->SetSilent();
		AddWidget(pButton);
		pButton = new CButtonWidget(TAG_ROT_CW, X_CW, Y_CW, CX_CW, CY_CW,
				g_pTheDB->GetMessageText(MID_CW));
		pButton->SetFocusAllowed(false);
		pButton->SetSilent();
		AddWidget(pButton);

		pButton = new CButtonWidget(TAG_WAIT, X_WAIT, Y_WAIT, CX_WAIT, CY_WAIT,
				g_pTheDB->GetMessageText(MID_Wait));
		pButton->SetFocusAllowed(false);
		pButton->SetSilent();
		AddWidget(pButton);
		pButton = new CButtonWidget(TAG_LOCK, X_LOCK, Y_LOCK, CX_LOCK, CY_LOCK,
				g_pTheDB->GetMessageText(MID_LockCommand));
		pButton->SetFocusAllowed(false);
		pButton->SetSilent();
		AddWidget(pButton);
		pButton = new CButtonWidget(TAG_UNDO, X_UNDO, Y_UNDO, CX_UNDO, CY_UNDO,
				g_pTheDB->GetMessageText(MID_Undo));
		pButton->SetFocusAllowed(false);
		pButton->SetSilent();
		AddWidget(pButton);
		pButton = new CButtonWidget(TAG_USECOMMAND, X_COMMAND, Y_COMMAND,
				CX_COMMAND, CY_COMMAND, g_pTheDB->GetMessageText(MID_UseCommandButton));
		pButton->SetFocusAllowed(false);
		pButton->SetSilent();
		AddWidget(pButton);

		//Menu dialog.
		this->pMenuDialog = new CDialogWidget(0L, 0, 0, CX_MENUBOX, CY_MENUBOX);
		this->pMenuDialog->Hide();
		AddWidget(this->pMenuDialog);

		pLabel = new CLabelWidget(TAG_MENUPROMPT, X_TEXT, Y_TEXT,
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

		//Level list dialog box.
		this->pSpeechBox = new CEntranceSelectDialogWidget(0L);
		AddWidget(this->pSpeechBox);
		this->pSpeechBox->Move(
			X_ROOM + (CDrodBitmapManager::CX_ROOM - this->pSpeechBox->GetW()) / 2,
			Y_ROOM + (CDrodBitmapManager::CY_ROOM - this->pSpeechBox->GetH()) / 2);   //center over room widget
		this->pSpeechBox->Hide();

		//Pop-up map.
		CScrollableWidget *pScrollingMap = new CScrollableWidget(TAG_BIGMAPCONTAINER, 0, 0,
				CX_BIGMAP, CY_BIGMAP);
		pScrollingMap->Hide();
		this->pRoomWidget->AddWidget(pScrollingMap);
		pScrollingMap->Center();
		this->pBigMapWidget = new CMapWidget(TAG_BIGMAP, 0, 0,
				CDrodBitmapManager::DISPLAY_COLS * MAPSIZE_MULTIPLIER,
				CDrodBitmapManager::DISPLAY_ROWS * MAPSIZE_MULTIPLIER,
				NULL, MAPSIZE_MULTIPLIER);
		this->pBigMapWidget->bUserMoveable = false;
		this->pBigMapWidget->Hide();
		pScrollingMap->AddWidget(this->pBigMapWidget);

		//Pop-up battle stats dialog.
		CDialogWidget *pStatsDialog = new CDialogWidget(TAG_BATTLEDIALOG, 0, 0, CX_MESSAGE, CY_MESSAGE);
		pStatsDialog->Hide();
		this->pRoomWidget->AddWidget(pStatsDialog);

		pFrame = new CFrameWidget(TAG_BATTLEFRAME,
				CX_SPACE - FRAME_BUFFER, CY_SPACE - FRAME_BUFFER,
				CX_BATTLEFRAME, CY_BATTLEFRAME, NULL);
		pStatsDialog->AddWidget(pFrame);

		pFrame->AddWidget(new CTilesWidget(TAG_BATTLETILES,
				X_BATTLETILES, Y_BATTLETILES, CX_BATTLETILES, 0));

		pLabel = new CLabelWidget(TAG_BATTLETEXT,
				X_BATTLETEXT, Y_BATTLETILES, CX_BATTLETEXT, 0,
				F_Message, wszEmpty);
		pFrame->AddWidget(pLabel);

		pButton = new CButtonWidget(TAG_BATTLEOK, X_OK1, Y_MESSAGE_BUTTON,
						CX_MESSAGE_BUTTON, CY_MESSAGE_BUTTON, g_pTheDB->GetMessageText(MID_Okay));
		pFrame->AddWidget(pButton);

		//Score checkpoint dialog.
		this->pScoreDialog = new CDialogWidget(TAG_SCOREDIALOG, 0, 0, CX_SCOREDIALOG, CY_SCOREDIALOG);
		this->pScoreDialog->Hide();
		this->pRoomWidget->AddWidget(this->pScoreDialog);

		//Score checkpoint name.
		pLabel = new CLabelWidget(TAG_SCORENAME, 0, Y_SCORENAME,
				CX_SCOREDIALOG, CY_SCORENAME, F_Title, wszEmpty);
		pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
		this->pScoreDialog->AddWidget(pLabel);

		pFrame = new CFrameWidget(TAG_SCOREFRAME, CX_SPACE, Y_SCOREFRAME,
				CX_SCOREFRAME, CY_SCOREFRAME, NULL);
		this->pScoreDialog->AddWidget(pFrame);

		//Score texts.
		pLabel = new CLabelWidget(TAG_SCORETEXT, X_SCORETEXT, Y_SCORETEXT,
					CX_SCORETEXT, CY_SCORETEXT,
					F_Message, wszEmpty);
		pFrame->AddWidget(pLabel);

		pLabel = new CLabelWidget(TAG_SCORETOTAL, 0, Y_SCORETOTAL,
					CX_SCOREFRAME, CY_SCORETOTAL,
					F_ScoreTotal, wszEmpty);
		pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
		pFrame->AddWidget(pLabel);

		//Score buttons.
		pButton = new CButtonWidget(TAG_SCORE_OK,
				(CX_SCOREFRAME - CX_OK_BUTTON) / 2, CY_SCOREFRAME - CY_STANDARD_BUTTON - CY_SPACE,
				CX_OK_BUTTON, CY_STANDARD_BUTTON, g_pTheDB->GetMessageText(MID_Okay));
		pFrame->AddWidget(pButton);

		AddChatDialog();
	} else {
		CLabelWidget *pLabel = new CLabelWidget(TAG_ESC, X_ESC, Y_ESC, CX_ESC, CY_ESC, F_ButtonWhite,
				g_pTheDB->GetMessageText(MID_EscMenu));
//		pLabel->SetClickable(eScreen != SCR_Demo);
		AddWidget(pLabel);
	}
}

//******************************************************************************
CGameScreen::~CGameScreen()
{
	delete[] this->fPos;
	UnloadGame();
}

//*****************************************************************************
void CGameScreen::AddChatDialog()
//Show dialog box for CaravelNet chat.
{
	static const UINT CX_SPACE = 10;
	static const UINT CY_SPACE = 12;

	static const UINT CX_DIALOG = 620;
	static const UINT CY_DIALOG = 415;

	static const int Y_HEADER = 15;
	static const int X_HEADER = 20;
	static const UINT CX_HEADER = CX_DIALOG - 2*X_HEADER;
	static const UINT CY_HEADER = 30;

	static const int X_CHATOPTION = X_HEADER;
	static const int Y_CHATOPTION = Y_HEADER + CY_HEADER + CY_STANDARD_OPTIONBUTTON + CY_SPACE;
	static const UINT CY_CHATOPTION = CY_STANDARD_OPTIONBUTTON;
	static const UINT CX_CHATOPTION = 300;
	static const int X_CHATWHISPEROPTION = X_CHATOPTION;
	static const int Y_CHATWHISPEROPTION = Y_CHATOPTION + CY_CHATOPTION + CY_SPACE;
	static const UINT CX_CHATWHISPEROPTION = CX_CHATOPTION;

	static const int Y_OKAY_BUTTON = CY_DIALOG - CY_STANDARD_BUTTON - CY_SPACE*3/2;
	static const int Y_CHAT = Y_OKAY_BUTTON - CY_STANDARD_TBOX - CY_SPACE;

	static const UINT CY_USERLISTLABEL = 27;
	static const int X_USERLIST = X_CHATOPTION + CX_CHATOPTION + CX_SPACE;
	static const int Y_USERLISTLABEL = Y_CHATOPTION - CY_USERLISTLABEL;
	static const int Y_USERLIST = Y_USERLISTLABEL + CY_USERLISTLABEL;
	static const UINT CX_USERLIST = CX_DIALOG - X_USERLIST - X_HEADER;
	static const UINT CY_USERLIST = 10*22 + 4;

	static const UINT CX_OK_BUTTON = 80;

	CDialogWidget *pStatsBox = new CDialogWidget(TAG_CHATBOX, 0, 0, CX_DIALOG, CY_DIALOG);

	CLabelWidget *pLabel = new CLabelWidget(0L, X_HEADER, Y_HEADER, CX_HEADER,
			CY_HEADER, F_Header, g_pTheDB->GetMessageText(MID_ChatTitle));
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	pStatsBox->AddWidget(pLabel);

	//Chat.
	pStatsBox->AddWidget(new COptionButtonWidget(TAG_CHATENABLE, X_CHATOPTION, Y_CHATOPTION,
			CX_CHATOPTION, CY_CHATOPTION, g_pTheDB->GetMessageText(MID_ChatEnableOnGameScreen)));
	pStatsBox->AddWidget(new COptionButtonWidget(TAG_CHATWHISPERSONLY, X_CHATWHISPEROPTION, Y_CHATWHISPEROPTION,
			CX_CHATWHISPEROPTION, CY_CHATOPTION, g_pTheDB->GetMessageText(MID_ReceiveWhispersOnly)));
	pStatsBox->AddWidget(new CTextBoxWidget(TAG_CHATINPUT, X_CHATOPTION, Y_CHAT,
			CX_DIALOG - X_CHATOPTION*2, CY_STANDARD_TBOX));
	pStatsBox->AddWidget(new CLabelWidget(0, X_USERLIST, Y_USERLISTLABEL,
			CX_USERLIST, CY_USERLISTLABEL, F_FrameCaption, g_pTheDB->GetMessageText(MID_PlayersChatting)));
	pStatsBox->AddWidget(new CListBoxWidget(TAG_CHATUSERS, X_USERLIST, Y_USERLIST,
			CX_USERLIST, CY_USERLIST, false, false, true));

	//Buttons.
	CButtonWidget *pButton = new CButtonWidget(TAG_OK,
			(CX_DIALOG - CX_OK_BUTTON) / 2, Y_OKAY_BUTTON,
			CX_OK_BUTTON, CY_STANDARD_BUTTON, g_pTheDB->GetMessageText(MID_Okay));
	pStatsBox->AddWidget(pButton);

	this->pRoomWidget->AddWidget(pStatsBox,true);
	pStatsBox->Center();
	pStatsBox->Hide();
}

//*****************************************************************************
void CGameScreen::AddVisualEffect(const VisualEffectInfo* pEffect)
//Add a visual effect to the room.
//wX/Y: sound location, wO: direction, wValue: effect type, wValue2: play sound or not
{
	const CMoveCoordEx2& effect = pEffect->effect;
	const CCoord& src = pEffect->source;
	CMoveCoord coord(effect.wX, effect.wY, effect.wO);
	const bool bSound = effect.wValue2 != 0;
	UINT particles = 5; //can be increased by triggering multiple effects

	if (bSound)
	{
		UINT soundID=SEID_NONE;
		switch (effect.wValue)
		{
			case VET_BLOODSPLAT: case VET_SEEPSPLAT: soundID = SEID_SPLAT; break;
			case VET_MUDSPLAT: case VET_TARSPLAT: case VET_GELSPLAT: soundID = SEID_STABTAR; break;
			case VET_SLAYERSPLAT: soundID = SEID_SLAYERDIE; break;
			case VET_GOLEMSPLAT: case VET_DEBRIS: soundID = SEID_BREAKWALL; break;
			case VET_SPARKS: soundID = SEID_STARTFUSE; break;
			case VET_EXPLOSION: soundID = SEID_BOMBEXPLODE; break;
			case VET_SPLASH: soundID = SEID_SPLASH; break;
			case VET_STEAM: soundID = SEID_SIZZLE; break;
			case VET_BOLT: soundID = SEID_ORBHITQUIET; break;
			case VET_MONSTERBITE: soundID = SEID_MONSTERATTACK; break;
			case VET_PARRY: soundID = SEID_SHIELDED; break;
			case VET_STRONGHIT: soundID = SEID_HIT; break;
			case VET_EQUIP: soundID = SEID_SWORDS; break;
			default: break;
		}

		if (soundID != (UINT)SEID_NONE)
			g_pTheSound->PlaySoundEffect(soundID);
	}

	switch (effect.wValue)
	{
		case VET_BLOODSPLAT:
			this->pRoomWidget->AddTLayerEffect(
				new CBloodEffect(this->pRoomWidget, coord, particles,
						GetEffectDuration(7), GetParticleSpeed(3)));
		break;
		case VET_MUDSPLAT:
			this->pRoomWidget->AddTLayerEffect(
				new CMudStabEffect(this->pRoomWidget, coord,
						GetEffectDuration(7), GetParticleSpeed(3), particles));
		break;
		case VET_TARSPLAT:
			this->pRoomWidget->AddTLayerEffect(
				new CTarStabEffect(this->pRoomWidget, coord,
						GetEffectDuration(7), GetParticleSpeed(3), particles));
		break;
		case VET_GELSPLAT:
			this->pRoomWidget->AddTLayerEffect(
				new CGelStabEffect(this->pRoomWidget, coord,
						GetEffectDuration(7), GetParticleSpeed(3), particles));
		break;
		case VET_SEEPSPLAT:
			this->pRoomWidget->AddTLayerEffect(
				new CBloodInWallEffect(this->pRoomWidget, coord, particles));
		break;
		case VET_GOLEMSPLAT:
			this->pRoomWidget->AddTLayerEffect(
				new CGolemDebrisEffect(this->pRoomWidget, coord, particles,
						GetEffectDuration(7), GetParticleSpeed(3)));
		break;
		case VET_SLAYERSPLAT:
			this->pRoomWidget->AddTLayerEffect(
				new CVerminEffect(this->pRoomWidget, coord, particles, true));
		break;
		case VET_DEBRIS:
			this->pRoomWidget->AddTLayerEffect(
				new CDebrisEffect(this->pRoomWidget, coord, particles,
						GetEffectDuration(7), GetParticleSpeed(4)));
		break;
		case VET_SPARKS:
			this->pRoomWidget->AddTLayerEffect(
					new CSparkEffect(this->pRoomWidget, coord, particles, false));
		break;
		case VET_EXPLOSION:
			this->pRoomWidget->AddMLayerEffect(
				new CExplosionEffect(this->pRoomWidget, coord,
						GetEffectDuration(500)));
		break;
		case VET_SPLASH:
			this->pRoomWidget->AddTLayerEffect(
					new CSplashEffect(this->pRoomWidget, coord));
		break;
		case VET_STEAM:
			this->pRoomWidget->AddTLayerEffect(
					new CSteamEffect(this->pRoomWidget, coord));
		break;
		case VET_SWIRL:
			this->pRoomWidget->AddMLayerEffect(
				new CSwordsmanSwirlEffect(this->pRoomWidget, coord));
		break;
		case VET_VERMIN:
			this->pRoomWidget->AddTLayerEffect(
				new CVerminEffect(this->pRoomWidget, coord, particles));
		break;
		case VET_BOLT:
		{
			static COrbData orbData;
			orbData.wX = src.wX;
			orbData.wY = src.wY;
			orbData.AddAgent(new COrbAgentData(effect.wX, effect.wY, OA_NULL));

			this->pRoomWidget->AddStrikeOrbEffect(orbData, false);

			orbData.ClearAgents();
		}
		break;
		case VET_MONSTERBITE: break; //show nothing
		case VET_PARRY:
			this->pRoomWidget->AddMLayerEffect(
					new CFadeTileEffect(this->pRoomWidget, coord, TI_CHECKPOINT_L, 500));
		break;
		case VET_JITTER:
			this->pRoomWidget->AddJitter(coord, 0.1f); //10%
		break;
		case VET_STRONGHIT:
			this->pRoomWidget->AddMLayerEffect(
					new CFadeTileEffect(this->pRoomWidget, coord, TI_STRONGHIT, 500));
		break;
		default: break; //do nothing
	}
}

//*****************************************************************************
void CGameScreen::ApplyINISettings()
//(Re)query the INI for current values and apply them.
{
	CDrodScreen::ApplyINISettings();

/*
	//Set game replay speed optimization.
	string str;
	if (CFiles::GetGameProfileString(INISection::Customizing, INIKey::MaxDelayForUndo, str))
		this->pCurrentGame->SetComputationTimePerSnapshot(atoi(str.c_str()));
*/

	//Force room style reload.
	this->pRoomWidget->UpdateFromCurrentGame(true);
}

bool CGameScreen::CanShowVarUpdates() const {
	return
		this->bPlayTesting ||
		(this->pCurrentGame && this->pCurrentGame->pHold && (
			g_pTheNet->IsBetaHold(this->pCurrentGame->pHold->dwHoldID) ||
			this->pCurrentGame->pHold->PlayerCanEdit(g_pTheDB->GetPlayerID()))
		);
}

//*****************************************************************************
void CGameScreen::ChatPolling(const UINT tagUserList)
//Override CDrodScreen's method.
{
	//During playtesting, a temporary player profile is used that doesn't
	//have the player's CaravelNet user settings.
	//So, when processing CaravelNet transactions, set the active player
	//profile to the real user for the duration of the transaction.
	CEditRoomScreen *pEditRoomScreen = NULL;
	if (this->bPlayTesting)
	{
		pEditRoomScreen = DYN_CAST(CEditRoomScreen*, CScreen*,
				g_pTheSM->GetScreen(SCR_EditRoom));
		ASSERT(pEditRoomScreen);
		g_pTheDB->SetPlayerID(pEditRoomScreen->GetSavePlayerID(), false); //don't relogin to CaravelNet
	}

	CDrodScreen::ChatPolling(tagUserList);

	if (this->bPlayTesting)
	{
		ASSERT(pEditRoomScreen);
		g_pTheDB->SetPlayerID(pEditRoomScreen->GetTestPlayerID(), false);
	}
}

//******************************************************************************
void CGameScreen::ClearCueEvents()
//Clears static instance of the cue events.
{
	this->sCueEvents.Clear();
}

//*****************************************************************************
void CGameScreen::ClearSpeech(const bool bForceClearAll) //[default=false]
//If bForceClearAll is true, then reset queued speech actions and cut any playing custom sound clips.
//Otherwise, only stop any speech/sound playing that doesn't apply to this point in game time.
{
	const UINT currentTurn = this->pCurrentGame ? this->pCurrentGame->wTurnNo : 0;

	//Keep any speech that hasn't played yet.
	deque<CFiredCharacterCommand*> retain;
	deque<CFiredCharacterCommand*>::iterator iter;
	for (iter=this->speech.begin(); iter!=this->speech.end(); ++iter)
	{
		CFiredCharacterCommand *pCommand = *iter;
		if (pCommand->turnNo < currentTurn && !bForceClearAll)
		{
			//Hook up the command to the current instance of the character with its scriptID.
			ASSERT(this->pCurrentGame);
			if (pCommand->bPseudoMonster)
			{
				//No information is stored to reattach speech command.
				delete pCommand;
			} else {
				pCommand->pExecutingNPC = pCommand->pSpeakingEntity =
						pCurrentGame->GetCharacterWithScriptID(pCommand->scriptID);
				ASSERT(pCommand->pExecutingNPC);
				if (pCommand->pExecutingNPC) //robustness
				{
					//This speech command may be reliably retained.
					retain.push_back(pCommand); 

					//Also hook up this wrapper object to the current instance of this script command.
					CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pCommand->pExecutingNPC);
					ASSERT(pCommand->commandIndex < pCharacter->commands.size());
					pCommand->pCommand = &(pCharacter->commands[pCommand->commandIndex]);
					PrepCustomSpeaker(pCommand);
				} else {
					delete pCommand;
				}
			}
		} else {
			delete pCommand;
		}
	}
	this->speech = retain;

	CutSpeech(bForceClearAll);

	//The call to CutSpeech marks which subtitles should be retained.
	//Now remove subtitles which have not been marked.
	ASSERT(this->pRoomWidget);
	this->pRoomWidget->RemoveLastLayerEffectsOfType(ESUBTITLE, bForceClearAll);

	//If all playing sounds have been stopped, then we can free them.
	//Otherwise, let everything remain for now.
	if (this->speechChannels.empty() && this->ambientChannels.empty())
	{
		if (g_pTheSound)
			g_pTheSound->FreeSoundDump();

		//Show player when no one is speaking.
		this->pFaceWidget->SetSpeaker(false);
	}

	SetGameAmbience();
}

//*****************************************************************************
void CGameScreen::ClickOnEquipment(
//Handle user clicking on an equipment item to activate it.
//
//Params:
	const UINT eCommand, //game command to execute
	const bool bRightMouseButton)  //whether right mouse button was clicked
{
	if (GetScreenType() != SCR_Game || this->bIsScrollVisible)
		return;
	if (!this->pCurrentGame || !this->pCurrentGame->bIsGameActive)
		return;

	if (!bRightMouseButton)
	{
		//Execute game command.
		ProcessCommandWrapper(eCommand);
	} else {
		//Display info text for this equipment item.
		WSTRING text = GetEquipmentPropertiesText(eCommand);

		RemoveToolTip();
		ShowToolTip(text.c_str());
	}
}

//******************************************************************************
void CGameScreen::DisplayChatText(const WSTRING& text, const SDL_Color& color)
{
	this->pRoomWidget->DisplayChatText(text, color);
}

//******************************************************************************
void CGameScreen::DrawCurrentTurn()
//Redraws everything to show the current game state.
{
	UpdateSound();

	//Refresh pointers but don't reload front-end resources.
	this->pRoomWidget->LoadFromCurrentGame(this->pCurrentGame, false);

	this->bPersistentEventsDrawn = this->bNeedToProcessDelayedQuestions = false;
	ProcessCueEventsBeforeRoomDraw(this->sCueEvents);

	this->pRoomWidget->DontAnimateMove();
	this->pRoomWidget->Paint();

	ProcessCueEventsAfterRoomDraw(this->sCueEvents);
	this->pFaceWidget->Paint();
	this->pMapWidget->DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom, this->pCurrentGame->pRoom->mapMarker);
	this->pMapWidget->RequestPaint();
	UpdateScroll();
	PaintClock();

	RedrawStats(this->pCurrentGame->pCombat, true);
}

//*****************************************************************************
WSTRING CGameScreen::GetEquipmentPropertiesText(const UINT eCommand)
//Returns: a string containing text describing the properties of the indicated equipment type (based on execution command)
{
	//Get equipment properties.
	const PlayerStats& st = this->pCurrentGame->pPlayer->st;
	int atk=0, def=0;
	bool bMetal=false, bBeamBlock=false, bBriar=false, bLuckyGR=false,
		bAttackFirst=false, bAttackLast=false, bBackstab=false, bNoEnemyDEF=false,
		bGoblinWeakness=false, bSerpentWeakness=false, bCustomWeakness=false, bLuckyXP=false;

	CCharacter *pCharacter = NULL; //custom equipment
	switch (eCommand)
	{
		case CMD_USE_WEAPON:
			if (this->pCurrentGame->IsPlayerSwordDisabled())
				break;

			atk = this->pCurrentGame->getWeaponPower(st.sword);
			if (bIsCustomEquipment(st.sword))
			{
				pCharacter = this->pCurrentGame->getCustomEquipment(ScriptFlag::Weapon);
				if (pCharacter)
				{
					def = (int)pCharacter->getDEF();
					bGoblinWeakness = pCharacter->HasGoblinWeakness();
					bSerpentWeakness = pCharacter->HasSerpentWeakness();
					bCustomWeakness = pCharacter->HasCustomWeakness();
				}
			}
			bMetal = this->pCurrentGame->IsSwordMetal(st.sword);
			bBeamBlock = this->pCurrentGame->equipmentBlocksGaze(ScriptFlag::Weapon);
			bBriar = (st.sword == BriarSword);
			bLuckyGR = this->pCurrentGame->IsLuckyGRItem(ScriptFlag::Weapon);
			bLuckyXP = this->pCurrentGame->IsLuckyXPItem(ScriptFlag::Weapon);
			bGoblinWeakness |= (st.sword == GoblinSword);
			bSerpentWeakness |= (st.sword == SerpentSword);
		break;
		case CMD_USE_ARMOR:
			if (this->pCurrentGame->IsPlayerShieldDisabled())
				break;

			def = this->pCurrentGame->getShieldPower(st.shield);
			if (bIsCustomEquipment(st.shield))
			{
				pCharacter = this->pCurrentGame->getCustomEquipment(ScriptFlag::Armor);
				if (pCharacter)
					atk = (int)pCharacter->getATK();
			}
			bMetal = this->pCurrentGame->IsShieldMetal(st.shield);
			bBeamBlock = this->pCurrentGame->equipmentBlocksGaze(ScriptFlag::Armor);
			bLuckyGR = this->pCurrentGame->IsLuckyGRItem(ScriptFlag::Armor);
			bLuckyXP = this->pCurrentGame->IsLuckyXPItem(ScriptFlag::Armor);
		break;
		case CMD_USE_ACCESSORY:
			if (this->pCurrentGame->IsPlayerAccessoryDisabled())
				break;

			if (bIsCustomEquipment(st.accessory))
			{
				pCharacter = this->pCurrentGame->getCustomEquipment(ScriptFlag::Accessory);
				if (pCharacter)
				{
					atk = (int)pCharacter->getATK();
					def = (int)pCharacter->getDEF();
					bMetal = pCharacter->IsMetal();
					bBeamBlock = pCharacter->HasRayBlocking();
					bGoblinWeakness = pCharacter->HasGoblinWeakness();
					bSerpentWeakness = pCharacter->HasSerpentWeakness();
					bCustomWeakness = pCharacter->HasCustomWeakness();
				}
			}
			bLuckyGR = this->pCurrentGame->IsLuckyGRItem(ScriptFlag::Accessory);
			bLuckyXP = this->pCurrentGame->IsLuckyXPItem(ScriptFlag::Accessory);
		break;
	}
	//Properties available to custom equipment.
	if (pCharacter)
	{
		bBriar |= pCharacter->CanCutBriar();
		bAttackFirst |= pCharacter->CanAttackFirst();
		bAttackLast |= pCharacter->CanAttackLast();
		bBackstab |= pCharacter->TurnToFacePlayerWhenFighting();
		bNoEnemyDEF |= pCharacter->HasNoEnemyDefense();
	}

	//Format as text.
	WCHAR temp[16];
	WSTRING text;
	bool bNeedCR = false;
	if (atk != 0)
	{
		if (atk > 0)
			text += wszPlus;
		text += _itoW(atk, temp, 10);
		text += wszSpace;
		text += g_pTheDB->GetMessageText(MID_ATKStat);
		bNeedCR = true;
	}
	if (def != 0)
	{
		if (bNeedCR)
			text += wszCRLF;
		if (def > 0)
			text += wszPlus;
		text += _itoW(def, temp, 10);
		text += wszSpace;
		text += g_pTheDB->GetMessageText(MID_DEFStat);
		bNeedCR = true;
	}
	if (bMetal)
	{
		if (bNeedCR)
			text += wszCRLF;
		text += g_pTheDB->GetMessageText(MID_BehaviorMetal);
		bNeedCR = true;
	}
	if (bGoblinWeakness)
	{
		if (bNeedCR)
			text += wszCRLF;
		text += g_pTheDB->GetMessageText(MID_BehaviorGoblinWeakness);
		bNeedCR = true;
	}
	if (bSerpentWeakness)
	{
		if (bNeedCR)
			text += wszCRLF;
		text += g_pTheDB->GetMessageText(MID_BehaviorSerpentWeakness);
		bNeedCR = true;
	}
	if (bCustomWeakness)
	{
		if (bNeedCR)
			text += wszCRLF;
		text += WCSReplace(
			g_pTheDB->GetMessageText(MID_StrongAgainstAspect),
			wszStringToken,
			pCharacter->GetCustomWeakness()
		);
		bNeedCR = true;
	}
	if (bBeamBlock)
	{
		if (bNeedCR)
			text += wszCRLF;
		text += g_pTheDB->GetMessageText(MID_BehaviorBeamBlock);
		bNeedCR = true;
	}
	if (bBriar)
	{
		if (bNeedCR)
			text += wszCRLF;
		text += g_pTheDB->GetMessageText(MID_BehaviorBriarCut);
		bNeedCR = true;
	}
	if (bLuckyGR)
	{
		if (bNeedCR)
			text += wszCRLF;
		text += g_pTheDB->GetMessageText(MID_BehaviorLuckyGR);
		bNeedCR = true;
	}
	if (bLuckyXP)
	{
		if (bNeedCR)
			text += wszCRLF;
		text += g_pTheDB->GetMessageText(MID_DoubleXP);
		bNeedCR = true;
	}
	if (bAttackFirst)
	{
		if (bNeedCR)
			text += wszCRLF;
		text += g_pTheDB->GetMessageText(MID_AttackFirst);
		bNeedCR = true;
	}
	if (bAttackLast)
	{
		if (bNeedCR)
			text += wszCRLF;
		text += g_pTheDB->GetMessageText(MID_AttackLast);
		bNeedCR = true;
	}
	if (bBackstab)
	{
		if (bNeedCR)
			text += wszCRLF;
		text += g_pTheDB->GetMessageText(MID_BehaviorSurprisedBehind);
		bNeedCR = true;
	}
	if (bNoEnemyDEF)
	{
		if (bNeedCR)
			text += wszCRLF;
		text += g_pTheDB->GetMessageText(MID_NoEnemyDefense);
		bNeedCR = true;
	}

	return text;
}

//*****************************************************************************
MESSAGE_ID CGameScreen::GetHintTextForRegion(int nX, int nY)
//Returns: MID describing the information on a screen region
{
	static const int xStat = 16;
	static const int x2Stat = xStat + 112;

	if (nX >= xStat && nX <= x2Stat)
	{
		//Player stats.
		static const int yStat[7] = {202, 236, 264, 291, 325, 352, 400};

		if (IsInRect(nX, nY, xStat, yStat[0], x2Stat, yStat[1]))
			return MID_HPTooltip;
		if (IsInRect(nX, nY, xStat, yStat[1], x2Stat, yStat[2]))
			return MID_ATKTooltip;
		if (IsInRect(nX, nY, xStat, yStat[2], x2Stat, yStat[3]))
			return MID_DEFTooltip;
		if (IsInRect(nX, nY, xStat, yStat[3], x2Stat, yStat[4]))
			return MID_GRTooltip;
		if (IsInRect(nX, nY, xStat, yStat[4], x2Stat, yStat[5]))
			return MID_REPTooltip;
		if (IsInRect(nX, nY, xStat, yStat[5], x2Stat, yStat[6]))
			return MID_KeysTooltip;

		//Monster stats.
		static const int y2Stat[4] = {450, 482, 512, 542};

		if (IsInRect(nX, nY, xStat, y2Stat[0], x2Stat, y2Stat[1]))
			return MID_MonHPTooltip;
		if (IsInRect(nX, nY, xStat, y2Stat[1], x2Stat, y2Stat[2]))
			return MID_MonATKTooltip;
		if (IsInRect(nX, nY, xStat, y2Stat[2], x2Stat, y2Stat[3]))
			return MID_MonDEFTooltip;
	}

	return 0;
}

//*****************************************************************************
void CGameScreen::GotoHelpPage()
{
	CBrowserScreen::SetPageToLoad("quickstart.html");
	GoToScreen(SCR_Browser);
}

//*****************************************************************************
void CGameScreen::LoadGame()
//Transition to the restore screen to select a saved game to load.
{
	if (this->bPlayTesting)
		return;

	this->pCurrentGame->SaveToContinue();
	GoToScreen(SCR_Restore);
/*
	//Import/load a player save file from disk and continue playing from that game state.

	CIDSet importedSavedGameIDs;
	set<WSTRING> importedStyles;
	Import(EXT_SAVE, importedSavedGameIDs, importedStyles, true);
	if (CDbXML::WasImportSuccessful() && !importedSavedGameIDs.empty())
	{
		//Successful import of a saved game removes the current game's save slot from the DB.
		//Now, start playing the imported saved game.
		if (LoadSavedGame(importedSavedGameIDs.getFirst()))
		{
			this->pMapWidget->LoadFromCurrentGame(this->pCurrentGame);
			this->pRoomWidget->LoadFromCurrentGame(this->pCurrentGame);
			this->pMapWidget->RequestPaint();
			Paint();
		}
	}
	ASSERT(importedStyles.empty());
*/
}

//******************************************************************************
void CGameScreen::LogHoldVars()
//Output the current state of all hold vars to clipboard and a text file.
{
	ASSERT(this->pCurrentGame);

	WSTRING wstrPos = (const WCHAR *)this->pCurrentGame->pLevel->NameText;
	wstrPos += wszColon;
	this->pCurrentGame->pRoom->GetLevelPositionDescription(wstrPos, true);
	wstrPos += wszColon;
	string strPos = UnicodeToUTF8(wstrPos);

	string str = "Game vars ";
	str += strPos;

	char temp[16];
	bool bFirst = true;
	for (UNPACKEDVAR *pVar = this->pCurrentGame->stats.GetFirst();
			pVar != NULL; pVar = this->pCurrentGame->stats.GetNext())
	{
		//Skip non-vars (i.e. not of format "v<varID>".
		if (pVar->name[0] != 'v')
			continue;

		if (bFirst)
		{
			str += NEWLINE;
			bFirst = false;
		}
		const UINT wVarID = atoi(pVar->name.c_str() + 1); //skip the "v"
		str += UnicodeToUTF8(this->pCurrentGame->pHold->GetVarName(wVarID));
		str += ": ";
		const bool bInteger = pVar->eType == UVT_int;
		if (bInteger)
		{
			const int nVal = this->pCurrentGame->stats.GetVar(pVar->name.c_str(), (int)0);
			str += _itoa(nVal, temp, 10);
		} else {
			const WSTRING wstr = this->pCurrentGame->stats.GetVar(pVar->name.c_str(), wszEmpty);
			str += UnicodeToUTF8(wstr);
		}
		str += NEWLINE;
	}
	if (bFirst) //no vars were encountered
		str += " None set" NEWLINE;
	CFiles f;
	f.AppendUserLog(str.c_str());

	CClipboard::SetString(str);
}

//*****************************************************************************
void CGameScreen::OnWindowEvent(const SDL_WindowEvent &wevent)
{
	CEventHandlerWidget::OnWindowEvent(wevent);

	if (this->pCurrentGame)
	{
		this->pCurrentGame->UpdateTime(SDL_GetTicks()); //add time until app activity change

		switch (wevent.event)
		{
			case SDL_WINDOWEVENT_FOCUS_LOST:
				//When app is minimized or w/o focus, pause game time and speech.
				this->pCurrentGame->UpdateTime();
				if (!this->dwTimeMinimized)
					this->dwTimeMinimized = SDL_GetTicks();
				break;

			case SDL_WINDOWEVENT_FOCUS_GAINED:
				//Unpause speech.
				if (this->dwTimeMinimized)
				{
					if (this->dwNextSpeech)
						this->dwNextSpeech += SDL_GetTicks() - this->dwTimeMinimized;
					this->dwTimeMinimized = 0;
				}
				break;

			default: break;
		}
	}
}

//*****************************************************************************
void CGameScreen::OnWindowEvent_GetFocus()
{
	CEventHandlerWidget::OnWindowEvent_GetFocus();

	if (this->dwTimeMinimized)
	{
		if (this->dwNextSpeech)
			this->dwNextSpeech += SDL_GetTicks() - this->dwTimeMinimized;
		this->dwTimeMinimized = 0;
	}
}

//*****************************************************************************
void CGameScreen::OnWindowEvent_LoseFocus()
{
	CEventHandlerWidget::OnWindowEvent_LoseFocus();

	this->pCurrentGame->UpdateTime();
	if (!this->dwTimeMinimized)
		this->dwTimeMinimized = SDL_GetTicks();

	if (this->bIsDialogDisplayed)
		g_pTheSound->PauseSounds();
}

//*****************************************************************************
void CGameScreen::OnBetweenEvents()
//Called between frames.
{
	UploadDemoPolling();

	UpdateEffectsFreeze();

	if (this->bShowingBigMap || this->bShowingTempRoom)
		return;

	if (this->bEnableChat)
		ChatPolling(TAG_CHATUSERS);

	//Continue displaying animated room and new chat messages received
	//while the chat/status dialog is visible (i.e. typing a chat message, etc).
	if (GetScreenType() == SCR_Game)
	{
		CDialogWidget *pChatBox = DYN_CAST(CDialogWidget*, CWidget*,
				this->pRoomWidget->GetWidget(TAG_CHATBOX));
		if (pChatBox->IsVisible())
		{
			this->pRoomWidget->Paint(false);
			pChatBox->Paint(false);
			this->pRoomWidget->UpdateRect();
			return; //everything below is paused while entering a chat message
		}

		//Enable Undo button only when valid to undo.
		CWidget *pWidget = GetWidget(TAG_UNDO);
		if (pWidget->IsVisible())
		{
			bool bEnable = this->pCurrentGame && this->pCurrentGame->wTurnNo > 0;
			if (bEnable != pWidget->IsEnabled())
			{
				pWidget->Enable(bEnable);
				pWidget->Paint();
			}
		}

		//Enable Command button when operation is available and allowed.
		pWidget = GetWidget(TAG_USECOMMAND);
		if (pWidget->IsVisible())
		{
			bool bEnable = this->pCurrentGame &&
					this->pCurrentGame->QueryInventoryStatus(ScriptFlag::Command) &&
					this->pCurrentGame->getCustomEquipment(ScriptFlag::Command) != NULL;
			if (bEnable != pWidget->IsEnabled())
			{
				pWidget->Enable(bEnable);
				pWidget->Paint();
			}
		}
	}

	if (this->dwTimeMinimized)
		return; //don't do the rest while minimized

	if (this->pCurrentGame)
	{
		//Keep sign synched with current game state.
		if ((this->pCurrentGame->dwCutScene != 0) != this->bShowingCutScene)
		{
			this->bShowingCutScene = (this->pCurrentGame->dwCutScene != 0);
			UpdateSign();
		}

		//Combat must be resolved before any questions can be answered.
		if (this->pCurrentGame->InCombat())
		{
			//Player can't be quick-exiting room when in combat.
			this->wRoomQuickExitDirection = NO_ORIENTATION;

			//Is it time to advance combat?
			const Uint32 dwNow = SDL_GetTicks();
			if (!this->dwLastCombatTick)
				this->dwLastCombatTick = dwNow;
			if (dwNow - this->dwLastCombatTick >= this->wThisCombatTickSpeed)
			{
				if (!ProcessCommandWrapper(CMD_ADVANCE_COMBAT))
					return;
				this->dwLastCombatTick = dwNow;

				//If the user manually sped up this combat to instantaneous resolution,
				//now that it's over, revert to slow mode.
				if (!this->wThisCombatTickSpeed && this->wCombatTickSpeed)
					this->pCurrentGame->bQuickCombat = false;
			}
		}
		//Handle question prompts that were delayed while player did not have control.
		else if (this->bNeedToProcessDelayedQuestions)
		{
			SCREENTYPE eNextScreen = SCR_Game;
			ProcessQuestionPrompts(sCueEvents, eNextScreen);
			if (eNextScreen != SCR_Game)
			{
				if (IsDeactivating())
					SetDestScreenType(eNextScreen); //override any other specified destination screen
				else
					GoToScreen(eNextScreen);
			}
			return;
		} else {
			//No combat and no questions.
			this->wThisCombatTickSpeed = this->wCombatTickSpeed; //default preference

			if (!this->pCurrentGame->dwCutScene)
			{
				this->dwLastCutSceneMove = 0;
				//Return per-move duration to what it was before being altered.
				if (this->dwSavedMoveDuration && this->wRoomQuickExitDirection == NO_ORIENTATION)
				{
					this->pRoomWidget->SetMoveDuration(this->dwSavedMoveDuration);
					this->dwSavedMoveDuration = 0;
				}

				//If player is quick-exiting room, take periodic steps along route.
				if (this->wRoomQuickExitDirection != NO_ORIENTATION)
					TakeStepTowardQuickExit();
			} else if (GetScreenType() == SCR_Game) { //don't automatically advance turns during demo playback
				//Cut scene is active -- halt any mapped movements.
				this->wRoomQuickExitDirection = NO_ORIENTATION;
				this->wMoveDestX = this->wMoveDestY = NO_DESTINATION;

				//Turns pass automatically when a cut scene is playing.

				//Is it time to advance the cut scene by a game turn?
				const Uint32 dwNow = SDL_GetTicks();
				if (!this->dwLastCutSceneMove)
					this->dwLastCutSceneMove = dwNow;
				if (dwNow - this->dwLastCutSceneMove >= this->pCurrentGame->dwCutScene)
				{
					//Save per-move duration before cut scene began.
					if (!this->dwSavedMoveDuration)
					{
						this->dwSavedMoveDuration = this->pRoomWidget->GetMoveDuration();
						this->pRoomWidget->FinishMoveAnimation(); //finish last move now
					}
					//Set per-move duration so cut scene moves advance smoothly.
					this->pRoomWidget->SetMoveDuration(this->pCurrentGame->dwCutScene);

					//Advance the cut scene another turn.
					if (!ProcessCommandWrapper(CMD_ADVANCE_CUTSCENE))
						return;
					this->dwLastCutSceneMove = dwNow;
				}
			}
		}

		//Move player toward on-screen location specified by mouse click.
		if (this->pCurrentGame && !this->pCurrentGame->dwCutScene &&
				!this->bShowingBigMap && !this->bShowingTempRoom)
		{
			int nX, nY;
			const Uint8 buttons = GetMouseState(&nX, &nY);
			if (buttons == SDL_BUTTON_LMASK && !this->bDisableMouseMovement && !this->bNoMoveByCurrentMouseClick)
				MovePlayerToward(nX, nY);
			else if (!buttons)
			{
				//Tool tips when hovering over stat labels on sidebar.
				if (IsCursorVisible())
				{
					MESSAGE_ID mid = this->bIsScrollVisible ? 0 : GetHintTextForRegion(nX, nY);

					//Tool tip for minimap, if allowed.
					if (!mid && this->pMapWidget->ContainsCoords(nX, nY) && this->pCurrentGame)
					{
						UINT roomX, roomY;
						this->pMapWidget->GetRoomAtCoords(nX, nY, roomX, roomY);
						const UINT roomID = this->pCurrentGame->pLevel->GetRoomIDAtCoords(roomX, roomY);
						if (roomID)
						{
							CDbRoom *pRoom = NULL;
							bool bCurrentRoom = false;
							if (this->pCurrentGame->pRoom->dwRoomID == roomID)
							{
								pRoom = this->pCurrentGame->pRoom;
								bCurrentRoom = true;
							}
							if (!pRoom)
							{
								ExploredRoom *pExpRoom = this->pCurrentGame->getExploredRoom(roomID);
								if (pExpRoom && !pExpRoom->bMapOnly)
								{
									//Determine room state.
									//!!This might be a time-intensive operation, and could be optimized.
									pRoom = g_pTheDB->Rooms.GetByID(roomID);
									pRoom->SetMembersFromExploredRoomData(pExpRoom);
								}
							}

							if (pRoom)
							{
								//Minimap room color-coding information.
								if (pRoom->HasGrabbableItems())
									mid = MID_RoomHasTreasure;
								else if (pRoom->HasCombatableMonsters())
									mid = MID_RoomHasEnemies;
								else if (pRoom->HasClosedDoors())
									mid = MID_RoomHasClosedDoors;

								if (!bCurrentRoom)
									delete pRoom;
							}
						}
					}

					if (mid)
						RequestToolTip(mid);
					else
					{
						//Tool tip describing the equipment item hovered over.
						WSTRING text;
						if (IsInRect(nX, nY, X_PIC[4], Y_PIC[4], rightEndOfEquipmentSlot, Y_PIC[4] + g_pTheDBM->CY_TILE))
							text = GetEquipmentPropertiesText(CMD_USE_WEAPON);
						else if (IsInRect(nX, nY, X_PIC[5], Y_PIC[5], rightEndOfEquipmentSlot, Y_PIC[5] + g_pTheDBM->CY_TILE))
							text = GetEquipmentPropertiesText(CMD_USE_ARMOR);
						else if (IsInRect(nX, nY, X_PIC[6], Y_PIC[6], rightEndOfEquipmentSlot, Y_PIC[6] + g_pTheDBM->CY_TILE))
							text = GetEquipmentPropertiesText(CMD_USE_ACCESSORY);
						if (!text.empty())
							CScreen::RequestToolTip(text.c_str());
					}
				}
			}

			//Whenever a destination tile has been specified,
			//move the player toward that location.
			if (!this->pRoomWidget->IsMoveAnimating())
				MovePlayerTowardDestTile();
		}

		//Time to play a thunder sound effect?
		if (!this->pRoomWidget->playThunder.empty())
		{
			if (SDL_GetTicks() >= this->pRoomWidget->playThunder.front())
			{
				this->pRoomWidget->playThunder.pop();
				g_pTheSound->PlaySoundEffect(SEID_THUNDER, NULL, NULL, false, 1.0f + fRAND_MID(0.2f));
			}
		}
	}

	CRoomScreen::OnBetweenEvents();
	ProcessSpeech();
}

//*****************************************************************************
void CGameScreen::OnClick(
//Called when widget receives a mouse click event.
//
//Params:
	const UINT dwTagNo) //(in) Widget that event applied to.
{
	ShowCursor();

	//Widgets that may be clicked on whether or not the level map is being shown.
	switch (dwTagNo)
	{
		case TAG_MAP:
		{
			if (!this->pCurrentGame)
				break;
			const UINT roomID = this->pCurrentGame->pLevel->GetRoomIDAtCoords(
				this->pMapWidget->dwClickedRoomX, this->pMapWidget->dwClickedRoomY);
			if (!roomID)
			{
				ToggleBigMap();
				break;
			}
			if (RightMouseButton())
			{
				//Mark room.
				const UINT mark = MarkMapRoom(roomID);
				if (mark != MAP_NOT_MARKED)
				{
					this->pMapWidget->RefreshRoom(roomID, mark);
					this->pMapWidget->RequestPaint();
					if (this->bShowingBigMap)
					{
						//Also refresh level map with this information.
						this->pBigMapWidget->RefreshRoom(roomID, mark);
						this->pBigMapWidget->RequestPaint();
					}
				}
			} else {
				//When current room is clicked on the minimap, pop-up or hide the big level map.
				if (roomID == this->pCurrentGame->pRoom->dwRoomID)
				{
					ToggleBigMap();
				} else {
					//Show the room clicked on immediately.  No pop-up map after.

					//Don't change display in the middle of an action.
					if (!this->pCurrentGame->InCombat() && !this->bNeedToProcessDelayedQuestions)
					{
						ExploredRoom *pExpRoom = this->pCurrentGame->getExploredRoom(roomID);
						if (pExpRoom && !pExpRoom->bMapOnly)
							ShowRoomTemporarily(roomID);
						else ToggleBigMap();
					}
				}
			}
		}
		break;
	}

	if (this->bShowingBigMap)
	{
		switch (dwTagNo)
		{
			case TAG_BIGMAP: //show room at this location
			{
				//Show view of the clicked room.
				const UINT roomID = this->pCurrentGame->pLevel->GetRoomIDAtCoords(
					this->pBigMapWidget->dwClickedRoomX, this->pBigMapWidget->dwClickedRoomY);
				if (!roomID)
				{
					//No room to show.
					ToggleBigMap();
					break;
				}
				if (RightMouseButton())
				{
					const UINT mark = MarkMapRoom(roomID);
					if (mark != MAP_NOT_MARKED)
					{
						this->pMapWidget->RefreshRoom(roomID, mark);
						this->pMapWidget->RequestPaint();
						this->pBigMapWidget->RefreshRoom(roomID, mark);
						this->pBigMapWidget->RequestPaint();
					}
					break;
				}
				if (roomID == this->pCurrentGame->pRoom->dwRoomID)
				{
					//Clicking on current room just returns to normal play.
					ToggleBigMap();
				} else {
					ExploredRoom *pExpRoom = this->pCurrentGame->getExploredRoom(roomID);
					if (pExpRoom && !pExpRoom->bMapOnly)
						ShowRoomTemporarily(roomID);
					else ToggleBigMap(); //no room to show -- close map
				}
			}
			break;
			case TAG_MAP:
				//Was handled above.
			break;
			case TAG_BIGMAPCONTAINER:
				//Don't hide map when interacting with map widgets.
			break;
			default: //clicking elsewhere returns to normal play
				ToggleBigMap();
			break;
		}

		//Don't process any other input while the map is being shown.
		return;
	}

	//Clicks that are processed whenever the level map is not being displayed.
	switch (dwTagNo)
	{
		//Command buttons.
		case TAG_ESC:
			//Esc -- menu
			GoToScreen(SCR_Return);
		break;

		case TAG_HELP:
			//F1 -- help
			GotoHelpPage();
		break;
		case TAG_UNDO:
			this->wUndoToTurn = this->pCurrentGame->wTurnNo > MOVES_TO_UNDO_BY_BUTTON ?
					this->pCurrentGame->wTurnNo - MOVES_TO_UNDO_BY_BUTTON : 0;
			UndoMove();
		break;

		case TAG_LOAD:
			LoadGame();
		break;
		case TAG_SAVE:
			SaveGame();
		break;

		case TAG_ROT_CCW:
			ProcessCommandWrapper(CMD_CC);
		return;
		case TAG_ROT_CW:
			ProcessCommandWrapper(CMD_C);
		return;
		case TAG_WAIT:
			ProcessCommandWrapper(CMD_WAIT);
		return;
		case TAG_LOCK:
			ProcessCommandWrapper(CMD_LOCK);
		return;
		case TAG_USECOMMAND:
			ProcessCommandWrapper(CMD_EXEC_COMMAND);
		return;

		//Activating an inventory item.
		case TAG_SWORD:
			ClickOnEquipment(CMD_USE_WEAPON, RightMouseButton());
		return;
		case TAG_SHIELD:
			ClickOnEquipment(CMD_USE_ARMOR, RightMouseButton());
		return;
		case TAG_ACCESSORY:
			ClickOnEquipment(CMD_USE_ACCESSORY, RightMouseButton());
		return;

		case TAG_SIGN_AREA:
			//When custom room location text is displayed,
			//clicking on it will display a tooltip with the standard room location text.
			if (this->pCurrentGame && !this->pCurrentGame->customRoomLocationText.empty())
			{
				ASSERT(this->pCurrentGame->pRoom);

				WSTRING wstrSignText = (const WCHAR*)this->pCurrentGame->pLevel->NameText;
				wstrSignText += wszColon;
				wstrSignText += wszSpace;
				this->pCurrentGame->pRoom->GetLevelPositionDescription(wstrSignText);

				ShowToolTip(wstrSignText.c_str());
			}
			break;

		default: break;
	}
}

//*****************************************************************************
void CGameScreen::OnDeactivate()
{
	g_pTheDBM->fLightLevel = 1.0; //full light level
	this->dwTimeMinimized = 0;
	g_pTheSound->StopAllSoundEffects(); //stop any game sounds that were playing
	this->wMoveDestX = this->wMoveDestY = NO_DESTINATION;

	//Reset any simulated combat, as pointers might be left dangling.
	delete g_pPredictedCombat;
	g_pPredictedCombat = NULL;

	if (this->pCurrentGame && GetScreenType() == SCR_Game)
	{
		this->pCurrentGame->UpdateTime();   //stop timing game play

		//Save updated persistent player info.
		CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
		CDbPackedVars& vars = pCurrentPlayer->Settings;

		//Save chat preference.
		vars.SetVar(Settings::EnableChatInGame, this->bEnableChat);
		vars.SetVar(Settings::ReceiveWhispersOnlyInGame, this->bReceiveWhispersOnly);

		pCurrentPlayer->Update();

		//Save to the continue slot whenever leaving the game screen.
		if (!this->bPlayTesting && this->pCurrentGame->pHold->status != CDbHold::Tutorial)
		{
/*
			//Stop any demo recording.
			if (this->pCurrentGame->IsDemoRecording() &&
					(GetDestScreenType() == SCR_Return || GetDestScreenType() == SCR_None))
				this->pCurrentGame->EndDemoRecording();
*/
			this->pCurrentGame->SaveToContinue();
			g_pTheDB->Commit();
		}

		delete pCurrentPlayer;

		//Ensure all internet upload requests have been sent.
		WaitToUploadDemos();
	}

	UploadExploredRooms();

	CRoomScreen::OnDeactivate();
}

//*****************************************************************************
void CGameScreen::OnDoubleClick(
//Called when widget receives a mouse double click event.
//
//Params:
	const UINT dwTagNo)   //(in) Widget that event applied to.
{
	switch (dwTagNo)
	{
		case TAG_FACE:
			ShowSpeechLog();
		break;
		default: break;
	}
}

//*****************************************************************************
void CGameScreen::OnKeyDown(
//Called when widget receives SDL_KEYDOWN event.
//
//Params:
	const UINT dwTagNo,       //(in)   Widget that event applied to.
	const SDL_KeyboardEvent &Key) //(in)   Key event.
{
	if (this->bShowingBigMap)
	{
		ToggleBigMap();
		return;
	}

	CScreen::OnKeyDown(dwTagNo, Key);

	//Check for a game command.
	int nCommand = GetCommandForKeysym(Key.keysym.sym);
	if (nCommand != CMD_UNSPECIFIED)
	{
		//Hide mouse cursor while playing.
		HideCursor();

		const bool bCtrl = (Key.keysym.mod & KMOD_CTRL) != 0;
		const bool bShift = (Key.keysym.mod & KMOD_SHIFT) != 0;

		//When Ctrl-moveCommand is pressed and no cut scene is playing,
		//perform a quick backtracking to the next room in that direction.
		if (bCtrl && !this->pCurrentGame->dwCutScene && (bIsMovementCommand(nCommand) || nCommand == CMD_WAIT))
		{
			switch (nCommand)
			{
				case CMD_N: SearchForPathToNextRoom(N); break;
				case CMD_S: SearchForPathToNextRoom(S); break;
				case CMD_E: SearchForPathToNextRoom(E); break;
				case CMD_W: SearchForPathToNextRoom(W); break;
				case CMD_WAIT: SearchForPathToNextRoom(GO_TO_STAIRS); break;
				default: break; //no other Ctrl-move is recognized
			}
		} else {
			//Only allow inputting player movement commands when no cutscene is playing.
			if (!this->pCurrentGame->dwCutScene || nCommand >= CMD_WAIT)
			{
				//Ctrl and Shift key modifiers use weapon and armor instead of accessory.
				if (nCommand == CMD_USE_ACCESSORY)
				{
					if (bShift)
						nCommand = CMD_USE_WEAPON;
					else if (bCtrl)
						nCommand = CMD_USE_ARMOR;
				}

				if (!ProcessCommandWrapper(nCommand))
					return;

				//If a mouse-inputted or quick backtracking path
				//has been calculated and is in process of execution,
				//any manually-inputted commands will terminate it.
				this->wMoveDestX = this->wMoveDestY = NO_DESTINATION;
				this->pCurrentGame->pPlayer->ResetMyPathToGoal();
			}
		}
	}

	//Check for other keys.
	switch (Key.keysym.sym)
	{
		//Help screen.
		case SDLK_F1:
			GotoHelpPage();
		break;

		//Save game.
		case SDLK_F2:
			SaveGame();
		break;

		//Load game.
		case SDLK_F3:
			LoadGame();
		break;

		//Global var editor.
		case SDLK_F4:
			if (!this->bShowingBigMap && !this->bShowingTempRoom && GetScreenType() == SCR_Game)
			{
				if (!this->pCurrentGame->InCombat() && !this->pCurrentGame->dwCutScene && //don't allow altering vars during self-advancing play segments
						this->pCurrentGame->GetUnansweredQuestion() == NULL) //or when constrained to a specific type of response
				{
#ifndef ENABLE_CHEATS
					if (this->bPlayTesting)
#endif
						EditGlobalVars(this->pSpeechBox, NULL, this->pCurrentGame);
				}
			}
		break;

/*
		case SDLK_F4:
#if defined(__linux__) || defined(__FreeBSD__)
		case SDLK_PAUSE:
#endif
		if ((Key.keysym.mod & (KMOD_CTRL | KMOD_ALT)) == 0)
		{
			if (this->bPlayTesting) break;
			if (!this->pCurrentGame->wTurnNo) break;
			//Save a demo for the current room from entrance up to the present turn.
			WSTRING wstrDescription = this->pCurrentGame->AbbrevRoomLocation();
			const UINT dwTagNo = ShowTextInputMessage(MID_DescribeDemo,
					wstrDescription);
			if (dwTagNo == TAG_OK)
			{
				this->pCurrentGame->BeginDemoRecording( (wstrDescription.size()==0) ?
						wszEmpty : wstrDescription.c_str(), false);
				ShowOkMessage(this->pCurrentGame->EndDemoRecording() ?
						MID_DemoSaved : MID_DemoNotSaved);
				PaintSign();
			}
		}
		break;
*/

		case SDLK_F6:
			if (GetScreenType() == SCR_Game && !this->bPlayTesting) {
				GoToScreen(SCR_Settings);
			} else {
				g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);
			}
		return;

		//Quick save/load.
		case SDLK_F5:
			if (!this->bPlayTesting)
			{
				this->pCurrentGame->QuickSave();
				CTextEffect *pTextEffect = new CTextEffect(this->pRoomWidget,
						g_pTheDB->GetMessageText(MID_QuickSaveCompleted), F_Stats, 500, 1000, false, true);
				pTextEffect->Move(5, 5);
				this->pRoomWidget->AddLastLayerEffect(pTextEffect);
			}
		break;
		case SDLK_F9:
			if (!this->bPlayTesting)
			{
				const UINT quicksaveID = g_pTheDB->SavedGames.FindByContinue(ST_Quicksave);
				if (!quicksaveID)
				{
					ShowOkMessage(MID_QuickSave_NoneAvailable);
					break;
				}
				if (ShowYesNoMessage(MID_QuickLoadQuestion) == TAG_YES)
				{
					SetCursor(CUR_Wait);
					if (LoadQuicksave())
					{
						this->pMapWidget->LoadFromCurrentGame(this->pCurrentGame);
						this->pRoomWidget->LoadFromCurrentGame(this->pCurrentGame);
						this->pMapWidget->RequestPaint();
						Paint();
					}
					SetCursor();
					HideCursor();
				}
			}
		break;

/*
		case SDLK_F5:
			if (this->bPlayTesting) break;
			if (this->pCurrentGame->IsDemoRecording())
			{
				//End recording and save demo.
				const UINT dwTagNo = this->pCurrentGame->EndDemoRecording();
				UpdateSign();

				if (!dwTagNo)
					ShowOkMessage(MID_DemoNotSaved);
				else if (dwTagNo != TAG_ESCAPE)
					ShowOkMessage(MID_DemoSaved);
			}
			else
			{
				WSTRING wstrDescription = this->pCurrentGame->AbbrevRoomLocation();
				const UINT dwTagNo = ShowTextInputMessage(MID_DescribeDemo,
						wstrDescription);
				if (dwTagNo == TAG_OK)
				{
					this->pCurrentGame->BeginDemoRecording( (wstrDescription.size()==0) ?
							wszEmpty : wstrDescription.c_str() );

					//Repaint sign to show new recording status.
					UpdateSign();
				}
			}
		break;
*/
/*
		//Room demos displayed will be of active room in current game.
		case SDLK_F6:
		{
			if (this->bPlayTesting) break;
			CScreen *pScreen = g_pTheSM->GetScreen(SCR_Demos);
			if (!pScreen)
			{
				ShowOkMessage(MID_CouldNotLoadResources);
				break;
			}
			CDemosScreen *pDemosScreen = DYN_CAST(CDemosScreen*, CScreen*, pScreen);
			ASSERT(pDemosScreen);

			ASSERT(this->pCurrentGame);
			ASSERT(this->pCurrentGame->pRoom);
			pDemosScreen->ShowRoom(this->pCurrentGame->pRoom->dwRoomID);
			GoToScreen(SCR_Demos);
		}
		break;
*/

		//Room screenshot.
		case SDLK_F11:
		if (Key.keysym.mod & KMOD_CTRL)
		{
			SDL_Surface *pRoomSurface = SDL_CreateRGBSurface(
					SDL_SWSURFACE, this->pRoomWidget->GetW(), this->pRoomWidget->GetH(),
					g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0);
			if (!pRoomSurface) break;
			SDL_Rect screenRect = MAKE_SDL_RECT(this->pRoomWidget->GetX(), this->pRoomWidget->GetY(),
					this->pRoomWidget->GetW(), this->pRoomWidget->GetH());
			SDL_Rect roomRect = MAKE_SDL_RECT(0, 0, this->pRoomWidget->GetW(), this->pRoomWidget->GetH());
			SDL_BlitSurface(GetDestSurface(), &screenRect, pRoomSurface, &roomRect);
			SaveSurface(pRoomSurface);
			SDL_FreeSurface(pRoomSurface);
		}
		break;

		//Show chat dialog.
		case SDLK_RETURN:
		case SDLK_KP_ENTER:
			if (!(Key.keysym.mod & (KMOD_ALT|KMOD_CTRL)))
			{
				ShowCursor();
				g_pTheSound->PlaySoundEffect(SEID_BUTTON);
				this->pCurrentGame->UpdateTime(SDL_GetTicks());  //make time current
//				DisplayRoomStats();
				DisplayChatDialog();
			} else if (Key.keysym.mod & (KMOD_CTRL)) {
				ShowChatHistory(this->pSpeechBox);
			}
		break;
/*
		//Toggle room lock.
		case SDLK_LSHIFT: case SDLK_RSHIFT:
			this->pCurrentGame->bRoomExitLocked = !this->pCurrentGame->bRoomExitLocked;
			g_pTheSound->PlaySoundEffect(this->pCurrentGame->bRoomExitLocked ?
					SEID_WISP : SEID_CHECKPOINT);
			ShowLockIcon(this->pCurrentGame->bRoomExitLocked);
		break;
*/

		//Skip cutscene/clear playing speech.
		case SDLK_SPACE:
			if (this->pCurrentGame && this->pCurrentGame->dwCutScene)
			{
				if (this->pCurrentGame->dwCutScene == 1)
				{
					//When speeding past a cutscene, space clears all speech and subtitles of any sort.
					ClearSpeech(true);
				} else {
					this->pCurrentGame->dwCutScene = 1; //run cutscene quickly to its conclusion
				}
			} else if (this->pCurrentGame && this->pCurrentGame->InCombat()) {
				//Speed up combat x2.
				this->wThisCombatTickSpeed /= 2;
				if (this->wThisCombatTickSpeed < 8) //speed up to instantaneous resolution
				{
					this->wThisCombatTickSpeed = 0;
					this->pCurrentGame->bQuickCombat = true;
				}
			} else {
				//Clear all speech and subtitles of any sort
				ClearSpeech(true);
				this->pRoomWidget->RemoveLastLayerEffectsOfType(ECHATTEXT);
			}
		break;

		//Force full style reload.
		case SDLK_F8:
			this->pRoomWidget->UpdateFromCurrentGame(true);
		break;
		//Persistent move count display / Frame rate / Game var output.
		case SDLK_F7:
			if (Key.keysym.mod & KMOD_SHIFT) {
				LogHoldVars();
			} else if (Key.keysym.mod & KMOD_CTRL) {
#ifndef ENABLE_CHEATS
				if (CanShowVarUpdates())
#endif
					this->pRoomWidget->ToggleVarDisplay();
			} else if (Key.keysym.mod & KMOD_ALT) {
				this->pRoomWidget->ToggleFrameRate();
			} else {
				this->pRoomWidget->ToggleMoveCount();
			}
		break;

		case SDLK_UP: //Up arrow.
			if (Key.keysym.mod & KMOD_CTRL)
				SearchForPathToNextRoom(N);
			else if (GetCommandForKeysym(Key.keysym.sym) == CMD_UNSPECIFIED)
				ProcessCommandWrapper(CMD_N);
		return;
		case SDLK_DOWN:   //Down arrow.
			if (Key.keysym.mod & KMOD_CTRL)
				SearchForPathToNextRoom(S);
			else if (GetCommandForKeysym(Key.keysym.sym) == CMD_UNSPECIFIED)
				ProcessCommandWrapper(CMD_S);
		return;
		case SDLK_LEFT: //Left arrow.
			if (Key.keysym.mod & KMOD_CTRL)
				SearchForPathToNextRoom(W);
			else if (GetCommandForKeysym(Key.keysym.sym) == CMD_UNSPECIFIED)
				ProcessCommandWrapper(CMD_W);
		return;
		case SDLK_RIGHT: //Right arrow.
			if (Key.keysym.mod & KMOD_CTRL)
				SearchForPathToNextRoom(E);
			else if (GetCommandForKeysym(Key.keysym.sym) == CMD_UNSPECIFIED)
				ProcessCommandWrapper(CMD_E);
		return;

		default: break;
	}
}

//*****************************************************************************
void CGameScreen::OnMouseDown(
	const UINT dwTagNo,
	const SDL_MouseButtonEvent &Button)
{
	this->bNoMoveByCurrentMouseClick = false; //receiving this event implies last mouse click ended

	if (this->bShowingBigMap || this->bShowingTempRoom)
		return;

	switch (dwTagNo)
	{
		//When clicking on widgets, don't process movement commands.
		case TAG_MAP:
		case TAG_ESC:
		case TAG_HELP:
		case TAG_UNDO:
		case TAG_LOAD:	case TAG_SAVE:
		case TAG_ROT_CW: case TAG_ROT_CCW:
		break;

		case TAG_SIGN_AREA:
			if (this->pCurrentGame && !this->pCurrentGame->customRoomLocationText.empty())
				this->bNoMoveByCurrentMouseClick = true;
		break;

		default:
			//Game commands.
			if (this->bDisableMouseMovement)
				break; //mouse command input disabled
			switch (Button.button)
			{
				//Attempt to move player towards clicked tile.
				case SDL_BUTTON_LEFT:
					MovePlayerToward(Button.x, Button.y);
				break;
				//Lock door stood upon.
				case SDL_BUTTON_MIDDLE:
					ProcessCommandWrapper(CMD_LOCK);
				return;
			}
		break;
	}
}

//*****************************************************************************
void CGameScreen::OnMouseMotion(
//
//Params:
	const UINT dwTagNo,
	const SDL_MouseMotionEvent &MotionEvent)
{
	if (this->bShowingBigMap || this->bShowingTempRoom)
		return;

	CDrodScreen::OnMouseMotion(dwTagNo, MotionEvent);
	this->pFaceWidget->MovePupils(MotionEvent.x, MotionEvent.y);
}

//*****************************************************************************
void CGameScreen::OnMouseUp(
//Mouse button is released.
//
//Params:
	const UINT dwTagNo, const SDL_MouseButtonEvent &Button)
{
	CDrodScreen::OnMouseUp(dwTagNo, Button);

	if (GetScreenType() == SCR_Game)
	{
		if (this->bIsScrollVisible)
		{
			//Clicking scroll dismisses it.
			if (IsInRect(Button.x, Button.y, 5, 185, 5 + 270, 185 + 380))
			{
				g_pTheSound->PlaySoundEffect(SEID_READ);
				HideScroll();
				Paint();
			}
		} else {
			if (!this->bShowingBigMap && !this->bShowingTempRoom)
			{
				//Clicked on an inventory item?
				if (IsInRect(Button.x, Button.y, X_PIC[4], Y_PIC[4], X_PIC[4] + g_pTheDBM->CX_TILE, Y_PIC[4] + g_pTheDBM->CY_TILE))
					ClickOnEquipment(CMD_USE_WEAPON, Button.button == SDL_BUTTON_RIGHT);
				else if (IsInRect(Button.x, Button.y, X_PIC[5], Y_PIC[5], X_PIC[5] + g_pTheDBM->CX_TILE, Y_PIC[5] + g_pTheDBM->CY_TILE))
					ClickOnEquipment(CMD_USE_ARMOR, Button.button == SDL_BUTTON_RIGHT);
				else if (IsInRect(Button.x, Button.y, X_PIC[6], Y_PIC[6], X_PIC[6] + g_pTheDBM->CX_TILE, Y_PIC[6] + g_pTheDBM->CY_TILE))
					ClickOnEquipment(CMD_USE_ACCESSORY, Button.button == SDL_BUTTON_RIGHT);
			}

			//Texts describing stat labels on sidebar.
			MESSAGE_ID mid = GetHintTextForRegion(Button.x, Button.y);
			if (mid)
			{
				RemoveToolTip();
				ShowToolTip(g_pTheDB->GetMessageText(mid));
			}
		}
	}

	this->ignoreDestUntilMouseUp.set(NO_DESTINATION, NO_DESTINATION);
}

//*****************************************************************************
void CGameScreen::OnMouseWheel(
//Called when a mouse wheel event is received.
//
//Params:
	const SDL_MouseWheelEvent &Wheel)
{
	if (this->bShowingBigMap || this->bShowingTempRoom)
		return;

	if (!this->bDisableMouseMovement)
	{
		if (Wheel.y < 0)
			ProcessCommandWrapper(CMD_C);
		else if (Wheel.y > 0)
			ProcessCommandWrapper(CMD_CC);
	}
}

//*****************************************************************************
void CGameScreen::OnSelectChange(
//Handles a selection change.
//
//Params:
	const UINT dwTagNo) //(in) Widget event applies to.
{
	switch (dwTagNo)
	{
		case TAG_CHATINPUT:
			ReformatChatText(TAG_CHATINPUT, true);
		break;
	}
}

//*****************************************************************************
void CGameScreen::PlayHitObstacleSound(const UINT wAppearance, CCueEvents& CueEvents)
//Play sound when the player hits an obstacle.
{
	UINT eSoundID = SEID_NONE;
	switch (wAppearance)
	{
		case M_BEETHRO: case M_BEETHRO_IN_DISGUISE: eSoundID = SEID_OOF; break;
		case M_NEATHER:
		case M_HALPH: eSoundID = SEID_HALPH_OOF; break;
		case M_GOBLIN:	case M_GOBLINKING: eSoundID = SEID_GOB_OOF; break;
		case M_TARBABY: case M_MUDBABY: case M_GELBABY:
		case M_TARMOTHER: case M_MUDMOTHER: case M_GELMOTHER:
			eSoundID = SEID_TAR_OOF; break;
		case M_ROCKGOLEM: case M_ROCKGIANT: eSoundID = SEID_ROCK_OOF; break;
		case M_CLONE: case M_DECOY: case M_MIMIC: case M_GUARD: case M_PIRATE:
		case M_CITIZEN1: case M_CITIZEN2:
		case M_MUDCOORDINATOR: case M_TARTECHNICIAN:
		case M_CITIZEN: eSoundID = SEID_CIT_OOF; break;
		case M_STALWART: eSoundID = SEID_STALWART_OOF; break;
		case M_WUBBA: eSoundID = SEID_WUBBA; break;
		case M_NEGOTIATOR: case M_INSTRUCTOR:
		case M_CITIZEN3: case M_CITIZEN4:
			eSoundID = SEID_WOM_OOF; break;
		case M_SLAYER: eSoundID = SEID_SLAYER_OOF; break;
		default: eSoundID = SEID_MON_OOF; break;
	}
	if (eSoundID != (UINT)SEID_NONE &&
			!g_pTheSound->IsSoundEffectPlaying(eSoundID) &&
			!CueEvents.HasOccurred(CID_BumpedLockedDoor)) //don't play sound when a different sound provides feedback
		PlaySoundEffect(eSoundID);
}

//*****************************************************************************
void CGameScreen::PlaySoundEffect(
//Wrapper function to set the frequency multiplier for sound effects
//based on game context.
	const UINT eSEID, float* pos, float* vel, //default=[NULL,NULL]
	const bool bUseVoiceVolume)
{
	//Special contexts where sounds are played back differently for effect.
	float frequencyMultiplier = 1.0f;
	if (this->pCurrentGame)
	{
		ASSERT(this->pCurrentGame->pRoom);
		CSwordsman& player = *this->pCurrentGame->pPlayer;

		if (player.IsHasted())// || player.bFrozen)
			frequencyMultiplier = 0.8f;

		switch (eSEID)
		{
			case SEID_STABTAR:
				if (this->pCurrentGame->pRoom->bBetterVision)
					 frequencyMultiplier *= 1.25f;
			break;
			default: break;
		}
	}

	g_pTheSound->PlaySoundEffect(eSEID, pos, vel, bUseVoiceVolume, frequencyMultiplier);
}

//*****************************************************************************
void CGameScreen::SaveGame()
//Saves the current game state to a saved game record in the DB.
{
	if (this->bPlayTesting)
		return;
	if (this->pCurrentGame->Commands.IsFrozen() || !this->pCurrentGame->bIsGameActive)
		return;

	//Default save name: Level name + room loc.
	WSTRING wstrDescText = (const WCHAR*)this->pCurrentGame->pLevel->NameText;
	wstrDescText += wszColon;
	wstrDescText += wszSpace;
	WSTRING abbrevRoomPosition;
	this->pCurrentGame->pRoom->GetLevelPositionDescription(abbrevRoomPosition, true);
	wstrDescText += abbrevRoomPosition;

	const UINT dwTagNo = ShowTextInputMessage(MID_DescribeSave, wstrDescText);
	if (dwTagNo == TAG_OK)
	{
		this->pCurrentGame->SaveGame(ST_Manual, wstrDescText);
		CTextEffect *pTextEffect = new CTextEffect(this->pRoomWidget,
				g_pTheDB->GetMessageText(MID_SavedGamesSaved), F_Stats, 500, 1000, false, true);
		pTextEffect->Move(5, 5);
		this->pRoomWidget->AddLastLayerEffect(pTextEffect);
	}
}

//******************************************************************************
bool CGameScreen::SetForActivate()
//Called before screen is activated and first paint.
//
//Returns:
//True if activation should continue, false if not.
{
	//Caller didn't load a current game.
	if (!this->pCurrentGame) {ASSERT(!"Current game not set."); return false;}

	//Set current game for widgets.
	if (!this->pMapWidget->LoadFromCurrentGame(this->pCurrentGame) ||
			!this->pRoomWidget->LoadFromCurrentGame(this->pCurrentGame))
		return false;

	SetCursor(CUR_Wait);

	//Always return to the title screen for escape.
	if (!this->bPlayTesting)
	{
		g_pTheSM->ClearReturnScreens();
		g_pTheSM->InsertReturnScreen(SCR_Title);
	}

	UpdateSound();
	this->pRoomWidget->AllowSleep(false);
	this->pRoomWidget->ClearEffects();
	if (!CanShowVarUpdates())
		this->pRoomWidget->ShowVarUpdates(false);
	this->pMapWidget->DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom, this->pCurrentGame->pRoom->mapMarker);
	SetSignTextToCurrentRoom();
	this->pFaceWidget->SetMood(PlayerRole, Mood_Normal);
	SynchScroll();
	PaintClock(true);

	//Init the keysym-to-command map and load other player settings.
	ApplyPlayerSettings();

	//Set frame rate as high as needed for smooth animations.
	SetBetweenEventsInterval(15); //60+ fps

	SetMusicStyle();
	SwirlEffect();
	ClearSpeech();
	SetGameAmbience(true);
	AmbientSoundSetup();

	SelectFirstWidget(false);

	this->pCurrentGame->UpdateTime(SDL_GetTicks());
//	ShowLockIcon(this->pCurrentGame->bRoomExitLocked);

	this->pRoomWidget->DontAnimateMove();

	//Never return to the restore screen.  Yank it out of the return list so
	//that we go back to the title screen instead.
	if (g_pTheSM->GetReturnScreenType() == SCR_Restore)
		g_pTheSM->RemoveReturnScreen();

/*
	//Has the current player previously conquered this hold and room?
	const UINT dwHoldID = this->pCurrentGame->pHold->dwHoldID;
	this->bHoldConquered = g_pTheDB->SavedGames.FindByEndHold(dwHoldID) != 0;
	if (this->bHoldConquered) //don't need to compile this set unless this is true
		g_pTheDB->Holds.GetRoomsExplored(dwHoldID,
				g_pTheDB->GetPlayerID(), this->roomsPreviouslyConquered);
	else
		this->roomsPreviouslyConquered.clear();
*/

/*
	//Whenever entering the game screen,
	//remind the player if the level requirements have been completed.
	if (this->pCurrentGame->IsCurrentLevelComplete())
		this->sCueEvents.Add(CID_CompleteLevel);
*/

	delete g_pPredictedCombat;
	g_pPredictedCombat = NULL;
	this->pCurrentGame->pPlayer->ResetMyPathToGoal();
	this->wMoveDestX = this->wMoveDestY = NO_DESTINATION;

	this->signColor = Black;

	//When coming from another screen, cue events might have been generated that
	//need to be displayed when returning to this screen.
	//Examples of this are the "exit level!" or "secret room!" cues that should
	//display on level entrance, or persistent effects like burning fuses.
	//
	//Also, the level/hold might be exited on the first move.
	//In this event, just show the exit event stuff here and then decide whether
	//to continue activating the game screen.
	this->bPersistentEventsDrawn = this->bNeedToProcessDelayedQuestions = false;
	ProcessSpeechCues(this->sCueEvents);
	SCREENTYPE eNextScreen = ProcessCueEventsAfterRoomDraw(this->sCueEvents);
	if (eNextScreen == SCR_Game)
		if (ProcessExitLevelEvents(this->sCueEvents, eNextScreen))
		{
			//Transition to specified screen instead of the game screen.
			if (eNextScreen != SCR_Game)
				g_pTheDSM->InsertReturnScreen(eNextScreen);
		}

	this->chat.SetRefreshInterval(20000); //20s automatic refresh
	this->chat.refreshNow();

	//Eat existing events since early key presses could cause unexpected player movement.
	ClearEvents();

	//Set button widget states.
	CWidget *pButton = GetWidget(TAG_SAVE);
	pButton->Enable(!this->bPlayTesting);
	pButton = GetWidget(TAG_LOAD);
	pButton->Enable(!this->bPlayTesting);

	SetCursor();

	//If the game says to go to a different screen, then don't stay on this screen.
	return eNextScreen == SCR_Game;
}

//*****************************************************************************
void CGameScreen::SetSignTextToCurrentRoom()
//Set sign text to description of current room and repaint it.
{
	ASSERT(this->pCurrentGame);

	WSTRING wstrSignText = this->pCurrentGame->customRoomLocationText;
	if (wstrSignText.empty()) {
		wstrSignText = (const WCHAR*)this->pCurrentGame->pLevel->NameText;
	}
	if (this->bShowingCutScene)
	{
		wstrSignText += wszSpace;
		wstrSignText += (const WCHAR *) CDbMessageText(MID_PlayingCutScene);
		this->signColor = Red;
	} else {
		this->signColor = Black;

		if (this->pCurrentGame->customRoomLocationText.empty()) {
			ASSERT(this->pCurrentGame->pRoom);
			wstrSignText += wszColon;
			wstrSignText += wszSpace;
			this->pCurrentGame->pRoom->GetLevelPositionDescription(wstrSignText);
		}
	}

/*
	if (this->pCurrentGame->IsDemoRecording())
	{
		wstrSignText += wszSpace;
		wstrSignText += (const WCHAR *) CDbMessageText(MID_RecordingStatus);
	}
*/

	SetSignText(wstrSignText.c_str());
}

//*****************************************************************************
void CGameScreen::ShowBattleDialog(const WCHAR* pwczMessage)
{
	CDialogWidget *pStatsDialog = DYN_CAST(CDialogWidget*, CWidget*, GetWidget(TAG_BATTLEDIALOG));
	CLabelWidget *pText = DYN_CAST(CLabelWidget*, CWidget*, pStatsDialog->GetWidget(TAG_BATTLETEXT));
	pText->SetText(pwczMessage);

	CWidget *pFrame = pStatsDialog->GetWidget(TAG_BATTLEFRAME);
	CWidget *pTilesWidget = pStatsDialog->GetWidget(TAG_BATTLETILES);

	//Resize label text for text height.
	UINT wTextWidth, wTextHeight;
	g_pTheFM->GetTextRectHeight(FONTLIB::F_Message,
			pwczMessage, pText->GetW(), wTextWidth, wTextHeight);
	pText->SetHeight(wTextHeight);
	pTilesWidget->SetHeight(wTextHeight);

	//Center OK button at bottom of frame.
	CWidget *pWidget = pStatsDialog->GetWidget(TAG_BATTLEOK);
	pWidget->Move((pFrame->GetW() - pWidget->GetW())/2, wTextHeight + CY_SPACE*3/2);

	//Resize frame to fit text and OK button.
	pFrame->SetHeight((pWidget->GetY()-pFrame->GetY()) + pWidget->GetH() + CY_SPACE/2);

	//Dialog is tall enough to display everything.
	pStatsDialog->SetHeight(pFrame->GetH() + (pFrame->GetY()-pStatsDialog->GetY())*2);

	//Center the dialog on its parent.
	pStatsDialog->Center();
	ASSERT(pStatsDialog->IsInsideOfParent()); //If this fires, probably the dialog text is too long.

	//Activate the dialog widget.
	bool bWasCursorVisible = IsCursorVisible();
	ShowCursor();
	UINT dwRetIgnored = pStatsDialog->Display();
	if (!bWasCursorVisible) HideCursor();

	//Repaint the screen to fix area underneath dialog.
	Paint();

	//Throw away anything causing possible side effects of having popped up this dialog.
	StopKeyRepeating();
	StopMouseRepeating();
	SDL_Event event;
	while (PollEvent(&event)) ;
}

//*****************************************************************************
void CGameScreen::ShowMonsterStats(CDbRoom *pRoom, CRoomWidget *pRoomWidget)
//Show stats for all monsters in room in a pop-up box.
//If there are too many monster types to fit in the pop-up box, then multiple
//boxes of fitting text will be displayed.
{
	static const UINT MAX_DISPLAY_TYPES = 6;

	CSwordsman& player = *this->pCurrentGame->pPlayer;
	std::multimap<UINT, CCoord> damageFromMonsters;
	vector<MonsterStats> monsterNPCStats;

	CDialogWidget *pStatsDialog = DYN_CAST(CDialogWidget*, CWidget*, GetWidget(TAG_BATTLEDIALOG));
	CTilesWidget *pTilesWidget = DYN_CAST(CTilesWidget*, CWidget*, pStatsDialog->GetWidget(TAG_BATTLETILES));
	pTilesWidget->ClearTiles();

	//Sort monsters by damage to player, w/ "can't be hurt" ones indicated by UINT(-1) value
	CIDSet encounteredMonsterTypes;
	CMonster *pMonster = pRoom->pFirstMonster;
	while (pMonster)
	{
		bool bDistinctType = pMonster->wType == M_CHARACTER || !encounteredMonsterTypes.has(pMonster->wType);
		if (bDistinctType && pMonster->IsCombatable())
		{
			if (pMonster->wType == M_CHARACTER)
			{
				//Track NPCs type, stats and color -- if they all match a previous NPC, ignore this one.
				MonsterStats npcStats(pMonster->GetIdentity(), pMonster->getATK(), pMonster->getDEF(),
						pMonster->getHP(), pMonster->getGOLD(), pMonster->getColor());
				UINT i;
				for (i=0; i<monsterNPCStats.size(); ++i)
					if (monsterNPCStats[i] == npcStats)
						break;
				if (i<monsterNPCStats.size())
					bDistinctType = false;
				else {
					//Remember this NPC's values.
					monsterNPCStats.push_back(npcStats);
				}
			}
			if (bDistinctType)
			{
				CCombat combat(pRoom->GetCurrentGame(), pMonster, player.HasSword(),
						player.wX, player.wY, pMonster->wX, pMonster->wY);
				const int damage = combat.GetExpectedDamage();
				damageFromMonsters.insert(std::make_pair(static_cast<UINT>(damage), CCoord(pMonster->wX, pMonster->wY)));
				encounteredMonsterTypes += pMonster->wType;
			}
		}
		pMonster = pMonster->pNext;
	}

	//Each iteration displays combat stats for one monster.
	WSTRING text;
	UINT count=0;
	bool bDisplayedOnce = false;
	for (std::multimap<UINT, CCoord>::reverse_iterator riter=damageFromMonsters.rbegin();
			riter!=damageFromMonsters.rend(); ++riter)
	{
		CMonster *pMonster = pRoom->GetMonsterAtSquare(
				riter->second.wX, riter->second.wY);
		ASSERT(pMonster);
		pMonster = pMonster->GetOwningMonster();
		bool success = AddMonsterStats(pRoom, pRoomWidget, pMonster, text);

		//Show full page of monster stats.
		if (++count >= MAX_DISPLAY_TYPES || !success)
		{
			ShowBattleDialog(text.c_str());
			text.resize(0);
			count=0;
			pTilesWidget->ClearTiles();
			bDisplayedOnce = true;
			if (!success) {
				--riter;
			}

			pRoomWidget->DirtyRoom(); //temp room isn't auto-redrawn
			pRoomWidget->Paint();
			UpdateRect();
		}
	}

	if (!text.empty())
		ShowBattleDialog(text.c_str());
	else if (!bDisplayedOnce)
		ShowOkMessage(MID_NoMonstersToDisplay);
}

//
//CGameScreen private methods.
//

//*****************************************************************************
bool CGameScreen::AddMonsterStats(
//Add the combat information for a given monster to a string, but only if the
// resulting string fits within the allowed space.
//
//Params:
	CDbRoom* pRoom, //current room
	CRoomWidget* pRoomWidget, //the display widget for the room
	CMonster* pMonster, //monster to display information for
	WSTRING& text) //string to append to. may be non-empty.
//
//Returns: If the text was appended
{
	CDialogWidget* pStatsDialog = DYN_CAST(CDialogWidget*, CWidget*, GetWidget(TAG_BATTLEDIALOG));
	CTilesWidget* pTilesWidget = DYN_CAST(CTilesWidget*, CWidget*, pStatsDialog->GetWidget(TAG_BATTLETILES));
	CLabelWidget* pText = DYN_CAST(CLabelWidget*, CWidget*, pStatsDialog->GetWidget(TAG_BATTLETEXT));

	WSTRING newText = pRoomWidget->GetMonsterInfo(pMonster->wX, pMonster->wY, true);
	UINT wTextWidth = 0, wCurrentTextHeight = 0, wNewTextHeight = 0;

	if (!text.empty()) //determine text height
		g_pTheFM->GetTextRectHeight(FONTLIB::F_Message,
			text.c_str(), pText->GetW(), wTextWidth, wCurrentTextHeight);

	g_pTheFM->GetTextRectHeight(FONTLIB::F_Message,
		newText.c_str(), pText->GetW(), wTextWidth, wNewTextHeight);

	//If the resulting text will make the dialog to large, don't add it
	if (wCurrentTextHeight + wNewTextHeight > CDrodBitmapManager::CY_ROOM - 60) {
		return false;
	}

	//Add monster 2x2 display on left side of dialog, like is done in sidebar monster stats.
	//Draw monster tiles at current text height.
	float r, g, b;
	pRoomWidget->TranslateMonsterColor(pMonster->getColor(), r, g, b);
	UINT tileNo = TI_UNSPECIFIED;
	for (UINT i = 2 * 2; i--; )
	{
		const UINT x = i % 2;
		const UINT y = i / 2;
		const bool bCentered = (x == 0 && tileNo == TI_UNSPECIFIED);
		tileNo = GetMonsterDisplayTile(pMonster, x, y);
		if (tileNo != TI_UNSPECIFIED)
		{
			const int xPixel = x * CDrodBitmapManager::CX_TILE +
				(bCentered ? CDrodBitmapManager::CX_TILE / 2 : 0);
			pTilesWidget->AddTile(tileNo, xPixel,
				wCurrentTextHeight + y * CDrodBitmapManager::CY_TILE, r, g, b);
		}
	}

	if (!text.empty())
		text += wszCRLF;
	text += newText;

	return true;
}

//******************************************************************************
UINT CGameScreen::MarkMapRoom(const UINT roomID)
//Mark indicated room with a map color.
//
//Returns: new map marker, or MAP_NOT_MARKED if room could not be marked
{
	ASSERT(roomID);

	//Mark only if room is on the player's map.
	ExploredRoom *pExpRoom = this->pCurrentGame->getExploredRoom(roomID);
	if (pExpRoom || roomID == this->pCurrentGame->pRoom->dwRoomID)
	{
		CDbRoom *pRoom = this->pCurrentGame->pRoom;
		if (pRoom->dwRoomID == roomID)
		{
			if (++pRoom->mapMarker > MAX_MARKERS)
				pRoom->mapMarker = 0;
			return pRoom->mapMarker;
		} else {
			if (pExpRoom)
			{
				if (++pExpRoom->mapMarker > MAX_MARKERS)
					pExpRoom->mapMarker = 0;
				return pExpRoom->mapMarker;
			}
		}
	}
	return MAP_NOT_MARKED;
}

//******************************************************************************
void CGameScreen::MovePlayerInDirection(const int dx, const int dy)
//Execute a movement command in the direction of (dx,dy)
{
	int cmd = CMD_WAIT;
	switch (nGetO(sgn(dx),sgn(dy)))
	{
		case N: cmd = CMD_N; break;
		case NE: cmd = CMD_NE; break;
		case E: cmd = CMD_E; break;
		case SE: cmd = CMD_SE; break;
		case S: cmd = CMD_S; break;
		case SW: cmd = CMD_SW; break;
		case W: cmd = CMD_W; break;
		case NW: cmd = CMD_NW; break;
		case NO_ORIENTATION:
	/*
			//Clicked on player -- avatar greeting.
			if (bAllowAvatarGreeting && !this->pRoomWidget->bPlayerSleeping)
			{
				UINT eSoundID = SEID_NONE;
				switch (this->pCurrentGame->pPlayer->wAppearance)
				{
					case M_CLONE: case M_DECOY: case M_MIMIC:
					case M_BEETHRO: case M_BEETHRO_IN_DISGUISE: eSoundID = SEID_HI; break;
					case M_NEATHER:
					case M_HALPH: eSoundID = SEID_HALPHENTERED; break;
					case M_EYE: case M_MADEYE: eSoundID = SEID_EVILEYEWOKE; break;
					case M_GOBLIN:	case M_GOBLINKING: eSoundID = SEID_GOB_HI; break;
					case M_TARBABY: case M_MUDBABY: case M_GELBABY:
					case M_TARMOTHER: case M_MUDMOTHER: case M_GELMOTHER:
						eSoundID = SEID_SPLAT; break;
					case M_CITIZEN1: case M_CITIZEN2: case M_GUARD:
					case M_MUDCOORDINATOR: case M_TARTECHNICIAN:
					case M_CITIZEN:
						eSoundID = SEID_CIT_HI; break;
					case M_ROCKGOLEM: case M_ROCKGIANT:
						eSoundID = SEID_ROCK_HI; break;
					case M_WUBBA: eSoundID = SEID_WUBBA; break;
					case M_SLAYER: eSoundID = SEID_SLAYERCOMBAT; break;
					case M_ROACH: case M_QROACH: case M_WWING: case M_REGG:
					case M_SERPENT: case M_SPIDER: case M_SERPENTG: case M_SERPENTB:
					case M_WATERSKIPPER: case M_AUMTLICH: case M_SEEP: case M_FEGUNDO:
						eSoundID = SEID_MON_CLEAR; break;
					case M_NEGOTIATOR: case M_INSTRUCTOR:
					case M_CITIZEN3: case M_CITIZEN4:
						eSoundID = SEID_WOM_HI; break;
					case M_STALWART: eSoundID = SEID_STALWART_HI; break;
					default: break;
				}

				if (eSoundID != (UINT)SEID_NONE)
					g_pTheSound->PlaySoundEffect(eSoundID, NULL, NULL, true);
				g_pTheSound->PlaySoundEffect(SEID_HI, NULL, NULL, true);
			}
	*/
		break;
		default: ASSERT(!"Bad direction"); break;
	}

	if (cmd != CMD_WAIT)
		ProcessCommandWrapper(cmd);
}

//******************************************************************************
bool CGameScreen::IsOpenMove(const int dx, const int dy) const
//Returns: whether the player may move in this direction from his current position
{
	const CSwordsman& p = *this->pCurrentGame->pPlayer;
	return this->pCurrentGame->CanPlayerMoveTo(p.wX + sgn(dx), p.wY + sgn(dy));
}

//******************************************************************************
void CGameScreen::MovePlayerToward(const int nScreenX, const int nScreenY)
//Move player towards the indicated on-screen location.
{
	if (!this->pCurrentGame)
		return;

	//Anything that wouldn't allow the player to move.
	if (this->pCurrentGame->Commands.IsFrozen() ||
			this->pCurrentGame->dwCutScene || !this->pCurrentGame->pPlayer->IsInRoom() ||
			this->pCurrentGame->GetUnansweredQuestion() || this->pCurrentGame->InCombat())
		return;

	//Mouse clicks on widgets in the sidebar shouldn't cause movement.
	if (nScreenX <= (int)CX_SIDEBAR)
		return;

	//Determine destination tile.
	const int nRoomPX = nScreenX - this->pRoomWidget->GetX();
	const int nRoomPY = nScreenY - this->pRoomWidget->GetY();
	int nRoomX = nRoomPX / int(CDrodBitmapManager::CX_TILE);
	int nRoomY = nRoomPY / int(CDrodBitmapManager::CY_TILE);

	if (nRoomPX < 0) //ensure negative number
		--nRoomX;
	if (nRoomPY < 0)
		--nRoomY;

	if (nRoomX < -1) //if clicking goes into sidebar, don't count as movements
		return;

	//Quick backtracking to an adjacent room by clicking a room edge while holding Ctrl.
	if (SDL_GetModState() & KMOD_CTRL)
	{
		UINT direction = NO_ORIENTATION;
		if (nRoomY <= 0)
		{
			direction = N;
			nRoomY = 0;
		}
		else if (nRoomX <= 0)
		{
			direction = W;
			nRoomX = 0;
		}
		else if (nRoomX >= int(this->pCurrentGame->pRoom->wRoomCols-1))
		{
			direction = E;
			nRoomX = this->pCurrentGame->pRoom->wRoomCols-1;
		}
		else if (nRoomY >= int(this->pCurrentGame->pRoom->wRoomRows-1))
		{
			direction = S;
			nRoomY = this->pCurrentGame->pRoom->wRoomRows-1;
		}
		if (direction != NO_ORIENTATION)
		{
			CCoord exitCoord(nRoomX, nRoomY);
			SearchForPathToNextRoom(direction, &exitCoord);
		}
		return;
	}

	MovePlayerTowardTile(nRoomX, nRoomY);
}

//******************************************************************************
void CGameScreen::MovePlayerTowardTile(int nRoomX, int nRoomY)
//Move player toward the indicated room tile.
{
	ASSERT(this->pCurrentGame);
	ASSERT(this->pCurrentGame->pRoom);

	if (this->ignoreDestUntilMouseUp.equals(nRoomX, nRoomY))
		return;

	CSwordsman& p = *(this->pCurrentGame->pPlayer);

	//Bounds checking, after ignore coord check.
	//When clicking outside the room on a side,
	//then this indicates moving to the room edge and exiting.
	if (nRoomX < 0)
	{
		nRoomX = 0;
		if (p.wX == (UINT)nRoomX)
			nRoomY = p.wY; //if already at room edge, this will exit
	}
	else if (nRoomX >= (int)this->pCurrentGame->pRoom->wRoomCols)
	{
		nRoomX = this->pCurrentGame->pRoom->wRoomCols-1;
		if (p.wX == (UINT)nRoomX)
			nRoomY = p.wY;
	}
	if (nRoomY < 0)
	{
		nRoomY = 0;
		if (p.wY == (UINT)nRoomY)
			nRoomX = p.wX;
	}
	else if (nRoomY >= (int)this->pCurrentGame->pRoom->wRoomRows)
	{
		nRoomY = this->pCurrentGame->pRoom->wRoomRows-1;
		if (p.wY == (UINT)nRoomY)
			nRoomX = p.wX;
	}

	//Get vector to goal.
	int dx = nRoomX - int(p.wX);
	int dy = nRoomY - int(p.wY);

	const bool bSameDest = (UINT)nRoomX == this->wMoveDestX && (UINT)nRoomY == this->wMoveDestY;

	//Display where player is moving toward.
	if (dx || dy)
	{
		if (!bSameDest)
		{
			this->pRoomWidget->RemoveLastLayerEffectsOfType(ESCALETILE);
			CMonster *pMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(nRoomX, nRoomY);
			const UINT tileNo = pMonster != NULL ? TI_CHECKPOINT : TI_CHECKPOINT_L;
			this->pRoomWidget->AddLastLayerEffect(new CScaleTileEffect(this->pRoomWidget,
					this->pRoomWidget->GetX() + nRoomX * g_pTheDBM->CX_TILE + g_pTheDBM->CX_TILE/2,
					this->pRoomWidget->GetY() + nRoomY * g_pTheDBM->CY_TILE + g_pTheDBM->CX_TILE/2,
					tileNo, 500, -(float)(2*g_pTheDBM->CX_TILE)));
		}

		//Setting this will prevent multiple bumps into an adjacent obstacle.
		this->ignoreDestUntilMouseUp.set(nRoomX, nRoomY);
	}

	//Move player toward this tile at earliest opportunity.
	this->wMoveDestX = nRoomX;
	this->wMoveDestY = nRoomY;
	this->touchedTilesOnPath.clear(); //haven't taken any steps to dest yet
}

//******************************************************************************
void CGameScreen::MovePlayerTowardDestTile()
//Move the player one step toward the marked destination tile.
{
	if (!this->pCurrentGame)
		return;
	ASSERT(this->pCurrentGame->pRoom);
	if (!this->pCurrentGame->pRoom->IsValidColRow(this->wMoveDestX, this->wMoveDestY))
		return;

	//Get vector to goal.
	CSwordsman& p = *(this->pCurrentGame->pPlayer);
	int dx = this->wMoveDestX - int(p.wX);
	int dy = this->wMoveDestY - int(p.wY);

	//Don't oscillate or follow a cyclical path.
	if (this->touchedTilesOnPath.has(p.wX, p.wY))
	{
		this->wMoveDestX = this->wMoveDestY = NO_DESTINATION;
		this->touchedTilesOnPath.clear();
		return;
	}

	if (!dx && !dy)
	{
		//Holding mouse button down on player's location.
		int nX, nY;
		const Uint8 buttons = GetMouseState(&nX, &nY);
		if (buttons == SDL_BUTTON_LMASK)
		{
			//After moving to room edge, exit room in this direction.
			if (p.wY > 0 && p.wY < this->pCurrentGame->pRoom->wRoomRows-1) //corners are ambiguous -- don't do anything here
			{
				if (!p.wX)
					ProcessCommandWrapper(CMD_W);
				else if (p.wX == this->pCurrentGame->pRoom->wRoomCols-1)
					ProcessCommandWrapper(CMD_E);
			}
			else if (p.wX > 0 && p.wX < this->pCurrentGame->pRoom->wRoomCols-1)
			{
				if (!p.wY)
					ProcessCommandWrapper(CMD_N);
				else if (p.wY == this->pCurrentGame->pRoom->wRoomRows-1)
					ProcessCommandWrapper(CMD_S);
			}
		}
		this->wMoveDestX = this->wMoveDestY = NO_DESTINATION;
		this->touchedTilesOnPath.clear();
		return;
	}

	//If the player isn't adjacent to the goal tile, allow for smarter beelining.
	const int absDx = abs(dx), absDy = abs(dy);
	if (absDx > 1 || absDy > 1)
	{
		//Attempt to find a smart path to the goal,
		//except when the player is on a platform, with more restrictive movement rules.
		const bool bOnPlatform = bIsPlatform(this->pCurrentGame->pRoom->GetOSquare(p.wX, p.wY));
		if (!bOnPlatform)
		{
			CCoordSet dests(this->wMoveDestX, this->wMoveDestY);
			p.ResetMyPathToGoal();
			p.bIntraRoomPath = true;
			p.bPathToStairs = bIsStairs(this->pCurrentGame->pRoom->GetOSquare(
					this->wMoveDestX, this->wMoveDestY));

			bool bFound = p.FindOptimalPathTo(p.wX, p.wY, dests, false);
			if (!bFound) //attempt to move next to goal
				bFound = p.FindOptimalPathTo(p.wX, p.wY, dests, true);
			p.bIntraRoomPath = p.bPathToStairs = false;
			if (bFound)
			{
				CCoordStack *pPath = p.GetPathToGoal();
				ASSERT(pPath);

				//Take a step along the path.
				UINT wNextX, wNextY;
				pPath->Pop(wNextX, wNextY);

				const int deltaX = wNextX - p.wX;
				const int deltaY = wNextY - p.wY;
				if (abs(deltaX) <= 1 && abs(deltaX) <= 1)
				{
					//Valid step.
					dx = deltaX;
					dy = deltaY;
					goto MovePlayer;
				}
				//otherwise, just beeline
			}
		}

		//If a diagonal move is required, see if directly moving to the square will work.
		const bool bDiagonal = dx && dy;
		bool bTriedHorizontal = false;
		if (bDiagonal)
		{
			if (IsOpenMove(dx, dy))
				goto MovePlayer;  //Can take the direct move.

			//A diagonal move is desired but can't be made.  Determine whether moving in
			//the vertical or horizontal direction will come closer to the target.
			//Try it first, and choose it if legal.
			if (nDist(p.wX + dx, p.wY, this->wMoveDestX, this->wMoveDestY) <
					nDist(p.wX, p.wY + dy, this->wMoveDestX, this->wMoveDestY))
			{
				//Moving horizontally would get closer to the target.
				//Try it first.
				if (IsOpenMove(dx, 0))
				{
					dy = 0;
					goto MovePlayer;
				}
				//Moving horizontally didn't work -- don't try it again below.
				bTriedHorizontal = true;
			}
		}

		//See if moving in just the vertical direction will work.
		if (dy)
		{
			if (IsOpenMove(0, dy))
			{
				dx = 0;
				goto MovePlayer;
			}
		}

		//See if moving in just the horizontal direction will work.
		if (dx && !bTriedHorizontal)
		{
			if (IsOpenMove(dx, 0))
			{
				dy = 0;
				goto MovePlayer;
			}
		}

		//At this point, we've calculated that the player cannot move from here
		//toward the destination tile, but we'll allow trying to anyway so the
		//player can receive feedback that it doesn't work.
	}

/*
	//Move in direct "jaggy" line toward the goal,
	//almost like drawing a straight line of pixels.
	if (absDx > 2 * absDy)
		dy = 0;
	else if (absDy > 2 * absDx)
		dx = 0;
*/

MovePlayer:
	const UINT wPX = p.wX, wPY = p.wY;
	this->touchedTilesOnPath.insert(wPX, wPY);
	MovePlayerInDirection(dx,dy);

	//If the player couldn't move, interrupt path to destination.
	if (p.wX == wPX && p.wY == wPY)
		this->wMoveDestX = this->wMoveDestY = NO_DESTINATION;
	//If player used a tunnel warp, then reset path to avoid oscillations.
	else if (this->sCueEvents.HasOccurred(CID_Tunnel))
		this->wMoveDestX = this->wMoveDestY = NO_DESTINATION;
}

//*****************************************************************************
void CGameScreen::AddDamageEffect(
//Adds an effect for the player or a monster type getting damaged to the room.
//
//Params:
	const UINT wMonsterType,
	const CMoveCoord& coord,
	float fDamagePercent) //[default=1.0]
{
	if (fDamagePercent > 1.0)
		fDamagePercent = 1.0;

	//Effect shown based on monster type.
	switch (wMonsterType)
	{
		case M_TARBABY:
		case M_TARMOTHER:
		{
			UINT particles = UINT(20 * fDamagePercent);
			if (!particles)
				particles = 1;
			this->pRoomWidget->AddTLayerEffect(
				new CTarStabEffect(this->pRoomWidget, coord,
						GetEffectDuration(7), GetParticleSpeed(3), particles));
		}
		break;
		case M_MUDBABY:
		case M_MUDMOTHER:
		{
			UINT particles = UINT(20 * fDamagePercent);
			if (!particles)
				particles = 1;
			this->pRoomWidget->AddTLayerEffect(
				new CMudStabEffect(this->pRoomWidget, coord,
						GetEffectDuration(7), GetParticleSpeed(3), particles));
		}
		break;
		case M_GELBABY:
		case M_GELMOTHER:
		{
			UINT particles = UINT(20 * fDamagePercent);
			if (!particles)
				particles = 1;
			this->pRoomWidget->AddTLayerEffect(
				new CGelStabEffect(this->pRoomWidget, coord,
						GetEffectDuration(7), GetParticleSpeed(3), particles));
		}
		break;
		case M_SEEP:
		{
			UINT particles = UINT(20 * fDamagePercent);
			if (!particles)
				particles = 1;
			this->pRoomWidget->AddTLayerEffect(
				new CBloodInWallEffect(this->pRoomWidget, coord, particles));
		}
		break;
		case M_ROCKGOLEM:
		case M_ROCKGIANT:
		{
			UINT particles = UINT(10 * fDamagePercent);
			if (!particles)
				particles = 1;
			this->pRoomWidget->AddTLayerEffect(
				new CGolemDebrisEffect(this->pRoomWidget, coord, particles,
						GetEffectDuration(7), GetParticleSpeed(3)));
		}
		break;
		case M_SLAYER:
		{
			UINT particles = UINT(40 * fDamagePercent);
			if (!particles)
				particles = 1;
			this->pRoomWidget->AddTLayerEffect(
				new CVerminEffect(this->pRoomWidget, coord, particles, true));
		}
		break;
		case M_BEETHRO:
		case M_BEETHRO_IN_DISGUISE:
		case M_PIRATE: case M_STALWART:
		{
			//Player and monster types.
			UINT particles = UINT(30 * fDamagePercent);
			if (!particles)
				particles = 1;
			this->pRoomWidget->AddTLayerEffect(
				new CBloodEffect(this->pRoomWidget, coord, particles,
						GetEffectDuration(7), GetParticleSpeed(3)));
		}
		break;
		default:
		{
			UINT particles = UINT(16 * fDamagePercent);
			if (!particles)
				particles = 1;
			this->pRoomWidget->AddTLayerEffect(
				new CBloodEffect(this->pRoomWidget, coord, particles,
						GetEffectDuration(7), GetParticleSpeed(3)));
		}
		break;
	}

	this->pRoomWidget->AddJitter(coord, fDamagePercent);
}

//*****************************************************************************
void CGameScreen::AddKillEffect(const UINT wMonsterType, const CMoveCoord& coord)
//Adds an effect for a monster type getting killed to the room.
{
	//If player stabs monster, center sound on player for nicer effect.
	CSwordsman &player = *this->pCurrentGame->pPlayer;
	if (player.GetSwordX() == coord.wX && player.GetSwordY() == coord.wY)
	{
		this->fPos[0] = static_cast<float>(player.wX);
		this->fPos[1] = static_cast<float>(player.wY);
	} else {
		this->fPos[0] = static_cast<float>(coord.wX);
		this->fPos[1] = static_cast<float>(coord.wY);
	}

	UINT soundID;
	switch (wMonsterType)
	{
		case M_SLAYER: soundID = SEID_SLAYERDIE; break;
		case M_ROCKGOLEM: case M_ROCKGIANT: soundID = SEID_BREAKWALL; break;
		default: soundID = SEID_SPLAT; break;
	}
	g_pTheSound->PlaySoundEffect(soundID, this->fPos);

	AddDamageEffect(wMonsterType, coord);
}

//*****************************************************************************
void CGameScreen::AmbientSoundSetup()
//Queries current game for which ambient sounds should be playing at this
//moment of play.
{
	for (vector<CMoveCoordEx>::const_iterator sound = this->pCurrentGame->ambientSounds.begin();
			sound != this->pCurrentGame->ambientSounds.end(); ++sound)
	{
		PlayAmbientSound(sound->wO, sound->wValue != 0, sound->wX, sound->wY);
	}
}

//*****************************************************************************
void CGameScreen::ApplyPlayerSettings()
//Apply player settings to the game screen.
{
	ApplyINISettings();

	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	if (!pCurrentPlayer) {
		ASSERT(!"CGameScreen::ApplyPlayerSettings(): Couldn't retrieve current player.");
		return; //Corrupt db.
	}

	//Set the keysym-to-command map from player settings.
	CDbPackedVars &settings = pCurrentPlayer->Settings;
	InitKeysymToCommandMap(settings);

	//Set room widget to either show checkpoints or not.
	this->pRoomWidget->ShowCheckpoints(true); //always show checkpoints
//			settings.GetVar(Settings::ShowCheckpoints, true));

	this->bShowingSubtitlesWithVoice =
			settings.GetVar(Settings::ShowSubtitlesWithVoices, true);

	//Set mouse UI preference.
	this->bDisableMouseMovement =
			settings.GetVar(Settings::DisableMouse, false);
	CWidget *pWidget = GetWidget(TAG_UNDO);
	pWidget->Show(!this->bDisableMouseMovement);
	pWidget = GetWidget(TAG_ROT_CW);
	pWidget->Show(!this->bDisableMouseMovement);
	pWidget = GetWidget(TAG_ROT_CCW);
	pWidget->Show(!this->bDisableMouseMovement);
	pWidget = GetWidget(TAG_LOCK);
	pWidget->Show(!this->bDisableMouseMovement);
	pWidget = GetWidget(TAG_WAIT);
	pWidget->Show(!this->bDisableMouseMovement);
	pWidget = GetWidget(TAG_USECOMMAND);
	pWidget->Show(!this->bDisableMouseMovement);

	//Move repeat rate.
	const UINT dwRepeatRate = (((long)(settings.GetVar(Settings::RepeatRate,
			(BYTE)128))) * MAX_REPEAT_RATE / 256) + 1;  //value from 1 to MAX
	const UINT dwTimePerRepeat = 1000L / dwRepeatRate;
	SetKeyRepeat(dwTimePerRepeat);
	this->pRoomWidget->SetMoveDuration(dwTimePerRepeat);

	this->bEnableChat = settings.GetVar(Settings::EnableChatInGame, false);
	this->bReceiveWhispersOnly = settings.GetVar(Settings::ReceiveWhispersOnlyInGame, false);

	SetQuickCombat();

	const bool bShowDamagePreview = settings.GetVar(Settings::DamagePreview, true);
	this->pRoomWidget->ShowDamagePreview(bShowDamagePreview);
	this->pTempRoomWidget->ShowDamagePreview(bShowDamagePreview);

/*
	//Set times when saved games and demos are saved automatically.
	if (!this->bPlayTesting)
	{
		const UINT dwAutoSaveOptions = settings.GetVar(
				Settings::AutoSaveOptions, ASO_DEFAULT | ASO_CONQUERDEMO);
		this->pCurrentGame->SetAutoSaveOptions(dwAutoSaveOptions);
		pCurrentPlayer->Update();
	}
*/

	delete pCurrentPlayer;
}

//*****************************************************************************
void CGameScreen::SearchForPathToNextRoom(
	const UINT direction, //direction to exit current room from
	CCoord *pDest) //[default=NULL] specific tile to exit room from
{
	//Interrupts any mouse-selected path.
	this->wMoveDestX = this->wMoveDestY = NO_DESTINATION;

	//Has the player already visited the room to this direction?
	int xOffset=0, yOffset=0;
	switch (direction)
	{
		case N: yOffset = -1; break;
		case S: yOffset = +1; break;
		case W: xOffset = -1; break;
		case E: xOffset = +1; break;
		case GO_TO_STAIRS: break;
		default: ASSERT(!"Invalid direction"); return;
	}

	//If the player hasn't been to a room in the requested exit direction,
	//then give a general error message saying that way is not accessible.
	CSwordsman& player = *(this->pCurrentGame->pPlayer);
	const CDbRoom& room = *this->pCurrentGame->pRoom;
	const UINT wPX = player.wX, wPY = player.wY;
	if (direction == GO_TO_STAIRS)
	{
		if (room.Exits.empty()) {
			this->pRoomWidget->DisplaySubtitle(
					g_pTheDB->GetMessageText(MID_QuickPathNotAvailable), wPX, wPY, true);
			return;
		}
	} else {
		CDbRoom *pRoom = this->pCurrentGame->pLevel->GetRoomAtCoords(
				this->pCurrentGame->pRoom->dwRoomX + xOffset, this->pCurrentGame->pRoom->dwRoomY + yOffset);
		if (!pRoom)
		{
			this->pRoomWidget->DisplaySubtitle(
					g_pTheDB->GetMessageText(MID_QuickPathNotAvailable), wPX, wPY, true);
			return;
		}
		const UINT newRoomID = pRoom->dwRoomID;
		delete pRoom;
		if (!this->pCurrentGame->roomsExploredAtRoomStart.has(newRoomID))
		{
			this->pRoomWidget->DisplaySubtitle(
					g_pTheDB->GetMessageText(MID_QuickPathNotAvailable), wPX, wPY, true);
			return;
		}
	}

	//Want to reach the N/S/W/E edge of the room.
	CCoordSet dests;
	if (pDest)
	{
		dests.insert(pDest->wX, pDest->wY);

		//Verify only tiles actually on the edge of the room are being targeted.
		switch (direction)
		{
			case N: ASSERT(!pDest->wY); break;
			case S: ASSERT(pDest->wY == room.wRoomRows-1); break;
			case W: ASSERT(!pDest->wX); break;
			case E: ASSERT(pDest->wX == room.wRoomCols-1); break;
			default:
			case GO_TO_STAIRS: ASSERT(!"Room exit direction not supported"); break;
		}
	} else {
		switch (direction)
		{
			case N:
			{
				for (UINT i=room.wRoomCols; i--; )
					dests.insert(i,0);
			}
			break;
			case S:
			{
				const UINT bottomRow = room.wRoomRows-1;
				for (UINT i=room.wRoomCols; i--; )
					dests.insert(i,bottomRow);
			}
			break;
			case W:
			{
				for (UINT j=room.wRoomRows; j--; )
					dests.insert(0,j);
			}
			break;
			case E:
			{
				const UINT rightCol = room.wRoomCols-1;
				for (UINT j=room.wRoomRows; j--; )
					dests.insert(rightCol,j);
			}
			break;
			case GO_TO_STAIRS:
			{
				player.bPathToStairs = true;

				for (UINT s=0; s<room.Exits.size(); ++s)
				{
					//Land on closest staircase edge.
					const CExitData& exit = *(room.Exits[s]);
					const bool bMoreThanOneTall = exit.wTop != exit.wBottom;
					for (UINT i=exit.wLeft; i<=exit.wRight; ++i) {
						if (bIsStairs(room.GetOSquare(i,exit.wTop))) //robustness check against bad stair records
							dests.insert(i,exit.wTop);
						if (bMoreThanOneTall && bIsStairs(room.GetOSquare(i,exit.wBottom)))
							dests.insert(i,exit.wBottom);
					}
					const bool bMoreThanOneWide = exit.wLeft != exit.wRight;
					for (UINT j=exit.wTop+1; j<exit.wBottom; ++j) {
						if (bIsStairs(room.GetOSquare(exit.wLeft,j)))
							dests.insert(exit.wLeft,j);
						if (bMoreThanOneWide && bIsStairs(room.GetOSquare(exit.wRight,j)))
							dests.insert(exit.wRight,j);
					}
				}
			}
			break;
		}
	}

	player.ResetMyPathToGoal();
	const bool bFoundPath = player.FindOptimalPathTo(wPX, wPY, dests, false);
	player.bPathToStairs = false;
	if (!bFoundPath)
	{
		this->pRoomWidget->DisplaySubtitle(
				g_pTheDB->GetMessageText(MID_QuickPathNotAvailable), wPX, wPY, true);
		return;  //no path is available
	}

	this->wRoomQuickExitDirection = direction;
}

//*****************************************************************************
void CGameScreen::TakeStepTowardQuickExit()
//When the player has a path mapped to a room exit, then take a step each time
//room animation is completed.
{
	ASSERT(this->pCurrentGame);
	ASSERT(this->wRoomQuickExitDirection != NO_ORIENTATION);

	//Wait until last move animation is completed.
	if (this->pRoomWidget->IsMoveAnimating())
		return;

	//Take another step along path to the selected room exit.
	CSwordsman& player = *(this->pCurrentGame->pPlayer);
	CCoordStack *pPath = player.GetPathToGoal();
	ASSERT(pPath);
	if (pPath->IsEmpty())
	{
		//Path to the room exit is completed.  Now take a step exiting the room.
		switch (this->wRoomQuickExitDirection)
		{
			case N:
				if (!player.wY) //ensure player is actually on room edge in order to exit
					ProcessCommandWrapper(CMD_N);
			break;
			case S:
				if (player.wY == this->pCurrentGame->pRoom->wRoomRows-1)
					ProcessCommandWrapper(CMD_S);
			break;
			case W:
				if (!player.wX)
					ProcessCommandWrapper(CMD_W);
			break;
			case E:
				if (player.wX == this->pCurrentGame->pRoom->wRoomCols-1)
					ProcessCommandWrapper(CMD_E);
			break;
			case GO_TO_STAIRS: break; 
			default: ASSERT(!"Invalid room exit direction"); break;
		}

		//Done with this path.
		this->wRoomQuickExitDirection = NO_ORIENTATION;
		return;
	}
	
	//Take next step.
	UINT wNextX, wNextY;
	pPath->Pop(wNextX, wNextY);

	const int dx = wNextX - player.wX;
	const int dy = wNextY - player.wY;
	if (abs(dx) > 1 || abs(dy) > 1)
	{
		//Invalid step.  The last step couldn't be made for whatever reason.
		this->wRoomQuickExitDirection = NO_ORIENTATION;
		return;
	}

	const UINT o = nGetO(dx, dy);
	const UINT cmd = orientationToCommand(o);
	if (cmd == CMD_UNSPECIFIED || cmd == CMD_WAIT)
	{
		//Something's wrong with this path -- stop advancing along it.
		this->wRoomQuickExitDirection = NO_ORIENTATION;
		return;
	}

	//Double speed of quick moves.
	if (!this->dwSavedMoveDuration)
	{
		this->dwSavedMoveDuration = this->pRoomWidget->GetMoveDuration();
		this->pRoomWidget->SetMoveDuration(this->dwSavedMoveDuration / 2);
	}

	ProcessCommandWrapper(cmd);

	//If some potentially stat-altering event occurred while back-tracking,
	//stop advancing along the path so the player can respond before the
	//room has been exited.
	if (this->sCueEvents.HasOccurred(CID_EntityAffected))
		this->wRoomQuickExitDirection = NO_ORIENTATION;
}

//*****************************************************************************
void CGameScreen::UpdateSign()
{
	SetSignTextToCurrentRoom();
	PaintSign();
}

void CGameScreen::UpdateScroll()
{
	SynchScroll();
	PaintScroll();
}

//*****************************************************************************
SCREENTYPE CGameScreen::LevelExit_OnKeydown(
//Handles SDL_KEYDOWN events for the game screen when exiting level.
//
//Params:
	const SDL_KeyboardEvent &KeyboardEvent)   //(in) Event to handle.
//
//Returns:
//Screen to go to next or SCR_Game to remain at game screen.
{
	switch (KeyboardEvent.keysym.sym)
	{
		case SDLK_RALT: case SDLK_LALT:
			return SCR_Game;  //don't advance to next screen
		case SDLK_RETURN:
			if (KeyboardEvent.keysym.mod & KMOD_ALT)
			{
				ToggleScreenSize();
				return SCR_Game;  //changing screen size shouldn't advance to next screen
			}
			break;
		case SDLK_F10:
			ToggleScreenSize();
			return SCR_Game;  //changing screen size shouldn't advance to next screen

		case SDLK_F4:
#if defined(__linux__) || defined(__FreeBSD__)
		case SDLK_PAUSE:
#endif
			if (KeyboardEvent.keysym.mod & (KMOD_ALT | KMOD_CTRL))
				return SCR_None;        //boss key -- exit immediately
		break;
		default: break;
	}

	return SCR_LevelStart;
}

//*****************************************************************************
void CGameScreen::CutSpeech(const bool bForceClearAll) //[default=true]
//If bForceClearAll is true, stop playing any running sound clips.
//Otherwise, only stop any speech/sound playing that doesn't apply to this point in game time.
{
	const UINT currentTurn = this->pCurrentGame ? this->pCurrentGame->wTurnNo : 0;

	vector<ChannelInfo> retain;
	for (vector<ChannelInfo>::const_iterator channel=this->speechChannels.begin();
			channel!=this->speechChannels.end(); ++channel)
	{
		//Stop this channel from playing if it's a speech sound clip.
		if (g_pTheSound->IsSoundPlayingOnChannel(channel->wChannel) &&
				g_pTheSound->GetSoundIDPlayingOnChannel(channel->wChannel) ==
						static_cast<UINT>(SOUNDLIB::SEID_NONE))
		{
			//If speech is still valid, then let it continue playing.
			if (channel->turnNo < currentTurn && !bForceClearAll)
			{
				retain.push_back(*channel);
				CSubtitleEffect *pEffect = channel->pEffect;
				if (this->pRoomWidget->SubtitlesHas(pEffect))
				{
					//Keep related subtitle effect alive.
					ASSERT(pEffect);
					pEffect->RequestRetainOnClear();
					pEffect->RemoveFromSubtitles();
				} else {
					retain.back().pEffect = NULL; //subtitle effect connected to this channel is finished
				}
			} else {
				VERIFY(g_pTheSound->StopSoundOnChannel(channel->wChannel));
			}
		}
	}
	this->speechChannels = retain;
	if (this->speechChannels.empty())
		this->dwNextSpeech = 0; //can start next speech immediately
}

//*****************************************************************************
void CGameScreen::DeleteCurrentGame()
//Deletes current game.  Ensures any demos are uploaded first.
{
	//Ensure all queued victory demos are submitted before deleting them.
	WaitToUploadDemos();

	StopAmbientSounds();
	ClearSpeech(true);

	delete g_pPredictedCombat;
	g_pPredictedCombat = NULL;

	delete this->pCurrentGame;
	this->pCurrentGame = NULL;
	this->pRoomWidget->UnloadCurrentGame();
//	this->bHoldConquered = false;

	this->pRoomWidget->RemoveHighlight();
}

//*****************************************************************************
void CGameScreen::DisplayChatDialog()
//Show dialog box displaying CaravelNet chat interface.
{
	CDialogWidget *pChatBox = DYN_CAST(CDialogWidget*, CWidget*,
			this->pRoomWidget->GetWidget(TAG_CHATBOX));

	pChatBox->SelectWidget(TAG_CHATINPUT, false);

	COptionButtonWidget *pOption = DYN_CAST(COptionButtonWidget*, CWidget*, pChatBox->GetWidget(TAG_CHATENABLE));
	pOption->SetChecked(this->bEnableChat);

	COptionButtonWidget *pWhisperOption = DYN_CAST(COptionButtonWidget*, CWidget*, pChatBox->GetWidget(TAG_CHATWHISPERSONLY));
	pWhisperOption->SetChecked(this->bReceiveWhispersOnly);

	CTextBoxWidget *pChat = DYN_CAST(CTextBoxWidget*, CWidget*, pChatBox->GetWidget(TAG_CHATINPUT));
	pChat->SetText(wszEmpty);

	CListBoxWidget *pUserList = DYN_CAST(CListBoxWidget*, CWidget*, pChatBox->GetWidget(TAG_CHATUSERS));
	if (!g_pTheNet->IsLoggedIn())
	{
		pUserList->Clear();
		pUserList->AddItem(0, g_pTheDB->GetMessageText(MID_CNetNotConnected), true);
	}

	//Display.
	pChatBox->SetBetweenEventsHandler(this); //keep updating room effects and
							//receiving chat messages while prompt is displayed
	const UINT returnTag = pChatBox->Display(false);
	pChatBox->SetBetweenEventsHandler(NULL);
	this->pRoomWidget->DirtyRoom();
	this->pRoomWidget->Paint();

	this->bEnableChat = pOption->IsChecked();
	this->bReceiveWhispersOnly = pWhisperOption->IsChecked();

	//Send chat (if text was entered and OK was pressed).
	const WCHAR *pText = returnTag == TAG_OK ? pChat->GetText() : wszEmpty;
	if (!ParseConsoleCommand(pText)) //Intercept console commands.
		this->chat.SendText(pText, pUserList);
}

//*****************************************************************************
void CGameScreen::FadeRoom(const bool bFadeIn, const Uint32 dwDuration)
{
	//Prepare fade surface.
	SDL_Rect srcRect;
	this->pRoomWidget->GetRect(srcRect);
	SDL_Rect destRect = MAKE_SDL_RECT(0, 0, srcRect.x + srcRect.w, srcRect.y + srcRect.h);
	SDL_Surface *pSurface = g_pTheBM->ConvertSurface(SDL_CreateRGBSurface(
			SDL_SWSURFACE, destRect.w, destRect.h, g_pTheBM->BITS_PER_PIXEL, 0, 0, 0, 0));
	if (!pSurface) return; //no memory

	SDL_Surface *pDestSurface = GetDestSurface();
	if (bFadeIn)
	{
		SDL_BlitSurface(this->pRoomWidget->pRoomSnapshotSurface, &srcRect,
				pSurface, &srcRect);
		this->pRoomWidget->RenderRoomLayers(pSurface);
	} else {
		SDL_BlitSurface(pDestSurface, &srcRect, pSurface, &srcRect);
	}

	//Fade.
	{
		CFade fade(bFadeIn ? NULL : pSurface, bFadeIn ? pSurface : NULL);
		SDL_Surface *pFadingSurface = fade.GetDestSurface();
		const Uint32 dwStart = SDL_GetTicks();
		const Uint32 dwEnd = dwStart + dwDuration;
		Uint32 dwNow = dwStart;
		do
		{
			dwNow = SDL_GetTicks();
			fade.IncrementFade((dwNow - dwStart) / (float)dwDuration);
			SDL_BlitSurface(pFadingSurface, &srcRect, pDestSurface, &srcRect);
			PresentRect(pDestSurface, &srcRect);
		} while (dwNow < dwEnd);
	}
	SDL_FreeSurface(pSurface);
}

//*****************************************************************************
void CGameScreen::ScoreCheckpoint(const WCHAR* pScoreIDText)
//Displays score checkpoint stats and uploads the score.
{
	//Stats involved in score tallying.
	ASSERT(this->pCurrentGame);
	ASSERT(this->pCurrentGame->pPlayer);
	const PlayerStats& st = this->pCurrentGame->pPlayer->st;
	UINT dwTotalScore = this->pCurrentGame->GetScore();

/*
	wstrLevelStats += wszCRLF;

	this->pRoomWidget->AddLastLayerEffect(new CTextEffect(
		this->pRoomWidget, wstrLevelStats.c_str(), F_Stats, 5000, 8000, true));
*/

	SendAchievement(UnicodeToUTF8(pScoreIDText).c_str(), dwTotalScore);

	//Display.
	ShowScoreDialog(pScoreIDText, st);
}

void CGameScreen::ShowScoreDialog(const WSTRING pTitle, const PlayerStats& st)
//Displays score stats.
{
	WSTRING wstrLevelStats;

	UINT dwHP, dwATK, dwDEF, dwYKeys, dwGKeys, dwBKeys, dwSKeys, dwGOLD, dwXP;
	UINT dwHPScore, dwATKScore, dwDEFScore, dwYKeysScore, dwGKeysScore, dwBKeysScore, dwSKeysScore, dwGOLDScore, dwXPScore, dwTotalScore;

	//Stats involved in score tallying.
	ASSERT(this->pCurrentGame);
	dwHP = st.HP;
	dwATK = this->pCurrentGame->getPlayerATK();
	dwDEF = this->pCurrentGame->getPlayerDEF();
	dwYKeys = st.yellowKeys;
	dwGKeys = st.greenKeys;
	dwBKeys = st.blueKeys;
	dwSKeys = st.skeletonKeys;
	dwGOLD = st.GOLD;
	dwXP = st.XP;

	dwHPScore = CDbSavedGames::CalculateStatScore(dwHP, st.scoreHP);
	dwATKScore = CDbSavedGames::CalculateStatScore(dwATK, st.scoreATK);
	dwDEFScore = CDbSavedGames::CalculateStatScore(dwDEF, st.scoreDEF);
	dwGOLDScore = CDbSavedGames::CalculateStatScore(dwGOLD, st.scoreGOLD);
	dwXPScore = CDbSavedGames::CalculateStatScore(dwXP, st.scoreXP);
	dwYKeysScore = CDbSavedGames::CalculateStatScore(dwYKeys, st.scoreYellowKeys);
	dwGKeysScore = CDbSavedGames::CalculateStatScore(dwGKeys, st.scoreGreenKeys);
	dwBKeysScore = CDbSavedGames::CalculateStatScore(dwBKeys, st.scoreBlueKeys);
	dwSKeysScore = CDbSavedGames::CalculateStatScore(dwSKeys, st.scoreSkeletonKeys);
	dwTotalScore = this->pCurrentGame->GetScore();

	WCHAR temp[16];

	if (st.scoreHP != 0) wstrLevelStats += GetScoreCheckpointLine(MID_MonsterHP, dwHP, st.scoreHP, dwHPScore);
	if (st.scoreATK != 0) wstrLevelStats += GetScoreCheckpointLine(MID_ATKStat, dwATK, st.scoreATK, dwATKScore);
	if (st.scoreDEF != 0) wstrLevelStats += GetScoreCheckpointLine(MID_DEFStat, dwDEF, st.scoreDEF, dwDEFScore);
	if (st.scoreGOLD != 0) wstrLevelStats += GetScoreCheckpointLine(MID_GRStat, dwGOLD, st.scoreGOLD, dwGOLDScore);
	if (st.scoreXP != 0) wstrLevelStats += GetScoreCheckpointLine(MID_XPStat, dwXP, st.scoreXP, dwXPScore);
	if (st.scoreYellowKeys != 0) wstrLevelStats += GetScoreCheckpointLine(MID_YKEYStatFull, dwYKeys, st.scoreYellowKeys, dwYKeysScore);
	if (st.scoreGreenKeys != 0) wstrLevelStats += GetScoreCheckpointLine(MID_GKEYStatFull, dwGKeys, st.scoreGreenKeys, dwGKeysScore);
	if (st.scoreBlueKeys != 0) wstrLevelStats += GetScoreCheckpointLine(MID_BKEYStatFull, dwBKeys, st.scoreBlueKeys, dwBKeysScore);
	if (st.scoreSkeletonKeys != 0) wstrLevelStats += GetScoreCheckpointLine(MID_SKEYStatFull, dwSKeys, st.scoreSkeletonKeys, dwSKeysScore);

	//Set texts.
	CLabelWidget* pNameLabel = DYN_CAST(CLabelWidget*, CWidget*, this->pScoreDialog->GetWidget(TAG_SCORENAME));
	pNameLabel->SetText(pTitle.c_str());

	CLabelWidget* pTextLabel = DYN_CAST(CLabelWidget*, CWidget*, this->pScoreDialog->GetWidget(TAG_SCORETEXT));
	pTextLabel->SetText(wstrLevelStats.c_str());
	SDL_Rect rect;
	pTextLabel->GetRect(rect);
	UINT wTextHeight, wIgnored;
	g_pTheFM->GetTextRectHeight(FONTLIB::F_Message, wstrLevelStats.c_str(), rect.w, wIgnored, wTextHeight);
	pTextLabel->SetHeight(wTextHeight);

	CLabelWidget* pTotalLabel = DYN_CAST(CLabelWidget*, CWidget*, this->pScoreDialog->GetWidget(TAG_SCORETOTAL));
	pTotalLabel->Move(0, wTextHeight - CY_SPACE);

	CDialogWidget* pDialog = DYN_CAST(CDialogWidget*, CWidget*, GetWidget(TAG_SCOREDIALOG));
	CButtonWidget* pButton = DYN_CAST(CButtonWidget*, CWidget*, GetWidget(TAG_SCORE_OK));
	static const UINT CX_OK_BUTTON = 100;
	static const UINT CX_SCOREFRAME = pDialog->GetW() - CX_SPACE * 2;
	const UINT wButtonY = wTextHeight + pTotalLabel->GetH() - CY_SPACE;
	pButton->Move((CX_SCOREFRAME - CX_OK_BUTTON) / 2, wButtonY);

	static const UINT FRAME_BUFFER = 3;
	const UINT wTotalHeight = pNameLabel->GetH() + wTextHeight + pTotalLabel->GetH() + pButton->GetH() + FRAME_BUFFER * 2 + CY_SPACE * 2;
	pDialog->SetHeight(wTotalHeight);

	CWidget* pFrame = DYN_CAST(CWidget*, CWidget*, GetWidget(TAG_SCOREFRAME));
	const UINT wFrameHeight = wTextHeight + pTotalLabel->GetH() + pButton->GetH();
	pFrame->SetHeight(wFrameHeight);

	wstrLevelStats = g_pTheDB->GetMessageText(MID_Score);
	wstrLevelStats += wszSpace;
	wstrLevelStats += wszEqual;
	wstrLevelStats += wszSpace;
	wstrLevelStats += _itoW(dwTotalScore, temp, 10);

	pTotalLabel->SetText(wstrLevelStats.c_str());

	//Display.
	SetCursor();
	this->pScoreDialog->Center();
	this->pScoreDialog->Display();
	Paint();
}

//*****************************************************************************
WSTRING CGameScreen::GetScoreCheckpointLine(const MID_CONSTANT statName, const UINT statAmount, const int scoreMultiplier, const UINT statScore)
//Returns: A line for the score breakdown of a particular stat on the Score Checkpoint dialog
{
	WSTRING wstrText;
	WCHAR temp[16];

	wstrText += g_pTheDB->GetMessageText(statName);
	wstrText += wszColon;
	wstrText += wszSpace;
	wstrText += _itoW(statAmount, temp, 10);
	wstrText += wszSpace;
	wstrText += scoreMultiplier < 0 ? wszForwardSlash : wszAsterisk;
	wstrText += wszSpace;
	wstrText += _itoW(abs(scoreMultiplier), temp, 10);
	wstrText += wszSpace;
	wstrText += wszEqual;
	wstrText += wszSpace;
	wstrText += _itoW(statScore, temp, 10);
	wstrText += wszCRLF;

	return wstrText;
}

//*****************************************************************************
UINT CGameScreen::GetMessageAnswer(const CMonsterMessage *pMsg)
//Display a dialog box with a question and a menu of answer options.
//
//Returns: value of answer selected by user, TAG_UNDO_FROM_QUESTION to undo
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
			option != pCharacter->answerOptions.end(); ++option, ++count)
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
		wstr += this->pCurrentGame->ExpandText(pText, pCharacter); //resolve var refs
		pMenu->AddText(wstr.c_str(), command.x);
	}

	//Add interface to undo last move.
	WSTRING wstrUndo = wszAmpersand; //add # hotkey
	wstrUndo += _itoW(0, temp, 10);
	wstrUndo += wszPeriod;
	wstrUndo += wszSpace;
	wstrUndo += wszLeftParen;
	wstrUndo += g_pTheDB->GetMessageText(MID_Undo);
	wstrUndo += wszRightParen;
	pMenu->AddText(wstrUndo.c_str(), TAG_UNDO_FROM_QUESTION);

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
	this->pMenuDialog->Move(this->pRoomWidget->GetX() + (this->pRoomWidget->GetW() - this->pMenuDialog->GetW()) / 2,
			this->pMenuDialog->GetY());
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

//*****************************************************************************
UINT CGameScreen::GetEffectDuration(const UINT baseDuration) const
//Returns: duration of particles in particle effects
{
	//When player is hasted, particles move at half speed, so they last twice as long
	return this->pCurrentGame && this->pCurrentGame->pPlayer->IsHasted() ?
			baseDuration*2 : baseDuration;
}

//*****************************************************************************
UINT CGameScreen::GetParticleSpeed(const UINT baseSpeed) const
//Returns: speed of particles in particle effects
{
	//When player is hasted, particles move at half speed
	return this->pCurrentGame && this->pCurrentGame->pPlayer->IsHasted() ?
//			(baseSpeed > 1 ? baseSpeed/2 : 1) : baseSpeed;
			baseSpeed : baseSpeed * 2;
}

//*****************************************************************************
/*
void CGameScreen::HandleEventsForHoldExit()
//Display end hold stats until user presses a button.
{
	//Necessary to call explicitly because I am taking over event-handling.
	StopKeyRepeating();
	ClearEvents();

	UINT dwNow = SDL_GetTicks() - 100,  //first tick delayed, so start it now
			dwLastAnimate = dwNow;

	//Get any events waiting in the queue.
	//Finish on a key/mouse press.
	SDL_Event event;
	while (true)
	{
		while (PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_WINDOWEVENT:
					OnWindowEvent(event.window);
				break;

				case SDL_KEYDOWN:
				case SDL_MOUSEBUTTONDOWN:
				case SDL_QUIT:
				return;

				default: break;
			}
		}
		dwNow = SDL_GetTicks();

		//Animate every so often.
		if (dwNow - dwLastAnimate > 90)
		{
			this->pRoomWidget->pLastLayerEffects->UpdateAndDrawEffects();
			g_pTheBM->UpdateScreen(GetWidgetScreenSurface());
			dwLastAnimate = dwNow;
		}
		//Keep uploading demos.
		UploadDemoPolling();
	}
}
*/

//*****************************************************************************
SCREENTYPE CGameScreen::HandleEventsForLevelExit()
//Plays level exit music, shows player walking down the stairs, etc.
//Allow only certain key commands during this time.
//
//Note that the On*() handlers are not going to be called by CEventHandlerWidget's
//Activate() loop until after this method exits.  Events must be handled here.
//
//Returns:
//Screen to activate after this one.  SCR_None indicates an application exit,
//and SCR_Return indicates the screen previously activated.
{
	bool bDoneDescendingStairs = false;

	//Necessary to call explicitly because I am taking over event-handling.
	StopKeyRepeating();

	this->wRoomQuickExitDirection = NO_ORIENTATION; //stop any quick path to room exit

	//Play level exit music.
	CRoomScreen::SetMusicStyle(this->pCurrentGame->pRoom->style, SONG_EXIT, 1000);

	HideScroll();
	this->pFaceWidget->SetReading(false);
	this->pFaceWidget->SetMood(PlayerRole, Mood_Happy);
	this->pRoomWidget->AllowSleep(false);

	//Show the screen after first arriving here.
	Paint();
	UINT dwLastStep = SDL_GetTicks() - 100,  //first step's delayed, so take it a bit faster
			dwLastAnimate = dwLastStep;

	//Process events.
	SDL_Event event;
	SCREENTYPE eNextScreen;
	for (;;)
	{
		//Get any events waiting in the queue.
		while (PollEvent(&event))
		{
			switch (event.type)
			{
				case SDL_WINDOWEVENT:
					OnWindowEvent(event.window);
				break;

				case SDL_KEYDOWN:
					eNextScreen = LevelExit_OnKeydown(event.key);
					if (eNextScreen != SCR_Game)
						goto Done;
				break;

				case SDL_MOUSEBUTTONUP:	//not DOWN, so mouse up doesn't exit next screen immediately too
					eNextScreen = SCR_LevelStart;
					goto Done;
				break;

				case SDL_MOUSEMOTION:
					OnMouseMotion(GetTagNo(), event.motion);
				break;

				case SDL_QUIT:
				{
					this->bQuitPrompt = true;
					const UINT ret = ShowYesNoMessage(MID_ReallyQuit);
					this->bQuitPrompt = false;
					if (ret != TAG_NO)
						return SCR_None;
				}
				break;
			}
		}

		//Show player walking down stairs every 360ms.
		const Uint32 dwNow = SDL_GetTicks();
		if (dwNow - dwLastStep > 360)
		{
			if (bDoneDescendingStairs) {
				eNextScreen = SCR_LevelStart;
				goto Done;
			}

			this->pRoomWidget->DirtyRoom();
			this->pRoomWidget->AddPlayerLight(true);
			bDoneDescendingStairs = !this->pCurrentGame->WalkDownStairs();
			if (bDoneDescendingStairs)
			{
				//done walking down stairs (player disappears)
				this->pRoomWidget->HidePlayer();
				this->pRoomWidget->Paint();

				//Player is thinking about the next level now.
				this->pFaceWidget->SetMood(PlayerRole, Mood_Normal);
			}
			dwLastStep = dwNow;
		}

		//Animate every so often.
		if (dwNow - dwLastAnimate > 30)
		{
			this->pRoomWidget->Paint();
			this->pFaceWidget->Paint();
			dwLastAnimate = dwNow;
			g_pTheBM->UpdateRects(GetWidgetScreenSurface());
		}

		//Keep playing any remaining speech.
		ProcessSpeech();

		//Update music (switch song or continue music fade if one is in progress).
		g_pTheSound->UpdateMusic();

		//Keep uploading demos.
		UploadDemoPolling();

		SDL_Delay(1); //be nice to the CPU
	}

Done:
	this->pFaceWidget->SetMood(PlayerRole, Mood_Normal);
	g_pTheSound->StopSong();
	if (GetScreenType() == SCR_Game)	//don't redraw player at end of demo
		this->pRoomWidget->ShowPlayer();
	this->pRoomWidget->AllowSleep(true);

	return eNextScreen;
}

//*****************************************************************************
bool CGameScreen::HandleEventsForPlayerDeath(CCueEvents &CueEvents)
//Displays player death animation:
// wait a short period of time while the scream wave plays,
// a CBloodEffect spurts out of his body,
// and the face widget wags its tongue.
//End after the scream wave finishes.
//Accept no commands during this period.
//
//Note that the On*() handlers are not going to be called by CEventHandlerWidget's
//Activate() loop until after this method exits.  Events must be handled here.
//
//Returns: whether death was not undone
{
	static const Uint32 dwDeathDuration = 2000;

	//Necessary to call explicitly because I am taking over event-handling.
	StopKeyRepeating();

	//Stop currently playing speech line.
	//If player hits undo, then speech will continue at the next line.
	CutSpeech();

	//Shows current stats.
	RedrawStats(this->pCurrentGame->pCombat, true);

	this->wRoomQuickExitDirection = NO_ORIENTATION; //stop any quick path to room exit

	const CSwordsman& player = *(this->pCurrentGame->pPlayer);
	const UINT wOrigO = player.wO;	//save value for demo record
	bool bSwordSwingsClockwise = true;
	bool bUndoDeath = false;

	const bool bPlayerFellInPit = CueEvents.HasOccurred(CID_PlayerFellIntoPit);
	const bool bExplosionKilledPlayer = CueEvents.HasOccurred(CID_ExplosionKilledPlayer);
	const bool bPlayerDrowned = CueEvents.HasOccurred(CID_PlayerDrownedInWater);
	const bool bPlayerDied = CueEvents.HasOccurred(CID_MonsterKilledPlayer) ||
			CueEvents.HasOccurred(CID_PlayerBurned) ||
			CueEvents.HasOccurred(CID_BriarKilledPlayer) ||
			bExplosionKilledPlayer || bPlayerFellInPit || bPlayerDrowned;
//	const bool bNPCBeethroDied = CueEvents.HasOccurred(CID_NPCBeethroDied);
	const bool bCriticalNPCDied = CueEvents.HasOccurred(CID_CriticalNPCDied);
//			|| (!bPlayerDied && !bNPCBeethroDied && player.wAppearance != M_BEETHRO);

	ProcessFuseBurningEvents(CueEvents);

	//Need player falling to draw last (on top of) other effects migrated below to the m-layer
	if (bPlayerFellInPit && bPlayerDied) {
		this->pRoomWidget->HidePlayer();

		UINT wO, wFrame, wSManTI, wSwordTI;
		if (this->pRoomWidget->GetPlayerDisplayTiles(player, wO, wFrame, wSManTI, wSwordTI)) {
			const UINT fallTime = player.IsHasted() ? 260 : 130;
			if (wSManTI != TI_UNSPECIFIED) {
				const vector<UINT> playerTile(1, wSManTI);
				this->pRoomWidget->AddMLayerEffect(
						new CTrapdoorFallEffect(this->pRoomWidget, player, playerTile, fallTime));
			}
			if (wSwordTI != TI_UNSPECIFIED) {
				const CCoord sword(player.GetSwordX(), player.GetSwordY());
				if (this->pRoomWidget->pRoom->IsValidColRow(sword.wX, sword.wY)) {
					const vector<UINT> swordTile(1, wSwordTI);
					this->pRoomWidget->AddMLayerEffect(
							new CTrapdoorFallEffect(this->pRoomWidget, sword, swordTile, fallTime));
				}
			}
		}
	}

	//Show the screen after first arriving here.
	this->pFaceWidget->SetReading(false);
	UpdatePlayerFace();
//	this->pRoomWidget->RemoveMLayerEffectsOfType(EPENDINGBUILD); //stop showing where pending building was
	this->pRoomWidget->RemoveHighlight();
	this->pRoomWidget->AllowSleep(false);
	this->pRoomWidget->Paint();

	bool bNonMonsterDeath = false;
//	CMonster *pNPCBeethro = bNPCBeethroDied ? this->pCurrentGame->pRoom->GetNPCBeethro() : NULL;
	if (bCriticalNPCDied)
		this->pFaceWidget->SetDying(true, Mood_Nervous);
	else {
		this->pFaceWidget->SetDying(true, Mood_Dying);
		UINT eSoundID;
		switch (player.wAppearance)
		{
			case M_BEETHRO: case M_BEETHRO_IN_DISGUISE: eSoundID = SEID_DIE; break;
			case M_NEATHER:
			case M_HALPH: eSoundID = SEID_HALPH_DIE; break;
			case M_GOBLIN:	case M_GOBLINKING: eSoundID = SEID_GOB_DIE; break;
			case M_TARBABY: case M_MUDBABY: case M_GELBABY:
			case M_TARMOTHER: case M_MUDMOTHER: case M_GELMOTHER:
				eSoundID = SEID_SPLAT; break;
			case M_ROCKGOLEM: case M_ROCKGIANT: eSoundID = SEID_ROCK_DIE; break;
			case M_CLONE: case M_DECOY: case M_MIMIC: case M_GUARD: case M_PIRATE:
			case M_CITIZEN1: case M_CITIZEN2:
			case M_MUDCOORDINATOR: case M_TARTECHNICIAN:
			case M_CITIZEN: eSoundID = SEID_CIT_DIE; break;
			case M_NEGOTIATOR: case M_INSTRUCTOR:
			case M_CITIZEN3: case M_CITIZEN4:
				eSoundID = SEID_WOM_DIE; break;
			case M_STALWART: eSoundID = SEID_STALWART_DIE; break;
			case M_WUBBA: eSoundID = SEID_WUBBA; break;
			case M_SLAYER: eSoundID = SEID_SLAYERDIE; break;
			default: eSoundID = SEID_MON_OOF; break;
		}
		PlaySoundEffect(eSoundID);

		if (bPlayerDied) // || bNPCBeethroDied)
		{
			//If Slayer killed the player, he says something.
			const CMonster *pMonster = DYN_CAST(const CMonster*, const CAttachableObject*,
					 CueEvents.GetFirstPrivateData(CID_MonsterKilledPlayer)); /*bPlayerDied ?
							CID_MonsterKilledPlayer : CID_NPCBeethroDied));*/
			if (pMonster && pMonster->wType == M_SLAYER)
					 PlaySoundEffect(SEID_SLAYERKILL);
		}
		else	//killed by some other room element.  Handle a bit differently.
			bNonMonsterDeath = true;
	}

	//These events can be played if triggered regardless of who's dying.
	if (CueEvents.HasOccurred(CID_PlayerBurned))
	{
		PlaySoundEffect(SEID_SIZZLE);
		this->pRoomWidget->AddMLayerEffect(
				new CSteamEffect(this->pRoomWidget, player));
	}

	UINT wSX=0, wSY=0;
	if (bPlayerDied)
	{
		if (!this->pCurrentGame->GetSwordsman(wSX, wSY))
		{
			wSX = player.wX;
			wSY = player.wY;
		}
	}
/*
	else if (bNPCBeethroDied)
	{
		ASSERT(pNPCBeethro);
		wSX = pNPCBeethro->wX;
		wSY = pNPCBeethro->wY;
	}
*/
	const CMoveCoord coord(wSX, wSY, NO_ORIENTATION);

	const bool bPlayerSwordWobblesAround = !bCriticalNPCDied && !bPlayerFellInPit;

	UINT dwStart = SDL_GetTicks();
	Uint32 dwLastFade = 0, dwLastSwordWobble = 0, dwLastMonsterChomp = 0;
	SDL_Event event;
	while (true)
	{
		//Get any events waiting in the queue.
		while (PollEvent(&event))
		{
         switch (event.type)
			{
				case SDL_WINDOWEVENT:
					OnWindowEvent(event.window);
				break;
				case SDL_KEYDOWN:
				{
					const int nCommand = GetCommandForKeysym(event.key.keysym.sym);
					if (nCommand == CMD_UNDO)
						bUndoDeath = true;
					else if (nCommand == CMD_RESTART)
						dwStart = 0; //finish effect now
				}
				break;
				default: break;
			}
		}

		//Animate as fast as possible.
		const Uint32 dwNow = SDL_GetTicks();

		//Sword wobbles around.
		if (bPlayerSwordWobblesAround && dwNow - dwLastSwordWobble > 100)
		{
/*
			if (pNPCBeethro)
				pNPCBeethro->wO = bSwordSwingsClockwise ?
							nNextCO(pNPCBeethro->wO) : nNextCCO(pNPCBeethro->wO);
			else
*/
			this->pCurrentGame->pPlayer->SetOrientation(
					bSwordSwingsClockwise || bNonMonsterDeath ?
							nNextCO(player.wO) : nNextCCO(player.wO));
			//sometimes sword rotation changes direction
			if (RAND(3) == 0)
				bSwordSwingsClockwise = !bSwordSwingsClockwise;
			dwLastSwordWobble = dwNow;
		}

		//Fade to black.
		if (g_pTheBM->bAlpha && dwStart)
		{
			const float durationFraction = min(1.0f, (dwNow - dwStart) / (float)dwDeathDuration);
			const float remainingFraction = 1 - durationFraction;

			this->pRoomWidget->SetDeathFadeOpacity(durationFraction);
			this->pRoomWidget->pMLayerEffects->SetOpacityForEffects(remainingFraction);
			this->pRoomWidget->pLastLayerEffects->SetOpacityForEffects(remainingFraction);
			this->pRoomWidget->DirtyRoom();  //repaint whole room each fade

			dwLastFade = dwNow;
		}

		if (bPlayerFellInPit || /*bPlayerImpaledOnSpikes ||*/ bExplosionKilledPlayer) {
			//no other effects
		} else if (bPlayerDrowned) {
			//Drowning effect.
			if (dwNow - dwLastMonsterChomp > 400) {
				dwLastMonsterChomp = dwNow;
				this->pRoomWidget->AddMLayerEffect(
						new CSplashEffect(this->pRoomWidget, player));
			}
		} else if ((bPlayerDied /*|| bNPCBeethroDied*/) &&
				this->pRoomWidget->pRoom->IsValidColRow(wSX, wSY))
		{
			//Monster chomps on the player.
			if (dwNow - dwLastMonsterChomp > 200)
			{
				//Get killing monster tile.
				UINT wMX = wSX, wMY = wSY;
				CMonster *pMonster = this->pRoomWidget->pRoom->GetMonsterAtSquare(wSX, wSY);
				if (pMonster && pMonster->IsPiece())
				{
					pMonster = pMonster->GetOwningMonster();
					wMX = pMonster->wX;
					wMY = pMonster->wY;
				}

				//Animate killing monster.
				if ((this->pRoomWidget->AdvanceAnimationFrame(wMX, wMY) % 2) == 0)
				{
					//Blood effect.
					CEffect *pEffect = new CBloodEffect(this->pRoomWidget, coord, 16,
							GetEffectDuration(7), GetParticleSpeed(3));
					if (pEffect)
						this->pRoomWidget->AddMLayerEffect(pEffect); //must go on top to show up
				}
				dwLastMonsterChomp = dwNow;
			}
		}

		this->pFaceWidget->Paint();
		this->pRoomWidget->Paint();

		g_pTheBM->UpdateRects(GetWidgetScreenSurface());

		//Scream has finished.  Return from animation.
		if (dwNow - dwStart > dwDeathDuration || bUndoDeath)
			break;
	}
	
	this->pRoomWidget->SetDeathFadeOpacity(0);
	this->pRoomWidget->pMLayerEffects->SetOpacityForEffects(1);
	this->pRoomWidget->pLastLayerEffects->SetOpacityForEffects(1);
	this->pCurrentGame->pPlayer->SetOrientation(wOrigO);	//restore value

	if (bPlayerFellInPit)
		this->pRoomWidget->ShowPlayer();

	if (bUndoDeath)
	{
		this->pRoomWidget->DirtyRoom();	//remove fading
		UndoMove();
		return false;
	}

	StopAmbientSounds();
	ClearSpeech();
	this->pFaceWidget->SetMood(PlayerRole, Mood_Normal);
	this->pFaceWidget->SetDying(false);
	return true;
}

//*****************************************************************************
bool CGameScreen::ProcessCommandWrapper(const int nCommand)
//Processes the indicated game command.
//Checks whether game screen should be exited as a result of processing this command.
//
//Returns: false if game screen is being exited, otherwise true
{
	const SCREENTYPE eNextScreen = ProcessCommand(nCommand);
	if (eNextScreen != SCR_Game)
	{
		if (IsDeactivating())
			SetDestScreenType(eNextScreen); //override any other specified destination screen
		else
			GoToScreen(eNextScreen);
		return false;
	}
	return true;
}

//*****************************************************************************
SCREENTYPE CGameScreen::ProcessCommand(
//Processes game command, making calls to update game data and respond to cue
//events.  If movement macro is requested, perform move combination if
//room-ending events don't occur.
//
//Returns:
//Screen to go to next or SCR_Game to remain at game screen.
//
//Params:
	const int nCommand)
{
	//If question prompts have been delayed until after the transition to the
	//game screen, or combat, has completed, then forbid the user to input commands before
	//the event engine has gotten around to prompting the user with these questions.
	if (this->bNeedToProcessDelayedQuestions && nCommand != CMD_ADVANCE_COMBAT)
		return SCREENTYPE(GetScreenType());

	//When player inputs a command, remove potentially obscuring tool tips from the screen.
	//Do this before changing game state below.
	RemoveToolTip();

	bool bLeftRoom = false, bPlayerDied = false;
	UINT wNewLightLevel;
	bool bFadeLight = false;

	ASSERT(nCommand != CMD_UNSPECIFIED);
	ASSERT(nCommand < COMMAND_COUNT || nCommand == CMD_ADVANCE_CUTSCENE ||
			nCommand == CMD_ADVANCE_COMBAT || nCommand == CMD_BATTLE_KEY);
	switch (nCommand)
	{
		case CMD_RESTART: //case CMD_RESTART_PARTIAL: case CMD_RESTART_FULL:
			//Rewind moves to previous checkpoints or restart the room.
			g_pTheSound->StopAllSoundEffects(); //stop any game sounds that were playing
			RestartRoom(nCommand, this->sCueEvents);

			if (GetScreenType() == SCR_Demo)
				return SCR_Game; //can skip everything below
		break;

		case CMD_UNDO:
			UndoMove();
		return SCR_Game;	//everything below has already been handled in UndoMove

		case CMD_ANSWER:
//		case CMD_CLONE: case CMD_DOUBLE:
			//was already handled in the calling ProcessCommand
//			this->bUndoJustMade = false;
		break;

		case CMD_BATTLE_KEY:
			ShowMonsterStats(this->pCurrentGame->pRoom, this->pRoomWidget);
		return SCR_Game;

		default:
		{
			//Send command to current game and cue events list back.
			if (!this->pCurrentGame->bIsGameActive)
				break;

			ClearCueEvents();

			//Force the room to finish rendering any previous move
			//before this one is made for smoother motion.
			if (this->pRoomWidget->IsMoveAnimating())
			{
				this->pRoomWidget->FinishMoveAnimation();
				this->pRoomWidget->Paint();
				g_pTheBM->UpdateRects(GetWidgetScreenSurface());
			}

			const bool bWasCutScene = this->pCurrentGame->dwCutScene != 0;
			UINT wPrevTurnNo = this->pCurrentGame->wTurnNo;
			this->pCurrentGame->ProcessCommand(nCommand, this->sCueEvents);

			//Undo handling:
			//1. Don't modify undo turn during cut scenes to enable
			//a single undo skipping back past the entire cut scene.
			if (!bWasCutScene)
				this->wUndoToTurn = wPrevTurnNo;

			bLeftRoom = this->sCueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerLeftRoom), CIDA_PlayerLeftRoom);
			if (bLeftRoom)
			{
				//If light level is changing, save value for a light fade below.
				bPlayerDied = this->sCueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied);
				if (!bPlayerDied && !this->sCueEvents.HasOccurred(CID_ExitLevelPending) &&
						this->pCurrentGame->bIsGameActive &&
						this->pRoomWidget->IsLightingRendered() &&
						!this->pCurrentGame->pRoom->weather.bSkipLightfade)
				{
					bFadeLight = true;
					wNewLightLevel = this->pCurrentGame->pRoom->weather.wLight;
					this->pCurrentGame->pRoom->weather.wLight = this->pRoomWidget->wDark;
				}
				this->pRoomWidget->ResetForPaint();
 				this->pRoomWidget->ClearEffects();

/*
				if (!bPlayerDied)
					this->bRoomClearedOnce = false;
				//else if "leaving" is actually death, don't reset this yet in case death is undone
*/
				//Reset any simulated combat, as pointers might be left dangling.
				delete g_pPredictedCombat;
				g_pPredictedCombat = NULL;

				this->wRoomQuickExitDirection = NO_ORIENTATION;
			}
			else if (nCommand < CMD_C || nCommand == CMD_NW)
			{
				//If moving, stop showing any effects still fading from previous turns
				this->pRoomWidget->RemoveMLayerEffectsOfType(ESWORDSWING); //sword rotations
				this->pRoomWidget->RemoveMLayerEffectsOfType(EFADETILE); //no damage shield
			}

			if (this->sCueEvents.HasOccurred(CID_InvalidAttackMove))
			{
				//This move was just undone.
				//Speech pointers need to be relinked and room state refreshed.
				this->pRoomWidget->ResetForPaint();
 				this->pRoomWidget->ClearEffects();
				ClearSpeech(false);  //retain speech that started before the previous turn
				ReattachRetainedSubtitles(); //after clearing effects, rejoin subtitles to current room objects
				RetainSubtitleCleanup();

				//Must reset any temp combat display because monster pointers have been regenerated.
				delete g_pPredictedCombat;
				g_pPredictedCombat = NULL;
			}
		}
		break;
	}

	if (!this->sCueEvents.HasOccurred(CID_ExitLevelPending) &&
			!this->sCueEvents.HasOccurred(CID_ExitRoomPending) &&
			!this->sCueEvents.HasOccurred(CID_ExitRoom))
		UpdateSound();
	const Uint32 dwNow = SDL_GetTicks();
	this->pCurrentGame->UpdateTime(dwNow);

	//Process cue events list to create effects that should occur before
	//room is drawn.  Might alter cue events when going to a new screen.
	this->bPersistentEventsDrawn = false;
	bPlayerDied = this->sCueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied);
	const bool bPlayingVideo = this->sCueEvents.HasOccurred(CID_PlayVideo);
 	SCREENTYPE eNextScreen = ProcessCueEventsBeforeRoomDraw(this->sCueEvents);
	if (eNextScreen == SCR_Game)
	{
		//Redraw the room.
		SetGameAmbience();

		if (!bPlayingVideo)
			this->pRoomWidget->Paint();
		else if (!bLeftRoom) //after a video, only redraw the room (and screen)
			Paint(); //when the room is not exited upon video's completion

		//Process cue events list to create effects that should occur after
		//room is drawn.
		eNextScreen = ProcessCueEventsAfterRoomDraw(this->sCueEvents);

/*
		PaintClock();
		if (bLeftRoom && !this->bIsScrollVisible)
			this->pClockWidget->Paint();	//new clock should be shown right now
*/
		if (bPlayerDied)
		{
			SwirlEffect(); //must call after ProcessCueEventsBeforeRoomDraw()
			ProcessSpeechCues(this->sCueEvents); //start showing speech at room start
		}

		//Option to log vars each time a new room is entered.
		if (bLeftRoom && this->pCurrentGame && this->pCurrentGame->bIsGameActive)
		{
			string str;
			if (CFiles::GetGameProfileString(INISection::Customizing, INIKey::LogVars, str))
				if (atoi(str.c_str()) > 0)
					if (g_pTheDB->Holds.PlayerCanEditHold(this->pCurrentGame->pHold->dwHoldID))
						LogHoldVars();
		}
	}

	//Changing to a new light level.
	if (bFadeLight)
	{
		this->pCurrentGame->pRoom->weather.wLight = wNewLightLevel;
		this->pRoomWidget->FadeToLightLevel(wNewLightLevel);
	}

	return eNextScreen;
}

//*****************************************************************************
SCREENTYPE CGameScreen::ProcessCommand(
//Processes game command, making calls to update game data and respond to cue
//events.
//
//Returns:
//Screen to go to next or SCR_Game to remain at game screen.
//
//Params:
	const int nCommand, const UINT wX, const UINT wY)	//(in)
{
	ASSERT(nCommand != CMD_UNSPECIFIED);
	ASSERT(nCommand < COMMAND_COUNT);
	this->pCurrentGame->ProcessCommand(nCommand, this->sCueEvents, wX, wY);	//back-end logic
/*
	if (nCommand == CMD_CLONE)
		SwirlEffect();
*/
	return ProcessCommand(CMD_ANSWER);	//use CMD_ANSWER to handle the front-end stuff here w/o executing another command
}

//*****************************************************************************
SCREENTYPE CGameScreen::ProcessCueEventsBeforeRoomDraw(
//Process cue events list to create effects that should occur before
//room is drawn.
//
//Params:
	CCueEvents &CueEvents) //(in)
//
//Returns:
//Screen to go to next or SCR_Game to remain at game screen.
{
	SCREENTYPE eNextScreen = SCR_Game;

	if (this->pCurrentGame->wTurnNo != this->pRoomWidget->wLastTurn)
		this->pRoomWidget->ResetJitter(); //call before a new turn begins

	//Remember for later if player left room (dying, level exit, room exit,
	//win game) because room reloading actions will erase cue events.
	const bool bPlayerLeftRoom = CueEvents.HasAnyOccurred(
			IDCOUNT(CIDA_PlayerLeftRoom), CIDA_PlayerLeftRoom);
	const CAttachableObject *pObj;
	CSwordsman& player = *this->pCurrentGame->pPlayer;

	ProcessMovieEvents(CueEvents);

	if (!bPlayerLeftRoom)
		ProcessSpeechCues(CueEvents);   //can be done here only when no ClearSpeech calls will be made below

/*
	//Notify player when they can't exit the room because they have temporarily locked exit.
	if (CueEvents.HasOccurred(CID_RoomExitLocked))
	{
		g_pTheSound->PlaySoundEffect(SEID_WISP);
		this->pRoomWidget->DisplaySubtitle(g_pTheDB->GetMessageText(MID_RoomLockEnabled),
				player.wX, player.wY, true);
	}
*/

	//Handle private sound channels.
	//Channel n+1 -- Swordsman's voice.
	this->pRoomWidget->RemoveMLayerEffectsOfType(EBUMPOBSTACLE);
	if (CueEvents.HasOccurred(CID_HitObstacle))
	{
		const CMoveCoord *pMoveCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*,
				CueEvents.GetFirstPrivateData(CID_HitObstacle) );
		ASSERT(pMoveCoord);
		PlayHitObstacleSound(player.wAppearance, CueEvents);
		this->pRoomWidget->AddMLayerEffect(new CBumpObstacleEffect(this->pRoomWidget,
				pMoveCoord->wX, pMoveCoord->wY, pMoveCoord->wO));
	}
	else if (CueEvents.HasOccurred(CID_Scared))
	{
		UINT eSoundID = SEID_NONE;
		switch (player.wAppearance)
		{
			case M_BEETHRO: case M_BEETHRO_IN_DISGUISE: eSoundID = SEID_SCARED; break;
			case M_NEATHER:
			case M_HALPH: eSoundID = SEID_HALPH_SCARED; break;
			case M_GOBLIN:	case M_GOBLINKING: eSoundID = SEID_GOB_SCARED; break;
			case M_TARBABY: case M_MUDBABY: case M_GELBABY:
			case M_TARMOTHER: case M_MUDMOTHER: case M_GELMOTHER:
				eSoundID = SEID_TAR_SCARED; break;
			case M_ROCKGOLEM: case M_ROCKGIANT: eSoundID = SEID_ROCK_SCARED; break;
			case M_CLONE: case M_DECOY: case M_MIMIC: case M_GUARD: case M_PIRATE:
			case M_CITIZEN1: case M_CITIZEN2:
			case M_MUDCOORDINATOR: case M_TARTECHNICIAN:
			case M_CITIZEN: eSoundID = SEID_CIT_SCARED; break;
			case M_STALWART: eSoundID = SEID_STALWART_SCARED; break;
			case M_WUBBA: eSoundID = SEID_WUBBA; break;
			case M_NEGOTIATOR: case M_INSTRUCTOR:
			case M_CITIZEN3: case M_CITIZEN4:
				eSoundID = SEID_WOM_SCARED; break;
			case M_SLAYER: eSoundID = SEID_SLAYER_SCARED; break;
			default: eSoundID = SEID_MON_OOF; break;
		}
		if (eSoundID != (UINT)SEID_NONE &&
				!g_pTheSound->IsSoundEffectPlaying(eSoundID))
			PlaySoundEffect(eSoundID);
	}

	if (CueEvents.HasOccurred(CID_SwordsmanTired))
	{
		if (bIsHuman(player.wAppearance))
			PlaySoundEffect(SEID_TIRED);
	}

	//Handle dynamically-allocated sound channels -- give most important sounds priority.

	//1st. Player's actions.
	if (CueEvents.HasOccurred(CID_SwingSword))
	{
		g_pTheSound->PlaySoundEffect(SEID_SWING);
		const CAttachableWrapper<UINT> *pOrientation =
				DYN_CAST(const CAttachableWrapper<UINT>*, const CAttachableObject*,
				CueEvents.GetFirstPrivateData(CID_SwingSword));
		ASSERT(pOrientation);
		this->pRoomWidget->AddMLayerEffect(
				new CSwordSwingEffect(this->pRoomWidget,
					player, static_cast<UINT>(*pOrientation)));
	}
	if (CueEvents.HasOccurred(CID_Step))
		g_pTheSound->PlaySoundEffect(SEID_WALK);
	if (CueEvents.HasOccurred(CID_Jump))
		g_pTheSound->PlaySoundEffect(SEID_JUMP);
	if (CueEvents.HasOccurred(CID_Swim))
	{
		g_pTheSound->PlaySoundEffect(SEID_WATERSTEP);

		pObj = CueEvents.GetFirstPrivateData(CID_Swim);
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
			this->pRoomWidget->AddTLayerEffect(
					new CSplashEffect(this->pRoomWidget, *pCoord));
	}

	if (CueEvents.HasOccurred(CID_StepOnScroll))
		g_pTheSound->PlaySoundEffect(SEID_READ);
	if (CueEvents.HasOccurred(CID_AccessoryUsed))
		g_pTheSound->PlaySoundEffect(SEID_MIMIC);
	if (CueEvents.HasOccurred(CID_CantUseAccessory))
		g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);
	if (CueEvents.HasOccurred(CID_DrankPotion))
		g_pTheSound->PlaySoundEffect(SEID_POTION);
	if (CueEvents.HasOccurred(CID_ReceivedATK))
		g_pTheSound->PlaySoundEffect(SEID_ATK_PICKUP);
	if (CueEvents.HasOccurred(CID_ReceivedDEF))
		g_pTheSound->PlaySoundEffect(SEID_DEF_PICKUP);
	if (CueEvents.HasOccurred(CID_ReceivedHP))
		g_pTheSound->PlaySoundEffect(SEID_HP_PICKUP);
	if (CueEvents.HasOccurred(CID_CompleteLevel))
	{
		this->pMapWidget->UpdateFromCurrentGame();
		this->pMapWidget->RequestPaint();
	}
	if (CueEvents.HasOccurred(CID_LevelMap))
	{
		this->pMapWidget->UpdateFromCurrentGame();
		this->pMapWidget->RequestPaint();
		g_pTheSound->PlaySoundEffect(SEID_READ);

		//Show map going to minimap.
		CWidget *pMapWidget = GetWidget(TAG_MAP);
		if (pMapWidget)
		{
			const CAttachableWrapper<UINT>* pMapType =
				DYN_CAST(const CAttachableWrapper<UINT>*, const CAttachableObject*,
					CueEvents.GetFirstPrivateData(CID_LevelMap));

			const UINT tMapTile = GetTileImageForTileNo(pMapType ? pMapType->data : T_MAP);
			ASSERT(tMapTile != CALC_NEEDED);
			CMovingTileEffect *pEffect = new CMovingTileEffect(this->pRoomWidget,
					tMapTile, CCoord(player.wX, player.wY),
					CCoord(pMapWidget->GetX(), pMapWidget->GetY()), 50.0);
			this->pRoomWidget->AddLastLayerEffect(pEffect); //add on top of game screen
		}
	}

	if (CueEvents.HasOccurred(CID_MoneyDoorOpened) || CueEvents.HasOccurred(CID_MoneyDoorLocked))
		g_pTheSound->PlaySoundEffect(SEID_ORBHIT); //!!new sound
	if (CueEvents.HasOccurred(CID_KnockOpenedDoor) || CueEvents.HasOccurred(CID_DoorLocked))
		g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR); //!!new sound

	if (CueEvents.HasOccurred(CID_PortableOrbActivated)) {
		g_pTheSound->PlaySoundEffect(SEID_ORBHIT);
	}
	else if (CueEvents.HasOccurred(CID_OrbActivatedByPlayer))
	{
		pObj = CueEvents.GetFirstPrivateData(CID_OrbActivatedByPlayer);
		if (!pObj)
			g_pTheSound->PlaySoundEffect(SEID_ORBHIT);   //just play sound
		else while (pObj)
		{
			const COrbData *pOrbData = DYN_CAST(const COrbData*, const CAttachableObject*, pObj);
			//If player hits orb, center sound on player for nicer effect.
			if (player.GetSwordX() == pOrbData->wX && player.GetSwordY() == pOrbData->wY)
			{
				this->fPos[0] = static_cast<float>(player.wX);
				this->fPos[1] = static_cast<float>(player.wY);
			} else {
				this->fPos[0] = static_cast<float>(pOrbData->wX);
				this->fPos[1] = static_cast<float>(pOrbData->wY);
			}
			g_pTheSound->PlaySoundEffect(pOrbData->eType == OT_BROKEN ?
					SEID_ORBBROKE : SEID_ORBHIT, this->fPos);
			this->pRoomWidget->AddStrikeOrbEffect(*pOrbData);
			pObj = CueEvents.GetNextPrivateData();
		}
	}
	if (CueEvents.HasOccurred(CID_Tunnel))
		g_pTheSound->PlaySoundEffect(SEID_TUNNEL);

	//2nd. Important room events.
	if (CueEvents.HasOccurred(CID_RedGatesToggled) ||
//			CueEvents.HasOccurred(CID_AllMonstersKilled) ||
			CueEvents.HasOccurred(CID_BlackGatesToggled))
		g_pTheSound->PlaySoundEffect(SEID_DOOROPEN);

/*
	static bool bWispOnPlayer = false;
	if (CueEvents.HasOccurred(CID_WispOnPlayer))
	{
		//Sound wisp only on initial contact.
		if (!bWispOnPlayer)
		{
			g_pTheSound->PlaySoundEffect(SEID_WISP);
			bWispOnPlayer = true;
		}
	}
	else bWispOnPlayer = false;
*/

	if (CueEvents.HasOccurred(CID_AllBrainsRemoved))
		g_pTheSound->PlaySoundEffect(SEID_LASTBRAIN);

	if (CueEvents.HasOccurred(CID_PlayerFrozen))
	{
		pObj = CueEvents.GetFirstPrivateData(CID_PlayerFrozen);
		ASSERT(pObj);

		//Several of these could overlap.
		//Don't allow placing more than one sound per tile.
		CCoordSet coords;
		while (pObj)
		{
			const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
			coords.insert(pCoord->wX, pCoord->wY);
			pObj = CueEvents.GetNextPrivateData();
		}

		bool bFreezeOneNPC = true;
		for (CCoordSet::const_iterator coord=coords.begin(); coord!=coords.end(); ++coord)
		{
			//To prevent slowdown, only play one sound at a non-player position.
			if (!this->pCurrentGame->IsPlayerAt(coord->wX, coord->wY))
			{
				if (!bFreezeOneNPC)
					continue;
				bFreezeOneNPC = false;
			}

			this->fPos[0] = static_cast<float>(coord->wX);
			this->fPos[1] = static_cast<float>(coord->wY);
			g_pTheSound->PlaySoundEffect(SEID_FROZEN, this->fPos);
		}
	}

	//3nd. Player actions (possibly multiple instances).
	if (CueEvents.HasOccurred(CID_TokenToggled))
		g_pTheSound->PlaySoundEffect(SEID_MIMIC);   //SEID_TOKEN

	if (CueEvents.HasOccurred(CID_MonsterEngaged))
	{
		if (this->pCurrentGame->pCombat)
		{
			//If a monster can't be harmed, report on it
			CMonster *pMonster = this->pCurrentGame->pCombat->PlayerCantHarmAQueuedMonster();
			if (pMonster)
			{
				static CMoveCoord coord;
				coord = *(pMonster);
				this->pRoomWidget->AddInfoSubtitle(&coord, g_pTheDB->GetMessageText(MID_CantHarmEnemy), 1000);
			}
		}
	}

	if (CueEvents.HasOccurred(CID_ItemUsed))
	{
		//Effect showing item being used.
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*,
				CueEvents.GetFirstPrivateData(CID_ItemUsed));
		UINT tTile = 0;
		const UINT item = pCoord->wO;
		switch (item)
		{
			case YellowKey: case GreenKey: case BlueKey:
//we'll try showing this for now:				//Don't show anything for normal key usage.
				tTile = CalcTileImageForKey(item);
			break;
			case SkeletonKey:
				//Show special item usage.
				tTile = CalcTileImageForKey(item);
			break;
			default: break;
		}
		if (tTile)
		{
			UINT destX = this->pRoomWidget->GetX() + pCoord->wX * g_pTheBM->CX_TILE;
			UINT destY = this->pRoomWidget->GetY() + pCoord->wY * g_pTheBM->CY_TILE;
			if (pCoord->wX == player.wX && pCoord->wY == player.wY)
				destY += g_pTheBM->CY_TILE; //show item going down one tile
			CMovingTileEffect *pEffect = new CMovingTileEffect(this->pRoomWidget,
					tTile, CCoord(player.wX, player.wY),
					CCoord(destX, destY), 250.0);
			pEffect->behavior = CMovingTileEffect::UniformSpeed;
			this->pRoomWidget->AddLastLayerEffect(pEffect);
		}
	}

	if (CueEvents.HasOccurred(CID_ReceivedKey))
	{
		g_pTheSound->PlaySoundEffect(SEID_KEY);

		//Effect showing key entering player inventory.
		const CAttachableWrapper<BYTE> *pKeyType =
				DYN_CAST(const CAttachableWrapper<BYTE>*, const CAttachableObject*,
				CueEvents.GetFirstPrivateData(CID_ReceivedKey));

		//Get destination of effect.
		UINT destX = 0, destY = 0, keyTag = 0;
		switch (pKeyType->data)
		{
			case YellowKey: keyTag = TAG_YKEY; break;
			case GreenKey: keyTag = TAG_GKEY; break;
			case BlueKey: keyTag = TAG_BKEY; break;
			case SkeletonKey: keyTag = TAG_SKEY; break;
			default: break;
		}
		CLabelWidget *pLabel = DYN_CAST(CLabelWidget*, CWidget*, GetWidget(keyTag));
		if (pLabel)
		{
			destX = pLabel->GetX();
			destY = pLabel->GetY();

			const UINT tKeyTile = CalcTileImageForKey(pKeyType->data);
			CMovingTileEffect *pEffect = new CMovingTileEffect(this->pRoomWidget,
					tKeyTile, CCoord(player.wX, player.wY),
					CCoord(destX, destY), 50.0);
			this->pRoomWidget->AddLastLayerEffect(pEffect);
		}
	}

	if (CueEvents.HasOccurred(CID_ReceivedEquipment))
		g_pTheSound->PlaySoundEffect(SEID_SWORDS);

	for (pObj = CueEvents.GetFirstPrivateData(CID_MonsterDiedFromStab);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMonster *pMonster = DYN_CAST(const CMonster*, const CAttachableObject*, pObj);
		const UINT wO = IsValidOrientation(pMonster->wProcessSequence) ?
				pMonster->wProcessSequence : NO_ORIENTATION;
		const CMoveCoord coord(pMonster->wX,pMonster->wY,wO);
		AddKillEffect(pMonster->GetIdentity(), coord);
	}

	for (pObj = CueEvents.GetFirstPrivateData(CID_TrapDoorRemoved);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
		//If player drops trapdoor, center sound on player for nicer effect.
		if (player.wPrevX == pCoord->wX && player.wPrevY == pCoord->wY)
		{
			this->fPos[0] = static_cast<float>(player.wX);
			this->fPos[1] = static_cast<float>(player.wY);
		} else {
			this->fPos[0] = static_cast<float>(pCoord->wX);
			this->fPos[1] = static_cast<float>(pCoord->wY);
		}
		g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR, this->fPos);
		const UINT oTile = this->pCurrentGame->pRoom->GetOSquare(pCoord->wX, pCoord->wY);
		if (bIsPit(oTile))
		{
			static const vector<UINT> trapdoorTile(1, TI_TRAPDOOR_F);
			this->pRoomWidget->AddOLayerEffect(
					new CTrapdoorFallEffect(this->pRoomWidget, *pCoord, trapdoorTile,
							player.IsHasted() ? 260 : 130));
		} else if (oTile == T_WATER) {
			PlaySoundEffect(SEID_SPLASH, this->fPos);
			this->pRoomWidget->AddTLayerEffect(
					new CSplashEffect(this->pRoomWidget, *pCoord));
		}
	}

	for (pObj = CueEvents.GetFirstPrivateData(CID_CutBriar);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);

		//If player cuts briar, center sound on player for nicer effect.
		if (player.GetSwordX() == pCoord->wX && player.GetSwordY() == pCoord->wY)
		{
			this->fPos[0] = static_cast<float>(player.wX);
			this->fPos[1] = static_cast<float>(player.wY);
		} else {
			this->fPos[0] = static_cast<float>(pCoord->wX);
			this->fPos[1] = static_cast<float>(pCoord->wY);
		}
		g_pTheSound->PlaySoundEffect(SEID_BRIAR_BREAK, this->fPos);
		this->pRoomWidget->AddTLayerEffect(
				new CDebrisEffect(this->pRoomWidget, *pCoord, 10,
						GetEffectDuration(5), GetParticleSpeed(4)));  //!!briar pieces?
	}

	if (CueEvents.HasOccurred(CID_ObjectBuilt))
	{
		this->fPos[0] = static_cast<float>(player.wX);
		this->fPos[1] = static_cast<float>(player.wY);
		g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR, this->fPos);
	}
	if (CueEvents.HasOccurred(CID_ObjectFell))
	{
		TilesMap fallingTiles;
		pObj = CueEvents.GetFirstPrivateData(CID_ObjectFell);
		while (pObj)
		{
			//Show object as it falls.
			const CMoveCoordEx2 *pCoord = DYN_CAST(const CMoveCoordEx2*, const CAttachableObject*, pObj);
			
			UINT wTileNo;
			if (pCoord->wValue >= M_OFFSET)
			{
				const UINT eLogicalType = pCoord->wValue - M_OFFSET;
				UINT eMonsterType = eLogicalType; //same by default

				//Look up base type of custom character.
				if (eMonsterType >= CUSTOM_CHARACTER_FIRST)
				{
					HoldCharacter *pChar = this->pCurrentGame->pHold->GetCharacter(eLogicalType);
					if (pChar)
						eMonsterType = pChar->wType;
					else
						eMonsterType = M_CITIZEN1;
				}
				
				if (eMonsterType == M_ROCKGIANT && pCoord->wO >= MONSTER_PIECE_OFFSET) //pieces of rock giant
				{
					const UINT wPieceIndex = (pCoord->wO / MONSTER_PIECE_OFFSET) - 1;
					const UINT wO = pCoord->wO % MONSTER_PIECE_OFFSET;
					wTileNo = GetTileImageForRockGiantPiece(wPieceIndex, wO, 0);
				} else {
					wTileNo = this->pRoomWidget->GetEntityTile(eMonsterType, eLogicalType, pCoord->wO, 0);
				}

				if (pCoord->wValue2 != SwordType::NoSword) {
					const UINT swordTile = this->pRoomWidget->GetSwordTileFor(pCoord->wValue - M_OFFSET, pCoord->wO, pCoord->wValue2);
					if (swordTile) {
						const int wSwordX = pCoord->wX + nGetOX(pCoord->wO);
						const int wSwordY = pCoord->wY + nGetOY(pCoord->wO);
						if (this->pRoomWidget->pRoom->IsValidColRow(wSwordX, wSwordY))
							fallingTiles[ROOMCOORD(wSwordX, wSwordY)].push_back(swordTile);
					}
				}
			}
			else if (bIsSerpentTile(pCoord->wValue))
				wTileNo = GetTileImageForSerpentPiece(pCoord->wO, pCoord->wValue);
			else {
				wTileNo = GetTileImageForTileNo(pCoord->wValue);
				if (bIsBriar(pCoord->wValue))
				{
					//Examining the room state to render the correct briar shape
					//is no longer valid.  But we can look at the prior tiles
					//rendered to the room to get this information.
					const UINT index = this->pCurrentGame->pRoom->ARRAYINDEX(pCoord->wX,pCoord->wY);
					ASSERT(index < this->pCurrentGame->pRoom->CalcRoomArea());
					const UINT tile = this->pRoomWidget->pTileImages[index].t;
					if (bIsBriarTI(tile)) //robustness check
						wTileNo = tile;
				}
				if (wTileNo == CALC_NEEDED)
					wTileNo = CalcTileImageFor(this->pCurrentGame->pRoom, pCoord->wValue,
							pCoord->wX, pCoord->wY);
			}
			fallingTiles[ROOMCOORD(pCoord->wX, pCoord->wY)].push_back(wTileNo);
			pObj = CueEvents.GetNextPrivateData();
		}

		UINT wSoundCount=0;
		for (TilesMap::iterator tiles=fallingTiles.begin(); tiles!=fallingTiles.end(); ++tiles)
		{
			const CCoord coord(tiles->first);
			this->pRoomWidget->AddOLayerEffect(
					new CTrapdoorFallEffect(this->pRoomWidget, coord, tiles->second,
							player.IsHasted() ? 260 : 130));
			if (wSoundCount++ < 3)
			{
				this->fPos[0] = static_cast<float>(coord.wX);
				this->fPos[1] = static_cast<float>(coord.wY);
				g_pTheSound->PlaySoundEffect(SEID_FALLING, this->fPos);
			}
		}
	}

	if (CueEvents.HasOccurred(CID_PressurePlate))
	{
		pObj = CueEvents.GetFirstPrivateData(CID_PressurePlate);
		if (!pObj)
			g_pTheSound->PlaySoundEffect(SEID_PRESSPLATE);   //just play sound
		else while (pObj)
		{
			const COrbData *pOrbData = DYN_CAST(const COrbData*, const CAttachableObject*, pObj);
			this->fPos[0] = static_cast<float>(pOrbData->wX);
			this->fPos[1] = static_cast<float>(pOrbData->wY);
			g_pTheSound->PlaySoundEffect(SEID_PRESSPLATE, this->fPos);
			this->pRoomWidget->AddStrikeOrbEffect(*pOrbData, false);
			pObj = CueEvents.GetNextPrivateData();
		}
	}
	if (CueEvents.HasOccurred(CID_PressurePlateReleased))
	{
		pObj = CueEvents.GetFirstPrivateData(CID_PressurePlateReleased);
		if (!pObj)
			g_pTheSound->PlaySoundEffect(SEID_PRESSPLATEUP);   //just play sound
		else while (pObj)
		{
			const COrbData *pOrbData = DYN_CAST(const COrbData*, const CAttachableObject*, pObj);
			this->fPos[0] = static_cast<float>(pOrbData->wX);
			this->fPos[1] = static_cast<float>(pOrbData->wY);
			g_pTheSound->PlaySoundEffect(SEID_PRESSPLATEUP, this->fPos);
			this->pRoomWidget->AddStrikeOrbEffect(*pOrbData, false);
			pObj = CueEvents.GetNextPrivateData();
		}
	}

	for (pObj = CueEvents.GetFirstPrivateData(CID_CrumblyWallDestroyed);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		//If player stabs monster, center sound on player for nicer effect.
		if (player.GetSwordX() == pCoord->wX && player.GetSwordY() == pCoord->wY)
		{
			this->fPos[0] = static_cast<float>(player.wX);
			this->fPos[1] = static_cast<float>(player.wY);
		} else {
			this->fPos[0] = static_cast<float>(pCoord->wX);
			this->fPos[1] = static_cast<float>(pCoord->wY);
		}
		g_pTheSound->PlaySoundEffect(SEID_BREAKWALL, this->fPos);
		this->pRoomWidget->AddTLayerEffect(
				new CDebrisEffect(this->pRoomWidget, *pCoord, 10,
						GetEffectDuration(5), GetParticleSpeed(4)));
		this->pRoomWidget->AddTLayerEffect(
				new CVerminEffect(this->pRoomWidget, *pCoord));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_MonsterPieceStabbed);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		UINT wMonsterType = M_SERPENT; //default
		//If a golem is at the target square, that means a rock giant was shattered.
		if (this->pCurrentGame->pRoom->IsMonsterOfTypeAt(M_ROCKGOLEM, pCoord->wX, pCoord->wY))
			wMonsterType = M_ROCKGIANT;
		AddKillEffect(wMonsterType, *pCoord);
	}
	if (CueEvents.HasOccurred(CID_OrbActivated))
	{
		pObj = CueEvents.GetFirstPrivateData(CID_OrbActivated);
		if (!pObj)
			g_pTheSound->PlaySoundEffect(SEID_ORBHITQUIET);   //just play sound
		else while (pObj)
		{
			const COrbData *pOrbData = DYN_CAST(const COrbData*, const CAttachableObject*, pObj);
			this->fPos[0] = static_cast<float>(pOrbData->wX);
			this->fPos[1] = static_cast<float>(pOrbData->wY);
			g_pTheSound->PlaySoundEffect(pOrbData->eType == OT_BROKEN ?
					SEID_ORBBROKE : SEID_ORBHITQUIET, this->fPos);
			this->pRoomWidget->AddStrikeOrbEffect(*pOrbData);
			pObj = CueEvents.GetNextPrivateData();
		}
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_OrbDamaged);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const COrbData *pOrbData = DYN_CAST(const COrbData*, const CAttachableObject*, pObj);
		this->fPos[0] = static_cast<float>(pOrbData->wX);
		this->fPos[1] = static_cast<float>(pOrbData->wY);
		g_pTheSound->PlaySoundEffect(SEID_ORBBROKE, this->fPos);
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_MirrorShattered);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		if (player.GetSwordX() == pCoord->wX && player.GetSwordY() == pCoord->wY)
		{
			this->fPos[0] = static_cast<float>(player.wX);
			this->fPos[1] = static_cast<float>(player.wY);
		} else {
			this->fPos[0] = static_cast<float>(pCoord->wX);
			this->fPos[1] = static_cast<float>(pCoord->wY);
		}
		g_pTheSound->PlaySoundEffect(SEID_SHATTER, this->fPos);
		this->pRoomWidget->AddTLayerEffect(
				new CDebrisEffect(this->pRoomWidget, *pCoord, 10,
						GetEffectDuration(7), GetParticleSpeed(4))); //!!ShatteringGlass effect
	}
	if (CueEvents.HasOccurred(CID_BombExploded))
	{
		const UINT wRandMod = CueEvents.GetOccurrenceCount(CID_BombExploded) /
				(g_pTheBM->bAlpha ? (g_pTheBM->eyeCandy > 0 ? 100 : 50) : 10);
		pObj = CueEvents.GetFirstPrivateData(CID_BombExploded);
		while (pObj)
		{
			const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
			if (RAND(wRandMod) == 0)
			{
				if (pCoord->wO == NO_ORIENTATION) //show debris only for this wO
					this->pRoomWidget->AddTLayerEffect(
							new CDebrisEffect(this->pRoomWidget, *pCoord, 3,
									GetEffectDuration(7), GetParticleSpeed(4)));
				this->fPos[0] = static_cast<float>(pCoord->wX);
				this->fPos[1] = static_cast<float>(pCoord->wY);
				g_pTheSound->PlaySoundEffect(SEID_BOMBEXPLODE, this->fPos);
			}
			pObj = CueEvents.GetNextPrivateData();
		}
	}
	if (CueEvents.HasOccurred(CID_OrbActivatedByDouble))
	{
		pObj = CueEvents.GetFirstPrivateData(CID_OrbActivatedByDouble);
		if (!pObj) {
			g_pTheSound->PlaySoundEffect(SEID_ORBHITQUIET);   //just play sound
		} else while (pObj) {
			const COrbData *pOrbData = DYN_CAST(const COrbData*, const CAttachableObject*, pObj);
			this->fPos[0] = static_cast<float>(pOrbData->wX);
			this->fPos[1] = static_cast<float>(pOrbData->wY);
			const bool bOrb = this->pCurrentGame->pRoom->GetTSquare(
					pOrbData->wX, pOrbData->wY) == T_ORB; //as opposed to a pressure plate
			g_pTheSound->PlaySoundEffect(bOrb ?
					(pOrbData->eType == OT_BROKEN ? SEID_ORBBROKE : SEID_ORBHITQUIET) :
					SEID_PRESSPLATE, this->fPos);
			this->pRoomWidget->AddStrikeOrbEffect(*pOrbData, bOrb);
			pObj = CueEvents.GetNextPrivateData();
		}
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_TarstuffDestroyed);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		//Green/clean map room might need to be reverted back to red
		//if tar was hit and tar babies were created in a clean room.
		//This can be accomplished by redrawing the room whenever tar
		//changes configuration.
		const CMoveCoordEx *pCoord = DYN_CAST(const CMoveCoordEx*, const CAttachableObject*, pObj);
		//If player stabs tar, center sound on player for nicer effect.
		if (player.GetSwordX() == pCoord->wX && player.GetSwordY() == pCoord->wY)
		{
			this->fPos[0] = static_cast<float>(player.wX);
			this->fPos[1] = static_cast<float>(player.wY);
		} else {
			this->fPos[0] = static_cast<float>(pCoord->wX);
			this->fPos[1] = static_cast<float>(pCoord->wY);
		}
		g_pTheSound->PlaySoundEffect(SEID_STABTAR, this->fPos);
		switch (pCoord->wValue)
		{
			case T_TAR:
				this->pRoomWidget->AddTLayerEffect(
						new CTarStabEffect(this->pRoomWidget, *pCoord,
								GetEffectDuration(7), GetParticleSpeed(4)));
			break;
			case T_MUD:
				this->pRoomWidget->AddTLayerEffect(
						new CMudStabEffect(this->pRoomWidget, *pCoord,
								GetEffectDuration(7), GetParticleSpeed(4)));
			break;
			case T_GEL:
				this->pRoomWidget->AddTLayerEffect(
						new CGelStabEffect(this->pRoomWidget, *pCoord,
								GetEffectDuration(7), GetParticleSpeed(4)));
			break;
			default: ASSERT(!"Invalid tar type"); break;
		}
	}
	if (CueEvents.HasOccurred(CID_StepOnScroll))
	{
		this->pFaceWidget->SetReading(true);
		const CDbMessageText *pScrollText = DYN_CAST(const CDbMessageText*, const CAttachableObject*,
				CueEvents.GetFirstPrivateData(CID_StepOnScroll));
		ASSERT((const WCHAR*)(*pScrollText));
		this->pScrollLabel->SetText((const WCHAR*)(*pScrollText));
		ShowScroll();
	}

	//Remove old sparks before drawing the current ones.
	this->pRoomWidget->RemoveTLayerEffectsOfType(ESPARK);

	//Spark rendering must come both before and after room is drawn so it will
	//show up correctly both on room entrance and  in double-placing freeze frame.
	if (!bPlayerLeftRoom)
		ProcessFuseBurningEvents(CueEvents);

	//3rd. Monster actions.
	if (CueEvents.HasOccurred(CID_GoblinAttacks))
		g_pTheSound->PlaySoundEffect(SEID_HIT);

	for (pObj = CueEvents.GetFirstPrivateData(CID_SnakeDiedFromTruncation);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMonster *pMonster = DYN_CAST(const CMonster*, const CAttachableObject*, pObj);
		const CMoveCoord coord(pMonster->wX,pMonster->wY,pMonster->wO);
		AddKillEffect(pMonster->GetIdentity(), coord);
	}
	if (CueEvents.HasOccurred(CID_EvilEyeWoke))
		g_pTheSound->PlaySoundEffect(SEID_EVILEYEWOKE, this->fPos);
/*
	for (pObj = CueEvents.GetFirstPrivateData(CID_EvilEyeWoke);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		CMoveCoord *pCoord = DYN_CAST(CMoveCoord*, CAttachableObject*, pObj);
		this->fPos[0] = static_cast<float>(pCoord->wX);
		this->fPos[1] = static_cast<float>(pCoord->wY);
		g_pTheSound->PlaySoundEffect(SEID_EVILEYEWOKE, this->fPos);
		this->pRoomWidget->AddMLayerEffect(new CEvilEyeGazeEffect(
				this->pRoomWidget,pCoord->wX,pCoord->wY,pCoord->wO, 500));
	}
*/
	if (CueEvents.HasOccurred(CID_PlayerBurned))
	{
		g_pTheSound->PlaySoundEffect(SEID_SIZZLE);
		this->pRoomWidget->AddMLayerEffect(
				new CSteamEffect(this->pRoomWidget, player));
	}

	for (pObj = CueEvents.GetFirstPrivateData(CID_MonsterBurned);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pMoveCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		const CMoveCoord coord(pMoveCoord->wX,pMoveCoord->wY,NO_ORIENTATION);
		this->fPos[0] = static_cast<float>(coord.wX);
		this->fPos[1] = static_cast<float>(coord.wY);
		g_pTheSound->PlaySoundEffect(SEID_SIZZLE, this->fPos);
		this->pRoomWidget->AddMLayerEffect(
				new CSteamEffect(this->pRoomWidget, coord));
	}
/*
	for (pObj = CueEvents.GetFirstPrivateData(CID_FegundoToAsh);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		CMonster *pMonster = DYN_CAST(CMonster*, CAttachableObject*, pObj);
		CMoveCoord coord(pMonster->wX,pMonster->wY,NO_ORIENTATION);
		this->fPos[0] = static_cast<float>(coord.wX);
		this->fPos[1] = static_cast<float>(coord.wY);
		g_pTheSound->PlaySoundEffect(SEID_SIZZLE, this->fPos);
		this->pRoomWidget->AddMLayerEffect(
				new CSteamEffect(this->pRoomWidget, coord));
	}
*/
	for (pObj = CueEvents.GetFirstPrivateData(CID_Splash);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
		this->fPos[0] = static_cast<float>(pCoord->wX);
		this->fPos[1] = static_cast<float>(pCoord->wY);
		g_pTheSound->PlaySoundEffect(SEID_SPLASH, this->fPos);
		if (this->pCurrentGame->pRoom->GetOSquare(pCoord->wX, pCoord->wY) == T_WATER)
			this->pRoomWidget->AddTLayerEffect(
					new CSplashEffect(this->pRoomWidget, *pCoord));
	}

	//Events w/o accompanying sounds.
/*
	if (CueEvents.HasOccurred(CID_ConquerRoom))
	{
		this->pMapWidget->DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom);
		this->pMapWidget->RequestPaint();
		if (this->bHoldConquered)
			this->roomsPreviouslyConquered += this->pCurrentGame->ConqueredRooms;
	}
*/
	if (CueEvents.HasOccurred(CID_Explosion))
	{
		//Since several explosions could overlap, don't allow placing
		//more than one effect on each tile.
		CCoordSet coords;
		for (pObj = CueEvents.GetFirstPrivateData(CID_Explosion);
				pObj != NULL; pObj = CueEvents.GetNextPrivateData())
		{
			const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
			coords.insert(pCoord->wX, pCoord->wY);
		}
		for (CCoordSet::const_iterator coord=coords.begin(); coord!=coords.end(); ++coord)
		{
			this->pRoomWidget->AddTLayerEffect(
					new CExplosionEffect(this->pRoomWidget, CCoord(coord->wX, coord->wY),
							GetEffectDuration(500)));
		}
	}

	if (CueEvents.HasOccurred(CID_MonsterSpoke))
		//complete turn animation immediately in preparation for question
		this->pRoomWidget->FinishMoveAnimation();

	//Update room lights as needed.
	const bool bLightToggled = CueEvents.HasOccurred(CID_LightToggled);
	if (bLightToggled)
		this->pRoomWidget->RenderRoomLighting();
	this->pRoomWidget->AddPlayerLight();

	//
	//Begin section where room load can occur.  If room load occurs then
	//original cue events from command before room load will be discarded, and cue
	//events from first step into room will be in CueEvents.  Original cue events
	//should be handled before this section or stored in variables for handling
	//after this section.
	//

	//Check for player dying.
	if (CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied))
	{
		//Add effects that normally aren't displayed until after the room is rendered.
		ProcessStatEffects(CueEvents);

		//Update tile image arrays before death sequence.
		if (CueEvents.HasOccurred(CID_Plots) || bLightToggled)
		{
			const CCoordSet *pSet = DYN_CAST(const CCoordSet*, const CAttachableObject*,
					CueEvents.GetFirstPrivateData(CID_Plots) );
			this->pRoomWidget->UpdateFromPlots(pSet, &this->pCurrentGame->pRoom->geometryChanges);
		}
		const bool bDeathNotUndone = HandleEventsForPlayerDeath(CueEvents);
		CueEvents.Clear();	//clear after death sequence (whether move is undone or not)
		//but before room restart so speech on room start can be retained

		//Repaint the sign in case demo recording ended.
		UpdateSign();

		ASSERT(!(this->pCurrentGame->bIsGameActive && bDeathNotUndone));
		if (GetScreenType() == SCR_Demo)
			return eNextScreen; //stop showing demo on death
		if (bDeathNotUndone)
		{
/*
			//Create demo/End demo recording if game is now inactive.
			if (this->pCurrentGame->IsDemoRecording())  //End a demo that is recording.
			{
				if (!this->pCurrentGame->EndDemoRecording())
				{
					CFiles f;
					f.AppendErrorLog("Failed to save a demo when recording ended.\r\n");
				}
				UpdateSign();
			}
*/
/*
			else if ((this->pCurrentGame->GetAutoSaveOptions() & ASO_DIEDEMO) == ASO_DIEDEMO)
				this->pCurrentGame->WriteCurrentRoomDieDemo();
			ASSERT(!this->pCurrentGame->IsDemoRecording());
*/
			delete g_pPredictedCombat;
			g_pPredictedCombat = NULL;

			const CIDSet mappedRooms = this->pCurrentGame->GetExploredRooms(true);

//			this->pCurrentGame->RestartRoomFromLastCheckpoint(CueEvents);
			this->pCurrentGame->RestartRoom(this->sCueEvents);
//			this->wUndoToTurn = this->pCurrentGame->wTurnNo; //can't undo after restart
//			this->bRoomClearedOnce = this->pCurrentGame->IsCurrentRoomPendingExit();
			UpdateSound();
			StopAmbientSounds();
			ClearSpeech();
			AmbientSoundSetup();
			this->pRoomWidget->ClearEffects();
			this->pRoomWidget->RenderRoomLighting();
			this->pRoomWidget->ResetForPaint();

			//Refresh map.
			const CIDSet nowMappedRooms = this->pCurrentGame->GetExploredRooms(true);
			if (nowMappedRooms.size() != mappedRooms.size())
				this->pMapWidget->LoadFromCurrentGame(this->pCurrentGame);
		}
		UpdateScroll();
		RedrawStats(this->pCurrentGame->pCombat, true);
		this->pMapWidget->DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom, this->pCurrentGame->pRoom->mapMarker);
		this->pMapWidget->RequestPaint();
		return eNextScreen;
	}

	if (ProcessExitLevelEvents(CueEvents, eNextScreen))
		return eNextScreen;

	//
	//End section where room load can occur.
	//

	if (bPlayerLeftRoom) //Went to a new room or this room was reloaded.
	{
		//Play music for style.
		SetMusicStyle();

		//When changing rooms, save progress to disk.
		//Call this right before room transition to minimize apparent delay.
		if (!this->bPlayTesting && GetScreenType() == SCR_Game)
			g_pTheDB->Commit();

		//Reset any simulated combat, as pointers might be left dangling.
		delete g_pPredictedCombat;
		g_pPredictedCombat = NULL;

		//Determine direction of exit (if any).
		//Note that the following only occur if no demo playback is in progress.
		UINT wExitOrientation = NO_ORIENTATION;
		const CAttachableWrapper<UINT> *pExitOrientation =
				DYN_CAST(const CAttachableWrapper<UINT>*, const CAttachableObject*,
				CueEvents.GetFirstPrivateData(CID_ExitRoom));
		//Show transition to new room.
		if (pExitOrientation)
		{
			//Remove ambient sounds.
			StopAmbientSounds();

			wExitOrientation = static_cast<UINT>(*pExitOrientation);

			this->pRoomWidget->ShowRoomTransition(wExitOrientation);
			ClearEventBuffer(); //don't buffer up commands while transitioning, but don't reset pressed keys/buttons

			//Update other UI elements that may have changed.
			this->pMapWidget->UpdateFromCurrentGame();
			this->pMapWidget->DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom, this->pCurrentGame->pRoom->mapMarker);
			this->pMapWidget->RequestPaint();
			UpdateSign();
			StopAmbientSounds();
			ClearSpeech();
			SetGameAmbience(true);
			UpdateSound();
			ProcessSpeechCues(CueEvents);	//call after ClearSpeech
		}
	}
	else //Still in the same room.
	{
		if (CueEvents.HasOccurred(CID_Plots) || bLightToggled)
		{
			//Do an update of tile image arrays.
			const CCoordSet *pSet = DYN_CAST(const CCoordSet*, const CAttachableObject*,
					CueEvents.GetFirstPrivateData(CID_Plots) );
			this->pRoomWidget->UpdateFromPlots(pSet, &this->pCurrentGame->pRoom->geometryChanges);
			this->pMapWidget->DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom, this->pCurrentGame->pRoom->mapMarker);
			this->pMapWidget->RequestPaint();
		}
/*
		else if (CueEvents.HasOccurred(CID_NPCTypeChange))
		{
			//Just ensure minimap state is current.
			this->pMapWidget->DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom);
			this->pMapWidget->RequestPaint();
		}
*/
	}

	return eNextScreen;
}

//***************************************************************************************
SCREENTYPE CGameScreen::ProcessCueEventsAfterRoomDraw(
//Process cue events list to create effects that should occur after
//room is drawn.
//
//Params:
	CCueEvents &CueEvents) //(in)
{
	SCREENTYPE eNextScreen = SCR_Game;
	const CAttachableObject *pObj;

	CSwordsman& player = *(this->pCurrentGame->pPlayer);
	const UINT wSX = player.wX;
	const UINT wSY = player.wY;

	//Hide old scroll before stats get drawn.
	if (this->bIsScrollVisible &&
			this->pCurrentGame->pRoom->GetTSquare(wSX, wSY) != T_SCROLL)
	{
		this->pFaceWidget->SetReading(false);
		HideScroll();
	}

	if (!g_pTheDSM->bTransitioning)
		RedrawStats(this->pCurrentGame->pCombat, true);

	SetMusicStyle();

	//Allow handling either before or after room is drawn.
	ProcessMovieEvents(CueEvents);

	//ensure texts aren't on top of each other
	int numFlashingTexts = CueEvents.GetOccurrenceCount(CID_FlashingMessage);
/*
	if (bLevelComplete)
		++numFlashingTexts;
	if (bSecretFound)
		++numFlashingTexts;
	if (bHoldMastered)
		++numFlashingTexts;
*/
	static const int CY_FLASHING_TEXT = 50;
	int yFlashingTextOffset = (-numFlashingTexts / 2) * CY_FLASHING_TEXT;
	const int Y_FLASHING_TEXT_MAX = int(this->pRoomWidget->GetH()) / 2 - CY_FLASHING_TEXT;
	const int Y_FLASHING_TEXT_MIN = -Y_FLASHING_TEXT_MAX + CY_FLASHING_TEXT; //leave space at top for score ranking
	if (yFlashingTextOffset < Y_FLASHING_TEXT_MIN)
		yFlashingTextOffset = Y_FLASHING_TEXT_MIN;

	if (CueEvents.HasOccurred(CID_InvalidAttackMove))
	{
		pObj = CueEvents.GetFirstPrivateData(CID_InvalidAttackMove);
		ASSERT(pObj);
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		static CMoveCoord coord;
		coord = *pCoord;
		this->pRoomWidget->AddInfoSubtitle(&coord, g_pTheDB->GetMessageText(MID_CantHarmDangerousEnemy), 2000);
		PlayHitObstacleSound(player.wAppearance, CueEvents);
		ShowStatsForMonsterAt(pCoord->wX, pCoord->wY);
	}

	if (CueEvents.HasOccurred(CID_BumpedLockedDoor))
	{
		g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR);

		pObj = CueEvents.GetFirstPrivateData(CID_BumpedLockedDoor);
		ASSERT(pObj);
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
		const UINT oTile = this->pCurrentGame->pRoom->GetOSquare(pCoord->wX, pCoord->wY);
		if (oTile == T_DOOR_Y || oTile == T_DOOR_C || oTile == T_DOOR_G || oTile == T_DOOR_MONEY)
			this->pRoomWidget->DisplaySubtitle(
					g_pTheDB->GetMessageText(MID_BumpedLockedDoorMessage), wSX, wSY, true);
	}
	if (CueEvents.HasOccurred(CID_CantLockHere))
	{
		g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR);
		this->pRoomWidget->DisplaySubtitle(
			g_pTheDB->GetMessageText(MID_CantLockHereMessage), wSX, wSY, true);
	}
	if (CueEvents.HasOccurred(CID_ExitBlockedOnOtherSide))
	{
		g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR);
		this->pRoomWidget->DisplaySubtitle(
			g_pTheDB->GetMessageText(MID_ExitBlockedOnOtherSide), wSX, wSY, true);
	}
	if (CueEvents.HasOccurred(CID_NoAdjoiningRoom))
	{
		g_pTheSound->PlaySoundEffect(SEID_TRAPDOOR);
		this->pRoomWidget->DisplaySubtitle(
			g_pTheDB->GetMessageText(MID_NoAdjoiningRoom), wSX, wSY, true);
	}

/*
	const bool bLevelComplete = CueEvents.HasOccurred(CID_CompleteLevel) &&
			this->pCurrentGame->pLevel->bIsRequired;
*/
	const bool bSecretFound = CueEvents.HasOccurred(CID_SecretRoomFound);

/*
	if (bLevelComplete)
	{
		g_pTheSound->PlaySoundEffect(SEID_LEVELCOMPLETE);
		this->pRoomWidget->AddLastLayerEffect(new CFlashMessageEffect(
				this->pRoomWidget, g_pTheDB->GetMessageText(MID_LevelComplete),
				bSecretFound ? -50 : 0));	//ensure texts aren't on top of each other
	}
*/
	if (bSecretFound)
	{
//		if (!bLevelComplete)
		g_pTheSound->PlaySoundEffect(SEID_SECRET);
		this->pRoomWidget->AddLastLayerEffect(new CFlashMessageEffect(
				this->pRoomWidget, g_pTheDB->GetMessageText(MID_SecretRoomFound)));
//				, bLevelComplete ? 50 : 0));	//ensure texts aren't on top of each other
	}
	//Priority of player moods
	UpdatePlayerFace();
	if (CueEvents.HasOccurred(CID_SwordsmanAfraid))
		this->pFaceWidget->SetMood(PlayerRole, Mood_Nervous);
	else if (CueEvents.HasOccurred(CID_SwordsmanAggressive))
		this->pFaceWidget->SetMood(PlayerRole, Mood_Aggressive);
	else if (CueEvents.HasOccurred(CID_SwordsmanNormal))
		this->pFaceWidget->SetMood(PlayerRole, Mood_Normal);

/*
	if (CueEvents.HasOccurred(CID_AllMonstersKilled))  //priority of temporary moods
		this->pFaceWidget->SetMoodToSoundEffect(Mood_Happy, SEID_CLEAR);
	else
*/
	if (CueEvents.HasOccurred(CID_MonsterDiedFromStab))
	{
/*	No special mood to display in this game
		bool bBeethroStabbed = false;
		const UINT wSSwordX = player.GetSwordX();
		const UINT wSSwordY = player.GetSwordY();
		pObj = CueEvents.GetFirstPrivateData(CID_MonsterDiedFromStab);
		while (pObj)
		{
			CMonster *pMonster = DYN_CAST(CMonster*, CAttachableObject*, pObj);
			if (wSSwordX == pMonster->wX && wSSwordY == pMonster->wY)
			{
				bBeethroStabbed = true;
				break;
			}
			pObj = CueEvents.GetNextPrivateData();
		}
		if (bBeethroStabbed)
			this->pFaceWidget->SetMood(Mood_Strike,250);
*/
	}
	else if (CueEvents.HasOccurred(CID_Scared))
		this->pFaceWidget->SetMood(PlayerRole, Mood_Nervous,250);
	else if (CueEvents.HasOccurred(CID_HitObstacle))
		this->pFaceWidget->SetMood(PlayerRole, Mood_Aggressive,250);

	if (CueEvents.HasOccurred(CID_PlayerOnWaterEdge))
	{
		this->pFaceWidget->SetMood(PlayerRole, Mood_Nervous,250);
		g_pTheSound->PlaySoundEffect(SEID_WATERSTEP);
	}

	//Ambient sounds.
	if (CueEvents.HasOccurred(CID_AmbientSound))
	{
		pObj = CueEvents.GetFirstPrivateData(CID_AmbientSound);
		while (pObj)
		{
			//wX/Y: sound location, wO: DataID (0 = stop all), wValue: loop if set
			const CMoveCoordEx *pCoord = DYN_CAST(const CMoveCoordEx*, const CAttachableObject*, pObj);
			PlayAmbientSound(pCoord->wO, pCoord->wValue != 0, pCoord->wX, pCoord->wY);
			pObj = CueEvents.GetNextPrivateData();
		}
	}

	//Scripted effects.
	if (CueEvents.HasOccurred(CID_VisualEffect))
	{
		pObj = CueEvents.GetFirstPrivateData(CID_VisualEffect);
		while (pObj)
		{
			const VisualEffectInfo *pEffect = DYN_CAST(const VisualEffectInfo*, const CAttachableObject*, pObj);
			AddVisualEffect(pEffect);
			pObj = CueEvents.GetNextPrivateData();
		}
	}
	if (CueEvents.HasOccurred(CID_Autosave))
	{
		CTextEffect *pTextEffect = new CTextEffect(this->pRoomWidget,
				g_pTheDB->GetMessageText(MID_Autosaving), F_Stats, 1000, 2000, false, true);
		pTextEffect->Move(5, 5); //display in top-left corner of room
		this->pRoomWidget->AddLastLayerEffect(pTextEffect);
		if (!g_pTheSound->IsSoundEffectPlaying(SEID_AUTOSAVE))
			g_pTheSound->PlaySoundEffect(SEID_AUTOSAVE);
	}
	if (CueEvents.HasOccurred(CID_ScoreCheckpoint) &&
			!g_pTheDSM->bTransitioning) //don't pop-up score info if this screen is only now being activated
	{
		for (pObj = CueEvents.GetFirstPrivateData(CID_ScoreCheckpoint);
				pObj != NULL; pObj = CueEvents.GetNextPrivateData())
		{
			const CDbMessageText *pScoreIDText = DYN_CAST(const CDbMessageText*, const CAttachableObject*, pObj);
			ASSERT((const WCHAR*)(*pScoreIDText));
			if (!g_pTheSound->IsSoundEffectPlaying(SEID_LEVELCOMPLETE))
				g_pTheSound->PlaySoundEffect(SEID_LEVELCOMPLETE); //SEID_AREACLEAR is jarring over other music
			ScoreCheckpoint((const WCHAR*)(*pScoreIDText));
		}
	}

	ProcessStatEffects(CueEvents);

/*
	//Player doubles might get placed between an Aumtlich and its target.
	if (CueEvents.HasOccurred(CID_DoublePlaced))
	{
		pRoomWidget->RemoveTLayerEffectsOfType(ESPARK);
		this->bPersistentEventsDrawn = false; //Any fuse sparks got removed too
	}
*/

	if (CueEvents.HasOccurred(CID_PlayerStabbed))
		g_pTheSound->PlaySoundEffect(SEID_SPLAT);

	for (pObj = CueEvents.GetFirstPrivateData(CID_ZombieGaze);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMonster *pMonster = DYN_CAST(const CMonster*, const CAttachableObject*, pObj);
		this->pRoomWidget->AddZombieGazeEffect(pMonster);
	}
	if (CueEvents.HasOccurred(CID_Swordfight))
		g_pTheSound->PlaySoundEffect(SEID_SWORDS);

	if (CueEvents.HasOccurred(CID_RoomLocationTextUpdate))
		UpdateSign();

	for (pObj = CueEvents.GetFirstPrivateData(CID_FlashingMessage);
		pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		if (yFlashingTextOffset > Y_FLASHING_TEXT_MAX)
			break; //no room to display

		const CColorText* pColorText = DYN_CAST(const CColorText*, const CAttachableObject*, pObj);
		const CDbMessageText* pText = pColorText->pText;
		ASSERT((const WCHAR*)(*pText));
		CFlashMessageEffect* pFlashText = new CFlashMessageEffect(
			this->pRoomWidget, (const WCHAR*)(*pText), yFlashingTextOffset, 2000, 500);
		pFlashText->SlowExpansion();
		if (pColorText->customColor)
			pFlashText->SetColor(pColorText->r, pColorText->g, pColorText->b);
		this->pRoomWidget->AddLastLayerEffect(pFlashText);
		yFlashingTextOffset += CY_FLASHING_TEXT;
	}

	//Process both before and after room is drawn.
	if (!this->bPersistentEventsDrawn)
		ProcessFuseBurningEvents(CueEvents);

	ProcessQuestionPrompts(CueEvents, eNextScreen);

	//Check for winning game.
	if (CueEvents.HasOccurred(CID_WinGame) && GetScreenType() != SCR_Demo)
	{
		LOGCONTEXT("CGameScreen::ProcessCueEventsAfterRoomDraw--Winning game.");
		if (!this->bPlayTesting && GetScreenType() == SCR_Game)
		{
			SendAchievement("WIN");
			UploadExploredRooms(ST_EndHold);	//end hold saved game should exist by now
			g_pTheDB->Commit();
		}

		//Reset any simulated combat, as pointers might be left dangling.
		delete g_pPredictedCombat;
		g_pPredictedCombat = NULL;

		//Update map to latest state.
		this->pMapWidget->DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom, this->pCurrentGame->pRoom->mapMarker);
		this->pMapWidget->RequestPaint();

		//Level exit effect is different depending on method of exit.
		const bool bStairs = bIsStairs(this->pCurrentGame->pRoom->GetOSquare(
				player.wX, player.wY));
		SCREENTYPE eExitRetScreen = SCR_Game;
		if (bStairs)
		{
			eExitRetScreen = HandleEventsForLevelExit(); //walk down stairs first
			this->pRoomWidget->ResetRoom();
		} else {
			//Fade out.
			if (!CueEvents.HasOccurred(CID_PlayVideo))
				Paint();
			FadeRoom(false, 750);
		}
		CueEvents.Clear();
		this->pRoomWidget->ClearEffects();
		if (this->bPlayTesting)
		{
			//Return to level editor.
			eNextScreen = SCR_Return;
		} else {
			//Won the game.
			DeleteCurrentGame(); //unload without saving to continue slot
			eNextScreen = SCR_WinStart;
		}
		UnloadGame();  //current game has ended

		if (eExitRetScreen == SCR_None)
			eNextScreen = SCR_None;
	}

	return eNextScreen;
}

//*****************************************************************************
bool CGameScreen::ProcessExitLevelEvents(CCueEvents& CueEvents, SCREENTYPE& eNextScreen)
//Returns: whether the game screen should be exited
{
	//Check for level exiting.
	if (!CueEvents.HasOccurred(CID_ExitLevelPending))
		return false;

	const CCoord *pExitInfo =
			DYN_CAST(const CCoord*, const CAttachableObject*,
			CueEvents.GetFirstPrivateData(CID_ExitLevelPending));
	const UINT dwEntranceID = pExitInfo->wX;
	const bool bSkipEntranceScreen = pExitInfo->wY != 0;
	if (!dwEntranceID)
		return false; //not a valid event field value -- ignore it

	if (!this->bPlayTesting && GetScreenType() == SCR_Game)
		g_pTheDB->Commit();

	//Update room/map to latest state.
	const CCoordSet *pSet = DYN_CAST(const CCoordSet*, const CAttachableObject*,
			CueEvents.GetFirstPrivateData(CID_Plots) );
	this->pRoomWidget->UpdateFromPlots(pSet, &this->pCurrentGame->pRoom->geometryChanges);
	this->pMapWidget->DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom, this->pCurrentGame->pRoom->mapMarker);
	this->pMapWidget->RequestPaint();
//	ShowLockIcon(false); //remove room lock display

	//Reset any simulated combat, as pointers might be left dangling.
	delete g_pPredictedCombat;
	g_pPredictedCombat = NULL;

	//Level exit effect is different depending on method of exit.
	CSwordsman& player = *(this->pCurrentGame->pPlayer);
	const bool bStairs = bIsStairs(this->pCurrentGame->pRoom->GetOSquare(
			player.wX, player.wY));
	if (bStairs)
	{
		//Show player walking down stairs, etc.
		eNextScreen = HandleEventsForLevelExit();
		if (eNextScreen == SCR_None)
			return SCR_None; //quit
	} else {
		//Paint room in latest state.
		if (!CueEvents.HasOccurred(CID_PlayVideo))
			Paint();
	}

	//Fade out when level entrance screen is not shown.
	CEntranceData *pEntrance = this->pCurrentGame->pHold->GetEntrance(dwEntranceID);
	bool bShowLevelEntranceDescription =
			pEntrance && pEntrance->eShowDescription != CEntranceData::DD_No && !bSkipEntranceScreen;
	if (pEntrance && pEntrance->eShowDescription == CEntranceData::DD_Once &&
			this->pCurrentGame->entrancesExplored.has(dwEntranceID))
		bShowLevelEntranceDescription = false; //this entrance has been entered before
	if (bShowLevelEntranceDescription)
		eNextScreen = SCR_LevelStart;
	else
	{
		if (eNextScreen == SCR_LevelStart)
			eNextScreen = SCR_Game;
		FadeRoom(false, 600);
	}

	//Reset these things.
	StopAmbientSounds();
	ClearSpeech(true);
	this->pRoomWidget->ClearEffects();

	if (GetScreenType() != SCR_Demo)
	{
		CueEvents.Clear();

		//Handle uploads before loading new level data so room isn't
		//redrawn in incorrect style on error.
		WaitToUploadDemos();

		this->pCurrentGame->LoadFromLevelEntrance(this->pCurrentGame->pHold->dwHoldID,
				dwEntranceID, CueEvents);
		//stop drawing new room until after Level Start screen
		this->pRoomWidget->ResetRoom();
//				this->bRoomClearedOnce = this->pCurrentGame->IsCurrentRoomPendingExit();

		if (!bShowLevelEntranceDescription)
		{
			//Handle things now that would normally be done on reactivating
			//this screen after showing the level entrance screen.
			this->pRoomWidget->UpdateFromCurrentGame();
			this->pMapWidget->LoadFromCurrentGame(this->pCurrentGame);
			//Show active room in its current state.
			this->pMapWidget->DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom,
					this->pCurrentGame->pRoom->mapMarker);
			this->pMapWidget->RequestPaint();
			UpdateSign();
			UpdateScroll();
			UpdateSound();
			ProcessSpeechCues(CueEvents);
			PaintClock(true);
			RedrawStats(this->pCurrentGame->pCombat, true);

			//Fade in.
			FadeRoom(true, 400);

			//Throw away everything that happened during fade.
			ClearEvents();
			this->wMoveDestX = this->wMoveDestY = NO_DESTINATION;

			SetMusicStyle();
		}

		//Mark this entrance as explored.
		//If the entrance has the "show desc once" flag set, the description
		//won't be displayed any more on entrance.
		this->pCurrentGame->entrancesExplored += dwEntranceID;
	}

	return true;
}

//*****************************************************************************
void CGameScreen::ProcessFuseBurningEvents(CCueEvents& CueEvents)
//Handle events for fuse burning.
//This must be called both before and after the room is drawn.
{
	if (!CueEvents.HasOccurred(CID_FuseBurning))
		return;

	ASSERT(this->pCurrentGame);
	const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*,
			CueEvents.GetFirstPrivateData(CID_FuseBurning));
	ASSERT(pCoord);
	static const UINT MAX_FUSE_SOUNDS = 1;
	UINT wCount = 0;
	while (pCoord)
	{
		const UINT wTSquare = this->pCurrentGame->pRoom->GetTSquare(pCoord->wX, pCoord->wY);
		if (wTSquare == T_FUSE || wTSquare == T_BOMB)	//needed to avoid effects
					//where fuses have already disappeared since the cue event fired
		{
			if (pCoord->wO == NO_ORIENTATION && wTSquare == T_FUSE)
			{
				if (wCount++ < MAX_FUSE_SOUNDS)
				{
					this->fPos[0] = static_cast<float>(pCoord->wX);
					this->fPos[1] = static_cast<float>(pCoord->wY);
					g_pTheSound->PlaySoundEffect(SEID_STARTFUSE, this->fPos);
				}
			}
			this->pRoomWidget->AddTLayerEffect(
					new CSparkEffect(this->pRoomWidget, *pCoord, 10));
		}
		pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*,
				CueEvents.GetNextPrivateData());
	}

	this->bPersistentEventsDrawn = true;
}

//*****************************************************************************
void CGameScreen::ProcessStatEffects(CCueEvents& CueEvents)
//Handle display effects for stat changes.
{
	if (!CueEvents.HasOccurred(CID_EntityAffected))
		return;

	CSwordsman& player = *this->pCurrentGame->pPlayer;
	const CAttachableObject *pObj = CueEvents.GetFirstPrivateData(CID_EntityAffected);
	UINT count=0, incCount;
	int healDelta=0, atkDelta=0, defDelta=0, goldDelta=0, xpDelta=0;
	int yKeyDelta=0, gKeyDelta=0, bKeyDelta=0, sKeyDelta=0;
	while (pObj)
	{
		const CCombatEffect *pEffect = DYN_CAST(const CCombatEffect*, const CAttachableObject*, pObj);
		CEntity *pEntity = pEffect->pEntity;
		ASSERT(pEntity);
		incCount=0;

		switch (pEffect->eType)
		{
			case CET_NODAMAGE:
			{
				UINT tile = TI_CHECKPOINT;
				//Show shield if it is helping the player to block the attack.
				if (pEntity == &player)
				{
					const bool bShieldDisabled = this->pCurrentGame->IsPlayerShieldDisabled();
					if (!bShieldDisabled && this->pCurrentGame->getShieldPower(player.st.shield) > 0)
					{
						const UINT shieldTile = CalcTileImageForEquipment(ScriptFlag::Armor);
						if (shieldTile != TI_UNSPECIFIED)
							tile = shieldTile;
					}
				}
				this->pRoomWidget->AddMLayerEffect(
						new CFadeTileEffect(this->pRoomWidget, *pEntity, tile, 500));
				g_pTheSound->PlaySoundEffect(SEID_SHIELDED, NULL);
			}
			break;
			case CET_STRONGHIT:
				this->pRoomWidget->AddMLayerEffect(
						new CFadeTileEffect(this->pRoomWidget, *pEntity, TI_STRONGHIT, 300));
			break;
			case CET_NODEFENSEHIT:
				this->pRoomWidget->AddMLayerEffect(
						new CFadeTileEffect(this->pRoomWidget, *pEntity, TI_NODEFENSEHIT, 300));
			break;
			case CET_HARM:
			{
				//Determine relative positions of combatants.
				CMoveCoord coord(pEntity->wX,pEntity->wY,NO_ORIENTATION);
				CCombat *pCombat = this->pCurrentGame->pCombat;
				int xOffset;
				if (coord.wX == this->pCurrentGame->pRoom->wRoomCols-1)
					xOffset = -int(g_pTheBM->CX_TILE/2);
				else
					xOffset = RAND(g_pTheBM->CX_TILE/4); //jiggle numbers a bit
				int yOffset = coord.wY ? -int(g_pTheBM->CY_TILE/2) : 0;
				static const SURFACECOLOR red = {255, 0, 0};
				static const SURFACECOLOR lightRed = {255, 128, 128};
				SURFACECOLOR color;
				const bool bPlayerHarmed = pEntity == &player;
				if (bPlayerHarmed)
				{
					//stat is shown for player
					color = red;
					if (pCombat)
					{
						coord.wO = GetOrientation(pCombat->pMonster->wX, pCombat->pMonster->wY, player.wX, player.wY);

						//Show monster chomping on player.
						this->pRoomWidget->AnimateMonster(pCombat->pMonster);

						//Monster strikes player,
						const SEID seid = GetMonsterAttackSoundEffect(pCombat);
						g_pTheSound->PlaySoundEffect(seid, NULL);
					}

					//Don't overlap display of multiple hit number on player.
					yOffset += (count++) * g_pTheBM->CY_TILE/2;
				} else {
					//stat is shown for monster being fought
					color = lightRed;
					if (pCombat)
					{
						coord.wO = GetOrientation(player.wX, player.wY, pEntity->wX, pEntity->wY);
						//Offset coord if this display is probably overlapping player effect display.
						if (pEntity->wX == player.wX)
						{
							xOffset = coord.wX ? -int(g_pTheBM->CX_TILE/2) : g_pTheBM->CX_TILE/2;
							yOffset = 0;
						}
					}

					//Play attack sound when player/mimic hits monster.
					if (pCombat)
					{
						if (this->pCurrentGame->pRoom->GetMonsterAtSquare(pCombat->wFromX, pCombat->wFromY))
							g_pTheSound->PlaySoundEffect(SEID_SPLAT, NULL); //mimic is attacking
						else
							g_pTheSound->PlaySoundEffect(player.HasSword() ? SEID_SPLAT : SEID_PUNCH, NULL);
					}
				}

				const float damagePercent = pEffect->amount / float(pEffect->originalAmount);
				AddDamageEffect(pEntity->GetIdentity(), coord, damagePercent);
				this->pRoomWidget->AddLastLayerEffect(new CFloatTextEffect(this->pRoomWidget,
					coord.wX*g_pTheBM->CX_TILE + xOffset, coord.wY*g_pTheBM->CY_TILE + yOffset,
					pEffect->amount, color, F_Title));
			}
			break;

			//Show these effects with a single combined stat for the player.
			case CET_HEAL:
				if (pEntity == &player)
					healDelta += pEffect->amount;
				else
					showStat(pEffect->eType, pEffect->amount, pEntity, incCount);
			break;
			case CET_GOLD:
				if (pEntity == &player)
					goldDelta += pEffect->amount;
				else
					showStat(pEffect->eType, pEffect->amount, pEntity, incCount);
			break;
			case CET_XP:
				if (pEntity == &player)
					xpDelta += pEffect->amount;
				else
					showStat(pEffect->eType, pEffect->amount, pEntity, incCount);
			break;
			case CET_ATK:
				if (pEntity == &player)
					atkDelta += pEffect->amount;
				else
					showStat(pEffect->eType, pEffect->amount, pEntity, incCount);
			break;
			case CET_DEF:
				if (pEntity == &player)
					defDelta += pEffect->amount;
				else
					showStat(pEffect->eType, pEffect->amount, pEntity, incCount);
			break;
			case CET_YKEY:
				if (pEntity == &player)
					yKeyDelta += pEffect->amount;
				else
					showStat(pEffect->eType, pEffect->amount, pEntity, incCount);
			break;
			case CET_GKEY:
				if (pEntity == &player)
					gKeyDelta += pEffect->amount;
				else
					showStat(pEffect->eType, pEffect->amount, pEntity, incCount);
			break;
			case CET_BKEY:
				if (pEntity == &player)
					bKeyDelta += pEffect->amount;
				else
					showStat(pEffect->eType, pEffect->amount, pEntity, incCount);
			break;
			case CET_SKEY:
				if (pEntity == &player)
					sKeyDelta += pEffect->amount;
				else
					showStat(pEffect->eType, pEffect->amount, pEntity, incCount);
			break;
			default: break;
		}
		pObj = CueEvents.GetNextPrivateData();
	}

	//Show sum total of simultaneous effects on player all together.
	incCount=0;
	if (atkDelta != 0)
		showStat(CET_ATK, atkDelta, &player, incCount);
	if (defDelta != 0)
		showStat(CET_DEF, defDelta, &player, incCount);
	if (sKeyDelta != 0)
		showStat(CET_SKEY, sKeyDelta, &player, incCount);
	if (bKeyDelta != 0)
		showStat(CET_BKEY, bKeyDelta, &player, incCount);
	if (gKeyDelta != 0)
		showStat(CET_GKEY, gKeyDelta, &player, incCount);
	if (yKeyDelta != 0)
		showStat(CET_YKEY, yKeyDelta, &player, incCount);
	if (healDelta != 0)
		showStat(CET_HEAL, healDelta, &player, incCount);
	if (goldDelta != 0)
		showStat(CET_GOLD, goldDelta, &player, incCount);
	if (xpDelta != 0)
		showStat(CET_XP, xpDelta, &player, incCount);
}

//*****************************************************************************
void CGameScreen::ProcessQuestionPrompts(CCueEvents& CueEvents, SCREENTYPE& eNextScreen)
//Only send message prompts when the player has control over game play.
{
	if (GetScreenType() != SCR_Game)
		return;

	//If a game is being loaded to a turn where the player must answer questions,
	//delay bringing up the question dialog until after the screen transition
	//has completed.
	//
	//Also, if the player is in a combat sequence, questions must wait until afterwards.
	if (g_pTheSM->bTransitioning || this->pCurrentGame->InCombat())
	{
		this->bNeedToProcessDelayedQuestions = CueEvents.HasOccurred(CID_MonsterSpoke);
		return;
	}

	this->bNeedToProcessDelayedQuestions = false; //must be reset before invoking game commands below

	while (const CMonsterMessage *pMsg = this->pCurrentGame->GetUnansweredQuestion())
	{
		switch (pMsg->eType)
		{
			case MMT_OK:
				if (ShowOkMessage(pMsg->eMessageID) == TAG_QUIT)
					eNextScreen = SCR_None;
			break;

			case MMT_YESNO:
			{
				UINT dwRet;
				if (pMsg->eMessageID)
					dwRet = ShowYesNoMessage(pMsg->eMessageID);
				else
					dwRet = ShowYesNoMessage(pMsg->message.c_str());
				switch (dwRet)
				{
					case TAG_QUIT:
						eNextScreen = SCR_None;
					break;
					case TAG_ESCAPE:
					//case TAG_UNDO_FROM_QUESTION: //not currently supported
						eNextScreen = ProcessCommand(CMD_UNDO);
					break;
					case TAG_YES:
						//Recursive call.
						eNextScreen = ProcessCommand(CMD_YES);
					break;
					default:
						//Recursive call.
						eNextScreen = ProcessCommand(CMD_NO);
					break;
				}

				//Refresh room in case command changed its state.
				this->pRoomWidget->DirtyRoom();
			}
			break;

			case MMT_MENU:
			{
				const UINT dwAnswer = GetMessageAnswer(pMsg);
				switch (dwAnswer)
				{
					case TAG_QUIT:
						eNextScreen = SCR_None;
					break;
					case TAG_ESCAPE:
						eNextScreen = SCR_Return;
					break;

					case TAG_UNDO_FROM_QUESTION:
						eNextScreen = ProcessCommand(CMD_UNDO);
					break;

					default:
						eNextScreen = ProcessCommand(CMD_ANSWER, dwAnswer/256, dwAnswer%256);
					break;
				}
			}
			break;
		}

		if (!this->pCurrentGame) //game might have been ended and unloaded by a command above
			break;
	}
}

//*****************************************************************************
void CGameScreen::ProcessMovieEvents(CCueEvents& CueEvents)
//Handle events for playing movies.
//This must be called both before and after the room is drawn to catch all cases.
{
	if (!CueEvents.HasOccurred(CID_PlayVideo))
		return;

	const CAttachableObject *pObj = CueEvents.GetFirstPrivateData(CID_PlayVideo);
	while (pObj)
	{
		const CMoveCoord *pDataVals = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		const UINT dwDataID = static_cast<UINT>(pDataVals->wO);
		if (dwDataID)
		{
			this->pRoomWidget->AllowSleep(false);
			PlayVideo(dwDataID, int(pDataVals->wX), int(pDataVals->wY));

			// Redraw the whole screen after each video to ensure there are no artifacts left anywhere
			Paint(true);
		}

		//Don't reprocess these events if this method is called again.
		//This is done instead of calling ClearEvent so the occurred flag isn't reset.
		CueEvents.Remove(CID_PlayVideo, pObj);

		//The next item is now the first item.
		pObj = CueEvents.GetFirstPrivateData(CID_PlayVideo);
	}
}

//*****************************************************************************
void CGameScreen::Paint(
//Paint the whole screen.
//
//Params:
	bool bUpdateRect)          //(in)   If true (default) and destination
										//    surface is the screen, the screen
										//    will be immediately updated in
										//    the widget's rect.
{
	PaintBackground();
	PaintSign();
	const UINT wTurnNo = this->pRoomWidget->wLastTurn;
	if (this->pTempRoomWidget && this->pTempRoomWidget->IsVisible()) {
		this->pTempRoomWidget->ResetForPaint();
	} else {
		this->pRoomWidget->ResetForPaint();
		if (this->pCurrentGame)
			this->pRoomWidget->wLastTurn = wTurnNo; //don't reanimate last move
	}
	this->pFaceWidget->ResetForPaint();
	RedrawStats(this->pCurrentGame ? this->pCurrentGame->pCombat : NULL, false);
	PaintChildren();
	if (this->bIsScrollVisible)
		PaintScroll(); //must overwrite stats label widgets just drawn by PaintChildren over the scroll

	if (bUpdateRect)
		UpdateRect();
}

//*****************************************************************************
void CGameScreen::PaintClock(const bool /*bShowImmediately*/)	//[default=false]
//Draw clock for current turn, if needed.
{
/*
	//Threat clock is only visible when no scroll is shown.
	if (!this->bIsScrollVisible && this->pCurrentGame)
		this->pClockWidget->ShowClock(this->pCurrentGame->wSpawnCycleCount,
				this->pCurrentGame->bHalfTurn,
				this->pCurrentGame->pRoom->IsTimerNeeded(), bShowImmediately);
	else
		this->pClockWidget->Hide();
*/
}

//*****************************************************************************
void CGameScreen::PlayAmbientSound(
//Plays specified sound effect.
//
//Params:
	const UINT dwDataID, //sound dataID (0 = stop ambient sounds)
	const bool bLoop,     //whether to loop sound indefinitely
	const UINT wX, const UINT wY) //if valid room coord, indicates where sound is located
{
	const bool bPos = this->pCurrentGame->pRoom->IsValidColRow(wX, wY);
	if (!dwDataID)
	{
		//Request to stop ambient sounds.
		if (!bPos)
			StopAmbientSounds();
		else {
			//Stop ambient sounds playing at (x,y).
			vector<ChannelInfo> continuing;
			for (vector<ChannelInfo>::const_iterator ac = this->ambientChannels.begin();
					ac != this->ambientChannels.end(); ++ac)
			{
				if (ac->bUsingPos && wX == static_cast<UINT>(ac->pos[0]) &&
						wY == static_cast<UINT>(ac->pos[1]))
					g_pTheSound->StopSoundOnChannel(ac->wChannel); //stop this sound
				else
					continuing.push_back(*ac); //this sound is still playing
			}
			this->ambientChannels = continuing;
		}
	}

	CStretchyBuffer buffer;
	if (!CDbData::GetRawDataForID(dwDataID, buffer))
		return;

	if (!bPos)
	{
		const int nChannel = g_pTheSound->PlaySoundEffect(buffer, bLoop);
		if (nChannel >= 0)
		{
			//Keep track of which channels are playing ambient sound effects.
			this->ambientChannels.push_back(ChannelInfo(nChannel));
		}
	} else {
		this->fPos[0] = static_cast<float>(wX);
		this->fPos[1] = static_cast<float>(wY);
		const int nChannel = g_pTheSound->PlaySoundEffect(buffer, bLoop, this->fPos);
		if (nChannel >= 0)
		{
			this->ambientChannels.push_back(ChannelInfo(nChannel, true,
					this->fPos[0], this->fPos[1], this->fPos[2]));
		}
	}
}

//*****************************************************************************
void CGameScreen::PlaySpeakerSoundEffect(
//Plays sound effects uttered by a human speaker at volume set for speech.
//Only plays when no speech sound clips are playing to avoid the speaker
//uttering two voices in tandem.
//
//Params:
	const UINT eSEID,
	float* pos, float* vel) //[default=NULL]
const
{
	//If a speech sound clip is already playing, don't play sound effect over it.
	for (vector<ChannelInfo>::const_iterator channel=this->speechChannels.begin();
			channel!=this->speechChannels.end(); ++channel)
	{
		if (g_pTheSound->IsSoundPlayingOnChannel(channel->wChannel) &&
				g_pTheSound->GetSoundIDPlayingOnChannel(channel->wChannel) ==
						static_cast<UINT>(SOUNDLIB::SEID_NONE))
			return;
	}

	g_pTheSound->PlaySoundEffect(eSEID, pos, vel, true);
}

//*****************************************************************************
WSTRING CGameScreen::PrintRank(const int nRanking, const bool bTie)
//Returns: a natural language string displaying a rank/place score
{
	WSTRING wStr;

	WCHAR wRank[10];
	_itoW(nRanking, wRank, 10);

	//English format.
	switch (Language::GetLanguage())
	{
		default:
		case Language::English:
		case Language::Russian: //rules below aren't Russian grammar, but texts still work out correctly
			wStr += wRank;
			if (nRanking >= 11 && nRanking <= 13)
				wStr += g_pTheDB->GetMessageText(MID_th);
			else switch (nRanking % 10)
			{
				case 1: wStr += g_pTheDB->GetMessageText(MID_st); break;
				case 2: wStr += g_pTheDB->GetMessageText(MID_nd); break;
				case 3: wStr += g_pTheDB->GetMessageText(MID_rd); break;
				default: wStr += g_pTheDB->GetMessageText(MID_th); break;
			}
			wStr += wszSpace;
			wStr += g_pTheDB->GetMessageText(MID_Rank);
			if (bTie)
			{
				wStr += wszSpace;
				wStr += g_pTheDB->GetMessageText(MID_TieScore);
			}
		break;
	}
	return wStr;
}

//*****************************************************************************
void CGameScreen::PrepCustomSpeaker(CFiredCharacterCommand *pCmd)
//Call to prepare the speaking entity for custom speaker commands.
{
	CCharacterCommand *pCommand = pCmd->pCommand;
	ASSERT(pCommand);
	CDbSpeech *pSpeech = pCommand->pSpeech;
	ASSERT(pSpeech);
	if (!pSpeech)
		return; //robustness
	if (pSpeech->wCharacter == Speaker_Custom)
	{
		//Check location for a monster/NPC to attach this speech to.
		ASSERT(pCmd->pExecutingNPC);
		CCharacter *pCharacter = DYN_CAST(CCharacter*, CMonster*, pCmd->pExecutingNPC);
		ASSERT(pCharacter);
		UINT x, y;
		pCharacter->getCommandXY(*pCommand, x, y);
		ASSERT(this->pCurrentGame->pRoom->IsValidColRow(x,y));
		CMonster *pMonster = this->pCurrentGame->pRoom->GetMonsterAtSquare(x,y);
		if (pMonster)
		{
			//Attach speech to monster at this tile.
			pCmd->pSpeakingEntity = pMonster;
		} else {
			//No monster there -- create temporary pseudo-monster to attach to.
			CCharacter *pSpeaker = new CCharacter();
			pSpeaker->wX = x;
			pSpeaker->wY = y;
			pSpeaker->wLogicalIdentity = pCharacter->wLogicalIdentity;
			pSpeaker->wIdentity = pCharacter->wIdentity;
			pCmd->pSpeakingEntity = pSpeaker;
			pCmd->bPseudoMonster = true;  //indicates to delete temp monster when speech effect is done
		}
	}
}

//*****************************************************************************
void CGameScreen::ProcessSpeech()
//Play any speech dialog queued to play.
{
	//Determine whether it's time to start playing next speech.
	const Uint32 dwNow = SDL_GetTicks();
	if (dwNow < this->dwNextSpeech)
		return;   //Wait some more.

	//Queue has been emptied and completed.  Return everything to normal.
	if (this->speech.empty() && this->dwNextSpeech)
	{
		//Can start a new speech command at any time.
		this->dwNextSpeech = 0;

		//Return to showing player again.
		this->pFaceWidget->SetSpeaker(false);
		return;
	}

/*
	//Don't continue speech while action is frozen.
	if (this->pCurrentGame && player.wPlacingDoubleType)
		return;
*/

	//Start next queued speech.
	while (!this->speech.empty())
	{
		//Get entity speaking.  Set face portrait.
		CFiredCharacterCommand *pCommand = this->speech.front();
		ASSERT(pCommand->pCommand);
		if (!ProcessSpeechSpeaker(pCommand))
		{
			//This character couldn't speak for some reason.  Skip to next speech.
			delete pCommand;
			this->speech.pop_front();
			continue;
		}

		//Play sound clip if allowed.
		CDbSpeech *pSpeech = pCommand->pCommand->pSpeech;
		ASSERT(pSpeech);
		bool bPlayingSound = false;
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
					//Also track character's ID and turn speech was executed.
					this->speechChannels.push_back(ChannelInfo(nChannel, false, 0.0, 0.0, 0.0,
							pCommand->turnNo, pCommand->scriptID, pCommand->commandIndex));
					this->speechChannels.back().text = pCommand->text;
					bPlayingSound = true;
					//If delay is default, set it to length of the sound sample.
					if (!dwDelay)
						dwDelay = g_pTheSound->GetSoundLength(nChannel);
				}
				pSpeech->UnloadSound();
			}
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
			CSubtitleEffect *pEffect = this->pRoomWidget->AddSubtitle(
					pCommand, max(dwMinDisplayDuration, dwDelay));
			if (bPlayingSound)
			{
				ASSERT(!this->speechChannels.empty());
				this->speechChannels.back().pEffect = pEffect;
			}
		}

		//Mark time next speech will begin.
		this->dwNextSpeech = dwNow + dwDelay;

		//Beethro shouldn't be sleeping while dialogue occurs.
		this->pRoomWidget->AllowSleep(false);

		//Done with this speech command.
		delete pCommand;
		this->speech.pop_front();
		break;
	}
}

//*****************************************************************************
void CGameScreen::ProcessSpeechCues(CCueEvents& CueEvents)
//Process speech cue events.  This might queue speeches for processing.
{
	if (!CueEvents.HasOccurred(CID_Speech))
		return;

	const CAttachableObject *pObj = CueEvents.GetFirstPrivateData(CID_Speech);
	while (pObj)
	{
		const CFiredCharacterCommand *pCmd = DYN_CAST(const CFiredCharacterCommand*, const CAttachableObject*, pObj);
		if (!pCmd->bFlush)
		{
			//Add this speech command to queue.
			const CCharacterCommand *pCommand = pCmd->pCommand;
			ASSERT(pCommand);
			const CDbSpeech *pSpeech = pCommand->pSpeech;
			ASSERT(pSpeech);
			if (pSpeech) //robustness
			{
				PrepCustomSpeaker(const_cast<CFiredCharacterCommand*>(pCmd));
				this->speech.push_back(const_cast<CFiredCharacterCommand*>(pCmd));
			}
		} else {
			//Flush the speech queue.  Stop any currently-playing sounds.
			if (!pCmd->bPlaySound)
				ClearSpeech(true);
			else
			{
				//Minimize duration of any queued speeches so they get displayed now
				//and new ones can be executed immediately thereafter.
				for (deque<CFiredCharacterCommand*>::iterator iter=this->speech.begin();
						iter!=this->speech.end(); ++iter)
				{
					CFiredCharacterCommand *pCommand = *iter;
					pCommand->bPlaySound = false;
					ASSERT(pCommand->pCommand);
					CDbSpeech *pSpeech = pCommand->pCommand->pSpeech;
					ASSERT(pSpeech);
					pSpeech->dwDelay = 1;
				}
				CutSpeech();
			}
			delete pCmd;
		}
		pObj = CueEvents.GetNextPrivateData();
	}

	//Clear refs to data members now to avoid exceptions later.
	CueEvents.ClearEvent(CID_Speech, false);
}

//*****************************************************************************
bool CGameScreen::ProcessSpeechSpeaker(CFiredCharacterCommand *pCommand)
//Returns: true if valid speaker was found and face set.
//False is returned if the character firing the command is to speak, but is now dead.
{
	//If the scripted speaker has died, it can't perform any more speech commands.
	ASSERT(this->pCurrentGame);
	CEntity *pEntity = this->pCurrentGame->getSpeakingEntity(pCommand);
	ASSERT(pEntity);
	if (!pEntity)
		return false; //robustness
	if (pEntity == pCommand->pExecutingNPC && !pCommand->pExecutingNPC->IsAlive())
		return false;

	//Set face giving speech.
	CDbSpeech *pSpeech = pCommand->pCommand->pSpeech;
	ASSERT(pSpeech);
	UINT speaker = pSpeech->wCharacter;

	//Show custom speaker, if set.
	HoldCharacter* pCustomChar = NULL;
	if (speaker >= CUSTOM_CHARACTER_FIRST && speaker != M_NONE)
		pCustomChar = this->pCurrentGame->pHold->GetCharacter(speaker);
	else if (speaker == Speaker_Player) {
		SPEAKER ePlayerSpeaker = Speaker_Beethro;
		ResolvePlayerFace(ePlayerSpeaker, &pCustomChar);
		speaker = ePlayerSpeaker;
	}

	if (speaker >= Speaker_Count && !pCustomChar) //indicates a dangling reference
		pSpeech->wCharacter = Speaker_None;
	if (pSpeech->wCharacter == Speaker_None)
	{
		//Just show player if no speaker is being shown.
		this->pFaceWidget->SetSpeaker(false);
	}
	else {
		//Show who is speaking.
		if (pSpeech->wCharacter != Speaker_Custom && pSpeech->wCharacter != Speaker_Self)
		{
			if (pCustomChar)
				speaker = getSpeakerType(MONSTERTYPE(pCustomChar->wType));
			this->pFaceWidget->SetSpeaker(true, (SPEAKER)speaker, pCustomChar, (MOOD)pSpeech->wMood);
		}
		else {
			//Determine who is speaking.  Show their face, if applicable.
			HoldCharacter* pRemoteCustomChar = NULL;
			if (pCommand->pSpeakingEntity->wType == M_CHARACTER) //another custom speaker?
			{
				CCharacter* pRemoteCharacter = DYN_CAST(CCharacter*, CMonster*,
					pCommand->pSpeakingEntity);
				pRemoteCustomChar = this->pCurrentGame->pHold->GetCharacter(
					pRemoteCharacter->wLogicalIdentity);
			}
			UINT wIdentity = pCommand->pSpeakingEntity->GetIdentity();
			/*if (wIdentity == M_EYE_ACTIVE)
				wIdentity = M_EYE;       //map to same type
			*/
			wIdentity = getSpeakerType((MONSTERTYPE)wIdentity);
			if (wIdentity != Speaker_None) {
				this->pFaceWidget->SetSpeaker(true, (SPEAKER)wIdentity, pRemoteCustomChar, (MOOD)pSpeech->wMood);
			}
		}
	}
	return true;
}

//*****************************************************************************
void CGameScreen::ReattachRetainedSubtitles()
//Call this method after a move undo has cleared most room effects, but
//speech subtitles in progress have been retained.
//Since the room data have been reinstantiated, these dangling effects need
//to be hooked back in to the entities they are tracking.
{
	ASSERT(this->pCurrentGame);
	for (vector<ChannelInfo>::const_iterator channel=this->speechChannels.begin();
			channel!=this->speechChannels.end(); ++channel)
	{
		CSubtitleEffect *pEffect = channel->pEffect;
		if (pEffect)
		{
			//Find current instance of the entity this subtitle effect is following.
			CCharacter *pCharacter = this->pCurrentGame->
						GetCharacterWithScriptID(channel->scriptID);
			ASSERT(pCharacter);
			if (pCharacter) //robustness
			{
				CFiredCharacterCommand *pSpeechCommand = new CFiredCharacterCommand(pCharacter,
						&(pCharacter->commands[channel->commandIndex]), channel->turnNo,
						channel->scriptID, channel->commandIndex);
				pSpeechCommand->text = channel->text;
				PrepCustomSpeaker(pSpeechCommand);
				if (ProcessSpeechSpeaker(pSpeechCommand))
				{
					CEntity *pEntity = this->pCurrentGame->getSpeakingEntity(pSpeechCommand);
					pEffect->FollowCoord(pEntity);
					this->pRoomWidget->AddToSubtitles(pEffect);
				} else {
					delete pEffect;
				}
				delete pSpeechCommand;
			} else {
				delete pEffect;
			}
		}
	}
}

//*****************************************************************************
void CGameScreen::RestartRoom(int nCommand, CCueEvents& CueEvents)
{
	/*
		//Rewind moves to previous checkpoints or restart the room.
		if (nCommand == CMD_RESTART_FULL)
			this->pCurrentGame->RestartRoom(this->sCueEvents);
		else if (nCommand == CMD_RESTART_PARTIAL)
			this->pCurrentGame->RestartRoomFromLastDifferentCheckpoint(this->sCueEvents);
		else
			this->pCurrentGame->RestartRoomFromLastCheckpoint(this->sCueEvents);
	*/
	const CIDSet mappedRooms = this->pCurrentGame->GetExploredRooms(true);
	const CIDSet roomPreviews = this->pCurrentGame->GetPreviouslyExploredRooms();

	delete g_pPredictedCombat;
	g_pPredictedCombat = NULL;

	this->pCurrentGame->RestartRoom(this->sCueEvents);

	//	this->bRoomClearedOnce = this->pCurrentGame->IsCurrentRoomPendingExit();
	this->wUndoToTurn = this->pCurrentGame->wTurnNo;

	bool bReloadEntireMap = roomPreviews.containsAny(mappedRooms); //preview state could have changed for these rooms
	if (!bReloadEntireMap) {
		const CIDSet nowMappedRooms = this->pCurrentGame->GetExploredRooms(true);
		if (nowMappedRooms.size() != mappedRooms.size())
			bReloadEntireMap = true;
	}
		
	UpdateUIAfterRoomRestart(bReloadEntireMap);
}

void CGameScreen::UpdateUIAfterRoomRestart(const bool bReloadEntireMap) //[default=false]
{
	ClearSpeech();

	StopAmbientSounds();
	g_pTheSound->StopAllSoundEffects(); //stop any game sounds that were playing
	SetGameAmbience(true);
	AmbientSoundSetup(); //determine what sounds should be playing now

	this->pRoomWidget->ClearEffects();
	this->pRoomWidget->RenderRoomLighting();
	this->pRoomWidget->ResetForPaint();

	if (bReloadEntireMap) {
		this->pMapWidget->LoadFromCurrentGame(this->pCurrentGame);
	} else {
		this->pMapWidget->DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom, this->pCurrentGame->pRoom->mapMarker);
	}
	this->pMapWidget->RequestPaint();

	UpdateSign();

	SwirlEffect();
}

//*****************************************************************************
void CGameScreen::RetainSubtitleCleanup(const bool bVal) //[default=false]
//After a move has been undone and currently playing speech and subtitles
//have been successfully retained, unmark subtitle effects from being
//retained on effect clears any longer.
{
	for (vector<ChannelInfo>::const_iterator channel=this->speechChannels.begin();
			channel!=this->speechChannels.end(); ++channel)
	{
		CSubtitleEffect *pEffect = channel->pEffect;
		if (pEffect)
			pEffect->RequestRetainOnClear(bVal);
	}
}

//*****************************************************************************
void CGameScreen::showStat(const UINT eType, const int delta, CEntity *pEntity, UINT& count)
//Add an effect showing how some stat value was altered at pEntity's position.
{
	CCoord coord(pEntity->wX, pEntity->wY);
	UINT px = coord.wX * g_pTheBM->CX_TILE;
	UINT py = coord.wY * g_pTheBM->CY_TILE - (coord.wY ? g_pTheBM->CY_TILE/2 : 0);
	py += count * g_pTheBM->CY_TILE/2;
	switch (eType)
	{
		case CET_NODAMAGE:
		case CET_STRONGHIT:
		case CET_NODEFENSEHIT:
		case CET_HARM:
		default:
			//not handled here
		return; //don't increment count
		case CET_HEAL:
		{
			static const SURFACECOLOR greenblue = {0, 255, 128};
			this->pRoomWidget->AddLastLayerEffect(new CFloatTextEffect(this->pRoomWidget,
				px, py,
				delta, greenblue, F_Title));
		}
		break;
		case CET_GOLD:
		{
			static const SURFACECOLOR gold = {255, 255, 0};
			WCHAR temp[12];
			WSTRING text;
			if (delta > 0)
				text += wszPlus;
			text += _itoW(delta, temp, 10);
			this->pRoomWidget->AddLastLayerEffect(new CFloatTextEffect(this->pRoomWidget,
				px, py,
				text, gold, F_Sign, 1000, 20.0));
		}
		break;
		case CET_XP:
		if (delta != 0)
		{
			static const SURFACECOLOR golden = {255, 255, 196};
			static const SURFACECOLOR darkgolden = {128, 128, 64};
			WCHAR temp[12];
			WSTRING text;
			if (delta > 0)
				text += wszPlus;
			text += _itoW(delta, temp, 10);
			text += wszSpace;
			text += g_pTheDB->GetMessageText(MID_XPStat);
			this->pRoomWidget->AddLastLayerEffect(new CFloatTextEffect(this->pRoomWidget,
				px, py,
				text,  delta > 0 ? golden : darkgolden, F_Sign, 1000, 20.0));
		}
		break;
		case CET_ATK:
		if (delta != 0)
		{
			static const SURFACECOLOR reddish = {255, 128, 192};
			static const SURFACECOLOR darkReddish = {220, 100, 160};
			WCHAR temp[12];
			WSTRING text;
			if (delta > 0)
				text += wszPlus;
			text += _itoW(delta, temp, 10);
			text += wszSpace;
			text += g_pTheDB->GetMessageText(MID_ATKStat);
			this->pRoomWidget->AddLastLayerEffect(new CFloatTextEffect(this->pRoomWidget,
				px, py,
				text, delta > 0 ? reddish : darkReddish,
				F_Title, 1000, 20.0));
		}
		break;
		case CET_DEF:
		if (delta != 0)
		{
			static const SURFACECOLOR purple = {192, 128, 255};
			static const SURFACECOLOR darkPurple = {160, 100, 220};
			WCHAR temp[12];
			WSTRING text;
			if (delta > 0)
				text += wszPlus;
			text += _itoW(delta, temp, 10);
			text += wszSpace;
			text += g_pTheDB->GetMessageText(MID_DEFStat);
			this->pRoomWidget->AddLastLayerEffect(new CFloatTextEffect(this->pRoomWidget,
				px, py,
				text, delta > 0 ? purple : darkPurple,
				F_Title, 1000, 20.0));
		}
		break;
		case CET_YKEY:
		if (delta != 0)
		{
			static const SURFACECOLOR yellow = {255, 255, 60};
			WCHAR temp[12];
			WSTRING text;
			if (delta > 0)
				text += wszPlus;
			text += _itoW(delta, temp, 10);
			text += wszSpace;
			text += g_pTheDB->GetMessageText(MID_YKEYStat);
			this->pRoomWidget->AddLastLayerEffect(new CFloatTextEffect(this->pRoomWidget,
				coord.wX * g_pTheBM->CX_TILE, coord.wY * g_pTheBM->CY_TILE - (coord.wY ? g_pTheBM->CY_TILE : 0),
				text, yellow, F_Title, 1000, 20.0));
		}
		break;
		case CET_GKEY:
		if (delta != 0)
		{
			static const SURFACECOLOR green = {100, 255, 100};
			WCHAR temp[12];
			WSTRING text;
			if (delta > 0)
				text += wszPlus;
			text += _itoW(delta, temp, 10);
			text += wszSpace;
			text += g_pTheDB->GetMessageText(MID_GKEYStat);
			this->pRoomWidget->AddLastLayerEffect(new CFloatTextEffect(this->pRoomWidget,
				px, py,
				text, green, F_Title, 1000, 20.0));
		}
		break;
		case CET_BKEY:
		if (delta != 0)
		{
			static const SURFACECOLOR blue = {180, 180, 255};
			WCHAR temp[12];
			WSTRING text;
			if (delta > 0)
				text += wszPlus;
			text += _itoW(delta, temp, 10);
			text += wszSpace;
			text += g_pTheDB->GetMessageText(MID_BKEYStat);
			this->pRoomWidget->AddLastLayerEffect(new CFloatTextEffect(this->pRoomWidget,
				px, py,
				text, blue, F_Title, 1000, 20.0));
		}
		break;
		case CET_SKEY:
		if (delta != 0)
		{
			static const SURFACECOLOR whitish = {240, 240, 240};
			WCHAR temp[12];
			WSTRING text;
			if (delta > 0)
				text += wszPlus;
			text += _itoW(delta, temp, 10);
			text += wszSpace;
			text += g_pTheDB->GetMessageText(MID_SKEYStat);
			this->pRoomWidget->AddLastLayerEffect(new CFloatTextEffect(this->pRoomWidget,
				px, py,
				text, whitish, F_Title, 1000, 20.0));
		}
		break;
	}
	++count;
}

//*****************************************************************************
void CGameScreen::HideBigMap()
//Hides the big level map being displayed, which returns input to normal play.
{
	ASSERT(this->bShowingBigMap);

	CScrollableWidget *pScrollingMap =
			DYN_CAST(CScrollableWidget*, CWidget*, GetWidget(TAG_BIGMAPCONTAINER));
	pScrollingMap->Hide();
	this->pBigMapWidget->Hide();

	this->bShowingBigMap = false;

	//Draw widgets.
	this->pRoomWidget->DirtyRoom();
	PaintChildren();
	UpdateRect();
}

//*****************************************************************************
void CGameScreen::ShowBigMap()
//Display scrollable minimap of the entire level over the room.
//While this map is shown, play commands are suspended.
//Clicking on visited rooms on this map allows the player to view them in their
//current state.  Clicking elsewhere or hitting a key returns to normal play.
{
	ASSERT(!this->bShowingBigMap);
	ASSERT(this->pCurrentGame);

	//Don't expand minimap whenever the current game move is still being resolved.
	if (this->pCurrentGame->InCombat())
		return;
	if (this->pCurrentGame->dwCutScene)
		return; //in order to not desynchronize cut scene
	if (this->bNeedToProcessDelayedQuestions)
		return;

	if (!this->pBigMapWidget->LoadFromCurrentGame(this->pCurrentGame))
		return;
	//Show active room in its current state.
	this->pBigMapWidget->DrawMapSurfaceFromRoom(this->pCurrentGame->pRoom,
			this->pCurrentGame->pRoom->mapMarker);

	this->bShowingBigMap = true;

	CScrollableWidget *pScrollingMap =
			DYN_CAST(CScrollableWidget*, CWidget*, GetWidget(TAG_BIGMAPCONTAINER));
	pScrollingMap->Show();
	this->pBigMapWidget->Show();

	this->pRoomWidget->AllowSleep(false);
}

//***************************************************************************************
void CGameScreen::ShowStatsForMonsterAt(const UINT wX, const UINT wY)
//Display stats for the monster at (x,y) in the monster pane on the sidebar.
{
	ASSERT(this->pCurrentGame);
	if (this->pCurrentGame->InCombat())
		return; //don't pop-up different stats when already fighting

	ShowStatsForMonster(this->pCurrentGame->pRoom->GetMonsterAtSquare(wX, wY));
}

void CGameScreen::ShowStatsForMonster(CMonster *pMonster)
{
	if (pMonster)
	{
		const CMonster *pOrigMonster = pMonster;
		pMonster = pMonster->GetOwningMonster();

		if (g_pPredictedCombat)
			delete g_pPredictedCombat;
		g_pPredictedCombat = new CCombat(this->pCurrentGame, pMonster, true, pOrigMonster->wX, pOrigMonster->wY);
		RedrawStats(NULL, true); //predicted combat will be shown if no combat is actually in progress
	}
}

//*****************************************************************************
UINT CGameScreen::ShowRoom(CDbRoom *pRoom) //room to display
//Temporarily display another room in place of the current room.
//
//Returns: roomID of a different room to display, or 0 for none
{
	ASSERT(pRoom);

	UINT newShowRoomID = 0;
	const UINT originalRoomID = pRoom->dwRoomID;

	this->pTempRoomWidget->HidePlayer();
	VERIFY(this->pTempRoomWidget->LoadFromRoom(pRoom));
	this->pTempRoomWidget->pCurrentGame = pRoom->GetCurrentGame();

	if (this->bShowingBigMap)
		this->pBigMapWidget->Hide(); //hide big map while displaying another room
	this->pRoomWidget->Hide();
	this->pTempRoomWidget->Show();

	this->pTempRoomWidget->Paint();
	ShowRoomCoords(this->pTempRoomWidget->pRoom);
	UpdateRect();

	//Display until a key/button is pressed.
	bool bShow = true;
	while (bShow)
	{
		//Get any events waiting in the queue.
		SDL_Event event;
		while (PollEvent(&event))
		{
			if (IsDeactivating()) //skip events
				continue;

			switch (event.type)
			{
				case SDL_WINDOWEVENT:
					OnWindowEvent(event.window);
				break;

				case SDL_MOUSEBUTTONDOWN:
				{
					bShow = false;

					//If the map widget was clicked on, then temporarily display the new room immediately.
					CWidget *pWidget = GetWidgetContainingCoords(event.button.x, event.button.y, WT_Unspecified);
					if (pWidget && pWidget->GetTagNo() == TAG_MAP)
					{
						UINT roomX, roomY;
						this->pMapWidget->GetRoomAtCoords(event.button.x, event.button.y, roomX, roomY);
						newShowRoomID = this->pCurrentGame->pLevel->GetRoomIDAtCoords(roomX, roomY);
						if (newShowRoomID == this->pCurrentGame->pRoom->dwRoomID ||
								newShowRoomID == originalRoomID)
							newShowRoomID = 0; //no new room to display
					}
				}
				break;

				case SDL_KEYDOWN:
				{
					int nCommand;
					switch (event.key.keysym.sym)
					{
						case SDLK_UP: nCommand = CMD_N; break;
						case SDLK_DOWN: nCommand = CMD_S; break;
						case SDLK_LEFT: nCommand = CMD_W; break;
						case SDLK_RIGHT: nCommand = CMD_E; break;
						default: nCommand = GetCommandForKeysym(event.key.keysym.sym); break;
					}
					switch (nCommand)
					{
						case CMD_BATTLE_KEY:
							ShowMonsterStats(this->pTempRoomWidget->pRoom, this->pTempRoomWidget);
							this->pTempRoomWidget->DirtyRoom(); //room isn't auto redrawn on dialogue erase
							this->pTempRoomWidget->Paint();
							UpdateRect();
							break;
						case CMD_N:
							DisplayAdjacentTempRoom(N);
							break;
						case CMD_W:
							DisplayAdjacentTempRoom(W);
							break;
						case CMD_S:
							DisplayAdjacentTempRoom(S);
							break;
						case CMD_E:
							DisplayAdjacentTempRoom(E);
							break;
						default:
							bShow = false;
							break;
					}
				}
				break;

				case SDL_QUIT:
					this->bQuitPrompt = true;
					if (ShowYesNoMessage(MID_ReallyQuit) != TAG_NO)
					{
						bShow = false;
						GoToScreen(SCR_None);
					}
					this->bQuitPrompt = false;
				break;
			}
		}

		//Update music (switch song or continue music fade if one is in progress).
		g_pTheSound->UpdateMusic();
		SDL_Delay(1); //be nice to the CPU
	}

	this->pTempRoomWidget->Hide();
	this->pTempRoomWidget->pCurrentGame = NULL;

	if (this->bShowingBigMap)
		this->pBigMapWidget->Show();

	//Reload graphics for current room.
	this->pRoomWidget->Show();
	this->pRoomWidget->LoadRoomImages();
	this->pRoomWidget->UpdateFromCurrentGame();
	UpdateSign();
	Paint();

	//Show a new room?
	return newShowRoomID;
}

//*****************************************************************************
void CGameScreen::DisplayAdjacentTempRoom(const UINT direction)
{
	ASSERT(this->pTempRoomWidget);
	ASSERT(this->pTempRoomWidget->IsVisible());

	UINT newRoomX = this->pTempRoomWidget->pRoom->dwRoomX;
	UINT newRoomY = this->pTempRoomWidget->pRoom->dwRoomY;
	switch (direction)
	{
		case N: newRoomY--; break;
		case S: newRoomY++; break;
		case E: newRoomX++; break;
		case W: newRoomX--; break;
		default: ASSERT(!"Unsupported temp room pan direction"); return;
	}

	if (!this->pCurrentGame->IsRoomAtCoordsExplored(newRoomX, newRoomY)) {
		g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);
		return;
	}

	CCueEvents cueEventsIgnored;
	CCurrentGame *pTempGame = this->pTempRoomWidget->pCurrentGame;
	pTempGame->pPlayer->wIdentity = pTempGame->pPlayer->wAppearance = M_NONE; //not in room
	if (!pTempGame->LoadNewRoomForExit(direction, cueEventsIgnored)) {
		g_pTheSound->PlaySoundEffect(SEID_CHECKPOINT);
	} else {
		VERIFY(this->pTempRoomWidget->LoadFromRoom(pTempGame->pRoom));
		const bool bShowPlayer = this->pRoomWidget->bShowingPlayer &&
				pTempGame->dwRoomID == this->pCurrentGame->dwRoomID;
		if (bShowPlayer) {
			pTempGame->SetPlayerRole(this->pCurrentGame->pPlayer->wIdentity);
			pTempGame->SetPlayer(this->pCurrentGame->pPlayer->wX, this->pCurrentGame->pPlayer->wY);
			pTempGame->UpdatePrevCoords();
		}
		this->pTempRoomWidget->ShowPlayer(bShowPlayer);
		this->pTempRoomWidget->ShowRoomTransition(direction, bShowPlayer);
		this->pTempRoomWidget->Paint();
		ShowRoomCoords(this->pTempRoomWidget->pRoom);
		UpdateRect();
	}
}

//*****************************************************************************
void CGameScreen::ShowRoomCoords(CDbRoom *pRoom)
{
	ASSERT(pRoom);

	WSTRING wstr;
	pRoom->GetLevelPositionDescription(wstr, true);
	if (!wstr.empty())
	{
		wstr += wszColon;
		wstr += wszSpace;
	}
	wstr += g_pTheDB->GetMessageText(MID_DisplayingOtherRoom);
	SetSignText(wstr.c_str());
	this->signColor = Blue;

	PaintSign();
}

//*****************************************************************************
void CGameScreen::ShowRoomTemporarily(UINT roomID)
//Temporarily show the indicated room instead of the current room.
{
	if (!roomID)
		return;

	RemoveToolTip();

Loop:
	CCurrentGame *pTempGame = g_pTheDB->GetDummyCurrentGame();
	*pTempGame = *this->pCurrentGame;
	if (!pTempGame->PrepTempGameForRoomDisplay(roomID)) {
		delete pTempGame;
		return;
	}

	this->bShowingTempRoom = true;
	this->bNoMoveByCurrentMouseClick = true;

	const UINT showNewRoomID = ShowRoom(pTempGame->pRoom);
	delete pTempGame;

	this->bShowingTempRoom = false;

	//If a request to show a different room was received,
	//restart the method with the new room's info.
	if (showNewRoomID)
	{
		ExploredRoom *pExpRoom = this->pCurrentGame->getExploredRoom(showNewRoomID);
		if (pExpRoom && !pExpRoom->bMapOnly)
		{
			roomID = showNewRoomID;
			goto Loop;
		}
	}
}

//*****************************************************************************
void CGameScreen::ShowChatHistory(CEntranceSelectDialogWidget* pBox)
{
	g_pTheSound->PauseSounds();

	const Uint32 dwSpeechRemaining = this->dwNextSpeech - SDL_GetTicks();
	this->bIsDialogDisplayed = true;
	this->pRoomWidget->SetEffectsFrozen(true);

	CDrodScreen::ShowChatHistory(pBox);

	// We compute remaining speech time and replace it instead of just calculating the time the dialog
	// was visible because dwNextSpeech can also be updated by focus changes
	if (this->dwNextSpeech)
		this->dwNextSpeech = SDL_GetTicks() + dwSpeechRemaining;
	this->bIsDialogDisplayed = false;

	g_pTheSound->UnpauseSounds();
}

//*****************************************************************************
/*
void CGameScreen::ShowLockIcon(const bool bShow)
//Show hide persistent lock icon.
{
	if (bShow)
	{
		CSubtitleEffect *pEffect = new CSubtitleEffect(this, NULL,
				g_pTheDB->GetMessageText(MID_RoomLockIcon), lockColor, 0);
		pEffect->SetOffset(161, 7);
		pEffect->SetAlpha(128);
		this->pEffects->AddEffect(pEffect);
	} else {
		this->pEffects->RemoveEffectsOfType(EFFECTLIB::ESUBTITLE);
	}
}
*/

//*****************************************************************************
void CGameScreen::UpdatePlayerFace()
// Refresh player face to match reality
{
	if (!this->pCurrentGame)
		return;

	HoldCharacter* pPlayerHoldCharacter = NULL;

	//Handle custom character images specially.
	UINT dwCharID = this->pCurrentGame->pPlayer->wIdentity;
	if (dwCharID >= CUSTOM_CHARACTER_FIRST && dwCharID != M_NONE)
	{
		pPlayerHoldCharacter = this->pCurrentGame->pHold->GetCharacter(dwCharID);
	}

	SPEAKER player = getSpeakerType(MONSTERTYPE(dwCharID));
	if (pPlayerHoldCharacter) {
		player = getSpeakerType(MONSTERTYPE(pPlayerHoldCharacter->wType));
	}

	if (player == Speaker_None)
	{
		//If player is not in the room, show Beethro's face if NPC Beethro is in the room.
		CMonster* pNPCBeethro = this->pCurrentGame->pRoom->GetNPCBeethro();
		if (pNPCBeethro) {
			player = Speaker_Beethro;
		}
	}

	this->pFaceWidget->SetSleeping(false);
	this->pFaceWidget->SetCharacter(PlayerRole, player, pPlayerHoldCharacter);
}

//*****************************************************************************
void CGameScreen::ResolvePlayerFace(
	SPEAKER& pSpeaker, //(out) player's speaker
	HoldCharacter** playerHoldCharacter) //(out) custom player character if
// Refresh player face to match reality
{
	if (!this->pCurrentGame)
		return;

	//Handle custom character images specially.
	UINT dwCharID = this->pCurrentGame->pPlayer->wIdentity;
	if (dwCharID >= CUSTOM_CHARACTER_FIRST && dwCharID != M_NONE)
	{
		*playerHoldCharacter = this->pCurrentGame->pHold->GetCharacter(dwCharID);
	}

	pSpeaker = getSpeakerType(MONSTERTYPE(dwCharID));
	if (pSpeaker == Speaker_None)
	{
		//If player is not in the room, show Beethro's face if NPC Beethro is in the room.
		CMonster* pNPCBeethro = this->pCurrentGame->pRoom->GetNPCBeethro();
		if (pNPCBeethro) {
			pSpeaker = Speaker_Beethro;
		}
	}
}

//*****************************************************************************
void CGameScreen::ShowSpeechLog()
//Pop up a dialog box that contains a log of the speech played in the current room.
{
	if (!this->pCurrentGame)
		return;
	if (this->pCurrentGame->roomSpeech.empty())
		return;
	if (this->bShowingBigMap)
		return;

	this->pSpeechBox->SetPrompt(MID_SpeechLogTitle);
	this->pSpeechBox->SetCurrentGame(this->pCurrentGame);
	this->pSpeechBox->PopulateList(CEntranceSelectDialogWidget::Speech);
	this->pSpeechBox->SelectItem(0);

	g_pTheSound->PauseSounds();

	const Uint32 dwSpeechRemaining = this->dwNextSpeech - SDL_GetTicks();
	this->bIsDialogDisplayed = true;
	this->pRoomWidget->SetEffectsFrozen(true);

	UINT dwItemID=0;
	int playingChannel=-1;
	do {
		const CEntranceSelectDialogWidget::BUTTONTYPE eButton =
				(CEntranceSelectDialogWidget::BUTTONTYPE)this->pSpeechBox->Display();
		dwItemID = eButton == CEntranceSelectDialogWidget::OK ? this->pSpeechBox->GetSelectedItem() : 0;
		if (dwItemID)
		{
			//Play any attached speech for this line.
			ASSERT(this->pCurrentGame->roomSpeech.size() >= dwItemID);
			CCharacterCommand *pCmd = this->pCurrentGame->roomSpeech[dwItemID - 1].pSpeechCommand;
			ASSERT(pCmd);

			const CDbDatum *pSound = pCmd->pSpeech->GetSound();
			if (pSound)
			{
				//Stop any replaying sound.
				if (playingChannel >= 0)
					g_pTheSound->StopSoundOnChannel(playingChannel);

				playingChannel = g_pTheSound->PlayVoice(pSound->data);
				pCmd->pSpeech->UnloadSound();
			}
		}
	} while (dwItemID != 0);

	if (playingChannel >= 0)
		g_pTheSound->StopSoundOnChannel(playingChannel);

	// We compute remaining speech time and replace it instead of just calculating the time the dialog
	// was visible because dwNextSpeech can also be updated by focus changes
	if (this->dwNextSpeech)
		this->dwNextSpeech = SDL_GetTicks() + dwSpeechRemaining;
	this->bIsDialogDisplayed = false;

	g_pTheSound->UnpauseSounds();

	Paint();
}

//*****************************************************************************
void CGameScreen::StopAmbientSounds()
//Stops all playing ambient sound effects.
{
	for (vector<ChannelInfo>::const_iterator channel=this->ambientChannels.begin();
			channel!=this->ambientChannels.end(); ++channel)
	{
		//Stop this channel from playing if it's an ambient sound clip.
		if (g_pTheSound->IsSoundPlayingOnChannel(channel->wChannel) &&
				g_pTheSound->GetSoundIDPlayingOnChannel(channel->wChannel) ==
						static_cast<UINT>(SOUNDLIB::SEID_NONE))
			VERIFY(g_pTheSound->StopSoundOnChannel(channel->wChannel));
	}
	this->ambientChannels.clear();
}

//*****************************************************************************
void CGameScreen::SwirlEffect()
//Swirl effect to highlight player.
{
	this->pRoomWidget->RemoveMLayerEffectsOfType(ESWIRL);
	this->pRoomWidget->AddMLayerEffect(
			new CSwordsmanSwirlEffect(this->pRoomWidget, this->pCurrentGame));
}

//*****************************************************************************
void CGameScreen::SynchScroll()
//Sets scroll display for the current game state.
{
	const bool bOnScroll = this->pCurrentGame->pRoom->GetTSquare(
			this->pCurrentGame->pPlayer->wX, this->pCurrentGame->pPlayer->wY) == T_SCROLL;
	this->bIsScrollVisible = bOnScroll;
	this->pFaceWidget->SetReading(bOnScroll);
	if (bOnScroll)
	{
		this->pScrollLabel->SetText(this->pCurrentGame->GetScrollTextAt(
				this->pCurrentGame->pPlayer->wX, this->pCurrentGame->pPlayer->wY).c_str());
	}
}

//*****************************************************************************
void CGameScreen::ToggleBigMap()
//Toggle pop-up level map.
{
	if (this->bShowingBigMap)
		HideBigMap();
	else
		ShowBigMap();

	//Draw widgets
	PaintChildren();

	PaintScroll(); //must overwrite/refresh stats label widgets just drawn by PaintChildren over the scroll
	if (!this->bIsScrollVisible)
		RedrawStats(this->pCurrentGame ? this->pCurrentGame->pCombat : NULL, false);

	UpdateRect();
}

//*****************************************************************************
void CGameScreen::UndoMove()
//Undoes last game move made, if any.
{
	if (!this->pCurrentGame->wTurnNo)
		return; //no turns to undo

	delete g_pPredictedCombat;
	g_pPredictedCombat = NULL;

//	const bool bRecordingDemo = this->pCurrentGame->IsDemoRecording();

	const CIDSet exploredRooms = this->pCurrentGame->GetExploredRooms();
	const CIDSet mappedRooms = this->pCurrentGame->GetMappedRooms();
	const CIDSet previewedRooms = this->pCurrentGame->GetPreviouslyExploredRooms();

	//If undo to turn is set to the current turn,
	//then just undo one turn.
	if (this->pCurrentGame->wTurnNo <= this->wUndoToTurn)
	{
		ASSERT(this->pCurrentGame->wTurnNo > 0);
		this->wUndoToTurn = this->pCurrentGame->wTurnNo - 1;

		//Undo a move.
		//When undoing a cut scene, return to before the cut scene began.
		do {
			ClearCueEvents();
			this->pCurrentGame->UndoCommand(this->sCueEvents);
		} while (this->pCurrentGame->wTurnNo && this->pCurrentGame->dwCutScene);
	} else {
		//Undo until the specified turn is reached.
		//Question commands might be interspersed with turn-incrementing commands,
		//causing more than one iteration to be required.
		while (this->pCurrentGame->wTurnNo > this->wUndoToTurn)
		{
			const UINT delta = this->pCurrentGame->wTurnNo - this->wUndoToTurn;
			ClearCueEvents();
			this->pCurrentGame->UndoCommands(delta, this->sCueEvents);
		}

		//If undo ended in a cut scene, undo back past the cut scene as well.
		while (this->pCurrentGame->dwCutScene && this->pCurrentGame->wTurnNo)
		{
			ClearCueEvents();
			this->pCurrentGame->UndoCommand(this->sCueEvents);
		}
	}
	//If something unexpected happened while undoing (e.g. altered game logic
	//since the move sequence was created), just undo back to the latest valid move.
	while (!this->pCurrentGame->bIsGameActive && this->pCurrentGame->wTurnNo)
	{
		ClearCueEvents();
		this->pCurrentGame->UndoCommand(this->sCueEvents);
	}

/*
	//Refresh game screen display info.
	if (bRecordingDemo)
	{
		if (this->pCurrentGame->IsDemoRecording())
		{
			//Don't show a long delay between last move and next move in the recorded demo.
			this->pCurrentGame->Commands.ResetTimeOfLastAdd();
		}
	}
*/

	//Refresh map if something changed by undoing.
	bool bReloadEntireMap = previewedRooms.containsAny(exploredRooms); //these may have changed
	if (!bReloadEntireMap) {
		const CIDSet nowExploredRooms = this->pCurrentGame->GetExploredRooms();
		const CIDSet nowMappedRooms = this->pCurrentGame->GetMappedRooms();
		if (nowMappedRooms != mappedRooms || nowExploredRooms != exploredRooms)
			bReloadEntireMap = true;
	}

	UpdateUIAfterMoveUndo(bReloadEntireMap);
}

void CGameScreen::UpdateUIAfterMoveUndo(bool bReloadEntireMap) //[default=false]
{
	this->pRoomWidget->pRoom = this->pCurrentGame->pRoom; //synch

	StopAmbientSounds();
	ClearSpeech(false);  //retain speech that started before the previous turn

	this->pRoomWidget->ClearEffects();
	this->pRoomWidget->RenderRoomLighting();

	UpdateSign();

	ReattachRetainedSubtitles(); //after clearing effects, rejoin subtitles to current room objects

	if (bReloadEntireMap) {
		this->pMapWidget->LoadFromCurrentGame(this->pCurrentGame);
	}

	SetGameAmbience(true);
	AmbientSoundSetup();
	DrawCurrentTurn();

	RetainSubtitleCleanup();
}

//*****************************************************************************
void CGameScreen::UpdateSound()
//Update listener and sounds state.
{
	//Update player position and orientation in sound engine.
	//const UINT wO = this->pCurrentGame->pPlayer->wO;
	float pos[3] = {static_cast<float>(this->pCurrentGame->pPlayer->wX),
		static_cast<float>(this->pCurrentGame->pPlayer->wY), 0.0f};
	//float dir[3] = {static_cast<float>(nGetOX(wO)), static_cast<float>(nGetOY(wO)), 0.0f};
	float dir[3] = {0.0f,-1.0f,0.0f};	//face "ears" in direction user looks at room
	static float vel[3] = {0.0f, 0.0f, 0.0f}; //no velocity
	g_pTheSound->Update(pos, dir, vel);

	//Update positioned speech and ambient sound effects.
	vector<ChannelInfo>::const_iterator channel;
	vector<ChannelInfo> continuing; //sounds that are still playing
	for (channel=this->speechChannels.begin();
			channel!=this->speechChannels.end(); ++channel)
	{
		if (g_pTheSound->IsSoundPlayingOnChannel(channel->wChannel) &&
				g_pTheSound->GetSoundIDPlayingOnChannel(channel->wChannel) ==
						static_cast<UINT>(SOUNDLIB::SEID_NONE))
		{
			if (channel->bUsingPos)
				g_pTheSound->Update(channel->wChannel, (float*)channel->pos);
			continuing.push_back(*channel);
		}
	}
	this->speechChannels = continuing;

	continuing.clear();
	for (channel=this->ambientChannels.begin();
			channel!=this->ambientChannels.end(); ++channel)
	{
		if (g_pTheSound->IsSoundPlayingOnChannel(channel->wChannel) &&
				g_pTheSound->GetSoundIDPlayingOnChannel(channel->wChannel) ==
						static_cast<UINT>(SOUNDLIB::SEID_NONE))
		{
			if (channel->bUsingPos)
				g_pTheSound->Update(channel->wChannel, (float*)channel->pos);
			continuing.push_back(*channel);
		}
	}
	this->ambientChannels = continuing;
}

//*****************************************************************************
void CGameScreen::UpdateEffectsFreeze()
{
	bool bFreezeEffects = !WindowHasFocus() || this->bIsDialogDisplayed;

	this->pRoomWidget->SetEffectsFrozen(bFreezeEffects);
}

//*****************************************************************************
bool CGameScreen::UploadDemoPolling()
//As the current game queues scores for upload, process them here.
//As results are received, display them onscreen.
//
//Returns: false if score queues are empty, else true
{
	if (!this->pCurrentGame) return false;	//where upload info is stored
	if (!g_pTheNet) return false;

	//Ensure last request was completed before another upload is initiated.
	if (this->wUploadingScoreHandle)
	{
		ASSERT(!this->dwUploadingDemo || !this->dwUploadingSavedGame); //only one at a time

		const int status = g_pTheNet->GetStatus(this->wUploadingScoreHandle);
		if (status >= 0)
		{
			//Get ranking.
			CStretchyBuffer* pBuffer = g_pTheNet->GetResults(this->wUploadingScoreHandle);
			if (!pBuffer) {
				//Fail gracefully from server-related errors.
				const long responseCode = CInternet::GetErrorResponseCode(this->wUploadingScoreHandle);
				if (responseCode >= 300) {
					//Remove saved game from the upload queue.
					//(It can be manually uploaded later from the Settings Screen.)
					SCORE_UPLOAD* pScoreInfo = CCurrentGame::scoresForUpload.front();
					delete pScoreInfo;
					CCurrentGame::scoresForUpload.pop();

					SetCursor();
					ShowOkMessage(MID_CaravelServerError);
				}
			} else {
				if (pBuffer->Size())
				{
					//Get demo ranking.
					(*pBuffer) += (UINT)0;	//null terminate
					const char *pwczRank = (char*)(BYTE*)*pBuffer;
					if (strlen(pwczRank) > 0) {
						WSTRING wStr;
						UTF8ToUnicode(pwczRank, wStr);
						this->pRoomWidget->AddLastLayerEffect(new CFlashMessageEffect(
								this->pRoomWidget, wStr.c_str(), -300, 3000));	//show at top of room for 3s
					}
/*
					const int nRanking = atoi(pwczRank);
					if (nRanking && this->pRoomWidget)
					{
						//Score ranked on hi-score list -- give feedback to user.
						const bool bTie = strstr(pwczRank, ".5") != NULL;
						WSTRING wStr = PrintRank(nRanking, bTie);
						wStr += wszExclamation;
						if (nRanking == 1 && !bTie)
							wStr += wszExclamation;
						this->pRoomWidget->AddLastLayerEffect(new CFlashMessageEffect(
								this->pRoomWidget, wStr.c_str(), -300, 3000));	//show at top of room for 3s
					}
*/
/*
					//Mark demo as confirmed uploaded to server.
					if (this->dwUploadingDemo)
					{
						//Remove demo from demo upload queue.
						DEMO_UPLOAD *pDemoInfo = CCurrentGame::demosForUpload.front();
						delete pDemoInfo;
						CCurrentGame::demosForUpload.pop();

						CDbDemo *pDemo = g_pTheDB->Demos.GetByID(this->dwUploadingDemo);
						if (pDemo)
						{
							pDemo->SetFlag(CDbDemo::TestedForUpload);
							pDemo->Update();
							delete pDemo;
						} //else the demo was deleted before the response was received
					}
*/
					if (this->dwUploadingSavedGame)
					{
						//Remove saved game from the upload queue.
						SCORE_UPLOAD *pScoreInfo = CCurrentGame::scoresForUpload.front();
						g_pTheDB->SavedGames.Delete(pScoreInfo->dwSavedGameID); //may delete this record now that it has been uploaded
						delete pScoreInfo;
						CCurrentGame::scoresForUpload.pop();
					}
				}
				delete pBuffer;
			}
			this->dwUploadingDemo = this->dwUploadingSavedGame = this->wUploadingScoreHandle = 0;
		}
	}

	//Upload next queued score record when ready.
	if (!this->wUploadingScoreHandle)
	{
/*
		if (CCurrentGame::demosForUpload.empty())
			return false;	//nothing to upload
		DEMO_UPLOAD *pDemoInfo = CCurrentGame::demosForUpload.front();
		this->wUploadingDemoHandle = g_pTheNet->UploadDemo(pDemoInfo->buffer,
				pDemoInfo->wTurn, pDemoInfo->dwTimeElapsed);
		if (this->wUploadingDemoHandle)
			this->dwUploadingDemo = pDemoInfo->dwDemoID;
*/
		if (!this->dwUploadingDemo)
		{
			if (CCurrentGame::scoresForUpload.empty())
				return false;	//nothing to upload
			SCORE_UPLOAD *pScoreInfo = CCurrentGame::scoresForUpload.front();
			this->wUploadingScoreHandle = g_pTheNet->UploadScore(pScoreInfo->buffer,
					pScoreInfo->scorename, pScoreInfo->wScore);
			if (this->wUploadingScoreHandle)
				this->dwUploadingSavedGame = pScoreInfo->dwSavedGameID;
		}
	}

//	return !CCurrentGame::demosForUpload.empty();
	return !CCurrentGame::scoresForUpload.empty();
}

//*****************************************************************************
void CGameScreen::UploadExploredRooms(const SAVETYPE eSaveType)	//[default=ST_Continue]
//Upload list of explored rooms to site.
{
	if (!g_pTheNet) return;
	if (GetScreenType() != SCR_Game) return;
	if (this->bPlayTesting) return;

	const UINT dwPlayerID = g_pTheDB->GetPlayerID();
	if (!dwPlayerID) return;

	//Skip for holds that aren't on CaravelNet.
	if (this->pCurrentGame)
		if (!g_pTheNet->IsLocalHold(this->pCurrentGame->pHold->dwHoldID))
			return;

	SetCursor(CUR_Internet);
	string text;
	CIDSet ids(dwPlayerID);
	if (CDbXML::ExportXML(V_Players, ids, text, eSaveType))
		g_pTheNet->UploadExploredRooms(text);
	SetCursor();
}

//*****************************************************************************
void CGameScreen::WaitToUploadDemos()
//Uploads demos to server in order received.
//If an upload times out, the user may opt to cancel the upload.
{
	static const Uint32 INTERNET_TIMEOUT = 10000; //10s
	Uint32 dwNow, dwStartUploading = SDL_GetTicks();
	bool bPolling = false;
	const UINT oldHandle = this->wUploadingScoreHandle;
	while (UploadDemoPolling())
	{
		bPolling = true;
		SetCursor(CUR_Internet);
		ShowStatusMessage(MID_UploadingRoomConquerDemos);
		SDL_Delay(20);
		if (oldHandle != this->wUploadingScoreHandle)
			dwStartUploading = SDL_GetTicks(); //next demo started uploading
		if ((dwNow = SDL_GetTicks()) > dwStartUploading + INTERNET_TIMEOUT)
		{
#ifdef DEBUG
			SetCursor();
			const UINT dwTag = ShowYesNoMessage(MID_InternetTimeoutPrompt);
			SetCursor(CUR_Internet);
			if (dwTag == TAG_YES)
				dwStartUploading = dwNow;	//continue trying to upload
			else
#endif
			{
				//Stop trying to upload for now.
				//Attempts to upload will resume when this screen is reentered.
				this->dwUploadingDemo = this->dwUploadingSavedGame = this->wUploadingScoreHandle = 0;
				break;
			}
		}
	}
	if (bPolling)
	{
		SetCursor();
		HideStatusMessage();
	}
}

//*****************************************************************************
void CGameScreen::SendAchievement(const char* achievement, const UINT dwScore)
{
#ifdef STEAMBUILD
	if (GetScreenType() == SCR_Game && !this->bPlayTesting &&
			(this->pCurrentGame->pHold && this->pCurrentGame->pHold->status == CDbHold::Main) &&
			SteamUserStats())
	{
		const WSTRING holdName = static_cast<const WCHAR *>(this->pCurrentGame->pHold->NameText);

		string achievementName = "ACH_";
		achievementName += UnicodeToUTF8(filterFirstLettersAndNumbers(holdName));
		achievementName += "_";
		achievementName += achievement;

		SteamUserStats()->SetAchievement(achievementName.c_str());
		SteamUserStats()->StoreStats();

		if (dwScore)
			g_steamLeaderboards.FindLeaderboardAndUploadScore(achievementName.c_str(), int(dwScore));
	}
#endif
}

void CGameScreen::ShowScorepointRating(int playerScore, int topScore)
{
	if (g_pTheNet && g_pTheNet->IsLoggedIn())
		return; //don't show Steam scores along with CaravelNet scores

	int percent;
	if (playerScore <= 0) {
		percent = 0;
	} else if (playerScore >= topScore) {
		percent = 100;
	} else {
		percent = (playerScore * 100) / topScore;
	}

	char czPercent[16];
	sprintf(czPercent, "%d%%", percent);

	WSTRING wStr;
	AsciiToUnicode(czPercent, wStr);

	if (this->pRoomWidget)
		this->pRoomWidget->AddLastLayerEffect(new CFlashMessageEffect(
				this->pRoomWidget, wStr.c_str(), -300, 3000));	//show at top of room for 3s
}

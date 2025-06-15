// $Id: DrodScreen.cpp 9758 2011-10-28 13:37:06Z mrimer $

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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "DrodScreen.h"
#include "DrodBitmapManager.h"
#include "DrodFontManager.h"
#include "DrodSound.h"
#include "RoomScreen.h"

#include "BloodEffect.h"
#include "DebrisEffect.h"
#include "ExplosionEffect.h"
#include "FiretrapEffect.h"
#include "IceMeltEffect.h"
#include "ImageOverlayEffect.h"
#include "RoomWidget.h"
#include "SparkEffect.h"
#include "SplashEffect.h"
#include "SteamEffect.h"
#include "SwordSwingEffect.h"
#include "TarStabEffect.h"
#include "TrapdoorFallEffect.h"
#include "VerminEffect.h"

#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbPlayers.h"
#include "../DRODLib/DbXML.h"
#include "../DRODLib/GameConstants.h"
#include "../DRODLib/SettingsKeys.h"

#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/FlashMessageEffect.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/ProgressBarWidget.h>
#include <FrontEndLib/TextBoxWidget.h>
#include <FrontEndLib/TheoraPlayer.h>
#include <BackEndLib/Base64.h>
#include <BackEndLib/Browser.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Metadata.h>

#include <sstream>

#ifdef DEV_BUILD //enable to embed media files to master .dat file
#	define EMBED_STYLES
#endif

vector<WSTRING> CDrodScreen::importFiles;
CNetChat::Interface CDrodScreen::chat;

typedef map<ROOMCOORD, vector<UINT> > TilesMap;

const char noFocusPlaysMusic[] = "NoFocusPlaysMusic";
const SDL_Color FriendColor = {0, 0, 128, 0};
const SDL_Color FriendAFKColor = {64, 64, 128, 0};
const SDL_Color AFKColor = {64, 64, 64, 0}; //darker than ignored Gray-128

const WCHAR wszDotOgg[] = { We('.'),We('o'),We('g'),We('g'),We(0) };

//For exporting style mods.
#define DATA_IMAGE (0)
#define DATA_MUSIC (1)

//*****************************************************************************
//Message dialog coords and dimensions (copied from CScreen).
const UINT CX_MESSAGE = 650;
const UINT CY_MESSAGE = 200;
const UINT CX_MESSAGE_BUTTON = 80;
const UINT CX_SPACE = 8;
const UINT CY_SPACE = 8;
const UINT CY_MESSAGE_BUTTON = CY_STANDARD_BUTTON;
const UINT CY_TEXT = CY_MESSAGE - (CY_SPACE * 3) - CY_MESSAGE_BUTTON;
const int Y_MESSAGE_BUTTON = CY_TEXT + (CY_SPACE * 2);
const int X_OK1 = (CX_MESSAGE - CX_MESSAGE_BUTTON) / 2;
const int X_YES = (CX_MESSAGE - CX_SPACE) / 2 - CX_MESSAGE_BUTTON;
const int X_NO = (CX_MESSAGE + CX_SPACE) / 2;
const int X_OK2 = CX_MESSAGE/2 - CX_SPACE - CX_MESSAGE_BUTTON;
const int X_CANCEL = X_OK2 + CX_MESSAGE_BUTTON + CX_SPACE;

//*****************************************************************************
CDrodScreen::CDrodScreen(
//Constructor.
//
//Params:
	const SCREENTYPE eSetType)       //(in)   Type of screen this is.
	: CScreen(eSetType)
	, pProgressWidget(NULL)
	, bQuitPrompt(false)
	, bEnableChat(false), bReceiveWhispersOnly(false)
{
	//Set up hidden message and text input dialog widgets.
	ASSERT(g_pTheDB->IsOpen());

	//Add buttons to message dialog.
	CButtonWidget *pButton = new CButtonWidget(TAG_YES, X_YES, Y_MESSAGE_BUTTON,
			CX_MESSAGE_BUTTON, CY_MESSAGE_BUTTON, g_pTheDB->GetMessageText(MID_Yes));
	this->pMessageDialog->AddWidget(pButton);

	pButton = new CButtonWidget(TAG_NO, X_NO, Y_MESSAGE_BUTTON,
			CX_MESSAGE_BUTTON, CY_MESSAGE_BUTTON, g_pTheDB->GetMessageText(MID_No));
	this->pMessageDialog->AddWidget(pButton);

	pButton = new CButtonWidget(TAG_OK, X_OK1, Y_MESSAGE_BUTTON,
					CX_MESSAGE_BUTTON, CY_MESSAGE_BUTTON, g_pTheDB->GetMessageText(MID_Okay));
	this->pMessageDialog->AddWidget(pButton);

	//Add buttons to input text dialog.
	pButton = new CButtonWidget(TAG_OK, X_OK2, Y_MESSAGE_BUTTON,
			CX_MESSAGE_BUTTON, CY_MESSAGE_BUTTON, g_pTheDB->GetMessageText(MID_OkayNoHotkey));
	this->pInputTextDialog->AddWidget(pButton);

	pButton = new CButtonWidget(TAG_CANCEL_, X_CANCEL, Y_MESSAGE_BUTTON,
			CX_MESSAGE_BUTTON, CY_MESSAGE_BUTTON, g_pTheDB->GetMessageText(MID_CancelNoHotkey));
	this->pInputTextDialog->AddWidget(pButton);

	//File selection dialog box.
	this->pFileBox = new CDrodFileDialogWidget(0L);
	AddWidget(this->pFileBox);
	this->pFileBox->Center();
	this->pFileBox->Hide();

	//Progress bar.
	static const int X_PROGRESS = CX_SPACE;
	const UINT CX_PROGRESS = (this->w - X_PROGRESS*2); //center widget
	static const UINT CY_PROGRESS = 30;
	const int Y_PROGRESS = (this->h - CY_SPACE - CY_PROGRESS);
	this->pProgressWidget = new CProgressBarWidget(0L, X_PROGRESS, Y_PROGRESS, CX_PROGRESS, CY_PROGRESS, 0);
	this->pProgressWidget->Hide(false);
	AddWidget(this->pProgressWidget);
}

//
// Protected methods
//

//*****************************************************************************
void CDrodScreen::AddDamageEffect(CRoomWidget* pRoomWidget, CCurrentGame* pGame,
											 const UINT monsterType, const CMoveCoord& coord)
{
	ASSERT(pRoomWidget);
	switch (monsterType)
	{
		case M_TARBABY:
		case M_TARMOTHER:
			pRoomWidget->AddTLayerEffect(
				new CTarStabEffect(pRoomWidget, coord,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
		break;
		case M_MUDBABY:
		case M_MUDMOTHER:
			pRoomWidget->AddTLayerEffect(
				new CMudStabEffect(pRoomWidget, coord,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
		break;
		case M_GELBABY:
		case M_GELMOTHER:
			pRoomWidget->AddTLayerEffect(
				new CGelStabEffect(pRoomWidget, coord,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
		break;
		case M_SEEP:
			pRoomWidget->AddTLayerEffect(
				new CBloodInWallEffect(pRoomWidget, coord));
		break;
		case M_ROCKGOLEM:
		case M_ROCKGIANT:
		case M_CONSTRUCT:
			pRoomWidget->AddTLayerEffect(
				new CGolemDebrisEffect(pRoomWidget, coord, 10,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
		break;
		case M_SLAYER:
			pRoomWidget->AddTLayerEffect(
				new CVerminEffect(pRoomWidget, coord, 40, true));
		break;
		case M_BEETHRO:
		case M_BEETHRO_IN_DISGUISE:
		case M_PIRATE: case M_STALWART:
			pRoomWidget->AddTLayerEffect(
				new CBloodEffect(pRoomWidget, coord, 30,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
		break;
		case M_FLUFFBABY:
			pRoomWidget->AddTLayerEffect(
				new CFluffStabEffect(pRoomWidget, coord,
					GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
		break;
		default:
			pRoomWidget->AddTLayerEffect(
				new CBloodEffect(pRoomWidget, coord, 16,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
		break;
	}

	pRoomWidget->AddJitter(coord, 0.1f);
}

//*****************************************************************************
void CDrodScreen::AddVisualCues(CCueEvents& CueEvents, CRoomWidget* pRoomWidget, CCurrentGame* pGame)
//Applies cued events to a room widget in the form of graphical effects.
{
	if (!pRoomWidget)
		return;
	ASSERT(pGame);

	ProcessImageEvents(CueEvents, pRoomWidget, pGame);

	const int numFlashingTexts = CueEvents.GetOccurrenceCount(CID_FlashingMessage);
	static const int CY_FLASHING_TEXT = 50;
	int yFlashingTextOffset = (-numFlashingTexts / 2) * CY_FLASHING_TEXT;
	const int Y_FLASHING_TEXT_MAX = int(pRoomWidget->GetH()) / 2 - CY_FLASHING_TEXT;
	const int Y_FLASHING_TEXT_MIN = -Y_FLASHING_TEXT_MAX + CY_FLASHING_TEXT; //leave space at top for score ranking
	if (yFlashingTextOffset < Y_FLASHING_TEXT_MIN)
		yFlashingTextOffset = Y_FLASHING_TEXT_MIN;

	const CAttachableObject *pObj;

/*
	if (CueEvents.HasOccurred(CID_DoublePlaced))
		pRoomWidget->RenderRoomInPlay(); //remove double placement effect
*/

	if (CueEvents.HasOccurred(CID_SwingSword))
	{
		const CAttachableWrapper<UINT> *pOrientation =
				DYN_CAST(const CAttachableWrapper<UINT>*, const CAttachableObject*,
				CueEvents.GetFirstPrivateData(CID_SwingSword));
		ASSERT(pOrientation);
		pRoomWidget->AddMLayerEffect(
				new CSwordSwingEffect(pRoomWidget,
					*pGame->pPlayer, static_cast<UINT>(*pOrientation)));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_Swim);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
			pRoomWidget->AddTLayerEffect(
					new CSplashEffect(pRoomWidget, *pCoord));
	}

	for (pObj = CueEvents.GetFirstPrivateData(CID_OrbActivatedByPlayer);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const COrbData *pOrbData = DYN_CAST(const COrbData*, const CAttachableObject*, pObj);
		pRoomWidget->AddStrikeOrbEffect(*pOrbData);
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_OrbActivatedByDouble);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const COrbData *pOrbData = DYN_CAST(const COrbData*, const CAttachableObject*, pObj);
		pRoomWidget->AddStrikeOrbEffect(*pOrbData);
	}

	for (pObj = CueEvents.GetFirstPrivateData(CID_PressurePlate);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const COrbData *pOrbData = DYN_CAST(const COrbData*, const CAttachableObject*, pObj);
		pRoomWidget->AddStrikeOrbEffect(*pOrbData, false);
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_PressurePlateReleased);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const COrbData *pOrbData = DYN_CAST(const COrbData*, const CAttachableObject*, pObj);
		pRoomWidget->AddStrikeOrbEffect(*pOrbData, false);
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_CrumblyWallDestroyed);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		pRoomWidget->AddTLayerEffect(
				new CDebrisEffect(pRoomWidget, *pCoord, 10,
						GetEffectDuration(pGame, 5), GetParticleSpeed(pGame, 4)));
		pRoomWidget->AddTLayerEffect(
				new CVerminEffect(pRoomWidget, *pCoord));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_MirrorShattered);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		pRoomWidget->AddTLayerEffect(
				new CDebrisEffect(pRoomWidget, *pCoord, 10,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_CrateDestroyed);
		pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord* pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		pRoomWidget->AddTLayerEffect(
			new CDebrisEffect(pRoomWidget, *pCoord, 12,
				GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_BombExploded);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		if (pCoord->wO == NO_ORIENTATION)
			pRoomWidget->AddTLayerEffect(
					new CDebrisEffect(pRoomWidget, *pCoord, 3,
							GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
	}

	for (pObj = CueEvents.GetFirstPrivateData(CID_TrapDoorRemoved);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
		const UINT oTile = pGame->pRoom->GetOSquare(pCoord->wX, pCoord->wY);
		if (bIsPit(oTile))
		{
			static const vector<UINT> trapdoorTile(1, TI_TRAPDOOR_F);
			pRoomWidget->AddOLayerEffect(
					new CTrapdoorFallEffect(pRoomWidget, *pCoord, trapdoorTile,
							pGame->pPlayer->IsHasted() ? 260 : 130));
		} else if (oTile == T_WATER) {
			pRoomWidget->AddTLayerEffect(
					new CSplashEffect(pRoomWidget, *pCoord));
		}
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_ThinIceMelted);
		pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CCoord* pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
		pRoomWidget->AddOLayerEffect(new CIceMeltEffect(pRoomWidget, *pCoord));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_CutBriar);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		pRoomWidget->AddTLayerEffect(
				new CDebrisEffect(pRoomWidget, *pCoord, 10,
						GetEffectDuration(pGame, 5), GetParticleSpeed(pGame, 4)));  //!!briar pieces?
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
					HoldCharacter *pChar = pGame->pHold->GetCharacter(eLogicalType);
					if (pChar)
						eMonsterType = pChar->wType;
					else
						eMonsterType = M_CITIZEN1;
				}

				if (eMonsterType == M_ROCKGIANT && pCoord->wValue) //pieces of rock giant
				{
					wTileNo = GetTileImageForRockGiantPiece(pCoord->wValue, pCoord->wO, 0);
				} else {
					wTileNo = pRoomWidget->GetEntityTile(eMonsterType, eLogicalType, pCoord->wO, 0);
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
					const UINT index = pGame->pRoom->ARRAYINDEX(pCoord->wX,pCoord->wY);
					ASSERT(index < pGame->pRoom->CalcRoomArea());
					const UINT tile = pRoomWidget->pTileImages[index].t;
					if (bIsBriarTI(tile)) //robustness check
						wTileNo = tile;
				}
				if (wTileNo == CALC_NEEDED)
					wTileNo = CalcTileImageFor(pGame->pRoom, pCoord->wValue,
							pCoord->wX, pCoord->wY);
			}
			fallingTiles[ROOMCOORD(pCoord->wX, pCoord->wY)].push_back(wTileNo);
			pObj = CueEvents.GetNextPrivateData();
		}

		for (TilesMap::iterator tiles=fallingTiles.begin(); tiles!=fallingTiles.end(); ++tiles)
		{
			const CCoord coord(tiles->first);
			pRoomWidget->AddOLayerEffect(
					new CTrapdoorFallEffect(pRoomWidget, coord, tiles->second,
					pGame->pPlayer->IsHasted() ? 260 : 130));
		}
	}

	for (pObj = CueEvents.GetFirstPrivateData(CID_MonsterDiedFromStab);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMonster *pMonster = DYN_CAST(const CMonster*, const CAttachableObject*, pObj);
		const UINT wO = IsValidOrientation(pMonster->wProcessSequence) ?
				pMonster->wProcessSequence : NO_ORIENTATION;
		const CMoveCoord coord(pMonster->wX,pMonster->wY,wO);
		AddDamageEffect(pRoomWidget, pGame, pMonster->GetIdentity(), coord);
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_MonsterPieceStabbed);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		UINT wMonsterType = M_SERPENT; //default
		//If a golem is at the target square, that means a rock giant was shattered.
		if (pGame->pRoom->IsMonsterOfTypeAt(M_ROCKGOLEM, pCoord->wX, pCoord->wY))
			wMonsterType = M_ROCKGIANT;
		AddDamageEffect(pRoomWidget, pGame, wMonsterType, *pCoord);
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_SnakeDiedFromTruncation);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMonster *pMonster = DYN_CAST(const CMonster*, const CAttachableObject*, pObj);
		const CMoveCoord coord(pMonster->wX,pMonster->wY,pMonster->wO);
		AddDamageEffect(pRoomWidget, pGame, pMonster->GetIdentity(), coord);
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_TarstuffDestroyed);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoordEx *pCoord = DYN_CAST(const CMoveCoordEx*, const CAttachableObject*, pObj);
		switch (pCoord->wValue)
		{
			case T_TAR:
				pRoomWidget->AddTLayerEffect(
						new CTarStabEffect(pRoomWidget, *pCoord,
								GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
			break;
			case T_MUD:
				pRoomWidget->AddTLayerEffect(
						new CMudStabEffect(pRoomWidget, *pCoord,
								GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
			break;
			case T_GEL:
				pRoomWidget->AddTLayerEffect(
						new CGelStabEffect(pRoomWidget, *pCoord,
								GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
			break;
		}
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_MistDestroyed);
		pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoordEx* pCoord = DYN_CAST(const CMoveCoordEx*, const CAttachableObject*, pObj);
		switch (pCoord->wValue)
		{
		case T_MIST:
			if (bIsSolidOTile(pGame->pRoom->GetOSquare(pCoord->wX, pCoord->wY)))
				pRoomWidget->AddTLayerEffect(
					new CFluffInWallEffect(pRoomWidget, *pCoord));
			else
				pRoomWidget->AddTLayerEffect(
					new CFluffStabEffect(pRoomWidget, *pCoord,
						GetEffectDuration(pGame, 6), GetParticleSpeed(pGame, 4)));
			break;
		}
	}

	if (CueEvents.HasOccurred(CID_Firetrap)) {
		pRoomWidget->RemoveMLayerEffectsOfType(EFIRETRAP);
		const UINT duration = GetEffectDuration(pGame, 450);
		for (pObj = CueEvents.GetFirstPrivateData(CID_Firetrap);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
		{
			const CCoord* pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
			pRoomWidget->AddMLayerEffect(
				new CFiretrapEffect(pRoomWidget, *pCoord, GetEffectDuration(pGame, duration)));
		}
	}

	//Remove old sparks before drawing the current ones.
	pRoomWidget->RemoveTLayerEffectsOfType(ESPARK);
	for (pObj = CueEvents.GetFirstPrivateData(CID_FuseBurning);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		const UINT wTSquare = pGame->pRoom->GetTSquare(pCoord->wX, pCoord->wY);
		if (bIsCombustibleItem(wTSquare))	//needed to avoid effects
					//where fuses have already disappeared since the cue event fired
		{
			pRoomWidget->AddTLayerEffect(
					new CSparkEffect(pRoomWidget, *pCoord, 10));
		}
	}

/*
	for (pObj = CueEvents.GetFirstPrivateData(CID_EvilEyeWoke);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		CMoveCoord *pCoord = DYN_CAST(CMoveCoord*, CAttachableObject*, pObj);
		pRoomWidget->AddMLayerEffect(new CEvilEyeGazeEffect(
				pRoomWidget,pCoord->wX,pCoord->wY,pCoord->wO, 500));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_FegundoToAsh);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		CMonster *pMonster = DYN_CAST(CMonster*, CAttachableObject*, pObj);
		CMoveCoord coord(pMonster->wX,pMonster->wY,NO_ORIENTATION);
		pRoomWidget->AddMLayerEffect(
				new CSteamEffect(pRoomWidget, coord));
	}
*/
	if (CueEvents.HasOccurred(CID_PlayerBurned))
	{
		pRoomWidget->AddMLayerEffect(
				new CSteamEffect(pRoomWidget, *pGame->pPlayer));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_MonsterBurned);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pMoveCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		pRoomWidget->AddMLayerEffect(
				new CSteamEffect(pRoomWidget, *pMoveCoord));
	}

	for (pObj = CueEvents.GetFirstPrivateData(CID_Splash);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
			if (pGame->pRoom->GetOSquare(pCoord->wX, pCoord->wY) == T_WATER)
				pRoomWidget->AddTLayerEffect(
						new CSplashEffect(pRoomWidget, *pCoord));
	}

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
			pRoomWidget->AddTLayerEffect(
					new CExplosionEffect(pRoomWidget, CCoord(coord->wX, coord->wY),
							GetEffectDuration(pGame, 500)));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_ZombieGaze);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMonster *pMonster = DYN_CAST(const CMonster*, const CAttachableObject*, pObj);
		pRoomWidget->AddZombieGazeEffect(pMonster);
	}

	for (pObj = CueEvents.GetFirstPrivateData(CID_FlashingMessage);
		pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		if (yFlashingTextOffset > Y_FLASHING_TEXT_MAX)
			break; //no room to display

		const CColorText* pColorText = DYN_CAST(const CColorText*, const CAttachableObject*, pObj);
		const CDbMessageText* pText = pColorText->pText;
		ASSERT((const WCHAR*)(*pText));
		CFlashMessageEffect* pFlashText = new CFlashMessageEffect(
			pRoomWidget, (const WCHAR*)(*pText), yFlashingTextOffset, 2000, 500);
		pFlashText->SlowExpansion();
		if (pColorText->customColor)
			pFlashText->SetColor(pColorText->r, pColorText->g, pColorText->b);
		pRoomWidget->AddLastLayerEffect(pFlashText);
		yFlashingTextOffset += CY_FLASHING_TEXT;
	}

	const bool bLightToggled = CueEvents.HasOccurred(CID_LightToggled);
	if (bLightToggled)
		pRoomWidget->RenderRoomLighting();
	pRoomWidget->AddPlayerLight();
	pRoomWidget->RerenderRoomCeilingLight(CueEvents);
	if (CueEvents.HasOccurred(CID_Plots) || bLightToggled)
	{
		//Do an update of tile image arrays.
		const CCoordSet *pSet = DYN_CAST(const CCoordSet*, const CAttachableObject*,
				CueEvents.GetFirstPrivateData(CID_Plots) );
		pRoomWidget->UpdateFromPlots(pSet, &pGame->pRoom->geometryChanges);
	}
}

//*****************************************************************************
void CDrodScreen::AdvanceDemoPlayback(
//Advance a game forward from the beginning,
//one turn each frame, until replayed to the current move.
	CCurrentGame* pGame, CRoomWidget* pRoomWidget, //game and widget where game room is displayed
	const UINT containerWidgetTag) //optional tag of room container where screen is displaying the room widget
{
	if (!pGame)
		return;

	CWidget *pRoomDisplay = containerWidgetTag ? GetWidget(containerWidgetTag) : NULL;
	if (pRoomDisplay && !pRoomDisplay->IsVisible())
		return;

	//Replay is not over yet?
	CDbCommands::const_iterator commandIter = pGame->Commands.GetCurrent();
	if (commandIter != pGame->Commands.end())
	{
		UINT wX=(UINT)-1, wY=(UINT)-1;
		const int nCommand = static_cast<int>(commandIter->command);
		if (bIsComplexCommand(nCommand))	//handle two-part commands here
		{
			vector<COMMANDNODE>::const_iterator data = commandIter + 1;
			ASSERT(data != pGame->Commands.end());
			wX = data->command;
			wY = data->msElapsedSinceLast;
		}

		CCueEvents CueEvents;
		pGame->Commands.Freeze();
		pGame->ProcessCommand(nCommand, CueEvents, wX, wY);
		pGame->Commands.Unfreeze();
	
		//Get next turn ready.
		commandIter = pGame->Commands.GetNext();

		//If player died (for good) or exited level, stop replay.
		if ((CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied) ||
				CueEvents.HasOccurred(CID_WinGame) ||
				CueEvents.HasOccurred(CID_ExitLevelPending) ||
				CueEvents.HasOccurred(CID_ExitToWorldMapPending) ||
				CueEvents.HasOccurred(CID_InvalidAttackMove)) || //invalid move indicates play sequence is for a different hold version
				CueEvents.HasOccurred(CID_StalledCombat)) //stalled combat indicates play sequence is for a different hold version
		{
			while (commandIter != pGame->Commands.end())
				commandIter = pGame->Commands.GetNext();
		}

		AddVisualCues(CueEvents, pRoomWidget, pGame);
	}

	if (pRoomDisplay)
		pRoomDisplay->RequestPaint();
	else
		pRoomWidget->RequestPaint();
}

//*****************************************************************************
void CDrodScreen::ProcessImageEvents(
	//Handle events for displaying image overlay effects.
	//This must be called both before and after the room is drawn to catch all cases.
	//
	//Pre-condition: Persistent image effects were handled previously in DisplayPersistingImageOverlays.
	CCueEvents& CueEvents,
	CRoomWidget* pRoomWidget, const CCurrentGame* pGame)
{
	static const CUEEVENT_ID cid = CID_ImageOverlay;
	if (!CueEvents.HasOccurred(cid))
		return;

	const UINT currentTurn = pGame ? pGame->wTurnNo : 0;
	const Uint32 dwNow = CScreen::dwCurrentTicks;

	const CAttachableObject* pObj = CueEvents.GetFirstPrivateData(cid);
	while (pObj)
	{
		const CImageOverlay* pImageOverlay = DYN_CAST(const CImageOverlay*, const CAttachableObject*, pObj);

		//Don't wait for additional images to be added to the room widget before clearing effects.
		const int clearsLayer = pImageOverlay->clearsImageOverlays();
		const int clearsGroup = pImageOverlay->clearsImageOverlayGroup();
		if (clearsLayer == ImageOverlayCommand::NO_LAYERS &&
			clearsGroup == ImageOverlayCommand::NO_GROUP) {
			CImageOverlayEffect* pEffect = new CImageOverlayEffect(pRoomWidget, pImageOverlay, currentTurn, dwNow);
			pRoomWidget->AddLayerEffect(pEffect, pImageOverlay->getLayer());
		} else {
			if (clearsLayer != ImageOverlayCommand::NO_LAYERS) {
				pRoomWidget->RemoveLayerEffects(EIMAGEOVERLAY, clearsLayer);
			}
			if (clearsGroup != ImageOverlayCommand::NO_GROUP) {
				pRoomWidget->RemoveGroupEffects(clearsGroup);
			}
		}

		//Don't reprocess these events if this method is called again.
		//This is done instead of calling ClearEvent so the occurred flag isn't reset.
		CueEvents.Remove(cid, pObj);

		//The next item is now the first item.
		pObj = CueEvents.GetFirstPrivateData(cid);
	}
}

//*****************************************************************************
UINT CDrodScreen::GetEffectDuration(CCurrentGame* pGame, const UINT baseDuration) const
//Returns: duration of particles in particle effects
{
	//When player is hasted, particles move at half speed, so they last twice as long
	return pGame && pGame->pPlayer->IsHasted() ?
			baseDuration*2 : baseDuration;
}

//*****************************************************************************
UINT CDrodScreen::GetParticleSpeed(CCurrentGame* pGame, const UINT baseSpeed) const
//Returns: speed of particles in particle effects
{
	//When player is hasted, particles move at half speed
	return pGame && pGame->pPlayer->IsHasted() ?
			(baseSpeed > 1 ? baseSpeed/2 : 1) : baseSpeed;
}

//******************************************************************************
CDbHold::HoldStatus CDrodScreen::GetHoldStatus()
{
	return CDbHolds::GetStatus(g_pTheDB->GetHoldID());
}

//*****************************************************************************
WSTRING CDrodScreen::getStatsText(
//Print player stats as text.
//
//Params:
	const PlayerStats& st,
	CCurrentGame* pGame) //[default=NULL]
{
	WSTRING wstr;
	WCHAR num[16];

	wstr += g_pTheDB->GetMessageText(MID_MonsterHP);
	wstr += wszColon;
	wstr += wszSpace;
	wstr += _itoW(st.HP, num, 10);
	wstr += wszSpace;

	wstr += g_pTheDB->GetMessageText(MID_ATKStat);
	wstr += wszColon;
	wstr += wszSpace;
	wstr += _itoW(st.ATK, num, 10);
	wstr += wszSpace;

	wstr += g_pTheDB->GetMessageText(MID_DEFStat);
	wstr += wszColon;
	wstr += wszSpace;
	wstr += _itoW(st.DEF, num, 10);
	wstr += wszSpace;

	wstr += g_pTheDB->GetMessageText(MID_GRStat);
	wstr += wszColon;
	wstr += wszSpace;
	wstr += _itoW(st.GOLD, num, 10);
	wstr += wszSpace;

	wstr += g_pTheDB->GetMessageText(MID_XPStat);
	wstr += wszColon;
	wstr += wszSpace;
	wstr += _itoW(st.XP, num, 10);
	wstr += wszSpace;
	wstr += wszSpace;

	wstr += g_pTheDB->GetMessageText(MID_Keys);
	wstr += wszColon;
	wstr += wszSpace;
	wstr += _itoW(st.yellowKeys, num, 10);
	wstr += wszForwardSlash;
	wstr += _itoW(st.greenKeys, num, 10);
	wstr += wszForwardSlash;
	wstr += _itoW(st.blueKeys, num, 10);
	if (st.skeletonKeys)
	{
		wstr += wszForwardSlash;
		wstr += _itoW(st.skeletonKeys, num, 10);
	}
	wstr += wszSpace;
	wstr += wszSpace;

	wstr += g_pTheDB->GetMessageText(MID_ShovelsStat);
	wstr += wszColon;
	wstr += wszSpace;
	wstr += _itoW(st.shovels, num, 10);

	const bool bHasSword = st.sword != NoSword && st.sword != WeaponSlot;
	const bool bHasShield = st.shield != NoShield && st.shield != ArmorSlot;
	const bool bHasAccessory = st.accessory != NoAccessory && st.accessory != AccessorySlot;
	const bool bHasEquipment = bHasSword || bHasShield || bHasAccessory;
	if (bHasEquipment)
	{
		wstr += wszCRLF;

		if (bHasSword)
		{
			bool bCustomDesc = false;
			if (bIsCustomEquipment(st.sword))
			{
				if (pGame && pGame->pHold)
				{
					HoldCharacter *pChar = pGame->pHold->GetCharacter(st.sword);
					if (pChar)
					{
						wstr += pChar->charNameText.c_str();
						bCustomDesc = true;
					}
				}
			}
			if (!bCustomDesc)
				wstr += g_pTheDB->GetMessageText(CRoomWidget::GetSwordMID(st.sword));
			if (bHasShield || bHasAccessory)
			{
				wstr += wszSpace;
				wstr += wszSpace;
			}
		}

		if (bHasShield)
		{
			bool bCustomDesc = false;
			if (bIsCustomEquipment(st.shield))
			{
				if (pGame && pGame->pHold)
				{
					HoldCharacter *pChar = pGame->pHold->GetCharacter(st.shield);
					if (pChar)
					{
						wstr += pChar->charNameText.c_str();
						bCustomDesc = true;
					}
				}
			}
			if (!bCustomDesc)
				wstr += g_pTheDB->GetMessageText(CRoomWidget::GetShieldMID(st.shield));
			if (bHasAccessory)
			{
				wstr += wszSpace;
				wstr += wszSpace;
			}
		}

		if (bHasAccessory)
		{
			bool bCustomDesc = false;
			if (bIsCustomEquipment(st.accessory))
			{
				if (pGame && pGame->pHold)
				{
					HoldCharacter *pChar = pGame->pHold->GetCharacter(st.accessory);
					if (pChar)
					{
						wstr += pChar->charNameText.c_str();
						bCustomDesc = true;
					}
				}
			}
			if (!bCustomDesc)
				wstr += g_pTheDB->GetMessageText(CRoomWidget::GetAccessoryMID(st.accessory));
		}
	}

	//Will be written on a second line, after any equipment.
	if (st.totalMoves || st.totalTime)
	{
		if (!bHasEquipment)
			wstr += wszCRLF;

		wstr += wszSpace;
		wstr += wszSpace;
		wstr += wszSpace;
		wstr += _itoW(st.totalMoves, num, 10);
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_Moves);

		wstr += wszSpace;
		wstr += wszSpace;
		wstr += CDate::FormatTime(st.totalTime / 1000); //convert ms -> s
	}

	return wstr;
}

//*****************************************************************************
void appendDoorStatsLine(WSTRING& wstr, UINT mid, UINT closed, UINT open)
{
	if (closed || open)
	{
		WCHAR num[16];

		wstr += wszCRLF;
		wstr += g_pTheDB->GetMessageText(mid);
		wstr += wszColon;
		wstr += wszSpace;
		wstr += _itoW(closed, num, 10);
		if (open) {
			wstr += wszSpace;
			wstr += wszLeftParen;
			wstr += _itoW(open, num, 10);
			wstr += wszRightParen;
		}
	}
}

//*****************************************************************************
WSTRING CDrodScreen::getStatsText(
//Print stats as text.
//
//Params:
	const RoomStats& st,
	CCurrentGame* pGame) //[default=NULL]
{
	WSTRING wstr;
	WCHAR num[16];

	wstr += getStatsText(PlayerStats(st), pGame);

	wstr += wszCRLF;
	if (st.levels > 1)
	{
		wstr += g_pTheDB->GetMessageText(MID_Levels);
		wstr += wszColon;
		wstr += wszSpace;
		wstr += _itoW(st.levels, num, 10);
		wstr += wszSpace;
		wstr += wszSpace;
	}
	wstr += g_pTheDB->GetMessageText(MID_Rooms);
	wstr += wszColon;
	wstr += wszSpace;
	wstr += _itoW(st.rooms, num, 10);
	if (st.secrets)
	{
		wstr += wszSpace;
		wstr += wszSpace;
		wstr += g_pTheDB->GetMessageText(MID_Secrets);
		wstr += wszColon;
		wstr += wszSpace;
		wstr += _itoW(st.secrets, num, 10);
	}

	appendDoorStatsLine(wstr, MID_YellowDoor, st.yellowDoors, st.openYellowDoors);
	appendDoorStatsLine(wstr, MID_GreenDoor, st.greenDoors, st.openGreenDoors);
	appendDoorStatsLine(wstr, MID_BlueDoor, st.blueDoors, st.openBlueDoors);
	appendDoorStatsLine(wstr, MID_MoneyDoor, st.moneyDoorCost, st.openMoneyDoorCost);
	appendDoorStatsLine(wstr, MID_RedDoor, st.redDoors, st.openRedDoors);
	appendDoorStatsLine(wstr, MID_BlackDoor, st.blackDoors, st.openBlackDoors);
	appendDoorStatsLine(wstr, MID_Dirt1, st.dirtBlockCost, 0);

	return wstr;
}

//*****************************************************************************
void CDrodScreen::ApplyINISettings()
//(Re)query the INI for current values and apply them.
{
	g_pTheDSM->InitCrossfadeDuration();

	g_pTheDBM->GetLanguage();

	//Disable error logging if requested.
	string str;
	if (CFiles::GetGameProfileString(INISection::Startup, INIKey::LogErrors, str))
		SetLogErrors(atoi(str.c_str()) != 0);
}

//*****************************************************************************
void CDrodScreen::Callback(long val)
//Status messages during import.
{
	if (!val)
		return;
	Paint();
	WSTRING text = this->callbackContext;
	if (!this->callbackContext.empty())
	{
		text += wszColon;
		text += wszSpace;
	}
	text += g_pTheDB->GetMessageText(MESSAGE_ID(val));
	CScreen::ShowStatusMessage(text.c_str());
}

//*****************************************************************************
void CDrodScreen::Callbackf(float fVal)
//Progress indicator during import.
{
	this->pProgressWidget->Show();

	//Be nice to other apps.
	SDL_Delay(0);

	//Don't update value if change is to small to be seen.
	float fDelta = fVal - this->pProgressWidget->GetValue();
	if (fDelta < 0.0) fDelta = -fDelta;	//fabs
	if (fDelta * (float)this->pProgressWidget->GetW() < 1.0) return;

	//Monitor events received.  Handle window changes properly.
	SDL_Event event;
	while (PollEvent(&event)) 
	{
		switch (event.type)
		{
			case SDL_WINDOWEVENT:
				OnWindowEvent(event.window);
				this->pStatusDialog->Paint();	//make sure this stays on top
			break;
		}
	}

	this->pProgressWidget->SetValue(fVal);
	this->pProgressWidget->Paint();
	g_pTheDBM->UpdateRects(GetWidgetScreenSurface());
}

//*****************************************************************************
void CDrodScreen::CallbackText(const WCHAR* wpText)
//Status messages during import.
{
	if (!wpText)
		return;
	Paint();
	CScreen::ShowStatusMessage(wpText);
}

//*****************************************************************************
void CDrodScreen::ChatPolling(const UINT tagUserList)
//Poll for server chat responses.
{
	switch (this->chat.PollCNet())
	{
		case CNetChat::Res_Received:
			ProcessReceivedChatData(tagUserList);
		break;
		case CNetChat::Res_InvalidData:
		break;
		default:
		case CNetChat::Res_None: break;
	}
}

//*****************************************************************************
void CDrodScreen::EditGlobalVars(
//Pop up a dialog box that contains a list of global game vars.
//Double-clicking vars will edit their value.
//
//Params:
	CEntranceSelectDialogWidget *pListBox,
	PlayerStats* st,     //if NULL, use the player stats in the current game, if set
	CCurrentGame* pGame) //[default=NULL]
{
	//Var resolution.
	ASSERT(st || pGame); //there must be something relevant to alter
	if (!st)
	{
		if (!pGame)
			return; //robustness check
		st = &(pGame->pPlayer->st); //default to altering game stats
	}
	ASSERT(st);

	//Set up widgets.
	pListBox->SetPrompt(MID_GameVarsTitle);
	pListBox->PrepareToPopulateList(CEntranceSelectDialogWidget::GlobalVars);
	pListBox->PopulateListBoxFromGlobalVars(*st);
	if (pGame) {
		pListBox->PopulateListBoxFromHoldVars(pGame);
	}
	pListBox->SelectItem(0);

	SetCursor();

	int itemID=0;
	do {
		const CEntranceSelectDialogWidget::BUTTONTYPE eButton =
				(CEntranceSelectDialogWidget::BUTTONTYPE)pListBox->Display();
		itemID = eButton == CEntranceSelectDialogWidget::OK ? pListBox->GetSelectedItem() : 0;
		if (itemID < 0)
		{
			//Change this variable's value.
			WCHAR temp[16];
			WSTRING wstrValue = _itoW((int)st->getVar(ScriptVars::Predefined(itemID)), temp, 10);
			const UINT tagNo = ShowTextInputMessage(MID_ChangeVarValuePrompt,
					wstrValue);
			if (tagNo == TAG_OK)
			{
				UINT newVal = _Wtoi(wstrValue.c_str());

				//Protect against certain dangerous states.
				switch (itemID)
				{
					case (int)ScriptVars::P_HP:
						if (int(newVal) < 1)
							newVal = 1; //constrain HP to preserve life
					break;
				}

				if (pGame)
				{
					CCueEvents Ignored;
					pGame->ProcessCommand(CMD_SETVAR, Ignored, itemID, newVal);
				} else {
					st->setVar(ScriptVars::Predefined(itemID), newVal);
				}

				//Update value in widget list.
				pListBox->PrepareToPopulateList(CEntranceSelectDialogWidget::GlobalVars);
				pListBox->PopulateListBoxFromGlobalVars(*st);
				if (pGame) {
					pListBox->PopulateListBoxFromHoldVars(pGame);
				}
				pListBox->SelectItem(itemID);
			}
		} else if (itemID > 0 && pGame) {
			CDbPackedVars& stats = pGame->stats;
			char varID[10], varName[11] = "v";
			//Get local hold var.
			_itoa(itemID, varID, 10);
			strcat(varName, varID);

			UNPACKEDVARTYPE vType;
			WSTRING wstrValue;
			vType = stats.GetVarType(varName);
			bool bIsArray = pGame->pHold->IsArrayVar(itemID);

			if (bIsArray) {
				wstrValue = pGame->GetArrayVarAsString(itemID);
			} else if (vType == UVT_int) {
				int iVarValue = stats.GetVar(varName, (int)0);
				std::basic_stringstream<WCHAR_t> stream;
				stream << iVarValue;
				wstrValue = stream.str();
			}
			else {
				wstrValue = stats.GetVar(varName, WS("0"));
			}

			const UINT tagNo = ShowTextInputMessage(MID_ChangeVarValuePrompt,
				wstrValue);
			if (tagNo == TAG_OK)
			{
				// Hold variables can be arrays, strings or integers
				if (bIsArray) {
					pGame->SetArrayVarFromString(itemID, wstrValue);
				} else if (isWInteger(wstrValue.c_str())) {
					int varValue = _Wtoi(wstrValue.c_str());
					stats.SetVar(varName, varValue);
				}
				else {
					stats.SetVar(varName, wstrValue.c_str());
				}

				//Update value in widget list.
				pListBox->PrepareToPopulateList(CEntranceSelectDialogWidget::GlobalVars);
				pListBox->PopulateListBoxFromGlobalVars(*st);
				if (pGame) {
					pListBox->PopulateListBoxFromHoldVars(pGame);
				}
				pListBox->SelectItem(itemID);
			}
		}
	} while (itemID != 0);

	Paint();
}

//*****************************************************************************
UINT CDrodScreen::ImportHoldImage(const UINT holdID, const UINT extensionFlags)
//Load an image file from disk into the hold,
//using any of the specified supported file extensions.
//Returns: dataID if operation completed successfully or 0 if it was canceled.
{
	static const char importImagePath[] = "ImportImagePath";

	//Get image import path.
	CFiles Files;
	CDbPlayer* pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	WSTRING wstrImportPath = pCurrentPlayer ?
		pCurrentPlayer->Settings.GetVar(importImagePath, Files.GetDatPath().c_str()) :
		Files.GetDatPath();

	WSTRING wstrImportFile;
	for (;;)
	{
		const UINT dwTagNo = SelectFile(wstrImportPath,
			wstrImportFile, MID_ImageSelectPrompt, false, extensionFlags);
		if (dwTagNo != TAG_OK)
		{
			delete pCurrentPlayer;
			return 0;
		}

		//Update the path in player settings, so next time dialog
		//comes up it will have the same path.
		if (pCurrentPlayer)
		{
			pCurrentPlayer->Settings.SetVar(importImagePath, wstrImportPath.c_str());
			pCurrentPlayer->Update();
		}

		//Forbid importing multiple images with the same name into a hold.
		const WSTRING filename = getFilenameFromPath(wstrImportFile.c_str());
		CDb db;
		db.Data.FilterByHold(holdID);
		if (db.Data.FindByName(filename.c_str()) != 0) {
			ShowOkMessage(MID_HoldImportDuplicateNameError);
			continue;
		}

		//Load image.
		CStretchyBuffer buffer;
		if (!Files.ReadFileIntoBuffer(wstrImportFile.c_str(), buffer, true)) {
			ShowOkMessage(MID_FileNotFound);
		}
		else {
			const UINT wDataFormat = g_pTheBM->GetImageType(buffer);
			if (wDataFormat == DATA_UNKNOWN) {
				ShowOkMessage(MID_FileCorrupted);
			}
			else {
				CDbDatum* pImage = g_pTheDB->Data.GetNew();
				pImage->wDataFormat = wDataFormat;
				pImage->data.Set((const BYTE*)buffer, buffer.Size());
				pImage->DataNameText = filename;
				pImage->dwHoldID = holdID; //image belongs to this hold
				pImage->Update();
				const UINT dwDataID = pImage->dwDataID;
				delete pImage;
				delete pCurrentPlayer;
				return dwDataID;
			}
		}
	}

	ASSERT(!"Bad logic path.");
	delete pCurrentPlayer;
	return 0;
}

//*****************************************************************************
bool CDrodScreen::ParseConsoleCommand(const WCHAR *pText)
//Scan text for console commands.
//
//Returns: whether text starts with a tilde, indicating a console command
{
	ASSERT(pText);
	if (pText[0] != W_t('~')) //all console commands begin with tilde
		return false; //not a command

	//Identify and process command.
	UINT index=1; //skip tilde

//Predefined macros and console command tokens.
#define MACROMATCH(pwMacroStr) (!WCSncmp(pText+index, pwMacroStr, WCSlen(pwMacroStr)))

	//Reserved console commands.
	static const WCHAR consoleHelpStr[] = {We('h'),We('e'),We('l'),We('p'),We(0)};

	//Reply in whisper to the last user who whispered me.
	if (MACROMATCH(consoleHelpStr))
	{
		const WSTRING wstr = g_pTheDB->GetMessageText(MID_ConsoleHelp);
		DisplayChatText(wstr, Black);
		return true;
	}
#undef MACROMATCH

	//Parse "section:key[=value]".
	string str = UnicodeToUTF8(pText+index);
	char *pStr = (char*)str.c_str();
	string originalStr = pStr;
	char *pSection = strtok(pStr, ":");
	if (pSection && strlen(pSection))
	{
		char *pKey = strtok(NULL, "=");
		if (pKey && strlen(pKey))
		{
			//Look up INI setting.
			string val;
			if (CFiles::GetGameProfileString(pSection, pKey, val))
			{
				//Setting exists.  Is "=newValue" specified?
				char *pValue = strtok(NULL, "");
				if (pValue)
				{
					//Update setting in INI.
					CFiles f;
					f.WriteGameProfileString(pSection, pKey, pValue);
					ApplyINISettings();
				} else {
					//Show current setting.
					originalStr += "=";
					originalStr += val;
				}
				//Display current/new setting for key.
				WSTRING wstr;
				UTF8ToUnicode(originalStr.c_str(), wstr);
				DisplayChatText(wstr, Black);
				return true;
			} else {
				//No setting exists.  Add it to the INI if an assignment is made.
				char *pValue = strtok(NULL, "");
				if (pValue)
				{
					//Update setting in INI.
					CFiles f;
					f.WriteGameProfileString(pSection, pKey, pValue);
					ApplyINISettings();

					//Display inputted setting for key.
					WSTRING wstr;
					UTF8ToUnicode(originalStr.c_str(), wstr);
					DisplayChatText(wstr, Black);
					return true;
				}
			}
		}
	}

	//Not recognized.
	WSTRING orig, wstr = g_pTheDB->GetMessageText(MID_UnrecognizedConsoleCommand);
	UTF8ToUnicode(originalStr.c_str(), orig);
	wstr += wszColon;
	wstr += wszSpace;
	wstr += orig;
	DisplayChatText(wstr, Black);
	return true;
}

//*****************************************************************************
void CDrodScreen::PopulateChatUserList(const UINT tagUserList)
//Maintain list of users chatting.
//Decorate texts with contextual information.
{
	CListBoxWidget *pList = DYN_CAST(CListBoxWidget*, CWidget*, GetWidget(tagUserList));
	const CIDSet selectedUserIDs = pList->GetSelectedItems(); //retain selection after refresh
	pList->Clear();
	CNetChat::CHAT_VECTOR::const_iterator iter;
	for (iter = this->chat.users.begin(); iter != this->chat.users.end(); ++iter)
	{
		CNetChat::Data* c = *iter;
		pList->AddItem(c->chatID, c->sender.c_str(), this->chat.IsIgnored(c->sender));
		if (this->chat.IsFriend(c->sender))
			pList->SetItemColor(c->chatID, c->bAFK ? FriendAFKColor : FriendColor);
		else if (c->bAFK)
			pList->SetItemColor(c->chatID, AFKColor);
		delete c;
	}
	if (pList->IsEmpty())
		pList->AddItem(TAG_EMPTYCHATUSERLIST,
				g_pTheDB->GetMessageText(MID_ChatEmptyUserList), true);
	pList->SelectItems(selectedUserIDs);
	this->chat.users.clear();
}

//*****************************************************************************
void CDrodScreen::ProcessReceivedChatData(const UINT tagUserList)
//Chat data have been received from the server -- display relevant data.
{
	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	WSTRING playerCNetName = pPlayer ? (const WCHAR*)(pPlayer->CNetNameText) : wszEmpty;
	delete pPlayer;

	//Add new chats to display in order they were received.
	if (!this->chat.received.empty())
	{
		for (CNetChat::ORDERED_CHATS::const_iterator iter = this->chat.received.begin();
				iter != this->chat.received.end(); ++iter)
		{
			CNetChat::Data* c = *iter;
			if (!this->chat.IsIgnored(c->sender) || c->bAdminMessage)
			{
				if (this->bReceiveWhispersOnly && !this->chat.IsWhisperedTo(*c, playerCNetName))
				{
					delete c;
					continue;
				}
				this->chat.AddToHistory(c);
				const WSTRING wstr = this->chat.FormatMessageForDisplay(c);
				DisplayChatText(wstr, c->color);
			} else {
				delete c;
			}
		}
		this->chat.received.clear();
	}

	PopulateChatUserList(tagUserList);

	//Not needed here -- discard.
	this->chat.deleted.clear();
}

//*****************************************************************************
void CDrodScreen::ReformatChatText(
//As text is entered to chat input, update context-sensitive macro texts.
//
//Params:
	const UINT chatInputTextTag, //tag of text box widget for chat text
	const bool bNextOption) //whether to get the next parameter option for a recognized macro [default=false]
{
	CTextBoxWidget *pText = DYN_CAST(CTextBoxWidget*, CWidget*,
			GetWidget(chatInputTextTag));
	WSTRING wstr = pText->GetText();
	if (this->chat.ReformatText(wstr, bNextOption))
	{
		pText->SetText(wstr.c_str());
		pText->RequestPaint();
	}
}

//*****************************************************************************
void CDrodScreen::ShowChatHistory(CEntranceSelectDialogWidget *pBox)
//Pops up a dialog box that shows the list of chat history for this app session.
{
	ASSERT(pBox);

	SetCursor();

	pBox->SetPrompt(MID_ChatHistory);
	pBox->PopulateList(CEntranceSelectDialogWidget::ChatHistory);
	pBox->Display();

	Paint();
}

//*****************************************************************************
void CDrodScreen::logoutFromChat()
//When leaving a chat session, perform chat clean up.
{
	//Wait for server response for outstanding request.
	if (CDrodScreen::chat.IsBusy())
	{
		SetCursor(CUR_Internet);
		CDrodScreen::chat.resolveOutstandingRequests();
		SetCursor();
	}

	//Notify the server of exiting the chat.  No response is needed or expected.
	g_pTheNet->ChatLogout();
}

//*****************************************************************************
void CDrodScreen::ExportHold(const UINT dwHoldID)
{
	if (!dwHoldID) return;

	CDbHold *pHold = g_pTheDB->Holds.GetByID(dwHoldID);
	if (!pHold) return;

#ifndef STEAMBUILD
	//Load graphics/music into hold for inclusion in export.
	if (CDbHold::IsOfficialHold(pHold->status))
		ImportMedia();
#endif

	//Default filename is hold name.
	WSTRING wstrExportFile = (WSTRING)pHold->NameText;
	if (ExportSelectFile(MID_SaveHoldPath, wstrExportFile, EXT_HOLD))
	{
		//Write the hold file.
		SetCursor(CUR_Wait);
		this->callbackContext = WSTRING(pHold->NameText);
		ShowStatusMessage(MID_Exporting);
		CDbXML::SetCallback(this);
		const bool bResult = CDbXML::ExportXML(V_Holds, dwHoldID, wstrExportFile.c_str());
		this->pProgressWidget->Hide();
		HideStatusMessage();
		SetCursor();
		ShowOkMessage(bResult ? MID_HoldFileSaved :
				MID_HoldFileNotSaved);
		this->callbackContext.resize(0);
	}

	//Export hold texts for localization if requested.
	string str;
	bool bExportText = false;
	if (CFiles::GetGameProfileString(INISection::Localization, INIKey::ExportText, str))
		bExportText = atoi(str.c_str()) != 0;
	if (bExportText && ShowYesNoMessage(MID_ExportHoldTextPrompt) == TAG_YES)
		ExportHoldTexts(pHold);

	//Export hold script if requested.
	bool bExportScripts = false;
	if (CFiles::GetGameProfileString(INISection::Customizing, INIKey::ExportSpeech, str))
		bExportScripts = atoi(str.c_str()) != 0;
	if (bExportScripts && ShowYesNoMessage(MID_ExportHoldScriptsPrompt) == TAG_YES)
		ExportHoldScripts(pHold);

	delete pHold;
}

//*****************************************************************************
void CDrodScreen::ExportHoldScripts(CDbHold *pHold)
//Exports all scripted speech in a hold to an HTML file.
{
	if (!pHold) return;
	
	//Default filename is hold name.
	WSTRING wstrExportFile = (WSTRING)pHold->NameText;
	if (!ExportSelectFile(MID_SaveHoldPath, wstrExportFile, EXT_HTML))
		return;

	//Write the hold script data.
	SetCursor(CUR_Wait);
	ShowStatusMessage(MID_Exporting);
	WSTRING holdText = g_pTheDB->Holds.ExportSpeech(pHold->dwHoldID);
	if (holdText.empty()) {HideStatusMessage(); SetCursor(); return;}
	CStretchyBuffer buffer((const BYTE*)(holdText.c_str()), holdText.size() * sizeof(WCHAR)); //no terminating null
	const bool bRes = CFiles::WriteBufferToFile(wstrExportFile.c_str(),buffer);

	HideStatusMessage();
	SetCursor();
	ShowOkMessage(bRes ? MID_HoldScriptsSaved : MID_HoldFileNotSaved);
}

//*****************************************************************************
void CDrodScreen::ExportHoldTexts(CDbHold *pHold)
//Export hold texts.
{
	if (!pHold) return;
	
	//Default filename is hold name.
	WSTRING wstrExportFile = (WSTRING)pHold->NameText;
	if (!ExportSelectFile(MID_SaveHoldPath, wstrExportFile, EXT_XML))
		return;

	SetCursor(CUR_Wait);
	ShowStatusMessage(MID_Exporting);

	//Header and root tag.
	char temp[16];
	CStretchyBuffer str;
	str += "<?xml version=\"1.0\" encoding=\"utf-8\" ?>" NEWLINE;
	str += "<";
	str += szDROD;
	str += " ";
	str += szDRODVersion;
	str += "='";
	str += _itoa(VERSION_NUMBER, temp, 10);
	str += "'>" NEWLINE;

	CDbRefs refs(V_Holds, CIDSet(pHold->dwHoldID));
	g_pTheDB->Holds.ExportText(pHold->dwHoldID, refs, str);

	str += "</";
	str += szDROD;
	str += ">" NEWLINE;

	CFiles f;
	const bool bRes = f.WriteBufferToFile(wstrExportFile.c_str(), str);

	HideStatusMessage();
	SetCursor();
	ShowOkMessage(bRes ? MID_HoldTextSaved : MID_HoldFileNotSaved);
}

//*****************************************************************************
bool CDrodScreen::ExportSelectFile(
//Select a file to export data to.
//
//Returns: true if export confirmed, false on cancel
//
//Params:
	const MESSAGE_ID messageID,   //(in) text prompt to display
	WSTRING &wstrExportFile,      //(in) default filename,
											//(out) Path + File to export to
	const UINT extensionTypes)    //(in) file extension
{
	wstrExportFile = filenameFilter(wstrExportFile);

	//Get default export path.
	CFiles Files;
	WSTRING wstrDefExportPath = Files.GetDatPath();
	wstrDefExportPath += wszSlash;
	wstrDefExportPath += wszDefExportDir;
	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	WSTRING wstrExportPath = pCurrentPlayer ? pCurrentPlayer->Settings.GetVar(
		"ExportPath", wstrDefExportPath.c_str()) : wstrDefExportPath.c_str();

	//Ask user for export path.
	ShowCursor();
	const UINT dwTagNo = SelectFile(wstrExportPath, wstrExportFile,
			messageID, true, extensionTypes);
	if (pCurrentPlayer)
	{
		if (dwTagNo == TAG_OK)
		{
			//Update the export path in player settings, so next time dialog
			//comes up it will have the same path.
			pCurrentPlayer->Settings.SetVar("ExportPath", wstrExportPath.c_str());
			pCurrentPlayer->Update();
		}
		delete pCurrentPlayer;
	}

	return dwTagNo == TAG_OK;
}

//*****************************************************************************
MESSAGE_ID CDrodScreen::GetVersionMID(const UINT wVersion)
//Returns: a text MID for the specified export format version #
{
	if (wVersion >= 401 && wVersion < 406)
		return MID_DROD_RPG1;   //1.0
	if (wVersion == 406)
		return MID_DROD_RPG1_2; //1.1/1.2
	if (wVersion >= 407 && wVersion < 500)
		return MID_DROD_RPG1_3; //1.3
	if (wVersion >= 500 && wVersion < NEXT_VERSION_NUMBER)
		return MID_DROD_RPG2_0;
	return MID_DROD_UnsupportedVersion;
}

//*****************************************************************************
void CDrodScreen::GoToBuyNow()
//Sets the game to windowed mode and opens a browser with appropriate sell link.
{
	SetFullScreen(false);
	string url = "http://www.caravelgames.com/buyRPG.html"; //UPDATE FOR DROD RPG 2
#ifdef STEAMBUILD
	url = "http://store.steampowered.com/app/867797";
#elif defined(WIN32)
	//use default
#elif defined(__linux__)
	url = "http://www.caravelgames.com/buyRPGLinux.html";
#elif defined(__FreeBSD__)
	url = "http://www.caravelgames.com/buyRPGFreeBSD.html";
#elif defined(__APPLE__)
	url = "http://www.caravelgames.com/buyRPGOSX.html";
#else
#	error Add a buy link for this platform ?
#endif

	if (!OpenExtBrowser(url.c_str()))
		ShowOkMessage(MID_NoBrowserToBuy);
}

//*****************************************************************************
void CDrodScreen::GoToForum()
{
	SetFullScreen(false);
	OpenExtBrowser("http://forum.caravelgames.com");
}

//*****************************************************************************
MESSAGE_ID CDrodScreen::Import(
//Import a data file.
//
//Returns: status result of import process, or MID_Success (0) on canceled file selection
//
//Params:
	const UINT extensionTypes, //(in) type of record being imported
	CIDSet& importedIDs,       //(out) IDs of the imported parent records
	set<WSTRING>& importedStyles, //(out)
	const bool bSilent)       //[false]
{
	LOGCONTEXT("CDrodScreen::Import");
	importedIDs.clear();
	importedStyles.clear();

	//Get default export(import) path.
	CFiles Files;
	WSTRING wstrDefExportPath = Files.GetDatPath();
	wstrDefExportPath += wszSlash;
	wstrDefExportPath += wszDefExportDir;
	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	WSTRING wstrImportPath = pCurrentPlayer ? pCurrentPlayer->Settings.GetVar("ImportPath",
			wstrDefExportPath.c_str()) : wstrDefExportPath.c_str();
	vector<WSTRING> wstrImportFiles;

	//Ask player for path + file.
	ShowCursor();
	const UINT dwTagNo = SelectFiles(wstrImportPath, wstrImportFiles,
			MID_ImportPath, extensionTypes);
	if (dwTagNo != TAG_OK)
	{
		delete pCurrentPlayer;
		return MID_Success;
	}

	//Update the path in player settings, so next time dialog
	//comes up it will have the same path.
	if (pCurrentPlayer)
	{
		pCurrentPlayer->Settings.SetVar("ImportPath", wstrImportPath.c_str());
		pCurrentPlayer->Update();
		delete pCurrentPlayer;
	}

	//Commit any changes before proceeding in case the DB gets rolled back.
	g_pTheDB->Commit();

	return ImportFiles(wstrImportFiles, importedIDs, importedStyles, bSilent);
}

//*****************************************************************************
MESSAGE_ID CDrodScreen::ImportFiles(
//Returns: exit code of import process
//
//
	const vector<WSTRING>& wstrImportFiles,    //(in) filenames to import
	CIDSet& importedIDs,  //(out) IDs of the imported parent records
	set<WSTRING>& importedStyles, //(out)
	const bool bSilent)       //[false]
{
	SetCursor(CUR_Wait);

	//Each iteration processes one file.
	MESSAGE_ID result = MID_NoText;
	CImportInfo::ImportType type = CImportInfo::None;
	for (vector<WSTRING>::const_iterator wFilename = wstrImportFiles.begin();
			wFilename != wstrImportFiles.end(); ++wFilename)
	{
		//Determine file type through file name extension.
		const UINT ext = CDrodFileDialogWidget::GetExtensionType(*wFilename);
		switch (ext)
		{
			case FileExtension::DATA: type = CImportInfo::Data; break;
			case FileExtension::DEMO: type = CImportInfo::Demo; break;
			case FileExtension::HOLD: type = CImportInfo::Hold; break;
			case FileExtension::PLAYER: type = CImportInfo::Player; break;
			case FileExtension::XML: type = CImportInfo::LanguageMod; break;
			case FileExtension::SAVE: type = CImportInfo::SavedGame; break;
			default:
				ASSERT(!"Unexpected import type");
				continue;
		}

		//Read and import one file.
		this->callbackContext = *wFilename;
		Callback(MID_Importing);

		CDbXML::SetCallback(this);
		result = CDbXML::ImportXML(wFilename->c_str(), type);
		if (!ImportConfirm(result, &(*wFilename)))
		{
			result = MID_NoText; //nothing was done for this hold
			continue;
		}
		HideStatusMessage();

		//Compile results from each file import.
		UINT dwImportedID;
		switch (type)
		{
			case CImportInfo::LanguageMod:
			case CImportInfo::Hold: dwImportedID = CDbXML::info.dwHoldImportedID; break;
			case CImportInfo::Player: dwImportedID = CDbXML::info.dwPlayerImportedID; break;
			case CImportInfo::Data: dwImportedID = CDbXML::info.dwDataImportedID;
				if (result == MID_ImportSuccessful)
				{
					CFiles f;
					WSTRING wstr;
					Base64::decode(CDbXML::info.headerInfo, wstr);
					f.WriteGameProfileBuffer(wstr,false,false);
				}
			break;
			case CImportInfo::Demo: dwImportedID = 1; break;   //value not used -- indicates a demo was in fact imported
			case CImportInfo::SavedGame: dwImportedID = CDbXML::info.importedSavedGameID; break;
			default: dwImportedID = 0; break;  //value not important
		}
		if (dwImportedID)
			importedIDs += dwImportedID;
		for (set<WSTRING>::const_iterator iter = CDbXML::info.roomStyles.begin();
				iter != CDbXML::info.roomStyles.end(); ++iter)
			importedStyles.insert(*iter);
	}

	//Display result.
	this->pProgressWidget->Hide();
	HideStatusMessage();
	SetCursor();
	if (result != MID_NoText)
		if (!bSilent || result != MID_ImportSuccessful)
			ShowOkMessage(result);
	this->callbackContext.resize(0);
	return result;
}

//*****************************************************************************
bool CDrodScreen::ImportConfirm(MESSAGE_ID& result, const WSTRING* pwFilename)
//Ask for user confirmation, if required.
{
	if (!(result == MID_OverwriteHoldPrompt || result == MID_OverwritePlayerPrompt ||
			result == MID_DowngradeHoldPrompt))
		return true; //no confirmation needed otherwise

	//Data requiring special handling was found requiring user confirmation
	//before proceeding with import.
	HideStatusMessage();

	SetCursor();
	WSTRING wPrompt;
	if (pwFilename)
	{
		wPrompt += *pwFilename;
		wPrompt += wszColon;
		wPrompt += wszCRLF;
	}
	wPrompt += g_pTheDB->GetMessageText(result);
	if (ShowYesNoMessage(wPrompt.c_str()) != TAG_YES)
	{
		//Not continuing import
		CDbXML::CleanUp();
		return false;
	}

	SetCursor(CUR_Wait);
	WSTRING wImporting = g_pTheDB->GetMessageText(MID_Importing);
	if (pwFilename)
	{
		wImporting += wszSpace;
		wImporting += *pwFilename;
	}
	CScreen::ShowStatusMessage(wImporting.c_str());
	switch (result)
	{
		case MID_OverwriteHoldPrompt:
			CDbXML::info.bReplaceOldHolds = true; break;
		case MID_OverwritePlayerPrompt:
			CDbXML::info.bReplaceOldPlayers = true; break;
		case MID_DowngradeHoldPrompt:
			CDbXML::info.bReplaceOldHolds = true;
			CDbXML::info.bAllowHoldDowngrade = true;
			break;
		default: ASSERT(!"Handle this case"); break;
	}
	CDbXML::SetCallback(this);
	result = CDbXML::ImportXML();
	return true;
}

//*****************************************************************************
//Return: a "good enough" threshold to distinguish between the demo and full official game hold
//        i.e., if the hold has at least this many level entrances defined, this is the full (registered) hold
UINT CDrodScreen::EntrancesInFullVersion()
{
	return 50;
}

//*****************************************************************************
CDbHold::HoldStatus CDrodScreen::GetInstalledOfficialHold()
{
	static const CDbHold::HoldStatus officialHolds[] = {
		CDbHold::ACR, CDbHold::Tendry, CDbHold::NoStatus };

	for (UINT i = 0; officialHolds[i] != CDbHold::NoStatus; ++i) {
		const UINT holdID = g_pTheDB->Holds.GetHoldIDWithStatus(officialHolds[i]);
		if (holdID)
			return officialHolds[i];
	}

	return CDbHold::NoStatus;
}

//*****************************************************************************
bool CDrodScreen::IsGameFullVersion()
//Returns: whether the included hold is the full version (true), otherwise false
{
#ifdef STEAMBUILD
	return Metadata::GetInt(MetaKey::DEMO) != 1;
#endif

	//Programmatic override for embedding game media files in dev build.
	if (Metadata::GetInt(MetaKey::EMBEDMEDIA) == 1)
		return Metadata::GetInt(MetaKey::DEMO) != 1;

	const UINT mainholdID = g_pTheDB->Holds.GetHoldIDWithStatus(CDbHold::GetOfficialHoldStatus());
	if (!mainholdID)
		return false;
	CDbHold *pHold = g_pTheDB->Holds.GetByID(mainholdID);
	const bool bFull = pHold && pHold->Entrances.size() >= EntrancesInFullVersion();
	delete pHold;
	return bFull;
}

//*****************************************************************************
void CDrodScreen::ExportStyle(const WSTRING& style)
//Exports a style from files on disk into a DROD-format media file.
{
	CFiles f;
	list<WSTRING> styleName, tiles;
	if (!f.GetGameProfileString(INISection::Graphics, style.c_str(), styleName))
		return; //style not listed
	if (styleName.empty())
		return; //no entries for style

	//Sky images are optionally included.
	const bool bExportSkies = ShowYesNoMessage(MID_IncludeSkyImagesInStylePrompt) == TAG_YES;
	list<WSTRING> skies;
	WSTRING skyName;
	if (bExportSkies)
	{
		skyName = style;
		skyName += wszSpace;
		skyName += wszSKIES;
		f.GetGameProfileString(INISection::Graphics, skyName.c_str(), skies);
	}

	UINT wI;
	CIDSet ids;

	//One or more files named after this style must be found on disk.
	if (!IsStyleOnDisk(styleName, skies))
		return;

	//Select file name.
	WSTRING wstrExportFile = style;
	if (!ExportSelectFile(MID_SaveStylePath, wstrExportFile, EXT_DATA))
		return;

	//Export style images.
	const WSTRING wstrBasename = styleName.front();
	for (wI=TEXTURE_COUNT; wI--; )
	{
		if (wI == TEXTURE_COUNT - 1) { //skip OVERHEAD_IMAGE texture
			continue;
		}

		WSTRING wstrFile = wstrBasename;
		if (wI == TEXTURE_COUNT-2) { //skip FLOOR_IMAGE texture: load 'tiles' images instead
			wstrFile += wszTILES;
		} else {
			//Style textures.
			WSTRING wstr;
			UTF8ToUnicode(textureTileNames[wI], wstr);
			wstrFile += wstr;
		}

		PrepareDatumForExport(style, wstrFile, ids, DATA_IMAGE);
	}

	list<WSTRING>::const_iterator iStr = styleName.begin();
	++iStr;
	for ( ; iStr != styleName.end(); ++iStr)
		PrepareDatumForExport(style, *iStr, ids, DATA_IMAGE);

	if (bExportSkies)
		for (iStr = skies.begin(); iStr != skies.end(); ++iStr)
			PrepareDatumForExport(style, *iStr, ids, DATA_IMAGE);

	//Gather .ini info.
	WSTRING iniInfo;
	AsciiToUnicode("[Graphics]\nStyle=", iniInfo);
	iniInfo += style;
	iniInfo += wszCRLF;
	iniInfo += style;
	iniInfo += wszEqual;
	for (iStr = styleName.begin(); iStr != styleName.end(); ++iStr)
	{
		if (iStr != styleName.begin())
			iniInfo += wszSemicolon; //semicolon separated
		iniInfo += *iStr;
	}
	iniInfo += wszCRLF;
	if (!skies.empty())
	{
		iniInfo += skyName;
		iniInfo += wszEqual;
		for (list<WSTRING>::const_iterator iStr = skies.begin(); iStr != skies.end(); ++iStr)
		{
			if (iStr != skies.begin())
				iniInfo += wszSemicolon; //semicolon separated
			iniInfo += *iStr;
		}
		iniInfo += wszCRLF;
	}
	CDbXML::info.headerInfo = Base64::encode(iniInfo);

	const bool bResult = CDbXML::ExportXML(V_Data, ids, wstrExportFile.c_str());

	//Clean up temporary data objects.
	for (CIDSet::const_iterator iter = ids.begin(); iter != ids.end(); ++iter)
		g_pTheDB->Data.Delete(*iter);

	this->pProgressWidget->Hide();
	HideStatusMessage();
	SetCursor();
	ShowOkMessage(bResult ? MID_DataFileSaved : MID_DataFileNotSaved);
}

//*****************************************************************************
void CDrodScreen::ImportMedia()
//Imports sound and graphics files from disk into the DB.
{
#ifdef EMBED_STYLES
	//Get names of all styles.
	list<WSTRING> styles;
	if (!CFiles::GetGameProfileString(INISection::Graphics, INIKey::Style, styles))
		return;	//styles are missing

	CFiles f;

	//Log all files being exported for debugging purposes.
	WSTRING wstrLog;
	AsciiToUnicode(".export.log", wstrLog);
	WSTRING wstrLogFilename = f.GetDatPath();
	wstrLogFilename += wszSlash;
	wstrLogFilename += CFiles::wGameName;
	wstrLogFilename += wstrLog;

#define LOG_TEXT(wtext) { string strLog = UnicodeToUTF8(wtext); strLog += NEWLINE; CStretchyBuffer text(strLog); f.WriteBufferToFile(wstrLogFilename.c_str(), text, true); }

	//Default: if official hold is the demo version, include selected demo styles only. Otherwise include all of them.
//	CDbHold::HoldStatus status = GetInstalledOfficialHold();
//	if (status == CDbHold::NoStatus)
//		status = (CDbHold::HoldStatus)Metadata::GetInt(MetaKey::APPLYHOLDSTATUS);
	const bool bDemo = !IsGameFullVersion();
	if (bDemo)
	{
		//Export only Deep Spaces style for demo.
		styles.clear();
		WSTRING demoStyle;
		AsciiToUnicode("Deep Spaces", demoStyle);
		styles.push_back(demoStyle);
	}
	UINT wStylesToExport = styles.size(); 

	//Ensure each song is only imported once.
	set<WSTRING> embeddedImages, embeddedSongs, embeddedSounds;

	UINT wStyleNo, wI;
	for (wStyleNo = 1; wStyleNo <= wStylesToExport; ++wStyleNo)
	{
		ASSERT(!styles.empty());
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

		//Import images.
		static const UINT NUM_SELL_SCREENS = 4;
		static const UINT NUM_EXIT_SCREENS = NUM_SELL_SCREENS;
		static const UINT NUM_GRAPHICS_FILES = 23;
		static const char graphicFilename[NUM_GRAPHICS_FILES][32] = {
			"Background", "Bolts",
//			"Clock1", "Clock2",
			"Credits", "Dialog",
			"Faces", "GameScreen", "GameScreenParts",
			"LevelStartBackground", "RoomEditScreen",
			"TitleShadow", "TitleBG", "TitleBG1",
			"TitleBG2", "TitleBGTunnel", "TitleDROD", "TitleLightMask",
			"Fog1", "Clouds1", "Sunshine1",
			"SignalBad", "SignalGood", "SignalNo", "SignalYes"
		};
		for (wI=TEXTURE_COUNT + 1 + NUM_GRAPHICS_FILES + NUM_EXIT_SCREENS; wI--; )
		{
			WSTRING wstrImportFile = wstrBasename;
			if (wI >= TEXTURE_COUNT)
			{
				//Embed non-style image files.

				if (wStyleNo > 1)
					continue;	//only embed the following graphics once

				if (wI == TEXTURE_COUNT)
				{
					list<WSTRING> tiles;

					//General game tiles.
					//ATTN: This only recognizes the first name in a set of tiles names.
					if (!f.GetGameProfileString(INISection::Graphics, "General", tiles))
						continue;	//missing
					ASSERT(!tiles.empty());
					wstrImportFile = tiles.front();
				}
				else if (wI <= TEXTURE_COUNT + NUM_GRAPHICS_FILES)
				{
					//Graphics files to be embedded.
					UTF8ToUnicode(graphicFilename[wI - TEXTURE_COUNT - 1], wstrImportFile);
				} else {
					//Demo/registered exit screens.  Import one kind or the other.
					if (wStylesToExport > 1) //no sell screens in full version
						continue;
					static const WCHAR wszSell[] = {{'S'},{'e'},{'l'},{'l'},{'1'},{'1'},{0}};
					static const WCHAR wszSellSShots[] = {{'S'},{'e'},{'l'},{'l'},{'S'},{'S'},{'h'},{'o'},{'t'},{'s'},{0}};
					static const WCHAR wszSell2[] = {{'S'},{'e'},{'l'},{'l'},{'1'},{'2'},{0}};
					static const WCHAR wszSellSShots2[] = {{'S'},{'e'},{'l'},{'l'},{'S'},{'S'},{'h'},{'o'},{'t'},{'s'},{'2'},{0}};

					const UINT wSellScreenNum = wI - TEXTURE_COUNT - NUM_GRAPHICS_FILES;
					ASSERT(wStylesToExport == 1 && wSellScreenNum <= NUM_SELL_SCREENS);
					switch (wSellScreenNum)
					{
						case 1: wstrImportFile = wszSell; break;
						case 2: wstrImportFile = wszSellSShots; break;
						case 3: wstrImportFile = wszSell2; break;
						case 4: wstrImportFile = wszSellSShots2; break;
						default: ASSERT(!"Bad sell screen image index"); break;
					}
				}
			} else if (wI == TEXTURE_COUNT-1) { //skip OVERHEAD_IMAGE texture
				continue;
			} else if (wI == TEXTURE_COUNT-2) {
				wstrImportFile += wszTILES;	//skip FLOOR_IMAGE texture: load 'tiles' images now
			} else {
				//Room style image files.
				WSTRING wstr;
				UTF8ToUnicode(textureTileNames[wI], wstr);
				wstrImportFile += wstr;
			}

			if (embeddedImages.count(wstrImportFile) != 0)
				continue; //this image was already processed -- don't reimport it
			embeddedImages.insert(wstrImportFile);

			LOG_TEXT(wstrImportFile);

			WSTRING wstrFilepath;
			const UINT wDataFormat = g_pTheBM->GetImageFilepath(wstrImportFile.c_str(), wstrFilepath);
			if (wDataFormat == DATA_UNKNOWN)
			{
				WSTRING wstr = wstrImportFile;
				wstr += wszColon;
				wstr += wszSpace;
				wstr += g_pTheDB->GetMessageText(MID_FileNotFound);
				ShowOkMessage(wstr.c_str());
			} else {
				CStretchyBuffer data;
				if (f.ReadFileIntoBuffer(wstrFilepath.c_str(), data, true))
				{
					CScreen::ShowStatusMessage(wstrImportFile.c_str());
					CDbDatum *pImage = g_pTheDB->Data.GetNew();
					pImage->wDataFormat = wDataFormat;
					pImage->DataNameText = wstrImportFile.c_str();
					pImage->dwHoldID = 0;
					pImage->data.Set((const BYTE*)data,data.Size());
					if (wI == TEXTURE_COUNT || wI == FLOOR_IMAGE)
					{
						//Get .tim data.
						const WSTRING wstrTimFilepath = g_pTheBM->GetTileImageMapFilepath(wstrImportFile.c_str());
						if (!CFiles::ReadFileIntoBuffer(wstrTimFilepath.c_str(), pImage->timData))
							ShowOkMessage(MID_FileNotFound);
					}
					pImage->Update();
					delete pImage;
				}
			}
		}

		//Import style sky images.
		WSTRING wstrSkyname = wstrBasename;
		wstrSkyname += wszSpace;
		wstrSkyname += wszSKIES;
		list<WSTRING> skies;
		if (CFiles::GetGameProfileString(INISection::Graphics, wstrSkyname.c_str(), skies))
			for (list<WSTRING>::const_iterator sky = skies.begin(); sky != skies.end(); ++sky)
			{
				if (embeddedImages.count(*sky) != 0)
					continue; //this image was already processed -- don't reimport it
				embeddedImages.insert(*sky);

				WSTRING wstrFilepath;
				const UINT wDataFormat = g_pTheBM->GetImageFilepath(sky->c_str(), wstrFilepath);
				if (wDataFormat == DATA_UNKNOWN)
				{
					WSTRING wstr = *sky;
					wstr += wszColon;
					wstr += wszSpace;
					wstr += g_pTheDB->GetMessageText(MID_FileNotFound);
					ShowOkMessage(wstr.c_str());
				} else {
					CStretchyBuffer data;
					if (f.ReadFileIntoBuffer(wstrFilepath.c_str(), data, true))
					{
						CScreen::ShowStatusMessage(sky->c_str());
						CDbDatum *pImage = g_pTheDB->Data.GetNew();
						pImage->wDataFormat = wDataFormat;
						pImage->DataNameText = sky->c_str();
						pImage->dwHoldID = 0;
						pImage->data.Set((const BYTE*)data,data.Size());
						pImage->Update();
						delete pImage;
					}
				}
			}

		//Import music.
		WSTRING wstrDir;
		for (wI=SONG_MOOD_COUNT+35; wI--; )	//include other music too
		{
			list<WSTRING> songlist;

			if (wI < SONG_MOOD_COUNT)
			{
				//Get song list for this mood from INI
				WSTRING wMoodText;
				UTF8ToUnicode(moodText[wI], wMoodText);
				WSTRING wstrSongmood = styleName + wMoodText;
				f.GetGameProfileString(INISection::Songs, wstrSongmood.c_str(), songlist);
			}
			else switch (wI)
			{
				case SONG_MOOD_COUNT+34:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_TITLE_2, songlist);
					break;

				//Include end game music for full hold/version only.
				case SONG_MOOD_COUNT+33:
					if (wStyleNo == 2)
						g_pTheSound->GetSongFilepaths(SONGID_CREDITS, songlist);
				break;

				//Import other playable songs once.
				case SONG_MOOD_COUNT+32:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_ARCHITECTS, songlist);
				break;
				case SONG_MOOD_COUNT+31:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_BATTLE, songlist);
				break;
				case SONG_MOOD_COUNT+30:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_BENEATH, songlist);
				break;
				case SONG_MOOD_COUNT+29:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_DREAMER, songlist);
				break;
				case SONG_MOOD_COUNT+28:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_GOBLINS, songlist);
				break;
				case SONG_MOOD_COUNT+27:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_REDGUARD, songlist);
				break;
				case SONG_MOOD_COUNT+26:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_SEASIDE, songlist);
				break;
				case SONG_MOOD_COUNT+25:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_SERPENTS, songlist);
				break;
				case SONG_MOOD_COUNT+24:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_SLAYER, songlist);
				break;
				case SONG_MOOD_COUNT+23:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_PIRATES, songlist);
					break;
				case SONG_MOOD_COUNT+22:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_GOBLINKING, songlist);
					break;
				case SONG_MOOD_COUNT+21:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_BIGSERPENT, songlist);
					break;
				case SONG_MOOD_COUNT+20:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_TAR, songlist);
					break;
				case SONG_MOOD_COUNT+19:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_PUZZLE, songlist);
					break;
				case SONG_MOOD_COUNT+18:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_SECRETAREA, songlist);
					break;
				case SONG_MOOD_COUNT+17:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_SMALLERPLANS, songlist);
					break;
				case SONG_MOOD_COUNT+16:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_GEOMETRY, songlist);
					break;
				case SONG_MOOD_COUNT+15:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_SLIPSTAIR, songlist);
					break;
				case SONG_MOOD_COUNT+14:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_SYMPATHETIC, songlist);
					break;
				case SONG_MOOD_COUNT+13:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_ASCENDANT, songlist);
					break;
				case SONG_MOOD_COUNT+12:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_ROACHESRUN, songlist);
					break;
				case SONG_MOOD_COUNT+11:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_NEWIDEA, songlist);
					break;
				case SONG_MOOD_COUNT+10:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_WALTZ, songlist);
					break;
				case SONG_MOOD_COUNT+9:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_SWINGHALLS, songlist);
					break;
				case SONG_MOOD_COUNT+8:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_ELIZABETH, songlist);
					break;
				case SONG_MOOD_COUNT+7:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_AMBWINDY, songlist);
					break;
				case SONG_MOOD_COUNT+6:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_AMBBEACH, songlist);
					break;
				case SONG_MOOD_COUNT+5:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_AMBFOREST, songlist);
					break;
				case SONG_MOOD_COUNT+4:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_AMBDRIPS, songlist);
					break;
				case SONG_MOOD_COUNT+3:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_AMBROACHES, songlist);
					break;

				//Import intro, exit, and end hold music once.
				case SONG_MOOD_COUNT+2:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_INTRO, songlist);
				break;
				case SONG_MOOD_COUNT+1:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_QUIT, songlist);
				break;
				case SONG_MOOD_COUNT:
					if (wStyleNo == 1)
						g_pTheSound->GetSongFilepaths(SONGID_WINGAME, songlist);
				break;

				default: ASSERT(!"Importing unspecified music"); break;
			}
			wstrDir = g_pTheSound->GetSongPath();
			while (!songlist.empty())
			{
				WSTRING wstrSongFilepath, wstrSongFilename = songlist.front();
				songlist.pop_front();

				if (embeddedSongs.count(wstrSongFilename) != 0)
					continue; //this song was already processed -- don't redo it
				embeddedSongs.insert(wstrSongFilename);

				LOG_TEXT(wstrSongFilename);

				wstrSongFilepath = wstrDir + wstrSongFilename;

				CStretchyBuffer data;
				if (f.ReadFileIntoBuffer(wstrSongFilepath.c_str(), data, true))
				{
					if (!g_pTheSound->VerifySound(data))
						ShowOkMessage(MID_FileCorrupted);
					else
					{
						CScreen::ShowStatusMessage(wstrSongFilename.c_str());
						CDbDatum *pSound = g_pTheDB->Data.GetNew();
						pSound->wDataFormat = DATA_OGG;
						pSound->DataNameText = wstrSongFilename.c_str();
						pSound->dwHoldID = 0;
						pSound->data.Set((const BYTE*)data,data.Size());
						pSound->Update();
						delete pSound;
					}
				}
			}
		}
	}

	//Import sound effects.
	WSTRING wstrSoundPath, wstrDir = CSoundEffect::GetPath();

	list<WSTRING> singleList;
	for (wI=SEID_BUTTON; wI<SEID_COUNT; ++wI)
	{
		g_pTheSound->GetSoundFilenames(wI, singleList);

		//Add all filenames into a set.
		for (list<WSTRING>::const_iterator iFilename = singleList.begin();
			iFilename != singleList.end(); ++iFilename)
		{
			if (embeddedSounds.count(*iFilename) != 0)
				continue; //this sound file has already been imported -- skip it
			embeddedSounds.insert(*iFilename);

			LOG_TEXT(*iFilename);

			wstrSoundPath = wstrDir + *iFilename;

			CStretchyBuffer data;
			if (f.ReadFileIntoBuffer(wstrSoundPath.c_str(), data, true))
			{
				if (!g_pTheSound->VerifySound(data))
					ShowOkMessage(MID_FileCorrupted);
				else
				{
					//Import a sound file.
					CScreen::ShowStatusMessage(iFilename->c_str());
					CDbDatum *pSound = g_pTheDB->Data.GetNew();
					pSound->wDataFormat = DATA_OGG;
					pSound->DataNameText = iFilename->c_str();
					pSound->dwHoldID = 0;
					pSound->data.Set((const BYTE*)data,data.Size());
					pSound->Update();
					delete pSound;
				}
			}
		}
	}

	HideStatusMessage();
#undef LOG_TEXT
#endif
}

//*****************************************************************************
void CDrodScreen::EnablePlayerSettings(
//Set game parameters according to player settings.
//
//Params:
	const UINT dwPlayerID) //(in) Player
{
	CDbPlayer *pPlayer = g_pTheDB->Players.GetByID(dwPlayerID);
	if (!pPlayer)
	{
		ASSERT(!"Could not retrieve player data.");
		return;
	}

	CDbPlayer::ConvertInputSettings(pPlayer->Settings);

	g_pTheSound->EnableSoundEffects(pPlayer->Settings.GetVar(Settings::SoundEffects, true));
	g_pTheSound->SetSoundEffectsVolume(pPlayer->Settings.GetVar(Settings::SoundEffectsVolume, (BYTE)DEFAULT_SOUND_VOLUME));

	g_pTheSound->EnableVoices(pPlayer->Settings.GetVar(Settings::Voices, true));
	g_pTheSound->SetVoicesVolume(pPlayer->Settings.GetVar(Settings::VoicesVolume, (BYTE)DEFAULT_VOICE_VOLUME));

	g_pTheSound->EnableMusic(pPlayer->Settings.GetVar(Settings::Music, true));
	g_pTheSound->SetMusicVolume(pPlayer->Settings.GetVar(Settings::MusicVolume, (BYTE)DEFAULT_MUSIC_VOLUME));

	SetFullScreen(pPlayer->Settings.GetVar(Settings::Fullscreen, false));
	g_pTheBM->bAlpha = pPlayer->Settings.GetVar(Settings::Alpha, true);
	g_pTheDBM->SetGamma(pPlayer->Settings.GetVar(Settings::Gamma, (BYTE)CDrodBitmapManager::GetGammaOne()));
	g_pTheBM->eyeCandy = (pPlayer->Settings.GetVar(Settings::EyeCandy, true) ? 1 : 0);
	g_pTheDBM->tarstuffAlpha = pPlayer->Settings.GetVar(Settings::TarstuffAlpha, BYTE(255));
	g_pTheDBM->mapIconAlpha = pPlayer->Settings.GetVar(Settings::MapIconAlpha, BYTE(255));

	CScreen::inputKeyFullScreen = pPlayer->Settings.GetVar(
		InputCommands::GetKeyDefinition(InputCommands::DCMD_ToggleFullScreen)->settingName,
		(InputKey)SDLK_F10
	);
	CScreen::inputKeyScreenshot = pPlayer->Settings.GetVar(
		InputCommands::GetKeyDefinition(InputCommands::DCMD_Screenshot)->settingName,
		(InputKey)SDLK_F11
	);

	//RepeatRate is queried in CGameScreen::ApplyPlayerSettings().

	delete pPlayer;
}

//*****************************************************************************
bool CDrodScreen::ImportQueuedFiles()
//Import queued filenames.
{
	if (importFiles.empty())
		return false;

	CIDSet ignored;
	set<WSTRING> importedStyles;
	ImportFiles(CDrodScreen::importFiles, ignored, importedStyles);
	importFiles.clear();

#ifdef DEV_BUILD
	return true; //signal to exit app
#endif
	return false;
}

//*****************************************************************************
bool CDrodScreen::IsStyleOnDisk(
//Style files must be on disk (not in .dat files) to perform an export.
//
//Returns: whether any files specified for this room style are on disk,
//as opposed to being found in the .dat files.
//
//Params:
	list<WSTRING>& styleName, list<WSTRING>& skies) //files in style
{
	if (styleName.empty())
		return false;
	const WSTRING wstrBasename = styleName.front();

	//Texture set.
	for (UINT wI=TEXTURE_COUNT; wI--; )
	{
		WSTRING wstrFile = wstrBasename;
		if (wI == TEXTURE_COUNT-1) { //skip FLOOR_IMAGE texture: load 'tiles' images instead
			wstrFile += wszTILES;
		} else {
			//Style textures.
			WSTRING wstr;
			UTF8ToUnicode(textureTileNames[wI], wstr);
			wstrFile += wstr;
		}

		WSTRING wstrFilepath;
		const UINT wDataFormat = g_pTheBM->GetImageFilepath(wstrFile.c_str(), wstrFilepath);
		if (wDataFormat != DATA_UNKNOWN)  //valid file available
			return true;
	}
	//Additional tile files.
	list<WSTRING>::const_iterator iStr = styleName.begin();
	++iStr;
	for ( ; iStr != styleName.end(); ++iStr)
	{
		WSTRING wstrFilepath, wstrFile = *iStr;
		const UINT wDataFormat = g_pTheBM->GetImageFilepath(wstrFile.c_str(), wstrFilepath);
		if (wDataFormat != DATA_UNKNOWN)  //valid file available
			return true;
	}
	//Sky images.
	for (iStr = skies.begin(); iStr != skies.end(); ++iStr)
	{
		WSTRING wstrFilepath;
		const UINT wDataFormat = g_pTheBM->GetImageFilepath(iStr->c_str(), wstrFilepath);
		if (wDataFormat != DATA_UNKNOWN)  //valid file available
			return true;
	}

	//No style files are available on disk.
	return false;
}

//*****************************************************************************
bool CDrodScreen::IsCommandSupported(int command) const
//Returns: if the given command does something on this screen.
{
	return false;
}

//*****************************************************************************
int CDrodScreen::GetCommandForInputKey(InputKey inputKey) const
{
	std::map<InputKey, int>::const_iterator it = this->InputKeyToCommandMap.find(inputKey);
	if (it != this->InputKeyToCommandMap.end())
		return it->second;

	return CMD_UNSPECIFIED;
}

//*****************************************************************************
InputKey CDrodScreen::GetInputKeyForCommand(const UINT wCommand) const
//Returns: keysym currently set for indicated command
{
	for (std::map<InputKey, int>::const_iterator it = InputKeyToCommandMap.begin(); it != InputKeyToCommandMap.end(); ++it)
		if (it->second == (int)wCommand)
			return it->first;

	ASSERT(!"Command not assigned");
	return UNKNOWN_INPUT_KEY;
}

//*****************************************************************************
void CDrodScreen::InitKeysymToCommandMap(
//Set the keysym-to-command map with values from player settings that will determine
//which commands correspond to which keys.
//
//Params:
	CDbPackedVars& PlayerSettings)   //(in)   Player settings to load from.
{
	//Clear the map.
	this->InputKeyToCommandMap.clear();

	//Check whether default is for keyboard or laptop.
	CFiles Files;
	string strKeyboard;
	UINT wKeyboard = 0;	//default to numpad
	if (Files.GetGameProfileString(INISection::Localization, INIKey::Keyboard, strKeyboard))
	{
		wKeyboard = atoi(strKeyboard.c_str());
		if (wKeyboard > 1) wKeyboard = 0;	//invalid setting
	}

	//Get key command values from current player settings.
	for (UINT wIndex = 0; wIndex < InputCommands::DCMD_Count; ++wIndex)
	{
		const InputCommands::KeyDefinition* keyDefinition = InputCommands::GetKeyDefinition(wIndex);
		const int command = (int)keyDefinition->eCommand;

		//Different screens support different commands
		if (!IsCommandSupported(command))
			continue;

		const InputKey inputKey = PlayerSettings.GetVar(keyDefinition->settingName, keyDefinition->GetDefaultKey(wKeyboard));
		const bool bInvalidSDL1mapping = inputKey >= 128 && inputKey <= 323;
		this->InputKeyToCommandMap[inputKey] = command;

		if (InputCommands::DoesCommandUseModifiers((InputCommands::DCMD)wIndex)) // Support for macros
			this->InputKeyToCommandMap[BuildInputKey(int32_t(inputKey), false, false, true)] = command;
	}
}

//*****************************************************************************
bool CDrodScreen::OnQuit()
//Called when SDL_QUIT event is received.
{
	//If a Quit event is received while a quit prompt is already active,
	//then this serves as confirmation of the quit prompt.
	if (this->bQuitPrompt)
		return true;

	//Instead of quitting, go to the sell screen when this happens (demo only).
	if (IsGameFullVersion())
	{
		this->bQuitPrompt = true;
		const UINT ret = ShowYesNoMessage(MID_ReallyQuit);
		this->bQuitPrompt = false;
		if (ret != TAG_NO)
			GoToScreen(SCR_None);
	}
	else
		GoToScreen(SCR_Sell);
	return false;
}

//*****************************************************************************
bool CDrodScreen::PlayVideo(const WCHAR *pFilename, const UINT dwHoldID, const int x, const int y)
//Plays a video file on the current screen.  Look in the DB if not found on disk.
{
	if (CScreen::PlayVideo(pFilename, x, y))
	{
		ClearEvents();
		return true; //played successfully
	}

	//Wasn't found on disk -- look in DB for file.
	SetCursor(SCREENLIB::CUR_Wait);
	CDb db;
	CIDSet imageFormats(DATA_THEORA);
	db.Data.FilterByFormat(imageFormats);
	db.Data.FilterByHold(dwHoldID);
	const UINT dwDataID = db.Data.FindByName(pFilename);
	if (!dwDataID)
		return false; //not found in DB

	return PlayVideo(dwDataID, x, y);
}

//*****************************************************************************
bool CDrodScreen::PlayVideo(const UINT dwDataID, const int x, const int y)
//Plays a video file on the current screen.  Uses the specified data record.
{
	CStretchyBuffer buffer;
	if (!CDbData::GetRawDataForID(dwDataID, buffer))
		return false;

	const bool bIsCursorShowing = IsCursorVisible();
	if (bIsCursorShowing)
		HideCursor();
	const bool bRes = PlayVideoBuffer(buffer, GetMainWindow(), x, y);
	if (bIsCursorShowing)
	{
		ShowCursor();
		SetCursor();
	}
	ClearEvents();
	return bRes;
}

//*****************************************************************************
bool CDrodScreen::PollForCNetInterrupt()
//This method is called while waiting blocking and waiting for a response from
//the prior CaravelNet operation.
//Returns: true if user has requested interrupting the current CaravelNet operation
{
	//Get any events waiting in the queue.
	bool bInterrupt = false;
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
						bInterrupt = true;
					break;
					default: break;
				}
			break;
			case SDL_QUIT:
				bInterrupt = true;
			break;
			default: break;
		}
	}
	return bInterrupt;
}

//*****************************************************************************
void CDrodScreen::RequestToolTip(const MESSAGE_ID messageID)
//Overrides CScreen version for DB access.
{
	CScreen::RequestToolTip(g_pTheDB->GetMessageText(messageID));
}

//*****************************************************************************
bool CDrodScreen::SaveGamesToDisk(const CIDSet& savedGameIDs)
//Prompts the user to save the indicated saved game record to a file on disk.
//
//Returns: true if the saved game record was saved successfully to a file on disk
{
	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	if (!pPlayer)
		return false;

	//Default filename is player name.
	WSTRING wstrExportFile = (WSTRING)pPlayer->NameText;
	//!!use some other decorators
//	wstrExportFile += wszSpace;
//	wstrExportFile += g_pTheDB->GetMessageText(MID_Saves);
	bool bResult = false;
	if (ExportSelectFile(MID_SavePlayerPath, wstrExportFile, EXT_SAVE))
	{
		//Write the player's current game to a save file.
		SetCursor(CUR_Wait);
		ShowStatusMessage(MID_Exporting);
		CDbXML::SetCallback(this);

		bResult = CDbXML::ExportXML(V_SavedGames, savedGameIDs,
			wstrExportFile.c_str());
		this->pProgressWidget->Hide();
		HideStatusMessage();
		SetCursor();
	}
	delete pPlayer;
	return bResult;
}

//*****************************************************************************
void CDrodScreen::SaveSurface(SDL_Surface *pSurface)
//Saves surface as image to selected export dir.
{
	//Get default export path.
	CFiles Files;
	WSTRING wstrDefExportPath = Files.GetDatPath();
	wstrDefExportPath += wszSlash;
	wstrDefExportPath += wszDefExportDir;
	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	WSTRING wstrExportPath = pCurrentPlayer ? pCurrentPlayer->Settings.GetVar(
		"ExportPath", wstrDefExportPath.c_str()) : wstrDefExportPath.c_str();
	delete pCurrentPlayer;

	//Find first free filename.
	static WCHAR drodFileName[] = {We('d'),We('r'),We('o'),We('d'),We('s'),We('c'),We('r'),We('e'),We('e'),We('n'),We(0)};
	WSTRING fileName, baseFileName = wstrExportPath;
	baseFileName += wszSlash;
	baseFileName += drodFileName;

	//Save with incrementing number appended.
	UINT wIndex=0, wImageFormat=0;
	do {
		WCHAR num[10];
		_itoW(wIndex,num,10);
		fileName = baseFileName + num;
		wImageFormat = CBitmapManager::GetImageFilepathExtensionType(fileName.c_str());
		if (wImageFormat) //this image file already exists
			++wIndex;      //check next incremental name
	} while (wImageFormat);

	if (!pSurface)
		pSurface = GetWidgetScreenSurface();
	SaveSnapshot(pSurface, fileName);
}

//*****************************************************************************
CEntranceSelectDialogWidget::BUTTONTYPE CDrodScreen::SelectListID(
//Displays a dialog box with a list box of all of the stated datatype in the hold.
//Hitting OK will set the parameter to the selected item's ID.
//Hitting Cancel will leave it unchanged.
//
//Returns: which button type was clicked
//
//Params:
	CEntranceSelectDialogWidget *pEntranceBox,   //(in)
	CDbHold *pHold,      //(in)
	UINT &dwListItemID, //(in/out)  ID of selected item on OK.
	const MESSAGE_ID messagePromptID,   //(in)
	const CEntranceSelectDialogWidget::DATATYPE datatype) //[default=Entrances]
{
	ASSERT(pEntranceBox);
	ASSERT(pHold);
	ASSERT(pEntranceBox->IsLoaded());

	pEntranceBox->SetPrompt(messagePromptID);
	pEntranceBox->SetSourceHold(pHold);
	pEntranceBox->PopulateList(datatype);

	//Select current choice.
	pEntranceBox->SelectItem(dwListItemID);

	const CEntranceSelectDialogWidget::BUTTONTYPE eButton =
			(CEntranceSelectDialogWidget::BUTTONTYPE)pEntranceBox->Display();
	Paint();

	//Get selected value.
	dwListItemID = pEntranceBox->GetSelectedItem();
	return eButton;
}

//*****************************************************************************
UINT CDrodScreen::SelectFile(
//Displays a dialog box with a file list box.
//Hitting OK will set fileName to the selected file.
//Hitting Cancel will leave it unchanged.
//
//Return: tag # of button clicked
//
//Params:
	WSTRING &filePath,   //(in/out) File path; new path on OK.
	WSTRING &fileName,   //(in/out) Default filename; File path + filename on OK.
	const MESSAGE_ID messagePromptID,   //(in)
	const bool bWrite,   //(in) whether the file being selected is being written to
	const UINT extensionTypes)  //(in) file extension
{
	this->pFileBox->SetPrompt(g_pTheDB->GetMessageText(messagePromptID));
	this->pFileBox->SetExtensions(extensionTypes);

	return CScreen::SelectFile(filePath, fileName, bWrite);
}

//******************************
UINT CDrodScreen::SelectFiles(
	WSTRING& filePath,          //(in/out) File path; new path on OK.
	vector<WSTRING>& fileNames, //(in/out) Default filenames; File path + filenames on OK.
	const MESSAGE_ID messagePromptID, //(in)
	const UINT extensionTypes)  //(in) file extension
{
	this->pFileBox->SetPrompt(g_pTheDB->GetMessageText(messagePromptID));
	this->pFileBox->SetExtensions(extensionTypes);

	return CScreen::SelectFiles(filePath, fileNames);
}

//*****************************************************************************
UINT CDrodScreen::ShowOkMessage(
//Display a message in a dialog.  Dialog has an okay button and execution
//waits for the button to be pushed before returning.
//
//Params:
	const MESSAGE_ID dwMessageID) //(in)   Indicates message to display.
//
//Returns:
//TAG_OK or TAG_QUIT.
{
	//Show just the "Ok" button.
	CWidget *pButton = this->pMessageDialog->GetWidget(TAG_OK);
	pButton->Show();
	pButton = this->pMessageDialog->GetWidget(TAG_YES);
	pButton->Hide(false);
	pButton = this->pMessageDialog->GetWidget(TAG_NO);
	pButton->Hide(false);

	//Activate the dialog widget.
	return ShowMessage(dwMessageID);
}

//*****************************************************************************
UINT CDrodScreen::ShowOkMessage(
//Display a message in a dialog.  Dialog has an okay button and execution
//waits for the button to be pushed before returning.
//
//Params:
	const WCHAR* pwczText) //(in)   Message to display.
//
//Returns:
//TAG_OK or TAG_QUIT.
{
	//Show just the "Ok" button.
	CWidget *pButton = this->pMessageDialog->GetWidget(TAG_OK);
	pButton->Show();
	pButton = this->pMessageDialog->GetWidget(TAG_YES);
	pButton->Hide(false);
	pButton = this->pMessageDialog->GetWidget(TAG_NO);
	pButton->Hide(false);

	//Activate the dialog widget.
	return ShowMessage(pwczText);
}

//*****************************************************************************
UINT CDrodScreen::ShowYesNoMessage(
//Display a message in a dialog.  Dialog has a yes and no button and execution
//waits for a button to be pushed before returning.
//
//Params:
	const MESSAGE_ID dwMessageID, //(in)   Indicates message to display.
	const MESSAGE_ID dwYesButtonText, //[default=MID_Yes]
	const MESSAGE_ID dwNoButtonText)  //[default=MID_No]
//
//Returns:
//TAG_YES (user pressed yes button), TAG_NO (user pressed no button),
//or TAG_QUIT (SDL_Quit was received).
{
	const WSTRING wstr = g_pTheDB->GetMessageText(dwMessageID);
	return ShowYesNoMessage(wstr.c_str(), dwYesButtonText, dwNoButtonText);
}

//*****************************************************************************
UINT CDrodScreen::ShowYesNoMessage(
//Display a message in a dialog.  Dialog has a yes and no button and execution
//waits for a button to be pushed before returning.
//
//Params:
	const WCHAR* pwczText, //(in)   Message to display.
	const MESSAGE_ID dwYesButtonText, //[default=MID_Yes]
	const MESSAGE_ID dwNoButtonText)  //[default=MID_No]
//
//Returns:
//TAG_YES (user pressed yes button), TAG_NO (user pressed no button),
//or TAG_QUIT (SDL_Quit was received).
{
	const WCHAR *pText;
	UINT wTextW, wTextH;

	//Show just the "Yes" and "No" buttons.
	CButtonWidget *pButton = DYN_CAST(CButtonWidget*, CWidget*, this->pMessageDialog->GetWidget(TAG_OK));
	pButton->Hide(false);

	//Set text on Yes button.
	pButton = DYN_CAST(CButtonWidget*, CWidget*, this->pMessageDialog->GetWidget(TAG_YES));
	pText = g_pTheDB->GetMessageText(dwYesButtonText);
	if (dwYesButtonText == MID_Yes)
	{
		pButton->SetWidth(CX_MESSAGE_BUTTON);
	} else {
		//Resize width to fit text.
		g_pTheFM->GetTextWidthHeight(F_Button, pText, wTextW, wTextH);
		pButton->SetWidth(wTextW + CX_SPACE);
	}
	pButton->SetCaption(pText);
	pButton->SetHotkeyFromText(pText);
	pButton->Show();

	//Set text on No button.
	pButton = DYN_CAST(CButtonWidget*, CWidget*, this->pMessageDialog->GetWidget(TAG_NO));
	pText = g_pTheDB->GetMessageText(dwNoButtonText);
	if (dwNoButtonText == MID_No)
	{
		pButton->SetWidth(CX_MESSAGE_BUTTON);
	} else {
		//Resize width to fit text.
		g_pTheFM->GetTextWidthHeight(F_Button, pText, wTextW, wTextH);
		pButton->SetWidth(wTextW + CX_SPACE);
	}
	pButton->SetCaption(pText);
	pButton->SetHotkeyFromText(pText);
	pButton->Show();

	//Activate the dialog widget.
	return ShowMessage(pwczText);
}

//*****************************************************************************
void CDrodScreen::ShowStatusMessage(
//Display a message in a dialog.  Execution continues.
//Call HideStatusMessage() to make dialog box disappear.
//
//Params:
	const MESSAGE_ID dwMessageID) //(in)   Indicates message to display.
//
//Returns:
//TAG_YES (user pressed yes button), TAG_NO (user pressed no button),
//or TAG_QUIT (SDL_Quit was received).
{
	//Load text for message.
	const WCHAR *pwczMessage = g_pTheDB->GetMessageText(dwMessageID);

	CScreen::ShowStatusMessage(pwczMessage);
}

//*****************************************************************************
UINT CDrodScreen::ShowTextInputMessage(
//Display a message in a dialog, prompting user for input.
//Dialog has an OK and Cancel button and execution
//waits for a button to be pushed before returning.
//
//Params:
	const MESSAGE_ID dwMessageID, //(in)   Indicates message to display.
	WSTRING &wstrUserInput,    //(in/out)  Default text/text entered by user.
	const bool bMultiLineText, //(in)      if true, show 2D text box, else single-line text box
	const bool bMustEnterText) //(in)      if true, OK button is disabled
										//          when text box is empty
										//          (default = true)
{
	//Load text for message.
	const WCHAR *pwczMessage = g_pTheDB->GetMessageText(dwMessageID);
	if (!pwczMessage) pwczMessage = g_pTheDB->GetMessageText(MID_MessageLoadError);

	return CScreen::ShowTextInputMessage(pwczMessage, wstrUserInput, bMultiLineText, bMustEnterText);
}

//*****************************************************************************
void CDrodScreen::ToggleScreenSize()
//Updates player settings in DB, if current player exists.
{
	CScreen::ToggleScreenSize();

	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
	if (pCurrentPlayer)
	{
		pCurrentPlayer->Settings.SetVar(Settings::Fullscreen, IsFullScreen());
		pCurrentPlayer->Update();
		delete pCurrentPlayer;
	}
}

//*****************************************************************************
void CDrodScreen::ExportHoldProgressForUpload(const UINT holdID, const UINT playerID, string& outText)
//Exports a saved game record containing hold upload progress.
{
	CIDSet roomsExplored, exportPlayerIDs(playerID);
	//If player has not played this hold yet, then skip it.
	CDbHolds::GetRoomsExplored(holdID, playerID, roomsExplored);
	if (roomsExplored.empty())
		return; //player has not explored any rooms in the hold

	//Upload progress for this hold.
	CCueEvents Ignored;
	CCurrentGame *pCurrentGame = g_pTheDB->GetNewCurrentGame(holdID, Ignored);
	if (!pCurrentGame)
		return; //couldn't load data for hold

	//Upload a saved game containing all explored rooms for the player.
	pCurrentGame->dwPlayerID = playerID;
	for (CIDSet::const_iterator room=roomsExplored.begin(); room!=roomsExplored.end(); ++room)
		pCurrentGame->AddRoomToMap(*room);
	pCurrentGame->eType = ST_Progress;	//a one-use record type for exporting saved game data
	if (pCurrentGame->Update())	//ensure record was saved to DB for export
	{
		string text;
		if (CDbXML::ExportXML(V_Players, exportPlayerIDs, text, ST_Progress))
			outText += text;
		//Once exported, can remove this record from DB
		g_pTheDB->SavedGames.Delete(pCurrentGame->dwSavedGameID);
	}
	delete pCurrentGame;

	//Upload end hold record, if exists.
	CDb db;
	if (db.SavedGames.FindByEndHold(holdID))
	{
		string endHoldText;
		g_pTheDB->SetHoldID(holdID);
		if (CDbXML::ExportXML(V_Players, exportPlayerIDs, endHoldText, ST_EndHold))
			outText += endHoldText;
	}
}

//
// Private methods
//

//*****************************************************************************
void CDrodScreen::PrepareDatumForExport(
//Loads a file from disk and adds it to the DB for temporary storage before export.
//The dataID of the imported file is added to ids.
//If the file doesn't exist, nothing happens here.
//
//Params:
	const WSTRING& modName,  //name of mod
	const WSTRING& wstrFile, //name of data object
	CIDSet& ids,             //(in/out) record ID of imported data objects
	const UINT dataType)     //enum of media type to import
{
	CScreen::ShowStatusMessage(wstrFile.c_str());

	//Look for file on disk.
	WSTRING wstrFilepath;
	UINT wDataFormat = DATA_UNKNOWN;
	switch (dataType)
	{
		case DATA_IMAGE:
			wDataFormat = g_pTheBM->GetImageFilepath(wstrFile.c_str(), wstrFilepath);
		break;
		case DATA_MUSIC:
			//Assume OGG format.
			wstrFilepath = g_pTheSound->GetSongPath();
			wstrFilepath += wstrFile;
			wstrFilepath += wszDotOgg;
			wDataFormat = DATA_OGG;
		break;
		default: break;
	}

	if (wDataFormat == DATA_UNKNOWN)
		return; //no file on disk was found under this name

	//Temporarily add to DB in order to export the data object.
	//It should be removed from the DB by the caller when done exporting the data.
	CStretchyBuffer data;
	if (CFiles::ReadFileIntoBuffer(wstrFilepath.c_str(), data, true))
	{
		CDbDatum *pMedia = g_pTheDB->Data.GetNew();
		pMedia->wDataFormat = wDataFormat;
		pMedia->DataNameText = wstrFile.c_str();
		pMedia->dwHoldID = 0;
		pMedia->modName = modName.c_str();
		pMedia->data.Set((const BYTE*)data,data.Size());

		if (dataType == DATA_IMAGE)
		{
			//Get optional .tim data.
			WSTRING wstrTimFilepath = g_pTheBM->GetTileImageMapFilepath(wstrFile.c_str());
			CFiles::ReadFileIntoBuffer(wstrTimFilepath.c_str(), pMedia->timData);
		}

		pMedia->Update();
		ASSERT(pMedia->dwDataID);
		ids += pMedia->dwDataID;
		delete pMedia;
	}
}

//*****************************************************************************
UINT CDrodScreen::ShowMessage(
//Display a message in a dialog.  Execution waits for a button to be pushed
//before returning.
//
//Params:
	const MESSAGE_ID dwMessageID) //(in)   Indicates message to display.
//
//Returns:
//TAG_YES (user pressed yes button), TAG_NO (user pressed no button),
//or TAG_QUIT (SDL_Quit was received).
{
	//Load text for message.
	const WCHAR *pwczMessage = g_pTheDB->GetMessageText(dwMessageID);

	return CScreen::ShowMessage(pwczMessage);
}

//*****************************************************************************
UINT CDrodScreen::ShowMessage(
//Display a message in a dialog.  Execution waits for a button to be pushed
//before returning.
//
//Params:
	const WCHAR* pwczText) //(in)   Message to display.
//
//Returns:
//TAG_YES (user pressed yes button), TAG_NO (user pressed no button),
//or TAG_QUIT (SDL_Quit was received).
{
	return CScreen::ShowMessage(pwczText);
}

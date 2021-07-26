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
#include "CheckpointEffect.h"
#include "DebrisEffect.h"
#include "ExplosionEffect.h"
#include "EvilEyeGazeEffect.h"
#include "FiretrapEffect.h"
#include "IceMeltEffect.h"
#include "ImageOverlayEffect.h"
#include "PuffExplosionEffect.h"
#include "RoomWidget.h"
#include "SparkEffect.h"
#include "SpikeEffect.h"
#include "SplashEffect.h"
#include "SteamEffect.h"
#include "StunEffect.h"
#include "SwordSwingEffect.h"
#include "SwordsmanSwirlEffect.h"
#include "TarStabEffect.h"
#include "TrapdoorFallEffect.h"
#include "VerminEffect.h"
#include "WadeEffect.h"

#include "../DRODLib/CurrentGame.h"
#include "../DRODLib/Db.h"
#include "../DRODLib/DbPlayers.h"
#include "../DRODLib/DbXML.h"
#include "../DRODLib/GameConstants.h"
#include "../DRODLib/SettingsKeys.h"

#include <FrontEndLib/ButtonWidget.h>
#include <FrontEndLib/FlashMessageEffect.h>
#include <FrontEndLib/FrameWidget.h>
#include <FrontEndLib/LabelWidget.h>
#include <FrontEndLib/ListBoxWidget.h>
#include <FrontEndLib/ProgressBarWidget.h>
#include <FrontEndLib/TextBoxWidget.h>
#include <FrontEndLib/TheoraPlayer.h>
#include <BackEndLib/Base64.h>
#include <BackEndLib/Browser.h>
#include <BackEndLib/Files.h>
#include <BackEndLib/Metadata.h>
#include <BackEndLib/Ports.h>

#include <sstream>

vector<WSTRING> CDrodScreen::importFiles;
CNetChat::Interface CDrodScreen::chat;

const SDL_Color FriendColor = {0, 0, 128, 0};
const SDL_Color FriendAFKColor = {64, 64, 128, 0};
const SDL_Color AFKColor = {64, 64, 64, 0}; //darker than ignored Gray-128

const WCHAR wszDotOgg[] = { We('.'),We('o'),We('g'),We('g'),We(0) };

//For exporting style mods.
#define DATA_IMAGE (0)
#define DATA_MUSIC (1)

const UINT UNDO_SETTING_LEVELS[UNDO_LEVEL_INCREMENTS] = {0, 1, 2, 3, 5, 8, 30, INT_MAX};

//*****************************************************************************
//Message dialog coords and dimensions (copied from CScreen).
const UINT CX_MESSAGE = 650;
const UINT CY_MESSAGE = 200;
const UINT CX_MESSAGE_BUTTON = 80;
const UINT CX_SPACE = 8;
const UINT CY_SPACE = 8;
const UINT CY_MESSAGE_BUTTON = CY_STANDARD_BUTTON;

const UINT CX_TEXT = CX_MESSAGE - (CX_SPACE * 2);
const UINT CY_TEXT = CY_MESSAGE - (CY_SPACE * 3) - CY_MESSAGE_BUTTON;
const int X_TEXT = CX_SPACE;
const int Y_TEXT = CY_SPACE;
const int FRAME_BUFFER = 5;

const int X_CLOUD_OK = (CX_MESSAGE - CX_SPACE) / 2 - CX_MESSAGE_BUTTON;
const int X_CLOUD_CANCEL = (CX_MESSAGE + CX_SPACE) / 2;

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
	, pGetCloudPlayerDialog(NULL)
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

//*****************************************************************************
void CDrodScreen::PublicHideProgressWidget() {this->pProgressWidget->Hide();}

//
// Protected methods
//

//*****************************************************************************
void CDrodScreen::AddCloudDialog()
{
	ASSERT(!this->pGetCloudPlayerDialog);
	this->pGetCloudPlayerDialog = new CDialogWidget(0L, 0, 0, CX_MESSAGE, CY_MESSAGE + CY_SPACE);
	this->pGetCloudPlayerDialog->Hide();

	static const UINT CY_FRAME = CY_TEXT;
	CFrameWidget *pFrame = new CFrameWidget(TAG_FRAME,
			X_TEXT, Y_TEXT, CX_TEXT, CY_FRAME, NULL);
	this->pGetCloudPlayerDialog->AddWidget(pFrame);

	static const int Y_LABEL = FRAME_BUFFER;
	static const UINT CY_LABEL = 40;
	CLabelWidget *pLabel = new CLabelWidget(TAG_TEXT, FRAME_BUFFER, Y_LABEL,
			CX_TEXT - 2*FRAME_BUFFER, CY_LABEL, FONTLIB::F_Message, wszEmpty);
	pLabel->SetAlign(CLabelWidget::TA_CenterGroup);
	pFrame->AddWidget(pLabel);

	static const int X_CNETNAME_LABEL = CX_SPACE;
	static const int Y_CNETNAME_LABEL = Y_LABEL + CY_LABEL + CY_SPACE;
	static const UINT CX_CNETNAME_LABEL = 170;
	static const UINT CY_CNETNAME_LABEL = CY_LABEL;
	static const int X_CNETNAME = X_CNETNAME_LABEL + CX_CNETNAME_LABEL + CX_SPACE;
	static const int Y_CNETNAME = Y_CNETNAME_LABEL;
	const UINT CX_CNETNAME = CX_TEXT - X_CNETNAME - CX_SPACE;
	static const UINT CY_CNETNAME = CY_STANDARD_TBOX;

	static const int X_CNETPASSWORD_LABEL = CX_SPACE;
	static const int Y_CNETPASSWORD_LABEL = Y_CNETNAME_LABEL + CY_CNETNAME + CY_SPACE;
	static const UINT CX_CNETPASSWORD_LABEL = CX_CNETNAME_LABEL;
	static const UINT CY_CNETPASSWORD_LABEL = CY_CNETNAME_LABEL;
	static const int X_CNETPASSWORD = X_CNETPASSWORD_LABEL + CX_CNETPASSWORD_LABEL + CX_SPACE;
	static const int Y_CNETPASSWORD = Y_CNETPASSWORD_LABEL;
	const UINT CX_CNETPASSWORD = CX_TEXT - X_CNETPASSWORD - CX_SPACE;
	static const UINT CY_CNETPASSWORD = CY_STANDARD_TBOX;

	pFrame->AddWidget(
		new CLabelWidget(0L, X_CNETNAME_LABEL, Y_CNETNAME_LABEL,
		CX_CNETNAME_LABEL, CY_CNETNAME_LABEL, F_FrameCaption, g_pTheDB->GetMessageText(MID_CaravelNetName)) );
	pFrame->AddWidget(new CTextBoxWidget(TAG_CNETNAME,
			X_CNETNAME, Y_CNETNAME, CX_CNETNAME, CY_CNETNAME, 30));

	pFrame->AddWidget(
		new CLabelWidget(0L, X_CNETPASSWORD_LABEL, Y_CNETPASSWORD_LABEL,
		CX_CNETPASSWORD_LABEL, CY_CNETPASSWORD_LABEL, F_FrameCaption, g_pTheDB->GetMessageText(MID_CaravelNetKey)) );
	CTextBoxWidget *pCaravelNetPasswordWidget = new CTextBoxWidget(TAG_CNETPASSWORD,
				X_CNETPASSWORD, Y_CNETPASSWORD, CX_CNETPASSWORD, CY_CNETPASSWORD, 33);
#ifndef ENABLE_CHEATS
	pCaravelNetPasswordWidget->SetViewAsPassword(true);
#endif
	pFrame->AddWidget(pCaravelNetPasswordWidget);

	this->pGetCloudPlayerDialog->AddWidget(new CButtonWidget(TAG_OK,
			X_CLOUD_OK, Y_MESSAGE_BUTTON, CX_MESSAGE_BUTTON, CY_MESSAGE_BUTTON,
			g_pTheDB->GetMessageText(MID_Okay)));
	this->pGetCloudPlayerDialog->AddWidget(new CButtonWidget(TAG_CANCEL_,
			X_CLOUD_CANCEL, Y_MESSAGE_BUTTON, CX_MESSAGE_BUTTON, CY_MESSAGE_BUTTON,
			g_pTheDB->GetMessageText(MID_CancelNoHotkey)));

	AddWidget(this->pGetCloudPlayerDialog);
	this->pGetCloudPlayerDialog->Center();
}

//*****************************************************************************
void CDrodScreen::AddDamageEffect(CRoomWidget* pRoomWidget, const CCurrentGame* pGame,
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
		case M_CONSTRUCT: //!!FIXME: needs own effect
			pRoomWidget->AddTLayerEffect(
				new CGolemDebrisEffect(pRoomWidget, coord, 10,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
		break;
		case M_SLAYER:
		case M_SLAYER2:
			pRoomWidget->AddTLayerEffect(
				new CVerminEffect(pRoomWidget, coord, 40, true));
		break;
		default:
			pRoomWidget->AddTLayerEffect(
				new CBloodEffect(pRoomWidget, coord, 16,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
		break;
	}
}

//*****************************************************************************
void CDrodScreen::AddVisualEffect(const VisualEffectInfo* pEffect, CRoomWidget* pRoomWidget, const CCurrentGame* pGame)
//Add a visual effect to the room.
//wO: direction, wValue: effect type
{
	const CMoveCoordEx2& effect = pEffect->effect;
	const CCoord& src = pEffect->source;
	CMoveCoord coord(effect.wX, effect.wY, effect.wO);
	UINT particles = 5; //can be increased by triggering multiple effects

	switch (effect.wValue)
	{
		case VET_BLOODSPLAT:
			pRoomWidget->AddTLayerEffect(
				new CBloodEffect(pRoomWidget, coord, particles,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 3)));
		break;
		case VET_MUDSPLAT:
			pRoomWidget->AddTLayerEffect(
				new CMudStabEffect(pRoomWidget, coord,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 3)));
		break;
		case VET_TARSPLAT:
			pRoomWidget->AddTLayerEffect(
				new CTarStabEffect(pRoomWidget, coord,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 3)));
		break;
		case VET_GELSPLAT:
			pRoomWidget->AddTLayerEffect(
				new CGelStabEffect(pRoomWidget, coord,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 3)));
		break;
		case VET_SEEPSPLAT:
			pRoomWidget->AddTLayerEffect(
				new CBloodInWallEffect(pRoomWidget, coord, particles));
		break;
		case VET_GOLEMSPLAT:
			pRoomWidget->AddTLayerEffect(
				new CGolemDebrisEffect(pRoomWidget, coord, particles,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 3)));
		break;
		case VET_SLAYERSPLAT:
			pRoomWidget->AddTLayerEffect(
				new CVerminEffect(pRoomWidget, coord, particles, true));
		break;
		case VET_DEBRIS:
			pRoomWidget->AddTLayerEffect(
				new CDebrisEffect(pRoomWidget, coord, particles,
						GetEffectDuration(pGame, 7), GetParticleSpeed(pGame, 4)));
		break;
		case VET_SPARKS:
			pRoomWidget->AddTLayerEffect(
					new CSparkEffect(pRoomWidget, coord, particles, false));
		break;
		case VET_EXPLOSION:
			pRoomWidget->AddMLayerEffect(
				new CExplosionEffect(pRoomWidget, coord,
						GetEffectDuration(pGame, 500)));
		break;
		case VET_SPLASH:
			pRoomWidget->AddTLayerEffect(
					new CSplashEffect(pRoomWidget, coord));
		break;
		case VET_STEAM:
			pRoomWidget->AddTLayerEffect(
					new CSteamEffect(pRoomWidget, coord));
		break;
		case VET_SWIRL:
			pRoomWidget->AddMLayerEffect(
				new CSwordsmanSwirlEffect(pRoomWidget, pGame));
		break;
		case VET_VERMIN:
			pRoomWidget->AddTLayerEffect(
				new CVerminEffect(pRoomWidget, coord, particles));
		break;
		case VET_BOLT:
		{
			static COrbData orbData;
			orbData.wX = src.wX;
			orbData.wY = src.wY;
			orbData.AddAgent(new COrbAgentData(effect.wX, effect.wY, OA_NULL));

			pRoomWidget->AddStrikeOrbEffect(orbData, false);

			orbData.ClearAgents();
		}
		break;
		case VET_JITTER:
			pRoomWidget->AddJitter(coord, 0.25f); //25%
		break;
		case VET_PUFFEXPLOSION:
			pRoomWidget->AddMLayerEffect(
					new CPuffExplosionEffect(pRoomWidget, coord));
		break;
		case VET_SPIKES:
			pRoomWidget->AddOLayerEffect(
					new CSpikeEffect(pRoomWidget, coord));
		break;
		case VET_FIRETRAP:
			pRoomWidget->AddMLayerEffect(
					new CFiretrapEffect(pRoomWidget, coord));
		break;
		case VET_ICEMELT:
			pRoomWidget->AddOLayerEffect(
					new CIceMeltEffect(pRoomWidget, coord));
		break;
		case VET_PUFFSPLAT:
			pRoomWidget->AddMLayerEffect(
				new CFluffStabEffect(pRoomWidget, coord,
					GetEffectDuration(pGame, 6), GetParticleSpeed(pGame, 2)));
		break;
		default: break; //do nothing
	}
}

//*****************************************************************************
void CDrodScreen::AddVisualCues(CCueEvents& CueEvents, CRoomWidget* pRoomWidget, const CCurrentGame* pGame)
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

	//Scripted effects.
	for (pObj = CueEvents.GetFirstPrivateData(CID_VisualEffect);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const VisualEffectInfo *pEffect = DYN_CAST(const VisualEffectInfo*, const CAttachableObject*, pObj);
		AddVisualEffect(pEffect, pRoomWidget, pGame);
	}

	if (CueEvents.HasOccurred(CID_SetDisplayFilter))
		pRoomWidget->RerenderRoom();

	if (CueEvents.HasOccurred(CID_DoublePlaced))
		pRoomWidget->RenderRoomInPlay(); //remove double placement effect

	if (CueEvents.HasOccurred(CID_SwingSword))
	{
		const CCoord *pOrientation = DYN_CAST(const CCoord*, const CAttachableObject*,
				CueEvents.GetFirstPrivateData(CID_SwingSword));
		pRoomWidget->AddMLayerEffect(
				new CSwordSwingEffect(pRoomWidget,
					pGame->swordsman, pOrientation->wX,
					pGame->swordsman.GetActiveWeapon()
		));
	}

	if (CueEvents.HasOccurred(CID_CheckpointActivated) &&
			pRoomWidget->AreCheckpointsVisible())
	{
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*,
			CueEvents.GetFirstPrivateData(CID_CheckpointActivated) );
		pRoomWidget->AddTLayerEffect(
				new CCheckpointEffect(pRoomWidget, *pCoord));
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
	for (pObj = CueEvents.GetFirstPrivateData(CID_BombExploded);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
		CMoveCoord moveCoord(pCoord->wX, pCoord->wY, NO_ORIENTATION);
		pRoomWidget->AddTLayerEffect(
				new CDebrisEffect(pRoomWidget, moveCoord, 3,
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
						pGame->swordsman.bIsHasted ? 260 : 130));
		} else {
			pRoomWidget->AddTLayerEffect(
					new CSplashEffect(pRoomWidget, *pCoord));
		}
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_ThinIceMelted);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
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
		typedef map<ROOMCOORD, vector<UINT> > TilesMap;
		TilesMap fallingTiles;
		pObj = CueEvents.GetFirstPrivateData(CID_ObjectFell);
		while (pObj)
		{
			//Show object as it falls.
			const CMoveCoordEx2 *pCoord = DYN_CAST(const CMoveCoordEx2*, const CAttachableObject*, pObj);

			UINT wTileNo;
			if (pCoord->wValue >= M_OFFSET)
			{
				if (pCoord->wValue == T_GENTRYII && pCoord->wO >= ORIENTATION_COUNT) {
					const UINT wO = pCoord->wO - ORIENTATION_COUNT;
					wTileNo = GetTileImageForGentryiiPiece(0, 0, nGetOX(wO), nGetOY(wO));
				} else {
					const UINT eLogicalType = pCoord->wValue - M_OFFSET;
					UINT eMonsterType = eLogicalType; //same by default

					//Look up base type of custom character.
					if (eMonsterType >= CUSTOM_CHARACTER_FIRST)
					{
						const HoldCharacter *pChar = pGame->pHold->GetCharacter(eLogicalType);
						if (pChar)
							eMonsterType = pChar->wType;
						else
							eMonsterType = M_CITIZEN1;
					}

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
					pGame->swordsman.bIsHasted ? 260 : 130));
		}
	}

	for (pObj = CueEvents.GetFirstPrivateData(CID_MonsterDiedFromStab);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMonster *pMonster = DYN_CAST(const CMonster*, const CAttachableObject*, pObj);
		const UINT wO = IsValidOrientation(pMonster->wKillInfo) ?
				pMonster->wKillInfo : NO_ORIENTATION;
		CMoveCoord coord(pMonster->wX,pMonster->wY,wO);
		AddDamageEffect(pRoomWidget, pGame, pMonster->GetIdentity(), coord);
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_Stun);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CStunTarget *pStunTarget = DYN_CAST(const CStunTarget*, const CAttachableObject*, pObj);

		if (pStunTarget->bIsPlayerStunned)
			pRoomWidget->AddMLayerEffect(new CStunEffect(pRoomWidget, pGame->swordsman.wX, pGame->swordsman.wY, pStunTarget->stunDuration));
		else if (pStunTarget->pStunnedMonster && pStunTarget->pStunnedMonster->IsAlive())
			pRoomWidget->AddMLayerEffect(new CStunEffect(
				pRoomWidget,
				pStunTarget->pStunnedMonster->wX,
				pStunTarget->pStunnedMonster->wY,
				pStunTarget->stunDuration
			));
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
		CMoveCoord coord(pMonster->wX,pMonster->wY,pMonster->wO);
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
	for (pObj = CueEvents.GetFirstPrivateData(CID_FluffDestroyed);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoordEx *pCoord = DYN_CAST(const CMoveCoordEx*, const CAttachableObject*, pObj);
		switch (pCoord->wValue)
		{
			case T_FLUFF:
				if (bIsSolidOTile(pGame->pRoom->GetOSquare(pCoord->wX, pCoord->wY)))
					pRoomWidget->AddTLayerEffect(
							new CFluffInWallEffect(pRoomWidget, *pCoord));
				else
					pRoomWidget->AddTLayerEffect(
							new CFluffStabEffect(pRoomWidget, *pCoord,
									GetEffectDuration(pGame, 6), GetParticleSpeed(pGame, 2)));
			break;
		}
	}

	if (CueEvents.HasOccurred(CID_SpikesUp)) {
		pRoomWidget->RemoveTLayerEffectsOfType(ESPIKES);
		const UINT duration = GetEffectDuration(pGame, 300);
		for (pObj = CueEvents.GetFirstPrivateData(CID_SpikesUp);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
		{
			const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
			pRoomWidget->AddTLayerEffect(
				new CSpikeEffect(pRoomWidget, *pCoord, GetEffectDuration(pGame, duration)));
		}
	}

	if (CueEvents.HasOccurred(CID_Firetrap)) {
		pRoomWidget->RemoveMLayerEffectsOfType(EFIRETRAP);
		const UINT duration = GetEffectDuration(pGame, 450);
		for (pObj = CueEvents.GetFirstPrivateData(CID_Firetrap);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
		{
			const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
			pRoomWidget->AddMLayerEffect(
				new CFiretrapEffect(pRoomWidget, *pCoord, GetEffectDuration(pGame, duration)));
		}
	}

	//Remove old sparks before drawing the current ones.
	if (//Leave sparks burning while double is being placed.
		(!pGame->swordsman.wPlacingDoubleType ||
				CueEvents.HasOccurred(CID_DrankPotion)) &&
				!CueEvents.HasOccurred(CID_DoublePlaced))
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

	for (pObj = CueEvents.GetFirstPrivateData(CID_EvilEyeWoke);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		pRoomWidget->AddMLayerEffect(new CEvilEyeGazeEffect(
				pRoomWidget,pCoord->wX,pCoord->wY,pCoord->wO, 500));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_MonsterBurned);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pMoveCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		pRoomWidget->AddMLayerEffect(
				new CSteamEffect(pRoomWidget, *pMoveCoord));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_FluffPuffDestroyed);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
		pRoomWidget->AddMLayerEffect(
				new CPuffExplosionEffect(pRoomWidget, *pCoord));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_PuffMergedIntoFluff);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMoveCoord *pMoveCoord = DYN_CAST(const CMoveCoord*, const CAttachableObject*, pObj);
		pRoomWidget->AddMLayerEffect(
			new CFluffStabEffect(pRoomWidget, *pMoveCoord,
					GetEffectDuration(pGame, 6), GetParticleSpeed(pGame, 1)));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_FegundoToAsh);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMonster *pMonster = DYN_CAST(const CMonster*, const CAttachableObject*, pObj);
		CMoveCoord coord(pMonster->wX,pMonster->wY,NO_ORIENTATION);
		pRoomWidget->AddMLayerEffect(
				new CSteamEffect(pRoomWidget, coord));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_Splash);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
		const UINT oTile = pGame->pRoom->GetOSquare(pCoord->wX, pCoord->wY);
		if (bIsWater(oTile) || bIsSteppingStone(oTile))
			pRoomWidget->AddTLayerEffect(
					new CSplashEffect(pRoomWidget, *pCoord));
	}
	for (pObj = CueEvents.GetFirstPrivateData(CID_Wade);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CCoord *pCoord = DYN_CAST(const CCoord*, const CAttachableObject*, pObj);
		const UINT oTile = pGame->pRoom->GetOSquare(pCoord->wX, pCoord->wY);
		if (bIsWater(oTile))
			pRoomWidget->AddTLayerEffect(
					new CWadeEffect(pRoomWidget, *pCoord));
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
	for (pObj = CueEvents.GetFirstPrivateData(CID_AumtlichGaze);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		const CMonster *pMonster = DYN_CAST(const CMonster*, const CAttachableObject*, pObj);
		pRoomWidget->AddAumtlichGazeEffect(pMonster);
	}

	for (pObj = CueEvents.GetFirstPrivateData(CID_FlashingMessage);
			pObj != NULL; pObj = CueEvents.GetNextPrivateData())
	{
		if (yFlashingTextOffset > Y_FLASHING_TEXT_MAX)
			break; //no room to display

		const CColorText *pColorText = DYN_CAST(const CColorText*, const CAttachableObject*, pObj);
		const CDbMessageText *pText = pColorText->pText;
		ASSERT((const WCHAR*)(*pText));
		CFlashMessageEffect *pFlashText = new CFlashMessageEffect(
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
		const int nCommand = static_cast<int>(commandIter->bytCommand);
		if (bIsComplexCommand(nCommand))	//handle two-part commands here
			pGame->Commands.GetData(wX, wY);

		CCueEvents CueEvents;
		pGame->Commands.Freeze();
		pGame->ProcessCommand(nCommand, CueEvents, wX, wY);
		pGame->Commands.Unfreeze();

		//Get next turn ready.
		if (nCommand == CMD_CLONE) {
			//rewinding to a temporal split point will desync the commands iterator
			//so here we need to re-sync it
			commandIter = pGame->Commands.Get(pGame->wTurnNo);
			pRoomWidget->SyncRoomPointerToGame(pGame);

			if (CueEvents.HasOccurred(CID_ActivatedTemporalSplit))
				pRoomWidget->DirtyRoom();
		} else {
			commandIter = pGame->Commands.GetNext();
		}

		//If player died (for good) or exited level, stop replay.
		if ((CueEvents.HasAnyOccurred(IDCOUNT(CIDA_PlayerDied), CIDA_PlayerDied) ||
				CueEvents.HasOccurred(CID_WinGame) ||
				CueEvents.HasOccurred(CID_ExitLevelPending)))
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
void CDrodScreen::BrowseForRoomHints(CDbRoom* pRoom)
{
	SetFullScreen(false);

	int dX, dY;
	pRoom->GetPositionInLevel(dX, dY);

	CDbLevel *pLevel = pRoom->GetLevel();
	if (!pLevel) return;
	CDbHold *pHold = pLevel->GetHold();
	if (!pHold) {
		delete pLevel;
		return;
	}

	std::ostringstream url;
	url << CNetInterface::cNetBaseURL << "gamehints.php?"
		<< "hc=" << long(time_t(pHold->GetCreated()))
		<< "&hu=" << long(time_t(pHold->LastUpdated))
		<< "&l=" << pLevel->dwLevelIndex
		<< "&rx=" << dX
		<< "&ry=" << dY;

	delete pLevel;
	delete pHold;

	OpenExtBrowser(url.str().c_str());
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

	const CAttachableObject *pObj = CueEvents.GetFirstPrivateData(cid);
	while (pObj)
	{
		const CImageOverlay *pImageOverlay = DYN_CAST(const CImageOverlay*, const CAttachableObject*, pObj);

		//Don't wait for additional images to be added to the room widget before clearing effects.
		const int clearsLayer = pImageOverlay->clearsImageOverlays();
		const int clearsGroup = pImageOverlay->clearsImageOverlayGroup();
		if (clearsLayer == ImageOverlayCommand::NO_LAYERS &&
				clearsGroup == ImageOverlayCommand::NO_GROUP) {
			CImageOverlayEffect *pEffect = new CImageOverlayEffect(pRoomWidget, pImageOverlay, currentTurn, dwNow);
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
UINT CDrodScreen::GetEffectDuration(const CCurrentGame* pGame, const UINT baseDuration)
//Returns: duration of particles in particle effects
{
	//When player is hasted, particles move at half speed, so they last twice as long
	return pGame && pGame->swordsman.bIsHasted ?
			baseDuration*2 : baseDuration;
}

//*****************************************************************************
UINT CDrodScreen::GetParticleSpeed(const CCurrentGame* pGame, const UINT baseSpeed)
//Returns: speed of particles in particle effects
{
	//When player is hasted, particles move at half speed
	return pGame && pGame->swordsman.bIsHasted ?
			(baseSpeed > 1 ? baseSpeed/2 : 1) : baseSpeed;
}

//******************************************************************************
CDbHold::HoldStatus CDrodScreen::GetHoldStatus()
{
	return CDbHolds::GetStatus(g_pTheDB->GetHoldID());
}

//*****************************************************************************
WSTRING CDrodScreen::getStatsText(
//Print stats as text.
//
//Params:
	const RoomStats& st)
{
	WSTRING wstr;
	WCHAR num[16];

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

	if (st.yellowDoors)
	{
		wstr += wszCRLF;
		wstr += g_pTheDB->GetMessageText(MID_YellowDoor);
		wstr += wszColon;
		wstr += wszSpace;
		wstr += _itoW(st.yellowDoors, num, 10);
	}
	if (st.greenDoors)
	{
		wstr += wszCRLF;
		wstr += g_pTheDB->GetMessageText(MID_GreenDoor);
		wstr += wszColon;
		wstr += wszSpace;
		wstr += _itoW(st.greenDoors, num, 10);
	}
	if (st.blueDoors)
	{
		wstr += wszCRLF;
		wstr += g_pTheDB->GetMessageText(MID_BlueDoor);
		wstr += wszColon;
		wstr += wszSpace;
		wstr += _itoW(st.blueDoors, num, 10);
	}

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
		SETLOGERRORS(atoi(str.c_str()) != 0);
}

//*****************************************************************************
void CDrodScreen::Callback(long val)
//Status messages during import.
{
	if (!val) return;
	this->pStatusDialog->Hide();
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
	if (!wpText) return;
	this->pStatusDialog->Hide();
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
void CDrodScreen::ExportCleanup()
{
	CDbXML::SetCallback(NULL);
	this->pProgressWidget->Hide();
	HideStatusMessage();
	SetCursor();
}

//*****************************************************************************
void CDrodScreen::GenericNetTransactionWait(const UINT handle, MESSAGE_ID mid,
	const float progressStart, const float progressEnd)
{
	if (!handle)
		return;

	SetCursor(CUR_Internet);
	Callback(mid);
	CDbXML::SetCallback(this);

	float fFakeProgress = progressStart;
	while (g_pTheNet->GetStatus(handle) < CURLE_OK) {
		Callbackf(fFakeProgress); //event handler/keep-active
		SDL_Delay(20);
		if (fFakeProgress < progressEnd)
			fFakeProgress += 0.001f;
		else
			fFakeProgress = progressEnd;
	}

	ExportCleanup();
}

//*****************************************************************************
void CDrodScreen::CloudTransactionWait(MESSAGE_ID mid, const float progressStart,
	const float progressEnd)
{
	SetCursor(CUR_Internet);
	Callback(mid);
	CDbXML::SetCallback(this);

	float fFakeProgress = progressStart;
	while (g_pTheNet->CloudQueueSize() > 0) {
		Callbackf(fFakeProgress); //event handler/keep-active
		SDL_Delay(20);
		if (fFakeProgress < progressEnd)
			fFakeProgress += 0.001f;
		else
			fFakeProgress = progressEnd;
	}

	ExportCleanup();
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
	CDbPlayer *pCurrentPlayer = g_pTheDB->GetCurrentPlayer();
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
		} else {
			const UINT wDataFormat = g_pTheBM->GetImageType(buffer);
			if (wDataFormat == DATA_UNKNOWN) {
				ShowOkMessage(MID_FileCorrupted);
			} else {
				CDbDatum *pImage = g_pTheDB->Data.GetNew();
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
	const string str = UnicodeToUTF8(pText+index);
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
UINT CDrodScreen::ShowGetCloudPlayerDialog()
{
	//Get authentication info.
	WSTRING username, userpassword;
	bool bCloudActivated = false;

	CDbPlayer *pPlayer = g_pTheDB->GetCurrentPlayer();
	if (pPlayer) {
		username = pPlayer->CNetNameText;
		userpassword = pPlayer->CNetPasswordText;
		bCloudActivated = pPlayer->Settings.GetVar(Settings::CloudActivated, false);
		delete pPlayer;
	}

	if (!bCloudActivated)
	{
		ShowOkMessage(MID_CaravelNetCloudExplanation);

		const WCHAR *pwczMessage = g_pTheDB->GetMessageText(MID_CaravelNetAuthenticationPrompt);
		ASSERT(pwczMessage);

		ASSERT(this->pGetCloudPlayerDialog);
		CLabelWidget *pText = DYN_CAST(CLabelWidget*, CWidget*,
				this->pGetCloudPlayerDialog->GetWidget(TAG_TEXT));
		pText->SetText(pwczMessage);

		//Get credentials if player is not already logged in to CaravelNet.
		vector<CNetMedia*>& cNetMedia = g_pTheNet->GetCNetMedia();
		if (cNetMedia.empty()) {
			const UINT result = this->pGetCloudPlayerDialog->Display();
			Paint();

			if (result != TAG_OK)
				return 0;

			CTextBoxWidget *pTextBox = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pGetCloudPlayerDialog->GetWidget(TAG_CNETNAME));
			username = pTextBox->GetText();
			pTextBox = DYN_CAST(CTextBoxWidget*, CWidget*,
					this->pGetCloudPlayerDialog->GetWidget(TAG_CNETPASSWORD));
			userpassword = pTextBox->GetText();
		}
	}

	ShowStatusMessage(MID_CloudSynchInProgress);
	SetCursor(CUR_Internet);

	//Create temp player profile to faciliate sync operation.
	pPlayer = g_pTheDB->Players.GetNew();
	pPlayer->NameText = username.c_str();
	pPlayer->CNetNameText = username.c_str();
	pPlayer->CNetPasswordText = userpassword.c_str();
	pPlayer->Update();
	const UINT tempPlayerID = pPlayer->dwPlayerID;
	delete pPlayer;

	g_pTheDB->SetPlayerID(tempPlayerID);

	UINT handle = g_pTheNet->CloudGetPlayer(username.c_str(), userpassword.c_str());
	if (!handle) {
		SetCursor();
		ShowOkMessage(MID_CaravelNetAuthError);
		Paint();
		g_pTheDB->Players.Delete(tempPlayerID, false);
		g_pTheDB->SetPlayerID(0);
		return 0;
	}

	GenericNetTransactionWait(handle, MID_CloudSynchInProgress, 0.0f, 0.01f);

	bool bSuccess = false;
	UINT newPlayerID = 0;

	if (g_pTheNet->GetStatus(handle) != CURLE_OK) {
		// Failed to get a response, etc.
		ShowOkMessage(MID_CaravelNetConnectionError);
		goto FinishSync;
	}

	{
	CNetResult *pResult = g_pTheNet->GetResults(handle);
	if (pResult)
	{
		if (pResult->pJson && pResult->pJson->isMember("player")) {
			const string playerXml = pResult->pJson->get("player", "").asString();
			if (playerXml.empty()) {
				ShowOkMessage(MID_CloudNoPlayerAvailable);
				delete pResult;
				goto FinishSync;
			}

			const MESSAGE_ID result = CDbXML::ImportXMLRaw(playerXml, CImportInfo::Player);
			if (result) {
				if (CDbXML::WasImportSuccessful()) {
					if (result == MID_ImportSuccessful) {
						newPlayerID = CDbXML::info.dwPlayerImportedID;
						ASSERT(newPlayerID);
						CDbPlayer *pPlayer = g_pTheDB->Players.GetByID(newPlayerID);
						if (pPlayer) {
							pPlayer->CNetNameText = username.c_str();
							pPlayer->CNetPasswordText = userpassword.c_str();
							pPlayer->Settings.SetVar("ConnectToInternet", true);
							pPlayer->Update();
							delete pPlayer;

							g_pTheDB->SetPlayerID(newPlayerID);
						}
					}
					//ignore MID_OverwritePlayerPrompt -- it indicates the player profile is already present locally
				} else {
					ShowOkMessage(MID_CloudSynchError);
					delete pResult;
					goto FinishSync;
				}
			}
			CDrodScreen::Callbackf(0.1f);

			if (pResult->pJson->isMember("holds")) {
				const Json::Value& holds = (*(pResult->pJson))["holds"];
				float fStartProgress = 0.1f;
				const float perHoldProgressIncrement = (1.0f - fStartProgress) / (holds.size() ? holds.size() : 1.0f);
				for (UINT x = 0; x < holds.size(); ++x, fStartProgress += perHoldProgressIncrement) {
					CDbMessageText author, holdName;
					const CDate date(holds[x].get("created", 0).asInt());
					WSTRING wstr;
					Base64::decode(holds[x].get("architect", "").asString(), wstr);
					author = wstr.c_str();

					Base64::decode(holds[x].get("name", "").asString(), wstr);
					holdName = wstr.c_str();

					const UINT holdID = g_pTheDB->Holds.GetHoldID(date, holdName, author);
					if (!holdID) {
						// Don't have it installed.  Install from cloud.
						UINT holdHandle = g_pTheNet->CloudDownloadGeneralHold(holds[x]);
						GenericNetTransactionWait(holdHandle, MID_CloudSynchInProgress,
							fStartProgress, fStartProgress + perHoldProgressIncrement);

						if (g_pTheNet->GetStatus(holdHandle) == CURLE_OK) {
							CNetResult* pHoldResult = g_pTheNet->GetResults(holdHandle);
							if (pHoldResult) {
								if (pHoldResult->pJson && pHoldResult->pJson->isMember("hold")) {
									//Import hold.
									//Unconvert from base-64, and decompress.
									const string holdXmlB64 = pHoldResult->pJson->get("hold", "").asString();
									string compressedXML;
									Base64::decode(holdXmlB64, compressedXML);
									const MESSAGE_ID result = CDbXML::ImportXMLRaw(compressedXML, CImportInfo::Hold, true);
									//??? if (!CDbXML::WasImportSuccessful()) { error message }
								}

								delete pHoldResult;
							}
						}
					}
				}
			}
			if (pResult->pJson->isMember("demos")) {
				const Json::Value& demos = (*(pResult->pJson))["demos"];
				float fStartProgress = 0.1f;
				const float perHoldProgressIncrement = (1.0f - fStartProgress) / (demos.size() ? demos.size() : 1.0f);
				for (UINT x = 0; x < demos.size(); ++x, fStartProgress += perHoldProgressIncrement) {
					CDbMessageText author, holdName;
					const CDate date(demos[x].get("created", 0).asInt());
					WSTRING wstr;
					Base64::decode(demos[x].get("architect", "").asString(), wstr);
					author = wstr.c_str();

					Base64::decode(demos[x].get("name", "").asString(), wstr);
					holdName = wstr.c_str();

					const UINT holdID = g_pTheDB->Holds.GetHoldID(date, holdName, author);
					if (holdID) {
						const UINT progressHandle = g_pTheNet->CloudDownloadProgress(holdID);
						GenericNetTransactionWait(progressHandle, MID_CloudSynchInProgress,
							fStartProgress, fStartProgress + perHoldProgressIncrement);
						if (g_pTheNet->GetStatus(progressHandle) == CURLE_OK) {
							CNetResult* pDemos = g_pTheNet->GetResults(progressHandle);
							if (pDemos) {
								if (pDemos->pJson && pDemos->pJson->isMember("data")) {
									const string demoXmlB64 = pDemos->pJson->get("data", "").asString();
									string compressedXml;
									Base64::decode(demoXmlB64, compressedXml);
									const MESSAGE_ID result = CDbXML::ImportXMLRaw(compressedXml, CImportInfo::DemosAndSavedGames, true);
								}
								delete pDemos;
							}
						}
					}
				}
			}
		}
		delete pResult;
		bSuccess = true;
	}
	}

FinishSync:
	ExportCleanup();
	Paint();

	ShowOkMessage(bSuccess ? MID_CloudSynchSuccessful : MID_CloudSynchError);

	g_pTheDB->Players.Delete(tempPlayerID, false);

	if (bSuccess) {
		// Copy over this current caravel username and pw into the imported profile
		CDbPlayer *pPlayer = g_pTheDB->Players.GetByID(newPlayerID);
		if (pPlayer) {
			pPlayer->CNetNameText = username.c_str();
			pPlayer->CNetPasswordText = userpassword.c_str();
			pPlayer->Settings.SetVar(Settings::CloudActivated, true);
			pPlayer->Update();
			delete pPlayer;

			g_pTheDB->SetPlayerID(newPlayerID);
			EnablePlayerSettings(newPlayerID);
			return newPlayerID;
		}
	}

	g_pTheDB->SetPlayerID(0);
	return 0;
}

//*****************************************************************************
void CDrodScreen::ExportHold(const UINT dwHoldID)
{
	if (!dwHoldID) return;

	CDbHold *pHold = g_pTheDB->Holds.GetByID(dwHoldID);
	if (!pHold) return;

#ifndef STEAMBUILD
	//Load graphics/music into hold for inclusion in export.
	if (pHold->status == CDbHold::GetOfficialHoldStatus())
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
		ExportCleanup();
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
		Settings::ExportPath, wstrDefExportPath.c_str()) : wstrDefExportPath.c_str();

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
			pCurrentPlayer->Settings.SetVar(Settings::ExportPath, wstrExportPath.c_str());
			pCurrentPlayer->Update();
		}
		delete pCurrentPlayer;
	}

	return dwTagNo == TAG_OK;
}

//*****************************************************************************
void CDrodScreen::GoToBuyNow()
//Sets the game to windowed mode and opens a browser with appropriate sell link.
{
	SetFullScreen(false);
	string url = "http://www.caravelgames.com/buyTSS.html";
#ifdef STEAMBUILD
	const CDbHold::HoldStatus holdStatus = CDbHolds::GetNewestInstalledOfficialHoldStatus();
	url = "http://store.steampowered.com/app/";
	switch (holdStatus) {
		case CDbHold::KDD: url += "422180"; break;
		case CDbHold::JtRH: url += "422181"; break;
		case CDbHold::TCB: url += "422182"; break;
		default:
		case CDbHold::GatEB: url += "314330"; break;
		case CDbHold::TSS: url += "351320"; break;
	}
#elif defined(WIN32)
	//use default
#elif defined(__linux__)
	url = "http://www.caravelgames.com/buyTSSLinux.html";
#elif defined(__FreeBSD__)
	url = "http://www.caravelgames.com/buyTSSFreeBSD.html";
#elif defined(__APPLE__)
	url = "http://www.caravelgames.com/buyTSSOSX.html";
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
	OpenExtBrowser(CNetInterface::cNetBaseURL.c_str());
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
	set<WSTRING>& importedStyles) //(out)
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

	return ImportFiles(wstrImportFiles, importedIDs, importedStyles);
}

//*****************************************************************************
MESSAGE_ID CDrodScreen::ImportFiles(
//Returns: exit code of import process
//
//
	const vector<WSTRING>& wstrImportFiles,    //(in) filenames to import
	CIDSet& importedIDs,  //(out) IDs of the imported parent records
	set<WSTRING>& importedStyles) //(out)
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
			case CImportInfo::Data:	dwImportedID = CDbXML::info.dwDataImportedID;
				if (result == MID_ImportSuccessful)
				{
					CFiles f;
					WSTRING wstr;
					Base64::decode(CDbXML::info.headerInfo, wstr);
					f.WriteGameProfileBuffer(wstr,false,false);
				}
				break;
			case CImportInfo::Demo: dwImportedID = 1; break;   //value not used -- indicates a demo was in fact imported
			default: dwImportedID = 0; break;  //value not important
		}
		if (dwImportedID)
			importedIDs += dwImportedID;
		for (set<WSTRING>::const_iterator iter = CDbXML::info.roomStyles.begin();
				iter != CDbXML::info.roomStyles.end(); ++iter)
			importedStyles.insert(*iter);
	}

	//Display result.
	ExportCleanup();
	if (result != MID_NoText) {
#ifdef DEV_BUILD
		if (result != MID_ImportSuccessful)
#endif
		ShowOkMessage(result);
	}
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
			CDbXML::info.bReplaceOldHolds = true;
			break;
		case MID_OverwritePlayerPrompt:
			CDbXML::info.bReplaceOldPlayers = true;
			break;
		case MID_DowngradeHoldPrompt:
			CDbXML::info.bReplaceOldHolds = true;
			CDbXML::info.bAllowHoldDowngrade = true;
			break;
		default: ASSERT(!"Handle this case"); break;
	}
	CDbXML::SetCallback(this);
	result = CDbXML::ImportXML(); //restart import process with the above override flag(s) set
	return true;
}

//*****************************************************************************
MESSAGE_ID CDrodScreen::GetVersionMID(const UINT wVersion)
//Returns: a text MID for the specified export format version #
{
	if (wVersion < 200)
		return MID_DROD_AE;     //1.6
	if (wVersion < 300)
		return MID_DROD_JtRH;   //2.0
	if (wVersion < 302)
		return MID_DROD_TCB;    //3.0
	if (wVersion == 302)
		return MID_DROD_TCB3_1; //3.1
	if (wVersion == 303)
		return MID_DROD_TCB3_2; //3.2
	if (wVersion == 304)
		return MID_DROD_TCB3_3; //3.3
	if (wVersion >= 323 && wVersion <= 400)
		return MID_DROD_FnM;    //4.0
	if (wVersion >= 500 && wVersion < 509)
		return MID_DROD_TSS;    //5.0, 5.1
	if (wVersion >= 509 && wVersion < NEXT_VERSION_NUMBER)
		return MID_DROD_TSS5_2; //5.2
	return MID_DROD_UnsupportedVersion; //???
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
		CDbHold::TSS, CDbHold::GatEB, CDbHold::TCB, CDbHold::JtRH, CDbHold::KDD,
		CDbHold::NoStatus};

	for (UINT i=0; officialHolds[i] != CDbHold::NoStatus; ++i) {
		const UINT holdID = g_pTheDB->Holds.GetHoldIDWithStatus(officialHolds[i]);
		if (holdID)
			return officialHolds[i];
	}

	return CDbHold::NoStatus;
}

//*****************************************************************************
bool CDrodScreen::IsGameFullVersion()
//Returns: whether the game's pre-installed official hold is the full version (true), otherwise false
{
#ifdef STEAMBUILD
	return Metadata::GetInt(MetaKey::DEMO) != 1;
#endif

	//Programmatic override for embedding game media files in dev build.
	if (Metadata::GetInt(MetaKey::EMBEDMEDIA) == 1)
		return Metadata::GetInt(MetaKey::DEMO) != 1;

	const UINT holdID = g_pTheDB->Holds.GetHoldIDWithStatus(CDbHold::GetOfficialHoldStatus());
	if (!holdID)
		return false;
	CDbHold *pHold = g_pTheDB->Holds.GetByID(holdID);
	const bool bFull = pHold && pHold->Entrances.size() > EntrancesInFullVersion();
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
		WSTRING wstrFile = wstrBasename;
		if (wI == FLOOR_IMAGE) { //skip FLOOR_IMAGE texture: load 'tiles' images instead
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

	CDbXML::SetCallback(this);
	const bool bResult = CDbXML::ExportXML(V_Data, ids, wstrExportFile.c_str());

	//Clean up temporary data objects.
	for (CIDSet::const_iterator iter = ids.begin(); iter != ids.end(); ++iter)
		g_pTheDB->Data.Delete(*iter);

	ExportCleanup();
	ShowOkMessage(bResult ? MID_DataFileSaved : MID_DataFileNotSaved);
}

//*****************************************************************************
void CDrodScreen::ImportMedia()
//Imports sound and graphics files from disk into the DB.
{
#ifdef DEV_BUILD
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
	CDbHold::HoldStatus status = GetInstalledOfficialHold();
	if (status == CDbHold::NoStatus)
		status = (CDbHold::HoldStatus)Metadata::GetInt(MetaKey::APPLYHOLDSTATUS);
	const bool bDemo = !IsGameFullVersion();
	if (bDemo)
	{
		//Export only certain styles for the demo version.
		styles.clear();
		WSTRING demoStyle;
		switch (status) {
			case CDbHold::KDD:
			case CDbHold::JtRH:
				AsciiToUnicode("Foundation", demoStyle);
				styles.push_back(demoStyle);
			break;
			case CDbHold::TCB:
				AsciiToUnicode("City", demoStyle);
				styles.push_back(demoStyle);
			break;
			case CDbHold::GatEB:
				AsciiToUnicode("Forest", demoStyle);
				styles.push_back(demoStyle);
				AsciiToUnicode("City", demoStyle);
				styles.push_back(demoStyle);
				AsciiToUnicode("Aboveground", demoStyle);
				styles.push_back(demoStyle);
			break;
			case CDbHold::TSS:
				AsciiToUnicode("City", demoStyle);
				styles.push_back(demoStyle);
				AsciiToUnicode("Aboveground", demoStyle);
				styles.push_back(demoStyle);
				AsciiToUnicode("Deep Spaces", demoStyle);
				styles.push_back(demoStyle);
			break;
		}
	}
	UINT wStylesToExport = styles.size();

	//Ensure each song is only imported once.
	set<WSTRING> embeddedImages, embeddedSongs, embeddedSounds;

	UINT wStyleNo, wI;
	float fProgress = 0.0f;
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
		static const UINT NUM_GRAPHICS_FILES = 74;
		static const char graphicFilename[NUM_GRAPHICS_FILES][32] = {
			"Background", "Bolts", "Clock1", "Clock2", "Clouds1",
			"Credits", "CreditsJtRH", "Dialog",
			"Faces", "Fog1",
			"GameScreen", "GameScreenParts",
			"geb_logo", "gunthro_title_back", "gunthro_title_front", "gunthro_title_windows",
			"LevelStartBackground",
			"SignalBad", "SignalGood", "SignalNo", "SignalYes", "SignalCloud",
			"Sunshine1",
			"Sell_TSS_Left", "Sell_TSS_Right",
			"Sell1GatEB",
			"Sell1JtRH", "Sell2JtRH", "Sell3JtRH", "Sell4JtRH",
			"Sell1KDD", "Sell1TSS",
			"SellTexts",
			"StroutOfficeBG", "StroutOfficeCabinet", "StroutOfficePosters",
			"StroutOfficeSShotsGatEB", "StroutOfficeSShotsTCB", "StroutOfficeSShotsTSS",
			"Thanks1GatEB", "Thanks1JtRH", "Thanks1KDD",
			"TitleBG_KDD", "TitleBorderJtRH",
			"TitleLightMask",
			"TitleLogo_JtRH", "TitleLogo_KDD", "TitleLogo_TCB",
			"TitleLogo_TSS", "TitleLogo_TSS_2",
			"TitleMap", "TitleTCB_BG", "TitleTCB_BG1", "TitleTCB_BG2", "TitleTCB_Shadow",
			"TitleTSS_BG1", "TitleTSS_BG2", "TitleTSS_BG3", "TitleTSS_BG4", "TitleTSS_BG5", "TitleTSS_BG6",
			"TitleTSSChair", "TitleTSSChimney", "TitleTSSCog", "TitleTSSCrate",
			"TitleTSSFence", "TitleTSSLighthouse", "TitleTSSPipe",
			"TitleTSSRocks1", "TitleTSSRocks2", "TitleTSSStairs",
			"TitleTSSTree", "TitleTSSTub", "TitleTSSWall"
		};
		for (wI=TEXTURE_COUNT + 1 + NUM_GRAPHICS_FILES; wI--; )
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
				} else {
					//Graphics files to be embedded.
					const UINT image_index = wI - (TEXTURE_COUNT + 1);
					UTF8ToUnicode(graphicFilename[image_index], wstrImportFile);
				}
			} else if (wI == FLOOR_IMAGE) {
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
				if (wI != DEEP_MOSAIC && wI != SHALLOW_MOSAIC && wI != OVERHEAD_IMAGE) { //only present for some room styles
					WSTRING wstr = wstrImportFile;
					wstr += wszColon;
					wstr += wszSpace;
					wstr += g_pTheDB->GetMessageText(MID_FileNotFound);
					ShowOkMessage(wstr.c_str());
				}
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
			fProgress += 0.001f;
			Callbackf(fProgress); //event handler/keep-active
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

				LOG_TEXT(*sky);

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
		{
			WSTRING wstrDir;
			const UINT NON_STYLE_SONGS = 22;
			for (wI=SONG_MOOD_COUNT+NON_STYLE_SONGS; wI--; )	//include other music too
			{
				list<WSTRING> songlist;

				if (wI < SONG_MOOD_COUNT)
				{
					//Get song list for this mood from INI
					WSTRING wMoodText;
					UTF8ToUnicode(moodText[wI], wMoodText);
					WSTRING wstrSongmood = styleName + wMoodText;
					f.GetGameProfileString(INISection::Songs, wstrSongmood.c_str(), songlist);
				} else {
					static const SONGID songs[NON_STYLE_SONGS] = {
						SONGID_CREDITS_KDD, SONGID_CREDITS_JTRH, SONGID_CREDITS_TCB, SONGID_CREDITS_GATEB, SONGID_CREDITS_TSS,
						SONGID_ENDOFTHEGAME_JTRH, SONGID_ENDOFTHEGAME_TCB,
						SONGID_INTRO_KDD, SONGID_INTRO_JTRH, SONGID_INTRO_TCB, SONGID_INTRO_GATEB, SONGID_INTRO_TSS,
						SONGID_QUIT_KDD, SONGID_QUIT_JTRH, SONGID_QUIT_TCB, SONGID_QUIT_GATEB, SONGID_QUIT_TSS,
						SONGID_WINGAME_KDD, SONGID_WINGAME_JTRH, SONGID_WINGAME_TCB, SONGID_WINGAME_GATEB, SONGID_WINGAME_TSS
					};
					const UINT non_style_index = wI - SONG_MOOD_COUNT;
					ASSERT(non_style_index < NON_STYLE_SONGS);
					const SONGID songID = songs[non_style_index];

					//Import intro/title, exit, and end hold music once.
					if (wStyleNo == 1) {
						bool bImport = true;
						switch (status) {
							default:
							case CDbHold::KDD:
							case CDbHold::JtRH:
							case CDbHold::TCB:
								break; //no custom music exclusion defined for stand-alone installation for these holds
							case CDbHold::GatEB:
							case CDbHold::TSS:
								switch (songID) {
									//Demo versions don't use these songs
									case SONGID_CREDITS_GATEB:
									case SONGID_WINGAME_GATEB:
									case SONGID_CREDITS_TSS:
									case SONGID_WINGAME_TSS:
										if (bDemo)
											bImport = false;
									break;
									default: break; //include everything else
								}
							break;
						}
						if (bImport)
							g_pTheSound->GetSongFilepaths(songID, songlist);
					}
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
						if (!g_pTheSound->VerifySound(data)) {
							ShowOkMessage(MID_FileCorrupted);
						} else {
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
					fProgress += 0.001f;
					Callbackf(fProgress); //event handler/keep-active
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
				if (!g_pTheSound->VerifySound(data)) {
					ShowOkMessage(MID_FileCorrupted);
				} else {
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

			fProgress += 0.001f;
			Callbackf(fProgress); //event handler/keep-active
		}
	}

	HideStatusMessage();
	PublicHideProgressWidget();
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

	g_pTheSound->EnableSoundEffects(pPlayer->Settings.GetVar(Settings::SoundEffects, true));
	g_pTheSound->SetSoundEffectsVolume(pPlayer->Settings.GetVar(Settings::SoundEffectsVolume, (BYTE)DEFAULT_SOUND_VOLUME));

	g_pTheSound->EnableVoices(pPlayer->Settings.GetVar(Settings::Voices, true));
	g_pTheSound->SetVoicesVolume(pPlayer->Settings.GetVar(Settings::VoicesVolume, (BYTE)DEFAULT_VOICE_VOLUME));

	g_pTheSound->EnableMusic(pPlayer->Settings.GetVar(Settings::Music, true));
	g_pTheSound->SetMusicVolume(pPlayer->Settings.GetVar(Settings::MusicVolume, (BYTE)DEFAULT_MUSIC_VOLUME));

#ifdef __APPLE
	//KLUDGE: fullscreen doesn't work on some user's Macs, so just leave screen mode as-is
#else
	SetFullScreen(pPlayer->Settings.GetVar(Settings::Fullscreen, false));
#endif
	g_pTheBM->bAlpha = pPlayer->Settings.GetVar(Settings::Alpha, true);
	g_pTheDBM->SetGamma(pPlayer->Settings.GetVar(Settings::Gamma, (BYTE)CDrodBitmapManager::GetGammaOne()));
	g_pTheBM->eyeCandy = pPlayer->Settings.GetVar(Settings::EyeCandy, BYTE(Metadata::GetInt(MetaKey::MAX_EYE_CANDY)));
	g_pTheDBM->tarstuffAlpha = pPlayer->Settings.GetVar(Settings::TarstuffAlpha, BYTE(255));

	//Increment number of play sessions.
	pPlayer->Settings.SetVar(Settings::PlaySessions, pPlayer->Settings.GetVar(Settings::PlaySessions, UINT(0)) + 1);
	pPlayer->Update();

	CScreen::inputKeyFullScreen = pPlayer->Settings.GetVar(
		InputCommands::GetKeyDefinition(InputCommands::DCMD_ToggleFullScreen)->settingName,
		(InputKey)SDLK_F10
	);
	CScreen::inputKeyScreenshot = pPlayer->Settings.GetVar(
		InputCommands::GetKeyDefinition(InputCommands::DCMD_Screenshot)->settingName,
		(InputKey)SDLK_F11
	);

	//RepeatRate and UndoLevel are queried in CGameScreen::ApplyPlayerSettings().

	delete pPlayer;
}

//*****************************************************************************
bool CDrodScreen::ImportQueuedFiles()
//Import filenames queued on command line.
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
		if (wI == FLOOR_IMAGE) { //skip FLOOR_IMAGE texture: load 'tiles' images instead
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
bool CDrodScreen::OnQuit()
//Called when SDL_QUIT event is received.
{
	//If a Quit event is received while a quit prompt is already active,
	//then this serves as confirmation of the quit prompt.
	if (this->bQuitPrompt)
		return true;

	//Instead of quitting, go to the sell screen when this happens.
	GoToScreen(SelectSellScreen());
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
void CDrodScreen::SaveSurface(SDL_Surface *pSurface)
//Saves surface as image to selected export dir.
{
	g_pTheSound->PlaySoundEffect(SEID_SCREENSHOT);

	//Get default export path.
	CFiles Files;
	WSTRING wstrDefExportPath = Files.GetDatPath();
	wstrDefExportPath += wszSlash;
	wstrDefExportPath += wszDefExportDir;
	const CDbPackedVars settings = g_pTheDB->GetCurrentPlayerSettings();
	WSTRING wstrExportPath = settings.GetVar(Settings::ExportPath, wstrDefExportPath.c_str());

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
SCREENTYPE CDrodScreen::SelectSellScreen() const
{
	switch (GetHoldStatus())
	{
		case CDbHold::GatEB: return SCR_SellGatEB;
		case CDbHold::JtRH: return SCR_SellJtRH;
		case CDbHold::KDD: return SCR_SellKDD;
		case CDbHold::TCB: return SCR_SellTCB;
		case CDbHold::TSS: return SCR_SellTSS;
		default: return SCR_SellTSS;
	}
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
		pButton->SetWidth(wTextW + CX_SPACE * 2);
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
		pButton->SetWidth(wTextW + CX_SPACE * 2);
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
void CDrodScreen::ExportHoldProgressForUpload(const UINT holdID, const UINT playerID, string& outText)
//Exports a saved game record containing hold upload progress.
{
	CIDSet roomsConquered, roomsExplored, exportPlayerIDs(playerID);
	//If player has not played this hold yet, then skip it.
	CDbHolds::GetRoomsExplored(holdID, playerID, roomsExplored, roomsConquered);
	if (roomsExplored.empty())
		return; //player has not explored any rooms in the hold

	SAVETYPE eSaveType = ST_Progress; //a one-use record type for exporting saved game data
	 //If an EndHold save exists, mark the ST_Progress save accordingly
	CDb db;
	if (db.SavedGames.FindByEndHold(holdID, playerID))
		eSaveType = ST_ProgressEndHold;

	//Upload progress for this hold.
	CCueEvents Ignored;
	CCurrentGame *pCurrentGame = g_pTheDB->GetNewCurrentGame(holdID, Ignored,
			ASO_NONE); //don't add any saved games to hold during this call
	if (!pCurrentGame)
		return; //couldn't load data for hold

	//Upload a saved game containing all explored+conquered rooms for the player.
	pCurrentGame->dwPlayerID = playerID;
	pCurrentGame->ExploredRooms = roomsExplored;
	pCurrentGame->ConqueredRooms = roomsConquered;
	pCurrentGame->eType = eSaveType;

	if (pCurrentGame->Update())	//ensure record was saved to DB for export
	{
		string text;
		if (CDbXML::ExportXML(V_Players, exportPlayerIDs, text, eSaveType))
			outText += text;
		//Once exported, can remove this record from DB
		g_pTheDB->SavedGames.Delete(pCurrentGame->dwSavedGameID);
	}
	delete pCurrentGame;
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
			wDataFormat = DATA_OGG;

			wstrFilepath = g_pTheSound->GetSongModPath();
			wstrFilepath += wstrFile;
			wstrFilepath += wszDotOgg;

			if (!CFiles::DoesFileExist(wstrFilepath.c_str())) {
				wstrFilepath = g_pTheSound->GetSongPath();
				wstrFilepath += wstrFile;
				wstrFilepath += wszDotOgg;
			}
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
			WSTRING wstrTimFilepath = g_pTheBM->GetTileImageMapModFilepath(wstrFile.c_str());
			CFiles::ReadFileIntoBuffer(wstrTimFilepath.c_str(), pMedia->timData);
			if (pMedia->timData.empty()) {
				wstrTimFilepath = g_pTheBM->GetTileImageMapFilepath(wstrFile.c_str());
				CFiles::ReadFileIntoBuffer(wstrTimFilepath.c_str(), pMedia->timData);
			}
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

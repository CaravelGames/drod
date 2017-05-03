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
 *
 * ***** END LICENSE BLOCK ***** */

#include "RoomDrawStatsEffect.h"
#include "DrodEffect.h"
#include "RoomWidget.h"
//#include <BackEndLib/Assert.h>

const UINT numCounts = 3;

//
//Public methods.
//

//*****************************************************************************
CRoomDrawStatsEffect::CRoomDrawStatsEffect(CWidget *pSetWidget)
	: CFrameRateEffect(pSetWidget,EROOMDRAWSTATS)
	, lastTurn((UINT)-1)
{
	this->y += 50; //quick hack to not display over frame rate effect
	this->dirtyRects[0].y = this->y;

	this->pRoomWidget = DYN_CAST(CRoomWidget*, CWidget*, pSetWidget);

	this->counts.resize(numCounts, 0);
}

CRoomDrawStatsEffect::~CRoomDrawStatsEffect()
{ }

//*****************************************************************************
void CRoomDrawStatsEffect::PrepDisplay()
{
	++this->wFrameCount;

	UINT dirty[numCounts];
	this->pRoomWidget->CountDirtyTiles(dirty[0], dirty[1], dirty[2]);

	this->counts[0] += dirty[0];
	this->counts[1] += dirty[1];
	this->counts[2] += dirty[2];

	const UINT turn = this->pRoomWidget->GetLastTurn();
	const UINT dwNow = SDL_GetTicks();
	static const Uint32 INTERVAL = 1000; //ms
	if ((dwNow >= this->dwLastDrawTime + INTERVAL) || turn != this->lastTurn)
	{
		WSTRING wStr;
		WCHAR wczNum[12];

		for (UINT i=0; i<numCounts; ++i) {
			float fOneSecondCount = this->counts[i] / float(this->wFrameCount);

			fOneSecondCount *= 10.f;
			if (i)
				wStr += wszSpace;
			wStr += _itoW(int(fOneSecondCount) / 10, wczNum, 10);
			wStr += wszPeriod;
			wStr += _itoW(int(fOneSecondCount) % 10, wczNum, 10);

			if (turn != this->lastTurn) {
				wStr += wszSpace;
				wStr += wszLeftBracket;
				wStr += _itoW(dirty[i], wczNum, 10);
				wStr += wszRightBracket;
			}

			this->counts[i] = 0;
		}

		SetText(wStr.c_str());

		this->dwLastDrawTime = dwNow;
		this->lastTurn = turn;
		this->wFrameCount = 0;
	}
}

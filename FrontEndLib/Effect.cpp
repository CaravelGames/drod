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

#include "Effect.h"
#include <BackEndLib/Assert.h>

//*****************************************************************************
CEffect::CEffect(CWidget *pSetOwnerWidget, const UINT eType)
	: pOwnerWidget(pSetOwnerWidget)
	, dwTimeStarted(0), dwTimeOfLastMove(0)
	, eEffectType(eType)
	, bRequestRetainOnClear(false)
	, fOpacity(1.0)
//Constructor.
{
	ASSERT(pSetOwnerWidget);
}

//*****************************************************************************
Uint32 CEffect::TimeElapsed()
//Return: the time elapsed since effect started.
{
	const Uint32 dwNow = SDL_GetTicks();
	if (!this->dwTimeStarted)
	{
		//Effect starts now.
		this->dwTimeStarted = this->dwTimeOfLastMove = dwNow;
		return 0;
	}
	ASSERT(dwNow >= this->dwTimeStarted);
	return dwNow - this->dwTimeStarted;
}

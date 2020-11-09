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
 * 1997, 2000, 2001, 2002, 2005, 2020 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "InputKey.h"


#include <SDL_syswm.h>
#include <SDL_events.h>
#include <BackEndLib/Types.h>

const int64_t ShiftBit = int64_t(1) << 63;
const int64_t AltBit = int64_t(1) << 62;
const int64_t CtrlBit = int64_t(1) << 61;
const int64_t KeycodeBitmask = 0x2FFFFFFFFFFFFFFF;

InputKey BuildInputKey(int32_t keycode, bool isShift, bool isAlt, bool isCtrl)
{
	if (keycode == SDLK_LSHIFT || keycode == SDLK_RSHIFT)
		return ShiftBit;

	else if (keycode == SDLK_LALT || keycode == SDLK_RALT) 
		return AltBit;
	
	else if (keycode == SDLK_LCTRL || keycode == SDLK_RCTRL)
		return CtrlBit;
	
	else
		return int64_t(keycode)
			| (isShift ? ShiftBit : 0)
			| (isAlt ? AltBit : 0)
			| (isCtrl ? CtrlBit : 0);
}

InputKey BuildInputKey(SDL_KeyboardEvent keyEvent)
{
	return BuildInputKey(
		keyEvent.keysym.sym,
		keyEvent.keysym.mod & KMOD_SHIFT,
		keyEvent.keysym.mod & KMOD_ALT,
		keyEvent.keysym.mod & KMOD_CTRL
	);
}

void ReadInputKey(InputKey inputKey, SDL_Keycode& keycode, bool& isShift, bool& isAlt, bool& isCtrl)
{
	keycode = static_cast<SDL_Keycode>(inputKey & KeycodeBitmask);
	isShift = inputKey & ShiftBit;
	isAlt = inputKey & AltBit;
	isCtrl = inputKey & CtrlBit;
}

SDL_Keycode ReadInputKey(InputKey inputKey)
{
	return static_cast<SDL_Keycode>(inputKey & KeycodeBitmask);
}
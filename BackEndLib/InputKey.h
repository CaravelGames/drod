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

#ifndef _INPUTKEY_H
#define _INPUTKEY_H

#include <SDL_syswm.h>
#include <SDL_events.h>
#include "Types.h"

#define UNKNOWN_INPUT_KEY InputKey(0)

typedef int64_t InputKey;

extern const InputKey    BuildInputKey(int32_t keycode, bool isShift, bool isAlt, bool isCtrl);
extern const InputKey    BuildInputKey(SDL_KeyboardEvent keyEvent);
extern const SDL_Keycode ReadInputKey(InputKey inputKey);
extern void              ReadInputKey(InputKey inputKey, SDL_Keycode &keycode, bool &isShift, bool &isAlt, bool &isCtrl);

#endif //...#ifndef _INPUTKEY_H

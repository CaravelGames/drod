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
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "I18N.h"

#include <sstream>

namespace I18N {

	//************************************************************************************
	WSTRING DescribeInputKey(const InputKey key)
	{
		static WSTRING ShiftShort = AsciiToUnicode("Sh+");
		static WSTRING ShiftLong = AsciiToUnicode("Shift ");
		static WSTRING CtrlShort = AsciiToUnicode("Ct+");
		static WSTRING CtrlLong = AsciiToUnicode("Ctrl ");
		static WSTRING AltShort = AsciiToUnicode("ALt+");
		static WSTRING AltLong = AsciiToUnicode("Alt ");

		if (key == SDLK_UNKNOWN)
			return g_pTheDB->GetMessageText(MID_UNKNOWN);

		SDL_Keycode keyCode;
		bool bIsShift, bIsCtrl, bIsAlt;
		ReadInputKey(key, keyCode, bIsShift, bIsAlt, bIsCtrl);
		const UINT wModifierCount = bIsShift + bIsAlt + bIsCtrl;

		bool bHasModifier = bIsShift || bIsCtrl || bIsAlt;
		bool bHasNonModifier = keyCode != SDLK_UNKNOWN;

		std::wstringstream str;

		if (bIsShift) {
			str << (wModifierCount > 1 && bHasNonModifier ? ShiftShort : ShiftLong);
		}

		if (bIsCtrl) {
			str << (wModifierCount > 1 && bHasNonModifier ? CtrlShort : CtrlLong);
		}

		if (bIsAlt) {
			str << (wModifierCount > 1 && bHasNonModifier ? AltShort : AltLong);
		}

		if (keyCode != SDLK_UNKNOWN)
			str << g_pTheDB->GetMessageText(InputCommands::KeyToMID(keyCode));

		return str.str();
	}
}

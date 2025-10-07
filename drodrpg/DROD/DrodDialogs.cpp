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

#include "DrodDialogs.h"

CDrodDialogs *g_pTheDialogs = NULL;

//*****************************************************************************
UINT CDrodDialogs::ShowYesNoMessage(
// See CDrodScreen::ShowYesNoMessage() for documentation
	const WCHAR *pwczText,
	const MESSAGE_ID dwYesButtonText,
	const MESSAGE_ID dwNoButtonText)
{
	return GetEditRoomScreen()->ShowYesNoMessage(pwczText, dwYesButtonText, dwNoButtonText);
}


//*****************************************************************************
void CDrodDialogs::HideStatusMessage()
{
	return GetEditRoomScreen()->HideStatusMessage();
}

//*****************************************************************************
UINT CDrodDialogs::SelectFile(WSTRING &filePath, WSTRING &fileName,
	const MESSAGE_ID messagePromptID, const bool bWrite,
	const UINT extensionTypes)
{
	return GetEditRoomScreen()->SelectFile(filePath, fileName, messagePromptID, bWrite, extensionTypes);
}

//*****************************************************************************
UINT  CDrodDialogs::ShowOkMessage(const MESSAGE_ID dwMessageID)
{
	return GetEditRoomScreen()->ShowOkMessage(dwMessageID);
}

//*****************************************************************************
UINT  CDrodDialogs::ShowOkMessage(const WCHAR *pwczText)
{
	return GetEditRoomScreen()->ShowOkMessage(pwczText);
}

//*****************************************************************************
void CDrodDialogs::ShowStatusMessage(const MESSAGE_ID dwMessageID)
{
	return GetEditRoomScreen()->ShowStatusMessage(dwMessageID);
}

//*****************************************************************************
CEditRoomScreen *CDrodDialogs::GetEditRoomScreen() const
{
	return DYN_CAST(CEditRoomScreen *, CScreen *, g_pTheSM->GetScreen(SCR_EditRoom));
}

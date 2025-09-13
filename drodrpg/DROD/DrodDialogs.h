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

#ifndef _DRODDIALOGS_H
#define _DRODDIALOGS_H


#include "DrodScreen.h"
#include "DrodScreenManager.h"
#include "EditRoomScreen.h"

class CDrodDialogs {
public:
	CDrodDialogs() {}

	void  HideStatusMessage();

	UINT SelectFile(WSTRING &filePath, WSTRING &fileName,
		const MESSAGE_ID messagePromptID, const bool bWrite,
		const UINT extensionTypes);

	UINT  ShowOkMessage(const MESSAGE_ID dwMessageID);
	UINT  ShowOkMessage(const WCHAR *pwczText);
	void  ShowStatusMessage(const MESSAGE_ID dwMessageID);
	UINT  ShowYesNoMessage(const WCHAR *pwczText, const MESSAGE_ID dwYesButtonText = MID_Yes, const MESSAGE_ID dwNoButtonText = MID_No);


private:
	CEditRoomScreen *GetEditRoomScreen() const;
};

extern CDrodDialogs *g_pTheDialogs;

#endif //...#ifndef _DRODDIALOGS_H

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

 //This is a CDialog that has been augmented to show a list of level entrances.

#ifndef _SELECTMEDIADIALOGWIDGET_H
#define _SELECTMEDIADIALOGWIDGET_H

#include "DrodScreen.h"
#include <FrontEndLib/DialogWidget.h>
#include "../DRODLib/DbLevels.h"
#include <BackEndLib/MessageIDs.h>
#include <set>

class CDbHold;
class CEntranceData;
class CLabelWidget;
class CListBoxWidget;
class CCurrentGame;
class CSelectMediaDialogWidget : public CDialogWidget
{
public:
	CSelectMediaDialogWidget(const UINT dwSetTagNo, const int nSetX = 0, const int nSetY = 0);

	enum DATATYPE
	{
		Images,        //custom hold images
		Sounds,        //custom hold sounds
		Videos,        //custom hold videos
	};

	UINT    GetSelectedItem() const;
	virtual void   OnClick(const UINT dwTagNo);
	virtual void   OnDoubleClick(const UINT dwTagNo);
	virtual void   OnSelectChange(const UINT dwTagNo);
	void     SelectItem(const UINT dwTagNo);
	void     SetForDisplay(const MESSAGE_ID headerMID, CDbHold *pHold, const DATATYPE eDatatype);

private:
	void          DeleteMedia(const UINT wMediaId);
	const WSTRING GetMediaName(const UINT wMediaId) const;
	const WSTRING GetMediaName(const UINT wMediaId, CIDSet *deletedDataIDs) const;
	UINT          ImportFile(DATATYPE eType, const UINT wReplacedMediaId = 0);
	void          RefreshButtons();
	void          RefreshList();
	void          RefreshMediaName(const UINT wMediaId);
	void          UndeleteMedia(const UINT wMediaId);

	CDrodScreen *GetParentDrodScreen() const;

	bool ValidateVideo(CStretchyBuffer &buffer);

	CIDSet GetSupportedFormats() const;

	CListBoxWidget *pListBoxWidget;
	CDbHold        *pSourceHold;
	CLabelWidget   *pHeaderLabel;

	DATATYPE eDatatype;
};

#endif   //_SELECTMEDIADIALOGWIDGET_H

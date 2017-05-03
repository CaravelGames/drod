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

#ifndef HYPERLINKWIDGET_H
#define HYPERLINKWIDGET_H

#include "FontManager.h"
#include "LabelWidget.h"

//******************************************************************************
class CHyperLinkWidget : public CLabelWidget
{
public:
	CHyperLinkWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
		const UINT wSetW, const UINT wSetH, const UINT eSetFontType,
		const UINT eSetFontType2, const WCHAR *pwczSetText, const WCHAR *pwczSetLink,
		const bool bResizeToFit=false, const UINT wFirstIndent=0, const bool bCacheRendering=false);

	virtual ~CHyperLinkWidget();

	virtual bool      ContainsCoords(const int nX, const int nY) const;
	const WSTRING &   GetLink() {return wstrLink;}
	bool              IsExternal();
	void              SetLink(const WCHAR *pwczSetLink) { wstrLink = (pwczSetLink ? pwczSetLink : wszEmpty); }

protected:
	virtual void      HandleMouseMotion(const SDL_MouseMotionEvent &Motion);
	virtual void      HandleMouseOut();
	virtual bool      IsHoverable() const {return true;}

private:
	WSTRING  wstrLink;
	UINT     eFontType1;
	UINT     eFontType2;
};

#endif //#ifndef HYPERLINKWIDGET_H

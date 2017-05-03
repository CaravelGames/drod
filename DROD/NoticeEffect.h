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

#ifndef NOTICEEFFECT_H
#define NOTICEEFFECT_H

#include "DrodEffect.h"

#include <DRODLib/NetInterface.h>
#include <FrontEndLib/FontManager.h>
#include <BackEndLib/Coord.h>
#include <BackEndLib/Wchar.h>

#include <string>
#include <map>

//******************************************************************************
class CNoticeEffect : public CEffect
{
	friend class CCaravelNetNoticeEffect;

public:
	CNoticeEffect(CWidget *pSetWidget, const WCHAR* pTitle, const WCHAR* pText,
		const UINT wYOffset=0, const UINT eType=ETEXTNOTICE);
	virtual ~CNoticeEffect();

	virtual bool   Draw(SDL_Surface* pDestSurface=NULL);

	void           SetAlpha(const Uint8 opacity);

	virtual long   GetDrawSequence() const {return 1000L;}   //draw last

protected:
	void           PrepWidget(const WCHAR* pTitle, const WCHAR* pText);
	virtual void   SetLocation();

	UINT              wYOffset;
	UINT              eFontType;
	int               x, y;
	int               targetX, targetY; // Where should this effect be moving to at each step
	UINT              w, h;
	UINT              wDisplayLines;
	UINT              maxWidth;
	Uint32            dwStartTime, dwDuration;

	enum EState {
		NS_Init,
		NS_SlideIn,
		NS_DisplayPause,
		NS_MoveUp,
		NS_MovingUp,
		NS_SlideOut,
		NS_Done
	};
	EState         state;

	SDL_Rect screenRect;
	SDL_Surface *  pNoticeSurface;
	SURFACECOLOR   BGColor;
	Uint8          opacity;
};

//******************************************************************************
class CCaravelNetNoticeEffect;
typedef std::vector<CCaravelNetNoticeEffect*> NOTICES;

class CCaravelNetNoticeEffect : public CNoticeEffect
{
public:
	CCaravelNetNoticeEffect(CWidget *pSetWidget, const CNetNotice &pNotice, NOTICES &notices);
	virtual ~CCaravelNetNoticeEffect();

	void           RemoveFromNotices();

private:
	virtual void   SetLocation();

	NOTICES &notices; //set of notice effects maintained by my parent
};

#endif //#ifndef NOTICEEFFECT_H

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
 * Portions created by the Initial Developer are Copyright (C)
 * 2003, 2005 Caravel Software. All Rights Reserved.
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef SELLSCREEN_H
#define SELLSCREEN_H

#include "DrodScreen.h"
#include "DrodSound.h"

//***************************************************************************************
class CSellScreen2 : public CDrodScreen
{
protected:
	CSellScreen2(SCREENTYPE screentype)
		: CDrodScreen(screentype)
	{ }

	virtual void OnClick(const UINT dwTagNo);
	virtual bool OnQuit();
	virtual bool SetForActivate();

	virtual string GetImageSuffix() const { return string(); }
	virtual void PlaySellSong() { PlaySong(SONGID_QUIT_TSS); }

	void PlaySong(SONGID songid);

	void Version2ScreenSetup();

public:
	virtual void Paint(bool bUpdateRect=true);
};

//***************************************************************************************
class CSellScreenKDD : public CSellScreen2
{
protected:
	friend class CDrodScreenManager;
	CSellScreenKDD()
		: CSellScreen2(SCR_SellKDD)
	{
		Version2ScreenSetup();
	}

	virtual string GetImageSuffix() const { return string("KDD"); }
	virtual void PlaySellSong() { PlaySong(SONGID_QUIT_KDD); }
};

//***************************************************************************************
class CSellScreenJtRH : public CSellScreen2
{
protected:
	friend class CDrodScreenManager;
	CSellScreenJtRH()
		: CSellScreen2(SCR_SellJtRH)
	{
		Version2ScreenSetup();
	}

	virtual string GetImageSuffix() const { return string("JtRH"); }
	virtual void PlaySellSong() { PlaySong(SONGID_QUIT_JTRH); }
};

//***************************************************************************************
class CSellScreen3 : public CSellScreen2
{
protected:
	friend class CDrodScreenManager;
	CSellScreen3(SCREENTYPE screentype);

	virtual void OnBetweenEvents();

protected:
	void MoveScreenshots(SDL_Surface *pDestSurface, const bool bUpdateRect);
	virtual int GetScreenshotY() const { return 12; }

private:
	virtual void PaintCabinetEdge(SDL_Surface* /*pDestSurface*/) { }

	int nextSShotToDisplay;
	Uint32 dwNextSShotMove;
};

//***************************************************************************************
class CSellScreenGatEB : public CSellScreen3
{
protected:
	friend class CDrodScreenManager;
	CSellScreenGatEB();

	virtual void PlaySellSong() { PlaySong(SONGID_QUIT_GATEB); }

	virtual bool SetForActivate();

public:
	virtual void Paint(bool bUpdateRect=true);

private:
	void SelectNextSellText();

	int sellTextIndex;
};

//***************************************************************************************
class CSellScreenTSS : public CSellScreen3
{
protected:
	friend class CDrodScreenManager;
	CSellScreenTSS();

	virtual void PlaySellSong() { PlaySong(SONGID_QUIT_TSS); }

	virtual bool SetForActivate();

public:
	virtual void Paint(bool bUpdateRect=true);

private:
	void SelectNextSellText();

	int badTextIndex, goodTextIndex;
};

#endif //...#ifndef SELLSCREEN_H

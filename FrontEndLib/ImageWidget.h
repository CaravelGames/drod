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

#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include "Widget.h"
#include <BackEndLib/Wchar.h>

//******************************************************************************
class CImageWidget : public CWidget
{
public:
	CImageWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY, SDL_Surface *pSurface);
	CImageWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY, const WCHAR *pwczFilename);
	~CImageWidget();

	SDL_Surface* GetImageSurface() const {return this->pImageSurface;}
	virtual void   Paint(bool bUpdateRect = true);

	virtual void SetAlpha(const Uint8 setAlpha, bool usePerPixelAlphaBlending=false);
	void   SetImage(SDL_Surface *pSurface);
	void   SetImage(const WCHAR *pwczFilename);

protected:
	Uint8 alpha;
	bool bPerPixelAlphaBlending;

private:
	void FreeSurface();

	SDL_Surface *pImageSurface;
	WSTRING wFilename;
};

class CImageVectorWidget : public CImageWidget
{
	struct Image {
		Image(SDL_Surface* pImage, int x, int y) : pImage(pImage), x(x), y(y) { }

		SDL_Surface* pImage;
		int x, y;
	};

public:
	CImageVectorWidget(const UINT dwSetTagNo);
	~CImageVectorWidget();

	virtual void   Paint(bool bUpdateRect = true);

	void AddImage(SDL_Surface *pSurface, const int nSetX, const int nSetY);
	void ClearImages();
	virtual void SetAlpha(const Uint8 setAlpha, bool usePerPixelAlphaBlending=false);

private:
	vector<Image> imageSurfaces;
};

#endif //#ifndef IMAGEWIDGET_H

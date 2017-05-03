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

#include "ImageWidget.h"

#include "BitmapManager.h"
#include <BackEndLib/Assert.h>

//
//Public methods.
//

//*****************************************************************************
CImageWidget::CImageWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget
	const int nSetX, const int nSetY,               //    constructor.
	SDL_Surface *pSurface)          //(in)   Image surface to display.
	: CWidget(WT_Image, dwSetTagNo, nSetX, nSetY, 0, 0)
	, alpha(255)
	, bPerPixelAlphaBlending(false)
	, pImageSurface(NULL)
{
	if (pSurface)
		SetImage(pSurface);
	Load();
}

//*****************************************************************************
CImageWidget::CImageWidget(
//Constructor.
//
//Params:
	const UINT dwSetTagNo,                //(in)   Required params for CWidget
	const int nSetX, const int nSetY,               //    constructor.
	const WCHAR *pwczFilename)          //(in)   Image file to display.
	: CWidget(WT_Image, dwSetTagNo, nSetX, nSetY, 0, 0)
	, alpha(255)
	, bPerPixelAlphaBlending(false)
	, pImageSurface(NULL)
{
	if (pwczFilename)
		SetImage(pwczFilename);
	Load();
}

//*****************************************************************************
CImageWidget::~CImageWidget()
{
	FreeSurface();

	if (IsLoaded())
		Unload();
}

void CImageWidget::FreeSurface()
{
	if (this->pImageSurface) {
		SDL_FreeSurface(this->pImageSurface);
		this->pImageSurface = NULL;
	}
}

//*****************************************************************************
void CImageWidget::Paint(bool /*bUpdateRect*/)
{
	if (!this->pImageSurface) return;
	ASSERT(this->w > 0);
	ASSERT(this->h > 0);
	ASSERT(this->w <= (UINT)this->pImageSurface->w);
	ASSERT(this->h <= (UINT)this->pImageSurface->h);

	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);

	SDL_Rect src = MAKE_SDL_RECT(0, 0, this->w, this->h);
	SDL_Rect dest;
	GetRect(dest);
	dest.x += nOffsetX;
	dest.y += nOffsetY;
	if (this->bPerPixelAlphaBlending) {
		if (this->alpha > 0)
			g_pTheBM->BlitAlphaSurfaceWithTransparency(src, this->pImageSurface, dest, GetDestSurface(), this->alpha);
	} else {
		SDL_BlitSurface(this->pImageSurface, &src, GetDestSurface(), &dest);
	}
}

//*****************************************************************************
void CImageWidget::SetAlpha(
	const Uint8 setAlpha,
	bool usePerPixelAlphaBlending) //[false]
{
	ASSERT(setAlpha > 0);
	this->alpha = setAlpha;
	this->bPerPixelAlphaBlending = usePerPixelAlphaBlending;
	if (this->pImageSurface && !usePerPixelAlphaBlending)
		AutodetectSurfaceBlending(this->pImageSurface, this->alpha);
}

//*****************************************************************************
void CImageWidget::SetImage(SDL_Surface *pSurface)
//Sets the image to surface (may be NULL).
{
	FreeSurface();

	this->pImageSurface = pSurface;
	if (this->pImageSurface)
	{
		this->w = this->pImageSurface->w;
		this->h = this->pImageSurface->h;
		SetAlpha(this->alpha);
	} else {
		this->w = this->h = 0;
	}

	this->wFilename = wszEmpty;	//don't know image name
}

//*****************************************************************************
void CImageWidget::SetImage(const WCHAR *pwczFilename)
{
	ASSERT(pwczFilename);

	if (this->wFilename == pwczFilename) return;	//don't reload same image

	FreeSurface();

	ASSERT(g_pTheBM);
	const UINT wFormat = g_pTheBM->GetImageExtensionType(pwczFilename);
	if (wFormat)
		this->pImageSurface = g_pTheBM->LoadImageSurface(pwczFilename, wFormat);
	if (!this->pImageSurface)  //try to determine path if load didn't work
		this->pImageSurface = g_pTheBM->LoadImageSurface(pwczFilename);
	if (this->pImageSurface)
	{
		this->w = this->pImageSurface->w;
		this->h = this->pImageSurface->h;
		SetAlpha(this->alpha);
	} else {
		this->w = this->h = 0;
	}

	this->wFilename = pwczFilename;	//save image filename (reload optimization)
}

//*****************************************************************************
CImageVectorWidget::CImageVectorWidget(
	const UINT dwSetTagNo)
	: CImageWidget(dwSetTagNo, 0, 0, (SDL_Surface*)NULL)
{
}

//*****************************************************************************
CImageVectorWidget::~CImageVectorWidget()
{
	ClearImages();
}

//*****************************************************************************
void CImageVectorWidget::AddImage(SDL_Surface *pSurface, const int nSetX, const int nSetY)
{
	ASSERT(pSurface);
	this->imageSurfaces.push_back(Image(pSurface, nSetX, nSetY));
}

void CImageVectorWidget::ClearImages()
{
	for (vector<Image>::iterator it=this->imageSurfaces.begin();
			it!=this->imageSurfaces.end(); ++it)
	{
		SDL_FreeSurface(it->pImage);
	}

	this->imageSurfaces.clear();
}

//*****************************************************************************
void CImageVectorWidget::Paint(bool /*bUpdateRect*/)
{
	if (this->imageSurfaces.empty()) return;

	int nOffsetX, nOffsetY;
	GetScrollOffset(nOffsetX, nOffsetY);

	SDL_Surface *pDestSurface = GetDestSurface();
	for (vector<Image>::iterator it=this->imageSurfaces.begin();
			it!=this->imageSurfaces.end(); ++it)
	{
		const Image& image = *it;
		SDL_Surface *pSurface = image.pImage;
		this->w = pSurface->w;
		this->h = pSurface->h;

		SDL_Rect src = MAKE_SDL_RECT(0, 0, this->w, this->h);
		SDL_Rect dest;
		GetRect(dest);
		dest.x += nOffsetX + image.x;
		dest.y += nOffsetY + image.y;
		if (this->bPerPixelAlphaBlending) {
			if (this->alpha > 0)
				g_pTheBM->BlitAlphaSurfaceWithTransparency(src, pSurface, dest, pDestSurface, this->alpha);
		} else {
			SDL_BlitSurface(pSurface, &src, pDestSurface, &dest);
		}
	}

	this->w = this->h = 0; //revert
}


//*****************************************************************************
void CImageVectorWidget::SetAlpha(
	const Uint8 setAlpha,
	bool usePerPixelAlphaBlending) //[false]
{
	CImageWidget::SetAlpha(setAlpha, usePerPixelAlphaBlending);

	for (vector<Image>::iterator it=this->imageSurfaces.begin();
		it!=this->imageSurfaces.end(); ++it)
	{
		SDL_Surface *pImage = it->pImage;
		if (!usePerPixelAlphaBlending)
			AutodetectSurfaceBlending(pImage, this->alpha);
	}
}

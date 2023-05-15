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
 * Gerry Jo Jellestad (trick)
 *
 * ***** END LICENSE BLOCK ***** */

#define SUPPORT_PNG

#ifdef SUPPORT_PNG
#include <png.h>
#endif

#include "PNGHandler.h"
#include "Colors.h"
#include <BackEndLib/Files.h>
#include <SDL.h>

#ifdef SUPPORT_PNG
//**********************************************************************************
// Callback function for libpng.
typedef struct {
	BYTE *buffer;
	UINT len;
	UINT pos;
} ReadInfo;

static void PNGRead_cb(png_structp pPNG, png_bytep pData, png_size_t length)
{
	ReadInfo *info = (ReadInfo *)png_get_io_ptr(pPNG);
	BYTE *buffer = info->buffer;
	UINT pos = info->pos;

	if (pos + length > info->len)
		png_error(pPNG, "Buffer too short");

	for (UINT i = 0; i < length; ++i, ++pos)
		pData[i] = buffer[pos];

	info->pos = pos;
}

//Here's the routine that will replace the standard png_error method.
void my_error_exit(png_structp png_ptr, png_const_charp /*error_message*/)
{
	// Return control to the setjmp point.
#if PNG_LIBPNG_VER >= 10500
	png_longjmp(png_ptr, 1);
#else
	longjmp(png_ptr->jmpbuf, 1);
#endif
}

#endif // SUPPORT_PNG

//**********************************************************************************
SDL_Surface *CPNGHandler::CreateSurface(
//Open the specified PNG file, and uncompress it into a new SDL_Surface.
//
//Returns: The new SDL_Surface with the image if successful, NULL otherwise
//
//Params:
	const WCHAR *wszFile) //(in)  The file containing the PNG image
{
#ifdef SUPPORT_PNG
	SDL_Surface *surface = NULL;
	FILE *fp = CFiles::Open(wszFile, "rb");
	if (fp) {
		surface = CreateSurface(fp, 0);
		fclose(fp);
	}
	return surface;
#else // SUPPORT_PNG
	return NULL;
#endif // SUPPORT_PNG
}

//**********************************************************************************
SDL_Surface *CPNGHandler::CreateSurface(
//Uncompress a PNG file from memory or file, into a new SDL_Surface.
//
//Returns: The new SDL_Surface with the image if successful, NULL otherwise
//
//Params:
	const void *buffer,  //(in)  Buffer to read PNG file from
	const UINT length)   //(in)  Length of buffer, or 0 if buffer is a file pointer
{
#ifdef SUPPORT_PNG
	png_bytep *volatile pRows_v = NULL;
	SDL_Surface *volatile surface_v = NULL;

	// Setup stuff
	png_structp pPNG = png_create_read_struct(PNG_LIBPNG_VER_STRING,
			NULL, my_error_exit, NULL);
	if (!pPNG) return NULL;
	png_infop pInfo = png_create_info_struct(pPNG);
#if PNG_LIBPNG_VER >= 10603
	png_set_option(pPNG, PNG_MAXIMUM_INFLATE_WINDOW, PNG_OPTION_ON);
#endif

	if (!pInfo || setjmp(png_jmpbuf(pPNG)))
	{
CS_error:
		png_destroy_read_struct(&pPNG,
				pInfo ? &pInfo : (png_infopp)NULL, (png_infopp)NULL);
		delete[] (png_bytep *)pRows_v;
		if (surface_v)
			SDL_FreeSurface((SDL_Surface*)surface_v);
		return NULL;
	}

	ReadInfo readinfo = { (BYTE*)buffer, length, 0 };
	SDL_Surface *surface = NULL;
	png_uint_32 width, height;
	int bitdepth, colortype, interlace, colorkey = -1, surfalpha = -1;
	png_color_16p trans_values;

	// If length is 0, read from a file, otherwise, read from memory
	if (length)
		png_set_read_fn(pPNG, (png_voidp)&readinfo, PNGRead_cb);
	else
		png_init_io(pPNG, (FILE*)buffer);

	png_read_info(pPNG, pInfo);
	png_get_IHDR(pPNG, pInfo, &width, &height, &bitdepth, &colortype, &interlace, NULL, NULL);

	// Convert 16-bit to 8-bit and unpack pixels
	png_set_strip_16(pPNG);
	png_set_packing(pPNG);
	// Expand grayscale to 8-bit
	if (colortype == PNG_COLOR_TYPE_GRAY && bitdepth < 8)
		 png_set_expand(pPNG);
	// Grayscale with alpha isn't supported by SDL, so make it RGBA
	if (colortype == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(pPNG);
	if (colortype == PNG_COLOR_TYPE_PALETTE)
		png_set_tRNS_to_alpha(pPNG);

	// Done with transformation setup, update info
	png_read_update_info(pPNG, pInfo);
	png_get_IHDR(pPNG, pInfo, &width, &height, &bitdepth, &colortype, &interlace, NULL, NULL);

	const int channels = png_get_channels(pPNG, pInfo);
#if (SDL_BYTEORDER != SDL_LIL_ENDIAN)
	if (channels == 4)
		surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, bitdepth * 4,
			0xff000000, 0x00ff0000, 0x0000ff00, 0x000000ff);
	else
		surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, bitdepth * channels,
			0x00ff0000, 0x0000ff00, 0x000000ff, 0);
#else
	surface = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, bitdepth * channels,
		0x000000ff, 0x0000ff00, 0x00ff0000, channels == 4 ? 0xff000000 : 0);
#endif
	if (surface && !colorkey)
	{
		SetColorKey(surface, SDL_TRUE,
			SDL_MapRGB(surface->format, Uint8(trans_values->red),
				Uint8(trans_values->green), Uint8(trans_values->blue)));
		SDL_SetSurfaceRLE(surface, 1);
	}

	// Read image directly into the SDL surface
	png_bytep *pRows = new png_bytep[height];
	pRows_v = pRows;  surface_v = surface; //update volatile variables for possible longjmp
	if (!surface || !pRows) goto CS_error;
	if (SDL_MUSTLOCK(surface)) SDL_LockSurface(surface);
	{
	 png_uint_32 rowno;
	 png_bytep rowdata;
	 for (rowno = 0, rowdata = (png_bytep)surface->pixels; rowno < height;
			++rowno, rowdata += surface->pitch)
		pRows[rowno] = rowdata;
	}
	png_read_image(pPNG, pRows);
	png_read_end(pPNG, pInfo);
	if (SDL_MUSTLOCK(surface)) SDL_UnlockSurface(surface);
	delete [] pRows;
	pRows = NULL;

	// Fix palette
	if (surface->format->palette)
	{
		SDL_Color *colors = surface->format->palette->colors;
		png_colorp palette;
		int i;

		if (colortype == PNG_COLOR_TYPE_GRAY)
		{
			i = surface->format->palette->ncolors = 256;
			while (i--)
				colors[i].b = colors[i].g = colors[i].r = i;
		}
		else if (png_get_PLTE(pPNG, pInfo, &palette, &i) && i > 0)
		{
			surface->format->palette->ncolors = i;
			while (i--)
			{
				colors[i].r = palette[i].red;
				colors[i].g = palette[i].green;
				colors[i].b = palette[i].blue;
			}
		}
	}

	png_destroy_read_struct(&pPNG, &pInfo, (png_infopp)NULL);
	return surface;
#else // SUPPORT_PNG
	return NULL;
#endif // SUPPORT_PNG
}

//*****************************************************************************
bool CPNGHandler::Write(
//Compress an SDL_Surface (with subregion optionally specified) into a PNG file.
//
//Returns: whether operation succeeded
//
//Params:
	const WCHAR* wszFile,  //output filename
	SDL_Surface* pSurface, //image to output
	SDL_Rect* pRect) //if not NULL (default), use as image output region
{
#ifdef SUPPORT_PNG
	if (!pSurface || !wszFile)
		return false;

	FILE *fp = CFiles::Open(wszFile, "wb");
	if (!fp)
		return false;

	int transforms =
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		PNG_TRANSFORM_IDENTITY;
#else
		PNG_TRANSFORM_BGR;
#endif

	bool bSuccess = true;
	png_bytep *wrow_pointers=NULL;
	png_structp png_ptr=NULL;
	png_infop info_ptr=NULL;

	int colortype = PNG_COLOR_MASK_COLOR; // grayscale not supported

	const UINT wBPP = pSurface->format->BytesPerPixel;
	int x, y;
	UINT width, height;
	if (pRect)
	{
		x = pRect->x;
		y = pRect->y;
		width = pRect->w;
		height = pRect->h;

		//Bounds checking.
		if (width > (UINT)pSurface->w)
			width = pSurface->w;
		if (height > (UINT)pSurface->h)
			height = pSurface->h;
		if (x < 0)
		{
			width += x;
			x = 0;
		}
		if (y < 0)
		{
			height += y;
			y = 0;
		}
		if (x >= pSurface->w || y >= pSurface->h)
		{
			bSuccess = false; //out of bounds -- nothing to output
			goto CleanUp;
		}
	} else {
		x = y = 0;
		width = pSurface->w;
		height = pSurface->h;
	}

	if (SDL_MUSTLOCK(pSurface))
		SDL_LockSurface(pSurface);

	png_ptr=png_create_write_struct(PNG_LIBPNG_VER_STRING,
			NULL, NULL, NULL);
	if (!png_ptr)
	{
		bSuccess = false;
		goto CleanUp;
	}

	info_ptr=png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		bSuccess = false;
		goto CleanUp;
	}

	//Prepare for output.
	//Set a pointer to the starting byte of each row of the image.
	wrow_pointers = new png_bytep[height];
	if (!wrow_pointers)
	{
		bSuccess = false;
		goto CleanUp;
	}

	{
		wrow_pointers[0] = (png_bytep)pSurface->pixels + y * pSurface->pitch + (x * wBPP);
		for (UINT i=1; i<height; ++i)
			wrow_pointers[i] = wrow_pointers[i-1] + pSurface->pitch;
	}

	if (pSurface->format->palette)
	{
		colortype |= PNG_COLOR_MASK_PALETTE;
		//!!tofix: not fully supported
	}
	else if (pSurface->format->BitsPerPixel == 32)
	{
		colortype |= PNG_COLOR_MASK_ALPHA;
		transforms |= PNG_TRANSFORM_INVERT_ALPHA;
	}

	//Output.
	png_init_io(png_ptr, fp);
	png_set_IHDR(png_ptr, info_ptr,
			width, height,
			8, //color bit depth
			colortype,
			PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_DEFAULT,
			PNG_FILTER_TYPE_DEFAULT);
	png_set_rows(png_ptr, info_ptr, wrow_pointers);
	png_write_png(png_ptr, info_ptr, transforms, NULL);
CleanUp:
	png_destroy_write_struct(&png_ptr, &info_ptr);
	delete[] wrow_pointers;
	fclose(fp);
	if (SDL_MUSTLOCK(pSurface))
		SDL_UnlockSurface(pSurface);

	return bSuccess;
#else
	return false;
#endif
}

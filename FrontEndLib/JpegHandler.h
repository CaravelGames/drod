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

#ifndef JPEGHANDLER_H
#define JPEGHANDLER_H

#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>

struct SDL_Surface;
class CJpegHandler
{
public:
	CJpegHandler() {}

	static bool Compress(const WCHAR *wszName, SDL_Surface *pSurface, const int quality=100);
	static bool Compress(const WCHAR *wszName, BYTE* imageBuffer,
			const UINT wWidth, const UINT wHeight, const int quality=100);

	static bool Decompress(const WCHAR *wszName,
			BYTE* &imageBuffer, UINT &wWidth, UINT &wHeight);
	static bool Decompress(BYTE* inBuffer, const UINT wInSize,
			BYTE* &imageBuffer, UINT &wWidth, UINT &wHeight);
};

#endif //...#ifndef JPEGHANDLER_H

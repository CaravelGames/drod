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

#include "Ports.h"
#include "Types.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#ifdef WIN32
#  include <sstream>
#  include <string>
#  include <algorithm>
#endif

char* writeInt32 (char* buf, size_t bufsize, int32_t i)
{
	_snprintf(buf, bufsize, "%d", i);
#ifdef WIN32
	buf[bufsize-1] = 0;
#endif
	return buf;
}

char* writeUint32 (char* buf, size_t bufsize, uint32_t i)
{
	_snprintf(buf, bufsize, "%u", i);
#ifdef WIN32
	buf[bufsize-1] = 0;
#endif
	return buf;
}

char* writeTimeT (char* buf, size_t bufsize, time_t i)
{
	_snprintf(buf, bufsize, "%lld", (long long)i);
#ifdef WIN32
	buf[bufsize-1] = 0;
#endif
	return buf;
}

#ifndef WIN32

#if !defined __linux__ && !defined __FreeBSD__ && !defined __APPLE__
WCHAR * _itow(int value, WCHAR *buffer, int radix)
{
	if (NULL == buffer) buffer = (WCHAR*)(malloc(sizeof(WCHAR) * 33));
	std::wostringstream oss;
	oss << value << std::ends;

	std::wstring resultWstring(oss.str());
	for (size_t i = 0; i != resultWstring.length(); ++i) {
	        WCv(buffer[i]) = resultWstring[i];
		// This does not work for characters over 0x7fff
	}
	//std::copy(resultWstring.begin(), resultWstring.end(), buffer);
	return buffer;
}
#else
//Horrible hack, but hey, it works
WCHAR * _itow(int value, WCHAR *buffer, int radix)
{
	if (NULL == buffer) buffer = (WCHAR*)(malloc(sizeof(WCHAR) * 33));
	char *chbuf = (char*)buffer;
	_itoa(value, chbuf, radix);
	UINT i = strlen(chbuf);
	do { WCv(buffer[i]) = chbuf[i]; } while (i--);
	return buffer;
}
#endif // #ifndef __linux__ && !defined __FreeBSD__ && !defined __APPLE__

char* _itoa(int value, char* buffer, int radix)
{
	assert(radix > 0);
	//Passing in a NULL string allocates a string, to be deallocated by the caller.
	if (NULL == buffer) buffer = (char*)(malloc(sizeof(char) * 33));

	//Handle negative numbers.
	bool bNegative = false;
	if (value < 0) {bNegative = true; value = -value;}

	UINT i=0;
	do {
		const UINT val = value % radix;
		buffer[i++] = val < 10 ? val + '0' : val - 10 + 'A';
	} while ((value /= radix) > 0);

	if (bNegative) buffer[i++] = '-';
	buffer[i] = '\0';

	//Reverse string.
	char c;
	UINT j=i-1;
	for (i=0; i<j; ++i, --j) {
		c = buffer[i];
		buffer[i] = buffer[j];
		buffer[j] = c;
	}
	return buffer;
}

#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)

void LittleToBig(char *pBuffer, int bytesPer, int count)
{
	char* psz = pBuffer;
	char c;
	for (int n=0; n < count * bytesPer; n += bytesPer)
	{
		for (int b=0; b < bytesPer/2; b++)
		{
			c = psz[n+b];
			psz[n+b] = psz[n+bytesPer-1-b];
			psz[n+bytesPer-1-b] = c;
		}
	}
}

void LittleToBig(USHORT *pBuffer, int count)
{
	LittleToBig((char*)pBuffer, sizeof(USHORT), count);
}

void LittleToBig(UINT *pBuffer, int count)
{
	LittleToBig((char*)pBuffer, sizeof(UINT), count);
}

void LittleToBig(int *pBuffer, int count)
{
	LittleToBig((char*)pBuffer, sizeof(int), count);
}

void LittleToBig(WCHAR *pBuffer, int count)
{
	LittleToBig((char*)(&pWCv(pBuffer)), sizeof(WCHAR), count);
}

#endif // big endian

#ifndef USE_CXX11

WCHAR towlower(const WCHAR ch)
{
  WCHAR_t c = WCv(ch);
  WCHAR r = W_t(tolower(c));
  return r;
}

bool iswlower(const WCHAR ch)
{
  WCHAR_t c = WCv(ch);
  return islower(static_cast<char>(c));
}

bool iswupper(const WCHAR ch)
{
  WCHAR_t c = WCv(ch);
  return isupper(static_cast<char>(c));
}

bool iswspace(const WCHAR ch)
{
	WSTRING wsz;
	// Any other spaces ?
	AsciiToUnicode(" \t\r\n", wsz);
	return wsz.find(ch, 0) != WSTRING::npos;
}

bool iswdigit(const WCHAR ch)
{
	WSTRING wsz;
	// Any other digits ?
	AsciiToUnicode("0123456789", wsz);
	return wsz.find(ch, 0) != WSTRING::npos;
}

bool iswxdigit(const WCHAR ch)
{
	WSTRING wsz;
	AsciiToUnicode("0123456789abcdefABCDEF", wsz);
	return wsz.find(ch, 0) != WSTRING::npos;
}

bool iswalpha(const WCHAR ch)
{
	return isalpha(static_cast<char>(WCv(ch)));
}

bool iswalnum(const WCHAR ch)
{
	return isalnum(static_cast<char>(WCv(ch)));
}

#endif // !c++11
#endif // !WIN32

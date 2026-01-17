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
 * Portions created by the Initial Developer are Copyright (C) 2003, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Matt Schikore (schik)
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "Wchar.h"
#include "Assert.h"
#include "Types.h"
#include "Ports.h"  // for towlower, iswspace
#include <cstdio>
#include <cstring>
#include <sstream>

//Some common small strings.  Larger strings are localizable and should be kept in database.
const WCHAR wszAmpersand[] =    { We('&'),We(0) };
const WCHAR wszAsterisk[] =     { We('*'),We(0) };
const WCHAR wszOpenAngle[] =    { We('<'),We(0) };
const WCHAR wszCloseAngle[] =   { We('>'),We(0) };
const WCHAR wszColon[] =        { We(':'),We(0) };
const WCHAR wszComma[] =        { We(','),We(0) };
const WCHAR wszCommaSpace[] =   { We(','),We(' '),We(0) };
#ifdef WIN32
const WCHAR wszCRLF[] =         { We('\r'),We('\n'),We(0) };
#else
const WCHAR wszCRLF[] =         { We('\n'),We(0) };
#endif
const WCHAR wszDollarSign[] =   { We('$'),We(0) };
const WCHAR wszElipsis[] =      { We('.'),We('.'), We('.'),We(0) };
const WCHAR wszEmpty[] =        { We(0) };
const WCHAR wszEqual[] =        { We('='),We(0) };
const WCHAR wszExclamation[] =  { We('!'),We(0) };
const WCHAR wszForwardSlash[] = { We('/'),We(0) };
const WCHAR wszTimes[] =        { We('x'),We(0) };
const WCHAR wszHyphen[] =       { We('-'),We(0) };
const WCHAR wszTilde[] =        { We('~'),We(0) };
const WCHAR wszParentDir[] =    { We('.'),We('.'),We(0) };
const WCHAR wszPercent[] =      { We('%'),We(0) };
const WCHAR wszPeriod[] =       { We('.'),We(0) };
const WCHAR wszPoundSign[] =    { We('#'),We(0) };
const WCHAR wszPlus[] =         { We('+'),We(0) };
const WCHAR wszQuestionMark[] = { We('?'),We(0) };
const WCHAR wszQuote[] =        { We('\"'),We(0) };
const WCHAR wszLeftBracket[] =  { We('['),We(0) };
const WCHAR wszRightBracket[] = { We(']'),We(0) };
const WCHAR wszLeftParen[] =    { We('('),We(0) };
const WCHAR wszRightParen[] =   { We(')'),We(0) };
const WCHAR wszSemicolon[] =    { We(';'),We(0) };
const WCHAR wszSpace[] =        { We(' '),We(0) };
const WCHAR wszUnderscore[] =   { We('_'),We(0) };
const WCHAR wszZero[] =         { We('0'),We(0) };
const WCHAR wszStringToken[] =  { We('%'),We('s'),We(0) };
const WCHAR wszOne[] =          { We('1'),We(0) };
const WCHAR wszTwo[] =          { We('2'),We(0) };
#ifdef WIN32
const WCHAR wszSlash[] = { We('\\'),We(0) };
#else
const WCHAR wszSlash[] = { We('/'),We(0) };
#endif

//HTML formatting strings.
const WCHAR wszHtml[] = { We('<'),We('h'),We('t'),We('m'),We('l'),We('>'), We(0) };
const WCHAR wszEndHtml[] = { We('<'),We('/'),We('h'),We('t'),We('m'),We('l'),We('>'), We(0) };
const WCHAR wszBody[] = { We('<'),We('b'),We('o'),We('d'),We('y'),We('>'), We(0) };
const WCHAR wszEndBody[] = { We('<'),We('/'),We('b'),We('o'),We('d'),We('y'),We('>'), We(0) };
const WCHAR wszHeader[] = { We('<'),We('h'),We('3'),We('>'), We(0) };
const WCHAR wszEndHeader[] = { We('<'),We('/'),We('h'),We('3'),We('>'), We(0) };
const WCHAR wszBTag[] = { We('<'),We('b'),We('>'), We(0) };
const WCHAR wszEndBTag[] = { We('<'),We('/'),We('b'),We('>'), We(0) };
const WCHAR wszITag[] = { We('<'),We('i'),We('>'), We(0) };
const WCHAR wszEndITag[] = { We('<'),We('/'),We('i'),We('>'), We(0) };
const WCHAR wszPTag[] = { We('<'),We('p'),We('>'), We(0) };

static char buffer[WCHAR_INTERNAL_BUFFER];

//*****************************************************************************
void SanitizeSingleLineString(WSTRING &wstr)
//Remove any unwanted characters from a single-line string. Currently strips:
// * both newline characters
// * horizontal tab
// Ultimately if one day we can define the list of allowed characters we can convert it to a whitelist
{
	wstr.erase(std::remove(wstr.begin(), wstr.end(), '\n'), wstr.end());
	wstr.erase(std::remove(wstr.begin(), wstr.end(), '\r'), wstr.end());
	wstr.erase(std::remove(wstr.begin(), wstr.end(), '\t'), wstr.end());
}

//*****************************************************************************
void SanitizeMultiLineString(WSTRING &wstr)
//Remove any unwanted characters from a multi-line string. Currently strips:
// * horizontal tab
{
	wstr.erase(std::remove(wstr.begin(), wstr.end(), '\t'), wstr.end());
}

//*****************************************************************************
void AsciiToUnicode(const char *psz, WSTRING &wstr)
{
	WCHAR *pwczConvertBuffer = new WCHAR[strlen(psz) + 1];

	const unsigned char *pszRead = (unsigned char*)psz;
	WCHAR *pwczWrite = pwczConvertBuffer;
	while (*pszRead != '\0')
	{
		pWCv(pwczWrite++) = static_cast<WCHAR_t>(*(pszRead++));
	}
	pWCv(pwczWrite) = 0;

	wstr = pwczConvertBuffer;
	delete[] pwczConvertBuffer;
}

//*****************************************************************************
void AsciiToUnicode(const std::string &str, WSTRING &wstr) {
	AsciiToUnicode(str.c_str(), wstr);
}

//*****************************************************************************
const WSTRING AsciiToUnicode(const char *psz) {
	WSTRING res;
	AsciiToUnicode(psz, res);
	return res;
}

//******************************************************************************
void CTextToUnicode(const char *psz, WSTRING &wstr)
{
	UINT len = wstr.length();
	if (!UTF8ToUnicode(psz, wstr))
	{
		wstr.resize(len);
		AsciiToUnicode(psz, wstr);
	}
}

//******************************************************************************
bool UTF8ToUnicode(const char *psz, WSTRING &wstr)
{
	return UTF8ToUnicode(psz, strlen(psz), wstr);
}

//*****************************************************************************
bool UnicodeToAscii(const WSTRING& wstr, char *psz)
{
	WCHAR_t w = 0;
	unsigned char* pszWrite = (unsigned char*)psz;
	for (WSTRING::const_iterator i = wstr.begin(); i != wstr.end(); ++i)
	{
		*(pszWrite++) = (unsigned char)pWCv(i);
		w |= pWCv(i);
	}
	*(pszWrite++) = 0;
	return !(w & 0xff00);
}

//*****************************************************************************
bool UnicodeToAscii(const WSTRING& wstr, string &str)
{
	WCHAR_t w = 0;
	str.clear();
	str.reserve(wstr.size());
	for (WSTRING::const_iterator i = wstr.begin(); i != wstr.end(); ++i)
	{
		str += (unsigned char)pWCv(i);
		w |= pWCv(i);
	}
	return !(w & 0xff00);
}
//*****************************************************************************
bool UnicodeToAscii(const WCHAR* wsz, string &str)
{
	WCHAR_t w = 0;
	UINT len = WCSlen(wsz);
	str.clear();
	str.reserve(len);
	for (UINT c = 0; c < len; c++)
	{
		str += (unsigned char)WCv(wsz[c]);
		w |= WCv(wsz[c]);
	}
	return !(w & 0xff00);
}

//*****************************************************************************
string UnicodeToAscii(const WCHAR* wsz)
{
	string str;
	UnicodeToAscii(wsz, str);
	return str;
}

//*****************************************************************************
void UnicodeToUTF8(const WCHAR *pwsz, string &str)
{
	BYTE* bytes = NULL;
	UINT len = to_utf8(pwsz, bytes);
	str.clear();
	str.reserve(len);
	for (UINT i = 0; i < len; ++i)
		str += bytes[i];
	delete[] bytes;
}

//*****************************************************************************
#define MB_BLOCK_BEGIN(mask,cmp) \
	ucs4 = (ch & mask) << 6; \
	if ((ch = *++*ppsz) < cmp && !ucs4) \
		success = false; \
	if (ch) do \
	{ \
		ucs4 += (ch & 0x3f)

#define MB_BLOCK_END \
	} while (0)

#define ADD_NEXT_BYTE \
	if ((ch = *++*ppsz)) \
	{ \
		ucs4 <<= 6; \
		ucs4 += (ch & 0x3f); \
	} \
	else return 0xffffffff

unsigned int UTF8ToUCS4Char(
//Convert UTF-8 to UCS-4 (Unicode). (Based on info from RFC 2279.)
//
//Params:
	const char **ppsz)   //(in/out)  Pointer to pointer to UTF-8 char-data.
								//          The size of the char-data is determined
								//          automagically, so no need for 0-termination.
								//          The pointer will be updated to point to the
								//          end of the current UTF-8 char after the
								//          call, even if it was an invalid char.  If a
								//          zero byte is encountered, conversion will
								//          abort and the pointer will point to that.
//
//Returns:
//UCS-4 char, or 0xffffffff if the UTF-8 code was invalid.
{
	ASSERTP(ppsz && *ppsz, "NULL pointer passed to UTF8ToUCS4Char");

	unsigned char ch = **ppsz;
	bool success = true;
	UINT ucs4 = 0;

	// Could be done with a loop, but this is faster
	if (ch < 0x80)      // 00-7f: 1 byte
		ucs4 = ch;
	else if (ch < 0xc2) // 80-c1: error
		success = false;
	else if (ch < 0xe0) // c2-df: 2 bytes
	{
		ucs4 = (ch & 0x1f);
		ADD_NEXT_BYTE;
	}
	else if (ch < 0xf0) // e0-ef: 3 bytes
	{
		MB_BLOCK_BEGIN(0x0f,0xa0);
			ADD_NEXT_BYTE;
		MB_BLOCK_END;
	}
	else if (ch < 0xf8) // f0-f7: 4 bytes
	{
		MB_BLOCK_BEGIN(0x07,0x90);
			ADD_NEXT_BYTE;
			ADD_NEXT_BYTE;
		MB_BLOCK_END;
	}
	else if (ch < 0xfc) // f8-fb: 5 bytes
	{
		MB_BLOCK_BEGIN(0x03,0x88);
			ADD_NEXT_BYTE;
			ADD_NEXT_BYTE;
			ADD_NEXT_BYTE;
		MB_BLOCK_END;
	}
	else if (ch < 0xfe) // fc-fd: 6 bytes
	{
		MB_BLOCK_BEGIN(0x01,0x84);
			ADD_NEXT_BYTE;
			ADD_NEXT_BYTE;
			ADD_NEXT_BYTE;
			ADD_NEXT_BYTE;
		MB_BLOCK_END;
	}
	else                // fe-ff: error
		success = false;

	return success ? ucs4 : 0xffffffff;
}
#undef MB_BLOCK_BEGIN
#undef MB_BLOCK_END
#undef ADD_NEXT_BYTE

//*****************************************************************************
bool UTF8ToUnicode(const char* s, const UINT len, WSTRING &wstr)
{
	bool ok = true;
	wstr.resize(0);
	wstr.reserve(len);
	for (const char *s_end = s + len; s < s_end; ++s)
	{
		//UTF-8 to UTF-16 conversion
		UINT wc = UTF8ToUCS4Char(&s);
		if (wc > 0x10ffff || (wc >= 0xd800 && wc <= 0xdfff))
		{
			// unknown unicode character
			wstr += W_t(0xfffd);
			ok = false;
		}
		else if (wc >= 0x10000)
		{
			// surrogate pair
			wc -= 0x10000;
			wstr += W_t(0xd800 + (wc >> 10));
			wstr += W_t(0xdc00 + (wc & 0x3ff));
		}
		else
		{
			wstr += W_t(wc);
		}
	}
	return ok;
}

//*****************************************************************************
void UTF8ToAscii(const char* s, const UINT len, string &str)
{
	str.resize(0);
	str.reserve(len);
	for (const char *s_end = s + len; s < s_end; ++s)
	{
		// UTF-8 to ASCII conversion (UCS-4 is just stripped)
		str += (unsigned char)UTF8ToUCS4Char(&s);
	}
}

//*****************************************************************************
UINT utf8len(const WCHAR* pwczText)
//Returns: byte length of 16-bit Unicode string, converted to utf-8
{
	UINT wLength=0;
	while (WCv(*pwczText))
	{
		const UINT ch=WCv(*pwczText++);
		if (ch <= 0x7F)
			++wLength;
		else if (ch <= 0x7FF)
			wLength+=2;
		else if (ch <= 0xFFFF)
			wLength+=3;
		else if (ch <= 0x1FFFFF)
			wLength+=4;
		else if (ch <= 0x3FFFFFF)
			wLength+=5;
		else
			wLength+=6;
	}
	return wLength;
}

//*****************************************************************************
UINT utf8len(const char* pczText)
//Returns: byte length of ASCII string, converted to utf-8
{
	UINT wLength;
	for (wLength = 0; *pczText; ++pczText)
	{
		const unsigned char ch=*((unsigned char*)pczText);
		if (ch <= 0x7F)
			++wLength;
		else wLength+=2;
	}
	return wLength;
}

//*****************************************************************************
UINT to_utf8(const WCHAR *pwStr, BYTE* &pbOutStr)
//OUT: allocated byte buffer of Unicode-2 text passed in, converted to utf-8.
//Caller must delete allocated byte buffer.
{
	ASSERT(pwStr);
	ASSERT(!pbOutStr);

	const UINT wByteLength = utf8len(pwStr);
	pbOutStr = new BYTE[wByteLength+1]; //null terminated
	BYTE *res = pbOutStr;

	while (WCv(*pwStr))
	{
		//Convert one Unicode character and append to byte buffer.
		const UINT ch=WCv(*pwStr++);
		if (ch <= 0x7F) {
			*res++=static_cast<char unsigned>(ch);
		} else if (ch <= 0x7FF) {
			*res++=static_cast<char unsigned>(0xC0 | ((ch >> 6) & 0x1F));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 0) & 0x3F));
		} else if (ch <= 0xFFFF) {
			*res++=static_cast<char unsigned>(0xE0 | ((ch >> 12) & 0x0F));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 6) & 0x3F));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 0) & 0x3F));
		} else if (ch <= 0x1FFFFF) {
			*res++=static_cast<char unsigned>(0xF0 | ((ch >> 18) & 0x07));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 12) & 0x3F));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 6) & 0x3F));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 0) & 0x3F));
		} else if (ch <= 0x3FFFFFF) {
			*res++=static_cast<char unsigned>(0xF8 | ((ch >> 24) & 0x03));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 18) & 0x3F));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 12) & 0x3F));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 6) & 0x3F));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 0) & 0x3F));
		} else {
			*res++=static_cast<char unsigned>(0xFC | ((ch >> 30) & 0x03));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 24) & 0x3F));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 18) & 0x3F));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 12) & 0x3F));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 6) & 0x3F));
			*res++=static_cast<char unsigned>(0x80 | ((ch >> 0) & 0x3F));
		}
	}
	*res = 0;

	return wByteLength;
}

//*****************************************************************************
UINT to_utf8(const char *pStr, BYTE* &pbOutStr)
//OUT: allocated byte buffer of ASCII text passed in, converted to utf-8.
//Caller must delete allocated byte buffer.
{
	ASSERT(pStr);
	ASSERT(!pbOutStr);

	const UINT wByteLength = utf8len(pStr);
	pbOutStr = new BYTE[wByteLength+1]; //null terminated
	BYTE *res = pbOutStr;

	for (; *pStr; ++pStr)
	{
		//Convert one Unicode character and append to byte buffer.
		const unsigned char ch=*((unsigned char*)pStr);
		if (ch <= 0x7f) {
			*res++ = ch;
		} else {
			*res++ = 0xc0 | ((ch >> 6) & 0x1f);
			*res++ = 0x80 | ((ch >> 0) & 0x3f);
		}
	}
	*res = 0;

	return wByteLength;
}

//*****************************************************************************
bool charFilenameSafe(const WCHAR wc)
//Returns: whether char can be used in filenames
//
//ASSUME: alphanumeric and space and some punctuation are usable
{
	const WCHAR lwc = towlower(wc);
	if (wc >= 256)
		return true; //assume Unicode chars are usable
	return
		(lwc >= 'a' && lwc <= 'z') ||
		(lwc >= '0' && lwc <= '9') ||
		lwc == ' ' || lwc == '-' || lwc == '_';
}

//*****************************************************************************
WSTRING filenameFilter(const WSTRING &wstr)
//Returns: input string with all characters, unusable in filenames, removed
{
	WSTRING filteredStr;
	for (WSTRING::const_iterator i = wstr.begin(); i != wstr.end(); ++i)
	{
		const WCHAR wc = (WCHAR)*i;
		if (charFilenameSafe(wc))
			filteredStr += wc;
	}
	return filteredStr;
}

//*****************************************************************************
WSTRING filterFirstLettersAndNumbers(const WSTRING &wstr)
//Returns: string with only the first letter from each word, and digits
{
	WSTRING filteredStr;
	bool bAfterWhiteSpace = true;
	for (WSTRING::const_iterator i = wstr.begin(); i != wstr.end(); ++i)
	{
		const WCHAR wc = (WCHAR)*i;
		if (iswspace(wc))
			bAfterWhiteSpace = true;
		else if ((bAfterWhiteSpace && iswalpha(wc)) || iswdigit(wc))
		{
			filteredStr += wc;
			bAfterWhiteSpace = false;
		}
	}
	return filteredStr;
}

//*****************************************************************************
WSTRING filterUpperCase(const WSTRING &wstr)
//Returns: string with only upper-case characters
{
	WSTRING filteredStr;
	for (WSTRING::const_iterator i = wstr.begin(); i != wstr.end(); ++i)
	{
		const WCHAR wc = (WCHAR)*i;
		if (iswupper(wc))
			filteredStr += wc;
	}
	return filteredStr;
}

//*****************************************************************************
WCHAR* getFilenameFromPath(const WCHAR *wstrFilepath) //(in) file path + name
//Returns: pointer to filename part of path only
{
	const WCHAR *wszFilename = wstrFilepath;
	while (*wszFilename != wszEmpty[0]) ++wszFilename; //Search to end.
	while (wszFilename >= wstrFilepath && *wszFilename != wszSlash[0])
		--wszFilename; //Search backwards to last backslash.
	if (*wszFilename == wszSlash[0])
		++wszFilename;  //go past backslash

	return (WCHAR*)wszFilename;
}

//*****************************************************************************
bool isWInteger(const WCHAR* wcz)
//Returns: whether the text is an integer of a form accepted by _Wtoi,
//i.e. "[whitespace] [+|-] digits [whitespace]",
//with nothing else afterwards.
{
	string str;
	UnicodeToAscii(wcz, str);
	return isInteger(str.c_str());
}

//*****************************************************************************
bool isInteger(const char* pcz)
//Returns: whether the text is an integer of a form accepted by atoi,
//i.e. "[whitespace] [+|-] digits [whitespace]",
//with nothing else afterwards.
{
	int index=0;

	while (isspace(pcz[index]))
		++index;
	if (pcz[index] == '-' || pcz[index] == '+')
		++index;
	if (!isdigit(pcz[index]))
		return false;
	++index; //we just encountered a digit, so don't need to check it again
	while (isdigit(pcz[index]))
		++index;
	while (isspace(pcz[index])) //allow trailing whitespace
		++index;

	return !pcz[index]; //if nothing is left, this string is only an integer
}

//*****************************************************************************
int _Wtoi(const WCHAR* wsz)
{
	string str;
	UnicodeToAscii(wsz, str);
	return atoi(str.c_str());
}

//*****************************************************************************
WCHAR* _itoW(int value, WCHAR* wcz, int radix, int bufferLength)
{
	// Internal buffer's max length should never ever be exceeded
	bufferLength = min(WCHAR_INTERNAL_BUFFER, bufferLength);

	int i = 0;
	bool bNegative = false;
	if (value < 0) //negative?
	{
		value = -value;
		bNegative = true;
		bufferLength--; // We have one character fewers for numbers because we need negative sign
	}
	bufferLength--; // Make space for null termination char
	do {
		if (i >= bufferLength) {
			ASSERT(!"_itoW: ran out of space in the provided buffer - it will be trimmed.");
			break;
		}

		int val = value % radix;
		if (val < 10)
			buffer[i++] = (value % radix) + '0';
		else buffer[i++] = (value % radix) - 10 + 'A';

	} while ((value /= radix) > 0);
	if (bNegative)
		buffer[i++] = '-';

	buffer[i] = '\0';

	char c;
	int j;
	for (i = 0, j = strlen(buffer)-1; i<j; ++i, --j) {
		c = buffer[i];
		buffer[i] = buffer[j];
		buffer[j] = c;
	}
	WSTRING wStr;
	AsciiToUnicode(buffer, wStr);
	WCScpy(wcz, wStr.c_str());
	return wcz;
}

//*****************************************************************************
UINT WCSlen(const WCHAR* wsz)
{
	UINT length = 0;
	const WCHAR* wszRead = wsz;
	while (*wszRead++ != 0)
		++length;

	return length;
}

//*****************************************************************************
WCHAR* WCScpy(WCHAR* pwczDest, const WCHAR* pwczSrc)
{
	UINT index = 0;
	while (pwczSrc[index] != 0)
		++index;

	memcpy(pwczDest, pwczSrc, (index+1) * sizeof(WCHAR));
	return pwczDest;
}

//*****************************************************************************
WCHAR* WCSncpy(WCHAR* pwczDest, const WCHAR* pwczSrc, UINT n)
{
	WCHAR *pwczWrite = pwczDest;
	const WCHAR *pwczRead = pwczSrc;

	while ((*pwczRead != 0) && n--)
	{
		*(pwczWrite++) = *(pwczRead++);
	}
	pWCv(pwczWrite) = 0;
	return pwczDest;
}

//*****************************************************************************
WCHAR* WCScat(WCHAR *pwcz1, const WCHAR *pwcz2)
{
	WCHAR *pwczWrite = pwcz1;
	while (*pwczWrite != 0)
		++pwczWrite;

	const WCHAR *pwczRead = pwcz2;
	while (*pwczRead != 0)
		*(pwczWrite++) = *(pwczRead++);
	pWCv(pwczWrite) = 0;

	return pwcz1;
}

//*****************************************************************************
int WCScmp(const WCHAR* pwcz1, const WCHAR* pwcz2)
{
	const UINT len1 = WCSlen(pwcz1);
	const UINT len2 = WCSlen(pwcz2);
	UINT index = 0;

	while (index < len1 && index < len2) {
		if (pwcz1[index] < pwcz2[index]) return -1;
		if (pwcz2[index] < pwcz1[index]) return 1;
		++index;
	}
	// If we get to this point, either they're the same or one string
	// is longer than the other.
	if (len1 == len2) return 0;
	if (len1 < len2) return -1;
	return 1;
}

//*****************************************************************************
int WCSicmp(const WCHAR* pwcz1, const WCHAR* pwcz2)
{
	const UINT len1 = WCSlen(pwcz1);
	const UINT len2 = WCSlen(pwcz2);
	UINT index = 0;

	while (index < len1 && index < len2) {
		if (towlower(pwcz1[index]) < towlower(pwcz2[index])) return -1;
		if (towlower(pwcz2[index]) < towlower(pwcz1[index])) return 1;
		++index;
	}
	// If we get to this point, either they're the same or one string
	// is longer than the other.
	if (len1 == len2) return 0;
	if (len1 < len2) return -1;
	return 1;
}

//*****************************************************************************
int WCSncmp(const WCHAR* pwcz1, const WCHAR* pwcz2, const UINT n)
{
	UINT len1 = 0, len2 = 0;
	while (len1 < n && pwcz1[len1] != 0) ++len1;
	while (len2 < n && pwcz2[len2] != 0) ++len2;
	UINT index = 0;

	while (index < len1 && index < len2 && index < n) {
		if (pwcz1[index] < pwcz2[index]) return -1;
		if (pwcz2[index] < pwcz1[index]) return 1;
		++index;
	}
	// If we get to this point, either n chars have been matched,
	// or they're the same, or one string is longer than the other.
	if (index == n) return 0;
	if (len1 == len2) return 0;
	if (len1 < len2) return -1;
	return 1;
}

//*****************************************************************************
int WCSnicmp(const WCHAR* pwcz1, const WCHAR* pwcz2, const UINT n)
{
	UINT len1 = 0, len2 = 0;
	while (len1 < n && pwcz1[len1] != 0) ++len1;
	while (len2 < n && pwcz2[len2] != 0) ++len2;
	UINT index = 0;

	while (index < len1 && index < len2 && index < n) {
		if (towlower(pwcz1[index]) < towlower(pwcz2[index])) return -1;
		if (towlower(pwcz2[index]) < towlower(pwcz1[index])) return 1;
		++index;
	}
	// If we get to this point, either n chars have been matched,
	// or they're the same, or one string is longer than the other.
	if (index == n) return 0;
	if (len1 == len2) return 0;
	if (len1 < len2) return -1;
	return 1;
}

//*****************************************************************************
WCHAR* WCStok(WCHAR *wszStart, const WCHAR *wszDelim)
{
	static WCHAR *wszNext = NULL;

	if ((NULL == wszStart) && (NULL == (wszStart = wszNext)))
		return NULL;

	UINT dindex;

	// Skip initial delimiters
	for (; pWCv(wszStart); ++wszStart)
	{
		for (dindex = 0;
				wszDelim[dindex] && (*wszStart != wszDelim[dindex]);
				++dindex)
		{ }

		if (!wszDelim[dindex]) break;  // No more initial delimiters
	}

	// Now hunt for next delimiters
	for (wszNext = wszStart; pWCv(wszNext); ++wszNext)
	{
		for (dindex = 0;
				wszDelim[dindex] && (*wszNext != wszDelim[dindex]);
				++dindex)
		{ }

		if (!wszDelim[dindex]) continue;

		// Found a delimiter: Terminate token and prepare for next (if any)
		pWCv(wszNext++) = 0;
		wszNext = (pWCv(wszNext) ? wszNext : NULL);
		return wszStart;
	}

	// No delimiters, end of string
	wszNext = NULL;
	return wszStart;
}

//*****************************************************************************
WCHAR* fgetWs(WCHAR* pwcz, int n, FILE* pFile)
{
	ASSERT(pwcz != NULL);

	WCHAR c;
	WCHAR* wszWrite = pwcz;
	int numread;
	do {
		numread = fread( &c, sizeof(WCHAR), 1, pFile );
		if (numread == 0)
		{
			//Reached end of file data.
			pWCv(wszWrite++) = '\0';   //null terminate string
			break;
		}
		*(wszWrite++) = c;
		--n;
	} while (c != 0 && n > 0);

	return pwcz;
}

//*****************************************************************************
void fputWs (const WCHAR *wsz, FILE *pFile)
{
	for (; pWCv(wsz); ++wsz)
		putc((unsigned char)pWCv(wsz), pFile);
}

//*****************************************************************************
std::string strReplace(
//Replaces all occurrences of a string within another string.
//Returns: copy of transformed string
	std::string const &source, //Source string.
	std::string const &from,   //The substring to find.
	std::string const &to)     //Replace instances of 'from'.
{
	std::string result;
	std::string::size_type processed=0;
	for (;;) {
		std::string::size_type loc = source.find(from, processed);
		if (loc == std::string::npos)
			return result + source.substr(processed);
		ASSERT(loc>=processed);
		result += source.substr(processed, loc-processed)+to;
		processed = loc + from.size();
	}
}

//*****************************************************************************
WSTRING WCSReplace(
//Replaces all occurrences of a string within another string.
//Returns: copy of transformed string
	WSTRING const &source, //Source string.
	WSTRING const &from,   //The substring to find.
	WSTRING const &to)     //Replace instances of 'from'.
{
	WSTRING result;
	WSTRING::size_type processed=0;
	for (;;) {
		WSTRING::size_type loc = source.find(from, processed);
		if (loc == std::string::npos)
			return result + source.substr(processed);
		ASSERT(loc>=processed);
		result += source.substr(processed, loc-processed)+to;
		processed = loc + from.size();
	}
}

//*****************************************************************************
WSTRING WCSToLower(WSTRING const &source)
//Converts WSTRING to lowercase
{
	WSTRING lowercased;
	lowercased.reserve(source.size());
	for (WSTRING::const_iterator it = source.begin(); it != source.end(); ++it)
		lowercased += towlower(*it);

	return lowercased;
}

//*****************************************************************************
const std::vector<WSTRING> WCSExplode(WSTRING const &source, WCHAR const delimiter)
// Explodes a string into pieces
// Adapted from: https://stackoverflow.com/a/12967010
{
	std::vector<WSTRING> result;
	std::basic_istringstream<WCHAR> iss(source);

	for (WSTRING token; std::getline(iss, token, delimiter); )
	{
		if (!token.empty())
			result.push_back(token);
	}

	return result;
}

//*****************************************************************************
const std::set<WSTRING> WCSExplodeSet(WSTRING const& source, WCHAR const delimiter)
// Explodes a string into pieces, and put them into a set
{
	std::set<WSTRING> result;
	std::basic_istringstream<WCHAR> iss(source);

	for (WSTRING token; std::getline(iss, token, delimiter); )
	{
		if (!token.empty())
			result.insert(token);
	}

	return result;
}

//*****************************************************************************
bool WCSContainsAll(WSTRING const &haystack, std::vector<WSTRING> const &needles)
// Returns true if haystack contains every string in needle, even if they overlap
{
	for (std::vector<WSTRING>::const_iterator iter = needles.begin(); iter < needles.end(); ++iter)
	{
		if (haystack.find(*iter) == WSTRING::npos)
			return false;
	}

	return true;
}

//*****************************************************************************
WSTRING to_WSTRING(int value)
// Returns: string representation of input
{
	WCHAR temp[32];
	_itoW(value, temp, 10, 32);
	WSTRING string(temp);
	return string;
}

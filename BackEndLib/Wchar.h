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
 * Matt Schikore (schik)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef WCHAR_H
#define WCHAR_H

#ifdef WIN32
#pragma warning(disable:4786)
#endif

#include "PortsBase.h"
#include "Types.h"  //need BYTE, UINT

#define STRFY(x) #x
#define STRFY_EXPAND(x) STRFY(x)

#define WCHAR_INTERNAL_BUFFER 512

// C++11 has native 16-bit string support, so use that if we can
#if !defined(USE_CXX11) && (defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__))

#	include "CharTraits.h"

#	define NEWLINE "\n"

#else  //WINDOWS or C++11

#	define WCv(x)  (x)
#	define pWCv(x) (*(x))
#	define W_t(x)  (x)
#	define We(x)   (x)

#define WS(x) WS_(x)  // WS("utf-16 string")

#ifdef USE_CXX11
typedef char16_t        WCHAR_t;
#define WS_(x) u ## x
#elif defined(WIN32)
typedef wchar_t         WCHAR_t;
#define WS_(x) L ## x
#endif
typedef WCHAR_t         WCHAR;    //wc, 16-bit UNICODE character

#	define NEWLINE "\r\n"

#endif

#include <string>
#include <functional>

typedef std::basic_string<WCHAR, std::char_traits<WCHAR>, std::allocator<WCHAR> > WSTRING;

//Some common small strings.  Larger strings are localizable and should be kept in database.
extern const WCHAR wszAmpersand[], wszAsterisk[], wszOpenAngle[], wszCloseAngle[], wszColon[],
	wszComma[], wszCRLF[], wszDollarSign[], wszElipsis[], wszEmpty[], wszEqual[],
	wszExclamation[], wszForwardSlash[], wszHyphen[], wszParentDir[],
	wszPercent[], wszPeriod[], wszPoundSign[], wszPlus[], wszQuestionMark[], wszQuote[],
	wszLeftBracket[], wszRightBracket[], wszLeftParen[], wszRightParen[],
	wszSemicolon[], wszSpace[], wszSlash[], wszUnderscore[], wszZero[];

//HTML formatting strings.
extern const WCHAR wszHtml[], wszEndHtml[], wszBody[], wszEndBody[], wszHeader[],
	wszEndHeader[], wszBTag[], wszEndBTag[], wszITag[], wszEndITag[], wszPTag[];

void AsciiToUnicode(const char *psz, WSTRING &wstr);
void AsciiToUnicode(const std::string& str, WSTRING &wstr);
void CTextToUnicode(const char *psz, WSTRING &wstr);
bool UnicodeToAscii(const WSTRING& wstr, char *psz);
bool UnicodeToAscii(const WSTRING& wstr, std::string &str);
bool UnicodeToAscii(const WCHAR *wsz, std::string &str);
std::string UnicodeToAscii(const WCHAR *wsz);
static inline std::string UnicodeToAscii(const WSTRING& wstr) { return UnicodeToAscii(wstr.c_str()); }
static inline void UnicodeToAscii(const WCHAR *wsz, char *psz)
		{ do { *(psz++) = (char)pWCv(wsz); } while (pWCv(wsz++)); }

void UnicodeToUTF8(const WCHAR *pwsz, std::string &str);
static inline void UnicodeToUTF8(const WSTRING& wstr, std::string &str)
		{ UnicodeToUTF8(wstr.c_str(), str); }
static inline std::string UnicodeToUTF8(const WCHAR* pwsz)
		{ std::string result; UnicodeToUTF8(pwsz, result); return result; }
static inline std::string UnicodeToUTF8(const WSTRING& wstr)
		{ std::string result; UnicodeToUTF8(wstr, result); return result; }

unsigned int UTF8ToUCS4Char(const char **ppsz);
void UTF8ToAscii(const char* s, const UINT len, std::string &str);
UINT utf8len(const WCHAR* pwczText);
UINT utf8len(const char* pczText);
UINT to_utf8(const WCHAR* pwStr, BYTE* &pbOutStr);
UINT to_utf8(const char* pStr, BYTE* &pbOutStr);

bool UTF8ToUnicode(const char *psz, WSTRING &wstr);
bool UTF8ToUnicode(const char *psz, UINT length, WSTRING &wstr);
static inline bool UTF8ToUnicode(const std::string &str, WSTRING &wstr)
		{ return UTF8ToUnicode(str.c_str(), str.length(), wstr); }
static inline WSTRING UTF8ToUnicode(const char* str)
		{ WSTRING result; UTF8ToUnicode(str, result); return result; }
static inline WSTRING UTF8ToUnicode(const std::string &str)
		{ WSTRING result; UTF8ToUnicode(str, result); return result; }


bool charFilenameSafe(const WCHAR wc);
WSTRING  filenameFilter(const WSTRING &wstr);
WSTRING  filterFirstLettersAndNumbers(const WSTRING &wstr);
WSTRING  filterUpperCase(const WSTRING &wstr);
WCHAR* getFilenameFromPath(const WCHAR *wstrFilepath);
bool isWInteger(const WCHAR* wcz);
bool isInteger(const char* pcz);

int      _Wtoi(const WCHAR* wsz);
WCHAR*   _itoW(int value, WCHAR* wcz, int radix, int bufferLength = WCHAR_INTERNAL_BUFFER);

UINT     WCSlen(const WCHAR* wsz)  FUNCATTR_PURE;
int      WCScmp(const WCHAR* pwcz1, const WCHAR* pwcz2)  FUNCATTR_PURE;
int      WCSicmp(const WCHAR* pwcz1, const WCHAR* pwcz2)  FUNCATTR_PURE;
int      WCSncmp(const WCHAR* pwcz1, const WCHAR* pwcz2, const UINT n)  FUNCATTR_PURE;
int      WCSnicmp(const WCHAR* pwcz1, const WCHAR* pwcz2, const UINT n)  FUNCATTR_PURE;
WCHAR*   WCScpy(WCHAR* wszDest, const WCHAR* wszSrc);
WCHAR*   WCSncpy(WCHAR* wszDest, const WCHAR* wszSrc, UINT n);
WCHAR*   WCScat(WCHAR* pwcz1, const WCHAR* pwcz2);
WCHAR*   WCStok(WCHAR *wszStart, const WCHAR *wszDelim);

struct WSTRINGicmp : public std::binary_function<WSTRING, WSTRING, bool> {
	bool operator()(const WSTRING& x, const WSTRING& y) const { return WCSicmp(x.c_str(), y.c_str()) < 0;}
};

WCHAR*   fgetWs(WCHAR* wcz, int n, FILE* pFile);
void     fputWs(const WCHAR* wsz, FILE* pFile);

std::string strReplace(std::string const &source, std::string const &from, std::string const &to);
WSTRING WCSReplace(WSTRING const &source, WSTRING const &from, WSTRING const &to);

#endif

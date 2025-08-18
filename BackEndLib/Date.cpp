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
 * Portions created by the Initial Developer are Copyright (C) 1995, 1996,
 * 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Mike Rimer (mrimer), Matthew Schikore (schik) Gerry Jo Jellestad (trick)
 *
 * ***** END LICENSE BLOCK ***** */

//Date.cpp
//Implementation of CDate.

#include "Date.h"
#include "Assert.h"

WSTRING  CDate::wstrMonthName[MONTH_COUNT];
bool     CDate::bIsClassInitialized = false;
CDate::DateFormat CDate::dateFormat = CDate::MDY;

const WCHAR wstrAM[] = {We('A'), We('M'), We(0)};
const WCHAR wstrPM[] = {We('P'), We('M'), We(0)};

//
//CDate public methods.
//

//****************************************************************************************
void CDate::InitClass(const WCHAR *pwzSetMonthNames[MONTH_COUNT])
{
	 ASSERT(!CDate::bIsClassInitialized);
	 for (UINT wMonthNo = 0; wMonthNo < MONTH_COUNT; ++wMonthNo)
	 {
		  ASSERT(pwzSetMonthNames[wMonthNo] && pWCv(pwzSetMonthNames[wMonthNo]));
		  CDate::wstrMonthName[wMonthNo] = pwzSetMonthNames[wMonthNo];
	 }
	 CDate::bIsClassInitialized = true;
}

//****************************************************************************************
CDate::CDate(
//Constructor that takes time_t value to specify date/time object will hold.
//
//Params:
	const time_t tSet)   //(in) POSIX date/time to set this object to.
{
	SetGMT(tSet);
}

//****************************************************************************************
CDate::CDate(
//Constructor that takes individual values to specify date/time object will hold.
//
//Params:
	UINT wSetYear,    //(in) 4-digit year.       All of these are GMT.
	UINT wSetMonth,      //(in) 1-12                   .
	UINT wSetDay,     //(in) 1-31                      .
	UINT wSetHour,    //(in) 0-23. 0 is default.       .
	UINT wSetMinute,  //(in) 0-59. 0 is default.       .
	UINT wSetSecond)  //(in) 0-59. 0 is default.       .
{
	tm tmSet;

	ASSERT(wSetYear >= 1970 && wSetYear <= 9999);
	ASSERT(wSetMonth >=1 && wSetMonth <= 12);
	ASSERT(wSetDay >= 1 && wSetDay <= 31);
	ASSERT(wSetHour <= 23);
	ASSERT(wSetMinute <= 59);
	ASSERT(wSetSecond <= 59);

#if !defined __APPLE__ && !defined __FreeBSD__

#	ifdef __linux__
	tzset();
#	endif
	tmSet.tm_year = (int) wSetYear - 1900;
	tmSet.tm_mon = (int) wSetMonth - 1;
	tmSet.tm_mday = (int) wSetDay;
	tmSet.tm_hour = (int) wSetHour;
	tmSet.tm_min = (int) wSetMinute;
	tmSet.tm_sec = (int) wSetSecond;
	tmSet.tm_yday = 0;
	tmSet.tm_wday = 0;
	tmSet.tm_isdst = 0;
#	ifdef __linux__
	tmSet.tm_gmtoff = timezone;
	tmSet.tm_zone = NULL;
#	endif

	const time_t tSet = mktime(&tmSet); //gives local time offset

	//Remove local time zone offset.
#	ifdef __linux__
	static tm tmp = {0, 0, 0, 0, 0, 0, 0, 0, 0, timezone, NULL};
	static const time_t timeOffset = mktime(&tmp); //get timezone offset
#	else
	static tm Jan11997 = {0, 0, 0, 1, 0, 97, 0, 0, 0};
	static const time_t posixJan11997 = 852076800; // the POSIX time at midnight on Jan 1, 1997
	static const time_t localJan11997 = mktime(&Jan11997);  //get local time offset
	static const time_t timeOffset = static_cast<time_t>(difftime(localJan11997, posixJan11997));
#	endif

	SetGMT(tSet - timeOffset);

#else  //simplified version, using nonstandard timegm
	tzset();

	tmSet.tm_year = (int) wSetYear - 1900;
	tmSet.tm_mon = (int) wSetMonth - 1;
	tmSet.tm_mday = (int) wSetDay;
	tmSet.tm_hour = (int) wSetHour;
	tmSet.tm_min = (int) wSetMinute;
	tmSet.tm_sec = (int) wSetSecond;
	tmSet.tm_yday = 0;
	tmSet.tm_wday = 0;
	tmSet.tm_isdst = 0;
	//the following are not standard C
	tmSet.tm_gmtoff = 0;
	tmSet.tm_zone = NULL;

	time_t gmtTime = timegm(&tmSet);  // not standard
	ASSERT(gmtTime != time_t(-1));
ASSERT(!"untested");
	SetGMT(gmtTime);
#endif
}

//****************************************************************************************
void CDate::SetDateFormat(const DateFormat df)
//Set date display format.
{
	if (df < CDate::DateFormatCount)
		CDate::dateFormat = df;
}

//****************************************************************************************
void CDate::SetGMT(
//Sets date/time for this object from a POSIX time_t.
//
//Params:
	const time_t tSetGMT) //(in) GMT date/time.
{
	this->tGMTime = tSetGMT;
}

//****************************************************************************************
void CDate::SetToNow()
//Set time to now.
{
	time(&(this->tGMTime));
}

//****************************************************************************************
void CDate::GetLocalFormattedText(
//Get formatted date/time with conversion to local time zone.
//
//Params:
	const UINT dwFormatFlags, //(in)      One or more DF_* flag constants.
	WSTRING &wstrText)      //(in/out)  Accepts an empty or non-empty string to which
							//       text will be appended.
const
{
	WCHAR dummy[20];

#ifdef WIN32
	_tzset();
#endif
	struct tm *tmGet = localtime(&this->tGMTime);   //convert to local time
	bool bShowDate = false;
	WSTRING wTmp;

	if ((dwFormatFlags & DF_LONG_DATE) == DF_LONG_DATE)
	{
		switch (CDate::dateFormat)
		{
			default:
			case MDY:
				wstrText += this->wstrMonthName[tmGet->tm_mon];
				wstrText += wszSpace;
				wstrText += _itoW(tmGet->tm_mday,dummy,10);
				wstrText += wszComma;
				wstrText += wszSpace;
				wstrText += _itoW(1900 + tmGet->tm_year,dummy,10);
			break;
			case DMY:
				wstrText += _itoW(tmGet->tm_mday,dummy,10);
				wstrText += wszSpace;
				wstrText += this->wstrMonthName[tmGet->tm_mon];
				wstrText += wszSpace;
				wstrText += _itoW(1900 + tmGet->tm_year,dummy,10);
			break;
			case YMD:
				wstrText += _itoW(1900 + tmGet->tm_year, dummy, 10);
				wstrText += wszSpace;
				wstrText += this->wstrMonthName[tmGet->tm_mon];
				wstrText += wszSpace;
				wstrText += _itoW(tmGet->tm_mday, dummy, 10);
				break;
		}
		bShowDate = true;
	} else if ((dwFormatFlags & DF_SHORT_DATE) == DF_SHORT_DATE)
	{
		const int nYear = (1900 + tmGet->tm_year) % 100;
		switch (CDate::dateFormat)
		{
			default:
			case MDY:
				wstrText += _itoW(tmGet->tm_mon + 1,dummy,10);
				AsciiToUnicode("/", wTmp);
				wstrText += wTmp;
				wstrText += _itoW(tmGet->tm_mday,dummy,10);
				wstrText += wTmp;
				if (nYear < 10)
				{
					AsciiToUnicode("0", wTmp);
					wstrText += wTmp;
				}
				wstrText += _itoW(nYear,dummy,10);
			break;
			case DMY:
				wstrText += _itoW(tmGet->tm_mday,dummy,10);
				AsciiToUnicode("/", wTmp);
				wstrText += wTmp;
				wstrText += _itoW(tmGet->tm_mon + 1,dummy,10);
				wstrText += wTmp;
				if (nYear < 10)
				{
					AsciiToUnicode("0", wTmp);
					wstrText += wTmp;
				}
				wstrText += _itoW(nYear,dummy,10);
			break;
			case YMD:
				//ISO-8601: YYYY-MM_DD
				wstrText += to_WSTRING(1900 + tmGet->tm_year);
				wstrText += wszHyphen;
				wstrText += to_WSTRING(tmGet->tm_mon + 1);
				wstrText += wszHyphen;
				wstrText += to_WSTRING(tmGet->tm_mday);
			break;
		}
		bShowDate = true;
	}

	if ((dwFormatFlags & DF_SHORT_TIME) == DF_SHORT_TIME)
	{
		WSTRING ampm = wstrAM;
		if (tmGet->tm_hour >= 12)
		{
			ampm = wstrPM;
			tmGet->tm_hour -= 12;
		}
		if (tmGet->tm_hour == 0)  // Adjust if midnight/noon hour.
			tmGet->tm_hour = 12;

		if (bShowDate)
			wstrText += wszSpace;

		wstrText += _itoW(tmGet->tm_hour,dummy,10);
		wstrText += wszColon;
		if (tmGet->tm_min < 10)
		{
			AsciiToUnicode("0", wTmp);
			wstrText += wTmp;
		}
		wstrText += _itoW(tmGet->tm_min,dummy,10);
		wstrText += ampm;
	}
}

//****************************************************************************************
WSTRING CDate::FormatTime(const UINT dwTime)   //in seconds
{
	const UINT dwHours = dwTime / 3600;
	const UINT dwMinutes = (dwTime / 60) % 60;
	const UINT dwSeconds = dwTime % 60;
	WCHAR dummy[20];
	WSTRING wstrText, wTmp;
	if (dwHours)
	{
		wstrText += _itoW(dwHours,dummy,10);
		wstrText += wszColon;
		if (dwMinutes < 10)
		{
			AsciiToUnicode("0", wTmp);
			wstrText += wTmp;
		}
	}
	wstrText += _itoW(dwMinutes,dummy,10);
	wstrText += wszColon;
	if (dwSeconds < 10)
	{
		AsciiToUnicode("0", wTmp);
		wstrText += wTmp;
	}
	wstrText += _itoW(dwSeconds,dummy,10);
	return wstrText;
}

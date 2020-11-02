// $Id: DRODUtil.cpp 9486 2010-12-29 22:29:08Z mrimer $

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
 * Contributor(s):
 * Michael Welsh Duggan (md5i), JP Burford (jpburford), Rik Cookney (timeracer)
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef WIN32
#  include <windows.h> //Should be first include.
#endif

#include <BackEndLib/Files.h>

#include "Assert.h"
#include "OptionList.h"
#include "Util3_0.h"

#include <string.h>
#include <stdio.h>
#ifdef WIN32
#include <conio.h>
#endif

#include <zlib.h>

//This is a filename that will probably exist in this specific game only.
#define APPNAME   "DRODRPGUtil v1.0"
const WCHAR wszDROD_VER[] = {{'1'},{'_'},{'0'},{0}};

static const WCHAR wszUniqueResFile[] = {
    We('d'),We('r'),We('o'),We('d'),We('r'),We('p'),We('g'),We('1'),We('_'),We('0'),We('.'),We('d'),We('a'),We('t'),We(0) };

//Prototypes.
const WSTRING GetDefaultPath();
VERSION     GetDefaultVersion();
VERSION     GetPrevVersion();
UINT    GetIDFromParam(const WCHAR *pszParam);
CUtil *     GetUtil(VERSION eVersion, const WCHAR *pszSrcPath);
VERSION     GetVersionFromParam(const WCHAR *pszSrcVersion);
void     PrintCreate(const COptionList &Options, const WCHAR *pszDestPath, 
		const WCHAR *pszDestVersion);
void     PrintCreateHelp();
void     PrintDelete(const COptionList &Options, const WCHAR *pszSrcPath, 
		const WCHAR *pszSrcVersion);
void     PrintDeleteHelp();
void     PrintDemo(const COptionList &Options, const WCHAR *pszDemoID, 
		const WCHAR *pszSrcPath, const WCHAR *pszSrcVersion);
void     PrintDemoHelp();
void     PrintExport(const COptionList &Options, const WCHAR *pszDestPath,
		const WCHAR *pszSrcPath,   const WCHAR *pszSrcVersion);
void     PrintExportHelp();
void     PrintHeader();
void     PrintHelp(const COptionList &Options, const WCHAR *pszCommand, const WCHAR *pszJoke);
void     PrintHelpHelp();
void     PrintHelpHelpHelp();
void     PrintHold(const COptionList &Options, const WCHAR *pszHoldID, 
		const WCHAR *pszSrcPath, const WCHAR *pszSrcVersion);
void     PrintHoldHelp();
void     PrintImport(const COptionList &Options, const WCHAR *pszSrcPath,  
		const WCHAR *pszSrcVersion, const WCHAR *pszDestPath, const WCHAR *pszDestVersion);
void     PrintImportHelp();
void     PrintLevel(const COptionList &Options, const WCHAR *pszLevelID,
		const WCHAR *pszSrcPath, const WCHAR *pszSrcVersion);
void     PrintLevelHelp();
void     PrintMysql(const COptionList &Options, const WCHAR *pszFilePath,
		const WCHAR *pszSrcPath, const WCHAR *pszSrcVersion);
void     PrintMysqlHelp();
void     PrintUncompress(const COptionList &Options, const WCHAR *pszFilePath,
		const WCHAR *pszSrcPath);
void     PrintUncompressHelp();
void     PrintCompress(const COptionList &Options, const WCHAR *pszFilePath,
		const WCHAR *pszSrcPath);
void     PrintCompressHelp();
void     PrintProtect(const COptionList &Options, const WCHAR *pszFilePath);
void     PrintProtectHelp();
void     PrintRoom(const COptionList &Options, const WCHAR *pszRoomID, 
		const WCHAR *pszSrcPath, const WCHAR *pszSrcVersion);
void     PrintRoomHelp();
void     PrintSummary(const COptionList &Options, const WCHAR *pszSrcPath, 
		const WCHAR *pszSrcVersion);
void     PrintSummaryHelp();
void     PrintTest(const COptionList &Options, const WCHAR *pszDemoID,
		const WCHAR *pszSrcPath, const WCHAR *pszSrcVersion);
void     PrintTestHelp();
void     PrintUnprotect(const COptionList &Options, const WCHAR *pszFilePath);
void     PrintUnprotectHelp();
void     PrintUsage();

//Constants
static const WCHAR wszCreate[] = {{'c'},{'r'},{'e'},{'a'},{'t'},{'e'},{0}};
static const WCHAR wszDelete[] = {{'d'},{'e'},{'l'},{'e'},{'t'},{'e'},{0}};
static const WCHAR wszDemo[] = {{'d'},{'e'},{'m'},{'o'},{0}};
static const WCHAR wszExport[] = {{'e'},{'x'},{'p'},{'o'},{'r'},{'t'},{0}};
static const WCHAR wszHelp[] = {{'h'},{'e'},{'l'},{'p'},{0}};
static const WCHAR wszHold[] = {{'h'},{'o'},{'l'},{'d'},{0}};
static const WCHAR wszImport[] = {{'i'},{'m'},{'p'},{'o'},{'r'},{'t'},{0}};
static const WCHAR wszLevel[] = {{'l'},{'e'},{'v'},{'e'},{'l'},{0}};
static const WCHAR wszTest[] = {{'t'},{'e'},{'s'},{'t'},{0}};
static const WCHAR wszRoom[] = {{'r'},{'o'},{'o'},{'m'},{0}};
static const WCHAR wszSummary[] = {{'s'},{'u'},{'m'},{'m'},{'a'},{'r'},{'y'},{0}};
static const WCHAR wszUnprotect[] = {{'u'},{'n'},{'p'},{'r'},{'o'},{'t'},{'e'},{'c'},{'t'},{0}};
static const WCHAR *wszProtect = wszUnprotect + 2;
static const WCHAR wszMySQL[] = {{'m'},{'y'},{'s'},{'q'},{'l'},{0}};
static const WCHAR wszUncompress[] = {{'u'},{'n'},{'c'},{'o'},{'m'},{'p'},{'r'},{'e'},{'s'},{'s'},{0}};
static const WCHAR *wszCompress = wszUncompress + 2;

static const WCHAR wszDefault[] = {{'d'},{'e'},{'f'},{'a'},{'u'},{'l'},{'t'},{0}};

//******************************************************************************************
int wmain(int argc, WCHAR* argv[])
{
#ifdef _DEBUG
#  ifdef WIN32
#     define DEBUGPAUSE _getch()
#  else
#     define DEBUGPAUSE getchar()
#  endif
#else
#  define DEBUGPAUSE
#endif

	WSTRING wstrPath;
	//Initialize the app.
#if defined(__sgi) || defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
	wstrPath = argv[0]; //main below takes care of this.
#elif defined (WIN32)
	WCHAR wszPathBuffer[MAX_PATH+1];
	GetModuleFileName(NULL, wszPathBuffer, MAX_PATH);
	wstrPath = wszPathBuffer;
#else
#error How does this system get the Unicode path of the executable
#endif

#ifndef WMAIN
	WSTRING wstrArg;
#endif

	std::vector<string> datFiles; //writable .dats.  [0]: + = copy, - = no copy
	CFiles::InitAppVars(wszUniqueResFile, datFiles);

	static const WCHAR wszDROD[] = {{'d'},{'r'},{'o'},{'d'},{'r'},{'p'},{'g'},{0}};
	static CFiles files(wstrPath.c_str(), wszDROD, wszDROD_VER, false, false);
	if (CFiles::bad_data_path_file) {
		printf("The information in DataPath.ini is invalid.\n");
		return 1;
	}

	if (argc < 2)
	{
		PrintUsage();
		DEBUGPAUSE;
		return 0;
	}

	//Parse options which precede params into a class.
	COptionList OptionList;
	if (!OptionList.Set(argc, argv))
	{
		DEBUGPAUSE;
		return 0;
	}

//Readability macro for optional params.
#define OPT_PARAM(n) ( ((UINT)argc > ((n) + OptionList.GetSize())) ? \
		argv[((n) + OptionList.GetSize())] : NULL)

	//Parse command and call appropriate function.
	if     (WCSicmp(argv[1], wszCreate) == 0)    PrintCreate(OptionList, OPT_PARAM(2), OPT_PARAM(3));
	else if(WCSicmp(argv[1], wszDelete) == 0)    PrintDelete(OptionList, OPT_PARAM(2), OPT_PARAM(3));
	else if(WCSicmp(argv[1], wszDemo) == 0)         PrintDemo(OptionList, OPT_PARAM(2), OPT_PARAM(3), OPT_PARAM(4));
	else if(WCSicmp(argv[1], wszExport) == 0)    PrintExport(OptionList, OPT_PARAM(2), OPT_PARAM(3), OPT_PARAM(4));
	else if(WCSicmp(argv[1], wszHelp) == 0)         PrintHelp(OptionList, OPT_PARAM(2), OPT_PARAM(3));
	else if(WCSicmp(argv[1], wszHold) == 0)         PrintHold(OptionList, OPT_PARAM(2), OPT_PARAM(3), OPT_PARAM(4));
	else if(WCSicmp(argv[1], wszImport) == 0)    PrintImport(OptionList, OPT_PARAM(2), OPT_PARAM(3), OPT_PARAM(4), OPT_PARAM(5));
	else if(WCSicmp(argv[1], wszLevel) == 0)     PrintLevel(OptionList, OPT_PARAM(2), OPT_PARAM(3), OPT_PARAM(4));
	else if(WCSicmp(argv[1], wszTest) == 0)         PrintTest(OptionList, OPT_PARAM(2), OPT_PARAM(3), OPT_PARAM(4));
	else if(WCSicmp(argv[1], wszRoom) == 0)         PrintRoom(OptionList, OPT_PARAM(2), OPT_PARAM(3), OPT_PARAM(4));
	else if(WCSicmp(argv[1], wszSummary) == 0)      PrintSummary(OptionList, OPT_PARAM(2), OPT_PARAM(3));
	else if(WCSicmp(argv[1], wszProtect) == 0)      PrintProtect(OptionList, OPT_PARAM(2));
	else if(WCSicmp(argv[1], wszUnprotect) == 0) PrintUnprotect(OptionList, OPT_PARAM(2));
	else if(WCSicmp(argv[1], wszMySQL) == 0)     PrintMysql(OptionList, OPT_PARAM(2), OPT_PARAM(3), OPT_PARAM(4));
	else if(WCSicmp(argv[1], wszCompress) == 0)     PrintCompress(OptionList, OPT_PARAM(2), OPT_PARAM(3));
	else if(WCSicmp(argv[1], wszUncompress) == 0)   PrintUncompress(OptionList, OPT_PARAM(2), OPT_PARAM(3));
	else                                PrintUsage();

#undef OPT_PARAM

	DEBUGPAUSE;
	return 0;
}

//******************************************************************************************
#ifdef WIN32
//Has wmain entrypoint.
#elif defined(__linux__) || defined(__FreeBSD__) || defined (__sgi) || defined(__APPLE__)
//No wmain, so we convert all args to WCHAR. It's not optimal, but i
//want to have this running ASAP. We can always optimize it later =)
int main(int argc, char *argv[])
{
	WCHAR *Wargv[argc];

	for (int i = argc; i--; )
	{
		WSTRING wstrTemp;
		UTF8ToUnicode(argv[i], wstrTemp);
		Wargv[i] = new WCHAR[wstrTemp.length()+1];
		WCScpy(Wargv[i], wstrTemp.c_str());
	}

	int retval = wmain(argc, Wargv);

	for (int i = argc; --i >= 0; delete[] Wargv[i]);

	return retval;
}
#else
#error Does this system have a wmain entry ?
#endif

//******************************************************************************************
void PrintHeader()
{
	const WSTRING wszDefaultPath = GetDefaultPath();

	printf(APPNAME "\r\n"
		"Portions created by Caravel Games are Copyright (C) 1995, 1996, \r\n"
		"1997, 2000, 2001, 2002, 2003, 2004, 2005 Caravel Software.\r\n"
		"All Rights Reserved.\r\n"
		"Contributors: Michael Welsh Duggan, JP Burford, Rik Cookney, John Wm. Wicks,\r\n"
		"Mike Rimer, Matt Schikore, Gerry Jo Jellestad\r\n"
		"\r\n");
#ifdef HAS_UNICODE
	printf("Default path is %S.\r\n"
		"Default version is %S.\r\n"
		"-------------------------------------------------------------------------------\r\n",
		wszDefaultPath.c_str(), g_szarrVersions[GetDefaultVersion()] );
#else
	WSTRING wstrTemp = wszDefaultPath;
	char pszDefaultPath[wstrTemp.length()];
	UnicodeToUTF8(wstrTemp, pszDefaultPath);
	wstrTemp = g_szarrVersions[GetDefaultVersion()];
	char pszVersion[wstrTemp.length()];
	UnicodeToUTF8(wstrTemp, pszVersion);
	printf("Default path is %s.\r\n"
		"Default version is %s.\r\n"
		"-------------------------------------------------------------------------------\r\n",
		pszDefaultPath.c_str(), pszVersion);
#endif
}

//******************************************************************************************
void PrintUsage()
{
	PrintHeader();
	printf(
			"The following commands are supported:\r\n"
			"  create    [ [ DestPath ] DestVersion ]\r\n"
			"  delete    [ [ SrcPath ] SrcVersion ]\r\n"
			"  demo      [ [ [ DemoID ] SrcPath ] SrcVersion ]\r\n"
			"  export    [ [ [ DestPath ] SrcPath ] SrcVersion ]\r\n"
			"  help      [ Command ]\r\n"
			"  hold      [ [ [ HoldID ] SrcPath ] SrcVersion ]\r\n"
			"  import    [ Options ] [ [ [ [ SrcPath ] SrcVersion ] DestPath ] DestVersion ]\r\n"
			"  level     [ [ [ LevelID ] SrcPath ] SrcVersion ]\r\n"
			"  mysql     [ [ [ HoldID ] SrcPath ] SrcVersion ]\r\n"
			"  room      [ [ [ RoomID ] SrcVersion ] SrcPath ]\r\n"
			"  summary   [ [ SrcPath ] SrcVersion ]\r\n"
			"  test      [ Options ] [ [ [ DemoID ] SrcVersion ] SrcPath ]\r\n"
			"  protect   SrcFilePath\r\n"
			"  unprotect SrcFilePath\r\n"
			"  compress  SrcFilePath DestFilePath\r\n"
			"  uncompress SrcFilePath DestFilePath\r\n"
			"\r\n"
			"Use \"help\" command for information on specific commands.\r\n");
}

//******************************************************************************************
void PrintHelpHelp()
{
	PrintHeader();
	printf(
	  "help        [ Command ]\r\n"
	  "\r\n"
	  "Shows help for a specific command.\r\n"
	  "\r\n"
	  "Params:\r\n"
	  "  Command     Command for which help will be displayed, if omitted you'll see\r\n"
	  "              help for the \"help\" command.  If you need even more help, try\r\n"
	  "              \"drodutil help help help\".\r\n");
}

//******************************************************************************************
void PrintHelpHelpHelp()
{
	printf("I was only joking.  Boy, you are a mess, aren't you?\r\n");
}

//******************************************************************************************
void PrintHelp(
//Shows help for a specific command.
//
//Params:
	const COptionList &/*Options*/,  //(in)
	const WCHAR *pszCommand,      //(in)
	const WCHAR *pszJoke)      //(in)
{
	if(!pszCommand || WCSicmp(pszCommand, wszHelp) == 0)     
	{
		if (pszJoke && WCSicmp(pszJoke, wszHelp) == 0)
			PrintHelpHelpHelp();
		else
			PrintHelpHelp();
	}
	else if (WCSicmp(pszCommand, wszCreate) == 0)      PrintCreateHelp();
	else if (WCSicmp(pszCommand, wszDelete) == 0)      PrintDeleteHelp();
	else if (WCSicmp(pszCommand, wszDemo) == 0)        PrintDemoHelp();
	else if (WCSicmp(pszCommand, wszExport) == 0)      PrintExportHelp();
	else if (WCSicmp(pszCommand, wszHold) == 0)        PrintHoldHelp();
	else if (WCSicmp(pszCommand, wszImport) == 0)      PrintImportHelp();
	else if (WCSicmp(pszCommand, wszLevel) == 0)    PrintLevelHelp();
	else if (WCSicmp(pszCommand, wszTest) == 0)        PrintTestHelp();
	else if (WCSicmp(pszCommand, wszMySQL) == 0)    PrintMysqlHelp();
	else if (WCSicmp(pszCommand, wszRoom) == 0)        PrintRoomHelp();
	else if (WCSicmp(pszCommand, wszSummary) == 0)     PrintSummaryHelp();
	else if (WCSicmp(pszCommand, wszProtect) == 0)     PrintProtectHelp();
	else if (WCSicmp(pszCommand, wszUnprotect) == 0)   PrintUnprotectHelp();
	else
		PrintHelpHelp();
}

//******************************************************************************************
void PrintCreateHelp()
{
	PrintHeader();
	printf(
	  "create [ [ DestPath ] DestVersion ]\r\n"
	  "\r\n"
	  "Creates empty DROD data.\r\n"
	  "\r\n"
	  "Params:\r\n"
	  "  DestPath      Location at which to create data.  If omitted, default path\r\n"
	  "                will be used.\r\n"
	  "  DestVersion   Data version to create.  If omitted, default version will be\r\n"
	  "                used.\r\n");
}

//******************************************************************************************
void PrintCreate(
//Create empty DROD data and print results.  See PrintCreateHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR* wpszDestPath, //(in)
	const WCHAR* wpszDestVersion) //(in)
{
	PrintHeader();

	if (!Options.AreOptionsValid(wszEmpty)) return;

	WSTRING wstrPath =
			(wpszDestPath == NULL || WCSicmp(wpszDestPath, wszDefault)==0 ) ?
			GetDefaultPath() : wpszDestPath;
	const VERSION eVersion =
			(wpszDestVersion == NULL || WCSicmp(wpszDestVersion, wszDefault)==0 ) ?
			GetDefaultVersion() : GetVersionFromParam(wpszDestVersion);

	//Get util for specified version.
	CUtil *pUtil = GetUtil(eVersion, wstrPath.c_str());
	if (!pUtil)
	{
		printf("FAILED--Version not supported.\r\n");
		return;
	}

	//Create the data.
	if (pUtil->PrintCreate(Options))
		printf("SUCCESS--DROD data was created.\r\n");
}

//******************************************************************************************
void PrintDeleteHelp()
{
	PrintHeader();
	printf(
	  "delete [ [ SrcPath ] SrcVersion ]\r\n"
	  "\r\n"
	  "Deletes DROD data.\r\n"
	  "\r\n"
	  "Params:\r\n"
	  "  SrcPath       Location of data to delete.  If omitted, default path will\r\n"
	  "                be used.\r\n"
	  "  SrcVersion    Version of data to delete.  If omitted, default version will\r\n"
	  "                be used.\r\n");
}

//******************************************************************************************
void PrintDelete(
//Deletes DROD data and print results.  See PrintDeleteHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszSrcPath,      //(in)
	const WCHAR *pszSrcVersion)   //(in)
{
	PrintHeader();

	if (!Options.AreOptionsValid(wszEmpty)) return;

	WSTRING strPath = (pszSrcPath == NULL || WCSicmp(pszSrcPath, wszDefault)==0 ) ?
			GetDefaultPath() : pszSrcPath;
	VERSION eVersion = (pszSrcVersion == NULL || WCSicmp(pszSrcVersion, wszDefault)==0 ) ?
			GetDefaultVersion() : GetVersionFromParam(pszSrcVersion);

	//Get util for specified version.
	CUtil *pUtil = GetUtil(eVersion, strPath.c_str());
	if (!pUtil)
	{
		printf("FAILED--Version not supported.\r\n");
		return;
	}

	//Delete the data.
	if (pUtil->PrintDelete(Options))
		printf("SUCCESS--DROD data was deleted.\r\n");
}

//******************************************************************************************
void PrintExportHelp()
{
	PrintHeader();
	printf(
	  "export      [ [ [ DestPath ] SrcPath ] SrcVersion]\r\n"
	  "\r\n"
	  "Exports DROD data to an XML format designed for DRODUtil imports.\r\n"
	  "\r\n"
	  "Params:\r\n"
	  "  DestPath      Location at which to write export files.  If omitted, default\r\n"
	  "                path will be used.\r\n"
	  "  SrcPath       Location of data to be exported.  If omitted, default path will\r\n"
	  "                be used.\r\n"
	  "  SrcVersion    Version of data to exported.  If omitted, default version will\r\n"
	  "                be used.\r\n"
	  );
}

//******************************************************************************************
void PrintExport(
//Exports DROD data and print results.  See PrintExportHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszDestPath,  //(in)
	const WCHAR *pszSrcPath,      //(in)
	const WCHAR *pszSrcVersion)   //(in)
{
	PrintHeader();

	if (!Options.AreOptionsValid(wszEmpty)) return;

	WSTRING strSrcPath = (pszSrcPath == NULL || WCSicmp(pszSrcPath, wszDefault)==0 ) ?
			GetDefaultPath() : pszSrcPath;
	VERSION eSrcVersion = (pszSrcVersion == NULL || WCSicmp(pszSrcVersion, wszDefault)==0 ) ?
			GetDefaultVersion() : GetVersionFromParam(pszSrcVersion);
	WSTRING strDestPath = (pszDestPath == NULL || WCSicmp(pszDestPath, wszDefault)==0 ) ?
			GetDefaultPath() : pszDestPath;

	//Get util for source version.
	CUtil *pUtil = GetUtil(eSrcVersion, strSrcPath.c_str());
	if (!pUtil)
	{
		printf("FAILED--Version not supported.\r\n");
		return;
	}

	//Export the data.
	if (pUtil->PrintExport(Options, strDestPath.c_str()))
		printf("SUCCESS--DROD data was exported.\r\n");
}

//******************************************************************************************
void PrintHoldHelp()
{
	PrintHeader();
	printf(
	  "hold        [ [ [ HoldID ] SrcPath ] SrcVersion ]\r\n"
	  "\r\n"
	  "Shows information about a specified hold.\r\n"
	  "\r\n"
	  "Params:\r\n"
	  "  HoldID        Indicates which hold to use.  If omitted, first hold will be\r\n"
	  "                used.\r\n"
	  "  SrcPath       Location of data.  If omitted, default path will be used.\r\n"
	  "  SrcVersion    Version of data.  If omitted, default version will be used.\r\n"
	  );
}

//******************************************************************************************
void PrintHold(
//Shows information about a specified hold.  See PrintHoldHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszHoldID,    //(in)
	const WCHAR *pszSrcPath,      //(in)
	const WCHAR *pszSrcVersion)   //(in)
{
	PrintHeader();

	if (!Options.AreOptionsValid(wszEmpty)) return;

	WSTRING strSrcPath =
			(pszSrcPath == NULL || WCSicmp(pszSrcPath, wszDefault)==0 ) ?
			GetDefaultPath() : pszSrcPath;
	VERSION eSrcVersion =
			(pszSrcVersion == NULL || WCSicmp(pszSrcVersion, wszDefault)==0 ) ?
			GetDefaultVersion() : GetVersionFromParam(pszSrcVersion);
	const UINT dwHoldID =
			(pszHoldID == NULL || WCSicmp(pszHoldID, wszDefault)==0 ) ?
			0L : GetIDFromParam(pszHoldID);

	//Get util for source version.
	CUtil *pUtil = GetUtil(eSrcVersion, strSrcPath.c_str());
	if (!pUtil)
	{
		printf("FAILED--Version not supported.\r\n");
		return;
	}

	//Print hold information.
	if (pUtil->PrintHold(Options, dwHoldID))
		printf("SUCCESS--Hold information retrieved.\r\n");
}

//******************************************************************************************
void PrintImportHelp()
{
	PrintHeader();
	printf(
	  "import          [ -d ] [ -h ] [ -p ] [ -t ] [ -i ]\r\n"
	  "                [ [ [ [ SrcPath ] SrcVersion ] DestPath ] DestVersion ]\r\n"
	  "\r\n"
	  "Imports DROD data from an earlier version.\r\n"
	  "\r\n"
	  "Options:\r\n"
	  "  -h            Import all holds in source data.\r\n"
	  "  -d            Import all demos and saved games found in source data.\r\n"
	  "  -p            Import all players found in source data.\r\n"
	  "  -t            Import all basic messages and generate MIDs.h file containing\r\n"
	  "                MID_* constants for messages.  MIDs.h will be written to SrcPath.\r\n"
	  "  -i            Import all images found in source data.\r\n"
	  "If no options are specified, options will default to \"-h -d -p -t\".\r\n"
	  "\r\n"
	  "Params:\r\n"
	  "  SrcPath       Location of data to import.  If omitted, default path will \r\n"
	  "                be used.\r\n"
	  "  SrcVersion    Version of data to import.  If omitted, the version previous to\r\n"
	  "                the default version will be used.\r\n"
	  "  DestPath      Location of data to recieve import.  If omitted, default path\r\n"
	  "                will be used.\r\n"
	  "  DestVersion   Version of data to recieve import.  If omitted, default version\r\n"
	  "                will be used.\r\n"
	  );
}

//******************************************************************************************
void PrintImport(
//Imports DROD data and print results.  See PrintImportHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszSrcPath,      //(in)
	const WCHAR *pszSrcVersion,   //(in)
	const WCHAR *pszDestPath,  //(in)
	const WCHAR *pszDestVersion)  //(in)
{
	PrintHeader();

	static const WCHAR options[] = {{'h'},{','},{'d'},{','},{'p'},{','},{'t'},{','},{'i'},{0}};
	if (!Options.AreOptionsValid(options)) return;

	WSTRING strSrcPath =
			(pszSrcPath == NULL || WCSicmp(pszSrcPath, wszDefault)==0 ) ?
			GetDefaultPath() : pszSrcPath;
	VERSION eSrcVersion =
			(pszSrcVersion == NULL || WCSicmp(pszSrcVersion, wszDefault)==0 ) ?
			GetPrevVersion() : GetVersionFromParam(pszSrcVersion);
	WSTRING strDestPath =
			(pszDestPath == NULL || WCSicmp(pszDestPath, wszDefault)==0 ) ?
			GetDefaultPath() : pszDestPath;
	VERSION eDestVersion =
			(pszDestVersion == NULL || WCSicmp(pszDestVersion, wszDefault)==0 ) ?
			GetDefaultVersion() : GetVersionFromParam(pszDestVersion);

	//Note: At this point there is only one case to handle--v1.11c to v1.5.  In the
	//future a general solution will be needed.
	if (!(eSrcVersion == v1_11c && eDestVersion == v1_5) &&
		(!eSrcVersion == v1_5 && (eDestVersion == v1_6 || eDestVersion == v2_0)) )
	{
		printf("FAILED--Only 1.11c-to-1.5 and 1.5-to-1.6/2.0 imports are supported.\r\n");
		return;
	}

	//Get util for dest version.
	CUtil *pUtil = GetUtil(eDestVersion, strDestPath.c_str());
	if (!pUtil)
	{
		printf("FAILED--Version not supported.\r\n");
		return;
	}

	//Import the data.
	if (pUtil->PrintImport(Options, strSrcPath.c_str(), eSrcVersion))
		printf("SUCCESS--DROD data was imported.\r\n");
}

//******************************************************************************************
void PrintLevelHelp()
{
	PrintHeader();
	printf(
	  "level        [ [ [ LevelID ] SrcPath ] SrcVersion ]\r\n"
	  "\r\n"
	  "Save image of level minimap to filename \"level\"+<ID>.\r\n"
	  "\r\n"
	  "Params:\r\n"
	  "  LevelID       Indicates which level to use.  If omitted, first level of first\r\n"
	  "                hold will be used.\r\n"
	  "  SrcPath       Location of data.  If omitted, default path will be used.\r\n"
	  "  SrcVersion    Version of data.  If omitted, default version will be used.\r\n"
	  );
}

//******************************************************************************************
void PrintLevel(
//Shows information about a specified level.  See PrintLevelHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszLevelID,      //(in)
	const WCHAR *pszSrcPath,      //(in)
	const WCHAR *pszSrcVersion)   //(in)
{
	PrintHeader();

	if (!Options.AreOptionsValid(wszEmpty)) return;

	WSTRING strSrcPath =
			(pszSrcPath == NULL || WCSicmp(pszSrcPath, wszDefault)==0 ) ?
			GetDefaultPath() : pszSrcPath;
	VERSION eSrcVersion =
			(pszSrcVersion == NULL || WCSicmp(pszSrcVersion, wszDefault)==0 ) ?
			GetDefaultVersion() : GetVersionFromParam(pszSrcVersion);
	const UINT dwLevelID =
			(pszLevelID == NULL || WCSicmp(pszLevelID, wszDefault)==0 ) ?
			0L : GetIDFromParam(pszLevelID);

	//Get util for source version.
	CUtil *pUtil = GetUtil(eSrcVersion, strSrcPath.c_str());
	if (!pUtil)
	{
		printf("FAILED--Version not supported.\r\n");
		return;
	}

	//Print level information.
	if (pUtil->PrintLevel(Options, dwLevelID))
		printf("SUCCESS--Level information retrieved.\r\n");
}

//******************************************************************************************
void PrintMysqlHelp()
{
	PrintHeader();
	printf(
	  "mysql        [ [ [ HoldID ] SrcPath ] SrcVersion ]\r\n"
	  "Shows information about an entire hold, formatted as a PHP file\r\n"
	  "which will create a MySQL database.\r\n"
	  "\r\n"
	  "Params:\r\n"
	  "  HoldID        Indicates which hold to use.  If omitted, first\r\n"
	  "                hold will be used.\r\n"
	  "  SrcPath       Location of data.  If omitted, default path will be used.\r\n"
	  "  SrcVersion    Version of data.  If omitted, default version will be used.\r\n"
	  );
}

//******************************************************************************************
void PrintMysql(
//Shows information about an entire hold, formatted as a PHP file that will create a MySQL database.
//See PrintMysqlHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszHoldID,    //(in)
	const WCHAR *pszSrcPath,      //(in)
	const WCHAR *pszSrcVersion)   //(in)
{
	PrintHeader();

	if (!Options.AreOptionsValid(wszEmpty)) return;

	WSTRING strSrcPath =
			(pszSrcPath == NULL || WCSicmp(pszSrcPath, wszDefault)==0 ) ?
			GetDefaultPath() : pszSrcPath;
	VERSION eSrcVersion =
			(pszSrcVersion == NULL || WCSicmp(pszSrcVersion, wszDefault)==0 ) ?
			GetDefaultVersion() : GetVersionFromParam(pszSrcVersion);
	const UINT dwHoldID =
			(pszHoldID == NULL || WCSicmp(pszHoldID, wszDefault)==0 ) ?
			0L : GetIDFromParam(pszHoldID);

	//Get util for source version.
	CUtil *pUtil = GetUtil(eSrcVersion, strSrcPath.c_str());
	if (!pUtil)
	{
		printf("FAILED--Version not supported.\r\n");
		return;
	}

	//Print hold information.
	if (pUtil->PrintMysql(Options, dwHoldID))
		printf("SUCCESS--Hold information retrieved.\r\n");
}

//******************************************************************************************
void PrintRoomHelp()
{
	PrintHeader();
	printf(
	  "room        [-h:holdID] [ [ [ RoomID ] SrcPath ] SrcVersion ]\r\n"
	  "\r\n"
	  "Save image of room to filename \"room\"+<RoomID>.\r\n"
	  "\r\n"
	  "Options:\r\n"
	  "  -h:holdID     Save all rooms in hold w/ holdID.\r\n"
	  "\r\n"
	  "Params:\r\n"
	  "  RoomID        Indicates which room to use.  If omitted, first room of first\r\n"
	  "                level of first hold will be used.\r\n"
	  "  SrcPath       Location of data.  If omitted, default path will be used.\r\n"
	  "  SrcVersion    Version of data.  If omitted, default version will be used.\r\n"
	  );
}

//******************************************************************************************
void PrintRoom(
//Shows information about a specified room.  See PrintRoomHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszRoomID,    //(in)
	const WCHAR *pszSrcPath,      //(in)
	const WCHAR *pszSrcVersion)   //(in)
{
	PrintHeader();

	static WCHAR options[] = {{'h'},{0}};
	if (!Options.AreOptionsValid(options)) return;

	WSTRING strSrcPath =
			(pszSrcPath == NULL || WCSicmp(pszSrcPath, wszDefault)==0 ) ?
			GetDefaultPath() : pszSrcPath;
	VERSION eSrcVersion =
			(pszSrcVersion == NULL || WCSicmp(pszSrcVersion, wszDefault)==0 ) ?
			GetDefaultVersion() : GetVersionFromParam(pszSrcVersion);
	const UINT dwRoomID =
			(pszRoomID == NULL || WCSicmp(pszRoomID, wszDefault)==0 ) ?
			0L : GetIDFromParam(pszRoomID);

	//Get util for source version.
	CUtil *pUtil = GetUtil(eSrcVersion, strSrcPath.c_str());
	if (!pUtil)
	{
		printf("FAILED--Version not supported.\r\n");
		return;
	}

	//Print room information.
	if (pUtil->PrintRoom(Options, dwRoomID))
		printf("SUCCESS--Room information retrieved.\r\n");
}


//******************************************************************************************
void PrintSummaryHelp()
{
	PrintHeader();
	printf(
	  "summary        [ [ [ SrcPath ] SrcVersion ]\r\n"
	  "\r\n"
	  "Shows summary information about DROD data.\r\n"
	  "\r\n"
	  "Params:\r\n"
	  "  SrcPath       Location of data.  If omitted, default path will be used.\r\n"
	  "  SrcVersion    Version of data.  If omitted, default version will be used.\r\n");
}

//******************************************************************************************
void PrintSummary(
//Shows summary information about DROD data.  See PrintSummaryHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszSrcPath,      //(in)
	const WCHAR *pszSrcVersion)   //(in)
{
	PrintHeader();

	if (!Options.AreOptionsValid(wszEmpty)) return;

	WSTRING strSrcPath =
			(pszSrcPath == NULL || WCSicmp(pszSrcPath, wszDefault)==0 ) ?
			GetDefaultPath() : pszSrcPath;
	VERSION eSrcVersion =
			(pszSrcVersion == NULL || WCSicmp(pszSrcVersion, wszDefault)==0 ) ?
			GetDefaultVersion() : GetVersionFromParam(pszSrcVersion);

	//Get util for source version.
	CUtil *pUtil = GetUtil(eSrcVersion, strSrcPath.c_str());
	if (!pUtil)
	{
		printf("FAILED--Version not supported.\r\n");
		return;
	}

	//Print summary information.
	if (pUtil->PrintSummary(Options))
		printf("SUCCESS--Summary information displayed.\r\n");
}

//******************************************************************************************
void PrintTestHelp()
{
	PrintHeader();
	printf(
	  "test        [-c] [-s:checksum] [ [ [ DemoID ] SrcPath ] SrcVersion ]\r\n"
	  "\r\n"
	  "Plays through a demo and shows results.\r\n"
	  "\r\n"
	  "Options:\r\n"
	  "  -c            Display failure if room is not conquered.\r\n"
	  "  -m            Display failure if monsters are present at end of demo.\r\n"
	  "  -s:checksum   Display failure if game state checksum does not match\r\n"
	  "                \"checksum\" attribute at end of demo.\r\n"
	  "\r\n"
	  "Params:\r\n"
	  "  SrcPath       Location of data.  If omitted, default path will be used.\r\n"
	  "  SrcVersion    Version of data.  If omitted, default version will be used.\r\n"
	  "  DemoID        Indicates which demo to use for testing.  If omitted, all demos\r\n"
	  "                will be tested.\r\n");
}

//******************************************************************************************
void PrintTest(
//Tests one or more demos.  See PrintTestHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszDemoID,    //(in)
	const WCHAR *pszSrcPath,      //(in)
	const WCHAR *pszSrcVersion)   //(in)
{
	PrintHeader();

	static WCHAR options[] = {{'c'},{','},{'m'},{','},{'s'},{0}};
	if (!Options.AreOptionsValid(options)) return;

	WSTRING strSrcPath =
			(pszSrcPath == NULL || WCSicmp(pszSrcPath, wszDefault)==0 ) ?
			GetDefaultPath() : pszSrcPath;
	VERSION eSrcVersion =
			(pszSrcVersion == NULL || WCSicmp(pszSrcVersion, wszDefault)==0 ) ?
			GetDefaultVersion() : GetVersionFromParam(pszSrcVersion);
	const UINT dwDemoID =
			(pszDemoID == NULL || WCSicmp(pszDemoID, wszDefault)==0 ) ?
			0L : GetIDFromParam(pszDemoID);

	//Get util for source version.
	CUtil *pUtil = GetUtil(eSrcVersion, strSrcPath.c_str());
	if (!pUtil)
	{
		printf("FAILED--Version not supported.\r\n");
		return;
	}

	//Test the demo.
	if (pUtil->PrintTest(Options, dwDemoID))
	{
		if (dwDemoID == 0)
			printf("SUCCESS--All demos tested.\r\n");
		else
			printf("SUCCESS--Demo tested.\r\n");
	}
}

//******************************************************************************************
void PrintDemoHelp()
{
	PrintHeader();
	printf(
	  "demo        [ [ [ DemoID ] SrcPath ] SrcVersion ]\r\n"
	  "\r\n"
	  "Displays information about a demo.\r\n"
	  "\r\n"
	  "Params:\r\n"
	  "  DemoID        Indicates which demo to use.  If omitted, first demo will be\r\n"
	  "                used.\r\n"
	  "  SrcPath       Location of data.  If omitted, default path will be used.\r\n"
	  "  SrcVersion    Version of data.  If omitted, default version will be used.\r\n"
	  );
}

//******************************************************************************************
void PrintDemo(
//Shows information about a specified demo.  See PrintDemoHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszDemoID,    //(in)
	const WCHAR *pszSrcPath,      //(in)
	const WCHAR *pszSrcVersion)   //(in)
{
	PrintHeader();

	if (!Options.AreOptionsValid(wszEmpty)) return;

	WSTRING strSrcPath =
			(pszSrcPath == NULL || WCSicmp(pszSrcPath, wszDefault)==0 ) ?
			GetDefaultPath() : pszSrcPath;
	VERSION eSrcVersion = (pszSrcVersion == NULL || WCSicmp(pszSrcVersion, wszDefault)==0 ) ?
			GetDefaultVersion() : GetVersionFromParam(pszSrcVersion);
	const UINT dwDemoID = (pszDemoID == NULL || WCSicmp(pszDemoID, wszDefault)==0 ) ?
			0L : GetIDFromParam(pszDemoID);

	//Get util for source version.
	CUtil *pUtil = GetUtil(eSrcVersion, strSrcPath.c_str());
	if (!pUtil)
	{
		printf("FAILED--Version not supported.\r\n");
		return;
	}

	//Test the demo.
	if (pUtil->PrintDemo(Options, dwDemoID))
	{
		printf("SUCCESS--Demo information retrieved.\r\n");
	}
}

//******************************************************************************************
VERSION GetVersionFromParam(
//Gets version enum constant that matches string.
//
//Params:
	const WCHAR* wpszSrcVersion) //(in) String to match.
//
//Returns:
//Constant.
{
	for(UINT wVerI = 0; wVerI < VERSION_COUNT; ++wVerI)
	{
		if (WCSicmp(g_szarrVersions[wVerI], wpszSrcVersion)==0) 
			return static_cast<VERSION>(wVerI); //Found it.
	}

	//Didn't find it.
	return GetDefaultVersion();
}

//******************************************************************************************
VERSION GetDefaultVersion()
//Returns the latest version.
{
	return static_cast<VERSION>(VERSION_COUNT - 1);
}

//******************************************************************************************
VERSION GetPrevVersion()
//Returns version before the latest version.
{
	ASSERT(VERSION_COUNT > 1);
//!!
	return static_cast<VERSION>(VERSION_COUNT - 3); //1.5
	return static_cast<VERSION>(VERSION_COUNT - 2);
}

//******************************************************************************************
const WSTRING GetDefaultPath()
//Returns default path.
{
	CFiles Files;
	return Files.GetDatPath();
}

//******************************************************************************************
UINT GetIDFromParam(const WCHAR *pszParam)
//Convert string param to DWORD.
{
	//Make sure the thing is numeric.
	const WCHAR *pszSeek = pszParam;
	while (*pszSeek != '\0')
	{
		if (*pszSeek < '0' && *pszSeek > '9') return 0L; //Not numeric.
		++pszSeek;
	}

	return static_cast<UINT>(_Wtoi(pszParam));
}

//******************************************************************************************
CUtil* GetUtil(VERSION eVersion, const WCHAR* pszSetPath)
//Factory for CUtil-derived classes.
{
	CUtil *pUtil = NULL;
	switch (eVersion)
	{
		case v3_0:
			pUtil = new CUtil3_0(pszSetPath);
			break;
		default:
			break;
	}
	return pUtil;
}

//******************************************************************************************
void PrintProtectHelp()
{
	PrintHeader();
	printf(
		"protect    SrcFilePath\r\n"
		"\r\n"
		"Scrambles the contents of a file so that its format can only be recognized\r\n"
		"by DROD.\r\n");
}

//******************************************************************************************
void PrintProtect(
//Scramble a data file.  See PrintProtectHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszFilePath)  //(in)
{
	PrintHeader();

	if (!Options.AreOptionsValid(wszEmpty)) return;
	if (!pszFilePath)
	{
		printf("FAILED--Must specify file to protect.\r\n");
		return;
	}

	if (CFiles::FileIsEncrypted(pszFilePath))
		printf("FAILED--File already looks protected.\r\n");
	else
	{
		//Scramble the data.
		if (CFiles::ProtectFile(pszFilePath))
			printf("SUCCESS--File protected.\r\n");
		else printf("FAILED--File could not be read/written.\r\n");
	}
}

//******************************************************************************************
void PrintUnprotectHelp()
{
	PrintHeader();
	printf(
		"unprotect  SrcFilePath\r\n"
		"\r\n"
		"Unscrambles the contents of a previously protected file so that its format\r\n"
		"can be recognized by other applications.\r\n");
}

//******************************************************************************************
void PrintUnprotect(
//Unscramble a data file.  See PrintUnprotectHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszFilePath)  //(in)
{
	PrintHeader();

	if (!Options.AreOptionsValid(wszEmpty)) return;
	if (!pszFilePath)
	{
		printf("FAILED--Must specify file to unprotect.\r\n");
		return;
	}

	if (!CFiles::FileIsEncrypted(pszFilePath))
		printf("FAILED--File already looks unprotected.\r\n");
	else
	{
		//Unscramble the data.
		if (CFiles::UnprotectFile(pszFilePath))
			printf("SUCCESS--File unprotected.\r\n");
		else printf("FAILED--File could not be read/written.\r\n");
	}
}

//******************************************************************************************
void PrintCompressHelp()
{
	PrintHeader();
	printf(
		"compress    SrcFilePath DestFilePath\r\n"
		"\r\n"
		"Compresses the contents of a file so that it can be imported by DROD.\r\n");
}

//******************************************************************************************
void PrintCompress(
//Compress a data file.  See PrintCompressHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszSrcFilePath,  //(in)
	const WCHAR *pszDestFilePath) //(in)
{
	PrintHeader();

	if (!Options.AreOptionsValid(wszEmpty)) return;
	if (!pszSrcFilePath)
	{
		printf("FAILED--Must specify file to compress and destination file.\r\n");
		return;
	}
	if (!pszDestFilePath)
	{
		printf("FAILED--Must specify destination file.\r\n");
		return;
	}

	CStretchyBuffer buffer;
	if (!CFiles::ReadFileIntoBuffer(pszSrcFilePath, buffer))
	{
		printf("FAILED--Unable to open source file.\r\n");
		return;
	}

	// Compress the data.
	const UINT srcLen = buffer.Size()-2; // remove the NULL wchar at the end
	ULONG destLen = (ULONG) (1.01 * srcLen) + 13;
	BYTE *dest = new BYTE[destLen];
	const int res = compress(dest, &destLen, (const BYTE*)buffer, srcLen);
	ASSERT(res == Z_OK);
	CStretchyBuffer output(dest, destLen); //no terminating null
	output.Encode();
	CFiles::WriteBufferToFile(pszDestFilePath, output);
	delete dest;

	printf("SUCCESS--File compressed.\r\n");
}

//******************************************************************************************
void PrintUncompressHelp()
{
	PrintHeader();
	printf(
		"uncompress    SrcFilePath DestFilePath\r\n"
		"\r\n"
		"Uncompresses the contents of an exported file so that it can be read\r\n"
		"by humans.\r\n");
}

//******************************************************************************************
void PrintUncompress(
//Scramble a data file.  See PrintProtectHelp for more info.
//
//Params:
	const COptionList &Options,   //(in)
	const WCHAR *pszSrcFilePath,  //(in)
	const WCHAR *pszDestFilePath) //(in)
{
	PrintHeader();

	if (!Options.AreOptionsValid(wszEmpty)) return;
	if (!pszSrcFilePath)
	{
		printf("FAILED--Must specify file to uncompress and destination file.\r\n");
		return;
	}
	if (!pszDestFilePath)
	{
		printf("FAILED--Must specify destination file.\r\n");
		return;
	}

	CStretchyBuffer buffer;
	if (!CFiles::ReadFileIntoBuffer(pszSrcFilePath, buffer))
	{
		printf("FAILED--Unable to open source file.\r\n");
		return;
	}
	buffer.Decode();
	// Uncompress the data stream.
	BYTE *decodedBuf;
	ULONG decodedSize;
	if (!buffer.Uncompress(decodedBuf, decodedSize))
	{
		printf("FAILED--Memory/data error during uncompression.\r\n");
		return;
	}

	CStretchyBuffer output(decodedBuf, decodedSize);
	if (!CFiles::WriteBufferToFile(pszDestFilePath, output))
	{
		printf("FAILED--Cannot write destination file.\r\n");
		return;
	}

	printf("SUCCESS--File uncompressed.\r\n");
}

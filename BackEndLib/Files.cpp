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
 * Contributor(s):
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

//********************************************************************************************
//File operations module.
//
//Note: Be careful about infinite loops caused by calling ASSERT, VERIFY, or LogErr in this
//module.
//********************************************************************************************

#ifdef WIN32
#	include <windows.h> //Should be first include.
#	include <errno.h>
#	include <direct.h>
#endif

#include "Files.h"
#include "Assert.h"
#include "IniFile.h"
#include "Date.h"
#include "Wchar.h"

#include <string.h>
#include <stdio.h>
#include <mk4.h>
#ifdef WIN32
#	include <io.h>
#	include <shlobj.h>
#	pragma comment(lib,"shlwapi.lib")
#endif
#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
#  include <unistd.h>
#  include <dirent.h>
#  include <sys/types.h>
#  include <sys/stat.h>
#  include <fcntl.h>
#  include <cstdlib>
#endif

#include <assert.h>
#include <string>
#include <set>
using std::string;

#ifndef WIN32
#include <errno.h>
#endif

#include <sys/stat.h>

//Affects what is logged in AppendErrorLog().
string  m_strUnwrittenContext;
UINT    m_wIndentLevel = 0;

char *lpszStartupPath=NULL;

//Initialize static members of class.
WSTRING CFiles::wszAppPath;
WSTRING CFiles::wszDatPath;
WSTRING CFiles::wszResPath;
WSTRING CFiles::wCompanyName;
WSTRING CFiles::wGameName;
WSTRING CFiles::wGameVer;
WSTRING CFiles::wUniqueResFile;
UINT CFiles::dwRefCount = 0;
CIniFile CFiles::gameIni;
bool CFiles::bInitedIni = false;
bool CFiles::bIsDemo = false;
bool CFiles::bad_data_path_file = false;
WSTRING CFiles::wstrHomePath;

#ifdef WIN32
bool CFiles::bDrives[NUMDRIVES];
bool CFiles::bDrivesChecked = false;
bool CFiles::bWindowsDataFilesInUserSpecificDir = true;
#endif

#ifdef USE_XDG_BASEDIR_SPEC
WSTRING CFiles::wstrHomeConfPath;
#endif

//Some constants
const WCHAR wszDefExportDir[] = {We('H'),We('o'),We('m'),We('e'),We('m'),We('a'),We('d'),We('e'),We(0) };
static const WCHAR wszDat[] = {We('.'),We('d'),We('a'),We('t'),We(0)};
static const WCHAR wszData[] = {We('D'),We('a'),We('t'),We('a'),We(0)};
static const WCHAR wszDataPathDotTxt[] = {We('D'),We('a'),We('t'),We('a'),We('P'),We('a'),We('t'),We('h'),We('.'),We('t'),We('x'),We('t'),We(0)};
static const WCHAR wszDemo[] = {We('-'),We('d'),We('e'),We('m'),We('o'),We(0)};
static const WCHAR wszCompanyName[] = {We('C'),We('a'),We('r'),We('a'),We('v'),We('e'),We('l'),We(' '),We('G'),We('a'),We('m'),We('e'),We('s'),We(0)}; // "Caravel Games"

vector<string> CFiles::datFiles;
vector<string> CFiles::playerDataSubDirs;

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
WSTRING CFiles::wstrDataPathDotTxtPath;

//Constants used in default path construction/search
#ifndef __APPLE__
# ifdef STEAMBUILD
// steam saves are always in "$XDG_DATA_HOME/Caravel"
static const WCHAR wszSteamHomeConfDir[] = WS("Caravel");
#  ifndef USE_XDG_BASEDIR_SPEC
#   error "steam build should use xdg on linux"
#  endif
# else
// ~/.caravel -- only used if saves already exist there
static const WCHAR wszOldHomeConfDir[] = WS(".caravel");
# endif // STEAMBUILD
#else
static const WCHAR wszHomeConfDir[] =
	{We('L'),We('i'),We('b'),We('r'),We('a'),We('r'),We('y'),We('/'),
	 We('A'),We('p'),We('p'),We('l'),We('i'),We('c'),We('a'),We('t'),We('i'),We('o'),We('n'),We(' '),We('S'),We('u'),We('p'),We('p'),We('o'),We('r'),We('t'),We('/'),
	 We('C'),We('a'),We('r'),We('a'),We('v'),We('e'),We('l'),
	 We(0)};
#endif
static const WCHAR wszTempPath[] = {We(SLASH),We('t'),We('m'),We('p'),We(0)};
static const WCHAR wszLocalizationDir[] = {We('L'),We('1'),We('0'),We('n'),We(0)};

#define IOBUFFERSIZE 65536

#else //#if defined __linux__ || defined __FreeBSD__ || defined __APPLE__
static const WCHAR wszHomeConfDir[] =
	{We('C'),We('a'),We('r'),We('a'),We('v'),We('e'),We('l'),We(0)};
static const WCHAR wszOldDatDirResourceFile[] =
	{We('p'),We('l'),We('a'),We('y'),We('e'),We('r'),We('.'),We('d'),We('a'),We('t'),We(0)};
#endif

const WCHAR *const CFiles::wszDemo = ::wszDemo;
#ifndef USE_XDG_BASEDIR_SPEC
const WCHAR *const CFiles::wszHomeConfDir = ::wszHomeConfDir;
#endif

#ifdef HAS_UNICODE
static const WCHAR UNICODE_BOM = 0xfeff;
#endif

//
//Public methods.
//

//******************************************************************************
CFiles::CFiles(
//Constructor for first instantiation.
//
//Params:
	const WCHAR *wszSetAppPath,   //(in) where the app is being executed from
	const WCHAR *wszSetGameName,  //(in) base name of game
	const WCHAR *wszSetGameVer,   //(in) game major version -- for finding resource files or paths
	const bool bIsDemo,           //(in) whether we should be looking for files for a demo version or not [default=false]
	const bool confirm_resource_file)
{
	ASSERT(this->dwRefCount == 0);

	ASSERT(wszSetAppPath != NULL);
	ASSERT(wszSetGameName != NULL);
	ASSERT(wszSetGameVer != NULL);

	InitClass(wszSetAppPath, wszSetGameName, wszSetGameVer, bIsDemo, confirm_resource_file);

	++this->dwRefCount;
}

//Call this one once the class has been inited.
CFiles::CFiles()
{
	ASSERT(this->dwRefCount > 0);
	++this->dwRefCount;
}

//******************************************************************************
CFiles::~CFiles()
//Destructor.
{
	--this->dwRefCount;
	if (this->dwRefCount == 0)
		DeinitClass();
}

//******************************************************************************
bool CFiles::CreatePathIfInvalid(
//Creates path and all required parent directories (unless they already exist).
//
//Params:
	const WCHAR *wszPath)   //(in)  The path that should be created.
//
//Returns: True if successful.
{
	ASSERT(wszPath);
	WSTRING wstrPath = wszPath;
	const UINT len = wstrPath.length();

	bool bOk;
	UINT i = 0;

	while (wstrPath[i] != SLASH && i < len) i++;  // Put i onto the first SLASH (to bypass "C:" in windows)

	while (true)
	{
		for (++i; i < len && wstrPath[i] != SLASH; ++i)
			;
		if (wstrPath[i] == SLASH)
			wstrPath[i] = W_t(0);
		if (!(bOk = MakeDirectory(wstrPath.c_str())) || i == len)
			break;
		wstrPath[i] = W_t(SLASH);
	}

	return bOk;
}

//*****************************************************************************
bool CFiles::MakeDirectory(const WCHAR *pwzPath)
//Create a directory.
//
//Returns: true if directory exists or was just created, false if not.
{
	ASSERT(pwzPath);
#ifdef WIN32
	int ret;
	if (CFiles::WindowsCanBrowseUnicode())
		ret = _wmkdir(pwzPath);
	else {
		const string path = UnicodeToAscii(pwzPath);
		ret = _mkdir(path.c_str());
	}
	if (ret == 0)
		return true;
	return errno == EEXIST;
#elif defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
	string path;
	UnicodeToCPath(pwzPath, path);

	struct stat st;
	return ((access(path.c_str(), R_OK | X_OK) || stat(path.c_str(), &st))
				? !mkdir(path.c_str(), 0755) : S_ISDIR(st.st_mode));
#else
#	error Implement CFiles::MakeDirectory()
#endif
}

//******************************************************************************
//Code for determining the "HOME" path we should use for the user's app data

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
//******************************************************************************
#define UWP_BUFFER 256
#define UWP_BUFPAD 256
#undef USE_GET_CURRENT_DIR_NAME

static bool GetNormalizedPath (
// Normalize path (make it absolute without any relative bits),
// and put it in the passed WSTRING.
	const char *path,
	WSTRING &wstr)
//
//Returns:
//True if successful.
{
	wstr.clear();

	char *newpath, prevchar;
	if (!path) return false;

	UINT index = 0, dotlevel = 0;
	bool newelement = true;

	if (*path != SLASH) {
		char *tmp;
#ifdef USE_GET_CURRENT_DIR_NAME
		if (!(newpath = get_current_dir_name()))
			return false;
#else
		if (!(newpath = (char*)malloc(UWP_BUFFER)))
			return false;
		for (index = UWP_BUFFER; getcwd(newpath, index) == NULL; newpath = tmp)
		{
			if ((errno != ERANGE) ||
				(!(tmp = (char*)realloc(newpath, index += UWP_BUFPAD))))
			{
				free(newpath);
				return false;
			}
		}
#endif
		index = strlen(newpath);
		tmp = (char*)realloc(newpath, index + strlen(path) + 2);
		if (!tmp)
		{
			free(newpath);
			return false;
		}
		newpath = tmp;
		newpath[index++] = SLASH;
	}
	else
	{
		newpath = (char*)malloc(strlen(path) + 1);
		if (!newpath)
			return false;
	}

	for (prevchar = 0; (newpath[index] = *path);
		++index, prevchar = *(path++))
	{
		if (newelement)
		{
			if (*path == SLASH)
			{
				if (prevchar == SLASH)
					--index;
				else
				{
					index -= dotlevel;
					while (dotlevel--)
						while (index && newpath[--index] != SLASH) ;
					dotlevel = 0;
				}
			}
			else if ((*path == '.' && ++dotlevel > 2) || *path != '.')
				newelement = false;
			continue;
		}
		if (*path == SLASH)
		{
			newelement = true;
			dotlevel = 0;
		}
	}
	if (newelement)
	{
		while (dotlevel--)
			while (index && newpath[--index] != SLASH) ;
		if (!index) ++index;
	}

	if (index > 1 && newpath[index - 1] == SLASH)
		--index;
	newpath[index] = 0;

	CFiles::CPathToUnicode(newpath, wstr);

	free(newpath);
	return true;
}

static bool GetNormalizedPathEnv (
	const char *env,
	WSTRING &wstr)
{
	return GetNormalizedPath(getenv(env), wstr);
}

// endif defined __linux__ || defined __FreeBSD__ || defined __APPLE__
#else

WSTRING CFiles::GetUserspacePath(bool bUserSpecificDir)
//User-specific dir: %USERPROFILE%\My Documents
//Otherwise, get path for non-user specific, non-roaming data (e.g., "ProgramData").
{
	WSTRING prodPath;
	TCHAR szPath[MAX_PATH];
	if (SUCCEEDED(SHGetFolderPath(NULL, bUserSpecificDir ? CSIDL_PERSONAL : CSIDL_COMMON_APPDATA, NULL, 0, szPath))) {
		prodPath = szPath;
	}
	return prodPath;
}

#endif

const WSTRING CFiles::GetGameConfPath() {
#ifdef USE_XDG_BASEDIR_SPEC
	WSTRING path = CFiles::wstrHomeConfPath + wszSlash
#else
	WSTRING path = CFiles::wstrHomePath + wszSlash
		+ CFiles::wszHomeConfDir + wszSlash
#endif
		+ CFiles::wGameName + wszHyphen	+ CFiles::wGameVer;
	if (CFiles::bIsDemo)
		path += CFiles::wszDemo;
	return path;
}

//******************************************************************************
void CFiles::TryToFindDataPath()
//Error-- try to find the Data dir myself and create a new DataPath.txt.
{
	WSTRING wstrDatPathTxt;

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)

	WSTRING datfile = CFiles::wstrDataPathDotTxtPath + wszSlash + wUniqueResFile;
	if (CFiles::wstrDataPathDotTxtPath.empty() || !DoesFileExist(datfile.c_str()))
		CFiles::wstrDataPathDotTxtPath = GetGameConfPath();

	CFiles::wszDatPath = CFiles::wstrDataPathDotTxtPath;
	CFiles::wszResPath = CFiles::GetAppPath() + wszSlash + wszData;

	wstrDatPathTxt = CFiles::GetDatPath() + wszSlash + wszDataPathDotTxt;
	WriteDataPathTxt(wstrDatPathTxt.c_str(), CFiles::GetDatPath().c_str(), CFiles::GetResPath().c_str());

#else
	if (!CFiles::wstrHomePath.empty()) {
		wstrDatPathTxt = CFiles::wszDatPath = GetGameConfPath();
	} else {
		wstrDatPathTxt = CFiles::wszAppPath;
	}
	wstrDatPathTxt += wszSlash;
	wstrDatPathTxt += wszDataPathDotTxt;

	if (!FindPossibleDatPath(CFiles::wszAppPath.c_str(), CFiles::wszDatPath) ||
		!FindPossibleResPath(CFiles::wszAppPath.c_str(), CFiles::wszResPath))
	{
		//Use app path so at least error logging may work later.
		CFiles::wszDatPath = CFiles::wszAppPath;
		CFiles::wszResPath = CFiles::wszAppPath;
	}
#endif
}

//*****************************************************************************
bool CFiles::DoesFileExist(
//Determines if a specified file exists by trying to open it.
//
//Params:
  const WCHAR *wszFilepath) //(in)
//
//Returns:
//True if file exist, false if not.
{
	FILE *pFile = Open(wszFilepath, "rb");
	if (NULL == pFile) {
		const int nErr = errno;
		return !(ENOTDIR == nErr || ENOENT == nErr || EACCES == nErr || EINVAL == nErr);
	}
	fclose(pFile);
	return true;
}

//UTF-8/ASCII version.
bool CFiles::DoesFileExist(const char *szFilepath)
{
	 WSTRING wstr;
	 CPathToUnicode(szFilepath, wstr);
	 return DoesFileExist(wstr.c_str());
}

//*****************************************************************************
bool CFiles::EraseFile(
//Erases a file from disk.
//
//Params:
	const WCHAR *wszFilepath) //(in)
//
//Returns:
//True if erase operation succeeded.
{
#ifdef HAS_UNICODE
	return !_wunlink(wszFilepath);
#else
	std::string fp;
	UnicodeToCPath(wszFilepath, fp);
	return !unlink(fp.c_str());
#endif
}

//UTF-8/ASCII version.
bool CFiles::EraseFile(const char *szFilepath)
{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
	return !unlink(szFilepath);
#else
	return !_unlink(szFilepath);
#endif
}

//*****************************************************************************
bool CFiles::RenameFile(
//Renames a file from disk.
//
//Params:
	const WCHAR *wszOldFilepath, const WCHAR *wszNewFilepath) //(in)
//
//Returns:
//True if erase operation succeeded.
{
#ifdef HAS_UNICODE
	return !_wrename(wszOldFilepath, wszNewFilepath);
#else
	std::string op, np;
	UnicodeToCPath(wszOldFilepath, op);
	UnicodeToCPath(wszNewFilepath, np);
	return !rename(op.c_str(), np.c_str());
#endif
}

//UTF-8/ASCII version.
bool CFiles::RenameFile(const char *szOldFilepath, const char *szNewFilepath)
{
	return !rename(szOldFilepath, szNewFilepath);
}

//*****************************************************************************
FILE * CFiles::Open(
//Unicode-friendly function to open a file.
	 const WCHAR *pwzFilepath,   //(in)    Full path to file.
	 const char *pszOptions)     //(in)    fopen()-style options.
//Returns:
//Pointer to file or NULL if an error occurred.  Use fclose() to close file later.
{
	 FILE *pFile = NULL;

	 //Unicode open.
#ifdef HAS_UNICODE
#   ifdef   WIN32
#      define WFOPEN _wfopen
#   endif

	 WSTRING wstrOptions;
	 AsciiToUnicode(pszOptions, wstrOptions);
	 pFile = WFOPEN( pwzFilepath, wstrOptions.c_str() );

	 //If failed, will try again with ASCII conversion in case wfopen() is stubbed on the O/S.
	 if (!pFile)
#endif
	 //UTF-8/ASCII open.
	 {
		string strPath;
		UnicodeToCPath(pwzFilepath, strPath);
		pFile = fopen(strPath.c_str(), pszOptions);
	 }

	 return pFile;
}

//*****************************************************************************
bool CFiles::HasReadWriteAccess(
//Determines if a specified file has read/write access by trying to open it.
//
//Params:
  const WCHAR *wszFilepath) //(in)
//
//Returns:
//True if file exists, false if not.
{
	FILE* pFile = Open(wszFilepath, "rb+");
	if (pFile == NULL)
		return false;
	fclose(pFile);
	return true;
}

//*****************************************************************************
bool CFiles::MakeFileWritable(
//Tries to make the specified file writable.
//
//Params:
  const WCHAR *wszFilepath) //(in) The file!
//
//Returns:
//True if the attempt was successful, false otherwise
{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
	string sFile;
	UnicodeToCPath(wszFilepath, sFile);
	return !chmod(sFile.c_str(), 0644) && HasReadWriteAccess(wszFilepath);
#else
//#warning No code to make files writable for this platform
	return false;
#endif
}

//******************************************************************************
void CFiles::AppendErrorLog(
//Appends text to the error log.
//
//Params:
	const char *pszText) //(in) Text to write at end of file.
{
	assert(pszText);

	WSTRING wstrExtension;
	AsciiToUnicode(".err", wstrExtension);

	WSTRING wstrDatPathTxt = GetDatPath();
	wstrDatPathTxt += wszSlash;
	wstrDatPathTxt += CFiles::wGameName;
	wstrDatPathTxt += wstrExtension;

	AppendLog(wstrDatPathTxt, pszText);
}

//******************************************************************************
void CFiles::AppendUserLog(
//Appends text to the user log.
//
//Params:
	const char *pszText) //(in) Text to write at end of file.
{
	assert(pszText);

	WSTRING wstrExtension;
	AsciiToUnicode(".log", wstrExtension);

	WSTRING wstrDatPathTxt = GetDatPath();
	wstrDatPathTxt += wszSlash;
	wstrDatPathTxt += CFiles::wGameName;
	wstrDatPathTxt += wstrExtension;

	AppendLog(wstrDatPathTxt, pszText);
}

//******************************************************************************
void CFiles::AppendLog(
//Appends text to the error log.
//
//Params:
	const WSTRING& wstrDatPathTxt, //file name
	const char *pszText) //(in) Text to write at end of file.
{
	static CDate LastLog;
	static bool bFirstLog = true;

	assert(pszText);

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
	// Make errorlog world-writable
	mode_t oldmask = umask(0);
#endif
	//append+binary mode -- to not modify the way we write out newlines below
	FILE* pFile = Open(wstrDatPathTxt.c_str(), "ab");
	if (pFile)
	{
		//Write any log context that hasn't been written already.
		if (!m_strUnwrittenContext.empty())
		{
			fwrite(m_strUnwrittenContext.c_str(), 1, m_strUnwrittenContext.size(), pFile);
			m_strUnwrittenContext = "";
		}

		string strIndent;
		for (UINT wIndentNo = 0; wIndentNo < m_wIndentLevel; ++wIndentNo) strIndent += "  ";

		//Write time if this is first log or at least a minute has elapsed since last write.
		CDate Now;
		if ((time_t) Now - (time_t) LastLog > 60 || bFirstLog)
		{
			WSTRING wstrTime;
			string strTime;
			Now.GetLocalFormattedText(DF_SHORT_DATE | DF_SHORT_TIME, wstrTime);

			strTime = strIndent;
			strTime += "*** ";
			if (bFirstLog)
			{
				strTime += "FIRST LOG IN SESSION ";
				bFirstLog = false;
			}
			strTime += UnicodeToAscii(wstrTime);
			strTime += " ***" NEWLINE;
			fwrite(strTime.c_str(), 1, strTime.size(), pFile);
		}

		//Write the actual text passed to method.
		fwrite(strIndent.c_str(), 1, strIndent.size(), pFile);
		fwrite(pszText, 1, strlen(pszText), pFile);
		fclose(pFile);
		LastLog = Now;
	}
#if defined __linux__ || defined __APPLE__
	// Restore old file creation mask.
	umask(oldmask);
#endif
}

//******************************************************************************
bool CFiles::DeleteINIEntry(const char *pszSection, const char *pszKey, const char *pszValue)
{
	return CFiles::gameIni.DeleteEntry(pszSection, pszKey, pszValue);
}

//******************************************************************************
bool CFiles::WriteGameProfileString(
//Writes a string to <GameName>.INI.
//
//Params:
  const char *pszSection,  //(in)
  const char *pszKey,      //(in)
  const char *pszValue)    //(in)
//
//Returns:
//True if successful, false if not.
{
	gameIni.WriteString(pszSection, pszKey, pszValue);
	return true;
}

//*********************************
bool CFiles::WriteGameProfileString(
	const char *pszSection, const WCHAR* pwszKey, const char *pszValue)
{
	char pszKey[256];
	UnicodeToAscii(pwszKey, pszKey);
	gameIni.WriteString(pszSection, pszKey, pszValue);
	return true;
}

bool CFiles::WriteGameProfileString(
	const char *pszSection, const WCHAR* pwszKey, const list<WSTRING>& wstrValue)
{
	char pszKey[256];
	UnicodeToAscii(pwszKey, pszKey);
	gameIni.WriteString(pszSection, pszKey, wstrValue);
	return true;
}

//******************************************************************************
bool CFiles::WriteGameProfileBuffer(const WSTRING& wstr,
	const bool bOverwrite, const bool bAddDuplicates) //[default=true,false]
//Writes a buffer of entries to <GameName>.INI.
{
	return gameIni.LoadText(wstr, bOverwrite, bAddDuplicates);
}

//******************************************************************************
bool CFiles::GetGameProfileString(
//Gets a string from <GameName>.INI.
//
//Params:
	const char *pszSection, //(in)
	const char *pszKey,     //(in)
	string& strValue)    //(out)
//
//Returns:
//True if successfully found entry or false if not.
{
	return gameIni.GetString(pszSection, pszKey, strValue);
}

//*********************************
bool CFiles::GetGameProfileString(
	const char *pszSection, const char *pszKey, list<string>& strValue)
{
	return gameIni.GetString(pszSection, pszKey, strValue);
}

//*********************************
bool CFiles::GetGameProfileString(
	const char *pszSection, const char *pszKey, list<WSTRING>& strValue)
{
	return gameIni.GetString(pszSection, pszKey, strValue);
}

//*********************************
bool CFiles::GetGameProfileString(
	const char *pszSection, const WCHAR* pwszKey, list<WSTRING>& strValue)
{
	char pszKey[1024];
	UnicodeToAscii(pwszKey, pszKey);
	return gameIni.GetString(pszSection, pszKey, strValue);
}

//******************************************************************************
bool CFiles::ReadFileIntoBuffer(
//Reads a file into a buffer.  Preferably only be used with small files.
//
//Params:
	const WCHAR *wszFilepath,  //(in)   Full path to file to be read.
	CStretchyBuffer &Buffer,   //(out)  Receives file data.
	bool bBinary)              //(in)   If true, doesn't add a NULL to the end
//
//Returns:
//True if successful, false if not.
{
	bool bSuccess = false;
	ASSERT(wszFilepath);
	ASSERT(Buffer.Size() == 0);

	FILE* pFile = Open( wszFilepath, "rb" );
	BYTE charBuf[16385];
	if (NULL != pFile)
	{
		//Get size of file.
		fseek(pFile, 0, SEEK_END);
		if (!Buffer.Alloc(ftell(pFile)))
		{
			fclose(pFile);
			return false;
		}
		fseek(pFile, 0, SEEK_SET);

		int bytes;
		do {
			bytes = fread( charBuf, 1, 16384, pFile );
			if (bytes > 0) {
				Buffer.Append( charBuf, bytes );
			}
		} while (bytes > 0);
		if (!bBinary)
			Buffer.Append((const BYTE*)wszEmpty, sizeof(WCHAR));
		fclose( pFile );
		bSuccess = true;
	}
	else
	{
		//char buf[MAX_PATH+1];
		//UnicodeToCPath(wszFilepath, buf);
		bSuccess = false;
	}

	return bSuccess;
}

//******************************************************************************
bool CFiles::WriteBufferToFile(
//Writes a file from a buffer.  Overwrites an existing file.
//
//Params:
	const char *pszFilepath,       //(in)   Full path to file to be written.
	const CStretchyBuffer &Buffer, //(in)   Data to write.
	const bool bAppend) //[default=false]
//
//Returns:
//True if successful, false if not.
{
	bool bSuccess = false;
	ASSERT(pszFilepath);
	ASSERT(Buffer.Size() != 0);

	FILE* pFile = fopen(pszFilepath, bAppend ? "ab" : "wb");
	if (NULL != pFile)
	{
		bSuccess = (Buffer.Size() == fwrite( (BYTE*)Buffer, 1, Buffer.Size(), pFile ));
		fclose( pFile );
	}

	return bSuccess;
}

//******************************************************************************
bool CFiles::WriteBufferToFile(
//Writes a file from a buffer.  Overwrites an existing file.
//
//Params:
	const WCHAR *pwszFilepath,     //(in)   Full path to file to be written.
	const CStretchyBuffer &Buffer, //(in)   Data to write.
	const bool bAppend) //[default=false]
//
//Returns:
//True if successful, false if not.
{
	bool bSuccess = false;
	ASSERT(pwszFilepath);
	ASSERT(Buffer.Size() != 0);

	FILE* pFile = Open(pwszFilepath, bAppend ? "ab" : "wb");
	if (NULL != pFile)
	{
		bSuccess = (Buffer.Size() ==
				fwrite( (BYTE*)Buffer, sizeof(BYTE), Buffer.Size(), pFile ));
		if (!bSuccess)
			printf("Error %i %s\n", ferror(pFile), strerror(errno));
		fclose(pFile);
	}

	return bSuccess;
}

//******************************************************************************
void CFiles::MutateFileName(
//Unprotected and protected datafiles differ by their last letter.
//This routine changes one variant to the other.
//Params:
	WCHAR *wszFilepath)     //(in/out)  Full path to file to be read.
							//Mutates file name by changing last letter.
{
	const UINT nLength = WCSlen(wszFilepath);
	if (FileIsEncrypted(wszFilepath))
	{
		switch (WCv(wszFilepath[nLength-2]))
		{
		case 'm':
			WCv(wszFilepath[nLength-1]) = 'p';
			break;
		case 'a':
			WCv(wszFilepath[nLength-1]) = 'v';
			break;
		case '3':
			WCv(wszFilepath[nLength-1]) = 'm';
			break;
		default: //the general ".dat"
			WCv(wszFilepath[nLength-1]) = 't';
			break;
		}
	} else {
		WCv(wszFilepath[nLength-1]) = '_';
	}
}

//******************************************************************************
bool CFiles::ProtectFile(
//Encodes a file.
//
//Params:
	const WCHAR *pszFilepath)  //(in)   Full path to file to be read.
//
//Returns:
//True if successful, false if not.
{
	bool bSuccess = false;
	CStretchyBuffer buffer;

	if (ReadFileIntoBuffer(pszFilepath,buffer))
	{
		buffer.Encode();

		//Get name of destination file.
		WSTRING destFilePath = pszFilepath;
		MutateFileName(&*destFilePath.begin());

		FILE* pFile = Open(destFilePath.c_str(), "wb");
		if (NULL != pFile)
		{
			if (buffer.Size() == fwrite( (BYTE*)buffer, 1, buffer.Size(), pFile ))
				bSuccess = true;
			fclose(pFile);
		}
	}

	return bSuccess;
}

//******************************************************************************
void CFiles::InitAppVars(const WCHAR* wszUniqueResFile, const vector<string>& datFiles, const vector<string>& playerDataSubDirs)
{
	assert(wszUniqueResFile);
	CFiles::wUniqueResFile = wszUniqueResFile;
	CFiles::datFiles = datFiles;
	CFiles::playerDataSubDirs = playerDataSubDirs;
}

//
//Private methods.
//

//******************************************************************************
void CFiles::InitClass(
//Sets up static members of class.
//
//Params:
	const WCHAR *wszSetAppPath,      //(in)
	const WCHAR *wszSetGameName,     //(in)
	const WCHAR *wszSetGameVer,      //(in)
	const bool bIsDemo,              //(in)
	const bool confirm_resource_file)
{
	ASSERT(wszSetAppPath);
	ASSERT(wszSetGameName);
	ASSERT(wszSetGameVer);

	CFiles::bIsDemo = bIsDemo;

	CFiles::wCompanyName = wszCompanyName;

	//Get full path to executable.
	ASSERT(CFiles::wszAppPath.empty());
	CFiles::wszAppPath = wszSetAppPath;

	//Get game name and version.
	ASSERT(CFiles::wGameName.empty());
	CFiles::wGameName = wszSetGameName;
	ASSERT(this->wGameVer.empty());
	CFiles::wGameVer = wszSetGameVer;

	//Truncate app path to just the path portion.
	int lastBackslashIndex = CFiles::wszAppPath.size(); //start at end
	while (lastBackslashIndex > 0 && CFiles::wszAppPath[lastBackslashIndex] != SLASH)
		--lastBackslashIndex; //Search to last backslash.
	CFiles::wszAppPath.resize(lastBackslashIndex); //Truncate the string.
#ifdef __APPLE__
	//if the executable is inside a bundle, look for data files within
	//that bundle's Resources directory
	WSTRING path = CFiles::wszAppPath;
	static const WCHAR wszMacOS[] = {
        	We('M'),We('a'),We('c'),We('O'),We('S'),We(0)
	};
	static const WCHAR wszResources[] = {
        	We('R'),We('e'),We('s'),We('o'),We('u'),We('r'),We('c'),We('e'),We('s'),
        	We(0)
	};
	WSTRING::size_type pos = path.rfind(wszMacOS);
	if (pos != WSTRING::npos) {
		path = WSTRING(path,0,pos) + wszResources;
	}
	CFiles::wszAppPath = path;
#elif defined(__linux__) || defined(__FreeBSD__)
	// handle multiarch directory layout on linux and friends
	WSTRING path = CFiles::wszAppPath;
	static const WCHAR wszPhath[] = { We('/'),We('B'),We('i'),We('n'),We('/'),We('l'),We('i'),We('n'),We('u'),We('x'),We('-'),We('x'),We(0) };  // "/Bin/linux-x"
	WSTRING::size_type pos = path.rfind(wszPhath);
	if (pos != WSTRING::npos)
		path.resize(pos);
	CFiles::wszAppPath = path;

	// add script bin to end of path
	std::string binpath = getenv("PATH");
	if (!binpath.empty())
		binpath += ":";
	binpath += UnicodeToCPath(CFiles::wszAppPath);
	binpath += "/Bin/xdg-utils";
	setenv("PATH", binpath.c_str(), 1);
#endif

	SetupHomePath();

	GetDatPathFromDataPathDotTxt();

	if (confirm_resource_file) {
		const bool validDataPath = DoesFileExist((CFiles::wszResPath + wszSlash + CFiles::wUniqueResFile).c_str());
		if (!validDataPath) {
			CFiles::bad_data_path_file = true;
			TryToFindDataPath();
		}
	}

	SetupHomePathSubDirs();

	InitINI();
}

void CFiles::SetupHomePath()
{
#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)

	if (!GetNormalizedPathEnv("HOME", CFiles::wstrHomePath))
		CFiles::wstrHomePath = wszTempPath;

# ifdef USE_XDG_BASEDIR_SPEC

	// 1. if new (xdg) config path already exists, use it
	// 2. otherwise, if old config path exists, use that
	// 3. otherwise, create and use new config path

	WSTRING newconfpath;

	if (!GetNormalizedPathEnv("XDG_DATA_HOME", newconfpath))
	{
		// xdg basedir spec says XDG_DATA_HOME should default to "$HOME/.local/share" if not set
		newconfpath = CFiles::wstrHomePath + WS("/.local/share");
	}
	newconfpath += wszSlash;
#  ifdef STEAMBUILD
	newconfpath += wszSteamHomeConfDir;
	CFiles::wstrHomeConfPath = newconfpath;
#  else
	newconfpath += CFiles::wCompanyName;
	CFiles::wstrHomeConfPath = newconfpath;
	if (!IsValidPath(GetGameConfPath().c_str()))
	{
		CFiles::wstrHomeConfPath = CFiles::wstrHomePath + wszSlash + wszOldHomeConfDir;  // old config path
		if (!IsValidPath(GetGameConfPath().c_str()))
		{
			CFiles::wstrHomeConfPath = newconfpath;
			// created below
		}
	}
#  endif
# endif

#else
	CFiles::wstrHomePath = CFiles::GetUserspacePath(CFiles::bWindowsDataFilesInUserSpecificDir);
#endif

	if (!CFiles::wstrHomePath.empty()) {
		//Need to create:
		//<player data path>
		//<player data path>/Data
		WSTRING datapath = GetGameConfPath();
		CreatePathIfInvalid(datapath.c_str());

		// Subdir creation may require wszResPath initialized, so do that later
	}
}

void CFiles::SetupHomePathSubDirs()
{
	if (!CFiles::wstrHomePath.empty()) {
		WSTRING datapath = GetGameConfPath();
		//Create any specified sub-dirs
		for (vector<string>::const_iterator it=CFiles::playerDataSubDirs.begin();
				it!=CFiles::playerDataSubDirs.end(); ++it) {
			WSTRING subdir;
			const char firstch = (*it)[0];
			AsciiToUnicode(&(it->c_str()[(firstch == '+' || firstch == '-') ? 1 : 0]), subdir);
			const WSTRING subdirpath = datapath + wszSlash + subdir;
			CreatePathIfInvalid(subdirpath.c_str());
			if (firstch == '+')
			{
#if defined(__linux__) || defined (__FreeBSD__) || defined(__APPLE__)
				DirectoryCopy(CFiles::GetResPath() + wszSlash + subdir, subdirpath, 0, true, true);
#else
				ASSERT(!"Directory copy requested but not implemented for this platform");
#endif
			}
		}
	}
}

void CFiles::GetDatPathFromDataPathDotTxt()
{
	ASSERT(CFiles::wszDatPath.empty());
	ASSERT(CFiles::wszResPath.empty());

	//Get dat path from DataPath.txt.
	if (!FindDataPathDotTxt(CFiles::wszDatPath)) {
		TryToFindDataPath();
	}

	WSTRING wstrDatPathTxt = CFiles::wszDatPath;
	wstrDatPathTxt += wszSlash;
	wstrDatPathTxt += wszDataPathDotTxt;

	FILE* pFile = Open(wstrDatPathTxt.c_str(), "r");
	if (pFile) {
		if (!
#if defined(HAS_UNICODE)
		ParseDataFileUnicode(pFile)
#else
		ParseDataFileCPath(pFile)
#endif
		) {
			CFiles::bad_data_path_file = true;
			TryToFindDataPath();
		}
		fclose( pFile );
	} else {
#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
		TryToFindDataPath();
#endif
	}
}

#if defined(HAS_UNICODE)
bool CFiles::ParseDataFileUnicode(FILE* pFile)
{
	WCHAR buffer[2 * (MAX_PATH + 1)];
	fgetWs(buffer, 2 * (MAX_PATH + 1), pFile);

	//Skip possible BOM header chars.
	UINT wFirstChar = 0;
	if (buffer[wFirstChar] == UNICODE_BOM)
		++wFirstChar;

	UINT i = wFirstChar;
	while (i < MAX_PATH && buffer[i] != ';')
		++i;
	if (buffer[i] != ';' || !buffer[0] || !buffer[i + 1] || WCSlen(buffer + i + 1) > MAX_PATH) {
		if (wFirstChar == 0) {
			//Attempt parsing other text formats.
			rewind(pFile);
			return ParseDataFileCPath(pFile);
		}
		return false;
	}

	UINT j = i+1;
	buffer[i] = 0;
	while (buffer[j] && buffer[j] != ';')
		++j;
	buffer[j] = 0;

	CFiles::wszDatPath = buffer + wFirstChar;
	CFiles::wszResPath = buffer + i + 1;

	return true;
}
#endif // HAS_UNICODE

bool CFiles::ParseDataFileCPath(FILE* pFile)
{
	char buffer[2 * (MAX_PATH + 1)];
	if (!fgets(buffer, 2 * (MAX_PATH + 1), pFile))
		return false;

	UINT i = 0;
	while (i < MAX_PATH && buffer[i] != ';')
		++i;
	if (buffer[i] != ';' || !buffer[0] || !buffer[i + 1] || strlen(buffer + i + 1) > MAX_PATH)
		return false;

	UINT j = i+1;
	buffer[i] = 0;
	while (buffer[j] && buffer[j] != ';')
		++j;
	buffer[j] = 0;

	WSTRING wstrTemp;
	CPathToUnicode(buffer, wstrTemp);
	if (!IsValidPath(wstrTemp.c_str()))
		return false;
	CFiles::wszDatPath = wstrTemp;
	wstrTemp.clear();
	CPathToUnicode(buffer + i + 1, wstrTemp);
	if (!IsValidPath(wstrTemp.c_str()))
		return false;
	CFiles::wszResPath = wstrTemp;

	return true;
}

bool CFiles::InitINI()
{
	bool res = true;
	if (!CFiles::bInitedIni)
	{
		WSTRING wstrIniPath = CFiles::wszDatPath;
		wstrIniPath += wszSlash;
		wstrIniPath += CFiles::wGameName;

		WSTRING wstrTmp;
		AsciiToUnicode(".ini", wstrTmp);
		wstrIniPath += wstrTmp;

		res = gameIni.Load(wstrIniPath.c_str());

		CFiles::bInitedIni = true;
	}
	return res;
}

//******************************************************************************
void CFiles::DeinitClass()
//Frees and zeroes static members of class.
{
	ASSERT(this->dwRefCount==0);

	CFiles::wszAppPath.resize(0);
	CFiles::wszDatPath.resize(0);
	CFiles::wszResPath.resize(0);

	CFiles::wGameName.resize(0);
	CFiles::wGameVer.resize(0);
}

//******************************************************************************
bool CFiles::FindDataPathDotTxt(
//Find DataPath.txt
//
//Params:
	WSTRING& wszPath)      //(out)  Valid path.
{
	WSTRING wstrTmp;

#ifdef WIN32
	//Support multiple separate installations of the same game:
	//If the user has a DataPath.txt file in the install dir, use it.
	wszPath = CFiles::wszAppPath;
	wstrTmp = wszPath + wszSlash + wszDataPathDotTxt;
	if (DoesFileExist(wstrTmp.c_str()))
		return true;
#endif

	if (!wstrHomePath.empty())
	{
		wszPath = GetGameConfPath();

		wstrTmp = wszPath;
		wstrTmp += wszSlash;
		wstrTmp += wszDataPathDotTxt;
		if (DoesFileExist(wstrTmp.c_str()))
		{
#ifndef WIN32
			CFiles::wstrDataPathDotTxtPath = wszPath;
#endif
			return true;
		}
	}

	return false;
}

//******************************************************************************
#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
//******************************************************************************
bool CFiles::FileCopy(
//Copies the source file to the destination file.
//
//Params:
	const WCHAR *source,	//Source filename
	const WCHAR *target,	//Destination filename
	const mode_t mode)		//Destination file mode
//
//Returns true if the copy was successful.
{
	char buffer[IOBUFFERSIZE];
	string src, dst;
	UnicodeToCPath(source, src);
	UnicodeToCPath(target, dst);

	int sf, tf;
	if ((sf = open(src.c_str(), O_RDONLY)) < 0 || (tf = creat(dst.c_str(), mode)) < 0)
	{
		if (sf >= 0) close(sf);
		return false;
	}

	ssize_t l;
	while ((l = read(sf, buffer, IOBUFFERSIZE)))
	{
		if (l == (ssize_t)-1)
		{
			if (errno == EINTR) continue; //read interrupted; try again
			break;
		}
		ssize_t written = 0;
		do {
			ssize_t w = write(tf, buffer + written, l - written);
			if (w == (ssize_t)-1)
			{
				if (errno == EINTR) continue;
				l = w; goto cleanup;  // break 2 loops, cleanup, return false
			}
			written += w;
		} while (written < l);
	}

cleanup:
	close(sf);
	close(tf);
	if (l == (ssize_t)-1)
	{
		unlink(dst.c_str());  // don't leave a half copied file hanging around
		return false;
	}

	return true;
}

//******************************************************************************
bool CFiles::DirectoryCopy (
// Copies files from the source directory to the destination directory.
//
// Params:
	const WSTRING &srcdir,  // Source directory
	const WSTRING &dstdir,  // Destination directory
	const WCHAR *extmask,   // Extension mask (not including "*."), or NULL to copy all files
	bool bOverwrite,        // Set to true if existing files should be overwritten
	bool bUpdate)           // Set both this and bOverwrite to true to copy/keep the most recently modified files
//
// Returns true if successful
{
	WSTRING wstrmask = extmask ? extmask : wszAsterisk;
	vector<WSTRING> files;

	if (!GetFileList(srcdir.c_str(), wstrmask, files, false, true))
		return false;

	WSTRING wstrSrcFile = srcdir, wstrDstFile = dstdir;
	wstrSrcFile += wszSlash;
	wstrDstFile += wszSlash;
	const WSTRING::size_type sdlen = wstrSrcFile.size(), ddlen = wstrDstFile.size();

	for (vector<WSTRING>::const_iterator it = files.begin(); it != files.end(); ++it)
	{
		wstrDstFile.resize(ddlen);
		wstrDstFile += (*it);
		if (bOverwrite || !DoesFileExist(wstrDstFile.c_str()))
		{
			std::string strSrcFile, strDstFile;
			struct stat srcstat, dststat;

			wstrSrcFile.resize(sdlen);
			wstrSrcFile += (*it);

			UnicodeToCPath(wstrSrcFile.c_str(), strSrcFile);
			if (stat(strSrcFile.c_str(), &srcstat))
				return false;
			if (!S_ISREG(srcstat.st_mode))
			{
				ASSERT(!"Skipping dircopy of irregular file");
				continue;
			}

			if (bUpdate)
			{
				UnicodeToCPath(wstrDstFile.c_str(), strDstFile);
				if (!stat(strDstFile.c_str(), &dststat) && dststat.st_mtime >= srcstat.st_mtime)
					continue;
			}
			if (!FileCopy(wstrSrcFile.c_str(), wstrDstFile.c_str(), srcstat.st_mode))
				return false;
		}
	}

	return true;
}

//******************************************************************************
bool CFiles::CopyLocalizationFile(const WCHAR *wszFile)
{
	WSTRING wstrDest = wszFile;
	WSTRING::size_type slashpos = wstrDest.find_last_of(wszSlash[0]);
	if (slashpos != WSTRING::npos)
	{
		if (slashpos + 1 == wstrDest.length())
			return false;
		wstrDest = wstrDest.substr(slashpos + 1);
	}
	wstrDest = CFiles::wszDatPath + wszSlash + wszLocalizationDir + wszSlash + wstrDest;
	return FileCopy(wszFile, wstrDest.c_str(), 0644);
}

//******************************************************************************
bool CFiles::GetLocalizationFiles(vector<WSTRING>& files)
{
	WSTRING wstrAsterisk = wszAsterisk;
	return GetFileList((CFiles::wszDatPath + wszSlash + wszLocalizationDir).c_str(),
		wstrAsterisk, files, true, false);
}

//******************************************************************************
// ifdef __linux__ || defined __FreeBSD__ || defined __APPLE__
#else
//******************************************************************************
bool CFiles::FindPossibleDatPath(
//Finds a directory that could be the player data directory for this app version.
//
//Params:
	const WCHAR *wszStartPath,  //(in)   Directory to begin looking at.  Will
								//    go up levels from this dir.  Expecting
								//    no "\" at end.
	WSTRING& wszPossibleDatPath) //(out)
//
//Returns:
//True if a possible directory was found, false if not.
{
	ASSERT(wszStartPath);

//DEPRECATED:
{
	//For backwards compatibility with existing installations:
	//First, check the app path for the player data files.
	wszPossibleDatPath = wszStartPath;
	int endPathIndex = wszPossibleDatPath.size();

	//Each iteration checks for "<GameName><GameVer>.dat" in one directory plus up in a "data"
	//directory.
	WSTRING wstrGameDatFilepath;
	while (endPathIndex != 0)
	{
		wstrGameDatFilepath = wszPossibleDatPath;   // "x:/somepath"
		wstrGameDatFilepath += wszSlash;            // "x:/somepath/"
		wstrGameDatFilepath += wszData;             // "x:/somepath/data"
		wstrGameDatFilepath += wszSlash;            // "x:/somepath/data/"
		wstrGameDatFilepath += wszOldDatDirResourceFile;
		if (DoesFileExist(wstrGameDatFilepath.c_str()))
		{
			wszPossibleDatPath += wszSlash;
			wszPossibleDatPath += wszData;
			return true;
		}

		//Change file path so that it is specifying parent dir.
		while (endPathIndex != 0 && wszPossibleDatPath[endPathIndex] != We(SLASH))
			--endPathIndex;
		wszPossibleDatPath.resize(endPathIndex);
	}
}

	//For newer installations:
	//Always just use the game path for where player data resources should go.
	if (!CFiles::wstrHomePath.empty()) {
		wszPossibleDatPath = GetGameConfPath();
		return true;
	}

	//Ran out of parent directories to check.
	return false;
}

//******************************************************************************
bool CFiles::FindPossibleResPath(
//Finds a directory that could be a resource directory because it contains
//"Bitmaps", "Fonts", "Help", "Music" and "Sounds".
//
//Params:
	const WCHAR *wszStartPath,  //(in)   Directory to begin looking at.  Will
								//    go up levels from this dir.  Expecting
								//    no slash at end.
	WSTRING& wszPossibleResPath) //(out)  Prealloced to MAX_PATH + 1.
//
//Returns:
//True if a possible directory was found, false if not.
{
	ASSERT(wszStartPath);

	wszPossibleResPath = wszStartPath;
	int endPathIndex = wszPossibleResPath.size();

	//Each iteration checks for the resource dirs in the dir and parent dir
	WSTRING wstrGameRespath;
	ASSERT(!CFiles::wUniqueResFile.empty());
	while (endPathIndex != 0)
	{
		wstrGameRespath = wszPossibleResPath;   // "x:/somepath"
		wstrGameRespath += wszSlash;            // "x:/somepath/"
		wstrGameRespath += CFiles::wUniqueResFile;    // "x:/somepath/<res>"
        if (DoesFileExist(wstrGameRespath.c_str()))
			return true;

		wstrGameRespath = wszPossibleResPath;   // "x:/somepath"
		wstrGameRespath += wszSlash;            // "x:/somepath/"
		wstrGameRespath += wszData;             // "x:/somepath/Data"
		wstrGameRespath += wszSlash;            // "x:/somepath/Data/"
		wstrGameRespath += CFiles::wUniqueResFile;    // "x:/somepath/Data/<res>"
		if (DoesFileExist(wstrGameRespath.c_str()))
		{
			// Add 'Data' to the end of the path
			wszPossibleResPath += wszSlash;
			wszPossibleResPath += wszData;
			return true;
		}

		//Change file path so that it is specifying parent dir.
		while (endPathIndex != 0 && wszPossibleResPath[endPathIndex] != We(SLASH))
			--endPathIndex;
		wszPossibleResPath.resize(endPathIndex);
	}

	//Ran out of parent directories to check.
	return false;
}

//******************************************************************************
#endif // ifdef __linux__, else

//******************************************************************************
bool CFiles::WriteDataPathTxt(
//Write a DataPath.txt file.
//
//Params:
	const WCHAR *pwszFilepath, //(in)   Complete filepath to write to.
	const WCHAR *pwszDatPath,  //(in)   Data path to write inside file.
	const WCHAR *pwszResPath)  //(in)   Resource path to write inside file.
//
//Returns:
//True if successful, false if not.
{
	ASSERT(pwszFilepath);
	ASSERT(pwszDatPath);
	ASSERT(pwszResPath);

	FILE* pFile = Open(pwszFilepath, "w");
	if (NULL == pFile) {
#if defined __linux__ || defined __FreeBSD__ || defined __APPLE__
		//Try making the file writable if it exists and is read-only
		string strFilepath;
		UnicodeToCPath(pwszFilepath, strFilepath);
		if (chmod(strFilepath.c_str(), 0644) ||
				(pFile = Open(pwszFilepath, "w")) == NULL)
			return false;
#else
		return false;
#endif
	}

	WSTRING tmp;
#ifdef HAS_UNICODE
	tmp.append(1, UNICODE_BOM);
#endif
	tmp += pwszDatPath;
	tmp += wszSemicolon;
	tmp += pwszResPath;
	tmp += wszSemicolon;
#ifdef HAS_UNICODE
	bool bSuccess = (tmp.length() == fwrite( tmp.c_str(), sizeof(WCHAR), tmp.length(), pFile ));
#else
	string strBuf;
	UnicodeToCPath(tmp, strBuf);
	bool bSuccess = (strBuf.length() == fwrite( strBuf.c_str(), sizeof(char), strBuf.length(), pFile ));
#endif // HAS_UNICODE

	fclose(pFile);

	return bSuccess;
}

//*****************************************************************************
bool CFiles::WindowsCanBrowseUnicode()
// Returns true if the Windows OS has _wfind* functions (i.e. it's NT or better)
{
	 //It might be preferable to make a call to wstat(apppath) instead and see if it works.

#ifdef WIN32
	 OSVERSIONINFO versionInfo;
	 versionInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
	if (!::GetVersionEx (&versionInfo)) {
		// Can't get version - let's assume we can't browse files in unicode
		return false;
	}

	return (
				//At time of writing (8/4/03) this includes: Windows NT 3.51, Windows NT 4.0,
				//Windows 2000, Windows XP, or Windows .NET Server.
				versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ||

				//Any future O/Ss from MS should probably support unicode too.
				versionInfo.dwMajorVersion >= 5);

#else //This isn't even Windows!
	ASSERTP(false, "Bad call to WindowsCanBrowseUnicode().");
	return false;
#endif
}

//*****************************************************************************
bool CFiles::IsValidPath(const WCHAR *pwzPath)
//Returns true if path is valid, false if not.
{
	if (!(pwzPath && pWCv(pwzPath))) return false;
	WSTRING wstrDirPath = pwzPath;

	//Get rid of the trailing slash (stat doesn't like it) UNLESS there is only one slash - i.e. e:\ .
	if (wstrDirPath.size() > 0) {
		int count = 0;
		for (WSTRING::iterator i = wstrDirPath.begin(); i != wstrDirPath.end(); ++i) {
			if (*i == wszSlash[0]) count++;
		}
		if (count > 1) {
			if (wstrDirPath[wstrDirPath.size()-1] == wszSlash[0]) {
				wstrDirPath.erase(wstrDirPath.size()-1);
			}
		}
	}

#ifdef WIN32
	//Check for read permission.
	if (CFiles::WindowsCanBrowseUnicode()) {
		return (_waccess(wstrDirPath.c_str(), 4) == 0);
	}

	const string aDirPath = UnicodeToAscii(wstrDirPath);
	const bool bValidPath = (_access(aDirPath.c_str(), 4) == 0);
	return bValidPath;
#else
	std::string strDirPath;
	struct stat buf;
	UnicodeToCPath(wstrDirPath, strDirPath);
	return !access(strDirPath.c_str(), R_OK | X_OK)
			&& (!stat(strDirPath.c_str(), &buf) ? S_ISDIR(buf.st_mode) : false);
#endif
}

//*****************************************************************************
bool CFiles::GetDirectoryList(
//Gets a list of all directories in the given directory.
//
//Params:
	const WCHAR *wszFilepath,  //(in) Directory to look in
	vector<WSTRING>& dirs,     //(out) vector of all subdirectories in wszFilepath
	bool bShowHidden)          //(in)  if true, show hidden directories
//
//Returns:
//True if the directory was found, false if not.
{
	dirs.clear();
	set<WSTRING, WSTRINGicmp> wstrDirs;
#ifdef WIN32
	// If not on Windows NT or better, we need to use non-unicode paths here...
	if (!CFiles::WindowsCanBrowseUnicode()) {
		//Show subdirs.
		WSTRING wbuffer;
		struct _finddata_t filedata;
		long hFile;

		//Find first sub-dir in current directory.
		string sFileFilter = UnicodeToAscii(wszFilepath);
		sFileFilter += "\\*";

		if ((hFile = _findfirst(sFileFilter.c_str(), &filedata )) == -1L) {
			 return (errno == ENOENT); //No files in current directory - fail only if it's not ENOENT
		}
		else
		{
			//Don't display the current directory (i.e., ".").
			if (filedata.attrib & _A_SUBDIR && strcmp(".", filedata.name)) {
				AsciiToUnicode(filedata.name, wbuffer);
				wstrDirs.insert(wbuffer);
			}

			//Find the rest of the sub-dirs.
			while (_findnext( hFile, &filedata ) == 0)
			{
				if (filedata.attrib & _A_SUBDIR) {
					AsciiToUnicode(filedata.name, wbuffer);
					wstrDirs.insert(wbuffer);
			  }
			}

			//Done.
			_findclose( hFile );
		}
	} else {
		// We have access to Unicode file browsing

		struct _wfinddata_t filedata;
		long hFile;

		//Find first sub-dir in current directory.
		WSTRING wFileFilter = wszFilepath;
		wFileFilter += wszSlash;
		wFileFilter += wszAsterisk;
		if ((hFile = _wfindfirst((WCHAR*)wFileFilter.c_str(), &filedata )) == -1L)
			 return (errno == ENOENT); //No files in current directory - fail only if it's not ENOENT

		//Don't display the current directory (i.e., ".").
		if (filedata.attrib & _A_SUBDIR && WCScmp(wszPeriod, filedata.name))
			wstrDirs.insert(filedata.name);

		//Find the rest of the sub-dirs.
		while (_wfindnext( hFile, &filedata ) == 0)
		{
			if (filedata.attrib & _A_SUBDIR)
				wstrDirs.insert(filedata.name);
		}

		//Done.
		_findclose( hFile );
	}
#else
#  ifdef HAS_UNICODE
#     error How does a non-Win32 machine get a Unicode list of directories in a directory?
#  else
	string sbuffer;
	DIR *pDir;
	UnicodeToCPath(wszFilepath, sbuffer);
	pDir = opendir(sbuffer.c_str());
	sbuffer += SLASH;
	UINT len = sbuffer.length();
	if (pDir)
	{
		dirent *pDirent;
		while ((pDirent = readdir(pDir)) != NULL)
		{
			sbuffer.resize(len);
			sbuffer += pDirent->d_name;
			struct stat st;
			if ((pDirent->d_name[0] != '.' || (bShowHidden ? pDirent->d_name[1] != 0
						: pDirent->d_name[1] == '.' && pDirent->d_name[2] == 0))
				&& !stat(sbuffer.c_str(), &st) && S_ISDIR(st.st_mode)
				&& !access(sbuffer.c_str(), R_OK | X_OK))
			{
				// The directory entry is a directory.
				WSTRING wbuffer;
				CPathToUnicode(pDirent->d_name, wbuffer);
				wstrDirs.insert(wbuffer);
			}
		}
	}

#  endif

#endif
	for (set<WSTRING, WSTRINGicmp>::iterator wstr = wstrDirs.begin(); wstr != wstrDirs.end(); ++wstr)
		dirs.push_back(*wstr);

	return true;
}
//*****************************************************************************
bool CFiles::GetFileList(
// Gets a list of all files that match the mask in the given directory.
//
//Params:
	const WCHAR *wszFilepath,     //(in) the directory to look in
	const WSTRING& mask,          //(in) the file extension to look for (not including the "*.")
	vector<WSTRING>& files,       //(out) the files that matched the mask in the given directory
	bool bFullPath,               //(in) if true, outputs whole path to the file, if false just the filename
	bool bShowHidden)             //(in) if true, shows hidden files
//
//Returns:
//True if the directory was found, false if not.
{
	files.clear();
	set<WSTRING, WSTRINGicmp> wstrFiles;

#ifdef WIN32
	if (!CFiles::WindowsCanBrowseUnicode()) {
		//Get the files in the current directory - ASCII version
		WSTRING wbuffer;
		struct _finddata_t filedata;
		long hFile;

		//Find first sub-dir in current directory.
		string sFileFilter = UnicodeToAscii(wszFilepath);
		sFileFilter += "\\*.";
		sFileFilter += UnicodeToAscii(mask);
		if ((hFile = _findfirst(sFileFilter.c_str(), &filedata)) == -1L) {
			 return (errno == ENOENT); //No files in current directory - fail only if it's not ENOENT
		}
		else
		{
			if (!(filedata.attrib & _A_SUBDIR || filedata.attrib & _A_SYSTEM ||
				filedata.attrib & _A_HIDDEN)) {
				AsciiToUnicode(filedata.name, wbuffer);
				wstrFiles.insert(wbuffer);
			}

			//Find the rest of the files.
			while (_findnext(hFile, &filedata) == 0)
			{
				if (!(filedata.attrib & _A_SUBDIR || filedata.attrib & _A_SYSTEM ||
					filedata.attrib & _A_HIDDEN)) {
					AsciiToUnicode(filedata.name, wbuffer);
					wstrFiles.insert(wbuffer);
				}
			}

			//Done.
			_findclose( hFile );
		}
	}
	else {
		//Get the files in the current directory - Unicode version
		struct _wfinddata_t filedata;
		long hFile;

		//Find first file in current directory.
		WSTRING wFileFilter = wszFilepath;
		wFileFilter += wszSlash;
		wFileFilter += wszAsterisk;
		wFileFilter += wszPeriod;
		wFileFilter += mask;
		if ((hFile = _wfindfirst((WCHAR*)wFileFilter.c_str(), &filedata)) == -1L)
			 return (errno == ENOENT); //No files in current directory - fail only if it's not ENOENT
		else
		{
			if (!(filedata.attrib & _A_SUBDIR || filedata.attrib & _A_SYSTEM ||
					filedata.attrib & _A_HIDDEN))
				wstrFiles.insert(filedata.name);

			//Find the rest of the files.
			while (_wfindnext(hFile, &filedata) == 0)
			{
				if (!(filedata.attrib & _A_SUBDIR || filedata.attrib & _A_SYSTEM ||
						filedata.attrib & _A_HIDDEN))
					wstrFiles.insert(filedata.name);
			}

			//Done.
			_findclose( hFile );
		}
	}
#else
#  ifdef HAS_UNICODE
#     error How does a non-Win32 machine get a Unicode list of files in a directory?
#  else
	std::string sbuffer;
	DIR *pDir;
	const bool bAllExts = (mask.length() == 1 && WCv(mask[0]) == '*');
	UnicodeToCPath(wszFilepath, sbuffer);
	pDir = opendir(sbuffer.c_str());
	sbuffer += SLASH;
	UINT len = sbuffer.length();
	if (pDir)
	{
		std::string strExt;
		UnicodeToCPath(mask, strExt);

		dirent *pDirent;
		while ((pDirent = readdir(pDir)) != NULL)
		{
			if (!bShowHidden && pDirent->d_name[0] == '.') continue;  //skip hidden

			if (!bAllExts)
			{
				char *szCurExt = pDirent->d_name + strlen(pDirent->d_name) - strExt.length();
				if (szCurExt < pDirent->d_name || (strcmp(strExt.c_str(), szCurExt)))
					continue;
			}

			sbuffer.resize(len);
			sbuffer += pDirent->d_name;
			struct stat st;
			if (!stat(sbuffer.c_str(), &st) && !S_ISDIR(st.st_mode))
			{
				// The entry is a file.
				WSTRING wbuffer;
				CPathToUnicode(pDirent->d_name, wbuffer);
				wstrFiles.insert(wbuffer);
			}
		}

		closedir(pDir);
	}
#  endif
#endif

	for (set<WSTRING, WSTRINGicmp>::iterator wstr = wstrFiles.begin(); wstr != wstrFiles.end(); ++wstr)
	{
		if (bFullPath) {
			WSTRING wstrNew = wszFilepath;
			wstrNew += wszSlash;
			wstrNew += *wstr;
			files.push_back(wstrNew);
		}
		else
			files.push_back(*wstr);
	}

	return true;
}

//*****************************************************************************
bool CFiles::GetDriveList(
//Gets a list of all valid drives (Win32) or some useful directories (Linux)
//
//Params:
	vector<WSTRING>& drives)      //(out) vector of strings representing all valid drives
//
//Returns:
//True if it makes sense to call this on this system, false otherwise
{
#ifdef WIN32
	drives.clear();

	if (!CFiles::bDrivesChecked)
	{
		for (char drive=0; drive<NUMDRIVES; ++drive)
		{
			char buffer[4] = { drive+'a', ':', '\\', 0};
			UINT uType = GetDriveTypeA(buffer); //Use instead of GetDriveType() for pre-NT compatibility.
			bDrives[drive] = ((uType != DRIVE_UNKNOWN) && (uType != DRIVE_NO_ROOT_DIR));
		}
		bDrivesChecked = true;
	}

	for (UINT drive=0; drive<NUMDRIVES; ++drive)
	{
		if (CFiles::bDrives[drive])
		{
			static const WCHAR wszLeft[] = {'[',' ',0}, wszRight[] = {':',' ',']',0};
			WCHAR wszDrive[] = {'A',0};
			wszDrive[0] = (WCHAR)(drive + 'A');
			WSTRING wstrDrive = wszLeft;
			wstrDrive += wszDrive;
			wstrDrive += wszRight;
			drives.push_back(wstrDrive);
		}
	}

	return true;
#elif defined (__linux__) || defined (__FreeBSD__)
	//Add the game dat directory and the home directory first.
	drives.clear();
	static const WCHAR wszLeft[] = {We('['),We(' '),We(0)}, wszRight[] = {We(' '),We(']'),We(0)};
	WSTRING wstr = wszLeft;
	wstr += CFiles::wszDatPath;
	wstr += wszRight;
	drives.push_back(wstr);
	wstr = wszLeft;
	wstr += wstrHomePath;
	wstr += wszRight;
	drives.push_back(wstr);

	//Add some mount points
	FILE *fp = fopen("/etc/mtab", "r");
	if (fp) {
		char buf[4096];
		while (fscanf(fp, "%4095s %4094s %*s %*s %*i %*i\n", buf, buf + 1) == 2)
		{
			if (*buf == SLASH) {
				WSTRING wb;
				CPathToUnicode(buf + 1, wb);
				wstr = wszLeft;
				wstr += wb;
				wstr += wszRight;
				drives.push_back(wstr);
			}
		}
		fclose(fp);
	}

	return true;
#elif defined __APPLE__
	//Add the DROD dat directory and the home directory first.
	drives.clear();
	static const WCHAR wszLeft[] = {We('['),We(' '),We(0)}, wszRight[] = {We(' '),We(']'),We(0)};
	WSTRING wstr = wszLeft;
	wstr += CFiles::wszDatPath;
	wstr += wszRight;
	drives.push_back(wstr);
	wstr = wszLeft;
	wstr += wstrHomePath;
	wstr += wszRight;
	drives.push_back(wstr);

	// Add mount points
	char buffer[MAX_PATH];
	DIR *pDir;
	strcpy(buffer, "/Volumes");
	pDir = opendir(buffer);
	UINT len = strlen(buffer);
	buffer[len++] = SLASH;
	if (pDir)
	{
		dirent *pDirent;
		while ((pDirent = readdir(pDir)) != NULL)
		{
			strcpy(&buffer[len], pDirent->d_name);
			struct stat st;
			if (pDirent->d_name[0] != '.'
				&& !stat(buffer, &st) && S_ISDIR(st.st_mode)
				&& !access(buffer, R_OK | X_OK))
			{
				// The directory entry is a directory.
				WSTRING wbuffer;
				CPathToUnicode(buffer, wbuffer);

				wstr = wszLeft;
				wstr += wbuffer;
				wstr += wszRight;
				drives.push_back(wstr);
			}
		}
	}

	return true;
#else
	return false;
#endif
}

//**************************************************************************************
#ifdef USE_LOGCONTEXT
void CFiles::PushLogContext(const char *pszDesc)
{
	 static const UINT MAXLEN_CONTEXT = 20000;
	 static const UINT CHOPLEN = 3000;
	 for (UINT wIndentNo = 0; wIndentNo < m_wIndentLevel; ++wIndentNo)
		  m_strUnwrittenContext += "  ";
	 m_strUnwrittenContext += "BEGIN ";
	 m_strUnwrittenContext += pszDesc;
	 m_strUnwrittenContext += NEWLINE;
	 ++m_wIndentLevel;

	 if (m_strUnwrittenContext.size() > MAXLEN_CONTEXT)
	 {
		  const char *pszSeek = m_strUnwrittenContext.c_str() + CHOPLEN;
		  while (*pszSeek && *pszSeek != '\n') ++pszSeek;
		  string strTemp = pszSeek + 1;
		  m_strUnwrittenContext = "(Beginning of log context truncated to free memory.)" NEWLINE;
		  m_strUnwrittenContext += strTemp;
	 }
}

//**************************************************************************************
void CFiles::PopLogContext(const char *pszDesc)
{
	 --m_wIndentLevel;
	 for (UINT wIndentNo = 0; wIndentNo < m_wIndentLevel; ++wIndentNo)
		  m_strUnwrittenContext += "  ";
	 m_strUnwrittenContext += "END ";
	 m_strUnwrittenContext += pszDesc;
	 m_strUnwrittenContext += NEWLINE;
}
#endif // #ifdef USE_LOGCONTEXT

//**************************************************************************************

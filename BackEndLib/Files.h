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

//Files.h
//Declarations for CFiles.
//Class for handling file-related tasks.

#ifndef FILES_H
#define FILES_H

#if defined WIN32
#  define HAS_UNICODE
#elif defined __sgi
#  undef HAS_UNICODE
#elif defined __APPLE__
#  undef HAS_UNICODE
#elif defined __linux__ || defined __FreeBSD__
#  undef HAS_UNICODE
#else
#error Platform unknown!  Does this platform support 16-bit Unicode?
#endif

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
#define USE_UTF8_PATHS
#else
#undef USE_UTF8_PATHS
#endif

#if defined(__linux__) || defined(__FreeBSD__)
#define USE_XDG_BASEDIR_SPEC
#else
#undef USE_XDG_BASEDIR_SPEC
#endif

#ifdef WIN32
#  define SLASH '\\'
#  pragma warning(disable:4786)
#else
#  define SLASH '/'
#endif

#include "Assert.h"
#include "StretchyBuffer.h"
#include "IniFile.h"

#include <string>
#include <vector>
using std::string;
using std::vector;

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
#include <sys/types.h>
#endif

#ifndef MAX_PATH
#  ifdef PATH_MAX
#     define MAX_PATH PATH_MAX
#  else
#     define MAX_PATH 260
#  endif
#endif

extern const WCHAR wszDefExportDir[];

class CFiles
{
public:
	//call this constructor first to init the class
	CFiles(const WCHAR *wszSetAppPath,
				const WCHAR *wszSetGameName, const WCHAR *wszSetGameVer,
				const bool bIsDemo = false, const bool confirm_resource_file = true);
	CFiles(); //may call this one after the class has been inited
	~CFiles();

	static bool          CreatePathIfInvalid (const WCHAR *wszPath);
	static bool          MakeDirectory(const WCHAR *pwzPath);

	static bool          DoesFileExist(const WCHAR *pszFilepath);
	static bool          DoesFileExist(const char *pszFilepath);
	static bool          EraseFile(const WCHAR *pszFilepath);
	static bool          EraseFile(const char *pszFilepath);
	static bool          HasReadWriteAccess(const WCHAR *pwzFilepath);
	static void          InitAppVars(const WCHAR* wszUniqueResFile, const vector<string>& datFiles, const vector<string>& playerDataSubDirs=vector<string>());
	static bool          IsValidPath(const WCHAR *pwzPath);
	static bool          MakeFileWritable(const WCHAR *pwzFilepath);
	static FILE *        Open(const WCHAR *pszFilepath, const char *pszOptions);
#ifdef USE_UTF8_PATHS
	static void          CPathToUnicode(const char *pszFilepath, WSTRING &wstr) { CTextToUnicode(pszFilepath, wstr); }
#else
	static void          CPathToUnicode(const char *pszFilepath, WSTRING &wstr) { AsciiToUnicode(pszFilepath, wstr); }
#endif
	static void          CPathToUnicode(const std::string &strFilepath, WSTRING &wstr) { CPathToUnicode(strFilepath.c_str(), wstr); }
	static WSTRING       CPathToUnicode(const char* pszFilepath) { WSTRING result; CPathToUnicode(pszFilepath, result); return result; }
	static WSTRING       CPathToUnicode(const std::string &strFilepath) { WSTRING result; CPathToUnicode(strFilepath, result); return result; }
	static bool          ReadFileIntoBuffer(const WCHAR *pwzFilepath, CStretchyBuffer &Buffer, bool bBinary = false);
	static bool          RenameFile(const WCHAR *wszOldFilepath, const WCHAR *wszNewFilepath);
	static bool          RenameFile(const char *szOldFilepath, const char *szNewFilepath);
#ifdef USE_UTF8_PATHS
	static bool          UnicodeToCPath(const WCHAR *pwszFilepath, std::string &str) { UnicodeToUTF8(pwszFilepath, str); return true; }
#else
	static bool          UnicodeToCPath(const WCHAR *pwszFilepath, std::string &str) { return UnicodeToAscii(pwszFilepath, str); }
#endif
	static bool          UnicodeToCPath(const WSTRING &wstrFilepath, std::string &str) { return UnicodeToCPath(wstrFilepath.c_str(), str); }
	static std::string   UnicodeToCPath(const WCHAR *pwszFilepath) { std::string result; UnicodeToCPath(pwszFilepath, result); return result; }
	static std::string   UnicodeToCPath(const WSTRING& wstrFilepath) { std::string result; UnicodeToCPath(wstrFilepath, result); return result; }
	static bool          WriteBufferToFile(const WCHAR *pwzFilepath, const CStretchyBuffer &Buffer, const bool bAppend=false);
	static bool          WriteBufferToFile(const char *pszFilepath, const CStretchyBuffer &Buffer, const bool bAppend=false);

	//These methods require class construction.
	void                 AppendErrorLog(const char *pszText);
	void                 AppendLog(const WSTRING& wstrDatPathTxt, const char *pszText);
	void                 AppendUserLog(const char *pszText);
	bool                 DeleteINIEntry(const char *pszSection, const char *pszKey, const char *pszValue);
	static const WSTRING GetAppPath() {return CFiles::wszAppPath;}
	static const WSTRING GetDatPath() {return CFiles::wszDatPath;}
	static const WSTRING GetResPath() {return CFiles::wszResPath;}
	static bool          GetGameProfileString(const char *pszSection, const char* pszKey, string& strValue);
	static bool          GetGameProfileString(const char *pszSection, const char* pszKey, list<string>& strValue);
	static bool          GetGameProfileString(const char *pszSection, const char* pszKey, list<WSTRING>& strValue);
	static bool          GetGameProfileString(const char *pszSection, const WCHAR* pszKey, list<WSTRING>& strValue);
	void                 TryToFindDataPath();
	bool                 WriteGameProfileString(const char *pszSection, const char* pszKey, const char* pszValue);
	bool                 WriteGameProfileString(const char *pszSection, const WCHAR* pwszKey, const char* pszValue);
	bool                 WriteGameProfileString(const char *pszSection, const WCHAR* pwszKey, const list<WSTRING>& wstrValue);
	bool                 WriteGameProfileBuffer(const WSTRING& wstr, const bool bOverwrite=false, const bool bAddDuplicates=true);

#ifdef USE_LOGCONTEXT
	//Used to set context that is described in error log.  Use LOGCONTEXT() macro from
	//instead of calling directly.
	static void          PushLogContext(const char *pszDesc);
	static void          PopLogContext(const char *pszDesc);
#endif

	//File encryption/unencryption.
	static bool          FileIsEncrypted(const WCHAR *wszFilepath) {
		return wszFilepath[WCSlen(wszFilepath) - 1] == '_';}
	static bool          ProtectFile(const WCHAR *pszFilepath);
	//symmetric operations
	static bool          UnprotectFile(const WCHAR *pszFilepath) {return ProtectFile(pszFilepath);}
	static void          MutateFileName(WCHAR *pszFilepath);
	static bool          WindowsCanBrowseUnicode();

	static bool          GetDirectoryList(const WCHAR *wszFilepath, vector<WSTRING>& dirs,
										 bool bShowHidden = false);
	static bool          GetFileList(const WCHAR *wszFilepath, const WSTRING& mask,
										 vector<WSTRING>& files, bool bFullPath = false, bool bShowHidden = false);
	static bool          GetDriveList(vector<WSTRING>& drives);
	static const WCHAR * GetHomePath() {return wstrHomePath.c_str();}
	static const WSTRING GetGameConfPath();
#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
	static bool          FileCopy(const WCHAR *src, const WCHAR *dst, mode_t mode);
	static bool          DirectoryCopy(const WSTRING& srcdir, const WSTRING& dstdir, const WCHAR *extmask = 0, bool bOverwrite = false, bool bUpdate = false);

	static bool CopyLocalizationFile(const WCHAR *wszFile);
	static bool GetLocalizationFiles(vector<WSTRING>& files);
#endif

	static WSTRING wCompanyName, wGameName, wGameVer, wUniqueResFile;
	static vector<string> datFiles, playerDataSubDirs;

	static bool bad_data_path_file;

#ifdef WIN32
	static WSTRING GetUserspacePath(bool bUserSpecificDir);

	static bool bWindowsDataFilesInUserSpecificDir;
#endif

private:
	void                 DeinitClass();
	void                 GetDatPathFromDataPathDotTxt();

	bool                 ParseDataFileUnicode(FILE* pFile);
	bool                 ParseDataFileCPath(FILE* pFile);

	static bool          FindDataPathDotTxt(WSTRING& wszPath);
#if defined(WIN32)
	static bool          FindPossibleDatPath(const WCHAR *wszStartPath, WSTRING& wszPossibleDatPath);
	static bool          FindPossibleResPath(const WCHAR *wszStartPath, WSTRING& wszPossibleResPath);
#endif
	void                 InitClass(const WCHAR *pszSetAppPath,
			const WCHAR *wszSetGameName, const WCHAR *wszSetGameVer, const bool bIsDemo,
			const bool confirm_resource_file);
	static bool          InitINI();
	static void          SetupHomePath();
	static void          SetupHomePathSubDirs();
	static bool          WriteDataPathTxt(const WCHAR *pszFilepath,
			const WCHAR *wszDatPath, const WCHAR *wszResPath);

#ifdef USE_XDG_BASEDIR_SPEC
	static WSTRING wstrHomeConfPath;
#else
	static const WCHAR *const wszHomeConfDir;
#endif
	static const WCHAR *const wszDemo;
	static WSTRING wstrHomePath;

	static WSTRING wszAppPath; //where the executable is located
	static WSTRING wszDatPath; //where user data is located
	static WSTRING wszResPath; //where the game's installed data is located
#ifndef WIN32
	static WSTRING wstrDataPathDotTxtPath; //path of directory containing DataPath.txt
#endif
	static UINT dwRefCount;
	static CIniFile gameIni;
	static bool bInitedIni;
	static bool bIsDemo;

#ifdef WIN32
#define NUMDRIVES (26)
	static bool bDrives[NUMDRIVES];
	static bool bDrivesChecked;
#endif

	PREVENT_DEFAULT_COPY(CFiles);
};

#endif //...#ifndef FILES_H

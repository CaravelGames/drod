// $Id: DbBase.h 9280 2008-10-29 01:57:24Z mrimer $

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
 *
 * ***** END LICENSE BLOCK ***** */

//DbBase.h
//Declarations for CDbBase.
//Class for handling common database functionality.

#ifndef DBBASE_H
#define DBBASE_H
#ifdef WIN32
#  pragma warning(disable:4786)
#endif

#include "DbProps.h"
#include "ImportInfo.h"
#include <BackEndLib/MessageIDs.h>
#include <BackEndLib/Assert.h>
#include <BackEndLib/CoordSet.h>
#include <BackEndLib/StretchyBuffer.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>

#include <mk4.h>

#define ROW_NO_MATCH ((UINT)-1)

extern void GetWString(WSTRING& wstr, const c4_Bytes& Bytes);
extern c4_Bytes PutWString(const WSTRING& wstr);

class CDbBase
{
friend class CDb;
public:
	CDbBase();
	virtual  ~CDbBase();

	MESSAGE_ID          AddMessageText(const WCHAR *pwczText);
	MESSAGE_ID          AddMessageText(const UINT eMessageID, const WCHAR *pwczText);
	MESSAGE_ID          ChangeMessageText(const MESSAGE_ID eMessageID, const WCHAR *pwczText);
	void                Close(const bool bCommit=true);
	static bool         CreateDatabase(const WSTRING& wstrFilepath, int initIncrementedIDs=-1);
	static void         DeleteMessage(const MESSAGE_ID eMessageID);
	static void         DeleteMarkedMessages();

	static void         DirtyData() {CDbBase::bDirtyData = true;}
	static void         DirtyHold() {CDbBase::bDirtyHold = true;}
	static void         DirtyPlayer() {CDbBase::bDirtyPlayer = true;}
	static void         DirtySave() {CDbBase::bDirtySave = true;}
	static void         DirtyText() {CDbBase::bDirtyText = true;}

	//Localization API.
	static void         ExportTexts(const WCHAR *pFilename);
	static void         ExportTexts(c4_View& MessageTextsView, CStretchyBuffer& buf);
	bool                ImportTexts(const WCHAR *pFilename);
	bool                ImportTexts(c4_View& MessageTextsView, CStretchyBuffer& buf);

	WCHAR*              GetAllocMessageText(const MESSAGE_ID eMessageID, UINT *pdwLen = NULL) const;
	static UINT         GetIncrementedID(const c4_IntProp &propID);
	const WCHAR*        GetMessageText(const MESSAGE_ID eMessageID, UINT *pdwLen = NULL);
	const WCHAR*        GetMessageText(const c4_Bytes& MessageTextBytes, UINT* pdwLen = NULL);
	virtual UINT        GetPrimaryKey() const {return 0;}
	static c4_IntProp*  GetPrimaryKeyProp(const VIEWTYPE eViewType);
	static c4_RowRef    GetRowRef(const VIEWTYPE vType, UINT dwGlobalIndex);
	static c4_ViewRef   GetActiveView(const VIEWTYPE vType);
	static c4_ViewRef   GetView(const VIEWTYPE vType, const UINT dwID);
	static UINT         GetViewSize(const VIEWTYPE vType);
	static bool         IsDirty();
	static bool         IsOpen();
	MESSAGE_ID          Open(const WCHAR *pwszDatFilepath = NULL);
	virtual MESSAGE_ID  SetProperty(const PROPTYPE pType, const char** atts,
			CImportInfo &info);
	virtual MESSAGE_ID  SetProperty(const PROPTYPE pType, char* const str,
			CImportInfo &info, bool &bSaveRecord);
	virtual MESSAGE_ID  SetProperty(const VIEWPROPTYPE vpType, const PROPTYPE pType,
			char* const str, CImportInfo &info);
	virtual bool        Update()=0;

	//For dev building of various DLC packs (static DB files)
	static bool SetCreateDataFileNum(UINT num);

protected:
	static bool bDirtyData, bDirtyHold, bDirtyPlayer, bDirtySave, bDirtyText; //one for each data file
	bool        bPartialLoad;  //set on quick record load

	UINT               FindMessageText(const MESSAGE_ID dwMessageID,
			const bool bReturnOtherLanguages=true) const;
	CCoordSet           GetMessageTextIDs(const MESSAGE_ID dwMessageID) const;

	static UINT         LookupRowByPrimaryKey(const UINT dwID, const VIEWTYPE vType, c4_View &View);

	//Accelerated lookup index generation.
	virtual void resetIndex();
	virtual void buildIndex();

	static const UINT START_LOCAL_ID;

private:
	static WSTRING BaseResourceFilename();

	virtual void  Commit();
	virtual void  Rollback();
	void          Undirty();
	void          ResetStorage();

	static void   addMessage(const UINT messageID, const UINT messageRow);
	static void   deleteMessages(const CIDSet& messageIDs, const CIDSet& rowIDs);
	static CIDSet getMessageRows(const UINT messageID);

	static c4_ViewRef GetPlayerDataView(const VIEWTYPE vType, const char* viewName);

	static UINT         LookupRowByPrimaryKey(const UINT dwID,
			const VIEWTYPE vType, const c4_IntProp *pPropID, const UINT rowCount, c4_View &View);

	static UINT   globalRowToLocalRow(UINT globalRowIndex, const VIEWTYPE vType);
	WCHAR*        SetLastMessageText(const WCHAR *pwczNewMessageText, const UINT dwNewMessageTextLen);
	WCHAR*  pwczLastMessageText;

	static void   RedefineDatabase(c4_Storage* storage);

	bool CreateContentFile(const WSTRING& wFilename, UINT num);
	void OpenStaticContentFiles(const WSTRING& wstrResPath);

	//For dev building of various DLC packs (static DB files)
	static UINT GetStartIDForDLC(UINT fileNum);
	static UINT creatingStaticDataFileNum;

	PREVENT_DEFAULT_COPY(CDbBase);
};

UINT GetDbRefCount();

//Debugging macros to check for a reference count change.  Put BEGIN_DBREFCOUNT_CHECK in
//front of code that creates CDbBase-derived objects, and END_DBREFCOUNT_CHECK at the end
//of the code.  They both need to be in the same scope.
#ifdef _DEBUG
	#define BEGIN_DBREFCOUNT_CHECK      UINT dwStartingDbRefCount = GetDbRefCount()
	#define END_DBREFCOUNT_CHECK     ASSERT(GetDbRefCount() == dwStartingDbRefCount)
#else
	#ifdef WIN32
		#define BEGIN_DBREFCOUNT_CHECK      NULL
		#define END_DBREFCOUNT_CHECK     NULL
	#else
		#define BEGIN_DBREFCOUNT_CHECK
		#define END_DBREFCOUNT_CHECK
	#endif
#endif

#endif //...#ifndef DBBASE_H


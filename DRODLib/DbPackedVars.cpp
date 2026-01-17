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
 * 1997, 2000, 2001, 2002, 2004, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Michael Welsh Duggan (md5i), Mike Rimer (mrimer), Gerry Jo Jellestad
 *
 * ***** END LICENSE BLOCK ***** */

//DbPackedVars.cpp
//Implementation of CDbPackedVars.

#include "DbPackedVars.h"
#include "SettingsKeys.h"

#include <BackEndLib/StretchyBuffer.h>
#include <BackEndLib/Ports.h>

//
//Public methods.
//

//*******************************************************************************************
CDbPackedVars::CDbPackedVars(
//Constructor.
//
//Params:
	const BYTE *pBuf) //(in) Buffer containing packed variables.
	: bOldFormat(false)
{
	UnpackBuffer(pBuf, 1);
}

//*******************************************************************************************
CDbPackedVars::~CDbPackedVars()
//Destructor.
{
	Clear();
}

//*******************************************************************************************
CDbPackedVars::CDbPackedVars(const CDbPackedVars& Src)
//Copy constructor.
	: bOldFormat(false)
{
	SetMembers(Src);
}

//*******************************************************************************************
void CDbPackedVars::Clear()
//Zeroes member vars and frees resources associated with this object.
{
	for (std::map <string, UNPACKEDVAR*>::const_iterator i = this->vars.begin();
			i != this->vars.end();	++i)
		delete i->second;
	this->vars.clear();
	this->varIter = this->vars.end();
	this->lastQueryIter = this->vars.end();
}

//*******************************************************************************************
UNPACKEDVAR* CDbPackedVars::GetFirst()
//API to retrieve the first var in the set.
{
	this->varIter = this->vars.begin();
	return GetNext();
}

//*******************************************************************************************
UNPACKEDVAR* CDbPackedVars::GetNext()
{
	if (this->varIter == this->vars.end())
		return NULL;
	return (this->varIter++)->second;
}

//*******************************************************************************************
void CDbPackedVars::SetMembers(const CDbPackedVars &Src)
{
	Clear();

	//Each iteration copies one unpacked var.
	for (std::map <string, UNPACKEDVAR*>::const_iterator i = Src.vars.begin();
			i != Src.vars.end();	++i)
	{
		UNPACKEDVAR *pSrcVar = i->second;
		SetVar(pSrcVar->name.c_str(), pSrcVar->pValue, pSrcVar->dwValueSize, pSrcVar->eType);
	}
}

//*******************************************************************************************
void CDbPackedVars::Unset(const char *pszVarName)
{
	this->vars.erase(pszVarName);

	this->lastQueryIter = this->varIter = this->vars.end(); //invalidate
}

//*******************************************************************************************
void * CDbPackedVars::GetVar(
//Gets value of a variable.
//
//Params:
	const char *pszVarName,    //(in)   Name of var to get value for.
	const void *pNotFoundValue)  const  //(in)   Pointer to return if variable is not found.  Default
								//    is NULL.
//
//Returns:
//Pointer to variable value.
{
	//Find var with matching name.
	const UNPACKEDVAR *pFoundVar = FindVarByName(pszVarName);
	if (pFoundVar)
		return pFoundVar->pValue;
	return (void *) pNotFoundValue;
}
int CDbPackedVars::GetVar(const char *pszVarName, int nNotFoundValue) const
//Overload for returning an int value.
{
	const int *pnRet = (int *)GetVar(pszVarName, (void *)NULL);
	if (pnRet)
	{
		int nRet(*pnRet);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		LittleToBig(&nRet);
#endif
		ASSERT(GetVarValueSize(pszVarName)==sizeof(int));
		return nRet;
	}
	return nNotFoundValue;
}
UINT CDbPackedVars::GetVar(const char *pszVarName, UINT wNotFoundValue) const
//Overload for returning a UINT value.
{
	const UINT *pwRet = (UINT *) GetVar(pszVarName, (void *)NULL);
	if (pwRet)
	{
		UINT wRet(*pwRet);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		LittleToBig(&wRet);
#endif
		ASSERT(GetVarValueSize(pszVarName)==sizeof(UINT));
		return wRet;
	}
	return wNotFoundValue;
}
char CDbPackedVars::GetVar(const char *pszVarName, char cNotFoundValue) const
//Overload for returning a char value.
{
	const char *pcRet = (char *) GetVar(pszVarName, (void *)NULL);
	if (pcRet)
	{
		ASSERT(GetVarValueSize(pszVarName)==sizeof(char));
		return *pcRet;
	}
	return cNotFoundValue;
}
const char * CDbPackedVars::GetVar(const char *pszVarName, const char *pszNotFoundValue) const
//Overload for returning a char * value.
{
	const char *pszRet = (char *) GetVar(pszVarName, (void *)NULL);
	if (pszRet)
	{
		ASSERT(GetVarValueSize(pszVarName)==strlen(pszRet)+1);
		return pszRet;
	}
	return pszNotFoundValue;
}
const WCHAR * CDbPackedVars::GetVar(const char *pszVarName, const WCHAR *pwczNotFoundValue) const
//Overload for returning a WCHAR * value.
{
	const WCHAR *pwczRet = (WCHAR *) GetVar(pszVarName, (void *)NULL);
	if (pwczRet)
	{
		size_t len = WCSlen(pwczRet);
		static USHORT * tempBuffer = 0;
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		static size_t tempBufferSize = 0;
		if (tempBufferSize < (len+1) * sizeof(WCHAR)) {
			delete[] tempBuffer;
			tempBuffer = new USHORT[(len+1) * sizeof(WCHAR)];
			// last one is leaked
		}
		memcpy(tempBuffer, pwczRet, (len+1) * sizeof(WCHAR));
		LittleToBig(tempBuffer, len);
#else
		tempBuffer = reinterpret_cast<USHORT*>(const_cast<WCHAR*>(pwczRet));
#endif
		ASSERT(GetVarValueSize(pszVarName)==(len+1)*sizeof(WCHAR));
		return reinterpret_cast<const WCHAR*>(tempBuffer);
	}
	return pwczNotFoundValue;
}
BYTE CDbPackedVars::GetVar(const char *pszVarName, BYTE ucNotFoundValue) const
//Overload for returning a BYTE value.
{
	const BYTE *pucRet = (BYTE*) GetVar(pszVarName, (void *)NULL);
	if (pucRet)
	{
		ASSERT(GetVarValueSize(pszVarName)==sizeof(BYTE));
		return *pucRet;
	}
	return ucNotFoundValue;
}
bool CDbPackedVars::GetVar(const char *pszVarName, bool bNotFoundValue) const
//Overload for returning a bool value.
{
	const BYTE *pucRet = (BYTE*) GetVar(pszVarName, (void *)NULL); //bool is not always 1 byte, e.g. on Mac, so can't cast to a bool*
	if (pucRet)
	{
		if (GetVarValueSize(pszVarName)==1)
			return *pucRet != 0;

		//Handle old data size (UINT).
		ASSERT(GetVarValueSize(pszVarName)==sizeof(UINT));
		UINT wRet(*((UINT*)pucRet)); //recast what's being pointed at to 4-byte value
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		LittleToBig(&wRet);
#endif
		return (wRet & 0xff) ? true : false; //consider only smallest byte
	}
	return bNotFoundValue;
}
int64_t CDbPackedVars::GetVar(const char* pszVarName, int64_t notFoundValue) const
//Overload for returning a long long int value.
{
	const int64_t* pwRet = (int64_t*)GetVar(pszVarName, (int64_t*)NULL);
	if (pwRet)
	{
		int64_t wRet(*pwRet);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		LittleToBig(&wRet);
#endif
		ASSERT(GetVarValueSize(pszVarName) == sizeof(int64_t));
		return wRet;
	}
	return notFoundValue;
}

//*******************************************************************************************
bool* CDbPackedVars::SetVar(const char *pszVarName, bool bValue)
{
	const BYTE val = bValue ? 1 : 0; //fixes Mac bool var size issue
	return (bool*) SetVar(pszVarName, (const void *)&val, 1, UVT_bool);
}

//*******************************************************************************************
void* CDbPackedVars::SetVar(
//Sets value of either an existing or new var.  If no var with specified name exists then a
//new one is created.
//
//Params:
	const char *pszVarName,     //(in) Name of var to set value for.
	const void *pValue,         //(in) Buffer containing value.
	UINT dwValueSize,          //(in) Size of value in buffer.
	const UNPACKEDVARTYPE eSetType)   //(in) Type of var.
//
//Returns:
//Pointer to memory where value was stored or NULL if not successful.
{
	bool bSuccess = true;

	//Try to get an existing unpacked var with matching name,
	UNPACKEDVAR *pVar = FindVarByName(pszVarName);
	if (!pVar) //No existing var of same name.
	{
		//Create new var and add to list.
		pVar = new UNPACKEDVAR(pszVarName, NULL, 0, eSetType);
		this->vars[pVar->name] = pVar;
	}

	//Set value of var.
	delete[] (char*)pVar->pValue;
	pVar->pValue = new BYTE[dwValueSize];
	if (!pVar->pValue) {bSuccess=false; goto Cleanup;}
	memcpy(pVar->pValue, pValue, dwValueSize);
	pVar->dwValueSize = dwValueSize;
	pVar->eType = eSetType;

Cleanup:
	if (!bSuccess) 
	{
		Clear();
		return NULL;
	}
	return pVar->pValue;
}

//*******************************************************************************************
BYTE * CDbPackedVars::GetPackedBuffer(
//Packs all of the vars stored in this object into one buffer allocated here.
//
//Params:
	UINT &dwBufferSize) //(out) Size in bytes of packed buffer.
const
//
//Returns:
//Pointer to new buffer which caller must delete.
{
	CStretchyBuffer PackedBuf;

	//Each iteration packs one var into buffer.
	for (std::map <string, UNPACKEDVAR*>::const_iterator i = this->vars.begin();
			i != this->vars.end();	++i)
	{
		UNPACKEDVAR *pReadVar = i->second;
		PackedBuf += (UINT) pReadVar->name.size() + 1;
		PackedBuf += pReadVar->name.c_str();
		PackedBuf += (BYTE)0; //null terminate var name

		//Store variable type.
		PackedBuf += static_cast<int>(pReadVar->eType);

		//Store variable data size.
		PackedBuf += pReadVar->dwValueSize;

		//Store variable data.
		PackedBuf.Append((const BYTE *) pReadVar->pValue, pReadVar->dwValueSize);    
	}

	//Append end code to buffer.
	PackedBuf += (UINT) 0;
	dwBufferSize = PackedBuf.Size();
	return PackedBuf.GetCopy();
}

//*******************************************************************************************
UNPACKEDVARTYPE CDbPackedVars::GetVarType(const char *pszVarName) const
//Returns: type of var, or UVT_unknown if no match
{
	UNPACKEDVAR *pVar = FindVarByName(pszVarName);
	return pVar ? pVar->eType : UVT_unknown;
}

//*******************************************************************************************
UINT CDbPackedVars::GetVarValueSize(
//Get size of a var.
//
//Params:
	const char *pszVarName) //(in)   Var to get size from.
const
//
//Returns:
//The size or 0 if no match.
{
	//Since this method is called only following a successful call to FindVarByName,
	//which sets the query iterator, now rewind the increment on this iterator
	//so the find operation below may be performed quickly.
	--this->lastQueryIter;

	UNPACKEDVAR *pVar = FindVarByName(pszVarName);
	return pVar ? pVar->dwValueSize : 0;
}

//
//Private methods.
//

//*******************************************************************************************
bool CDbPackedVars::UnpackBuffer(
//Unpacks variables in buffer into member vars that can be easily accessed.
//
//Params:
	const BYTE *pBuf, //(in) Buffer to unpack.
	const UINT bufSize)  //(in) Size of buffer (value of 0 indicates nothing to unpack)
//
//Returns:
//True if successful, false if not.
{
	bool bSuccess=true;
	UINT wVarNameSize = 0;
	UNPACKEDVAR *pNewVar = NULL;

	//Packed variable buffer format is:
	//{VarNameSize1 UINT}{VarName1 SZ}{VarType1 int}{VarValueSize1 UINT}{VarValue1}
	//{VarNameSize2 UINT}...
	//{EndCode UINT}

	//Remove any vars that have been previously unpacked.
	Clear();

	//Check for empty buffer.
	const BYTE *pRead = pBuf;
	if (bufSize == 0) goto Cleanup;  //Success--nothing to unpack.
	if (!pRead) goto Cleanup; //Success--nothing to unpack.

	//Each iteration unpacks one variable.
	memcpy(&wVarNameSize, pRead, sizeof(UINT));
	pRead += sizeof(UINT);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
	LittleToBig(&wVarNameSize);
#endif
	while (wVarNameSize != 0)
	{
		ASSERT(wVarNameSize < 256); //256 = reasonable limit to var name size.
		if (wVarNameSize >= 256) {bSuccess=false; goto Cleanup;} //more robust
		
		//Get var name.
		char *pszName = new char[wVarNameSize];
		if (!pszName) {bSuccess=false; goto Cleanup;}
		memcpy(pszName, pRead, wVarNameSize);
		ASSERT(pszName[wVarNameSize - 1] == '\0'); //Var name s/b null-terminated.
		if (pszName[wVarNameSize - 1] != '\0') {bSuccess=false; goto Cleanup;}
		pRead += wVarNameSize;

		//Create new packed var.
		pNewVar = new UNPACKEDVAR(pszName);
		delete[] pszName;

		if (this->bOldFormat)
		{
			//No explicit var type specified in the old format.  Look up by name.
			pNewVar->eType = Get1_6VarType(pNewVar->name.c_str());
		} else {
			//Get type of value.
			memcpy(&pNewVar->eType, pRead, sizeof(int));
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
			LittleToBig((int *)(&pNewVar->eType));
#endif
			pRead += sizeof(int);
		}

		//Get size of value.
		memcpy(&pNewVar->dwValueSize, pRead, sizeof(UINT));
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		LittleToBig(&pNewVar->dwValueSize);
#endif
		pRead += sizeof(UINT);

		//Copy value to new unpacked var.
		pNewVar->pValue = new char[pNewVar->dwValueSize];
		if (!pNewVar->pValue) {bSuccess=false; goto Cleanup;}
		memcpy(pNewVar->pValue, pRead, pNewVar->dwValueSize);
		pRead += pNewVar->dwValueSize;

		this->vars[pNewVar->name] = pNewVar;

		//Get size of next variable name or end code.
		memcpy(&wVarNameSize, pRead, sizeof(UINT));
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		LittleToBig(&wVarNameSize);
#endif
		pRead += sizeof(UINT);
	} //...unpack next var in buffer.

Cleanup:
	if (!bSuccess) Clear();
	return bSuccess;
}

//*******************************************************************************************
UNPACKEDVAR *CDbPackedVars::FindVarByName(
//Finds an unpacked variable in list that matches specified name.
//
//Params:
	const char *pszVarName) //(in) Name of var to find.
const
//
//Returns:
//Pointer to unpacked var if a match is found, otherwise NULL.
{
	//If a previous name lookup was performed, then there's a high probability
	//that the next thing looked up is the successor to this query.
	if (this->lastQueryIter != this->vars.end())
		if (++this->lastQueryIter != this->vars.end())
			if (!strcmp(this->lastQueryIter->first.c_str(), pszVarName))
				return this->lastQueryIter->second;

	this->lastQueryIter = this->vars.find(pszVarName);
	if (this->lastQueryIter != this->vars.end())
		return this->lastQueryIter->second;
	return NULL; //No match.
}

//*******************************************************************************************
UNPACKEDVARTYPE CDbPackedVars::Get1_6VarType(const char *pszVarName) const
//Given the name of 1.6 variable name, returns the variable type.
{
	if (strcmp(pszVarName, Settings::ExportPath ) == 0)
		return UVT_char_string;
	if (
		strcmp(pszVarName, Settings::AutoSaveOptions ) == 0 ||
		strcmp(pszVarName, "hd" ) == 0)
      return UVT_uint;
	if (
	  strcmp(pszVarName, Settings::MusicVolume ) == 0 ||
	  strcmp(pszVarName, Settings::SoundEffectsVolume ) == 0 ||
	  strcmp(pszVarName, Settings::RepeatRate ) == 0
	  )
      return UVT_byte;
	if (
	  strcmp(pszVarName, Settings::AutoSave ) == 0 ||
	  strcmp(pszVarName, Settings::Fullscreen ) == 0 ||
	  strcmp(pszVarName, Settings::ItemTips ) == 0 ||
	  strcmp(pszVarName, Settings::Language ) == 0 ||
	  strcmp(pszVarName, "MoveEast" ) == 0 ||
	  strcmp(pszVarName, "MoveNorth" ) == 0 ||
	  strcmp(pszVarName, "MoveNortheast" ) == 0 ||
	  strcmp(pszVarName, "MoveNorthwest" ) == 0 ||
	  strcmp(pszVarName, "MoveSouth" ) == 0 || 
	  strcmp(pszVarName, "MoveSoutheast" ) == 0 ||
	  strcmp(pszVarName, "MoveSouthwest" ) == 0 ||
	  strcmp(pszVarName, "MoveWest" ) == 0 ||
	  strcmp(pszVarName, Settings::Music ) == 0 ||
	  strcmp(pszVarName, "Restart" ) == 0 ||
	  strcmp(pszVarName, Settings::ShowCheckpoints ) == 0 ||
	  strcmp(pszVarName, Settings::ShowErrors ) == 0 ||
	  strcmp(pszVarName, Settings::SoundEffects ) == 0 ||
	  strcmp(pszVarName, "State" ) == 0 || 
	  strcmp(pszVarName, "SwingClockwise" ) == 0 ||
	  strcmp(pszVarName, "SwingCounterclockwise" ) == 0 ||
	  strcmp(pszVarName, "Wait" ) == 0
	  )
		return UVT_int;

   return UVT_unknown;
}

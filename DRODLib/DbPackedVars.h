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
 *
 * ***** END LICENSE BLOCK ***** */

//DbPackedVars.h
//Declarations for CDbPackedVars.
//Class for accessing multiple variables in blob intended for database storage.
//
//USAGE
//
//To add new variables to CDbPackedVars, call one of the SetVar() methods overloaded
//for the type you want to store.  For types not represented, you can choose to add
//a new SetVar() or use the general-purpose SetVar(const void *, UINT) method.
//
//To retrieve a variable, call one of the GetVar() methods.  Returned pointers are only
//good until CDbPackedVars goes out of scope or Clear() is called.
//
//To pack all the current variables into a single buffer, that can be stored in
//a database field, call GetPackedBuffer().
//
//You can unpack a byte buffer into CDbPackedVars in a few different ways.  Depending on
//where your calling code is starting from you may wish to use the "const byte *" or
//"c4_BytesRef &" assignment operators.  Unpacking the byte buffer will result in the
//variables that were stored in the buffer to become accessible through the class methods.

#ifndef DBPACKEDVARS_H
#define DBPACKEDVARS_H

#include <BackEndLib/Assert.h>
#include <BackEndLib/Ports.h>
#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>

#include <cstring>
#include <map>

#include <mk4.h>

enum UNPACKEDVARTYPE
{
    UVT_byte=0,
    UVT_char_string,
    UVT_deprecated_dword,
    UVT_int,
    UVT_deprecated_uchar,
    UVT_uint,
    UVT_wchar_string,
	 UVT_byte_buffer,
	 UVT_bool,
    UVT_unknown
};

struct UNPACKEDVAR
{
	UNPACKEDVAR(const char* pszName, void* pValue=NULL, const UINT dwValueSize=0, UNPACKEDVARTYPE eType=UVT_byte)
		: pValue(pValue), dwValueSize(dwValueSize), eType(eType)
	{ASSERT(pszName);  this->name = pszName;}
	~UNPACKEDVAR() {
		delete[] (char*)this->pValue;
	}

	bool operator <(const UNPACKEDVAR& that) const {return this->name < that.name;}
	string          name;
	void *          pValue;
	UINT           dwValueSize;
	UNPACKEDVARTYPE eType;
};

class CDbPackedVars
{
public:
	CDbPackedVars() : bOldFormat(false) {Clear();}
	CDbPackedVars(const CDbPackedVars& Src);
	CDbPackedVars(const BYTE *pBuf);
	~CDbPackedVars();

	CDbPackedVars& operator=(const CDbPackedVars &Src) {SetMembers(Src); return *this;}
	const BYTE* operator = (const BYTE *pBuf) {UnpackBuffer(pBuf, 1); return pBuf;}
	CDbPackedVars& operator = (const c4_BytesRef &Buf)
	{
      c4_Bytes Bytes = (c4_Bytes) Buf;
		UnpackBuffer(Bytes.Contents(), Bytes.Size());
		return *this;
	}
	c4_BytesRef& operator = (c4_BytesRef &Buf)
	{
		c4_Bytes Bytes = (c4_Bytes) Buf;
		UnpackBuffer(Bytes.Contents(), Bytes.Size());
		return Buf;
	}

	void        Clear();
	bool        DoesVarExist(const char *pszVarName)
	{
		return FindVarByName(pszVarName)!=NULL;
	}
	UNPACKEDVAR*   GetFirst();
	UNPACKEDVAR*   GetNext();
	BYTE *         GetPackedBuffer(UINT &dwBufferSize) const;
	void *         GetVar(const char *pszVarName, const void *pNotFoundValue = NULL) const;
	const char *   GetVar(const char *pszVarName, const char *pszNotFoundValue = NULL) const;
	const WCHAR *  GetVar(const char *pszVarName, const WCHAR *pwczNotFoundValue = NULL) const;
	int            GetVar(const char *pszVarName, int nNotFoundValue = 0) const;
	UINT        GetVar(const char *pszVarName, UINT wNotFoundValue = 0) const;
	char        GetVar(const char *pszVarName, char cNotFoundValue = 0) const;
	BYTE        GetVar(const char *pszVarName, BYTE ucNotFoundValue = 0) const;
	bool        GetVar(const char *pszVarName, bool ucNotFoundValue = false) const;

	UNPACKEDVARTYPE GetVarType(const char *pszVarName) const;
	UINT       GetVarValueSize(const char *pszVarName) const;

	void Unset(const char *pszVarName);

	void			UseOldFormat(const bool bVal=true) {this->bOldFormat = bVal;}

	void *         SetVar(const char *pszVarName, const void *pValue, UINT dwValueSize, const UNPACKEDVARTYPE eType);
	char *         SetVar(const char *pszVarName, const char *pszValue)
	{
		return (char *) SetVar(pszVarName, (const void *) pszValue, strlen(pszValue)+1, UVT_char_string);
	}
	WCHAR *        SetVar(const char *pszVarName, const WCHAR *pwczValue)
	{
		size_t size = WCSlen(pwczValue)+1;
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		ASSERT(sizeof(USHORT) == sizeof(WCHAR));
		USHORT* buffer = new USHORT[size];
		memcpy(
			reinterpret_cast<char*>(buffer),
			reinterpret_cast<const char*>(pwczValue),
			size * sizeof(WCHAR)
		);
		LittleToBig(buffer, size);
#else
		const WCHAR* buffer = pwczValue;
#endif
		WCHAR * result = reinterpret_cast<WCHAR*>(
			SetVar(pszVarName, reinterpret_cast<const void*>(buffer), (size)*sizeof(WCHAR), UVT_wchar_string)
		);
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		delete[] buffer;
#endif
		return result;
	}
	int *       SetVar(const char *pszVarName, int nValue)
	{
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		LittleToBig(&nValue);
#endif
		return (int *) SetVar(pszVarName, (const void *) &nValue, sizeof(nValue), UVT_int);
	}
	UINT *         SetVar(const char *pszVarName, UINT wValue)
	{
#if (GAME_BYTEORDER == GAME_BYTEORDER_BIG)
		LittleToBig(&wValue);
#endif
		return (UINT *) SetVar(pszVarName, (const void *) &wValue, sizeof(wValue), UVT_uint);
	}
	BYTE *         SetVar(const char *pszVarName, unsigned char ucValue)
	{
		return (BYTE *) SetVar(pszVarName, (const void *) &ucValue, 1, UVT_byte);
	}
	bool*         SetVar(const char *pszVarName, bool bValue);

private:
	UNPACKEDVARTYPE	Get1_6VarType(const char *pszVarName) const;
	void            SetMembers(const CDbPackedVars &Src);
	UNPACKEDVAR *   FindVarByName(const char *pszVarName) const;
	bool            UnpackBuffer(const BYTE *pBuf, const UINT bufSize);

	std::map <string, UNPACKEDVAR*> vars;
	std::map <string, UNPACKEDVAR*>::iterator varIter;
	mutable std::map <string, UNPACKEDVAR*>::const_iterator lastQueryIter;
	bool bOldFormat;	//indicates newer eType var field should be ignored
};

#endif //...#ifndef DBEXTRAVARS_H

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

//StretchyBuffer.h
//Declarations for CStretchyBuffer.
//Class for accessing a dynamically sized buffer.

#ifndef STRETCHYBUFFER_H
#define STRETCHYBUFFER_H

#include "Types.h"
#include "Assert.h"
#include "Wchar.h"
#include <mk4.h>
#ifdef __sgi
#include <string.h>
#else
#include <cstring>
#endif

//Enumeration of supported data formats.
enum DataFormat
{
	DATA_UNKNOWN=0,
	DATA_BMP=1,    //images
	DATA_JPG,
	DATA_PNG,
	DATA_S3M=20,   //music
	DATA_WAV=40,   //sound samples
	DATA_OGG,
	DATA_TTF=60,    //fonts
	DATA_THEORA=70, //movie
	DATA_NUM_FORMATS=8   //# formats supported
};

class CStretchyBuffer
{
public:
	CStretchyBuffer();
	CStretchyBuffer(const string &str);
	CStretchyBuffer(const WSTRING &wstr);
	CStretchyBuffer(const BYTE *pSetBuf, const UINT dwSetBufSize);
	CStretchyBuffer(const CStretchyBuffer &that);
	~CStretchyBuffer() {Clear();}
	CStretchyBuffer &operator= (const CStretchyBuffer &that)
	{
		Clear();
		if (that.Size())
			Set(that.pBuf, that.Size());
		return *this;
	}
	CStretchyBuffer &operator= (const c4_Bytes &that)
	{
		Clear();
		if (that.Size())
			Set(that.Contents(), that.Size());
		return *this;
	}
	operator BYTE * () const {return pBuf;}
	operator const BYTE * () const {return pBuf;}
	BYTE operator[] (UINT index) const {return pBuf[index];}
	UINT GetUINTat(UINT& index) const;
	
	CStretchyBuffer & operator += (UINT wAdd);
	CStretchyBuffer & operator += (int nAdd) { return operator += (UINT(nAdd)); }
	CStretchyBuffer & operator += (const char *pszAdd) {Append((const BYTE *) pszAdd, strlen(pszAdd)); return *this;}
	CStretchyBuffer & operator += (const WCHAR *pwczAdd) {Append((const BYTE *) pwczAdd, WCSlen(pwczAdd) * sizeof(WCHAR)); return *this;}
	CStretchyBuffer & operator += (BYTE bytAdd) {Append((const BYTE *) &bytAdd, sizeof(bytAdd)); return *this;}
	CStretchyBuffer & operator += (CStretchyBuffer& buf) {Append((const BYTE*)buf, buf.Size()); return *this;}

	bool  Alloc(UINT dwNewAllocSize);
	void  Append(const BYTE *pAddBuf, UINT dwAddBufSize);
	void  Clear();
	inline bool empty() const {return this->dwBufSize == 0;}
	void  Encode(BYTE code = 0xFF);
	void  Decode() {Encode();}
	BYTE *   GetCopy();
	UINT  ReadChunk(void* pDest, const UINT wBytes);
	bool  Realloc(UINT dwNewAllocSize);
	void  Set(const BYTE *pSetBuf, UINT dwSetBufSize);
	void  SetSize(const UINT dwSetBufSize);
	inline UINT Size() const {return this->dwBufSize;}
	bool  Compress(BYTE* &encodedBuf, ULONG &encodedSize);
	bool  Uncompress(BYTE* &decodedBuf, ULONG &decodedSize);
	bool  RemoveBytes(UINT from, UINT to);

private:
	UINT dwBufSize;
	UINT dwAllocSize;
	UINT dwReadIndex;
	BYTE *pBuf;
};

#endif //...#ifndef STRETCHYBUFFER_H

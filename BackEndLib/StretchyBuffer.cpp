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

//StretchyBuffer.cpp.
//Implementation of CStretchyBuffer.

#include <zlib.h> //must be first
#include "StretchyBuffer.h"
#include "Assert.h"
#include "Types.h"

#include <string.h>

//
//Public methods.
//

//****************************************************************************************
CStretchyBuffer::CStretchyBuffer()
	: pBuf(NULL)
//Constructor.
{
	Clear();
}

//****************************************************************************************
CStretchyBuffer::CStretchyBuffer(
//Constructor that initializes buffer.
//
//Params:
	const CStretchyBuffer &that)  //(in) Buffer that will be copied to memory alloced by this object.
	: pBuf(NULL)
{
	Clear();
	if (that.Size())
		Set(that.pBuf, that.Size());
}

//****************************************************************************************
CStretchyBuffer::CStretchyBuffer(const string &str)
//Constructor that initializes buffer.
	: pBuf(NULL)
{
	Clear();
	Set((const BYTE*)str.c_str(), str.size());
}

//****************************************************************************************
CStretchyBuffer::CStretchyBuffer(const WSTRING &wstr)
//Constructor that initializes buffer.
	: pBuf(NULL)
{
	Clear();
	Set((const BYTE*)wstr.c_str(), wstr.size());
}

//****************************************************************************************
CStretchyBuffer::CStretchyBuffer(
//Constructor that initializes buffer.
//
//Params:
	const BYTE *pSetBuf, //(in) Buffer that will be copied to memory alloced by this object.
	const UINT dwSetBufSize)     //(in) Size of buffer in bytes.
	: pBuf(NULL)
{
	Clear();
	Set(pSetBuf, dwSetBufSize);
}

//****************************************************************************************
void CStretchyBuffer::Clear()
//Zeros member vars and frees resources associated with this object.
{
	delete[] this->pBuf;
	this->pBuf = NULL;

	this->dwBufSize = this->dwAllocSize = this->dwReadIndex = 0;
}

//******************************************************************************
void CStretchyBuffer::Encode(BYTE code)
//Encodes/decodes data in the buffer.
//
//Params:
{
	BYTE *pBuf = this->pBuf;
	for (UINT nIndex=Size(); nIndex--; )
	{
		*pBuf = *pBuf ^ code;   //flip requested bits
		++pBuf;
	}
}

//****************************************************************************************
BYTE *CStretchyBuffer::GetCopy()
//Gets a copy of this object's buffer that this object will not delete or change.
//
//Returns:
//Pointer to new buffer or NULL.
{
	if (this->dwBufSize == 0) return NULL; //Buffer is empty.

	//Alloc new buffer.
	BYTE *pCopyBuf = new BYTE[this->dwBufSize];
	if (!pCopyBuf) return NULL;

	//Copy bytes into new buffer.
	memcpy(pCopyBuf, this->pBuf, this->dwBufSize);

	return pCopyBuf;
}

//****************************************************************************************
void CStretchyBuffer::Set(
//Sets object's buffer to contain same bytes as specified buffer.
//
//Params:
	const BYTE *pSetBuf, //(in) Buffer to copy.
	UINT dwSetBufSize)  //(in) Size of buffer.
{
	ASSERT(dwSetBufSize > 0);
	
	//Make sure object buffer is large enough.
	if (dwSetBufSize > this->dwAllocSize)
	{
		if (!Alloc(dwSetBufSize)) {ASSERTP(false, "Allocation failure."); return;}
	}

	//Copy buffer to object buffer.
	memcpy(this->pBuf, pSetBuf, dwSetBufSize);
	this->dwBufSize = dwSetBufSize;
	this->dwReadIndex = 0;
}

//****************************************************************************************
void CStretchyBuffer::SetSize(
//back door, when pBuf is written to directly
//
//Params:
	const UINT dwSetBufSize)  //(in)
{
	ASSERT(this->dwAllocSize >= this->dwBufSize);
	this->dwBufSize = dwSetBufSize;
}

//****************************************************************************************
void CStretchyBuffer::Append(
//Appends a buffer to the object's buffer.
//
//Params:
	const BYTE *pAddBuf, //(in) Buffer to append.
	UINT dwAddBufSize)   //(in) Size of buffer.
{
	ASSERT(dwAddBufSize > 0);

	//Make sure object buffer is large enough.
	if (this->dwBufSize + dwAddBufSize > this->dwAllocSize)
	{
		//Grow buffer by 2.5x each expansion for best general efficiency
		if (!Realloc(static_cast<UINT>((this->dwBufSize + dwAddBufSize) * 2.5)))
				{ASSERTP(false, "Allocation failure."); return;}
	}

	//Copy buffer to end of object buffer.
	memcpy(this->pBuf + this->dwBufSize, pAddBuf, dwAddBufSize);
	this->dwBufSize += dwAddBufSize;
}

//****************************************************************************************
bool CStretchyBuffer::Alloc(
//Allocs the buffer.  
//
//Only call Alloc() if you wish to copy data into the buffer directly (not using 
//class methods) and the buffer must be preallocated to a specific size.
//
//Params:
	UINT dwNewAllocSize) //(in) Size to allocate buffer to.
//
//Returns:
//True if successful, false if not.
{
	ASSERT(dwNewAllocSize > 0);

	if (dwNewAllocSize == this->dwAllocSize) return true; //Nothing to do.

	//Delete existing buffer.
	delete[] this->pBuf;
	this->dwAllocSize = this->dwBufSize = 0;

	//Alloc new buffer and set related member vars.
	this->pBuf = new BYTE[dwNewAllocSize];
	if (!this->pBuf) return false;
	this->dwAllocSize = dwNewAllocSize;

	return true;
}

//****************************************************************************************
bool CStretchyBuffer::Realloc(
//Reallocates the buffer to a new size and containing whatever is in the existing buffer.
//
//Only call Alloc() if you wish to copy data into the buffer directly (not using 
//class methods) and the buffer must be preallocated to a specific size.
//
//Params:
	UINT dwNewAllocSize) //(in) Size to reallocate buffer to.
//
//Returns:
//True if successful, false if not.
{
	BYTE *pNewBuf = NULL;
	UINT dwCopySize;

	ASSERT(dwNewAllocSize > 0);

	if (dwNewAllocSize == this->dwAllocSize) return true; //Nothing to do.

	//Create new buffer.
	pNewBuf = new BYTE[dwNewAllocSize];
	if (!pNewBuf) 
	{
		Clear(); 
		return false;
	}

	//Copy bytes from old buffer into new buffer.  Data will be truncated if
	//it all can't fit.
	dwCopySize = this->dwBufSize;
	if (dwCopySize > 0)
	{
		if (dwCopySize > dwNewAllocSize) dwCopySize = dwNewAllocSize;
		ASSERT(this->pBuf);
		memcpy(pNewBuf, this->pBuf, dwCopySize);
	}
	
	//Delete old buffer.
	delete[] this->pBuf;

	//Set member vars for new buffer.
	this->pBuf = pNewBuf;
	this->dwAllocSize = dwNewAllocSize;
	this->dwBufSize = dwCopySize;

	return true;
}

//****************************************************************************************
bool CStretchyBuffer::Compress(
//Compress the data.
//
//Returns: whether compression succeeded
//
//Params:
	BYTE* &encodedBuf, ULONG &encodedSize)   //(out)
{
	ASSERT(!encodedBuf);
	encodedSize = 0;

	const ULONG srcLen = Size();
	const uLongf max_size_needed = compressBound(srcLen);
	const uLongf min_size_to_attempt = max_size_needed / 10;
	uLongf zencodedSize = max_size_needed;
	do {
		try {
			encodedBuf = new BYTE[zencodedSize];
		}
		catch (std::bad_alloc&) {
			//try again with smaller buffer
			zencodedSize = uLongf(zencodedSize * 0.9f);
		}
	} while (!encodedBuf && zencodedSize > min_size_to_attempt);

	const int res = compress(encodedBuf, &zencodedSize, (const BYTE*)(*this), srcLen);
	const bool bSuccess = res == Z_OK;
	if (bSuccess)
		encodedSize = zencodedSize;
	return bSuccess;
}

//****************************************************************************************
bool CStretchyBuffer::Uncompress(
//Uncompress the compressed buffer.
//
//Returns: whether Uncompression succeeded
//
//Params:
	BYTE* &decodedBuf, ULONG &decodedSize)   //(out)
{
	const ULONG fileSize = Size();
	uLongf zdecodedSize = decodedSize ? decodedSize : //if provided, use this value
			fileSize * (fileSize > 250000 ? 4 : 15);   //assume: about as big as the uncompressed data will ever be
	if (!decodedBuf) //if not already allocated
		decodedBuf = new BYTE[zdecodedSize+1]; //allow null-termination
	if (!decodedBuf)
	{
		decodedSize = 0;
		return false;
	}
	int res;
	do {
		res = uncompress(decodedBuf, &zdecodedSize, (BYTE*)(*this), fileSize);
		switch (res)
		{
			case Z_BUF_ERROR:
				//This wasn't enough memory to decode the data to,
				//so double the buffer size.
				zdecodedSize *= 2;
				delete[] decodedBuf;
				decodedBuf = new BYTE[zdecodedSize+1]; //allow adding null-terminating char to decodedBuf, if desired
				if (!decodedBuf)
				{
					decodedSize = 0;
					return false;
				}
				break;

			case Z_DATA_ERROR:
				delete[] decodedBuf;
				decodedBuf = NULL;
				decodedSize = 0;
				return false;

			case Z_MEM_ERROR:
				delete[] decodedBuf;
				decodedBuf = NULL;
				decodedSize = 0;
				return false;
		}
	} while (res != Z_OK);  //success

	//Upon successful completion, decodedSize is the actual size of the uncompressed buffer.
	decodedSize = zdecodedSize;
	return true;
}

//****************************************************************************************
UINT CStretchyBuffer::ReadChunk(void* pDest, const UINT wBytes)
//Acts similar to a file pointer, copying the desired number of bytes to the destination buffer.
//Then advances the read pointer so next call will copy from there.
//Copies fewer bytes if less than the desired amount remains to be copied.
//
//Returns:
{
	ASSERT(pDest);
	UINT wBytesRead = wBytes;
	if (this->dwReadIndex + wBytesRead > this->dwBufSize)
		wBytesRead = this->dwBufSize - this->dwReadIndex;
	
	memcpy(pDest, this->pBuf + this->dwReadIndex, wBytesRead);
	this->dwReadIndex += wBytesRead;

	return wBytesRead;
}

//****************************************************************************************
bool CStretchyBuffer::RemoveBytes(
//Remove the selected bytes from the buffer.
//
//Returns: false if the range is invalid, true otherwise
//
//Params:
UINT from, UINT to)   //(in)
{
	ASSERT(to > from);
	const UINT dwSize = Size();
	if ((int)from < 0)
		from += dwSize;
	if ((int)to < 0)
		to += dwSize;

	if (((int)from < 0) || ((int)to < 0) || (from > dwSize-1) || (to > dwSize)) return false;

	memmove(this->pBuf + from, this->pBuf + to, dwSize - from);
	this->dwBufSize -= to-from;
	return true;
}

//****************************************************************************************
CStretchyBuffer &CStretchyBuffer::operator += (
//Concatenate a UINT to this CStretchyBuffer
//
//Returns: *this
//
//Params:
UINT wAdd)  //(in)
{
	BYTE b;
	b = (wAdd & 0xff); Append(&b, 1);
	b = (wAdd >> 8 & 0xff); Append(&b, 1);
	b = (wAdd >> 16 & 0xff); Append(&b, 1);
	b = (wAdd >> 24 & 0xff); Append(&b, 1);
	return *this;
}

//*****************************************************************************
UINT CStretchyBuffer::GetUINTat(
//Returns: UINT value encoded at this point in the buffer
//
//Params:
	UINT& index) //(in/out) index of data.  It is updated to past the UINT data on exit.
const
{
	const UINT val =
		(this->pBuf[index+3] << 24) +
		(this->pBuf[index+2] << 16) +
		(this->pBuf[index+1] << 8) +
		this->pBuf[index];
	index += 4;
	return val;
}

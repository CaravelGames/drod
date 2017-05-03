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
 * 1997, 2000, 2001, 2002, 2003, 2005 Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 * Matt Schikore (Schik)
 *
 * ***** END LICENSE BLOCK ***** */

#include "GameStream.h"
#include "Types.h"
#include "Files.h"

CGameStream::CGameStream(const WCHAR* wszSetFilename, const char *pszOptions)
{
	ASSERT(wszSetFilename);

	wszFilename = new WCHAR[WCSlen(wszSetFilename)+1];
	WCScpy(this->wszFilename, wszSetFilename);

	this->pFile = CFiles::Open(this->wszFilename, pszOptions);
}


CGameStream::~CGameStream()
{
	delete[] this->wszFilename;
	this->wszFilename = 0;
	if (this->pFile) 
  {
	  fclose(this->pFile);
	  this->pFile = 0;
  }
}


int CGameStream::Read(void* buffer, int size)
{
	if (!this->pFile)
		return 0;
	ASSERT(buffer);

	return fread(buffer, 1, size, this->pFile);
}


bool CGameStream::Write(const void* buffer, int size)
{
	if (!this->pFile)
		return false;
	ASSERT(buffer);

	if (static_cast<unsigned int>(size) != fwrite(buffer, 1, size, this->pFile))
		return false;
	return true;
}

bool CGameStream::Save(c4_Storage* pStorage)
{
	ASSERT(pStorage);

	if (this->pFile) {
		fclose(this->pFile);
		this->pFile = 0;
	}

	this->pFile = CFiles::Open(this->wszFilename, "wb");
	if (!this->pFile)
		return false;
	pStorage->SaveTo(*this);
	fclose(this->pFile);
	this->pFile = 0;
	return true;
}

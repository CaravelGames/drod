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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//DbCommands.h
//Declarations for CDbCommands.
//Class for storing game commands.

#ifndef DBCOMMANDS_H
#define DBCOMMANDS_H

#include <BackEndLib/Types.h>
#include <mk4.h>

#include <vector>
using std::vector;

struct COMMANDNODE
{
	COMMANDNODE() : bytCommand(0), byt10msElapsedSinceLast(0) { }
	COMMANDNODE(const BYTE bytCommand, const BYTE byt10msElapsedSinceLast)
		: bytCommand(bytCommand), byt10msElapsedSinceLast(byt10msElapsedSinceLast)
	{ }
	BYTE        bytCommand;
	BYTE        byt10msElapsedSinceLast;
};

//******************************************************************************
class CDbCommands
{
public:
	typedef vector<COMMANDNODE> CommandSequence;
	typedef CommandSequence::const_iterator const_iterator;

	const_iterator begin() const {return this->commands.begin();}
	const_iterator end() const {return this->commands.end();}

	CDbCommands();
	CDbCommands(CDbCommands &Src) {SetMembers(Src);}
	CDbCommands& operator = (const CDbCommands &Src);
	const BYTE* operator = (const BYTE *pBuf) {UnpackBuffer(pBuf); return pBuf;}
	CDbCommands& operator = (const c4_BytesRef &Buf);
	c4_BytesRef& operator = (c4_BytesRef &Buf);

	void        Add(const int nCommand, const BYTE byt10msElapsedSinceLast = 0);
	void			AddData(const BYTE bytX, const BYTE bytY);
	void        Clear();
	UINT        Count() const;
	bool        Empty() const {return this->commands.size() == 0;}
	void        Freeze();
	const_iterator  Get(UINT dwIndex);
	const COMMANDNODE& GetConst(UINT dwIndex) const;
	bool        GetData(UINT &wX, UINT &wY) const;
	const_iterator  GetFirst();
	const_iterator  GetCurrent();
	const_iterator  GetNext();
	const_iterator  GetPrev();
	BYTE *         GetPackedBuffer(UINT &dwBufferSize) const;
	UINT        GetSize() const {return this->commands.size();}
	UINT			GetTimeElapsed() const;
	bool        IsFrozen() const {return this->bIsFrozen;}
	void        RemoveLast();
	void        Replace(UINT dwStart, UINT dwStop, const int nCommand,
							const BYTE bytX=0, const BYTE bytY=0);
	void        ResetTimeOfLastAdd();
	void        Truncate(const UINT dwKeepCount);
	void        Unfreeze();

private:
	bool        SetMembers(const CDbCommands &Src);
	void        UnpackBuffer(const BYTE *pBuf);

	CommandSequence commands;
	const_iterator commandIter;
	bool        bIsFrozen;
	UINT dwTimeOfLastAdd;
};

#endif //...#ifndef DBCOMMANDS_H

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

#include "DbCommands.h"
#include "GameConstants.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/StretchyBuffer.h>
#include <BackEndLib/SysTimer.h>

using namespace std;

//
//CDbCommands public methods.
//

//******************************************************************************
CDbCommands::CDbCommands() : bIsFrozen(false), dwTimeOfLastAdd(0)
{
	Clear();
}

//******************************************************************************
CDbCommands& CDbCommands::operator = (const CDbCommands &Src)
{
	this->bIsFrozen = false; //to avoid assertion if previously frozen
	Clear();
	SetMembers(Src);
	return *this;
}

//******************************************************************************
CDbCommands& CDbCommands::operator = (const c4_BytesRef &Buf)
{
   c4_Bytes Bytes = (c4_Bytes) Buf;
	UnpackBuffer(Bytes.Contents());
	return *this;
}

//******************************************************************************
c4_BytesRef& CDbCommands::operator = (c4_BytesRef &Buf)
{
	c4_Bytes Bytes = (c4_Bytes) Buf;
	UnpackBuffer(Bytes.Contents());
	return Buf;
}

//******************************************************************************
void CDbCommands::Clear()
//Frees resources and resets members.
{
	//If object is frozen, then modifying list not allowed.
	if (this->bIsFrozen)
	{
		ASSERT(!"CDbCommands::Clear: Object is frozen.");
		return;
	}

	this->commands.clear();
	this->commandIter = end();
	this->dwTimeOfLastAdd = 0;
}

//******************************************************************************
UINT CDbCommands::Count() const
//Returns: number of commands in sequence.
//This number may not be equal to the number of elements, as there are some
//commands that require multiple element slots.
{
	UINT wCount=0;
	for (vector<COMMANDNODE>::const_iterator comIter = begin();
			comIter != end(); ++comIter, ++wCount)
	{
		//Skip extra data fields record for complex commands.
		if (bIsComplexCommand(comIter->bytCommand))
			if (++comIter == this->commands.end())
				break;
	}
	ASSERT(wCount <= GetSize());
	return wCount;
}

//******************************************************************************
void CDbCommands::Freeze()
{
	ASSERT(!this->bIsFrozen);
	this->bIsFrozen = true;
}

void CDbCommands::Unfreeze()
{
	ASSERT(this->bIsFrozen);
	this->bIsFrozen = false;
}

//******************************************************************************
void CDbCommands::RemoveLast()
//Removes last command.
{
	Truncate(Count()-1);
}

//******************************************************************************
void CDbCommands::Truncate(
//Truncate the command list to contain only the first X commands.
//
//Params:
	const UINT dwKeepCount)   //# of commands to keep.
{
	ASSERT(dwKeepCount < Count());

	//If object is frozen, then modifying list not allowed.
	if (this->bIsFrozen)
	{
		ASSERT(!"Object is frozen.");
		return;
	}

	UINT wCount=0;
	vector<COMMANDNODE>::iterator comIter;
	for (comIter = this->commands.begin();
			wCount < dwKeepCount; ++comIter, ++wCount)
	{
		//Skip extra data fields record for complex commands.
		if (bIsComplexCommand(comIter->bytCommand))
			++comIter;
	}
	this->commands.erase(comIter, this->commands.end());

	//Invalidate.
	this->commandIter = end();
}

//******************************************************************************
void CDbCommands::Add(
//Adds a new command to list.
//
//Params:
	const int nCommand,              //(in)   One of the CMD_* constants.
	const BYTE byt10msElapsedSinceLast) //(in)   Time elapsed since last command in 10ms
									//    increments.  If 0 (default), the time 
									//    between this call and last call to Add()
									//    will be used.
{
	//If object is frozen, then modifying list not allowed.
	if (this->bIsFrozen)
	{
		ASSERT(!"CDbCommands::Add: Object is frozen.");
		return;
	}

	ASSERT(nCommand < COMMAND_COUNT);
	if (nCommand >= COMMAND_COUNT)
		return; //don't allow adding invalid commands

	//Get time elapsed since last, if not specified.
	BYTE bytElapsed;
	if (byt10msElapsedSinceLast > 0)
	{
		bytElapsed = byt10msElapsedSinceLast;
	} else {
		if (this->dwTimeOfLastAdd == 0)
			bytElapsed = 0; //First call to Add, so elapsed time does not apply.
		else
		{
			//Get # of milliseconds between calls.
			const UINT dwMsElapsedSinceLast = GetTicks() - dwTimeOfLastAdd;
			
			//Limit to a byte value.  Use 10ms increments.
			bytElapsed = (dwMsElapsedSinceLast >= 2550) ? 255 :
					static_cast<BYTE>(dwMsElapsedSinceLast / 10);	//convert to 10ms increment
		}
	}

	//Create new command and set members.
	this->commands.push_back(COMMANDNODE(static_cast<BYTE>(nCommand), bytElapsed));

	//Integrity of original value should be preserved.
	ASSERT(static_cast<int>(this->commands.back().bytCommand)==nCommand);

	ResetTimeOfLastAdd();
}

//******************************************************************************
void CDbCommands::AddData(const BYTE bytX, const BYTE bytY)
//Adds a command item to the list that stores special data.
//This should be called only after adding a command that requires extra data.
{
	//the last command should be one that requires a data member to be added.
	//Currently, the following command is the only one that requires it.
	//Add others as needed.
	ASSERT(bIsComplexCommand(this->commands.back().bytCommand));

	//If object is frozen, then modifying list not allowed.
	if (this->bIsFrozen)
	{
		ASSERT(!"CDbCommands::AddData: Object is frozen.");
		return;
	}
		
	this->commands.push_back(COMMANDNODE(bytX, bytY));
}

//******************************************************************************
BYTE *CDbCommands::GetPackedBuffer(
//Gets a packed buffer containing all the commands.
//
//Params:
	UINT &dwBufferSize) //(out)  Size in bytes of the buffer.
//
//Returns:
//Pointer to packed buffer or NULL if no commands.
const
{
	CStretchyBuffer PackedBuf;

	//Each iteration packs one command record into buffer.
	for (vector<COMMANDNODE>::const_iterator comIter = begin();
			comIter != end(); ++comIter)
	{
		PackedBuf += BYTE(comIter->bytCommand);
		PackedBuf += BYTE(comIter->byt10msElapsedSinceLast);
	}

	//Append end code to buffer.
	PackedBuf += BYTE(0);
	dwBufferSize = PackedBuf.Size();
	return PackedBuf.GetCopy();
}

//******************************************************************************
CDbCommands::const_iterator CDbCommands::GetCurrent()
//Returns: the current command iterator
{
	return this->commandIter;
}

//******************************************************************************
CDbCommands::const_iterator CDbCommands::GetFirst()
//Returns: First command or end() if there are none.
{
	return this->commandIter = begin();
}

//******************************************************************************
CDbCommands::const_iterator CDbCommands::GetNext()
//Returns: Next command or end() if there are no more.
{
	if (this->commandIter != end())
	{
		//Skip extra fields for augmented commands.
		if (bIsComplexCommand(this->commandIter->bytCommand))
			++this->commandIter;

		++this->commandIter;
	}
	return this->commandIter;
}

//******************************************************************************
CDbCommands::const_iterator CDbCommands::GetPrev()
//Returns: Previous command or begin() if there are no more.
{
	//Advance forward until the command before the current one.
	//ATTN: Stepping backward is not reliable due to ambiguity
	//of complex command data values (possible misinterpretation as command types).
	CDbCommands::const_iterator iter = begin();
	for (CDbCommands::const_iterator next=iter; next != this->commandIter;
			next += (bIsComplexCommand(next->bytCommand) ? 2 : 1))
		iter = next;

	return this->commandIter = iter;
}

//******************************************************************************
CDbCommands::const_iterator CDbCommands::Get(
//Gets a command by zero-based index.  A subsequent call to GetNext() would
//then return the next command, or end() if at the end of the commands.
//
//Params:
	UINT dwIndex) //(in)   Index of command to get.
//
//Returns:
//Command.
{
	ASSERTP(dwIndex <= Count(), "Bad index param.");

	this->commandIter = begin();
	while (dwIndex--)
	{
		//Skip extra data fields record for complex commands.
		if (bIsComplexCommand(this->commandIter->bytCommand))
			++this->commandIter;
		++this->commandIter;
	}
	return this->commandIter;
}

//******************************************************************************
const COMMANDNODE& CDbCommands::GetConst(
//Gets a command by zero-based index, but does not modify the object.
//A subsequent call to GetNext() is not affected by this call.
//
//Params:
	UINT dwIndex) //(in)   Index of command to get.
//
//Returns:
//Command.
const
{
	ASSERTP(dwIndex < Count(), "Bad index param.");

	vector<COMMANDNODE>::const_iterator comIter = begin();
	while (dwIndex--)
	{
		//Skip extra data fields record for complex commands.
		if (bIsComplexCommand(comIter->bytCommand))
			++comIter;
		++comIter;
	}
	return *comIter;
}

//*****************************************************************************
bool CDbCommands::GetData(
//Retrieves data fields for complex commands.
//Returns: true if operation succeeded, otherwise false
//
//Params:
	UINT &wX, UINT &wY)	//(out)
const
{
	if (this->commandIter == end())
		return false;
	if (!bIsComplexCommand(this->commandIter->bytCommand))
		return false;
	vector<COMMANDNODE>::const_iterator data = this->commandIter + 1;
	if (data == end())
		return false;

	wX = data->bytCommand;
	wY = data->byt10msElapsedSinceLast;
	return true;
}

//*****************************************************************************
UINT CDbCommands::GetTimeElapsed() const
//Returns: the time duration covered by these commands (in 10ms increments)
{
	UINT dwTimeElapsed = 0;

	for (vector<COMMANDNODE>::const_iterator comIter = begin();
			comIter != end(); ++comIter)
	{
		dwTimeElapsed += comIter->byt10msElapsedSinceLast;
		if (bIsComplexCommand(comIter->bytCommand))
			++comIter;	//skip extra fields
	}

	return dwTimeElapsed;
}

 //******************************************************************************
void CDbCommands::Replace(
//Replaces commands within dwStart<=index<dwStop with a single new command.
//Afterward, GetCurrent() will return the new single command.
//
//Params:
	UINT dwStart,        // (in) index of first replaced command
	UINT dwStop,         // (in) index just after last replaced command
	const int nCommand,  // (in) command to replace with
	const BYTE bytX,     // (in) data for complex command
	const BYTE bytY)     // (in) data for complex command
{
	//If object is frozen, then modifying list not allowed.
	if (this->bIsFrozen)
	{
		ASSERT(!"CDbCommands::Replace: Object is frozen.");
		return;
	}

	ASSERT(nCommand < COMMAND_COUNT);
	if (nCommand >= COMMAND_COUNT)
		return; //don't allow adding invalid commands

	if (!(dwStart+bIsComplexCommand(nCommand)<dwStop && dwStop<=GetSize()))
	{
		ASSERT(!"CDbCommands::Replace: Invalid indices.");
		return;
	}

	UINT dwIndex = 0;
	CommandSequence::iterator iter = commands.begin();
	while (dwIndex < dwStart)
	{
		if (bIsComplexCommand(iter->bytCommand))
			++iter;
		++iter;
		++dwIndex;
	}
	CommandSequence::iterator startIter = iter;

	UINT dwTotalElapsed = 0;
	while (dwIndex < dwStop)
	{
		dwTotalElapsed += iter->byt10msElapsedSinceLast;
		if (bIsComplexCommand(iter->bytCommand))
			++iter;
		++iter;
		++dwIndex;
	}
	CommandSequence::iterator stopIter = iter;

	//Limit to a byte value.
	if (dwTotalElapsed > 255)
		dwTotalElapsed = 255;

	iter = startIter;
	iter->bytCommand = nCommand;
	iter->byt10msElapsedSinceLast = static_cast<BYTE>(dwTotalElapsed);
	++iter;
	if (bIsComplexCommand(nCommand))
	{
		*iter = COMMANDNODE(bytX, bytY);
		++iter;
	}

	commands.erase(iter, stopIter);
	commandIter = startIter;
}

//******************************************************************************
void CDbCommands::ResetTimeOfLastAdd()
{
	//Update time of last call to time of this call.
	this->dwTimeOfLastAdd = GetTicks();   
}

//
//Private methods.
//

//******************************************************************************
bool CDbCommands::SetMembers(const CDbCommands &Src)
//Deep member copy.
{
	this->bIsFrozen = false; //to avoid assertions in UnpackBuffer
	this->dwTimeOfLastAdd = Src.dwTimeOfLastAdd;
	UINT dwSize;
	BYTE *pbytCopy = Src.GetPackedBuffer(dwSize);
	UnpackBuffer(pbytCopy);
	delete[] pbytCopy;
	this->bIsFrozen = Src.bIsFrozen;
	return true;
}

//******************************************************************************
void CDbCommands::UnpackBuffer(
//Unpacks command list from a buffer previously packed by GetPackedBuffer().
//
//Params:
	const BYTE *pBuf) //(in)   Packed buffer to unpack into this object.
{
	const BYTE *pSeek = pBuf;
	while (pSeek[0] != 0)
	{
		Add(pSeek[0], pSeek[1]);
		if (bIsComplexCommand(pSeek[0]))
		{
			AddData(pSeek[2], pSeek[3]);
			pSeek += 2;
		}
		pSeek += 2;
	}
	this->commandIter = end();
}

// $Id: DbCommands.cpp 9347 2009-03-22 15:01:01Z mrimer $

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
		if (bIsComplexCommand(comIter->command))
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
		if (bIsComplexCommand(comIter->command))
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
	const UINT wCommand,              //(in)   One of the CMD_* constants.
	const UINT msElapsedSinceLast) //(in)   Time elapsed since last command (ms)
									//    If 0 (default), the time 
									//    between this call and last call to Add()
									//    will be used.
{
	//If object is frozen, then modifying list not allowed.
	if (this->bIsFrozen)
	{
		ASSERT(!"CDbCommands::Add: Object is frozen.");
		return;
	}

/*
	ASSERT(wCommand < COMMAND_COUNT);
	if (wCommand >= COMMAND_COUNT)
		return; //don't allow adding invalid commands
*/

	//Get time elapsed since last, if not specified.
	UINT elapsed;
	if (msElapsedSinceLast > 0)
	{
		elapsed = msElapsedSinceLast;
	} else {
		if (this->dwTimeOfLastAdd == 0)
			elapsed = 0; //First call to Add, so elapsed time does not apply.
		else
		{
			//Get # of milliseconds between calls.
			elapsed = GetTicks() - dwTimeOfLastAdd;
		}
	}

	//Create new command and set members.
	this->commands.push_back(COMMANDNODE(wCommand, elapsed));

	ResetTimeOfLastAdd();
}

//******************************************************************************
void CDbCommands::AddData(const UINT wX, const UINT wY)
//Adds a command item to the list that stores special data.
//This should be called only after adding a command that requires extra data.
{
	//The last command should be one that requires a data member to be added.
	//Currently, the following command is the only one that requires it.
	//Add others as needed.
	ASSERT(bIsComplexCommand(this->commands.back().command));

	//If object is frozen, then modifying list not allowed.
	if (this->bIsFrozen)
	{
		ASSERT(!"CDbCommands::AddData: Object is frozen.");
		return;
	}
		
	this->commands.push_back(COMMANDNODE(wX, wY));
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
	PackedBuf += static_cast<UINT>(-1); //version marker

	//Each iteration packs one command record into buffer.
	for (vector<COMMANDNODE>::const_iterator comIter = begin();
			comIter != end(); ++comIter)
	{
		//Serialize command.
		serializeUINT(PackedBuf, comIter->command);

		//Serialize time.
		serializeUINT(PackedBuf, comIter->msElapsedSinceLast);
	}

	//Append UINT-0 end code to buffer.
	PackedBuf += UINT(0);
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
		if (bIsComplexCommand(this->commandIter->command))
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
			next += (bIsComplexCommand(next->command) ? 2 : 1))
		iter = next;

	return this->commandIter = iter;
}

//******************************************************************************
CDbCommands::const_iterator CDbCommands::Get(
//Gets a command by zero-based index.  A subsequent call to GetNext() would
//then return the next command.
//
//Params:
	UINT dwIndex) //(in)   Index of command to get.
//
//Returns:
//Command.
{
	ASSERTP(dwIndex < Count(), "Bad index param.");

	this->commandIter = begin();
	while (dwIndex--)
	{
		//Skip extra data fields record for complex commands.
		if (bIsComplexCommand(this->commandIter->command))
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
		if (bIsComplexCommand(comIter->command))
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
	if (!bIsComplexCommand(this->commandIter->command))
		return false;
	vector<COMMANDNODE>::const_iterator data = this->commandIter + 1;
	if (data == end())
		return false;

	wX = data->command;
	wY = data->msElapsedSinceLast;
	return true;
}

//*****************************************************************************
UINT CDbCommands::GetTimeElapsed() const
//Returns: the time duration covered by these commands (ms)
{
	UINT dwTimeElapsed = 0;

	for (vector<COMMANDNODE>::const_iterator comIter = begin();
			comIter != end(); ++comIter)
	{
		dwTimeElapsed += comIter->msElapsedSinceLast;
		if (bIsComplexCommand(comIter->command))
			++comIter;	//skip extra fields
	}

	return dwTimeElapsed;
}

//*****************************************************************************
void CDbCommands::Replace(
//Replaces commands within dwStart<=index<dwStop with a single new command.
//Afterward, GetCurrent() will return the new single command.
//
//Params:
	UINT dwStart,        // (in) index of first replaced command
	UINT dwStop,         // (in) index just after last replaced command
	const int nCommand,  // (in) command to replace with
	const UINT wX,     // (in) data for complex command
	const UINT wY)     // (in) data for complex command
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
		if (bIsComplexCommand(iter->command))
			++iter;
		++iter;
		++dwIndex;
	}
	CommandSequence::iterator startIter = iter;

	UINT dwTotalElapsed = 0;
	while (dwIndex < dwStop)
	{
		dwTotalElapsed += iter->msElapsedSinceLast;
		if (bIsComplexCommand(iter->command))
			++iter;
		++iter;
		++dwIndex;
	}
	CommandSequence::iterator stopIter = iter;

	iter = startIter;
	iter->command = nCommand;
	iter->msElapsedSinceLast = dwTotalElapsed;
	++iter;
	if (bIsComplexCommand(nCommand))
	{
		*iter = COMMANDNODE(wX, wY);
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

	UINT wCommandBytes = 1; //default: version 1: 1 byte for each command

	//Version 2 check:
	//Look for a -1 UINT.
	if (pSeek[0] && pSeek[1] && pSeek[2] && pSeek[3])
	{
		UINT val = deserializeUINT(pSeek);
		if (val == static_cast<UINT>(-1))
		{
			//Version 2: command param is 4 bytes, not 1
			wCommandBytes = 4;

			pSeek += 4; //advance past version header
		}
	}

	//Either a command is comprised of one byte, or four.
	//Whenever a NULL value of the appropriate type is found, we're done.
	const bool bUINTCommand = wCommandBytes == 4;
	while (pSeek[0] || (bUINTCommand && (pSeek[1] || pSeek[2] || pSeek[3])))
	{
		//Get command type.
		UINT command;
		if (bUINTCommand)
		{
			command = deserializeUINT(pSeek);
		} else {
			command = pSeek[0];
		}
		pSeek += wCommandBytes;

		//Get param value.  Deserialize UINT.
		UINT val = deserializeUINT(pSeek);
		pSeek += 4;

		Add(command, val);

		if (bIsComplexCommand(command))
		{
			//We have an additional data record to deserialize for this command.

			//Get param1.
			if (bUINTCommand)
			{
				command = deserializeUINT(pSeek);
			} else {
				command = pSeek[0];
			}
			pSeek += wCommandBytes;

			//Get param2.
			val = deserializeUINT(pSeek);
			pSeek += 4;

			AddData(command, val);
		}
	}
	this->commandIter = end();
}

//*****************************************************************************
UINT CDbCommands::deserializeUINT(const BYTE* buffer)
//Deserialize 4 bytes --> UINT
{
	UINT val=buffer[0];
	val *= 256;
	val += buffer[1];
	val *= 256;
	val += buffer[2];
	val *= 256;
	val += buffer[3];
	return val;
}

//*****************************************************************************
void CDbCommands::serializeUINT(
	CStretchyBuffer& buffer, //(in/out)
	UINT val) //(in)
//Serialize UINT --> 4 bytes.
//Add them to the buffer
{
	BYTE b4 = val % 256;
	val /= 256;
	BYTE b3 = val % 256;
	val /= 256;
	BYTE b2 = val % 256;
	val /= 256;
	BYTE b1 = val;

	buffer += b1;
	buffer += b2;
	buffer += b3;
	buffer += b4;
}

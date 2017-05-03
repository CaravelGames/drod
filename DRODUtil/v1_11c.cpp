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
 * Michael Welsh Duggan (md5i), JP Burford (jpburford), Rik Cookney (timeracer)
 *
 * ***** END LICENSE BLOCK ***** */

//v1_11.cpp
//This was originally cut-and-pasted from DRODUtil.cpp, and contains some contributions outside
//of those contained in erikh2000's commits.  See history of DRODUtil.cpp if interested.

#ifdef WIN32 //Many things will not compile w/o WIN32 API.  Fix them if you are porting.
#  include <windows.h> //Should be first include.
#endif

#include <BackEndLib/StretchyBuffer.h>

#include "v1_11c.h"
#include "Assert.h"
#include "OptionList.h"

#include <stdio.h>
#ifdef WIN32
#include <io.h>
#define SEEK_SET 0
#else
#include <unistd.h>
#define HFILE int
#define HFILE_ERROR -1
#define _open open
#define _close close
#define _read read
#define _lseek lseek
#define _O_RDONLY O_RDONLY
//always binary in posix
#define _O_BINARY 0
#endif
#include <fcntl.h>

//Private function prototypes.
static long    DoesRoomExist(HFILE hfile, UINT wRooms, UINT wHeaderSize, int xRoom,
		int yRoom);
static long    DoesRoomExist(HFILE hfile, DFILEHEADER99 *lpHeader, int xRoom, int yRoom);
static long    DoesRoomExist(HFILE hfile, DFILEHEADER *lpHeader, int xRoom, int yRoom);
static bool    GetRoom(HFILE hfile, long lPos, ROOM *lpRoom);
static bool    GetRoom(HFILE hfile, DFILEHEADER *lpHeader, int xRoom, int yRoom, 
		ROOM *lpRoom);
static bool    GetRoom(HFILE hfile, DFILEHEADER99 *lpHeader, int xRoom, int yRoom, 
		ROOM *lpRoom);
static bool    GetSquare(const char *lpszDunPath, int xRoom, int yRoom, int x, 
		int y, unsigned char *pOSquare, unsigned char *pTSquare);

//
//Public functions.
//

//*****************************************************************************
bool GetDemoFile(
//Load DMO file into DEMOFILE struct.
//
//Params:
	const WCHAR* pszDMOFilepath, //(in) Full path to DMO file.
	DEMOFILE &df,           //(out)  Receives demo data.
	UINT &wCommandCount,    //(out) Receives number of elements in parrCommands.  
	DEMOCMD * &parrCommands)   //(out)  Receives pointer to array of demo commands
								//    which caller must delete [].

//
//Returns:
//True if successful, false if not.
{
	//Open the file.
#ifdef HAS_UNICODE
	HFILE hfileDMO = _wopen(pszDMOFilepath, _O_RDONLY | _O_BINARY);
#else
	WSTRING wstrDMOFilepath = pszDMOFilepath;
	const string szDMOFilepath = UnicodeToAscii(wstrDMOFilepath);
	HFILE hfileDMO = open(szDMOFilepath.c_str(), O_RDONLY);
#endif
	if (hfileDMO == HFILE_ERROR)
		return false;

	//Alloc buffer to hold commands.
	bool bSuccess = true;

	//Read header data.
	if (bSuccess)
	{
		bSuccess &= (_read(hfileDMO, &df, sizeof(df)) != HFILE_ERROR);

		//Look for expected "DROD_DEMO" at beginning.
		bSuccess &= (strcmp(df.szCode, DEMOFILE_CODE)==0);
	}

	//Read commands.
	CStretchyBuffer CommandsBuf;
	wCommandCount = 0;
	DEMOCMD OneCommand;
	//2 byte commands are needed because that's how data is stored in file.
	ASSERT(sizeof(OneCommand)==2);
	if (bSuccess)
	{
		while (_read(hfileDMO, &OneCommand, sizeof(OneCommand)) == sizeof(OneCommand))
		{
			if (OneCommand.bit4Command == 14) break; //14 == End demo code.
			if (OneCommand.bitTrigger != 0)
			{
				//The trigger bit was never used in DROD1.11c and should always be set to
				//zero.  It makes a good sentry to check for corruption or errors caused
				//by struct alignment differences.
				printf("FAILED--Unexpected data found in DMO file.  Possible corruption." NEWLINE);
				bSuccess = false;
				break;
			}
			CommandsBuf.Append(reinterpret_cast<const BYTE *>(&OneCommand), sizeof(OneCommand));
			++wCommandCount;
		}
	}
	parrCommands = reinterpret_cast<DEMOCMD *>(CommandsBuf.GetCopy());

	//Close file.
	if (_close(hfileDMO) == HFILE_ERROR) ASSERTP(false, "Error closing file.");

	return bSuccess;
}

//*****************************************************************************
bool IsEmptyRoom(
//Checks if a room is one of the empty rooms incorrectly stored in DROD.DUN.
//Rooms are recognized by a certain pattern of o-squares containing just a 
//perimeter wall with no exits.
//
//Params:
	const char *pszOSquares) //(in)  O-squares for the room.
//
//Returns:
//True if room is an empty, false if not.
{
	//Check north row for exits.
	UINT x;
	for (x = 1; x < DISPLAY_COLS - 1; ++x)
	{
		if (pszOSquares[GetSquareI(x, 0)] != T111c_WALL_S)
			return false;
	}

	//Check south row for exits.
	for (x = 0; x < DISPLAY_COLS; ++x)
	{
		if (pszOSquares[GetSquareI(x, DISPLAY_ROWS - 1)] != T111c_WALL)
			return false;
	}

	//Check west and east columns for exits.
	for (UINT y = 0; y < DISPLAY_ROWS - 1; ++y)
	{
		if (pszOSquares[GetSquareI(0, y)] != T111c_WALL ||
				pszOSquares[GetSquareI(DISPLAY_COLS - 1, y)] != T111c_WALL)
			return false;
	}

	//Yeah, it's one of those.
	return true;
}

//**************************************************************************************
bool IsMonsterValidForOriginalRoom(
//Is this a monster that would be valid inside of the original room it is located within.
//
//Params:
	const MONSTER *pMonster)   //(in) Monster to evaluate.
//
//Returns:
//True if it is, false if not.
{
	//There is no reason for a dead monster to be in an original room.
	if (!pMonster->bAlive) return false;

	//Check for valid square coords.
	if (pMonster->x < 0 || pMonster->x > DISPLAY_COLS) return false;
	if (pMonster->y < 0 || pMonster->y > DISPLAY_ROWS) return false;

	//Check for valid monster type.
	if (pMonster->nType < 0 || pMonster->nType > MONSTER_TYPES_111c) return false;

	return true;
}

//*****************************************************************************
UINT GetSquareI(
//Gets index of square within osquares or tsquares array.
//
//Params:
	UINT x, UINT y)   //(in)   0-based coords of square.
//
//Returns:
//0-based index.
{
	ASSERT(x < DISPLAY_COLS);
	ASSERT(y < DISPLAY_ROWS);
	return (y * DISPLAY_COLS) + x;
}

//*************************************************************************************
UINT TileOrientation(
//Gets the orientation of a tile.
	UINT tile)                     //(in) Tile to check
//
//Returns:
//Orientation of the tile
{
	switch (tile) 
	{
	case T111c_SMAN_IYN: case T111c_SNK_N: case T111c_SWORD_IYN: case T111c_SMAN_YN:
	case T111c_SNK_AN: case T111c_SWORD_YN: case T111c_NTHR_N: case T111c_EYE_N:
	case T111c_ROACH_AN: case T111c_EYE_AN: case T111c_ROACH_N: case T111c_QROACH_AN:
	case T111c_QROACH_N: case T111c_GOBLIN_AN: case T111c_GOBLIN_N: case T111c_WW_N:
	case T111c_ARROW_N: case T111c_SWORD_BN: case T111c_WW_AN: case T111c_SMAN_BN:
	case T111c_TARBABY: case T111c_TARBABY_A:
		return O_N;
	case T111c_SMAN_IYNE: case T111c_SWORD_IYNE: case T111c_SMAN_YNE:
	case T111c_SWORD_YNE: case T111c_NTHR_NE: case T111c_EYE_NE:
	case T111c_ROACH_ANE: case T111c_EYE_ANE: case T111c_ROACH_NE: case T111c_QROACH_ANE:
	case T111c_QROACH_NE: case T111c_GOBLIN_ANE: case T111c_GOBLIN_NE: case T111c_WW_NE:
	case T111c_ARROW_NE: case T111c_SWORD_BNE: case T111c_WW_ANE: case T111c_SMAN_BNE:
		return O_NE;
	case T111c_SMAN_IYE: case T111c_SNK_E: case T111c_SWORD_IYE: case T111c_SMAN_YE:
	case T111c_SNK_AE: case T111c_SWORD_YE: case T111c_NTHR_E: case T111c_EYE_E:
	case T111c_ROACH_AE: case T111c_EYE_AE: case T111c_ROACH_E: case T111c_QROACH_AE:
	case T111c_QROACH_E: case T111c_GOBLIN_AE: case T111c_GOBLIN_E: case T111c_WW_E:
	case T111c_ARROW_E: case T111c_SWORD_BE: case T111c_WW_AE: case T111c_SMAN_BE:
		return O_E;
	case T111c_SMAN_IYSE: case T111c_SWORD_IYSE: case T111c_SMAN_YSE:
	case T111c_SWORD_YSE: case T111c_NTHR_SE: case T111c_EYE_SE:
	case T111c_ROACH_ASE: case T111c_EYE_ASE: case T111c_ROACH_SE: case T111c_QROACH_ASE:
	case T111c_QROACH_SE: case T111c_GOBLIN_ASE: case T111c_GOBLIN_SE: case T111c_WW_SE:
	case T111c_ARROW_SE: case T111c_SWORD_BSE: case T111c_WW_ASE: case T111c_SMAN_BSE:
		return O_SE;
	case T111c_SMAN_IYS: case T111c_SNK_S: case T111c_SWORD_IYS: case T111c_SMAN_YS:
	case T111c_SNK_AS: case T111c_SWORD_YS: case T111c_NTHR_S: case T111c_EYE_S:
	case T111c_ROACH_AS: case T111c_EYE_AS: case T111c_ROACH_S: case T111c_QROACH_AS:
	case T111c_QROACH_S: case T111c_GOBLIN_AS: case T111c_GOBLIN_S: case T111c_WW_S:
	case T111c_ARROW_S: case T111c_SWORD_BS: case T111c_WW_AS: case T111c_SMAN_BS:
		return O_S;
	case T111c_SMAN_IYSW: case T111c_SWORD_IYSW: case T111c_SMAN_YSW:
	case T111c_SWORD_YSW: case T111c_NTHR_SW: case T111c_EYE_SW:
	case T111c_ROACH_ASW: case T111c_EYE_ASW: case T111c_ROACH_SW: case T111c_QROACH_ASW:
	case T111c_QROACH_SW: case T111c_GOBLIN_ASW: case T111c_GOBLIN_SW: case T111c_WW_SW:
	case T111c_ARROW_SW: case T111c_SWORD_BSW: case T111c_WW_ASW: case T111c_SMAN_BSW:
		return O_SW;
	case T111c_SMAN_IYW: case T111c_SNK_W: case T111c_SWORD_IYW: case T111c_SMAN_YW:
	case T111c_SNK_AW: case T111c_SWORD_YW: case T111c_NTHR_W: case T111c_EYE_W:
	case T111c_ROACH_AW: case T111c_EYE_AW: case T111c_ROACH_W: case T111c_QROACH_AW:
	case T111c_QROACH_W: case T111c_GOBLIN_AW: case T111c_GOBLIN_W: case T111c_WW_W:
	case T111c_ARROW_W: case T111c_SWORD_BW: case T111c_WW_AW: case T111c_SMAN_BW:
		return O_W;
	case T111c_SMAN_IYNW: case T111c_SWORD_IYNW: case T111c_SMAN_YNW:
	case T111c_SWORD_YNW: case T111c_NTHR_NW: case T111c_EYE_NW:
	case T111c_ROACH_ANW: case T111c_EYE_ANW: case T111c_ROACH_NW: case T111c_QROACH_ANW:
	case T111c_QROACH_NW: case T111c_GOBLIN_ANW: case T111c_GOBLIN_NW: case T111c_WW_NW:
	case T111c_ARROW_NW: case T111c_SWORD_BNW: case T111c_WW_ANW: case T111c_SMAN_BNW:
		return O_NW;
	}
	return O_NO_ORIENTATION;
}

//**************************************************************************************
UINT GetOrbAgentCount(
//Gets number of orb agents in orb agent array by finding the end array element.
//
//Params:
	const ORB *pOrb) //(in)
//
//Returns:
//Number of orbs.
{
	UINT wOrbAgentI;

	for (wOrbAgentI = 0; wOrbAgentI < MAX_ORB_AGENTS; wOrbAgentI++)
	{
		if (pOrb->nAgentType[wOrbAgentI] == OA_NULL) break;
	}
	return wOrbAgentI;
}

//**************************************************************************************
void MoveOrbAgentIn111cRoom(
//Moves an orb agent to a different location in a 1.11c room struct.
	const UINT wOrb,     //(in)      Indicates orb whose agent will be modified.
	const UINT wAgent,   //(in)      Which orb agent will being modified.
	const UINT wX, const UINT wY, //(in)      Room position to move orb agent to.
	ROOM &Room)          //(in/out)  Accepts room containing specified orb.  
{
	ASSERT(wOrb < MAX_ORBS);
	ASSERT(wAgent < MAX_ORB_AGENTS);

	Room.stOrb[wOrb].xAgent[wAgent] = wX;
	Room.stOrb[wOrb].yAgent[wAgent] = wY;
}

//**************************************************************************************
void RemoveOrbAgentIn111cRoom(
//Removes an orb agent in a 1.11c room struct.
	const UINT wOrb,  //(in)      Indicates orb whose agent will be modified.
	const UINT wAgent,   //(in)      Which orb agent will being deleted.
	ROOM &Room)          //(in/out)  Accepts room containing specified orb.
{
	ASSERT(wOrb < MAX_ORBS);
	ASSERT(wAgent < MAX_ORB_AGENTS);

	Room.stOrb[wOrb].nAgentType[wAgent] = OA_NULL;
	Room.stOrb[wOrb].xAgent[wAgent] = Room.stOrb[wOrb].yAgent[wAgent] = 0;
}

//**************************************************************************************
void RemoveOrbIn111cRoom(
//Removes an orb (and any agents) in a 1.11c room struct.
	const UINT wOrb,  //(in)      Indicates orb whose agent will be modified.
	ROOM &Room)          //(in/out)  Accepts room containing specified orb.  
{
	ASSERT(wOrb < MAX_ORBS);

	//RemoveOrbAgents
	for (UINT wAgentI = 0; wAgentI < MAX_ORB_AGENTS; ++wAgentI)
	{
		Room.stOrb[wOrb].nAgentType[wAgentI] = OA_NULL;
		Room.stOrb[wOrb].xAgent[wAgentI] = Room.stOrb[wOrb].yAgent[wAgentI] = 0;
	}
	
	//remove orb
	Room.stOrb[wOrb].x = Room.stOrb[wOrb].y = 0;
}

//**************************************************************************************
void AddOrbAgentTo111cRoom(
//Adds an orb agent to an existing orb in a 1.11c room struct.
//
//Params:
	UINT wOrbCol, UINT wOrbRow,      //(in)      Indicates orb to which agent will be added.
	int nAgentType,               //(in)      Type of agent--OA_TOGGLE, OA_OPEN, or OA_CLOSE.
	UINT wAgentCol, UINT wAgentRow,  //(in)      Door square that agent spreads from when 
									//       orb is activated.
	ROOM &Room)                //(in/out)  Accepts room containing specified orb.  
									//       Returns with orb agent added to orb.
{
  //Find orb.
  UINT wOrbI;
  for (wOrbI = 0; Room.stOrb[wOrbI].nAgentType[0] != OA_NULL; ++wOrbI)
  {
	 ASSERT(wOrbI < MAX_ORBS);
	 if (Room.stOrb[wOrbI].x == (int)wOrbCol && Room.stOrb[wOrbI].y == (int)wOrbRow)
		break; //Found the orb.
  }
  if (Room.stOrb[wOrbI].nAgentType[0] == OA_NULL)
	 {ASSERTP(false, "Didn't find the orb."); return;} 

	//Find first open element of agent array.
	UINT wAgentI;
	for (wAgentI = 1; wAgentI < MAX_ORB_AGENTS; ++wAgentI)
	{
		if (Room.stOrb[wOrbI].nAgentType[wAgentI] == OA_NULL) break;
	}
	if (wAgentI == MAX_ORB_AGENTS)
		{ASSERTP(false, "This orb already has max# of agents.  No more can be added."); return;}

  //Add the agent.
  Room.stOrb[wOrbI].nAgentType[wAgentI] = nAgentType;
  Room.stOrb[wOrbI].xAgent[wAgentI] = wAgentCol;
  Room.stOrb[wOrbI].yAgent[wAgentI] = wAgentRow;
}

//
//Private functions.
//

//**************************************************************************************
static bool GetSquare(
//Gets contents of a square in a specified location, room, and dungeon file (9 or 99-level).
//
//Accepts:
	const char *lpszDunPath,      //Full path to dungeon file.
	int xRoom, int yRoom,     //Room to look in.
	int x, int y, //Square to get.
//
//Returns by parameter:
	unsigned char *pOSquare,
	unsigned char *pTSquare)
//
//Returns:
//true if successful, false otherwise.
{
	HFILE hfileDun=HFILE_ERROR;
	bool bIs99Format;
	UINT wRoomIndex;

	//Allocations.
	ROOM *lpRoom=new ROOM;
	DFILEHEADER99 *lpHeader=new DFILEHEADER99;
	bool bSuc=(lpRoom && lpHeader);

	//Open the dungeon file.
	if (bSuc) {bSuc=((hfileDun=_open(lpszDunPath, _O_RDONLY | _O_BINARY))!=HFILE_ERROR); ASSERT(bSuc);}

	//Figure out what format it is and load the header.
	if (bSuc) {bSuc=(_read(hfileDun, lpHeader, 13 )!=HFILE_ERROR); ASSERT(bSuc);}
	if (bSuc)
	{
		if (strcmp(lpHeader->szHeaderCode, DFILE_HEADER_CODE)==0) 
			bIs99Format=false;
		else if (strcmp(lpHeader->szHeaderCode, DFILE_HEADER_CODE99)==0) 
			bIs99Format=true;
		else 
			{bSuc=false; ASSERTP(false, "Bad dun format.");}
	}
	if (bSuc)
	{
		_lseek(hfileDun, 0, SEEK_SET);
		if (bIs99Format)
			{bSuc=(_read(hfileDun, lpHeader, sizeof(DFILEHEADER99))!=HFILE_ERROR);}
		else
			{bSuc=(_read(hfileDun, lpHeader, sizeof(DFILEHEADER))!=HFILE_ERROR);}
		ASSERT(bSuc);
	}
	
	//Load room.
	if (bSuc)
	{
		if (bIs99Format)
			{bSuc=GetRoom(hfileDun, lpHeader, xRoom, yRoom, lpRoom); ASSERT(bSuc);}
		else //Cast to DFILEHEADER will cause 9-level format GetRoom() to be called.
			{bSuc=GetRoom(hfileDun, (DFILEHEADER *) lpHeader, xRoom, yRoom, lpRoom); ASSERT(bSuc);}
	}
	
	//Get square in room.
	if (bSuc)
	{
		wRoomIndex=((DISPLAY_COLS*y)+x);
		*pOSquare=lpRoom->szOSquares[wRoomIndex];
		*pTSquare=lpRoom->szTSquares[wRoomIndex];
	}
	
	//Cleanup.  
	if (hfileDun) 
	{
		if (_close(hfileDun)==HFILE_ERROR) ASSERTP(false, "Error closing file.");
	}
	if (lpHeader) delete lpHeader;
	if (lpRoom) delete lpRoom;
	
	return bSuc;
}

//*********************************************************************
static bool GetRoom(
//Gets a room from a dungeon file.
//
//Accepts:
	HFILE hfile,   //Handle to open 9 or 99-level dungeon file.
	long lPos,      //Position in file to find room at--passed by one of the overrides below.
//
//Returns by parameter:
	ROOM *lpRoom)  //Preallocated.
//
//Returns:
//true if successful, false otherwise.
{
	if (_lseek(hfile, lPos, 0)==HFILE_ERROR) {ASSERTP(false, "Seek failed."); return false;}
	if (_read(hfile, lpRoom, sizeof(ROOM))==HFILE_ERROR) {ASSERTP(false, "Read failed."); return false;}
	
	return true;   
}
//Overrides for two different file formats.
static bool GetRoom(HFILE hfile, DFILEHEADER *lpHeader, int xRoom, int yRoom, ROOM *lpRoom)
{
	long lPos=DoesRoomExist(hfile, lpHeader, xRoom, yRoom);
	if (lPos==0) {ASSERTP(false, "Room does not exist."); return false;}
	return GetRoom(hfile, lPos, lpRoom);
}
static bool GetRoom(HFILE hfile, DFILEHEADER99 *lpHeader, int xRoom, int yRoom, ROOM *lpRoom)
{
	long lPos=DoesRoomExist(hfile, lpHeader, xRoom, yRoom);
	if (lPos==0) return false;
	return GetRoom(hfile, lPos, lpRoom);
}

//**************************************************************************
static long DoesRoomExist(
//Determines if a specified room exists in a dungeon file.  Uses the slow method of checking each
//room record as opposed to using the contents index file.
//
//Accepts:
	HFILE hfile, //Handle to open dungeon file.
	UINT wRooms, //# of rooms in the file.
	UINT wHeaderSize,   //Size of the file header.
	int xRoom, int yRoom)  //Room to check.
//
//Returns:
//Position of room in the file or 0 if the room doesn't exist.
{
	UINT wRoomI;
	long lFilePos;
	SHORT xThisRoom, yThisRoom;
	ROOM *lpRoom=new ROOM;
	ASSERT(lpRoom);
		
	//Seek to the first room.
	if (_lseek(hfile, wHeaderSize, 0)==HFILE_ERROR) ASSERTP(false, "Seek failed.");
			
	//For every room in the file...
	for (wRoomI=0; wRoomI<wRooms; wRoomI++)
		{
			lFilePos=(long) wHeaderSize+(long) ((long)wRoomI*(long) sizeof(ROOM));
			if (_read(hfile, lpRoom, sizeof(ROOM))==HFILE_ERROR) ASSERTP(false, "Read failed.");
			xThisRoom=lpRoom->xRoom;       
			yThisRoom=lpRoom->yRoom;
			
			ASSERT((xThisRoom>-50 && xThisRoom<10000 && yThisRoom>-50 && yThisRoom<10000) ||
									(xThisRoom==-9999 && yThisRoom==-9999));
			if ((SHORT) xRoom==xThisRoom && (SHORT) yRoom==yThisRoom) {delete lpRoom; return lFilePos;}
		}
	
	delete lpRoom;
	
	//Didn't find it.
	return 0;
}
//Overrides for two different file formats.
static long DoesRoomExist(HFILE hfile, DFILEHEADER99 *lpHeader, int xRoom, int yRoom)
{return DoesRoomExist(hfile, (UINT) lpHeader->nRooms, sizeof(DFILEHEADER99), xRoom, yRoom);}
static long DoesRoomExist(HFILE hfile, DFILEHEADER *lpHeader, int xRoom, int yRoom)
{return DoesRoomExist(hfile, (UINT) lpHeader->nRooms, sizeof(DFILEHEADER), xRoom, yRoom);}

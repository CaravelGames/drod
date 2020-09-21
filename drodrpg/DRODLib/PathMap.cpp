// $Id: PathMap.cpp 8102 2007-08-15 14:55:40Z trick $

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
 * Michael Welsh Duggan (md5i)
 * Mike Rimer (mrimer)
 *
 * ***** END LICENSE BLOCK ***** */

#include "Pathmap.h"
#include <BackEndLib/Coord.h>
#include <BackEndLib/Ports.h>

#include <utility>      //std::swap

#ifdef _DEBUG
//#define DEBUG_PATHMAP //Comment out to get rid of pathmap debug outputs.

#  ifdef DEBUG_PATHMAP
#     include <stdlib.h>
#     include <stdio.h>
#  endif

#endif

const UINT dwBigVal = UINT(-1); //this value indicates a tile is not part of the valid pathmap

//**********************************************************************************
CPathMap::CPathMap(
//Constructor.  Sets object vars to default values and allocates and initializes the map squares and immediate
//recalc arrays.
//
//Accepts:
	const UINT wCols, const UINT wRows, //Size to initialize map to.
	const UINT xTarget, const UINT yTarget,   //[default: (-1,-1) = none]
	const UINT dwPathThroughObstacleCost) //[default=0]
	: wCols(wCols)
	, wRows(wRows)
	, xTarget(xTarget), yTarget(yTarget)
	, dwPathThroughObstacleCost(dwPathThroughObstacleCost)
{
	//Allocate map.  Initialize map squares.
	const UINT wArea=wCols*wRows;
	this->squares.resize(wArea);
	UINT wSquareI;
	for (wSquareI=wArea; wSquareI--; )
		this->squares[wSquareI].eState=ok;

	//Get ready for path calculation if target is specified.
	if (xTarget < wCols && yTarget < wRows)
		Reset();
}

//**********************************************************************************
void CPathMap::CalcPaths()
//Calculates paths to a specified target.  If the target and map squares are
//unchanged and there is still a previous call did not calculate paths for all
//squares, this work is continued.
{
	if (this->recalcSquares.empty())
		return;

	int dx, dy, wNewX, wNewY;
	do {
		//Each iteration expands one recalc square to its eligible adjacent neighbors (breadth-first search).
		SORTPOINT coord = this->recalcSquares.top();
		this->recalcSquares.pop();

		//Check every adjacent square for recalc eligibility.
		static const UINT wNumNeighbors = 8;
		static const int dxDir[wNumNeighbors] = {-1,  0,  1, -1,  1, -1,  0,  1};
		static const int dyDir[wNumNeighbors] = {-1, -1, -1,  0,  0,  1,  1,  1};
		for (UINT nIndex=0; nIndex<wNumNeighbors; ++nIndex)
		{
			wNewX = coord.wX + (dx = dxDir[nIndex]);
			wNewY = coord.wY + (dy = dyDir[nIndex]);

			if ((UINT)wNewX >= this->wCols || (UINT)wNewY >= this->wRows) continue;

			SQUARE& square = this->squares[GetSquareIndex(wNewX,wNewY)];

			//If this square is considered a valid candidate...
			if (square.eState != obstacle || this->dwPathThroughObstacleCost)
			{
				//Calculate total distance score to this square (along path from current square).
				UINT dwScore = coord.dwScore + 1;  //one more step has been taken
				if (square.eState == obstacle)
				{
					//use (probably) higher cost for considering a path through an invalid region
					if (this->squares[GetSquareIndex(coord.wX, coord.wY)].eState == obstacle)
						dwScore += this->dwPathThroughObstacleCost;
					else
						//This line alters the search on entering an obstacle
						//connected component.  The practical effect is that only
						//valid movement inside one's connected component is considered.
						//In other words, the search may begin in an obstacle,
						//then leave it, but on re-entering an obstacle, the path cost
						//will be set as though it never left the obstacle.
						dwScore = (coord.wMoves+1) * (this->dwPathThroughObstacleCost + 1);
				}
				if (dwScore < square.dwTargetDist)
				{
					//This is the new best-cost path to this square.
					square.dwTargetDist = dwScore;
					square.dwSteps = coord.wMoves+1;
					this->recalcSquares.push(SORTPOINT(wNewX, wNewY, dwScore, square.dwSteps));
				}
			}
		}
	} while (!this->recalcSquares.empty());

	//Done with calculating paths.
#ifdef DEBUG_PATHMAP
	{
		string strOutput = "---Complete Pathmap---" NEWLINE;
		GetDebugOutput(true,false,false,strOutput);
		strOutput += NEWLINE;
		GetDebugOutput(false,true,false,strOutput);
		DEBUGPRINT(strOutput.begin());
	}
#endif // DEBUG_PATHMAP
}

//**********************************************************************************
void CPathMap::GetRecPaths(
//Gets recommended paths to take from a specified square in order to get to the target.
//
//Accepts:
	const UINT wX, const UINT wY, //Square to request paths from.
//
//Returns by parameter:
	SORTPOINTS& sortPoints)  //(out) Info for adjacent squares given in order of recommendation.
{
	//If distances to target are equal,
	//then prefer moving axially first, then staying still, then moving diagonally (1.6 compat).
	static const UINT DIRECTIONS = 9;
	static const int I_COLADJUST[DIRECTIONS] = {0, -1, 1, 0, 0, -1, 1, -1, 1};	//x
	static const int I_ROWADJUST[DIRECTIONS] = {-1, 0, 0, 1, 0, -1, -1, 1, 1};	//y

	//Ensure path map is current.
	CalcPaths();

	//Put valid squares to move to into an array.
	const UINT dwDefaultScore = this->squares[GetSquareIndex(wX,wY)].dwTargetDist;
	ASSERT(sortPoints.empty());
	for (UINT n=0; n<DIRECTIONS; ++n)
	{
		const UINT yTo = wY+I_ROWADJUST[n];
		if (yTo<this->wRows)
		{
			const UINT xTo = wX+I_COLADJUST[n];
			if (xTo<this->wCols)
			{
				const UINT dwScore = this->squares[GetSquareIndex(xTo,yTo)].dwTargetDist;
				if (dwScore <= dwDefaultScore && dwScore < dwBigVal)
				{
					//Add the square.
               sortPoints.push_back(SORTPOINT(xTo, yTo, dwScore, 0));
				}
			}
		}
	}

	//Sort the array of sort points by score.
	StableSortPoints(sortPoints);
}

//**********************************************************************************
const SQUARE& CPathMap::GetSquare(
//Gets a specified map square.  Object functions access square array directly.
//
//Accepts:
	const UINT wX, const UINT wY)
{
	//Ensure path map is current.
	CalcPaths();

	return this->squares[GetSquareIndex(wX,wY)];
}

//***************************************************************************
void CPathMap::Reset()
//Sets all the map squares so that they need recalculation.
//The recalculation only occurs when the pathmap is next queried.
//
//Changes:
//this->squares
{
	const UINT wLastSquareI=this->wRows*this->wCols;
	
	//Initialize all of the map squares.
	for (UINT wSquareI=wLastSquareI; wSquareI--; )
	{
		//Haven't calculated path for this square yet.
		this->squares[wSquareI].dwTargetDist =
			this->squares[wSquareI].dwSteps = dwBigVal;
	}

   while (!this->recalcSquares.empty())
      this->recalcSquares.pop();

	//Wasteful to recalc pathmap if no valid target is specified.
	if (this->xTarget >= this->wCols || this->yTarget >= this->wRows)
		return;

	SQUARE& square = this->squares[GetSquareIndex(this->xTarget, this->yTarget)];
	square.dwTargetDist=square.dwSteps=0;
	this->recalcSquares.push(SORTPOINT(this->xTarget, this->yTarget));
}

//*****************************************************************************
void CPathMap::SetMembers(const CPathMap& Src)
//Perform deep copy.
{
	this->wCols = Src.wCols;
	this->wRows = Src.wRows;
	this->squares = Src.squares;

	this->xTarget = Src.xTarget;
	this->yTarget = Src.yTarget;
	this->recalcSquares = Src.recalcSquares;
	this->dwPathThroughObstacleCost = Src.dwPathThroughObstacleCost;
}

//*****************************************************************************
void CPathMap::SetSquare(
//Intended for calls from outside object.  Sets the state of a specified square
//to obstacle or not an obstacle.  Other states are not allowed.
//
//Accepts:
	const UINT wX, const UINT wY, 
	const bool bIsObstacle) 
//
//Changes:
//this->squares
{
	SQUARE& square = this->squares[GetSquareIndex(wX,wY)];
		
	switch (square.eState)
	{
	case ok:
		if (bIsObstacle)
		{
			square.eState=obstacle;
			Reset();
		}
		break;

	case obstacle:
		if (!bIsObstacle)
		{
			square.eState=ok;
			Reset();
		}
		break;

	default: ASSERTP(false, "Bad square state."); break;
	}
}

//**********************************************************************************
void CPathMap::SetTarget(
//Sets the target and if it differs from the last sets squares for recalculation
//
//Accepts:
	const UINT xTarget, const UINT yTarget)
//
//Changes:
//this->xyTarget
{
	if (xTarget!=this->xTarget || yTarget!=this->yTarget)
	{
		this->xTarget=xTarget;
		this->yTarget=yTarget;
		Reset();
	}
}

//*****************************************************************************
inline UINT CPathMap::GetSquareIndex(const UINT x, const UINT y) const {return y*this->wCols+x;}

//*****************************************************************************
void CPathMap::StableSortPoints(
//Stable (bubble) sort for SORTPOINT structs.  Just as fast as a quick sort
//for 9 elements.  Faster for fewer elements.
//
//Changes:
	SORTPOINTS& sortPoints)
{
	if (sortPoints.size() < 2) return;

	UINT nI;
	for (UINT nJ=sortPoints.size(); --nJ; )
		for (nI=0; nI!=nJ; ++nI)
			if (sortPoints[nI].dwScore > sortPoints[nI+1].dwScore)
				std::swap(sortPoints[nI],sortPoints[nI+1]);
}

//**************************************************************************************
void CPathMap::GetDebugOutput(
//Gets output-formatted representation of pathmap for debugging purposes.
//
//Params:
	bool bShowDirection, //(in)      Show direction attribute of squares?
	bool bShowState,     //(in)      Show state attribute of squares?
	bool bShowDistance,     //(in)      Show distance attribute of squares?
	string &strOutput)      //(in/out)  Appends several CRLF separated lines.
const
{
	ASSERT(bShowDirection || bShowState || bShowDistance);

	//for enum DIRECTION {nw=0, n=1, ne=2, w=3, none=4, e=5, sw=6, s=7, se=8};
	const char szarrDirection[10] = "789456123";

	//for enum STATE {obstacle=0, ok=1};
	const char szarrState[3] = "#.";
	
	//Distance
	//00 to 99, and -- for larger than 99.

	char szChar[2];
	szChar[1] = '\0';
	char szDistance[3];

	//Each iteration concats output for one row of squares.
	for (UINT wRowI = 0; wRowI < this->wRows; ++wRowI)
	{
		//Each iteration concats output for one square.
		for (UINT wColI = 0; wColI < this->wCols; ++wColI)
		{
			const SQUARE& square = this->squares[GetSquareIndex(wColI, wRowI)];

			//Append direction for square.
			if (bShowDirection)
			{
				szChar[0] = szarrDirection[0];	//!!reimplement
				strOutput += szChar;
			}

			//Append state for square.
			if (bShowState)
			{
				szChar[0] = szarrState[square.eState];
				strOutput += szChar;
			}
			
			//Append distance for square.
			if (bShowDistance)
			{
				if (square.dwTargetDist < 100)
				{
					_itoa(square.dwTargetDist, szDistance, 10);
					strOutput += szDistance;
				}
				else
					strOutput += "--";
			}
		}

		//Append end of row CR/LF.
		strOutput += NEWLINE;
	}
}


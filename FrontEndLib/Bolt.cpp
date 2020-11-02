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
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#include "Bolt.h"
#include <BackEndLib/Assert.h>
#include <BackEndLib/Types.h>

#include <SDL.h>
#include <math.h>

#define ABS(x)  ( ((x) < 0) ? -(x) : (x) )

//********************************************************************************
void GenerateBolt(
//Draw an energy bolt from one point to another.
//
//Params:
	int xBegin, int yBegin,    //(in)   Starting point.
	int xEnd, int yEnd,        //(in)   Ending point.
	const UINT DISPLAY_SIZE,   //(in)   Max dimension of screen area (in tiles)
	BOLT_SEGMENTS &drawSegments,   //(out) storage for the segments that can be passed to DrawBolt()
	vector<SDL_Rect>& dirtyRects)  //(out) where blits will occur
{
	static SDL_Rect Long0_180 = {1, 55, 30, 4};
	static SDL_Rect Long22_202 = {40, 5, 28, 14};
	static SDL_Rect Long45_225 = {15, 5, 24, 23};
	static SDL_Rect Long67_247 = {1, 12, 13, 26};
	static SDL_Rect Long90_270 = {71, 22, 4, 29};
	static SDL_Rect Long112_292 = {57, 26, 13, 26};
	static SDL_Rect Long135_315 = {32, 30, 24, 23};
	static SDL_Rect Long157_337 = {3, 40, 28, 14};
	static SDL_Rect Short0_180 = {42, 54, 7, 3};
	static SDL_Rect Short22_202 = {53, 20, 7, 5};
	static SDL_Rect Short45_225 = {45, 20, 7, 6};
	static SDL_Rect Short67_247 = {40, 20, 4, 6};
	static SDL_Rect Short90_270 = {71, 52, 3, 7};
	static SDL_Rect Short112_292 = {66, 53, 4, 6};
	static SDL_Rect Short135_315 = {58, 53, 7, 6};
	static SDL_Rect Short157_337 = {50, 54, 7, 5};
	static SDL_Rect BigSparkle = {61, 22, 3, 3};
	static SDL_Rect SmallSparkle = {65, 23, 2, 2};

	enum ANGLE {A_0 = 0, A_22, A_45, A_67, A_90, A_112, A_135, A_157,
			A_180, A_202, A_225, A_247, A_270, A_292, A_315, A_337, A_Count};
	struct SEGMENT
	{
		SDL_Rect *pSrcRect; //Source rect of image used to draw segment.

		int dxSrc;  //Position inside of segment image that corresponds to bolt 
		int dySrc;  //position before segment is added.
		
		int dxPos;  //Offset to apply to bolt position after segment is added.
		int dyPos;  //
	};

	//These slopes are used to determine closest angle.
	const double SLOPE_11_25 = 0.19891; //Slope of 11.25 degree angle.
	const double SLOPE_33_75 = 0.66818; //Slope of 33.75 degree angle.
	const double SLOPE_56_25 = 1.49661; //Slope of 56.25 degree angle.
	const double SLOPE_78_75 = 5.02737; //Slope of 78.75 degree angle.

	//Data needed to draw long segments and update bolt position.
	static const SEGMENT arrLongSegments[A_Count] =
	{
		{&Long0_180, 1, 1, 27, 0}, //A_0
		{&Long22_202, 1, 12, 26, -11}, //A_22
		{&Long45_225, 1, 21, 21, -20}, //A_45
		{&Long67_247, 1, 24, 10, -23}, //A_67
		{&Long90_270, 2, 27, 0, -26}, //A_90
		{&Long112_292, 11, 24, -10, -23}, //A_112
		{&Long135_315, 22, 21, -22, -21}, //A_135
		{&Long157_337, 26, 12, -25, -11}, //A_157
		{&Long0_180, 28, 1, -27, 0}, //A_180
		{&Long22_202, 26, 1, -25, 11}, //A_202
		{&Long45_225, 22, 1, -21, 20}, //A_225
		{&Long67_247, 11, 1, -10, 23}, //A_247
		{&Long90_270, 2, 1, 0, 26}, //A_270
		{&Long112_292, 1, 1, 10, 23}, //A_292
		{&Long135_315, 1, 1, 21, 20}, //A_315
		{&Long157_337, 1, 1, 25, 11} //A_337
	};

	//Data needed to draw short segments and update bolt position.
	static const SEGMENT arrShortSegments[A_Count] =
	{
		{&Short0_180, 0, 1, 5, 0}, //A_0
		{&Short22_202, 1, 3, 5, -2}, //A_22
		{&Short45_225, 1, 4, 4, -3}, //A_45
		{&Short67_247, 1, 4, 1, -4}, //A_67
		{&Short90_270, 1, 5, 0, -5}, //A_90
		{&Short112_292, 2, 4, -1, -4}, //A_112
		{&Short135_315, 5, 4, -4, -3}, //A_135
		{&Short157_337, 5, 3, -4, -2}, //A_157
		{&Short0_180, 5, 1, -5, 0}, //A_180
		{&Short22_202, 5, 1, -4, 2}, //A_202
		{&Short45_225, 5, 1, -4, 3}, //A_225
		{&Short67_247, 2, 0, -1, 4}, //A_247
		{&Short90_270, 1, 0, 0, 5}, //A_270
		{&Short112_292, 1, 0, 1, 4}, //A_292
		{&Short135_315, 1, 1, 4, 3}, //A_315
		{&Short157_337, 1, 1, 4, 2} //A_337
	};

	//Initialize bolt position to beginning point.
	int x = xBegin, y = yBegin;

	//Draw a random sparkle near beginning of bolt.
	static const UINT CX_TILE_HALF = CBitmapManager::CX_TILE / 2;
	static const UINT CY_TILE_HALF = CBitmapManager::CY_TILE / 2;
	int xSparkle = x + RAND(CBitmapManager::CX_TILE) - CX_TILE_HALF;
	int ySparkle = y + RAND(CBitmapManager::CY_TILE) - CY_TILE_HALF;
	static const UINT USE_LARGE_SPARKLE_THRESHOLD = RAND_MAX / 2;
	if (static_cast<UINT>(rand()) > USE_LARGE_SPARKLE_THRESHOLD)
	{
		SDL_Rect Dest = MAKE_SDL_RECT(xSparkle, ySparkle, BigSparkle.w, BigSparkle.h);
		drawSegments.push_back(BoltSegment(&BigSparkle, Dest));
		dirtyRects.push_back(Dest);
	}
	else
	{
		SDL_Rect Dest = MAKE_SDL_RECT(xSparkle, ySparkle, SmallSparkle.w, SmallSparkle.h);
		drawSegments.push_back(BoltSegment(&SmallSparkle, Dest));
		dirtyRects.push_back(Dest);
	}

	//While the bolt position (x,y) has not reached the end point, draw one bolt segment.
	ANGLE eMoveDir;
	double dblDistToEnd = sqrt(static_cast<double>((x - xEnd) * (x - xEnd) + (y - yEnd) * (y - yEnd)));
	double dblDistFromBegin, dblDistToClosest = 0.0;
	UINT dwMoveDirectThreshold;
	bool bUseLongSegments;
	while (dblDistToEnd > CX_TILE_HALF)
	{
		//Choose whether to move bolt position randomly or directly toward end point.
		const double LARGEST_POSSIBLE_DIST = (DISPLAY_SIZE * CBitmapManager::CX_TILE);
		dblDistFromBegin = sqrt(static_cast<double>((x - xBegin) * (x - xBegin) + (y - yBegin) * (y - yBegin)));
		dblDistToClosest = (dblDistFromBegin < dblDistToEnd) ?
				dblDistFromBegin : dblDistToClosest;
		if (dblDistFromBegin == 0 || dblDistToEnd < (CBitmapManager::CX_TILE * 3))
			dwMoveDirectThreshold = 0L; //When close to begin and end point, move directly.
		else
			dwMoveDirectThreshold = static_cast<UINT>(RAND_MAX * (dblDistToClosest / LARGEST_POSSIBLE_DIST));
		if (static_cast<UINT>(rand()) > dwMoveDirectThreshold) //Move directly.
		{
			//Figure out which angle is the most direct.
			if ((x - xEnd) == 0) //Vertical angle.
			{
				if ((y - yEnd) > 0)
					eMoveDir = A_90;
				else
				{
					ASSERT((y - yEnd) != 0); //Should not be at end point.
					eMoveDir = A_270;
				}
			}
			else
			{
				const double dblSlope = ABS((double) (y - yEnd) / (double) (x - xEnd));
				if (yEnd - y < 0)
				{
					if (xEnd - x <= 0) //179.99 to 90 degrees.
					{
						if (dblSlope < SLOPE_11_25)
							eMoveDir = A_180;
						else if (dblSlope < SLOPE_33_75)
							eMoveDir = A_157;
						else if (dblSlope < SLOPE_56_25)
							eMoveDir = A_135;
						else if (dblSlope < SLOPE_78_75)
							eMoveDir = A_112;
						else
							eMoveDir = A_90;
					}
					else           //0 to 89.99 degrees.
					{
						if (dblSlope < SLOPE_11_25)
							eMoveDir = A_0;
						else if (dblSlope < SLOPE_33_75)
							eMoveDir = A_22;
						else if (dblSlope < SLOPE_56_25)
							eMoveDir = A_45;
						else if (dblSlope < SLOPE_78_75)
							eMoveDir = A_67;
						else
							eMoveDir = A_90;
					}
				}
				else
				{
					if (xEnd - x <= 0) //180.01 to 270 degrees.
					{
						if (dblSlope < SLOPE_11_25)
							eMoveDir = A_180;
						else if (dblSlope < SLOPE_33_75)
							eMoveDir = A_202;
						else if (dblSlope < SLOPE_56_25)
							eMoveDir = A_225;
						else if (dblSlope < SLOPE_78_75)
							eMoveDir = A_247;
						else
							eMoveDir = A_270;
					}
					else           //359.99 to 270.01 degrees.
					{
						if (dblSlope < SLOPE_11_25)
							eMoveDir = A_0;
						else if (dblSlope < SLOPE_33_75)
							eMoveDir = A_337;
						else if (dblSlope < SLOPE_56_25)
							eMoveDir = A_315;
						else if (dblSlope < SLOPE_78_75)
							eMoveDir = A_292;
						else
							eMoveDir = A_270;
					}
				}
			}

			if (dblDistToEnd < (CBitmapManager::CX_TILE * 3))
				bUseLongSegments = false; //Don't want to overshoot end point.
			else
			{
				//Decide randomly to use long segments or short segments.
				static const UINT USE_LONG_SEGMENT_THRESHOLD = RAND_MAX / 2;
				bUseLongSegments = (static_cast<UINT>(rand()) > USE_LONG_SEGMENT_THRESHOLD);
			}
		}
		else //Move randomly.
		{
			const int nRandDir = RAND(A_Count - 1);
			ASSERT(nRandDir >= 0 && nRandDir < A_Count);
			eMoveDir = static_cast<ANGLE>(nRandDir);
			bUseLongSegments = false;
		}

		if (bUseLongSegments)
		{
			//Blit long segment.
			SDL_Rect Dest = MAKE_SDL_RECT(x - arrLongSegments[eMoveDir].dxSrc, 
					y - arrLongSegments[eMoveDir].dySrc,
					arrLongSegments[eMoveDir].pSrcRect->w,
					arrLongSegments[eMoveDir].pSrcRect->h);
			drawSegments.push_back(BoltSegment(arrLongSegments[eMoveDir].pSrcRect, Dest));
			dirtyRects.push_back(Dest);

			//Update bolt position.
			x += arrLongSegments[eMoveDir].dxPos;
			y += arrLongSegments[eMoveDir].dyPos;
		}
		else
		{
			//Blit short segment.
			SDL_Rect Dest = MAKE_SDL_RECT(x - arrShortSegments[eMoveDir].dxSrc, 
					y - arrShortSegments[eMoveDir].dySrc,
					arrShortSegments[eMoveDir].pSrcRect->w,
					arrShortSegments[eMoveDir].pSrcRect->h);
			drawSegments.push_back(BoltSegment(arrShortSegments[eMoveDir].pSrcRect, Dest));
			dirtyRects.push_back(Dest);

			//Update bolt position.
			x += arrShortSegments[eMoveDir].dxPos;
			y += arrShortSegments[eMoveDir].dyPos;
		}

		//Draw a random sparkle.
		xSparkle = x + (RAND(CBitmapManager::CX_TILE)) - CX_TILE_HALF;
		ySparkle = y + (RAND(CBitmapManager::CY_TILE)) - CY_TILE_HALF;
		if (static_cast<UINT>(rand()) > USE_LARGE_SPARKLE_THRESHOLD)
		{
			SDL_Rect Dest = MAKE_SDL_RECT(xSparkle, ySparkle, BigSparkle.w, BigSparkle.h);
			drawSegments.push_back(BoltSegment(&BigSparkle, Dest));
			dirtyRects.push_back(Dest);
		}
		else
		{
			SDL_Rect Dest = MAKE_SDL_RECT(xSparkle, ySparkle, SmallSparkle.w, SmallSparkle.h);
			drawSegments.push_back(BoltSegment(&SmallSparkle, Dest));
			dirtyRects.push_back(Dest);
		}

		dblDistToEnd = sqrt(static_cast<double>((x - xEnd) * (x - xEnd) + (y - yEnd) * (y - yEnd)));
	} //...while bolt position is not very close to destination.
}

void DrawBolt(BOLT_SEGMENTS &segments, SDL_Surface& pPartsSurface, SDL_Surface& pDestSurface)
{
	for (UINT i = 0; i < segments.size(); ++i)
	{
		BoltSegment segment = segments.at(i);
		SDL_BlitSurface(&pPartsSurface, segment.pSourceRect, &pDestSurface, &(segment.destRect));
	}
}

void DrawBolt(int xBegin, int yBegin, int xEnd, int yEnd, const UINT DISPLAY_SIZE, SDL_Surface* pPartsSurface, SDL_Surface* pDestSurface, vector<SDL_Rect>& dirtyRects)
{
	BOLT_SEGMENTS boltSegments;
	GenerateBolt(xBegin, yBegin, xEnd, yEnd, DISPLAY_SIZE, boltSegments, dirtyRects);
	DrawBolt(boltSegments, *pPartsSurface, *pDestSurface);
}
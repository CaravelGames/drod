// $Id$

#include "Rectangle.h"
#include <BackEndLib/Assert.h>

#include <math.h>
#include <limits>

SceneRect::SceneRect(const Point& p1, const Point& p2)
	: SceneObject(SOT_Rect)
{
	//Set "bounding box"
	this->min = p1;
	this->max = p2;

	ASSERT(this->min.A[0] <= this->max.A[0]);
	ASSERT(this->min.A[1] <= this->max.A[1]);
	ASSERT(this->min.A[2] <= this->max.A[2]);
}

bool SceneRect::intersects(const Point& Ro, const Point& Rd, float& min_t)
//Is axially-aligned quad intersected?
//This intersection check is identical to that for a bounding box.
{
	//Consider each coordinate axis.
	float tNear = -std::numeric_limits<float>::max();
	float tFar = std::numeric_limits<float>::max();
	float t1, t2;
	for (int n=3; n--; )
	{
		if (Rd.A[n] == 0.0)
		{
			//Ray parallel to axis plane.
			if (Ro.A[n] < this->min.A[n] || Ro.A[n] > this->max.A[n])
				return false;	//ray origin not between planes
		} else {
			const float xd_inverse = 1.0f / Rd.A[n];
			//t1 = (xl - x0) / xd
			t1 = (this->min.A[n] - Ro.A[n]) * xd_inverse;
			//t2 = (xh - x0) / xd
			t2 = (this->max.A[n] - Ro.A[n]) * xd_inverse;
			if (t1 > t2) {
				const float temp = t1;
				t1 = t2;
				t2 = temp;
			}
			if (t1 > tNear)
			{
				tNear = t1;
				//If bounding box is further away than the closest t so far, then nothing in it will be used.
				//So the bounding box is effectively missed.
				if (tNear > min_t)
					return false;
			}
			if (t2 < tFar)
				tFar = t2;
			if (tNear > tFar)
				return false;	//box is missed
			if (tFar < 0)
				return false;	//box is behind ray
		}
	}

	//Ray does intersect bounding box.
	min_t = tNear;
	return true;
}

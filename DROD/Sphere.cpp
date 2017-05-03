// $Id$

#include "Sphere.h"
#include <BackEndLib/Types.h>
#include <math.h>

Sphere::Sphere(const Point& center, const float radius)
	: SceneObject(SOT_Sphere)
	, center(center)
	, radius(radius)
	, radiusSquared(radius * radius)
{
	for (UINT x=3; x--; )
	{
		this->min.A[x] = this->center.A[x] - this->radius;
		this->max.A[x] = this->center.A[x] + this->radius;
	}
}

//IN:
//Ro: origin of the ray
//Rd: direction of the ray
//
//OUT:
//min_t: distance to point of intersection
bool Sphere::intersects(const Point& Ro, const Point& Rd, float &min_t)
{
	//Step 1. Distances from ray origin to center of sphere.
	const float
		OxMinusCx = Ro.A[0] - this->center.A[0],
		OyMinusCy = Ro.A[1] - this->center.A[1],
		OzMinusCz = Ro.A[2] - this->center.A[2];

	//A = 1
	//B = 2(xdxo - xdxc + ydyo - ydyc + zdzo - zdzc)
	//  = 2(xd(xo - xc) + yd(yo - yc) + zd(zo - zc))
	//	= Rd . (O-C)
	const float B = //no 2.0 *  --  i.e., this is really half of B
		Rd.A[0] * OxMinusCx +
		Rd.A[1] * OyMinusCy +
		Rd.A[2] * OzMinusCz;
	//C = xo^2 - 2xoxc + xc^2 + yo^2 - 2yoyc + yc^2 + zo^2 - 2zozc + zc^2 - r^2
	//  = (xo - xc)^2 + (yo - yc)^2 + (zo - zc)^2 - r^2
	//	= (O-C) . (O-C) - r^2
	const float C =
		OxMinusCx * OxMinusCx +
		OyMinusCy * OyMinusCy +
		OzMinusCz * OzMinusCz -
		this->radiusSquared;

	//Step 2. Calculate discriminant.
	const float desc = B * B - C;	//no 4.0 *  -- i.e., this is really 1/4th of the discriminant
	//Step 3. If square root is imaginary, there are no real intersections.
	if (desc < 0.0f) return false;
	//Step 4. Solve quadratic.
	const float SqrtDesc = sqrt(desc);
	const float t0 = -(B + SqrtDesc);	//smaller of two roots -- check it first
	//Step 5. Determine parametric point(s) of intersection.
	if (t0 < 0.001f)
	{
		//First intersection is behind Ro (i.e., ray originates inside or past sphere).
		const float t1 = SqrtDesc - B;	//try other root
		//Step 6.
		if (t1 < 0.001f)
			return false;	//ray originates past sphere

		//Closer than other intersections?
		if (t1 >= min_t)
			return false;	//No.
		min_t = t1;
		return true;
	} else {
		//Closer than other intersections?
		if (t0 >= min_t)
			return false;	//No.
		min_t = t0;
		return true;
	}
}

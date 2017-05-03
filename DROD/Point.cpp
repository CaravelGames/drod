// $Id$

#include "Point.h"

#include <math.h>

Point::Point(const float x, const float y, const float z)
{
	A[0]=x; A[1]=y; A[2]=z;
}

Point::Point(const Point& point)
{
	A[0] = point.A[0]; A[1] = point.A[1]; A[2] = point.A[2];
}

void Point::operator = (const Point& point)
{
	this->A[0] = point.A[0]; this->A[1] = point.A[1]; this->A[2] = point.A[2];
}

Point Point::operator - () const
{
	return Point(-A[0], -A[1], -A[2]);
}

Point Point::operator - (const Point& point) const
{
	return Point(
		A[0] - point.A[0],
		A[1] - point.A[1],
		A[2] - point.A[2]);
}

Point Point::operator + (const Point& point) const
{
	return Point(
		A[0] + point.A[0],
		A[1] + point.A[1],
		A[2] + point.A[2]);
}

Point Point::operator * (const float val) const
{
	return Point(A[0] * val, A[1] * val, A[2] * val);
}

Point Point::operator * (const Point& point) const
{
	return Point(
		A[0] * point.A[0],
		A[1] * point.A[1],
		A[2] * point.A[2]);
}

Point Point::operator / (const float val) const
{
	const float oneOverVal = 1.0f / val;
	return Point(
		A[0] * oneOverVal,
		A[1] * oneOverVal,
		A[2] * oneOverVal);
}

void Point::operator += (const Point& point)
{
	A[0] += point.A[0]; A[1] += point.A[1]; A[2] += point.A[2];
}

void Point::operator -= (const Point& point)
{
	A[0] -= point.A[0]; A[1] -= point.A[1]; A[2] -= point.A[2];
}

void Point::operator *= (const float val)
{
	A[0] *= val; A[1] *= val; A[2] *= val;
}

void Point::operator *= (const Point& point)
{
	A[0] *= point.A[0]; A[1] *= point.A[1]; A[2] *= point.A[2];
}

void Point::operator /= (const float val)
{
	const float oneOverVal = 1.0f / val;
	this->A[0] *= oneOverVal;
	this->A[1] *= oneOverVal;
	this->A[2] *= oneOverVal;
}

bool Point::operator >= (const Point& point)
{
	return
		this->A[0] >= point.A[0] &&
		this->A[1] >= point.A[1] &&
		this->A[2] >= point.A[2];
}

bool Point::operator <= (const Point& point)
{
	return
		this->A[0] <= point.A[0] &&
		this->A[1] <= point.A[1] &&
		this->A[2] <= point.A[2];
}

Point Point::cross(const Point& p) const
{
	return Point(
		this->A[1] * p.A[2] - this->A[2] * p.A[1],
		this->A[2] * p.A[0] - this->A[0] * p.A[2],
		this->A[0] * p.A[1] - this->A[1] * p.A[0]);
}

float Point::dot(const Point& p) const
{
	return (
		this->A[0] * p.A[0] +
		this->A[1] * p.A[1] +
		this->A[2] * p.A[2]);
}

float Point::distance(const Point& p) const
{
	return sqrtf(distanceSq(p));
}

float Point::distanceSq(const Point& p) const
{
	return ((p.A[0] - this->A[0]) * (p.A[0] - this->A[0])) +
		((p.A[1] - this->A[1]) * (p.A[1] - this->A[1])) +
		((p.A[2] - this->A[2]) * (p.A[2] - this->A[2]));
}

float Point::length() const
{
	return sqrtf(lengthSq());
}

float Point::lengthSq() const
{
	return (this->A[0] * this->A[0]) +
		(this->A[1] * this->A[1]) +
		(this->A[2] * this->A[2]);
}

void Point::normalize()
{
	operator /= (length());
}

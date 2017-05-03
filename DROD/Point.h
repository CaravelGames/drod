// $Id$


//////////////////////////////////////////////////////////////////////
// Point.h
//////////////////////////////////////////////////////////////////////

#ifndef POINT_H
#define POINT_H

class Point {
public:
	Point() {}
	Point(const Point& point);
	Point(const float x, const float y, const float z);

	void operator = (const Point& point);
	Point operator - () const;

	Point operator + (const Point& point) const;
	Point operator - (const Point& point) const;
	Point operator * (const float val) const;
	Point operator * (const Point& point) const;
	Point operator / (const float val) const;

	void operator += (const Point& point);
	void operator -= (const Point& point);
	void operator *= (const float val);
	void operator *= (const Point& point);
	void operator /= (const float val);

	bool operator >= (const Point& point);
	bool operator <= (const Point& point);

	Point cross(const Point& p) const;
	float dot(const Point& p) const;
	float distance(const Point& p) const;
	float distanceSq(const Point& p) const;
	float length() const;
	float lengthSq() const;
	void normalize();
	inline void set(const float x, const float y, const float z)
		{A[0]=x; A[1]=y; A[2]=z;}

	inline float x() const {return this->A[0];}
	inline float y() const {return this->A[1];}
	inline float z() const {return this->A[2];}

	inline float &x() {return this->A[0];}
	inline float &y() {return this->A[1];}
	inline float &z() {return this->A[2];}

	float A[3];	//x, y, z
};

#endif

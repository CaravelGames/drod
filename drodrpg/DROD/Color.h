// $Id: Color.h 8102 2007-08-15 14:55:40Z trick $


//////////////////////////////////////////////////////////////////////
// Color.h
//////////////////////////////////////////////////////////////////////

#ifndef COLOR_H
#define COLOR_H

#include <BackEndLib/Types.h>

class Color {
public:
	Color() {set(0.0, 0.0, 0.0);}
	Color(const Color& color);
	Color(const float r, const float g, const float b, const float a=0.0);

	inline float r() const {return this->A[0];}
	inline float g() const {return this->A[1];}
	inline float b() const {return this->A[2];}
	inline float a() const {return this->A[3];}

	inline float &r() {return this->A[0];}
	inline float &g() {return this->A[1];}
	inline float &b() {return this->A[2];}
	inline float &a() {return this->A[3];}

	Color operator + (Color const &c) const;
	Color operator - (Color const &c) const;
	Color operator * (const float val) const;
	Color operator * (Color const &c) const;
	Color operator / (const float val) const;

	void operator += (const float val);
	void operator += (Color &c);
	void operator *= (const float val);
	void operator *= (Color &c);
	void operator /= (const float val);

	Color oneMinus() const;

	void set(const float r, const float g, const float b, const float a=0.0);

	float A[4];	//r, g, b, a;
};

#endif

// $Id$

#include "Color.h"

Color::Color(const Color& color)
{
	A[0]=color.A[0]; A[1]=color.A[1]; A[2]=color.A[2]; A[3]=color.A[3];
}

Color::Color(const float r, const float g, const float b, const float a)
{
	set(r,g,b,a);
}

Color Color::operator + (Color const &c) const
{
	return Color(A[0] + c.A[0], A[1] + c.A[1], A[2] + c.A[2], A[3] + c.A[3]);
}

Color Color::operator - (Color const &c) const
{
	return Color(A[0] - c.A[0], A[1] - c.A[1], A[2] - c.A[2], A[3] - c.A[3]);
}

Color Color::operator * (const float val) const
{
	return Color(A[0] * val, A[1] * val, A[2] * val, A[3] * val);
}

Color Color::operator * (Color const &c) const
{
	return Color(A[0] * c.A[0], A[1] * c.A[1], A[2] * c.A[2], A[3] * c.A[3]);
}

Color Color::operator / (const float val) const
{
	const float oneOverVal = 1.0f / val;
	return Color(A[0] * oneOverVal, A[1] * oneOverVal, A[2] * oneOverVal, A[3] * oneOverVal);
}

void Color::operator += (const float val)
{
	A[0] += val; A[1] += val; A[2] += val; A[3] += val;
}

void Color::operator += (Color &c)
{
	A[0] += c.A[0]; A[1] += c.A[1]; A[2] += c.A[2]; A[3] += c.A[3];
}

void Color::operator *= (const float val)
{
	A[0] *= val; A[1] *= val; A[2] *= val; A[3] *= val;
}

void Color::operator *= (Color &c)
{
	A[0] *= c.A[0]; A[1] *= c.A[1]; A[2] *= c.A[2]; A[3] *= c.A[3];
}

void Color::operator /= (const float val)
{
	const float oneOverVal = 1.0f / val;
	A[0] *= oneOverVal; A[1] *= oneOverVal; A[2] *= oneOverVal; A[3] *= oneOverVal;
}

Color Color::oneMinus() const
{
	return Color(1.0f - A[0], 1.0f - A[1], 1.0f - A[2], 1.0f - A[3]);
}

void Color::set(const float r, const float g, const float b, const float a)
{
	A[0]=r; A[1]=g; A[2]=b; A[3]=a;
}

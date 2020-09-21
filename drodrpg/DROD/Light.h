// $Id$


//////////////////////////////////////////////////////////////////////
// Light.h
//////////////////////////////////////////////////////////////////////

#ifndef LIGHT_H
#define LIGHT_H

#include "Color.h"
#include "Point.h"

class Light
{
public:
	Light(const Color& color);
	virtual ~Light();

	inline void setIntensity(const Color& color) {this->color = color;}

	virtual bool directionTo(const Point& fromPoint, Point& Ld, float& dist) const=0;
	virtual bool directionToAndIntensity(const Point& fromPoint, Point& Ld, float& dist, Color& intensity) const=0;

	Color color;
	float fMaxDistance;
};

class DirectionalLightObject : public Light
{
public:
	DirectionalLightObject(const Point& direction, const Color& color);

	virtual bool directionTo(const Point& fromPoint, Point& Ld, float& dist) const;
	virtual bool directionToAndIntensity(const Point& fromPoint, Point& Ld, float& dist, Color& intensity) const;

	inline void setDirection(const Point& direction) {this->direction = direction;}

	Point direction;
};

class PointLightObject : public Light
{
public:
	PointLightObject(const Point& location, const Color& color, const Color& attenuation);

	virtual bool directionTo(const Point& fromPoint, Point& Ld, float& dist) const;
	virtual bool directionToAndIntensity(const Point& fromPoint, Point& Ld, float& dist, Color& intensity) const;

	inline void setAttenuation(const Color& attenuation) {this->attenuation = attenuation;}
	inline void setLocation(const Point& location) {this->location = location;}

	Color attenuation;	//represents polynomial
	Point location;
};

class SpotLightObject : public PointLightObject
{
public:
	SpotLightObject(const Point& location, const Color& color, const Color& attenuation);

	virtual bool directionTo(const Point& fromPoint, Point& Ld, float& dist) const;
	virtual bool directionToAndIntensity(const Point& fromPoint, Point& Ld, float& dist, Color& intensity) const;

	inline void setDirection(const Point& direction) {this->direction = direction;}
	inline void setExponent(const float exp) {this->exponent = exp;}

	Point direction;
	float exponent;
	float cosineCutoff;
};

#endif

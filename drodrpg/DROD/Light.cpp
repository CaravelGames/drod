// $Id$

#include "Light.h"

#include <math.h>

//*****************************************************************************
Light::Light(const Color& color)
	: color(color)
{}

Light::~Light()
{}

//*****************************************************************************
DirectionalLightObject::DirectionalLightObject(const Point& direction, const Color& color)
	: Light(color)
	, direction(direction)
{}

bool DirectionalLightObject::directionTo(const Point& fromPoint, Point& Ld, float& dist) const
{
	//Direction to light
	Ld = this->direction * -100000000.0;	//located at infinity

	//Distance
	dist = Ld.length();
	Ld /= dist;	//normalize
	return true;
}

bool DirectionalLightObject::directionToAndIntensity(const Point& fromPoint, Point& Ld, float& dist, Color& intensity) const
{
	if (!directionTo(fromPoint, Ld, dist))
		return false;

	//Intensity
	intensity = this->color;

	return true;
}

//*****************************************************************************
PointLightObject::PointLightObject(const Point& location, const Color& color, const Color& attenuation)
	: Light(color)
	, attenuation(attenuation)
	, location(location)
{}

bool PointLightObject::directionTo(const Point& fromPoint, Point& Ld, float& dist) const
{
	//Direction to light
	Ld = this->location - fromPoint;

	//Distance
	dist = Ld.length();
	Ld /= dist;	//normalize
	return true;
}

bool PointLightObject::directionToAndIntensity(const Point& fromPoint, Point& Ld, float& dist, Color& intensity) const
{
	if (!directionTo(fromPoint, Ld, dist))
		return false;

	//Intensity
	const float val = (this->attenuation.A[0] * dist + this->attenuation.A[1]) * dist + this->attenuation.A[2];
	intensity = this->color / (val < 1.0f ? 1.0f : val);	//intensity ceiling

	return true;
}

//*****************************************************************************
SpotLightObject::SpotLightObject(const Point& location, const Color& color, const Color& attenuation)
	: PointLightObject(location, color, attenuation)
	, exponent(1.0)
	, cosineCutoff(0.0f)	//cos of 1.5707
{}

bool SpotLightObject::directionTo(const Point& fromPoint, Point& Ld, float& dist) const
{
	//Direction to light
	Ld = this->location - fromPoint;

	//Distance
	dist = Ld.length();
	Ld /= dist;	//normalize
	return -Ld.dot(this->direction) >= this->cosineCutoff;  //Whether light shines in this direction.
}

bool SpotLightObject::directionToAndIntensity(const Point& fromPoint, Point& Ld, float& dist, Color& intensity) const
{
	if (!directionTo(fromPoint, Ld, dist))
		return false;

	//Intensity
	//Based on angle of spotlight.
	float DdotDir = -Ld.dot(this->direction);
	DdotDir = powf(DdotDir, this->exponent);

	const float val = (this->attenuation.A[0] * dist + this->attenuation.A[1]) * dist + this->attenuation.A[2];
	intensity = this->color * (DdotDir / (val));// < 1.0f ? 1.0f : val));	//intensity ceiling

	return true;
}

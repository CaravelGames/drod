// $Id: SceneObject.h 8102 2007-08-15 14:55:40Z trick $


//////////////////////////////////////////////////////////////////////
// SceneObject.h
//////////////////////////////////////////////////////////////////////

#ifndef SCENEOBJECT_H
#define SCENEOBJECT_H

#include "Point.h"
#include <BackEndLib/Assert.h>

enum SceneObjectType
{
	SOT_Invalid=0,
	SOT_Sphere,
	SOT_Rect,
	SOT_BoundingBox
};

class Scene;
class SceneObject
{
public:
	SceneObject(const SceneObjectType type=SOT_Invalid)
		: type(type)
	{}
	virtual ~SceneObject() {}

	inline SceneObjectType getType() const {return this->type;}

	//Calculate point of intersection (parametrically).
	virtual bool intersects(const Point& Ro, const Point& Rd, float &min_t)=0;

	inline void resetMinMaxPoints()
	{
		static const float min_t = 99999.0;
		static const float max_t = -99999.0;

		this->min.set(max_t,max_t,max_t);
		this->max.set(min_t,min_t,min_t);
	}

	Point min, max;	//corners of bounding box

private:
	SceneObjectType type;
};

#endif


// $Id: Rectangle.h 8102 2007-08-15 14:55:40Z trick $


//////////////////////////////////////////////////////////////////////
// Rectangle.h
//////////////////////////////////////////////////////////////////////

#ifndef RECTANGLE_H
#define RECTANGLE_H

#include "SceneObject.h"
#include "Point.h"

//Axially-aligned rectangle -- like one face of a bounding box.

class SceneRect : public SceneObject
{
public:
	SceneRect(const Point& p1, const Point& p2);

	virtual bool intersects(const Point& Ro, const Point& Rd, float &min_t);
};

#endif

// $Id$


#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include "SceneObject.h"

#include <vector>
using std::vector;

class CBoundingBox : public SceneObject
{
public:
	CBoundingBox();

	void addObject(SceneObject* const sceneObject);
	void clear();
	void init(const int xMin, const int yMin, int nWidth, int nHeight);

	virtual bool intersects(const Point& Ro, const Point& Rd, float &min_t);
	const SceneObject* intersectsAny(const Point& Ro, const Point& Rd, float &min_t);
	bool intersectsBox(const Point& Ro, const Point& Rd, float &min_t) const;

	void prune();

	vector<SceneObject*> objects;
	const SceneObject *pIntersectedObject;	//object intersected inside bounding box (only valid until another intersection check is performed)
};

#endif

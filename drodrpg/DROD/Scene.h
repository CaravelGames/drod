// $Id: Scene.h 8102 2007-08-15 14:55:40Z trick $


//////////////////////////////////////////////////////////////////////
// Scene.h
//////////////////////////////////////////////////////////////////////

#ifndef SCENE_H
#define SCENE_H

#include "BoundingBox.h"
#include "Point.h"
#include <BackEndLib/Types.h>

#include <vector>
using std::vector;

class Light;
class SceneObject;
class Scene
{
public:
	Scene() {}

	void addObject(SceneObject* pSceneObject);

	void clear();
	bool empty() const;
	void init(const UINT x1, const UINT y1, const UINT x2, const UINT y2);
	void ready();

	bool lightShinesAt(const Point& Ro, const Light& light);

	void analyzeTree();
	void analyzeNode(CBoundingBox* pBBox, const UINT wDepth);

private:
	CBoundingBox objectHierarchy;	//traversed during intersection tests
};

#endif


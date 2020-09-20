// $Id$

//Contains an object hierarchy for rendering a scene.

#include "Scene.h"
#include "Color.h"
#include "Light.h"
#include <BackEndLib/Assert.h>
#include <cstring> //memset

//*****************************************************************************
void Scene::addObject(SceneObject* pSceneObject)
//Places object in a bounding box hierarchy (intersection optimization).
{
	ASSERT(pSceneObject);
	this->objectHierarchy.addObject(pSceneObject);
}

//*****************************************************************************
void Scene::clear()
{
	this->objectHierarchy.clear();
}

//*****************************************************************************
bool Scene::empty() const
//Returns: true if scene is empty
{
	return this->objectHierarchy.objects.empty();
}

//*****************************************************************************
void Scene::init(const UINT x1, const UINT y1, const UINT x2, const UINT y2)
//Prepare scene tree structure.
{
	clear();
	this->objectHierarchy.init(x1, y1, x2-x1+1, y2-y1+1);  //x,y,w,h
}

//*****************************************************************************
void Scene::ready()
//Notifies the scene that all objects ahve been inserted.
{
	this->objectHierarchy.prune();
}

//*****************************************************************************
bool Scene::lightShinesAt(const Point& Ro, const Light& light)
//Determine light hitting this point by sending out shadow rays from point Ro.
//
//Returns: color of light at Ro
{
	Point Ld;   //direction to light (normal vector)
	float t;	   //distance to light
	if (!light.directionTo(Ro, Ld, t))
		return false;	//light doesn't shine here

	if (empty())
		return true; //scene is void of obstructions

	//Shadow ray
	//Is any object intersected on the way to the light source?
	//Optimization: assume root bounding box is always intersected
	for (std::vector<SceneObject*>::iterator object=this->objectHierarchy.objects.begin();
			object!=this->objectHierarchy.objects.end(); ++object)
	{
		const SceneObject *pOccludingObject = this->objectHierarchy.intersectsAny(Ro, Ld, t);
		if (pOccludingObject)
			return false;
	}

	return true;
}

/*
//-----------------------------------------------------------------------------
#define WANT_TO_SEE_MORE_STATS

const UINT BINS = 1000;
UINT widthHisto[BINS];
UINT depthHisto[BINS];
unsigned long numObjects, numBBoxes, totalDepthSum;

//Traverse the tree.
void Scene::analyzeTree()
{
	memset(widthHisto, 0, BINS * sizeof(UINT));
	memset(depthHisto, 0, BINS * sizeof(UINT));
	numObjects = numBBoxes = totalDepthSum = 0L;

	analyzeNode(&this->objectHierarchy, 0);

	//Output results.
	printf("# objects in scene: %li\n", numObjects);
	printf("# bounding boxes used: %li\n", numBBoxes);
#ifdef WANT_TO_SEE_MORE_STATS
	printf("# of nodes of width i:\n");
	UINT i;
	for (i=0; i<BINS; ++i)
	{
		if (widthHisto[i] > 0)
			printf("%i: %i\n", i, widthHisto[i]);
	}
	printf("# of nodes of depth i:\n");
	for (i=0; i<BINS; ++i)
	{
		if (depthHisto[i] > 0)
		{
			printf("%i: %i\n", i+1, depthHisto[i]);
			totalDepthSum += (i+1) * depthHisto[i];
		}
	}
#endif
	printf("Total depth sum: %li\n", totalDepthSum);
}

//Tally widths of all the nodes in the object hierarchy.
void Scene::analyzeNode(CBoundingBox* pBBox, const UINT wDepth)
{
	++numBBoxes;
	const UINT width = pBBox->objects.size();
	ASSERT(width > 1 || wDepth == 0);	//no bounding box should enclose only one child -- except possibly for the root node

	++widthHisto[width >= BINS ? BINS-1 : width];
	++depthHisto[wDepth >= BINS ? BINS-1 : wDepth];

	for (std::vector<SceneObject*>::iterator object=pBBox->objects.begin(); object!=pBBox->objects.end(); ++object)
	{
		SceneObject *pObject = *object;
		if (pObject->getType() == SOT_BoundingBox)
		{
			CBoundingBox *pChildBBox = DYN_CAST(CBoundingBox*, SceneObject*, pObject);
			analyzeNode(pChildBBox, wDepth+1);	//recursive call
		} else {
			++numObjects;
			++depthHisto[wDepth+1 >= BINS ? BINS-1 : wDepth+1];
		}
	}
}
*/

// $Id: BoundingBox.cpp 8102 2007-08-15 14:55:40Z trick $

#include "BoundingBox.h"

#include <BackEndLib/Assert.h>
#include <BackEndLib/Types.h>

#include <queue>
#include <utility>
#include <limits>

using std::vector;
using std::pair;

//*****************************************************************************
CBoundingBox::CBoundingBox()
	: SceneObject(SOT_BoundingBox)
	, pIntersectedObject(NULL)
{
	//Prepare for setting up the bounding box limits as objects are added.
	resetMinMaxPoints();
}

//*****************************************************************************
void CBoundingBox::clear()
//Frees everything in the bounding box hierarchy.
{
	for (std::vector<SceneObject*>::iterator object=this->objects.begin(); object!=this->objects.end(); ++object)
	{
		SceneObject* pObject = *object;
		if (pObject->getType() == SOT_BoundingBox)
		{
			CBoundingBox *pBox = DYN_CAST(CBoundingBox*, SceneObject*, pObject);
			pBox->clear();
		}
		delete pObject;
	}
	this->objects.clear();
}

//*****************************************************************************
void CBoundingBox::init(const int xMin, const int yMin, int nWidth, int nHeight)
{
	//Define bounding box.
	const int xMax = xMin+nWidth;
	const int yMax = yMin+nHeight;
	this->min.set(float(xMin), float(yMin), 0);
	this->max.set(float(xMax), float(yMax), 1);

	//Don't divide further if it has reached the minimum size in either dimension.
	static const int MIN_SIZE = 3;
	if (nWidth <= MIN_SIZE || nHeight <= MIN_SIZE)
		return;

	//Add four children.
	nWidth /= 2;
	nHeight /= 2;
	const int xMid = xMin+nWidth;
	const int yMid = yMin+nHeight;
	const int nWidth2 = xMax-xMid;
	const int nHeight2 = yMax-yMid;

	CBoundingBox *pBox;
	pBox = new CBoundingBox();
	pBox->init(xMin, yMin, nWidth, nHeight);
	this->objects.push_back(pBox);
	pBox = new CBoundingBox();
	pBox->init(xMin, yMid, nWidth, nHeight2);
	this->objects.push_back(pBox);
	pBox = new CBoundingBox();
	pBox->init(xMid, yMid, nWidth2, nHeight2);
	this->objects.push_back(pBox);
	pBox = new CBoundingBox();
	pBox->init(xMid, yMin, nWidth2, nHeight);
	this->objects.push_back(pBox);
}

//*****************************************************************************
void CBoundingBox::addObject(SceneObject* const sceneObject)
//Adds a scene object to the bounding box.
{
	ASSERT(sceneObject);

	//Does object fit in a tighter bin?
	for (std::vector<SceneObject*>::iterator object=this->objects.begin(); object!=this->objects.end(); ++object)
	{
		SceneObject* pObject = *object;
		if (pObject->getType() == SOT_BoundingBox)
		{
			if (sceneObject->min >= pObject->min && sceneObject->max <= pObject->max)
			{
				CBoundingBox *pBox = static_cast<CBoundingBox*>(pObject);
				pBox->addObject(sceneObject);
				return;
			}
		}
	}

	this->objects.push_back(sceneObject);  //object goes here
}

//*****************************************************************************
bool CBoundingBox::intersectsBox(const Point& Ro, const Point& Rd, float &min_t) const
//Returns: true if bounding box is intersected
{
	//Is bounding box intersected?
	//Consider each coordinate axis.
	float tNear = -std::numeric_limits<float>::max();
	float tFar = std::numeric_limits<float>::max();
	float t1, t2;
	for (UINT n=3; n--; )
	{
		if (Rd.A[n] == 0.0)
		{
			//Ray parallel to axis plane.
			if (Ro.A[n] < this->min.A[n] || Ro.A[n] > this->max.A[n])
				return false;	//ray origin not between planes
		} else {
			//t1 = (xl - x0) / xd
			t1 = (this->min.A[n] - Ro.A[n]) / Rd.A[n];
			//t2 = (xh - x0) / xd
			t2 = (this->max.A[n] - Ro.A[n]) / Rd.A[n];
			if (t1 > t2) {
				float t = t1;
				t1 = t2;
				t2 = t;
			}
			if (t1 > tNear)
			{
				tNear = t1;
				//If bounding box is further away than the closest t so far, then nothing in it will be used.
				//So the bounding box is effectively missed.
				if (tNear > min_t)
					return false;
			}
			if (t2 < tFar)
				tFar = t2;
			if (tNear > tFar)
				return false;	//box is missed
			if (tFar < 0)
				return false;	//box is behind ray
		}
	}

	//Ray does intersect bounding box.
	min_t = tNear;
	return true;
}

//*****************************************************************************
bool CBoundingBox::intersects(const Point& Ro, const Point& Rd, float &min_t)
//Returns: true if an object inside the bounding box is intersected
{
	ASSERT(!this->objects.empty());
	float temp_t = min_t;
	if (!intersectsBox(Ro, Rd, temp_t))
		return false;

	this->pIntersectedObject = NULL;
	for (std::vector<SceneObject*>::iterator object=this->objects.begin(); object!=this->objects.end(); ++object)
		if ((*object)->intersects(Ro, Rd, min_t))
			this->pIntersectedObject = (*object);	//get closest object

	return this->pIntersectedObject != NULL;
}

//*****************************************************************************
SceneObject* CBoundingBox::intersectsAny(const Point& Ro, const Point& Rd, float &min_t)
//Returns: a SceneObject, if one inside the bounding box is intersected
{
	ASSERT(!this->objects.empty());

	float temp_t = min_t;
	if (!intersectsBox(Ro, Rd, temp_t))
		return NULL;

	//Check for intersections with immediate children.
	this->pIntersectedObject = NULL;
	for (std::vector<SceneObject*>::iterator object=this->objects.begin(); object!=this->objects.end(); ++object)
	{
		SceneObject *pTest, *pObject = *object;
		if (pObject->getType() == SOT_BoundingBox)
		{
			CBoundingBox *pBBox = static_cast<CBoundingBox*>(pObject);
			
			pTest = pBBox->intersectsAny(Ro, Rd, min_t);
			if (pTest)
			{
				//Return any object intersected.
				this->pIntersectedObject = pTest;
				return pTest;
			}
		} else {
			if (pObject->intersects(Ro, Rd, min_t))
			{
				//Return any object intersected immediately.
				this->pIntersectedObject = pObject;
				return pObject;
			}
		}
	}

	//No objects were intersected.
	return NULL;
}

//*****************************************************************************
void CBoundingBox::prune()
//Cleans up empty or unneeded bounding boxes.
{
	for (std::vector<SceneObject*>::iterator object=this->objects.begin(); object!=this->objects.end(); )
	{
		SceneObject *pObject = *object;
		if (pObject->getType() == SOT_BoundingBox)
		{
			CBoundingBox *pBBox = static_cast<CBoundingBox*>(pObject);
			pBBox->prune(); //recursive call

			SceneObject *pChild = NULL;
			if (pBBox->objects.size() == 1)
			{
				//Object doesn't need to be in a bounding box by itself -- move it up a level.
				pChild = pBBox->objects[0];
				ASSERT(pChild);
				pBBox->objects.clear();
			}
			if (pBBox->objects.empty())
			{
				delete pBBox;
				object = this->objects.erase(object);
			}
			else
				++object;

			if (pChild)
			{
				int index = object - this->objects.begin();
				this->objects.push_back(pChild);
				object = this->objects.begin() + index;
			}
		}
		else
			++object;
	}
}


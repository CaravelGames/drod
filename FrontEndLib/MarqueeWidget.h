// $Id$

/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Deadly Rooms of Death.
 *
 * The Initial Developer of the Original Code is
 * Caravel Software.
 * Portions created by the Initial Developer are Copyright (C) 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s): Gerry Jo Jellestad (trick)
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef MARQUEEWIDGET_H
#define MARQUEEWIDGET_H

#include "Widget.h"
#include <BackEndLib/Assert.h>

#include <list>
#include <queue>

//work in progress

enum MP_Type
{
	MP_Widget = 0,   //A widget
	MP_Speed,        //Accelerate/decelerate to speed (linear)
	MP_AccelTicks    //Set accelerate/decelerate duration (in ms)
};

class CMarqueePart {
private:

	MP_Type  type;
	CWidget *pMarquee;
	union {
		CWidget *pWidget;
		UINT     wArg;
	};

	UINT wUseCount;
	bool bEnabled;
	bool bRemoved;

protected:
	friend class CMarqueeWidget;

	CMarqueePart (CWidget *pParent, CWidget *pSetWidget, bool bSetEnabled=true)
			: type(MP_Widget), pMarquee(pParent), pWidget(pSetWidget), wUseCount(0),
			  bEnabled(bSetEnabled), bRemoved(false)
			  { ASSERT(pSetWidget); pMarquee->AddWidget(pWidget); }
	CMarqueePart (CWidget *pParent, MP_Type setType, UINT wSetArg, bool bSetEnabled=true)
			: type(setType), pMarquee(pParent), wArg(wSetArg), wUseCount(0),
			  bEnabled(bSetEnabled), bRemoved(false) {}
	~CMarqueePart () { if (type == MP_Widget) pMarquee->RemoveWidget(pWidget); }

	UINT IncUse() { return ++wUseCount; }
	UINT DecUse() { ASSERT(wUseCount); return --wUseCount; }

public:
	MP_Type  GetType() const     { return type; }
	UINT     GetUseCount() const { return wUseCount; }
	CWidget* GetWidget() const   { ASSERT(type == MP_Widget); return pWidget; }
	UINT     GetWArg() const     { ASSERT(type != MP_Widget); return wArg; }
	UINT     GetW() const        { return (type == MP_Widget ? pWidget->GetW() : 0); }

	bool IsEnabled() const             { return bEnabled; }
	bool IsRemoved() const             { return bRemoved; }
	//A part should be considered deleted when it is removed.
	void Remove()                      { bRemoved = true; }
	void Enable(bool bSetEnabled=true) { bEnabled = bSetEnabled; }
	void Disable()                     { bEnabled = false; }
};

class CMarqueeWidget : public CWidget
{
public:
	CMarqueeWidget(const UINT dwSetTagNo, const int nSetX, const int nSetY,
			const UINT wSetW, const UINT wSetH, const UINT wSetSpeed);
	virtual ~CMarqueeWidget();

	CWidget* GetWidgetPart(const UINT dwTagNo);
	virtual CWidget* GetWidgetContainingCoords(const int nX, const int nY,
			const WIDGETTYPE eWidgetType) const;

	virtual void   HandleAnimate();
	virtual bool   IsAnimated() const {return true;}
	virtual void   Paint(bool bUpdateRect = true);
	virtual bool   ParentMustPaintChildren() const {return true;}
	void           Reset();

	CMarqueePart* AddPart(CWidget *pWidget);
	CMarqueePart* AddPart(MP_Type type, UINT arg);
	CMarqueePart* AddPart(CMarqueePart *pPart);
	void          RemoveParts();

protected:
	typedef std::list<CMarqueePart *>::iterator PART_ITERATOR;
	typedef std::deque<CMarqueePart *>::const_iterator ACTIVEPART_ITERATOR;

	void RemovePart(PART_ITERATOR& iPart);

	UINT              wLastTick;
	UINT              wSpeed;
	UINT              wSourceSpeed;
	UINT              wTargetSpeed;
	UINT              wSourceSpeedTick;
	UINT              wAccelTicks;
	int               nFirstPos;     //Fixed point for sub-pixel accuracy =)
	PART_ITERATOR     iFirstPart;

	std::list<CMarqueePart*>  Parts;
	std::deque<CMarqueePart*> ActiveParts;
};

#endif

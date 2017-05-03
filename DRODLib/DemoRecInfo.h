#ifndef DEMO_REC_INFO_H
#define DEMO_REC_INFO_H

#include "DbDemos.h"

//*******************************************************************************
//Type used for grouping all the demo recording vars together.
struct DEMO_REC_INFO
{
	DEMO_REC_INFO() { clear(); }

	void clear() {
		dwDescriptionMessageID=0;
		wBeginTurnNo=0;
		dwPrevDemoID=dwFirstDemoID=0;
		dwFlags=0;
	}

	UINT dwDescriptionMessageID; //Description of demo.
	UINT wBeginTurnNo;        //Turn at which recording began.
	UINT dwPrevDemoID;        //DemoID for a previous demo record with recording
									//for room visited before the current.  That record
									//may be updated to link to demo record storing recording
									//for current room.
	UINT dwFirstDemoID;       //DemoID of first demo in a series or demoID of the only
									//demo if there was just one.
	UINT dwFlags;          //explicit properties of demo

	void SetFlag(const CDbDemo::DemoFlag eFlag, const bool bSet=true)
	{
		ASSERT(eFlag < CDbDemo::MaxFlags);
		const UINT dwFlagVal = 1 << eFlag;
		if (bSet)
		{
			this->dwFlags |= dwFlagVal;   //set bit
		} else {
			this->dwFlags &= ~dwFlagVal;  //inverse mask to reset bit
		}
	}
};

//Auto-save option flags.
static const UINT ASO_NONE = 0L;
static const UINT ASO_CHECKPOINT = 1L;
static const UINT ASO_ROOMBEGIN = 2L;
static const UINT ASO_LEVELBEGIN = 4L;
static const UINT ASO_CONQUERDEMO = 8L;
static const UINT ASO_DIEDEMO = 16L;
static const UINT ASO_DEFAULT = ASO_CHECKPOINT | ASO_ROOMBEGIN | ASO_LEVELBEGIN;

static const UINT NO_CONQUER_TOKEN_TURN = UINT(-1);

#endif

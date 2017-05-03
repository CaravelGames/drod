#ifndef ENTRANCEDATA_H
#define ENTRANCEDATA_H

#include "DbMessageText.h"

#include <BackEndLib/Types.h>
#include <BackEndLib/Wchar.h>

#include <vector>

//*****************************************************************************
//A level entrance within a hold.
class CEntranceData
{
public:
	enum DescriptionDisplay
	{
		DD_No=0,
		DD_Always=1,
		DD_Once=2
	};

	CEntranceData()
		: dwEntranceID(0), dwRoomID(0), wX(0), wY(0), wO(0), bIsMainEntrance(false)
		, eShowDescription(DD_Always), dwDataID(0)
	{ }
	CEntranceData(const UINT dwEntranceID, const UINT dwDescriptionMessageID,
			const UINT dwRoomID, const UINT wX, const UINT wY, const UINT wO,
			const bool bIsMainEntrance, const DescriptionDisplay eShowDescription,
			const UINT dwDataID)
		: dwEntranceID(dwEntranceID), dwRoomID(dwRoomID), wX(wX), wY(wY), wO(wO)
		, bIsMainEntrance(bIsMainEntrance), eShowDescription(eShowDescription)
		, dwDataID(dwDataID)
	{if (dwDescriptionMessageID)
		this->DescriptionText.Bind(dwDescriptionMessageID);}
	~CEntranceData()
	{ }

	CEntranceData(const CEntranceData &Src) {SetMembers(Src);}
	CEntranceData& operator= (CEntranceData &Src)
	{
		SetMembers(Src);
		return *this;
	}

	WSTRING GetPositionDescription();

	void ReflectX(const UINT wWidth) {
		this->wX = (wWidth-1) - this->wX;
		this->wO = nGetO(-nGetOX(this->wO),nGetOY(this->wO));
	}
	void ReflectY(const UINT wHeight) {
			this->wY = (wHeight-1) - this->wY;
			this->wO = nGetO(nGetOX(this->wO),-nGetOY(this->wO));
	}

	void SetMembers(const CEntranceData &Src, const bool bCopyLocalInfo=true);

	UINT       dwEntranceID;
	CDbMessageText DescriptionText;
	UINT       dwRoomID;
	UINT       wX, wY, wO;
	bool       bIsMainEntrance;
	DescriptionDisplay eShowDescription;
	UINT       dwDataID; //(audio: optional)
};

typedef std::vector<CEntranceData*> ENTRANCE_VECTOR;

#endif

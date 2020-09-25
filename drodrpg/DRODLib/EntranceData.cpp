#include "EntranceData.h"

#include "DbRooms.h"
#include "Db.h"

//*****************************************************************************
void CEntranceData::SetMembers(
	const CEntranceData &Src,
	const bool bCopyLocalInfo)  //[default=true]
{
	this->dwEntranceID = Src.dwEntranceID;
	this->dwRoomID = Src.dwRoomID;
	this->wX = Src.wX;
	this->wY = Src.wY;
	this->wO = Src.wO;
	this->bIsMainEntrance = Src.bIsMainEntrance;
	this->eShowDescription = Src.eShowDescription;

	if (bCopyLocalInfo)
	{
		//Don't make a duplicate copy of the text in DB.
		const UINT dwMessageID = Src.DescriptionText.GetMessageID();
		if (dwMessageID)
			this->DescriptionText.Bind(dwMessageID);
	}
	//Make a copy of the text string.
	this->DescriptionText = Src.DescriptionText;
}

//*****************************************************************************
WSTRING CEntranceData::GetPositionDescription() //can't be const
{
	WSTRING descText;

	CDbRoom *pRoom = g_pTheDB->Rooms.GetByID(this->dwRoomID, true);
	ASSERT(pRoom);
	if (!pRoom) return descText;
	CDbLevel *pLevel = g_pTheDB->Levels.GetByID(pRoom->dwLevelID);
	ASSERT(pLevel);
	if (!pLevel) {delete pRoom; return descText;}

	//Level name.
	descText += pLevel->NameText;
	descText += wszColon;
	descText += wszSpace;

	//Room name.
	WSTRING abbrevRoomPosition;
	pRoom->GetLevelPositionDescription(abbrevRoomPosition, true);
	descText += abbrevRoomPosition;
	descText += wszColon;
	descText += wszSpace;

	//Entrance description.
	descText += this->DescriptionText; //can't be const

	delete pLevel;
	delete pRoom;

	return descText;
}

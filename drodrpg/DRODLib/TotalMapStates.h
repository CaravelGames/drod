#ifndef TOTALMAPSTATES_H
#define TOTALMAPSTATES_H

#include "DbSavedGames.h"
#include <BackEndLib/Types.h>

#include <map>

//*******************************************************************************
class CTotalMapStates {
public:
	CTotalMapStates() { }
	~CTotalMapStates() { }
	void     Load(const UINT dwPlayerID, const UINT dwLevelID);
	MapState GetStoredMapStateForRoom(const UINT roomID) const;
	void     Update(const UINT roomID, const MapState state, const bool bNoSaves);
	void     Update(const CIDSet& roomIDs, const MapState state, const bool bNoSaves);

private:
	bool          bLevelIsFullyExplored = false;
	RoomMapStates mapStates;

	bool UpdateRoom(const UINT roomID, const MapState state);
	void Save() const;
};

#endif

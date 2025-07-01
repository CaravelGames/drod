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
	void Load(const UINT dwPlayerID, const UINT dwHoldID);
	const MapState GetStoredMapStateForRoom(const UINT roomID);
	void Update(const UINT roomID, const MapState state, const bool bNoSaves);
	void Update(const CIDSet roomIDs, const MapState state, const bool bNoSaves);

private:
	map<UINT, MapState> mapStates;

	bool UpdateRoom(const UINT roomID, const MapState state);
	void Save();
};

#endif

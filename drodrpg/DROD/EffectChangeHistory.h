// $Id: EffectChangeHistory.h $

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
 * Portions created by the Initial Developer are Copyright (C) 2007, 2023
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifndef EFFECT_CHANGE_HISTORY_H
#define EFFECT_CHANGE_HISTORY_H

#include <BackEndLib/Types.h>
#include <vector>

//******************************************************************************
//Class to record when changes to effects that require specific refreshing are made.
//When the game goes "back in time", these effects can then be downdated.
//It is assumed that adding a record means that any records in its future are now invalid
class EffectChangeHistory {
public:
	EffectChangeHistory() = default;
	~EffectChangeHistory() = default;

	bool isAfterLatest(UINT turn) {
		return turn > changeTurns.back();
	}

	//Remove all records after a given turn.
	void removeAfter(UINT turn) {
		while (!empty() && changeTurns.back() >= turn) {
			changeTurns.pop_back();
		}
	}

	//Add a new record. Any records from later turns are removed.
	void add(UINT turn) {
		removeAfter(turn);
		changeTurns.push_back(turn);
	}

	void reset() {
		changeTurns.clear();
		changeTurns.push_back(0);
	}

	bool empty() {
		return changeTurns.empty();
	}

private:
	std::vector<UINT> changeTurns;
};
#endif // !EFFECT_CHANGE_HISTORY_H

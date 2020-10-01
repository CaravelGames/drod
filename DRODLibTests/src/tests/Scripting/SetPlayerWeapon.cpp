#include "../../test-include.hpp"

TEST_CASE("Scripting: Set player weapon", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Should allow _MyScriptX to set wweapon type"){
		CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_VarSet, ScriptVars::P_SCRIPT_X, ScriptVars::Assign, WT_Staff);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_SetPlayerWeapon, WT_Dagger);

		CCurrentGame* pGame = Runner::StartGame(10, 10, N);
		
		REQUIRE(pGame->swordsman.weaponType == WT_Staff);
	}
}

#include "../../../test-include.hpp"

TEST_CASE("Beethro pushing an entity which pushes him back doesn't prevent hot tile damage", "[game]") {
	RoomBuilder::ClearRoom();

	CCharacter* character = RoomBuilder::AddVisibleCharacter(11, 10, SW, M_CLONE);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_SetPlayerWeapon, WT_Staff);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_VarSet, ScriptVars::P_MONSTER_WEAPON, ScriptVars::Assign, WT_Staff);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_TurnIntoMonster);

	RoomBuilder::PlotRect(T_HOT, 10, 10, 10, 10);

	CCurrentGame* game = Runner::StartGame(10, 10, SE);

	SECTION("Beethro should stay alive"){
		Runner::ExecuteCommand(CMD_CC);
		REQUIRE(game->GetDyingEntity() == NULL);
	}
}

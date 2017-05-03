#include "../../../test-include.hpp"

TEST_CASE("Scripting: Building bombs", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Built bomb should hold down pressure plate"){
		RoomBuilder::Plot(T_PRESSPLATE, 10, 10);
		RoomBuilder::Plot(T_DOOR_YO, 20, 10);
		COrbData* pOrbData = RoomBuilder::AddOrbDataToTile(10, 10, OT_NORMAL);
		pOrbData->AddAgent(20, 10, OA_CLOSE);

		CCharacter* pScript = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Wait, 1);
		RoomBuilder::AddCommand(pScript, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_BOMB);

		CCurrentGame* pGame = Runner::StartGame(5, 5);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		REQUIRE(pGame->pRoom->GetOSquare(20, 10) == T_DOOR_Y);
	}
}

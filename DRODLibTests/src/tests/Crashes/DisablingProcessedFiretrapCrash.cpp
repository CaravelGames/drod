#include "../../catch.hpp"
#include "../../CTestDb.h"
#include "../../Runner.h"
#include "../../RoomBuilder.h"
#include "../../../../DRODLib/CurrentGame.h"
#include "../../../../DRODLib/Character.h"
#include "../../../../DRODLib/CharacterCommand.h"

#include <vector>
using namespace std;

TEST_CASE("Disabling currently processed firetrap should no longer crash the game", "[game]") {
	RoomBuilder::ClearRoom();

	SECTION("Make sure bomb was fired and do not crash!"){
		RoomBuilder::Plot(T_FIRETRAP, 10, 10);
		RoomBuilder::Plot(T_FIRETRAP, 11, 10);
		RoomBuilder::Plot(T_BOMB, 10, 10);

		RoomBuilder::Plot(T_FLOOR, 8, 10);
		RoomBuilder::Plot(T_FLOOR, 9, 10);
		RoomBuilder::Plot(T_ORB, 9, 10);

		COrbData* orbData = RoomBuilder::AddOrbDataToTile(9, 10, OT_NORMAL);
		orbData->AddAgent(10, 10, OA_TOGGLE);
		orbData->AddAgent(11, 10, OA_TOGGLE);

		CCurrentGame* game = Runner::StartGame(8, 10, NE);
		Runner::ExecuteCommand(CMD_C);
		REQUIRE(game->pRoom->GetTSquare(10, 10) == T_EMPTY);
	}
}

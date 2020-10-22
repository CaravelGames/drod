#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Scripting: Build fuse", "[game][player moves][beethro][scripting][imperative][push]") {
	RoomBuilder::ClearRoom();

	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait);

	SECTION("Building fuse on a mirror should keep the mirror") {
		RoomBuilder::Plot(T_MIRROR, 10, 10);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_FUSE);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertTile(10, 10, T_MIRROR);
		AssertTile(10, 10, T_FUSE);
	}

	SECTION("Building fuse on a keg should keep the mirror") {
		RoomBuilder::Plot(T_POWDER_KEG, 10, 10);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_FUSE);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertTile(10, 10, T_POWDER_KEG);
		AssertTile(10, 10, T_FUSE);
	}
}

#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Scripting: Build mirrors", "[game][player moves][beethro][scripting][imperative][push]") {
	RoomBuilder::ClearRoom();

	CCharacter* character = RoomBuilder::AddVisibleCharacter(1, 1);
	RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait);

	SECTION("Building mirror on top of a keg should remove the keg") {
		RoomBuilder::Plot(T_POWDER_KEG, 10, 10);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_MIRROR);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertNoTile(10, 10, T_POWDER_KEG);
		AssertTile(10, 10, T_MIRROR);
	}

	SECTION("Building mirror on top of a fuse should keep the fuse") {
		RoomBuilder::Plot(T_FUSE, 10, 10);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_MIRROR);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertTile(10, 10, T_FUSE);
		AssertTile(10, 10, T_MIRROR);
	}

	SECTION("Building mirror on top of a scroll should keep the scroll") {
		RoomBuilder::Plot(T_SCROLL, 10, 10);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_MIRROR);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertTile(10, 10, T_SCROLL);
		AssertTile(10, 10, T_MIRROR);
	}

	SECTION("Building mirror on top of a token should keep the token") {
		RoomBuilder::PlotToken(PowerTarget, 10, 10);
		RoomBuilder::AddCommand(character, CCharacterCommand::CC_Build, 10, 10, 0, 0, T_MIRROR);

		CCurrentGame* game = Runner::StartGame(15, 15, N);
		Runner::ExecuteCommand(CMD_WAIT, 1);

		AssertTile(10, 10, T_TOKEN);
		AssertTile(10, 10, T_MIRROR);
	}
}

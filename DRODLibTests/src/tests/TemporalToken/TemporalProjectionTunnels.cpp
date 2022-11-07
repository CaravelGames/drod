#include "../../test-include.hpp"

TEST_CASE("Temporal Projection interaction with tunnels", "[game][time token]") {
	RoomBuilder::ClearRoom();

	SECTION("Projection uses tunnel") {
		RoomBuilder::PlotToken(RoomTokenType::TemporalSplit, 10, 10);
		RoomBuilder::Plot(T_TUNNEL_E, 11, 10);
		RoomBuilder::Plot(T_TUNNEL_W, 20, 10);

		CCurrentGame* game = Runner::StartGame(9, 10, N);
		Runner::ExecuteCommand(CMD_E, 3);
		Runner::ExecuteCommand(CMD_CLONE);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		// Temporal Projection should travel through tunnel
		AssertMonsterType(20, 10, M_TEMPORALCLONE);
	}

	SECTION("Projection tunnel use not blocked by adjacent arrow") {
		RoomBuilder::PlotToken(RoomTokenType::TemporalSplit, 10, 10);
		RoomBuilder::Plot(T_TUNNEL_E, 11, 10);
		RoomBuilder::Plot(T_TUNNEL_W, 20, 10);
		RoomBuilder::Plot(T_ARROW_W, 12, 10);

		CCurrentGame* game = Runner::StartGame(9, 10, N);
		Runner::ExecuteCommand(CMD_E, 3);
		Runner::ExecuteCommand(CMD_CLONE);
		Runner::ExecuteCommand(CMD_WAIT, 2);

		// Temporal Projection should travel through tunnel
		AssertMonsterType(20, 10, M_TEMPORALCLONE);
	}
}
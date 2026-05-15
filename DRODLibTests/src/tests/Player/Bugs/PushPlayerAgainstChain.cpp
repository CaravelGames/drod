#include "../../../test-include.hpp"

TEST_CASE("Players shouldn't be pushed over Gentryii chains", "[game][player moves][beethro][push][bug]") {
    RoomBuilder::ClearRoom();

    SECTION("Player should not be pushed") {
        RoomBuilder::AddLongMonster(M_GENTRYII, 10, 10, SE)
            .GrowTo(6, 6).End();

        RoomBuilder::Plot(T_WALL, 9, 9);
        RoomBuilder::AddMonsterWithWeapon(M_GUARD, WeaponType::WT_Staff, 5, 6, S);
        CCurrentGame* game = Runner::StartGame(6, 7, S);
        Runner::ExecuteCommand(CMD_WAIT, 1);

        REQUIRE(game->swordsman.wX == 6);
        REQUIRE(game->swordsman.wY == 7);
    }
}

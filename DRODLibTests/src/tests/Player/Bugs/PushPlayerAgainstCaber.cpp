#include "../../../test-include.hpp"
#include "../../../CAssert.h"

TEST_CASE("Pushing player against caber", "[game][player moves][beethro][push][bug]") {
    RoomBuilder::ClearRoom();
    RoomBuilder::AddMonsterWithWeapon(M_DECOY, WT_Caber, 8, 10, E);

    //......
    //...M.. - Mimic facing SE
    //.D-P\. - Decoy with caber facing E, player facing S
    //...|.. - will rotate CW to push player into caber

    SECTION("Player should not be pushed by staff") {
        RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Staff, 10, 9, SE);

        CCurrentGame* game = Runner::StartGame(10, 10, S);
        Runner::ExecuteCommand(CMD_C);

        AssertPlayerIsAlive();
        AssertPlayerAt(10, 10);
    }
    SECTION("Player should not be pushed by staff") {
        RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Staff, 10, 9, SE);

        CCurrentGame* game = Runner::StartGame(10, 10, S);
        Runner::ExecuteCommand(CMD_C);

        AssertPlayerIsAlive();
        AssertPlayerAt(10, 10);
    }
    SECTION("Player should be killed by caber") {
        RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Caber, 10, 9, SE);

        CCurrentGame* game = Runner::StartGame(10, 10, S);
        Runner::ExecuteCommand(CMD_C);

        AssertPlayerIsDead();
    }

    //....... 
    //.D-.PM. - Decoy with caber facing E, player facing S, Mimic with pickaxe facing W on top of the player
    //....|.. - will step W to push player into caber

    SECTION("Player should not killed by pickaxe") {
        RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WT_Pickaxe, 12, 10, W);

        CCurrentGame* game = Runner::StartGame(11, 10, S);
        Runner::ExecuteCommand(CMD_W);

        AssertPlayerIsAlive();
        AssertPlayerAt(10, 10);
    }
}

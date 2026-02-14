#include "../../../test-include.hpp"
#include <DRODLibTests/src/CAssert.h>

TEST_CASE("Wait for Weapon script command", "[game][scripting][weapon]") {
    SECTION("Check for any weapon") {
        CCharacter *character = RoomBuilder::AddVisibleCharacter(0, 0);

        RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForWeapon, 20, 10);
        RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, WS("Sword"));

        RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait, 1);
        RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForWeapon, 20, 10);
        RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, WS("Stick"));

        RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait, 1);
        RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForWeapon, 20, 10);
        RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, WS("Spear"));

        RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait, 1);
        RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForWeapon, 20, 10);
        RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, WS("Pick"));

        RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait, 1);
        RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForWeapon, 20, 10);
        RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, WS("Caber"));

        RoomBuilder::AddCommand(character, CCharacterCommand::CC_Wait, 1);
        RoomBuilder::AddCommand(character, CCharacterCommand::CC_WaitForWeapon, 20, 10);
        RoomBuilder::AddCommand(character, CCharacterCommand::CC_ChallengeCompleted, 0, 0, 0, 0, 0, WS("Dagger"));

        RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WeaponType::WT_Sword, 19, 11, N);
        RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WeaponType::WT_Staff, 17, 11, N);
        RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WeaponType::WT_Spear, 15, 11, N);
        RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WeaponType::WT_Pickaxe, 13, 11, N);
        RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WeaponType::WT_Caber, 11, 11, N);
        RoomBuilder::AddMonsterWithWeapon(M_MIMIC, WeaponType::WT_Dagger, 9, 10, N);

        CCueEvents CueEvents;
        CCurrentGame* game = Runner::StartGame(1, 15, S, CueEvents);
        Runner::ExecuteCommand(CMD_E, CueEvents); // Sword
        AssertEvent(CID_ChallengeCompleted);

        Runner::ExecuteCommand(CMD_E, 2); // Stick
        AssertEvent(CID_ChallengeCompleted);

        Runner::ExecuteCommand(CMD_E, 2); // Spear
        AssertEvent(CID_ChallengeCompleted);

        Runner::ExecuteCommand(CMD_E, 2); // Pick
        AssertEvent(CID_ChallengeCompleted);

        Runner::ExecuteCommand(CMD_E, 2); // Caber
        AssertEvent(CID_ChallengeCompleted);

        Runner::ExecuteCommand(CMD_E); // Dagger
        AssertEvent(CID_ChallengeCompleted);
    }
}

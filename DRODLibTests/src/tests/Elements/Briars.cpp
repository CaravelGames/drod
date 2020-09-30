#include "../../catch.hpp"
#include "../../CTestDb.h"
#include "../../CAssert.h"
#include "../../Runner.h"
#include "../../RoomBuilder.h"

#include <vector>
using namespace std;

TEST_CASE("Briars", "[game][elements]") {
	RoomBuilder::ClearRoom();
	
	SECTION("Components should not be connected through another component while recalculating") {
		// ....B.... - Bomb exploded by script on turn 3
		// .wwwwwww. - withered briar that's not part of the root below
		// .........
		// Rwwwwwww. - root with some withered briar
		// .........
		// ....B.... - Bomb exploded by script on turn 2

		// Refer to this thred for more details: http://forum.caravelgames.com/viewtopic.php?TopicID=38484
		// In short, there was a bug where during briar components' recalculation it would use a simple GetConnected8NeighborTiles
		// call, but then only actually use the briar tiles that were part of the component before recalculation. Meaning it
		// could use briar tiles to connect to another lump of briar without that briar being part of itself. And then when 
		// such disconnected briar was destroyed it did not force another reevaluation

		RoomBuilder::Plot(T_BRIAR_SOURCE, 10, 10);
		RoomBuilder::Plot(T_BOMB, 14, 7);
		RoomBuilder::Plot(T_BOMB, 14, 12);
		RoomBuilder::PlotRect(T_BRIAR_DEAD, 11, 8, 17, 8);
		RoomBuilder::PlotRect(T_BRIAR_DEAD, 11, 10, 18, 10);

		CCharacter* pCharacter = RoomBuilder::AddCharacter(1, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Wait, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_AttackTile, 14, 12, 0, 0, ScriptFlag::AT_Stab);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_Wait, 1);
		RoomBuilder::AddCommand(pCharacter, CCharacterCommand::CC_AttackTile, 14, 7, 0, 0, ScriptFlag::AT_Stab);

		CCurrentGame* pGame = Runner::StartGame(20, 20, N);
		Runner::ExecuteCommand(CMD_WAIT, 6);

		AssertTile(8, 8, T_BRIAR_LIVE);
		AssertNoTile(17, 8, T_BRIAR_LIVE);
		AssertNoTile(17, 8, T_BRIAR_DEAD);
	}
}

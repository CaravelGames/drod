// ReSharper disable CppExpressionWithoutSideEffects
#include "../../test-include.hpp"
#include "../../CharacterBuilder.h"

TEST_CASE("Scripting: Self kill crash", "[game]")
{
	// A bunch of test cases that verify no asserts or segfaults when processing a character who
	// kills itself by a command and then continues running further commands
	// REFERENCE: https://forum.caravelgames.com/viewtopic.php?TopicID=47382
	// The best way found to achieve the segfault is to move the dead character onto a pressure
	// plate which then segfaults when trying to release that plate and finds deleted monster
	// in the MOnsters array

	// BASE SETUP:
	// C.  <- Character (10,10)
	// .P  <- Pressure plate (11,11)
	RoomBuilder::ClearRoom();
	RoomBuilder::PlotPlate(11,11, OrbType::OT_NORMAL);
	auto characterScript = CharacterBuilder::Create(10, 10, 7, M_ROACH)
		.Appear()
		.Wait(0);

	SECTION("Attack Tile")
	{
		characterScript.AttackTile(10, 10, ScriptFlag::AT_Stab);
		characterScript.Move(1, 1);
		Runner::StartGame(20, 20);
		Runner::ExecuteCommand(CMD_WAIT, 3);
	}
	SECTION("Push")
	{
		RoomBuilder::Plot(T_PIT, 11, 10);
		characterScript.Imperative(ScriptFlag::Imperative::NotStunnable);
		characterScript.PushTile(10, 10, E);
		characterScript.Move(0, 1);
		Runner::StartGame(20, 20);
		Runner::ExecuteCommand(CMD_WAIT, 3);
	}
	SECTION("Build")
	{
		characterScript.Build(10, 10, T_PIT);
		characterScript.Move(1, 1);
		Runner::StartGame(20, 20);
		Runner::ExecuteCommand(CMD_WAIT, 3);
	}
	SECTION("Player stab")
	{
		characterScript.TeleportPlayerTo(9, 9);
		characterScript.Move(1, 1);
		Runner::StartGame(20, 20, SE);
		Runner::ExecuteCommand(CMD_WAIT, 3);
	}
	SECTION("Player stab (via var set)")
	{
		characterScript.SetVar(ScriptVars::P_PLAYER_X, ScriptVars::Assign, 9);
		characterScript.SetVar(ScriptVars::P_PLAYER_Y, ScriptVars::Assign, 9);
		characterScript.Move(1, 1);
		Runner::StartGame(20, 20, SE);
		Runner::ExecuteCommand(CMD_WAIT, 3);
	}
}

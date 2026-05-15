#include "RoomBuilder.h"
#include "CharacterBuilder.h"

#include <memory>

#include "catch.hpp"

CharacterBuilder::CharacterBuilder(const UINT wX, const UINT wY, const UINT wO,
                                   const MONSTERTYPE identity)
{
    this->pCharacter = RoomBuilder::AddCharacter(wX, wY, wO, identity);
}

CharacterBuilder::CharacterBuilder(CCharacter* pCharacter)
    : pCharacter(pCharacter)
{
}

CharacterBuilder CharacterBuilder::Create(const UINT wX, const UINT wY, const UINT wO,
                                          const MONSTERTYPE identity)
{
    return CharacterBuilder(wX, wY, wO, identity);
}

CharacterBuilder CharacterBuilder::Appear() const
{
    RoomBuilder::AddCommand(this->pCharacter, CCharacterCommand::CC_Appear);
    return CharacterBuilder(this->pCharacter);
}

CharacterBuilder CharacterBuilder::AttackTile(
    const UINT wX, const UINT wY,
    const ScriptFlag::AttackTileType eAttackTileType) const
{
    RoomBuilder::AddCommand(this->pCharacter, CCharacterCommand::CC_AttackTile, wX, wY, 0, 0,
                            eAttackTileType);
    return CharacterBuilder(this->pCharacter);
}

CharacterBuilder CharacterBuilder::Build(const UINT wX, const UINT wY,
                                         const UINT wTileType) const
{
    return Build(wX, wY, 0, 0, wTileType);
}

CharacterBuilder CharacterBuilder::Build(const UINT wX, const UINT wY, const UINT wW, const UINT wH,
                                         const UINT wTileType) const
{
    RoomBuilder::AddCommand(this->pCharacter, CCharacterCommand::CC_Build, wX, wY, wW, wH,
                            wTileType);
    return CharacterBuilder(this->pCharacter);
}

CharacterBuilder CharacterBuilder::Imperative(
    const ScriptFlag::Imperative eImperative) const
{
    RoomBuilder::AddCommand(this->pCharacter, CCharacterCommand::CC_Imperative, eImperative);
    return CharacterBuilder(this->pCharacter);
}

CharacterBuilder CharacterBuilder::Move(const UINT wDeltaX, const UINT wDeltaY,
                                        const bool bAllowTurning, const bool bIsSingleStep) const
{
    RoomBuilder::AddCommand(this->pCharacter, CCharacterCommand::CC_MoveRel, wDeltaX, wDeltaY,
                            bAllowTurning, bIsSingleStep);
    return CharacterBuilder(this->pCharacter);
}

CharacterBuilder CharacterBuilder::PushTile(const UINT wX, const UINT wY, const UINT wPushO) const
{
    RoomBuilder::AddCommand(this->pCharacter, CCharacterCommand::CC_PushTile, wX, wY, wPushO);
    return CharacterBuilder(this->pCharacter);
}

CharacterBuilder CharacterBuilder::SetVar(
    const ScriptVars::Predefined eVar,
    const ScriptVars::Op eOp,
    const UINT wValue) const
{
    REQUIRE((eOp == ScriptVars::Op::Assign || eOp == ScriptVars::Op::Dec
        || eOp == ScriptVars::Op::DivideBy || eOp == ScriptVars::Op::Inc
        || eOp == ScriptVars::Op::Mod || eOp == ScriptVars::Op::MultiplyBy));

    RoomBuilder::AddCommand(this->pCharacter, CCharacterCommand::CC_VarSet, eVar, eOp, wValue);
    return CharacterBuilder(this->pCharacter);
}

CharacterBuilder CharacterBuilder::SetVar(
    const ScriptVars::Predefined eVar,
    const ScriptVars::Op eOp,
    const WSTRING& wLabelOrExpression) const
{
    RoomBuilder::AddCommand(this->pCharacter, CCharacterCommand::CC_VarSet, eVar, eOp, 0, 0, 0,
                            wLabelOrExpression);
    return CharacterBuilder(this->pCharacter);
}

CharacterBuilder CharacterBuilder::SetVar(
    const ScriptVars::Predefined eVar,
    const ScriptVars::Op eOp,
    const char* wLabelOrExpression) const
{
    RoomBuilder::AddCommand(this->pCharacter, CCharacterCommand::CC_VarSet, eVar, eOp, 0, 0, 0,
                            wLabelOrExpression);
    return CharacterBuilder(this->pCharacter);
}

CharacterBuilder CharacterBuilder::TeleportPlayerTo(const UINT wX, const UINT wY) const
{
    RoomBuilder::AddCommand(this->pCharacter, CCharacterCommand::CC_TeleportPlayerTo, wX, wY);
    return CharacterBuilder(this->pCharacter);
}

CharacterBuilder CharacterBuilder::Wait(const UINT wTurns) const
{
    RoomBuilder::AddCommand(this->pCharacter, CCharacterCommand::CC_Wait, wTurns);
    return CharacterBuilder(this->pCharacter);
}

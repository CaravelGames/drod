#ifndef CHARACTER_BUILDER_H
#define CHARACTER_BUILDER_H

#include <memory>

#include "CTestDb.h"
#include "../../DRODLib/CharacterCommand.h"
#include <BackEndLib/Types.h>

class CharacterBuilder
{
public:
    static CharacterBuilder Create(UINT wX, UINT wY, UINT wO = 8, MONSTERTYPE identity = M_ROACH);
    CharacterBuilder Appear() const;
    CharacterBuilder AttackTile(UINT wX, UINT wY,
                                ScriptFlag::AttackTileType eAttackTileType) const;
    CharacterBuilder Build(UINT wX, UINT wY, UINT wTileType) const;
    CharacterBuilder Build(UINT wX, UINT wY, UINT wW, UINT wH, UINT wTileType) const;
    CharacterBuilder Imperative(ScriptFlag::Imperative eImperative) const;
    CharacterBuilder Move(UINT wDeltaX, UINT wDeltaY, bool bAllowTurning = false,
                          bool bIsSingleStep = false) const;
    CharacterBuilder PushTile(UINT wX, UINT wY, UINT wPushO) const;
    /// For simple numeric var sets
    CharacterBuilder SetVar(ScriptVars::Predefined eVar, ScriptVars::Op eOp, UINT wValue) const;
    /// For text or expressions (using WSTRING)
    CharacterBuilder SetVar(ScriptVars::Predefined eVar, ScriptVars::Op eOp,
                            const WSTRING& wLabelOrExpression) const;
    /// For text or expressions (using char*)
    CharacterBuilder SetVar(ScriptVars::Predefined eVar, ScriptVars::Op eOp,
                            const char* wLabelOrExpression) const;
    CharacterBuilder TeleportPlayerTo(UINT wX, UINT wY) const;
    CharacterBuilder Wait(UINT wTurns = 0) const;

private:
    explicit CharacterBuilder(UINT wX, UINT wY, UINT wO = 8, MONSTERTYPE identity = M_ROACH);
    // calls AddCharacter
    explicit CharacterBuilder(CCharacter* pCharacter);
    CCharacter* pCharacter;
};

#endif

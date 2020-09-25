// $Id: GameScreenDummy.h 8118 2007-08-18 20:33:50Z mrimer $

#ifndef GAMESCREEN_DUMMY_H
#define GAMESCREEN_DUMMY_H

//Used to avoid including several files in DROD non-essential to DRODUtil.

#include "../DROD/GameScreen.h"

SCREENTYPE CGameScreen::ProcessCommand(const int nCommand, const UINT wX, const UINT wY) {return (SCREENTYPE)0;}
void CGameScreen::ShowStatsForMonsterAt(const UINT wX, const UINT wY) {}
void CGameScreen::ShowStatsForMonster(CMonster*) {}

#endif

// $Id$

#ifndef GAMESCREEN_DUMMY_H
#define GAMESCREEN_DUMMY_H

//Used to avoid including several files in DROD non-essential to DRODUtil.

#ifndef __GNUC__
//gcc is picky

#include "../DROD/GameScreen.h"

SCREENTYPE CGameScreen::ProcessCommand(const int nCommand, const UINT wX, const UINT wY) {return (SCREENTYPE)0;}

#endif

#endif

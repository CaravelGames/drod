// $Id$

/* ***** BEGIN LICENSE BLOCK *****
* Version: MPL 1.1
*
* The contents of this file are subject to the Mozilla Public License Version
* 1.1 (the "License"); you may not use this file except in compliance with
* the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
* for the specific language governing rights and limitations under the
* License.
*
* The Original Code is Deadly Rooms of Death.
*
* The Initial Developer of the Original Code is
* Caravel Software.
* Portions created by the Initial Developer are Copyright (C) 1995, 1996,
* 1997, 2000, 2001, 2002, 2005, 2015 Caravel Software. All Rights Reserved.
*
* Contributor(s):
* Maurycy Zarzycki (skell)
*
* ***** END LICENSE BLOCK ***** */

#ifndef EQUATION_PARSER_H
#define EQUATION_PARSER_H

#include "Equation.h"
#include "CharacterCommand.h"
#include <vector>

class CCharacter;
class CCurrentGame;

class CEquationParser{
public:
	static CEquationExpression* extractExpression(const WCHAR *pwStr, UINT& index, const CCurrentGame *pGame, const bool bExpectCloseParen = false, const bool bExpectComma = false);
	static CEquationTerm* extractTerm(const WCHAR *pwStr, UINT& index, const CCurrentGame *pGame);
	static CEquationFactorBase* extractFactor(const WCHAR *pwStr, UINT& index, const CCurrentGame *pGame);
	static vector<CEquationExpression*>* generateExpressionCache(const CCurrentGame* pGame, const COMMAND_VECTOR *commands);
};

#endif
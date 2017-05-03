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
* 1997, 2000, 2001, 2002, 2005 Caravel Software. All Rights Reserved.
*
* Contributor(s):
*  - Maurycy Zarzycki (maurycy.zarzycki@mauft.com)
*
* ***** END LICENSE BLOCK ***** */

#include "Cache.h"
#include "Character.h"
#include "CurrentGame.h"
#include "Equation.h"
#include "EquationParser.h"

#include <map>
#include <vector>

//*****************************************************************************
CCache::~CCache()
{
	this->ClearCustomCharacterEquationCache();
}

//*****************************************************************************
void CCache::HandleBeforeHoldLoad()
{
	this->ClearCustomCharacterEquationCache();
}

//*****************************************************************************
vector<CEquationExpression*>* CCache::GetHoldCharacterEquations(const HoldCharacter* pHoldCharacter, const CCurrentGame* pGame)
{
	map<UINT, vector<CEquationExpression*>*>::iterator it = this->customCharacterEquationCache.find(pHoldCharacter->dwCharID);
	if (it == this->customCharacterEquationCache.end())
	{
		COMMAND_VECTOR commands;
//		CCharacter::LoadCommands(pHoldCharacter->ExtraVars, commands);
		vector<CEquationExpression*>* expressions = CEquationParser::generateExpressionCache(pGame, &commands);
		this->customCharacterEquationCache[pHoldCharacter->dwCharID] = expressions;

		return expressions;
	}
	else
	{
		return it->second;
	}
}

//*****************************************************************************
void CCache::ClearCustomCharacterEquationCache()
{
	for (map<UINT, vector<CEquationExpression*>*>::iterator it = this->customCharacterEquationCache.begin(); it != this->customCharacterEquationCache.end(); ++it)
	{
		vector<CEquationExpression*>* expressions = it->second;
		for (vector<CEquationExpression*>::iterator it2 = expressions->begin(); it2 != expressions->end(); ++it2)
		{
			CEquationExpression* expression = *it2;
			delete expression;
		}
		delete expressions;
	}
	this->customCharacterEquationCache.clear();
}
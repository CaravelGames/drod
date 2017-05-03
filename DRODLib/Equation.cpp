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
*
* ***** END LICENSE BLOCK ***** */

#include "Character.h"
#include "CurrentGame.h"
#include "Equation.h"

#include <vector>

//*****************************************************************************
CEquationExpression::CEquationExpression()
{
}

//*****************************************************************************
CEquationExpression::~CEquationExpression()
{
	for (vector<CEquationTerm*>::const_iterator iSeek = this->terms.begin();
		iSeek != this->terms.end(); ++iSeek)
	{
		delete *iSeek;
	}
	this->terms.clear();
	this->termIsSubtract.clear();
}

//*****************************************************************************
void CEquationExpression::addTerm(CEquationTerm* term, bool termIsSubtract)
{
	this->terms.push_back(term);
	this->termIsSubtract.push_back(termIsSubtract);
}

//*****************************************************************************
int CEquationExpression::resolve(const CCharacter* pNPC) const{
	int value = 0;
	for (UINT wIndex = 0; wIndex < this->terms.size(); ++wIndex)
	{
		CEquationTerm* term = this->terms[wIndex];
		if (this->termIsSubtract[wIndex])
		{
			value -= term->resolve(pNPC);
		}
		else
		{
			value += term->resolve(pNPC);
		}
	}

	return value;
}

//*****************************************************************************
CEquationTerm::CEquationTerm()
{
}

//*****************************************************************************
CEquationTerm::~CEquationTerm()
{
	for (vector<CEquationFactorBase*>::const_iterator iSeek = this->factors.begin();
		iSeek != this->factors.end(); ++iSeek)
	{
		delete *iSeek;
	}
	this->factors.clear();
	this->factorOperations.clear();
}

//*****************************************************************************
void CEquationTerm::addFactor(CEquationFactorBase* factor, EquationFactorOperation factorOperation)
{
	this->factors.push_back(factor);
	this->factorOperations.push_back(factorOperation);
}

//*****************************************************************************
int CEquationTerm::resolve(const CCharacter* pNPC) const {
	int value = 0;
	for (UINT wIndex = 0; wIndex < this->factors.size(); ++wIndex)
	{
		CEquationFactorBase* factor = this->factors[wIndex];
		switch (this->factorOperations[wIndex]){
			case(EquationFactorOperation::NoOperation):
				value = factor->resolve(pNPC);
				break;
			case(EquationFactorOperation::Divide) :
				value /= factor->resolve(pNPC);
				break;
			case(EquationFactorOperation::Multiply) :
				value *= factor->resolve(pNPC);
				break;
			case(EquationFactorOperation::Modulo) :
				value %= factor->resolve(pNPC);
				break;

		}
	}

	return value;
}

//*****************************************************************************
CEquationFactorBase::~CEquationFactorBase(){
	// No body in the base destructor
}

//*****************************************************************************
int CEquationFactorBase::resolve(const CCharacter* pNPC) const {
	return 0;
}

//*****************************************************************************
CEquationFactorNumber::CEquationFactorNumber(const int value)
:CEquationFactorBase(),
value(value)
{
}

//*****************************************************************************
int CEquationFactorNumber::resolve(const CCharacter* pNPC) const {
	return this->value;
}

//*****************************************************************************
CEquationFactorHoldVariable::CEquationFactorHoldVariable(const CCurrentGame* pGame, const char* varName)
:CEquationFactorBase(),
pGame(pGame),
varName(varName)
{
}

//*****************************************************************************
CEquationFactorHoldVariable::~CEquationFactorHoldVariable()
{
	delete this->varName;
	this->pGame = NULL;
}

//*****************************************************************************
int CEquationFactorHoldVariable::resolve(const CCharacter* pNPC) const{
	return this->pGame->stats.GetVar(this->varName, (int)0);
}

//*****************************************************************************
CEquationFactorLocalVariable::CEquationFactorLocalVariable(const WSTRING* varName)
:CEquationFactorBase(),
varName(varName)
{
}

//*****************************************************************************
CEquationFactorLocalVariable::~CEquationFactorLocalVariable()
{
	delete this->varName;
}

//*****************************************************************************
int CEquationFactorLocalVariable::resolve(const CCharacter* pNPC) const{
	return pNPC->getLocalVarInt(*(this->varName));
}

//*****************************************************************************
CEquationFactorPredefinedVariable::CEquationFactorPredefinedVariable(const ScriptVars::Predefined eVar)
:CEquationFactorBase(),
eVar(eVar)
{
}

//*****************************************************************************
CEquationFactorPredefinedVariable::~CEquationFactorPredefinedVariable()
{
}

//*****************************************************************************
int CEquationFactorPredefinedVariable::resolve(const CCharacter* pNPC) const{
	return pNPC->getPredefinedVarInt(this->eVar);
}

//*****************************************************************************
CEquationFactorNestedExpression::CEquationFactorNestedExpression(const CEquationExpression* pExpression)
:CEquationFactorBase(),
pExpression(pExpression)
{
}

//*****************************************************************************
CEquationFactorNestedExpression::~CEquationFactorNestedExpression(){
	delete this->pExpression;
}

//*****************************************************************************
int CEquationFactorNestedExpression::resolve(const CCharacter* pNPC) const{
	return this->pExpression->resolve(pNPC);
}

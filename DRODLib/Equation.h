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

#ifndef EQUATION_H
#define EQUATION_H

#include "PlayerStats.h"

#include <vector>

class CEquationExpression;
class CEquationFactorBase;
class CEquationFactorNumber;
class CEquationFactorHoldVariable;
class CEquationFactorLocalVariable;
class CEquationFactorPredefinedVariable;
class CEquationExpression;
class CEquationTerm;

class CCurrentGame;
class CCharacter;
class CEquationFactorBase{
public:
	virtual ~CEquationFactorBase();
	virtual int resolve(const CCharacter* pNPC) const;
};

class CEquationFactorNumber : public CEquationFactorBase
{
public:
	CEquationFactorNumber(const int value);
	int resolve(const CCharacter* pNPC) const;
private:
	const int value;
};

class CEquationFactorHoldVariable : public CEquationFactorBase
{
public:
	CEquationFactorHoldVariable(const CCurrentGame* pGame, const char* varName);
	~CEquationFactorHoldVariable();
	int resolve(const CCharacter* pNPC) const;
private:
	const char* varName;
	const CCurrentGame* pGame;
};

class CEquationFactorLocalVariable : public CEquationFactorBase
{
public:
	CEquationFactorLocalVariable(const WSTRING* varName);
	~CEquationFactorLocalVariable();
	int resolve(const CCharacter* pNPC) const;
private:
	const WSTRING* varName;
};

class CEquationFactorPredefinedVariable : public CEquationFactorBase
{
public:
	CEquationFactorPredefinedVariable(const ScriptVars::Predefined eVar);
	~CEquationFactorPredefinedVariable();
	int resolve(const CCharacter* pNPC) const;
private:
	const ScriptVars::Predefined eVar;
};

class CEquationFactorNestedExpression : public CEquationFactorBase
{
public:
	CEquationFactorNestedExpression(const CEquationExpression* pExpression);
	~CEquationFactorNestedExpression();
	int resolve(const CCharacter* pNPC) const;
private:
	const CEquationExpression* pExpression;
};

enum EquationFactorOperation{
	NoOperation = 0,
	Multiply = 1,
	Divide = 2,
	Modulo = 3
};

typedef std::vector<EquationFactorOperation> EQUATION_FACTOR_OPERATIONS;
typedef std::vector<CEquationFactorBase*> EQUATION_FACTORS;

class CEquationTerm{
public:
	CEquationTerm();
	~CEquationTerm();
	void addFactor(CEquationFactorBase* factor, EquationFactorOperation factorOperation);
	int resolve(const CCharacter* pNPC) const;

	EQUATION_FACTOR_OPERATIONS factorOperations;
	EQUATION_FACTORS factors;
};

typedef std::vector<bool> EQUATION_TERM_SIGNS;
typedef std::vector<CEquationTerm*> EQUATION_TERMS;

class CEquationExpression{
public:
	CEquationExpression();
	~CEquationExpression();
	void addTerm(CEquationTerm* term, bool termIsSubtract);
	int resolve(const CCharacter* pNPC) const;

	EQUATION_TERM_SIGNS termIsSubtract;
	EQUATION_TERMS terms;
};

#endif
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
#include "EquationParser.h"

#define SKIP_WHITESPACE(str, index) while (iswspace(str[index])) ++index

CEquationExpression* CEquationParser::extractExpression(const WCHAR *pwStr, UINT& index, const CCurrentGame *pGame, const bool bExpectCloseParen, const bool bExpectComma)
{
	CEquationExpression* expression = new CEquationExpression();

	bool bIsSubtract = false; //otherwise subtract
	ASSERT(pwStr);
	ASSERT(pGame);
	SKIP_WHITESPACE(pwStr, index);

	if (pwStr[index] == W_t('+'))
	{
		bIsSubtract = false;
		++index;
	}
	else if (pwStr[index] == W_t('-'))
	{
		bIsSubtract = true;
		++index;
	}

	CEquationTerm* term = extractTerm(pwStr, index, pGame);
	expression->addTerm(term, bIsSubtract);

	SKIP_WHITESPACE(pwStr, index);
	while (pwStr[index] != 0)
	{
		if (pwStr[index] == W_t('+'))
		{
			bIsSubtract = false;
			++index;
		}
		else if (pwStr[index] == W_t('-'))
		{
			bIsSubtract = true;
			++index;
		}
		else if (bExpectCloseParen && pwStr[index] == W_t(')')) //closing nested expression
		{
			return expression; //caller will parse the close paren
		}
		else if (bExpectComma && pwStr[index] == W_t(','))
		{
			return expression; //caller will parse the comma
		}
		else
		{
			//parse error -- return the current value
			return expression;
		}

		CEquationTerm* term = extractTerm(pwStr, index, pGame);
		expression->addTerm(term, bIsSubtract);
	}

	return expression;
}

CEquationTerm* CEquationParser::extractTerm(const WCHAR *pwStr, UINT& index, const CCurrentGame *pGame)
{
	CEquationTerm* term = new CEquationTerm();
	term->addFactor(extractFactor(pwStr, index, pGame), EquationFactorOperation::NoOperation);

	while (pwStr[index] != 0)
	{
		//Parse another term.
		SKIP_WHITESPACE(pwStr, index);

		EquationFactorOperation operation;
		if (pwStr[index] == W_t('*'))
		{
			operation = EquationFactorOperation::Multiply;
			++index;
		}
		else if (pwStr[index] == W_t('/'))
		{
			operation = EquationFactorOperation::Divide;
			++index;
		}
		else if (pwStr[index] == W_t('%'))
		{
			operation = EquationFactorOperation::Modulo;
			++index;
		}
		else {
			//no more factors in this term -- return result
			return term;
		}

		term->addFactor(extractFactor(pwStr, index, pGame), operation);
	}

	return term;
}

CEquationFactorBase* CEquationParser::extractFactor(const WCHAR *pwStr, UINT& index, const CCurrentGame *pGame)
{
	SKIP_WHITESPACE(pwStr, index);

	//A nested expression?
	if (pwStr[index] == W_t('('))
	{
		++index;
		CEquationExpression *expression = CEquationParser::extractExpression(pwStr, index, pGame, true);
		CEquationFactorNestedExpression *nestedExpression = new CEquationFactorNestedExpression(expression);

		SKIP_WHITESPACE(pwStr, index);
		if (pwStr[index] == W_t(')'))
			++index;
		else
		{
			//ERROR
		}
		return nestedExpression;
	}

	//Number?
	if (iswdigit(pwStr[index]))
	{
		int val = _Wtoi(pwStr + index);

		//Parse past digits.
		++index;
		while (iswdigit(pwStr[index]))
		{
			++index;
		}
		
		if (iswalpha(pwStr[index])) //i.e. of form <digits><alphas>
		{
			//Invalid var name -- skip to end of it and return zero value.
			while (CDbHold::IsVarCharValid(pwStr[index]))
			{
				++index;
			}
				
			// ERROR
			return new CEquationFactorNumber(0);
		}

		return new CEquationFactorNumber(val);
	}

	//Variable identifier?
	if (pwStr[index] == W_t('_') || iswalpha(pwStr[index]) || pwStr[index] == W_t('.')) //valid first char
	{
		//Find spot where var identifier ends.
		int endIndex = index + 1;
		int spcTrail = 0;
		while (CDbHold::IsVarCharValid(pwStr[endIndex]))
		{
			if (pwStr[endIndex] == W_t(' '))
				++spcTrail;
			else
				spcTrail = 0;
			++endIndex;
		}

		const WSTRING wVarName(pwStr + index, endIndex - index - spcTrail);
		index = endIndex;

		//Is it a predefined var?
		int val = 0;
		const ScriptVars::Predefined eVar = ScriptVars::parsePredefinedVar(wVarName);
		if (eVar != ScriptVars::P_NoVar)
		{
			if (!ScriptVars::IsStringVar(eVar)) {
				return new CEquationFactorPredefinedVariable(eVar);
			}
		}
		else if (ScriptVars::IsCharacterLocalVar(wVarName)) {
			return new CEquationFactorLocalVariable(new WSTRING(wVarName));
		}
		else {
			//Is it a local hold var?
//			char *varName = pGame->pHold->createVarAccessToken(wVarName.c_str()); //Commented out to avoid compilation problems
//			const UNPACKEDVARTYPE vType = pGame->stats.GetVarType(varName);
//			const bool bValidInt = vType == UVT_int || vType == UVT_uint || vType == UVT_unknown;
//			if (bValidInt){
//				return new CEquationFactorHoldVariable(pGame, varName);
//			}
		}

		return new CEquationFactorNumber(0);
	}

	// Function identifier?
	if (pwStr[index] == W_t('@'))
	{
		/*index++;
		return parseFunction(pwStr, index, pGame, pNPC);
		*/
	}

	//Invalid identifier
	//ERROR

	return new CEquationFactorNumber(0);
}


vector<CEquationExpression*>* CEquationParser::generateExpressionCache(const CCurrentGame* pGame, const COMMAND_VECTOR *commands)
{
	vector<CEquationExpression*>* expressions = new vector<CEquationExpression*>();
	if (commands != NULL)
	{
		expressions->resize(commands->size());

		for (std::vector<CCharacterCommand*>::size_type i = 0; i != commands->size(); i++) {
			CCharacterCommand command = (*commands)[i];
			if (command.command == CCharacterCommand::CC_VarSet)
			{
				UINT index = 0;
				(*expressions)[i] = extractExpression(command.label.c_str(), index, pGame, false, false);
			}
		}
	}

	return expressions;
}

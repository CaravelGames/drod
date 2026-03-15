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
* Portions created by the Initial Developer are Copyright (C) 2002, 2005
* Caravel Software. All Rights Reserved.
*
* Contributor(s): Schik
*
* ***** END LICENSE BLOCK ***** */

#include "PostData.h"

CPostData::CPostData()
	: pFormPostBegin(NULL)
	, pFormPostEnd(NULL)
{
}

CPostData::~CPostData()
{
	if (pFormPostBegin) {
		curl_formfree(pFormPostBegin);
	}
}

//***************************
void CPostData::Add(
//Adds a name/value pair to the POST data of the next request
//
//Params:
	const string& strName,          //(in) Name
	const CStretchyBuffer& value)   //(in) Value
{
	curl_formadd(&pFormPostBegin, &pFormPostEnd,
		CURLFORM_COPYNAME, strName.c_str(),
		CURLFORM_COPYCONTENTS, (char*)(BYTE*)value,
		CURLFORM_CONTENTSLENGTH, value.Size(),
		CURLFORM_CONTENTTYPE, "application/binary", 
		CURLFORM_END);
}

void CPostData::Add(
//Adds a name/value pair to the POST data of the next request
//
//Params:
	const string& strName,     //(in) Name
	const string& value)       //(in) Value
{
	curl_formadd(&pFormPostBegin, &pFormPostEnd,
		CURLFORM_COPYNAME, strName.c_str(),
		CURLFORM_COPYCONTENTS, value.c_str(),
		CURLFORM_CONTENTSLENGTH, value.length(),
		CURLFORM_END);
}

void CPostData::Add(
//Adds a name/value pair to the POST data of the next request
//
//Params:
	const string& strName,     //(in) Name
	const std::vector<string>& values)       //(in) Value
{
	UINT numValues = values.size();
	struct curl_forms* pForms = new struct curl_forms[numValues+1];

	for (UINT i=0; i<numValues; ++i)
	{
		pForms[i].option = CURLFORM_COPYCONTENTS;
		pForms[i].value  = values[i].c_str();
	}
	pForms[numValues].option = CURLFORM_END;

	curl_formadd(&pFormPostBegin, &pFormPostEnd,
		CURLFORM_COPYNAME, strName.c_str(),
		CURLFORM_ARRAY, pForms,
		CURLFORM_END);

	delete[] pForms;
}

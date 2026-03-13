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
{
	pMime = curl_mime_init(NULL);
}

CPostData::~CPostData()
{
	if (pMime) {
		curl_mime_free(pMime);
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
	curl_mimepart* part = curl_mime_addpart(pMime);
	curl_mime_name(part, strName.c_str());
	curl_mime_data(part, (const char*)(BYTE*)value, value.Size());
	curl_mime_type(part, "application/binary");
}

void CPostData::Add(
//Adds a name/value pair to the POST data of the next request
//
//Params:
	const string& strName,     //(in) Name
	const string& value)       //(in) Value
{
	curl_mimepart* part = curl_mime_addpart(pMime);
	curl_mime_name(part, strName.c_str());
	curl_mime_data(part, value.c_str(), value.length());
}

void CPostData::Add(
//Adds a name/value pair to the POST data of the next request
//
//Params:
	const string& strName,     //(in) Name
	const std::vector<string>& values)       //(in) Value
{
	for (const auto& value : values)
	{
		curl_mimepart* part = curl_mime_addpart(pMime);
		curl_mime_name(part, strName.c_str());
		curl_mime_data(part, value.c_str(), value.length());
	}
}

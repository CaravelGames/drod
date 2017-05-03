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

#ifndef POSTDATA_H
#define POSTDATA_H

#include <curl/curl.h>  //must come first
#include "StretchyBuffer.h"
#include <vector>
#include <string>

class CPostData
{
private:
	curl_httppost* pFormPostBegin;
	curl_httppost* pFormPostEnd;
public:
	CPostData();
	~CPostData();

	void Add(const string& strName, const CStretchyBuffer& value);
	void Add(const string& strName, const std::string& value);
	void Add(const string& strName, const std::vector<std::string>& values);

	const curl_httppost* PostData() const { return pFormPostBegin; }
};

#endif
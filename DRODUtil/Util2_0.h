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
 * Portions created by the Initial Developer are Copyright (C) 2003, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

//Util2_0.h
//Declarations for CUtil2_0.

//2.0 is unimplemented.  To work with 2.0 data, you would need to get a version of drodutil 
//from CVS that correspondeds to the 2.0 format that was distributed.  I've removed the 
//implementation to avoid confusion.  Also, long-term I would like to late-bind these CUtil-derived 
//objects to compatibility DLLs.

#ifndef UTIL2_0_H
#define UTIL2_0_H

#include "Util.h"

class CUtil2_0 : public CUtil
{
private:
	typedef map<string,UINT> ASSIGNEDMIDS;
public:
	CUtil2_0(const WCHAR* pszSetPath) : CUtil(v2_0, pszSetPath) { };
	
	//All unimplemented.  See notes above.  
};

#endif //...#ifndef UTIL2_0_H

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
 * Michael Welsh Duggan (md5i), JP Burford (jpburford), Rik Cookney (timeracer),
 * Mike Rimer (mrimer), Matt Schikore (schik)
 *
 * ***** END LICENSE BLOCK ***** */

//Util1_5.h
//Declarations for CUtil1_5.

//1.5 is unimplemented.  To work with 1.5 data, you would need to get a version of drodutil 
//from CVS that corresponded to the 1.5 format that was distributed.  I've removed the 
//implementation to avoid confusion.  Also, long-term I would like to late-bind these CUtil-derived 
//objects to compatibility DLLs.

#ifndef UTIL1_5_H
#define UTIL1_5_H

#include "Util.h"

class CUtil1_5 : public CUtil
{
public:
	CUtil1_5(const WCHAR* pszSetPath) : CUtil(v1_5, pszSetPath) { };
  
  //All unimplemented.  See notes above.  
};

#endif //...#ifndef V1_5_H

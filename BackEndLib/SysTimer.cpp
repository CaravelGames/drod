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
 * Portions created by the Initial Developer are Copyright (C) 2001, 2002, 2005
 * Caravel Software. All Rights Reserved.
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */

#ifdef WIN32
#  include <windows.h> //Should be first include.
#endif

#ifdef __sgi
#  include <sys/types.h>
#  include <sys/time.h>
#endif

#if defined(__linux__) || defined(__FreeBSD__) || defined(__APPLE__)
#  include <sys/time.h>
#endif

#include "Assert.h"
#include "SysTimer.h"

//********************************************************************************
UINT GetTicks()
//NOTE: Might have overflow problems, so don't use for absolute dates down to the millisecond.
{
#ifdef WIN32
	return GetTickCount();
#elif defined(__sgi)
	return 0;
#elif defined (__linux__) || defined (__FreeBSD__) || defined (__APPLE__)
	struct timeval tv;
	gettimeofday (&tv, NULL);
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#else
#  error System tick count code not provided.
	return 0;
#endif
}

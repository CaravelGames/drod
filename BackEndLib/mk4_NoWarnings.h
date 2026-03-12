#ifndef __MK4_NO_WARNINGS_H__
#define __MK4_NO_WARNINGS_H__

// The sole purpose of this file is to import mk4.h while also ignoring all
// warnings within it.

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wparentheses"
#pragma GCC diagnostic ignored "-Wdeprecated-copy"

#include <mk4.h>

#pragma GCC diagnostic pop

#endif // __MK4_NO_WARNINGS_H__

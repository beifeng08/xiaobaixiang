/*----------------------------------------------------------------------
| gwcommon.h
|
| This file contains handy #defines that I use in all my projects
| 
|  Copyright 2005-2021 Mersenne Research, Inc.  All rights reserved.
+---------------------------------------------------------------------*/

#ifndef _GWCOMMON_H
#define _GWCOMMON_H

#ifndef TRUE
#define TRUE	1
#endif
#ifndef FALSE
#define FALSE	0
#endif

#include "inttypes.h"
#ifndef __cplusplus
#include "stdbool.h"
#endif

#include <assert.h>
#ifdef GDEBUG
#define ASSERTG		assert
#else
#define ASSERTG(a)
#endif
#define GWASSERT  	assert

#endif

// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

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

// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

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

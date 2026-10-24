// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

#ifdef _WIN64
#define X86_64
#endif

#include "common.h"
#include "cpuid.h"
#include "gwnum.h"
#include "gwini.h"
#include "gwbench.h"
#include "commonb.h"
#include "commonc.h"
#include "comm95b.h"
#include "comm95c.h"
#include "primenet.h"

extern int THREAD_STOP;			
extern int DEBUGGING;			
extern int C_OPTION;			

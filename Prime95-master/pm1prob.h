// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#ifndef _PM1PROB_H
#define _PM1PROB_H

#ifdef __cplusplus
extern "C" {
#endif

double pm1prob (double takeAwayBits, double factoredUpTo, uint64_t B1, uint64_t B2);

double rho (double x);

#ifdef __cplusplus
}
#endif

#endif

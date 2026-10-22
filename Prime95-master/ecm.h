// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#ifndef _ECM_H
#define _ECM_H

#ifdef __cplusplus
extern "C" {
#endif

int ecm (int, struct PriorityInfo *, struct work_unit *);
int pminus1 (int, struct PriorityInfo *, struct work_unit *);
int pplus1 (int, struct PriorityInfo *, struct work_unit *);
int pfactor (int, struct PriorityInfo *, struct work_unit *);
double guess_pminus1_probability (struct work_unit *w);

int setN (int, struct work_unit *, giant *);
int ecm_QA (int, struct PriorityInfo *);
int pminus1_QA (int, struct PriorityInfo *);
int pplus1_QA (int, struct PriorityInfo *);

#ifdef __cplusplus
}
#endif

#endif

// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#ifndef _EXPONENTIATE_H
#define _EXPONENTIATE_H

#ifdef __cplusplus
extern "C" {
#endif

void simple_exponentiate (gwhandle *gwdata, gwnum x, uint64_t power);

void exponentiate (gwhandle *gwdata, gwnum x, uint64_t power);

void exponentiate_mpz (gwhandle *gwdata, gwnum x, mpz_t power);

void exponentiate_mpz_limited_temps (gwhandle *gwdata, gwnum x, mpz_t power, int num_temps);

#ifdef __cplusplus
}
#endif

#endif

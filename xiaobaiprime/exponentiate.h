// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

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

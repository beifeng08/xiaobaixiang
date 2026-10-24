// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

#ifndef SHA3_H
#define SHA3_H

#include <stddef.h>
#include <stdint.h>

#ifndef KECCAKF_ROUNDS
#define KECCAKF_ROUNDS 24
#endif

#ifndef ROTL64
#define ROTL64(x, y) (((x) << (y)) | ((x) >> (64 - (y))))
#endif

typedef struct {
    union {                                 
        uint8_t b[200];                     
        uint64_t q[25];                     
    } st;
    int pt, rsiz, mdlen;                    
} sha3_ctx_t;

void sha3_keccakf(uint64_t st[25]);

int sha3_init(sha3_ctx_t *c, int mdlen);    
int sha3_update(sha3_ctx_t *c, const void *data, size_t len);
int sha3_final(void *md, sha3_ctx_t *c);    

void *sha3(const void *in, size_t inlen, void *md, int mdlen);

#define shake128_init(c) sha3_init(c, 16)
#define shake256_init(c) sha3_init(c, 32)
#define shake_update sha3_update

void shake_xof(sha3_ctx_t *c);
void shake_out(sha3_ctx_t *c, void *out, size_t len);

#endif


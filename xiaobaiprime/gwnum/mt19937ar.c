// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

#define N 624
#define M 397
#define MATRIX_A 0x9908b0dfUL   
#define UPPER_MASK 0x80000000UL 
#define LOWER_MASK 0x7fffffffUL 

struct mt_state {
	unsigned long mt[N]; 
	int mti;
};

void init_genrand(struct mt_state *x, unsigned long s)
{
    x->mt[0]= s & 0xffffffffUL;
    for (x->mti=1; x->mti<N; x->mti++) {
        x->mt[x->mti] = 
	    (1812433253UL * (x->mt[x->mti-1] ^ (x->mt[x->mti-1] >> 30)) + x->mti); 
        
        x->mt[x->mti] &= 0xffffffffUL;
        
    }
}

unsigned long genrand_int32(struct mt_state *x)
{
    unsigned long y;
    static unsigned long mag01[2]={0x0UL, MATRIX_A};
    
    if (x->mti >= N) { 
        int kk;

        for (kk=0;kk<N-M;kk++) {
            y = (x->mt[kk]&UPPER_MASK)|(x->mt[kk+1]&LOWER_MASK);
            x->mt[kk] = x->mt[kk+M] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        for (;kk<N-1;kk++) {
            y = (x->mt[kk]&UPPER_MASK)|(x->mt[kk+1]&LOWER_MASK);
            x->mt[kk] = x->mt[kk+(M-N)] ^ (y >> 1) ^ mag01[y & 0x1UL];
        }
        y = (x->mt[N-1]&UPPER_MASK)|(x->mt[0]&LOWER_MASK);
        x->mt[N-1] = x->mt[M-1] ^ (y >> 1) ^ mag01[y & 0x1UL];

        x->mti = 0;
    }
  
    y = x->mt[x->mti++];

    y ^= (y >> 11);
    y ^= (y << 7) & 0x9d2c5680UL;
    y ^= (y << 15) & 0xefc60000UL;
    y ^= (y >> 18);

    return y;
}

#undef N
#undef M
#undef MATRIX_A
#undef UPPER_MASK
#undef LOWER_MASK

/* Copied with permission from https://github.com/preda/gpuowl/pm1 on 2020-08-11 */
/* Code courtesy of Mihai Preda */

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

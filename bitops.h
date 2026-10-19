/* Written by Aryeh Klempner <arayaydev@gmail.com>, 2020
The current contents of this file are not copyrightable on grounds of triviality and originality.
*/
#ifndef TESLA_BITOPS_H
#define TESLA_BITOPS_H
#include <stdint.h>

static inline uint64_t rol(const uint64_t a, int x) {
	return (a << x) | (a >> (64 - x));
}

static inline uint64_t ror(const uint64_t a, int x) {
	return (a >> x) | (a << (64 - x));
}
#endif

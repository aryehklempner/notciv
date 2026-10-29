/* Written by Aryeh Klempner <arayaydev@gmail.com>, 2020
The current contents of this file are not copyrightable on grounds of triviality and originality.
*/
#ifndef NOTCIV_BITOPS_H
#define NOTCIV_BITOPS_H
#include <stdint.h>

//masking keeps the shifts defined for x == 0 or x >= 64; gcc still emits one rotate
static inline uint64_t rol(const uint64_t a, unsigned x) {
	return (a << (x & 63)) | (a >> (-x & 63));
}

static inline uint64_t ror(const uint64_t a, unsigned x) {
	return (a >> (x & 63)) | (a << (-x & 63));
}
#endif

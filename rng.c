//RNG functionality and initialization
/* The xoshiro256+ and splitmix64 code was derived in 2020 by Aryeh Klempner
<arayaydev@gmail.com> from code written in 2015 and 2018 by David Blackman and
Sebastiano Vigna (vigna@acm.org).
To the extent possible under law, the author(s) have dedicated all copyright
and related and neighboring rights to the original version to the public domain
worldwide. This software is distributed without any warranty.
See <http://creativecommons.org/publicdomain/zero/1.0/>.

All other code is (c) 2020 Aryeh Klempner <arayaydev@gmail.com> and is free
software: you can redistribute it and/or modify it under the terms of the GNU
General Public License (GPL) as published by the Free Software Foundation,
either version 3 of the GPL, or (at your option) any later version. This
program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GPL for more details. You should have received a
copy of the GPL with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include <linux/version.h>//KERNEL_VERSION()
#if((__GLIBC__ > 2 || __GLIBC_MINOR__ >= 25) && LINUX_VERSION_CODE >= KERNEL_VERSION(3,17,0))
#include <sys/random.h>//getrandom()
#define LIBC_HAS_GETRANDOM 1
#else
#include <sys/syscall.h>//syscall()
#define LIBC_HAS_GETRANDOM 0
#endif
#include <stdint.h>
#include <stdio.h>//perror(), puts()
#include <stdlib.h>//NULL, exit()
#include <time.h>//time()
#include <unistd.h>//getpid(), getppid()

#include "bitops.h"

static inline long getrandom_wrapper(void* buf){
	long result;
	#if (LIBC_HAS_GETRANDOM == 0)
	result = syscall(SYS_getrandom, buf, 8, 0);
	#else
	result = (long)(getrandom(buf, 8, 0));
	#endif
	return result;
}

extern uint8_t srv_flags;
static uint64_t splitmix_state;
static uint64_t xoshiro_state[4];

/* This is a fixed-increment version of Java 8's SplittableRandom generator.
See https://dx.doi.org/10.1145/2714064.2660195 and https://docs.oracle.com/javase/8/docs/api/java/util/SplittableRandom.html
It is a very fast generator passing BigCrush, and can be useful if for some reason you absolutely want 64 bits of state. */

uint64_t splitmix64(void){
	uint64_t z = (splitmix_state += 0x9e3779b97f4a7c15);
	z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9;
	z = (z ^ (z >> 27)) * 0x94d049bb133111eb;
	return z ^ (z >> 31);
}

/* This is xoshiro256+ 1.0, our best and fastest generator for floating-point numbers. See http://prng.di.unimi.it/
We suggest using its upper bits for floating-point generation, as it is slightly faster than xoshiro256++/xoshiro256**. It passes all tests we are aware of except for the lowest three bits, which might fail linearity tests (and just those), so if low linear complexity is not considered an issue (as is usually the case) it can be used to generate 64-bit outputs, too.

We suggest using a sign test to extract a random Boolean value, and right shifts to extract subsets of bits.

The state must be seeded so that it is not everywhere zero. If you have a 64-bit seed, we suggest to seed a splitmix64 generator and use its output to fill s.
*/

uint64_t xoshiro256plus_next(void){
	const uint64_t result = xoshiro_state[0] + xoshiro_state[3];
	const uint64_t t = xoshiro_state[1] << 17;
	xoshiro_state[2] ^= xoshiro_state[0];
	xoshiro_state[3] ^= xoshiro_state[1];
	xoshiro_state[1] ^= xoshiro_state[2];
	xoshiro_state[0] ^= xoshiro_state[3];
	xoshiro_state[2] ^= t;
	xoshiro_state[3] = rol(xoshiro_state[3], 45);
	return result;
}

/* This is the jump function for the generator. It is equivalent
   to 2^128 calls to next(); it can be used to generate 2^128
   non-overlapping subsequences for parallel computations. */

/* void xoshiro_jump(void) {
	static const uint64_t jump_consts[] = {0x180ec6d33cfd0aba,
0xd5a61266f0c9392c, 0xa9582618e03fc9aa, 0x39abdc4529b1661c};
	uint64_t s0 = 0;
	uint64_t s1 = 0;
	uint64_t s2 = 0;
	uint64_t s3 = 0;
	for (int i = 0; i < sizeof jump_consts / sizeof *jump_consts; i++)
		for (int b = 0; b < 64; b++) {
			if (jump_consts[i] & UINT64_C(1) << b) {
				s0 ^= xoshiro_state[0];
				s1 ^= xoshiro_state[1];
				s2 ^= xoshiro_state[2];
				s3 ^= xoshiro_state[3];
			}
			xoshiro256plus_next();	
		}
	xoshiro_state[0] = s0;
	xoshiro_state[1] = s1;
	xoshiro_state[2] = s2;
	xoshiro_state[3] = s3;
} */

void rng_init(void){
	void* seedptr = &splitmix_state;
	if (getrandom_wrapper(seedptr) == -1L){
		perror("Failed to seed RNG, please examine your /dev/urandom");
		exit(EXIT_FAILURE);
	}
	xoshiro_state[0] = splitmix64();
	xoshiro_state[1] = splitmix64();
	xoshiro_state[2] = splitmix64();
	xoshiro_state[3] = splitmix64();
	if (srv_flags & 0x01) puts("RNG initialized successfully");
}

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

#include <sys/random.h>//getrandom()
#include <stdint.h>
#include <stdio.h>//perror()
#include <stdlib.h>//exit(), EXIT_FAILURE

#include "bitops.h"
#include "log.h"//log_msg(), LVL_INFO
#include "rng.h"//rng_init(), rng_below(), xoshiro256plus_next()

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

//full 64x64->128 product; __extension__ keeps -pedantic quiet about __int128
__extension__ typedef unsigned __int128 u128;

/* Unbiased value in [0, n), n > 0. Lemire's method (Lemire 2019, "Fast Random
Integer Generation in an Interval"): the high half of x*n scales x into range
using x's strong high bits, unlike x % n which leans on xoshiro256+'s weak low
bits. The low half tells us if x landed in the biased leftover, which we reject.
The division only runs when l < n, i.e. almost never. */
uint64_t rng_below(uint64_t n){
	u128 m = (u128)xoshiro256plus_next() * n;
	uint64_t l = (uint64_t)m;
	if (l < n){
		uint64_t t = -n % n;//(2^64 - n) mod n, the size of the biased leftover
		while (l < t){
			m = (u128)xoshiro256plus_next() * n;
			l = (uint64_t)m;
		}
	}
	return (uint64_t)(m >> 64);
}

void rng_init(void){
	if (getrandom(&splitmix_state, sizeof splitmix_state, 0) == -1){
		perror("Failed to seed splitmix w/ getrandom()");
		exit(EXIT_FAILURE);
	}
	xoshiro_state[0] = splitmix64();
	xoshiro_state[1] = splitmix64();
	xoshiro_state[2] = splitmix64();
	xoshiro_state[3] = splitmix64();
	log_msg(LVL_INFO, "RNG initialized successfully");
}

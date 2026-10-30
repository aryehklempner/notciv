//Monotonic clock helper
/*(c) 2026 Aryeh Klempner <arayaydev@gmail.com>
This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License (GPL) as published by the Free
Software Foundation, either version 3 of the GPL, or (at your option) any
later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GPL for more details. You should
have received a copy of the GPL with this program. If not, see
<http://www.gnu.org/licenses/>.
*/
#ifndef NOTCIV_CLK_H
#define NOTCIV_CLK_H
#include <stdint.h>
#include <time.h>//clock_gettime(), struct timespec, CLOCK_MONOTONIC

//nanoseconds on the monotonic clock; unlike the wall clock it never jumps backward
static inline uint64_t now_ns(void){
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t) ts.tv_sec * UINT64_C(1000000000) + (uint64_t) ts.tv_nsec;
}
#endif

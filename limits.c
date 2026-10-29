//Resource limits, i.e. how many clients we can hold open
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

#include <inttypes.h>//PRIu64, PRIu32
#include <stdint.h>
#include <stdio.h>//perror()
#include <stdlib.h>//exit(), EXIT_FAILURE
#include <sys/resource.h>//getrlimit(), setrlimit(), struct rlimit, rlim_t, RLIMIT_NOFILE

#include "log.h"//log_msg(), LVL_ERR, LVL_WARN, LVL_INFO

#define FD_CAP UINT64_C(65536) //don't raise the soft limit past this even if the hard limit allows
#define FD_RESERVE UINT64_C(16) //stdio, the listener, log/save files, and some slack

uint32_t max_clients;

void limits_init(void){
	struct rlimit rl;
	if (getrlimit(RLIMIT_NOFILE, &rl) == -1){
		perror("Failed to read the fd limit");
		exit(EXIT_FAILURE);
	}
	rlim_t want = rl.rlim_max > FD_CAP ? FD_CAP : rl.rlim_max;
	if (rl.rlim_cur < want){
		rl.rlim_cur = want;
		if (setrlimit(RLIMIT_NOFILE, &rl) == -1){
			log_msg(LVL_WARN, "Could not raise the fd limit; using the default");
			if (getrlimit(RLIMIT_NOFILE, &rl) == -1){
				perror("Failed to read the fd limit");
				exit(EXIT_FAILURE);
			}
		}
	}
	rlim_t usable = rl.rlim_cur > FD_CAP ? FD_CAP : rl.rlim_cur;
	if (usable <= FD_RESERVE){
		log_msg(LVL_ERR, "fd limit %" PRIu64 " is too low to serve any clients", (uint64_t) usable);
		exit(EXIT_FAILURE);
	}
	max_clients = (uint32_t)(usable - FD_RESERVE);
	log_msg(LVL_INFO, "fd limit %" PRIu64 ", max clients %" PRIu32, (uint64_t) rl.rlim_cur, max_clients);
}

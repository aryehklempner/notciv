//Server entry point and CLI argument handling
/*(c) 2018-26 Aryeh Klempner <arayaydev@gmail.com>
This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License (GPL) as published by the Free
Software Foundation, either version 3 of the GPL, or (at your option) any
later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GPL for more details. You should
have received a copy of the GPL with this program. If not, see
<http://www.gnu.org/licenses/>.
*/

#include <stdint.h>
#include <stdio.h>//puts()
#include <stdlib.h>//exit(), EXIT_SUCCESS
#include <string.h>//strcmp()

#include "log.h"//log_init(), log_msg(), LVL_WARN
#include "rng.h"//rng_init()

//bits: 0=loud 1=quiet 2-8=rsvd
uint8_t srv_flags;

void register_handlers(void);
void limits_init(void);
void shutdown_tty(void);

static inline void argchk(int argc, char *argv[]){
	for (int i = 1; i < argc; i++){
		if (!strcmp((const char*) argv[i], "-h") ||
		!strcmp((const char*) argv[i], "--help")){
			puts("Try \"info notciv\"");
			exit(EXIT_SUCCESS);
		}
		else if (!strcmp((const char*) argv[i], "--version")){
			puts("`notciv` is still in pre-alpha development");
			exit(EXIT_SUCCESS);
		}
		else if (!strcmp((const char*) argv[i], "--loud")){
			srv_flags |= 0x01;
		}
		else if (!strcmp((const char*) argv[i], "--quiet")){
			srv_flags |= 0x02;
		}
	}
}

static inline void parse_args(int argc, char *argv[]){
	for (int i = 1; i < argc; i++){
		if(!strcmp((const char*) argv[i], "--loud")){
			continue;
		}
		else if (!strcmp((const char*) argv[i], "--quiet")){
			continue;
		}
		else {
			log_msg(LVL_WARN, "Unknown argument: %s", argv[i]);
			log_msg(LVL_WARN, "Try `info notciv` for help");
		}
	}
}

int main(int argc, char *argv[]){
	log_init();
	if (argc > 1) argchk(argc, argv);
	register_handlers();
	if (argc > 1) parse_args(argc, argv);
	rng_init();
	limits_init();
	shutdown_tty();
	exit(EXIT_SUCCESS);
}

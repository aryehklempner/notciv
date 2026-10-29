//Diagnostics go to stderr; stdout is reserved for console output
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

#include <stdarg.h>//va_list, va_start(), va_end()
#include <stdint.h>
#include <stdio.h>//fputc(), setvbuf(), vfprintf(), stdout, stderr, _IOLBF, NULL
#include <unistd.h>//isatty(), STDOUT_FILENO

#include "log.h"//log_init(), log_msg(), LVL_WARN, LVL_INFO

extern uint8_t srv_flags;

//call before anything else writes to stdout, or setvbuf() is undefined
void log_init(void){
//stdout is fully buffered when redirected, so console lines would show up late
	if (!isatty(STDOUT_FILENO)) setvbuf(stdout, NULL, _IOLBF, 0);
}

void log_msg(uint8_t lvl, const char* fmt, ...){
	if (lvl == LVL_WARN && (srv_flags & 0x02)) return;
	if (lvl == LVL_INFO && !(srv_flags & 0x01)) return;
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

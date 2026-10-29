//Logging levels and prototypes
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
#ifndef NOTCIV_LOG_H
#define NOTCIV_LOG_H
#include <stdint.h>

#define LVL_ERR 0 //always shown
#define LVL_WARN 1 //hidden by --quiet
#define LVL_INFO 2 //shown only with --loud

void log_init(void);
void log_msg(uint8_t lvl, const char* fmt, ...) __attribute__((format(printf, 2, 3)));//gcc checks args vs fmt
#endif

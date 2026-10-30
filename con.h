//Server console interface
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
#ifndef NOTCIV_CON_H
#define NOTCIV_CON_H
#include <stdint.h>

void con_init(void);
uint8_t con_live(void);//1 while stdin should be in the poll set
void con_read(void);//call when stdin is readable
#endif

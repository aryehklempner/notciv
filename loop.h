//Main loop interface
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
#ifndef NOTCIV_LOOP_H
#define NOTCIV_LOOP_H
#include <stdint.h>

#define LOOP_CON UINT8_C(255) //stop reason for a console quit; 1-64 are signal numbers
#define LOOP_MAXSPEED 5 //speeds run 1 to this

void loop_run(int sigfd);//returns once a stop has been requested
void loop_stop(uint8_t why);//the first reason sticks
void loop_pause(void);
void loop_resume(void);
void loop_speed(uint8_t s);
void loop_ff(uint64_t n);//fast-forward n ticks without waiting for the clock
void loop_status(void);//prints the loop state on the console
#endif

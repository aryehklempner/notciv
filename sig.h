//Signal setup and handling; signals arrive through a signalfd, not handlers
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
#ifndef NOTCIV_SIG_H
#define NOTCIV_SIG_H

int sig_init(void);//blocks the signals we handle and returns the signalfd; exits on failure
void sig_read(void);//call when the signalfd is readable
void shutdown_tty(void);//normal context only; call on the way out of main()
#endif

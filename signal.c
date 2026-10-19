//Signal handling
/*(c) 2019-20 Aryeh Klempner <arayaydev@gmail.com>
This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License (GPL) as published by the Free
Software Foundation, either version 3 of the GPL, or (at your option) any
later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GPL for more details. You should
have received a copy of the GPL with this program. If not, see
<http://www.gnu.org/licenses/>.*/

#include <signal.h>//raise(), sigaction()
#include <stdint.h>
#include <stdio.h>//perror(), puts()
#include <stdlib.h>//exit(), _exit()
#include <termios.h>//tcdrain(), tcflush()
#include <unistd.h>//alarm(), isatty()

//0: whether a tty sig was rcvd in past second; 1-8 rsvd
volatile uint8_t sigflags;
extern uint8_t srv_flags;

void handl_alrm(int sig){
	sigflags &= 0xFE;
}

void clean_exit(int sig){
	if (isatty(STDIN_FILENO)) tcflush(STDIN_FILENO, TCIFLUSH);
	if (isatty(STDOUT_FILENO)) tcdrain(STDOUT_FILENO);
	if (isatty(STDERR_FILENO)) tcdrain(STDERR_FILENO);
	_exit(EXIT_SUCCESS);
}

void chk_dbl(int sig){
	if (!(sigflags & 0x01)){
		sigflags |= 0x01;
		alarm(1U);
	}
//Note that we only check # of this sig, so e.g. ^C^Z will stop.
	else switch (sig){
		case SIGINT:
			clean_exit(sig);
			break;
		case SIGQUIT:
			abort();
			break;
		case SIGTSTP:
			raise(SIGSTOP);
			break;
		default: clean_exit(sig);
	}
}

void register_handlers(void){//see `man sigaction` if confused
	struct sigaction sigstruct;
	sigset_t blkmask;
	sigemptyset(&blkmask);
	sigaddset(&blkmask, SIGALRM);
	sigaddset(&blkmask, SIGHUP);
	sigaddset(&blkmask, SIGINT);
	sigaddset(&blkmask, SIGQUIT);
	sigaddset(&blkmask, SIGTERM);
	sigaddset(&blkmask, SIGTSTP);
	sigstruct.sa_flags = SA_RESTART;
	sigstruct.sa_mask = blkmask;
	sigstruct.sa_handler = handl_alrm;
	if (sigaction(SIGALRM, &sigstruct, NULL) == -1) goto err;
	sigstruct.sa_handler = clean_exit;
	if (sigaction(SIGHUP, &sigstruct, NULL) == -1) goto err;
	if (sigaction(SIGTERM, &sigstruct, NULL) == -1) goto err;
	sigstruct.sa_handler = chk_dbl;
	if (sigaction(SIGINT, &sigstruct, NULL) == -1) goto err;
	if (sigaction(SIGQUIT, &sigstruct, NULL) == -1) goto err;
	if (sigaction(SIGTSTP, &sigstruct, NULL) == -1) goto err;
	if (srv_flags & 0x01) puts("All signal handlers set up successfully");
	return;
err:if (!(srv_flags & 0x02)) perror("Signal handler setup failed");
	exit(EXIT_FAILURE);
}

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
volatile sig_atomic_t sigflags;
//0 until a stop is requested, then the signal number; only handlers write it
volatile sig_atomic_t stop_sig;
extern uint8_t srv_flags;

void handl_alrm(int sig){
	(void)sig;
	sigflags &= 0xFE;
}

void rq_stop(int sig){
	if (stop_sig) _exit(EXIT_FAILURE);//already shutting down and the user insists
	stop_sig = sig;
}

//normal context only; call on the way out of main()
void shutdown_tty(void){
	if (isatty(STDIN_FILENO)) tcflush(STDIN_FILENO, TCIFLUSH);
	if (isatty(STDOUT_FILENO)) tcdrain(STDOUT_FILENO);
	if (isatty(STDERR_FILENO)) tcdrain(STDERR_FILENO);
}

void chk_dbl(int sig){
	if (!(sigflags & 0x01)){
		sigflags |= 0x01;
		alarm(1U);
	}
//Note that we only check # of this sig, so e.g. ^C^Z will stop.
	else switch (sig){
		case SIGINT:
			rq_stop(sig);
			break;
		case SIGQUIT:
			abort();
			break;
		case SIGTSTP:
			raise(SIGSTOP);
			break;
		default: rq_stop(sig);
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
	sigstruct.sa_handler = rq_stop;
	if (sigaction(SIGHUP, &sigstruct, NULL) == -1) goto err;
	if (sigaction(SIGTERM, &sigstruct, NULL) == -1) goto err;
	sigstruct.sa_handler = chk_dbl;
	if (sigaction(SIGINT, &sigstruct, NULL) == -1) goto err;
	if (sigaction(SIGQUIT, &sigstruct, NULL) == -1) goto err;
	if (sigaction(SIGTSTP, &sigstruct, NULL) == -1) goto err;
	sigstruct.sa_handler = SIG_IGN;//a dead client socket must not kill the server; check for EPIPE instead
	if (sigaction(SIGPIPE, &sigstruct, NULL) == -1) goto err;
	if (srv_flags & 0x01) puts("All signal handlers set up successfully");
	return;
err:if (!(srv_flags & 0x02)) perror("Signal handler setup failed");
	exit(EXIT_FAILURE);
}

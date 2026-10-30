//Signal handling through a signalfd
/*(c) 2019-20 Aryeh Klempner <arayaydev@gmail.com>
This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License (GPL) as published by the Free
Software Foundation, either version 3 of the GPL, or (at your option) any
later version. This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
or FITNESS FOR A PARTICULAR PURPOSE.  See the GPL for more details. You should
have received a copy of the GPL with this program. If not, see
<http://www.gnu.org/licenses/>.*/


#include <errno.h>//errno, EAGAIN, EINTR
#include <signal.h>//sigaction(), sigemptyset(), sigaddset(), sigprocmask(), raise(), sigset_t, struct sigaction, SIG*
#include <stdint.h>
#include <stdio.h>//perror()
#include <stdlib.h>//exit(), abort(), EXIT_FAILURE, NULL
#include <string.h>//memset()
#include <sys/signalfd.h>//signalfd(), struct signalfd_siginfo, SFD_NONBLOCK, SFD_CLOEXEC
#include <termios.h>//tcdrain(), tcflush(), TCIFLUSH
#include <unistd.h>//read(), isatty(), tcgetpgrp(), getpgrp(), STD*_FILENO

#include "clk.h"//now_ns()
#include "log.h"//log_msg(), LVL_INFO
#include "loop.h"//loop_stop()
#include "sig.h"//sig_init(), sig_read(), shutdown_tty()

#define DBL_NS UINT64_C(1000000000) //a second tty signal inside this window counts as a double press

static int sigfd;
static uint64_t armed_ns;//when the first tty signal arrived; 0 if none is pending

//first tty signal arms; a second one inside the window acts. Handled in normal
//context now, so unlike a handler it can't run while the loop is stuck.
static void chk_dbl(uint32_t sig){
	uint64_t now = now_ns();
	if (!armed_ns || now - armed_ns >= DBL_NS){//first press, or the last one went stale
		armed_ns = now;
		return;
	}
	armed_ns = 0;
//Note that we only check # of this sig, so e.g. ^C^Z will stop.
	switch (sig){
		case SIGINT:
			loop_stop(sig);
			break;
		case SIGQUIT:
			abort();
			break;
		case SIGTSTP:
			raise(SIGSTOP);
			break;
		default: loop_stop(sig);
	}
}

void sig_read(void){
	struct signalfd_siginfo si;
	while (read(sigfd, &si, sizeof si) == (ssize_t) sizeof si){
		switch (si.ssi_signo){
			case SIGINT:
			case SIGQUIT:
			case SIGTSTP:
				chk_dbl(si.ssi_signo);
				break;
			default: loop_stop(si.ssi_signo);//SIGHUP and SIGTERM stop at once
		}
	}
	if (errno != EAGAIN && errno != EINTR) perror("Reading the signalfd failed");
}

int sig_init(void){//see `man signalfd` if confused
	sigset_t mask;
	struct sigaction ign;
	sigemptyset(&mask);
	sigaddset(&mask, SIGHUP);
	sigaddset(&mask, SIGINT);
	sigaddset(&mask, SIGQUIT);
	sigaddset(&mask, SIGTERM);
	sigaddset(&mask, SIGTSTP);
//one thread for now; this becomes pthread_sigmask() before the first extra thread
	if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) goto err;
	sigfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
	if (sigfd == -1) goto err;
	memset(&ign, 0, sizeof ign);
	ign.sa_handler = SIG_IGN;//a dead client socket must not kill the server; check for EPIPE instead
	if (sigaction(SIGPIPE, &ign, NULL) == -1) goto err;
	log_msg(LVL_INFO, "Signal handling set up successfully");
	return sigfd;
err:perror("Signal setup failed");
	exit(EXIT_FAILURE);
}

//a background job that calls tcdrain() or tcflush() on its terminal is stopped by SIGTTOU
static uint8_t tty_fg(int fd){
	return isatty(fd) && tcgetpgrp(fd) == getpgrp();
}

//normal context only; call on the way out of main()
void shutdown_tty(void){
	if (tty_fg(STDIN_FILENO)) tcflush(STDIN_FILENO, TCIFLUSH);
	if (tty_fg(STDOUT_FILENO)) tcdrain(STDOUT_FILENO);
	if (tty_fg(STDERR_FILENO)) tcdrain(STDERR_FILENO);
}

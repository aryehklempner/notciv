//Main loop: one ppoll wait for I/O and the next tick deadline
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

#define _GNU_SOURCE//glibc only declares ppoll() under this

#include <errno.h>//errno, EINTR
#include <inttypes.h>//PRIu64, PRIu8
#include <poll.h>//ppoll(), struct pollfd, POLLIN
#include <stdint.h>
#include <stdio.h>//perror(), printf()
#include <stdlib.h>//exit(), EXIT_FAILURE, NULL
#include <time.h>//struct timespec
#include <unistd.h>//STDIN_FILENO

#include "clk.h"//now_ns()
#include "con.h"//con_live(), con_read()
#include "log.h"//log_msg(), LVL_INFO, LVL_WARN
#include "loop.h"//loop_run(), loop_stop(), loop_pause(), loop_resume(), loop_speed(), loop_ff(), loop_status(), LOOP_CON, LOOP_MAXSPEED
#include "sig.h"//sig_read()

#define CATCHUP_MAX 4 //ticks run per pass when behind, before we go service I/O again
#define FF_BATCH UINT64_C(1000) //fast-forwarded ticks per pass, so I/O stays responsive
#define NS_PER_S UINT64_C(1000000000)

//ns per tick at speeds 1 to LOOP_MAXSPEED
static const uint64_t period_ns[LOOP_MAXSPEED] = {
	UINT64_C(1000000000), UINT64_C(500000000), UINT64_C(250000000),
	UINT64_C(100000000), UINT64_C(50000000)
};

//bits: 0=paused 1-7=rsvd
static uint8_t loop_flags;
static uint8_t why_stop;//0 while running; else a signal number or LOOP_CON
static uint8_t speed = 1;
static uint64_t ticks;
static uint64_t next_ns;//when the next tick is due
static uint64_t ff_left;//ticks still owed to a console `run`

static void tick(void){
	ticks++;//the simulation will hang off this
}

static void run_ticks(uint64_t now){
	if (ff_left){
		uint64_t k = ff_left < FF_BATCH ? ff_left : FF_BATCH;
		for (uint64_t i = 0; i < k; i++) tick();
		ff_left -= k;
		if (!ff_left) next_ns = now_ns() + period_ns[speed - 1];//the clock moved on while we raced
	}
	else if (!(loop_flags & 0x01)){
		uint8_t c = 0;
		while (now >= next_ns && c < CATCHUP_MAX){
			tick();
			next_ns += period_ns[speed - 1];
			c++;
		}
		if (now >= next_ns){//still behind after the cap; skip ahead instead of snowballing
			log_msg(LVL_WARN, "Falling behind; skipping ahead");
			next_ns = now + period_ns[speed - 1];
		}
	}
}

void loop_run(int sigfd){
	struct pollfd fds[2];
	fds[0].fd = sigfd;
	fds[0].events = POLLIN;
	fds[1].events = POLLIN;
	next_ns = now_ns() + period_ns[speed - 1];
	while (!why_stop){
		struct timespec ts, *tp = NULL;//NULL = wait forever, i.e. paused with nothing owed
		uint64_t now = now_ns();
		if (ff_left){//no waiting, just a quick look at I/O between batches
			ts.tv_sec = 0;
			ts.tv_nsec = 0;
			tp = &ts;
		}
		else if (!(loop_flags & 0x01)){
			uint64_t d = next_ns > now ? next_ns - now : 0;
			ts.tv_sec = d / NS_PER_S;
			ts.tv_nsec = d % NS_PER_S;
			tp = &ts;
		}
		fds[1].fd = con_live() ? STDIN_FILENO : -1;//poll() skips negative fds
//Signals are blocked and arrive on the signalfd, so there is no gap for one to
//slip into and no mask to pass here: readiness on the fd is level-triggered.
		int n = ppoll(fds, 2, tp, NULL);
		if (n == -1){
			if (errno == EINTR) continue;
			perror("ppoll failed");
			exit(EXIT_FAILURE);
		}
		if (fds[0].revents) sig_read();
		if (fds[1].revents) con_read();
		if (why_stop) break;
		run_ticks(now_ns());
	}
	if (why_stop == LOOP_CON) log_msg(LVL_INFO, "Stopping: console quit");
	else log_msg(LVL_INFO, "Stopping on signal %" PRIu8, why_stop);
}

void loop_stop(uint8_t why){
	if (!why_stop) why_stop = why;
}

void loop_pause(void){
	loop_flags |= 0x01;
}

void loop_resume(void){
	loop_flags &= 0xFE;
	next_ns = now_ns() + period_ns[speed - 1];
}

void loop_speed(uint8_t s){
	speed = s;
	next_ns = now_ns() + period_ns[speed - 1];
}

void loop_ff(uint64_t n){
	ff_left = n;
}

void loop_status(void){
	printf("tick %" PRIu64 ", speed %" PRIu8 "%s, ff left %" PRIu64 "\n",
	ticks, speed, (loop_flags & 0x01) ? " (paused)" : "", ff_left);
}

//Server console: reads stdin lines and runs commands from a table
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

#include <errno.h>//errno, EAGAIN, EINTR, ERANGE
#include <inttypes.h>//PRIu32
#include <stdint.h>
#include <stdio.h>//perror(), printf(), puts()
#include <stdlib.h>//strtoull()
#include <string.h>//memchr(), memmove(), strcmp()
#include <unistd.h>//read(), isatty(), tcgetpgrp(), getpgrp(), STDIN_FILENO

#include "con.h"//con_init(), con_live(), con_read()
#include "log.h"//log_msg(), LVL_INFO, LVL_WARN
#include "loop.h"//loop_stop(), loop_pause(), loop_resume(), loop_speed(), loop_ff(), loop_status(), LOOP_CON, LOOP_MAXSPEED

#define CON_BUF 512 //longest console line, counting its newline
#define CON_ARGS 8 //tokens kept per line; the rest are dropped

extern uint32_t max_clients;

//bits: 0=stdin is done (eof or error) 1=skipping the rest of an overlong line 2=stdin is a tty 3-7=rsvd
static uint8_t con_flags;
static char buf[CON_BUF];
static size_t len;//bytes held in buf

struct cmd {
	const char* name;
	void (*fn)(uint8_t argc, char** argv);
	const char* help;
};

//splits line in place on spaces and tabs; returns the token count
static uint8_t tokenize(char* line, char** argv){
	uint8_t argc = 0;
	while (argc < CON_ARGS){
		while (*line == ' ' || *line == '\t') line++;
		if (!*line) break;
		argv[argc++] = line;
		while (*line && *line != ' ' && *line != '\t') line++;
		if (*line) *line++ = '\0';//end this token
	}
	return argc;
}

//plain decimal digits only; returns 0 on junk or overflow
static uint8_t parse_u64(const char* s, uint64_t* out){
	char* end;
	if (*s < '0' || *s > '9') return 0;
	errno = 0;
	uint64_t v = strtoull(s, &end, 10);
	if (errno == ERANGE || *end) return 0;
	*out = v;
	return 1;
}

static void cmd_help(uint8_t argc, char** argv);

static void cmd_status(uint8_t argc, char** argv){
	(void)argc;
	(void)argv;
	loop_status();
	printf("max clients %" PRIu32 "\n", max_clients);
}

static void cmd_pause(uint8_t argc, char** argv){
	(void)argc;
	(void)argv;
	loop_pause();
	puts("paused");
}

static void cmd_resume(uint8_t argc, char** argv){
	(void)argc;
	(void)argv;
	loop_resume();
	puts("running");
}

static void cmd_speed(uint8_t argc, char** argv){
	uint64_t s;
	if (argc != 2 || !parse_u64(argv[1], &s) || s < 1 || s > LOOP_MAXSPEED){
		puts("usage: speed <1-5>");
		return;
	}
	loop_speed((uint8_t) s);
}

static void cmd_run(uint8_t argc, char** argv){
	uint64_t n;
	if (argc != 2 || !parse_u64(argv[1], &n)){
		puts("usage: run <ticks>");
		return;
	}
	loop_ff(n);
}

static void cmd_quit(uint8_t argc, char** argv){
	(void)argc;
	(void)argv;
	loop_stop(LOOP_CON);
}

static const struct cmd cmds[] = {
	{"help", cmd_help, "list the commands"},
	{"status", cmd_status, "show tick, speed and the client cap"},
	{"pause", cmd_pause, "stop ticking"},
	{"resume", cmd_resume, "start ticking again"},
	{"speed", cmd_speed, "speed <1-5>: set the tick rate"},
	{"run", cmd_run, "run <ticks>: fast-forward without waiting for the clock"},
	{"quit", cmd_quit, "stop the server"},
};
#define NCMDS (sizeof cmds / sizeof *cmds)

static void cmd_help(uint8_t argc, char** argv){
	(void)argc;
	(void)argv;
	for (size_t i = 0; i < NCMDS; i++) printf("%-8s %s\n", cmds[i].name, cmds[i].help);
}

static void dispatch(char* line){
	char* argv[CON_ARGS];
	uint8_t argc = tokenize(line, argv);
	if (!argc) return;
	for (size_t i = 0; i < NCMDS; i++){
		if (!strcmp(argv[0], cmds[i].name)){
			cmds[i].fn(argc, argv);
			return;
		}
	}
	printf("Unknown command: %s\nTry \"help\"\n", argv[0]);
}

void con_init(void){
	if (isatty(STDIN_FILENO)) con_flags |= 0x04;
}

uint8_t con_live(void){
	if (con_flags & 0x01) return 0;//eof or error: leave stdin alone
//a background job that reads its terminal gets stopped by SIGTTIN
	if ((con_flags & 0x04) && tcgetpgrp(STDIN_FILENO) != getpgrp()) return 0;
	return 1;
}

//One read() per readiness. We don't set O_NONBLOCK: the open file description
//is shared with whatever started us (the shell), so that would change it for them too.
void con_read(void){
	ssize_t n = read(STDIN_FILENO, buf + len, sizeof buf - len - 1);
	if (n == -1){
		if (errno == EAGAIN || errno == EINTR) return;
		perror("Console read failed");
		con_flags |= 0x01;
		return;
	}
	if (n == 0){//eof: run a final line that had no newline, then stop listening
		if (len && !(con_flags & 0x02)){
			buf[len] = '\0';
			dispatch(buf);
		}
		len = 0;
		log_msg(LVL_INFO, "Console input closed");
		con_flags |= 0x01;
		return;
	}
	len += (size_t) n;
	char* start = buf;
	char* nl;
	while ((nl = memchr(start, '\n', (size_t) (buf + len - start)))){
		*nl = '\0';
		if (nl > start && nl[-1] == '\r') nl[-1] = '\0';
		if (con_flags & 0x02) con_flags &= 0xFD;//that was the tail of an overlong line
		else dispatch(start);
		start = nl + 1;
	}
	len -= (size_t) (start - buf);
	memmove(buf, start, len);
	if (len == sizeof buf - 1){//a full buffer with no newline in it
		log_msg(LVL_WARN, "Console line too long; dropping it");
		con_flags |= 0x02;
		len = 0;
	}
}

CC = gcc
CFLAGS = -O2 -fno-math-errno -fomit-frame-pointer -fexpensive-optimizations -flto -march=native -Wformat-security -Warray-bounds -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-include-dirs -Wswitch-enum -Wswitch-default -pedantic -Wfatal-errors -Wextra -Wall
ODIR = obj
BIN = ../bin/notciv-server
SRCS = server_main.c signal.c rng.c log.c limits.c
OBJS = $(SRCS:%.c=$(ODIR)/%.o)
HDRS = $(wildcard *.h)

all: $(BIN)

$(BIN): $(OBJS)
	mkdir -p $(dir $(BIN))
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(ODIR)/%.o: %.c $(HDRS) | $(ODIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(ODIR):
	mkdir -p $(ODIR)

clean:
	rm -rf $(ODIR) $(BIN)

.PHONY: all clean

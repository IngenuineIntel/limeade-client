#!/usr/bin/make

CC ?= clang
CFLAGS := -O2 -std=gnu23 -Wall -Wpedantic -Werror -Wno-pointer-arith -Wno-format-zero-length -Wno-format
LDLIBS += -llimeade -pthread

.PHONY: all clean

all: clean limeade-client

limeade-client: src/main.c src/logging.c
	$(CC) -Isrc $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -f limeade-client


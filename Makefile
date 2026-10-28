#!/usr/bin/make

CC     := clang
CFLAGS := -O2 -std=gnu23 -Wall -Wpedantic -Werror -Wno-pointer-arith -Wno-format-zero-length -Wno-format -Wno-format-pedantic
LDLIBS += -llimeade -lpthread

.PHONY: all clean

all: clean limeade-client

limeade-client: src/main.c src/logging.c
	$(CC) -Isrc $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

install: uninstall all
	sudo cp ./limeade-client /usr/local/bin/limeade-client
	-sudo ln -s /usr/local/bin/limeade-client /usr/local/sbin/limeade-client
	-sudo groupadd --system limeade
	-sudo useradd --system limeade -g limeade
	-sudo mkdir /etc/limeade
	-sudo chown root:limeade /etc/limeade
	-sudo chmod 640 /etc/limeade
	sudo cp ./limeade-client.ini /etc/limeade/limeade-client.ini

uninstall:
	-sudo rm /usr/local/sbin/limeade-client /usr/local/bin/limeade-client

uninstall-environment:
	@echo "this is not implemented yet!"



clean:
	rm -f limeade-client


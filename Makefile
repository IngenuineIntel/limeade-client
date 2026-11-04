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
	sudo usermod -d /etc/limeade limeade
	sudo install -d -o root -g limeade -m 0750 /etc/limeade/ssh
	sudo install -d -o root -g limeade -m 0750 /etc/limeade/.ssh
	sudo install -d -o root -g limeade -m 2770 /etc/limeade/history
	sudo cp ./limeade-client.ini /etc/limeade/limeade-client.ini
	@if ! sudo test -e /etc/limeade/ssh/id_ed25519; then sudo ssh-keygen -q -t ed25519 -N "" -f /etc/limeade/ssh/id_ed25519; fi
	sudo install -o root -g limeade -m 0640 /etc/limeade/ssh/id_ed25519.pub /etc/limeade/.ssh/authorized_keys
	sudo chown -R root:limeade /etc/limeade
	sudo chmod -R u=rwX,g=rX,o= /etc/limeade
	sudo chown root:limeade /etc/limeade/.ssh/known_hosts
	sudo chmod 0640 /etc/limeade/.ssh/known_hosts
	sudo chmod 2750 /etc/limeade/.ssh
	sudo chmod 2750 /etc/limeade
	sudo chmod 2770 /etc/limeade/history
	make -C engine install

uninstall:
	-sudo rm /usr/local/sbin/limeade-client /usr/local/bin/limeade-client

uninstall-environment:
	@echo "this is not implemented yet!"

clean:
	rm -f limeade-client


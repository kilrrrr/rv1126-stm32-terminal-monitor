CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
COMMON = ../common/protocol.c

all: monitor_daemon monitor_cli

monitor_daemon: monitor_daemon.c serial_port.c $(COMMON)
	$(CC) $(CFLAGS) -I../common -I. $^ -o $@

monitor_cli: monitor_cli.c serial_port.c $(COMMON)
	$(CC) $(CFLAGS) -I../common -I. $^ -o $@

clean:
	rm -f monitor_daemon monitor_cli


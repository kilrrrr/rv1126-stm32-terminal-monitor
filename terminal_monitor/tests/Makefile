CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2

all: protocol_test firmware_test simulator_test serial_port_test

protocol_test: protocol_test.c ../common/protocol.c
	$(CC) $(CFLAGS) -I../common $^ -o $@

firmware_test: firmware_test.c ../common/protocol.c ../stm32f103/monitor_firmware.c
	$(CC) $(CFLAGS) -I../common -I../stm32f103 $^ -o $@

simulator_test: simulator_test.c ../common/protocol.c ../linux/monitor_simulator_core.c
	$(CC) $(CFLAGS) -I../common -I../linux $^ -o $@

serial_port_test: serial_port_test.c ../linux/serial_port.c
	$(CC) $(CFLAGS) -I../linux $^ -o $@

check: all
	./protocol_test
	./firmware_test
	./simulator_test
	./serial_port_test

clean:
	rm -f protocol_test firmware_test simulator_test serial_port_test


#include "serial_port.h"

#include "../common/protocol.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static volatile sig_atomic_t running = 1;

static void stop_handler(int signal_number)
{
	(void)signal_number;
	running = 0;
}

static void on_frame(const terminal_frame_t *frame, void *user)
{
	(void)user;

	if (frame->type != TERMINAL_MSG_STATUS)
	{
		return;
	}
	{
		terminal_status_t status;
		if (terminal_decode_status_frame(frame, &status) != 0)
		{
			return;
		}
		printf("status seq=%u uptime_ms=%u voltage_mv=%u temperature_c10=%d "
			"fan_rpm=%u fan_duty=%u light_duty=%u faults=0x%04x\n",
			status.sequence, status.uptime_ms, status.voltage_mv,
			status.temperature_c10, status.fan_rpm, status.fan_duty_pct,
			status.light_duty_pct, status.fault_bits);
		fflush(stdout);
	}
}

int main(int argc, char **argv)
{
	const char *device = (argc > 1) ? argv[1] : "/dev/ttyUSB0";
	int fd;
	uint8_t buffer[128];
	terminal_parser_t parser;

	fd = serial_open(device, 115200);
	if (fd < 0)
	{
		fprintf(stderr, "cannot open serial device: %s\n", device);
		return 1;
	}
	signal(SIGINT, stop_handler);
	signal(SIGTERM, stop_handler);
	terminal_parser_init(&parser);
	while (running)
	{
		int count = serial_read_poll(fd, buffer, sizeof(buffer), 1000);
		if (count < 0)
		{
			fprintf(stderr, "serial read failed\n");
			break;
		}
		if (count > 0)
		{
			terminal_parser_feed(&parser, buffer, (size_t)count, on_frame, NULL);
		}
	}
	serial_close(fd);
	return 0;
}


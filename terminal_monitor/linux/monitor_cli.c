#include "serial_port.h"

#include "../common/protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int parse_duty(const char *text, uint8_t *duty)
{
	char *end = NULL;
	long value;

	if (text == NULL || duty == NULL) return -1;
	value = strtol(text, &end, 10);
	if (*text == '\0' || *end != '\0' || value < 0 || value > 100) return -1;
	*duty = (uint8_t)value;
	return 0;
}

int main(int argc, char **argv)
{
	const char *device;
	terminal_command_t command;
	uint8_t frame[TERMINAL_MAX_FRAME_SIZE];
	size_t frame_length = 0u;
	int fd;

	if (argc != 4 || (strcmp(argv[2], "set-fan") != 0 &&
		strcmp(argv[2], "set-light") != 0))
	{
		fprintf(stderr, "usage: %s <serial> set-fan|set-light <0..100>\n", argv[0]);
		return 2;
	}
	device = argv[1];
	if (parse_duty(argv[3], &command.value) != 0)
	{
		fprintf(stderr, "duty must be between 0 and 100\n");
		return 2;
	}
	command.command_id = (strcmp(argv[2], "set-fan") == 0) ?
		TERMINAL_CMD_SET_FAN_DUTY : TERMINAL_CMD_SET_LIGHT_DUTY;
	fd = serial_open(device, 115200);
	if (fd < 0)
	{
		fprintf(stderr, "cannot open serial device: %s\n", device);
		return 1;
	}
	if (terminal_encode_command(&command, 1u, frame, sizeof(frame), &frame_length) != 0 ||
		serial_write_all(fd, frame, frame_length) != 0)
	{
		serial_close(fd);
		fprintf(stderr, "failed to send command\n");
		return 1;
	}
	serial_close(fd);
	return 0;
}


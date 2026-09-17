#define _GNU_SOURCE

#include "monitor_simulator_core.h"

#include <errno.h>
#include <inttypes.h>
#include <pty.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t running = 1;

static void stop_handler(int signal_number)
{
	(void)signal_number;
	running = 0;
}

static int parse_u32(const char *text, uint32_t *value)
{
	char *end = NULL;
	unsigned long parsed;

	if (text == NULL || value == NULL || *text == '\0')
	{
		return -1;
	}
	parsed = strtoul(text, &end, 10);
	if (*end != '\0' || parsed > UINT32_MAX)
	{
		return -1;
	}
	*value = (uint32_t)parsed;
	return 0;
}

static int write_all(int fd, const uint8_t *data, size_t length)
{
	size_t offset = 0u;

	while (offset < length)
	{
		ssize_t written = write(fd, &data[offset], length - offset);
		if (written < 0)
		{
			if (errno == EINTR)
			{
				continue;
			}
			return -1;
		}
		if (written == 0)
		{
			return -1;
		}
		offset += (size_t)written;
	}
	return 0;
}

static void sleep_ms(uint32_t milliseconds)
{
	struct timespec request;

	request.tv_sec = (time_t)(milliseconds / 1000u);
	request.tv_nsec = (long)((milliseconds % 1000u) * 1000000u);
	while (nanosleep(&request, &request) != 0 && errno == EINTR && running)
	{
	}
}

static void print_usage(const char *program)
{
	fprintf(stderr,
		"usage: %s [--frames N] [--interval-ms N] "
		"[--drop-every N] [--bad-crc-every N]\n", program);
}

static int configure_raw_slave(int fd)
{
	struct termios settings;

	if (tcgetattr(fd, &settings) != 0)
	{
		return -1;
	}
	cfmakeraw(&settings);
	return tcsetattr(fd, TCSANOW, &settings);
}

int main(int argc, char **argv)
{
	uint32_t frames = 0u;
	uint32_t interval_ms = 1000u;
	uint32_t drop_every = 0u;
	uint32_t bad_crc_every = 0u;
	monitor_simulator_state_t state;
	uint8_t frame[TERMINAL_MAX_FRAME_SIZE];
	int master_fd = -1;
	int slave_fd = -1;
	char slave_name[128];

	for (int index = 1; index < argc; ++index)
	{
		uint32_t *target = NULL;
		if (strcmp(argv[index], "--frames") == 0)
		{
			target = &frames;
		}
		else if (strcmp(argv[index], "--interval-ms") == 0)
		{
			target = &interval_ms;
		}
		else if (strcmp(argv[index], "--drop-every") == 0)
		{
			target = &drop_every;
		}
		else if (strcmp(argv[index], "--bad-crc-every") == 0)
		{
			target = &bad_crc_every;
		}
		else
		{
			print_usage(argv[0]);
			return 2;
		}
		if (index + 1 >= argc || parse_u32(argv[++index], target) != 0)
		{
			print_usage(argv[0]);
			return 2;
		}
	}

	if (openpty(&master_fd, &slave_fd, slave_name, NULL, NULL) != 0)
	{
		perror("openpty");
		return 1;
	}
	if (configure_raw_slave(slave_fd) != 0)
	{
		perror("configure pty");
		close(slave_fd);
		close(master_fd);
		return 1;
	}
	(void)signal(SIGINT, stop_handler);
	(void)signal(SIGTERM, stop_handler);
	monitor_simulator_init(&state, interval_ms, drop_every, bad_crc_every);
	printf("SIM_DEVICE=%s\n", slave_name);
	fflush(stdout);

	while (running && (frames == 0u || state.frame_count < frames))
	{
		size_t frame_size = 0u;
		if (monitor_simulator_next_frame(&state, frame, sizeof(frame), &frame_size) != 0 ||
			write_all(master_fd, frame, frame_size) != 0)
		{
			perror("simulator write");
			close(slave_fd);
			close(master_fd);
			return 1;
		}
		if (frames == 0u || state.frame_count < frames)
		{
			sleep_ms(state.interval_ms);
		}
	}

	close(slave_fd);
	close(master_fd);
	return 0;
}


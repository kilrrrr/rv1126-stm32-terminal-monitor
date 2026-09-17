#define _GNU_SOURCE

#include "serial_port.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <termios.h>
#include <unistd.h>

static speed_t baud_to_termios(int baudrate)
{
	switch (baudrate)
	{
	case 9600: return B9600;
	case 19200: return B19200;
	case 38400: return B38400;
	case 57600: return B57600;
	case 115200: return B115200;
	default: return 0;
	}
}

int serial_open(const char *path, int baudrate)
{
	int fd;
	struct termios settings;
	speed_t speed = baud_to_termios(baudrate);

	if (path == NULL || speed == 0)
	{
		return -1;
	}
	fd = open(path, O_RDWR | O_NOCTTY | O_CLOEXEC);
	if (fd < 0 || tcgetattr(fd, &settings) != 0)
	{
		if (fd >= 0) close(fd);
		return -1;
	}
	cfmakeraw(&settings);
	cfsetispeed(&settings, speed);
	cfsetospeed(&settings, speed);
	settings.c_cflag |= CLOCAL | CREAD;
	settings.c_cflag &= ~CSTOPB;
	settings.c_cflag &= ~CRTSCTS;
	settings.c_cflag &= ~CSIZE;
	settings.c_cflag |= CS8;
	settings.c_cc[VMIN] = 0;
	settings.c_cc[VTIME] = 0;
	if (tcsetattr(fd, TCSANOW, &settings) != 0)
	{
		close(fd);
		return -1;
	}
	return fd;
}

int serial_close(int fd)
{
	return close(fd);
}

int serial_write_all(int fd, const uint8_t *data, size_t length)
{
	size_t written = 0u;

	while (written < length)
	{
		ssize_t result = write(fd, &data[written], length - written);
		if (result < 0)
		{
			if (errno == EINTR) continue;
			return -1;
		}
		if (result == 0) return -1;
		written += (size_t)result;
	}
	return 0;
}

int serial_read_poll(int fd, uint8_t *data, size_t capacity, int timeout_ms)
{
	struct pollfd descriptor = {fd, POLLIN, 0};
	int ready;

	if (data == NULL || capacity == 0u)
	{
		return -1;
	}
	ready = poll(&descriptor, 1u, timeout_ms);
	if (ready <= 0)
	{
		return ready;
	}
	if ((descriptor.revents & (POLLERR | POLLHUP | POLLNVAL)) != 0)
	{
		return -1;
	}
	return (int)read(fd, data, capacity);
}


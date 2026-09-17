#ifndef TERMINAL_MONITOR_SERIAL_PORT_H
#define TERMINAL_MONITOR_SERIAL_PORT_H

#include <stddef.h>
#include <stdint.h>

int serial_open(const char *path, int baudrate);
int serial_close(int fd);
int serial_write_all(int fd, const uint8_t *data, size_t length);
int serial_read_poll(int fd, uint8_t *data, size_t capacity, int timeout_ms);

#endif


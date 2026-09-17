#include <assert.h>
#include <stdint.h>
#include <unistd.h>

#include "../linux/serial_port.h"

static void test_reads_data_before_peer_hup(void)
{
	int pipe_fds[2];
	uint8_t expected[] = {0xA5u, 0x5Au, 0x01u};
	uint8_t actual[sizeof(expected)] = {0};

	assert(pipe(pipe_fds) == 0);
	assert(write(pipe_fds[1], expected, sizeof(expected)) == (ssize_t)sizeof(expected));
	assert(close(pipe_fds[1]) == 0);
	assert(serial_read_poll(pipe_fds[0], actual, sizeof(actual), 0) ==
		(int)sizeof(expected));
	assert(actual[0] == expected[0]);
	assert(actual[1] == expected[1]);
	assert(actual[2] == expected[2]);
	assert(close(pipe_fds[0]) == 0);
}

int main(void)
{
	test_reads_data_before_peer_hup();
	return 0;
}


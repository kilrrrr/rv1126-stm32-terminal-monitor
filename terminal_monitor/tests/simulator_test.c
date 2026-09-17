#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include "../common/protocol.h"
#include "../linux/monitor_simulator_core.h"

static void test_generates_deterministic_status(void)
{
	monitor_simulator_state_t state;
	uint8_t frame[TERMINAL_MAX_FRAME_SIZE];
	size_t frame_size = 0u;
	terminal_frame_t decoded_frame;
	terminal_status_t status;

	monitor_simulator_init(&state, 100u, 0u, 0u);
	assert(monitor_simulator_next_frame(&state, frame, sizeof(frame), &frame_size) == 0);
	assert(terminal_decode_frame(frame, frame_size, &decoded_frame) == 0);
	assert(terminal_decode_status_frame(&decoded_frame, &status) == 0);
	assert(status.sequence == 0u);
	assert(status.uptime_ms == 100u);
	assert(status.voltage_mv == 12000u);
	assert(status.temperature_c10 == 250);
	assert(status.fan_rpm == 1200u);
	assert(status.fan_duty_pct == 30u);
	assert(status.light_duty_pct == 10u);
	assert(status.fault_bits == 0u);
}

static void test_injects_sequence_gap_and_bad_crc(void)
{
	monitor_simulator_state_t state;
	uint8_t frame[TERMINAL_MAX_FRAME_SIZE];
	size_t frame_size = 0u;
	terminal_frame_t decoded_frame;
	terminal_status_t status;

	monitor_simulator_init(&state, 50u, 2u, 3u);
	assert(monitor_simulator_next_frame(&state, frame, sizeof(frame), &frame_size) == 0);
	assert(terminal_decode_frame(frame, frame_size, &decoded_frame) == 0);
	assert(terminal_decode_status_frame(&decoded_frame, &status) == 0);
	assert(status.sequence == 0u);

	assert(monitor_simulator_next_frame(&state, frame, sizeof(frame), &frame_size) == 0);
	assert(terminal_decode_frame(frame, frame_size, &decoded_frame) == 0);
	assert(terminal_decode_status_frame(&decoded_frame, &status) == 0);
	assert(status.sequence == 2u);

	assert(monitor_simulator_next_frame(&state, frame, sizeof(frame), &frame_size) == 0);
	assert(terminal_decode_frame(frame, frame_size, &decoded_frame) != 0);
}

int main(void)
{
	test_generates_deterministic_status();
	test_injects_sequence_gap_and_bad_crc();
	puts("simulator tests passed");
	return 0;
}


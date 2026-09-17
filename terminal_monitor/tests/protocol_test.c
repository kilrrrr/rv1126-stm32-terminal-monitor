#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../common/protocol.h"

static void test_encode_decode_status(void)
{
	terminal_status_t in = {
		.sequence = 42,
		.uptime_ms = 123456,
		.voltage_mv = 12050,
		.temperature_c10 = 356,
		.fan_rpm = 1680,
		.fan_duty_pct = 42,
		.light_duty_pct = 15,
		.fault_bits = 0,
	};
	terminal_status_t out;
	uint8_t frame[TERMINAL_MAX_FRAME_SIZE];
	size_t frame_size = 0;

	assert(terminal_encode_status(&in, frame, sizeof(frame), &frame_size) == 0);
	assert(terminal_decode_status(frame, frame_size, &out) == 0);
	assert(in.sequence == out.sequence);
	assert(in.uptime_ms == out.uptime_ms);
	assert(in.voltage_mv == out.voltage_mv);
	assert(in.temperature_c10 == out.temperature_c10);
	assert(in.fan_rpm == out.fan_rpm);
	assert(in.fan_duty_pct == out.fan_duty_pct);
	assert(in.light_duty_pct == out.light_duty_pct);
	assert(in.fault_bits == out.fault_bits);
}

static void test_rejects_corrupted_crc(void)
{
	terminal_status_t in = {0};
	uint8_t frame[TERMINAL_MAX_FRAME_SIZE];
	size_t frame_size = 0;

	assert(terminal_encode_status(&in, frame, sizeof(frame), &frame_size) == 0);
	frame[frame_size - 1u] ^= 0x01u;
	assert(terminal_decode_status(frame, frame_size, &in) != 0);
}

static void parser_callback(const terminal_frame_t *frame, void *user)
{
	unsigned int *count = (unsigned int *)user;
	assert(frame->type == TERMINAL_MSG_STATUS);
	assert(frame->payload_length == 16u);
	(*count)++;
}

static void test_parser_accepts_fragmented_frame(void)
{
	terminal_status_t status = {.sequence = 7u};
	uint8_t frame[TERMINAL_MAX_FRAME_SIZE];
	size_t frame_size = 0u;
	terminal_parser_t parser;
	unsigned int callback_count = 0u;
	size_t split;

	assert(terminal_encode_status(&status, frame, sizeof(frame), &frame_size) == 0);
	terminal_parser_init(&parser);
	split = frame_size / 2u;
	terminal_parser_feed(&parser, frame, split, parser_callback, &callback_count);
	assert(callback_count == 0u);
	terminal_parser_feed(&parser, &frame[split], frame_size - split,
		parser_callback, &callback_count);
	assert(callback_count == 1u);
}

int main(void)
{
	test_encode_decode_status();
	test_rejects_corrupted_crc();
	test_parser_accepts_fragmented_frame();
	puts("protocol tests passed");
	return 0;
}


#ifndef TERMINAL_MONITOR_PROTOCOL_H
#define TERMINAL_MONITOR_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#define TERMINAL_SOF0 0xA5u
#define TERMINAL_SOF1 0x5Au
#define TERMINAL_PROTOCOL_VERSION 1u
#define TERMINAL_MAX_PAYLOAD_SIZE 64u
#define TERMINAL_MAX_FRAME_SIZE (10u + TERMINAL_MAX_PAYLOAD_SIZE)

enum terminal_message_type
{
	TERMINAL_MSG_STATUS = 0x01u,
	TERMINAL_MSG_COMMAND = 0x02u,
	TERMINAL_MSG_ACK = 0x03u,
};

enum terminal_command_id
{
	TERMINAL_CMD_SET_FAN_DUTY = 0x01u,
	TERMINAL_CMD_SET_LIGHT_DUTY = 0x02u,
	TERMINAL_CMD_CLEAR_FAULTS = 0x03u,
};

typedef struct
{
	uint16_t sequence;
	uint32_t uptime_ms;
	uint16_t voltage_mv;
	int16_t temperature_c10;
	uint16_t fan_rpm;
	uint8_t fan_duty_pct;
	uint8_t light_duty_pct;
	uint16_t fault_bits;
} terminal_status_t;

typedef struct
{
	uint8_t command_id;
	uint8_t value;
} terminal_command_t;

typedef struct
{
	uint8_t version;
	uint8_t type;
	uint16_t payload_length;
	uint16_t sequence;
	uint8_t payload[TERMINAL_MAX_PAYLOAD_SIZE];
} terminal_frame_t;

typedef void (*terminal_frame_callback)(const terminal_frame_t *frame, void *user);

typedef struct
{
	uint8_t buffer[TERMINAL_MAX_FRAME_SIZE];
	size_t length;
} terminal_parser_t;

uint16_t terminal_crc16(const uint8_t *data, size_t length);

int terminal_encode_status(const terminal_status_t *status,
	uint8_t *output, size_t capacity, size_t *output_length);
int terminal_decode_status(const uint8_t *input, size_t input_length,
	terminal_status_t *status);
int terminal_decode_status_frame(const terminal_frame_t *frame,
	terminal_status_t *status);

int terminal_encode_command(const terminal_command_t *command, uint16_t sequence,
	uint8_t *output, size_t capacity, size_t *output_length);
int terminal_decode_command(const terminal_frame_t *frame,
	terminal_command_t *command);

int terminal_encode_frame(uint8_t type, uint16_t sequence,
	const uint8_t *payload, uint16_t payload_length,
	uint8_t *output, size_t capacity, size_t *output_length);
int terminal_decode_frame(const uint8_t *input, size_t input_length,
	terminal_frame_t *frame);

void terminal_parser_init(terminal_parser_t *parser);
void terminal_parser_feed(terminal_parser_t *parser,
	const uint8_t *data, size_t length,
	terminal_frame_callback callback, void *user);

#endif


#include "protocol.h"

#include <string.h>

#define TERMINAL_HEADER_SIZE 8u
#define TERMINAL_CRC_SIZE 2u

static uint16_t read_u16(const uint8_t *data)
{
	return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t read_u32(const uint8_t *data)
{
	return (uint32_t)data[0] |
		((uint32_t)data[1] << 8) |
		((uint32_t)data[2] << 16) |
		((uint32_t)data[3] << 24);
}

static void write_u16(uint8_t *data, uint16_t value)
{
	data[0] = (uint8_t)(value & 0xFFu);
	data[1] = (uint8_t)(value >> 8);
}

static void write_u32(uint8_t *data, uint32_t value)
{
	data[0] = (uint8_t)(value & 0xFFu);
	data[1] = (uint8_t)((value >> 8) & 0xFFu);
	data[2] = (uint8_t)((value >> 16) & 0xFFu);
	data[3] = (uint8_t)(value >> 24);
}

uint16_t terminal_crc16(const uint8_t *data, size_t length)
{
	uint16_t crc = 0xFFFFu;
	size_t i;

	for (i = 0; i < length; ++i)
	{
		uint8_t bit;
		crc ^= (uint16_t)data[i] << 8;
		for (bit = 0; bit < 8u; ++bit)
		{
			if ((crc & 0x8000u) != 0u)
			{
				crc = (uint16_t)((crc << 1) ^ 0x1021u);
			}
			else
			{
				crc <<= 1;
			}
		}
	}
	return crc;
}

int terminal_encode_frame(uint8_t type, uint16_t sequence,
	const uint8_t *payload, uint16_t payload_length,
	uint8_t *output, size_t capacity, size_t *output_length)
{
	size_t total_length;
	uint16_t crc;

	if (output == NULL || output_length == NULL ||
		(payload == NULL && payload_length != 0u) ||
		payload_length > TERMINAL_MAX_PAYLOAD_SIZE)
	{
		return -1;
	}

	total_length = TERMINAL_HEADER_SIZE + payload_length + TERMINAL_CRC_SIZE;
	if (capacity < total_length)
	{
		return -2;
	}

	output[0] = TERMINAL_SOF0;
	output[1] = TERMINAL_SOF1;
	output[2] = TERMINAL_PROTOCOL_VERSION;
	output[3] = type;
	write_u16(&output[4], payload_length);
	write_u16(&output[6], sequence);
	if (payload_length != 0u)
	{
		memcpy(&output[TERMINAL_HEADER_SIZE], payload, payload_length);
	}

	crc = terminal_crc16(&output[2], 6u + payload_length);
	write_u16(&output[TERMINAL_HEADER_SIZE + payload_length], crc);
	*output_length = total_length;
	return 0;
}

int terminal_decode_frame(const uint8_t *input, size_t input_length,
	terminal_frame_t *frame)
{
	size_t expected_length;
	uint16_t stored_crc;
	uint16_t calculated_crc;

	if (input == NULL || frame == NULL || input_length < 10u ||
		input[0] != TERMINAL_SOF0 || input[1] != TERMINAL_SOF1)
	{
		return -1;
	}
	if (input[2] != TERMINAL_PROTOCOL_VERSION)
	{
		return -2;
	}
	frame->payload_length = read_u16(&input[4]);
	if (frame->payload_length > TERMINAL_MAX_PAYLOAD_SIZE)
	{
		return -3;
	}
	expected_length = TERMINAL_HEADER_SIZE + frame->payload_length + TERMINAL_CRC_SIZE;
	if (input_length != expected_length)
	{
		return -4;
	}

	stored_crc = read_u16(&input[TERMINAL_HEADER_SIZE + frame->payload_length]);
	calculated_crc = terminal_crc16(&input[2], 6u + frame->payload_length);
	if (stored_crc != calculated_crc)
	{
		return -5;
	}

	frame->version = input[2];
	frame->type = input[3];
	frame->sequence = read_u16(&input[6]);
	if (frame->payload_length != 0u)
	{
		memcpy(frame->payload, &input[TERMINAL_HEADER_SIZE], frame->payload_length);
	}
	return 0;
}

int terminal_encode_status(const terminal_status_t *status,
	uint8_t *output, size_t capacity, size_t *output_length)
{
	uint8_t payload[16];

	if (status == NULL)
	{
		return -1;
	}
	write_u16(&payload[0], status->sequence);
	write_u32(&payload[2], status->uptime_ms);
	write_u16(&payload[6], status->voltage_mv);
	write_u16(&payload[8], (uint16_t)status->temperature_c10);
	write_u16(&payload[10], status->fan_rpm);
	payload[12] = status->fan_duty_pct;
	payload[13] = status->light_duty_pct;
	write_u16(&payload[14], status->fault_bits);

	return terminal_encode_frame(TERMINAL_MSG_STATUS, status->sequence,
		payload, sizeof(payload), output, capacity, output_length);
}

int terminal_decode_status(const uint8_t *input, size_t input_length,
	terminal_status_t *status)
{
	terminal_frame_t frame;

	if (status == NULL || terminal_decode_frame(input, input_length, &frame) != 0)
	{
		return -1;
	}
	return terminal_decode_status_frame(&frame, status);
}

int terminal_decode_status_frame(const terminal_frame_t *frame,
	terminal_status_t *status)
{
	if (status == NULL || frame == NULL || frame->type != TERMINAL_MSG_STATUS ||
		frame->payload_length != 16u)
	{
		return -1;
	}

	status->sequence = read_u16(&frame->payload[0]);
	status->uptime_ms = read_u32(&frame->payload[2]);
	status->voltage_mv = read_u16(&frame->payload[6]);
	status->temperature_c10 = (int16_t)read_u16(&frame->payload[8]);
	status->fan_rpm = read_u16(&frame->payload[10]);
	status->fan_duty_pct = frame->payload[12];
	status->light_duty_pct = frame->payload[13];
	status->fault_bits = read_u16(&frame->payload[14]);
	return 0;
}

int terminal_encode_command(const terminal_command_t *command, uint16_t sequence,
	uint8_t *output, size_t capacity, size_t *output_length)
{
	uint8_t payload[2];

	if (command == NULL || command->value > 100u)
	{
		return -1;
	}
	payload[0] = command->command_id;
	payload[1] = command->value;
	return terminal_encode_frame(TERMINAL_MSG_COMMAND, sequence, payload,
		sizeof(payload), output, capacity, output_length);
}

int terminal_decode_command(const terminal_frame_t *frame,
	terminal_command_t *command)
{
	if (frame == NULL || command == NULL || frame->type != TERMINAL_MSG_COMMAND ||
		frame->payload_length != 2u || frame->payload[1] > 100u)
	{
		return -1;
	}
	command->command_id = frame->payload[0];
	command->value = frame->payload[1];
	return 0;
}

void terminal_parser_init(terminal_parser_t *parser)
{
	if (parser != NULL)
	{
		parser->length = 0u;
	}
}

void terminal_parser_feed(terminal_parser_t *parser,
	const uint8_t *data, size_t length,
	terminal_frame_callback callback, void *user)
{
	size_t i;

	if (parser == NULL || data == NULL || callback == NULL)
	{
		return;
	}
	for (i = 0; i < length; ++i)
	{
		uint8_t byte = data[i];
		terminal_frame_t frame;
		size_t expected_length;

		if (parser->length == 0u)
		{
			if (byte == TERMINAL_SOF0)
			{
				parser->buffer[parser->length++] = byte;
			}
			continue;
		}
		if (parser->length == 1u && byte != TERMINAL_SOF1)
		{
			parser->length = (byte == TERMINAL_SOF0) ? 1u : 0u;
			if (parser->length == 1u)
			{
				parser->buffer[0] = byte;
			}
			continue;
		}
		if (parser->length >= TERMINAL_MAX_FRAME_SIZE)
		{
			parser->length = 0u;
			continue;
		}
		parser->buffer[parser->length++] = byte;
		if (parser->length < TERMINAL_HEADER_SIZE)
		{
			continue;
		}

		expected_length = TERMINAL_HEADER_SIZE +
			read_u16(&parser->buffer[4]) + TERMINAL_CRC_SIZE;
		if (expected_length > TERMINAL_MAX_FRAME_SIZE)
		{
			parser->length = 0u;
			continue;
		}
		if (parser->length == expected_length)
		{
			if (terminal_decode_frame(parser->buffer, parser->length, &frame) == 0)
			{
				callback(&frame, user);
			}
			parser->length = 0u;
		}
	}
}


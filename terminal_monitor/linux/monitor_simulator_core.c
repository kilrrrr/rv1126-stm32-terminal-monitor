#include "monitor_simulator_core.h"

#include <stddef.h>

void monitor_simulator_init(monitor_simulator_state_t *state,
	uint32_t interval_ms, uint32_t drop_every, uint32_t bad_crc_every)
{
	if (state == NULL)
	{
		return;
	}
	state->sequence = 0u;
	state->uptime_ms = 0u;
	state->interval_ms = (interval_ms == 0u) ? 1000u : interval_ms;
	state->frame_count = 0u;
	state->drop_every = drop_every;
	state->bad_crc_every = bad_crc_every;
	state->status.voltage_mv = 12000u;
	state->status.temperature_c10 = 250;
	state->status.fan_rpm = 1200u;
	state->status.fan_duty_pct = 30u;
	state->status.light_duty_pct = 10u;
	state->status.fault_bits = 0u;
}

int monitor_simulator_next_frame(monitor_simulator_state_t *state,
	uint8_t *output, size_t capacity, size_t *output_length)
{
	terminal_status_t status;
	uint32_t frame_number;

	if (state == NULL || output == NULL || output_length == NULL)
	{
		return -1;
	}
	frame_number = state->frame_count + 1u;
	if (state->drop_every != 0u && frame_number > 1u &&
		(frame_number % state->drop_every) == 0u)
	{
		state->sequence++;
	}
	state->uptime_ms += state->interval_ms;
	status = state->status;
	status.sequence = state->sequence;
	status.uptime_ms = state->uptime_ms;
	if (terminal_encode_status(&status, output, capacity, output_length) != 0)
	{
		return -1;
	}
	state->sequence++;
	state->frame_count = frame_number;
	if (state->bad_crc_every != 0u &&
		(frame_number % state->bad_crc_every) == 0u && *output_length > 0u)
	{
		output[*output_length - 1u] ^= 0x01u;
	}
	return 0;
}


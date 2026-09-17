#ifndef TERMINAL_MONITOR_SIMULATOR_CORE_H
#define TERMINAL_MONITOR_SIMULATOR_CORE_H

#include "../common/protocol.h"

typedef struct
{
	uint16_t sequence;
	uint32_t uptime_ms;
	uint32_t interval_ms;
	uint32_t frame_count;
	uint32_t drop_every;
	uint32_t bad_crc_every;
	terminal_status_t status;
} monitor_simulator_state_t;

void monitor_simulator_init(monitor_simulator_state_t *state,
	uint32_t interval_ms, uint32_t drop_every, uint32_t bad_crc_every);

int monitor_simulator_next_frame(monitor_simulator_state_t *state,
	uint8_t *output, size_t capacity, size_t *output_length);

#endif


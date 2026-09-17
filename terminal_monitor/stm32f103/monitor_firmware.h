#ifndef TERMINAL_MONITOR_FIRMWARE_H
#define TERMINAL_MONITOR_FIRMWARE_H

#include <stddef.h>
#include <stdint.h>

#include "../common/protocol.h"

#define TERMINAL_FAULT_UNDERVOLTAGE (1u << 0)
#define TERMINAL_FAULT_OVERVOLTAGE  (1u << 1)
#define TERMINAL_FAULT_OVERTEMP     (1u << 2)
#define TERMINAL_FAULT_FAN_STALL    (1u << 3)
#define TERMINAL_FAULT_SENSOR_READ  (1u << 4)

typedef struct
{
	int (*read_voltage_mv)(void *ctx, uint16_t *value);
	int (*read_temperature_c10)(void *ctx, int16_t *value);
	int (*read_fan_rpm)(void *ctx, uint16_t *value);
	int (*set_fan_duty)(void *ctx, uint8_t duty_pct);
	int (*set_light_duty)(void *ctx, uint8_t duty_pct);
	int (*uart_send)(void *ctx, const uint8_t *data, size_t length);
	void *ctx;
} monitor_hal_t;

typedef struct
{
	uint16_t undervoltage_mv;
	uint16_t overvoltage_mv;
	int16_t overtemperature_c10;
	uint16_t fan_stall_rpm;
} monitor_limits_t;

typedef struct
{
	monitor_hal_t hal;
	monitor_limits_t limits;
	terminal_status_t status;
	uint16_t next_sequence;
	uint8_t requested_fan_duty;
	uint8_t requested_light_duty;
} monitor_firmware_t;

void monitor_firmware_init(monitor_firmware_t *firmware,
	const monitor_hal_t *hal, const monitor_limits_t *limits);
int monitor_firmware_tick(monitor_firmware_t *firmware, uint32_t now_ms);
int monitor_firmware_on_frame(monitor_firmware_t *firmware,
	const uint8_t *data, size_t length);

#endif


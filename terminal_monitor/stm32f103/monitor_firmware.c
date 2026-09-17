#include "monitor_firmware.h"

#include <string.h>

static uint16_t calculate_faults(const monitor_firmware_t *firmware)
{
	uint16_t faults = 0u;

	if (firmware->status.voltage_mv < firmware->limits.undervoltage_mv)
	{
		faults |= TERMINAL_FAULT_UNDERVOLTAGE;
	}
	if (firmware->status.voltage_mv > firmware->limits.overvoltage_mv)
	{
		faults |= TERMINAL_FAULT_OVERVOLTAGE;
	}
	if (firmware->status.temperature_c10 > firmware->limits.overtemperature_c10)
	{
		faults |= TERMINAL_FAULT_OVERTEMP;
	}
	if (firmware->status.fan_rpm < firmware->limits.fan_stall_rpm &&
		firmware->requested_fan_duty != 0u)
	{
		faults |= TERMINAL_FAULT_FAN_STALL;
	}
	return faults;
}

static int apply_outputs(monitor_firmware_t *firmware)
{
	uint8_t light_duty = firmware->requested_light_duty;

	if ((firmware->status.fault_bits &
		(TERMINAL_FAULT_UNDERVOLTAGE | TERMINAL_FAULT_OVERVOLTAGE |
		 TERMINAL_FAULT_OVERTEMP)) != 0u)
	{
		light_duty = 0u;
	}

	if (firmware->hal.set_fan_duty != NULL &&
		firmware->hal.set_fan_duty(firmware->hal.ctx,
		firmware->requested_fan_duty) != 0)
	{
		return -1;
	}
	if (firmware->hal.set_light_duty != NULL &&
		firmware->hal.set_light_duty(firmware->hal.ctx, light_duty) != 0)
	{
		return -1;
	}
	firmware->status.fan_duty_pct = firmware->requested_fan_duty;
	firmware->status.light_duty_pct = light_duty;
	return 0;
}

void monitor_firmware_init(monitor_firmware_t *firmware,
	const monitor_hal_t *hal, const monitor_limits_t *limits)
{
	memset(firmware, 0, sizeof(*firmware));
	if (hal != NULL)
	{
		firmware->hal = *hal;
	}
	if (limits != NULL)
	{
		firmware->limits = *limits;
	}
}

int monitor_firmware_tick(monitor_firmware_t *firmware, uint32_t now_ms)
{
	uint16_t voltage_mv;
	int16_t temperature_c10;
	uint16_t fan_rpm;
	uint8_t frame[TERMINAL_MAX_FRAME_SIZE];
	size_t frame_length = 0u;

	if (firmware == NULL)
	{
		return -1;
	}
	firmware->status.sequence = ++firmware->next_sequence;
	firmware->status.uptime_ms = now_ms;
	if (firmware->hal.read_voltage_mv == NULL ||
		firmware->hal.read_voltage_mv(firmware->hal.ctx, &voltage_mv) != 0)
	{
		firmware->status.fault_bits |= TERMINAL_FAULT_SENSOR_READ;
	}
	else
	{
		firmware->status.voltage_mv = voltage_mv;
	}
	if (firmware->hal.read_temperature_c10 == NULL ||
		firmware->hal.read_temperature_c10(firmware->hal.ctx, &temperature_c10) != 0)
	{
		firmware->status.fault_bits |= TERMINAL_FAULT_SENSOR_READ;
	}
	else
	{
		firmware->status.temperature_c10 = temperature_c10;
	}
	if (firmware->hal.read_fan_rpm == NULL ||
		firmware->hal.read_fan_rpm(firmware->hal.ctx, &fan_rpm) != 0)
	{
		firmware->status.fault_bits |= TERMINAL_FAULT_SENSOR_READ;
	}
	else
	{
		firmware->status.fan_rpm = fan_rpm;
	}
	firmware->status.fault_bits &= TERMINAL_FAULT_SENSOR_READ;
	firmware->status.fault_bits |= calculate_faults(firmware);
	if (apply_outputs(firmware) != 0)
	{
		return -2;
	}
	if (firmware->hal.uart_send == NULL ||
		terminal_encode_status(&firmware->status, frame, sizeof(frame), &frame_length) != 0 ||
		firmware->hal.uart_send(firmware->hal.ctx, frame, frame_length) != 0)
	{
		return -3;
	}
	return 0;
}

int monitor_firmware_on_frame(monitor_firmware_t *firmware,
	const uint8_t *data, size_t length)
{
	terminal_frame_t frame;
	terminal_command_t command;

	if (firmware == NULL || terminal_decode_frame(data, length, &frame) != 0 ||
		terminal_decode_command(&frame, &command) != 0)
	{
		return -1;
	}
	if (command.command_id == TERMINAL_CMD_SET_FAN_DUTY)
	{
		firmware->requested_fan_duty = command.value;
	}
	else if (command.command_id == TERMINAL_CMD_SET_LIGHT_DUTY)
	{
		firmware->requested_light_duty = command.value;
	}
	else if (command.command_id == TERMINAL_CMD_CLEAR_FAULTS)
	{
		firmware->status.fault_bits = 0u;
	}
	else
	{
		return -2;
	}
	return apply_outputs(firmware);
}


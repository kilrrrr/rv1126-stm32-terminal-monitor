#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "../common/protocol.h"
#include "../stm32f103/monitor_firmware.h"

typedef struct
{
	uint16_t voltage_mv;
	int16_t temperature_c10;
	uint16_t fan_rpm;
	uint8_t fan_duty;
	uint8_t light_duty;
	uint8_t tx_frame[TERMINAL_MAX_FRAME_SIZE];
	size_t tx_length;
} fake_hal_t;

static int fake_voltage(void *ctx, uint16_t *value)
{
	*value = ((fake_hal_t *)ctx)->voltage_mv;
	return 0;
}

static int fake_temperature(void *ctx, int16_t *value)
{
	*value = ((fake_hal_t *)ctx)->temperature_c10;
	return 0;
}

static int fake_rpm(void *ctx, uint16_t *value)
{
	*value = ((fake_hal_t *)ctx)->fan_rpm;
	return 0;
}

static int fake_fan(void *ctx, uint8_t duty)
{
	((fake_hal_t *)ctx)->fan_duty = duty;
	return 0;
}

static int fake_light(void *ctx, uint8_t duty)
{
	((fake_hal_t *)ctx)->light_duty = duty;
	return 0;
}

static int fake_send(void *ctx, const uint8_t *data, size_t length)
{
	fake_hal_t *hal = (fake_hal_t *)ctx;
	assert(length <= sizeof(hal->tx_frame));
	memcpy(hal->tx_frame, data, length);
	hal->tx_length = length;
	return 0;
}

static void test_tick_reports_status(void)
{
	fake_hal_t fake = {12000u, 250, 1500u, 0u, 0u, {0}, 0u};
	monitor_hal_t hal = {
		.read_voltage_mv = fake_voltage,
		.read_temperature_c10 = fake_temperature,
		.read_fan_rpm = fake_rpm,
		.set_fan_duty = fake_fan,
		.set_light_duty = fake_light,
		.uart_send = fake_send,
		.ctx = &fake,
	};
	monitor_limits_t limits = {11000u, 13000u, 800, 200u};
	monitor_firmware_t firmware;
	terminal_status_t status;

	monitor_firmware_init(&firmware, &hal, &limits);
	assert(monitor_firmware_tick(&firmware, 100u) == 0);
	assert(terminal_decode_status(fake.tx_frame, fake.tx_length, &status) == 0);
	assert(status.voltage_mv == 12000u);
	assert(status.temperature_c10 == 250);
	assert(status.fan_rpm == 1500u);
	assert(status.fault_bits == 0u);
}

static void test_light_command_is_limited_by_overtemperature(void)
{
	fake_hal_t fake = {12000u, 900, 1500u, 0u, 0u, {0}, 0u};
	monitor_hal_t hal = {
		.read_voltage_mv = fake_voltage,
		.read_temperature_c10 = fake_temperature,
		.read_fan_rpm = fake_rpm,
		.set_fan_duty = fake_fan,
		.set_light_duty = fake_light,
		.uart_send = fake_send,
		.ctx = &fake,
	};
	monitor_limits_t limits = {11000u, 13000u, 800, 200u};
	monitor_firmware_t firmware;
	uint8_t frame[TERMINAL_MAX_FRAME_SIZE];
	size_t frame_length = 0u;
	terminal_command_t command = {TERMINAL_CMD_SET_LIGHT_DUTY, 80u};

	monitor_firmware_init(&firmware, &hal, &limits);
	assert(monitor_firmware_tick(&firmware, 100u) == 0);
	assert(terminal_encode_command(&command, 1u, frame, sizeof(frame), &frame_length) == 0);
	assert(monitor_firmware_on_frame(&firmware, frame, frame_length) == 0);
	assert(fake.light_duty == 0u);
}

int main(void)
{
	test_tick_reports_status();
	test_light_command_is_limited_by_overtemperature();
	puts("firmware tests passed");
	return 0;
}


# STM32F103C8T6 硬件适配说明

`monitor_firmware.c` 只包含与硬件无关的采集、告警和控制逻辑，硬件工程通过
`monitor_hal_t` 提供 ADC、I2C、Timer 和 USART 的回调。

当前原理图确认的是最小系统板，不包含温度传感器、风扇驱动或 RS485 收发器。
因此，在 CubeMX/Keil 工程中需要根据实际外接模块完成以下适配：

1. USART1（PA9/PA10）连接 3.3 V USB-TTL，后续可接 RS485 收发器；
2. I2C1（PB6/PB7）连接外部温度传感器；
3. 一个 ADC 输入连接经过分压的电压信号；
4. 一个 PWM 输出连接风扇或补光灯驱动；
5. 一个输入捕获通道连接风扇 FG 脉冲。

实际引脚复用、ADC 通道、定时器通道、分压电阻和传感器地址必须按最终板卡
原理图和 STM32F103C8T6 数据手册确认，不能由本文件自动推断。

建议在 100 ms 的定时任务中调用 `monitor_firmware_tick()`，在 USART 接收完成
后调用 `monitor_firmware_on_frame()`。


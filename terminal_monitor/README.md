# RV1126 + STM32F103C8T6 终端状态监测第一期

这一目录是独立于原 `music_player` 的状态监测模块。STM32F103C8T6 负责采集电压、温度、风扇转速并执行风扇/补光灯控制；RV1126 Linux 通过 UART 读取状态并发送控制命令。

## 当前代码边界

- `common/`：与平台无关的帧格式、CRC16 和流式解析；
- `stm32f103/`：与硬件无关的采集、告警和安全控制核心；
- `linux/`：Linux 串口服务和命令行控制工具；
- `tests/`：协议和固件核心的主机单元测试。

STM32 的 ADC/I2C/Timer/USART 初始化需要放入具体的 CubeMX/Keil 工程，并通过 `monitor_hal_t` 接入。当前最小系统板原理图没有板载温度传感器、风扇驱动或 RS485 收发器，这些属于外接硬件。

## Linux 构建

在 Linux 目标机或交叉编译环境执行：

```text
make -C terminal_monitor/linux
make -C terminal_monitor/tests check
```

运行监测服务：

```text
./terminal_monitor/linux/monitor_daemon /dev/ttyUSB0
```

设置风扇或补光灯占空比：

```text
./terminal_monitor/linux/monitor_cli /dev/ttyUSB0 set-fan 50
./terminal_monitor/linux/monitor_cli /dev/ttyUSB0 set-light 20
```


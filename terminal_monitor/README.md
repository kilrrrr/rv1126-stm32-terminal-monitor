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

## 无硬件模拟验证

Linux 端提供 `monitor_simulator`，通过伪终端模拟 STM32 的 UART 输出，不需要连接开发板即可验证协议解析和监测服务。

终端 1 启动模拟节点（发送 5 帧，间隔 200 ms）：

```text
./terminal_monitor/linux/monitor_simulator --frames 5 --interval-ms 200
```

程序会打印类似 `SIM_DEVICE=/dev/pts/3` 的设备路径。终端 2 使用该路径启动守护进程：

```text
./terminal_monitor/linux/monitor_daemon /dev/pts/3
```

可以注入通信异常：

```text
./terminal_monitor/linux/monitor_simulator --frames 20 \
  --interval-ms 100 --drop-every 4 --bad-crc-every 5
```

其中序号跳变用于观察丢帧，CRC 错误帧应被协议解析器丢弃。该模拟只覆盖协议、解析器和 Linux 服务逻辑；ADC、I2C、定时器捕获、PWM 输出以及 UART 电气层仍需连接 STM32F103 和实际外设后验证。


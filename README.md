# RV1126 + STM32F103C8T6 Terminal Monitor

基于 RV1126 Linux 与 STM32F103C8T6 的终端状态监测及远程诊断第一期实现。

项目代码和文档位于 [terminal_monitor](./terminal_monitor/)：

- Linux 串口监测服务与 CLI；
- STM32 采集、告警和外设控制核心；
- UART 帧协议、CRC16 和流式解析；
- 主机侧单元测试。

当前 STM32 硬件适配仍需接入具体 CubeMX/Keil 工程；最小系统板上的温度传感器、风扇驱动和 RS485 收发器均需外接。详见 [terminal_monitor/README.md](./terminal_monitor/README.md)。

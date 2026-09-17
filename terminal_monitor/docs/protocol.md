# UART 协议

## 帧格式

```text
SOF0(1) SOF1(1) VERSION(1) TYPE(1) LENGTH_LE(2) SEQ_LE(2)
PAYLOAD(LENGTH) CRC16_CCITT_LE(2)
```

CRC16 的计算范围是 `VERSION` 到 `PAYLOAD`，初值 `0xFFFF`，多项式 `0x1021`。

## STATUS payload

| 偏移 | 长度 | 字段 |
|---:|---:|---|
| 0 | 2 | status sequence |
| 2 | 4 | uptime_ms |
| 6 | 2 | voltage_mv |
| 8 | 2 | temperature_c10，有符号，单位 0.1℃ |
| 10 | 2 | fan_rpm |
| 12 | 1 | fan_duty_pct |
| 13 | 1 | light_duty_pct |
| 14 | 2 | fault_bits |

## COMMAND payload

| 偏移 | 长度 | 字段 |
|---:|---:|---|
| 0 | 1 | command_id |
| 1 | 1 | value，当前用于 0～100 占空比 |

命令值：`0x01` 设置风扇占空比，`0x02` 设置补光灯占空比，`0x03` 清除故障标志。


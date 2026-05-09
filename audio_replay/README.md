# Audio_Replay — MSPM0G3519 音频录放系统

基于 **TI MSPM0G3519** (Cortex-M0+, 80MHz) 的嵌入式音频录放系统。

## 功能

- **录音** — ADC12 采样 (16kHz / 32kHz)，DMA 双缓冲，写入 W25Q64 SPI Flash (8MB，最长 240s)
- **播放** — DAC12 输出，支持正常播放、倒放、2 倍速
- **IIR 滤波** — 4 级 Biquad 级联 (CMSIS-DSP Q31)，80Hz 高通 + 7.2kHz 低通，可选开关
- **FFT 频率分析** — 2048 点 RFFT Q15，Hann 窗 + 抛物线插值，频率检测 + SNR 测量
- **Bypass 直通** — ADC → DAC 实时旁路
- **UART 传输** — VOFA+ JustFloat 协议，发送原始值或电压值到上位机
- **元数据持久化** — EEPROM 仿真保存录音信息（采样率、滤波开关、ADC min/max）

## 硬件

| 外设 | 用途 |
|---|---|
| ADC12 | 12-bit 音频采样 |
| DAC12 | 12-bit 音频输出 |
| DMA (双通道) | ADC↔缓冲↔DAC 乒乓操作 |
| TimerA / TimerG | ADC 采样 + DAC 输出定时 |
| SPI | W25Q64 Flash 驱动 |
| UART | VOFA+ 上位机通信 |
| I2C (Flash 模拟) | EEPROM 仿真 |
| GPIO | 矩阵键盘 (8 键) |
| OLED (SSD1306) | 状态/菜单/进度条显示 |

## 项目结构

```
audio_replay/
├── BSP/                        # 板级支持包
│   ├── bsp.h                   # 总头文件
│   ├── audio_format.h          # 音频格式定义
│   ├── MenuSys.c / .h          # 状态机 + 菜单系统
│   ├── audio_filter/           # IIR Biquad 滤波器
│   ├── delay/                  # 延时函数
│   ├── eeprom_emulation_type_a/ # EEPROM 仿真驱动
│   ├── eeprom_metadata.c / .h  # 录音元数据持久化
│   ├── freq_snr_test.c / .h    # FFT 频率 + SNR 分析
│   └── UartVofa.c / .h         # VOFA+ 串口发送
├── Hardware/                   # 硬件驱动
│   ├── KeyBroad.c / .h         # 矩阵键盘
│   ├── OLED.c / .h             # OLED 显示
│   ├── OLED_Data.c / .h        # OLED 字库
│   └── W25Q64.c / .h           # SPI Flash 驱动
├── User/                       # 用户层
│   ├── main.c                  # 主程序入口
│   ├── ti_msp_dl_config.c / .h # TI 外设配置
│   └── arm_cortexM0l_math.a    # CMSIS-DSP 预编译库
├── calc_filter.py              # 滤波器系数计算脚本 (scipy)
└── .gitignore
```

## 依赖

- **MSPM0 SDK** 2.08.00.03
- **CMSIS-DSP** (arm_cortexM0l_math.a 已包含在仓库中)
- **Keil MDK-ARM** (编译工具链)
- **scipy** (运行 calc_filter.py 需要)

## 滤波器设计

滤波器系数由 `calc_filter.py` 使用 scipy 生成：

- **高通**: 2 阶 Butterworth, fc = 80Hz（去直流 + 低频噪声）
- **低通**: 6 阶 Butterworth, fc = 7.2kHz（抗混叠 + 高频抑制）
- **实现**: Q31 定点, 4 级 Biquad 级联, postShift = 1

## 状态机

按键映射：

| 按键 | 功能 |
|---|---|
| 1 | 录音 (选采样率 + 是否滤波) |
| 2 | 正常播放 / 停止 |
| 3 | 2 倍速播放 |
| 4 | UART 发送到 VOFA+ |
| 5 | 倒放 |
| 6 | 频率 / SNR 测试 |
| 7 | 擦除菜单 |
| 8 | Bypass 直通 |

## License

MIT

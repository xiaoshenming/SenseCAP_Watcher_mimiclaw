# Seeed Studio SenseCAP Watcher 引脚定义

## 芯片信息
- **主控**: ESP32-S3
- **显示驱动**: SPD2010 (QSPI)
- **音频编解码**: ES8311 + ES7243E
- **IO 扩展器**: TCA9555 (I2C 地址 0x21)

---

## GPIO 引脚总览

| GPIO | 功能 | 方向 | 说明 |
|------|------|------|------|
| 0 | BOOT_BUTTON | 输入 | 启动按键 |
| 1 | QSPI_DATA1 | 输出 | QSPI 数据线 1 |
| 2 | IO_EXP_INT | 输入 | IO 扩展器中断 |
| 3 | BAT_ADC | 输入 | 电池电压 (ADC_CH2) |
| 4 | SPI2_SCLK | 输出 | SPI2 时钟 |
| 5 | SPI2_MOSI | 输出 | SPI2 主出从入 |
| 6 | SPI2_MISO | 输入 | SPI2 主入从出 |
| 7 | QSPI_PCLK | 输出 | QSPI 时钟 |
| 8 | LCD_BACKLIGHT | 输出 | LCD 背光 PWM |
| 9 | QSPI_DATA0 | 输出 | QSPI 数据线 0 |
| 10 | I2S_MCLK | 输出 | I2S 主时钟 |
| 11 | I2S_BCLK | 输出 | I2S 位时钟 |
| 12 | I2S_WS | 输出 | I2S 字选择 |
| 13 | QSPI_DATA3 | 输出 | QSPI 数据线 3 |
| 14 | QSPI_DATA2 | 输出 | QSPI 数据线 2 |
| 15 | I2S_DIN | 输入 | I2S 数据输入 (麦克风) |
| 16 | I2S_DOUT | 输出 | I2S 数据输出 (扬声器) |
| 17 | UART1_TX | 输出 | UART1 发送 |
| 18 | UART1_RX | 输入 | UART1 接收 |
| 21 | SSCMA_CS | 输出 | AI 芯片片选 |
| 38 | TOUCH_SCL | 输出 | 触摸屏 I2C 时钟 |
| 39 | TOUCH_SDA | 双向 | 触摸屏 I2C 数据 |
| 40 | RGB_LED | 输出 | WS2813 RGB LED |
| 41 | KNOB_A | 输入 | 旋钮编码器 A 相 |
| 42 | KNOB_B | 输入 | 旋钮编码器 B 相 |
| 45 | LCD_CS | 输出 | LCD 片选 |
| 46 | SD_CS | 输出 | SD 卡片选 |
| 47 | I2C_SDA | 双向 | 主 I2C 数据线 |
| 48 | I2C_SCL | 输出 | 主 I2C 时钟线 |

---

## I2C 总线

### I2C0 (主总线)
- **SDA**: GPIO_47
- **SCL**: GPIO_48
- **设备**: TCA9555 (0x21), ES8311 (0x18), ES7243E (0x14)

### I2C1 (触摸屏)
- **SDA**: GPIO_39
- **SCL**: GPIO_38
- **频率**: 400kHz

---

## SPI 总线

### SPI2 (SD 卡 + AI 芯片)
- **SCLK**: GPIO_4
- **MOSI**: GPIO_5
- **MISO**: GPIO_6
- **SD_CS**: GPIO_46
- **AI_CS**: GPIO_21

### SPI3 (QSPI LCD)
- **PCLK**: GPIO_7
- **DATA0**: GPIO_9
- **DATA1**: GPIO_1
- **DATA2**: GPIO_14
- **DATA3**: GPIO_13
- **LCD_CS**: GPIO_45

---

## I2S 音频

- **MCLK**: GPIO_10
- **BCLK**: GPIO_11
- **WS**: GPIO_12
- **DIN**: GPIO_15 (麦克风输入)
- **DOUT**: GPIO_16 (扬声器输出)

---

## 显示屏

- **分辨率**: 412x412 (圆形)
- **驱动**: SPD2010 (QSPI)
- **背光**: GPIO_8 (PWM)
- **片选**: GPIO_45

---

## 输入设备

### 旋转编码器
- **A 相**: GPIO_41
- **B 相**: GPIO_42
- **按键**: IO_EXP_P0.3

### 按键
- **BOOT**: GPIO_0

---

## 输出设备

### RGB LED
- **引脚**: GPIO_40
- **型号**: WS2813 Mini (兼容 WS2812)

---

## 电源管理

### 电池检测
- **ADC**: GPIO_3 (ADC_CHANNEL_2)
- **衰减**: 2.5dB (0-1100mV)
- **分压比**: 4.1:1

---

## IO 扩展器 (TCA9555)

### 中断引脚
- **INT**: GPIO_2

### 输入引脚 (Port 0)
| 引脚 | 功能 | 说明 |
|------|------|------|
| P0.0 | PWR_CHRG_DET | 充电检测 |
| P0.1 | PWR_STDBY_DET | 待机检测 |
| P0.2 | PWR_VBUS_IN_DET | USB 电源检测 |
| P0.3 | KNOB_BTN | 旋钮按键 |
| P0.4 | SD_GPIO_DET | SD 卡检测 |
| P0.5 | TOUCH_GPIO_INT | 触摸屏中断 |
| P0.6 | SSCMA_SPI_SYNC | AI 芯片同步 |
| P0.7 | SSCMA_CLIENT_RST | AI 芯片复位 |

### 输出引脚 (Port 1)
| 引脚 | 功能 | 说明 |
|------|------|------|
| P1.0 | PWR_SDCARD | SD 卡电源 |
| P1.1 | PWR_LCD | LCD 电源 |
| P1.2 | PWR_SYSTEM | 系统电源 |
| P1.4 | PWR_AI_CHIP | AI 芯片电源 |
| P1.5 | PWR_CODEC_PA | 音频功放电源 |
| P1.6 | PWR_BAT_DET | 电池检测使能 |
| P1.7 | PWR_GROVE | Grove 接口电源 |
| P1.8 | PWR_BAT_ADC | 电池 ADC 使能 |

---

## 参考资料

- 硬件原理图: https://github.com/Seeed-Studio/OSHW-SenseCAP-Watcher/blob/main/Hardware/SenseCAP_Watcher_v1.0_SCH.pdf

# SenseCAP Watcher 硬件抽象层设计

## 概述

为 MimiClaw 项目添加 SenseCAP Watcher 硬件支持，实现完整的硬件抽象层（HAL），支持所有板载外设。

## 硬件规格

- **主控**: ESP32-S3
- **显示**: SPD2010 驱动的 412x412 圆形 QSPI 屏幕
- **音频**: ES8311 (DAC) + ES7243E (ADC)
- **触摸**: I2C 触摸屏控制器
- **IO 扩展**: TCA9555 (I2C 地址 0x21)
- **输入**: 旋转编码器 + 按键
- **输出**: WS2813 RGB LED
- **存储**: SD 卡（SPI2）
- **电源**: 电池电压检测（ADC）

## 架构设计

### 目录结构

```
main/hal/
├── hal_init.c/h              # 统一初始化
├── hal_config.h              # 引脚定义
├── io_expander/
│   ├── tca9555.c/h          # TCA9555 驱动
│   └── hal_io_exp.c/h       # IO 扩展器抽象
├── display/
│   ├── spd2010.c/h          # SPD2010 驱动
│   └── hal_display.c/h      # 显示抽象
├── audio/
│   ├── es8311.c/h           # ES8311 DAC 驱动
│   ├── es7243e.c/h          # ES7243E ADC 驱动
│   └── hal_audio.c/h        # 音频抽象
├── touch/
│   └── hal_touch.c/h        # 触摸屏抽象
├── input/
│   ├── hal_encoder.c/h      # 旋钮编码器
│   └── hal_button.c/h       # 按键处理
├── led/
│   └── hal_led.c/h          # RGB LED (WS2813)
├── sd_card/
│   └── hal_sd.c/h           # SD 卡
└── power/
    └── hal_power.c/h        # 电源管理 + 电池检测
```

### 引脚配置

#### I2C 总线
- **I2C0 (主总线)**: SDA=GPIO47, SCL=GPIO48
  - TCA9555 (0x21)
  - ES8311 (0x18)
  - ES7243E (0x14)
- **I2C1 (触摸屏)**: SDA=GPIO39, SCL=GPIO38

#### SPI 总线
- **SPI2**: SCLK=GPIO4, MOSI=GPIO5, MISO=GPIO6
  - SD 卡: CS=GPIO46
  - AI 芯片: CS=GPIO21

#### QSPI 显示屏
- PCLK=GPIO7, DATA0=GPIO9, DATA1=GPIO1, DATA2=GPIO14, DATA3=GPIO13
- CS=GPIO45, 背光=GPIO8

#### I2S 音频
- MCLK=GPIO10, BCLK=GPIO11, WS=GPIO12
- DIN=GPIO15 (麦克风), DOUT=GPIO16 (扬声器)

#### 其他 GPIO
- IO 扩展器中断: GPIO2
- 电池 ADC: GPIO3
- RGB LED: GPIO40
- 旋钮编码器: A=GPIO41, B=GPIO42
- BOOT 按键: GPIO0

### 初始化流程

1. **I2C 总线初始化**
   - I2C0 (400kHz) - 主总线
   - I2C1 (400kHz) - 触摸屏

2. **IO 扩展器初始化** (最优先)
   - 配置 TCA9555
   - 设置中断引脚
   - 配置输入/输出方向

3. **电源管理初始化**
   - 通过 IO 扩展器启用外设电源
   - 优先级: 系统电源 → LCD → 音频 → SD 卡

4. **显示屏初始化**
   - 配置 QSPI 总线
   - 初始化 SPD2010 驱动
   - 设置背光 PWM

5. **音频初始化**
   - 配置 I2S 总线
   - 初始化 ES8311 (DAC)
   - 初始化 ES7243E (ADC)

6. **触摸屏初始化**
   - 配置 I2C1 总线
   - 注册中断处理

7. **输入设备初始化**
   - 旋钮编码器（GPIO 中断）
   - 按键（IO 扩展器中断）

8. **LED 初始化**
   - 配置 RMT 外设（WS2813 协议）

9. **SD 卡初始化**
   - 配置 SPI2 总线
   - 挂载文件系统

10. **电池检测初始化**
    - 配置 ADC
    - 启用电池检测电源

### 模块接口设计

#### hal_init.h
```c
esp_err_t hal_init(void);
esp_err_t hal_deinit(void);
```

#### hal_io_exp.h
```c
esp_err_t hal_io_exp_init(void);
esp_err_t hal_io_exp_set_output(uint8_t port, uint8_t pin, bool level);
esp_err_t hal_io_exp_read_input(uint8_t port, uint8_t pin, bool *level);
esp_err_t hal_io_exp_enable_power(hal_power_domain_t domain, bool enable);
```

#### hal_display.h
```c
esp_err_t hal_display_init(void);
esp_err_t hal_display_set_backlight(uint8_t brightness);
esp_err_t hal_display_draw_bitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
esp_err_t hal_display_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
```

#### hal_audio.h
```c
esp_err_t hal_audio_init(void);
esp_err_t hal_audio_play(const int16_t *data, size_t len);
esp_err_t hal_audio_record(int16_t *data, size_t len);
esp_err_t hal_audio_set_volume(uint8_t volume);
```

#### hal_touch.h
```c
esp_err_t hal_touch_init(void);
esp_err_t hal_touch_read(uint16_t *x, uint16_t *y, bool *pressed);
typedef void (*hal_touch_callback_t)(uint16_t x, uint16_t y, bool pressed);
esp_err_t hal_touch_register_callback(hal_touch_callback_t cb);
```

#### hal_encoder.h
```c
esp_err_t hal_encoder_init(void);
int32_t hal_encoder_get_count(void);
void hal_encoder_reset_count(void);
typedef void (*hal_encoder_callback_t)(int32_t delta);
esp_err_t hal_encoder_register_callback(hal_encoder_callback_t cb);
```

#### hal_led.h
```c
esp_err_t hal_led_init(void);
esp_err_t hal_led_set_rgb(uint8_t r, uint8_t g, uint8_t b);
esp_err_t hal_led_set_hsv(uint16_t h, uint8_t s, uint8_t v);
esp_err_t hal_led_off(void);
```

#### hal_sd.h
```c
esp_err_t hal_sd_init(void);
esp_err_t hal_sd_deinit(void);
bool hal_sd_is_mounted(void);
```

#### hal_power.h
```c
esp_err_t hal_power_init(void);
uint16_t hal_power_get_battery_mv(void);
uint8_t hal_power_get_battery_percent(void);
bool hal_power_is_charging(void);
bool hal_power_is_usb_connected(void);
```

### 与现有系统集成

#### 消息总线集成
- 硬件事件通过消息总线发送
- 事件类型: 触摸、按键、编码器、电池状态
- 新增消息通道: `MIMI_CHAN_HARDWARE`

#### 配置文件扩展
- `mimi_config.h` 添加 HAL 配置项
- 支持运行时启用/禁用硬件模块

#### 主程序集成
- `app_main()` 中调用 `hal_init()`
- 创建硬件事件处理任务
- 显示状态信息到屏幕

## 实施计划

### 阶段 1: 基础设施（优先级最高）
1. IO 扩展器驱动（TCA9555）
2. 电源管理模块
3. 引脚配置文件

### 阶段 2: 核心外设
4. 显示屏驱动（SPD2010 + QSPI）
5. 音频驱动（ES8311 + ES7243E + I2S）
6. 触摸屏驱动

### 阶段 3: 输入输出
7. 旋钮编码器
8. 按键处理
9. RGB LED (WS2813)

### 阶段 4: 存储和电源
10. SD 卡驱动
11. 电池检测

### 阶段 5: 集成测试
12. 统一初始化接口
13. 消息总线集成
14. 主程序集成
15. 完整功能测试

## 技术要点

### IO 扩展器（TCA9555）
- 16 个 GPIO（Port 0 输入，Port 1 输出）
- 中断驱动的输入检测
- 控制所有外设电源开关

### QSPI 显示屏
- 4 线 QSPI 接口，提升传输速度
- 圆形显示区域 412x412
- PWM 背光控制

### I2S 音频
- 全双工模式（同时录音和播放）
- ES8311 (DAC) 用于扬声器输出
- ES7243E (ADC) 用于麦克风输入
- MCLK 提供主时钟

### WS2813 LED
- 使用 RMT 外设生成时序
- 支持 RGB 和 HSV 颜色空间
- 单个 LED 控制

### 电池检测
- ADC 读取电池电压
- 4.1:1 分压比
- 通过 IO 扩展器控制检测电源

## 依赖组件

- ESP-IDF v5.5+
- esp_lcd (QSPI 显示)
- esp_codec (音频编解码器)
- driver/i2c
- driver/spi_master
- driver/i2s_std
- driver/rmt_tx (LED)
- driver/adc
- esp_vfs_fat (SD 卡)

## 测试策略

### 单元测试
- 每个驱动模块独立测试
- 模拟硬件响应

### 集成测试
- 多外设协同工作测试
- 电源管理测试
- 中断处理测试

### 功能测试
- 显示文字和图形
- 播放和录制音频
- 触摸和编码器输入
- LED 颜色变化
- SD 卡读写
- 电池状态显示

## 风险和缓解

### 风险 1: QSPI 时序问题
- **缓解**: 参考 ESP-IDF 示例，使用标准 QSPI 配置

### 风险 2: I2S 音频同步
- **缓解**: 使用 DMA 缓冲，配置合适的采样率

### 风险 3: IO 扩展器中断丢失
- **缓解**: 使用边沿触发 + 轮询结合的方式

### 风险 4: 电源时序不当导致外设异常
- **缓解**: 严格按照上电顺序，添加延时

## 参考资料

- SenseCAP Watcher 硬件原理图
- ESP-IDF 编程指南
- TCA9555 数据手册
- SPD2010 数据手册
- ES8311/ES7243E 数据手册
- WS2813 时序规范

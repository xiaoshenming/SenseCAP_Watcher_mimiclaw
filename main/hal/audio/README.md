# 音频 HAL 驱动

SenseCAP Watcher 音频子系统驱动，支持 ES8311 DAC 和 ES7243E ADC。

## 硬件配置

- **DAC**: ES8311 (I2C 地址 0x18)
- **ADC**: ES7243E (I2C 地址 0x14)
- **I2S 接口**:
  - MCLK: GPIO 10
  - BCLK: GPIO 11
  - WS: GPIO 12
  - DIN: GPIO 15 (麦克风输入)
  - DOUT: GPIO 16 (扬声器输出)
- **功放电源**: 通过 IO 扩展器 P1.5 控制

## 功能特性

- ✅ 全双工 I2S 通信（同时播放和录制）
- ✅ 支持 16kHz / 48kHz 采样率
- ✅ 16-bit PCM 音频格式
- ✅ 单声道/立体声支持
- ✅ 音量控制（0-100）
- ✅ 麦克风增益控制（0dB / 6dB / 12dB / 18dB）
- ✅ 静音控制
- ✅ 阻塞式播放/录制
- ✅ 流式播放/录制（回调模式）

## API 使用

### 初始化

```c
#include "hal/audio/hal_audio.h"
#include "hal/io_expander/hal_io_exp.h"

// 1. 先初始化 IO 扩展器（提供 I2C 总线）
hal_io_exp_init();

// 2. 初始化音频 HAL
hal_audio_config_t config = {
    .sample_rate = 16000,       // 16kHz
    .bits_per_sample = 16,      // 16-bit
    .channels = 1,              // 单声道
    .mic_gain = 2,              // 12dB 增益
};
hal_audio_init(&config);

// 或使用默认配置
hal_audio_init(NULL);
```

### 播放音频（阻塞）

```c
// 准备音频数据（16-bit PCM）
int16_t audio_data[16000];  // 1 秒 @ 16kHz
// ... 填充音频数据 ...

// 播放
hal_audio_set_volume(80);  // 设置音量
hal_audio_play(audio_data, 16000);
```

### 录制音频（阻塞）

```c
// 准备缓冲区
int16_t audio_buffer[16000];  // 1 秒 @ 16kHz

// 录制
hal_audio_record(audio_buffer, 16000);
// ... 处理录制的音频 ...
```

### 流式播放/录制（回调模式）

```c
// 播放回调
void play_callback(const int16_t *data, size_t len, void *user_data) {
    // 填充 data 缓冲区，len 为采样点数
    // 例如：从文件读取、生成音频等
}

// 录制回调
void record_callback(const int16_t *data, size_t len, void *user_data) {
    // 处理录制的音频数据
    // 例如：保存到文件、发送到网络等
}

// 启动音频流
hal_audio_start_stream(play_callback, record_callback, NULL);

// ... 运行一段时间 ...

// 停止音频流
hal_audio_stop_stream();
```

### 音量和增益控制

```c
// 设置播放音量（0-100）
hal_audio_set_volume(80);

// 设置麦克风增益（0-3）
// 0 = 0dB, 1 = 6dB, 2 = 12dB, 3 = 18dB
hal_audio_set_mic_gain(2);

// 静音/取消静音
hal_audio_set_mute(true);   // 静音
hal_audio_set_mute(false);  // 取消静音
```

### 清理

```c
hal_audio_deinit();
```

## 示例代码

参见 `hal_audio_example.c` 中的完整示例：

- `audio_example_simple_play()` - 播放 440Hz 测试音
- `audio_example_record()` - 录制 3 秒音频
- `audio_example_stream()` - 流式播放和录制

## 技术细节

### I2S 配置

- **模式**: Master 模式
- **时钟**: MCLK = 256 × Fs, BCLK = 64 × Fs
- **格式**: Philips I2S 标准
- **DMA**: 4 个缓冲区，每个 1024 采样点

### ES8311 DAC

- **I2C 地址**: 0x18
- **功能**: 数字音频输出（扬声器）
- **音量范围**: 0x00 (-95.5dB) 到 0xBF (0dB)
- **工作模式**: I2S Slave

### ES7243E ADC

- **I2C 地址**: 0x14
- **功能**: 模拟音频输入（麦克风）
- **增益范围**: 0dB / 6dB / 12dB / 18dB
- **工作模式**: I2S Slave

## 注意事项

1. **初始化顺序**: 必须先调用 `hal_io_exp_init()` 初始化 I2C 总线
2. **电源管理**: 音频功放电源由 IO 扩展器控制，自动管理
3. **采样率**: 推荐使用 16kHz（语音）或 48kHz（音乐）
4. **内存**: 流式模式会创建后台任务，注意内存使用
5. **线程安全**: API 不是线程安全的，需要外部同步

## 故障排除

### 无声音输出

- 检查功放电源是否启用
- 检查音量设置
- 检查是否静音
- 验证 I2S 引脚连接

### 录音无信号

- 检查麦克风增益设置
- 验证 ES7243E 初始化成功
- 检查 I2S DIN 引脚连接

### I2C 通信失败

- 确保先调用 `hal_io_exp_init()`
- 检查 I2C 总线引脚（SDA=47, SCL=48）
- 验证设备地址（ES8311=0x18, ES7243E=0x14）

## 依赖

- ESP-IDF v5.5+
- `driver/i2s_std` - I2S 标准驱动
- `driver/i2c_master` - I2C 主机驱动
- `hal/io_expander` - IO 扩展器 HAL（提供 I2C 总线）

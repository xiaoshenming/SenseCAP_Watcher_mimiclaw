#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * 音频配置参数
 */
typedef struct {
    uint32_t sample_rate;       /* 采样率 (Hz): 16000, 48000 等 */
    uint8_t bits_per_sample;    /* 位深度: 16, 24, 32 */
    uint8_t channels;           /* 通道数: 1=单声道, 2=立体声 */
    uint8_t mic_gain;           /* 麦克风增益 (0-3) */
} hal_audio_config_t;

/**
 * 音频流回调函数类型
 *
 * @param data 音频数据缓冲区
 * @param len 数据长度（字节）
 * @param user_data 用户数据
 */
typedef void (*hal_audio_stream_cb_t)(const int16_t *data, size_t len, void *user_data);

/**
 * 初始化音频 HAL
 * - 配置 I2S 总线（全双工模式）
 * - 初始化 ES8311 DAC
 * - 初始化 ES7243E ADC
 * - 启用音频功放电源
 *
 * @param config 音频配置参数，NULL 使用默认配置
 * @return ESP_OK 成功
 */
esp_err_t hal_audio_init(const hal_audio_config_t *config);

/**
 * 反初始化音频 HAL
 *
 * @return ESP_OK 成功
 */
esp_err_t hal_audio_deinit(void);

/**
 * 播放音频数据（阻塞）
 *
 * @param data 音频数据（16-bit PCM）
 * @param len 数据长度（采样点数）
 * @return ESP_OK 成功
 */
esp_err_t hal_audio_play(const int16_t *data, size_t len);

/**
 * 录制音频数据（阻塞）
 *
 * @param data 音频数据缓冲区（16-bit PCM）
 * @param len 要录制的采样点数
 * @return ESP_OK 成功
 */
esp_err_t hal_audio_record(int16_t *data, size_t len);

/**
 * 设置播放音量
 *
 * @param volume 音量 (0-100)
 * @return ESP_OK 成功
 */
esp_err_t hal_audio_set_volume(uint8_t volume);

/**
 * 设置麦克风增益
 *
 * @param gain 增益 (0-3: 0=0dB, 1=6dB, 2=12dB, 3=18dB)
 * @return ESP_OK 成功
 */
esp_err_t hal_audio_set_mic_gain(uint8_t gain);

/**
 * 静音/取消静音播放
 *
 * @param mute true=静音, false=取消静音
 * @return ESP_OK 成功
 */
esp_err_t hal_audio_set_mute(bool mute);

/**
 * 启动音频流（连续播放/录制）
 * 使用回调函数处理音频数据
 *
 * @param play_cb 播放回调（NULL 表示不播放）
 * @param record_cb 录制回调（NULL 表示不录制）
 * @param user_data 用户数据
 * @return ESP_OK 成功
 */
esp_err_t hal_audio_start_stream(hal_audio_stream_cb_t play_cb,
                                  hal_audio_stream_cb_t record_cb,
                                  void *user_data);

/**
 * 停止音频流
 *
 * @return ESP_OK 成功
 */
esp_err_t hal_audio_stop_stream(void);

/**
 * 检查音频是否正在播放
 *
 * @return true=正在播放, false=空闲
 */
bool hal_audio_is_playing(void);

/**
 * 检查音频是否正在录制
 *
 * @return true=正在录制, false=空闲
 */
bool hal_audio_is_recording(void);

/**
 * 音频 HAL 使用示例
 *
 * 演示如何使用音频 HAL 进行播放和录制
 */

#include "hal_audio.h"
#include "hal/io_expander/hal_io_exp.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <math.h>

static const char *TAG = "AUDIO_EXAMPLE";

/* 生成正弦波测试音 */
static void generate_sine_wave(int16_t *buffer, size_t len, float freq, uint32_t sample_rate)
{
    static float phase = 0.0f;
    float phase_increment = 2.0f * M_PI * freq / sample_rate;

    for (size_t i = 0; i < len; i++) {
        buffer[i] = (int16_t)(sin(phase) * 16000.0f);
        phase += phase_increment;
        if (phase >= 2.0f * M_PI) {
            phase -= 2.0f * M_PI;
        }
    }
}

/* 播放回调 - 生成 440Hz 测试音 */
static void play_callback(const int16_t *data, size_t len, void *user_data)
{
    generate_sine_wave((int16_t *)data, len, 440.0f, 16000);
}

/* 录制回调 - 打印音频电平 */
static void record_callback(const int16_t *data, size_t len, void *user_data)
{
    /* 计算音频电平 */
    int32_t sum = 0;
    for (size_t i = 0; i < len; i++) {
        sum += abs(data[i]);
    }
    int32_t avg = sum / len;

    /* 每秒打印一次 */
    static uint32_t last_print = 0;
    uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
    if (now - last_print > 1000) {
        ESP_LOGI(TAG, "Audio level: %ld", avg);
        last_print = now;
    }
}

void audio_example_simple_play(void)
{
    ESP_LOGI(TAG, "=== Simple Audio Play Example ===");

    /* 初始化 IO 扩展器（必须先初始化，提供 I2C 总线）*/
    esp_err_t ret = hal_io_exp_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init IO expander");
        return;
    }

    /* 初始化音频 HAL */
    hal_audio_config_t audio_cfg = {
        .sample_rate = 16000,
        .bits_per_sample = 16,
        .channels = 1,
        .mic_gain = 2,
    };
    ret = hal_audio_init(&audio_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init audio HAL");
        return;
    }

    /* 设置音量 */
    hal_audio_set_volume(80);

    /* 生成 1 秒的 440Hz 正弦波 */
    size_t samples = 16000;  // 1 秒
    int16_t *audio_data = malloc(samples * sizeof(int16_t));
    if (!audio_data) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        hal_audio_deinit();
        return;
    }

    generate_sine_wave(audio_data, samples, 440.0f, 16000);

    /* 播放音频 */
    ESP_LOGI(TAG, "Playing 440Hz tone for 1 second...");
    ret = hal_audio_play(audio_data, samples);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to play audio");
    }

    free(audio_data);
    hal_audio_deinit();
    ESP_LOGI(TAG, "Audio play example completed");
}

void audio_example_record(void)
{
    ESP_LOGI(TAG, "=== Audio Record Example ===");

    /* 初始化 */
    hal_io_exp_init();
    hal_audio_config_t audio_cfg = {
        .sample_rate = 16000,
        .bits_per_sample = 16,
        .channels = 1,
        .mic_gain = 2,
    };
    hal_audio_init(&audio_cfg);

    /* 录制 3 秒音频 */
    size_t samples = 16000 * 3;  // 3 秒
    int16_t *audio_data = malloc(samples * sizeof(int16_t));
    if (!audio_data) {
        ESP_LOGE(TAG, "Failed to allocate audio buffer");
        hal_audio_deinit();
        return;
    }

    ESP_LOGI(TAG, "Recording for 3 seconds...");
    esp_err_t ret = hal_audio_record(audio_data, samples);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to record audio");
    } else {
        ESP_LOGI(TAG, "Recording completed");
        /* 这里可以处理录制的音频数据 */
    }

    free(audio_data);
    hal_audio_deinit();
}

void audio_example_stream(void)
{
    ESP_LOGI(TAG, "=== Audio Stream Example ===");

    /* 初始化 */
    hal_io_exp_init();
    hal_audio_config_t audio_cfg = {
        .sample_rate = 16000,
        .bits_per_sample = 16,
        .channels = 1,
        .mic_gain = 2,
    };
    hal_audio_init(&audio_cfg);
    hal_audio_set_volume(80);

    /* 启动音频流（同时播放和录制）*/
    ESP_LOGI(TAG, "Starting audio stream (play 440Hz + record)...");
    esp_err_t ret = hal_audio_start_stream(play_callback, record_callback, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start audio stream");
        hal_audio_deinit();
        return;
    }

    /* 运行 10 秒 */
    vTaskDelay(pdMS_TO_TICKS(10000));

    /* 停止音频流 */
    ESP_LOGI(TAG, "Stopping audio stream...");
    hal_audio_stop_stream();
    hal_audio_deinit();
    ESP_LOGI(TAG, "Audio stream example completed");
}

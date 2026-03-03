#include "hal_audio.h"
#include "es8311.h"
#include "es7243e.h"
#include "../hal_config.h"
#include "../io_expander/hal_io_exp.h"
#include "esp_log.h"
#include "driver/i2s_std.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "HAL_AUDIO";

/* 默认配置 */
#define DEFAULT_SAMPLE_RATE     16000
#define DEFAULT_BITS_PER_SAMPLE 16
#define DEFAULT_CHANNELS        1
#define DEFAULT_MIC_GAIN        2

/* I2S DMA 缓冲区配置 */
#define I2S_DMA_BUF_COUNT       4
#define I2S_DMA_BUF_LEN         1024

/* 全局状态 */
static struct {
    i2s_chan_handle_t tx_chan;
    i2s_chan_handle_t rx_chan;
    es8311_handle_t es8311;
    es7243e_handle_t es7243e;
    hal_audio_config_t config;
    bool initialized;
    bool streaming;
    TaskHandle_t stream_task;
    hal_audio_stream_cb_t play_cb;
    hal_audio_stream_cb_t record_cb;
    void *user_data;
} s_audio = {0};

/* 音频流任务 */
static void audio_stream_task(void *arg)
{
    int16_t *rx_buf = NULL;
    int16_t *tx_buf = NULL;
    size_t bytes_read = 0;
    size_t bytes_written = 0;

    if (s_audio.record_cb) {
        rx_buf = malloc(I2S_DMA_BUF_LEN * sizeof(int16_t));
        if (!rx_buf) {
            ESP_LOGE(TAG, "Failed to allocate RX buffer");
            goto cleanup;
        }
    }

    if (s_audio.play_cb) {
        tx_buf = malloc(I2S_DMA_BUF_LEN * sizeof(int16_t));
        if (!tx_buf) {
            ESP_LOGE(TAG, "Failed to allocate TX buffer");
            goto cleanup;
        }
    }

    while (s_audio.streaming) {
        /* 录制音频 */
        if (s_audio.record_cb && rx_buf) {
            if (i2s_channel_read(s_audio.rx_chan, rx_buf,
                                I2S_DMA_BUF_LEN * sizeof(int16_t),
                                &bytes_read, portMAX_DELAY) == ESP_OK) {
                s_audio.record_cb(rx_buf, bytes_read / sizeof(int16_t), s_audio.user_data);
            }
        }

        /* 播放音频 */
        if (s_audio.play_cb && tx_buf) {
            s_audio.play_cb(tx_buf, I2S_DMA_BUF_LEN, s_audio.user_data);
            i2s_channel_write(s_audio.tx_chan, tx_buf,
                            I2S_DMA_BUF_LEN * sizeof(int16_t),
                            &bytes_written, portMAX_DELAY);
        }

        vTaskDelay(pdMS_TO_TICKS(1));
    }

cleanup:
    if (rx_buf) free(rx_buf);
    if (tx_buf) free(tx_buf);
    s_audio.stream_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t hal_audio_init(const hal_audio_config_t *config)
{
    if (s_audio.initialized) {
        ESP_LOGW(TAG, "Audio already initialized");
        return ESP_OK;
    }

    esp_err_t ret;

    /* 使用默认配置或用户配置 */
    if (config) {
        memcpy(&s_audio.config, config, sizeof(hal_audio_config_t));
    } else {
        s_audio.config.sample_rate = DEFAULT_SAMPLE_RATE;
        s_audio.config.bits_per_sample = DEFAULT_BITS_PER_SAMPLE;
        s_audio.config.channels = DEFAULT_CHANNELS;
        s_audio.config.mic_gain = DEFAULT_MIC_GAIN;
    }

    /* 启用音频功放电源 */
    ret = hal_io_exp_set_power(IO_EXP_PWR_CODEC_PA, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable audio power: %s", esp_err_to_name(ret));
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(50));

    /* 配置 I2S 标准模式 */
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = I2S_DMA_BUF_COUNT;
    chan_cfg.dma_frame_num = I2S_DMA_BUF_LEN;

    ret = i2s_new_channel(&chan_cfg, &s_audio.tx_chan, &s_audio.rx_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2S channel: %s", esp_err_to_name(ret));
        goto err_power;
    }

    /* 配置 I2S 标准接口 */
    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(s_audio.config.sample_rate),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(
            s_audio.config.bits_per_sample,
            s_audio.config.channels == 2 ? I2S_SLOT_MODE_STEREO : I2S_SLOT_MODE_MONO
        ),
        .gpio_cfg = {
            .mclk = GPIO_I2S_MCLK,
            .bclk = GPIO_I2S_BCLK,
            .ws = GPIO_I2S_WS,
            .dout = GPIO_I2S_DOUT,
            .din = GPIO_I2S_DIN,
            .invert_flags = {
                .mclk_inv = false,
                .bclk_inv = false,
                .ws_inv = false,
            },
        },
    };

    ret = i2s_channel_init_std_mode(s_audio.tx_chan, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init I2S TX: %s", esp_err_to_name(ret));
        goto err_channel;
    }

    ret = i2s_channel_init_std_mode(s_audio.rx_chan, &std_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init I2S RX: %s", esp_err_to_name(ret));
        goto err_channel;
    }

    /* 启用 I2S 通道 */
    ret = i2s_channel_enable(s_audio.tx_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S TX: %s", esp_err_to_name(ret));
        goto err_channel;
    }

    ret = i2s_channel_enable(s_audio.rx_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable I2S RX: %s", esp_err_to_name(ret));
        goto err_tx_enable;
    }

    /* 获取 I2C 总线句柄（假设已由 hal_io_exp_init 初始化）*/
    i2c_master_bus_handle_t i2c_bus = hal_io_exp_get_i2c_bus();
    if (!i2c_bus) {
        ESP_LOGE(TAG, "I2C bus not initialized. Call hal_io_exp_init() first.");
        ret = ESP_ERR_INVALID_STATE;
        goto err_rx_enable;
    }

    /* 初始化 ES8311 DAC */
    es8311_config_t es8311_cfg = {
        .i2c_bus = i2c_bus,
        .i2c_addr = I2C_ADDR_ES8311,
        .sample_rate = s_audio.config.sample_rate,
        .bits_per_sample = s_audio.config.bits_per_sample,
    };
    ret = es8311_init(&s_audio.es8311, &es8311_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init ES8311: %s", esp_err_to_name(ret));
        goto err_rx_enable;
    }

    /* 初始化 ES7243E ADC */
    es7243e_config_t es7243e_cfg = {
        .i2c_bus = i2c_bus,
        .i2c_addr = I2C_ADDR_ES7243E,
        .sample_rate = s_audio.config.sample_rate,
        .bits_per_sample = s_audio.config.bits_per_sample,
        .mic_gain = s_audio.config.mic_gain,
    };
    ret = es7243e_init(&s_audio.es7243e, &es7243e_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init ES7243E: %s", esp_err_to_name(ret));
        goto err_es8311;
    }

    s_audio.initialized = true;
    ESP_LOGI(TAG, "Audio HAL initialized (SR=%lu, BPS=%d, CH=%d)",
             s_audio.config.sample_rate,
             s_audio.config.bits_per_sample,
             s_audio.config.channels);
    return ESP_OK;

err_es8311:
    es8311_deinit(&s_audio.es8311);
err_rx_enable:
    i2s_channel_disable(s_audio.rx_chan);
err_tx_enable:
    i2s_channel_disable(s_audio.tx_chan);
err_channel:
    i2s_del_channel(s_audio.tx_chan);
    i2s_del_channel(s_audio.rx_chan);
err_power:
    hal_io_exp_set_power(IO_EXP_PWR_CODEC_PA, false);
    return ret;
}

esp_err_t hal_audio_deinit(void)
{
    if (!s_audio.initialized) {
        return ESP_OK;
    }

    /* 停止音频流 */
    if (s_audio.streaming) {
        hal_audio_stop_stream();
    }

    /* 反初始化编解码器 */
    es7243e_deinit(&s_audio.es7243e);
    es8311_deinit(&s_audio.es8311);

    /* 禁用并删除 I2S 通道 */
    i2s_channel_disable(s_audio.rx_chan);
    i2s_channel_disable(s_audio.tx_chan);
    i2s_del_channel(s_audio.tx_chan);
    i2s_del_channel(s_audio.rx_chan);

    /* 关闭音频功放电源 */
    hal_io_exp_set_power(IO_EXP_PWR_CODEC_PA, false);

    s_audio.initialized = false;
    ESP_LOGI(TAG, "Audio HAL deinitialized");
    return ESP_OK;
}

esp_err_t hal_audio_play(const int16_t *data, size_t len)
{
    if (!s_audio.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t bytes_written = 0;
    return i2s_channel_write(s_audio.tx_chan, data, len * sizeof(int16_t),
                            &bytes_written, portMAX_DELAY);
}

esp_err_t hal_audio_record(int16_t *data, size_t len)
{
    if (!s_audio.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t bytes_read = 0;
    return i2s_channel_read(s_audio.rx_chan, data, len * sizeof(int16_t),
                           &bytes_read, portMAX_DELAY);
}

esp_err_t hal_audio_set_volume(uint8_t volume)
{
    if (!s_audio.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return es8311_set_volume(&s_audio.es8311, volume);
}

esp_err_t hal_audio_set_mic_gain(uint8_t gain)
{
    if (!s_audio.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return es7243e_set_mic_gain(&s_audio.es7243e, gain);
}

esp_err_t hal_audio_set_mute(bool mute)
{
    if (!s_audio.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    return es8311_set_mute(&s_audio.es8311, mute);
}

esp_err_t hal_audio_start_stream(hal_audio_stream_cb_t play_cb,
                                  hal_audio_stream_cb_t record_cb,
                                  void *user_data)
{
    if (!s_audio.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (s_audio.streaming) {
        ESP_LOGW(TAG, "Audio stream already running");
        return ESP_ERR_INVALID_STATE;
    }

    if (!play_cb && !record_cb) {
        ESP_LOGE(TAG, "At least one callback must be provided");
        return ESP_ERR_INVALID_ARG;
    }

    s_audio.play_cb = play_cb;
    s_audio.record_cb = record_cb;
    s_audio.user_data = user_data;
    s_audio.streaming = true;

    /* 创建音频流任务 */
    BaseType_t ret = xTaskCreate(audio_stream_task, "audio_stream",
                                 4096, NULL, 5, &s_audio.stream_task);
    if (ret != pdPASS) {
        s_audio.streaming = false;
        ESP_LOGE(TAG, "Failed to create audio stream task");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Audio stream started");
    return ESP_OK;
}

esp_err_t hal_audio_stop_stream(void)
{
    if (!s_audio.streaming) {
        return ESP_OK;
    }

    s_audio.streaming = false;

    /* 等待任务结束 */
    int timeout = 100;
    while (s_audio.stream_task && timeout-- > 0) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    if (s_audio.stream_task) {
        ESP_LOGW(TAG, "Force delete audio stream task");
        vTaskDelete(s_audio.stream_task);
        s_audio.stream_task = NULL;
    }

    ESP_LOGI(TAG, "Audio stream stopped");
    return ESP_OK;
}

bool hal_audio_is_playing(void)
{
    return s_audio.streaming && s_audio.play_cb != NULL;
}

bool hal_audio_is_recording(void)
{
    return s_audio.streaming && s_audio.record_cb != NULL;
}

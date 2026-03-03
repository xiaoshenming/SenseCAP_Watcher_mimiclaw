#include "es8311.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ES8311";

/* 内部函数：写寄存器 */
static esp_err_t es8311_write_reg(es8311_handle_t *handle, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(handle->i2c_dev, data, 2, 1000);
}

/* 内部函数：读寄存器 */
static esp_err_t es8311_read_reg(es8311_handle_t *handle, uint8_t reg, uint8_t *value)
{
    esp_err_t ret = i2c_master_transmit(handle->i2c_dev, &reg, 1, 1000);
    if (ret != ESP_OK) {
        return ret;
    }
    return i2c_master_receive(handle->i2c_dev, value, 1, 1000);
}

esp_err_t es8311_init(es8311_handle_t *handle, const es8311_config_t *config)
{
    if (!handle || !config) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&handle->config, config, sizeof(es8311_config_t));

    /* 添加 I2C 设备 */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = config->i2c_addr,
        .scl_speed_hz = 400000,
    };
    esp_err_t ret = i2c_master_bus_add_device(config->i2c_bus, &dev_cfg, &handle->i2c_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add I2C device: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 复位 ES8311 */
    ret = es8311_write_reg(handle, ES8311_REG_RESET, 0x1F);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset ES8311: %s", esp_err_to_name(ret));
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    ret = es8311_write_reg(handle, ES8311_REG_RESET, 0x00);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    /* 配置时钟管理 */
    /* MCLK = 256 * Fs, BCLK = 64 * Fs */
    ret = es8311_write_reg(handle, ES8311_REG_CLK_MANAGER, 0x30);  // MCLK DIV
    if (ret != ESP_OK) return ret;

    ret = es8311_write_reg(handle, ES8311_REG_CLK_MANAGER2, 0x00); // BCLK DIV
    if (ret != ESP_OK) return ret;

    ret = es8311_write_reg(handle, ES8311_REG_CLK_MANAGER3, 0x10); // LRCK DIV
    if (ret != ESP_OK) return ret;

    ret = es8311_write_reg(handle, ES8311_REG_CLK_MANAGER4, 0x10); // ADC/DAC CLK
    if (ret != ESP_OK) return ret;

    ret = es8311_write_reg(handle, ES8311_REG_CLK_MANAGER5, 0x00); // CLK SRC
    if (ret != ESP_OK) return ret;

    /* 配置系统设置 */
    ret = es8311_write_reg(handle, ES8311_REG_SYSTEM1, 0x3F);
    if (ret != ESP_OK) return ret;

    ret = es8311_write_reg(handle, ES8311_REG_SYSTEM2, 0x02);  // I2S Slave mode
    if (ret != ESP_OK) return ret;

    ret = es8311_write_reg(handle, ES8311_REG_SYSTEM3, 0x10);  // 16-bit
    if (ret != ESP_OK) return ret;

    ret = es8311_write_reg(handle, ES8311_REG_SYSTEM4, 0x00);
    if (ret != ESP_OK) return ret;

    ret = es8311_write_reg(handle, ES8311_REG_SYSTEM5, 0x00);
    if (ret != ESP_OK) return ret;

    /* 配置 DAC */
    ret = es8311_write_reg(handle, ES8311_REG_DAC_OSR, 0x28);  // DAC OSR
    if (ret != ESP_OK) return ret;

    ret = es8311_write_reg(handle, ES8311_REG_DAC_MUTE, 0x00);  // Unmute
    if (ret != ESP_OK) return ret;

    /* 设置默认音量 */
    ret = es8311_set_volume(handle, 80);
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "ES8311 initialized successfully");
    return ESP_OK;
}

esp_err_t es8311_deinit(es8311_handle_t *handle)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (handle->i2c_dev) {
        i2c_master_bus_rm_device(handle->i2c_dev);
        handle->i2c_dev = NULL;
    }

    return ESP_OK;
}

esp_err_t es8311_set_volume(es8311_handle_t *handle, uint8_t volume)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (volume > 100) {
        volume = 100;
    }

    /* ES8311 音量范围: 0x00 (-95.5dB) 到 0xBF (0dB) */
    /* 映射 0-100 到 0x00-0xBF */
    uint8_t reg_val = (volume * 0xBF) / 100;

    return es8311_write_reg(handle, ES8311_REG_SYSTEM14, reg_val);
}

esp_err_t es8311_set_mute(es8311_handle_t *handle, bool mute)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg_val = mute ? 0x1C : 0x00;
    return es8311_write_reg(handle, ES8311_REG_DAC_MUTE, reg_val);
}

esp_err_t es8311_set_sample_rate(es8311_handle_t *handle, uint32_t sample_rate)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    handle->config.sample_rate = sample_rate;

    /* 根据采样率配置时钟分频 */
    /* 这里简化处理，实际应根据 MCLK 和采样率计算分频系数 */
    return ESP_OK;
}

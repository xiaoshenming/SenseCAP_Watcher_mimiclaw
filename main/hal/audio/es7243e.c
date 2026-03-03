#include "es7243e.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ES7243E";

/* 内部函数：写寄存器 */
static esp_err_t es7243e_write_reg(es7243e_handle_t *handle, uint8_t reg, uint8_t value)
{
    uint8_t data[2] = {reg, value};
    return i2c_master_transmit(handle->i2c_dev, data, 2, 1000);
}

/* 内部函数：读寄存器 */
static esp_err_t es7243e_read_reg(es7243e_handle_t *handle, uint8_t reg, uint8_t *value)
{
    esp_err_t ret = i2c_master_transmit(handle->i2c_dev, &reg, 1, 1000);
    if (ret != ESP_OK) {
        return ret;
    }
    return i2c_master_receive(handle->i2c_dev, value, 1, 1000);
}

esp_err_t es7243e_init(es7243e_handle_t *handle, const es7243e_config_t *config)
{
    if (!handle || !config) {
        return ESP_ERR_INVALID_ARG;
    }

    memcpy(&handle->config, config, sizeof(es7243e_config_t));

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

    /* 复位 ES7243E */
    ret = es7243e_write_reg(handle, ES7243E_REG_RESET, 0xFF);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reset ES7243E: %s", esp_err_to_name(ret));
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    ret = es7243e_write_reg(handle, ES7243E_REG_RESET, 0x00);
    if (ret != ESP_OK) {
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    /* 配置时钟 - I2S Slave 模式 */
    ret = es7243e_write_reg(handle, ES7243E_REG_CLK_MANAGER, 0x00);
    if (ret != ESP_OK) return ret;

    ret = es7243e_write_reg(handle, ES7243E_REG_CLK_MANAGER2, 0x00);
    if (ret != ESP_OK) return ret;

    /* 配置 I2S 格式: Slave, 16-bit */
    ret = es7243e_write_reg(handle, ES7243E_REG_FORMAT, 0x01);
    if (ret != ESP_OK) return ret;

    /* 上电配置 */
    ret = es7243e_write_reg(handle, ES7243E_REG_POWER1, 0x3F);
    if (ret != ESP_OK) return ret;

    ret = es7243e_write_reg(handle, ES7243E_REG_POWER2, 0x00);
    if (ret != ESP_OK) return ret;

    /* 模拟模块上电 */
    ret = es7243e_write_reg(handle, ES7243E_REG_ANALOG_PDN, 0x00);
    if (ret != ESP_OK) return ret;
    vTaskDelay(pdMS_TO_TICKS(50));

    /* ADC 控制配置 */
    ret = es7243e_write_reg(handle, ES7243E_REG_ADC_CTRL, 0x20);
    if (ret != ESP_OK) return ret;

    /* 取消静音 */
    ret = es7243e_write_reg(handle, ES7243E_REG_ADC_MUTE, 0x00);
    if (ret != ESP_OK) return ret;

    /* 设置麦克风增益 */
    ret = es7243e_set_mic_gain(handle, config->mic_gain);
    if (ret != ESP_OK) return ret;

    /* 设置默认 ADC 音量 */
    ret = es7243e_write_reg(handle, ES7243E_REG_ADC_VOLUME, 0xC0);
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "ES7243E initialized successfully");
    return ESP_OK;
}

esp_err_t es7243e_deinit(es7243e_handle_t *handle)
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

esp_err_t es7243e_set_mic_gain(es7243e_handle_t *handle, uint8_t gain)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    if (gain > 3) {
        gain = 3;
    }

    /* MIC_GAIN 寄存器: bit[1:0] = gain */
    uint8_t reg_val = gain & 0x03;
    return es7243e_write_reg(handle, ES7243E_REG_MIC_GAIN, reg_val);
}

esp_err_t es7243e_set_mute(es7243e_handle_t *handle, bool mute)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg_val = mute ? 0x03 : 0x00;
    return es7243e_write_reg(handle, ES7243E_REG_ADC_MUTE, reg_val);
}

esp_err_t es7243e_set_volume(es7243e_handle_t *handle, uint8_t volume)
{
    if (!handle) {
        return ESP_ERR_INVALID_ARG;
    }

    return es7243e_write_reg(handle, ES7243E_REG_ADC_VOLUME, volume);
}

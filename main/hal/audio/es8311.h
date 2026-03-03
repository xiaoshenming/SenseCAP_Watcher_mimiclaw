#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"
#include <stdint.h>

/* ES8311 I2C 地址 */
#define ES8311_I2C_ADDR 0x18

/* ES8311 寄存器地址 */
#define ES8311_REG_RESET            0x00
#define ES8311_REG_CLK_MANAGER      0x01
#define ES8311_REG_CLK_MANAGER2     0x02
#define ES8311_REG_CLK_MANAGER3     0x03
#define ES8311_REG_CLK_MANAGER4     0x04
#define ES8311_REG_CLK_MANAGER5     0x05
#define ES8311_REG_SYSTEM1          0x06
#define ES8311_REG_SYSTEM2          0x07
#define ES8311_REG_SYSTEM3          0x08
#define ES8311_REG_SYSTEM4          0x09
#define ES8311_REG_SYSTEM5          0x0A
#define ES8311_REG_SYSTEM6          0x0B
#define ES8311_REG_SYSTEM7          0x0C
#define ES8311_REG_SYSTEM8          0x0D
#define ES8311_REG_SYSTEM9          0x0E
#define ES8311_REG_SYSTEM10         0x0F
#define ES8311_REG_SYSTEM11         0x10
#define ES8311_REG_SYSTEM12         0x11
#define ES8311_REG_SYSTEM13         0x12
#define ES8311_REG_SYSTEM14         0x13
#define ES8311_REG_ADC_OSR          0x14
#define ES8311_REG_ADC_MUTE         0x15
#define ES8311_REG_DAC_OSR          0x16
#define ES8311_REG_DAC_MUTE         0x17
#define ES8311_REG_GPIO             0x44

/* ES8311 配置结构 */
typedef struct {
    i2c_master_bus_handle_t i2c_bus;
    uint8_t i2c_addr;
    uint32_t sample_rate;
    uint8_t bits_per_sample;
} es8311_config_t;

typedef struct {
    es8311_config_t config;
    i2c_master_dev_handle_t i2c_dev;
} es8311_handle_t;

/**
 * 初始化 ES8311 DAC
 *
 * @param handle ES8311 句柄
 * @param config 配置参数
 * @return ESP_OK 成功
 */
esp_err_t es8311_init(es8311_handle_t *handle, const es8311_config_t *config);

/**
 * 反初始化 ES8311
 *
 * @param handle ES8311 句柄
 * @return ESP_OK 成功
 */
esp_err_t es8311_deinit(es8311_handle_t *handle);

/**
 * 设置音量
 *
 * @param handle ES8311 句柄
 * @param volume 音量 (0-100)
 * @return ESP_OK 成功
 */
esp_err_t es8311_set_volume(es8311_handle_t *handle, uint8_t volume);

/**
 * 静音/取消静音
 *
 * @param handle ES8311 句柄
 * @param mute true=静音, false=取消静音
 * @return ESP_OK 成功
 */
esp_err_t es8311_set_mute(es8311_handle_t *handle, bool mute);

/**
 * 配置采样率
 *
 * @param handle ES8311 句柄
 * @param sample_rate 采样率 (Hz)
 * @return ESP_OK 成功
 */
esp_err_t es8311_set_sample_rate(es8311_handle_t *handle, uint32_t sample_rate);

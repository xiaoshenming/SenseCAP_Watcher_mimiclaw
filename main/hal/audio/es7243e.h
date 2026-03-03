#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"
#include <stdint.h>

/* ES7243E I2C 地址 */
#define ES7243E_I2C_ADDR 0x14

/* ES7243E 寄存器地址 */
#define ES7243E_REG_RESET           0x00
#define ES7243E_REG_CLK_MANAGER     0x01
#define ES7243E_REG_CLK_MANAGER2    0x02
#define ES7243E_REG_FORMAT          0x03
#define ES7243E_REG_POWER1          0x04
#define ES7243E_REG_POWER2          0x05
#define ES7243E_REG_ANALOG_PDN      0x07
#define ES7243E_REG_ADC_CTRL        0x09
#define ES7243E_REG_ADC_MUTE        0x0A
#define ES7243E_REG_ADC_VOLUME      0x0B
#define ES7243E_REG_MIC_GAIN        0x0E

/* ES7243E 配置结构 */
typedef struct {
    i2c_master_bus_handle_t i2c_bus;
    uint8_t i2c_addr;
    uint32_t sample_rate;
    uint8_t bits_per_sample;
    uint8_t mic_gain;   /* 麦克风增益 0-3: 0=0dB, 1=6dB, 2=12dB, 3=18dB */
} es7243e_config_t;

typedef struct {
    es7243e_config_t config;
    i2c_master_dev_handle_t i2c_dev;
} es7243e_handle_t;

/**
 * 初始化 ES7243E ADC
 *
 * @param handle ES7243E 句柄
 * @param config 配置参数
 * @return ESP_OK 成功
 */
esp_err_t es7243e_init(es7243e_handle_t *handle, const es7243e_config_t *config);

/**
 * 反初始化 ES7243E
 *
 * @param handle ES7243E 句柄
 * @return ESP_OK 成功
 */
esp_err_t es7243e_deinit(es7243e_handle_t *handle);

/**
 * 设置麦克风增益
 *
 * @param handle ES7243E 句柄
 * @param gain 增益 (0-3: 0=0dB, 1=6dB, 2=12dB, 3=18dB)
 * @return ESP_OK 成功
 */
esp_err_t es7243e_set_mic_gain(es7243e_handle_t *handle, uint8_t gain);

/**
 * 静音/取消静音 ADC
 *
 * @param handle ES7243E 句柄
 * @param mute true=静音, false=取消静音
 * @return ESP_OK 成功
 */
esp_err_t es7243e_set_mute(es7243e_handle_t *handle, bool mute);

/**
 * 设置 ADC 采样音量
 *
 * @param handle ES7243E 句柄
 * @param volume 音量 (0-255)
 * @return ESP_OK 成功
 */
esp_err_t es7243e_set_volume(es7243e_handle_t *handle, uint8_t volume);

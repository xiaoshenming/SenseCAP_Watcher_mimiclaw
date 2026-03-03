#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    HAL_PWR_SYSTEM = 0,
    HAL_PWR_LCD,
    HAL_PWR_SDCARD,
    HAL_PWR_AI_CHIP,
    HAL_PWR_CODEC_PA,
    HAL_PWR_BAT_DET,
    HAL_PWR_GROVE,
    HAL_PWR_BAT_ADC,
    HAL_PWR_MAX
} hal_power_id_t;

typedef enum {
    HAL_CHARGE_DISCHARGING = 0,
    HAL_CHARGE_CHARGING,
    HAL_CHARGE_STANDBY,
    HAL_CHARGE_NO_POWER,
} hal_charge_state_t;

esp_err_t hal_power_init(void);
esp_err_t hal_power_enable(hal_power_id_t id, bool enable);
esp_err_t hal_power_all_on(void);
esp_err_t hal_power_all_off(void);
uint32_t hal_power_get_battery_mv(void);
uint8_t hal_power_get_battery_pct(void);
hal_charge_state_t hal_power_get_charge_state(void);
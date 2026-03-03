#ifndef HAL_BATTERY_H
#define HAL_BATTERY_H

#include <esp_err.h>
#include <stdint.h>

esp_err_t hal_battery_init(void);
float hal_battery_get_voltage(void);
uint8_t hal_battery_get_percent(void);

#endif // HAL_BATTERY_H

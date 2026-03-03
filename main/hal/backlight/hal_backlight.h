#ifndef HAL_BACKLIGHT_H
#define HAL_BACKLIGHT_H

#include <esp_err.h>
#include <stdint.h>

esp_err_t hal_backlight_init(void);
esp_err_t hal_backlight_set_brightness(uint8_t brightness);

#endif // HAL_BACKLIGHT_H

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Touch event callback */
typedef void (*hal_touch_cb_t)(uint16_t x, uint16_t y, bool pressed, void *user_data);

/**
 * Initialize touch controller (CST816S on I2C1)
 */
esp_err_t hal_touch_init(void);

/**
 * Read current touch state
 * @param x      pointer to store X coordinate (0-411)
 * @param y      pointer to store Y coordinate (0-411)
 * @param pressed pointer to store touch state
 */
esp_err_t hal_touch_read(uint16_t *x, uint16_t *y, bool *pressed);

/**
 * Register touch event callback
 */
void hal_touch_register_cb(hal_touch_cb_t cb, void *user_data);

/**
 * Deinitialize touch controller
 */
esp_err_t hal_touch_deinit(void);

#ifdef __cplusplus
}
#endif
#pragma once

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize all hardware modules for SenseCAP Watcher.
 *
 * Initialization order:
 *  1. IO Expander (TCA9555) - controls power to other peripherals
 *  2. Power management - enables peripheral power rails
 *  3. Display (SPD2010 QSPI) + backlight
 *  4. Audio (ES8311 DAC + ES7243E ADC + I2S)
 *  5. Touch (CST816S on I2C1)
 *  6. RGB LED (WS2813)
 *  7. Rotary encoder + buttons
 *  8. SD card (SPI2 + FAT)
 *
 * @return ESP_OK on success, or the first error encountered.
 *         Non-critical failures (LED, SD) are logged but do not
 *         prevent the remaining modules from initializing.
 */
esp_err_t hal_init(void);

/**
 * Deinitialize all hardware modules (reverse order).
 */
esp_err_t hal_deinit(void);

#ifdef __cplusplus
}
#endif
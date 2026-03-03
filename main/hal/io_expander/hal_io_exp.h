#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/* Power control pins (Port 1 outputs) */
typedef enum {
    HAL_PWR_SDCARD = 0,
    HAL_PWR_LCD = 1,
    HAL_PWR_SYSTEM = 2,
    HAL_PWR_AI_CHIP = 4,
    HAL_PWR_CODEC_PA = 5,
    HAL_PWR_BAT_DET = 6,
    HAL_PWR_GROVE = 7,
    HAL_PWR_BAT_ADC = 8,
} hal_power_pin_t;

/* Input detection pins (Port 0 inputs) */
typedef enum {
    HAL_INPUT_PWR_CHRG_DET = 0,
    HAL_INPUT_PWR_STDBY_DET = 1,
    HAL_INPUT_PWR_VBUS_IN_DET = 2,
    HAL_INPUT_KNOB_BTN = 3,
    HAL_INPUT_SD_GPIO_DET = 4,
    HAL_INPUT_TOUCH_GPIO_INT = 5,
    HAL_INPUT_SSCMA_SPI_SYNC = 6,
    HAL_INPUT_SSCMA_CLIENT_RST = 7,
} hal_input_pin_t;

/* Interrupt callback type */
typedef void (*hal_io_exp_isr_cb_t)(uint8_t port0_state, void *user_data);

/**
 * Initialize IO expander HAL.
 * - Creates I2C master bus on I2C0 (GPIO 47/48)
 * - Initializes TCA9555 at address 0x21
 * - Configures Port 0 as inputs, Port 1 as outputs
 * - Sets up GPIO2 interrupt for Port 0 changes
 *
 * @return ESP_OK on success.
 */
esp_err_t hal_io_exp_init(void);

/**
 * Enable power to a peripheral.
 *
 * @param pin  Power control pin (HAL_PWR_*).
 * @param on   true = power on, false = power off.
 */
esp_err_t hal_io_exp_set_power(hal_power_pin_t pin, bool on);

/**
 * Read an input pin state.
 *
 * @param pin   Input pin (HAL_INPUT_*).
 * @param state Output: 0 = low, 1 = high.
 */
esp_err_t hal_io_exp_read_input(hal_input_pin_t pin, uint8_t *state);

/**
 * Read all Port 0 inputs at once.
 *
 * @param state Output: 8-bit value from Port 0.
 */
esp_err_t hal_io_exp_read_port0(uint8_t *state);

/**
 * Register an interrupt callback for Port 0 changes.
 * The callback is invoked from ISR context when GPIO2 interrupt fires.
 *
 * @param cb        Callback function.
 * @param user_data User data passed to callback.
 */
esp_err_t hal_io_exp_register_isr(hal_io_exp_isr_cb_t cb, void *user_data);

/**
 * Deinitialize IO expander HAL (cleanup resources).
 */
esp_err_t hal_io_exp_deinit(void);

/**
 * Get I2C bus handle for other HAL modules.
 * Must be called after hal_io_exp_init().
 *
 * @return I2C bus handle, or NULL if not initialized.
 */
i2c_master_bus_handle_t hal_io_exp_get_i2c_bus(void);

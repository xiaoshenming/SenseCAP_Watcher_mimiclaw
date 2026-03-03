#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"
#include <stdint.h>
#include <stdbool.h>

/* TCA9555 register addresses */
#define TCA9555_REG_INPUT_PORT0     0x00
#define TCA9555_REG_INPUT_PORT1     0x01
#define TCA9555_REG_OUTPUT_PORT0    0x02
#define TCA9555_REG_OUTPUT_PORT1    0x03
#define TCA9555_REG_POL_INV_PORT0   0x04
#define TCA9555_REG_POL_INV_PORT1   0x05
#define TCA9555_REG_CONFIG_PORT0    0x06
#define TCA9555_REG_CONFIG_PORT1    0x07

/* Direction: config register bit=1 → input, bit=0 → output */
#define TCA9555_DIR_INPUT    1
#define TCA9555_DIR_OUTPUT   0

/* I2C bus config */
#define TCA9555_I2C_PORT        I2C_NUM_0
#define TCA9555_I2C_SPEED_HZ    400000
#define TCA9555_I2C_TIMEOUT_MS  50

typedef struct {
    i2c_master_bus_handle_t  bus_handle;
    i2c_master_dev_handle_t  dev_handle;
    uint8_t                  i2c_addr;
    uint8_t                  output_port0;   /* shadow register */
    uint8_t                  output_port1;   /* shadow register */
} tca9555_t;

/**
 * Initialize TCA9555 on an existing I2C master bus.
 *
 * @param dev     Device context (caller-allocated, zeroed by this function).
 * @param bus     I2C master bus handle already created by the caller.
 * @param addr    7-bit I2C address (e.g. 0x21).
 * @return ESP_OK on success.
 */
esp_err_t tca9555_init(tca9555_t *dev, i2c_master_bus_handle_t bus, uint8_t addr);

/**
 * Set direction for all pins of a port at once.
 *
 * @param dev       Device context.
 * @param port      0 or 1.
 * @param dir_mask  Bit mask: 1 = input, 0 = output (matches TCA9555 config register).
 */
esp_err_t tca9555_set_port_direction(tca9555_t *dev, uint8_t port, uint8_t dir_mask);

/**
 * Set direction for a single pin.
 *
 * @param dev   Device context.
 * @param port  0 or 1.
 * @param pin   Bit position 0–7.
 * @param dir   TCA9555_DIR_INPUT or TCA9555_DIR_OUTPUT.
 */
esp_err_t tca9555_set_pin_direction(tca9555_t *dev, uint8_t port, uint8_t pin, uint8_t dir);

/**
 * Read all 8 input pins of a port.
 *
 * @param dev   Device context.
 * @param port  0 or 1.
 * @param val   Output: raw byte from input register.
 */
esp_err_t tca9555_read_port(tca9555_t *dev, uint8_t port, uint8_t *val);

/**
 * Read a single input pin.
 *
 * @param dev   Device context.
 * @param port  0 or 1.
 * @param pin   Bit position 0–7.
 * @param val   Output: 0 or 1.
 */
esp_err_t tca9555_read_pin(tca9555_t *dev, uint8_t port, uint8_t pin, uint8_t *val);

/**
 * Write all 8 output pins of a port (updates shadow register then writes chip).
 *
 * @param dev   Device context.
 * @param port  0 or 1.
 * @param val   Byte to write to output register.
 */
esp_err_t tca9555_write_port(tca9555_t *dev, uint8_t port, uint8_t val);

/**
 * Write a single output pin (read-modify-write on the shadow register).
 *
 * @param dev   Device context.
 * @param port  0 or 1.
 * @param pin   Bit position 0–7.
 * @param val   0 = low, non-zero = high.
 */
esp_err_t tca9555_write_pin(tca9555_t *dev, uint8_t port, uint8_t pin, uint8_t val);

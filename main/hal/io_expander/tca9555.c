#include "tca9555.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "TCA9555";

/* Helper: write one byte to a register */
static esp_err_t tca9555_write_reg(tca9555_t *dev, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    return i2c_master_transmit(dev->dev_handle, buf, 2, TCA9555_I2C_TIMEOUT_MS);
}

/* Helper: read one byte from a register */
static esp_err_t tca9555_read_reg(tca9555_t *dev, uint8_t reg, uint8_t *val)
{
    esp_err_t ret = i2c_master_transmit_receive(
        dev->dev_handle,
        &reg, 1,
        val, 1,
        TCA9555_I2C_TIMEOUT_MS
    );
    return ret;
}

esp_err_t tca9555_init(tca9555_t *dev, i2c_master_bus_handle_t bus, uint8_t addr)
{
    if (!dev || !bus) {
        return ESP_ERR_INVALID_ARG;
    }

    memset(dev, 0, sizeof(*dev));
    dev->bus_handle = bus;
    dev->i2c_addr = addr;

    /* Add device to the I2C bus */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = addr,
        .scl_speed_hz = TCA9555_I2C_SPEED_HZ,
    };
    esp_err_t ret = i2c_master_bus_add_device(bus, &dev_cfg, &dev->dev_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add TCA9555 device: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Read current output registers to initialize shadow */
    ret = tca9555_read_reg(dev, TCA9555_REG_OUTPUT_PORT0, &dev->output_port0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read output port 0: %s", esp_err_to_name(ret));
        return ret;
    }
    ret = tca9555_read_reg(dev, TCA9555_REG_OUTPUT_PORT1, &dev->output_port1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read output port 1: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "TCA9555 initialized at 0x%02X", addr);
    return ESP_OK;
}

esp_err_t tca9555_set_port_direction(tca9555_t *dev, uint8_t port, uint8_t dir_mask)
{
    if (!dev || port > 1) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t reg = (port == 0) ? TCA9555_REG_CONFIG_PORT0 : TCA9555_REG_CONFIG_PORT1;
    return tca9555_write_reg(dev, reg, dir_mask);
}

esp_err_t tca9555_set_pin_direction(tca9555_t *dev, uint8_t port, uint8_t pin, uint8_t dir)
{
    if (!dev || port > 1 || pin > 7) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg = (port == 0) ? TCA9555_REG_CONFIG_PORT0 : TCA9555_REG_CONFIG_PORT1;
    uint8_t current;
    esp_err_t ret = tca9555_read_reg(dev, reg, &current);
    if (ret != ESP_OK) {
        return ret;
    }

    if (dir == TCA9555_DIR_INPUT) {
        current |= (1 << pin);
    } else {
        current &= ~(1 << pin);
    }

    return tca9555_write_reg(dev, reg, current);
}

esp_err_t tca9555_read_port(tca9555_t *dev, uint8_t port, uint8_t *val)
{
    if (!dev || port > 1 || !val) {
        return ESP_ERR_INVALID_ARG;
    }
    uint8_t reg = (port == 0) ? TCA9555_REG_INPUT_PORT0 : TCA9555_REG_INPUT_PORT1;
    return tca9555_read_reg(dev, reg, val);
}

esp_err_t tca9555_read_pin(tca9555_t *dev, uint8_t port, uint8_t pin, uint8_t *val)
{
    if (!dev || port > 1 || pin > 7 || !val) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t port_val;
    esp_err_t ret = tca9555_read_port(dev, port, &port_val);
    if (ret != ESP_OK) {
        return ret;
    }

    *val = (port_val >> pin) & 0x01;
    return ESP_OK;
}

esp_err_t tca9555_write_port(tca9555_t *dev, uint8_t port, uint8_t val)
{
    if (!dev || port > 1) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t reg = (port == 0) ? TCA9555_REG_OUTPUT_PORT0 : TCA9555_REG_OUTPUT_PORT1;
    esp_err_t ret = tca9555_write_reg(dev, reg, val);
    if (ret == ESP_OK) {
        if (port == 0) {
            dev->output_port0 = val;
        } else {
            dev->output_port1 = val;
        }
    }
    return ret;
}

esp_err_t tca9555_write_pin(tca9555_t *dev, uint8_t port, uint8_t pin, uint8_t val)
{
    if (!dev || port > 1 || pin > 7) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t *shadow = (port == 0) ? &dev->output_port0 : &dev->output_port1;
    if (val) {
        *shadow |= (1 << pin);
    } else {
        *shadow &= ~(1 << pin);
    }

    return tca9555_write_port(dev, port, *shadow);
}

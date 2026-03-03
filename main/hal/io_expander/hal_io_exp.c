#include "hal_io_exp.h"
#include "tca9555.h"
#include "../hal_config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"

static const char *TAG = "HAL_IO_EXP";

static i2c_master_bus_handle_t s_i2c_bus = NULL;
static tca9555_t s_tca9555;
static hal_io_exp_isr_cb_t s_isr_callback = NULL;
static void *s_isr_user_data = NULL;

/* GPIO interrupt handler for TCA9555 INT pin */
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    if (s_isr_callback) {
        uint8_t port0_state = 0;
        /* Read Port 0 from ISR (quick I2C read) */
        tca9555_read_port(&s_tca9555, 0, &port0_state);
        s_isr_callback(port0_state, s_isr_user_data);
    }
}

esp_err_t hal_io_exp_init(void)
{
    esp_err_t ret;

    /* Create I2C master bus on I2C0 */
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C0_SDA,
        .scl_io_num = I2C0_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ret = i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Initialize TCA9555 */
    ret = tca9555_init(&s_tca9555, s_i2c_bus, I2C_ADDR_TCA9555);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init TCA9555: %s", esp_err_to_name(ret));
        i2c_del_master_bus(s_i2c_bus);
        s_i2c_bus = NULL;
        return ret;
    }

    /* Configure Port 0 as all inputs (0xFF = all input) */
    ret = tca9555_set_port_direction(&s_tca9555, 0, 0xFF);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Port 0 direction: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Configure Port 1 as all outputs (0x00 = all output) */
    ret = tca9555_set_port_direction(&s_tca9555, 1, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Port 1 direction: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Initialize Port 1 outputs to all off (0x00) */
    ret = tca9555_write_port(&s_tca9555, 1, 0x00);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init Port 1 outputs: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Configure GPIO2 as interrupt input (falling edge) */
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_IO_EXP_INT),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config GPIO2 interrupt: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Install GPIO ISR service */
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        /* ESP_ERR_INVALID_STATE means already installed, which is OK */
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Add ISR handler for GPIO2 */
    ret = gpio_isr_handler_add(GPIO_IO_EXP_INT, gpio_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add GPIO ISR handler: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "IO Expander HAL initialized");
    return ESP_OK;
}

esp_err_t hal_io_exp_set_power(hal_power_pin_t pin, bool on)
{
    if (pin > 8) {
        return ESP_ERR_INVALID_ARG;
    }
    return tca9555_write_pin(&s_tca9555, 1, pin, on ? 1 : 0);
}

esp_err_t hal_io_exp_read_input(hal_input_pin_t pin, uint8_t *state)
{
    if (pin > 7 || !state) {
        return ESP_ERR_INVALID_ARG;
    }
    return tca9555_read_pin(&s_tca9555, 0, pin, state);
}

esp_err_t hal_io_exp_read_port0(uint8_t *state)
{
    if (!state) {
        return ESP_ERR_INVALID_ARG;
    }
    return tca9555_read_port(&s_tca9555, 0, state);
}

esp_err_t hal_io_exp_register_isr(hal_io_exp_isr_cb_t cb, void *user_data)
{
    s_isr_callback = cb;
    s_isr_user_data = user_data;
    return ESP_OK;
}

esp_err_t hal_io_exp_deinit(void)
{
    gpio_isr_handler_remove(GPIO_IO_EXP_INT);
    if (s_i2c_bus) {
        i2c_del_master_bus(s_i2c_bus);
        s_i2c_bus = NULL;
    }
    ESP_LOGI(TAG, "IO Expander HAL deinitialized");
    return ESP_OK;
}

i2c_master_bus_handle_t hal_io_exp_get_i2c_bus(void)
{
    return s_i2c_bus;
}

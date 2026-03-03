#include "hal_touch.h"
#include "../hal_config.h"
#include "../io_expander/hal_io_exp.h"

#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <string.h>

static const char *TAG = "hal_touch";

/* CST816S I2C address */
#define CST816S_ADDR            0x15

/* CST816S registers */
#define CST816S_REG_GESTURE     0x01
#define CST816S_REG_FINGER_NUM  0x02
#define CST816S_REG_XH          0x03
#define CST816S_REG_XL          0x04
#define CST816S_REG_YH          0x05
#define CST816S_REG_YL          0x06
#define CST816S_REG_CHIP_ID     0xA7
#define CST816S_REG_SLEEP       0xE5
#define CST816S_SLEEP_IN        0x03

/* Read 6 bytes starting from gesture register */
#define CST816S_READ_LEN        6

/* Module state */
static struct {
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t dev;
    hal_touch_cb_t          cb;
    void                   *cb_user_data;
    bool                    initialized;
} s_touch;

/* Read a block of registers from CST816S */
static esp_err_t cst816s_read_regs(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_transmit_receive(s_touch.dev, &reg, 1, buf, len, 100);
}

/* IO expander interrupt callback - reads touch and fires user callback */
static void touch_isr_handler(void *arg)
{
    if (!s_touch.cb) return;

    uint16_t x, y;
    bool pressed;
    if (hal_touch_read(&x, &y, &pressed) == ESP_OK) {
        s_touch.cb(x, y, pressed, s_touch.cb_user_data);
    }
}

esp_err_t hal_touch_init(void)
{
    if (s_touch.initialized) {
        ESP_LOGW(TAG, "Already initialized");
        return ESP_OK;
    }

    esp_err_t ret;

    /* Initialize I2C1 bus */
    i2c_master_bus_config_t bus_cfg = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_1,
        .sda_io_num = I2C1_SDA,
        .scl_io_num = I2C1_SCL,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ret = i2c_new_master_bus(&bus_cfg, &s_touch.bus);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C1 bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Add CST816S device */
    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = CST816S_ADDR,
        .scl_speed_hz = 400000,
    };

    ret = i2c_master_bus_add_device(s_touch.bus, &dev_cfg, &s_touch.dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "CST816S device add failed: %s", esp_err_to_name(ret));
        goto err_bus;
    }

    /* Verify chip by reading chip ID */
    uint8_t chip_id = 0;
    ret = cst816s_read_regs(CST816S_REG_CHIP_ID, &chip_id, 1);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "CST816S chip ID read failed (may need touch to wake): %s", esp_err_to_name(ret));
        /* Don't fail - some CST816S variants only respond after first touch */
    } else {
        ESP_LOGI(TAG, "CST816S chip ID: 0x%02X", chip_id);
    }

    s_touch.initialized = true;
    ESP_LOGI(TAG, "Touch initialized (I2C1: SDA=%d, SCL=%d)", I2C1_SDA, I2C1_SCL);

    return ESP_OK;

err_bus:
    i2c_del_master_bus(s_touch.bus);
    s_touch.bus = NULL;
    return ret;
}

esp_err_t hal_touch_read(uint16_t *x, uint16_t *y, bool *pressed)
{
    if (!s_touch.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!x || !y || !pressed) {
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t buf[CST816S_READ_LEN] = {0};
    esp_err_t ret = cst816s_read_regs(CST816S_REG_GESTURE, buf, CST816S_READ_LEN);
    if (ret != ESP_OK) {
        *pressed = false;
        *x = 0;
        *y = 0;
        return ret;
    }

    /* buf[0]=gesture, buf[1]=finger_num, buf[2]=XH, buf[3]=XL, buf[4]=YH, buf[5]=YL */
    uint8_t finger_num = buf[1];
    *pressed = (finger_num > 0);

    if (*pressed) {
        *x = ((uint16_t)(buf[2] & 0x0F) << 8) | buf[3];
        *y = ((uint16_t)(buf[4] & 0x0F) << 8) | buf[5];

        /* Clamp to display bounds */
        if (*x >= LCD_WIDTH)  *x = LCD_WIDTH - 1;
        if (*y >= LCD_HEIGHT) *y = LCD_HEIGHT - 1;
    } else {
        *x = 0;
        *y = 0;
    }

    return ESP_OK;
}

void hal_touch_register_cb(hal_touch_cb_t cb, void *user_data)
{
    s_touch.cb = cb;
    s_touch.cb_user_data = user_data;

    if (cb && s_touch.initialized) {
        /* Touch interrupt comes through IO expander P0.5 */
        hal_io_exp_register_isr(touch_isr_handler, NULL);
        ESP_LOGI(TAG, "Touch callback registered (via IO expander INT)");
    }
}

esp_err_t hal_touch_deinit(void)
{
    if (!s_touch.initialized) {
        return ESP_OK;
    }

    /* Send sleep command */
    uint8_t sleep_cmd[] = {CST816S_REG_SLEEP, CST816S_SLEEP_IN};
    i2c_master_transmit(s_touch.dev, sleep_cmd, sizeof(sleep_cmd), 100);

    i2c_master_bus_rm_device(s_touch.dev);
    i2c_del_master_bus(s_touch.bus);

    memset(&s_touch, 0, sizeof(s_touch));
    ESP_LOGI(TAG, "Touch deinitialized");

    return ESP_OK;
}
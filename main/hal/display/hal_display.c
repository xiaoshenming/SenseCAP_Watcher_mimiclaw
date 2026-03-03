#include "hal_display.h"
#include "spd2010.h"
#include "../hal_config.h"
#include "../io_expander/hal_io_exp.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "HAL_DISPLAY";

/* 填充缓冲区大小（行数），用于分批写入 */
#define FILL_BUF_LINES  20

/* 全局状态 */
static struct {
    bool initialized;
    bool backlight_on;
} s_display = {0};

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

esp_err_t hal_display_init(void)
{
    if (s_display.initialized) {
        ESP_LOGW(TAG, "Display already initialized");
        return ESP_OK;
    }

    esp_err_t ret;

    /* 启用 LCD 电源 */
    ret = hal_io_exp_set_power(IO_EXP_PWR_LCD, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable LCD power: %s", esp_err_to_name(ret));
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(50));

    /* 初始化 SPD2010 驱动 */
    ret = spd2010_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init SPD2010: %s", esp_err_to_name(ret));
        goto err_power;
    }

    s_display.initialized = true;
    s_display.backlight_on = true;
    ESP_LOGI(TAG, "Display HAL initialized (%dx%d)", LCD_WIDTH, LCD_HEIGHT);
    return ESP_OK;

err_power:
    hal_io_exp_set_power(IO_EXP_PWR_LCD, false);
    return ret;
}

esp_err_t hal_display_deinit(void)
{
    if (!s_display.initialized) {
        return ESP_OK;
    }

    /* 关闭背光 */
    gpio_set_level(GPIO_LCD_BACKLIGHT, 0);

    /* 进入睡眠 */
    spd2010_send_cmd(0x10);  /* SLPIN */

    /* 关闭 LCD 电源 */
    hal_io_exp_set_power(IO_EXP_PWR_LCD, false);

    s_display.initialized = false;
    s_display.backlight_on = false;
    ESP_LOGI(TAG, "Display HAL deinitialized");
    return ESP_OK;
}

esp_err_t hal_display_fill(uint16_t color)
{
    return hal_display_fill_area(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1, color);
}

esp_err_t hal_display_fill_area(uint16_t x0, uint16_t y0,
                                 uint16_t x1, uint16_t y1,
                                 uint16_t color)
{
    if (!s_display.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t ret;

    ret = spd2010_set_window(x0, y0, x1, y1);
    if (ret != ESP_OK) return ret;

    uint16_t width = x1 - x0 + 1;
    uint16_t height = y1 - y0 + 1;

    /* 分配一行缓冲区并填充颜色 */
    size_t buf_lines = FILL_BUF_LINES;
    if (buf_lines > height) {
        buf_lines = height;
    }
    size_t buf_pixels = width * buf_lines;
    uint16_t *buf = malloc(buf_pixels * sizeof(uint16_t));
    if (!buf) {
        ESP_LOGE(TAG, "Failed to allocate fill buffer");
        return ESP_ERR_NO_MEM;
    }

    for (size_t i = 0; i < buf_pixels; i++) {
        buf[i] = color;
    }

    /* 分批写入 */
    uint16_t lines_remaining = height;
    while (lines_remaining > 0) {
        uint16_t lines = (lines_remaining > buf_lines) ? buf_lines : lines_remaining;
        size_t pixels = width * lines;
        ret = spd2010_write_pixels(buf, pixels);
        if (ret != ESP_OK) {
            free(buf);
            return ret;
        }
        lines_remaining -= lines;
    }

    free(buf);
    return ESP_OK;
}

esp_err_t hal_display_draw(uint16_t x0, uint16_t y0,
                            uint16_t x1, uint16_t y1,
                            const uint16_t *data, size_t pixel_count)
{
    if (!s_display.initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!data || pixel_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret;

    ret = spd2010_set_window(x0, y0, x1, y1);
    if (ret != ESP_OK) return ret;

    return spd2010_write_pixels(data, pixel_count);
}

esp_err_t hal_display_set_backlight(bool on)
{
    if (!s_display.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    gpio_set_level(GPIO_LCD_BACKLIGHT, on ? 1 : 0);
    s_display.backlight_on = on;
    ESP_LOGD(TAG, "Backlight %s", on ? "ON" : "OFF");
    return ESP_OK;
}

esp_err_t hal_display_sleep(void)
{
    if (!s_display.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 关闭背光 */
    gpio_set_level(GPIO_LCD_BACKLIGHT, 0);
    s_display.backlight_on = false;

    /* 关闭显示 */
    esp_err_t ret = spd2010_send_cmd(0x28);  /* DISPOFF */
    if (ret != ESP_OK) return ret;

    vTaskDelay(pdMS_TO_TICKS(20));

    /* 进入睡眠模式 */
    ret = spd2010_send_cmd(0x10);  /* SLPIN */
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "Display entered sleep mode");
    return ESP_OK;
}

esp_err_t hal_display_wakeup(void)
{
    if (!s_display.initialized) {
        return ESP_ERR_INVALID_STATE;
    }

    /* 退出睡眠模式 */
    esp_err_t ret = spd2010_send_cmd(0x11);  /* SLPOUT */
    if (ret != ESP_OK) return ret;

    vTaskDelay(pdMS_TO_TICKS(120));

    /* 开启显示 */
    ret = spd2010_send_cmd(0x29);  /* DISPON */
    if (ret != ESP_OK) return ret;

    /* 开启背光 */
    gpio_set_level(GPIO_LCD_BACKLIGHT, 1);
    s_display.backlight_on = true;

    ESP_LOGI(TAG, "Display woke up from sleep");
    return ESP_OK;
}
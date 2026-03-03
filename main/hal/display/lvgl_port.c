#include "lvgl_port.h"
#include "hal_display.h"
#include "../touch/hal_touch.h"
#include "../hal_config.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

static const char *TAG = "lvgl_port";
static bool s_initialized = false;

static void lvgl_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    uint16_t w = lv_area_get_width(area);
    uint16_t h = lv_area_get_height(area);
    hal_display_draw(area->x1, area->y1, area->x2, area->y2, (uint16_t *)px_map, w * h);
    lv_display_flush_ready(disp);
}

static void lvgl_touch_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t x, y;
    bool pressed;
    if (hal_touch_read(&x, &y, &pressed) == ESP_OK && pressed) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

esp_err_t lvgl_port_init(void)
{
    if (s_initialized) return ESP_OK;

    const lvgl_port_cfg_t cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = NULL,
        .panel_handle = NULL,
        .buffer_size = LCD_WIDTH * 40,
        .double_buffer = true,
        .hres = LCD_WIDTH,
        .vres = LCD_HEIGHT,
        .monochrome = false,
        .rotation = {.swap_xy = false, .mirror_x = false, .mirror_y = false},
        .flags = {.buff_dma = true, .buff_spiram = true}
    };
    
    lv_display_t *disp = lvgl_port_add_disp(&disp_cfg);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    const lvgl_port_touch_cfg_t touch_cfg = {
        .disp = disp,
        .handle = NULL,
    };
    lv_indev_t *indev = lvgl_port_add_touch(&touch_cfg);
    lv_indev_set_read_cb(indev, lvgl_touch_cb);

    s_initialized = true;
    ESP_LOGI(TAG, "LVGL port initialized");
    return ESP_OK;
}

esp_err_t lvgl_port_deinit(void)
{
    if (!s_initialized) return ESP_OK;
    lvgl_port_deinit();
    s_initialized = false;
    return ESP_OK;
}

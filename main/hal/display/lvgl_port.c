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

esp_err_t hal_lvgl_init(void)
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

    s_initialized = true;
    ESP_LOGI(TAG, "LVGL initialized");
    return ESP_OK;
}

esp_err_t hal_lvgl_deinit(void)
{
    if (!s_initialized) return ESP_OK;
    lvgl_port_deinit();
    s_initialized = false;
    return ESP_OK;
}

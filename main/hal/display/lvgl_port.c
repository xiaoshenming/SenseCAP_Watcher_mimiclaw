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

    /* Create display using LVGL native API */
    lv_display_t *disp = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
    lv_display_set_flush_cb(disp, lvgl_flush_cb);

    /* Allocate draw buffers */
    size_t buf_size = LCD_WIDTH * 40;
    void *buf1 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    void *buf2 = heap_caps_malloc(buf_size * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

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

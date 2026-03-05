#include "lvgl_port.h"
#include "spd2010.h"
#include "../touch/hal_touch.h"
#include "../hal_config.h"
#include "esp_lvgl_port.h"
#include "esp_log.h"

static const char *TAG = "lvgl_port";
static bool s_initialized = false;
static lv_display_t *s_display = NULL;

static void spd2010_area_align_cb(lv_event_t *e)
{
    lv_area_t *area = (lv_area_t *)lv_event_get_param(e);
    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;
    area->x1 = (x1 >> 2) << 2;
    area->x2 = ((x2 >> 2) << 2) + 3;
}

esp_err_t hal_lvgl_init(void)
{
    if (s_initialized) return ESP_OK;

    ESP_LOGI(TAG, "Initialize LVGL library");
    lv_init();

    ESP_LOGI(TAG, "Initialize LVGL port");
    const lvgl_port_cfg_t port_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&port_cfg));

    ESP_LOGI(TAG, "Adding LCD display");
    const lvgl_port_display_cfg_t display_cfg = {
        .io_handle = spd2010_get_io_handle(),
        .panel_handle = spd2010_get_panel_handle(),
        .control_handle = NULL,
        .buffer_size = LCD_WIDTH * 20,
        .double_buffer = false,
        .trans_size = 0,
        .hres = LCD_WIDTH,
        .vres = LCD_HEIGHT,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = 1,
            .buff_spiram = 0,
            .sw_rotate = 0,
            .swap_bytes = 1,
            .full_refresh = 0,
            .direct_mode = 0,
        },
    };

    s_display = lvgl_port_add_disp(&display_cfg);
    if (s_display == NULL) {
        ESP_LOGE(TAG, "Failed to add display");
        return ESP_FAIL;
    }

    /* SPD2010 要求列地址必须是4的倍数 */
    lv_display_add_event_cb(s_display, spd2010_area_align_cb, LV_EVENT_INVALIDATE_AREA, NULL);

    s_initialized = true;
    ESP_LOGI(TAG, "LVGL initialized successfully");
    return ESP_OK;
}

esp_err_t hal_lvgl_deinit(void)
{
    if (!s_initialized) return ESP_OK;
    lvgl_port_deinit();
    s_initialized = false;
    return ESP_OK;
}

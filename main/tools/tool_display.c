#include "tool_display.h"
#include "hal/display/lvgl_port.h"
#include "cJSON.h"
#include <string.h>
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

static const char *TAG = "tool_display";

/* Persistent label widget - created once, updated on each call */
static lv_obj_t *s_label = NULL;

esp_err_t tool_display_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json ? input_json : "{}");
    if (!root) {
        snprintf(output, output_size, "Error: invalid JSON input");
        return ESP_FAIL;
    }

    cJSON *text_item = cJSON_GetObjectItem(root, "text");
    const char *text = (text_item && cJSON_IsString(text_item)) ? text_item->valuestring : "";

    if (text[0] == '\0') {
        cJSON_Delete(root);
        snprintf(output, output_size, "Error: text field required");
        return ESP_FAIL;
    }

    /* Copy text before freeing JSON */
    char text_copy[256];
    strncpy(text_copy, text, sizeof(text_copy) - 1);
    text_copy[sizeof(text_copy) - 1] = '\0';
    cJSON_Delete(root);

    ESP_LOGI(TAG, "Displaying text: %s", text_copy);

    /* Acquire LVGL lock */
    if (!lvgl_port_lock(100)) {
        snprintf(output, output_size, "Error: failed to acquire LVGL lock");
        return ESP_FAIL;
    }

    /* Create or reuse label on the active screen */
    if (!s_label || !lv_obj_is_valid(s_label)) {
        lv_obj_t *scr = lv_screen_active();
        s_label = lv_label_create(scr);
        lv_obj_set_width(s_label, lv_pct(90));
        lv_label_set_long_mode(s_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_color(s_label, lv_color_white(), 0);
        lv_obj_set_style_text_font(s_label, &lv_font_montserrat_14, 0);
        lv_obj_align(s_label, LV_ALIGN_CENTER, 0, 0);
    }

    lv_label_set_text(s_label, text_copy);

    lvgl_port_unlock();

    snprintf(output, output_size, "Displayed on screen: %s", text_copy);
    return ESP_OK;
}

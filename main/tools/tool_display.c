#include "tool_display.h"
#include "hal/display/lvgl_port.h"
#include "hal/hal_config.h"
#include "cJSON.h"
#include <string.h>
#include "esp_log.h"
#include "esp_lvgl_port.h"
#include "lvgl.h"

static const char *TAG = "tool_display";

/* Persistent label widget */
static lv_obj_t *s_label = NULL;

/* Color name to lv_color_t */
static lv_color_t parse_color(const char *name) {
    if (!name) return lv_color_white();
    if (strcmp(name, "black") == 0) return lv_color_black();
    if (strcmp(name, "red") == 0) return lv_color_make(255, 0, 0);
    if (strcmp(name, "green") == 0) return lv_color_make(0, 255, 0);
    if (strcmp(name, "blue") == 0) return lv_color_make(0, 100, 255);
    if (strcmp(name, "yellow") == 0) return lv_color_make(255, 255, 0);
    if (strcmp(name, "cyan") == 0) return lv_color_make(0, 255, 255);
    if (strcmp(name, "magenta") == 0) return lv_color_make(255, 0, 255);
    if (strcmp(name, "gray") == 0) return lv_color_make(128, 128, 128);
    if (strcmp(name, "orange") == 0) return lv_color_make(255, 165, 0);
    if (strcmp(name, "pink") == 0) return lv_color_make(255, 192, 203);
    return lv_color_white();
}

/* Ensure label exists */
static void ensure_label(void) {
    if (s_label && lv_obj_is_valid(s_label)) return;

    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    s_label = lv_label_create(scr);
    lv_obj_set_width(s_label, LCD_WIDTH - 20);
    lv_label_set_long_mode(s_label, LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_color(s_label, lv_color_white(), 0);
    lv_obj_set_style_text_font(s_label, &lv_font_montserrat_14, 0);
    lv_obj_align(s_label, LV_ALIGN_CENTER, 0, 0);

    ESP_LOGI(TAG, "Label created");
}

esp_err_t tool_display_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json ? input_json : "{}");
    if (!root) {
        snprintf(output, output_size, "Error: invalid JSON input");
        return ESP_FAIL;
    }

    /* Get action */
    cJSON *action_item = cJSON_GetObjectItem(root, "action");
    const char *action = (action_item && cJSON_IsString(action_item)) ? action_item->valuestring : "text";

    /* Get common parameters */
    cJSON *text = cJSON_GetObjectItem(root, "text");
    cJSON *color = cJSON_GetObjectItem(root, "color");

    const char *text_str = (text && cJSON_IsString(text)) ? text->valuestring : "";
    const char *color_str = (color && cJSON_IsString(color)) ? color->valuestring : "white";

    ESP_LOGI(TAG, "Display action: %s", action);

    /* Acquire LVGL lock */
    if (!lvgl_port_lock(100)) {
        cJSON_Delete(root);
        snprintf(output, output_size, "Error: failed to acquire LVGL lock");
        return ESP_FAIL;
    }

    /* Execute action */
    if (strcmp(action, "clear") == 0) {
        ensure_label();
        lv_label_set_text(s_label, "");
        snprintf(output, output_size, "Screen cleared");
    }
    else if (strcmp(action, "text") == 0) {
        if (text_str[0] == '\0') {
            snprintf(output, output_size, "Error: text required for 'text' action");
        } else {
            ensure_label();
            lv_obj_set_style_text_color(s_label, parse_color(color_str), 0);
            lv_label_set_text(s_label, text_str);
            snprintf(output, output_size, "Text displayed: %s", text_str);
        }
    }
    else {
        snprintf(output, output_size, "Error: unknown action '%s'. Use: clear, text", action);
    }

    lv_refr_now(lv_display_get_default());
    lvgl_port_unlock();
    cJSON_Delete(root);
    return ESP_OK;
}

#include "tool_camera.h"
#include "hal/camera/hal_camera.h"
#include "channels/telegram/telegram_bot.h"
#include "cJSON.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "tool_camera";

esp_err_t tool_camera_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json ? input_json : "{}");
    if (!root) {
        snprintf(output, output_size, "Error: invalid JSON input");
        return ESP_FAIL;
    }

    cJSON *send_tg = cJSON_GetObjectItem(root, "send_telegram");
    cJSON *chat_id = cJSON_GetObjectItem(root, "chat_id");

    bool should_send = cJSON_IsTrue(send_tg);
    const char *chat_id_str = (chat_id && cJSON_IsString(chat_id)) ? chat_id->valuestring : NULL;

    cJSON_Delete(root);

    if (should_send && (!chat_id_str || chat_id_str[0] == '\0')) {
        snprintf(output, output_size, "Error: chat_id required when send_telegram=true");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Capturing photo...");

    /* Ensure camera is initialized */
    esp_err_t err = hal_camera_init();
    if (err != ESP_OK) {
        snprintf(output, output_size, "Error: camera init failed (%s)", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", output);
        return err;
    }

    uint8_t *jpeg_buf = NULL;
    size_t jpeg_len = 0;
    err = hal_camera_capture(&jpeg_buf, &jpeg_len);

    if (err != ESP_OK) {
        snprintf(output, output_size, "Error: camera capture failed (%s)", esp_err_to_name(err));
        ESP_LOGE(TAG, "%s", output);
        return err;
    }

    ESP_LOGI(TAG, "Photo captured: %u bytes", (unsigned)jpeg_len);

    if (should_send) {
        ESP_LOGI(TAG, "Sending photo to Telegram chat %s", chat_id_str);
        err = telegram_send_photo(chat_id_str, jpeg_buf, jpeg_len, "Photo captured by AI");

        if (err == ESP_OK) {
            snprintf(output, output_size, "Photo captured and sent to Telegram (%u bytes)", (unsigned)jpeg_len);
        } else {
            snprintf(output, output_size, "Photo captured (%u bytes) but failed to send: %s",
                     (unsigned)jpeg_len, esp_err_to_name(err));
        }
    } else {
        snprintf(output, output_size, "Photo captured: %u bytes", (unsigned)jpeg_len);
    }

    free(jpeg_buf);
    return ESP_OK;
}

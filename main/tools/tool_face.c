#include "tool_face.h"
#include "ui/robot_face.h"
#include "cJSON.h"
#include <string.h>
#include "esp_log.h"

static const char *TAG = "tool_face";

esp_err_t tool_face_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json);
    if (!root) {
        snprintf(output, output_size, "{\"error\":\"Invalid JSON\"}");
        return ESP_OK;
    }

    cJSON *action = cJSON_GetObjectItem(root, "action");
    const char *act = (action && action->valuestring) ? action->valuestring : "set";

    if (strcmp(act, "list") == 0) {
        snprintf(output, output_size, "{\"emotions\":\"%s\", \"current\":\"%s\"}",
                 robot_face_list_emotions(), robot_face_get_emotion());
        cJSON_Delete(root);
        return ESP_OK;
    }

    if (strcmp(act, "status") == 0) {
        snprintf(output, output_size, "{\"active\":%s, \"emotion\":\"%s\"}",
                 robot_face_is_active() ? "true" : "false",
                 robot_face_get_emotion());
        cJSON_Delete(root);
        return ESP_OK;
    }

    /* Default: set emotion */
    cJSON *emo = cJSON_GetObjectItem(root, "emotion");
    if (!emo || !emo->valuestring) {
        snprintf(output, output_size, "{\"error\":\"Missing 'emotion' field. Use action=list to see options\"}");
        cJSON_Delete(root);
        return ESP_OK;
    }

    if (!robot_face_is_active()) {
        ESP_LOGI(TAG, "Starting robot face for emotion");
        robot_face_start();
    }

    esp_err_t ret = robot_face_set_emotion(emo->valuestring);
    if (ret == ESP_OK) {
        snprintf(output, output_size, "{\"ok\":true, \"emotion\":\"%s\"}", emo->valuestring);
    } else if (ret == ESP_ERR_NOT_FOUND) {
        snprintf(output, output_size,
                 "{\"error\":\"Unknown emotion '%s'. Use action=list to see available emotions\"}",
                 emo->valuestring);
    } else {
        snprintf(output, output_size, "{\"error\":\"Failed to set emotion\"}");
    }

    cJSON_Delete(root);
    return ESP_OK;
}

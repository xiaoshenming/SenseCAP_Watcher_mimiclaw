#include "tool_sysinfo.h"
#include "hal/power/hal_power.h"
#include "cJSON.h"
#include <string.h>
#include "esp_log.h"
#include "esp_wifi.h"

static const char *TAG = "tool_sysinfo";

esp_err_t tool_sysinfo_execute(const char *input_json, char *output, size_t output_size)
{
    cJSON *root = cJSON_Parse(input_json ? input_json : "{}");
    cJSON *type_item = root ? cJSON_GetObjectItem(root, "info_type") : NULL;
    const char *info_type_ptr = (type_item && cJSON_IsString(type_item)) ? type_item->valuestring : "all";

    /* Copy info_type before deleting JSON to avoid dangling pointer */
    char info_type[32];
    strncpy(info_type, info_type_ptr, sizeof(info_type) - 1);
    info_type[sizeof(info_type) - 1] = '\0';

    if (root) cJSON_Delete(root);

    char buf[512] = {0};
    int off = 0;

    if (strcmp(info_type, "battery") == 0 || strcmp(info_type, "all") == 0) {
        uint8_t percent = hal_power_get_battery_pct();
        uint32_t voltage_mv = hal_power_get_battery_mv();
        float voltage_v = voltage_mv / 1000.0f;
        off += snprintf(buf + off, sizeof(buf) - off, "Battery: %d%% (%.2fV)\n", percent, voltage_v);
    }

    if (strcmp(info_type, "wifi") == 0 || strcmp(info_type, "all") == 0) {
        wifi_ap_record_t ap_info;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            off += snprintf(buf + off, sizeof(buf) - off, "WiFi: %s (RSSI: %d dBm)\n",
                          (char *)ap_info.ssid, ap_info.rssi);
        } else {
            off += snprintf(buf + off, sizeof(buf) - off, "WiFi: not connected\n");
        }
    }

    if (off == 0) {
        snprintf(buf, sizeof(buf), "info_type='%s' not recognized, use battery/wifi/all\n", info_type);
    }

    snprintf(output, output_size, "%s", buf);
    ESP_LOGI(TAG, "System info: %s", buf);
    return ESP_OK;
}

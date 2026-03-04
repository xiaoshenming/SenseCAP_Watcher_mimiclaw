#include "tool_sysinfo.h"
#include "hal/power/hal_power.h"
#include "cJSON.h"
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_chip_info.h"
#include "esp_timer.h"
#include "esp_psram.h"
#include "esp_idf_version.h"
#include "esp_flash.h"
#include "esp_clk_tree.h"

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

    if (strcmp(info_type, "memory") == 0 || strcmp(info_type, "all") == 0) {
        uint32_t heap_free = esp_get_free_heap_size();
        uint32_t heap_min = esp_get_minimum_free_heap_size();
        off += snprintf(buf + off, sizeof(buf) - off, "Heap: %"PRIu32" KB free (min: %"PRIu32" KB)\n",
                       heap_free / 1024, heap_min / 1024);
        if (esp_psram_get_size() > 0) {
            off += snprintf(buf + off, sizeof(buf) - off, "PSRAM: %zu MB total\n",
                           esp_psram_get_size() / (1024 * 1024));
        }
    }

    if (strcmp(info_type, "cpu") == 0 || strcmp(info_type, "all") == 0) {
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        off += snprintf(buf + off, sizeof(buf) - off, "CPU: %d cores @ %d MHz\n",
                       chip_info.cores, CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ);
    }

    if (strcmp(info_type, "uptime") == 0 || strcmp(info_type, "all") == 0) {
        uint64_t uptime_us = esp_timer_get_time();
        uint32_t uptime_s = uptime_us / 1000000;
        uint32_t days = uptime_s / 86400;
        uint32_t hours = (uptime_s % 86400) / 3600;
        uint32_t mins = (uptime_s % 3600) / 60;
        off += snprintf(buf + off, sizeof(buf) - off, "Uptime: %"PRIu32"d %"PRIu32"h %"PRIu32"m\n", days, hours, mins);
    }

    if (strcmp(info_type, "flash") == 0 || strcmp(info_type, "all") == 0) {
        uint32_t flash_size;
        esp_flash_get_size(NULL, &flash_size);
        off += snprintf(buf + off, sizeof(buf) - off, "Flash: %"PRIu32" MB\n", flash_size / (1024 * 1024));
    }

    if (strcmp(info_type, "system") == 0 || strcmp(info_type, "all") == 0) {
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);
        off += snprintf(buf + off, sizeof(buf) - off, "Chip: ESP32-S3 rev%d\n", chip_info.revision);
        off += snprintf(buf + off, sizeof(buf) - off, "IDF: %s\n", esp_get_idf_version());
    }

    if (off == 0) {
        snprintf(buf, sizeof(buf), "info_type='%s' not recognized, use: battery/wifi/memory/cpu/uptime/flash/system/all\n", info_type);
    }

    snprintf(output, output_size, "%s", buf);
    ESP_LOGI(TAG, "System info: %s", buf);
    return ESP_OK;
}

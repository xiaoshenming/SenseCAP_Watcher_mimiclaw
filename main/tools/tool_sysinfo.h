#pragma once

#include "esp_err.h"
#include <stddef.h>

/**
 * Execute system info tool - get device status.
 *
 * Input JSON:
 * {
 *   "info_type": "battery"  // "battery", "wifi", "all"
 * }
 */
esp_err_t tool_sysinfo_execute(const char *input_json, char *output, size_t output_size);

#pragma once

#include "esp_err.h"
#include <stddef.h>

/**
 * Execute display tool - show text on screen.
 *
 * Input JSON:
 * {
 *   "text": "Hello World",
 *   "x": 10,
 *   "y": 10
 * }
 */
esp_err_t tool_display_execute(const char *input_json, char *output, size_t output_size);

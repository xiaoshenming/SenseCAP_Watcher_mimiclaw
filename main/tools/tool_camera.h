#pragma once

#include "esp_err.h"
#include <stddef.h>

/**
 * Execute camera capture tool.
 * Takes a photo and returns the file path or sends via Telegram.
 *
 * Input JSON:
 * {
 *   "action": "capture",           // "capture" to take photo
 *   "send_telegram": true,         // optional: send via Telegram
 *   "chat_id": "123456"           // required if send_telegram=true
 * }
 */
esp_err_t tool_camera_execute(const char *input_json, char *output, size_t output_size);

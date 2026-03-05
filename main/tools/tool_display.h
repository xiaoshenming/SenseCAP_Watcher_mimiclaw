#pragma once

#include "esp_err.h"
#include <stddef.h>

/**
 * Execute display tool - draw on screen.
 *
 * Input JSON (examples):
 * {"action": "text", "text": "Hello", "x": 10, "y": 200, "color": "white"}
 * {"action": "rect", "x": 50, "y": 50, "width": 100, "height": 80, "color": "red", "fill": true}
 * {"action": "circle", "x": 206, "y": 206, "radius": 50, "color": "cyan", "fill": false}
 * {"action": "line", "x": 0, "y": 0, "x2": 411, "y2": 411, "color": "green", "width": 3}
 * {"action": "point", "x": 100, "y": 100, "color": "yellow", "size": 5}
 * {"action": "clear"}
 * {"action": "fill", "color": "black"}
 */
esp_err_t tool_display_execute(const char *input_json, char *output, size_t output_size);

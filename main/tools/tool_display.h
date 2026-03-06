#pragma once

#include "esp_err.h"
#include <stddef.h>

/**
 * Execute display tool — draw on the 412×412 circular screen.
 *
 * Each call ADDS an element (accumulated). Use action="clear" to reset.
 *
 * Positioning: use x,y for absolute pixels OR align for named position:
 *   align: "center", "top", "bottom", "left", "right",
 *          "top_left", "top_right", "bottom_left", "bottom_right"
 *   With optional x_offset, y_offset for fine-tuning.
 *
 * Actions:
 *   text   — {"action":"text", "text":"Hi", "x":50, "y":100, "color":"white", "font_size":28, "bg_color":"blue"}
 *   rect   — {"action":"rect", "x":50, "y":50, "width":100, "height":80, "color":"red", "fill":true, "radius":10}
 *   circle — {"action":"circle", "align":"center", "radius":50, "color":"cyan", "fill":false}
 *   line   — {"action":"line", "x":0, "y":0, "x2":411, "y2":411, "color":"green", "width":3}
 *   arc    — {"action":"arc", "align":"center", "radius":80, "start_angle":0, "end_angle":270, "color":"yellow"}
 *   symbol — {"action":"symbol", "name":"wifi", "align":"top_right", "color":"green", "font_size":36}
 *   clear  — {"action":"clear"}
 *   fill   — {"action":"fill", "color":"blue"}
 */
esp_err_t tool_display_execute(const char *input_json, char *output, size_t output_size);

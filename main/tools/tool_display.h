#pragma once

#include "esp_err.h"
#include <stddef.h>

/**
 * Execute display tool — draw on the 412×412 circular screen.
 *
 * INCREMENTAL model: each call adds/modifies elements. Objects get an "id"
 * for later reference via update/delete/animate. No need to clear+redraw!
 *
 * Positioning: x,y for absolute or align for named position.
 *
 * Actions:
 *   query   — returns all objects currently on screen
 *   text    — add text with optional id
 *   rect    — add rectangle
 *   circle  — add circle
 *   line    — add line
 *   arc     — add arc
 *   symbol  — add built-in icon
 *   update  — modify existing object by id (color, position, text, opacity, size)
 *   delete  — remove specific object by id
 *   animate — animate object property (x, y, opacity, width, height) with easing
 *   clear   — remove all objects
 *   fill    — set background color
 */
esp_err_t tool_display_execute(const char *input_json, char *output, size_t output_size);

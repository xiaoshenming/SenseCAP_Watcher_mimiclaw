#pragma once

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Start the cute robot face idle animation on the display.
 * Includes blinking eyes, wandering pupils, and breathing effect.
 * Call robot_face_stop() before using the display for other purposes.
 */
esp_err_t robot_face_start(void);

/**
 * Stop the robot face animation and clean up all LVGL objects.
 */
esp_err_t robot_face_stop(void);

/**
 * Check if the robot face is currently displayed.
 */
bool robot_face_is_active(void);

#ifdef __cplusplus
}
#endif

#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t hal_lvgl_init(void);
esp_err_t hal_lvgl_deinit(void);

#ifdef __cplusplus
}
#endif

#endif

#ifndef LVGL_PORT_H
#define LVGL_PORT_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t lvgl_port_init(void);
esp_err_t lvgl_port_deinit(void);

#ifdef __cplusplus
}
#endif

#endif

#ifndef HAL_CAMERA_H
#define HAL_CAMERA_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t hal_camera_init(void);
esp_err_t hal_camera_capture(uint8_t **jpeg_buf, size_t *jpeg_len);
esp_err_t hal_camera_deinit(void);

#ifdef __cplusplus
}
#endif

#endif

#ifndef SSCMA_CLIENT_H
#define SSCMA_CLIENT_H

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *data;
    size_t len;
} sscma_image_t;

esp_err_t sscma_client_init(void);
esp_err_t sscma_client_capture(sscma_image_t *image);
esp_err_t sscma_client_deinit(void);

#ifdef __cplusplus
}
#endif

#endif

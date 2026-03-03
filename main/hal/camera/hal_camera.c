#include "hal_camera.h"
#include "sscma_client.h"
#include "esp_log.h"

static const char *TAG = "hal_camera";
static bool s_initialized = false;

esp_err_t hal_camera_init(void)
{
    if (s_initialized) return ESP_OK;

    esp_err_t ret = sscma_hal_init();
    if (ret == ESP_OK) {
        s_initialized = true;
        ESP_LOGI(TAG, "Camera HAL initialized");
    }
    return ret;
}

esp_err_t hal_camera_capture(uint8_t **jpeg_buf, size_t *jpeg_len)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    if (!jpeg_buf || !jpeg_len) return ESP_ERR_INVALID_ARG;

    sscma_image_t img;
    esp_err_t ret = sscma_hal_capture(&img);
    if (ret == ESP_OK) {
        *jpeg_buf = img.data;
        *jpeg_len = img.len;
        ESP_LOGI(TAG, "Captured %zu bytes", img.len);
    }
    return ret;
}

esp_err_t hal_camera_deinit(void)
{
    if (!s_initialized) return ESP_OK;

    esp_err_t ret = sscma_hal_deinit();
    s_initialized = false;
    return ret;
}

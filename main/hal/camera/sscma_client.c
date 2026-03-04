#include "sscma_client.h"
#include "../hal_config.h"
#include "../io_expander/hal_io_exp.h"
#include "sscma_client_io.h"
#include "sscma_client_ops.h"
#include "driver/spi_common.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "sscma";

static sscma_client_handle_t s_client = NULL;
static QueueHandle_t s_image_queue = NULL;
static uint8_t *s_img_buf = NULL;
static size_t s_img_len = 0;

typedef struct {
    uint8_t *data;
    size_t len;
} image_data_t;

static void on_event_cb(sscma_client_handle_t client, const sscma_client_reply_t *reply, void *user_ctx)
{
    if (!reply || !reply->data || reply->len == 0) return;

    image_data_t img = {
        .data = (uint8_t*)malloc(reply->len),
        .len = reply->len
    };

    if (img.data) {
        memcpy(img.data, reply->data, reply->len);
        xQueueSend(s_image_queue, &img, 0);
    }
}

esp_err_t sscma_hal_init(void)
{
    if (s_client) return ESP_OK;

    hal_io_exp_set_power(IO_EXP_PWR_AI_CHIP, true);
    vTaskDelay(pdMS_TO_TICKS(100));

    sscma_client_io_spi_config_t io_cfg = {
        .cs_gpio_num = GPIO_SSCMA_CS,
        .sync_gpio_num = 6,
        .spi_mode = 0,
        .wait_delay = 10,
        .pclk_hz = 12 * 1000 * 1000,
        .trans_queue_depth = 1,
        .io_expander = NULL,
        .flags = {
            .sync_use_expander = 0,
        }
    };

    sscma_client_io_handle_t io_handle;
    esp_err_t io_ret = sscma_client_new_io_spi_bus((sscma_client_spi_bus_handle_t)SPI2_HOST, &io_cfg, &io_handle);
    if (io_ret != ESP_OK) {
        ESP_LOGE(TAG, "sscma_client_new_io_spi_bus failed: %s", esp_err_to_name(io_ret));
        hal_io_exp_set_power(IO_EXP_PWR_AI_CHIP, false);
        return io_ret;
    }

    sscma_client_config_t client_cfg = SSCMA_CLIENT_CONFIG_DEFAULT();
    client_cfg.reset_gpio_num = 7;
    client_cfg.rx_buffer_size = 98304;
    client_cfg.io_expander = NULL;
    client_cfg.flags.reset_use_expander = 0;

    esp_err_t client_ret = sscma_client_new(io_handle, &client_cfg, &s_client);
    if (client_ret != ESP_OK) {
        ESP_LOGE(TAG, "sscma_client_new failed: %s", esp_err_to_name(client_ret));
        hal_io_exp_set_power(IO_EXP_PWR_AI_CHIP, false);
        return client_ret;
    }

    sscma_client_callback_t cb = {
        .on_event = on_event_cb,
    };
    sscma_client_register_callback(s_client, &cb, NULL);

    s_image_queue = xQueueCreate(1, sizeof(image_data_t));

    esp_err_t init_ret = sscma_client_init(s_client);
    if (init_ret != ESP_OK) {
        ESP_LOGE(TAG, "sscma_client_init failed: %s", esp_err_to_name(init_ret));
        sscma_client_del(s_client);
        s_client = NULL;
        hal_io_exp_set_power(IO_EXP_PWR_AI_CHIP, false);
        return init_ret;
    }
    vTaskDelay(pdMS_TO_TICKS(2000));

    ESP_LOGI(TAG, "SSCMA client initialized");
    return ESP_OK;
}

esp_err_t sscma_hal_capture(sscma_image_t *image)
{
    if (!s_client || !image) return ESP_ERR_INVALID_ARG;

    image_data_t dummy;
    while (xQueueReceive(s_image_queue, &dummy, 0) == pdPASS) {
        if (dummy.data) free(dummy.data);
    }

    ESP_ERROR_CHECK(sscma_client_sample(s_client, 1));

    image_data_t img;
    if (xQueueReceive(s_image_queue, &img, pdMS_TO_TICKS(3000)) != pdPASS) {
        ESP_LOGW(TAG, "Timeout waiting for image");
        return ESP_ERR_TIMEOUT;
    }

    if (s_img_buf) free(s_img_buf);
    s_img_buf = img.data;
    s_img_len = img.len;

    image->data = s_img_buf;
    image->len = s_img_len;

    ESP_LOGI(TAG, "Captured %zu bytes", s_img_len);
    return ESP_OK;
}

esp_err_t sscma_hal_deinit(void)
{
    if (s_img_buf) {
        free(s_img_buf);
        s_img_buf = NULL;
    }
    if (s_client) {
        sscma_client_del(s_client);
        s_client = NULL;
    }
    if (s_image_queue) {
        vQueueDelete(s_image_queue);
        s_image_queue = NULL;
    }
    hal_io_exp_set_power(IO_EXP_PWR_AI_CHIP, false);
    return ESP_OK;
}

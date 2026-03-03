#include "sscma_client.h"
#include "../hal_config.h"
#include "../io_expander/hal_io_exp.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "sscma";

#define SSCMA_CMD_INVOKE    0x01
#define SSCMA_CMD_SAMPLE    0x02
#define SSCMA_RESP_OK       0x00

static spi_device_handle_t s_spi = NULL;
static uint8_t *s_img_buf = NULL;
static size_t s_img_len = 0;

static esp_err_t sscma_spi_write(const uint8_t *data, size_t len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .tx_buffer = data,
    };
    return spi_device_polling_transmit(s_spi, &t);
}

static esp_err_t sscma_spi_read(uint8_t *data, size_t len)
{
    spi_transaction_t t = {
        .length = len * 8,
        .rx_buffer = data,
        .flags = SPI_TRANS_USE_RXDATA,
    };
    if (len > 4) {
        t.flags = 0;
    }
    return spi_device_polling_transmit(s_spi, &t);
}

esp_err_t sscma_client_init(void)
{
    if (s_spi) return ESP_OK;

    hal_io_exp_set_power(IO_EXP_PWR_AI_CHIP, true);
    vTaskDelay(pdMS_TO_TICKS(100));

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = GPIO_SPI2_MOSI,
        .miso_io_num = GPIO_SPI2_MISO,
        .sclk_io_num = GPIO_SPI2_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 65536,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = GPIO_SSCMA_CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_spi));

    vTaskDelay(pdMS_TO_TICKS(3000));
    ESP_LOGI(TAG, "SSCMA client initialized");
    return ESP_OK;
}

esp_err_t sscma_client_capture(sscma_image_t *image)
{
    if (!s_spi || !image) return ESP_ERR_INVALID_ARG;

    uint8_t cmd[] = {SSCMA_CMD_SAMPLE, 0x02};
    ESP_ERROR_CHECK(sscma_spi_write(cmd, sizeof(cmd)));
    vTaskDelay(pdMS_TO_TICKS(500));

    uint8_t resp[8];
    ESP_ERROR_CHECK(sscma_spi_read(resp, sizeof(resp)));
    
    if (resp[0] != SSCMA_RESP_OK) {
        ESP_LOGW(TAG, "Capture failed: 0x%02x", resp[0]);
        return ESP_FAIL;
    }

    uint32_t img_size = (resp[4] << 24) | (resp[5] << 16) | (resp[6] << 8) | resp[7];
    if (img_size == 0 || img_size > 100000) {
        ESP_LOGW(TAG, "Invalid image size: %lu", img_size);
        return ESP_FAIL;
    }

    if (s_img_buf) free(s_img_buf);
    s_img_buf = malloc(img_size);
    if (!s_img_buf) return ESP_ERR_NO_MEM;

    spi_transaction_t t = {
        .length = img_size * 8,
        .rx_buffer = s_img_buf,
    };
    ESP_ERROR_CHECK(spi_device_polling_transmit(s_spi, &t));

    image->data = s_img_buf;
    image->len = img_size;
    s_img_len = img_size;

    ESP_LOGI(TAG, "Captured %zu bytes", img_size);
    return ESP_OK;
}

esp_err_t sscma_client_deinit(void)
{
    if (s_img_buf) {
        free(s_img_buf);
        s_img_buf = NULL;
    }
    if (s_spi) {
        spi_bus_remove_device(s_spi);
        spi_bus_free(SPI2_HOST);
        s_spi = NULL;
    }
    hal_io_exp_set_power(IO_EXP_PWR_AI_CHIP, false);
    return ESP_OK;
}

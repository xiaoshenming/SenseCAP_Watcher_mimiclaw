#include "hal_camera.h"
#include "../hal_config.h"
#include "../io_expander/hal_io_exp.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "hal_camera";
static spi_device_handle_t s_spi_dev = NULL;
static bool s_initialized = false;

esp_err_t hal_camera_init(void)
{
    if (s_initialized) return ESP_OK;

    // 启用AI芯片电源
    hal_io_exp_set_power(IO_EXP_PWR_AI_CHIP, true);
    vTaskDelay(pdMS_TO_TICKS(100));

    // 初始化SPI2
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = GPIO_SPI2_MOSI,
        .miso_io_num = GPIO_SPI2_MISO,
        .sclk_io_num = GPIO_SPI2_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    ESP_ERROR_CHECK(spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO));

    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = 10 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = GPIO_SSCMA_CS,
        .queue_size = 1,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &dev_cfg, &s_spi_dev));

    s_initialized = true;
    ESP_LOGI(TAG, "Camera initialized");
    return ESP_OK;
}

esp_err_t hal_camera_capture(uint8_t **jpeg_buf, size_t *jpeg_len)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    
    // TODO: 实现SSCMA Client协议
    ESP_LOGW(TAG, "Camera capture not fully implemented yet");
    return ESP_ERR_NOT_SUPPORTED;
}

esp_err_t hal_camera_deinit(void)
{
    if (!s_initialized) return ESP_OK;
    
    spi_bus_remove_device(s_spi_dev);
    spi_bus_free(SPI2_HOST);
    hal_io_exp_set_power(IO_EXP_PWR_AI_CHIP, false);
    
    s_initialized = false;
    return ESP_OK;
}

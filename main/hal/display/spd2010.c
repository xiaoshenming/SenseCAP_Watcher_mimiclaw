#include "spd2010.h"
#include "../hal_config.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "SPD2010";

/* SPD2010 命令定义 */
#define SPD2010_CMD_NOP         0x00
#define SPD2010_CMD_SWRESET     0x01
#define SPD2010_CMD_SLPIN       0x10
#define SPD2010_CMD_SLPOUT      0x11
#define SPD2010_CMD_INVOFF      0x20
#define SPD2010_CMD_INVON       0x21
#define SPD2010_CMD_DISPOFF     0x28
#define SPD2010_CMD_DISPON      0x29
#define SPD2010_CMD_CASET       0x2A
#define SPD2010_CMD_RASET       0x2B
#define SPD2010_CMD_RAMWR       0x2C
#define SPD2010_CMD_MADCTL      0x36
#define SPD2010_CMD_COLMOD      0x3A

/* QSPI 配置 */
#define QSPI_CLK_FREQ_HZ       (40 * 1000 * 1000)
#define QSPI_MAX_TRANSFER_SZ   (LCD_WIDTH * 40 * 2)
#define QSPI_CMD_BITS           8
#define QSPI_ADDR_BITS          24

static spi_device_handle_t s_spi_dev = NULL;
static bool s_initialized = false;

/* ------------------------------------------------------------------ */
/*  内部函数                                                           */
/* ------------------------------------------------------------------ */

static esp_err_t qspi_send_cmd(uint8_t cmd)
{
    spi_transaction_t t = {
        .cmd = 0x02,
        .addr = ((uint32_t)cmd) << 8,
        .flags = SPI_TRANS_MODE_QIO,
        .length = 0,
    };
    return spi_device_polling_transmit(s_spi_dev, &t);
}

static esp_err_t qspi_send_cmd_data(uint8_t cmd, const uint8_t *data, size_t len)
{
    if (len == 0) {
        return qspi_send_cmd(cmd);
    }

    spi_transaction_t t = {
        .cmd = 0x02,
        .addr = ((uint32_t)cmd) << 8,
        .flags = SPI_TRANS_MODE_QIO,
        .length = len * 8,
        .tx_buffer = data,
    };

    if (len <= 4) {
        t.flags |= SPI_TRANS_USE_TXDATA;
        memcpy(t.tx_data, data, len);
        t.tx_buffer = NULL;
    }

    return spi_device_polling_transmit(s_spi_dev, &t);
}

static esp_err_t qspi_write_pixels(const uint8_t *data, size_t len)
{
    size_t offset = 0;
    while (offset < len) {
        size_t chunk = len - offset;
        if (chunk > QSPI_MAX_TRANSFER_SZ) {
            chunk = QSPI_MAX_TRANSFER_SZ;
        }

        spi_transaction_t t = {
            .cmd = 0x32,
            .addr = ((uint32_t)SPD2010_CMD_RAMWR) << 8,
            .flags = SPI_TRANS_MODE_QIO,
            .length = chunk * 8,
            .tx_buffer = data + offset,
        };
        esp_err_t ret = spi_device_polling_transmit(s_spi_dev, &t);
        if (ret != ESP_OK) {
            return ret;
        }
        offset += chunk;
    }
    return ESP_OK;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

esp_err_t spd2010_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "SPD2010 already initialized");
        return ESP_OK;
    }

    esp_err_t ret;

    /* 配置 QSPI 总线 */
    spi_bus_config_t bus_cfg = {
        .sclk_io_num = GPIO_QSPI_PCLK,
        .data0_io_num = GPIO_QSPI_DATA0,
        .data1_io_num = GPIO_QSPI_DATA1,
        .data2_io_num = GPIO_QSPI_DATA2,
        .data3_io_num = GPIO_QSPI_DATA3,
        .max_transfer_sz = QSPI_MAX_TRANSFER_SZ,
        .flags = SPICOMMON_BUSFLAG_QUAD,
    };
    ret = spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init QSPI bus: %s", esp_err_to_name(ret));
        return ret;
    }

    /* 添加 SPI 设备 */
    spi_device_interface_config_t dev_cfg = {
        .clock_speed_hz = QSPI_CLK_FREQ_HZ,
        .mode = 0,
        .spics_io_num = GPIO_LCD_CS,
        .queue_size = 4,
        .command_bits = QSPI_CMD_BITS,
        .address_bits = QSPI_ADDR_BITS,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    ret = spi_bus_add_device(SPI3_HOST, &dev_cfg, &s_spi_dev);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %s", esp_err_to_name(ret));
        goto err_bus;
    }

    /* 配置背光 GPIO */
    gpio_config_t bl_cfg = {
        .pin_bit_mask = (1ULL << GPIO_LCD_BACKLIGHT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&bl_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config backlight GPIO: %s", esp_err_to_name(ret));
        goto err_dev;
    }
    gpio_set_level(GPIO_LCD_BACKLIGHT, 0);

    /* SPD2010 初始化序列 */

    /* 软件复位 */
    ret = qspi_send_cmd(SPD2010_CMD_SWRESET);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Software reset failed: %s", esp_err_to_name(ret));
        goto err_dev;
    }
    vTaskDelay(pdMS_TO_TICKS(120));

    /* 退出睡眠模式 */
    ret = qspi_send_cmd(SPD2010_CMD_SLPOUT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Sleep out failed: %s", esp_err_to_name(ret));
        goto err_dev;
    }
    vTaskDelay(pdMS_TO_TICKS(120));

    /* 像素格式: RGB565 (16-bit) */
    uint8_t colmod_param = 0x55;
    ret = qspi_send_cmd_data(SPD2010_CMD_COLMOD, &colmod_param, 1);
    if (ret != ESP_OK) goto err_dev;

    /* 显存访问控制 */
    uint8_t madctl_param = 0x00;
    ret = qspi_send_cmd_data(SPD2010_CMD_MADCTL, &madctl_param, 1);
    if (ret != ESP_OK) goto err_dev;

    /* 设置默认显示窗口为全屏 */
    ret = spd2010_set_window(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);
    if (ret != ESP_OK) goto err_dev;

    /* 反转显示 */
    ret = qspi_send_cmd(SPD2010_CMD_INVON);
    if (ret != ESP_OK) goto err_dev;

    /* 开启显示 */
    ret = qspi_send_cmd(SPD2010_CMD_DISPON);
    if (ret != ESP_OK) goto err_dev;

    vTaskDelay(pdMS_TO_TICKS(20));

    /* 打开背光 */
    gpio_set_level(GPIO_LCD_BACKLIGHT, 1);

    s_initialized = true;
    ESP_LOGI(TAG, "SPD2010 initialized (%dx%d, QSPI@%dMHz)",
             LCD_WIDTH, LCD_HEIGHT, QSPI_CLK_FREQ_HZ / 1000000);
    return ESP_OK;

err_dev:
    spi_bus_remove_device(s_spi_dev);
    s_spi_dev = NULL;
err_bus:
    spi_bus_free(SPI3_HOST);
    return ret;
}

esp_err_t spd2010_send_cmd(uint8_t cmd)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    return qspi_send_cmd(cmd);
}

esp_err_t spd2010_send_cmd_param(uint8_t cmd, const uint8_t *param, size_t param_len)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (param_len > 0 && !param) {
        return ESP_ERR_INVALID_ARG;
    }
    return qspi_send_cmd_data(cmd, param, param_len);
}

esp_err_t spd2010_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    esp_err_t ret;

    /* 列地址设置 */
    uint8_t caset_param[4] = {
        (uint8_t)(x0 >> 8), (uint8_t)(x0 & 0xFF),
        (uint8_t)(x1 >> 8), (uint8_t)(x1 & 0xFF),
    };
    ret = qspi_send_cmd_data(SPD2010_CMD_CASET, caset_param, sizeof(caset_param));
    if (ret != ESP_OK) return ret;

    /* 行地址设置 */
    uint8_t raset_param[4] = {
        (uint8_t)(y0 >> 8), (uint8_t)(y0 & 0xFF),
        (uint8_t)(y1 >> 8), (uint8_t)(y1 & 0xFF),
    };
    ret = qspi_send_cmd_data(SPD2010_CMD_RASET, raset_param, sizeof(raset_param));
    return ret;
}

esp_err_t spd2010_write_pixels(const uint16_t *data, size_t pixel_count)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (!data || pixel_count == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    return qspi_write_pixels((const uint8_t *)data, pixel_count * sizeof(uint16_t));
}
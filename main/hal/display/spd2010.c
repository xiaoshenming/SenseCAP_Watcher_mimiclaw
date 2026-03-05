#include "spd2010.h"
#include "../hal_config.h"
#include "driver/spi_common.h"
#include "driver/gpio.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_spd2010.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "SPD2010";

static esp_lcd_panel_io_handle_t s_io_handle = NULL;
static esp_lcd_panel_handle_t s_panel_handle = NULL;
static bool s_initialized = false;

esp_err_t spd2010_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "SPD2010 already initialized");
        return ESP_OK;
    }

    esp_err_t ret;

    /* QSPI bus config */
    spi_bus_config_t bus_cfg = SPD2010_PANEL_BUS_QSPI_CONFIG(
        GPIO_QSPI_PCLK, GPIO_QSPI_DATA0, GPIO_QSPI_DATA1,
        GPIO_QSPI_DATA2, GPIO_QSPI_DATA3, LCD_WIDTH * 40 * 2);
    bus_cfg.flags = SPICOMMON_BUSFLAG_QUAD;

    ret = spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus init failed: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Panel IO config */
    esp_lcd_panel_io_spi_config_t io_cfg = SPD2010_PANEL_IO_QSPI_CONFIG(GPIO_LCD_CS, NULL, NULL);
    io_cfg.pclk_hz = 40 * 1000 * 1000;

    ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_cfg, &s_io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel IO init failed: %s", esp_err_to_name(ret));
        goto err_bus;
    }

    /* Panel config */
    esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = -1,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
        .bits_per_pixel = 16,
    };

    spd2010_vendor_config_t vendor_cfg = {
        .init_cmds = NULL,
        .init_cmds_size = 0,
        .flags = {
            .use_qspi_interface = 1,
        },
    };
    panel_cfg.vendor_config = &vendor_cfg;

    ret = esp_lcd_new_panel_spd2010(s_io_handle, &panel_cfg, &s_panel_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Panel create failed: %s", esp_err_to_name(ret));
        goto err_io;
    }

    /* Init panel */
    ret = esp_lcd_panel_reset(s_panel_handle);
    if (ret != ESP_OK) goto err_panel;

    ret = esp_lcd_panel_init(s_panel_handle);
    if (ret != ESP_OK) goto err_panel;

    ret = esp_lcd_panel_invert_color(s_panel_handle, true);
    if (ret != ESP_OK) goto err_panel;

    ret = esp_lcd_panel_disp_on_off(s_panel_handle, true);
    if (ret != ESP_OK) goto err_panel;

    vTaskDelay(pdMS_TO_TICKS(20));

    /* Backlight GPIO */
    gpio_config_t bl_cfg = {
        .pin_bit_mask = (1ULL << GPIO_LCD_BACKLIGHT),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&bl_cfg);
    if (ret != ESP_OK) goto err_panel;

    gpio_set_level(GPIO_LCD_BACKLIGHT, 1);

    s_initialized = true;
    ESP_LOGI(TAG, "SPD2010 initialized (%dx%d)", LCD_WIDTH, LCD_HEIGHT);
    return ESP_OK;

err_panel:
    esp_lcd_panel_del(s_panel_handle);
    s_panel_handle = NULL;
err_io:
    esp_lcd_panel_io_del(s_io_handle);
    s_io_handle = NULL;
err_bus:
    spi_bus_free(SPI3_HOST);
    return ret;
}

esp_err_t spd2010_send_cmd(uint8_t cmd)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    return esp_lcd_panel_io_tx_param(s_io_handle, cmd, NULL, 0);
}

esp_err_t spd2010_send_cmd_param(uint8_t cmd, const uint8_t *param, size_t param_len)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    return esp_lcd_panel_io_tx_param(s_io_handle, cmd, param, param_len);
}

esp_err_t spd2010_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;

    uint8_t caset[4] = {x0 >> 8, x0 & 0xFF, x1 >> 8, x1 & 0xFF};
    uint8_t raset[4] = {y0 >> 8, y0 & 0xFF, y1 >> 8, y1 & 0xFF};

    esp_err_t ret = esp_lcd_panel_io_tx_param(s_io_handle, 0x2A, caset, 4);
    if (ret != ESP_OK) return ret;

    return esp_lcd_panel_io_tx_param(s_io_handle, 0x2B, raset, 4);
}

esp_err_t spd2010_write_pixels(const uint16_t *data, size_t pixel_count)
{
    if (!s_initialized) return ESP_ERR_INVALID_STATE;
    if (!data || pixel_count == 0) return ESP_ERR_INVALID_ARG;

    return esp_lcd_panel_io_tx_color(s_io_handle, 0x2C, data, pixel_count * 2);
}

void* spd2010_get_io_handle(void)
{
    return s_io_handle;
}

void* spd2010_get_panel_handle(void)
{
    return s_panel_handle;
}

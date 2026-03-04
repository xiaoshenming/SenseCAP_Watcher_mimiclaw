#include "hal_sd.h"
#include "../hal_config.h"
#include "../io_expander/hal_io_exp.h"
#include "esp_log.h"
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "sdmmc_cmd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "HAL_SD";

#define SD_MOUNT_POINT      "/sdcard"
#define SD_MAX_FREQ_KHZ     20000
#define SD_FORMAT_ON_FAIL   0

/* \u5168\u5c40\u72b6\u6001 */
static struct {
    sdmmc_card_t *card;
    sdspi_dev_handle_t spi_handle;
    bool mounted;
    bool bus_initialized;
} s_sd = {0};

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

esp_err_t hal_sd_init(void)
{
    if (s_sd.mounted) {
        ESP_LOGW(TAG, "SD card already mounted");
        return ESP_OK;
    }

    esp_err_t ret;

    /* \u542f\u7528 SD \u5361\u7535\u6e90 */
    ret = hal_io_exp_set_power(IO_EXP_PWR_SDCARD, true);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable SD power: %s", esp_err_to_name(ret));
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(50));

    /* \u68c0\u67e5 SD \u5361\u662f\u5426\u63d2\u5165 */
    if (!hal_sd_is_inserted()) {
        ESP_LOGW(TAG, "SD card not inserted");
        hal_io_exp_set_power(IO_EXP_PWR_SDCARD, false);
        return ESP_ERR_NOT_FOUND;
    }

    /* \u521d\u59cb\u5316 SPI \u603b\u7ebf */
    if (!s_sd.bus_initialized) {
        spi_bus_config_t bus_cfg = {
            .mosi_io_num = SPI2_MOSI,
            .miso_io_num = SPI2_MISO,
            .sclk_io_num = SPI2_SCLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4096,
        };
        ret = spi_bus_initialize(SPI2_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
        if (ret == ESP_ERR_INVALID_STATE) {
            /* SPI2 already initialized externally (by hal_init), reuse it */
            ESP_LOGI(TAG, "SPI2 bus already initialized, reusing");
            s_sd.bus_initialized = true;
        } else if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to init SPI bus: %s", esp_err_to_name(ret));
            goto err_power;
        } else {
            s_sd.bus_initialized = true;
        }
    }

    /* \u914d\u7f6e SD SPI \u8bbe\u5907 */
    sdspi_device_config_t slot_cfg = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_cfg.gpio_cs = GPIO_SD_CS;
    slot_cfg.host_id = SPI2_HOST;

    /* \u6302\u8f7d FAT \u6587\u4ef6\u7cfb\u7edf */
    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = SD_FORMAT_ON_FAIL,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.max_freq_khz = SD_MAX_FREQ_KHZ;

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot_cfg,
                                   &mount_cfg, &s_sd.card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount FAT filesystem on SD card");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SD card: %s", esp_err_to_name(ret));
        }
        goto err_bus;
    }

    s_sd.mounted = true;

    /* \u6253\u5370 SD \u5361\u4fe1\u606f */
    sdmmc_card_print_info(stdout, s_sd.card);
    ESP_LOGI(TAG, "SD card mounted at %s", SD_MOUNT_POINT);
    return ESP_OK;

err_bus:
    /* SPI \u603b\u7ebf\u4fdd\u7559\uff0c\u907f\u514d\u5f71\u54cd\u5176\u4ed6 SPI2 \u8bbe\u5907 */
err_power:
    hal_io_exp_set_power(IO_EXP_PWR_SDCARD, false);
    return ret;
}

esp_err_t hal_sd_deinit(void)
{
    if (!s_sd.mounted) {
        return ESP_OK;
    }

    /* \u5378\u8f7d\u6587\u4ef6\u7cfb\u7edf */
    esp_err_t ret = esp_vfs_fat_sdcard_unmount(SD_MOUNT_POINT, s_sd.card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to unmount SD card: %s", esp_err_to_name(ret));
        return ret;
    }
    s_sd.card = NULL;
    s_sd.mounted = false;

    /* \u91ca\u653e SPI \u603b\u7ebf */
    /* SPI2 bus is shared with SSCMA camera, do not free it here */

    /* \u5173\u95ed SD \u5361\u7535\u6e90 */
    hal_io_exp_set_power(IO_EXP_PWR_SDCARD, false);

    ESP_LOGI(TAG, "SD card unmounted and deinitialized");
    return ESP_OK;
}

bool hal_sd_is_mounted(void)
{
    return s_sd.mounted;
}

bool hal_sd_is_inserted(void)
{
    uint8_t state = 1;
    esp_err_t ret = hal_io_exp_read_input(HAL_INPUT_SD_GPIO_DET, &state);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read SD detect pin: %s", esp_err_to_name(ret));
        return false;
    }
    /* SD \u68c0\u6d4b\u5f15\u811a\u4f4e\u7535\u5e73\u8868\u793a\u5df2\u63d2\u5165 */
    return (state == 0);
}

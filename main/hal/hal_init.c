#include "hal_init.h"
#include "hal_config.h"
#include "io_expander/hal_io_exp.h"
#include "power/hal_power.h"
#include "display/hal_display.h"
#include "audio/hal_audio.h"
#include "touch/hal_touch.h"
#include "led/hal_led.h"
#include "input/hal_encoder.h"
#include "input/hal_button.h"
#include "sd_card/hal_sd.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "hal_init";

esp_err_t watcher_hal_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "========== HAL Init Start ==========");

    /* 1. IO Expander - must be first, controls power for everything else */
    ESP_LOGI(TAG, "[1/8] IO Expander (TCA9555)...");
    ret = hal_io_exp_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "IO Expander init FAILED: %s - aborting", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "[1/8] IO Expander OK");

    /* 2. Power management - enable peripheral power rails */
    ESP_LOGI(TAG, "[2/8] Power management...");
    ret = hal_power_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Power init FAILED: %s - aborting", esp_err_to_name(ret));
        return ret;
    }
    /* Enable critical power rails */
    hal_power_enable(HAL_PWR_SYSTEM, true);
    hal_power_enable(HAL_PWR_LCD, true);
    hal_power_enable(HAL_PWR_CODEC_PA, true);
    hal_power_enable(HAL_PWR_BAT_ADC, true);
    vTaskDelay(pdMS_TO_TICKS(50));  /* Let power stabilize */
    ESP_LOGI(TAG, "[2/8] Power OK");

    /* 3. Display */
    ESP_LOGI(TAG, "[3/8] Display (SPD2010 QSPI)...");
    ret = hal_display_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Display init FAILED: %s", esp_err_to_name(ret));
        /* Display failure is critical - return error */
        return ret;
    }
    ESP_LOGI(TAG, "[3/8] Display OK");

    /* 4. Audio */
    ESP_LOGI(TAG, "[4/8] Audio (ES8311 + ES7243E)...");
    ret = hal_audio_init(NULL);  /* Use default config */
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Audio init FAILED: %s - continuing", esp_err_to_name(ret));
        /* Audio failure is non-critical */
    } else {
        ESP_LOGI(TAG, "[4/8] Audio OK");
    }

    /* 5. Touch */
    ESP_LOGI(TAG, "[5/8] Touch (CST816S)...");
    ret = hal_touch_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Touch init FAILED: %s - continuing", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "[5/8] Touch OK");
    }

    /* 6. RGB LED */
    ESP_LOGI(TAG, "[6/8] RGB LED (WS2813)...");
    ret = hal_led_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "LED init FAILED: %s - continuing", esp_err_to_name(ret));
    } else {
        /* Brief green flash to indicate successful boot */
        hal_led_set_rgb(0, 32, 0);
        ESP_LOGI(TAG, "[6/8] LED OK");
    }

    /* 7. Input (encoder + buttons) */
    ESP_LOGI(TAG, "[7/8] Input (encoder + buttons)...");
    ret = hal_encoder_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Encoder init FAILED: %s - continuing", esp_err_to_name(ret));
    }
    ret = hal_button_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Button init FAILED: %s - continuing", esp_err_to_name(ret));
    }
    ESP_LOGI(TAG, "[7/8] Input OK");

    /* 8. SD Card (optional) */
    ESP_LOGI(TAG, "[8/8] SD Card...");
    if (hal_sd_is_inserted()) {
        ret = hal_sd_init();
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "SD Card init FAILED: %s - continuing", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "[8/8] SD Card OK");
        }
    } else {
        ESP_LOGI(TAG, "[8/8] SD Card not inserted, skipped");
    }

    ESP_LOGI(TAG, "========== HAL Init Complete ==========");

    /* Turn off boot indicator LED after 500ms */
    vTaskDelay(pdMS_TO_TICKS(500));
    hal_led_off();

    return ESP_OK;
}

esp_err_t watcher_hal_deinit(void)
{
    ESP_LOGI(TAG, "HAL deinit...");

    /* Reverse order */
    hal_sd_deinit();
    hal_button_deinit();
    hal_touch_deinit();
    hal_audio_deinit();
    hal_display_deinit();
    hal_led_off();
    hal_power_all_off();
    hal_io_exp_deinit();

    ESP_LOGI(TAG, "HAL deinit complete");
    return ESP_OK;
}
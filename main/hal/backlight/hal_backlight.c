#include "hal_backlight.h"
#include "hal/hal_config.h"
#include <driver/ledc.h>
#include <esp_log.h>

#define LEDC_TIMER              LEDC_TIMER_0
#define LEDC_MODE               LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL            LEDC_CHANNEL_0
#define LEDC_DUTY_RES           LEDC_TIMER_8_BIT
#define LEDC_FREQUENCY          5000

static const char *TAG = "hal_backlight";

esp_err_t hal_backlight_init(void) {
    ledc_timer_config_t timer_conf = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC timer config failed");
        return ret;
    }

    ledc_channel_config_t channel_conf = {
        .gpio_num = GPIO_LCD_BACKLIGHT,
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL,
        .timer_sel = LEDC_TIMER,
        .duty = 0,
        .hpoint = 0
    };
    ret = ledc_channel_config(&channel_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "LEDC channel config failed");
        return ret;
    }

    ESP_LOGI(TAG, "Backlight initialized");
    return ESP_OK;
}

esp_err_t hal_backlight_set_brightness(uint8_t brightness) {
    if (brightness > 100) {
        brightness = 100;
    }
    uint32_t duty = (brightness * 255) / 100;
    esp_err_t ret = ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, duty);
    if (ret != ESP_OK) {
        return ret;
    }
    return ledc_update_duty(LEDC_MODE, LEDC_CHANNEL);
}

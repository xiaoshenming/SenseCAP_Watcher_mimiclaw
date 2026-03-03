#include "hal_battery.h"
#include "hal/hal_config.h"
#include <esp_adc/adc_oneshot.h>
#include <esp_adc/adc_cali.h>
#include <esp_adc/adc_cali_scheme.h>
#include <esp_log.h>

static const char *TAG = "hal_battery";
static adc_oneshot_unit_handle_t adc_handle = NULL;
static adc_cali_handle_t cali_handle = NULL;

esp_err_t hal_battery_init(void) {
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    esp_err_t ret = adc_oneshot_new_unit(&init_config, &adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC unit init failed");
        return ret;
    }

    adc_oneshot_chan_cfg_t ch_config = {
        .atten = BAT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_oneshot_config_channel(adc_handle, BAT_ADC_CHANNEL, &ch_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC channel config failed");
        return ret;
    }

    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = BAT_ADC_CHANNEL,
        .atten = BAT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC calibration failed");
        return ret;
    }

    ESP_LOGI(TAG, "Battery ADC initialized");
    return ESP_OK;
}

float hal_battery_get_voltage(void) {
    if (!adc_handle || !cali_handle) {
        return 0.0f;
    }

    int raw_value = 0;
    int voltage = 0;
    adc_oneshot_read(adc_handle, BAT_ADC_CHANNEL, &raw_value);
    adc_cali_raw_to_voltage(cali_handle, raw_value, &voltage);

    return (float)voltage * BAT_VOLTAGE_DIVIDER;
}

uint8_t hal_battery_get_percent(void) {
    int voltage = 0;
    for (uint8_t i = 0; i < 10; i++) {
        voltage += (int)hal_battery_get_voltage();
    }
    voltage /= 10;

    int percent = (-1 * voltage * voltage + 9016 * voltage - 19189000) / 10000;
    if (percent > 100) percent = 100;
    if (percent < 0) percent = 0;

    return (uint8_t)percent;
}

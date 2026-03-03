#include "hal_power.h"
#include "../hal_config.h"
#include "../io_expander/hal_io_exp.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static const char *TAG = "HAL_POWER";

/* ------------------------------------------------------------------ */
/*  Mapping from hal_power_id_t  ->  TCA9555 Port 1 pin number        */
/* ------------------------------------------------------------------ */
static const uint8_t s_id_to_pin[HAL_PWR_MAX] = {
    [HAL_PWR_SYSTEM]   = IO_EXP_P1_PWR_SYSTEM,   /* P1.2 */
    [HAL_PWR_LCD]      = IO_EXP_P1_PWR_LCD,       /* P1.1 */
    [HAL_PWR_SDCARD]   = IO_EXP_P1_PWR_SDCARD,    /* P1.0 */
    [HAL_PWR_AI_CHIP]  = IO_EXP_P1_PWR_AI_CHIP,   /* P1.4 */
    [HAL_PWR_CODEC_PA] = IO_EXP_P1_PWR_CODEC_PA,  /* P1.5 */
    [HAL_PWR_BAT_DET]  = IO_EXP_P1_PWR_BAT_DET,   /* P1.6 */
    [HAL_PWR_GROVE]    = IO_EXP_P1_PWR_GROVE,      /* P1.7 */
    [HAL_PWR_BAT_ADC]  = IO_EXP_P1_PWR_BAT_ADC,   /* P1.8 */
};

/* ------------------------------------------------------------------ */
/*  Battery ADC state                                                  */
/* ------------------------------------------------------------------ */
static adc_oneshot_unit_handle_t s_adc_handle = NULL;
static adc_cali_handle_t        s_adc_cali   = NULL;
static bool                     s_initialized = false;

/* ------------------------------------------------------------------ */
/*  Battery voltage -> percentage lookup (linear interpolation)        */
/*  Based on typical single-cell Li-Ion discharge curve (3.0 – 4.2 V) */
/* ------------------------------------------------------------------ */
typedef struct {
    uint32_t mv;
    uint8_t  pct;
} bat_lut_entry_t;

static const bat_lut_entry_t s_bat_lut[] = {
    { 4200, 100 },
    { 4100,  90 },
    { 3950,  75 },
    { 3800,  55 },
    { 3700,  35 },
    { 3600,  20 },
    { 3500,  10 },
    { 3300,   5 },
    { 3000,   0 },
};
#define BAT_LUT_SIZE  (sizeof(s_bat_lut) / sizeof(s_bat_lut[0]))

static uint8_t mv_to_pct(uint32_t mv)
{
    if (mv >= s_bat_lut[0].mv) {
        return s_bat_lut[0].pct;
    }
    if (mv <= s_bat_lut[BAT_LUT_SIZE - 1].mv) {
        return s_bat_lut[BAT_LUT_SIZE - 1].pct;
    }

    for (size_t i = 0; i < BAT_LUT_SIZE - 1; i++) {
        if (mv >= s_bat_lut[i + 1].mv) {
            /* Linear interpolation between lut[i] and lut[i+1] */
            uint32_t dv = s_bat_lut[i].mv  - s_bat_lut[i + 1].mv;
            uint32_t dp = s_bat_lut[i].pct  - s_bat_lut[i + 1].pct;
            uint32_t offset = mv - s_bat_lut[i + 1].mv;
            return s_bat_lut[i + 1].pct + (uint8_t)((offset * dp) / dv);
        }
    }
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

esp_err_t hal_power_init(void)
{
    if (s_initialized) {
        ESP_LOGW(TAG, "Power HAL already initialized");
        return ESP_OK;
    }

    esp_err_t ret;

    /* ---- ADC oneshot unit ---- */
    adc_oneshot_unit_init_cfg_t adc_cfg = {
        .unit_id  = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ret = adc_oneshot_new_unit(&adc_cfg, &s_adc_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ADC unit: %s", esp_err_to_name(ret));
        return ret;
    }

    /* ---- ADC channel config (GPIO3 = ADC1_CHANNEL_2) ---- */
    adc_oneshot_chan_cfg_t chan_cfg = {
        .atten    = BAT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_oneshot_config_channel(s_adc_handle, BAT_ADC_CHANNEL, &chan_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to config ADC channel: %s", esp_err_to_name(ret));
        goto err_unit;
    }

    /* ---- ADC calibration ---- */
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .chan     = BAT_ADC_CHANNEL,
        .atten    = BAT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_adc_cali);
#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    adc_cali_line_fitting_config_t cali_cfg = {
        .unit_id  = ADC_UNIT_1,
        .atten    = BAT_ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ret = adc_cali_create_scheme_line_fitting(&cali_cfg, &s_adc_cali);
#else
    ESP_LOGW(TAG, "No ADC calibration scheme available");
    ret = ESP_ERR_NOT_SUPPORTED;
#endif
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "ADC calibration init failed (%s), raw readings will be used",
                 esp_err_to_name(ret));
        s_adc_cali = NULL;
        /* Non-fatal: we can still read raw values */
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Power HAL initialized");
    return ESP_OK;

err_unit:
    adc_oneshot_del_unit(s_adc_handle);
    s_adc_handle = NULL;
    return ret;
}

esp_err_t hal_power_enable(hal_power_id_t id, bool enable)
{
    if (id >= HAL_PWR_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    esp_err_t ret = hal_io_exp_set_power((hal_io_exp_power_pin_t)s_id_to_pin[id], enable);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to %s power %d: %s",
                 enable ? "enable" : "disable", id, esp_err_to_name(ret));
    } else {
        ESP_LOGD(TAG, "Power %d %s", id, enable ? "ON" : "OFF");
    }
    return ret;
}

esp_err_t hal_power_all_on(void)
{
    esp_err_t ret;
    for (int i = 0; i < HAL_PWR_MAX; i++) {
        ret = hal_power_enable((hal_power_id_t)i, true);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to enable power %d during all-on", i);
            return ret;
        }
    }
    ESP_LOGI(TAG, "All power rails enabled");
    return ESP_OK;
}

esp_err_t hal_power_all_off(void)
{
    esp_err_t ret;
    for (int i = 0; i < HAL_PWR_MAX; i++) {
        ret = hal_power_enable((hal_power_id_t)i, false);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to disable power %d during all-off", i);
            return ret;
        }
    }
    ESP_LOGI(TAG, "All power rails disabled");
    return ESP_OK;
}

uint32_t hal_power_get_battery_mv(void)
{
    if (!s_adc_handle) {
        ESP_LOGE(TAG, "ADC not initialized");
        return 0;
    }

    int raw = 0;
    esp_err_t ret = adc_oneshot_read(s_adc_handle, BAT_ADC_CHANNEL, &raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC read failed: %s", esp_err_to_name(ret));
        return 0;
    }

    int voltage_mv = 0;
    if (s_adc_cali) {
        ret = adc_cali_raw_to_voltage(s_adc_cali, raw, &voltage_mv);
        if (ret != ESP_OK) {
            ESP_LOGW(TAG, "ADC calibration conversion failed: %s", esp_err_to_name(ret));
            voltage_mv = 0;
        }
    } else {
        /* Rough estimation without calibration (2.5 dB attenuation ~ 1.25 V full scale) */
        voltage_mv = (raw * 1250) / 4095;
    }

    /* Apply voltage divider ratio to get actual battery voltage */
    uint32_t bat_mv = (uint32_t)(voltage_mv * BAT_VOLTAGE_DIVIDER);
    return bat_mv;
}

uint8_t hal_power_get_battery_pct(void)
{
    uint32_t mv = hal_power_get_battery_mv();
    return mv_to_pct(mv);
}

hal_charge_state_t hal_power_get_charge_state(void)
{
    uint8_t chrg_det = 1;
    uint8_t stdby_det = 1;
    uint8_t vbus_det = 1;

    hal_io_exp_read_input(HAL_INPUT_PWR_CHRG_DET, &chrg_det);
    hal_io_exp_read_input(HAL_INPUT_PWR_STDBY_DET, &stdby_det);
    hal_io_exp_read_input(HAL_INPUT_PWR_VBUS_IN_DET, &vbus_det);

    /*
     * Typical TP4056 charge controller logic (active-low outputs):
     *   CHRG=0, STDBY=1  ->  Charging
     *   CHRG=1, STDBY=0  ->  Standby (charge complete)
     *   CHRG=1, STDBY=1  ->  No external power / discharging
     *
     * VBUS_IN_DET: 0 = VBUS present, 1 = no VBUS
     */
    if (vbus_det) {
        /* No USB power connected */
        return HAL_CHARGE_NO_POWER;
    }

    if (!chrg_det && stdby_det) {
        return HAL_CHARGE_CHARGING;
    }

    if (chrg_det && !stdby_det) {
        return HAL_CHARGE_STANDBY;
    }

    return HAL_CHARGE_DISCHARGING;
}
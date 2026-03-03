#include "hal_led.h"
#include "hal/hal_config.h"
#include "esp_log.h"
#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include <string.h>

static const char *TAG = "hal_led";

// WS2813 时序参数 (单位: ns)
#define WS2813_T0H_NS    300
#define WS2813_T0L_NS    900
#define WS2813_T1H_NS    600
#define WS2813_T1L_NS    600
#define WS2813_RESET_NS  280000  // 280us reset

// RMT 分辨率: 10MHz => 100ns/tick
#define RMT_RESOLUTION_HZ  10000000

typedef struct {
    rmt_encoder_t base;
    rmt_encoder_t *bytes_encoder;
    rmt_encoder_t *copy_encoder;
    int state;
    rmt_symbol_word_t reset_code;
} ws2813_rmt_encoder_t;

static rmt_channel_handle_t s_led_chan = NULL;
static rmt_encoder_handle_t s_led_encoder = NULL;
static rmt_transmit_config_t s_tx_config = {
    .loop_count = 0,
};

static size_t ws2813_rmt_encode(rmt_encoder_t *encoder, rmt_channel_handle_t channel,
                                const void *primary_data, size_t data_size,
                                rmt_encode_state_t *ret_state)
{
    ws2813_rmt_encoder_t *ws2813_encoder = __containerof(encoder, ws2813_rmt_encoder_t, base);
    rmt_encode_state_t session_state = 0;
    rmt_encode_state_t state = 0;
    size_t encoded_symbols = 0;

    switch (ws2813_encoder->state) {
    case 0:
        encoded_symbols += ws2813_encoder->bytes_encoder->encode(
            ws2813_encoder->bytes_encoder, channel,
            primary_data, data_size, &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            ws2813_encoder->state = 1;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
        // fallthrough
    case 1:
        encoded_symbols += ws2813_encoder->copy_encoder->encode(
            ws2813_encoder->copy_encoder, channel,
            &ws2813_encoder->reset_code,
            sizeof(ws2813_encoder->reset_code), &session_state);
        if (session_state & RMT_ENCODING_COMPLETE) {
            ws2813_encoder->state = 0;
            state |= RMT_ENCODING_COMPLETE;
        }
        if (session_state & RMT_ENCODING_MEM_FULL) {
            state |= RMT_ENCODING_MEM_FULL;
            goto out;
        }
    }
out:
    *ret_state = state;
    return encoded_symbols;
}

static esp_err_t ws2813_rmt_del(rmt_encoder_t *encoder)
{
    ws2813_rmt_encoder_t *ws2813_encoder = __containerof(encoder, ws2813_rmt_encoder_t, base);
    rmt_del_encoder(ws2813_encoder->bytes_encoder);
    rmt_del_encoder(ws2813_encoder->copy_encoder);
    free(ws2813_encoder);
    return ESP_OK;
}

static esp_err_t ws2813_rmt_reset(rmt_encoder_t *encoder)
{
    ws2813_rmt_encoder_t *ws2813_encoder = __containerof(encoder, ws2813_rmt_encoder_t, base);
    rmt_encoder_reset(ws2813_encoder->bytes_encoder);
    rmt_encoder_reset(ws2813_encoder->copy_encoder);
    ws2813_encoder->state = 0;
    return ESP_OK;
}

static esp_err_t ws2813_new_encoder(rmt_encoder_handle_t *ret_encoder)
{
    esp_err_t ret = ESP_OK;
    ws2813_rmt_encoder_t *encoder = NULL;

    encoder = calloc(1, sizeof(ws2813_rmt_encoder_t));
    if (!encoder) {
        return ESP_ERR_NO_MEM;
    }

    encoder->base.encode = ws2813_rmt_encode;
    encoder->base.del = ws2813_rmt_del;
    encoder->base.reset = ws2813_rmt_reset;

    // WS2813 bit0: 0.3us high, 0.9us low
    // WS2813 bit1: 0.6us high, 0.6us low
    rmt_bytes_encoder_config_t bytes_encoder_config = {
        .bit0 = {
            .level0 = 1,
            .duration0 = WS2813_T0H_NS / (1000000000 / RMT_RESOLUTION_HZ),
            .level1 = 0,
            .duration1 = WS2813_T0L_NS / (1000000000 / RMT_RESOLUTION_HZ),
        },
        .bit1 = {
            .level0 = 1,
            .duration0 = WS2813_T1H_NS / (1000000000 / RMT_RESOLUTION_HZ),
            .level1 = 0,
            .duration1 = WS2813_T1L_NS / (1000000000 / RMT_RESOLUTION_HZ),
        },
        .flags.msb_first = 1,
    };
    ret = rmt_new_bytes_encoder(&bytes_encoder_config, &encoder->bytes_encoder);
    if (ret != ESP_OK) {
        goto err;
    }

    rmt_copy_encoder_config_t copy_encoder_config = {};
    ret = rmt_new_copy_encoder(&copy_encoder_config, &encoder->copy_encoder);
    if (ret != ESP_OK) {
        goto err;
    }

    uint32_t reset_ticks = WS2813_RESET_NS / (1000000000 / RMT_RESOLUTION_HZ);
    encoder->reset_code = (rmt_symbol_word_t) {
        .level0 = 0,
        .duration0 = reset_ticks,
        .level1 = 0,
        .duration1 = 0,
    };

    *ret_encoder = &encoder->base;
    return ESP_OK;

err:
    if (encoder) {
        if (encoder->bytes_encoder) {
            rmt_del_encoder(encoder->bytes_encoder);
        }
        if (encoder->copy_encoder) {
            rmt_del_encoder(encoder->copy_encoder);
        }
        free(encoder);
    }
    return ret;
}

static void hsv_to_rgb(uint16_t h, uint8_t s, uint8_t v, uint8_t *r, uint8_t *g, uint8_t *b)
{
    h = h % 360;
    uint8_t region = h / 60;
    uint8_t remainder = (h - (region * 60)) * 6;

    uint8_t p = (v * (100 - s)) / 100;
    uint8_t q = (v * (100 - ((s * remainder) / 360))) / 100;
    uint8_t t = (v * (100 - ((s * (360 - remainder)) / 360))) / 100;

    v = (v * 255) / 100;
    p = (p * 255) / 100;
    q = (q * 255) / 100;
    t = (t * 255) / 100;

    switch (region) {
    case 0:
        *r = v; *g = t; *b = p;
        break;
    case 1:
        *r = q; *g = v; *b = p;
        break;
    case 2:
        *r = p; *g = v; *b = t;
        break;
    case 3:
        *r = p; *g = q; *b = v;
        break;
    case 4:
        *r = t; *g = p; *b = v;
        break;
    default:
        *r = v; *g = p; *b = q;
        break;
    }
}

esp_err_t hal_led_init(void)
{
    esp_err_t ret;

    rmt_tx_channel_config_t tx_chan_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .gpio_num = GPIO_RGB_LED,
        .mem_block_symbols = 64,
        .resolution_hz = RMT_RESOLUTION_HZ,
        .trans_queue_depth = 4,
    };
    ret = rmt_new_tx_channel(&tx_chan_config, &s_led_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create RMT TX channel: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = ws2813_new_encoder(&s_led_encoder);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create WS2813 encoder: %s", esp_err_to_name(ret));
        rmt_del_channel(s_led_chan);
        return ret;
    }

    ret = rmt_enable(s_led_chan);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable RMT channel: %s", esp_err_to_name(ret));
        rmt_del_encoder(s_led_encoder);
        rmt_del_channel(s_led_chan);
        return ret;
    }

    ESP_LOGI(TAG, "WS2813 LED initialized on GPIO %d", GPIO_RGB_LED);
    return ESP_OK;
}

esp_err_t hal_led_set_rgb(uint8_t r, uint8_t g, uint8_t b)
{
    if (!s_led_chan || !s_led_encoder) {
        ESP_LOGE(TAG, "LED not initialized");
        return ESP_ERR_INVALID_STATE;
    }

    // WS2813 使用 GRB 格式
    uint8_t led_data[3] = {g, r, b};

    esp_err_t ret = rmt_transmit(s_led_chan, s_led_encoder, led_data, sizeof(led_data), &s_tx_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to transmit LED data: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = rmt_tx_wait_all_done(s_led_chan, 100);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to wait for transmission: %s", esp_err_to_name(ret));
        return ret;
    }

    return ESP_OK;
}

esp_err_t hal_led_set_hsv(uint16_t h, uint8_t s, uint8_t v)
{
    uint8_t r, g, b;
    hsv_to_rgb(h, s, v, &r, &g, &b);
    return hal_led_set_rgb(r, g, b);
}

esp_err_t hal_led_off(void)
{
    return hal_led_set_rgb(0, 0, 0);
}


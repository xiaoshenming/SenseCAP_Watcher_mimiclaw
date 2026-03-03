#include "hal_encoder.h"
#include "../hal_config.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "hal_encoder";

// 编码器状态
typedef struct {
    volatile int32_t count;           // 累计计数
    volatile uint8_t last_state;      // 上一次的 AB 状态 (bit1=A, bit0=B)
    volatile uint32_t last_time_us;   // 上次中断时间（用于防抖）
    hal_encoder_callback_t callback;  // 用户回调函数
    portMUX_TYPE spinlock;            // 自旋锁保护计数器
} encoder_state_t;

static encoder_state_t s_encoder = {
    .count = 0,
    .last_state = 0,
    .last_time_us = 0,
    .callback = NULL,
    .spinlock = portMUX_INITIALIZER_UNLOCKED
};

// 防抖时间（微秒）
#define DEBOUNCE_TIME_US 1000

// 正交编码器状态转换表
// 根据当前状态和新状态判断旋转方向
// 返回值: 1=顺时针, -1=逆时针, 0=无效/抖动
static int8_t get_direction(uint8_t old_state, uint8_t new_state) {
    // 状态编码: bit1=A相, bit0=B相
    // 正交编码器的格雷码序列:
    // 顺时针: 00 -> 01 -> 11 -> 10 -> 00
    // 逆时针: 00 -> 10 -> 11 -> 01 -> 00

    static const int8_t transition_table[4][4] = {
        //  00  01  10  11  (new_state)
        {   0,  1, -1,  0 },  // 00 (old_state)
        {  -1,  0,  0,  1 },  // 01
        {   1,  0,  0, -1 },  // 10
        {   0, -1,  1,  0 }   // 11
    };

    return transition_table[old_state & 0x03][new_state & 0x03];
}

// GPIO 中断处理函数
static void IRAM_ATTR encoder_isr_handler(void* arg) {
    uint32_t current_time_us = esp_timer_get_time();

    // 防抖：如果距离上次中断时间太短，忽略
    if ((current_time_us - s_encoder.last_time_us) < DEBOUNCE_TIME_US) {
        return;
    }

    // 读取当前 AB 相状态
    uint8_t a = gpio_get_level(GPIO_KNOB_A);
    uint8_t b = gpio_get_level(GPIO_KNOB_B);
    uint8_t new_state = (a << 1) | b;

    // 计算旋转方向
    int8_t direction = get_direction(s_encoder.last_state, new_state);

    if (direction != 0) {
        // 更新计数（使用自旋锁保护）
        portENTER_CRITICAL_ISR(&s_encoder.spinlock);
        s_encoder.count += direction;
        int32_t current_count = s_encoder.count;
        portEXIT_CRITICAL_ISR(&s_encoder.spinlock);

        // 调用用户回调
        if (s_encoder.callback != NULL) {
            s_encoder.callback(direction);
        }

        // 更新状态
        s_encoder.last_state = new_state;
        s_encoder.last_time_us = current_time_us;
    }
}

esp_err_t hal_encoder_init(void) {
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing rotary encoder on GPIO%d(A) and GPIO%d(B)",
             GPIO_KNOB_A, GPIO_KNOB_B);

    // 配置 A 相 GPIO
    gpio_config_t io_conf_a = {
        .pin_bit_mask = (1ULL << GPIO_KNOB_A),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE  // 双边沿触发
    };
    ret = gpio_config(&io_conf_a);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO%d: %s", GPIO_KNOB_A, esp_err_to_name(ret));
        return ret;
    }

    // 配置 B 相 GPIO
    gpio_config_t io_conf_b = {
        .pin_bit_mask = (1ULL << GPIO_KNOB_B),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE  // 双边沿触发
    };
    ret = gpio_config(&io_conf_b);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure GPIO%d: %s", GPIO_KNOB_B, esp_err_to_name(ret));
        return ret;
    }

    // 读取初始状态
    uint8_t a = gpio_get_level(GPIO_KNOB_A);
    uint8_t b = gpio_get_level(GPIO_KNOB_B);
    s_encoder.last_state = (a << 1) | b;
    s_encoder.count = 0;
    s_encoder.last_time_us = esp_timer_get_time();

    // 安装 GPIO 中断服务
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        // ESP_ERR_INVALID_STATE 表示 ISR 服务已安装，可以继续
        ESP_LOGE(TAG, "Failed to install ISR service: %s", esp_err_to_name(ret));
        return ret;
    }

    // 注册 A 相中断处理函数
    ret = gpio_isr_handler_add(GPIO_KNOB_A, encoder_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ISR handler for GPIO%d: %s", GPIO_KNOB_A, esp_err_to_name(ret));
        return ret;
    }

    // 注册 B 相中断处理函数
    ret = gpio_isr_handler_add(GPIO_KNOB_B, encoder_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add ISR handler for GPIO%d: %s", GPIO_KNOB_B, esp_err_to_name(ret));
        gpio_isr_handler_remove(GPIO_KNOB_A);
        return ret;
    }

    ESP_LOGI(TAG, "Rotary encoder initialized, initial state: A=%d B=%d", a, b);
    return ESP_OK;
}

int32_t hal_encoder_get_count(void) {
    int32_t count;
    portENTER_CRITICAL(&s_encoder.spinlock);
    count = s_encoder.count;
    portEXIT_CRITICAL(&s_encoder.spinlock);
    return count;
}

void hal_encoder_reset_count(void) {
    portENTER_CRITICAL(&s_encoder.spinlock);
    s_encoder.count = 0;
    portEXIT_CRITICAL(&s_encoder.spinlock);
}

esp_err_t hal_encoder_register_callback(hal_encoder_callback_t cb) {
    portENTER_CRITICAL(&s_encoder.spinlock);
    s_encoder.callback = cb;
    portEXIT_CRITICAL(&s_encoder.spinlock);
    ESP_LOGI(TAG, "Encoder callback %s", cb ? "registered" : "unregistered");
    return ESP_OK;
}

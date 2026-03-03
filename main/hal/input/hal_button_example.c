/**
 * @file hal_button_example.c
 * @brief 按键模块使用示例
 *
 * 此文件展示如何使用按键 HAL 模块
 */

#include "hal_button.h"
#include "esp_log.h"

static const char *TAG = "button_example";

/**
 * @brief 按键事件回调函数
 */
static void button_event_callback(hal_button_id_t id, hal_button_event_t event)
{
    const char *button_name = (id == HAL_BUTTON_BOOT) ? "BOOT" : "KNOB";

    switch (event) {
        case HAL_BUTTON_EVENT_PRESS:
            ESP_LOGI(TAG, "[%s] Button pressed", button_name);
            // 在这里处理按键按下事件
            break;

        case HAL_BUTTON_EVENT_RELEASE:
            ESP_LOGI(TAG, "[%s] Button released", button_name);
            // 在这里处理按键释放事件
            break;

        case HAL_BUTTON_EVENT_LONG_PRESS:
            ESP_LOGI(TAG, "[%s] Button long pressed", button_name);
            // 在这里处理长按事件
            break;

        default:
            break;
    }
}

/**
 * @brief 按键模块使用示例
 */
void hal_button_example(void)
{
    esp_err_t ret;

    // 1. 初始化按键模块
    ret = hal_button_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize button module");
        return;
    }

    // 2. 注册回调函数
    ret = hal_button_register_callback(button_event_callback);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register button callback");
        return;
    }

    ESP_LOGI(TAG, "Button module example started");
    ESP_LOGI(TAG, "Press BOOT button to test");

    // 3. 主循环中可以查询按键状态
    while (1) {
        bool boot_pressed = false;
        hal_button_get_state(HAL_BUTTON_BOOT, &boot_pressed);

        // 这里可以根据按键状态执行其他操作

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

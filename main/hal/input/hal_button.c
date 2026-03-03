#include "hal_button.h"
#include "../hal_config.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

static const char *TAG = "hal_button";

// 按键配置
#define BUTTON_DEBOUNCE_MS      50      // 防抖时间 (ms)
#define BUTTON_LONG_PRESS_MS    1000    // 长按时间 (ms)

// 按键状态结构
typedef struct {
    hal_button_id_t id;
    bool current_state;         // 当前物理状态 (true=按下)
    bool last_stable_state;     // 上次稳定状态
    int64_t press_time;         // 按下时间戳
    int64_t last_change_time;   // 最后状态变化时间
    bool long_press_sent;       // 是否已发送长按事件
} button_state_t;

// 按键事件队列项
typedef struct {
    hal_button_id_t id;
    hal_button_event_t event;
} button_event_t;

// 全局变量
static button_state_t s_buttons[HAL_BUTTON_MAX];
static hal_button_callback_t s_callback = NULL;
static QueueHandle_t s_event_queue = NULL;
static TaskHandle_t s_task_handle = NULL;
static esp_timer_handle_t s_debounce_timer = NULL;

// 前向声明
static void button_task(void *arg);
static void debounce_timer_callback(void *arg);
static void IRAM_ATTR boot_button_isr_handler(void *arg);
static esp_err_t read_button_physical_state(hal_button_id_t id, bool *pressed);

/**
 * @brief 读取按键物理状态
 */
static esp_err_t read_button_physical_state(hal_button_id_t id, bool *pressed)
{
    if (pressed == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    switch (id) {
        case HAL_BUTTON_BOOT:
            // BOOT 按键低电平有效
            *pressed = (gpio_get_level(GPIO_BOOT_BUTTON) == 0);
            return ESP_OK;

        case HAL_BUTTON_KNOB:
            // TODO: 通过 IO 扩展器读取旋钮按键状态
            // 暂时返回未按下状态
            *pressed = false;
            ESP_LOGW(TAG, "Knob button not implemented yet");
            return ESP_ERR_NOT_SUPPORTED;

        default:
            return ESP_ERR_INVALID_ARG;
    }
}

/**
 * @brief BOOT 按键 GPIO 中断处理函数
 */
static void IRAM_ATTR boot_button_isr_handler(void *arg)
{
    // 启动防抖定时器
    if (s_debounce_timer != NULL) {
        esp_timer_start_once(s_debounce_timer, BUTTON_DEBOUNCE_MS * 1000);
    }
}

/**
 * @brief 防抖定时器回调
 */
static void debounce_timer_callback(void *arg)
{
    // 读取所有按键状态并更新
    for (int i = 0; i < HAL_BUTTON_MAX; i++) {
        bool current_state = false;
        if (read_button_physical_state(i, &current_state) == ESP_OK) {
            button_state_t *btn = &s_buttons[i];
            int64_t now = esp_timer_get_time();

            // 检查状态是否变化
            if (current_state != btn->current_state) {
                btn->current_state = current_state;
                btn->last_change_time = now;
            }

            // 防抖: 状态稳定超过防抖时间
            if ((now - btn->last_change_time) >= (BUTTON_DEBOUNCE_MS * 1000)) {
                if (current_state != btn->last_stable_state) {
                    btn->last_stable_state = current_state;

                    if (current_state) {
                        // 按键按下
                        btn->press_time = now;
                        btn->long_press_sent = false;

                        button_event_t event = {
                            .id = i,
                            .event = HAL_BUTTON_EVENT_PRESS
                        };
                        xQueueSend(s_event_queue, &event, 0);
                    } else {
                        // 按键释放
                        button_event_t event = {
                            .id = i,
                            .event = HAL_BUTTON_EVENT_RELEASE
                        };
                        xQueueSend(s_event_queue, &event, 0);
                    }
                }
            }

            // 检查长按
            if (btn->last_stable_state && !btn->long_press_sent) {
                if ((now - btn->press_time) >= (BUTTON_LONG_PRESS_MS * 1000)) {
                    btn->long_press_sent = true;

                    button_event_t event = {
                        .id = i,
                        .event = HAL_BUTTON_EVENT_LONG_PRESS
                    };
                    xQueueSend(s_event_queue, &event, 0);
                }
            }
        }
    }
}

/**
 * @brief 按键事件处理任务
 */
static void button_task(void *arg)
{
    button_event_t event;

    while (1) {
        // 等待事件
        if (xQueueReceive(s_event_queue, &event, portMAX_DELAY) == pdTRUE) {
            // 调用回调函数
            if (s_callback != NULL) {
                s_callback(event.id, event.event);
            }

            // 打印日志
            const char *button_name = (event.id == HAL_BUTTON_BOOT) ? "BOOT" : "KNOB";
            const char *event_name;
            switch (event.event) {
                case HAL_BUTTON_EVENT_PRESS:
                    event_name = "PRESS";
                    break;
                case HAL_BUTTON_EVENT_RELEASE:
                    event_name = "RELEASE";
                    break;
                case HAL_BUTTON_EVENT_LONG_PRESS:
                    event_name = "LONG_PRESS";
                    break;
                default:
                    event_name = "UNKNOWN";
                    break;
            }
            ESP_LOGI(TAG, "Button %s: %s", button_name, event_name);
        }

        // 周期性检查长按状态
        vTaskDelay(pdMS_TO_TICKS(100));
        debounce_timer_callback(NULL);
    }
}

/**
 * @brief 初始化按键模块
 */
esp_err_t hal_button_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing button module");

    // 初始化按键状态
    memset(s_buttons, 0, sizeof(s_buttons));
    for (int i = 0; i < HAL_BUTTON_MAX; i++) {
        s_buttons[i].id = i;
    }

    // 创建事件队列
    s_event_queue = xQueueCreate(10, sizeof(button_event_t));
    if (s_event_queue == NULL) {
        ESP_LOGE(TAG, "Failed to create event queue");
        return ESP_ERR_NO_MEM;
    }

    // 配置 BOOT 按键 GPIO
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_BOOT_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    ret = gpio_config(&io_conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure BOOT button GPIO");
        goto cleanup;
    }

    // 安装 GPIO ISR 服务
    ret = gpio_install_isr_service(0);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service");
        goto cleanup;
    }

    // 添加 BOOT 按键中断处理函数
    ret = gpio_isr_handler_add(GPIO_BOOT_BUTTON, boot_button_isr_handler, NULL);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add BOOT button ISR handler");
        goto cleanup;
    }

    // 创建防抖定时器
    esp_timer_create_args_t timer_args = {
        .callback = debounce_timer_callback,
        .arg = NULL,
        .dispatch_method = ESP_TIMER_TASK,
        .name = "button_debounce"
    };
    ret = esp_timer_create(&timer_args, &s_debounce_timer);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create debounce timer");
        goto cleanup;
    }

    // 创建按键处理任务
    BaseType_t task_ret = xTaskCreate(
        button_task,
        "button_task",
        4096,
        NULL,
        5,
        &s_task_handle
    );
    if (task_ret != pdPASS) {
        ESP_LOGE(TAG, "Failed to create button task");
        ret = ESP_ERR_NO_MEM;
        goto cleanup;
    }

    // 读取初始状态
    for (int i = 0; i < HAL_BUTTON_MAX; i++) {
        bool pressed = false;
        if (read_button_physical_state(i, &pressed) == ESP_OK) {
            s_buttons[i].current_state = pressed;
            s_buttons[i].last_stable_state = pressed;
        }
    }

    ESP_LOGI(TAG, "Button module initialized successfully");
    return ESP_OK;

cleanup:
    hal_button_deinit();
    return ret;
}

/**
 * @brief 注册按键事件回调函数
 */
esp_err_t hal_button_register_callback(hal_button_callback_t cb)
{
    if (cb == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    s_callback = cb;
    ESP_LOGI(TAG, "Button callback registered");
    return ESP_OK;
}

/**
 * @brief 读取按键当前状态
 */
esp_err_t hal_button_get_state(hal_button_id_t id, bool *pressed)
{
    if (id >= HAL_BUTTON_MAX || pressed == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    *pressed = s_buttons[id].last_stable_state;
    return ESP_OK;
}

/**
 * @brief 反初始化按键模块
 */
esp_err_t hal_button_deinit(void)
{
    ESP_LOGI(TAG, "Deinitializing button module");

    // 删除任务
    if (s_task_handle != NULL) {
        vTaskDelete(s_task_handle);
        s_task_handle = NULL;
    }

    // 删除定时器
    if (s_debounce_timer != NULL) {
        esp_timer_stop(s_debounce_timer);
        esp_timer_delete(s_debounce_timer);
        s_debounce_timer = NULL;
    }

    // 移除中断处理函数
    gpio_isr_handler_remove(GPIO_BOOT_BUTTON);

    // 删除队列
    if (s_event_queue != NULL) {
        vQueueDelete(s_event_queue);
        s_event_queue = NULL;
    }

    // 清空回调
    s_callback = NULL;

    ESP_LOGI(TAG, "Button module deinitialized");
    return ESP_OK;
}

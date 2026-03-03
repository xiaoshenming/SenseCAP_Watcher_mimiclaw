#ifndef HAL_BUTTON_H
#define HAL_BUTTON_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 按键 ID 枚举
 */
typedef enum {
    HAL_BUTTON_BOOT,    ///< BOOT 按键 (GPIO0)
    HAL_BUTTON_KNOB,    ///< 旋钮按键 (IO 扩展器 P0.3)
    HAL_BUTTON_MAX
} hal_button_id_t;

/**
 * @brief 按键事件类型
 */
typedef enum {
    HAL_BUTTON_EVENT_PRESS,       ///< 按键按下
    HAL_BUTTON_EVENT_RELEASE,     ///< 按键释放
    HAL_BUTTON_EVENT_LONG_PRESS   ///< 长按事件
} hal_button_event_t;

/**
 * @brief 按键事件回调函数类型
 *
 * @param id 按键 ID
 * @param event 事件类型
 */
typedef void (*hal_button_callback_t)(hal_button_id_t id, hal_button_event_t event);

/**
 * @brief 初始化按键模块
 *
 * @return
 *     - ESP_OK: 成功
 *     - ESP_FAIL: 失败
 */
esp_err_t hal_button_init(void);

/**
 * @brief 注册按键事件回调函数
 *
 * @param cb 回调函数指针
 * @return
 *     - ESP_OK: 成功
 *     - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t hal_button_register_callback(hal_button_callback_t cb);

/**
 * @brief 读取按键当前状态
 *
 * @param id 按键 ID
 * @param pressed 输出参数,true 表示按下,false 表示释放
 * @return
 *     - ESP_OK: 成功
 *     - ESP_ERR_INVALID_ARG: 参数无效
 */
esp_err_t hal_button_get_state(hal_button_id_t id, bool *pressed);

/**
 * @brief 反初始化按键模块
 *
 * @return
 *     - ESP_OK: 成功
 *     - ESP_FAIL: 失败
 */
esp_err_t hal_button_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // HAL_BUTTON_H

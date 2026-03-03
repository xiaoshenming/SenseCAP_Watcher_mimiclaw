#ifndef HAL_ENCODER_H
#define HAL_ENCODER_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief 旋转编码器回调函数类型
 *
 * @param delta 旋转增量，正值表示顺时针，负值表示逆时针
 */
typedef void (*hal_encoder_callback_t)(int32_t delta);

/**
 * @brief 初始化旋转编码器
 *
 * 配置 GPIO41(A相) 和 GPIO42(B相) 为输入模式，
 * 启用双边沿触发中断，初始化正交编码器状态机
 *
 * @return esp_err_t ESP_OK 成功, 其他值表示失败
 */
esp_err_t hal_encoder_init(void);

/**
 * @brief 获取编码器累计计数
 *
 * @return int32_t 当前累计计数值（线程安全）
 */
int32_t hal_encoder_get_count(void);

/**
 * @brief 重置编码器计数为 0
 */
void hal_encoder_reset_count(void);

/**
 * @brief 注册旋转回调函数
 *
 * 当检测到旋转时，会在中断上下文中调用回调函数
 *
 * @param cb 回调函数指针，传入 NULL 取消注册
 * @return esp_err_t ESP_OK 成功, 其他值表示失败
 */
esp_err_t hal_encoder_register_callback(hal_encoder_callback_t cb);

#endif // HAL_ENCODER_H

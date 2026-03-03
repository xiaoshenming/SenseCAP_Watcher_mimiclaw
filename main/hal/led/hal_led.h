#ifndef HAL_LED_H
#define HAL_LED_H

#include "esp_err.h"
#include <stdint.h>

/**
 * @brief 初始化 WS2813 RGB LED
 *
 * @return esp_err_t ESP_OK 成功, 其他值表示失败
 */
esp_err_t hal_led_init(void);

/**
 * @brief 设置 LED RGB 颜色
 *
 * @param r 红色分量 (0-255)
 * @param g 绿色分量 (0-255)
 * @param b 蓝色分量 (0-255)
 * @return esp_err_t ESP_OK 成功, 其他值表示失败
 */
esp_err_t hal_led_set_rgb(uint8_t r, uint8_t g, uint8_t b);

/**
 * @brief 设置 LED HSV 颜色
 *
 * @param h 色调 (0-359)
 * @param s 饱和度 (0-100)
 * @param v 明度 (0-100)
 * @return esp_err_t ESP_OK 成功, 其他值表示失败
 */
esp_err_t hal_led_set_hsv(uint16_t h, uint8_t s, uint8_t v);

/**
 * @brief 关闭 LED
 *
 * @return esp_err_t ESP_OK 成功, 其他值表示失败
 */
esp_err_t hal_led_off(void);

#endif // HAL_LED_H

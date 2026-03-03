#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>

/**
 * 初始化 SPD2010 显示屏驱动 IC
 * - 配置 QSPI 总线
 * - 发送初始化命令序列
 * - 配置显示参数 (412x412, RGB565)
 *
 * @return ESP_OK 成功
 */
esp_err_t spd2010_init(void);

/**
 * 发送命令 (不带参数)
 *
 * @param cmd 命令字节
 * @return ESP_OK 成功
 */
esp_err_t spd2010_send_cmd(uint8_t cmd);

/**
 * 发送命令 (带参数)
 *
 * @param cmd 命令字节
 * @param param 参数数组
 * @param param_len 参数长度
 * @return ESP_OK 成功
 */
esp_err_t spd2010_send_cmd_param(uint8_t cmd, const uint8_t *param, size_t param_len);

/**
 * 设置显示窗口区域
 *
 * @param x0 起始列
 * @param y0 起始行
 * @param x1 结束列
 * @param y1 结束行
 * @return ESP_OK 成功
 */
esp_err_t spd2010_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);

/**
 * 写入像素数据到显示屏
 *
 * @param data RGB565 像素数据
 * @param pixel_count 像素数量
 * @return ESP_OK 成功
 */
esp_err_t spd2010_write_pixels(const uint16_t *data, size_t pixel_count);
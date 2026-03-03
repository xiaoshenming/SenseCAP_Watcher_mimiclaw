#ifndef HAL_SD_H
#define HAL_SD_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief 初始化 SD 卡
 *
 * 配置 SPI2 总线并挂载 FAT 文件系统到 /sdcard
 * 使用前会先启用 SD 卡电源
 *
 * @return esp_err_t ESP_OK 成功, 其他值表示失败
 */
esp_err_t hal_sd_init(void);

/**
 * @brief 卸载 SD 卡并释放资源
 *
 * @return esp_err_t ESP_OK 成功, 其他值表示失败
 */
esp_err_t hal_sd_deinit(void);

/**
 * @brief 检查 SD 卡是否已挂载
 *
 * @return true SD 卡已挂载
 * @return false SD 卡未挂载
 */
bool hal_sd_is_mounted(void);

/**
 * @brief 检查 SD 卡是否插入
 *
 * 通过 IO 扩展器 P0.4 检测 SD 卡插入状态
 *
 * @return true SD 卡已插入
 * @return false SD 卡未插入
 */
bool hal_sd_is_inserted(void);

#endif // HAL_SD_H

#pragma once

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>

/**
 * \u521d\u59cb\u5316\u663e\u793a\u5c4f HAL
 * - \u542f\u7528 LCD \u7535\u6e90
 * - \u521d\u59cb\u5316 SPD2010 \u9a71\u52a8
 * - \u914d\u7f6e\u80cc\u5149
 *
 * @return ESP_OK \u6210\u529f
 */
esp_err_t hal_display_init(void);

/**
 * \u53cd\u521d\u59cb\u5316\u663e\u793a\u5c4f HAL
 *
 * @return ESP_OK \u6210\u529f
 */
esp_err_t hal_display_deinit(void);

/**
 * \u586b\u5145\u6574\u4e2a\u5c4f\u5e55\u4e3a\u6307\u5b9a\u989c\u8272 (RGB565)
 *
 * @param color RGB565 \u989c\u8272\u503c
 * @return ESP_OK \u6210\u529f
 */
esp_err_t hal_display_fill(uint16_t color);

/**
 * \u586b\u5145\u6307\u5b9a\u533a\u57df\u4e3a\u6307\u5b9a\u989c\u8272 (RGB565)
 *
 * @param x0 \u8d77\u59cb\u5217
 * @param y0 \u8d77\u59cb\u884c
 * @param x1 \u7ed3\u675f\u5217
 * @param y1 \u7ed3\u675f\u884c
 * @param color RGB565 \u989c\u8272\u503c
 * @return ESP_OK \u6210\u529f
 */
esp_err_t hal_display_fill_area(uint16_t x0, uint16_t y0,
                                 uint16_t x1, uint16_t y1,
                                 uint16_t color);

/**
 * \u5411\u6307\u5b9a\u533a\u57df\u5199\u5165\u50cf\u7d20\u6570\u636e
 *
 * @param x0 \u8d77\u59cb\u5217
 * @param y0 \u8d77\u59cb\u884c
 * @param x1 \u7ed3\u675f\u5217
 * @param y1 \u7ed3\u675f\u884c
 * @param data RGB565 \u50cf\u7d20\u6570\u636e
 * @param pixel_count \u50cf\u7d20\u6570\u91cf
 * @return ESP_OK \u6210\u529f
 */
esp_err_t hal_display_draw(uint16_t x0, uint16_t y0,
                            uint16_t x1, uint16_t y1,
                            const uint16_t *data, size_t pixel_count);

/**
 * \u8bbe\u7f6e\u80cc\u5149\u5f00\u5173
 *
 * @param on true=\u80cc\u5149\u5f00, false=\u80cc\u5149\u5173
 * @return ESP_OK \u6210\u529f
 */
esp_err_t hal_display_set_backlight(bool on);

/**
 * \u8fdb\u5165\u7761\u7720\u6a21\u5f0f (\u4f4e\u529f\u8017)
 *
 * @return ESP_OK \u6210\u529f
 */
esp_err_t hal_display_sleep(void);

/**
 * \u9000\u51fa\u7761\u7720\u6a21\u5f0f
 *
 * @return ESP_OK \u6210\u529f
 */
esp_err_t hal_display_wakeup(void);

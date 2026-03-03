# HAL 按键模块

## 概述

按键处理模块提供了统一的按键事件检测接口,支持:
- BOOT 按键 (GPIO0)
- 旋钮按键 (IO 扩展器 P0.3)

## 功能特性

- **防抖处理**: 软件防抖 50ms,消除机械抖动
- **长按检测**: 1000ms 长按阈值
- **事件驱动**: 基于回调的异步事件通知
- **中断驱动**: BOOT 按键使用 GPIO 中断,响应快速
- **状态查询**: 支持主动查询按键状态

## API 接口

### 初始化

```c
esp_err_t hal_button_init(void);
```

初始化按键模块,配置 GPIO 和中断。

### 注册回调

```c
typedef void (*hal_button_callback_t)(hal_button_id_t id, hal_button_event_t event);
esp_err_t hal_button_register_callback(hal_button_callback_t cb);
```

注册按键事件回调函数,当按键事件发生时会调用此函数。

### 查询状态

```c
esp_err_t hal_button_get_state(hal_button_id_t id, bool *pressed);
```

主动查询按键当前状态。

### 反初始化

```c
esp_err_t hal_button_deinit(void);
```

释放按键模块资源。

## 使用示例

```c
#include "hal_button.h"

// 按键事件回调
void button_callback(hal_button_id_t id, hal_button_event_t event)
{
    if (id == HAL_BUTTON_BOOT) {
        switch (event) {
            case HAL_BUTTON_EVENT_PRESS:
                printf("BOOT button pressed\n");
                break;
            case HAL_BUTTON_EVENT_RELEASE:
                printf("BOOT button released\n");
                break;
            case HAL_BUTTON_EVENT_LONG_PRESS:
                printf("BOOT button long pressed\n");
                break;
        }
    }
}

void app_main(void)
{
    // 初始化按键模块
    hal_button_init();

    // 注册回调
    hal_button_register_callback(button_callback);

    // 主循环
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
```

## 事件类型

- `HAL_BUTTON_EVENT_PRESS`: 按键按下
- `HAL_BUTTON_EVENT_RELEASE`: 按键释放
- `HAL_BUTTON_EVENT_LONG_PRESS`: 长按 (1000ms)

## 按键 ID

- `HAL_BUTTON_BOOT`: BOOT 按键 (GPIO0)
- `HAL_BUTTON_KNOB`: 旋钮按键 (IO 扩展器 P0.3)

## 技术细节

### 防抖机制

使用软件防抖,状态变化后需要稳定 50ms 才会触发事件,有效消除机械抖动。

### 长按检测

按键按下后,如果持续 1000ms 仍未释放,会触发长按事件。长按事件只触发一次。

### 中断处理

BOOT 按键使用 GPIO 边沿中断,任何状态变化都会触发中断,启动防抖定时器。

### 旋钮按键

旋钮按键通过 IO 扩展器读取,需要先实现 IO 扩展器驱动。当前版本返回 `ESP_ERR_NOT_SUPPORTED`。

## 依赖

- ESP-IDF driver/gpio
- ESP-IDF esp_timer
- FreeRTOS

## 注意事项

1. 必须先调用 `hal_button_init()` 才能使用其他 API
2. 回调函数在独立任务中执行,优先级为 5
3. 旋钮按键功能需要 IO 扩展器驱动支持
4. BOOT 按键低电平有效,内部上拉

## 未来改进

- [ ] 实现旋钮按键支持 (需要 IO 扩展器驱动)
- [ ] 支持双击检测
- [ ] 支持组合按键
- [ ] 可配置的防抖和长按时间

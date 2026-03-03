# 按键处理模块实现完成

## 实现内容

已完成按键处理模块 (`main/hal/input/hal_button.c/h`) 的实现,包含以下功能:

### 文件列表

1. **hal_button.h** - 按键模块头文件
   - 定义按键 ID 枚举 (BOOT, KNOB)
   - 定义事件类型 (PRESS, RELEASE, LONG_PRESS)
   - 声明公共 API 接口

2. **hal_button.c** - 按键模块实现
   - BOOT 按键 GPIO 中断处理
   - 软件防抖 (50ms)
   - 长按检测 (1000ms)
   - 事件队列和回调机制
   - 旋钮按键接口 (预留,待 IO 扩展器实现)

3. **hal_button_example.c** - 使用示例
   - 展示如何初始化和使用按键模块

4. **CMakeLists.txt** - 编译配置
   - 配置源文件和依赖

5. **README.md** - 模块文档
   - API 说明
   - 使用示例
   - 技术细节

## 功能特性

### 已实现

✅ BOOT 按键 (GPIO0) 完整支持
- GPIO 中断驱动
- 边沿触发检测
- 内部上拉配置

✅ 防抖处理
- 软件防抖 50ms
- 状态稳定后才触发事件

✅ 长按检测
- 1000ms 长按阈值
- 长按事件只触发一次

✅ 事件驱动架构
- 异步回调通知
- 独立任务处理事件
- 事件队列缓冲

✅ 状态查询
- 支持主动查询按键状态
- 返回稳定后的状态

### 待实现

⏳ 旋钮按键 (IO 扩展器 P0.3)
- 需要先实现 TCA9555 IO 扩展器驱动
- 当前返回 `ESP_ERR_NOT_SUPPORTED`
- 接口已预留,实现 IO 扩展器后可直接集成

## API 接口

```c
// 初始化
esp_err_t hal_button_init(void);

// 注册回调
typedef void (*hal_button_callback_t)(hal_button_id_t id, hal_button_event_t event);
esp_err_t hal_button_register_callback(hal_button_callback_t cb);

// 查询状态
esp_err_t hal_button_get_state(hal_button_id_t id, bool *pressed);

// 反初始化
esp_err_t hal_button_deinit(void);
```

## 技术实现

### 中断处理流程

1. GPIO 边沿中断触发
2. ISR 启动防抖定时器 (50ms)
3. 定时器回调读取物理状态
4. 检测状态变化并防抖
5. 发送事件到队列
6. 任务从队列取事件并调用回调

### 长按检测

- 按键按下时记录时间戳
- 周期性检查按下时长
- 超过 1000ms 触发长按事件
- 使用标志位防止重复触发

### 防抖机制

- 状态变化后记录时间
- 只有稳定超过 50ms 才认为有效
- 消除机械抖动干扰

## 依赖关系

- `driver/gpio` - GPIO 配置和中断
- `esp_timer` - 高精度定时器
- `freertos/FreeRTOS` - 任务和队列
- `hal_config.h` - 引脚定义

## 集成说明

### 与 IO 扩展器集成

旋钮按键需要通过 IO 扩展器读取,在 `read_button_physical_state()` 函数中预留了接口:

```c
case HAL_BUTTON_KNOB:
    // TODO: 通过 IO 扩展器读取旋钮按键状态
    // 示例代码:
    // return hal_io_exp_read_input(0, IO_EXP_P0_KNOB_BTN, pressed);
    *pressed = false;
    return ESP_ERR_NOT_SUPPORTED;
```

实现 IO 扩展器驱动后,只需修改此处即可支持旋钮按键。

### 与消息总线集成

可以在回调函数中发送消息到消息总线:

```c
void button_callback(hal_button_id_t id, hal_button_event_t event)
{
    // 发送到消息总线
    message_bus_publish(MIMI_CHAN_HARDWARE, button_event_json);
}
```

## 测试建议

1. **基础功能测试**
   - 按下 BOOT 按键,验证 PRESS 事件
   - 释放按键,验证 RELEASE 事件
   - 长按 1 秒,验证 LONG_PRESS 事件

2. **防抖测试**
   - 快速抖动按键,验证不会产生多余事件
   - 检查事件间隔是否符合防抖时间

3. **并发测试**
   - 多次快速按下释放
   - 验证事件顺序正确

4. **资源测试**
   - 检查内存使用
   - 验证反初始化后资源释放

## 下一步工作

1. 实现 TCA9555 IO 扩展器驱动
2. 集成旋钮按键支持
3. 与消息总线集成
4. 编写单元测试
5. 在实际硬件上测试

## 提交信息

```
feat(hal): implement button handling module

- Add BOOT button support with GPIO interrupt
- Implement software debouncing (50ms)
- Add long press detection (1000ms)
- Event-driven callback mechanism
- Reserve interface for knob button (IO expander)
- Add usage example and documentation

Files:
- main/hal/input/hal_button.h
- main/hal/input/hal_button.c
- main/hal/input/hal_button_example.c
- main/hal/input/CMakeLists.txt
- main/hal/input/README.md
```

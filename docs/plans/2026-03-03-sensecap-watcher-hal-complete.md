# SenseCAP Watcher 完整硬件抽象层实施计划

**创建日期**: 2026-03-03
**目标**: 实现 SenseCAP Watcher 所有硬件功能的完整模块化抽象层

## 📋 目标和范围

### 主要目标
1. 修复屏幕花屏问题（QSPI DATA1 缺失）
2. 实现摄像头模块（SSCMA Client + Himax WE2 AI 芯片）
3. 集成 LVGL 图形库
4. 实现电池管理和背光 PWM 控制
5. 确保 Telegram 拍照功能正常工作
6. 设计模块化架构，方便未来切换不同板子

### 范围内
- ✅ 所有 SenseCAP Watcher 硬件模块的 HAL 实现
- ✅ LVGL 图形库集成和基础 UI
- ✅ SSCMA Client 摄像头通信协议
- ✅ 电池管理和背光控制
- ✅ 板级配置抽象层

### 范围外
- ❌ 应用层业务逻辑（Agent、Telegram 等已有实现）
- ❌ OTA 升级功能（已有实现）
- ❌ 网络功能（WiFi、HTTP 等已有实现）

## 🏗️ 技术方案

### 架构设计

```
main/
├── board/
│   ├── board_config.h          # 板级配置抽象
│   └── sensecap_watcher.c      # 板级初始化
├── hal/
│   ├── display/
│   │   ├── spd2010.c           # [修复] 添加 QSPI DATA1
│   │   ├── hal_display.c       # [扩展] LVGL 适配
│   │   └── lvgl_port.c         # [新增] LVGL 移植层
│   ├── camera/
│   │   ├── sscma_client.c      # [新增] SSCMA 通信协议
│   │   ├── hal_camera.c        # [新增] 摄像头 HAL
│   │   └── hal_camera.h
│   ├── battery/
│   │   ├── hal_battery.c       # [新增] 电池管理
│   │   └── hal_battery.h
│   ├── backlight/
│   │   ├── hal_backlight.c     # [新增] PWM 背光
│   │   └── hal_backlight.h
│   └── [现有模块保持]
```

### 关键技术点

1. **QSPI 修复**: 添加 GPIO1 作为 DATA1 引脚
2. **SSCMA Client**: SPI 通信协议，支持拍照和 AI 推理
3. **LVGL 集成**: 使用 esp_lvgl_port 组件
4. **电池管理**: ADC 采样 + 分压比计算 + 电压-电量曲线
5. **背光 PWM**: LEDC 外设，支持亮度调节

## 📝 实施步骤

### 阶段 1: 修复屏幕显示 (优先级: 🔥 最高)

**任务 1.1: 添加 QSPI DATA1 引脚配置**
- 文件: `main/hal/hal_config.h`
- 操作: 添加 `#define GPIO_QSPI_DATA1 1`
- 验证: 编译通过

**任务 1.2: 修复 SPD2010 驱动**
- 文件: `main/hal/display/spd2010.c`
- 操作: 在 `spi_bus_config_t` 中添加 `.data1_io_num = GPIO_QSPI_DATA1`
- 验证: 屏幕显示正常，无花屏

**任务 1.3: 测试基础显示**
- 操作: 调用 `hal_display_fill()` 填充纯色
- 验证: 屏幕显示纯色（红、绿、蓝）无花屏

**依赖**: 无
**预计时间**: 30 分钟
**验收标准**: 屏幕显示纯色正常，无竖向条纹

---

### 阶段 2: LVGL 图形库集成 (优先级: 🔥 高)

**任务 2.1: 添加 LVGL 组件依赖**
- 文件: `main/idf_component.yml`
- 操作: 添加 `espressif/esp_lvgl_port` 依赖
- 验证: `idf.py reconfigure` 成功

**任务 2.2: 实现 LVGL 显示驱动适配**
- 文件: `main/hal/display/lvgl_port.c` (新建)
- 操作:
  - 实现 `lvgl_port_init()` 初始化函数
  - 实现 flush callback 调用 `hal_display_draw()`
  - 配置双缓冲和 DMA
- 验证: LVGL 示例能正常显示

**任务 2.3: 集成触摸输入**
- 文件: `main/hal/display/lvgl_port.c`
- 操作: 注册触摸回调到 LVGL input device
- 验证: 触摸事件能被 LVGL 接收

**任务 2.4: 创建基础 UI 示例**
- 文件: `main/hal/display/lvgl_port.c`
- 操作: 创建简单的标签和按钮示例
- 验证: UI 显示正常，触摸响应正常

**依赖**: 阶段 1 完成
**预计时间**: 4 小时
**验收标准**: LVGL UI 正常显示，触摸交互正常

---

### 阶段 3: 摄像头模块 (优先级: 🔥 最高)

**任务 3.1: 实现 SSCMA Client 通信协议**
- 文件: `main/hal/camera/sscma_client.c` (新建)
- 操作:
  - 实现 SPI 通信协议（参考模板项目）
  - 实现命令发送和响应接收
  - 实现 JPEG 数据接收
- 验证: 能与 Himax WE2 芯片通信

**任务 3.2: 实现摄像头 HAL 接口**
- 文件: `main/hal/camera/hal_camera.c` (新建)
- 操作:
  - `hal_camera_init()` - 初始化 SPI2 和 SSCMA Client
  - `hal_camera_capture()` - 拍照并返回 JPEG 数据
  - `hal_camera_set_resolution()` - 设置分辨率
- 验证: 能成功拍照并获取 JPEG 数据

**任务 3.3: 集成到 Telegram 拍照功能**
- 文件: `main/telegram/telegram.c` (修改)
- 操作: 调用 `hal_camera_capture()` 获取照���
- 验证: Telegram 发送拍照命令能收到照片

**依赖**: 阶段 1 完成
**预计时间**: 8 小时
**验收标准**: Telegram 拍照功能正常工作

---

### 阶段 4: 电池和背光 (优先级: 中)

**任务 4.1: 实现电池管理**
- 文件: `main/hal/battery/hal_battery.c` (新建)
- 操作:
  - 实现 ADC 采样（GPIO3, 分压比 4.1）
  - 实现电压-电量曲线计算
  - 提供 `hal_battery_get_voltage()` 和 `hal_battery_get_percent()`
- 验证: 能正确读取电池电压和电量

**任务 4.2: 实现 PWM 背光控制**
- 文件: `main/hal/backlight/hal_backlight.c` (新建)
- 操作:
  - 使用 LEDC 外设控制 GPIO8
  - 提供 `hal_backlight_set_brightness(0-100)`
- 验证: 能调节屏幕亮度

**依赖**: 无
**预计时间**: 3 小时
**验收标准**: 电池电量显示正确，背光亮度可调

---

### 阶段 5: 集成测试和优化 (优先级: 中)

**任务 5.1: 完整功能测试**
- 测试所有硬件模块联动
- 测试 Telegram 拍照功能
- 测试 LVGL UI 显示

**任务 5.2: 性能优化**
- 优化内存使用（SPIRAM）
- 优化启动时间
- 优化摄像头拍照速度

**任务 5.3: 文档更新**
- 更新 HAL 使用文档
- 更新编译烧录指南

**依赖**: 阶段 1-4 完成
**预计时间**: 2 小时
**验收标准**: 所有功能正常，性能满足要求

## 🔗 依赖关系

```
阶段 1 (屏幕修复)
    ├─→ 阶段 2 (LVGL 集成)
    └─→ 阶段 3 (摄像头模块)

阶段 4 (电池和背光) - 独立

阶段 5 (集成测试) ← 依赖所有阶段
```

**并行执行策略**:
- 阶段 1 完成后，阶段 2 和阶段 3 可以并行开发
- 阶段 4 可以独立并行开发

## ⚠️ 风险和缓解措施

| 风险 | 影响 | 概率 | 缓解措施 |
|------|------|------|----------|
| SSCMA 通信协议复杂 | 高 | 中 | 参考模板项目完整实现，逐步调试 |
| LVGL 内存占用过大 | 中 | 低 | 使用 SPIRAM，配置双缓冲 |
| 摄像头初始化耗时长 | 低 | 高 | 异步初始化，显示加载提示 |
| 屏幕修复后仍有问题 | 高 | 低 | 检查 SPI 时序和电源稳定性 |

## 📊 工作量估算

| 阶段 | 预计时间 | 优先级 |
|------|----------|--------|
| 阶段 1: 屏幕修复 | 0.5 小时 | 🔥 最高 |
| 阶段 2: LVGL 集成 | 4 小时 | 🔥 高 |
| 阶段 3: 摄像头模块 | 8 小时 | 🔥 最高 |
| 阶段 4: 电池和背光 | 3 小时 | 中 |
| 阶段 5: 集成测试 | 2 小时 | 中 |
| **总计** | **17.5 小时** | - |

## ✅ 验收标准

### 功能验收
- [ ] 屏幕显示正常，无花屏
- [ ] LVGL UI 正常显示，触摸交互正常
- [ ] Telegram 拍照功能正常工作
- [ ] 电池电量显示正确
- [ ] 背光亮度可调

### 性能验收
- [ ] 启动时间 < 5 秒
- [ ] 摄像头拍照响应 < 3 秒
- [ ] LVGL UI 刷新率 > 30 FPS
- [ ] 内存使用 < 80%

### 代码质量
- [ ] 所有模块有清晰的 HAL 接口
- [ ] 代码注释完整
- [ ] 无编译警告
- [ ] 通过基础功能测试

## 📝 实施说明

1. **使用团队模式开发**: 派发多个智能体并行开发独立模块
2. **优先修复屏幕**: 立即见效，提升信心
3. **参考模板项目**: 借鉴成熟实现，避免重复踩坑
4. **保持最小化代码**: 只实现必要功能，避免过度设计
5. **持续测试验证**: 每个阶段完成后立即测试


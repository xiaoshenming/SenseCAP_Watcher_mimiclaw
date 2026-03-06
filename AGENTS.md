# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MimiClaw is an ESP32-S3 AI agent firmware written in pure C/FreeRTOS — no Linux, no Node.js. It runs on a $5 chip and connects users to Claude/GPT via Telegram. The primary hardware target is **SenseCAP Watcher** (ESP32-S3, 16 MB flash, 8 MB PSRAM), but the codebase is designed to support other ESP32-S3 boards.

**ESP-IDF version**: 5.5.x (required, see `main/idf_component.yml`)

## Build Commands

### 已配置好的开发环境（优先使用）

**IDF 环境已在 tmux session `mimiidf` 中预先配置好**，直接在该 session 中执行编译、烧录命令即可，无需手动激活环境。

```bash
# 附加到已有的 tmux session
tmux attach -t mimiidf

# 如果 session 不存在，手动激活环境：
conda activate idf55
source /opt/esp-idf/export.sh
newgrp uucp
```

**AI 可主动在 `mimiidf` session 中执行编译和烧录来验证结果。**

### 常用命令

```bash
# 增量编译
idf.py build

# 修改过 mimi_secrets.h 后必须完全重编译
idf.py fullclean && idf.py build

# 编译 + 烧录 + 查看实时日志（一条命令完成，推荐）
idf.py build && idf.py flash monitor

# 只烧录 + 查看日志
idf.py build &&  idf.py flash monitor 

# 只查看日志（不烧录）
idf.py build &&  idf.py  monitor 

# 退出串口监视：Ctrl + ]
```

**SenseCAP Watcher 串口说明**：只有一个 USB-C 口，接的是 CP2102 UART 桥接芯片，Linux 下识别为 `/dev/ttyUSB0`。

**进入下载模式**（烧录失败时）：按住 Boot 键再按 Reset，松开 Reset 后松开 Boot。

### 首次配置

```bash
# Set target (one-time)
idf.py set-target esp32s3

# Configure secrets
cp main/mimi_secrets.h.example main/mimi_secrets.h
# 编辑 main/mimi_secrets.h，填写 WiFi / Telegram Token / API Key
```

## Configuration

MimiClaw uses a **two-layer config system**:

1. **Build-time** (`main/mimi_secrets.h`) — highest priority, gitignored
2. **Runtime NVS** — overrides via serial CLI, survives reboots

Key runtime CLI commands (connect via UART at 115200 baud):
```
mimi> wifi_set SSID PASSWORD
mimi> set_api_key sk-ant-...
mimi> set_model_provider anthropic   # or openai
mimi> set_proxy 10.0.0.1 7897
mimi> config_show
mimi> config_reset
```

All constants (stack sizes, queue depths, SPIFFS paths, etc.) are centralized in `main/mimi_config.h`.

## Architecture

### Data Flow

```
Telegram/WebSocket → Inbound Queue → Agent Loop (Core 1)
                                          ↓
                                    LLM API (Claude/GPT)
                                          ↓
                                    Tool Execution loop (max 10 iterations)
                                          ↓
                                    Outbound Queue → Telegram/WebSocket
```

### FreeRTOS Task Layout

| Task | Core | Purpose |
|------|------|---------|
| `tg_poll` | 0 | Telegram long polling (30s timeout) |
| `agent_loop` | 1 | ReAct agent loop — all LLM calls here |
| `outbound` | 0 | Route responses to Telegram/WebSocket |
| `serial_cli` | 0 | Interactive REPL (esp_console) |

Core 0 = all I/O; Core 1 = dedicated to agent/LLM processing.

### Key Module Locations

```
main/
├── mimi.c                  # Entry point: app_main() startup sequence
├── mimi_config.h           # All compile-time constants and tuning knobs
├── mimi_secrets.h(.example) # Build-time credentials (gitignored)
│
├── agent/                  # ReAct loop + system prompt builder
│   ├── agent_loop.c        # Multi-turn tool-use loop (max 10 iterations)
│   └── context_builder.c   # Reads SOUL.md + USER.md + MEMORY.md → system prompt
│
├── llm/
│   └── llm_proxy.c         # Anthropic + OpenAI API calls (non-streaming)
│
├── tools/
│   ├── tool_registry.c     # Register/dispatch tools; builds tools JSON for API
│   ├── tool_web_search.c   # Brave Search API
│   ├── tool_camera.c       # Camera capture tool (SenseCAP Watcher)
│   ├── tool_display.c      # Display control tool
│   └── tool_sysinfo.c      # System info tool
│
├── memory/
│   ├── memory_store.c      # MEMORY.md + daily notes (YYYY-MM-DD.md) on SPIFFS
│   └── session_mgr.c       # Per-chat JSONL session files, ring buffer history
│
├── hal/                    # Hardware Abstraction Layer (SenseCAP Watcher)
│   ├── hal_init.c          # watcher_hal_init(): initializes all peripherals in order
│   ├── hal_config.h        # All GPIO pin numbers and I2C/SPI addresses
│   ├── display/            # SPD2010 QSPI display + LVGL port
│   ├── audio/              # ES8311 DAC + ES7243E ADC via I2S
│   ├── camera/             # SSCMA client (Himax WE2 AI chip via SPI)
│   ├── io_expander/        # TCA9555 GPIO expander (controls power rails)
│   ├── battery/            # ADC-based battery voltage reading
│   ├── backlight/          # PWM backlight control (GPIO 8)
│   ├── touch/              # CST816S capacitive touch (I2C1)
│   ├── input/              # Rotary encoder + buttons
│   ├── led/                # WS2813 RGB LED
│   ├── sd_card/            # SD card (SPI2 + FAT)
│   └── power/              # Peripheral power rail management
│
├── skills/
│   └── skill_loader.c      # Loads SKILL.md files from SPIFFS into system prompt
│
├── telegram/               # Long polling, message splitting, send API
├── bus/                    # mimi_msg_t struct + two FreeRTOS queues
├── cli/                    # esp_console REPL with debug commands
├── cron/                   # Cron scheduler (LLM-created jobs, persisted to cron.json)
├── heartbeat/              # Periodic HEARTBEAT.md checker → autonomous agent trigger
├── gateway/                # WebSocket server (port 18789, max 4 clients)
├── proxy/                  # HTTP CONNECT tunnel (SOCKS5 also supported)
├── wifi/                   # WiFi STA lifecycle + exponential backoff
└── ota/                    # esp_https_ota wrapper
```

### SPIFFS Storage Layout

SPIFFS is mounted at `/spiffs` (12 MB partition, flat filesystem):
```
/spiffs/config/SOUL.md          # AI personality (editable)
/spiffs/config/USER.md          # User profile (editable)
/spiffs/memory/MEMORY.md        # Long-term memory
/spiffs/memory/YYYY-MM-DD.md    # Daily notes
/spiffs/sessions/tg_<id>.jsonl  # Per-chat history (JSONL)
/spiffs/cron.json               # Persisted cron jobs
/spiffs/HEARTBEAT.md            # Autonomous task list
/spiffs/skills/<name>/SKILL.md  # Skill definitions
```

The `spiffs_data/` directory is flashed as the initial SPIFFS image (`spiffs_create_partition_image` in CMakeLists.txt).

### Message Bus

Internal messages use `mimi_msg_t`:
```c
typedef struct {
    char channel[16];   // "telegram", "websocket", "cli"
    char chat_id[32];   // Telegram chat_id or WebSocket fd
    char *content;      // Heap-allocated; receiver must free()
} mimi_msg_t;
```

### Adding a New Tool

1. Create `main/tools/tool_<name>.c` and `.h`
2. Implement `esp_err_t tool_<name>_execute(const char *input_json, char *output, size_t output_size)`
3. Define a `mimi_tool_t` with `name`, `description`, `input_schema_json`, and the execute pointer
4. Register in `tool_registry_init()` via `tool_registry_register()`
5. Add the `.c` file to `main/CMakeLists.txt` SRCS

### HAL Init Order (SenseCAP Watcher)

The HAL must be initialized in this strict order (power dependencies):
1. IO Expander (TCA9555) — controls all power rails
2. Power management — enable LCD/SD/AI power
3. Display (SPD2010 QSPI, 412×412) + backlight
4. Audio (ES8311 DAC + ES7243E ADC via I2S)
5. Touch (CST816S on I2C1)
6. RGB LED (WS2813)
7. Rotary encoder + buttons
8. SD card (SPI2 + FAT)

### LLM Provider Switching

Supports both Anthropic (Claude) and OpenAI (GPT). Switch at runtime:
```
mimi> set_model_provider openai
mimi> set_model gpt-4o
```
Or at build time via `MIMI_SECRET_MODEL_PROVIDER` in `mimi_secrets.h`.

### Memory Allocation

Large buffers (32 KB+) must use PSRAM:
```c
heap_caps_calloc(1, size, MALLOC_CAP_SPIRAM)
```
Internal SRAM is limited to ~40 KB for task stacks.

## 参考实现：SenseCAP Watcher 问题排查指南

**遇到任何 SenseCAP Watcher 硬件相关问题时，首先查阅以下参考项目：**

```
/home/ming/data/Project/ClionProject/ESP32/xiaozhi-esp32
```

该项目包含针对 SenseCAP Watcher 几乎最完整、最可靠的硬件接口调用与模块化集成实现。具体来说：

- **HAL 模块初始化**：直接参照该项目的初始化顺序和调用方式一比一复刻，不要自行猜测或调整顺序
- **驱动接口**：显示、音频、触摸、摄像头、IO 扩展器等驱动的寄存器配置和时序，以该项目为准
- **组件依赖**：ESP-IDF 组件版本、Kconfig 选项、`idf_component.yml` 的写法参照该项目
- **GPIO 引脚**：如果 `hal_config.h` 中的引脚定义存疑，以该项目的实际使用为准

**排查流程建议**：

1. 先在 `xiaozhi-esp32` 中找到对应功能的实现文件
2. 对比 MimiClaw 中的实现，找出差异
3. 按照 `xiaozhi-esp32` 的模式修正 MimiClaw 的实现

> 原则：模块化调用和初始化只要按照 `xiaozhi-esp32` 的模板一模一样地复刻，就不会有硬件兼容性问题。

---

## Key Design Decisions

- **Non-streaming LLM**: Uses non-streaming JSON responses to simplify parsing on embedded hardware
- **PSRAM for large buffers**: TLS connections, JSON parse buffers, session cache all in PSRAM
- **NVS as runtime override**: `mimi_secrets.h` takes priority; NVS values persist across reboots when set via CLI
- **SPIFFS flat filesystem**: No real directories — files use path-like names (`/spiffs/memory/MEMORY.md`)
- **Content ownership**: `mimi_msg_t.content` is heap-allocated and ownership transfers on queue push; receiver must `free()`

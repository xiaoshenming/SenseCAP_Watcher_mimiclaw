del: glm-4.7, body: 7900 bytes)
I (24514) llm: LLM tools request (7900 bytes): {"model":"glm-4.7","max_completion_tokens":4096,"messages":[{"role":"system","content":"# MimiClaw\n\nYou are MimiClaw, a personal AI assistant running on an ES ...
I (24534) llm: Custom URL: base='https://api.coolyeah.net/v1' + endpoint='/chat/completions' = 'https://api.coolyeah.net/v1/chat/completions'
I (24554) llm: API URL: https://api.coolyeah.net/v1/chat/completions
I (25884) telegram: Message update_id=29221603 message_id=108 from chat 8663478141: 你还活着吗...
I (27624) llm: LLM tools raw response (814 bytes): {"choices":[{"finish_reason":"stop","index":0,"message":{"content":"当然还活着！🐾 我是 MimiClaw，在 ESP32-S3 上运行正常。\n\n有什么我可� ...
I (27644) llm: Response: 107 bytes text, 0 tool calls, stop=end_turn
I (28764) agent: Session saved for chat 8663478141
I (28764) agent: Queue final response to telegram:8663478141 (107 bytes)
I (28764) agent: Free PSRAM: 8153000 bytes
I (28774) agent: Processing message from telegram:8663478141
E (29574) esp-tls-mbedtls: mbedtls_ssl_handshake returned -0x7280
E (29574) esp-tls: Failed to open new connection
E (29574) proxy: TLS handshake failed over proxy tunnel
I (30994) skills: Skills summary: 221 bytes
I (30994) context: System prompt built: 2953 bytes
I (30994) agent: LLM turn context: channel=telegram chat_id=8663478141
I (31024) llm: Calling LLM API with tools (provider: openai, model: glm-4.7, body: 8087 bytes)
I (31024) llm: LLM tools request (8087 bytes): {"model":"glm-4.7","max_completion_tokens":4096,"messages":[{"role":"system","content":"# MimiClaw\n\nYou are MimiClaw, a personal AI assistant running on an ES ...
I (31054) llm: Custom URL: base='https://api.coolyeah.net/v1' + endpoint='/chat/completions' = 'https://api.coolyeah.net/v1/chat/completions'
I (31064) llm: API URL: https://api.coolyeah.net/v1/chat/completions
I (34484) llm: LLM tools raw response (899 bytes): {"choices":[{"finish_reason":"stop","index":0,"message":{"content":"是的，我还活着！🤖\n\n随时待命，有什么需要帮忙的吗？","reasoning_co ...
I (34504) llm: Response: 75 bytes text, 0 tool calls, stop=end_turn





I (45764) telegram: Sending telegram chunk to 8663478141 (75 bytes)
E (50864) esp-tls-mbedtls: mbedtls_ssl_handshake returned -0x7280
E (50864) esp-tls: Failed to open new connection
E (50864) proxy: TLS handshake failed over proxy tunnel
I (55064) telegram: Telegram send success to 8663478141 (75 bytes)
I (55064) mimi: Telegram send success for 8663478141 (75 bytes)
I (59684) telegram: Message update_id=29221604 message_id=113 from chat 8663478141: 拍张照片给我...
I (59684) agent: Processing message from telegram:8663478141
I (61904) skills: Skills summary: 221 bytes
I (61904) context: System prompt built: 2953 bytes
I (61904) agent: LLM turn context: channel=telegram chat_id=8663478141
I (61914) mimi: Dispatching response to telegram:8663478141
I (61914) telegram: Sending telegram chunk to 8663478141 (22 bytes)
I (61934) llm: Calling LLM API with tools (provider: openai, model: glm-4.7, body: 8245 bytes)
I (61934) llm: LLM tools request (8245 bytes): {"model":"glm-4.7","max_completion_tokens":4096,"messages":[{"role":"system","content":"# MimiClaw\n\nYou are MimiClaw, a personal AI assistant running on an ES ...
I (61954) llm: Custom URL: base='https://api.coolyeah.net/v1' + endpoint='/chat/completions' = 'https://api.coolyeah.net/v1/chat/completions'
I (61964) llm: API URL: https://api.coolyeah.net/v1/chat/completions
I (65414) llm: LLM tools raw response (1253 bytes): {"id":"msg_60bea9c8-ff86-4be2-b15d-bce992caf8d9","model":"glm-4.7","object":"chat.completion","created":1772595689,"choices":[{"index":0,"message":{"role":"assi ...
I (65424) llm: Response: 0 bytes text, 1 tool calls, stop=tool_use
I (65424) agent: Tool use iteration 1: 1 calls
I (65434) tools: Executing tool: camera_capture
I (65434) tool_camera: Capturing photo...
E (65544) spi_master: spi_master_init_driver(282): host_id not initialized
E (65544) sscma_client.io.spi: sscma_client_new_io_spi_bus(77): adding spi device to bus failed
ESP_ERROR_CHECK failed: esp_err_t 0x103 (ESP_ERR_INVALID_STATE) at 0x4201915b
--- 0x4201915b: sscma_hal_init at /home/ming/data/Project/ClionProject/ESP32/mimiclaw/main/hal/camera/sscma_client.c:63
file: "./main/hal/camera/sscma_client.c" line 63
func: sscma_hal_init
expression: sscma_client_new_io_spi_bus((sscma_client_spi_bus_handle_t)SPI2_HOST, &io_cfg, &io_handle)

abort() was called at PC 0x40380d2b on core 1
--- 0x40380d2b: _esp_error_check_failed at /opt/esp-idf/components/esp_system/esp_err.c:49


Backtrace: 0x40380d6d:0x3fccaf70 0x40380d35:0x3fccaf90 0x4038a6cd:0x3fccafb0 0x40380d2b:0x3fccb020 0x4201915b:0x3fccb050 0x420177ef:0x3fccb0e0 0x42016c84:0x3fccb100 0x420156a5:0x3fccb130 0x420119f0:0x3fccb150 0x42011ced:0x3fccb190 0x40381b31:0x3fccb3d0
--- 0x40380d6d: panic_abort at /opt/esp-idf/components/esp_system/panic.c:477
--- 0x40380d35: esp_system_abort at /opt/esp-idf/components/esp_system/port/esp_system_chip.c:87
--- 0x4038a6cd: abort at /opt/esp-idf/components/newlib/src/abort.c:38
--- 0x40380d2b: _esp_error_check_failed at /opt/esp-idf/components/esp_system/esp_err.c:49
--- 0x4201915b: sscma_hal_init at /home/ming/data/Project/ClionProject/ESP32/mimiclaw/main/hal/camera/sscma_client.c:63
--- 0x420177ef: hal_camera_init at /home/ming/data/Project/ClionProject/ESP32/mimiclaw/main/hal/camera/hal_camera.c:12
--- 0x42016c84: tool_camera_execute at /home/ming/data/Project/ClionProject/ESP32/mimiclaw/main/tools/tool_camera.c:34
--- 0x420156a5: tool_registry_execute at /home/ming/data/Project/ClionProject/ESP32/mimiclaw/main/tools/tool_registry.c:239
--- 0x420119f0: build_tool_results at /home/ming/data/Project/ClionProject/ESP32/mimiclaw/main/agent/agent_loop.c:155
--- 0x42011ced: agent_loop_task at /home/ming/data/Project/ClionProject/ESP32/mimiclaw/main/agent/agent_loop.c:263
--- 0x40381b31: vPortTaskWrapper at /opt/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/port.c:139




ELF file SHA256: c23ba8c35

Rebooting...
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0xc (RTC_SW_CPU_RST),boot:0x8 (SPI_FAST_FLASH_BOOT)
Saved PC:0x4037c686
--- 0x4037c686: esp_cpu_wait_for_intr at /opt/esp-idf/components/esp_hw_support/cpu.c:64
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce2820,len:0x1700
load:0x403c8700,len:0xec0
--- 0x403c8700: _stext at ??:?
load:0x403cb700,len:0x31c0
entry 0x403c894c
--- 0x403c894c: call_start_cpu0 at /opt/esp-idf/components/bootloader/subproject/main/bootloader_start.c:25
I (29) boot: ESP-IDF v5.5.2 2nd stage bootloader
I (29) boot: compile time Mar  4 2026 11:35:25
I (29) boot: Multicore bootloader
I (29) boot: chip revision: v0.2
I (32) boot: efuse block revision: v1.3
I (36) qio_mode: Enabling QIO for flash chip WinBond
I (40) boot.esp32s3: Boot SPI Speed : 80MHz
I (44) boot.esp32s3: SPI Mode       : QIO
I (48) boot.esp32s3: SPI Flash Size : 16MB
I (52) boot: Enabling RNG early entropy source...
I (56) boot: Partition Table:
I (59) boot: ## Label            Usage          Type ST Offset   Length
I (65) boot:  0 nvs              WiFi data        01 02 00009000 00006000
I (72) boot:  1 otadata          OTA data         01 00 0000f000 00002000
I (78) boot:  2 phy_init         RF data          01 01 00011000 00001000
I (85) boot:  3 ota_0            OTA app          00 10 00020000 00200000
I (91) boot:  4 ota_1            OTA app          00 11 00220000 00200000
I (98) boot:  5 spiffs           Unknown data     01 82 00420000 00bd0000
I (104) boot:  6 coredump         Unknown data     01 03 00ff0000 00010000
I (111) boot: End of partition table
I (114) esp_image: segment 0: paddr=00020020 vaddr=3c120020 size=4af9ch (307100) map
I (168) esp_image: segment 1: paddr=0006afc4 vaddr=3fca1000 size=05054h ( 20564) load
I (172) esp_image: segment 2: paddr=00070020 vaddr=42000020 size=11f8d0h (1177808) map
I (348) esp_image: segment 3: paddr=0018f8f8 vaddr=3fca6054 size=00530h (  1328) load
I (348) esp_image: segment 4: paddr=0018fe30 vaddr=40374000 size=1cef8h (118520) load
I (374) esp_image: segment 5: paddr=001acd30 vaddr=50000000 size=00020h (    32) load
I (385) boot: Loaded app from partition at offset 0x20000
I (386) boot: Disabling RNG early entropy source...
I (396) octal_psram: vendor id    : 0x0d (AP)
I (396) octal_psram: dev id       : 0x02 (generation 3)
I (396) octal_psram: density      : 0x03 (64 Mbit)
I (398) octal_psram: good-die     : 0x01 (Pass)
I (402) octal_psram: Latency      : 0x01 (Fixed)
I (407) octal_psram: VCC          : 0x01 (3V)
I (411) octal_psram: SRF          : 0x01 (Fast Refresh)
I (416) octal_psram: BurstType    : 0x01 (Hybrid Wrap)
I (421) octal_psram: BurstLen     : 0x01 (32 Byte)
I (425) octal_psram: Readlatency  : 0x02 (10 cycles@Fixed)
I (430) octal_psram: DriveStrength: 0x00 (1/1)
I (435) MSPI Timing: Enter psram timing tuning
I (439) esp_psram: Found 8MB PSRAM device

我看到亮灯了！但是没效果！好像设备重启了？



I (7515) cron: Cron service started (0 jobs, check every 60s)
I (7515) heartbeat: Heartbeat started (every 30 min)
I (7535) ws: WebSocket server started on port 18789
I (7535) mimi: All services started!
I (7535) mimi: MimiClaw ready. Type 'help' for CLI commands.
I (7545) main_task: Returned from app_main()
I (60185) telegram: Message update_id=29221605 message_id=115 from chat 8663478141: 显示helloworld...
I (60185) agent: Processing message from telegram:8663478141
I (62825) skills: Skills summary: 221 bytes
I (62825) context: System prompt built: 2953 bytes
I (62825) agent: LLM turn context: channel=telegram chat_id=8663478141
I (62845) mimi: Dispatching response to telegram:8663478141
I (62845) telegram: Sending telegram chunk to 8663478141 (22 bytes)
I (62855) llm: Calling LLM API with tools (provider: openai, model: glm-4.7, body: 8243 bytes)
I (62855) llm: LLM tools request (8243 bytes): {"model":"glm-4.7","max_completion_tokens":4096,"messages":[{"role":"system","content":"# MimiClaw\n\nYou are MimiClaw, a personal AI assistant running on an ES ...
I (62875) llm: Custom URL: base='https://api.coolyeah.net/v1' + endpoint='/chat/completions' = 'https://api.coolyeah.net/v1/chat/completions'
I (62895) llm: API URL: https://api.coolyeah.net/v1/chat/completions
E (65295) esp-tls-mbedtls: mbedtls_ssl_handshake returned -0x7280
E (65295) esp-tls: Failed to open new connection
E (65295) proxy: TLS handshake failed over proxy tunnel
I (65505) llm: LLM tools raw response (887 bytes): {"id":"msg_37264e70-024d-49dc-9c94-244f2f522205","model":"glm-4.7","object":"chat.completion","created":1772595754,"choices":[{"index":0,"message":{"role":"assi ...
I (65515) llm: Response: 0 bytes text, 1 tool calls, stop=tool_use
I (65515) agent: Tool use iteration 1: 1 calls
I (65525) tools: Executing tool: display_text
I (65525) tool_display: Displaying text: helloworld
I (65525) agent: Tool display_text result: 31 bytes
I (65555) llm: Calling LLM API with tools (provider: openai, model: glm-4.7, body: 8559 bytes)
I (65555) llm: LLM tools request (8559 bytes): {"model":"glm-4.7","max_completion_tokens":4096,"messages":[{"role":"system","content":"# MimiClaw\n\nYou are MimiClaw, a personal AI assistant running on an ES ...
I (65575) llm: Custom URL: base='https://api.coolyeah.net/v1' + endpoint='/chat/completions' = 'https://api.coolyeah.net/v1/chat/completions'
I (65595) llm: API URL: https://api.coolyeah.net/v1/chat/completions
I (67135) llm: LLM tools raw response (629 bytes): {"id":"b09f1b0b7d634ea7888419210c037127","model":"z-ai/glm4.7","object":"chat.completion","created":1772595756,"choices":[{"index":0,"message":{"role":"assistan ...
I (67155) llm: Response: 38 bytes text, 0 tool calls, stop=end_turn
I (67175) agent: Session saved for chat 8663478141
I (67175) agent: Queue final response to telegram:8663478141 (38 bytes)
I (67175) agent: Free PSRAM: 8172588 bytes
I (67855) telegram: Telegram send success to 8663478141 (22 bytes)
I (67855) mimi: Telegram send success for 8663478141 (22 bytes)
I (67855) mimi: Dispatching response to telegram:8663478141
I (67865) telegram: Sending telegram chunk to 8663478141 (38 bytes)
I (73485) telegram: Telegram send success to 8663478141 (38 bytes)
I (73485) mimi: Telegram send success for 8663478141 (38 bytes)

还是黑屏！但是背光正确！
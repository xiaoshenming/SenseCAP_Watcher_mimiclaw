 cd /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw
❯ conda activate idf55
❯ source /opt/esp-idf/export.sh
Checking "python3" ...
Python 3.12.12
"python3" has been detected
Activating ESP-IDF 5.5
Setting IDF_PATH to '/opt/esp-idf'.
* Checking python version ... 3.12.12
* Checking python dependencies ... OK
* Deactivating the current ESP-IDF environment (if any) ... OK
* Establishing a new ESP-IDF environment ... OK
* Identifying shell ... zsh
* Detecting outdated tools in system ... Found tools that are not used by active ESP-IDF version.
For removing old versions of xtensa-esp-elf-gdb, riscv32-esp-elf-gdb, xtensa-esp-elf, riscv32-esp-elf use command 'python /opt/esp-idf/tools/idf_tools.py uninstall'
To free up even more space, remove installation packages of those tools.
Use option python /opt/esp-idf/tools/idf_tools.py uninstall --remove-archives.
* Shell completion ... Autocompletion code generated

Done! You can now compile ESP-IDF projects.
Go to the project directory and run:

  idf.py build
❯ idf.py build &&  idf.py flash monitor
Executing action: all (aliases: build)
Running ninja in directory /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/build
Executing "ninja all"...
[0/1] Re-running CMake...
-- Minimal build - OFF
-- Building ESP-IDF components for target esp32s3
NOTICE: Manifest files have changed, solving dependencies.
.....NOTICE: Updating lock file at /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/dependencies.lock
NOTICE: Processing 3 dependencies:
NOTICE: [1/3] espressif/esp_lvgl_port (2.7.2)
NOTICE: [2/3] lvgl/lvgl (9.5.0)
NOTICE: [3/3] idf (5.5.2)
-- ESP-TEE is currently supported only on the esp32c6;esp32h2;esp32c5 SoCs
-- Project sdkconfig file /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/sdkconfig
Loading defaults file /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/sdkconfig.defaults...
Loading defaults file /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/sdkconfig.defaults.esp32s3...
-- Compiler supported targets: xtensa-esp-elf
-- USING O3
-- App "mimiclaw" version: 7ea4a4a
-- Adding linker script /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/build/esp-idf/esp_system/ld/memory.ld
-- Adding linker script /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/build/esp-idf/esp_system/ld/sections.ld.in
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.api.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.bt_funcs.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.libgcc.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.wdt.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.version.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.ble_master.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.ble_50.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.ble_smp.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.ble_dtm.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.ble_test.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.ble_scan.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.libc.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.newlib.ld
-- Adding linker script /opt/esp-idf/components/soc/esp32s3/ld/esp32s3.peripherals.ld
-- LVGL version: 9.5.0
-- Components: app_trace app_update bootloader bootloader_support bt cmock console cxx driver efuse esp-tls esp_adc esp_app_format esp_bootloader_format esp_coex esp_common esp_driver_ana_cmpr esp_driver_bitscrambler esp_driver_cam esp_driver_dac esp_driver_gpio esp_driver_gptimer esp_driver_i2c esp_driver_i2s esp_driver_isp esp_driver_jpeg esp_driver_ledc esp_driver_mcpwm esp_driver_parlio esp_driver_pcnt esp_driver_ppa esp_driver_rmt esp_driver_sdio esp_driver_sdm esp_driver_sdmmc esp_driver_sdspi esp_driver_spi esp_driver_touch_sens esp_driver_tsens esp_driver_twai esp_driver_uart esp_driver_usb_serial_jtag esp_eth esp_event esp_gdbstub esp_hid esp_http_client esp_http_server esp_https_ota esp_https_server esp_hw_support esp_lcd esp_local_ctrl esp_mm esp_netif esp_netif_stack esp_partition esp_phy esp_pm esp_psram esp_ringbuf esp_rom esp_security esp_system esp_timer esp_vfs_console esp_wifi espcoredump espressif__esp_lvgl_port esptool_py fatfs freertos hal heap http_parser idf_test ieee802154 json log lvgl__lvgl lwip main mbedtls mqtt newlib nvs_flash nvs_sec_provider openthread partition_table perfmon protobuf-c protocomm pthread rt sdmmc soc spi_flash spiffs tcp_transport touch_element ulp unity usb vfs wear_levelling wifi_provisioning wpa_supplicant xtensa
-- Component paths: /opt/esp-idf/components/app_trace /opt/esp-idf/components/app_update /opt/esp-idf/components/bootloader /opt/esp-idf/components/bootloader_support /opt/esp-idf/components/bt /opt/esp-idf/components/cmock /opt/esp-idf/components/console /opt/esp-idf/components/cxx /opt/esp-idf/components/driver /opt/esp-idf/components/efuse /opt/esp-idf/components/esp-tls /opt/esp-idf/components/esp_adc /opt/esp-idf/components/esp_app_format /opt/esp-idf/components/esp_bootloader_format /opt/esp-idf/components/esp_coex /opt/esp-idf/components/esp_common /opt/esp-idf/components/esp_driver_ana_cmpr /opt/esp-idf/components/esp_driver_bitscrambler /opt/esp-idf/components/esp_driver_cam /opt/esp-idf/components/esp_driver_dac /opt/esp-idf/components/esp_driver_gpio /opt/esp-idf/components/esp_driver_gptimer /opt/esp-idf/components/esp_driver_i2c /opt/esp-idf/components/esp_driver_i2s /opt/esp-idf/components/esp_driver_isp /opt/esp-idf/components/esp_driver_jpeg /opt/esp-idf/components/esp_driver_ledc /opt/esp-idf/components/esp_driver_mcpwm /opt/esp-idf/components/esp_driver_parlio /opt/esp-idf/components/esp_driver_pcnt /opt/esp-idf/components/esp_driver_ppa /opt/esp-idf/components/esp_driver_rmt /opt/esp-idf/components/esp_driver_sdio /opt/esp-idf/components/esp_driver_sdm /opt/esp-idf/components/esp_driver_sdmmc /opt/esp-idf/components/esp_driver_sdspi /opt/esp-idf/components/esp_driver_spi /opt/esp-idf/components/esp_driver_touch_sens /opt/esp-idf/components/esp_driver_tsens /opt/esp-idf/components/esp_driver_twai /opt/esp-idf/components/esp_driver_uart /opt/esp-idf/components/esp_driver_usb_serial_jtag /opt/esp-idf/components/esp_eth /opt/esp-idf/components/esp_event /opt/esp-idf/components/esp_gdbstub /opt/esp-idf/components/esp_hid /opt/esp-idf/components/esp_http_client /opt/esp-idf/components/esp_http_server /opt/esp-idf/components/esp_https_ota /opt/esp-idf/components/esp_https_server /opt/esp-idf/components/esp_hw_support /opt/esp-idf/components/esp_lcd /opt/esp-idf/components/esp_local_ctrl /opt/esp-idf/components/esp_mm /opt/esp-idf/components/esp_netif /opt/esp-idf/components/esp_netif_stack /opt/esp-idf/components/esp_partition /opt/esp-idf/components/esp_phy /opt/esp-idf/components/esp_pm /opt/esp-idf/components/esp_psram /opt/esp-idf/components/esp_ringbuf /opt/esp-idf/components/esp_rom /opt/esp-idf/components/esp_security /opt/esp-idf/components/esp_system /opt/esp-idf/components/esp_timer /opt/esp-idf/components/esp_vfs_console /opt/esp-idf/components/esp_wifi /opt/esp-idf/components/espcoredump /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/managed_components/espressif__esp_lvgl_port /opt/esp-idf/components/esptool_py /opt/esp-idf/components/fatfs /opt/esp-idf/components/freertos /opt/esp-idf/components/hal /opt/esp-idf/components/heap /opt/esp-idf/components/http_parser /opt/esp-idf/components/idf_test /opt/esp-idf/components/ieee802154 /opt/esp-idf/components/json /opt/esp-idf/components/log /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/managed_components/lvgl__lvgl /opt/esp-idf/components/lwip /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/main /opt/esp-idf/components/mbedtls /opt/esp-idf/components/mqtt /opt/esp-idf/components/newlib /opt/esp-idf/components/nvs_flash /opt/esp-idf/components/nvs_sec_provider /opt/esp-idf/components/openthread /opt/esp-idf/components/partition_table /opt/esp-idf/components/perfmon /opt/esp-idf/components/protobuf-c /opt/esp-idf/components/protocomm /opt/esp-idf/components/pthread /opt/esp-idf/components/rt /opt/esp-idf/components/sdmmc /opt/esp-idf/components/soc /opt/esp-idf/components/spi_flash /opt/esp-idf/components/spiffs /opt/esp-idf/components/tcp_transport /opt/esp-idf/components/touch_element /opt/esp-idf/components/ulp /opt/esp-idf/components/unity /opt/esp-idf/components/usb /opt/esp-idf/components/vfs /opt/esp-idf/components/wear_levelling /opt/esp-idf/components/wifi_provisioning /opt/esp-idf/components/wpa_supplicant /opt/esp-idf/components/xtensa
-- Configuring done (24.2s)
-- Generating done (0.4s)
-- Build files have been written to: /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/build
[0/1] Re-running CMake...
-- Minimal build - OFF
-- Building ESP-IDF components for target esp32s3
-- ESP-TEE is currently supported only on the esp32c6;esp32h2;esp32c5 SoCs
-- Project sdkconfig file /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/sdkconfig
-- Compiler supported targets: xtensa-esp-elf
-- Adding linker script /opt/esp-idf/components/soc/esp32s3/ld/esp32s3.peripherals.ld
-- Bootloader project name: "bootloader" version: 1
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.api.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.bt_funcs.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.libgcc.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.wdt.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.version.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.libc.ld
-- Adding linker script /opt/esp-idf/components/esp_rom/esp32s3/ld/esp32s3.rom.newlib.ld
-- Adding linker script /opt/esp-idf/components/bootloader/subproject/main/ld/esp32s3/bootloader.rom.ld
-- Components: bootloader bootloader_support efuse esp_app_format esp_bootloader_format esp_common esp_hw_support esp_rom esp_security esp_system esptool_py freertos hal log main micro-ecc newlib partition_table soc spi_flash xtensa
-- Component paths: /opt/esp-idf/components/bootloader /opt/esp-idf/components/bootloader_support /opt/esp-idf/components/efuse /opt/esp-idf/components/esp_app_format /opt/esp-idf/components/esp_bootloader_format /opt/esp-idf/components/esp_common /opt/esp-idf/components/esp_hw_support /opt/esp-idf/components/esp_rom /opt/esp-idf/components/esp_security /opt/esp-idf/components/esp_system /opt/esp-idf/components/esptool_py /opt/esp-idf/components/freertos /opt/esp-idf/components/hal /opt/esp-idf/components/log /opt/esp-idf/components/bootloader/subproject/main /opt/esp-idf/components/bootloader/subproject/components/micro-ecc /opt/esp-idf/components/newlib /opt/esp-idf/components/partition_table /opt/esp-idf/components/soc /opt/esp-idf/components/spi_flash /opt/esp-idf/components/xtensa
-- Adding linker script /opt/esp-idf/components/bootloader/subproject/main/ld/esp32s3/bootloader.ld
-- Configuring done (12.1s)
-- Generating done (0.1s)
-- Build files have been written to: /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/build/bootloader
[1/2] cd /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/build/bootloader/esp-idf/esptool_py && /hom...x8000 bootloader 0x0 /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/build/bootloader/bootloader.bin
Bootloader binary size 0x57e0 bytes. 0x2820 bytes (31%) free.
[1829/1879] Building C object esp-idf/main/CMakeFiles/__idf_main.dir/hal/audio/es8311.c.obj
/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/main/hal/audio/es8311.c:17:18: warning: 'es8311_read_reg' defined but not used [-Wunused-function]
   17 | static esp_err_t es8311_read_reg(es8311_handle_t *handle, uint8_t reg, uint8_t *value)
      |                  ^~~~~~~~~~~~~~~
[1834/1879] Building C object esp-idf/main/CMakeFiles/__idf_main.dir/hal/audio/es7243e.c.obj
/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/main/hal/audio/es7243e.c:17:18: warning: 'es7243e_read_reg' defined but not used [-Wunused-function]
   17 | static esp_err_t es7243e_read_reg(es7243e_handle_t *handle, uint8_t reg, uint8_t *value)
      |                  ^~~~~~~~~~~~~~~~
[1836/1879] Building C object esp-idf/main/CMakeFiles/__idf_main.dir/hal/display/lvgl_port.c.obj
FAILED: [code=1] esp-idf/main/CMakeFiles/__idf_main.dir/hal/display/lvgl_port.c.obj 
/home/ming/.espressif/tools/xtensa-esp-elf/esp-14.2.0_20251107/xtensa-esp-elf/bin/xtensa-esp32s3-elf-gcc -DESP_PLATFORM -DIDF_VER=\"v5.5.2\" -DLV_CONF_INCLUDE_SIMPLE -DMBEDTLS_CONFIG_FILE=\"mbedtls/esp_config.h\" -DSOC_MMU_PAGE_SIZE=CONFIG_MMU_PAGE_SIZE -DSOC_XTAL_FREQ_MHZ=CONFIG_XTAL_FREQ -D_GLIBCXX_HAVE_POSIX_SEMAPHORE -D_GLIBCXX_USE_POSIX_SEMAPHORE -D_GNU_SOURCE -D_POSIX_READER_WRITER_LOCKS -I/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/build/config -I/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/main -I/opt/esp-idf/components/newlib/platform_include -I/opt/esp-idf/components/freertos/config/include -I/opt/esp-idf/components/freertos/config/include/freertos -I/opt/esp-idf/components/freertos/config/xtensa/include -I/opt/esp-idf/components/freertos/FreeRTOS-Kernel/include -I/opt/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/include -I/opt/esp-idf/components/freertos/FreeRTOS-Kernel/portable/xtensa/include/freertos -I/opt/esp-idf/components/freertos/esp_additions/include -I/opt/esp-idf/components/esp_hw_support/include -I/opt/esp-idf/components/esp_hw_support/include/soc -I/opt/esp-idf/components/esp_hw_support/include/soc/esp32s3 -I/opt/esp-idf/components/esp_hw_support/dma/include -I/opt/esp-idf/components/esp_hw_support/ldo/include -I/opt/esp-idf/components/esp_hw_support/debug_probe/include -I/opt/esp-idf/components/esp_hw_support/mspi_timing_tuning/include -I/opt/esp-idf/components/esp_hw_support/mspi_timing_tuning/tuning_scheme_impl/include -I/opt/esp-idf/components/esp_hw_support/power_supply/include -I/opt/esp-idf/components/esp_hw_support/port/esp32s3/. -I/opt/esp-idf/components/esp_hw_support/port/esp32s3/include -I/opt/esp-idf/components/esp_hw_support/mspi_timing_tuning/port/esp32s3/. -I/opt/esp-idf/components/esp_hw_support/mspi_timing_tuning/port/esp32s3/include -I/opt/esp-idf/components/heap/include -I/opt/esp-idf/components/heap/tlsf -I/opt/esp-idf/components/log/include -I/opt/esp-idf/components/soc/include -I/opt/esp-idf/components/soc/esp32s3 -I/opt/esp-idf/components/soc/esp32s3/include -I/opt/esp-idf/components/soc/esp32s3/register -I/opt/esp-idf/components/hal/platform_port/include -I/opt/esp-idf/components/hal/esp32s3/include -I/opt/esp-idf/components/hal/include -I/opt/esp-idf/components/esp_rom/include -I/opt/esp-idf/components/esp_rom/esp32s3/include -I/opt/esp-idf/components/esp_rom/esp32s3/include/esp32s3 -I/opt/esp-idf/components/esp_rom/esp32s3 -I/opt/esp-idf/components/esp_common/include -I/opt/esp-idf/components/esp_system/include -I/opt/esp-idf/components/esp_system/port/soc -I/opt/esp-idf/components/esp_system/port/include/private -I/opt/esp-idf/components/xtensa/esp32s3/include -I/opt/esp-idf/components/xtensa/include -I/opt/esp-idf/components/xtensa/deprecated_include -I/opt/esp-idf/components/lwip/include -I/opt/esp-idf/components/lwip/include/apps -I/opt/esp-idf/components/lwip/include/apps/sntp -I/opt/esp-idf/components/lwip/lwip/src/include -I/opt/esp-idf/components/lwip/port/include -I/opt/esp-idf/components/lwip/port/freertos/include -I/opt/esp-idf/components/lwip/port/esp32xx/include -I/opt/esp-idf/components/lwip/port/esp32xx/include/arch -I/opt/esp-idf/components/lwip/port/esp32xx/include/sys -I/opt/esp-idf/components/nvs_flash/include -I/opt/esp-idf/components/esp_partition/include -I/opt/esp-idf/components/esp_wifi/include -I/opt/esp-idf/components/esp_wifi/include/local -I/opt/esp-idf/components/esp_wifi/wifi_apps/include -I/opt/esp-idf/components/esp_wifi/wifi_apps/nan_app/include -I/opt/esp-idf/components/esp_event/include -I/opt/esp-idf/components/esp_phy/include -I/opt/esp-idf/components/esp_phy/esp32s3/include -I/opt/esp-idf/components/esp_netif/include -I/opt/esp-idf/components/esp_http_client/include -I/opt/esp-idf/components/esp_http_server/include -I/opt/esp-idf/components/http_parser -I/opt/esp-idf/components/esp_https_ota/include -I/opt/esp-idf/components/bootloader_support/include -I/opt/esp-idf/components/bootloader_support/bootloader_flash/include -I/opt/esp-idf/components/esp_bootloader_format/include -I/opt/esp-idf/components/esp_app_format/include -I/opt/esp-idf/components/json/cJSON -I/opt/esp-idf/components/spiffs/include -I/opt/esp-idf/components/console -I/opt/esp-idf/components/vfs/include -I/opt/esp-idf/components/esp_vfs_console/include -I/opt/esp-idf/components/app_update/include -I/opt/esp-idf/components/esp-tls -I/opt/esp-idf/components/esp-tls/esp-tls-crypto -I/opt/esp-idf/components/mbedtls/port/include -I/opt/esp-idf/components/mbedtls/mbedtls/include -I/opt/esp-idf/components/mbedtls/mbedtls/library -I/opt/esp-idf/components/mbedtls/esp_crt_bundle/include -I/opt/esp-idf/components/mbedtls/mbedtls/3rdparty/everest/include -I/opt/esp-idf/components/mbedtls/mbedtls/3rdparty/p256-m -I/opt/esp-idf/components/mbedtls/mbedtls/3rdparty/p256-m/p256-m -I/opt/esp-idf/components/esp_timer/include -I/opt/esp-idf/components/driver/deprecated -I/opt/esp-idf/components/driver/i2c/include -I/opt/esp-idf/components/driver/touch_sensor/include -I/opt/esp-idf/components/driver/twai/include -I/opt/esp-idf/components/driver/touch_sensor/esp32s3/include -I/opt/esp-idf/components/esp_pm/include -I/opt/esp-idf/components/esp_ringbuf/include -I/opt/esp-idf/components/esp_driver_gpio/include -I/opt/esp-idf/components/esp_driver_pcnt/include -I/opt/esp-idf/components/esp_driver_gptimer/include -I/opt/esp-idf/components/esp_driver_spi/include -I/opt/esp-idf/components/esp_driver_mcpwm/include -I/opt/esp-idf/components/esp_driver_ana_cmpr/include -I/opt/esp-idf/components/esp_driver_i2s/include -I/opt/esp-idf/components/esp_driver_sdmmc/include -I/opt/esp-idf/components/sdmmc/include -I/opt/esp-idf/components/esp_driver_sdspi/include -I/opt/esp-idf/components/esp_driver_sdio/include -I/opt/esp-idf/components/esp_driver_dac/include -I/opt/esp-idf/components/esp_driver_rmt/include -I/opt/esp-idf/components/esp_driver_tsens/include -I/opt/esp-idf/components/esp_driver_sdm/include -I/opt/esp-idf/components/esp_driver_i2c/include -I/opt/esp-idf/components/esp_driver_uart/include -I/opt/esp-idf/components/esp_driver_ledc/include -I/opt/esp-idf/components/esp_driver_parlio/include -I/opt/esp-idf/components/esp_driver_usb_serial_jtag/include -I/opt/esp-idf/components/esp_driver_twai/include -I/opt/esp-idf/components/esp_adc/include -I/opt/esp-idf/components/esp_adc/interface -I/opt/esp-idf/components/esp_adc/esp32s3/include -I/opt/esp-idf/components/esp_adc/deprecated/include -I/opt/esp-idf/components/fatfs/diskio -I/opt/esp-idf/components/fatfs/src -I/opt/esp-idf/components/fatfs/vfs -I/opt/esp-idf/components/wear_levelling/include -I/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/managed_components/espressif__esp_lvgl_port/include -I/opt/esp-idf/components/esp_lcd/include -I/opt/esp-idf/components/esp_lcd/interface -I/opt/esp-idf/components/esp_lcd/rgb/include -I/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/managed_components/lvgl__lvgl -I/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/managed_components/lvgl__lvgl/src -I/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/managed_components -I/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/managed_components/lvgl__lvgl/examples -I/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/managed_components/lvgl__lvgl/demos -mlongcalls  -fno-builtin-memcpy -fno-builtin-memset -fno-builtin-bzero -fno-builtin-stpcpy -fno-builtin-strncpy -fdiagnostics-color=always -ffunction-sections -fdata-sections -Wall -Werror=all -Wno-error=unused-function -Wno-error=unused-variable -Wno-error=unused-but-set-variable -Wno-error=deprecated-declarations -Wextra -Wno-error=extra -Wno-unused-parameter -Wno-sign-compare -Wno-enum-conversion -gdwarf-4 -ggdb -mdisable-hardware-atomics -Og -fno-shrink-wrap -fmacro-prefix-map=/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw=. -fmacro-prefix-map=/opt/esp-idf=/IDF -fstrict-volatile-bitfields -fno-jump-tables -fno-tree-switch-conversion -std=gnu17 -Wno-old-style-declaration -MD -MT esp-idf/main/CMakeFiles/__idf_main.dir/hal/display/lvgl_port.c.obj -MF esp-idf/main/CMakeFiles/__idf_main.dir/hal/display/lvgl_port.c.obj.d -o esp-idf/main/CMakeFiles/__idf_main.dir/hal/display/lvgl_port.c.obj -c /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/main/hal/display/lvgl_port.c
/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/main/hal/display/lvgl_port.c:3:10: fatal error: hal_touch.h: No such file or directory
    3 | #include "hal_touch.h"
      |          ^~~~~~~~~~~~~
compilation terminated.
[1846/1879] Building C object esp-idf/main/CMakeFiles/__idf_main.dir/hal/input/hal_button.c.obj
/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/main/hal/input/hal_button.c:76:1: warning: ignoring attribute 'section (".iram1.1")' because it conflicts with previous 'section (".iram1.0")' [-Wattributes]
   76 | {
      | ^
/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/main/hal/input/hal_button.c:42:23: note: previous declaration here
   42 | static void IRAM_ATTR boot_button_isr_handler(void *arg);
      |                       ^~~~~~~~~~~~~~~~~~~~~~~
[1849/1879] Building C object esp-idf/main/CMakeFiles/__idf_main.dir/hal/input/hal_encoder.c.obj
/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/main/hal/input/hal_encoder.c: In function 'encoder_isr_handler':
/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/main/hal/input/hal_encoder.c:73:17: warning: unused variable 'current_count' [-Wunused-variable]
   73 |         int32_t current_count = s_encoder.count;
      |                 ^~~~~~~~~~~~~
[1853/1879] Building C object esp-idf/main/CMakeFiles/__idf_main.dir/hal/audio/hal_audio.c.obj
ninja: build stopped: subcommand failed.
Missing "hal_touch.h" file name found in the following component(s): main(/home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/main/hal/touch/hal_touch.h). Maybe one of the components needs to add the missing header directory to INCLUDE_DIRS of idf_component_register call in CMakeLists.txt. Another possibility may be that the component or its feature is not enabled in the configuration. Use "idf.py menuconfig" to check if the required options are enabled.
ninja failed with exit code 1, output of the command is in the /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/build/log/idf_py_stderr_output_3222318 and /home/ming/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw/build/log/idf_py_stdout_output_3222318

╭─░▒▓     ~/data/Project/ClionProject/ESP32/SenseCAP_Watcher_mimiclaw  on    main ⇡6 ▓▒░                                                                          ░▒▓ 2 ✘  took 47s    idf55    with ming@Archlinux  at 00:54:55   ▓▒░─╮
╰─                                                                                               
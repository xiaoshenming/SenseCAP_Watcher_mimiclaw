#ifndef HAL_CONFIG_H
#define HAL_CONFIG_H

// GPIO 引脚定义
#define GPIO_BOOT_BUTTON        0
#define GPIO_IO_EXP_INT         2
#define GPIO_BAT_ADC            3
#define GPIO_SPI2_SCLK          4
#define GPIO_SPI2_MOSI          5
#define GPIO_SPI2_MISO          6
#define GPIO_QSPI_PCLK          7
#define GPIO_LCD_BACKLIGHT      8
#define GPIO_QSPI_DATA0         9
#define GPIO_I2S_MCLK           10
#define GPIO_I2S_BCLK           11
#define GPIO_I2S_WS             12
#define GPIO_QSPI_DATA3         13
#define GPIO_QSPI_DATA2         14
#define GPIO_I2S_DIN            15
#define GPIO_I2S_DOUT           16
#define GPIO_UART1_TX           17
#define GPIO_UART1_RX           18
#define GPIO_SSCMA_CS           21
#define GPIO_TOUCH_SCL          38
#define GPIO_TOUCH_SDA          39
#define GPIO_RGB_LED            40
#define GPIO_KNOB_A             41
#define GPIO_KNOB_B             42
#define GPIO_LCD_CS             45
#define GPIO_SD_CS              46
#define GPIO_I2C_SDA            47
#define GPIO_I2C_SCL            48

// I2C 设备地址
#define I2C_ADDR_TCA9555        0x21
#define I2C_ADDR_ES8311         0x18
#define I2C_ADDR_ES7243E        0x14

// I2C 总线配置
#define I2C0_SDA                GPIO_I2C_SDA
#define I2C0_SCL                GPIO_I2C_SCL
#define I2C1_SDA                GPIO_TOUCH_SDA
#define I2C1_SCL                GPIO_TOUCH_SCL

// SPI 总线配置
#define SPI2_SCLK               GPIO_SPI2_SCLK
#define SPI2_MOSI               GPIO_SPI2_MOSI
#define SPI2_MISO               GPIO_SPI2_MISO

// 显示屏配置
#define LCD_WIDTH               412
#define LCD_HEIGHT              412

// IO 扩展器引脚定义
#define IO_EXP_P0_PWR_CHRG_DET      0
#define IO_EXP_P0_PWR_STDBY_DET     1
#define IO_EXP_P0_PWR_VBUS_IN_DET   2
#define IO_EXP_P0_KNOB_BTN          3
#define IO_EXP_P0_SD_GPIO_DET       4
#define IO_EXP_P0_TOUCH_GPIO_INT    5
#define IO_EXP_P0_SSCMA_SPI_SYNC    6
#define IO_EXP_P0_SSCMA_CLIENT_RST  7

#define IO_EXP_P1_PWR_SDCARD        0
#define IO_EXP_P1_PWR_LCD           1
#define IO_EXP_P1_PWR_SYSTEM        2
#define IO_EXP_P1_PWR_AI_CHIP       4
#define IO_EXP_P1_PWR_CODEC_PA      5
#define IO_EXP_P1_PWR_BAT_DET       6
#define IO_EXP_P1_PWR_GROVE         7
#define IO_EXP_P1_PWR_BAT_ADC       8

// 电池 ADC 配置
#define BAT_ADC_CHANNEL         ADC_CHANNEL_2
#define BAT_ADC_ATTEN           ADC_ATTEN_DB_2_5
#define BAT_VOLTAGE_DIVIDER     4.1f

#endif // HAL_CONFIG_H

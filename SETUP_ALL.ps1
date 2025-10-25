# MASTER SETUP SCRIPT - Creates ALL project files
# Run this in: C:\Users\matth\pgpemu-display
# Usage: .\SETUP_ALL.ps1

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "PGPemu Display - Complete Setup" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

# Check if we're in the right directory
if (!(Test-Path "components\lvgl") -or !(Test-Path "components\lvgl_esp32_drivers")) {
    Write-Host "ERROR: LVGL components not found!" -ForegroundColor Red
    Write-Host "Please make sure you're in the pgpemu-display directory" -ForegroundColor Red
    Write-Host "and have already cloned LVGL components." -ForegroundColor Red
    exit 1
}

Write-Host "Step 1/5: Creating configuration files..." -ForegroundColor Yellow

# Root CMakeLists.txt
@'
# ESP-IDF CMakeLists.txt for PGPemu with LVGL UI
cmake_minimum_required(VERSION 3.16)

set(EXTRA_COMPONENT_DIRS
    components/lvgl
    components/lvgl_esp32_drivers
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(pgpemu-display)
'@ | Out-File -FilePath "CMakeLists.txt" -Encoding UTF8

# sdkconfig.defaults
@'
CONFIG_IDF_TARGET="esp32c3"
CONFIG_BT_ENABLED=y
CONFIG_BTDM_CTRL_MODE_BLE_ONLY=y
CONFIG_BT_BLUEDROID_ENABLED=y
CONFIG_BT_BLE_ENABLED=y
CONFIG_LV_COLOR_DEPTH_16=y
CONFIG_SPI_MASTER_IN_IRAM=y
CONFIG_SPI_MASTER_ISR_IN_IRAM=y
CONFIG_PARTITION_TABLE_CUSTOM=y
CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
CONFIG_FREERTOS_HZ=1000
CONFIG_FREERTOS_UNICORE=y
CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG=y
'@ | Out-File -FilePath "sdkconfig.defaults" -Encoding UTF8

# partitions.csv
@'
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x9000,  0x6000,
phy_init, data, phy,     0xf000,  0x1000,
factory,  app,  factory, 0x10000, 2M,
nvs_keys, data, nvs_keys,0x210000,0x1000,
settings, data, nvs,     0x211000,0xF000,
'@ | Out-File -FilePath "partitions.csv" -Encoding UTF8

# main/CMakeLists.txt
@'
idf_component_register(
    SRCS 
        "main.c"
        "display_ui.c"
        "cst816_touch.c"
    INCLUDE_DIRS "."
    REQUIRES 
        nvs_flash esp_wifi esp_netif esp_http_server
        bt driver spi_flash esp_lcd lvgl lvgl_esp32_drivers
)
'@ | Out-File -FilePath "main\CMakeLists.txt" -Encoding UTF8

# LVGL config
@'
#ifndef LV_CONF_H
#define LV_CONF_H
#define LV_COLOR_DEPTH 16
#define LV_COLOR_16_SWAP 0
#define LV_MEM_SIZE (48U * 1024U)
#define LV_FONT_MONTSERRAT_12 1
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_INFO
#endif
'@ | Out-File -FilePath "components\lvgl\lv_conf.h" -Encoding UTF8

Write-Host "✓ Configuration files created" -ForegroundColor Green

Write-Host "`nStep 2/5: Creating header files..." -ForegroundColor Yellow

# Header files content in the next section due to length
# cst816_touch.h
$cst816_h = Get-Content -Raw -Path "main\cst816_touch.h" -ErrorAction SilentlyContinue
if (!$cst816_h) {
@'
#ifndef CST816_TOUCH_H
#define CST816_TOUCH_H
#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c.h"
#define CST816_I2C_ADDR 0x15
typedef enum { TOUCH_EVENT_NONE=0, TOUCH_EVENT_DOWN=1, TOUCH_EVENT_UP=2, TOUCH_EVENT_CONTACT=3 } cst816_event_t;
typedef enum { GESTURE_NONE=0x00, GESTURE_SWIPE_UP=0x01, GESTURE_SWIPE_DOWN=0x02, GESTURE_SWIPE_LEFT=0x03, GESTURE_SWIPE_RIGHT=0x04, GESTURE_SINGLE_CLICK=0x05 } cst816_gesture_t;
typedef struct { uint16_t x; uint16_t y; cst816_event_t event; cst816_gesture_t gesture; bool touched; } cst816_touch_data_t;
esp_err_t cst816_init(i2c_port_t i2c_num, gpio_num_t sda_pin, gpio_num_t scl_pin);
esp_err_t cst816_read_touch(cst816_touch_data_t *touch_data);
esp_err_t cst816_get_version(uint8_t *version);
#endif
'@ | Out-File -FilePath "main\cst816_touch.h" -Encoding UTF8
}

# display_ui.h
$display_ui_h = Get-Content -Raw -Path "main\display_ui.h" -ErrorAction SilentlyContinue
if (!$display_ui_h) {
@'
#ifndef DISPLAY_UI_H
#define DISPLAY_UI_H
#include "lvgl.h"
#include <stdbool.h>
void ui_init(void);
void ui_update_status(bool connected, bool catching, bool spinning);
void ui_update_stats(uint32_t pokemon_caught, uint32_t stops_spun, uint32_t battery_percent);
void ui_update_connection_status(const char* status_text);
void ui_show_catch_animation(bool success);
bool ui_get_autocatch_enabled(void);
bool ui_get_autospin_enabled(void);
typedef enum { UI_SCREEN_MAIN, UI_SCREEN_SETTINGS, UI_SCREEN_STATS } ui_screen_t;
void ui_switch_screen(ui_screen_t screen);
#endif
'@ | Out-File -FilePath "main\display_ui.h" -Encoding UTF8
}

Write-Host "✓ Header files created" -ForegroundColor Green

Write-Host "`nStep 3/5: Creating cst816_touch.c..." -ForegroundColor Yellow
Write-Host "Please copy the cst816_touch.c content from the artifact 'create_source_files.ps1'" -ForegroundColor White
Write-Host "Or I can provide a simplified version. Continue? (Y/N)" -ForegroundColor Yellow
$response = Read-Host

Write-Host "`nStep 4/5: Creating display_ui.c..." -ForegroundColor Yellow  
Write-Host "Please copy the display_ui.c content from the artifact 'create_display_ui.ps1'" -ForegroundColor White

Write-Host "`nStep 5/5: Creating main.c..." -ForegroundColor Yellow
Write-Host "Please copy the main.c content from the artifact 'create_main_c.ps1'" -ForegroundColor White

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Setup script completed!" -ForegroundColor Cyan
Write-Host "========================================`n" -ForegroundColor Cyan

Write-Host "MANUAL STEPS REQUIRED:" -ForegroundColor Yellow
Write-Host "You need to copy 3 large source files from the artifacts above:" -ForegroundColor White
Write-Host "1. main\cst816_touch.c - from 'create_source_files.ps1' artifact" -ForegroundColor Gray
Write-Host "2. main\display_ui.c - from 'create_display_ui.ps1' artifact" -ForegroundColor Gray
Write-Host "3. main\main.c - from 'create_main_c.ps1' artifact" -ForegroundColor Gray
Write-Host "`nClick each artifact, copy the content, and paste into the files." -ForegroundColor White

Write-Host "`nThen run:" -ForegroundColor Yellow
Write-Host "  idf.py set-target esp32c3" -ForegroundColor White
Write-Host "  idf.py build" -ForegroundColor White
Write-Host "  idf.py -p COM3 flash monitor`n" -ForegroundColor White
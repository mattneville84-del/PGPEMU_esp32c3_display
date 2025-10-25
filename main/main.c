#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"

#include "lvgl.h"
#include "cst816_touch.h"
#include "display_ui.h"

static const char *TAG = "PGPEMU";

#define LCD_HOST        SPI2_HOST
#define LCD_PIXEL_CLK   40000000
#define LCD_BK_LIGHT    GPIO_NUM_3
#define LCD_PIN_MOSI    GPIO_NUM_7
#define LCD_PIN_CLK     GPIO_NUM_6
#define LCD_PIN_CS      GPIO_NUM_10
#define LCD_PIN_DC      GPIO_NUM_2
#define LCD_PIN_RST     GPIO_NUM_NC

#define TOUCH_I2C_PORT  I2C_NUM_0
#define TOUCH_PIN_SDA   GPIO_NUM_4
#define TOUCH_PIN_SCL   GPIO_NUM_5

#define BUTTON_PIN      GPIO_NUM_9

#define LCD_H_RES       240
#define LCD_V_RES       240

static lv_disp_draw_buf_t disp_buf;
static lv_disp_drv_t disp_drv;
static lv_color_t *buf1;
static lv_color_t *buf2;
static esp_lcd_panel_handle_t panel_handle = NULL;

static uint32_t pokemon_caught = 0;
static uint32_t pokestops_spun = 0;
static bool device_connected = false;

static void lv_tick_task(void *arg) {
    (void) arg;
    lv_tick_inc(10);
}

static bool notify_lvgl_flush_ready(esp_lcd_panel_io_handle_t panel_io, 
                                   esp_lcd_panel_io_event_data_t *edata, 
                                   void *user_ctx) {
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *)user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, 
                             offsetx2 + 1, offsety2 + 1, color_map);
}

static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    cst816_touch_data_t touch_data;
    
    if (cst816_read_touch(&touch_data) == ESP_OK && touch_data.touched) {
        data->point.x = touch_data.x;
        data->point.y = touch_data.y;
        data->state = LV_INDEV_STATE_PR;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

static void init_lcd(void) {
    ESP_LOGI(TAG, "Initialize LCD");
    
    gpio_set_direction(LCD_BK_LIGHT, GPIO_MODE_OUTPUT);
    gpio_set_level(LCD_BK_LIGHT, 1);
    
    spi_bus_config_t buscfg = {
        .mosi_io_num = LCD_PIN_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .sclk_io_num = LCD_PIN_CLK,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));
    
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = LCD_PIN_DC,
        .cs_gpio_num = LCD_PIN_CS,
        .pclk_hz = LCD_PIXEL_CLK,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .on_color_trans_done = notify_lvgl_flush_ready,
        .user_ctx = &disp_drv,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = LCD_PIN_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
    
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    
    ESP_LOGI(TAG, "LCD initialized");
}

static void init_lvgl(void) {
    ESP_LOGI(TAG, "Initialize LVGL");
    
    lv_init();
    
    size_t buf_size = LCD_H_RES * 40 * sizeof(lv_color_t);
    buf1 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    buf2 = heap_caps_malloc(buf_size, MALLOC_CAP_DMA);
    assert(buf1 && buf2);
    
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_H_RES * 40);
    
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);
    
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = lv_tick_task,
        .name = "lv_tick"
    };
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 10 * 1000));
    
    ESP_LOGI(TAG, "LVGL initialized");
}

static void init_touch(void) {
    ESP_LOGI(TAG, "Initialize touch");
    
    ESP_ERROR_CHECK(cst816_init(TOUCH_I2C_PORT, TOUCH_PIN_SDA, TOUCH_PIN_SCL));
    
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);
    
    ESP_LOGI(TAG, "Touch initialized");
}

static void button_task(void *arg) {
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << BUTTON_PIN),
        .pull_down_en = 0,
        .pull_up_en = 1,
    };
    gpio_config(&io_conf);
    
    while (1) {
        if (gpio_get_level(BUTTON_PIN) == 0) {
            ESP_LOGI(TAG, "Button pressed");
            device_connected = !device_connected;
            ui_update_status(device_connected, false, false);
            vTaskDelay(pdMS_TO_TICKS(500));
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void lvgl_task(void *arg) {
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10));
        lv_task_handler();
    }
}

static void pgpemu_task(void *arg) {
    uint32_t last_update = 0;
    
    while (1) {
        if (device_connected && ui_get_autocatch_enabled()) {
            if (xTaskGetTickCount() - last_update > pdMS_TO_TICKS(5000)) {
                pokemon_caught++;
                ui_update_stats(pokemon_caught, pokestops_spun, 95);
                ui_show_catch_animation(true);
                last_update = xTaskGetTickCount();
                ESP_LOGI(TAG, "Caught Pokemon! Total: %lu", (unsigned long)pokemon_caught);
            }
        }
        
        if (device_connected && ui_get_autospin_enabled()) {
            if (xTaskGetTickCount() % pdMS_TO_TICKS(7000) == 0) {
                pokestops_spun++;
                ui_update_stats(pokemon_caught, pokestops_spun, 94);
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "PGPemu Display starting...");
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    init_lcd();
    init_lvgl();
    init_touch();
    
    ui_init();
    ui_update_stats(0, 0, 100);
    ui_update_status(false, false, false);
    
    ESP_LOGI(TAG, "Creating tasks...");
    
    xTaskCreate(lvgl_task, "lvgl", 4096, NULL, 5, NULL);
    xTaskCreate(button_task, "button", 2048, NULL, 5, NULL);
    xTaskCreate(pgpemu_task, "pgpemu", 4096, NULL, 5, NULL);
    
    ESP_LOGI(TAG, "PGPemu Display ready!");
}
'@ | Out-File -FilePath "main\main.c" -Encoding UTF8

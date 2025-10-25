#include "cst816_touch.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include <string.h>

static const char *TAG = "CST816";
static i2c_port_t cst816_i2c_port = I2C_NUM_0;

#define CST816_REG_GESTURE      0x01
#define CST816_REG_FINGER_NUM   0x02
#define CST816_REG_XPOS_H       0x03
#define CST816_REG_XPOS_L       0x04
#define CST816_REG_YPOS_H       0x05
#define CST816_REG_YPOS_L       0x06
#define CST816_REG_VERSION      0xA7
#define CST816_REG_SLEEP        0xE5

static esp_err_t cst816_read_reg(uint8_t reg, uint8_t *data, size_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CST816_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CST816_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(cst816_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

static esp_err_t cst816_write_reg(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (CST816_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(cst816_i2c_port, cmd, pdMS_TO_TICKS(100));
    i2c_cmd_link_delete(cmd);
    return ret;
}

esp_err_t cst816_init(i2c_port_t i2c_num, gpio_num_t sda_pin, gpio_num_t scl_pin) {
    cst816_i2c_port = i2c_num;
    
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = 400000,
    };
    
    esp_err_t ret = i2c_param_config(i2c_num, &conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C param config failed");
        return ret;
    }
    
    ret = i2c_driver_install(i2c_num, conf.mode, 0, 0, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "I2C driver install failed");
        return ret;
    }
    
    uint8_t version;
    ret = cst816_get_version(&version);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "CST816 initialized, version: 0x%02X", version);
    } else {
        ESP_LOGW(TAG, "CST816 version read failed, but continuing...");
    }
    
    return ESP_OK;
}

esp_err_t cst816_read_touch(cst816_touch_data_t *touch_data) {
    if (touch_data == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    uint8_t data[6];
    esp_err_t ret = cst816_read_reg(CST816_REG_GESTURE, data, sizeof(data));
    if (ret != ESP_OK) {
        return ret;
    }
    
    touch_data->gesture = (cst816_gesture_t)data[0];
    uint8_t points = data[1] & 0x0F;
    touch_data->touched = (points > 0);
    
    if (touch_data->touched) {
        touch_data->x = ((data[2] & 0x0F) << 8) | data[3];
        touch_data->y = ((data[4] & 0x0F) << 8) | data[5];
        touch_data->event = (cst816_event_t)((data[2] >> 6) & 0x03);
    } else {
        touch_data->event = TOUCH_EVENT_NONE;
    }
    
    return ESP_OK;
}

esp_err_t cst816_get_version(uint8_t *version) {
    if (version == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    return cst816_read_reg(CST816_REG_VERSION, version, 1);
}

esp_err_t cst816_sleep(void) {
    return cst816_write_reg(CST816_REG_SLEEP, 0x03);
}

esp_err_t cst816_wake(void) {
    uint8_t version;
    return cst816_get_version(&version);
}

#ifndef CST816_TOUCH_H
#define CST816_TOUCH_H
#include <stdbool.h>
#include <stdint.h>
#include "driver/i2c.h"
#define CST816_I2C_ADDR 0x15
typedef enum { TOUCH_EVENT_NONE=0, TOUCH_EVENT_DOWN=1, TOUCH_EVENT_UP=2, TOUCH_EVENT_CONTACT=3 } cst816_event_t;
typedef enum { GESTURE_NONE=0x00, GESTURE_SWIPE_UP=0x01, GESTURE_SWIPE_DOWN=0x02 } cst816_gesture_t;
typedef struct { uint16_t x; uint16_t y; cst816_event_t event; cst816_gesture_t gesture; bool touched; } cst816_touch_data_t;
esp_err_t cst816_init(i2c_port_t i2c_num, gpio_num_t sda_pin, gpio_num_t scl_pin);
esp_err_t cst816_read_touch(cst816_touch_data_t *touch_data);
esp_err_t cst816_get_version(uint8_t *version);
#endif

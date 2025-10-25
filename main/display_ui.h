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

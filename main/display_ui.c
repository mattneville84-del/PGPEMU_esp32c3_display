#include "display_ui.h"
#include "esp_log.h"
#include <stdio.h>

static const char *TAG = "UI";

static lv_obj_t *screen_main;
static lv_obj_t *screen_settings;
static lv_obj_t *screen_stats;
static lv_obj_t *label_status;
static lv_obj_t *label_pokemon_count;
static lv_obj_t *label_stops_count;
static lv_obj_t *label_battery;
static lv_obj_t *btn_autocatch;
static lv_obj_t *btn_autospin;
static lv_obj_t *btn_settings;
static lv_obj_t *arc_progress;

static bool autocatch_enabled = true;
static bool autospin_enabled = true;
static ui_screen_t current_screen = UI_SCREEN_MAIN;

static void create_main_screen(void);
static void create_settings_screen(void);
static void create_stats_screen(void);

static void btn_autocatch_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        autocatch_enabled = !autocatch_enabled;
        lv_obj_t *btn = lv_event_get_target(e);
        lv_obj_t *label = lv_obj_get_child(btn, 0);
        lv_label_set_text(label, autocatch_enabled ? "Catch: ON" : "Catch: OFF");
        
        if (autocatch_enabled) {
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_GREEN), 0);
        } else {
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_RED), 0);
        }
        ESP_LOGI(TAG, "Autocatch %s", autocatch_enabled ? "enabled" : "disabled");
    }
}

static void btn_autospin_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        autospin_enabled = !autospin_enabled;
        lv_obj_t *btn = lv_event_get_target(e);
        lv_obj_t *label = lv_obj_get_child(btn, 0);
        lv_label_set_text(label, autospin_enabled ? "Spin: ON" : "Spin: OFF");
        
        if (autospin_enabled) {
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), 0);
        } else {
            lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_RED), 0);
        }
        ESP_LOGI(TAG, "Autospin %s", autospin_enabled ? "enabled" : "disabled");
    }
}

static void btn_settings_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ui_switch_screen(UI_SCREEN_SETTINGS);
    }
}

static void btn_back_event_cb(lv_event_t *e) {
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CLICKED) {
        ui_switch_screen(UI_SCREEN_MAIN);
    }
}

static void create_main_screen(void) {
    screen_main = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_main, lv_color_hex(0x000000), 0);
    
    label_status = lv_label_create(screen_main);
    lv_label_set_text(label_status, "Disconnected");
    lv_obj_set_style_text_color(label_status, lv_color_hex(0xFF9900), 0);
    lv_obj_align(label_status, LV_ALIGN_TOP_MID, 0, 10);
    
    arc_progress = lv_arc_create(screen_main);
    lv_obj_set_size(arc_progress, 140, 140);
    lv_obj_align(arc_progress, LV_ALIGN_CENTER, 0, -10);
    lv_arc_set_rotation(arc_progress, 270);
    lv_arc_set_bg_angles(arc_progress, 0, 360);
    lv_arc_set_value(arc_progress, 0);
    lv_obj_set_style_arc_color(arc_progress, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
    lv_obj_remove_style(arc_progress, NULL, LV_PART_KNOB);
    
    label_pokemon_count = lv_label_create(screen_main);
    lv_label_set_text(label_pokemon_count, "0");
    lv_obj_set_style_text_font(label_pokemon_count, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(label_pokemon_count, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(label_pokemon_count, LV_ALIGN_CENTER, 0, -20);
    
    label_stops_count = lv_label_create(screen_main);
    lv_label_set_text(label_stops_count, "Stops: 0");
    lv_obj_set_style_text_color(label_stops_count, lv_color_hex(0x00FF00), 0);
    lv_obj_align(label_stops_count, LV_ALIGN_CENTER, 0, 10);
    
    label_battery = lv_label_create(screen_main);
    lv_label_set_text(label_battery, "100%");
    lv_obj_set_style_text_color(label_battery, lv_color_hex(0x00FF00), 0);
    lv_obj_align(label_battery, LV_ALIGN_TOP_RIGHT, -10, 10);
    
    btn_autocatch = lv_btn_create(screen_main);
    lv_obj_set_size(btn_autocatch, 90, 35);
    lv_obj_align(btn_autocatch, LV_ALIGN_BOTTOM_LEFT, 10, -40);
    lv_obj_set_style_bg_color(btn_autocatch, lv_palette_main(LV_PALETTE_GREEN), 0);
    lv_obj_add_event_cb(btn_autocatch, btn_autocatch_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *label_ac = lv_label_create(btn_autocatch);
    lv_label_set_text(label_ac, "Catch: ON");
    lv_obj_center(label_ac);
    
    btn_autospin = lv_btn_create(screen_main);
    lv_obj_set_size(btn_autospin, 90, 35);
    lv_obj_align(btn_autospin, LV_ALIGN_BOTTOM_RIGHT, -10, -40);
    lv_obj_set_style_bg_color(btn_autospin, lv_palette_main(LV_PALETTE_BLUE), 0);
    lv_obj_add_event_cb(btn_autospin, btn_autospin_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *label_as = lv_label_create(btn_autospin);
    lv_label_set_text(label_as, "Spin: ON");
    lv_obj_center(label_as);
    
    btn_settings = lv_btn_create(screen_main);
    lv_obj_set_size(btn_settings, 60, 30);
    lv_obj_align(btn_settings, LV_ALIGN_BOTTOM_MID, 0, -5);
    lv_obj_set_style_bg_color(btn_settings, lv_palette_main(LV_PALETTE_GREY), 0);
    lv_obj_add_event_cb(btn_settings, btn_settings_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *label_set = lv_label_create(btn_settings);
    lv_label_set_text(label_set, "SET");
    lv_obj_center(label_set);
}

static void create_settings_screen(void) {
    screen_settings = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_settings, lv_color_hex(0x001122), 0);
    
    lv_obj_t *title = lv_label_create(screen_settings);
    lv_label_set_text(title, "Settings");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
    
    lv_obj_t *info = lv_label_create(screen_settings);
    lv_label_set_text(info, "WiFi AP Mode:\nHold button\non boot");
    lv_obj_align(info, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_align(info, LV_TEXT_ALIGN_CENTER, 0);
    
    lv_obj_t *btn_back = lv_btn_create(screen_settings);
    lv_obj_set_size(btn_back, 80, 35);
    lv_obj_align(btn_back, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_obj_add_event_cb(btn_back, btn_back_event_cb, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t *label_back = lv_label_create(btn_back);
    lv_label_set_text(label_back, "Back");
    lv_obj_center(label_back);
}

static void create_stats_screen(void) {
    screen_stats = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen_stats, lv_color_hex(0x112200), 0);
    
    lv_obj_t *title = lv_label_create(screen_stats);
    lv_label_set_text(title, "Statistics");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);
}

void ui_init(void) {
    ESP_LOGI(TAG, "Initializing UI");
    create_main_screen();
    create_settings_screen();
    create_stats_screen();
    lv_disp_load_scr(screen_main);
}

void ui_update_status(bool connected, bool catching, bool spinning) {
    if (connected) {
        lv_label_set_text(label_status, "Connected");
        lv_obj_set_style_text_color(label_status, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(label_status, "Disconnected");
        lv_obj_set_style_text_color(label_status, lv_color_hex(0xFF9900), 0);
    }
}

void ui_update_stats(uint32_t pokemon_caught, uint32_t stops_spun, uint32_t battery_percent) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)pokemon_caught);
    lv_label_set_text(label_pokemon_count, buf);
    snprintf(buf, sizeof(buf), "Stops: %lu", (unsigned long)stops_spun);
    lv_label_set_text(label_stops_count, buf);
    snprintf(buf, sizeof(buf), "%lu%%", (unsigned long)battery_percent);
    lv_label_set_text(label_battery, buf);
    lv_arc_set_value(arc_progress, pokemon_caught % 100);
}

void ui_update_connection_status(const char* status_text) {
    lv_label_set_text(label_status, status_text);
}

void ui_show_catch_animation(bool success) {
    if (success) {
        lv_obj_set_style_text_color(label_pokemon_count, lv_color_hex(0xFFFF00), 0);
    }
}

bool ui_get_autocatch_enabled(void) {
    return autocatch_enabled;
}

bool ui_get_autospin_enabled(void) {
    return autospin_enabled;
}

void ui_switch_screen(ui_screen_t screen) {
    current_screen = screen;
    switch (screen) {
        case UI_SCREEN_MAIN:
            lv_disp_load_scr(screen_main);
            break;
        case UI_SCREEN_SETTINGS:
            lv_disp_load_scr(screen_settings);
            break;
        case UI_SCREEN_STATS:
            lv_disp_load_scr(screen_stats);
            break;
    }
}
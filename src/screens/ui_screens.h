/**
 * ui_screens.h — 所有界面管理
 */
#pragma once
#include "lvgl.h"

typedef enum {
    SCREEN_WATCHFACE,
    SCREEN_TIMER,
    SCREEN_LED,
    SCREEN_COUNT
} screen_id_t;

void screens_init(void);
void screens_nav(screen_id_t id);
void screens_update_time(const char *time_str, const char *date_str);
void screens_update_timer(const char *time_str);
void screens_update_wifi_status(bool connected);
void ui_timer_set_callbacks(void (*start)(void), void (*pause)(void), void (*reset)(void));
void ui_led_set_callbacks(void (*on)(void), void (*off)(void), void (*back)(void));

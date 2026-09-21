/**
 * ui_screens.c — 所有界面实现（表盘 / 计时器 / LED 控制）
 *
 * 布局: 240 x 320, 深色主题, LVGL v9
 *
 *   ┌──────────────────────┐
 *   │  [返回]    页面标题   │  标题栏 44px
 *   ├──────────────────────┤
 *   │                      │
 *   │      页面内容区       │  中间区域
 *   │                      │
 *   ├──────────────────────┤
 *   │  ┌────┐ ┌────┐ ┌──┐ │  底部按钮 60px (可选)
 *   └──┴────┴─┴────┴─┴──┴─┘
 */

#include "ui_screens.h"
#include "led.h"
#include "Timer.h"
#include "photo_data.h"

/* ============================================================
 *  全局常量
 * ============================================================ */
#define SCREEN_W  240
#define SCREEN_H  320

#define COLOR_BG        lv_color_hex(0x0D1117)
#define COLOR_HEADER    lv_color_hex(0x161B22)
#define COLOR_TIME      lv_color_hex(0xFFFFFF)
#define COLOR_DATE      lv_color_hex(0x8B949E)
#define COLOR_ACCENT    lv_color_hex(0x58A6FF)
#define COLOR_BTN_BG    lv_color_hex(0x21262D)
#define COLOR_BTN_TEXT  lv_color_hex(0xC9D1D9)
#define COLOR_GREEN     lv_color_hex(0x238636)
#define COLOR_RED       lv_color_hex(0xDA3633)

/* ============================================================
 *  内部状态
 * ============================================================ */
static lv_obj_t *screens[SCREEN_COUNT] = {NULL};

/* 表盘控件 */
static lv_obj_t *wf_label_time    = NULL;
static lv_obj_t *wf_label_date    = NULL;
static lv_obj_t *wf_label_wifi    = NULL;

/* 计时器控件 */
static lv_obj_t *tm_label_time    = NULL;

/* LED 控件 */
static lv_obj_t *led_sw           = NULL;
static lv_obj_t *led_lbl_status   = NULL;

/* 回调 */
static void (*cb_start)(void)  = NULL;
static void (*cb_pause)(void)  = NULL;
static void (*cb_reset)(void)  = NULL;
static void (*cb_led_on)(void)  = NULL;
static void (*cb_led_off)(void) = NULL;

/* ============================================================
 *  通用辅助
 * ============================================================ */

/* 创建屏幕基底 */
static lv_obj_t *create_base(lv_color_t bg_color)
{
    lv_obj_t *scr = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(scr, bg_color, 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(scr, 0, 0);
    lv_obj_set_style_pad_gap(scr, 0, 0);
    return scr;
}

/* 创建标题栏（带返回按钮） */
static lv_obj_t *create_header(lv_obj_t *parent, const char *title,
                                lv_event_cb_t back_cb)
{
    lv_obj_t *header = lv_obj_create(parent);
    lv_obj_set_size(header, SCREEN_W, 44);
    lv_obj_set_style_bg_color(header, COLOR_HEADER, 0);
    lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 0, 0);
    lv_obj_set_flex_flow(header, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(header, LV_FLEX_ALIGN_START,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(header, 8, 0);

    /* 返回按钮 */
    lv_obj_t *btn = lv_button_create(header);
    lv_obj_set_size(btn, 50, 30);
    lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, back_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, LV_SYMBOL_LEFT);
    lv_obj_set_style_text_color(lbl, COLOR_BTN_TEXT, 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_16, 0);
    lv_obj_center(lbl);

    /* 标题文字 */
    lv_obj_t *title_lbl = lv_label_create(header);
    lv_label_set_text(title_lbl, title);
    lv_obj_set_style_text_color(title_lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(title_lbl, &lv_font_montserrat_18, 0);

    return header;
}

/* 创建圆角按钮 */
static lv_obj_t *create_btn(lv_obj_t *parent, const char *text,
                             lv_color_t bg, lv_event_cb_t cb)
{
    lv_obj_t *btn = lv_button_create(parent);
    lv_obj_set_size(btn, 100, 42);
    lv_obj_set_style_bg_color(btn, bg, 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(btn, 12, 0);
    lv_obj_set_style_border_width(btn, 0, 0);
    lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
    lv_obj_set_style_text_color(lbl, lv_color_white(), 0);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);
    return btn;
}

/* ============================================================
 *  回调函数
 * ============================================================ */

/* 导航回调 */
static void nav_watchface(lv_event_t *e) { (void)e; screens_nav(SCREEN_WATCHFACE); }
static void nav_timer(lv_event_t *e)     { (void)e; screens_nav(SCREEN_TIMER); }
static void nav_led(lv_event_t *e)       { (void)e; screens_nav(SCREEN_LED); }

/* 计时器按钮 */
static void on_start(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && cb_start) cb_start();
}
static void on_pause(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && cb_pause) cb_pause();
}
static void on_reset(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED && cb_reset) cb_reset();
}

/* LED 开关 */
static void on_led_switch(lv_event_t *e)
{
    lv_obj_t *sw = (lv_obj_t *)lv_event_get_target(e);
    bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (is_on) {
        if (cb_led_on) cb_led_on();
        lv_label_set_text(led_lbl_status, "LED 已开启");
        lv_obj_set_style_text_color(led_lbl_status, COLOR_GREEN, 0);
    } else {
        if (cb_led_off) cb_led_off();
        lv_label_set_text(led_lbl_status, "LED 已关闭");
        lv_obj_set_style_text_color(led_lbl_status, COLOR_RED, 0);
    }
}

/* ============================================================
 *  表盘界面
 * ============================================================ */
static void create_watchface(void)
{
    lv_obj_t *scr = create_base(COLOR_BG);

    /* --- 状态栏 --- */
    lv_obj_t *bar = lv_obj_create(scr);
    lv_obj_set_size(bar, SCREEN_W, 32);
    lv_obj_set_style_bg_color(bar, COLOR_HEADER, 0);
    lv_obj_set_style_bg_opa(bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(bar, 0, 0);
    lv_obj_set_style_radius(bar, 0, 0);
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_hor(bar, 12, 0);

    lv_obj_t *wifi = lv_label_create(bar);
    lv_label_set_text(wifi, LV_SYMBOL_WIFI);
    lv_obj_set_style_text_color(wifi, COLOR_DATE, 0);
    lv_obj_set_style_text_font(wifi, &lv_font_montserrat_14, 0);
    wf_label_wifi = wifi;

    lv_obj_t *bt = lv_label_create(bar);
    lv_label_set_text(bt, LV_SYMBOL_BLUETOOTH);
    lv_obj_set_style_text_color(bt, COLOR_DATE, 0);
    lv_obj_set_style_text_font(bt, &lv_font_montserrat_14, 0);

    lv_obj_t *batt = lv_label_create(bar);
    lv_label_set_text(batt, LV_SYMBOL_BATTERY_FULL);
    lv_obj_set_style_text_color(batt, COLOR_GREEN, 0);
    lv_obj_set_style_text_font(batt, &lv_font_montserrat_14, 0);

    /* --- 中间时间区 --- */
    lv_obj_t *mid = lv_obj_create(scr);
    lv_obj_set_size(mid, SCREEN_W, 220);
    lv_obj_set_style_bg_opa(mid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(mid, 0, 0);
    lv_obj_set_flex_flow(mid, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(mid, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(mid, 4, 0);

    wf_label_time = lv_label_create(mid);
    lv_label_set_text(wf_label_time, "00:00:00");
    lv_obj_set_style_text_color(wf_label_time, COLOR_TIME, 0);
    lv_obj_set_style_text_font(wf_label_time, &lv_font_montserrat_44, 0);

    wf_label_date = lv_label_create(mid);
    lv_label_set_text(wf_label_date, "等待同步...");
    lv_obj_set_style_text_color(wf_label_date, COLOR_DATE, 0);
    lv_obj_set_style_text_font(wf_label_date, &lv_font_montserrat_16, 0);

    /* --- 底部按钮 --- */
    lv_obj_t *row = lv_obj_create(scr);
    lv_obj_set_size(row, SCREEN_W, 60);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(row, 0, 0);

    create_btn(row, LV_SYMBOL_PLAY " 计时", COLOR_BTN_BG, nav_timer);
    create_btn(row, LV_SYMBOL_SETTINGS " LED", COLOR_BTN_BG, nav_led);

    screens[SCREEN_WATCHFACE] = scr;
}

/* ============================================================
 *  计时器界面
 * ============================================================ */
static void create_timer(void)
{
    lv_obj_t *scr = create_base(COLOR_BG);
    create_header(scr, "计时器", nav_watchface);

    /* 时间显示 */
    lv_obj_t *time_area = lv_obj_create(scr);
    lv_obj_set_size(time_area, SCREEN_W, 180);
    lv_obj_set_style_bg_opa(time_area, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(time_area, 0, 0);
    lv_obj_set_flex_flow(time_area, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(time_area, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    tm_label_time = lv_label_create(time_area);
    lv_label_set_text(tm_label_time, "00:00:00");
    lv_obj_set_style_text_color(tm_label_time, COLOR_ACCENT, 0);
    lv_obj_set_style_text_font(tm_label_time, &lv_font_montserrat_44, 0);

    /* 按钮行 */
    lv_obj_t *row = lv_obj_create(scr);
    lv_obj_set_size(row, SCREEN_W, 60);
    lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_radius(row, 0, 0);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_SPACE_EVENLY,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(row, 0, 0);

    create_btn(row, LV_SYMBOL_PLAY,  COLOR_GREEN, on_start);
    create_btn(row, LV_SYMBOL_PAUSE, COLOR_RED,   on_pause);
    create_btn(row, LV_SYMBOL_REFRESH, COLOR_BTN_BG, on_reset);

    screens[SCREEN_TIMER] = scr;
}

/* ============================================================
 *  LED 控制界面
 * ============================================================ */
static void create_led(void)
{
    lv_obj_t *scr = create_base(COLOR_BG);
    create_header(scr, "LED 控制", nav_watchface);

    lv_obj_t *content = lv_obj_create(scr);
    lv_obj_set_size(content, SCREEN_W, 250);
    lv_obj_set_style_bg_opa(content, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(content, 0, 0);
    lv_obj_set_flex_flow(content, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(content, LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_gap(content, 10, 0);

    /* 照片 */
    lv_obj_t *img = lv_image_create(content);
    lv_image_set_src(img, &photo_img_dsc);
    lv_obj_set_style_pad_top(img, 5, 0);

    /* 状态文字 */
    led_lbl_status = lv_label_create(content);
    lv_label_set_text(led_lbl_status, "LED 已关闭");
    lv_obj_set_style_text_color(led_lbl_status, COLOR_RED, 0);
    lv_obj_set_style_text_font(led_lbl_status, &lv_font_montserrat_20, 0);

    /* 开关 */
    led_sw = lv_switch_create(content);
    lv_obj_set_size(led_sw, 70, 35);
    lv_obj_add_event_cb(led_sw, on_led_switch, LV_EVENT_VALUE_CHANGED, NULL);

    screens[SCREEN_LED] = scr;
}

/* ============================================================
 *  公开接口
 * ============================================================ */

void screens_init(void)
{
    create_watchface();
    create_timer();
    create_led();

    /* 默认显示表盘 */
    lv_screen_load(screens[SCREEN_WATCHFACE]);
}

void screens_nav(screen_id_t id)
{
    if (id < SCREEN_COUNT && screens[id])
        lv_screen_load(screens[id]);
}

void screens_update_time(const char *time_str, const char *date_str)
{
    if (wf_label_time && time_str)
        lv_label_set_text(wf_label_time, time_str);
    if (wf_label_date && date_str)
        lv_label_set_text(wf_label_date, date_str);
}

void screens_update_timer(const char *time_str)
{
    if (tm_label_time && time_str)
        lv_label_set_text(tm_label_time, time_str);
}

void screens_update_wifi_status(bool connected)
{
    if (wf_label_wifi) {
        lv_obj_set_style_text_color(wf_label_wifi,
            connected ? COLOR_GREEN : COLOR_RED, 0);
    }
}

void ui_timer_set_callbacks(void (*start)(void), void (*pause)(void),
                            void (*reset)(void))
{
    cb_start = start;
    cb_pause = pause;
    cb_reset = reset;
}

void ui_led_set_callbacks(void (*on)(void), void (*off)(void),
                          void (*back)(void))
{
    cb_led_on  = on;
    cb_led_off = off;
}

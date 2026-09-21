/**
 * lvgl_setup.cpp — LVGL 显示与触摸驱动 (ESP32-S3 + ILI9341 + XPT2046)
 */

#include "lvgl_setup.h"
#include "lvgl.h"
#include "touch.h"
#include "config.h"
#include "screens/ui_screens.h"
#include <TFT_eSPI.h>

/* ----- 屏幕参数 ----- */
#define SCREEN_W  240
#define SCREEN_H  320

/* ----- 显示缓冲区：单缓冲 40 行 ----- */
#define BUF_LINES  40
static lv_color_t *draw_buf = NULL;

/* ----- 外部函数声明 ----- */
extern TFT_eSPI &getTft();

/* ============================================================
 *  LVGL tick
 * ============================================================ */
static uint32_t my_millis(void) { return (uint32_t)millis(); }

static void tick_cb(lv_timer_t *) { lv_tick_inc(5); }

/* ============================================================
 *  Flush — 推送像素到 TFT_eSPI（含字节序修复）
 * ============================================================ */
static void flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    TFT_eSPI &tft = getTft();
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    /* 交换高低字节，修复 ILI9341 RGB/BGR 颜色反转 */
    uint16_t *p = (uint16_t *)px_map;
    uint32_t total = w * h;
    for (uint32_t i = 0; i < total; i++) {
        p[i] = (p[i] >> 8) | (p[i] << 8);
    }

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushPixels((uint16_t *)px_map, w * h);
    tft.endWrite();

    lv_display_flush_ready(disp);
}

/* ============================================================
 *  Read — 读取 XPT2046 触摸坐标
 * ============================================================ */
static void read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    uint16_t rawX, rawY, z;
    touchRead(rawX, rawY, z);

    if (z > TOUCH_Z_THRESHOLD) {
        data->point.x = map(rawX, 220, 1780, 0, SCREEN_W - 1);
        data->point.y = map(rawY, 0, 1765, 0, SCREEN_H - 1);
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

/* ============================================================
 *  公开接口
 * ============================================================ */

void lvgl_setup_init(void)
{
    lv_init();

    /* Tick */
    lv_tick_set_cb(my_millis);

    /* 显示缓冲区（单缓冲 40 行） */
    size_t buf_bytes = SCREEN_W * BUF_LINES * sizeof(lv_color_t);
    draw_buf = (lv_color_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_SPIRAM);
    if (!draw_buf)
        draw_buf = (lv_color_t *)heap_caps_malloc(buf_bytes, MALLOC_CAP_INTERNAL);

    if (!draw_buf) {
        Serial.println("[LVGL] FATAL: No memory for buffer!");
        return;
    }
    Serial.printf("[LVGL] Buffer: %u bytes OK\n", buf_bytes);

    /* 显示驱动 */
    lv_display_t *disp = lv_display_create(SCREEN_W, SCREEN_H);
    lv_display_set_flush_cb(disp, flush_cb);
    lv_display_set_buffers(disp, draw_buf, NULL, buf_bytes,
                           LV_DISPLAY_RENDER_MODE_PARTIAL);

    /* 触摸输入 */
    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, read_cb);

    /* Tick 定时器 */
    lv_timer_create(tick_cb, 5, NULL);

    Serial.println("[LVGL] Init complete");
}

void lvgl_setup_loop(void)
{
    lv_timer_handler();
}

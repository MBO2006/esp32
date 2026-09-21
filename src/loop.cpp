/**
 * loop.cpp — LVGL 应用逻辑（回调注册 + 定时器更新）
 */
#include "lvgl.h"
#include <WiFi.h>
#include "config.h"
#include "led.h"
#include "Timer.h"
#include "screens/ui_screens.h"
#include <time.h>

/* ===== NTP 实时时钟更新 ===== */

static void clock_update_cb(lv_timer_t *timer)
{
    /* 更新 WiFi 状态指示 */
    screens_update_wifi_status(WiFi.status() == WL_CONNECTED);

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        return;  // NTP 还没同步，跳过
    }

    /* 格式化时间 HH:MM:SS */
    char timeStr[9];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    /* 格式化日期 YYYY-MM-DD  星期X */
    const char *weekdays[] = {"星期日", "星期一", "星期二", "星期三",
                              "星期四", "星期五", "星期六"};
    char dateStr[30];
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d  %s",
             timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
             weekdays[timeinfo.tm_wday]);

    screens_update_time(timeStr, dateStr);
}

/* ===== 计时器 LVGL 定时器回调 ===== */

static void timer_update_cb(lv_timer_t *timer)
{
    AddTime();

    char timeStr[9];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d",
             getTimerHours(), getTimerMinutes(), getTimerSeconds());
    screens_update_timer(timeStr);
}

/* ===== 初始化回调 ===== */

void app_init_callbacks(void)
{
    /* 注册计时器回调 */
    ui_timer_set_callbacks(startTimer, pauseTimer, resetTimer);

    /* 注册 LED 回调 */
    ui_led_set_callbacks(ledon, ledoff, NULL);

    /* 每秒更新计时器显示 */
    lv_timer_create(timer_update_cb, 1000, NULL);

    /* 每秒更新表盘时钟 */
    lv_timer_create(clock_update_cb, 1000, NULL);
}

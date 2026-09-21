/**
 * Time.cpp — 计时器逻辑（纯状态管理，不直接操作显示）
 *
 * 显示更新由 LVGL 定时器回调完成。
 */
#include "Timer.h"
#include "Arduino.h"

/* 计时器状态 */
static bool timerRunning = false;
static unsigned long lastMillis = 0;
static int seconds = 0;
static int minutes = 0;
static int hours = 0;

void startTimer()
{
    timerRunning = true;
    lastMillis = millis();  /* 重置基准时间，避免跳变 */
}

void pauseTimer()
{
    timerRunning = false;
}

void resetTimer()
{
    timerRunning = false;
    seconds = 0;
    minutes = 0;
    hours = 0;
}

void AddTime()
{
    if (!timerRunning)
        return;

    unsigned long currentMillis = millis();
    if (currentMillis - lastMillis >= 1000)
    {
        lastMillis = currentMillis;
        seconds++;
        if (seconds >= 60) {
            seconds = 0;
            minutes++;
            if (minutes >= 60) {
                minutes = 0;
                hours++;
                if (hours >= 24) {
                    hours = 0;
                }
            }
        }
    }
}

int getTimerHours(void)   { return hours; }
int getTimerMinutes(void) { return minutes; }
int getTimerSeconds(void) { return seconds; }

#include "Time.h"
#include "display.h"
#include "Arduino.h"

// 计时器状态
static bool timerRunning = false;
static unsigned long lastMillis = 0;
static int seconds = 0;
static int minutes = 0;
static int hours = 0;

// 开始计时
void startTimer()
{
    timerRunning = true;
}

// 暂停计时（保留当前时间）
void pauseTimer()
{
    timerRunning = false;
}

// 重置计时（清零并暂停）
void resetTimer()
{
    timerRunning = false;
    seconds = 0;
    minutes = 0;
    hours = 0;
    // 立刻清掉屏幕上的时间显示
    TFT_eSPI &tft = getTft();
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.drawString("00:00:00", 10, 60);
}

void AddTime()
{
    // 没在运行就直接返回，什么都不做
    if (!timerRunning)
        return;

    unsigned long currentMillis = millis();
    if (getCurrentPage() == PAGE_FUNCTION)
    {
        if (currentMillis - lastMillis >= 1000)
        {
            lastMillis = currentMillis;
            seconds++;
            if (seconds >= 60)
            {
                seconds = 0;
                minutes++;
                if (minutes >= 60)
                {
                    minutes = 0;
                    hours++;
                    if (hours >= 24)
                    {
                        hours = 0;
                    }
                }
            }

            // 用 getTft() 获取已初始化的屏幕对象
            TFT_eSPI &tft = getTft();
            char timeStr[9];
            snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", hours, minutes, seconds);
            tft.setTextColor(TFT_WHITE, TFT_BLACK);
            tft.setTextFont(2);
            tft.drawString(timeStr, 10, 60);
        }
    }
}

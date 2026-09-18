#include "loop.h"
#include "display.h"
#include "touch.h"
#include "config.h"
#include "Time.h"
#include "led.h"
#include <Arduino.h>

// 页面
Button caidan = {0, 278, 100, 40, "菜单"};
Button page1 = {0, 278, 100, 40, "功能页"};
Button page2 = {120, 278, 100, 40, "页面2"};
Button backBtn = {0, 278, 100, 40, "返回"};

// 计时器
Button functionBtn = {0, 30, 100, 40, "计时器"};
Button jishikaishi = {0, 238, 100, 40, "计时开始"};
Button jishitingzhi = {0, 188, 100, 40, "计时停止"};
Button jishichongzhi = {120, 188, 100, 40, "计时重置"};

// LED灯
Button onled = {0, 30, 100, 40, "开灯"};
Button offled = {120, 30, 100, 40, "关灯"};
Button diandeng = {120, 30, 100, 40, "灯"};

void loop()
{
    uint16_t rawX, rawY, z;
    touchRead(rawX, rawY, z);

    bool touched = (z > TOUCH_Z_THRESHOLD);
    displayTouchInfo(touched);

    if (touched)
    {
        // 触摸校准（实测：开灯按钮中心 rawY=276 ↔ y=50，返回按钮中心 rawY=1655 ↔ y=298）
        uint16_t screenX = map(rawX, 220, 1780, 0, 239);
        uint16_t screenY = map(rawY, 0, 1765, 0, 319);

        Serial.printf("rawX=%d rawY=%d z=%d  →  screenX=%d screenY=%d\n",
                      rawX, rawY, z, screenX, screenY);
        Serial.flush();

        Page currentPage = getCurrentPage();

        if (currentPage == PAGE_HOME)
        {
            if (isInButton(screenX, screenY, page1))
            {
                gotopage1();
            }
            else if (isInButton(screenX, screenY, page2))
            {
                gotopage2();
            }
        }
        if (currentPage == PAGE1)
        {
            if (isInButton(screenX, screenY, backBtn))
            {
                switchToHome();
            }
            else if (isInButton(screenX, screenY, functionBtn))
            {
                switchToFunction();
            }
            else if (isInButton(screenX, screenY, diandeng))
            {
                switchdiandeng();
            }
        }
        if (currentPage == PAGE2)
        {
            if (isInButton(screenX, screenY, backBtn))
            {
                switchToHome();
            }
        }
        if (currentPage == PAGE_FUNCTION)
        {
            if (isInButton(screenX, screenY, backBtn))
            {
                gotopage1();
            }
            else if (isInButton(screenX, screenY, jishikaishi))
            {
                startTimer();
            }
            else if (isInButton(screenX, screenY, jishitingzhi))
            {
                pauseTimer();
            }
            else if (isInButton(screenX, screenY, jishichongzhi))
            {
                resetTimer();
            }
        }
        if (currentPage == PAGE_DIANDENG)
        {
            if (isInButton(screenX, screenY, onled))
            {
                ledon();
            }
            else if (isInButton(screenX, screenY, offled))
            {
                ledoff();
            }
            else if (isInButton(screenX, screenY, backBtn))
            {
                gotopage1();
            }
        }
    }
    AddTime(); // 每帧都调用，内部自己判断 timerRunning
    delay(100);
}

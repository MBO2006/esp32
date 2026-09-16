#include "loop.h"
#include "display.h"
#include "touch.h"
#include "config.h"

// 按钮定义（变量名全小写）
Button caidan = {20, 278, 100, 40, "菜单"};
Button page1 = {20, 278, 100, 40, "页面1"};
Button page2 = {140, 278, 100, 40, "页面2"};
Button backBtn = {20, 278, 100, 40, "返回"};
Button functionBtn = {140, 278, 100, 40, "功能"};

void loop()
{
    uint16_t rawX, rawY, z;
    touchRead(rawX, rawY, z);

    bool touched = (z > TOUCH_Z_THRESHOLD);
    displayTouchInfo(touched);

    if (touched)
    {
        uint16_t screenX = map(rawX, 220, 1780, 0, 239);
        uint16_t screenY = map(rawY, 200, 1830, 0, 319);

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
        }
    }
    delay(100);
}

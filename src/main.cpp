/**
 * main.cpp — 应用主程序
 *
 * 功能：读取 XPT2046 触摸值并在 ILI9341 屏幕上显示
 *
 * 模块依赖：
 *   config.h    — 引脚定义
 *   touch.h/cpp — XPT2046 触摸驱动
 *   display.h/cpp — 显示辅助函数
 */
#include <Arduino.h>
#include "config.h"
#include "touch.h"
#include "display.h"

Button clearBtn = {26, 288, 100, 40, "Clear"}; // 全局按钮

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(2000);
    Serial.println("=== Touch Demo ===");
    Serial.flush();

    displayInit(); // 初始化 ILI9341 显示屏
    touchInit();   // 初始化 XPT2046 触摸引脚

    Serial.println("READY");
    Serial.flush();

    drawButton(clearBtn); // 画按钮
}

void loop()
{
    uint16_t rawX, rawY, z;
    touchRead(rawX, rawY, z); // 读取触摸值

    bool touched = (z > TOUCH_Z_THRESHOLD);
    displayTouchInfo(rawX, rawY, z, touched); // 更新屏幕显示

    if (touched)
    {
        // 原始值 → 屏幕像素（校准范围）
        uint16_t screenX = map(rawX, 220, 1780, 0, 239);
        uint16_t screenY = map(rawY, 200, 1830, 0, 319);

        Serial.printf("rawX=%d rawY=%d z=%d  →  screenX=%d screenY=%d\n",
                      rawX, rawY, z, screenX, screenY);
        Serial.flush();

        if (isInButton(screenX, screenY, clearBtn))
        {
            displayClean();       // 清屏
            drawButton(clearBtn); // 重画按钮
        }
        else
        {
            getTft().fillCircle(screenX, screenY, 3, TFT_GREEN);
        }
    }

    delay(100);
}

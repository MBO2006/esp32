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

void setup()
{
    Serial.begin(SERIAL_BAUD);
    delay(2000);
    Serial.println("=== Touch Demo ===");
    Serial.flush();

    displayInit();  // 初始化 TFT_eSPI
    drawPageHome(); // 画首页（含按钮）

    touchInit(); // 初始化触摸
    loop();      // 进入循环（检测触摸并处理）

    Serial.println("READY");
    Serial.flush();
}

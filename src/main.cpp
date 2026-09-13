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

void setup() {
    Serial.begin(SERIAL_BAUD);
    delay(2000);
    Serial.println("=== Touch Demo ===");
    Serial.flush();

    displayInit();   // 初始化 ILI9341 显示屏
    touchInit();     // 初始化 XPT2046 触摸引脚

    Serial.println("READY");
    Serial.flush();
}

void loop() {
    uint16_t rawX, rawY, z;
    touchRead(rawX, rawY, z);     // 读取触摸值

    bool touched = (z > TOUCH_Z_THRESHOLD);
    displayTouchInfo(rawX, rawY, z, touched);  // 更新屏幕显示

    if (touched) {
        Serial.printf("rawX=%d rawY=%d z=%d\n", rawX, rawY, z);
        Serial.flush();
        getTft().fillCircle(120, 160, 5, TFT_GREEN);
    }

    delay(100);
}

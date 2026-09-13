/*
 * Basic.ino - TFT_ESP32 基础示例
 * 演示画点、画线、矩形、圆形、文字
 */

#include <SPI.h>
#include <TFT_ESP32.h>

// 创建 TFT 对象 (使用默认引脚: CS=5, DC=2, RST=4, BLK=15)
TFT_ESP32 tft;

void setup() {
    Serial.begin(115200);

    tft.begin(40000000);  // 40MHz SPI
    tft.setRotation(1);  // 横屏
    tft.fillScreen(COLOR_BLACK);

    // 画一个红色矩形
    tft.drawRect(10, 10, 100, 80, COLOR_RED);

    // 画一个填充蓝色圆形
    tft.fillCircle(180, 60, 30, COLOR_BLUE);

    // 画一条绿色斜线
    tft.drawLine(0, 0, 239, 319, COLOR_GREEN);

    // 显示文字
    tft.setCursor(20, 100);
    tft.setTextSize(2);
    tft.setTextColor(COLOR_YELLOW);
    tft.println("Hello ESP32!");
    tft.println("TFT Display");
    tft.println("ILI9341 Driver");

    Serial.println("TFT init done!");
}

void loop() {
    // 闪烁背光
    delay(3000);
    tft.setBacklight(false);
    delay(500);
    tft.setBacklight(true);
}

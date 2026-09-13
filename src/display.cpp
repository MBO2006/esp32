/**
 * display.cpp — 显示辅助函数实现
 */
#include "display.h"
#include "config.h"

static TFT_eSPI tft = TFT_eSPI();

TFT_eSPI& getTft() {
    return tft;
}

void displayInit() {
    tft.init();
    tft.setRotation(0);            // 竖屏 240×320
    tft.fillScreen(TFT_BLACK);
    tft.setTextFont(1);             // 8px 小字体
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("Touch Demo", 5, 5);
}

void displayTouchInfo(uint16_t rawX, uint16_t rawY, uint16_t z, bool touched) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString("X:" + String(rawX) + "    ", 5, 20);
    tft.drawString("Y:" + String(rawY) + "    ", 5, 32);
    tft.drawString("Z:" + String(z) + "    ", 5, 44);

    if (touched) {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
        tft.drawString("TOUCHED!", 5, 56);
    } else {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("no touch ", 5, 56);
    }
}

/**
 * display.cpp — 显示辅助函数实现
 */
#include "display.h"
#include "config.h"
#include "touch.h"
#include "chinese_font.h"
#include <JPEGDecoder.h>
#include "img_photo.h"
#include "Timer.h"

#define minimum(a, b) (((a) < (b)) ? (a) : (b))

static TFT_eSPI tft = TFT_eSPI();
static Page currentPage = PAGE_HOME;

TFT_eSPI &getTft() { return tft; }
Page getCurrentPage() { return currentPage; }

void displayInit() { tft.init(); }

void displayTouchInfo(bool touched)
{
    tft.setTextColor(touched ? TFT_GREEN : TFT_RED, TFT_BLACK);
}

// ===== 页面绘制 =====

void drawPageHome()
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    cnDrawString(&tft, 10, 10, "这是菜单");
    cnDrawString(&tft, 10, 40, "你可以选择页面并点击");
    drawButton(page1);
    drawButton(page2);
}

void drawPageFunction()
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.setTextSize(4);
    tft.drawString("00:00:00", 10, 60);
    cnDrawString(&tft, 10, 10, "计时器");
    drawButton(jishikaishi);
    drawButton(jishichongzhi);
    drawButton(jishitingzhi);
    drawButton(backBtn);
}

void drawpage1()
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    drawButton(backBtn);
    drawButton(functionBtn);
    drawButton(diandeng);
}

void drawpage2()
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.drawString("PAGE2", 10, 10);
    drawButton(backBtn);
    drawJpeg(img_photo, sizeof(img_photo), 0, 0);
}

// ===== 页面切换 =====

void switchToHome() { currentPage = PAGE_HOME; drawPageHome(); }
void switchToFunction() { currentPage = PAGE_FUNCTION; drawPageFunction(); }
void gotopage1() { currentPage = PAGE1; drawpage1(); }
void gotopage2() { currentPage = PAGE2; drawpage2(); }

// ===== 点灯 =====

void drawpagediandeng()
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    drawButton(onled);
    drawButton(offled);
    drawButton(backBtn);
}

void switchdiandeng() { currentPage = PAGE_DIANDENG; drawpagediandeng(); }

// ===== 按钮 =====

void drawButton(const Button &btn)
{
    tft.setTextFont(2);
    cnDrawString(&tft, btn.x + 10, btn.y + 5, btn.label);
}

bool isInButton(uint16_t rawX, uint16_t rawY, const Button &btn)
{
    return (rawX >= btn.x && rawX <= (btn.x + btn.w) &&
            rawY >= btn.y && rawY <= (btn.y + btn.h));
}

// ===== JPEG 图片显示 =====

void drawJpeg(const uint8_t *data, uint32_t size, int xpos, int ypos)
{
    JpegDec.decodeArray(data, size);
    uint16_t mcu_w = JpegDec.MCUWidth;
    uint16_t mcu_h = JpegDec.MCUHeight;
    uint32_t img_w = JpegDec.width;
    uint32_t img_h = JpegDec.height;
    uint32_t min_w = minimum(mcu_w, img_w % mcu_w);
    uint32_t min_h = minimum(mcu_h, img_h % mcu_h);
    uint32_t max_x = xpos + img_w;
    uint32_t max_y = ypos + img_h;

    while (JpegDec.read())
    {
        uint16_t *pImg = JpegDec.pImage;
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;
        uint32_t win_w = (mcu_x + mcu_w <= max_x) ? mcu_w : min_w;
        uint32_t win_h = (mcu_y + mcu_h <= max_y) ? mcu_h : min_h;

        if (win_w != mcu_w)
        {
            uint16_t *cImg = pImg + win_w;
            int p = 0;
            for (uint32_t h = 1; h < win_h; h++)
            {
                p += mcu_w;
                for (uint32_t w = 0; w < win_w; w++)
                    *cImg++ = *(pImg + w + p);
            }
        }

        if ((mcu_x + (int)win_w) <= tft.width() && (mcu_y + (int)win_h) <= tft.height())
        {
            tft.startWrite();
            tft.setAddrWindow(mcu_x, mcu_y, win_w, win_h);
            uint32_t n = win_w * win_h;
            while (n--) tft.pushColor(*pImg++);
            tft.endWrite();
        }
        else if ((mcu_y + win_h) >= tft.height())
        {
            JpegDec.abort();
        }
    }
}

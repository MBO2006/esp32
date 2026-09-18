/**
 * display.cpp — 显示辅助函数实现
 */
#include "display.h"
#include "config.h"
#include "touch.h"
#include "chinese_font.h"
#include <JPEGDecoder.h>
#include "img_photo.h"
#include "time.h"

#define minimum(a, b) (((a) < (b)) ? (a) : (b))

static TFT_eSPI tft = TFT_eSPI();
static Page currentPage = PAGE_HOME;

TFT_eSPI &getTft()
{
    return tft;
}

Page getCurrentPage()
{
    return currentPage;
}

void displayInit()
{
    tft.init();
}

void displayTouchInfo(bool touched)
{
    if (touched)
    {
        tft.setTextColor(TFT_GREEN, TFT_BLACK);
    }
    else
    {
        tft.setTextColor(TFT_RED, TFT_BLACK);
    }
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
    tft.drawString("00:00:00", 10, 60); // 初始显示时间
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

// ===== 页面切换（更新状态 + 重绘）=====

void switchToHome()
{
    currentPage = PAGE_HOME;
    drawPageHome();
}

void switchToFunction()
{
    currentPage = PAGE_FUNCTION;
    drawPageFunction();
}

void gotopage1()
{
    currentPage = PAGE1;
    drawpage1();
}

void gotopage2()
{
    currentPage = PAGE2;
    drawpage2();
}

// ===== 计时功能 =====

void jishion()
{
    currentPage = PAGE_FUNCTION;
}

void jishioff()
{
    currentPage = PAGE_FUNCTION;
}

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

void switchdiandeng()
{
    currentPage = PAGE_DIANDENG;
    drawpagediandeng();
}

// ===== 按钮 =====

void drawButton(const Button &btn)
{
    // 按钮不可见，只画文字（触摸区域仍是 btn.x/y/w/h 那个矩形）
    tft.setTextFont(2);
    cnDrawString(&tft, btn.x + 10, btn.y + 5, btn.label);
}

bool isInButton(uint16_t rawX, uint16_t rawY, const Button &btn)
{
    return (rawX >= btn.x && rawX <= (btn.x + btn.w) &&
            rawY >= btn.y && rawY <= (btn.y + btn.h));
}

// ===== JPEG 图片显示 =====

/**
 * 在屏幕指定位置显示 JPEG 图片
 *
 * @param data  JPEG 数据（PROGMEM 数组）
 * @param size  JPEG 数据大小（字节）
 * @param xpos  显示起始 X 坐标
 * @param ypos  显示起始 Y 坐标
 *
 * 工作原理：
 *   JPEG 是压缩格式，不能直接推送到屏幕。
 *   需要先解码为 RGB565 像素，再逐块（MCU，通常16×16）推送到 TFT。
 *   JPEGDecoder 库负责解码，TFT_eSPI 负责显示。
 */
void drawJpeg(const uint8_t *data, uint32_t size, int xpos, int ypos)
{
    // 解码 JPEG 数据
    JpegDec.decodeArray(data, size);

    // 获取图片信息
    uint16_t mcu_w = JpegDec.MCUWidth;  // MCU 块宽度（通常16像素）
    uint16_t mcu_h = JpegDec.MCUHeight; // MCU 块高度（通常16像素）
    uint32_t img_w = JpegDec.width;     // 图片总宽度
    uint32_t img_h = JpegDec.height;    // 图片总高度

    // 边缘 MCU 块可能不满 16×16，计算实际尺寸
    uint32_t min_w = minimum(mcu_w, img_w % mcu_w);
    uint32_t min_h = minimum(mcu_h, img_h % mcu_h);

    uint32_t win_w = mcu_w;
    uint32_t win_h = mcu_h;

    // 图片右下角的绝对坐标（用于裁剪）
    uint32_t max_x = xpos + img_w;
    uint32_t max_y = ypos + img_h;

    // 逐块解码并显示
    while (JpegDec.read())
    {
        uint16_t *pImg = JpegDec.pImage;

        // 当前 MCU 块在屏幕上的位置
        int mcu_x = JpegDec.MCUx * mcu_w + xpos;
        int mcu_y = JpegDec.MCUy * mcu_h + ypos;

        // 右边缘：最后一列 MCU 可能不满宽度
        win_w = (mcu_x + mcu_w <= max_x) ? mcu_w : min_w;
        // 下边缘：最后一行 MCU 可能不满高度
        win_h = (mcu_y + mcu_h <= max_y) ? mcu_h : min_h;

        // 如果 MCU 宽度被裁剪，把像素压缩到连续内存
        if (win_w != mcu_w)
        {
            uint16_t *cImg = pImg + win_w;
            int p = 0;
            for (int h = 1; h < win_h; h++)
            {
                p += mcu_w;
                for (int w = 0; w < win_w; w++)
                {
                    *cImg = *(pImg + w + p);
                    cImg++;
                }
            }
        }

        // 只显示在屏幕范围内的 MCU 块
        if ((mcu_x + win_w) <= tft.width() && (mcu_y + win_h) <= tft.height())
        {
            tft.startWrite();
            tft.setAddrWindow(mcu_x, mcu_y, win_w, win_h);

            // 逐像素推送到 TFT
            uint32_t mcu_pixels = win_w * win_h;
            while (mcu_pixels--)
            {
                tft.pushColor(*pImg++);
            }

            tft.endWrite();
        }
        else if ((mcu_y + win_h) >= tft.height())
        {
            // 图片超出屏幕底部，停止解码
            JpegDec.abort();
        }
    }
}

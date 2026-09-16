/**
 * display.cpp — 显示辅助函数实现
 */
#include "display.h"
#include "config.h"
#include "touch.h"
#include "chinese_font.h"

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
        tft.drawString("TOUCHED!", 140, 20);
    }
    else
    {
        tft.setTextColor(TFT_RED, TFT_BLACK);
        tft.drawString("no touch ", 140, 20);
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
    tft.drawString("FUNCTION PAGE", 10, 10);
    cnDrawString(&tft, 10, 40, "佳佳快学习");
    cnDrawString(&tft, 10, 80, "琼琼别学了");
    drawButton(backBtn);
}

void drawpage1()
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.drawString("PAGE1", 10, 10);
    drawButton(backBtn);
    drawButton(functionBtn);
}

void drawpage2()
{
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextFont(2);
    tft.drawString("PAGE2", 10, 10);
    drawButton(backBtn);
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

// ===== 按钮 =====

void drawButton(const Button &btn)
{
    tft.drawRect(btn.x, btn.y, btn.w, btn.h, TFT_BLACK);
    tft.fillRect(btn.x + 1, btn.y + 1, btn.w - 2, btn.h - 2, TFT_BLACK);
    tft.setTextColor(TFT_BLACK, TFT_BLACK);
    tft.setTextFont(2);
    cnDrawString(&tft, btn.x + 10, btn.y + 8, btn.label);
}

bool isInButton(uint16_t rawX, uint16_t rawY, const Button &btn)
{
    return (rawX >= btn.x && rawX <= (btn.x + btn.w) &&
            rawY >= btn.y && rawY <= (btn.y + btn.h));
}

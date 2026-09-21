/**
 * display.h — 显示辅助函数接口
 */
#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

struct Button
{
    uint16_t x, y, w, h;
    const char *label;
};

enum Page
{
    PAGE_HOME,
    PAGE_FUNCTION,
    PAGE1,
    PAGE2,
    PAGE_DIANDENG,
    PAGE_COUNT
};

extern Button caidan;
extern Button page1;
extern Button page2;
extern Button backBtn;
extern Button functionBtn;
extern Button jishikaishi;
extern Button jishitingzhi;
extern Button jishichongzhi;
extern Button onled;
extern Button offled;
extern Button diandeng;

TFT_eSPI &getTft();
void displayInit();
void displayTouchInfo(bool touched);

void drawPageHome();
void drawpage1();
void drawPageFunction();
void drawpage2();
void drawpagediandeng();

void switchToHome();
void switchToFunction();
void gotopage1();
void gotopage2();
void switchdiandeng();

Page getCurrentPage();

void drawButton(const Button &btn);
bool isInButton(uint16_t rawX, uint16_t rawY, const Button &btn);
void drawJpeg(const uint8_t *data, uint32_t size, int x, int y);

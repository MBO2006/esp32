/**
 * display.h — 显示辅助函数接口
 */
#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>

/**
 * 按钮结构体
 */
struct Button
{
    uint16_t x, y, w, h; // 按钮位置和大小
    const char *label;   // 按钮标签
};

/**
 * 页面枚举
 */
enum Page
{
    PAGE_HOME,
    PAGE_FUNCTION,
    PAGE1,
    PAGE2,
    PAGE_COUNT,
    PAGE_DIANDENG
};

// 按钮声明（实际定义在 loop.cpp）
// 页面
extern Button caidan;
extern Button page1;
extern Button page2;
extern Button backBtn;

// 计时器
extern Button functionBtn;
extern Button jishikaishi;
extern Button jishitingzhi;
extern Button jishichongzhi;

// LED 灯
extern Button onled;
extern Button offled;
extern Button diandeng;

// 显示
TFT_eSPI &getTft();
void displayInit();
void displayTouchInfo(bool touched);

// 页面绘制（每个函数负责清屏 + 画内容 + 画该页的按钮）
void drawPageHome(); // home

void drawpage1(); // page1
void drawPageFunction();

void drawpage2(); // page2

// 页面切换
void switchToHome();
void switchToFunction();

void gotopage1();

void gotopage2();

Page getCurrentPage();

// 计时功能

void jishion();
void jishioff();

// 点灯功能

void drawpagediandeng();
void switchdiandeng();

// 按钮
void drawButton(const Button &btn);
bool isInButton(uint16_t rawX, uint16_t rawY, const Button &btn);

// JPEG 图片显示
void drawJpeg(const uint8_t *data, uint32_t size, int x, int y);
